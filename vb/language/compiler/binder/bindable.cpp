//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to move a module from Declared state to Bindable state.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//
// Globals
//

// !!! Ordering based on ACCESS enumeration order in types.h
static WCHAR *AccessTableStrings[ 7 ] =
{
  L"Compiler", // maps to ACCESS_CompilerControlled (privatescope aka compilercontrolled)
  L"Private",
  L"Protected", // maps to ACCESS_IntersectionProtectedFriend
  L"Protected",
  L"Friend",
  L"Protected Friend",
  L"Public"
};

#if IDE 
bool IsInCompilationThread(Compiler *Compiler)
{
    bool Result = false;

    if (Compiler)
    {
        if (Compiler->IsCompilationInMainThread())
        {
            Result = GetCompilerSharedState()->IsInMainThread();
        }
        else
        {
            Result = GetCompilerSharedState()->IsInBackgroundThread();
        }
    }

    //VSASSERT(Result, "debugging!!!");

    return Result;
}
#endif IDE

void Bindable::ValidateInterfaceInheritance(BCSYM_Interface *Interface)
{
    // Make sure they aren't doing something like: Inherits I1,...I1, etc.
    //
    CheckForDuplicateInherits(Interface);

    for(BCSYM_Implements *BaseImplements = Interface->GetFirstImplements();
        BaseImplements;
        BaseImplements = BaseImplements->GetNext())
    {
        if (BaseImplements->IsBad())
            continue;

        BCSYM_NamedRoot *BaseInterface = BaseImplements->GetRoot()->PNamedRoot();

        if (BaseInterface->IsBad())
            continue;

        bool IsBaseBad = false;

        if (BaseImplements->GetRawRoot()->IsNamedType())
        {
            if (!VerifyBaseNotGenericParam(
                    BaseImplements->GetRawRoot()->PNamedType()))
            {
                return;
            }

            IsBaseBad = !VerifyBaseNotNestedInDerived(
                            BaseImplements->GetRawRoot()->PNamedType());
        }

        if (!BaseInterface->IsInterface())
        {
            // Error already generated for
            if (!BaseInterface->IsGenericParam())
            {
                ReportErrorOnSymbol(
                    ERRID_InheritsFromNonInterface,
                    CurrentErrorLog(Interface),
                    BaseImplements);

                IsBaseBad = true;
            }
        }

        // No more inheritance validation for this base, if base has already been
        // detected to be bad
        //
        if (IsBaseBad)
        {
            BaseImplements->SetIsBad();
            continue;
        }

        // Make sure that we aren't exposing an interface with a restricted type,
        // e.g. a public interface can't inherit from a private interface
        //
        if (!VerifyAccessExposureOfBaseClassOrInterface(
                Interface,
                BaseImplements->GetRawRoot()))
        {
            BaseImplements->SetIsBad();
        }
    }
}


/*****************************************************************************
;CheckForDuplicateInherits

Make sure they aren't inheriting the same interface twice.
Different than a cycles check, i.e. we are looking for Inherits i1, ... i1
*****************************************************************************/
void
Bindable::CheckForDuplicateInherits
(
    BCSYM_Interface *InterfaceBeingChecked
)
{
    BCSYM_Implements *FirstInheritsStatement = InterfaceBeingChecked->GetFirstImplements();

    // Only do this work if there is more than one entry in this list
    //
    if (FirstInheritsStatement != NULL &&
        FirstInheritsStatement->GetNext() != NULL)
    {
        NorlsAllocator ScratchAllocator(NORLSLOC);
        Symbols SymbolFactory(CurrentCompilerInstance(), &ScratchAllocator, NULL);

        // The inherits list is built backwards by declared, e.g. the
        // last Inherits statement as read in the class is the first
        // one encountered in the list
        //
        for (BCSYM_Implements *InheritsStatement = FirstInheritsStatement;
             InheritsStatement != NULL;
             InheritsStatement = InheritsStatement->GetNext())
        {
            // don't process horked inherits statements
            if (InheritsStatement->IsBad() ||
                InheritsStatement->GetRoot()->IsBad())
            {
                continue;
            }

            BCSYM_NamedRoot *CurrentBaseInterface = InheritsStatement->GetRoot()->PNamedRoot();

            for (BCSYM_Implements *PossiblyMatchingExistingBase = FirstInheritsStatement;
                 PossiblyMatchingExistingBase != InheritsStatement;
                 PossiblyMatchingExistingBase = PossiblyMatchingExistingBase->GetNext())
            {
                if (PossiblyMatchingExistingBase->IsBad() ||
                    PossiblyMatchingExistingBase->GetRoot()->IsBad())
                {
                    continue;
                }

                unsigned CompareFlags =
                    PossiblyMatchingExistingBase->GetRoot()->PNamedRoot() == CurrentBaseInterface ?
                        BCSYM::CompareReferencedTypes(
                            PossiblyMatchingExistingBase->GetRoot(),
                            (GenericBinding *)NULL,
                            InheritsStatement->GetRoot(),
                            (GenericBinding *)NULL,
                            &SymbolFactory) :
                        EQ_Shape;

                if ((CompareFlags & EQ_Shape) == 0)
                {
                    // The inherits list is built FILO so mark the one that
                    // comes earlier in the list as being bad since from the
                    // user's point of view, it is the rightmost on the line.

                    if ((CompareFlags & EQ_GenericTypeParams) == 0)
                    {
                        // Duplicate inherits

                        StringBuffer InterfaceName;

                        InheritsStatement->GetRoot()->GetBasicRep(
                            CurrentCompilerInstance(),
                            InterfaceBeingChecked->GetContainingClass(),
                            &InterfaceName,
                            NULL,
                            NULL,
                            FALSE );

                        ReportErrorOnSymbol(
                            ERRID_DuplicateInInherits1,
                            CurrentErrorLog(InterfaceBeingChecked),
                            InheritsStatement,
                            InterfaceName.GetString());
                    }
                    else
                    {
                        // Different inherits, but differ only by type params and could unify

                        StringBuffer InterfaceName;
                        StringBuffer OtherInterfaceName;

                        InheritsStatement->GetRoot()->GetBasicRep(
                            CurrentCompilerInstance(),
                            InterfaceBeingChecked->GetContainingClass(),
                            &InterfaceName,
                            NULL,
                            NULL,
                            FALSE );

                        PossiblyMatchingExistingBase->GetRoot()->GetBasicRep(
                            CurrentCompilerInstance(),
                            InterfaceBeingChecked->GetContainingClass(),
                            &OtherInterfaceName,
                            NULL,
                            NULL,
                            FALSE );

                        ReportErrorOnSymbol(
                            ERRID_InterfaceUnifiesWithInterface2,
                            CurrentErrorLog(InterfaceBeingChecked),
                            InheritsStatement,
                            InterfaceName.GetString(),
                            OtherInterfaceName.GetString());
                    }

                    // Mark it bad so we don't try to do any further semantics
                    // with this symbol.
                    //
                    InheritsStatement->SetIsBad();
                    break;
                }
                else
                {
                    BCSYM *UnifyingInterface1 = NULL;
                    BCSYM *UnifyingInterface2 = NULL;

                    BCSYM *Interface1 = InheritsStatement->GetRoot();
                    BCSYM *Interface2 = PossiblyMatchingExistingBase->GetRoot();

                    if (CanInterfacesOrBasesUnify(
                            Interface1,
                            Interface2,
                            &SymbolFactory,
                            &UnifyingInterface1,
                            &UnifyingInterface2,
                            true))      // Skip direct comparison between the passed in interfaces because the comparison
                                        // has already been done above.
                    {
                        AssertIfNull(UnifyingInterface1);
                        AssertIfNull(UnifyingInterface2);

                        ERRID ErrId = ERRID_None;

                        StringBuffer Name1;
                        StringBuffer Name2;
                        StringBuffer Name3;
                        StringBuffer Name4;

                        if (UnifyingInterface1 == Interface1)
                        {
                            ErrId = ERRID_InterfaceUnifiesWithBase3;

                            Interface1->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name1,
                                NULL,
                                NULL,
                                FALSE );

                            UnifyingInterface2->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name2,
                                NULL,
                                NULL,
                                FALSE );

                            Interface2->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name3,
                                NULL,
                                NULL,
                                FALSE );
                        }
                        else if (UnifyingInterface2 == Interface2)
                        {
                            ErrId = ERRID_BaseUnifiesWithInterfaces3;

                            Interface1->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name1,
                                NULL,
                                NULL,
                                FALSE );

                            UnifyingInterface1->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name2,
                                NULL,
                                NULL,
                                FALSE );

                            Interface2->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name3,
                                NULL,
                                NULL,
                                FALSE );
                        }
                        else
                        {
                            ErrId = ERRID_InterfaceBaseUnifiesWithBase4;

                            Interface1->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name1,
                                NULL,
                                NULL,
                                FALSE );

                            UnifyingInterface1->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name2,
                                NULL,
                                NULL,
                                FALSE );

                            UnifyingInterface2->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name3,
                                NULL,
                                NULL,
                                FALSE );

                            Interface2->GetBasicRep(
                                CurrentCompilerInstance(),
                                InterfaceBeingChecked->GetContainingClass(),
                                &Name4,
                                NULL,
                                NULL,
                                FALSE );
                        }

                        ReportErrorOnSymbol(
                            ErrId,
                            CurrentErrorLog(InterfaceBeingChecked),
                            InheritsStatement,
                            Name1.GetString(),
                            Name2.GetString(),
                            Name3.GetString(),
                            Name4.GetString());

                        // Mark it bad so we don't try to do any further semantics
                        // with this symbol.
                        //
                        InheritsStatement->SetIsBad();
                        break;
                    }
                }
            }
        }
    }
}

bool
Bindable::CanInterfacesOrBasesUnify
(
    BCSYM *Interface1,
    BCSYM *Interface2,
    Symbols *SymbolFactory,
    BCSYM **UnifyingInterface1,
    BCSYM **UnifyingInterface2,
    bool SkipDirectComparison
)
{
    AssertIfNull(Interface1);
    AssertIfFalse(Interface1->IsContainer());
    AssertIfNull(Interface2);
    AssertIfFalse(Interface2->IsContainer());

    AssertIfNull(SymbolFactory);
    AssertIfNull(UnifyingInterface1);
    AssertIfNull(UnifyingInterface2);

    if (Interface1->IsGenericTypeBinding() &&
        CanInterface1UnifyWithInterface2OrItsBases(
            Interface1->PGenericTypeBinding(),
            Interface2,
            SymbolFactory,
            UnifyingInterface1,
            UnifyingInterface2,
            SkipDirectComparison))
    {
        return true;
    }

    BasesIter Interface1Bases(Interface1->PContainer());
    BCSYM_GenericTypeBinding *BaseBinding = NULL;
    while (BCSYM *Base = Interface1Bases.GetNextBase(&BaseBinding))
    {
        if (BaseBinding)
        {
            Base =
                Interface1->IsGenericBinding() ?
                    ReplaceGenericParametersWithArguments(
                        BaseBinding,
                        Interface1->PGenericBinding(),
                        *SymbolFactory) :
                    BaseBinding;
        }

        if (CanInterfacesOrBasesUnify(
                Base,
                Interface2,
                SymbolFactory,
                UnifyingInterface1,
                UnifyingInterface2,
                false))
        {
            return true;
        }
    }

    return false;
}

bool
Bindable::CanInterface1UnifyWithInterface2OrItsBases
(
    BCSYM_GenericTypeBinding *Interface1,
    BCSYM *Interface2,
    Symbols *SymbolFactory,
    BCSYM **UnifyingInterface1,
    BCSYM **UnifyingInterface2,
    bool SkipDirectComparison
)
{
    AssertIfNull(Interface1);
    AssertIfFalse(Interface1->IsContainer());
    AssertIfNull(Interface2);
    AssertIfFalse(Interface2->IsContainer());

    AssertIfNull(SymbolFactory);
    AssertIfNull(UnifyingInterface1);
    AssertIfNull(UnifyingInterface2);

    if (!SkipDirectComparison &&
        CanInterfacesUnify(Interface1, Interface2, SymbolFactory, CurrentCompilerInstance()))
    {
        *UnifyingInterface1 = Interface1;
        *UnifyingInterface2 = Interface2;

        return true;
    }

    BasesIter Interface2Bases(Interface2->PContainer());
    BCSYM_GenericTypeBinding *BaseBinding = NULL;
    while (BCSYM *Base = Interface2Bases.GetNextBase(&BaseBinding))
    {
        if (BaseBinding)
        {
            Base =
                Interface2->IsGenericBinding() ?
                    ReplaceGenericParametersWithArguments(
                        BaseBinding,
                        Interface2->PGenericBinding(),
                        *SymbolFactory) :
                    BaseBinding;
        }

        if (CanInterface1UnifyWithInterface2OrItsBases(
                Interface1,
                Base,
                SymbolFactory,
                UnifyingInterface1,
                UnifyingInterface2,
                false))
        {
            return true;
        }
    }

    return false;
}

bool
Bindable::CanInterfacesUnify
(
    BCSYM *Interface1,
    BCSYM *Interface2,
    Symbols *SymbolFactory,
    Compiler *CompilerInstance
)
{
    AssertIfNull(Interface1);
    AssertIfNull(Interface2);
    AssertIfNull(SymbolFactory);

    unsigned CompareFlags =
        BCSYM::CompareReferencedTypes(
            Interface1,
            (GenericBinding *)NULL,
            Interface2,
            (GenericBinding *)NULL,
            SymbolFactory);

    return ((CompareFlags & (EQ_Shape | EQ_GenericTypeParams)) == EQ_GenericTypeParams);
}

/*****************************************************************************
;DoClassInheritanceValidationNotRequiringOtherBoundBasesInfo

Checks to make sure we don't inherit from non-classes, mustinherit classes,
etc. or restricted classes (can't inherit from System.Array, for instance).
*****************************************************************************/
void
Bindable::DoClassInheritanceValidationNotRequiringOtherBoundBasesInfo
(
    BCSYM_Class *MainClass // [in] class to check
)
{
    VSASSERT(!MainClass->IsPartialType(), "Partial type unexpected!!!");

    BCSYM *RawLogicalBaseClass = NULL;
    BCSYM *LogicalBaseClass = NULL;
    bool ExplicitlySpecifiedBaseFound = false;

    for(BCSYM_Class *Class = MainClass;
        Class;
        Class = Class->GetNextPartialType())
    {
        // Note: Do not use the GetBaseClass, GetCompileBaseClass accessors here because
        // they dig through to the logical base and at this point of time, the logical base
        // is still being determined.
        //
        BCSYM *RawBase = Class->GetRawBase();

        if (!RawBase ||
            Class->IsBaseBad() ||
            Class->IsBaseInvolvedInCycle())
        {
            continue;
        }

        BCSYM *Base = RawBase->DigThroughNamedType();

        if (!Base || Base->IsBad()) continue;

        Compiler *CompilerInstance = CurrentCompilerInstance();
        CompilerHost *CompilerHost = CurrentCompilerHost();

        bool IsBaseBad = false;

        if (Class->GetRawBase() &&
            Class->GetRawBase()->IsNamedType())
        {
            if (!VerifyBaseNotGenericParam(Class->GetRawBase()->PNamedType()))
            {
                continue;
            }

            IsBaseBad = !VerifyBaseNotNestedInDerived(
                            Class->GetRawBase()->PNamedType());
        }

        // There are some types you can't inherit from...
        //
        if (Base == CompilerHost->GetFXSymbolProvider()->GetType(FX::ArrayType) ||
            Base == CompilerHost->GetFXSymbolProvider()->GetType(FX::DelegateType) ||
            Base == CompilerHost->GetFXSymbolProvider()->GetType(FX::MultiCastDelegateType) ||
            Base == CompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType) ||
            Base == CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType))
        {
            ReportErrorAtLocation(
                ERRID_InheritsFromRestrictedType1,
                CurrentErrorLog(Class),
                Class->GetInheritsLocation(),
                Base->PClass()->GetName());

            MarkBaseClassBad(Class);

            continue;
        }

        if (!IsClass(Base))
        {
            ReportErrorAtLocation(
                ERRID_InheritsFromNonClass,
                CurrentErrorLog(Class),
                Class->GetInheritsLocation());

            MarkBaseClassBad(Class);

            continue;
        }

        if (Base->PClass()->IsNotInheritable())
        {
            ReportErrorAtLocation(
                ERRID_InheritsFromCantInherit3,
                CurrentErrorLog(Class),
                Class->GetInheritsLocation(),
                Class->GetName(),
                Base->PNamedRoot()->GetName(),
                StringOfSymbol(CompilerInstance, Base));

            MarkBaseClassBad(Class);

            continue;
        }


        // if bad inheritance has already been detected, then do not proceed with further validation.
        //
        if (IsBaseBad)
        {
            continue;
        }

        if (!Class->IsBaseExplicitlySpecified())
        {
            if (!ExplicitlySpecifiedBaseFound)
            {
                LogicalBaseClass  = Base;
                RawLogicalBaseClass = Class->GetRawBase();
            }
        }
        else if (!ExplicitlySpecifiedBaseFound)
        {
            LogicalBaseClass  = Base;
            RawLogicalBaseClass = Class->GetRawBase();
            ExplicitlySpecifiedBaseFound = true;
        }
        else if (!BCSYM::AreTypesEqual(Base, LogicalBaseClass))
        {
            // Base class '|1' specified for class '|2' cannot be different from the base class '|3' of one of its other partial types.

            ReportErrorAtLocation(
                ERRID_BaseMismatchForPartialClass3,
                CurrentErrorLog(Class),
                Class->GetInheritsLocation(),
                GetQualifiedErrorName(Base),
                Class->GetName(),
                GetQualifiedErrorName(LogicalBaseClass));

            MarkBaseClassBad(Class);

            continue;
        }
    }

    // Change the main type's base class to point to the first explicitly specified good base class.
    //
    if (RawLogicalBaseClass)
    {
        MainClass->SetRawBase(RawLogicalBaseClass);

        VSASSERT(!RawLogicalBaseClass->ChaseToType()->IsGenericBadNamedRoot(), "Bad base unexpected!!!");

        MainClass->SetIsBaseBad(false);
    }
    else
    {
        MarkBaseClassBad(MainClass);
    }
}

/*****************************************************************************
;DoClassInheritanceValidationRequiringOtherBoundBasesInfo

Checks to make sure we don't have public classes inheriting from private classes,
i.e. access exposure. Also check that generic classes don't inherit from attribute
classes.

Note that this needs to be invoked only after inheritance cycle detection for the class
being validated is completed because these checks walk inheritance chains.
*****************************************************************************/
void
Bindable::DoClassInheritanceValidationRequiringOtherBoundBasesInfo
(
    BCSYM_Class *MainClass // [in] class to check
)
{
    VSASSERT(!MainClass->IsPartialType(), "Partial type unexpected!!!");

    if (MainClass->IsBaseBad() ||
        MainClass->IsBaseInvolvedInCycle())
    {
        return;
    }

    // Note: Do not use the GetBaseClass, GetCompileBaseClass accessors here because
    // they dig through to the logical base and at this point of time, the logical base
    // is still being determined.
    //

    BCSYM *RawBase = MainClass->GetRawBase();

    // Non-named types not expected to be bad for the scenarios being validated here.
    //
    if (!RawBase ||
        !RawBase->IsNamedType())
    {
        return;
    }

    BCSYM *Base = RawBase->DigThroughNamedType();

    if (!Base || Base->IsBad())
    {
        return;
    }

    // Generic type cannot be attributes. So they cannot inherit from System.Attribute nor
    // System.Security.Permissions.SecurityAttribute either directly or indirectly
    if (IsGenericOrHasGenericParent(MainClass) &&
        (CurrentCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::AttributeType) &&
         TypeHelpers::IsOrInheritsFrom(
             Base,
             CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::AttributeType)) ||
        (CurrentCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::SecurityAttributeType) &&
         TypeHelpers::IsOrInheritsFrom(
             Base,
             CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::SecurityAttributeType)))))
    {
        // Class that is or nested in a generic type cannot inherit from an attribute class.

        ReportErrorOnSymbol(
            ERRID_GenericClassCannotInheritAttr,
            CurrentErrorLog(RawBase->PNamedType()),
            RawBase);

        MarkBaseClassBad(MainClass);

        return;
    }

    // Verify that we don't have public classes/interfaces inheriting from private ones, etc.
    //
    if (!VerifyAccessExposureOfBaseClassOrInterface(
            MainClass,
            MainClass->GetRawBase()))
    {
        MarkBaseClassBad(MainClass);

        return;
    }
}

// Return true if Base is valid, else false
//
bool
Bindable::VerifyBaseNotNestedInDerived
(
    BCSYM_NamedType *Base
)
{
    if (Base->GetSymbol() && Base->GetSymbol()->IsNamedRoot())
    {
        VSASSERT(!Base->GetSymbol()->IsGenericBadNamedRoot(), "How did a bad root get here ?");

        BCSYM_NamedRoot *BoundBase = Base->GetSymbol()->PNamedRoot();

        if (!BoundBase->IsGenericParam() && IsTypeNestedIn(BoundBase, CurrentContainer()))
        {
            ReportErrorOnSymbol(
                ERRID_NestedBase2,
                CurrentErrorLog(Base),
                Base,
                StringOfSymbol(CurrentCompilerInstance(), CurrentContainer()),
                CurrentContainer()->GetName());

            Base->SetSymbol(Symbols::GetGenericBadNamedRoot());

            return false;
        }
    }

    return true;
}

bool
Bindable::IsTypeNestedIn
(
    BCSYM_NamedRoot *ProbableNestedType,
    BCSYM_Container *ProbableEnclosingContainer
)
{
    if (!ProbableNestedType || !ProbableEnclosingContainer)
    {
        return false;
    }

    for (BCSYM_Container *Parent = ProbableNestedType->GetContainer();
         Parent;
         Parent = Parent->GetContainer())
    {
        if (ProbableEnclosingContainer->IsSameContainer(Parent))
        {
            return true;
        }
    }

    return false;
}

// Return true if Base is valid, else false
//
bool
Bindable::VerifyBaseNotGenericParam
(
    BCSYM_NamedType *Base
)
{
    if (Base->GetSymbol() && Base->GetSymbol()->IsNamedRoot())
    {
        VSASSERT(!Base->GetSymbol()->IsGenericBadNamedRoot(), "How did a bad root get here ?");

        BCSYM_NamedRoot *BoundBase = Base->GetSymbol()->PNamedRoot();

        if (BoundBase->IsGenericParam())
        {
            ReportErrorOnSymbol(
                ERRID_GenericParamBase2,
                CurrentErrorLog(Base),
                Base,
                StringOfSymbol(CurrentCompilerInstance(), CurrentContainer()),
                CurrentContainer()->GetName());

            Base->SetSymbol(Symbols::GetGenericBadNamedRoot());

            return false;
        }
    }

    return true;
}

void
Bindable::MarkBaseClassBad
(
    BCSYM_Class *DerivedClass
)
{
    BCSYM * rawBase = DerivedClass->GetRawBase();
    if (rawBase && rawBase->IsNamedType())
    {
        rawBase->PNamedType()->SetSymbol(Symbols::GetGenericBadNamedRoot());
    }

    DerivedClass->SetIsBaseBad(true);
}


/*****************************************************************************
;DimAsNewSemantics

A variable has been delcared 'as New', e.g. dim x as new y.  Verify that it is
ok to create it, i.e. it has to be an object, there must be an appropriate
constructor that takes no arguments, the class must not be abstract, etc.
*****************************************************************************/
void
Bindable::DimAsNewSemantics
(
    BCSYM *SymbolToCheck,
    ERRID &Error
)
{
    // Get past aliasing
    SymbolToCheck = SymbolToCheck->DigThroughAlias();

    // don't look at symbols that we know have problems already
    //
    if (SymbolToCheck->IsBad())
    {
        return;
    }

    // Semantics checks it out
    //
    Error = CheckConstraintsOnNew(SymbolToCheck);
}


bool
Bindable::IsTypeValidForConstantVariable
(
    BCSYM *Type
)
{
    // Constants must be typed as an intrinsic or Enum type
    // Note: Constants cannot have Structure types
    //
    if (Type->IsObject() ||
        Type->IsIntrinsicType() ||
        Type->IsEnum())
    {
        return true;
    }

    return false;
}


void
Bindable::ResolvePartialTypesForContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfResolvePartialTypes() ==
            Done)
    {
        return;
    }

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();

    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);
    BindableInstanceForContainer->ResolvePartialTypesForContainer();
    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}


// The reason this is split into a separate task from ResolvePartialTypesForContainer
// is that if we made it one single task, then Resolving the partial types for
// any container would complete this task for all the types in the symbol table.
// So what is wrong with that ? - Yes, it is not wrong or a perf. hit, but we want
// to keep Binding a container as lightweight (binding other stuff only when absolutely
// needed) as possible so that other parts of the compiler (mainly UI) could make
// use of this per container model to be more responsive to the user in the future.
//
void
Bindable::ResolvePartialTypesForNestedContainers
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->
            GetStatusOfResolvePartialTypesForNestedContainers() == Done)
    {
        return;
    }

    ResolvePartialTypesForContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();

    VSASSERT( BindableInstanceForContainer->GetStatusOfResolvePartialTypes() ==
                    Done,
                "How can this task start before its previous task is done ?");

    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);
    BindableInstanceForContainer->ResolvePartialTypesForNestedContainers();
    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

// This is needed so that when a name is being bound to a type in this container,
// a type that is in the future going to be marked as duplicate and bad is not bound
// to. In order to enable this scenario, the duplicates need to indentified and
// marked as such before hand. This method is the helper that GetHash invokes
// to guarantee this.
//
void
Bindable::EnsurePartialTypesAreResolved
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container))
    {
        return;
    }

    // If not in the transition from declared to bound, then this
    // is not necessary because then the information is either
    // not needed or already available.
    //
    if (!Container->GetCompilerFile()->GetProject()->IsDeclaredToBoundCompilationInProgress())
    {
        return;
    }

#if IDE 
    if (!IsInCompilationThread(Container->GetCompiler()))
    {
        return;
    }
#endif IDE

    if (Container->GetBindableInstance()->
            GetStatusOfResolvePartialTypesForNestedContainers() != NotStarted)
    {
        return;
    }

    if (Container->GetBindableInstance()->
            GetStatusOfResolvePartialTypes() == InProgress)
    {
        return;
    }

#if DEBUG && IDE

    BCSYM_Container *CurrentContainer;
    if (Container->IsPartialTypeAndHasMainType())
    {
        CurrentContainer = Container->GetMainType();
    }
    else
    {
        CurrentContainer = Container;
    }

    do
    {
        CompilationState CurrentState = CurrentContainer->GetSourceFile()->GetCompState();
        CompilationState CurrentDecompilationState =
            CurrentContainer->GetSourceFile()->GetDecompilationState();

        VSASSERT( CurrentState == CS_Declared &&
                  CurrentState == CurrentDecompilationState,
                        "This should not be invoked in any other state!!!");

        VSASSERT( CurrentContainer->GetSourceFile()->GetProject()->GetDecompilationState() == CS_Declared,
                        "This should not be invoked in any other state!!!");

    }
    while (CurrentContainer = CurrentContainer->GetNextPartialType());

#endif

    ResolvePartialTypesForContainer(Container, IDECompilationCaches);

    ResolvePartialTypesForNestedContainers(Container, IDECompilationCaches);
}

void
Bindable::ResolveFileLevelImports
(
    CompilerFile *File,
    CompilationCaches *IDECompilationCaches
)
{
    if (!File)
    {
        return;
    }

    if (File->IsMetaDataFile())
    {
        return;
    }

    SourceFile *SourceFile = File->PSourceFile();

    if (SourceFile->HaveImportsBeenResolved())
    {
        return;
    }

    VSASSERT(!SourceFile->AreImportsBeingResolved(),
                "unexpected cyclic dependency detected when resolving imports!!!");

    // Indicate in progress
    //
    SourceFile->SetAreImportsBeingResolved(true);

    // Bind all of the "imports" clauses
    ResolveImports(
        SourceFile->GetUnnamedNamespace(),
        SourceFile,
        SourceFile->GetProject(),
        SourceFile->SymbolStorage(),
        SourceFile->GetLineMarkerTable(),
        SourceFile->GetCurrentErrorTable(),
        SourceFile->GetCompiler(),
        IDECompilationCaches);

    // Remove indication of in progress
    //
    SourceFile->SetAreImportsBeingResolved(false);

    SourceFile->SetHaveImportsBeenResolved(true);
}


void
Bindable::ResolveBasesForContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Resolve Bases not needed for MetaData symbols

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfResolveBases() ==
            Done)
    {
        return;
    }

    ResolvePartialTypesForNestedContainers(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfResolveBases(InProgress);

        ResolveBasesForContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfResolveBases(Done);
    }
    else
    {
        //Ensure that File Level imports have been resolved
        for(BCSYM_Container *CurrentType = Container;
            CurrentType;
            CurrentType = CurrentType->GetNextPartialType())
        {
            ResolveFileLevelImports(CurrentType->GetCompilerFile(), IDECompilationCaches);
        }

        BindableInstanceForContainer->ResolveBasesForContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}


void
Bindable::ResolveAllNamedTypesForContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Resolve Named Types not needed for MetaData container

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfResolveNameTypes() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    ResolveBasesForContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfResolveNamedTypes(InProgress);

        ResolveAllNamedTypesForContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfResolveNamedTypes(Done);
    }
    else
    {
        BindableInstanceForContainer->ResolveAllNamedTypesForContainer(TypeResolveNoFlags);
    }
}

void
Bindable::DetectStructMemberCyclesForContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Detecting struct member cycles not needed for MetaData container

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfDetectStructMemberCycles() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    ResolveAllNamedTypesForContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfDetectStructMemberCycles(InProgress);

        DetectStructMemberCyclesForContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfDetectStructMemberCycles(Done);
    }
    else
    {
        BindableInstanceForContainer->DetectStructMemberCyclesForContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

void
Bindable::ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone())
    {
        return;
    }

    // Metadata containers need this too, but the task is then done
    // through a different API BindMetaDataContainer
    //
    if (DefinedInMetaData(Container))
    {
        BindMetaDataContainer(Container, IDECompilationCaches);
        return;
    }

    if (Container->GetBindableInstance()->GetStatusOfResolveShadowingOverloadingAndOverriding() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    ResolveAllNamedTypesForContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfResolveShadowingOverloadingAndOverriding(InProgress);

        ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfResolveShadowingOverloadingAndOverriding(Done);
    }
    else
    {
        // Generic constraints cannot be checked until named types are resolved, because
        // the constraint types need to have been resolved.
        BindableInstanceForContainer->CheckGenericConstraintsForContainer();

        BindableInstanceForContainer->CheckVarianceValidityOfContainer(IDECompilationCaches);

        BindableInstanceForContainer->ResolveShadowingOverloadingAndOverridingForContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

void
Bindable::----AttributesOnAllSymbolsInContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Cracking of attributes not needed for MetaData symbols. Done in metaimport itself
    // when importing symbols. That is needed so early to determine default property,
    // withevents properties.
    //

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOf----AttributesOnAllSymbolsInContainer() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOf----AttributesOnAllSymbolsInContainer(InProgress);

        ----AttributesOnAllSymbolsInContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOf----AttributesOnAllSymbolsInContainer(Done);
    }
    else
    {
        BindableInstanceForContainer->----AttributesOnAllSymbolsInContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

void
Bindable::VerifyAttributesOnAllSymbolsInContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfVerifyAttributesOnAllSymbolsInContainer() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    ----AttributesOnAllSymbolsInContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfVerifyAttributesOnAllSymbolsInContainer(InProgress);

        VerifyAttributesOnAllSymbolsInContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfVerifyAttributesOnAllSymbolsInContainer(Done);
    }
    else
    {
        BindableInstanceForContainer->VerifyAttributesOnAllSymbolsInContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

void
Bindable::ValidateWithEventsVarsAndHookUpHandlersInContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Validate Events not needed for metadata containers

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfValidateWithEventsVarsAndHookUpHandlers() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    VerifyAttributesOnAllSymbolsInContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfValidateWithEventsVarsAndHookUpHandlers(InProgress);

        ValidateWithEventsVarsAndHookUpHandlersInContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfValidateWithEventsVarsAndHookUpHandlers(Done);
    }
    else
    {
        BindableInstanceForContainer->ValidateWithEventsVarsAndHookUpHandlersInContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

void
Bindable::ResolveImplementsInContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Resolve Implements not needed for MetaData symbols

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfResolveImplementsInContainer() ==
            Done)
    {
        return;
    }

    // Make sure the previous step is done
    ValidateWithEventsVarsAndHookUpHandlersInContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfResolveImplementsInContainer(InProgress);

        ResolveImplementsInContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfResolveImplementsInContainer(Done);
    }
    else
    {
        BindableInstanceForContainer->ResolveImplementsInContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}


void
Bindable::VerifyOperatorsInContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfVerifyOperatorsInContainer() ==
            Done)
    {
        return;
    }

    ResolveImplementsInContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfVerifyOperatorsInContainer(InProgress);

        VerifyOperatorsInContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfVerifyOperatorsInContainer(Done);
    }
    else
    {
        BindableInstanceForContainer->VerifyOperatorsInContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}


void
Bindable::GenSyntheticCodeForContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Synthetic code gen not needed for metadata containers

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfGenSyntheticCodeForContainer() ==
            Done)
    {
        return;
    }

    // We can't generate the synthetic code for a member until we've done all of
    // the implements work because the symbol for an implemented event isn't
    // complete until the implements is resolved and the delegate in the interface
    // is found.  We must also wait until operators are verified because synthetic code
    // will not be produced for malformed synthetic division operators.
    //
    VerifyOperatorsInContainer(Container, IDECompilationCaches);

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfGenSyntheticCodeForContainer(InProgress);

        GenSyntheticCodeForContainer(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfGenSyntheticCodeForContainer(Done);
    }
    else
    {
        BindableInstanceForContainer->GenSyntheticCodeForContainer();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}


void
Bindable::ScanContainerForObsoleteUsage
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches,
    bool NeedToCheckObsolsete
)
{
    // Scan for obsolete usage not needed for metadata container

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfScanContainerForObsoleteUsage() ==
            Done)
    {
        return;
    }

    GenSyntheticCodeForContainer(Container, IDECompilationCaches);

    // 


    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfScanContainerForObsoleteUsage(InProgress);

        ScanContainerForObsoleteUsage(Container->GetMainType(), IDECompilationCaches, NeedToCheckObsolsete);

        BindableInstanceForContainer->
            SetStatusOfScanContainerForObsoleteUsage(Done);
    }
    else
    {
        BindableInstanceForContainer->ScanContainerForObsoleteUsage(NeedToCheckObsolsete);
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

void
Bindable::ScanContainerForResultTypeChecks
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // Scan for obsolete usage not needed for metadata container

    // The ordering of the conditions in this "if" are particularly important
    // in order to avoid unnecessarily creating BindableInstances for MetaData
    // Containers
    //
    if (Container->IsBindingDone() ||
        DefinedInMetaData(Container) ||
        Container->GetBindableInstance()->GetStatusOfScanContainerForReturnTypeCheck() ==
            Done)
    {
        return;
    }

    // 


    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();
    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);

    if (Container->IsPartialType() && Container->GetMainType())
    {
        BindableInstanceForContainer->
            SetStatusOfScanContainerForReturnTypeCheck(InProgress);

        ScanContainerForResultTypeChecks(Container->GetMainType(), IDECompilationCaches);

        BindableInstanceForContainer->
            SetStatusOfScanContainerForReturnTypeCheck(Done);
    }
    else
    {
        BindableInstanceForContainer->ScanContainerForResultTypeChecks();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);
}

// This should only be invoked by metaimport!! Have added an assert to verify this.
// All this hoopla is needed for MetaData containers because we need to delete the
// Bindable Instance after the task unlike Source containers where the bindable
// instance is guaranteed to be deleted because the complete binding process is
// alway invoked on them.
//
void
Bindable::BindMetaDataContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    VSASSERT( DefinedInMetaData(Container),
                    "Non-metadata containers unexpected!!");

    // Note - Access the m_phash directly and do not go through GetHash because it
    // would call this function again and thus result in infinite recursion i.e. a
    // stack overflow.
    //

    CompilerIdeLock spLock(Container->GetCompilerFile()->
                            GetCompiler()->GetMetaImportCritSec());

    if (Container->IsBindingDone())
    {
        Container->DeleteBindableInstance();
        return;
    }

    Bindable *BindableInstanceForContainer = Container->GetBindableInstance();

    if (BindableInstanceForContainer->GetStatusOfResolveShadowingOverloadingAndOverriding() ==
            InProgress ||
        BindableInstanceForContainer->GetStatusOfResolveShadowingOverloadingAndOverriding() ==
            Done)
    {
        return;
    }

    BindableInstanceForContainer->SetIDECompilationCaches(IDECompilationCaches);
    BindableInstanceForContainer->ResolveShadowingOverloadingAndOverridingForContainer();

    if (IsClass(Container))
    {
        BindableInstanceForContainer->InheritWellKnowAttributesFromBases();
    }

    BindableInstanceForContainer->SetIDECompilationCaches(NULL);

    // This is the last task for metadata containers
    Container->SetBindingDone(true);

    // Clean up the Bindable Instance
    Container->DeleteBindableInstance();
}

void Bindable::ValidateExtensionAttributeOnMetaDataClass(BCSYM_Class * pClass)
{
    if (! IsBad(pClass))
    {
        BCITER_CHILD iter(pClass);
        BCSYM_NamedRoot * pCur = NULL;
        while (pCur = iter.GetNext())
        {
            if (pCur->IsProc())
            {
                Attribute::VerifyExtensionAttributeUsage(pCur->PProc(), NULL, m_CompilerHost);
            }
        }
    }
}

void
Bindable::BindSourceFile
(
    SourceFile *SourceFile,
    CompilationCaches *IDECompilationCaches
)
{

#if IDE 
    // we need to check this because binding is not done completely until MyClass, etc.
    // have been resolved. So although binding the containers is done, the file's state
    // may not have been promoted to CS_Bound. So we might try to bind the containers
    // of this file again. So inorder to guard against any such issues, we need this check.
    //
    if (SourceFile->IsBindingContainersDone())
    {
        return;
    }
#endif

    // Bind all of the "imports" clauses
    ResolveFileLevelImports(SourceFile, IDECompilationCaches);

    BindContainerAndNestedTypes(SourceFile->GetUnnamedNamespace(), IDECompilationCaches);

#if IDE 
    SourceFile->SetBindingContainersDone();
#endif

}


// This should only be used when Binding SourceFiles i.e. VB source code
// This should not be used when importing metadata because we want to load
// metadata symbols on demand.
//
void
Bindable::BindContainerAndNestedTypes
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    // MetaData containers are not allowed here in order to keep the
    // loading of nested types "on demand".
    //
    VSASSERT( !DefinedInMetaData(Container),
                    "Metadata containers not allowed here!!");

    BindContainer(Container, IDECompilationCaches);

    // iterate Container for all nested types and do bindable for them too
    //
    for(BCSYM_Container *NestedType = Container->GetNestedTypes();
        NestedType;
        NestedType = NestedType->GetNextNestedType())
    {
        BindContainerAndNestedTypes(NestedType, IDECompilationCaches);
    }

    // Note: Normally the bindable instance of the container is deleted at the end of BindContainer(Container)
    // In some cases the instance can wrongly be reassigned to a new bindable during the binding of the nested
    // containers. See bug VSWhidbey 344045 for an example.
    if (Container->GetBindableInstanceIfAlreadyExists() &&
        !IsMyGroupCollection(Container))
    {
        // See bug VSWhidbey 344045 for more details.
        VSFAIL("Bindable instance unexpected!!!");
        Container->DeleteBindableInstance();
    }
}

void
Bindable::BindContainer
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    VSASSERT( Container, "How can we bind a NULL container ?");

    // Some containers might already be bound due to the other
    // parts of the compiler like codemodel preemptively binding
    // containers which their clients are interested in.
    //
    if (Container->IsBindingDone())
    {
        return;
    }

    // Ensure project level imports and app objects are resolved because lookup during
    // the binding of this container might involve looking in them.
    //
    Container->GetContainingProject()->ResolveProjectLevelImports(IDECompilationCaches);

    ResolvePartialTypesForContainer(Container, IDECompilationCaches);

    ResolvePartialTypesForNestedContainers(Container, IDECompilationCaches);

    // Resolve Bases
    //
    ResolveBasesForContainer(Container, IDECompilationCaches);

    // Resolve NamedTypes
    // Resolve all the namedtypes for this container
    // Note: We cannot do ResolveBase and ResolveNamedTypes in one step because
    // when we are in the middle of resolving a base, ResolveNamedTypes for
    // another class (say a parent class) might start and fail for some named
    // type when trying to bind against a member in the base that we are currently
    // resolving.Need an eg. ?
    //
    ResolveAllNamedTypesForContainer(Container, IDECompilationCaches);

    // Detect structure member cycles
    // Eg:
    //    Structure S1
    //      Dim x as S1     'Cycle here
    //    End Structure
    //
    DetectStructMemberCyclesForContainer(Container, IDECompilationCaches);

    // Resolve Shadowing, Overloading And Overriding for container
    // Besides these, the following tasks are also done here
    // - Evaluate Constant expressions
    // - Validate withevent variables in handles clauses and synthetsize any
    //      properties needed to handle WithEventVariableInBase.Event events.
    // - Validate generic constraints
    // - Validate generic variance
    // (Generics are check here because they depend on generic-constraint-names
    // having been resolve).
    //
    ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(Container, IDECompilationCaches);

    // - ---- Attributes on All Symbols in the container.
    // - Also Inherit well known inheritable attributes from bases
    // - Determine if this container can be used as a valid attribute
    //
    ----AttributesOnAllSymbolsInContainer(Container, IDECompilationCaches);

    // Verify attribute usage
    //
    VerifyAttributesOnAllSymbolsInContainer(Container, IDECompilationCaches);

    // - Validate all WithEvent variables in container
    // - Hookup handlers specified in the handles clauses in the container
    //      to the appropriate events.
    //
    ValidateWithEventsVarsAndHookUpHandlersInContainer(Container, IDECompilationCaches);

    // Resolve all the implements clauses and connect the implementing
    // members to the implemented members.
    //
    ResolveImplementsInContainer(Container, IDECompilationCaches);

    // Verify user-defined operators are correctly formed.
    //
    VerifyOperatorsInContainer(Container, IDECompilationCaches);

    // We can't generate the synthetic code for a member until we've done
    // ResolveImplements because the delegate for an implementing event isn't
    // known until the implements is resolved and the delegate of the
    // implemented event in the interface is found.
    // This also does Access Exposure validation. We cannot verify the Access
    // Exposure for the same reason that the event's delegate might not be known.
    //
    GenSyntheticCodeForContainer(Container, IDECompilationCaches);

    // Scan for use of Obsolete types and members
    //
    ScanContainerForObsoleteUsage(Container, 
                                  IDECompilationCaches,
                                  !ObsoleteChecker::IsObsoleteOrHasObsoleteContainer(Container));

    // Some ResultType checks have been cached until after ResolveProjectLevelImports
    //
    ScanContainerForResultTypeChecks(Container, IDECompilationCaches);

    // Mark the container as Binding done and delete the Bindable instance associated
    // with the Container
    //
    // But if the container has the MyCollection Attribute in which case properties
    // might need to be injected into it later, then we will delay the binding
    // completion.
    //
    if (!IsMyGroupCollection(Container))
    {
        Container->SetBindingDone(true);
        Container->DeleteBindableInstance();
    }
}

void
Bindable::ResolveBasesForContainer()
{
    //This function resolves bases for the
    //- Container's Parents
    //- Container
    //- and for the Container's bases
    //
    //This function processes non-classes and non-interfaces too. Why ?
    //Because if the container is a structure which itself is inside another
    //class, then we still have to resolve bases for the structure's parent class.
    //This is need so that ResolveNamedTypes on this container can bind to types
    //in the Parent's bases too. The only exception being namespaces and modules
    //which cannot be nested in classes or interfaces.

    BCSYM_Container *Container = CurrentContainer();

    if (Container->IsNamespace() ||
        IsStdModule(Container))
    {
        SetStatusOfResolveBases(Done);
        return;
    }

    // If this Class/Interface has already been processed for this, return
    if (GetStatusOfResolveBases() == Done)
    {
        return;
    }

    if (GetStatusOfResolveBases() == InProgress)
    {
        // Possible cycle - flag in order to do a thorough cycle check after base
        // resolution is done
        //
        SetPossibleInheritanceCycleDetected(true);

        // Check if any parent cycle ?
        //
        BCSYM *CurrentBase =
            GetCurrentBase() ?
                GetCurrentBase()->GetSymbol() :
                NULL;
        BCSYM_Container *CurrentBoundBase = NULL;

        while (CurrentBase)
        {
            if (!CurrentBase->IsContainer())
            {
                CurrentBoundBase = NULL;
                break;
            }

            CurrentBoundBase = CurrentBase->PContainer();

            if (CurrentBoundBase->IsSameContainer(Container))
            {
                CurrentBoundBase = NULL;
                break;
            }

            if (GetStatusOfResolveBases(CurrentBoundBase) == NotStarted)
            {
                break;
            }

            CurrentBase = CurrentBoundBase->GetBindableInstance()->GetCurrentBase();
        }

        if (CurrentBoundBase)
        {
            // this implies that this base's parent's base is being resolved
            // this will then error out at the below parent check.
            //
            ResolveBasesForContainer(CurrentBoundBase, m_CompilationCaches);
        }

        return;
    }

    // Resolve bases for parent because this affects the resolving of named types in the
    // current container. Walk up the parent hierarchy because if the parent is a structure
    // which itself is inside another class, then we still have to resolve base for the
    // structure's parent class.
    //
    BCSYM_Container *Parent = Container->GetContainer();
    if (Parent)
    {
        if (GetStatusOfResolveBases(Parent) == InProgress)
        {
            // BaseOfParent could be NULL if this is triggered when resolving the namedtype
            // that represents the parent's base
            //
            BCSYM_NamedType *BaseOfParent =
                Parent->GetBindableInstance()->GetCurrentBase();

            if (BaseOfParent &&
                BaseOfParent->GetSymbol() &&
                !BaseOfParent->GetSymbol()->IsBad())
            {
                ReportErrorOnSymbol(
                    ERRID_CircularBaseDependencies4,
                    CurrentErrorLog(BaseOfParent),
                    BaseOfParent,
                    StringOfSymbol(CurrentCompilerInstance(), Parent),
                    Parent->GetName(),
                    StringOfSymbol(CurrentCompilerInstance(), Container),
                    Container->GetName());

                BaseOfParent->SetSymbol(Symbols::GetGenericBadNamedRoot());
            }

            // This is so that cycle detection is forced on the current container after
            // it bases are resolved later. See bug VSWhidbey 187738.
            //
            SetPossibleInheritanceCycleDetected(true);

            return;
        }
        else
        {
            ResolveBasesForContainer(Parent, m_CompilationCaches);

            // This class's bases could itself get triggered off through its parents,
            // after we break the circular parent - nested type dependencies.
            // If this Class/Interface has already been processed for this, return
            // else continue on and complete the task now.
            //
            if (GetStatusOfResolveBases() == Done)
            {
                return;
            }
        }
    }

    // Mark the Class/Interface indicating that work is in progress to resolve its bases
    //
    SetStatusOfResolveBases(InProgress);

    if (IsClass(Container))
    {
        ResolveBaseForClass(Container->PClass());
    }
    else if (IsInterface(Container))
    {
        ResolveBasesForInterface(Container->PInterface());
    }

    SetStatusOfResolveBases(Done);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIANCE
//
// CheckVarianceValidityOfContainer - this is the only entrypoint for validity-checking of variance.
// It is called for each container (including nested containers) in each source file.
// If it is called for each container in each sourcefile of a program, then we guarantee that either
// (1) the program had something already marked IsBad(), or (2) we produce a variance-validity error
// message, or (3) the program is variance-valid.
//
// The variance-validity-checker never marks anything as IsBad().
//
// Variance validity is defined in $2 of the VB Variance Specification. Each of the validity-checking
// functions cites (and quotes verbatim) the portion of the spec that it implements.
//
// One thing to note: we only ever check variance-validity of bindable members
// (i.e. ones written by the user); we don't check compiler-generated members.
//////////////////////////////////////////////////////////////////////////////////////////////////////


void
Bindable::CheckVarianceValidityOfContainer
(
    CompilationCaches *IDECompilationCaches
)
{

    BCSYM_Container *Container = this->CurrentContainer();
    VSASSERT(Container, "unexpected NULL CurrentContainer");
    
    // We only check variance on the maintype, not on partials.
    if (Container->IsPartialType())
    {
        VSASSERT(Container->IsPartialTypeAndHasMainType(), "unexpected partial type with no maintype");
        return;
    }

    // Variance spec $2:
    // An enum is valid. A namespace and a module are valid if and
    // only if every nested type is valid.

    if (IsNamespace(Container))
    {
    }
    else if (IsStdModule(Container))
    {
    }
    else if (IsInterface(Container))
    {
        CheckVarianceValidityOfInterface(Container->PInterface());
        CheckVarianceAmbiguity(Container, IDECompilationCaches);
    }
    else if (Container->IsDelegate())
    {
        CheckVarianceValidityOfDelegate(Container->PClass());
    }
    else if (IsClass(Container) || Container->IsStruct() || Container->IsEnum())
    {
        CheckVarianceValidityOfClassOrStructOrEnum(Container->PClass());
        CheckVarianceAmbiguity(Container, IDECompilationCaches);
    }
    else
    {
        VSFAIL("Unexpected container type when checking for variance-validity");
    }
}


void
Bindable::CheckVarianceValidityOfClassOrStructOrEnum(BCSYM_Container *Container)
{
    VSASSERT(Container, "unexpected NULL class/struct/enum");
    VSASSERT(IsClass(Container) || Container->IsStruct() || Container->IsEnum(), "expected a class/struct container");
    if (Container->IsBad())
    {
        return;
    }

    // Variance spec $2:
    // A class/struct is valid if and only if
    //  * its own generic parameters are invariant, and
    //  * it has no generic methods with variant generic parameters, and
    //  * every nested type is valid.
    //
    // The test that nested types are valid isn't needed, since CheckVarianceValidityOfContainer
    // will anyways be called on them all. (and for any invalid nested type, we report the
    // error on it, rather than on this container.)
    //
    // On a separate topic, here we also implement a check that this class/struct/enum isn't
    // contained (even nested) inside a variant interface. See the comments in
    // CheckVarianceValidityOfInterface for why.
    //

    // 1. own generic parameters are invariant.
    // NB. Container->GetFirstGenericParam() will return null if it's not a generic container
    for (GenericParameter *gParam=Container->GetFirstGenericParam(); gParam; gParam=gParam->GetNextParam()) 
    {
        VSASSERT(gParam, "unexpected NULL generic param");
        if (!gParam->IsBad() && gParam->GetVariance()!=Variance_None)
        {
            // Only interface and delegate types can be specified as variant
            ReportErrorOnSymbol(ERRID_VarianceDisallowedHere, CurrentErrorLog(gParam), gParam);
            
            // we'll set the variance to what the user presumably will correct it to be, so that
            // future errors make more sense
            gParam->SetVariance(Variance_None);
        }
    }

    // 2. no generic methods with variant generic parameters
    // Has already been dealt with in ValidateGenericParameters()

    // 3. isn't contained (not even nested) in a variant interface
    for (BCSYM_Container *parent = Container->GetContainingClassOrInterface(); parent; parent=parent->GetContainingClassOrInterface())
    {
        if (TypeHelpers::HasVariance(parent))
        {
            // This type declaration cannot go inside a variant Interface
            ReportErrorOnSymbol(ERRID_VarianceInterfaceNesting, CurrentErrorLog(Container), Container);
            break;

            // Incidentally, all parent containers are validated before their child containers.
            // So if we had "Class C(Of Out T) : Structure S : End Structure : End Class", then the
            // outer class would have been validated first, and there'd have been an error on the
            // variance of T, and we'd have silently changed it to "Variance_None". That means that
            // we won't now report an error for "S" being inside a variant container. Good. We shouldn't.

            // NB. We won't mark this container as bad. That's because the user will likely fix the
            // problem by moving the container into a place where it's allowed. So we can still
            // make useful reports on the rest of the code which references this container.
        }
    }
}


void
Bindable::CheckVarianceValidityOfDelegate(BCSYM_Class *Delegate)
{
    VSASSERT(Delegate, "unexpected NULL delegate");
    if (Delegate->IsBad())
    {
        return;
    }

    // Variance spec $2
    // A delegate "Delegate Function/Sub Foo(Of T1, ... Tn)Signature" is valid if and only if
    //  * the signature is valid.
    //
    // That delegate becomes "Class Foo(Of T1, ... Tn) : Function Invoke(...) As ... : End Class
    // So we just need to pick up the "Invoke" method and check that it's valid.
    // NB. that delegates can have variance in their generic params, and hence so can e.g. "Class Foo(Of Out T1)"
    // This is the only place in the CLI where a class can have variant generic params.

    // Note: delegates that are synthesized from events are already dealt with in CheckVarianceValidityOfInterface,
    // so we won't examine them again here:
    if (Delegate->CreatedByEventDecl()!=NULL)
    {
        return;
    }

    Declaration *Invoke = GetInvokeFromDelegate(Delegate, this->CurrentCompilerInstance());
    VSASSERT(Invoke, "unexpected null invoke method in delegate");
    if (Invoke->IsBad())
    {
        return;
    }
    VSASSERT(Invoke->IsMethodDecl(), "unexpected non-decl invoke in delegate");
    CheckVarianceValidityOfSignature(Invoke->PMethodDecl());
}


void
Bindable::CheckVarianceValidityOfInterface(BCSYM_Interface *Interface)
{
    VSASSERT(Interface, "unexpected NULL interface");
    if (Interface->IsBad())
    {
        return;
    }

    // Variance spec $2:
    // An interface I is valid if and only if
    //  * every method signature in I is valid and has no variant generic parameters, and
    //  * every property in I is valid, and
    //  * every event in I is valid, and
    //  * every immediate base interface type of I is valid covariantly, and
    //  * the interface is either invariant or it lacks nested classes and structs, and
    //  * every nested type is valid.
    //
    // A property "Property Foo as T" is valid if and only if either
    //  * The property is read-only and T is valid covariantly, or
    //  * The property is write-only and T is valid invariantly, or
    //  * The property is readable and writable and T is invariant.
    //
    // An event "Event e as D" is valid if and only if
    //  * the delegate type D is valid contravariantly
    //
    // An event "Event e(Signature)" is valid if and only if
    //  * it is not contained (not even nested) in a variant interface
    //
    // The test that nested types are valid isn't needed, since CheckVarianceValidityOfContainer
    // will anyways be called on them all. (and for any invalid nested type, we report the
    // error on it, rather than on this container.)
    //
    // The check that an interface lacks nested classes and structs is done inside
    // CheckVarianceValidityOfClassOrStruct. Why? Because we have to look for indirectly
    // nested classes/structs, not just immediate ones. And it seemed nicer for classes/structs
    // to look UP for variant containers, rather than for interfaces to look DOWN for class/struct contents.


    // 1. every method signature in I is valid, and properties are valid
    BCITER_CHILD Members(Interface);
    while (BCSYM_NamedRoot *Member = Members.GetNext())
    {
        if (Member->IsBad())
        {
            continue;
        }
        else if (Member->IsMethodDecl() && !Member->IsEventDecl()) // nb. a MethodImpl is also a MethodDecl.
        {
            CheckVarianceValidityOfSignature(Member->PMethodDecl());
        }
        else if (Member->IsProperty())
        {
            BCSYM_Property *Property = Member->PProperty();
            BCSYM *Type = Property->GetType();
            VSASSERT(Type, "unexpected NULL property type");
            if (Type->IsBad())
            {
                continue;
            }

            // Gettable: requires covariance. Settable: requires contravariance. Gettable and settable: requires invariance.
            Variance_Kind RequiredVariance;
            VarianceContextEnum Context;

            if (Property->IsReadOnly())
            {
                RequiredVariance = Variance_Out;
                Context = VarianceContextReadOnlyProperty;
            }
            else if (Property->IsWriteOnly())
            {
                RequiredVariance = Variance_In;
                Context = VarianceContextWriteOnlyProperty;
            }
            else
            {
                RequiredVariance = Variance_None;
                Context = VarianceContextProperty;
            }

            CheckThatTypeSatisfiesVariance(Type, RequiredVariance, Interface, Context, Property->GetRawType(), CurrentErrorLog(Property));

            // A property might be declared with extra parameters, so we have to check that these are variance-valid.
            // If it's readonly then these extra params will be found in the getter; if it's writeonly then they'll
            // be found in the setter; if it's readable and writable then they'll be found in both. But we'll
            // only check one of these accessors, since we don't want such parameter errors to be reported twice.
            // Note: the CheckVarianceValidityOfSignature() function will ignore the property-type when asked to check
            // accessors, because the property-type shouldn't really be considered part of the property signature proper.
            // That's good, because we've already reported any errors on the property-type above.
            BCSYM_Proc *Accessor = Property->IsWriteOnly() ? Property->SetProperty() : Property->GetProperty();
            VSASSERT(Accessor!=NULL, "unexpected property without gettor or settor");
            CheckVarianceValidityOfSignature(Accessor);
        }
        else if (Member->IsEventDecl())
        {
            BCSYM_EventDecl *Event = Member->PEventDecl();
            BCSYM *Delegate = Event->GetDelegate();
            if (!Delegate || !Delegate->IsDelegate() || Delegate->IsBad())
            {
                continue;
            }
            if (Delegate->PClass()->CreatedByEventDecl() == Event)
            {
                // Some events are declared as "Event e1(ByVal x as T), with a synthesized delegate type.
                // These aren't allowed inside variant interfaces.
                BCSYM_Container *OutermostVariantInterface = NULL;
                for (BCSYM_Container *parent = Event->GetContainingClassOrInterface();
                     parent;
                     parent = parent->GetContainingClassOrInterface())
                {
                    if (TypeHelpers::HasVariance(parent))
                    {
                        OutermostVariantInterface = parent;
                    }
                }

                if (OutermostVariantInterface!=NULL)
                {
                    // "Event definitions with parameters are not allowed in an interface such as '|1' that has
                    // 'In' or 'Out' type parameters. Consider declaring the event by using a delegate type which
                    // is not defined within '|1'. For example, 'Event |2 As Action(Of ...)'."
                    ERRID ErrId = ERRID_VariancePreventsSynthesizedEvents2;
                    ReportErrorOnSymbol(ErrId, CurrentErrorLog(Event), Event,  
                                        GetQualifiedErrorName(OutermostVariantInterface),
                                        Event->GetName());
                    // And we mark the event as bad because the only way to fix the event is by redoing it entirely.
                    Event->SetIsBad();
                }
            }
            else
            {
                // Other events are declared as "Event e1 as SomeDelegateType"
                // If there are errors in the variance of SomeDelegateType, we report them on it.
                CheckThatTypeSatisfiesVariance(Delegate, Variance_In, Interface, VarianceContextComplex, Event->GetRawDelegate(), CurrentErrorLog(Member));
            }
        }
    }

    // 2. no generic methods with variant generic parameters
    // Has already been dealt with in ValidateGenericParameters

    // 3. every immediate base interface is valid covariantly.
    // Actually, the only type that's invalid covariantly and allowable as a base interface,
    // is a generic instantiation X(T1,...) where we've instantiated it wrongly (e.g. given it "Out Ti"
    // for a generic parameter that was declared as an "In"). Look what happens:
    // Interface IZoo(Of In T)     | Dim x as IZoo(Of Animal)
    //   Inherits IReadOnly(Of T)  | Dim y as IZoo(Of Mammal) = x   ' through contravariance of IZoo
    // End Interface               | Dim z as IReadOnly(Of Mammal) = y  ' through inheritance from IZoo
    // Now we might give "z" to someone who's expecting to read only Mammals, even though we know the zoo
    // contains all kinds of animals.
    for (BCSYM_Implements *BaseImplements = Interface->GetFirstImplements();  BaseImplements;  BaseImplements = BaseImplements->GetNext())
    {
        VSASSERT(BaseImplements!=NULL, "unexpected NULL baseimplements");
        if (BaseImplements->IsBad())
        {
            continue;
        }
        BCSYM *BaseInterface = BaseImplements->GetRoot(); // this may be a generic binding context
        VSASSERT(BaseInterface!=NULL, "unexpected NULL baseinterface");
        if (BaseInterface->IsBad())
        {
            continue;
        }

        CheckThatTypeSatisfiesVariance(BaseInterface, Variance_Out, Interface, VarianceContextComplex, BaseImplements, CurrentErrorLog(Interface));
    }

}


void
Bindable::CheckVarianceValidityOfSignature(BCSYM_Proc *Signature)
{
    VSASSERT(Signature, "unexpected NULL signature");
    if (Signature->IsBad())
    {
        return;
    }

    // Variance spec $2:
    // A signature "Function F(Of U1...Un)(P1, ..., Pn) as Tr" or "Sub F(Of U1...Un)(P1, ..., Pn)"
    // is valid if and only if
    //  * the type of each ByVal parameter Pi is valid contravariantly, and
    //  * the type of each ByRef parameter Pi is valid invariantly, and
    //  * (in the case of functions), Tr is valid covariantly, and
    //  * the type of each type constraint on each U1...Un is valid contravariantly.

    // This function can also be called to check the signatures of property-accessors.
    // But note that the BCSYM_MethodDecl of a "getter" stores the property-type as
    // a return-type, and that of a "setter" stores the property-type as the last
    // parameter. We don't consider these to be part of the signature proper, so we
    // don't check them here. (The validity of property-types is checked by the
    // CheckVarianceValidityOfInterface function).
    bool SuppressCheckOfReturnType = Signature->IsPropertyGet();
    bool SuppressCheckOfLastParam = Signature->IsPropertySet();

    // 1. Each ByVal Pi is valid contravariantly, and each ByRef Pi is valid invariantly
    for (BCSYM_Param *param = Signature->GetFirstParam(); param; param=param->GetNext())
    {
        if (param->IsBad())
        {
            continue;
        }
        if (SuppressCheckOfLastParam && param->GetNext()==NULL)
        {
            continue;
        }

        Variance_Kind RequiredVariance = Variance_In;
        VarianceContextEnum Context = VarianceContextByVal;
        BCSYM *paramType = param->GetType();
        VSASSERT(paramType!=NULL, "unexpected NULL paramType");
        if (paramType->IsBad())
        {
            continue;
        }

        if (param->IsByRefKeywordUsed())
        {
            RequiredVariance = Variance_None;
            Context = VarianceContextByRef;
            VSASSERT(paramType->IsPointerType(), "expected ByRef parameter to be a pointer type");
            paramType = paramType->PPointerType()->GetRoot();
            VSASSERT(paramType!=NULL, "unexpected NULL type from pointer type");
            if (paramType->IsBad())
            {
                continue;
            }
        }

        CheckThatTypeSatisfiesVariance(paramType, RequiredVariance, Signature->GetContainingClassOrInterface(), Context, param->GetRawType(), CurrentErrorLog(Signature));
    }

    // 2. Tr is valid covariantly
    BCSYM *Tr = Signature->GetType();
    if (Tr!=NULL && !Tr->IsBad() && !SuppressCheckOfReturnType)
    {
        CheckThatTypeSatisfiesVariance(Tr, Variance_Out, Signature->GetContainingClassOrInterface(), VarianceContextReturn, Signature->GetRawType(), CurrentErrorLog(Signature));
    }

    // 3. each constraint on U1...Un is valid contravariantly
    // "It is character-building to consider why this is required" [Eric Lippert, 2008]
    // Interface IReadOnly(Of Out T) | Class Zoo(Of T)            |  Dim m As IReadOnly(Of Mammal) = new Zoo(Of Mammal)  ' OK through inheritance
    // Sub Fun(Of U As T)()          | Implements IReadOnly(Of T) |  Dim a as IReadOnly(Of Animal) = m   ' OK through covariance
    // End Interface                 | End Class                  |  a.Fun(Of Fish)() ' BAD: Fish is an Animal (satisfies "U as T"), but fun is expecting a mammal!
    for (BCSYM_GenericParam *gParam=Signature->GetFirstGenericParam(); gParam; gParam=gParam->GetNextParam())
    {
        if (gParam->IsBad())
        {
            continue;
        }
        for (BCSYM_GenericTypeConstraint *Constraint=gParam->GetTypeConstraints(); Constraint; Constraint=Constraint->Next())
        {
            if (Constraint->IsBad())
            {
                continue;
            }
            BCSYM *Tconstraint = Constraint->GetType();
            VSASSERT(Tconstraint!=NULL, "unexpected NULL Tconstraint");
            if (Tconstraint->IsBad())
            {
                continue;
            }
            CheckThatTypeSatisfiesVariance(Tconstraint, Variance_In, Signature->GetContainingClassOrInterface(), VarianceContextConstraint, Constraint, CurrentErrorLog(gParam));
        }
    }
}


void
Bindable::CheckThatTypeSatisfiesVariance(
    BCSYM *Type,
    Variance_Kind RequiredVariance,
    BCSYM_Container *Container,
    VarianceContextEnum Context,
    BCSYM *ErrorSymbol,
    ErrorTable *ErrorTable)
{
    CheckThatTypeSatisfiesVariance_Helper(Type, RequiredVariance, Container, Context, ErrorSymbol, ErrorTable, NULL, NULL, false);
}


void
Bindable::CheckThatTypeSatisfiesVariance_Helper(
    BCSYM *Type,                    // We will check this type...
    Variance_Kind RequiredVariance, // ...to make sure it can deliver this kind of variance
    BCSYM_Container *Container,     // The container where we're trying to refer to the type
    VarianceContextEnum Context,    // And where specifically in that container we're trying to refer to the type
    BCSYM *ErrorSymbol,             // If it can't, we report an error squiggly under this symbol
    ErrorTable *ErrorTable,         // in this error table
    _In_opt_ BCSYM_GenericBinding *ErrorBinding, // This is the binding that immediately contains Type (if Type is contained by a binding)
    _In_opt_ BCSYM_GenericParam *ErrorParam,     // and this is the generic parameter that it was bound to
    int ErrorBindingNesting                      // Was that generic at the very top level of ErrorSymbol? (if so, it will simplify error messages)
)
{
    VSASSERT(Type!=NULL, "unexpected NULL type");
    VSASSERT(ErrorSymbol!=NULL, "please supply a symbol under which this error will be reported");
    VSASSERT((ErrorBinding==NULL) == (ErrorParam==NULL), "Please supply an ErrorBinding if and only if you supply an ErrorParam");
    VSASSERT(ErrorBinding==NULL || ErrorBinding->GetGeneric()!=NULL, "Unexpected: a generic binding of no particular generic type");
    VSASSERT(ErrorBinding==NULL || ErrorBinding->GetArgumentCount()>0, "Unexpected: a generic binding with no generic arguments");
    if (Type->IsBad())
    {
        return;
    }


    // Variance spec $2:
    //
    // A type T is valid invariantly if and only if:
    //  * it is valid covariantly, and
    //  * it is valid contravariantly.
    //
    // A type T is valid covariantly if and only if one of the following hold: either
    //  * T is a generic parameter which was not declared contravariant, or
    //  * T is an array type U() where U is valid covariantly, or
    //  * T is a construction X1(Of T11...)...Xn(Of Tn1...) of some generic struct/class/interface/delegate Xn
    //    declared as X1(Of X11...)...Xn(Of Xn1...) such that for each i and j,
    //      - if Xij was declared covariant then Tij is valid covariantly
    //      - if Xij was declared contravariant then Tij is valid contravariantly
    //      - if Xij was declared invariant then Tij is valid invariantly
    //  * or T is a non-generic struct/class/interface/delegate/enum.
    //
    // A type T is valid contravariantly if and only if one of the following hold: either
    //  * T is a generic parameter which was not declared covariant, or
    //  * T is an array type U() where U is valid contravariantly, or
    //  * T is a construction X1(Of T11...)...Xn(Of Tn1...) of some generic struct/class/interface/delegate Xn
    //    declared as X1(Of X11...)...Xn(Of Xn1...) such that for each i and j,
    //      - if Xij was declared covariant then Tij is valid contravariantly
    //      - if Xij was declared contravariant then Tij is valid covariantly
    //      - if Xij was declared invariant then Tij is valid invariantly
    //  * or T is a non-generic struct/class/interface/delegate/enum.
    //
    //
    // In all cases, if a type fails a variance validity check, then it ultimately failed
    // because somewhere there were one or more generic parameters "T" which were declared with
    // the wrong kind of variance. In particular, they were either declared In when they'd have
    // to be Out or InOut, or they were declared Out when they'd have to be In or InOut.
    // We mark all these as errors.
    //
    // BUT... CLS restrictions say that in any generic type, all nested types first copy their
    // containers's generic parameters. This restriction is embodied in the BCSYM structure.
    //    SOURCE:                        BCSYM:                               IL:
    //    Interface I(Of Out T1)         Interface"I"/genericparams=T1        .interface I(Of Out T1)
    //      Interface J : End Interface    Interface"J"/no genericparams         .interface J(Of Out T1)
    //      Sub f(ByVal x as J)            ... GenericTypeBinding(J,args=[],       .proc f(x As J(Of T1))
    //    End Interface                                     parentargs=I[T1])
    // Observe that, by construction, any time we use a nested type like J in a contravariant position
    // then it's bound to be invalid. If we naively applied the previous paragraph then we'd emit a
    // confusing error to the user like "J is invalid because T1 is an Out parameter". So we want
    // to do a better job of reporting errors. In particular,
    //   * If we are checking a GenericTypeBinding (e.g. x as J(Of T1)) for contravariant validity, look up
    //     to find the outermost ancester binding (e.g. parentargs=I[T1]) which is of a variant interface.
    //     If this is also the outermost variant container of the current context, then it's an error.


    // 1. if T is a generic parameter which was declared wrongly
    if (Type->IsGenericParam())
    {
        if ((Type->PGenericParam()->GetVariance()==Variance_Out && RequiredVariance!=Variance_Out) ||
            (Type->PGenericParam()->GetVariance()==Variance_In && RequiredVariance!=Variance_In))
        {
            // The error is either because we have an "Out" param and Out is inappropriate here,
            // or we used an "In" param and In is inappropriate here. This flag says which:
            bool InappropriateOut = (Type->PGenericParam()->GetVariance()==Variance_Out);

            if (ErrorBinding!=NULL && ErrorBinding->GetGeneric()->IsBad())
            {
                return;
            }

            // OKAY, so now we need to report an error. Simple enough, but we've tried to give helpful
            // context-specific error messages to the user, and so the code has to work through a lot
            // of special cases.

            if (Context == VarianceContextByVal)
            {
                // "Type '|1' cannot be used as a ByVal parameter type because '|1' is an 'Out' type parameter."
                VSASSERT(InappropriateOut, "unexpected: an variance error in ByVal must be due to an inappropriate out");
                ReportErrorOnSymbol(ERRID_VarianceOutByValDisallowed1, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            else if (Context == VarianceContextByRef)
            {
                // "Type '|1' cannot be used in this context because 'In' and 'Out' type parameters cannot be used for ByRef parameter types, and '|1' is an 'Out/In' type parameter."
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutByRefDisallowed1 : ERRID_VarianceInByRefDisallowed1;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            else if (Context == VarianceContextReturn)
            {
                // "Type '|1' cannot be used as a return type because '|1' is an 'In' type parameter."
                VSASSERT(!InappropriateOut, "unexpected: a variance error in Return Type must be due to an inappropriate in");
                ReportErrorOnSymbol(ERRID_VarianceInReturnDisallowed1, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            else if (Context == VarianceContextConstraint)
            {
                // "Type '|1' cannot be used as a generic type constraint because '|1' is an 'Out' type parameter."
                VSASSERT(InappropriateOut, "unexpected: a variance error in Constraint must be due to an inappropriate out");
                ReportErrorOnSymbol(ERRID_VarianceOutConstraintDisallowed1, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            else if (Context == VarianceContextNullable)
            {
                StringBuffer sb;
                // "Type '|1' cannot be used in '|2' because 'In' and 'Out' type parameters cannot be made nullable, and '|1' is an 'In/Out' type parameter."
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutNullableDisallowed2 : ERRID_VarianceInNullableDisallowed2;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type), ErrorTable->ExtractErrorName(ErrorSymbol, NULL, sb));
            }

            else if (Context == VarianceContextReadOnlyProperty)
            {
                // "Type '|1' cannot be used as a ReadOnly property type because '|1' is an 'In' type parameter."
                VSASSERT(!InappropriateOut, "unexpected: a variance error in ReadOnlyProperty must be due to an inappropriate in");
                ReportErrorOnSymbol(ERRID_VarianceInReadOnlyPropertyDisallowed1, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            else if (Context == VarianceContextWriteOnlyProperty)
            {
                // "Type '|1' cannot be used as a WriteOnly property type because '|1' is an 'Out' type parameter."
                VSASSERT(InappropriateOut, "unexpected: a variance error in WriteOnlyProperty must be due to an inappropriate out");
                ReportErrorOnSymbol(ERRID_VarianceOutWriteOnlyPropertyDisallowed1, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            else if (Context == VarianceContextProperty)
            {
                // "Type '|1' cannot be used as a property type in this context because '|1' is an 'Out/In' type parameter and the property is not marked ReadOnly/WriteOnly.")
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutPropertyDisallowed1 : ERRID_VarianceInPropertyDisallowed1;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,  GetQualifiedErrorName(Type));
            }

            // Otherwise, we're in "VarianceContextComplex" property. And so the error message needs
            // to spell out precisely where in the context we are:
            // "Type '|1' cannot be used in this context because '|1' is an 'Out|In' type parameter."
            // "Type '|1' cannot be used for the '|2' in '|3' in this context because '|1' is an 'Out|In' type parameter."
            // "Type '|1' cannot be used in '|2' in this context because '|1' is an 'Out|In' type parameter."
            // "Type '|1' cannot be used in '|2' for the '|3' in '|4' in this context because '|1' is an 'Out' type parameter."
            // We need the "in '|2' here" clause when ErrorBindingIsNested, to show which instantiation we're talking about.
            // We need the "for the '|3' in '|4'" when ErrorBinding->GetGenericParamCount()>1

            else if (ErrorBinding==NULL)
            {   // "Type '|1' cannot be used in this context because '|1' is an 'Out|In' type parameter."
                // Used for simple errors where the erroneous generic-param is NOT inside a generic binding:
                // e.g. "Sub f(ByVal a as O)" for some parameter declared as "Out O"
                // gives the error "An 'Out' parameter like 'O' cannot be user here".
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutParamDisallowed1 : ERRID_VarianceInParamDisallowed1;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,    GetQualifiedErrorName(Type));
            }

            else if (ErrorBindingNesting<=1 && ErrorBinding->GetArgumentCount()<=1)
            {
                // "Type '|1' cannot be used in this context because '|1' is an 'Out|In' type parameter."
                // e.g. "Sub f(ByVal a As IEnumerable(Of O))" yields
                // "An 'Out' parameter like 'O' cannot be used here."
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutParamDisallowed1 : ERRID_VarianceInParamDisallowed1;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,    GetQualifiedErrorName(Type));
            }

            else if (ErrorBindingNesting<=1 && ErrorBinding->GetArgumentCount()>1)
            {
                // "Type '|1' cannot be used for the '|2' in '|3' in this context because '|1' is an 'Out|In' type parameter."
                // e.g. "Sub f(ByVal a As IDoubleEnumerable(Of O,I)) yields
                // "An 'Out' parameter like 'O' cannot be used for type parameter 'T1' of 'IDoubleEnumerable(Of T1,T2)'."
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutParamDisallowedForGeneric3 : ERRID_VarianceInParamDisallowedForGeneric3;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,
                                    GetQualifiedErrorName(Type),
                                    GetQualifiedErrorName(ErrorParam),
                                    GetQualifiedErrorName(ErrorBinding->GetGeneric()));
            }

            else if (ErrorBindingNesting>1 && ErrorBinding->GetArgumentCount()<=1)
            {
                // "Type '|1' cannot be used in '|2' in this context because '|1' is an 'Out|In' type parameter."
                // e.g. "Sub f(ByVal a as Func(Of IEnumerable(Of O), IEnumerable(Of O))" yields
                // "In 'IEnumerable(Of O)' here, an 'Out' parameter like 'O' cannot be used."
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutParamDisallowedHere2 : ERRID_VarianceInParamDisallowedHere2;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,   
                                    GetQualifiedErrorName(Type),
                                    GetQualifiedErrorName(ErrorBinding));
            }

            else if (ErrorBindingNesting>1 && ErrorBinding->GetArgumentCount()>1)
            {
                // "Type '|1' cannot be used in '|2' for the '|3' in '|4' in this context because '|1' is an 'Out' type parameter."
                // e.g. "Sub f(ByVal a as IEnumerable(Of Func(Of O,O))" yields
                // "In 'Func(Of O,O)' here, an 'Out' parameter like 'O' cannot be used for type parameter 'Tresult' of 'Func(Of Tresult,T)'."
                ERRID ErrId = InappropriateOut ? ERRID_VarianceOutParamDisallowedHereForGeneric4 : ERRID_VarianceInParamDisallowedHereForGeneric4;
                ReportErrorOnSymbol(ErrId, ErrorTable, ErrorSymbol,
                                    GetQualifiedErrorName(Type),
                                    GetQualifiedErrorName(ErrorBinding),
                                    GetQualifiedErrorName(ErrorParam),
                                    GetQualifiedErrorName(ErrorBinding->GetGeneric()));
            }

            else
            {
                VSFAIL("InternalCompilerError: we should already have covered all cases of Context/ErrorBindingNesting/ErrorBindingArgCount");
                ReportErrorOnSymbol(ERRID_InternalCompilerError, ErrorTable, ErrorSymbol);
            }
        }
    }

    // 2. if T is an array U():
    else if (Type->IsArrayType())
    {
        BCSYM_ArrayType *ArrayType = Type->PArrayType();
        CheckThatTypeSatisfiesVariance_Helper(
                ArrayType->GetRoot(),
                RequiredVariance,
                Container,
                Context,
                ErrorSymbol,
                ErrorTable,
                ErrorBinding,
                ErrorParam,
                ErrorBindingNesting);
    }

    // 3. T is a construction X1(Of T11...)...Xn(Of Tn1...) of some generic struct/class/interface/delegate X1(Of X11...)...Xn(Of Xn1...)
    else if (Type->IsGenericBinding())
    {
        // Special check, discussed above, for better error-reporting when we find a generic binding in an
        // illegal contravariant position
        if (RequiredVariance != Variance_Out)
        {
            BCSYM_NamedRoot *OutermostVarianceContainerOfBinding = NULL;
            //
            for (BCSYM_GenericBinding *GenericBinding=Type->PGenericBinding()->GetParentBinding();
                 GenericBinding;
                 GenericBinding=GenericBinding->GetParentBinding())
            {
                if (TypeHelpers::HasVariance(GenericBinding))
                {
                    OutermostVarianceContainerOfBinding = GenericBinding->GetGeneric();
                    VSASSERT(OutermostVarianceContainerOfBinding!=NULL, "Expected generic-binding to have non-null generic");
                }
            }

            BCSYM_NamedRoot *OutermostVarianceContainerOfContext = NULL;
            //
            for (BCSYM_Container *Parent = Container; Parent; Parent=Parent->GetContainingClassOrInterface())
            {
                if (TypeHelpers::HasVariance(Parent))
                {
                    OutermostVarianceContainerOfContext = Parent;
                }
            }

            if (OutermostVarianceContainerOfBinding!=NULL && OutermostVarianceContainerOfBinding == OutermostVarianceContainerOfContext )
            {
                BCSYM_NamedRoot *NestedType = Type->PGenericBinding()->GetGeneric();
                VSASSERT(NestedType!=NULL, "Expected generic-binding to have non-null generic");

                // ERRID_VarianceTypeDisallowed2.               "Type '|1' cannot be used in this context because both the context and the definition of '|1' are nested within type '|2', and '|2' has 'In' or 'Out' type parameters. Consider moving '|1' outside of '|2'."
                // ERRID_VarianceTypeDisallowedForGeneric4.     "Type '|1' cannot be used for the '|3' in '|4' in this context because both the context and the definition of '|1' are nested within type '|2', and '|2' has 'In' or 'Out' type parameters. Consider moving '|1' outside of '|2'."
                // ERRID_VarianceTypeDisallowedHere3.           "Type '|1' cannot be used in '|3' in this context because both the context and the definition of '|1' are nested within type '|2', and '|2' has 'In' or 'Out' type parameters. Consider moving '|1' outside of '|2'."
                // ERRID_VarianceTypeDisallowedHereForGeneric5. "Type '|1' cannot be used for the '|4' of '|5' in '|3' in this context because both the context and the definition of '|1' are nested within type '|2', and '|2' has 'In' or 'Out' type parameters. Consider moving '|1' outside of '|2'."
                if (ErrorBinding==NULL)
                {
                    ReportErrorOnSymbol(ERRID_VarianceTypeDisallowed2, ErrorTable, ErrorSymbol,
                                        GetQualifiedErrorName(NestedType),
                                        GetQualifiedErrorName(OutermostVarianceContainerOfBinding));
                }
                else if (ErrorBindingNesting<=1 && ErrorBinding->GetArgumentCount()<=1)
                {
                    ReportErrorOnSymbol(ERRID_VarianceTypeDisallowed2, ErrorTable, ErrorSymbol,
                                        GetQualifiedErrorName(NestedType),
                                        GetQualifiedErrorName(OutermostVarianceContainerOfBinding));
                }
                else if (ErrorBindingNesting<=1 && ErrorBinding->GetArgumentCount()>1)
                {
                    ReportErrorOnSymbol(ERRID_VarianceTypeDisallowedForGeneric4, ErrorTable, ErrorSymbol,
                                        GetQualifiedErrorName(NestedType),
                                        GetQualifiedErrorName(OutermostVarianceContainerOfBinding),
                                        GetQualifiedErrorName(ErrorParam),
                                        GetQualifiedErrorName(ErrorBinding->GetGeneric()));
                }
                else if (ErrorBindingNesting>1 && ErrorBinding->GetArgumentCount()<=1)
                {
                    ReportErrorOnSymbol(ERRID_VarianceTypeDisallowedHere3, ErrorTable, ErrorSymbol,
                                        GetQualifiedErrorName(NestedType),
                                        GetQualifiedErrorName(OutermostVarianceContainerOfBinding),
                                        GetQualifiedErrorName(ErrorBinding));
                }
                else if (ErrorBindingNesting>1 && ErrorBinding->GetArgumentCount()>1)
                {
                    ReportErrorOnSymbol(ERRID_VarianceTypeDisallowedHereForGeneric5, ErrorTable, ErrorSymbol,
                                        GetQualifiedErrorName(NestedType),
                                        GetQualifiedErrorName(OutermostVarianceContainerOfBinding),
                                        GetQualifiedErrorName(ErrorBinding),
                                        GetQualifiedErrorName(ErrorParam),
                                        GetQualifiedErrorName(ErrorBinding->GetGeneric()));
                }
                else
                {
                    VSFAIL("InternalCompilerError: impossible combination of nested/argcount");
                    ReportErrorOnSymbol(ERRID_InternalCompilerError, ErrorTable, ErrorSymbol);
                }
                return;
            }
        }

        // The general code below will catch the case of nullables "T?" or "Nullable(Of T)", which require T to
        // be inviarant. But we want more specific error reporting for this case, so we check for it first.
        if (TypeHelpers::IsNullableType(Type, this->m_CompilerHost))
        {
            BCSYM_GenericBinding *GenericBinding = Type->PGenericBinding();
            VSASSERT(GenericBinding->GetGenericParamCount()==1 && GenericBinding->GetFirstGenericParam()->GetVariance() == Variance_None, "unexpected: a nullable type should have one generic parameter with no variance");
            CheckThatTypeSatisfiesVariance_Helper(GenericBinding->GetArgument(0), Variance_None, Container, VarianceContextNullable, ErrorSymbol, ErrorTable, ErrorBinding, ErrorParam, ErrorBindingNesting);
            return;
        }


        // "Type" will refer to the last generic binding, Xn(Of Tn1...). So we have to check all the way up to X1.
        for (BCSYM_GenericBinding *GenericBinding=Type->PGenericBinding(); GenericBinding; GenericBinding=GenericBinding->GetParentBinding())
        {
            int ArgumentIndex = 0;
            for (BCSYM_GenericParam *gParam=GenericBinding->GetFirstGenericParam(); gParam; gParam=gParam->GetNextParam(), ArgumentIndex++)
            {
                BCSYM *gArgument = GenericBinding->GetArgument(ArgumentIndex);

                Variance_Kind SubRequiredVariance =
                        RequiredVariance==Variance_None
                            ? Variance_None
                            : (RequiredVariance==Variance_Out ? gParam->GetVariance() : InvertVariance(gParam->GetVariance()));

                // nb. the InvertVariance() here is the only difference between covariantly-valid and contravariantly-valid
                // for generic constructions.
                CheckThatTypeSatisfiesVariance_Helper(
                    gArgument,
                    SubRequiredVariance,
                    Container,
                    VarianceContextComplex,
                    ErrorSymbol,
                    ErrorTable,
                    GenericBinding,
                    gParam,
                    ErrorBindingNesting+1);
            }
        }
    }

    else if (Type->IsClass() || Type->IsInterface()) // Type->IsClass() also covers StdModule, Struct, Enum and Delegate.
    {
        // Non-generic types intrinsically satisfy all variance requirements.
    }
    else
    {
        VSFAIL("unexpected type when checking variance");
    }
}


Variance_Kind Bindable::InvertVariance(Variance_Kind v)
{
    switch (v)
    {
        case Variance_Out:
            return Variance_In;
        case Variance_In:
            return Variance_Out;
        case Variance_None:
            return Variance_None;
        default:
            VSFAIL("bad variance");
            return v;
    }
}




// "AmbiguousClashData": this datastructure is used by CheckVarianceAmbiguity.
// It detects ambiguity-clashes between pairs of "implements" declarations in a class/interface.
// For each clash, it stores Key=one of the pair, and Value=AmbiguousClashData.
// The AmbiguousClashData includes the other element of the pair, and also the
// actual culprit interfaces that caused the clash. Note that it's possible for an implements
// declaration to INHERIT from cuplrits, rather than being the culprit itself.
// e.g. Class C : I1, I2,  where I1:R(Of Mammal), and I2:R(Of Fish).
struct AmbiguousClashData
{
    BCSYM_Implements *otherDeclaration;  
    BCSYM_GenericTypeBinding *left, *right; // "left" and "right" are the two interfaces that caused the clash.

    AmbiguousClashData(
        BCSYM_Implements *otherDeclaration,
        BCSYM_GenericTypeBinding *left,
        BCSYM_GenericTypeBinding *right) :
        otherDeclaration(otherDeclaration), left(left),right(right)
    {
    }
    
    AmbiguousClashData()
    {
    }
};



// "AmbiguousInterfaceData": this datastructure is used by CheckVarianceAmbiguity.
// The function searches through all "implements" declarations in a class/interface,
// and also for all the interfaces that these declarations inherit from.
// Note that a given interface might be inherited by several different "implements" declarations,
// e.g. Class C : I1, I2,  where I1:R(Of Mammal) and I2:R(Of Mammal)
struct AmbiguousInterfaceData
{
    BCSYM_GenericTypeBinding *iface;  // an interface that we're going to investigate
    HashSet<BCSYM_Implements*> implementsDeclarations;
};


// PopulateInterfaceListWithGenerics:
// This function scans the list of interfaces starting from "firstImplements", along with every interface
// that those interfaces inherit. For every one that's a generic type binding with variant parameters,
// it gets put in "bins". There may be several bins. Each bin contains bindings of one particular 
// generic interface: e.g. one bin will contain IEnumerable(Of String) and IEnumerable(Of Object),
// and another bin will contain IFred(Of Alf) and IFred(Of Jones) and IFred(Of Wilbur).
// Note: we assume that the caller has already resolved partial types for "Container" in case it's a class.
// Interfaces can't be partial, anyways.
void PopulateInterfaceListWithGenerics(
        DynamicArray<DynamicArray<AmbiguousInterfaceData>> &bins,  // store results here
        BCSYM *container,    // scan all the interfaces implemented by this container
        int depth,   // inheritance depth that we're at
        BCSYM_Implements *declaration,  // If depth>0 then we use "declaration" for the declaration it came from
        Symbols &SymbolFactory
)
{
    VSASSERT(depth==0 || declaration!=NULL, "unexpected: when walking up the hierarchy, we should have kept track of which declaration we started from");
    VSASSERT(container!=NULL && ((container->IsClass() && depth==0) || container->IsInterface()), "please invoke PopulateInterfaceList with a container or an interface");
    // note: container is either a GenericBinding of a Class/Interface, or is a Class/Interface itself.
    // The functions IsClass() and IsInterface() above both peer through GenericBindings to the unbound Generic itself.
    // And below, container->PContainer() does likewise to enumerate the implemented interfaces of the unboundGeneric.
    // Then (Dev10#570404) we reapply any generic binding contexts that are needed.

    BCITER_ImplInterfaces ImplInterfacesIter(container->PContainer());
    while (BCSYM_Implements *implements = ImplInterfacesIter.GetNext())
    {
        if (implements->IsBad() || implements->GetRoot()==NULL ||
            implements->GetRoot()->IsBad() || !implements->GetRoot()->IsInterface())
        {
            // in all these error conditions we won't even bother looking for ambiguity problems...
            continue;
        }

        BCSYM *iface = implements->GetRoot();
        if (container->IsGenericBinding())
        {
            // Dev10#570404: e.g. if container was "IList(Of String)", then ImplInterfaces would iterate
            // over the implemented interfaces of IList(Of T) giving ICollection(Of T) and IEnumerable(Of T),
            // and then we have to reapply the {String/T} binding onto each of those implemented interfaces:
            iface = ReplaceGenericParametersWithArguments(iface, container->PGenericBinding(), SymbolFactory);
        }

        // We want to associate each interface with the interface-declaration it came from. In the original
        // class's list of declarations, that will be just the same as "implements". But further up the interface
        // inheritance hierarchy, it won't be... we deal with that through "declaration"
        if (depth==0)
        {
            declaration = implements;
        }
        VSASSERT(declaration!=NULL && declaration->PImplements(), "unexpected: an implements declaration that wasn't a BCSYM_Implements declaration");

        // Recursive search through the interface's inherited interfaces
        PopulateInterfaceListWithGenerics(bins, iface, depth+1, declaration, SymbolFactory);

        if (iface->IsGenericTypeBinding() && TypeHelpers::HasVariance(iface))
        {
            // Now we have to add "iface" into the appropriate bin.
            // First find the bin (creating a new bin if necessary)
            ULONG bindex;
            for (bindex=0; bindex<bins.Count(); bindex++)
            {
                VSASSERT(bins.Array()[bindex].Count()>0, "unexpected: an empty bin");
                if (BCSYM::AreTypesEqual(bins.Array()[bindex].Array()[0].iface->GetGeneric(), iface->PGenericTypeBinding()->GetGeneric()))
                {
                    break;
                }
            }
            if (bindex == bins.Count())
            {
                bins.CreateNew();
            }
            DynamicArray<AmbiguousInterfaceData> &bin = bins.Array()[bindex];

            // Next, add a new AmbiguousInterfaceData only if it's not already there
            ULONG dataIndex;
            for (dataIndex=0; dataIndex<bin.Count(); dataIndex++)
            {
                if (BCSYM::AreTypesEqual(bin.Array()[dataIndex].iface, iface))
                {
                    break;
                }
            }
            if (dataIndex == bin.Count())
            {
                bin.CreateNew();
                bin.Array()[dataIndex].iface = iface->PGenericTypeBinding();
            }
            AmbiguousInterfaceData &interfaceData = bin.Array()[dataIndex];
            
            // Finally, add baseInterface to the interfaceData's list if it's not already present.
            interfaceData.implementsDeclarations.Add(declaration);
        }
    }
}


void
Bindable::CheckVarianceAmbiguity(BCSYM_Container *Container, CompilationCaches *IDECompilationCaches)
{
    // What is "Variance Ambiguity"? Here's an example:
    // Class ReflectionType
    //   Implements IEnumerable(Of Field)
    //   Implements IEnumerable(Of Method)
    //   Public Sub GetEnumeratorF() As IEnumerator(Of Field) Implements IEnumerable(Of Field).GetEnumerator ...
    //   Public Sub GetEnumeratorM() As IEnumerator(Of Method) Implements IEnumberale(Of Method).GetEnumerator ...
    // End Class
    // Dim x as new ReflectionType
    // Dim y as IEnumerable(Of Member) = x
    // Dim z = y.GetEnumerator()
    //
    // Note that, through variance, both IEnumerable(Of Field) and IEnumerable(Of Method) have widening
    // conversions to IEnumerable(Of Member). So it's ambiguous whether the initialization of "z" would
    // invoke GetEnumeratorF or GetEnumeratorM. This function avoids such ambiguity at the declaration
    // level, i.e. it reports a warning on the two implements classes inside ReflectionType that they
    // may lead to ambiguity.

    // For an interface which inherits a list of interfaces, or for a class which imports a list of interfaces,
    // we check whether any of those interfaces (or the ones reachable through inheritance) might lead
    // to variance-conversion-ambiguity.
    // (for simplicity on callers, this function might also be called when Container is any other kind of
    // container; they'll always have a null "Implements" clause. But if the language were changed so that
    // an enum could implement interfaces, for instance, then we'll end up doing the right thing.)

    NorlsAllocator ScratchAllocator(NORLSLOC);
    Symbols SymbolFactory(CurrentCompilerInstance(), &ScratchAllocator, NULL);


    // First step: gather up the list of interfaces into separate bins.
    // Each bin contains interfaces with the same variant generic root, but different bindings of it.
    // Note: the outer DynamicArray takes care of calling destructors on each of the inner DynamicArrays.
    // And the inner DynamicArrays would notionally call the destructor of their contents, but their
    // contents are just pointers to BCSYM_GenericTypeBinding, so they're not touched.
    DynamicArray<DynamicArray<AmbiguousInterfaceData>> bins;
    ResolvePartialTypesForContainer(Container, IDECompilationCaches);
    PopulateInterfaceListWithGenerics(bins, Container, 0, NULL, SymbolFactory);

    if (bins.Count() == 0)
    {
        return;
    }

    // What we produce in the end should be a list of which implements clauses clashed with which
    // other implements clauses, for error reporting. But our various collection classes aren't up
    // to the job. So we'll keep simpler clash data. It'll still be decent enough to give good
    // error reports. If clashes has a pair (X, {Y,a,b}) then it means that the implements
    // delcaration "X" clashed with the implements declaration "Y", and the culprits for this clash
    // were the two interfaces "a" and "b". Note that it's possible for X==Y (in the case
    // that we imported clashing metadata).
    DynamicHashTable<BCSYM_Implements*, AmbiguousClashData, VBAllocWrapper> clashes;
    

    // Second step: within each bin, do a pairwise comparison of each interface to look for ambiguity.
    // Note that the ambiguity tests are symmetric.
    for (ULONG ibin=0; ibin<bins.Count(); ibin++)
    {
        DynamicArray<AmbiguousInterfaceData> &bin = bins.Array()[ibin];
        for (ULONG outer=0; outer<bin.Count(); outer++)
        {
            for (ULONG inner=outer+1; inner<bin.Count(); inner++)
            {
                BCSYM_GenericTypeBinding *left = bin.Array()[outer].iface;
                BCSYM_GenericTypeBinding *right = bin.Array()[inner].iface;
                VSASSERT(left!=NULL && right!=NULL && left->IsInterface() && right->IsInterface(), "unexpected: non-interfaces in a bin");
                VSASSERT(!BCSYM::AreTypesEqual(left,right), "unexpected: equal types in a bin");
                VSASSERT(BCSYM::AreTypesEqual(left->GetGeneric(), right->GetGeneric()), "unexpected: types with different generics in the same bin");

                // We have something like left=ICocon(Of Mammal, int32[]), right=ICocon(Of Fish, int32[])
                // for some interface ICocon(Of Out T, In U). And we have to decide if left and right 
                // might lead to ambiguous member-lookup later on in execution.
                //
                // To do this: go through each type parameter T, U...
                //   * For "Out T", judge whether the arguments Mammal/Fish cause ambiguity or prevent it.
                //   * For "In T", judge whether the arguments int32[]/int32[] cause ambiguity or prevent it.
                //
                // "Causing/preventing ambiguity" is as follows.
                //   * Covariant parameters "Out T": ambiguity is caused when the two type arguments Mammal/Fish
                //     are non-identical non-object types not known to be values. Ambiguity is prevented when
                //     Mammal/Fish are non-identical non-generic types of which at least one is a value type.
                //   * Contravariant parameters "In U": ambiguity is caused when the two type arguments int32[]/int32[]
                //     are non-identical types not known to be values. Ambiguity is prevented when int32[]/int32[]
                //     are non-identical non-generic types of which at least one is a value type.
                //   * Invariant parameters "V": these never cause ambiguity. They prevent ambiguity when
                //     they are provided with two non-identical non-generic arguments.
                // 
                // Given all that, ambiguity was prevented in any positions, then left/right are fine.
                // Otherwise, if ambiguity wasn't caused in any positions, then left/right are fine.
                // Otherwise, left/right have an ambiguity...
                int causesAmbiguityCount = 0;
                int preventsAmbiguityCount = 0;

                // A generic binding is constructed in layers, e.g. C(Of Z).I(Of X,Y). So we have to
                // iterate over those layers...
                for (BCSYM_GenericTypeBinding *leftBinding=left, *rightBinding=right;
                     leftBinding!=NULL && rightBinding!=NULL && preventsAmbiguityCount==0;
                     leftBinding = leftBinding->GetParentBinding(),  rightBinding = rightBinding->GetParentBinding())
                {
                    VSASSERT(BCSYM::AreTypesEqual(leftBinding->GetGeneric(), rightBinding->GetGeneric()), "unexpected: different generic bindings");
                    BCSYM_NamedRoot *genericType = left->GetGenericType();
                    
                    // Within each layer, we have to iterate over each parameter
                    int argumentIndex = 0;
                    for (BCSYM_GenericParam *gParam = genericType->GetFirstGenericParam();
                         gParam && preventsAmbiguityCount==0;
                         gParam=gParam->GetNextParam(), argumentIndex++)
                    {
                        BCSYM *leftArgument = leftBinding->GetArgument(argumentIndex);
                        BCSYM *rightArgument = rightBinding->GetArgument(argumentIndex);
                        VSASSERT(leftArgument!=NULL && rightArgument!=NULL, "unexpected: null type arguments to a generic type binding");
                        bool areEqual = BCSYM::AreTypesEqual(leftArgument, rightArgument);

                        switch (gParam->GetVariance())
                        {
                            case Variance_Out:
                            {
                                if (!areEqual && !TypeHelpers::IsValueType(leftArgument) && !TypeHelpers::IsValueType(rightArgument) &&
                                    !leftArgument->IsObject() && !rightArgument->IsObject())
                                {
                                    causesAmbiguityCount++;
                                }
                                else if (!areEqual && !leftArgument->IsGeneric() && !rightArgument->IsGeneric() &&
                                    (TypeHelpers::IsValueType(leftArgument) || TypeHelpers::IsValueType(rightArgument)))
                                {
                                    preventsAmbiguityCount++;
                                }
                                break;
                            }
                            case Variance_In:
                            {
                                if (!areEqual && !TypeHelpers::IsValueType(leftArgument) && !TypeHelpers::IsValueType(rightArgument))
                                {
                                    causesAmbiguityCount++;
                                }
                                else if (!areEqual && !leftArgument->IsGeneric() && !rightArgument->IsGeneric() &&
                                    (TypeHelpers::IsValueType(leftArgument) || TypeHelpers::IsValueType(rightArgument)))
                                {
                                    preventsAmbiguityCount++;
                                }
                                break;
                            }
                            case Variance_None:
                            {
                                if (!areEqual && !leftArgument->IsGeneric() && !rightArgument->IsGeneric())
                                {
                                    preventsAmbiguityCount++;
                                }
                                break;
                            }
                            default:
                            {
                                VSFAIL("unexpected: unknown variance");
                                preventsAmbiguityCount++;
                            }

                        } // end switch(Variance)
                    } // end for each argument index in a layer of generic binding
                } // end for layers of generic binding


                // At this point, all that remains is to add this ambiguity (if there was one)
                // to our list "clashes" which records which implements declarations clashed with which other ones.
                if (preventsAmbiguityCount>0 || causesAmbiguityCount==0)
                {
                    continue;
                }

                // The thing is, the interface "left" might have come from several different Implements declarations.
                // And "right" might also have come from several different declarations.
                // e.g. Class C : implements I1 : implements I2 : implements I3,
                // where "Interface I1 : implements R(Of Fish)" and "I2: implements R(Of Mammal)" and "I3: implements R(Of Mammal)".
                // It's even possible (if we imported metadata that had variance ambiguity) that
                // a single interface I1 might contain clashes INSIDE itself.
                //
                VSASSERT(bin.Array()[outer].implementsDeclarations.Count()>0 &&
                         bin.Array()[inner].implementsDeclarations.Count()>0,
                         "unexpected: clashing left/right interfaces, but apparently the interfaces didn't come from any declaration!");

                HashSetIterator<BCSYM_Implements*> leftDeclarations(&bin.Array()[outer].implementsDeclarations);
                while (leftDeclarations.MoveNext())
                {
                    BCSYM_Implements *leftDeclaration = leftDeclarations.Current();

                    HashSetIterator<BCSYM_Implements*> rightDeclarations(&bin.Array()[inner].implementsDeclarations);
                    while (rightDeclarations.MoveNext())
                    {
                        BCSYM_Implements *rightDeclaration = rightDeclarations.Current();

                        VSASSERT(leftDeclaration!=NULL && rightDeclaration!=NULL, "unexpected: null left/right declarations");

                        // In the following, if a clash had previously been added because of a clash inside
                        // a single interface declaration with itself (e.g. due to bad metadata), then
                        // we can probably find a more interesting clash to report to the user...
                        if (!clashes.Contains(leftDeclaration) ||
                            clashes.GetValue(leftDeclaration).otherDeclaration == leftDeclaration)
                        {
                            clashes.SetValue(leftDeclaration, AmbiguousClashData(rightDeclaration, left, right));
                        }
                        if (!clashes.Contains(rightDeclaration) ||
                            clashes.GetValue(rightDeclaration).otherDeclaration == rightDeclaration)
                        {
                            clashes.SetValue(rightDeclaration, AmbiguousClashData(leftDeclaration, right, left));
                        }
                    } // end while rightDeclarations.MoveNext
                } // end while leftDeclarations.MoveNext



            } // end for inner GenericTypeBinding within bin
        } // end for outer GenericTypeBinding within bin
    } // end for bin


    // Now, at the end, we have "clashes". For each ambiguous interface declaration, it reports
    // another interface that it's ambiguous with.
    if (clashes.Count()==0)
    {
        return;
    }

    HashTableIterator<BCSYM_Implements*,AmbiguousClashData,VBAllocWrapper> clash(&clashes);
    while (clash.MoveNext())
    {
        BCSYM_Implements *leftDeclaration = clash.Current().Key();
        BCSYM_Implements *rightDeclaration = clash.Current().Value().otherDeclaration;
        BCSYM_GenericTypeBinding *leftCulprit = clash.Current().Value().left;
        BCSYM_GenericTypeBinding *rightCulprit = clash.Current().Value().right;

        if (leftDeclaration==NULL || rightDeclaration==NULL || leftCulprit==NULL || rightCulprit==NULL ||
            !BCSYM::AreTypesEqual(leftCulprit->GetGeneric(), rightCulprit->GetGeneric()))
        {
            VSFAIL("InternalCompilerError: we somehow ended up with inconsistent clash data");;
            ReportErrorOnSymbol(ERRID_InternalCompilerError, CurrentErrorLog(Container), Container);
            continue;
        }

        if (leftDeclaration->IsBad() || rightDeclaration->IsBad())
        {
            continue;
        }

        // "Interface '|1' is ambiguous with another implemented interface '|2' due to the 'In' and 'Out' parameters in '|3'."
        // Note: we use GetBasicRep on the generic to show its full name, including In/Out annotations.
        StringBuffer FullGenericName;
        leftCulprit->GetGeneric()->GetBasicRep(this->m_CompilerHost->GetCompiler(), Container, &FullGenericName);
        ReportErrorOnSymbol(WRNID_VarianceDeclarationAmbiguous3, CurrentErrorLog(Container), leftDeclaration,
                GetQualifiedErrorName(leftDeclaration->GetRoot()),
                GetQualifiedErrorName(rightDeclaration->GetRoot()),
                FullGenericName.GetString());
    }


}



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////





// Returns True only if Class Or Interface
// Returns False if Module or Structure of Enum or any other non-Class
//
bool
Bindable::IsClassOrInterface(BCSYM *Container)
{
    return (IsClass(Container) || IsInterface(Container));
}


bool
Bindable::IsClass
(
    BCSYM *Container
)
{
    return (Container &&
                (Container->IsClass() &&
                 !Container->PClass()->IsStdModule() &&
                 !Container->PClass()->IsStruct() &&
                 !Container->PClass()->IsEnum() &&
                 !Container->PClass()->IsDelegate()));
}

bool
Bindable::IsInterface
(
    BCSYM *Container
)
{
    return (Container && Container->IsInterface());
}

bool
Bindable::IsStdModule
(
    BCSYM *Container
)
{
    return Container &&
           Container->IsClass() &&
           Container->PClass()->IsStdModule();
}

bool
Bindable::IsUnnamedNamespace
(
    BCSYM *PossibleNamespace,
    Compiler *CompilerInstance
)
{
    bool UnnamedNamespace =
        PossibleNamespace &&
        PossibleNamespace->IsNamespace() &&
        StringPool::IsEqual(
            PossibleNamespace->PNamespace()->GetName(),
            STRING_CONST(CompilerInstance, EmptyString));

    VSASSERT(!UnnamedNamespace ||
             !PossibleNamespace->PNamespace()->GetContainer(),
                "Unnamed Namespace expected to be at the root of the symbol hierarchy!!!");

    return UnnamedNamespace;
}

void
Bindable::ResolveBaseForClass
(
    BCSYM_Class *Class
)
{
    for(BCSYM_Class *PartialType = Class;
        PartialType;
        PartialType = PartialType->GetNextPartialType()->PClass())
    {
        BCSYM *BaseClass = PartialType->GetRawBase();

        if (!BaseClass)
        {
            continue;
        }

        // Resolve the named type of the base
        if (BaseClass->IsNamedType())
        {
            ResolveBaseType(BaseClass->PNamedType());

            BCSYM *BoundBaseClass = BaseClass->DigThroughNamedType();

            if (IsClass(BoundBaseClass) &&
                (PossibleInheritanceCycleDetected() ||
                    GetStatusOfResolveBases(BoundBaseClass->PClass()) == InProgress))
            {
                SetCurrentBase(BaseClass->PNamedType());

                // Check for cycles on the current container after resolving a base
                //
                if (DoesBaseCauseInheritanceCycle(Class, BoundBaseClass->PClass(), BaseClass->GetLocation()))
                {
                    // Break the cycle by marking the Container as having cycles so that
                    // other folks don't loop infinitely over the bases
                    //
                    Class->SetBaseInvolvedInCycle(true);
                }

                SetCurrentBase(NULL);
            }
        }
    }

    // Note that this call might change the raw base of the class itself sometine
    // for partial types. So after this call, the base should be got again from the
    // class symbol.
    //
    DoClassInheritanceValidationNotRequiringOtherBoundBasesInfo(Class);

    // Make sure that the base is also resolved
    // This recursive call makes sure that all the bases up the
    // inheritance hierarchy are also processed completely.
    //
    BCSYM *BoundBaseClass = Class->GetBaseClass();
    BCSYM *BaseClass = Class->GetRawBase();

    if (BoundBaseClass && !BoundBaseClass->IsBad())
    {
        if (BaseClass->IsNamedType())
        {
            SetCurrentBase(BaseClass->PNamedType());

            VSASSERT( BoundBaseClass->IsClass(),
                        "Validate should have marked non-class inherits as bad!!!");

            ResolveBasesForContainer(BoundBaseClass->PClass(), m_CompilationCaches);

            // This class's bases could itself get triggered off through its parents,
            // after we break the circular parent - nested type dependencies.
            // If this Class/Interface has already been processed for this, return
            // else continue on and complete the task now.
            // The parent - nested type cycle detection algorithm is kind of reentrant,
            // so...
            //
            if (GetStatusOfResolveBases() == Done)
            {
                return;
            }

            SetCurrentBase(NULL);
        }
        else
        {
            VSASSERT( BoundBaseClass->IsContainer() && DefinedInMetaData(BoundBaseClass->PContainer()),
                        "How can a non-metadata class be set as the default base for any class ?");

            // If ever for any reason this restriction is gone, then the Parent cycle detection which
            // depends on the CurrentBase being set to a named type will have to be changed.
        }

        // Note that this needs to be invoked only after inheritance cycle detection for the class
        // being validated is completed because these checks walk inheritance chains.
        //
        DoClassInheritanceValidationRequiringOtherBoundBasesInfo(Class);
    }
}

void
Bindable::ResolveBasesForInterface
(
    BCSYM_Interface *Interface
)
{
    BCSYM_Implements *HeadOfBasesList = Interface->GetFirstImplements();

    for (BCSYM_Implements *CurrentBase = HeadOfBasesList;
         CurrentBase;
         CurrentBase = CurrentBase->GetNext())
    {
        BCSYM *BaseInterface = CurrentBase->GetRawRoot();

        if (!BaseInterface)
        {
            continue;
        }

        if (BaseInterface->IsNamedType())
        {
            ResolveBaseType(BaseInterface->PNamedType());

            BCSYM *BoundBaseInterface = BaseInterface->DigThroughNamedType();

            if (IsInterface(BoundBaseInterface) &&
                (PossibleInheritanceCycleDetected() ||
                    GetStatusOfResolveBases(BoundBaseInterface->PInterface()) == InProgress))
            {
                SetCurrentBase(CurrentBase->GetRawRoot()->PNamedType());

                // Check for cycles on the current container after resolving a base
                //
                if (DoesBaseCauseInheritanceCycle(Interface, BoundBaseInterface->PInterface(), CurrentBase->GetLocation()))
                {
                    // Break the cycle by marking the Container as having cycles so that
                    // other folks don't loop infinitely over the bases
                    //
                    CurrentBase->SetIsBad();
                }

                SetCurrentBase(NULL);
            }
        }
    }

    // validate base interfaces here
    ValidateInterfaceInheritance(Interface);

    for (BCSYM_Implements *CurrentBase = HeadOfBasesList;
         CurrentBase;
         CurrentBase = CurrentBase->GetNext())
    {
        if (CurrentBase->IsBad())
            continue;

        BCSYM *BoundBaseInterface = CurrentBase->GetRoot();

        if (!BoundBaseInterface || BoundBaseInterface->IsGenericBadNamedRoot())
        {
            continue;
        }

        SetCurrentBase(CurrentBase->GetRawRoot()->PNamedType());

        VSASSERT( BoundBaseInterface->IsInterface(),
                    "Validation should have marked non-interface inherits as bad!!!");

        ResolveBasesForContainer(BoundBaseInterface->PInterface(), m_CompilationCaches);

        // This class's bases could itself get triggered off through its parents,
        // after we break the circular parent - nested type dependencies.
        // If this Class/Interface has already been processed for this, return
        // else continue on and complete the task now.
        // The parent - nested type cycle detection algorithm is kind of reentrant,
        // so...
        //
        if (GetStatusOfResolveBases() == Done)
        {
            return;
        }

        SetCurrentBase(NULL);
    }
}

bool
Bindable::DoesBaseCauseInheritanceCycle
(
    BCSYM_Container *Container,
    BCSYM_Container *Base,
    Location *InheritsLocation
)
{
    DynamicArray<BCSYM_Container *> ContainersInCycle;

    Location *Location = NULL;

    if (DetectInheritanceCycle(
            Container,
            Base,
            &ContainersInCycle))
    {
        // This Container is the node at which the cycle was detected
        VSASSERT( ContainersInCycle.Element(0)->IsSameContainer(Container),
                        "Cycle detected - but the start and end of the cycle are not the same !!!");

        ContainersInCycle.AddElement(Container);

        // Report Cycles Error
        ReportInheritanceCyclesError(
            &ContainersInCycle,
            InheritsLocation);

        // Note: Don't mark this bad since we want this Container to still go through the
        // rest of bindable and also for intellisense, etc. to show up for it.
        // The cycle is broken in DetectInheritanceCycle

        // Cycle Present
        return true;
    }

    return false;
}


bool
Bindable::DetectInheritanceCycle
(
    BCSYM_Container *ContainerToVerify,
    BCSYM_Container *BaseContainer,
    DynamicArray<BCSYM_Container *> *ContainersInCycle
)
{
    if (ContainerToVerify->IsSameContainer(BaseContainer))
    {
        ContainersInCycle->AddElement(BaseContainer);

        return true;
    }

    BasesIter Bases(BaseContainer);

    for (BCSYM_Container *Base = Bases.GetNextBase();
         Base;
         Base = Bases.GetNextBase())
    {
        if (Base->IsBad())
        {
            continue;
        }

        if (DetectInheritanceCycle(ContainerToVerify, Base, ContainersInCycle))
        {
            ContainersInCycle->AddElement(BaseContainer);

            return true;
        }
    }

    return false;
}


void
Bindable::ReportInheritanceCyclesError
(
    DynamicArray<BCSYM_Container *> *ClassesInCycle,
    Location *Location
)
{
    int Count = ClassesInCycle->Count();
    STRING *Derived, *Base;

    VSASSERT( Count >= 2, "How can we have a cycle here ?");

    BCSYM_Container *ErrorRoot = ClassesInCycle->Element(0);
    StringBuffer CycleDesc;

    // Append the Inheritance path followed to detect the cycle
    for (int Index = Count - 1;
         Index >= 1;
         Index--)
    {
        Derived = ClassesInCycle->Element(Index)->GetName();
        Base = ClassesInCycle->Element(Index - 1)->GetName();

        ResLoadStringRepl(
            ERRID_InheritsFrom2,
            &CycleDesc,
            Derived,
            Base);
    }

    VSASSERT(CurrentContainer() == ClassesInCycle->Element(0),
                "Unexpected container as cycle originator!!!");

    // Initialize with the error specifying the error node at which the cycle was detected
    ReportErrorAtLocation(
        IsInterface(ClassesInCycle->Element(0)) ? ERRID_InterfaceCycle1 : ERRID_InheritanceCycle1,
        CurrentErrorLog(GetCurrentBase()),
        Location,
        ErrorRoot->GetName(),
        CycleDesc.GetString());
}


// this is an entry point for BCSYM_Class And BCSYM_Interface to Resolve bases on demand
void
Bindable::ResolveBasesIfNotStarted
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches
)
{
    if (DefinedInMetaData(Container) ||
        Container->IsBindingDone())
    {
        return;
    }

#if IDE 
    if (!IsInCompilationThread(Container->GetCompiler()))
    {
        return;
    }
#endif IDE

    // 






    VSASSERT( !Container->GetSourceFile() ||
              Container->GetSourceFile()->GetCompState() >= CS_Bound ||
              (Container->GetSourceFile()->GetCompState() == (CS_Bound - 1) &&
                    Container->GetSourceFile()->GetProject()->GetCompState() == (CS_Bound - 1)),
                    "This should only be invoked after this file has reached bound or when transitioning from previous state to Bound!!!");

    if (Container->GetMainType())
    {
        Container = Container->GetMainType();
    }

    if (Container->GetBindableInstance()->GetStatusOfResolveBases() == NotStarted)
    {
        Bindable::ResolveBasesForContainer(Container, IDECompilationCaches);
    }
}

Bindable::TaskTriState
Bindable::GetStatusOfResolveBases
(
    BCSYM_Container *Container
)
{
  // Metadata containers not allowed in here
    if (DefinedInMetaData(Container) ||
        Container->IsBindingDone())
    {
        return Done;
    }

    return Container->GetBindableInstance()->GetStatusOfResolveBases();
}


// Entry point for Obsoletechecker
// IsContainerReadyForObsoleteChecking here will only return true
// if the current Container Context's Attributes have already been ----ed.
// This is a helper for obsolete checker to avoid circular dependencies when
// doing obsolete checking and attribute cracking might be required on other
// containers on demand when doing name binding for earlier stages
//
#if DEBUG
bool
Bindable::IsContainerReadyForObsoleteChecking
(
    BCSYM_Container *Container
)
{
    // Metadata containers are always ready for Obsolete checking
    // because there will not be any circular dependencies when
    // processing metadata containers.
    //
    if (DefinedInMetaData(Container) ||
        // Yes, obsolete checker gets Conditional Compilation constants
        // too for obsolete checking!!!
        Container->IsCCContainer())
    {
        return true;
    }

    BCSYM_Container *ContainerToCheck;
    if (Container->IsPartialTypeAndHasMainType())
    {
        ContainerToCheck = Container->GetMainType();
    }
    else
    {
        ContainerToCheck = Container;
    }

    if (ContainerToCheck &&
        (ContainerToCheck->IsBindingDone() ||
            ContainerToCheck->GetBindableInstance()->GetStatusOf----AttributesOnAllSymbolsInContainer() ==
                Done))
    {
        return true;
    }

    return false;
}
#endif DEBUG

void
Bindable::ResolveNamedType
(
    BCSYM_NamedType *NamedType,
    TypeResolutionFlags Flags,
    NameFlags NameInterpretFlags,
    bool DisableNameLookupCaching
)
{
    ResolveNamedType(
        NamedType,
        CurrentAllocator(),
        CurrentErrorLog(NamedType->GetContext()),
        CurrentCompilerInstance(),
        CurrentCompilerHost(),
        CurrentSourceFile(NamedType->GetContext()),
        // Don't perform obsolete checks. Will do the
        // obsolete checking for the whole container later.
        Flags & ~TypeResolvePerformObsoleteChecks,
        m_CompilationCaches,
        NameInterpretFlags,
        DisableNameLookupCaching);
}

void
Bindable::ResolveAllNamedTypesForContainer
(
    TypeResolutionFlags Flags      // [in] indicates how to resolve the type
)
{
    if (GetStatusOfResolveNameTypes() == Done)
    {
        return;
    }

    // This could happen if there are structure fields in current container
    // and we are inturn resolving named types in that structure.
    //
    if (GetStatusOfResolveNameTypes() == InProgress)
    {
        SetStructureMemberCycleDetected(true);
        return;
    }

    // Mark the current container indicating that work is in progress to resolve its named types
    SetStatusOfResolveNamedTypes(InProgress);

    BCSYM_Container *Container = CurrentContainer();

    // Resolve all named types in this container
    //
    do
    {
        for(BCSYM_NamedType *NamedType = Container->GetNamedTypesList();
            NamedType;
            NamedType = NamedType->GetNext())
        {
            ResolveNamedType(NamedType, TypeResolveNoFlags);
        }
    } while (Container = Container->GetNextPartialType());

    // Validate the generic parameters if any. They need to be validated this
    // early because other entities could potentially traverse the constraints
    // after this when they in turn validate constraints, etc.

    // Check that any specified constraints on type parameters of the container are valid.
    // Note that it is advantageous to do this task earlier here than later in shadowing
    // semantics so that we could possibly avoid checking constraint against bad constraint
    // types.
    //
    // Note that it is imperative that this checking be done here in order to break cycles
    // between type parameter constraints.
    //
    ValidateGenericParameters(CurrentContainer());

    // Note that it is imperative that this checking be done herein order to break cycles
    // between type parameter constraints.
    //
    BCITER_CHILD_ALL Members(CurrentContainer());
    while (BCSYM_NamedRoot *Member = Members.GetNext())
    {
        if (Member->IsProc())
        {
            ValidateGenericParameters(Member->PProc());
        }
    }

    // Resolve the named types for all the delegates nested within the current container
    // This is needed because delegate clashes with other members, etc. which show the
    // signature of clashing member in the error message will need the named types resolved.
    //
    // Given that sometimes we treat delegates less as containers and more as methods, this
    // is necessary. Eg: reporting errors, etc.,
    //
    Container = CurrentContainer();
    do
    {
        for (BCSYM_Container *PossibleDelegate = Container->GetNestedTypes();
            PossibleDelegate;
            PossibleDelegate = PossibleDelegate->GetNextNestedType())
        {
            if (!PossibleDelegate->IsDelegate())
            {
                continue;
            }

            ResolveAllNamedTypesForContainer(PossibleDelegate, m_CompilationCaches);
        }

    } while (Container = Container->GetNextPartialType());


    // check structure layout cycles
    //
    CheckStructureMemberCycles();

    ReTypeEnumMembersIfNecessary();
    
#if IDE 
    if (CurrentContainer()->GetSourceFile()->GetProject()->GenerateENCableCode())
    {
        GenerateENCTrackingCode();
        GenerateENCHiddenRefreshCode();
    }
#endif IDE

    // Mark the current container indicating that work to resolve its named types is done
    SetStatusOfResolveNamedTypes(Done);
}

#if IDE

void
Bindable::GenerateENCTrackingCode()
{
    if (!IsClass(CurrentContainer()))
    {
        return;
    }

    // Do not generate these all the time.
    // If no accessible withevents or events in class or bases, then
    // don't generate.
    BCSYM_Class *Class = CurrentContainer()->PClass();
    if (!Class || Class->IsMustInherit())
    {
        return;
    }

    Compiler *CompilerInstance = CurrentCompilerInstance();

    bool fHasEvents = false;
    BCSYM_Class *BaseClass = Class;
    do
    {
        BCITER_CHILD iter;
        iter.Init(BaseClass, false, true);
        BCSYM_NamedRoot *Named;
        while ((Named = iter.GetNext()) != NULL)
        {
            if ((Named->IsEventDecl() || Named->IsVariable() && Named->PVariable()->IsWithEvents()) && IsTypeAccessible(Named, NULL, Class))
            {
                fHasEvents = true;
                break;
            }
        }
        if (fHasEvents)
        {
            break;
        }

        BaseClass = BaseClass->GetCompilerBaseClass();
    }
    while (BaseClass && !BaseClass->IsMustInherit());

    if (!fHasEvents)
    {
        return;
    }

    // Generate field
    //
    // Private Shared __ENCList As New ArrayList
    // or
    // Private Shared __ENCList As New System.Collections.Generic.List(Of WeakReference)

    Symbols SymbolAllocator(CompilerInstance, CurrentAllocator(), NULL);

    const DECLFLAGS CommonFlags =
        DECLF_ShadowsKeywordUsed | DECLF_Private | DECLF_Hidden | DECLF_Bracketed;

    Type* TypeOfENCTrackingList = DetermineTypeOfENCTrackingList(
        &SymbolAllocator);
    if (!TypeOfENCTrackingList)
    {
        VSASSERT(false, "Class libraries does not support WeakRefernece and (ArrayList or List(Of WeakReference))");
        return;
    }

    // Alloc and fill in the variable
    BCSYM_Variable *Variable = SymbolAllocator.AllocVariable(false, true);

    SymbolAllocator.GetVariable(
        NULL,                                               // no location
        STRING_CONST(CompilerInstance, ENCTrackingList),    // Name
        STRING_CONST(CompilerInstance, ENCTrackingList),    // Emitted Name
        CommonFlags | DECLF_Shared | DECLF_Dim | DECLF_New,
        VAR_Member,
        TypeOfENCTrackingList,
        NULL,
        NULL,
        Variable);

    Variable->SetIsENCTrackingList();

    // Generate method
    //
    // Private Shared Sub __ENCIterate()

    // Alloc and fill in the proc

    BCSYM_SyntheticMethod *Iterator = SymbolAllocator.AllocSyntheticMethod(false);

    SymbolAllocator.GetProc(
        NULL,                                                       // no location
        STRING_CONST(CompilerInstance, ENCTrackingListIterator),    // Name
        STRING_CONST(CompilerInstance, ENCTrackingListIterator),    // Emitted Name
        NULL,
        NULL,
        CommonFlags | DECLF_Shared,
        NULL,   // Sub
        NULL,   // No params
        NULL,
        NULL,   // no lib name
        NULL,   // no alias name
        SYNTH_IterateENCTrackingList,
        NULL,   // no generic params
        NULL,
        Iterator);

    // Generate method
    //
    // Private Shared Sub __ENCAddToList(ByVal Value As Object)

    BCSYM_Param *ParamFirst = NULL, *LastParam = NULL;

    SymbolAllocator.GetParam( NULL, // no location
        STRING_CONST(CompilerInstance, ValueParam), // Name
        CurrentCompilerHost()->GetFXSymbolProvider()->GetObjectType(), // Type
        NULL,
        NULL,
        &ParamFirst,
        &LastParam,
        false);  // Not a return type param

    BCSYM_SyntheticMethod *AddToList = SymbolAllocator.AllocSyntheticMethod(false);

    SymbolAllocator.GetProc(
        NULL,                                                       // no location
        STRING_CONST(CompilerInstance, ENCTrackingListAddTo),    // Name
        STRING_CONST(CompilerInstance, ENCTrackingListAddTo),    // Emitted Name
        NULL,
        NULL,
        CommonFlags | DECLF_Shared,
        NULL,   // Sub
        ParamFirst,
        NULL,
        NULL,   // no lib name
        NULL,   // no alias name
        SYNTH_AddToENCTrackingList,
        NULL,   // no generic params
        NULL,
        AddToList);

    // Generate method
    //
    // Private Sub __ENCUpdateHandlers

    // Alloc and fill in the proc

    BCSYM_SyntheticMethod *UpdateHandler = SymbolAllocator.AllocSyntheticMethod(false);

    SymbolAllocator.GetProc(
        NULL,                                                       // no location
        STRING_CONST(CompilerInstance, ENCUpdateHandlers),    // Name
        STRING_CONST(CompilerInstance, ENCUpdateHandlers),    // Emitted Name
        NULL,
        NULL,
        CommonFlags,
        NULL,   // Sub
        NULL,   // No params
        NULL,
        NULL,   // no lib name
        NULL,   // no alias name
        SYNTH_ENCUpdateHandler,
        NULL,   // no generic params
        NULL,
        UpdateHandler);


    // Add the symbols to the the container's bindable members hash

    Symbols::AddSymbolToHash(
        Class->GetHash(),
        Variable,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

    Symbols::AddSymbolToHash(
        Class->GetHash(),
        Iterator,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

    Symbols::AddSymbolToHash(
        Class->GetHash(),
        AddToList,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

    Symbols::AddSymbolToHash(
        Class->GetHash(),
        UpdateHandler,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

#if IDE 

    // Add these to the list of symbols created in bindable.
    // This list is used to delete these from their respective hashes during decompilation
    // from bindable.
    //
    SymbolAllocator.GetAliasOfSymbol(
        Variable,
        ACCESS_Private,
        Class->GetBindableSymbolList());

    SymbolAllocator.GetAliasOfSymbol(
        Iterator,
        ACCESS_Private,
        Class->GetBindableSymbolList());

    SymbolAllocator.GetAliasOfSymbol(
        AddToList,
        ACCESS_Private,
        Class->GetBindableSymbolList());

    SymbolAllocator.GetAliasOfSymbol(
        UpdateHandler,
        ACCESS_Private,
        Class->GetBindableSymbolList());
#endif

    // Make sure a shared constructor is present, else synthesize one
    //
    // This is needed so that the shared ArrayList variable can be initialized.
    //
    const BuildSharedConstructor = true;
    if (!Class->GetSharedConstructor(CurrentCompilerInstance()))
    {
        // Synthesize one
        SynthesizeConstructor(BuildSharedConstructor);
    }
}

Type*
Bindable::DetermineTypeOfENCTrackingList
(
    Symbols* SymbolAllocator
)
{
    // Get and verify WeakReferenceSymbol.
    ClassOrRecordType* WeakReferenceSymbol = GetAndVerifyCOMClassSymbol(FX::WeakReferenceType);
    if (WeakReferenceSymbol == NULL)
    {
        return NULL;
    }

    // Get and verify List(Of T)
    ClassOrRecordType* GenericListSymbol = GetAndVerifyCOMClassSymbol(FX::GenericListType);
    ClassOrRecordType* ArrayListSymbol = GetAndVerifyCOMClassSymbol(FX::ArrayListType);
    if (GenericListSymbol != NULL &&
        // Always pick GenericList unless we have ArrayList, wich is NOT obsolete and GenericList is obsolete.
        !(ArrayListSymbol != NULL && !ArrayListSymbol->IsObsolete() && GenericListSymbol->IsObsolete()))
    {
        GenericBinding *ListOfWeakReferenceOpenBinding = SynthesizeOpenGenericBinding(
            GenericListSymbol,
            *SymbolAllocator);
        ListOfWeakReferenceOpenBinding->GetArguments()[0] = WeakReferenceSymbol;

        return SymbolAllocator->GetGenericBinding(
            false,
            GenericListSymbol,
            ListOfWeakReferenceOpenBinding->GetArguments(),
            ListOfWeakReferenceOpenBinding->GetArgumentCount(),
            NULL);
    }
    else
    {
        return GetAndVerifyCOMClassSymbol(FX::ArrayListType);
    }
}

ClassOrRecordType*
Bindable::GetAndVerifyCOMClassSymbol
(
    FX::TypeName Symbol
)
{
    ClassOrRecordType* ClassType = NULL;
    if (CurrentCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(Symbol))
    {
        Type* Type = CurrentCompilerHost()->GetFXSymbolProvider()->GetType(Symbol);

        Assume(Type->IsClass(), L"Must be a Class");
        ClassType = Type->PClass();

        if (ClassType && !IsAccessible(ClassType))
        {
            ClassType = NULL;
        }
    }

    return ClassType;
}

void
Bindable::GenerateENCHiddenRefreshCode
(
)
{
    if (!IsClass(CurrentContainer()) &&
        !IsStructure(CurrentContainer()) &&
        !IsStdModule(CurrentContainer()))
    {
        return;
    }

    BCSYM_Class *Class = CurrentContainer()->PClass();
    if (Class->IsMustInherit() && !Class->IsStdModule())
    {
        return;
    }

    Compiler *CompilerInstance = CurrentCompilerInstance();

    Symbols SymbolAllocator(CompilerInstance, CurrentAllocator(), NULL);

    const DECLFLAGS CommonFlags =
        DECLF_ShadowsKeywordUsed | DECLF_Private | DECLF_Hidden | DECLF_Bracketed;

    // Generate method
    //
    // Private Sub __ENCHiddenRefresh

    if (!IsStdModule(Class))
    {
        // Alloc and fill in the proc
        BCSYM_SyntheticMethod *HiddenRefresh = SymbolAllocator.AllocSyntheticMethod(false);

        SymbolAllocator.GetProc(
            NULL,                                                       // no location
            STRING_CONST(CompilerInstance, ENCHiddenRefresh),    // Name
            STRING_CONST(CompilerInstance, ENCHiddenRefresh),    // Emitted Name
            NULL,
            NULL,
            CommonFlags,
            NULL,   // Sub
            NULL,   // No params
            NULL,
            NULL,   // no lib name
            NULL,   // no alias name
            SYNTH_ENCHiddenRefresh,
            NULL,   // no generic params
            NULL,
            HiddenRefresh);


        // Add the symbols to the the container's bindable members hash
        Symbols::AddSymbolToHash(
            Class->GetHash(),
            HiddenRefresh,
            true,       // set the parent of this member
            Class->IsStdModule(),
            false);     // container is not namespace

#if IDE 
        SymbolAllocator.GetAliasOfSymbol(
            HiddenRefresh,
            ACCESS_Private,
            Class->GetBindableSymbolList());
#endif
    }

    //
    // Private Sub __ENCSharedHiddenRefresh

    // Alloc and fill in the proc
    BCSYM_SyntheticMethod *SharedHiddenRefresh = SymbolAllocator.AllocSyntheticMethod(false);

    SymbolAllocator.GetProc(
        NULL,                                                       // no location
        STRING_CONST(CompilerInstance, ENCSharedHiddenRefresh),    // Name
        STRING_CONST(CompilerInstance, ENCSharedHiddenRefresh),    // Emitted Name
        NULL,
        NULL,
        CommonFlags | DECLF_Shared,
        NULL,   // Sub
        NULL,   // No params
        NULL,
        NULL,   // no lib name
        NULL,   // no alias name
        SYNTH_ENCSharedHiddenRefresh,
        NULL,   // no generic params
        NULL,
        SharedHiddenRefresh);


    // Add the symbols to the the container's bindable members hash
    Symbols::AddSymbolToHash(
        Class->GetHash(),
        SharedHiddenRefresh,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

#if IDE 
    SymbolAllocator.GetAliasOfSymbol(
        SharedHiddenRefresh,
        ACCESS_Private,
        Class->GetBindableSymbolList());
#endif
}

#endif IDE

void
Bindable::ValidateGenericParameters
(
    BCSYM_Container *MainContainer
)
{
    // Check that for every partial of this container, for every generic parameter, it has the same
    // name and constraints. "Same constraints" means set-equality on the set of constraints, as implemented
    // by CompareConstraints(...).
    
    // The rest of the compiler assumes that the "Main Container" for a class (i.e. the one passed as
    // argument to this function) will be populated by this function with a complete list of classes constraints.
    // If the spec allowed each partial to declare different constraints, then this function would need
    // to gather them together or copy them into the Main Container. It would also need a flag by which it
    // knows that they're generated constraints that should be cleared when the class is decompiled.
    //
    // But, things are simpler thanks to Spec$7.11: "Partial types with type parameters can declare constraints
    // on the type parameters, but the constraints from each partial declaration must match. Thus, constraints
    // are special in that they are not automatically combined like other modifiers."

    for (BCSYM_Container *ContainerToCheck = MainContainer; ContainerToCheck; ContainerToCheck = ContainerToCheck->GetNextPartialType())
    {
        bool ConstraintsFound; ValidateGenericParamsAndDirectConstraints(ContainerToCheck->GetFirstGenericParam(), ConstraintsFound);
        if (ContainerToCheck == MainContainer) continue;
        BCSYM_Container *SecondaryContainer = ContainerToCheck;

        // Now check, for each generic param, that its names and constraints match.
        for (BCSYM_GenericParam *MainParam = MainContainer->GetFirstGenericParam(), *SecondaryParam = SecondaryContainer->GetFirstGenericParam();
             MainParam && SecondaryParam;
             MainParam = MainParam->GetNextParam(), SecondaryParam = SecondaryParam->GetNextParam())
        {
            bool NamesMatch = StringPool::IsEqual(MainParam->GetName(), SecondaryParam->GetName());
            if (!NamesMatch)
            {
                // Type parameter name '|1' does not match the name '|2' of the corresponding type
                // parameter defined on one of the other partial types of '|3'.
                ReportErrorOnSymbol(
                    ERRID_PartialTypeTypeParamNameMismatch3,
                    CurrentErrorLog(SecondaryParam),
                    SecondaryParam,
                    SecondaryParam->GetName(),
                    MainParam->GetName(),
                    SecondaryContainer->GetName());
            }

            bool ConstraintsMatch = CompareConstraints(MainParam, NULL, SecondaryParam, NULL, NULL);
            if (!ConstraintsMatch)
            {
                // Constraints for this type parameter do not match the constraints on the corresponding
                // type parameter defined on one of the other partial types of '|1'.
                ReportErrorOnSymbol(
                    ERRID_PartialTypeConstraintMismatch1,
                    CurrentErrorLog(SecondaryParam),
                    SecondaryParam,
                    SecondaryContainer->GetName());
            }
        }
    }

    // The following function expects MainContainer to have the complete set of constraints:
    //
    ValidateIndirectConstraintsForGenericParams(
        MainContainer->GetFirstGenericParam(),
        CurrentErrorLog(MainContainer));
}

void
Bindable::ValidateGenericParameters
(
    BCSYM_Proc *Generic
)
{
    if (!Generic->IsGeneric())
    {
        return;
    }

    bool ConstraintsFound;

    ValidateGenericParamsAndDirectConstraints(
        Generic->GetFirstGenericParam(),
        ConstraintsFound);

    ValidateIndirectConstraintsForGenericParams(
        Generic->GetFirstGenericParam(),
        CurrentErrorLog(Generic));

    // VB Variance Spec $2: no members can be declared variant
    for (GenericParameter *param=Generic->GetFirstGenericParam(); param; param=param->GetNextParam())
    {
        VSASSERT(param, "unexpected NULL param");
        if (param->IsBad())
        {
            continue;
        }
        if (param->GetVariance()!=Variance_None)
        {
            // Only interface and delegate types can be specified as variant
            ReportErrorOnSymbol(ERRID_VarianceDisallowedHere, CurrentErrorLog(param), param);
            param->SetIsBadVariance();
        }
    }
}

void
Bindable::ValidateGenericParamsAndDirectConstraints
(
    BCSYM_GenericParam *ParameterList,
    bool &ConstraintsFound
)
{
    ConstraintsFound = false;

    ValidateNameShadowingForGenericParams(ParameterList);

    ValidateDirectConstraintsForGenericParams(ParameterList, ConstraintsFound);
}

void
Bindable::ValidateNameShadowingForGenericParams
(
    BCSYM_GenericParam *ParameterList
)
{
    for (BCSYM_GenericParam *Parameter = ParameterList; Parameter; Parameter = Parameter->GetNextParam())
    {
        BCSYM_NamedRoot *ParameterParent = Parameter->GetParent();

        // Skip name clash validation for the partial components of a type in order to avoid redundant errors.
        //
        if (ParameterParent->IsClass() && ParameterParent->PClass()->IsPartialType())
        {
            continue;
        }

        // Check that this parameter does not have the same name as a parameter of an enclosing generic type.
        //
        for (BCSYM_NamedRoot *Parent = ParameterParent->GetParent(); Parent->IsType(); Parent = Parent->GetParent())
        {
            BCSYM_Hash *ParamsHash = Parent->GetGenericParamsHash();

            if (ParamsHash && ParamsHash->SimpleBind(Parameter->GetName()))
            {
                ReportErrorAtLocation(
                    WRNID_ShadowingGenericParamWithParam1,
                    CurrentErrorLog(Parameter),
                    Parameter->GetLocation(),
                    Parameter->GetName());
            }
        }

        if (!ParameterParent->IsContainer())
        {
            continue;
        }

        // Check that no member of the type has the same name as this parameter.

        BCSYM_NamedRoot *TypeMember = ParameterParent->PContainer()->GetHash()->SimpleBind(Parameter->GetName());

        if (TypeMember)
        {
            BCSYM_NamedRoot *SourceOfTypeMember = NULL;
            if (!IsSynthetic(TypeMember, &SourceOfTypeMember))
            {
                ReportErrorAtLocation(
                    ERRID_ShadowingGenericParamWithMember1,
                    CurrentErrorLog(TypeMember),
                    TypeMember->GetLocation(),
                    TypeMember->GetName());
            }
            else
            {
                VSASSERT(SourceOfTypeMember,
                            "How can a synthetic member not have a source member ?");

                ReportErrorAtLocation(
                    ERRID_SyntMemberShadowsGenericParam3,
                    CurrentErrorLog(SourceOfTypeMember),
                    SourceOfTypeMember->GetLocation(),
                    StringOfSymbol(CurrentCompilerInstance(), SourceOfTypeMember),
                    SourceOfTypeMember->GetName(),
                    TypeMember->GetName());
            }
        }
    }
}

void
Bindable::ValidateDirectConstraintsForGenericParams
(
    BCSYM_GenericParam *ParameterList,
    bool &ConstraintsFound
)
{
    CompilerHost *CompilerHost = CurrentSourceFile(CurrentContainer())->GetCompilerHost();

    bool DisallowRedundancy = true;

    if (ParameterList)
    {
        // Bug VSWhidbey 365493.
        //
        // Overriding methods and Private methods that implement interface methods are allowed
        // to have redundancy and special types in constraints.

        BCSYM_NamedRoot *Parent = ParameterList->GetParent();

        if (Parent->IsProc() &&
            (Parent->PProc()->IsOverridesKeywordUsed() ||
             (Parent->PProc()->GetAccess() == ACCESS_Private &&
              Parent->PProc()->GetImplementsList() != NULL)))
        {
            DisallowRedundancy = false;
        }
    }

    for (BCSYM_GenericParam *Parameter = ParameterList; Parameter; Parameter = Parameter->GetNextParam())
    {
        // Validate that the constraints on the parameter are well formed.

        VSASSERT( !DefinedInMetaData(CurrentContainer()), "Metadata container unexpected!!!");

        bool HasReferenceConstraint = false;
        bool HasValueConstraint = false;
        BCSYM *ClassConstraintType = NULL;

        for (BCSYM_GenericConstraint *Constraint = Parameter->GetConstraints(); Constraint; Constraint = Constraint->Next())
        {
            ConstraintsFound = true;

            // Reset the badness of the constraint. We don't reset this during
            // decompilation, instead just reset it here.
            //
            Constraint->SetIsBadConstraint(false);

            ValidateDirectConstraint(
                Constraint,
                Parameter,
                CompilerHost,
                HasReferenceConstraint,
                HasValueConstraint,
                ClassConstraintType,
                DisallowRedundancy);
        }
    }
}

// Check the given direct constraint for clashes with other constraints specified directly on the type
// parameter given by "Parameter".
//
void
Bindable::ValidateDirectConstraint
(
    BCSYM_GenericConstraint *Constraint,
    BCSYM_GenericParam *Parameter,
    CompilerHost *CompilerHost,
    bool &HasReferenceConstraint,
    bool &HasValueConstraint,
    BCSYM *&ClassConstraintType,
    bool DisallowRedundancy
)
{
    VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

    // !DirectConstraintCompare:
    // Constraint clashes being checked among constraints of which atleast one is specified on another type parameter which
    // has either directly or indirect been supplied as a constraint to the type parameter given by "Parameter".

    ErrorTable *ErrorLog = CurrentErrorLog(Parameter);

    if (Constraint->IsReferenceConstraint())
    {
        if (ClassConstraintType && DisallowRedundancy)
        {
            ReportErrorAtLocation(
                ERRID_RefAndClassTypeConstrCombined,
                ErrorLog,
                Constraint->GetLocation());

            Constraint->SetIsBadConstraint(true);
        }
        else
        {
            HasReferenceConstraint = true;
        }
    }
    else if (Constraint->IsValueConstraint())
    {
        if (ClassConstraintType && DisallowRedundancy)
        {
            ReportErrorAtLocation(
                ERRID_ValueAndClassTypeConstrCombined,
                ErrorLog,
                Constraint->GetLocation());

            Constraint->SetIsBadConstraint(true);
        }
        else
        {
            HasValueConstraint = true;
        }
    }

    if (!Constraint->IsGenericTypeConstraint())
    {
        return;
    }

    BCSYM *ConstraintType = Constraint->PGenericTypeConstraint()->GetType();

    if (!ConstraintType || ConstraintType->IsBad())
    {
        Constraint->SetIsBadConstraint(true);
        return;
    }

    for(BCSYM_GenericTypeConstraint *ExistingConstraint = Parameter->GetTypeConstraints();
        ExistingConstraint != Constraint;
        ExistingConstraint = ExistingConstraint->Next())
    {
        BCSYM *ExistingConstraintType = ExistingConstraint->GetType();

        if (!ExistingConstraintType || ExistingConstraintType->IsBad())
        {
            continue;
        }

        if (BCSYM::AreTypesEqual(ConstraintType, ExistingConstraintType))
        {
            if (ErrorLog)
            {
                StringBuffer ConstraintBuffer;

                ReportErrorAtLocation(
                    ERRID_ConstraintAlreadyExists1,
                    ErrorLog,
                    Constraint->GetLocation(),
                    ErrorLog->ExtractErrorName(ConstraintType, CurrentContainer(), ConstraintBuffer));
            }

            Constraint->SetIsBadConstraint(true);
            break;
        }
    }

    // Continue all the other error checking even if it is a duplicate so that all the
    // other errors are reported at both the original and the duplicate locations.

    if (!ValidateConstraintType(
            ConstraintType,
            Constraint->GetLocation(),
            Parameter->GetName(),
            ErrorLog,
            CompilerHost,
            CurrentCompilerInstance(),
            CurrentAllocator(),
            HasReferenceConstraint,
            HasValueConstraint,
            ClassConstraintType,
            DisallowRedundancy))
    {
        Constraint->SetIsBadConstraint(true);
    }
}

// Returns false if the constraint type is not valid
// else returns true.
//
bool
Bindable::ValidateConstraintType
(
    BCSYM *ConstraintType,
    Location *ConstraintLocation,       // can be NULL
    _In_opt_z_ STRING *GenericParameterName,       // can be NULL
    ErrorTable *ErrorLog,               // can be NULL
    CompilerHost *CompilerHost,
    Compiler *CompilerInstance,
    NorlsAllocator *Allocator,
    bool &HasReferenceConstraint,
    bool &HasValueConstraint,
    BCSYM *&ClassConstraintType,
    bool DisallowRedundancy
)
{
    bool IsBad = false;

    if (IsClass(ConstraintType))
    {
        if (HasReferenceConstraint && DisallowRedundancy)
        {
            if (ErrorLog)
            {
                ReportErrorAtLocation(
                    ERRID_RefAndClassTypeConstrCombined,
                    ErrorLog,
                    ConstraintLocation);
            }

            IsBad = true;
        }
        else if (HasValueConstraint && DisallowRedundancy)
        {
            if (ErrorLog)
            {
                ReportErrorAtLocation(
                    ERRID_ValueAndClassTypeConstrCombined,
                    ErrorLog,
                    ConstraintLocation);
            }

            IsBad = true;
        }
        else
        {
            if (DisallowRedundancy)
            {
                if (ClassConstraintType)
                {
                    if (ErrorLog)
                    {
                        ReportErrorAtLocation(
                            ERRID_MultipleClassConstraints1,
                            ErrorLog,
                            ConstraintLocation,
                            GenericParameterName);
                    }

                    IsBad = true;
                }

                if (ConstraintType->PClass()->IsNotInheritable())
                {
                    if (ErrorLog)
                    {
                        ReportErrorAtLocation(
                            ERRID_ClassConstraintNotInheritable1,
                            ErrorLog,
                            ConstraintLocation,
                            ConstraintType->GetErrorName(CompilerInstance));
                    }

                    IsBad = true;
                }
                else if (ConstraintType == CompilerHost->GetFXSymbolProvider()->GetType(FX::ArrayType) ||
                         ConstraintType == CompilerHost->GetFXSymbolProvider()->GetType(FX::DelegateType) ||
                         ConstraintType == CompilerHost->GetFXSymbolProvider()->GetType(FX::MultiCastDelegateType) ||
                         ConstraintType == CompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType) ||
                         ConstraintType == CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType) ||
                         ConstraintType == CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType))
                {
                    if (ErrorLog)
                    {
                        ReportErrorAtLocation(
                            ERRID_ConstraintIsRestrictedType1,
                            ErrorLog,
                            ConstraintLocation,
                            ConstraintType->GetErrorName(CompilerInstance));
                    }

                    IsBad = true;
                }
                else if (!ClassConstraintType)
                {
                    ClassConstraintType = ConstraintType;
                }
            }
            else if (!ClassConstraintType)  // && Disallow redundancy
            {
                ClassConstraintType = ConstraintType;
            }
        }
    }
    else if (ConstraintType->IsGenericParam())
    {
        if (DisallowRedundancy && ConstraintType->PGenericParam()->HasValueConstraint())
        {
            if (ErrorLog)
            {
                ReportErrorAtLocation(
                    ERRID_TypeParamWithStructConstAsConst,
                    ErrorLog,
                    ConstraintLocation);
            }

            IsBad = true;
        }
    }
    else if (!IsInterface(ConstraintType))
    {
        if (DisallowRedundancy)
        {
            if (ErrorLog)
            {
                ReportErrorAtLocation(
                    ERRID_ConstNotClassInterfaceOrTypeParam1,
                    ErrorLog,
                    ConstraintLocation,
                    ConstraintType->GetErrorName(CompilerInstance));
            }

            IsBad = true;
        }
        else if (!ClassConstraintType)  // && Disallow redundancy
        {
            ClassConstraintType = ConstraintType;
        }
    }

    return !IsBad;
}

bool
Bindable::IsValidConstraintType
(
    BCSYM *ConstraintType,
    CompilerHost *CompilerHost,
    Compiler *CompilerInstance,
    NorlsAllocator *Allocator,
    bool DisallowRedundancy
)
{
    bool HasReferenceConstraint = false;
    bool HasValueConstraint = false;
    BCSYM *ClassConstraintType = NULL;

    return
        ValidateConstraintType(
            ConstraintType,
            NULL,
            NULL,
            NULL,
            CompilerHost,
            CompilerInstance,
            Allocator,
            HasReferenceConstraint,
            HasValueConstraint,
            ClassConstraintType,
            DisallowRedundancy);
}

void
Bindable::ValidateIndirectConstraintsForGenericParams
(
    BCSYM_GenericParam *ParameterList,
    ErrorTable *ErrorLog
)
{
    if (!ParameterList)
    {
        return;
    }

    unsigned int Count = 0;
    for (BCSYM_GenericParam *Parameter = ParameterList; Parameter; Parameter = Parameter->GetNextParam())
    {
        Count++;
    }

    const unsigned int MaxParams = 12;
    BCSYM_GenericTypeConstraint *CurrentConstraintsBuffer[MaxParams];
    bool ParametersCheckedBuffer[MaxParams];

    BCSYM_GenericTypeConstraint **CurrentConstraints = CurrentConstraintsBuffer;
    bool *ParametersChecked = ParametersCheckedBuffer;

    if (Count > MaxParams)
    {
        NorlsAllocator TempAllocator(NORLSLOC);

        if (!VBMath::TryMultiply(sizeof(BCSYM_GenericTypeConstraint *),Count) ||
             !VBMath::TryMultiply(sizeof(bool),Count))
        {
            return; // Overflow
        }

        CurrentConstraints =
            (BCSYM_GenericTypeConstraint **)TempAllocator.Alloc(Count * sizeof(BCSYM_GenericTypeConstraint *));

        ParametersChecked =
            (bool *)TempAllocator.Alloc(Count * sizeof(bool));

        CheckAllGenericParamsForConstraintCycles(
            ParameterList,
            CurrentConstraints,
            ParametersChecked,
            ErrorLog);

        // Re-init the ParametersChecked flags so that they can be used during the
        // constraint clash checking to avoid redundant errors i.e. in orer to avoid
        // comparing constraints with constraints of other type parameters that could
        // be marked bad when they inturn are checked for clashes. This will also avoid
        // conflicting errors by avoiding comparisons with conflicting constraints on
        // type parameters.
        //
        // Note that ParametersChecked could indeed be used as ParametersNotChecked and
        // thus reinitialization avoided, but this reinit is cheap considering the number
        // of type parameters that are usually present and also this is a better design
        // than depending on implementation details of CheckAllGenericParamsForConstraint-
        // -Cycles.
        //
        memset(ParametersChecked, 0, Count * sizeof(ParametersChecked[0]));

        CheckAllGenericParamsForIndirectConstraintClashes(
            ParameterList,
            ParametersChecked,
            ErrorLog);
    }
    else
    {
        memset(CurrentConstraintsBuffer, 0, sizeof(CurrentConstraintsBuffer));
        memset(ParametersCheckedBuffer, 0, sizeof(ParametersCheckedBuffer));

        CheckAllGenericParamsForConstraintCycles(
            ParameterList,
            CurrentConstraints,
            ParametersChecked,
            ErrorLog);

        // Re-init the ParametersChecked flags so that they can be used during the
        // constraint clash checking to avoid redundant errors i.e. in orer to avoid
        // comparing constraints with constraints of other type parameters that could
        // be marked bad when they inturn are checked for clashes. This will also avoid
        // conflicting errors by avoiding comparisons with conflicting constraints on
        // type parameters.
        //
        // Note that ParametersChecked could indeed be used as ParametersNotChecked and
        // thus reinitialization avoided, but this reinit is cheap considering the number
        // of type parameters that are usually present and also this is a better design
        // than depending on implementation details of CheckAllGenericParamsForConstraint-
        // -Cycles.
        //
        memset(ParametersCheckedBuffer, 0, sizeof(ParametersCheckedBuffer));

        CheckAllGenericParamsForIndirectConstraintClashes(
            ParameterList,
            ParametersChecked,
            ErrorLog);
    }
}

void
Bindable::CheckAllGenericParamsForConstraintCycles
(
    BCSYM_GenericParam *ParameterList,
    BCSYM_GenericTypeConstraint *CurrentConstraints[],
    bool ParametersChecked[],
    ErrorTable *ErrorLog
)
{
    // Note that we only prevent T1 As T2, T2 As T1 kind of circular dependencies
    // and do not prevent T1 As C1(Of T2), T2 As T1 kind of circular dependencies.

    for (BCSYM_GenericParam *Parameter = ParameterList; Parameter; Parameter = Parameter->GetNextParam())
    {
        if (!ParametersChecked[Parameter->GetPosition()])
        {
            CheckGenericParamForConstraintCycles(
                Parameter,
                CurrentConstraints,
                ParametersChecked,
                ErrorLog);
        }
    }
}

void
Bindable::CheckGenericParamForConstraintCycles
(
    BCSYM_GenericParam *Parameter,
    BCSYM_GenericTypeConstraint *CurrentConstraints[],
    bool ParametersChecked[],
    ErrorTable *ErrorLog
)
{
    // Note that we only prevent T1 As T2, T2 As T1 kind of circular dependencies
    // and do not prevent T1 As C1(Of T2), T2 As T1 kind of circular dependencies.

    unsigned int ParamIndex = Parameter->GetPosition();

    if (CurrentConstraints[ParamIndex])
    {
        // Cycle detected

        ReportConstraintCycleError(ErrorLog, Parameter, CurrentConstraints, ParamIndex);
        CurrentConstraints[ParamIndex]->SetIsBadConstraint(true);
        return;
    }

    for (BCSYM_GenericTypeConstraint *Constraint = Parameter->GetTypeConstraints();
         Constraint;
         Constraint = Constraint->Next())
    {
        if (Constraint->IsBadConstraint() || !Constraint->GetType()->IsGenericParam())
        {
            continue;
        }

        // Type param from parent cannot cause cycles
        //
        if (Constraint->GetType()->PGenericParam()->GetParent() == Parameter->GetParent())
        {
            CurrentConstraints[ParamIndex] = Constraint;

            CheckGenericParamForConstraintCycles(
                Constraint->GetType()->PGenericParam(),
                CurrentConstraints,
                ParametersChecked,
                ErrorLog);
        }
    }

    CurrentConstraints[ParamIndex] = NULL;
    ParametersChecked[ParamIndex] = true;

#if DEBUG
    Parameter->SetConstraintCycleCheckingDone(true);
#endif
}

void
Bindable::ReportConstraintCycleError
(
    ErrorTable *ErrorLog,
    BCSYM_GenericParam *Parameter,
    BCSYM_GenericTypeConstraint *ConstraintsInCycle[],
    unsigned int CycleInitiatingConstraintIndex
)
{
    StringBuffer CycleDesc;

    BCSYM_GenericParam *PrevParam = Parameter;

    unsigned int Current = CycleInitiatingConstraintIndex;
    do
    {
        VSASSERT(ConstraintsInCycle[Current] &&
                 ConstraintsInCycle[Current]->GetType()->IsGenericParam(),
                    "Unexpected constraint type in constraint cycle!!!!");

        BCSYM_GenericParam *CurrentParam =
            ConstraintsInCycle[Current]->GetType()->PGenericParam();

        ResLoadStringRepl(
            ERRID_ConstraintCycleLink2,
            &CycleDesc,
            PrevParam->GetName(),
            CurrentParam->GetName());

        PrevParam = CurrentParam;
        Current = CurrentParam->GetPosition();
    }
    while (Current != CycleInitiatingConstraintIndex);

    ReportErrorAtLocation(
        ERRID_ConstraintCycle2,
        ErrorLog,
        ConstraintsInCycle[CycleInitiatingConstraintIndex]->GetLocation(),
        Parameter->GetName(),
        CycleDesc.GetString());
}

void
Bindable::CheckAllGenericParamsForIndirectConstraintClashes
(
    BCSYM_GenericParam *ParameterList,
    bool ParametersChecked[],
    ErrorTable *ErrorLog
)
{
    for (BCSYM_GenericParam *Parameter = ParameterList; Parameter; Parameter = Parameter->GetNextParam())
    {
        CheckGenericParamForIndirectConstraintClash(
            Parameter,
            ParametersChecked,
            ErrorLog);
    }
}

void
Bindable::CheckGenericParamForIndirectConstraintClash
(
    BCSYM_GenericParam *Parameter,
    bool ParametersChecked[],
    ErrorTable *ErrorLog
)
{
    // Definitely bad constraint combinations:
    // struct and class
    // struct and a class type constraint
    // two unrelated class type constraints

    unsigned int ParamIndex = Parameter->GetPosition();

    // If this type parameter has already been checked, then return immediately
    //
    if (ParametersChecked[ParamIndex])
    {
        return;
    }

    ValidateIndirectConstraints(Parameter, ParametersChecked, ErrorLog);

    ParametersChecked[ParamIndex] = true;
}

void
Bindable::ValidateIndirectConstraints
(
    BCSYM_GenericParam *Parameter,
    bool ParametersChecked[],
    ErrorTable *ErrorLog
)
{
    BCITER_Constraints Iter1(Parameter, true, CurrentCompilerInstance());
    BCITER_Constraints Iter2(Parameter, true, CurrentCompilerInstance());

    while (BCSYM_GenericConstraint *Constraint = Iter1.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (Iter1.CurrentGenericParamContext() != Parameter)
        {
            // For constraints of type parameter type, first validate the constraints of the type
            // parameter type and then only validate its constraints against the constraints of
            // the current parameter. This is required to avoid conflicting errors.
            //
            BCSYM_GenericParam *GenericParamTypeConstraint = Iter1.CurrentGenericParamContext();
            BCSYM_NamedRoot *ParentOfGenericParam = GenericParamTypeConstraint->GetParent();

            if (ParentOfGenericParam == CurrentContainer())
            {
                CheckGenericParamForIndirectConstraintClash(
                    GenericParamTypeConstraint,
                    ParametersChecked,
                    ErrorLog);
            }
            else if (ParentOfGenericParam->IsContainer())
            {
                ResolveAllNamedTypesForContainer(ParentOfGenericParam->PContainer(), m_CompilationCaches);
            }
        }

        Iter2.Reset();
        while (BCSYM_GenericConstraint *PossiblyClashingConstraint = Iter2.Next())
        {
            VSASSERT(!PossiblyClashingConstraint->IsBadConstraint(), "Bad constraint unexpected!!!");

            // Validating direct constraints already completed previously, so skip.
            //
            if (Iter1.FirstGenericParamContext() == Parameter &&
                Iter2.FirstGenericParamContext() == Parameter)
            {
                // But for overriding members which can have multiple class constraints, need
                // to wait till here to check for conflicts between the direct class constraints.
                //
                // The reason this check needs to be delayed till here is to avoid walking the
                // constraints of the type parameter before cycle checking has been completed.

                if (ConstraintsConflict(Constraint, PossiblyClashingConstraint))
                {
                    if (ErrorLog)
                    {
                        StringBuffer ConstraintString1;
                        StringBuffer ConstraintString2;

                        ReportErrorAtLocation(
                            ERRID_ConflictingDirectConstraints3,
                            ErrorLog,
                            PossiblyClashingConstraint->GetLocation(),
                            GetErrorNameForConstraint(PossiblyClashingConstraint, &ConstraintString1),
                            GetErrorNameForConstraint(Constraint, &ConstraintString2),
                            Parameter->GetName());
                    }

                    PossiblyClashingConstraint->SetIsBadConstraint(true);
                }

                continue;
            }

            // Checking the indirect constraint with all the constraints before the type parameter
            // constraint through which the indirect constraint is seen

            if (Iter1.FirstGenericParamContext() == Iter2.FirstGenericParamContext())
            {
                break;
            }

            if (ConstraintsConflict(Constraint, PossiblyClashingConstraint))
            {
                // Indirect constraint '|1' obtained from the type parameter constraint '|2' conflicts
                // with the indirect constraint '|3' obtained from the type parameter constraint '|4'.

                if (Iter1.FirstGenericParamContext() == Parameter)
                {
                    if (ErrorLog)
                    {
                        StringBuffer Buffer1;
                        StringBuffer Buffer2;

                        ReportErrorAtLocation(
                            ERRID_ConstraintClashDirectIndirect3,
                            ErrorLog,
                            Constraint->GetLocation(),
                            GetErrorNameForConstraint(Constraint, &Buffer1),
                            GetErrorNameForConstraint(PossiblyClashingConstraint, &Buffer2),
                            Iter2.FirstGenericParamContext()->GetName());
                    }

                    Constraint->SetIsBadConstraint(true);
                }
                else if (Iter2.FirstGenericParamContext() == Parameter)
                {
                    if (ErrorLog)
                    {
                        StringBuffer Buffer1;
                        StringBuffer Buffer2;

                        ReportErrorAtLocation(
                            ERRID_ConstraintClashIndirectDirect3,
                            ErrorLog,
                            Iter1.FirstGenericParamConstraintContext()->GetLocation(),
                            GetErrorNameForConstraint(Constraint, &Buffer1),
                            Iter1.FirstGenericParamContext()->GetName(),
                            GetErrorNameForConstraint(PossiblyClashingConstraint, &Buffer2));
                    }

                    Iter1.FirstGenericParamConstraintContext()->SetIsBadConstraint(true);
                }
                else
                {
                    if (ErrorLog)
                    {
                        StringBuffer Buffer1;
                        StringBuffer Buffer2;

                        ReportErrorAtLocation(
                            ERRID_ConstraintClashIndirectIndirect4,
                            ErrorLog,
                            Iter1.FirstGenericParamConstraintContext()->GetLocation(),
                            GetErrorNameForConstraint(Constraint, &Buffer1),
                            Iter1.FirstGenericParamContext()->GetName(),
                            GetErrorNameForConstraint(PossiblyClashingConstraint, &Buffer2),
                            Iter2.FirstGenericParamContext()->GetName());
                    }

                    Iter1.FirstGenericParamConstraintContext()->SetIsBadConstraint(true);
                }

                break;
            }
        }
    }
}

bool
Bindable::ConstraintsConflict
(
    BCSYM_GenericConstraint *Constraint1,
    BCSYM_GenericConstraint *Constraint2
)
{
    VSASSERT(!Constraint1->IsBadConstraint(), "Bad constraint unexpected!!!");
    VSASSERT(!Constraint2->IsBadConstraint(), "Bad constraint unexpected!!!");

    // Interface constraints do not clash with any other constraints.
    // Bug VSWhidbey 449053.
    if ((Constraint1->IsGenericTypeConstraint() &&
            IsInterface(Constraint1->PGenericTypeConstraint()->GetType())) ||
        (Constraint2->IsGenericTypeConstraint() &&
            IsInterface(Constraint2->PGenericTypeConstraint()->GetType())))
    {
        return false;
    }

    Symbols SymbolFactory(CurrentCompilerInstance(), CurrentAllocator(), NULL);

    // "structure" constraint and non-value type conflict.

    BCSYM *PossibleValueType = NULL;

    if (Constraint1->IsValueConstraint())
    {
        PossibleValueType =
            Constraint2->IsGenericTypeConstraint() ?
                Constraint2->PGenericTypeConstraint()->GetType() :
                NULL;
    }
    else if (Constraint2->IsValueConstraint())
    {
        PossibleValueType =
            Constraint1->IsGenericTypeConstraint() ?
                Constraint1->PGenericTypeConstraint()->GetType() :
                NULL;
    }

    if (PossibleValueType &&
        !ValidateValueConstraintForType(
            PossibleValueType,
            NULL,
            NULL,
            NULL,
            NULL,
            CurrentCompilerHost(),
            CurrentCompilerInstance(),
            false) &&
        !TypeHelpers::IsOrInheritsFrom(
            CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::EnumType),
            PossibleValueType,
            SymbolFactory,
            true,
            NULL,
            false,
            NULL,
            NULL))
    {
        return true;
    }


    // "class" constraint and value type conflict.

    BCSYM *PossibleReferenceType = NULL;

    if (Constraint1->IsReferenceConstraint())
    {
        PossibleReferenceType =
            Constraint2->IsGenericTypeConstraint() ?
                Constraint2->PGenericTypeConstraint()->GetType() :
                NULL;
    }
    else if (Constraint2->IsReferenceConstraint())
    {
        PossibleReferenceType =
            Constraint1->IsGenericTypeConstraint() ?
                Constraint1->PGenericTypeConstraint()->GetType() :
                NULL;
    }

    if (PossibleReferenceType &&
        !ValidateReferenceConstraintForType(
            PossibleReferenceType,
            NULL,
            NULL,
            NULL,
            NULL))
    {
        return true;
    }

    // Note that Derived and Base classes do not clash. The base class is just redundant and
    // we do allow redundancy for indirect constraints.

    if (Constraint1->IsGenericTypeConstraint() &&
        Constraint2->IsGenericTypeConstraint() &&
        !ValidateTypeConstraintForType(
            Constraint1->PGenericTypeConstraint()->GetType(),
            Constraint2->PGenericTypeConstraint()->GetType(),
            CurrentCompilerHost(),
            &SymbolFactory) &&
        !ValidateTypeConstraintForType(
            Constraint2->PGenericTypeConstraint()->GetType(),
            Constraint1->PGenericTypeConstraint()->GetType(),
            CurrentCompilerHost(),
            &SymbolFactory))
    {
        return true;
    }

    return false;
}

WCHAR *
Bindable::GetErrorNameForConstraint
(
    BCSYM_GenericConstraint *Constraint,
    StringBuffer *Buffer
)
{
    WCHAR *Result = NULL;

    if (Constraint->IsReferenceConstraint())
    {
        Result = CurrentCompilerInstance()->TokenToString(tkCLASS);
    }
    else if (Constraint->IsValueConstraint())
    {
        Result = CurrentCompilerInstance()->TokenToString(tkSTRUCTURE);
    }
    else if (Constraint->IsNewConstraint())
    {
        Result = CurrentCompilerInstance()->TokenToString(tkNEW);
    }
    else
    {
        VSASSERT(Constraint->IsGenericTypeConstraint(), "Unexpected constraint kind!!!");

        BCSYM *Type = Constraint->PGenericTypeConstraint()->GetType();

        if (Type->IsNamedType())
        {
            Result = GetQualifiedErrorName(Type);
        }
        else
        {
            Type->GetBasicRep(CurrentCompilerInstance(), CurrentContainer(), Buffer);
            Result = Buffer->GetString();
        }
    }

    return Result;
}

void
Bindable::CheckGenericConstraintsForContainer
(
)
{
    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    BCSYM_Container *Container = CurrentContainer();

    // Validate the argument types in all named types in the container.

    // Even in the case of partial types, only their main type is expected here
    //
    VSASSERT(!Container->IsPartialTypeAndHasMainType(), "Partial type unexpected!!!");

    for (BCSYM_Container *CurrentContainer = Container;
         CurrentContainer;
         CurrentContainer = CurrentContainer->GetNextPartialType())
    {
        for (BCSYM_NamedType *NamedType = CurrentContainer->GetNamedTypesList();
            NamedType;
            NamedType = NamedType->GetNext())
        {
            CheckGenericConstraints(
                NamedType,
                CurrentErrorLog(NamedType),
                CurrentCompilerHost(),
                CurrentCompilerInstance(),
                &SymbolFactory,
                m_CompilationCaches);

            // Check any member variable or Auto Property with "As New" to ensure that the type of the variable can be instantiated.
            //
            // Need to delay DimAsNewSemantics for member variables because the constraint type for type params in the parent
            // will not yet be bound nor cycle checking done during ResolveAllNamedTypes.
            //
            if (NamedType->GetSymbol() &&
                NamedType->GetContext() &&
                ( ( NamedType->GetContext()->IsProperty() && NamedType->GetContext()->PProperty()->IsAutoProperty() &&
                    NamedType->GetContext()->PProperty()->IsNewAutoProperty()) ||
                  ( NamedType->GetContext()->IsVariable() && NamedType->GetContext()->PVariable()->IsNew() ) ) &&
                NamedType->GetContext()->PMember()->GetRawType() == NamedType)
            {
                ERRID ErrId = NOERROR;

                DimAsNewSemantics(
                    NamedType->GetSymbol(),
                    ErrId);

                if (ErrId != NOERROR)
                {
                    // Log error discovered above
                    // Build the qualified name.
                    StringBuffer QualifiedName;
                    unsigned CountOfNames = NamedType->GetNameCount();

                    for(unsigned i = 0; i < CountOfNames; i++)
                    {
                        if (NamedType->GetNameArray()[i])
                        {
                            QualifiedName.AppendString(NamedType->GetNameArray()[i]);

                            if (i != CountOfNames - 1)
                            {
                                QualifiedName.AppendChar( L'.');
                            }
                        }
                    }

                    ReportErrorOnSymbol(
                        ErrId,
                        CurrentErrorLog(NamedType),
                        NamedType,
                        QualifiedName.GetString());            // not used by all errids that come through here

                    // Invalidate the namedtype to prevent any other downstream errors.
                    NamedType->SetSymbol(Symbols::GetGenericBadNamedRoot());
                }
            }
        }
    }

    if (IsClass(Container))
    {
        CheckGenericConstraints(
            Container->PClass()->GetRawBase(),
            Container->PClass()->GetRawBase()->IsNamedType() ?
                CurrentErrorLog(Container->PClass()->GetRawBase()->PNamedType()) :
                CurrentErrorLog(Container),
            CurrentCompilerHost(),
            CurrentCompilerInstance(),
            &SymbolFactory,
            m_CompilationCaches,
            true);      // check constraints for the bindings among the type arguments too
    }

    else if (IsInterface(Container))
    {
        for (BCSYM_Implements *BaseImplements = Container->PInterface()->GetFirstImplements();
             BaseImplements;
             BaseImplements = BaseImplements->GetNext())
        {
            if (BaseImplements->IsBad())
            {
                continue;
            }

            CheckGenericConstraints(
                BaseImplements->GetRawRoot(),
                CurrentErrorLog(Container),
                CurrentCompilerHost(),
                CurrentCompilerInstance(),
                &SymbolFactory,
                m_CompilationCaches,
                true);      // check constraints for the bindings among the type arguments too
        }
    }

    else if (IsNamespace(Container))
    {
        for(ImportedTarget *CurrentImport = Container->PNamespace()->GetImports();
            CurrentImport;
            CurrentImport = CurrentImport->m_pNext)
        {
            if (CurrentImport->m_hasError)
            {
                continue;
            }

            if (CurrentImport->m_pTarget &&
                CurrentImport->m_pTarget->IsGenericTypeBinding())
            {
                CheckGenericConstraintsForImportsTarget(
                    CurrentImport,
                    CurrentErrorLog(Container),
                    CurrentCompilerHost(),
                    CurrentCompilerInstance(),
                    &SymbolFactory,
                    m_CompilationCaches);
            }
        }
    }
}


void
Bindable::CheckStructureMemberCycles()
{
    BCSYM_Container *Container = CurrentContainer();

    if (!IsStructure(Container))
    {
        return;
    }

    do
    {
        for(BCSYM_NamedType *NamedType = Container->GetNamedTypesList();
            NamedType;
            NamedType = NamedType->GetNext())
        {
            BCSYM_Container *StructType;
            BCSYM_GenericTypeBinding *StructTypeBindingContext;

            StructType =
                    IsTypeStructAndIsTypeOfAValidInstanceMemberVariable(
                        NamedType,
                        StructTypeBindingContext);

            if (!StructType)
            {
                continue;
            }

            ResolveAllNamedTypesForContainer(StructType, m_CompilationCaches);

            // For generics, cycles could possibly be created
            // after substitution. In order to detect such cases,
            // we need to consider the possiblity even if
            // one generic binding exists.
            //
            if (StructTypeBindingContext)
            {
                SetStructureMemberCycleDetected(true);

                // NOTE: do not break out here!!! We need to complete
                // resolve named types for all the structs that are
                // used as types of instance member variables in this
                // structure. So continue with the loop.
                //
            }
        }

    } while (Container = Container->GetNextPartialType());

    return;
}

BCSYM_Class *
Bindable::IsTypeStructAndIsTypeOfAValidInstanceMemberVariable
(
    BCSYM_NamedType *NamedType,
    BCSYM_GenericTypeBinding *&BindingContextForMemberVariableType
)
{
    VSASSERT( NamedType->DigThroughNamedType(),
                    "Unresolved named type unexpected!!!");

    BindingContextForMemberVariableType = NULL;

    if (NamedType->DigThroughNamedType()->IsBad())
    {
        return NULL;
    }

    BCSYM_NamedRoot *ContextOfNamedType = NamedType->GetContext();

    if (!ContextOfNamedType)
    {
        return NULL;
    }

    // Only Instance Member variables having structures type could possibly cause embedded
    // type cycles.
    if (ContextOfNamedType->IsBad() ||
       !(ContextOfNamedType->IsVariable() ||
            (ContextOfNamedType->IsProperty() && ContextOfNamedType->PProperty()->IsAutoProperty())) ||
        ContextOfNamedType->PMember()->IsShared())
    {
        return NULL;
    }


    // Note: Array variables don't count because although the Array element type might
    // be of this type, the Array itself is a different type.
    //
    BCSYM *TypeOfMemberVar = ContextOfNamedType->PMember()->GetType()->DigThroughAlias();

    VSASSERT( TypeOfMemberVar,
                    "How can a named type not be bound yet at least to a bad type ?");

    if (!IsStructure(TypeOfMemberVar))
    {
        return NULL;
    }

    VSASSERT(NamedType->GetSymbol(), "Named type should already have been bound!!!");

    BCSYM *PossibleStructType = NamedType->GetSymbol();

    if (!IsStructure(PossibleStructType))
    {
        return NULL;
    }


    if (PossibleStructType->IsGenericTypeBinding())
    {
        BindingContextForMemberVariableType = PossibleStructType->PGenericTypeBinding();
    }

    return PossibleStructType->PClass();
}

void
Bindable::DetectStructMemberCyclesForContainer()
{
    VSASSERT(GetStatusOfDetectStructMemberCycles() == NotStarted,
                "Reentrancy during structure member cycle unexpected!!!");

    // Mark the current container indicating that structure member cycle detection work is in progress
    SetStatusOfDetectStructMemberCycles(InProgress);

    BCSYM_Container *Container = CurrentContainer();

    if (!IsStructure(Container))
    {
        // Mark the current container indicating that structure member cycle detection work is done
        SetStatusOfDetectStructMemberCycles(Done);
        return;
    }

    if (StructureMemberCycleDetected() &&
        !Container->PClass()->IsStructCycleCheckingDone())
    {
        NorlsAllocator TempAllocator(NORLSLOC);

        Symbols
            SymbolAllocator(
                CurrentCompilerInstance(),
                &TempAllocator,
                NULL,
                NULL);

        BCSYM_GenericTypeBinding *OpenBindingForStructure =
            IsGenericOrHasGenericParent(Container) ?
                SynthesizeOpenGenericBinding(Container, SymbolAllocator)->PGenericTypeBinding() :
                NULL;

        StructCycleNode *CycleCausingNode = NULL;

        DetectStructureMemberCycle(
            Container->PClass(),
            OpenBindingForStructure,
            OpenBindingForStructure,
            NULL,
            &CycleCausingNode,
            &SymbolAllocator);

        VSASSERT(Container->PClass()->IsStructCycleCheckingDone(),
                    "Exiting struct layout cycle detection unexpectedly!!!");

        VSASSERT(CycleCausingNode == NULL, "Inconsistency in struct cycle detection!!!");
    }

    // It is imperative that the cycle info be cleared, else future compiles or cycle detection
    // for other structures during this compilation cycle could be very adversely affected.
    //
    VSASSERT(Container->PClass()->GetStructCycleInfo() == NULL,
                "Non-NULL struct info unexpected after cycle detection completed!!!");

    // Mark the current container indicating that structure member cycle detection work is done
    SetStatusOfDetectStructMemberCycles(Done);
}

void
Bindable::DetectStructureMemberCycle
(
    BCSYM_Class *Structure,
    BCSYM_GenericTypeBinding *StructureBindingContext,
    BCSYM_GenericTypeBinding *OpenBindingForStructure,
    StructCycleNode *PrevCycleNode,
    StructCycleNode **CycleCausingNode,
    Symbols *SymbolAllocator
)
{
    StructCycleNode NewNode;
    bool CurrentBindingIsOpenBinding = false;

    if (StructureBindingContext == OpenBindingForStructure ||   // Need to match up null bindings for non-generic structs
        !StructureBindingContext ||                             // Treat any open types in bad metadata as their equivalent
                                                                // open bindings
        BCSYM::AreTypesEqual(StructureBindingContext, OpenBindingForStructure))
    {
        if (Structure->GetStructCycleInfo())
        {
            ReportStructureMemberCycleError(Structure->GetStructCycleInfo(), SymbolAllocator);
            *CycleCausingNode = Structure->GetStructCycleInfo();
            return;
        }

        // Only set this when the type being checked is the open binding or a non-generic to
        // avoid walking the cycle nodes list in order to check for a cycle.
        //
        Structure->SetStructCycleInfo(&NewNode);

        CurrentBindingIsOpenBinding = true;
    }

    VSASSERT(*CycleCausingNode == NULL, "Inconsistency in struct cycle detection!!!");

    NewNode.m_Struct = StructureBindingContext ? (BCSYM *)StructureBindingContext : Structure;
    NewNode.m_Field = NULL;
    NewNode.m_Next = NULL;

    if (PrevCycleNode)
    {
        PrevCycleNode->m_Next = &NewNode;
    }

    bool StructureDefinedInMetaData = DefinedInMetaData(Structure);
    BCITER_CHILD StructMembers(Structure);

    while (BCSYM_NamedRoot *Member = StructMembers.GetNext())
    {
        VSASSERT(*CycleCausingNode == NULL, "Inconsistency in struct cycle detection!!!");

        // Bug VSWhidbey 196029
        // Ignore bad members only if they are in source. Need to consider bad
        // metadata members too because the metadata structure might actually
        // have a private field that could cause a cycle, but which is marked
        // bad for reasons like member clash.
        //
        // Ignore the synthetic withevents field - see bug VSWhidbey 208063
        //
        if ((Member->IsBad() &&
                // Bug VSWhidbey 196029 - see explanation above.
                //
                !StructureDefinedInMetaData) ||
            !Member->IsVariable() ||
            Member->PVariable()->IsShared() ||
            Member->PVariable()->FieldCausesStructCycle() ||
            Member->PVariable()->CreatedByWithEventsDecl())
        {
            continue;
        }

        BCSYM *Type = Member->PVariable()->GetType();

        VSASSERT(Type, "NULL type unexpected for member variable!!!");
        VSASSERT(!Member->IsBad() || StructureDefinedInMetaData, "Member state changed to bad unexpectedly!!!");

        BCSYM *TypeAfterReplacement =
            ReplaceGenericParametersWithArguments(
                Type,
                StructureBindingContext,
                *SymbolAllocator);

        VSASSERT(TypeAfterReplacement, "NULL substituted type unexpected for member variable!!!");

        if (TypeAfterReplacement->IsBad() ||
            !IsStructure(TypeAfterReplacement))
        {
            continue;
        }

        BCSYM_Class *VariableType = TypeAfterReplacement->PClass();

        BCSYM_GenericTypeBinding *VariableTypeBindingContext =
                        TypeAfterReplacement->IsGenericTypeBinding() ?
                            TypeAfterReplacement->PGenericTypeBinding() :
                            NULL;

        // Optimization: Non-generic metadata structures cannot cause cycles involving source types.
        //
        if (DefinedInMetaData(VariableType) &&
            !IsGenericOrHasGenericParent(VariableType))
        {
            continue;
        }

        // Fill a new cycle info node
        //
        NewNode.m_Field = Member->PVariable();

        BCSYM_GenericTypeBinding *OpenBindingForVariableType =
            IsGenericOrHasGenericParent(VariableType) ?
                OpenBindingForVariableType =
                    SynthesizeOpenGenericBinding(
                        VariableType,
                        *SymbolAllocator)->PGenericTypeBinding() :
                NULL;

        // Use the open binding to detect any cycle on this variable's struct type
        // irrespective of any binding. This will also help catch infinite type
        // expansion issues.
        //
        if (!VariableType->IsStructCycleCheckingDone())
        {
            DetectStructureMemberCycle(
                VariableType,
                OpenBindingForVariableType,
                OpenBindingForVariableType,
                &NewNode,
                CycleCausingNode,
                SymbolAllocator);

            // Cycle already reported on this field, so skip further checking.
            // So in order to skip further checking through that field, need to exit here.
            // If the rest of the cycle checking through this field is not skipped, this
            // could lead to infinite bindings and stack overflow.
            //
            if (*CycleCausingNode == &NewNode)
            {
                *CycleCausingNode = NULL;
                continue;
            }

            // A cycle error has already been reported on a field further down the stack.
            // So in order to skip further checking through that field, need to exit here.
            // If the rest of the cycle checking through this field is not skipped, this
            // could lead to infinite bindings and stack overflow. See Bug VSWhidbey 263032.
            //
            if (*CycleCausingNode != NULL)
            {
                break;
            }

            VSASSERT(Member->PVariable()->FieldCausesStructCycle() == false,
                        "Should not continue further with struct cycle causing field!!!");
        }

        // Now continue looking for any cycles that might result due to substitution,
        // but do so only for non-open bindings because open bindings have already
        // been handled above
        //
        if (VariableTypeBindingContext &&
            !BCSYM::AreTypesEqual(VariableTypeBindingContext, OpenBindingForVariableType))
        {
            DetectStructureMemberCycle(
                VariableType,
                VariableTypeBindingContext,
                OpenBindingForVariableType,
                &NewNode,
                CycleCausingNode,
                SymbolAllocator);

            // Cycle already reported on this field, so skip further checking.
            // So in order to skip further checking through that field, need to exit here.
            // If the rest of the cycle checking through this field is not skipped, this
            // could lead to infinite bindings and stack overflow.
            //
            if (*CycleCausingNode == &NewNode)
            {
                *CycleCausingNode = NULL;
                continue;
            }

            // A cycle error has already been reported on a field further down the stack.
            // So in order to skip further checking through that field, need to exit here.
            // If the rest of the cycle checking through this field is not skipped, this
            // could lead to infinite bindings and stack overflow. See Bug VSWhidbey 263032.
            //
            if (*CycleCausingNode != NULL)
            {
                break;
            }

            VSASSERT(Member->PVariable()->FieldCausesStructCycle() == false,
                        "Should not continue further with struct cycle causing field!!!");
        }
    }

    // Remove the current node from the list of cycle nodes
    //
    if (PrevCycleNode)
    {
        PrevCycleNode->m_Next = NULL;
    }

    if (CurrentBindingIsOpenBinding)
    {
        // A common way to indicate cycle checking completion for both metadata and source structures
        // and the only way to indicate this for metadata container.
        //
        Structure->SetIsStructCycleCheckingDone(true);

        // Clear out the cycle info in the structure that was used during cycle detection
        //
        Structure->SetStructCycleInfo(NULL);
    }

    return;
}

void
Bindable::ReportStructureMemberCycleError
(
    StructCycleNode *CycleInitiatingNode,
    Symbols *SymbolAllocator
)
{
    VSASSERT(CycleInitiatingNode, "Unexpected cycle reporting!!!");
    VSASSERT(CycleInitiatingNode->m_Field, "NULL field unexpected in struct cycle!!!");
    VSASSERT(CycleInitiatingNode->m_Struct, "NULL struct unexpected in struct cycle!!!");

    BCSYM_Variable *CycleInitiatingField = CycleInitiatingNode->m_Field;

    VSASSERT(CycleInitiatingField->FieldCausesStructCycle() == false,
                    "How can a field already reported in a struct cycle be involved in a cycle again ?");

    BCSYM *CycleInitiatingStruct = CycleInitiatingNode->m_Struct;

    VSASSERT(IsStructure(CycleInitiatingStruct), "Non-structure unexpected in struct member cycle!!!");

    if (DefinedInMetaData(CycleInitiatingStruct->PClass()))
    {
        // Ignore bad metadata
        //
        // Why no error ? Users will eventually get a typeload exception.
        // Treat it as a few other bad metadata error where the user
        // gets type load exception.
        //
        // Also Perf-wise would be bad to validate metadata structures before,
        // hand, we never did this before nor should we start now.
        //
        CycleInitiatingField->SetFieldCausesStructCycle(true);
        return;
    }

    Bindable *BindableInstanceForStructure =
            CycleInitiatingStruct->PClass()->GetBindableInstanceIfAlreadyExists();

    if (!BindableInstanceForStructure)
    {
        VSFAIL("How can bindable not already be created for the structure with the cycle error ?");
        return;
    }

    VSASSERT( !CycleInitiatingStruct->PClass()->IsBindingDone() &&
              BindableInstanceForStructure->GetStatusOfDetectStructMemberCycles() != Done,
                    "How can we have a cycle if the structure causing the cycle is beyond the process of cycle detection ?");

    StringBuffer CycleDesc;
    BCSYM *ContainingStructure = CycleInitiatingNode->m_Struct;

    for(StructCycleNode *CurrentNode = CycleInitiatingNode;
        CurrentNode;
        CurrentNode = CurrentNode->m_Next)
    {
        BCSYM_Variable *MemberVar = CurrentNode->m_Field;

        VSASSERT(MemberVar, "NULL Member var unexpected in cycle!!!");

        VSASSERT(MemberVar->GetContainer() == ContainingStructure->PClass(),
                    "Inconsistency during struct member cycle error reporting!!!");

        VSASSERT(MemberVar->FieldCausesStructCycle() == false,
                    "How can a field already reported in a struct cycle be involved in a cycle again ?");

        BCSYM *FieldType = MemberVar->GetType();

        BCSYM *ContainedStructure =
            ContainingStructure->IsGenericBinding() ?
                ReplaceGenericParametersWithArguments(
                    FieldType,
                    ContainingStructure->PGenericBinding(),
                    *SymbolAllocator) :
                FieldType;

        VSASSERT(ContainingStructure && ContainedStructure, "NULL structures unexpected in struct cycle!!!");

        VSASSERT(IsStructure(ContainingStructure) && IsStructure(ContainedStructure),
                    "How can a non-structure be involved in a structure embedded member cycle ?");

        ResLoadStringRepl(
            ERRID_RecordEmbeds2,
            &CycleDesc,
            BindableInstanceForStructure->GetQualifiedErrorName(ContainingStructure),
            BindableInstanceForStructure->GetQualifiedErrorName(ContainedStructure),
            MemberVar->GetName());

        ContainingStructure = ContainedStructure;
    }

    // Report the error specifying the error node at which the cycle was detected

    VSASSERT(CycleInitiatingField->GetRawType()->IsNamedType(),
                    "Non-named type unexpected in structure cycle!!!");

    BindableInstanceForStructure->ReportErrorOnSymbol(
                                        ERRID_RecordCycle2,
                                        BindableInstanceForStructure->CurrentErrorLog(CycleInitiatingField),
                                        CycleInitiatingField->GetRawType(),
                                        BindableInstanceForStructure->GetQualifiedErrorName(CycleInitiatingStruct),
                                        CycleDesc.GetString());

    // Mark the structure Member on which the cycle error is detected as bad in order to
    // avoid reporting other spurious errors that would anyway go away once this is fixed.
    //
    CycleInitiatingField->SetIsBad();
    CycleInitiatingField->SetFieldCausesStructCycle(true);

    return;
}

bool
Bindable::IsStructure
(
    BCSYM_Container *Container
)
{
    return Container->IsClass() &&
            Container->PClass()->IsStruct() &&
            !Container->PClass()->IsEnum() &&
            !Container->PClass()->IsIntrinsicType();
}

bool
Bindable::IsStructure
(
    BCSYM *PossibleStructure
)
{
    return PossibleStructure->IsContainer() &&
            IsStructure(PossibleStructure->PContainer());
}

// If underlying type of enum is detected only in bindable,
// then we need to reType the enum members and expressions
// used in it.
//
// So this begs the question: why not always do the typing
// in Bindable. We don't do this to optimize the most common
// case where the type is either not specified or specified
// as primitive type. If this case is done in declared, then
// we avoid having to redo the typing for this case during the
// more frequent declared -> Bindable transitions.
//
void
Bindable::ReTypeEnumMembersIfNecessary()
{
    if (!CurrentContainer()->IsEnum())
    {
        return;
    }

    BCSYM_Class *Enum = CurrentContainer()->PClass();
    BCSYM *UnBoundUnderlyingType = Enum->GetUnderlyingTypeForEnum();

    if (!UnBoundUnderlyingType || !UnBoundUnderlyingType->IsNamedType())
    {
        // Bad non-named types for enums should have already been handled in declared
        //
        VSASSERT(!UnBoundUnderlyingType ||
                 UnBoundUnderlyingType->IsGenericBadNamedRoot() ||
                 (TypeHelpers::IsIntrinsicType(UnBoundUnderlyingType) && TypeHelpers::IsIntegralType(UnBoundUnderlyingType)),
                    "Unexpected enum underlying type!!!");
        return;
    }

    BCSYM *UnderlyingType = UnBoundUnderlyingType->PNamedType()->DigThroughNamedType();

    if (UnderlyingType->IsGenericBadNamedRoot())
    {
        return;
    }

    // IsIntegralType returns true for enums too, so need to check IsIntrinsic too additionally
    //
    if (!TypeHelpers::IsIntegralType(UnderlyingType) ||
        !UnderlyingType->IsIntrinsicType())
    {
        VSASSERT(UnBoundUnderlyingType->IsNamedType(), "Bad Non-named type unexpected as enum type in bindable!!!");

        ReportErrorOnSymbol(
            ERRID_InvalidEnumBase,
            CurrentErrorLog(Enum),
            UnBoundUnderlyingType);

        // Change the bound type to a bad type so that when evaluating initializers of the enum
        // members, errors resulting from conversions to this type are avoided.
        //
        UnBoundUnderlyingType->PNamedType()->SetSymbol(Symbols::GetGenericBadNamedRoot());

        return;
    }

    // Set the VType of the enum to the correct type
    //
    Enum->SetVtype(UnderlyingType->GetVtype());

    return;
}

// This is a helper for and only for BCSYM_Class::GetVType()
// to resolve an enum's underlying type if possible when
// trying to determine theVType.
//
// This is needed because during bindable, some constant
// expressions in declarations might need to be evaluated
// for which we might need to know the VType of enums.
//
// Returns true if the underlying type was resolved, else
// returns false.
//
bool
Bindable::DetermineEnumUnderlyingTypeIfPossible
(
    BCSYM_Class *Enum,
    CompilationCaches *IDECompilationCaches
 )
{
    // NOTE: This is a helper for and only for BCSYM_Class::GetVType()

    VSASSERT(Enum->IsEnum(), "Non-enum unexpected!!!");

    if (Enum->IsBindingDone() ||
        DefinedInMetaData(Enum))
    {
        return true;
    }

    BCSYM *UnderlyingType = Enum->PClass()->GetUnderlyingTypeForEnum();

    if (!UnderlyingType ||
        !UnderlyingType->IsNamedType() ||
        UnderlyingType->PNamedType()->GetSymbol())
    {
        return true;
    }

#if IDE 
    if (!IsInCompilationThread(Enum->GetCompiler()))
    {
        return false;
    }
#endif IDE

    // UnderlyingType of enum is a NamedType that has not yet been resolved,
    // so resolve it if possible.

    return ResolveAllNamedTypesInContainerIfPossible(
                Enum,
                IDECompilationCaches,
                true);  // true indicates - ensure that none of the previous steps are in progress
}

// This is a helper for and only for BCSYM::DigThroughNamedType
// to resolve named types on demand when digging through
// a named type.
//
// This is need because during bindable, some constant
// expressions in declarations might need to be evaluated
// for which we might need to dig through the named types
// in containers which have not yet been through named type
// resolution.
//
// Returns true if the NamedType was resolved, else returns false.
//
void
Bindable::ResolveNamedTypeIfPossible
(
    BCSYM_NamedType *NamedType,
    CompilationCaches *IDECompilationCaches
)
{
    // NOTE: This is a helper for and only for BCSYM::DigThroughNamedType

    BCSYM_NamedRoot *ContextOfNamedType = NamedType->GetContext();

    VSASSERT( ContextOfNamedType,
                    "Named Types with NULL context unexpected!!!");

    BCSYM_Container *ContainerContextOfNamedType;
    if (ContextOfNamedType->IsContainer())
    {
        ContainerContextOfNamedType = ContextOfNamedType->PContainer();
    }
    else
    {
        ContainerContextOfNamedType = ContextOfNamedType->GetContainer();
    }

    if (ContainerContextOfNamedType)
    {
#if IDE 
        if (!IsInCompilationThread(ContainerContextOfNamedType->GetCompiler()))
        {
            return;
        }
#endif IDE

        Bindable::ResolveAllNamedTypesInContainerIfPossible(
            ContainerContextOfNamedType,
            IDECompilationCaches,
            false);     // false indicates - don't bother to check whether any of the previous steps are in progress
                        //
                        // We don't bother to ensure here because the assertion is that this should never be invoked
                        // when any of the previous steps are in progress.
                        //
                        // Assert in ResolveAllNamedTypesInContainerIfPossible to detect any rogue cases.
    }
}

// Returns true if resolved, else returns false.
//
bool
Bindable::ResolveAllNamedTypesInContainerIfPossible
(
    BCSYM_Container *Container,
    CompilationCaches *IDECompilationCaches,
    bool EnsureNoPreviousStepsAreInProgress
)
{
    VSASSERT( Container,
                "NULL container unexpected!!!");

    if (Container->IsBindingDone())
    {
        return true;
    }

    VSASSERT( !Container->GetSourceFile() || Container->GetSourceFile()->GetProject(),
                    "How can a container in source not have a containing project ?");

    SourceFile *ContainingSourceFile = Container->GetSourceFile();

    // Nothing to do for metadata container
    //
    if (!ContainingSourceFile)
    {
        return true;
    }

    CompilerProject *ContainingProject = ContainingSourceFile->GetProject();

    // If requested to resolve named types before completing declared,
    // then ignore this request.
    //
    if (!ContainingProject->IsDeclaredToBoundCompilationInProgress())
    {
        return false;
    }

    if (EnsureNoPreviousStepsAreInProgress)
    {
        // If the previous steps i.e ResolveBases, etc. are in progress,
        // then this step i.e. ResolveAllNamedTypes cannot yet be started.
        // Bug VSWhidbey 176508
        //
        if (!ContainingProject->HaveImportsBeenResolved() ||
            ContainingSourceFile->AreImportsBeingResolved() ||
            Container->GetBindableInstance()->GetStatusOfResolvePartialTypes() == Bindable::InProgress ||
            Container->GetBindableInstance()->GetStatusOfResolveBases() == Bindable::InProgress)
        {
            return false;
        }
    }

    VSASSERT(!(
                !ContainingProject->HaveImportsBeenResolved() ||
                ContainingSourceFile->AreImportsBeingResolved() ||
                Container->GetBindableInstance()->GetStatusOfResolvePartialTypes() == Bindable::InProgress ||
                Container->GetBindableInstance()->GetStatusOfResolveBases() == Bindable::InProgress
              ),
                "Unexpected call to ResolveAllNamedTypesForContainer - dependencies still in progress!!!");

    ResolveAllNamedTypesForContainer(Container, IDECompilationCaches);

    return true;
}

WCHAR *
Bindable::GetBasicRep
(
    BCSYM *Symbol,
    StringBuffer *BasicRep,
    BCSYM_Container *ContainerContext,
    BCSYM_GenericBinding *GenericBindingContext
)
{
    if (!ContainerContext)
    {
        ContainerContext = CurrentContainer();
    }

    Symbol->GetBasicRep(
        ContainerContext->GetCompiler(),
        ContainerContext,
        BasicRep,
        GenericBindingContext);

    return BasicRep->GetString();
}


STRING *
Bindable::GetQualifiedErrorName
(
    BCSYM *NamedRoot
)
{
    VSASSERT( NamedRoot->IsNamedRoot(), "Non-NamedRoot unexpected!!!");

    if (NamedRoot->IsGenericBinding())
    {
        GenericBinding *Binding = NamedRoot->PGenericBinding();
        return Binding->GetGeneric()->GetQualifiedName(true, CurrentContainer(), true, Binding);
    }
    else if (NamedRoot->IsContainer())
    {
        BCSYM_Container *Container = NamedRoot->PContainer();

        STRING *NameOfUnnamedNamespace = STRING_CONST(CurrentCompilerInstance(), EmptyString);

        if (StringPool::IsEqual(Container->GetName(), NameOfUnnamedNamespace))
        {
            VSASSERT( Container->IsNamespace() && !Container->GetContainer(),
                            "Root namespace expected!!!");

            return STRING_CONST(CurrentCompilerInstance(), UnnamedNamespaceErrName);
        }

        return Container->GetQualifiedName(true, CurrentContainer(), true /* Append type parameters */);

    }
    else if (NamedRoot->IsNamedRoot())
    {
        return NamedRoot->PNamedRoot()->GetQualifiedName(true, CurrentContainer());
    }

    return NULL;
}

WCHAR *
Bindable::GetErrorNameAndSig
(
    BCSYM *Symbol,
    BCSYM_GenericTypeBinding *GenericBindingContext,
    ErrorTable *ErrorLog,
    StringBuffer *TextBuffer
)
{
    AssertIfNull(Symbol);
    AssertIfNull(TextBuffer);

    return
        ErrorLog->ExtractErrorName(
            Symbol,
            // Fully qualify the name for non-members
            //
            Symbol->IsMember() ? Symbol->PMember()->GetContainer() : NULL,
            *TextBuffer,
            GenericBindingContext);
}

WCHAR *
Bindable::GetSigsForAmbiguousOverridingAndImplErrors
(
    DynamicArray<BCSYM_Proc *> &AmbiguousMembers,
    StringBuffer &Buffer
)
{
    for(unsigned Index = 0; Index < AmbiguousMembers.Count(); Index++)
    {
        StringBuffer Signature;

        GetBasicRep(
            AmbiguousMembers.Element(Index),
            &Signature,
            AmbiguousMembers.Element(Index)->GetContainer(),
            NULL);      // Display the unsubstituted signatures so that users can distinguish between the different members.

        ResLoadStringRepl(
                ERRID_OverriddenCandidate1,
                &Buffer,
                Signature.GetString());
    }

    return Buffer.GetString();
}

void
Bindable::ReportErrorOnSymbol
(
    ERRID ErrID,
    ErrorTable *ErrorLog,
    BCSYM *SymbolToReportErrorOn,
    _In_opt_z_ const STRING *ErrorReplString1,
    _In_opt_z_ const STRING *ErrorReplString2,
    _In_opt_z_ const STRING *ErrorReplString3,
    _In_opt_z_ const STRING *ErrorReplString4,
    _In_opt_z_ const STRING *ErrorReplString5,
    _In_opt_z_ const STRING *ErrorReplString6,
    _In_opt_z_ const STRING *ErrorReplString7,
    _In_opt_z_ const STRING *ErrorReplString8,
    _In_opt_z_ const STRING *ErrorReplString9
)
{
    if (ErrorLog)
    {
        ErrorLog->CreateErrorWithSymbol(
            ErrID,
            SymbolToReportErrorOn,
            ErrorReplString1,
            ErrorReplString2,
            ErrorReplString3,
            ErrorReplString4,
            ErrorReplString5,
            ErrorReplString6,
            ErrorReplString7,
            ErrorReplString8,
            ErrorReplString9);
    }
}

void
Bindable::ReportErrorOnSymbolWithBindingContexts
(
    ERRID ErrID,
    ErrorTable *ErrorLog,
    BCSYM *SymbolToReportErrorOn,
    BCSYM *Substitution1,
    BCSYM_GenericTypeBinding *GenericBindingContext1,
    BCSYM *Substitution2,
    BCSYM_GenericTypeBinding *GenericBindingContext2,
    BCSYM *Substitution3,
    BCSYM_GenericTypeBinding *GenericBindingContext3,
    BCSYM *Substitution4,
    BCSYM_GenericTypeBinding *GenericBindingContext4,
    BCSYM *Substitution5,
    BCSYM_GenericTypeBinding *GenericBindingContext5
)
{
    if (ErrorLog)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;
        StringBuffer TextBuffer4;
        StringBuffer TextBuffer5;

        ErrorLog->CreateErrorWithSymbol(
            ErrID,
            SymbolToReportErrorOn,
            Substitution1==NULL ? NULL : GetErrorNameAndSig(Substitution1, GenericBindingContext1, ErrorLog, &TextBuffer1),
            Substitution2==NULL ? NULL : GetErrorNameAndSig(Substitution2, GenericBindingContext2, ErrorLog, &TextBuffer2),
            Substitution3==NULL ? NULL : GetErrorNameAndSig(Substitution3, GenericBindingContext3, ErrorLog, &TextBuffer3),
            Substitution4==NULL ? NULL : GetErrorNameAndSig(Substitution4, GenericBindingContext4, ErrorLog, &TextBuffer4),
            Substitution5==NULL ? NULL : GetErrorNameAndSig(Substitution5, GenericBindingContext5, ErrorLog, &TextBuffer5));
    }
}

void
Bindable::ReportErrorOnSymbol
(
    ERRID ErrID,
    ErrorTable *ErrorLog,
    BCSYM *SymbolToReportErrorOn,
    BCSYM *Substitution1,
    BCSYM *Substitution2,
    BCSYM *Substitution3,
    BCSYM *Substitution4,
    BCSYM *Substitution5
)
{
    ReportErrorOnSymbolWithBindingContexts(
        ErrID,
        ErrorLog,
        SymbolToReportErrorOn,
        Substitution1,
        NULL,
        Substitution2,
        NULL,
        Substitution3,
        NULL,
        Substitution4,
        NULL,
        Substitution5,
        NULL);
}

void
Bindable::ReportErrorAtLocation
(
    ERRID ErrID,
    ErrorTable *ErrorLog,
    Location *Location,
    _In_opt_z_ STRING *ErrorReplString1,
    _In_opt_z_ STRING *ErrorReplString2,
    _In_opt_z_ STRING *ErrorReplString3,
    _In_opt_z_ STRING *ErrorReplString4,
    _In_opt_z_ STRING *ErrorReplString5,
    _In_opt_z_ STRING *ErrorReplString6,
    _In_opt_z_ STRING *ErrorReplString7,
    _In_opt_z_ STRING *ErrorReplString8,
    _In_opt_z_ STRING *ErrorReplString9
)
{
    if (ErrorLog)
    {
        VSASSERT( Location, "How can we report an error when we don't have a location ?");

        ErrorLog->CreateError(
            ErrID,
            Location,
            ErrorReplString1,
            ErrorReplString2,
            ErrorReplString3,
            ErrorReplString4,
            ErrorReplString5,
            ErrorReplString6,
            ErrorReplString7,
            ErrorReplString8,
            ErrorReplString9);
    }
}

void
Bindable::ReportErrorAtLocation
(
    ERRID ErrID,
    ErrorTable *ErrorLog,
    Location *Location,
    BCSYM *Substitution
)
{
    if (ErrorLog)
    {
        StringBuffer TextBuffer;

        ErrorLog->CreateError(
            ErrID,
            Location,
            ErrorLog->ExtractErrorName(
                Substitution,
                Substitution->IsProc() ?
                    Substitution->PProc()->GetContainer() :
                    NULL,
                TextBuffer));
    }
}

Location *
Bindable::GetTypeUseErrorLocation
(
    BCSYM *RawType,
    Location *DefaultLocation
)
{
    BCSYM *PossiblyNamedType = RawType->ChaseToNamedType();

    if (PossiblyNamedType &&
        PossiblyNamedType->IsNamedType() &&
        PossiblyNamedType->HasLocation())
    {
        return PossiblyNamedType->GetLocation();
    }

    return DefaultLocation;
}

bool
Bindable::CanOverload
(
    BCSYM_NamedRoot *Member,
    BCSYM_NamedRoot *OtherMember,
    unsigned &CompareFlags
)
{
    // Overloading is possible only if the members being considered are
    // both Methods or both Properties.

    if (BothAreMethodsOrBothAreProperties(Member, OtherMember))
    {
        CompareFlags = BCSYM::CompareProcs(Member->PProc(), (GenericBinding *)NULL, OtherMember->PProc(), (GenericBinding *)NULL, NULL);

        // EQ_Shape - Members that differ by parameter signature can overload each other.
        //
        // EQ_GenericMethodTypeParamCount - generic methods that differ only by the number of type
        // parameters can overload each other
        //
        // EQ_GenericTypeParams - Methods in generic types that can possibly unify for some type
        // arguments can overload each other
        //
        // Dev10 #792284
        // EQ_OptionalTypes - this means that two corresponding optional parameter have different types.
        // Effectively the same thing as EQ_Shape or EQ_GenericTypeParams, see BCSYM::CompareParams().
        if (CompareFlags & (EQ_Shape | EQ_GenericMethodTypeParamCount | EQ_GenericTypeParams | EQ_OptionalTypes))
        {
            return true;
        }

        // Dev10 #792284
        // EQ_Optional - this means that (see BCSYM::CompareParams())
        // a) there is a mismatch in Optional modifier for corresponding parameters; 
        //     or
        // b) one proc has more arguments than the other and all extra arguments are optional.
        //
        // We should accept b) as a valid overloading case.
        if ( (CompareFlags & EQ_Optional) != 0 && 
            Member->PProc()->GetParameterCount() != OtherMember->PProc()->GetParameterCount())
        {
            return true;
        }
        
        if (IsConversionOperator(Member) && IsConversionOperator(OtherMember))
        {
            // User defined conversion operators can additionally overload by return type.
            if (CompareFlags & EQ_Return)
            {
                return true;
            }

            // We allow possibly unifying methods to coexist when they are conversion operators
            if (CompareFlags & (EQ_GenericTypeParams | EQ_GenericTypeParamsForReturn))
            {
                return true;
            }
        }

        return false;
    }
    else if (Member->IsType() &&
             OtherMember->IsType() &&
             Member->GetGenericParamCount() != OtherMember->GetGenericParamCount())
    {
        CompareFlags = EQ_Shape;
        return true;
    }
    else
    {
        CompareFlags = EQ_Shape;
        return false;
    }
}

bool
Bindable::IsMethod
(
    BCSYM *Member
)
{
    if (Member->IsProc() &&
        !Member->IsProperty() &&
        !Member->IsEventDecl())
    {
        return true;    // Is a method
    }
    else
    {
        return false;
    }
}

bool
Bindable::IsProperty
(
    BCSYM *Member
)
{
    return Member->IsProperty();
}


bool
Bindable::IsSynthetic
(
    BCSYM_NamedRoot *Member,
    BCSYM_NamedRoot **SourceOfMember,
    BCSYM_NamedRoot **NamedContextOfMember,
    SyntheticContext *SyntheticContext
)
{
    BCSYM_NamedRoot *SourceSymbol = NULL;
    BCSYM_NamedRoot *NamedContextOfSyntheticMember = NULL;
    Bindable::SyntheticContext Context = UnKnown;

    if (SourceSymbol = Member->CreatedByEventDecl())
    {
        Context = EventDecl;
    }
    else if (SourceSymbol = Member->CreatedByWithEventsDecl())
    {
        if (SourceSymbol->GetContainer()->IsSameContainer(Member->GetContainer()))
        {
            Context = WithEventsDecl;
        }
        else
        {
            VSASSERT( Member->IsProperty() ||
                      (Member->IsSyntheticMethod() && Member->PProc()->GetAssociatedPropertyDef()),
                            "How can any other member be synthesized for a handles clause ?");

            Context = HandlesOfBaseWithEvents;

            if (!Member->GetContainer() ||
                !DefinedInMetaData(Member->GetContainer()))
            {
                if (Member->IsProperty())
                {
                    VSASSERT( Member->PProperty()->CreatedByHandlesClause(),
                                    "How can this member not be created due to a handles clause ?");
                    NamedContextOfSyntheticMember =
                        Member->PProperty()->CreatedByHandlesClause()->GetHandlingMethod();
                }
                else
                {
                    BCSYM_Property *AssociatedProperty = Member->PProc()->GetAssociatedPropertyDef();

                    VSASSERT( AssociatedProperty->CreatedByHandlesClause(),
                                    "How can this member not be created due to a handles clause ?");
                    NamedContextOfSyntheticMember =
                        AssociatedProperty->CreatedByHandlesClause()->GetHandlingMethod();
                }
            }
        }
    }
    else if (SourceSymbol = Member->GetAutoPropertyThatCreatedSymbol())
    {
        Context = AutoPropertyBackingField;
    }
    else if (Member->IsProc())
    {
        BCSYM_Proc *Proc = Member->PProc();
        if ((Proc->IsPropertyGet() || Proc->IsPropertySet()) &&
            (SourceSymbol = Proc->GetAssociatedPropertyDef()))  // Sometime imported getters and setters do not have
                                                                // an associated property if the property is bad
                                                                // in some way. So we will treat these a non-synthetic
                                                                // symbols and will generate any name clash errors
                                                                // against these symbols themselves
        {
            Context = PropertyGetOrSet;

            // Get/Set could be due to a withevents property.
            // So dig through the property to find the withevents variable
            //
            Bindable::SyntheticContext InnerContext;
            BCSYM_NamedRoot *InnerSourceSymbol;
            BCSYM_NamedRoot *InnerNamedContext;

            if (IsSynthetic(SourceSymbol, &InnerSourceSymbol, &InnerNamedContext, &InnerContext))
            {
                SourceSymbol = InnerSourceSymbol;
                NamedContextOfSyntheticMember = InnerNamedContext;
                Context = InnerContext;
            }
        }
        else if (Proc->IsUserDefinedOperatorMethod())
        {
            SourceSymbol = Proc->PMethodDecl()->GetAssociatedOperatorDef();
            Context = OperatorMethod;
        }
    }


    if (SourceOfMember)
    {
        *SourceOfMember = SourceSymbol;
    }

    if (NamedContextOfMember)
    {
        if (NamedContextOfSyntheticMember)
        {
            *NamedContextOfMember = NamedContextOfSyntheticMember;
        }
        else
        {
            *NamedContextOfMember = SourceSymbol;
        }
    }

    if (SyntheticContext)
    {
        *SyntheticContext = Context;
    }

    if (SourceSymbol)
    {
        return true;
    }
    else
    {
        return false;
    }
}


/*****************************************************************************
;ResolveAllNamedTypes

Resolve all of the named types in a list.
This little function exists to expose this functionality outside of Bindable
*****************************************************************************/
void
Bindable::ResolveAllNamedTypes
(
    BCSYM_NamedType *NamedTypeList,   // [in] The list of named type symbols to be resolved.
    NorlsAllocator *Allocator, // [in] allocator to use
    ErrorTable *ErrorLog,  // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance,  // [in] compiler context
    CompilerHost *CompilerHost,    // [in] the compiler host
    SourceFile *OwningSourceFile, // [in] current file
    TypeResolutionFlags Flags,   // [in] indicates how to resolve the type
    CompilationCaches *IDECompilationCaches
)
{

    // Haul through all the named types in the list we were passed and resolve them
    for (BCSYM_NamedType *ThisNamedType = NamedTypeList;
         ThisNamedType;
         ThisNamedType = ThisNamedType->GetNext())
    {
        ResolveNamedType(
            ThisNamedType,
            Allocator,
            ErrorLog,
            CompilerInstance,
            CompilerHost,
            OwningSourceFile,
            Flags,
            IDECompilationCaches,
            NameNoFlags,
            false /* don't disable name lookup caching */);
    } // haul through named types
}


/*****************************************************************************
;ResolveNamedType

Resolve a single named type and do some semantics on it such as determing if
it is IsNew and if so whether IsNew is legal; make sure they aren't using a
private UDT illegally, Does obsolete checking, etc.
*****************************************************************************/
void
Bindable::ResolveNamedType
(
    BCSYM_NamedType *ThisNamedType, // [in] the named type to resolve
    NorlsAllocator *Allocator,     // [in] for the Binder - may be NULL if no generic bindings occur in the type
    ErrorTable *ErrorLog,          // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance,    // [in] the current context
    CompilerHost *CompilerHost,    // [in] the compilerhost
    SourceFile *CurrentSourceFile, // [in] the current file
    TypeResolutionFlags Flags,     // [in] indicates how to resolve the type
    CompilationCaches *IDECompilationCaches,
    NameFlags NameInterpretFlags,  // [in] indicates the name binding
    bool DisableNameLookupCaching  // [in] indicates that name look up caching in semantics helpers
                                   //   should be disabled
)
{
    Location        Loc;
    Loc.SetLocation(0,0);
    bool            IsBadName = false;
    BCSYM_NamedRoot *BoundSymbol = NULL; // the symbol that semantics says represents this named type
    BCSYM_Hash      *ContextHash = NULL; // the container where the named type was found.
    BCSYM_Hash      *LookupHash = NULL;  // the container where to start looking for the namedtype
    Symbols
        SymbolAllocator(
        CompilerInstance,
        Allocator,
        NULL,
        CurrentSourceFile->GetCurrentGenericBindingCache());
    if (ThisNamedType->HasLocation())
    {
        Loc = *ThisNamedType->GetLocation();
    }

    VSASSERT( !ThisNamedType->IsTHISType() || ThisNamedType->GetNameCount() == 0,
                "Inconsistency in named type!!!");

    if ( ThisNamedType->GetNameCount() == 0 )
    {
        if (ThisNamedType->IsTHISType())
        {
            BCSYM_NamedRoot *NamedContext = ThisNamedType->GetContext();

            BCSYM_Container *ContainingType =
                NamedContext->IsContainer() ?
                    NamedContext->PContainer() :
                    NamedContext->GetContainer();

            if (ContainingType->IsPartialTypeAndHasMainType())
            {
                ContainingType = ContainingType->GetMainType();
            }

            VSASSERT(ContainingType->IsClass(), "Class expected!!!");

            if (IsGenericOrHasGenericParent(ContainingType))
            {

                BoundSymbol =
                    SynthesizeOpenGenericBinding(
                        ContainingType,
                        SymbolAllocator);
            }
            else
            {
                BoundSymbol = ContainingType;
            }
        }
        else
        {
            if (!CurrentSourceFile)
            {
                if (ThisNamedType->GetContext())
                {
                    if (IsUnnamedNamespace(ThisNamedType->GetContext(), CompilerInstance))
                    {
                        BoundSymbol = ThisNamedType->GetContext()->PNamespace()->GetHash();
                    }
                    else
                    {
                        VSFAIL("UnnamedNamespace not found!!!");
                    }
                }
                else
                {
                    VSFAIL("UnnamedNamespace not found!!!");
                }
            }
            else
            {
                // can happen when the type name is only "GlobalNameSpace". In prettylisting could show up
                BoundSymbol = CurrentSourceFile->GetUnnamedNamespace();
            }

            VSASSERT( ThisNamedType->IsGlobalNameSpaceBased(), "Only GlobalNameSpace can have no qualified id" );
        }
    }
    else
    {
        BCSYM_NamedRoot *Context = ThisNamedType->GetContext();

        // Need to do this so that for statics within generic methods, we can bind the types
        // from the context of the generic method so that we can see the generic method's
        // type parameters too during binding.
        //
        // Need to do this although statics are not allowed inside generic methods in order to
        // report any type binding errors on the type of the variable.
        //
        if (Context->IsStaticLocalBackingField() &&
            Context->PStaticLocalBackingField()->GetProcDefiningStatic() &&
            Context->PStaticLocalBackingField()->GetProcDefiningStatic()->IsGeneric())
        {
            Context = Context->PStaticLocalBackingField()->GetProcDefiningStatic();
        }

        if (Context->IsContainer() &&
            (Context->IsNamespace() ||
                !ThisNamedType->IsUsedInAppliedAttributeContext()))     // Attributes on a non-namespace container need to be bound in the
                                                                        // context of the container's parent
        {
            ContextHash = Context->PContainer()->GetHash();
        }
        else if (Context->IsProc() && Context->PProc()->IsGeneric())
        {
            ContextHash = Context->PProc()->GetGenericParamsHash();
        }
        else
        {
            BCSYM_NamedRoot *ContainerOrMethod = Context->GetContainerOrContainingMethod();

            if (ContainerOrMethod->IsContainer())
            {
                ContextHash = ContainerOrMethod->PContainer()->GetHash();
            }
            else if (ContainerOrMethod->IsProc() && ContainerOrMethod->PProc()->IsGeneric())
            {
                ContextHash = ContainerOrMethod->PProc()->GetGenericParamsHash();
            }
            else
            {
                ContextHash = ContainerOrMethod->GetContainer()->GetHash();
            }
        }

        if (ThisNamedType->IsGlobalNameSpaceBased())
        {
            if (!CurrentSourceFile)
            {
                if (ThisNamedType->GetContext())
                {
                    if (IsUnnamedNamespace(ThisNamedType->GetContext(), CompilerInstance))
                    {
                        LookupHash = ThisNamedType->GetContext()->PNamespace()->GetHash();
                    }
                    else
                    {
                        VSFAIL("UnnamedNamespace not found!!!");
                    }
                }
                else
                {
                    VSFAIL("UnnamedNamespace not found!!!");
                }
            }
            else
            {
                LookupHash = CurrentSourceFile->GetUnnamedNamespace()->GetHash();
            }

            NameInterpretFlags |= NameSearchGlobalNameButLookIntoModule;
        }
        else
        {
            LookupHash = ContextHash;
        }

        // If this named type is from the context of an applied attribute, then determine
        // the applied attribute context.
        //
        BCSYM_NamedRoot *AppliedAttributeContext =
            ThisNamedType->IsUsedInAppliedAttributeContext() ?
                ThisNamedType->GetContext() :
                NULL;

        BoundSymbol =
            Semantics::EnsureNamedRoot
            (
                Semantics::InterpretQualifiedName
                (
                    ThisNamedType->GetNameArray(),
                    ThisNamedType->GetNameCount(),
                    ThisNamedType->GetArgumentsArray(),
                    Allocator,
                    LookupHash,
                    NameInterpretFlags |
                    NameSearchTypeReferenceOnly |
                    NameSearchIgnoreExtensionMethods |
                    (ThisNamedType->IsAttributeName() ?
                        NameSearchAttributeReference :
                        NameNoFlags),
                    Loc,
                    ErrorLog,
                    CompilerInstance,
                    CompilerHost,
                    IDECompilationCaches,
                    CurrentSourceFile,
                    Flags & TypeResolvePerformObsoleteChecks,
                    IsBadName,
                    DisableNameLookupCaching,
                    ContextHash,
                    AppliedAttributeContext
                )
            );
    }

    if ( IsBadName )
    {
        //AssertIfFalse(!ErrorLog || ErrorLog->HasErrors());
        BoundSymbol = Symbols::GetGenericBadNamedRoot();
    }

    ERRID errid = 0;
    if (!BoundSymbol)
    { // The type is not defined.
        if (ThisNamedType->GetNameCount() == 1)
        {
            if (StringPool::IsEqual(
                    ThisNamedType->GetNameArray()[0],
                    CompilerInstance->TokenToString(tkCURRENCY)))
            {
                // Give a good error on currency
                errid = ERRID_ObsoleteDecimalNotCurrency;
            }
            else if (StringPool::IsEqual(
                        ThisNamedType->GetNameArray()[0],
                        CompilerInstance->TokenToString(tkANY)) &&
                     ThisNamedType->GetContext()->IsDllDeclare())
            {
                // Give a good error on As Any
                errid = ERRID_ObsoleteAsAny;
            }
            else
            {
                errid = ERRID_UndefinedType1;
            }
        }
        else
        {
            errid = ERRID_UndefinedType1;
        }
    }
    else if (!BoundSymbol->IsType() && !BoundSymbol->IsBad())
    { // if the type name is bad than the error generate for it will be sufficient - otherwise log the missing type error
        errid = ERRID_UnrecognizedType;
    }
    else if (ErrorLog &&
             BoundSymbol->DigThroughAlias() != NULL &&
             BoundSymbol->DigThroughAlias()->IsClass() &&
             (BoundSymbol->DigThroughAlias()->PClass()->IsStdModule() ||
              BoundSymbol == CompilerHost->GetFXSymbolProvider()->GetType(FX::VoidType)) &&
             (!(Flags & TypeResolveAllowAllTypes) || ThisNamedType->IsTypeArgumentForGenericType()))
    {
        // A module or Void can't be used as a type in all cases except GetType()
        if (BoundSymbol->DigThroughAlias()->PClass()->IsStdModule())
            errid = ERRID_ModuleAsType1;
        else
            errid = ERRID_BadUseOfVoid;
    }
    else if (ThisNamedType->IsAttributeName() &&
            (BoundSymbol->IsGenericTypeBinding() || BoundSymbol->IsGenericParam()))
    {
        errid = ERRID_AttrCannotBeGenerics;
    }
    else if (ThisNamedType->GetContext()->IsVariable())
    {
        BCSYM_Variable *Context = ThisNamedType->GetContext()->PVariable();

        // We don't want to give errors for the type arguments to generic classes,
        // so skip the validation unless the type is the type of the variable
        //
        if (Context->GetRawType() == ThisNamedType)
        {
            // If the variable is marked As New, make sure the type can be created
            // (isn't abstract, has an appropriate constructor, etc)
            //
            if (Context->IsNew() &&
                Context->IsLocal())     // Need to delay DimAsNewSemantics for member variables
                                        // because the constraint type for type params in the parent
                                        // will not yet be bound and cycle checking of constraints
                                        // will not yet be done.
            {
                // Need to do this here so bilgen won't throw an error about default needed.
                // We'll catch abstract creation errors in ResolveImplements()
                //
                DimAsNewSemantics(
                    BoundSymbol,
                    errid);
            }

            // Constant variables must be intrinsic
            //
            if (Context->IsConstant() &&
                !ThisNamedType->IsAttributeName() &&
                !IsTypeValidForConstantVariable(BoundSymbol))
            {
                errid = ERRID_ConstAsNonConstant;
            }
        }
    }

    if (!errid)
    {
        ThisNamedType->SetSymbol(BoundSymbol->DigThroughAlias());

        // We don't need to check for bad symbols with errid here because
        // semantics will already have done that in InterpretQualifiedName
        //
        if (!BoundSymbol->IsGenericBadNamedRoot())
        {
            // Dev10 #735384 Check if we are improperly using embedded types
            if (ErrorLog != NULL && ThisNamedType->HasLocation())
            {
                CompilerProject * pCurrentProject = NULL;

                if (CurrentSourceFile != NULL)
                {
                    pCurrentProject = CurrentSourceFile->GetCompilerProject();
                }
                else 
                {
                    BCSYM_NamedRoot * pNamedContext = ThisNamedType->GetContext();

                    if (pNamedContext != NULL)
                    {
                        pCurrentProject = pNamedContext->GetContainingProject();
                    }
                }
                
                if (pCurrentProject != NULL && pCurrentProject->NeedToCheckUseOfEmbeddedTypes())
                {
                    TypeHelpers::CheckClassesForUseOfEmbeddedTypes(
                        ThisNamedType->GetSymbol(),            
                        pCurrentProject,
                        ErrorLog,
                        ThisNamedType->GetLocation());
                }
            }
        
            return;
        }
    }
    else
    {
        // Log error discovered above
        // Build the qualified name.
        StringBuffer QualifiedName;
        unsigned CountOfNames = ThisNamedType->GetNameCount();

        for(unsigned i = 0; i < CountOfNames; i++)
        {
            if (ThisNamedType->GetNameArray()[i])
            {
                QualifiedName.AppendString( ThisNamedType->GetNameArray()[i]);

                if (i != CountOfNames - 1)
                {
                    QualifiedName.AppendChar( L'.');
                }
            }
        }

        if (ErrorLog)
        {
            if(!ErrorLog->HasThisErrorWithSymLocation(errid,ThisNamedType))
            {
                ErrorLog->CreateErrorWithSymbol(
                    errid,
                    ThisNamedType,
                    QualifiedName.GetString(),
                    ThisNamedType->GetContext() ?                               // not used by all errids that come through here
                        ThisNamedType->GetContext()->GetName() :
                        L"",
                    ThisNamedType->GetContext()->GetContainer() ?               // not used by all errids that come through here
                        ThisNamedType->GetContext()->GetContainer()->GetName() :
                        L"");
            }
        }

        ThisNamedType->SetSymbol(Symbols::GetGenericBadNamedRoot());

        // If one of the named types behind a parameter or a return type is bad, mark the procedure as bad
        //
        // Note that if the attribute applied to a procedure is bad, then that does not mean the procedure
        // is bad
        //
    }

    // Microsoft:2008.09.21. Consider handling the following code differently.
    // Notionally we're in "bindable", so if GetContext() is a proc then we're assumed to be binding a method
    // signature, and that's why we mark the method as BadParamTypeOrReturnType. It's UGLY for the
    // function ResolveNamedType to have an undocumented side effect on the context of the argument it was
    // given, but so it goes.
    // More problematically, we are also invoked when interpreting a lambda.
    // Notionally, we should likewise be marking the lambda as BadParamTypeOrReturnType.
    // But (1) lambdas don't even have such a field, and (2) even if they did, GetContext() points
    // to the proc that contained the lambda; not to the lambda itself.
    // As a workaround, lambda-interpretation passes the flag "TypeResolveDontMarkContainingProcAsBad" 
    // to suppress any marking-as-bad. And the lambda-interpretation (in LambdaSemantics::InterpretLambdaParameters)
    // will mark the appropriate lambda paramers as bad after this function has returned.
    // Dev10#489103.
    if (ThisNamedType->GetContext()->IsProc() && !ThisNamedType->IsAttributeName() && !(Flags & TypeResolveDontMarkContainingProcAsBad))
    {
        ThisNamedType->GetContext()->PProc()->SetIsBad();

        // Also, indicate that the problem here is with a type associated with the proc, not the proc itself.
        ThisNamedType->GetContext()->PProc()->SetIsBadParamTypeOrReturnType(true);
    }
}

BCSYM_GenericBinding *
Bindable::ResolveGenericType
(
    BCSYM_NamedRoot *GenericType,  // [in] the generic
    NameTypeArguments *Arguments,  // [in] the arguments
    BCSYM_GenericTypeBinding *ParentBinding,
    ErrorTable *ErrorLog,          // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance,    // [in] the current context
    Symbols *SymbolFactory
)
{
    bool IsBad = false;
    Location ErrorLoc;
    ErrorLoc.Invalidate();

    ValidateArity(
        GenericType->GetName(),
        GenericType,
        ParentBinding,
        Arguments->m_ArgumentCount,
        Arguments->GetAllArgumentsLocation(&ErrorLoc),
        ErrorLog,
        CompilerInstance,
        IsBad);

    if (IsBad)
    {
    }
    // For generic import alias A=Class1(Of Integer), reference to "A" can result in this condition.
    //
    else if (Arguments->m_ArgumentCount == 0 &&
             ParentBinding &&
             ParentBinding->GetGeneric() == GenericType)
    {
        return ParentBinding;
    }

    else if (ResolveGenericArguments(
                GenericType,
                GenericType->GetFirstGenericParam(),
                Arguments,
                ErrorLog,
                CompilerInstance))
    {
        IsBad = true;
    }

    // Absence of the type for the type arguments implies the specification for an uninstantiated
    // generic type in a gettype expression. Eg: GetType(C1(Of ,))
    //
    else if (Arguments->m_ArgumentCount > 0 &&
             !Arguments->m_TypesForTypeArgumentsSpecified)
    {
        // Uninstantiated generic, so return NULL binding
        //
        return NULL;
    }

    BCSYM_GenericBinding *Binding =
        SymbolFactory->GetGenericBinding(
            IsBad,
            GenericType,
            Arguments->m_Arguments,
            Arguments->m_ArgumentCount,
            ParentBinding);

    // Store the generic binding to which the type arguments are associated
    //
    Arguments->m_BoundGenericBinding = Binding;

    return Binding;
}

void
Bindable::ValidateArity
(
    _In_z_ STRING *Name,                           // [in] the name being searched for
    BCSYM_NamedRoot *Generic,               // [in] the generic, can be NULL
    BCSYM_GenericTypeBinding *ParentBinding,// [in] the parent binding if any for this generic
    unsigned ArgumentCount,                 // [in] the number of type arguments for Generic
    Location *Loc,                          // [in] the location of the type arguments
    ErrorTable *ErrorLog,                   // [in] the error log to put our errors into - NULL in UI cases
    Compiler *CompilerInstance,             // [in] the current compiler context
    bool &IsBad                             // [out] returns true if any arity mismatch issues found - caller needs to initialize
)
{
    VSASSERT(IsBad || Generic, "Unexpected NULL argument!!!");

    if (IsBad)
    {
        if (ErrorLog)
        {
            // No accessible '|1' accepts this number of type arguments.
            //
            // No accessible non-generic '|1' found.

            ErrorLog->CreateError(
                ArgumentCount == 0 ?
                    ERRID_NoAccessibleNonGeneric1 :
                    ERRID_NoAccessibleGeneric1,
                Loc,
                Name);
        }

        return;
    }

    if (Generic->IsBad())
    {
        IsBad = true;
    }

    else if (ArgumentCount > 0 &&
             (!Generic->IsGeneric() ||
              // For generic import alias A=Class1(Of Integer),
              // A(Of Double) can result in this condition.
              //
              (ParentBinding &&
               ParentBinding->GetGeneric() == Generic)))
    {
        if (ErrorLog)
        {
            StringBuffer ErrorStringForSymbol;

            ErrorLog->CreateError(
                ERRID_TypeOrMemberNotGeneric1,
                Loc,
                !Generic->IsGeneric() ?
                    ErrorLog->ExtractErrorName(Generic, NULL, ErrorStringForSymbol) :
                    ErrorLog->ExtractErrorName(ParentBinding, NULL, ErrorStringForSymbol));
        }

        IsBad = true;
    }

    else if (ArgumentCount == 0 &&
             ParentBinding &&
             ParentBinding->GetGeneric() == Generic)
    {
    }

    else
    {
        ERRID ErrId = 0;
        unsigned ParameterCount = Generic->GetGenericParamCount();

        if (ParameterCount == ArgumentCount)
        {
        }
        else if (ParameterCount == 0)
        {
            ErrId = ERRID_TypeOrMemberNotGeneric1;
        }
        else if (ParameterCount < ArgumentCount)
        {
            ErrId = ERRID_TooManyGenericArguments1;
        }
        else if (ParameterCount > ArgumentCount)
        {
            ErrId = ERRID_TooFewGenericArguments1;
        }

        if (ErrId)
        {
            if (ErrorLog)
            {
                StringBuffer ErrorStringForSymbol;

                ErrorLog->CreateError(
                    ErrId,
                    Loc,
                    ErrorLog->ExtractErrorName(Generic, NULL, ErrorStringForSymbol));
            }

            IsBad = true;
        }
    }

    return;
}

void
Bindable::CheckAllGenericConstraints
(
    BCSYM_NamedType *NamedTypeList,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    CompilationCaches *IDECompilationCaches
)
{
    for (BCSYM_NamedType *ThisNamedType = NamedTypeList;
         ThisNamedType;
         ThisNamedType = ThisNamedType->GetNext())
    {
        CheckGenericConstraints(
            ThisNamedType,
            ErrorLog,
            CompilerHostInstance,
            CompilerInstance,
            SymbolFactory,
            IDECompilationCaches);
    }
}

void
Bindable::CheckGenericConstraints
(
    BCSYM_NamedType *NamedType,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    CompilationCaches *IDECompilationCaches,
    bool CheckForArgumentBindingsToo
)
{
    // Orcas behavior was to guard this loop with "!NamedType->IsBad()", but Orcas behavior
    // was also that NamedType->IsBad() always returned FALSE!
    // The idea behind the guard was correct, and for Dev10#489103 we changed
    // the behavior of IsBad() to return true for the case where it's a NamedType pointing
    // to a bad type.
    if (!NamedType->IsBad() &&
        NamedType->GetArgumentsArray())
    {
        for (unsigned Index = 0; Index < NamedType->GetNameCount(); Index++)
        {
            NameTypeArguments *TypeArguments = NamedType->GetArgumentsArray()[Index];

            if (!TypeArguments)
            {
                continue;
            }

            BCSYM_GenericBinding *Binding =  TypeArguments->m_BoundGenericBinding;

            CheckGenericConstraints(
                TypeArguments->m_BoundGenericBinding,
                NULL,
                TypeArguments,
                ErrorLog,
                CompilerHostInstance,
                CompilerInstance,
                SymbolFactory,
                IDECompilationCaches);

            // if requested, recursively check the constraints for the bindings among the type arguments
            // of the current binding too.
            //
            if (CheckForArgumentBindingsToo)
            {
                CheckGenericConstraintsForBindingsInTypeArguments(
                    TypeArguments,
                    ErrorLog,
                    CompilerHostInstance,
                    CompilerInstance,
                    SymbolFactory,
                    IDECompilationCaches);
            }
        }
    }
}

void
Bindable::CheckGenericConstraintsForBindingsInTypeArguments
(
    NameTypeArguments *TypeArguments,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    CompilationCaches *IDECompilationCaches
)
{
    if (!TypeArguments || !TypeArguments->m_TypesForTypeArgumentsSpecified)
    {
        return;
    }

    for (unsigned ArgumentIndex = 0; ArgumentIndex < TypeArguments->m_ArgumentCount; ArgumentIndex++)
    {
        CheckGenericConstraints(
            TypeArguments->m_Arguments[ArgumentIndex],
            ErrorLog,
            CompilerHostInstance,
            CompilerInstance,
            SymbolFactory,
            IDECompilationCaches,
            true);      // recursively check the constraints for the bindings among the type arguments of the
                        // current binding too
    }
}

bool
Bindable::CheckGenericConstraints
(
    BCSYM_GenericBinding *Binding,
    Location TypeArgumentLocations[],
    NameTypeArguments *TypeArgumentsLocationHolder,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    CompilationCaches *IDECompilationCaches
)
{
    // Only one of TypeArgumentLocations and TypeArgumentsLocationHolder can be passed in
    //
    VSASSERT( !(TypeArgumentLocations && TypeArgumentsLocationHolder),
                    "Two sets of type argument locations unexpected!!!");

    if (!Binding || Binding->IsBadGenericBinding())
    {
        return false;
    }

    if (Binding->GetGeneric()->IsContainer())
    {
        // The constraint types need to be resolved.
        ResolveAllNamedTypesForContainer(Binding->GetGeneric()->PContainer(), IDECompilationCaches);
    }

    bool paramsValid = true;

    for (BCSYM_GenericParam *Parameter = Binding->GetGeneric()->GetFirstGenericParam(); Parameter; Parameter = Parameter->GetNextParam())
    {
        BCSYM *ArgumentType = Binding->GetArgument(Parameter->GetPosition());

        if (ArgumentType->IsBad())
        {
            continue;
        }

        if (ArgumentType->IsContainer())
        {
            // Need to ensure that all bases and implements have been resolved before
            // this arguments can be validated against the constraints for the corres-
            // -ponding type parameter
            //
            ResolveAllNamedTypesForContainer(ArgumentType->PContainer(), IDECompilationCaches);
        }

        CheckRestrictedType(
            ERRID_RestrictedType1,
            ArgumentType->ChaseToType(),
            GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations),
            CompilerHostInstance,
            ErrorLog);

        // Validate the argument type against the type constraints of the parameter.

        if (!
            ValidateGenericConstraintsForType(
                ArgumentType,
                Parameter,
                Binding,
                TypeArgumentLocations,
                TypeArgumentsLocationHolder,
                ErrorLog,
                CompilerHostInstance,
                CompilerInstance,
                SymbolFactory
            )
        )
        {
            paramsValid = false;
        }
    }

    return paramsValid;
}

BCSYM_Proc *
Bindable::GetPublicParameterlessConstructor
(
    BCSYM_Class *Class,
    Compiler    *CompilerInstance
)
{
    BCSYM_Proc *Constructor = NULL;

    for(Constructor = Class->GetFirstInstanceConstructor(CompilerInstance);
        Constructor;
        Constructor = Constructor->GetNextInstanceConstructor())
    {
        if (Constructor->GetParameterCount() == 0 &&
            Constructor->GetAccess() == ACCESS_Public)
        {
            break;
        }
    }
    return Constructor;
}

bool
Bindable::ValidateTypeConstraintOnPartiallyBoundExtensionMethod
(
    Procedure * pProc,
    PartialGenericBinding * pPartialGenericBinding,
    GenericTypeConstraint * pConstraint,
    Type * pTypeParameterValue,
    Symbols * pSymbols,
    CompilerHost * pCompilerHost
)
{
    ThrowIfNull(pConstraint);
    ThrowIfNull(pPartialGenericBinding);


    Type * pConstraintType =
        ReplaceGenericParametersWithArguments
        (
            pConstraint->PGenericTypeConstraint()->GetType(),
            pPartialGenericBinding->m_pGenericBinding,
            *pSymbols
        );

    return
        ! RefersToGenericParameter
        (
            pConstraintType,
            pProc,
            NULL,
            NULL,
            0,
            0,
            false,
            pPartialGenericBinding->m_pFixedTypeArgumentBitVector
        )
        &&
        ValidateTypeConstraintForType
        (
            pTypeParameterValue,
            pConstraintType,
            pCompilerHost,
            pSymbols
        );
}

bool
Bindable::ValidateConstraintOnPartiallyBoundExtensionMethod
(
    BCSYM_Proc * pProc,
    PartialGenericBinding * pPartialGenericBinding,
    GenericConstraint * pConstraint,
    GenericParameter * pParameter,
    Type * pTypeParameterValue,
    Symbols * pSymbols,
    CompilerHost * pCompilerHost
)
{
    return
    (
        (
            pConstraint->IsGenericTypeConstraint() &&
            ValidateTypeConstraintOnPartiallyBoundExtensionMethod
            (
                pProc,
                pPartialGenericBinding,
                pConstraint->PGenericTypeConstraint(),
                pTypeParameterValue,
                pSymbols,
                pCompilerHost
            )
        ) ||
        (
            pConstraint->IsReferenceConstraint() &&

            TypeHelpers::IsReferenceType(pTypeParameterValue)
        ) ||
        (
            pConstraint->IsValueConstraint() &&
            TypeHelpers::IsValueType(pTypeParameterValue)
        ) ||
        (
            pConstraint->IsNewConstraint() &&
            ValidateNewConstraintForType(pTypeParameterValue, pParameter, NULL, NULL, NULL, pCompilerHost->GetCompiler())
        )
    );

}


bool
Bindable::ValidateGenericConstraintsOnPartiallyBoundExtensionMethod
(
    BCSYM_Proc * pProc,
    PartialGenericBinding * pPartialGenericBinding,
    Symbols * pSymbols,
    CompilerHost * pCompilerHost
)
{
    ThrowIfNull(pProc);
    ThrowIfNull(pSymbols);
    ThrowIfNull(pCompilerHost);

    if (! pPartialGenericBinding)
    {
        return true;
    }


    GenericParameter * pParameter = pProc->GetFirstGenericParam();

    while (pParameter)
    {
        if (pPartialGenericBinding->IsFixed(pParameter))
        {
            Type * pTypeParameterValue = pPartialGenericBinding->m_pGenericBinding->GetCorrespondingArgument(pParameter);

            GenericConstraint * pConstraint = pParameter->GetConstraints();

            while (pConstraint)
            {
                if
                (
                    !
                    ValidateConstraintOnPartiallyBoundExtensionMethod
                    (
                        pProc,
                        pPartialGenericBinding,
                        pConstraint,
                        pParameter,
                        pTypeParameterValue,
                        pSymbols,
                        pCompilerHost
                    )
                )
                {
                    return false;
                }
                pConstraint = pConstraint->Next();
            }
        }
        pParameter = pParameter->GetNextParam();
    }
    return true;
}


bool
Bindable::ValidateGenericConstraintsForType
(
    BCSYM *Type,
    BCSYM_GenericParam *Parameter,
    BCSYM_GenericBinding *Binding,
    Location TypeArgumentLocations[],
    NameTypeArguments *TypeArgumentsLocationHolder,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    bool IgnoreProjectEquivalence,                  // Consider types as equivalent ignoring their projects' equivalence.
    CompilerProject **ProjectForType,               // The project containing a component of type "Type" that was considered
                                                    // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForGenericParam        // The project containing a component of type "Parameter" that was considered
                                                    // for a successful match ignoring project equivalence.
)
{
    // Validate the argument type against the type constraints of the parameter.

    // Only one of TypeArgumentLocations and TypeArgumentsLocationHolder can be passed in
    //
    VSASSERT( !(TypeArgumentLocations && TypeArgumentsLocationHolder),
                    "Two sets of type argument locations unexpected!!!");

    bool ConstraintsSatisfied = true;

    for (BCSYM_GenericConstraint *Constraint = Parameter->GetConstraints(); Constraint; Constraint = Constraint->Next())
    {
        if (Constraint->IsBadConstraint())
        {
            continue;
        }

        if (Constraint->IsGenericTypeConstraint())
        {
            // Validate the argument type against the type constraint of the parameter.

            if (!ValidateTypeConstraintForType(
                    Type,
                    Parameter,
                    Constraint->PGenericTypeConstraint(),
                    Binding,
                    TypeArgumentLocations,
                    TypeArgumentsLocationHolder,
                    ErrorLog,
                    CompilerHostInstance,
                    CompilerInstance,
                    SymbolFactory,
                    IgnoreProjectEquivalence,
                    ProjectForType,
                    ProjectForGenericParam))
            {
                ConstraintsSatisfied = false;
            }
        }
        else if (Constraint->IsNewConstraint())
        {
            // Validate the argument type against the "New" constraint of the parameter.

            if (!ValidateNewConstraintForType(
                    Type,
                    Parameter,
                    TypeArgumentLocations,
                    TypeArgumentsLocationHolder,
                    ErrorLog,
                    CompilerInstance))
            {
                ConstraintsSatisfied = false;
            }
        }
        else if (Constraint->IsReferenceConstraint())
        {
            // Validate the argument type against the "Class" constraint of the parameter.

            if (!ValidateReferenceConstraintForType(
                    Type,
                    Parameter,
                    TypeArgumentLocations,
                    TypeArgumentsLocationHolder,
                    ErrorLog))
            {
                ConstraintsSatisfied = false;
            }
        }
        else if (Constraint->IsValueConstraint())
        {
            // Validate the argument type against the "Structure" constraint of the parameter.

            if (!ValidateValueConstraintForType(
                    Type,
                    Parameter,
                    TypeArgumentLocations,
                    TypeArgumentsLocationHolder,
                    ErrorLog,
                    CompilerHostInstance,
                    CompilerInstance,
                    TypeHelpers::IsNullableType(Binding,CompilerHostInstance)))
            {
                ConstraintsSatisfied = false;
            }
        }

        // Short circuit by returning as soon as validation fails when no error reporting is required
        //
        if (!ConstraintsSatisfied &&
            !ErrorLog)
        {
            break;
        }
    }

    return ConstraintsSatisfied;
}

// Returns true if constraint is satisfied, else false
//
bool
Bindable::ValidateTypeConstraintForType
(
    BCSYM *Type,
    BCSYM_GenericParam *Parameter,
    BCSYM_GenericTypeConstraint *Constraint,
    BCSYM_GenericBinding *Binding,
    Location TypeArgumentLocations[],
    NameTypeArguments *TypeArgumentsLocationHolder,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    bool IgnoreProjectEquivalence,                  // Consider types as equivalent ignoring their projects' equivalence.
    CompilerProject **ProjectForType,               // The project containing a component of type "Type" that was considered
                                                    // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForConstraint          // The project containing a component of the "Constraint"'s type that was considered
                                                    // for a successful match ignoring project equivalence.
)
{
    BCSYM *ConstraintType = Constraint->GetType();

    ConstraintType =
        ReplaceGenericParametersWithArguments(
            ConstraintType,
            Binding,
            *SymbolFactory);

    if (!ValidateTypeConstraintForType(
            Type,
            ConstraintType,
            CompilerHostInstance,
            SymbolFactory,
            IgnoreProjectEquivalence,
            ProjectForType,
            ProjectForConstraint))
    {
        if (ErrorLog)
        {
            StringBuffer ArgumentBuffer;
            StringBuffer ConstraintBuffer;
            Location* ErrorLocation = GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations);

            WCHAR* ArgumentString = ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer);
            WCHAR* ConstraintTypeString = ErrorLog->ExtractErrorName(ConstraintType, NULL, ConstraintBuffer);

            if (!ErrorLog->HasThisErrorWithLocationAndParameters(
                    ERRID_GenericConstraintNotSatisfied2,
                    ErrorLocation,
                    ArgumentString,
                    ConstraintTypeString))
            {
                ErrorLog->CreateError(
                    ERRID_GenericConstraintNotSatisfied2,
                    ErrorLocation,
                    ArgumentString,
                    ConstraintTypeString);
            }
        }

        return false;
    }

    return true;
}

bool
Bindable::ValidateTypeConstraintForType
(
    BCSYM *Type,
    BCSYM *ConstraintType,
    CompilerHost *CompilerHostInstance,
    Symbols *SymbolFactory,
    bool IgnoreProjectEquivalence,                  // Consider types as equivalent ignoring their projects' equivalence.
    CompilerProject **ProjectForType,               // The project containing a component of type "Type" that was considered
                                                    // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForConstraintType      // The project containing a component of type "ConstraintType" that was considered
                                                    // for a successful match ignoring project equivalence.
)
{
    if (ConstraintType->IsBad())
    {
        return true;
    }

    ConversionClass Conversion =
        ClassifyPredefinedCLRConversion(
            ConstraintType,
            Type,
            *SymbolFactory,
            CompilerHostInstance,
            ConversionSemantics::ForConstraints,   // for constraint-checks we don't allow value-conversions e.g. enum->underlying
            0, // RecursionCount=0 because we're calling from outside
            IgnoreProjectEquivalence,
            ProjectForConstraintType,
            ProjectForType,
            NULL);

    if (Conversion == ConversionIdentity ||
        Conversion == ConversionWidening)
    {
        return true;
    }

    return false;
}

// Returns true if constraint is satisfied, else false
//
bool
Bindable::ValidateNewConstraintForType
(
    BCSYM *Type,
    BCSYM_GenericParam *Parameter,
    Location TypeArgumentLocations[],
    NameTypeArguments *TypeArgumentsLocationHolder,
    ErrorTable *ErrorLog,
    Compiler *CompilerInstance
)
{
    VSASSERT(Parameter->HasNewConstraint(), "New constraint validation unexpected!!!");

    if (Type->IsStruct() || Type->IsEnum())
    {
        // All structs are assumed to have a default constructor
        // because they can always be constructed using initobj
    }
    else if (Type->IsGenericParam())
    {
        BCSYM_GenericParam *GenericParamArgument = Type->PGenericParam();

        // "Type parameter '|1' must have a 'New' constraint or 'Structure' constraint to satisfy the
        // 'New' constraint for type parameter '|2'.")

        if (!GenericParamArgument->CanBeInstantiated())
        {
            if (ErrorLog)
            {
                StringBuffer ArgumentBuffer;
                StringBuffer ConstraintBuffer;

                ErrorLog->CreateError(
                    ERRID_BadGenericParamForNewConstraint2,
                    GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations),
                    ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer),
                    ErrorLog->ExtractErrorName(Parameter, NULL, ConstraintBuffer));
            }

            return false;
        }
    }
    else
    {
        BCSYM_Proc *SuitableConstructor = NULL;

        if (IsClass(Type))
        {
            BCSYM_Class *Class = Type->PClass();
            SuitableConstructor = GetPublicParameterlessConstructor(Class, CompilerInstance);

            // Report the MustInherit error only if no error is generated regarding a public parameterless
            // constructor. The reasons for this are:
            // - The public parameterless constructor missing is a more understandable error in this context
            // - Too much clutter to report both errors - Bug VSWhidbey 169091
            //
            if (SuitableConstructor && Class->IsMustInherit())
            {
                if (ErrorLog)
                {
                    // Type argument '|1' is declared 'MustInherit' and so does not satisfy the 'New' constraint
                    // for type parameter '|2'.

                    StringBuffer ArgumentBuffer;
                    StringBuffer ConstraintBuffer;

                    ErrorLog->CreateError(
                        ERRID_MustInheritForNewConstraint2,
                        GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations),
                        ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer),
                        ErrorLog->ExtractErrorName(Parameter, NULL, ConstraintBuffer));
                }

                return false;
            }

        }

        if (!SuitableConstructor)
        {
            if (ErrorLog)
            {
                // 

                // Type argument '|1' does not have a public parameterless instance constructor and so does not
                // satisfy the 'New' constraint for type parameter '|2'

                StringBuffer ArgumentBuffer;
                StringBuffer ConstraintBuffer;

                ErrorLog->CreateError(
                    ERRID_NoSuitableNewForNewConstraint2,
                    GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations),
                    ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer),
                    ErrorLog->ExtractErrorName(Parameter, NULL, ConstraintBuffer));
            }

            return false;
        }
    }

    return true;
}

// Returns true if constraint is satisfied, else false
//
bool
Bindable::ValidateReferenceConstraintForType
(
    BCSYM *Type,
    BCSYM_GenericParam *Parameter,
    Location TypeArgumentLocations[],
    NameTypeArguments *TypeArgumentsLocationHolder,
    ErrorTable *ErrorLog
)
{
    VSASSERT(!Parameter || Parameter->HasReferenceConstraint(), "Reference type constraint validation unexpected!!!");

    if (!TypeHelpers::IsReferenceType(Type))
    {
        if (ErrorLog)
        {
            // Type argument '|1' does not satisfy the 'Class' constraint for type parameter '|2'

            StringBuffer ArgumentBuffer;
            StringBuffer ConstraintBuffer;

            ErrorLog->CreateError(
                ERRID_BadTypeArgForRefConstraint2,
                GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations),
                ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer),
                ErrorLog->ExtractErrorName(Parameter, NULL, ConstraintBuffer));
        }

        return false;
    }

    return true;
}

// Returns true if constraint is satisfied, else false
//
bool
Bindable::ValidateValueConstraintForType
(
    BCSYM *Type,
    BCSYM_GenericParam *Parameter,
    Location TypeArgumentLocations[],
    NameTypeArguments *TypeArgumentsLocationHolder,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    bool isNullableValue
)
{
    VSASSERT(!Parameter || Parameter->HasValueConstraint(), "Value type constraint validation unexpected!!!");

    if (!TypeHelpers::IsValueType(Type))
    {
        if (ErrorLog)
        {
            Location* ErrorLocation = GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations);

            if (isNullableValue)
            {
                if (!ErrorLog->HasThisErrorWithLocation(ERRID_BadTypeArgForStructConstraintNull, *ErrorLocation))
                {
                    StringBuffer ArgumentBuffer;

                    ErrorLog->CreateError(
                        ERRID_BadTypeArgForStructConstraintNull,
                        ErrorLocation,
                        ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer));
                }
            }
            else
            {
                if (!ErrorLog->HasThisErrorWithLocation(ERRID_BadTypeArgForStructConstraint2, *ErrorLocation))
                {
                    // Type argument '|1' does not satisfy the 'Structure' constraint for type parameter '|2'
                    StringBuffer ArgumentBuffer;
                    StringBuffer ParameterBuffer;

                    ErrorLog->CreateError(
                        ERRID_BadTypeArgForStructConstraint2,
                        ErrorLocation,
                        ErrorLog->ExtractErrorName(Type, NULL, ArgumentBuffer),
                        ErrorLog->ExtractErrorName(Parameter, NULL, ParameterBuffer));
                }
            }
        }

        return false;
    }
    else if 
    (
        TypeHelpers::IsNullableType(Type, CompilerHostInstance) ||
        TypeHelpers::IsConstrainedToNullable(Type, CompilerHostInstance)
    )
    {
        if (ErrorLog)
        {
            Location* ErrorLocation = GetTypeArgumentLocation(Parameter, TypeArgumentsLocationHolder, TypeArgumentLocations);

            if (!ErrorLog->HasThisErrorWithLocation(ERRID_BadTypeArgForStructConstraint2, *ErrorLocation))
            {
                // 'System.Nullable' does not satisfy the 'Structure' constraint for type parameter '|1'. Only non-nullable 'Structure' types are allowed.
                StringBuffer ParameterBuffer;

                ErrorLog->CreateError(
                    ERRID_NullableDisallowedForStructConstr1,
                    ErrorLocation,
                    ErrorLog->ExtractErrorName(Parameter, NULL, ParameterBuffer));
            }
        }

        return false;
    }

    return true;
}

Location*
Bindable::GetTypeArgumentLocation
(
    BCSYM_GenericParam *Parameter,
    NameTypeArguments *TypeArgumentsLocationHolder,
    Location TypeArgumentLocations[]
)
{
    return TypeArgumentsLocationHolder ?
                TypeArgumentsLocationHolder->GetArgumentLocation(Parameter->GetPosition()) :
                TypeArgumentLocations ?
                    &TypeArgumentLocations[Parameter->GetPosition()] :
                    NULL;
}

void
Bindable::CheckGenericConstraints
(
    BCSYM *Type,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    CompilationCaches *IDECompilationCaches,
    bool CheckForArgumentBindingsToo
)
{
    if (Type->IsNamedType())
    {
        CheckGenericConstraints(
            Type->PNamedType(),
            ErrorLog,
            CompilerHostInstance,
            CompilerInstance,
            SymbolFactory,
            IDECompilationCaches,
            CheckForArgumentBindingsToo);
    }
}

void
Bindable::CheckGenericConstraintsForImportsTarget
(
    ImportedTarget *ImportsTarget,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHostInstance,
    Compiler *CompilerInstance,
    Symbols *SymbolFactory,
    CompilationCaches *IDECompilationCaches
)
{
    if (ImportsTarget->m_hasError ||
        !ImportsTarget->m_pTarget ||
        ImportsTarget->m_pTarget->IsBad() ||
        !ImportsTarget->m_pTarget->IsGenericTypeBinding())
    {
        return;
    }

    VSASSERT(ImportsTarget->m_ppTypeArguments, "Type arguments expected for type binding!!!");

    unsigned CurrentBindingIndex = ImportsTarget->m_NumDotDelimitedNames - 1;

    for(BCSYM_GenericBinding *Binding = ImportsTarget->m_pTarget->PGenericTypeBinding();
        Binding;
        Binding = Binding->GetParentBinding())
    {
        CheckGenericConstraints(
            Binding,
            NULL,
            ImportsTarget->m_ppTypeArguments[CurrentBindingIndex],
            ErrorLog,
            CompilerHostInstance,
            CompilerInstance,
            SymbolFactory,
            IDECompilationCaches);

        CheckGenericConstraintsForBindingsInTypeArguments(
            ImportsTarget->m_ppTypeArguments[CurrentBindingIndex],
            ErrorLog,
            CompilerHostInstance,
            CompilerInstance,
            SymbolFactory,
            IDECompilationCaches);

        CurrentBindingIndex--;
    }
}

bool
Bindable::ResolveGenericArguments
(
    BCSYM *UnboundGeneric,
    BCSYM_GenericParam *Parameters,
    NameTypeArguments *ArgumentsInfo,
    ErrorTable *ErrorLog,          // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance     // [in] the current context
)
{
    BCSYM **Arguments = ArgumentsInfo->m_Arguments;
    unsigned ArgumentCount = ArgumentsInfo->m_ArgumentCount;

    bool IsBad = false;
    unsigned ArgumentIndex = 0;

    while (Parameters != NULL && ArgumentIndex < ArgumentCount)
    {
        // Arguments[ArgumentIndex] can be NULL for unspecified type arguments
        // which is a valid scenario for types in getype expressions.
        // Eg: GetType(C(Of ,))

        if (Arguments[ArgumentIndex])
        {
            BCSYM *ArgumentType = Arguments[ArgumentIndex]->DigThroughNamedType();

            if (ArgumentType->IsBad())
            {
                IsBad = true;
            }
        }

        // Constraints cannot be checked yet because the constraint types have not necessarily been resolved.

        Parameters = Parameters->GetNextParam();
        ArgumentIndex++;
    }

    if (Parameters != NULL)
    {
        // Error -- too few arguments.

        if (ErrorLog)
        {
            Location ErrorLoc;

            ErrorLog->CreateError(
                ERRID_TooFewGenericArguments1,
                ArgumentsInfo->GetAllArgumentsLocation(&ErrorLoc),
                UnboundGeneric->GetErrorName(CompilerInstance));
        }

        IsBad = true;
    }

    if (ArgumentIndex != ArgumentCount)
    {
        // Error -- too many arguments.

        if (ErrorLog)
        {
            Location ErrorLoc;

            ErrorLog->CreateError(
                ERRID_TooManyGenericArguments1,
                ArgumentsInfo->GetAllArgumentsLocation(&ErrorLoc),
                UnboundGeneric->GetErrorName(CompilerInstance));
        }

        IsBad = true;
    }

    return IsBad;
}

/*****************************************************************************
;ResolveImports

Go through all the imports for the namespaces in this file and resolve them
*****************************************************************************/
void
Bindable::ResolveImports
(
    BCSYM_Namespace *NamespaceToCheck, // The namespace that owns the imports being resolved
    SourceFile *SourceFile, // [in] used to get the default namespace
    CompilerProject *Project, // [in] used by semantics when binding the name
    NorlsAllocator *Allocator, // [in] symbols will be allocated from this heap
    LineMarkerTable *LineMarkerTbl, // [in]
    ErrorTable *ErrorLog,  // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance,   // [in] the current context
    CompilationCaches *IDECompilationCaches
)
{
    VSASSERT( NamespaceToCheck, "A file should have a namespace" );

    BCITER_CHILD NamespaceIterator(NamespaceToCheck);
    ImportedTarget *ImportsToResolve = NamespaceToCheck->GetImports();

    if (ImportsToResolve)
    {
        ResolveImportsList(
            ImportsToResolve,
            SourceFile,
            Project,
            Allocator,
            LineMarkerTbl,
            ErrorLog,
            CompilerInstance,
            IDECompilationCaches);
    }

    // for add imports validation: resolve the extra imports statement here
    if(SourceFile->GetExtraTrackedImport())
    {
        ErrorTable dummyErrorLog(CompilerInstance, Project, NULL);

        ResolveImportsList(
            SourceFile->GetExtraTrackedImport(),
            SourceFile,
            Project,
            Allocator,
            NULL,
            &dummyErrorLog,
            CompilerInstance,
            IDECompilationCaches);
    }

    // Iterate through any nested namespaces
    while (BCSYM_NamedRoot *CurrentNamespaceSymbol = NamespaceIterator.GetNext())
    {
        if (CurrentNamespaceSymbol->IsNamespace() &&
            CurrentNamespaceSymbol->GetBindingSpace() != BINDSPACE_IgnoreSymbol )
        {
            ResolveImports(
                CurrentNamespaceSymbol->PNamespace(),
                SourceFile,
                Project,
                Allocator,
                LineMarkerTbl,
                ErrorLog,
                CompilerInstance,
                IDECompilationCaches);
        }
    }
}


/**************************************************************************************************
;ResolveImportsList

Resolves all of the imports clauses
***************************************************************************************************/
void
Bindable::ResolveImportsList
(
    ImportedTarget *ImportsStatement, // List of imports to resolve
    SourceFile *SourceFile, // Used to get the unnamed namespace (may be null)
    CompilerProject *Project, // [in] the Project that Sourcefile belongs to
    NorlsAllocator *Allocator, // [in] the allocator to use for building aliases
    LineMarkerTable* LineMarkerTbl, // [in] the line marker table
    ErrorTable *ErrorLog,  // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance,   // [in] the current compilation context
    CompilationCaches *IDECompilationCaches
)
{
    bool IsBadImportsName;

    // SourceFile might be NULL.

    Symbols
        SymbolFactory(
            CompilerInstance,
            Allocator,
            LineMarkerTbl,
            SourceFile == NULL ? NULL : (
                Allocator == SourceFile->SymbolStorage() ?
                    SourceFile->GetCurrentGenericBindingCache() :
                    NULL));

    // For project-level imports there is no source file.
    BCSYM_Namespace *UnnamedNamespace =
        SourceFile ?
            SourceFile->GetUnnamedNamespace() :
            CompilerInstance->GetUnnamedNamespace(Project);

    // If there is no unnamed namespace for this project, that means it contains
    // no files, so no need to do this.
    //
    if (!UnnamedNamespace)
        return;

    ResolvedImportsTree *pImportsCache = Project->GetImportsCache();

    for(ImportedTarget *CurrentImport = ImportsStatement;
        CurrentImport;
        CurrentImport = CurrentImport->m_pNext)
    {
        // Make sure we null the target out first, that way, if we punt out because
        // of an error, we won't have a pointer that points to garbage.
        //
        // Clearing the following info is particularly important because for project
        // level imports we don't clear during decompilation and expect it to be cleared
        // before binding.
        //
        CurrentImport->m_pAlias  = NULL;
        CurrentImport->m_hasError = false;

        if (CurrentImport->m_IsXml)
        {
            // Project-level imports need to have their XmlNamespaceDeclaration symbol rebound each Bound phase to the UnnamedNamespace, which can change
            // as files are removed and added to the project.  File-level imports create the symbol in the Declared phase.
            if (CurrentImport->m_IsProjectLevelImport)
                CurrentImport->m_pTarget = SymbolFactory.GetXmlNamespaceDeclaration(CurrentImport->m_pstrQualifiedName, NULL, Project, NULL, true);
        }
        else
        {
            CurrentImport->m_pTarget = NULL;

            // If there are no names, then this import has an error.  Punt this guy
            //
            if (!CurrentImport->m_NumDotDelimitedNames)
            {
                CurrentImport->m_hasError = true;
                continue;
            }

            bool HasGenericArguments = false;

#if IDE 
            VSASSERT(
                GetCompilerSharedState()->IsInBackgroundThread(),
                "Compilation cache is not being set for the FG thread, so the following call could cause performance problems");
#endif

            ResolveTypeArguments(
                CurrentImport->m_ppTypeArguments,
                CurrentImport->m_NumDotDelimitedNames,
                Allocator,
                ErrorLog,
                CompilerInstance,
                Project->GetCompilerHost(),
                SourceFile,
                TypeResolveNoFlags,
                IDECompilationCaches, // Compilation Caches
                NameSearchTypeReferenceOnly |
                    NameSearchIgnoreImports |
                    NameSearchImmediateOnly |
                    NameSearchIgnoreBases,
                false,
                HasGenericArguments);

            ResolvedImportsNode *pResolvedImportsNode = NULL;
            BCSYM_NamedRoot *BoundImport = NULL;

            if (!HasGenericArguments &&
                pImportsCache->Find(&CurrentImport->m_pstrQualifiedName, &pResolvedImportsNode))
            {
                BoundImport = pResolvedImportsNode->BoundImport;
                IsBadImportsName = false;
            }
            else
            {

#if IDE 
                VSASSERT(
                    GetCompilerSharedState()->IsInBackgroundThread(),
                    "Compilation cache is not being set for the FG thread, so InterpretQualifiedName could cause performance problems");
#endif
                BoundImport =
                    Semantics::EnsureNamedRoot
                    (
                        Semantics::InterpretQualifiedName
                        (
                            CurrentImport->m_DotDelimitedNames,
                            CurrentImport->m_NumDotDelimitedNames,
                            CurrentImport->m_ppTypeArguments,
                            Allocator,
                            UnnamedNamespace->GetHash(),
                            // Restriction to not look at types in containers that are
                            // visible through inheritance. Without this restriction,
                            // file compile ordering will matter and will make binding
                            // imports indeterminate in some cases.
                            //
                            (
                                NameSearchTypeReferenceOnly |
                                NameSearchIgnoreImports |
                                NameSearchImmediateOnly |
                                NameSearchIgnoreBases |
                                NameSearchIgnoreExtensionMethods
                            ),
                            CurrentImport->m_loc,
                            ErrorLog,
                            CompilerInstance,
                            Project->GetCompilerHost(),
                            IDECompilationCaches, //CompilationCaches
                            SourceFile,
                            // don't perform obsolete checks. This will be done later
                            // in Obsolete checker after cracking attributes.
                            //
                            false,
                            IsBadImportsName
                        )
                    );
            }

            // Handle errors that fell out of binding the import statement
            //
            if (IsBadImportsName)
            {
                CurrentImport->m_hasError = true;
                continue;
            }

            // Cache the import results.
            if (!HasGenericArguments && pResolvedImportsNode == NULL && BoundImport != NULL)
            {
                if (pImportsCache->Insert(&CurrentImport->m_pstrQualifiedName, &pResolvedImportsNode))
                {
                    pResolvedImportsNode->BoundImport = BoundImport;
                }
            }

            ERRID ErrId = 0;
            if (!BoundImport)
            {
                ErrId = SourceFile ?
                                WRNID_UndefinedOrEmptyNamespaceOrClass2 :
                                WRNID_UndefinedOrEmpyProjectNamespaceOrClass2;
            }
            else // is this an importable thing?
            {
                BCSYM *ImportType = BoundImport->DigThroughAlias();

                if (!ImportType->IsNamespace() &&
                    (!ImportType->IsType() ||
                        // 
                        ImportType->IsDelegate() ||
                        // Non-aliased interface imports are disallowed
                        //
                        (ImportType->IsInterface() && !CurrentImport->m_pstrAliasName)))
                {
                    ErrId =
                        CurrentImport->m_pstrAliasName ?
                            ERRID_InvalidTypeForAliasesImport2 :
                            ERRID_NonNamespaceOrClassOnImport2;
                }
            }

            if (ErrId)
            {
                // don't log repeating errors
                //
                if (!CurrentImport->m_hasError)
                {
                    STRING *ImportsString =
                        CompilerInstance->ConcatStringsArray(
                            CurrentImport->m_NumDotDelimitedNames,
                            (const WCHAR **)CurrentImport->m_DotDelimitedNames,
                            WIDE("."));

                    const WCHAR *LastName =
                        CurrentImport->m_DotDelimitedNames[CurrentImport->m_NumDotDelimitedNames - 1] ?
                            CurrentImport->m_DotDelimitedNames[CurrentImport->m_NumDotDelimitedNames - 1] :
                            L"";

                    if (ErrorLog)
                    {
                        ErrorLog->CreateError(
                            ErrId,
                            &CurrentImport->m_loc,
                            ImportsString,
                            LastName);
                    }

                    CurrentImport->m_hasError = true;

                }

                continue;
            }

            // The Import name is ok so far - CheckForDuplicates
            //
            CurrentImport->m_pTarget = BoundImport->DigThroughAlias();
        }

        // All the imports for the namespace being processed in SourceFile are linked together
        // in the ImportsStatement List. There aren't very many - compare the guy just built
        // against everybody before him in the list looking for dupes
        //
        for(ImportedTarget *DupeIter = ImportsStatement;
            DupeIter != CurrentImport;
            DupeIter = DupeIter->m_pNext )
        {
             // skip horked guys
            if (DupeIter->m_hasError)
                continue;

            // Duplicate alias names?
            if (CurrentImport->m_pstrAliasName &&
                CurrentImport->m_IsXml == DupeIter->m_IsXml)
            {
                // Use case sensitive comparison if this is an Xml import
                if (CurrentImport->m_IsXml ?
                        StringPool::IsEqualCaseSensitive(CurrentImport->m_pstrAliasName, DupeIter->m_pstrAliasName) :
                        StringPool::IsEqual(CurrentImport->m_pstrAliasName, DupeIter->m_pstrAliasName))
                {
                    ErrorLog->CreateError(
                        CurrentImport->m_IsXml ? ERRID_DuplicatePrefix : ERRID_DuplicateNamedImportAlias1,
                        &DupeIter->m_loc,
                        DupeIter->m_pstrAliasName);

                    DupeIter->m_hasError = true;
                }
            }
            // Already imported this namespace?
            else if (!CurrentImport->m_pstrAliasName &&
                     !DupeIter->m_pstrAliasName &&
                     DupeIter->m_pTarget->PContainer()->IsSameContainer(CurrentImport->m_pTarget->PContainer()))
            {
                // When referring to the same container, either both should be bindings or both should be not
                //
                VSASSERT(!(DupeIter->m_pTarget->IsGenericTypeBinding() ^ CurrentImport->m_pTarget->IsGenericTypeBinding()),
                            "Inconsistent type bindings in imports unexpected!!!");

                VSASSERT(!(DupeIter->m_IsProjectLevelImport ^ CurrentImport->m_IsProjectLevelImport),
                            "File level and project-level imports mix up unexpected!!!");

                // Are imported entities equal ?
                //
                if (!DupeIter->m_pTarget->IsGenericBinding() ||
                        BCSYM::AreTypesEqual(DupeIter->m_pTarget, CurrentImport->m_pTarget))
                {
                    // Note no duplicate checking for project level imports.

                    if (!DupeIter->m_IsProjectLevelImport)
                    {
                        StringBuffer ErrorSymbolNameBuffer;

                        ErrorLog->CreateError(
                            ERRID_DuplicateImport1,
                            &DupeIter->m_loc,
                            ErrorLog->ExtractErrorName(DupeIter->m_pTarget, NULL, ErrorSymbolNameBuffer));

                        DupeIter->m_hasError = true;
                    }
                }
                else if (DupeIter->m_pTarget->IsGenericBinding())
                {
                    // Generic type '|1' has already been imported, although with different type arguments.

                    StringBuffer ErrorSymbolNameBuffer;

                    ErrorLog->CreateError(
                        ERRID_DuplicateRawGenericTypeImport1,
                        &DupeIter->m_loc,
                        ErrorLog->ExtractErrorName(DupeIter->m_pTarget->PContainer(), NULL, ErrorSymbolNameBuffer));

                    DupeIter->m_hasError = true;
                }
            }
        } // Looking for duplicates

        // Make sure that the Import's alias doesn't have the same name as a type in the global namespace
        //
        if (!CurrentImport->m_IsXml)
        {
            if (!CurrentImport->m_hasError &&
                CurrentImport->m_pstrAliasName != NULL &&
                UnnamedNamespace)
            {
                BCSYM_Namespace *UnnamedNamespaceIter = UnnamedNamespace;

                do
                {
                    // If this namespace is not available to the current project, ignore it
                    if (!CompilerInstance->NamespaceIsAvailable(Project, UnnamedNamespaceIter))
                    {
                        continue;
                    }

                    BCSYM_NamedRoot *TypeInGlobalNamespace =
                        UnnamedNamespaceIter->SimpleBind(NULL, CurrentImport->m_pstrAliasName);

                    if (TypeInGlobalNamespace)
                    {
                        StringBuffer TypeRep;

                        TypeInGlobalNamespace->GetBasicRep(
                            CompilerInstance,
                            TypeInGlobalNamespace->GetContainer(),
                            &TypeRep,
                            NULL,
                            NULL,
                            TRUE /* full expansion */ );

                        ErrorLog->CreateError(
                            ERRID_ImportAliasConflictsWithType2,
                            &CurrentImport->m_loc,
                            CurrentImport->m_pstrAliasName,
                            TypeRep.GetString());

                        CurrentImport->m_hasError = true;
                        break;
                    }
                }
                while ((UnnamedNamespaceIter = UnnamedNamespaceIter->GetNextNamespace()) &&
                       // detect when we have looped around (Namespaces are on circularly linked lists
                       UnnamedNamespaceIter != UnnamedNamespace);
            }
        }

        if (!CurrentImport->m_hasError)
        {
            CurrentImport->m_pAlias =
                SymbolFactory.GetAlias(
                    CurrentImport->m_pstrAliasName,
                    0,
                    CurrentImport->m_pTarget,
                    NULL);
        }
    } // loop imports list
}

/*****************************************************************************
;BuildEvent

    Process an event declaration that is shaped by a delegate instead of the event
    declaration itself.

    Ok, this is ugly, but it's the simplest way to deal with it.
    Events that are declared "Event x as delegate" cannot be fully built
    until the delegate type that they are supposed to be is resolved.
    Once the delegate is known, we have to go back and set the parameters
    of the event.

    NOTE: this has to be kept in sync with the associated decompilation logic in
          bindableDecompilation.cpp
*****************************************************************************/
void
Bindable::BuildEvent(BCSYM_EventDecl *Event)
{
    BCSYM *RawDelegate = Event->GetRawDelegate();

    // No delegate, no need to build anything.  Someone else will report an error
    // if needed, or perhaps this is an implementing event which wouldn't have a
    // delegate yet (we hook it up later)
    //
    if (!RawDelegate || RawDelegate->IsGenericBadNamedRoot())
    {
        return;
    }

    if (!RawDelegate->IsDelegate())
    {
        // If we bind to anything but a delegate, give an error.
        if (!RawDelegate->DigThroughNamedType()->IsDelegate())
        {
            if (!RawDelegate->DigThroughNamedType()->IsBad())
            {
                ReportErrorOnSymbol(
                    ERRID_EventTypeNotDelegate,
                    CurrentErrorLog(Event),
                    // intrinsics have no location so use event instead
                    //
                    RawDelegate->IsIntrinsicType() || RawDelegate->IsObject() ?
                        Event :
                        RawDelegate);

                Event->SetIsBad();
            }
            return;
        }

        // Propagate the delegate shape to the event
        BCSYM *Delegate = Event->GetDelegate();

        // Resolve the named types for the delegate so that the types
        // of the parameters of the invoke method are known.
        //
        ResolveAllNamedTypesForContainer(Delegate->PClass(), m_CompilationCaches);

        BCSYM_NamedRoot *InvokeMethod =
            Delegate->PClass()->SimpleBind(
                NULL,
                STRING_CONST(CurrentCompilerInstance(), DelegateInvoke));

        if (InvokeMethod && InvokeMethod->IsBad())
        {
            InvokeMethod->ReportError(
                CurrentCompilerInstance(),
                CurrentErrorLog(Event),
                RawDelegate->GetLocation());

            Event->SetIsBad();
            return;
        }

        if (InvokeMethod == NULL || !InvokeMethod->IsProc())
        {
            ReportErrorOnSymbol(
                ERRID_UnsupportedType1,
                CurrentErrorLog(Event),
                RawDelegate,
                Delegate->PClass()->GetName());

            Event->SetIsBad();
            return;
        }

        // If the /target:winmdobj switch was specified then all events will take the WinRT shape unless
        // they implement a normal .NET shaped event.
        bool isWinRTShapedEventTheDefault = 
                       CurrentSourceFile(Event)->GetCompilerProject() && 
                       CurrentSourceFile(Event)->GetCompilerProject()->GetOutputType() == OUTPUT_WinMDObj;

        // If the delegate has a return type, that's not cool
        if (InvokeMethod->PProc()->GetType())
        {
            ReportErrorOnSymbol(
                ERRID_EventDelegatesCantBeFunctions,
                CurrentErrorLog(Event),
                Event);
            Event->SetIsBad();
        }
        else
        {
            // Validate the signatures of the event methods for a block event

            if (Event->IsBlockEvent())
            {
                if (Event->GetProcAdd() &&
                    Event->GetProcAdd()->GetFirstParam())
                {
                    if (!BCSYM::AreTypesEqual(
                            Event->GetProcAdd()->GetFirstParam()->GetType(),
                            Delegate))
                    {
                        // 'AddHandler' and 'RemoveHandler' method parameters must have the same delegate type as the containing event.
                        // The type of the 'AddHandler' method's parameter must be the same as the type of the event.

                        
                        BCSYM *RawParamType = Event->GetProcAdd()->GetFirstParam()->GetRawType();

                        ReportErrorAtLocation(

                            isWinRTShapedEventTheDefault ? ERRID_AddParamWrongForWinRT : ERRID_AddRemoveParamNotEventType,
                            CurrentErrorLog(Event),
                            RawParamType->IsNamedType() ?
                                RawParamType->GetLocation() :
                                Event->GetProcAdd()->GetFirstParam()->GetLocation());

                        Event->SetIsBad();
                    }
                }

                // If the event is implementing an interface member then we will do 
                // parameter validation in ImplementsSemantics.
                if (Event->GetProcRemove() &&
                    Event->GetProcRemove()->GetFirstParam() &&
                    Event->GetImplementsList() == NULL)
                {
                    BCSYM *RawParamType = Event->GetProcRemove()->GetFirstParam()->GetRawType();

                    if ((!isWinRTShapedEventTheDefault &&
                         !BCSYM::AreTypesEqual(
                            Event->GetProcRemove()->GetFirstParam()->GetType(),
                            Delegate)) ||
                        (isWinRTShapedEventTheDefault &&
                         CurrentCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::EventRegistrationTokenType) &&
                         !BCSYM::AreTypesEqual(
                            Event->GetProcRemove()->GetFirstParam()->GetType(),
                            CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::EventRegistrationTokenType))))
                    {
                        // 'AddHandler' and 'RemoveHandler' method parameters must have the same delegate type as the containing event.
                        // In a Windows Runtime event, the type of the 'RemoveHandler' method parameter must be 'EventRegistrationToken'.
                        
                        ReportErrorAtLocation(
                            isWinRTShapedEventTheDefault ? ERRID_RemoveParamWrongForWinRT : ERRID_AddRemoveParamNotEventType,
                            CurrentErrorLog(Event),
                            RawParamType->IsNamedType() ?
                                RawParamType->GetLocation() :
                                Event->GetProcRemove()->GetFirstParam()->GetLocation());

                        Event->SetIsBad();

                    }
                }

                if (Event->GetProcFire())
                {
                    Symbols
                        SymbolFactory(
                            CurrentCompilerInstance(),
                            CurrentAllocator(),
                            NULL,
                            CurrentGenericBindingCache());

                    MethodConversionClass MethodConversion = Semantics::ClassifyMethodConversion(
                            Event->GetProcFire(),
                            GenericBindingInfo(),
                            InvokeMethod->PProc(),
                            Delegate->IsGenericBinding() ?
                                Delegate->PGenericBinding() :
                                NULL,
                            false, //IgnoreReturnValueErrorsForInference
                            &SymbolFactory,
                            CurrentCompilerHost());
                    if (!Semantics::IsSupportedMethodConversion(
                            (CurrentSourceFile(Event)->GetOptionFlags() & OPTION_OptionStrict),
                            MethodConversion))
                    {
                        // 'RaiseEvent' method must have the same signature as the containing event's delegate type.

                        StringBuffer DelegateSignature;

                        Delegate->PContainer()->GetBasicRep(
                            CurrentCompilerInstance(),
                            CurrentContainer(),
                            &DelegateSignature,
                            Delegate->IsGenericBinding() ?
                                Delegate->PGenericBinding() :
                                NULL);

                        ReportErrorOnSymbol(
                            ERRID_RaiseEventShapeMismatch1,
                            CurrentErrorLog(Event),
                            Event->GetProcFire(),
                            DelegateSignature.GetString());

                        Event->SetIsBad();
                    }
                }
            }
        }

        Symbols
            SymbolFactory(
                CurrentCompilerInstance(),
                CurrentAllocator(),
                NULL,
                CurrentGenericBindingCache());
                
        // If the /target:winmdobj switch was specified then all events will take the WinRT shape unless
        // they implement a normal .NET shaped event. We will handle that case in ImplementsSemantics
        if (isWinRTShapedEventTheDefault &&
            Event->GetImplementsList() == NULL)
        {
            Event->ConvertToWindowsRuntimeEvent(CurrentCompilerHost(), &SymbolFactory);
        }

        // We need to make a copy of the parameter list. Other people
        // may need to store symbol-specific information in it.
        // Make sure you copy the Location too!!!!

        CopyInvokeParamsToEvent(
            Event,
            Delegate,
            InvokeMethod->PProc(),
            CurrentAllocator(),
            &SymbolFactory);
    }
}

void
Bindable::CopyInvokeParamsToEvent
(
    BCSYM_EventDecl *Event,
    BCSYM *Delegate,
    BCSYM_Proc *DelegateInvoke,
    NorlsAllocator *Allocator,
    Symbols *SymbolFactory
)
{
    // We need to make a copy of the parameter list. Other people
    // may need to store symbol-specific information in it.
    // Make sure you copy the Location too!!!!

    BCSYM_Param *CurrentParameter = NULL;
    BCSYM_Param *FirstParameter = NULL;
    BCSYM_Param *LastParameter = NULL;

    for(CurrentParameter = DelegateInvoke->PProc()->GetFirstParam();
        CurrentParameter != NULL;
        CurrentParameter = CurrentParameter->GetNext())
    {
        size_t cbSize =
            CurrentParameter->IsParamWithValue() ?
                sizeof(BCSYM_ParamWithValue) :
                sizeof(BCSYM_Param);

        // Microsoft 9/13/2004: The numbers added together for the allocation are all values that we own, and are "small"
        // -- there is no danger of overflow, and so I have not altered the code at all.
        BCSYM_Param *BuiltParamSymbol =
            (BCSYM_Param*)Allocator->Alloc(
                cbSize + (CurrentParameter->HasLocation() ? sizeof(TrackedLocation) : 0));
        if (CurrentParameter->HasLocation())
        {
            // This is a little funky, but this is what we do in Symbols::AllocSymbol() to adjust the
            // base pointer for the symbol just created.
            // pTrackedLocation is of type TrackedLocation*, so adding 1 to that pointer adds
            // sizeof(TrackedLocation) bytes, not 1 byte. Tricky, but it works.
            //
            TrackedLocation* pTrackedLocation = (TrackedLocation*) BuiltParamSymbol;
            BuiltParamSymbol = (BCSYM_Param *)(pTrackedLocation + 1);
        }

        // First copy the Param symbol
        memcpy(BuiltParamSymbol, CurrentParameter, cbSize);

        // If the delegate is a generic binding, then the types of the event's parameters
        // need to use the bound argument types of the delegate (rather than the generic parameter types).
        //
        // Not needed if the delegate is implicitly created by this event, because then it is an
        // an open binding and replacing the types does not gain much. More importantly, this helps
        // the event parameters retain the namedtypes as their types which helps during error
        // reporting. See VSWhidbey 264180
        //
        if (Delegate->IsGenericBinding() &&
            Delegate->PNamedRoot()->CreatedByEventDecl() != Event)
        {
            BuiltParamSymbol->SetType(
                ReplaceGenericParametersWithArguments(
                    BuiltParamSymbol->GetCompilerType(),
                    Delegate->PGenericBinding(),
                    *SymbolFactory));
        }

        // The location does not get copied with the memcopy above (since it's actually outside the symbol)
        // so we need to copy it explicitly.
        if (CurrentParameter->HasLocation())
        {
            BuiltParamSymbol->SetLocation(CurrentParameter->GetLocation());
        }

        // Link them in the linked list of Parameters
        Symbols::SetNextParam(BuiltParamSymbol, NULL);

        if (LastParameter)
        {
            Symbols::SetNextParam(LastParameter, BuiltParamSymbol);
        }
        else
        {
            FirstParameter = BuiltParamSymbol;
        }

        LastParameter = BuiltParamSymbol;
    }

    Event->SetParamList(FirstParameter);

    // Clone the return type if required. Bug VSWhidbey 357546.

    BCSYM *ReturnType = Event->GetCompilerType();

    if (ReturnType &&
        !ReturnType->IsVoidType() &&
        Delegate->IsGenericBinding() &&
        Delegate->PNamedRoot()->CreatedByEventDecl() != Event)
    {
        Event->SetType(
            ReplaceGenericParametersWithArguments(
                ReturnType,
                Delegate->PGenericBinding(),
                *SymbolFactory));
    }
}

void
Bindable::EvaluateAllConstantDeclarationsInContainer()
{
    NorlsAllocator norlsStorage(NORLSLOC);

    BCSYM_Container *Container = CurrentContainer();

    do
    {
        for(BCSYM_Expression *ExpressionSymbol = Container->GetExpressionList();
            ExpressionSymbol != NULL;
            ExpressionSymbol = ExpressionSymbol->GetNext())
        {
            if (!ExpressionSymbol->IsEvaluated())
            {
                EvaluateDeclaredExpression(
                    ExpressionSymbol,
                    &norlsStorage,
                    CurrentAllocator(),
                    CurrentErrorLog(ExpressionSymbol->GetReferringDeclaration()),
                    CurrentCompilerInstance(),
                    CurrentCompilerHost(),
                    m_CompilationCaches);
            }
        }

    } while (Container = Container->GetNextPartialType());
}

/*****************************************************************************
;EvaluateConstantDeclarations

Evaluate all of the constant expressions in the given list.  These are the constant
expressions that couldn't be evaluated during Declared state because they referenced
variables, etc.
*****************************************************************************/
void
Bindable::EvaluateConstantDeclarations
(
    BCSYM_Expression *ExpressionList, // [in] list of constant expressions to evaluate
    NorlsAllocator *Allocator, // [in] allocator for symbol storage
    ErrorTable *ErrorLog, // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance, // [in] the current compilation context
    CompilerHost *CompilerHost,  // [in] the current compilerhost
    CompilationCaches *IDECompilationCaches
)
{
    NorlsAllocator norlsStorage( NORLSLOC);

    for(BCSYM_Expression *ExpressionSymbol = ExpressionList;
        ExpressionSymbol != NULL;
        ExpressionSymbol = ExpressionSymbol->GetNext())
    {
        if (!ExpressionSymbol->IsEvaluated())
        {
            EvaluateDeclaredExpression(
                ExpressionSymbol,
                &norlsStorage,
                Allocator,
                ErrorLog,
                CompilerInstance,
                CompilerHost,
                IDECompilationCaches);
        }
    }
}


/*****************************************************************************
;EvaluateDeclaredExpression

Step an expression from declared to evaluated.  The text of the expression is obtained,
parsed, and interpreted.
*****************************************************************************/
void Bindable::EvaluateDeclaredExpression
(
    BCSYM_Expression * ExpressionSymbol, // [in] the expression we want to evaluate
    NorlsAllocator * TreeAllocator, // [in] allocator to use
    NorlsAllocator * SymbolAllocator, // [in] allocator to use
    ErrorTable *ErrorLog, // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance, // [in] the current compilation context
    CompilerHost *CompilerHost,  // [in] the current compilerhost
    CompilationCaches *IDECompilationCaches
)
{
    ParseTree::Expression *ExpressionTree = NULL;
    const Location *ExpressionLocation = ExpressionSymbol->GetLocation();
    BCSYM_NamedRoot *ReferringDeclaration = ExpressionSymbol->GetReferringDeclaration();
    BCSYM_Hash *ConstantsHash = NULL; // [in] hash table to use when resolving constants

    // a bad referring declaration can create havoc later (VSW 340844). Filter it out here.
    if (ReferringDeclaration->IsBad())
    {
        return;
    }

    VSASSERT(ReferringDeclaration &&
             (ReferringDeclaration->IsProc() ||
                (ReferringDeclaration->IsVariable() && ReferringDeclaration->PVariable()->IsConstant())),
             "unexpected declaration with deferred expression : #10/29/2003#");

    if (ExpressionSymbol->IsSyntheticExpressionForEnumMemberIncrement())
    {
        VSASSERT( ExpressionLocation,
                        "Synthetic expressions for Enums should have the location of the enum member for which they are defined!!!");

        // This tree could have been built up during declared when synthesizing these
        // expressions, but they take up quite a bit of storage, so instead build up the
        // trees in bindable at the point of the evaluation, since building up these trees
        // is quick.
        //
        // So then the question, why not synthesize text ? This would result in the parse
        // trees having locations in non-user code, so any errors could end up squiggling
        // non-existing locations

        // There's a possibility to stack overflow if we evaluate this enum recursively 1 element at a time.
        // Instead, count to the first enum member that did not have a synthetic increment initializer value.
        // See Dev11 222108, 235772, 369336
        int countFromPreviousEnumWithValue = 1;
        BCSYM_VariableWithValue *pFirstEnumVariableWithActualValue = ExpressionSymbol->GetPrevousEnumMember();
        while (pFirstEnumVariableWithActualValue->GetExpression() &&
            !pFirstEnumVariableWithActualValue->GetExpression()->IsEvaluated() &&
            pFirstEnumVariableWithActualValue->GetExpression()->IsSyntheticExpressionForEnumMemberIncrement())
        {
            pFirstEnumVariableWithActualValue = pFirstEnumVariableWithActualValue->GetExpression()->GetPrevousEnumMember();
            countFromPreviousEnumWithValue++;
        }

        // Create the ParseTree Node representing the name of the previous enum member
        ParseTree::NameExpression *PrevEnumMember =
            new (TreeAllocator->Alloc(sizeof ParseTree::NameExpression)) ParseTree::NameExpression;

        PrevEnumMember->Opcode = ParseTree::Expression::Name;

        PrevEnumMember->Name.Name = CompilerInstance->AddString(pFirstEnumVariableWithActualValue->GetName());
        PrevEnumMember->Name.TypeCharacter = chType_NONE;
        PrevEnumMember->Name.IsBracketed = false;
        PrevEnumMember->Name.IsBad = false;
        PrevEnumMember->Name.IsNullable = false;
        PrevEnumMember->Name.TextSpan = *ExpressionLocation;

        PrevEnumMember->TextSpan = *ExpressionLocation;

        ParseTree::IntegralLiteralExpression *IntegerLiteral =
            new (TreeAllocator->Alloc(sizeof ParseTree::IntegralLiteralExpression)) ParseTree::IntegralLiteralExpression;

        IntegerLiteral->Opcode = ParseTree::Expression::IntegralLiteral;
        IntegerLiteral->Value = countFromPreviousEnumWithValue;
        IntegerLiteral->Base = ParseTree::IntegralLiteralExpression::Decimal;
        IntegerLiteral->TextSpan = *ExpressionLocation;

        // For Ulong enums, force the constant increment value to be typed as an unsigned value to
        // avoid a decimal result. Bug VSWhidbey 285217.
        //
        IntegerLiteral->TypeCharacter =
            (ExpressionSymbol->GetForcedType() && ExpressionSymbol->GetForcedType()->GetVtype() == t_ui8) ?
                chType_U8 :
                chType_NONE;

        ParseTree::BinaryExpression *Increment =
            new (TreeAllocator->Alloc(sizeof ParseTree::BinaryExpression)) ParseTree::BinaryExpression;

        Increment->Opcode = ParseTree::Expression::Plus;
        Increment->Left = PrevEnumMember;
        Increment->Right = IntegerLiteral;
        Increment->TextSpan = *ExpressionLocation;

        // Note: No Punctuator needed for because this is a synthetic expression

        ExpressionTree = Increment;
    }
    else
    {
        // the text of the expression we are evaluating,
        // e.g. "test" given the expression: Const str As String * 5 = "Test"
        //
        WCHAR *ExpressionText = ExpressionSymbol->GetExpressionText();
        CompilerFile *pFile = ReferringDeclaration->GetCompilerFile();
        Parser ExpressionParser(
            TreeAllocator,
            CompilerInstance,
            CompilerHost,
            false,
            pFile ? pFile->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT);

        Scanner Scanner(
                        CompilerInstance,
                        ExpressionText,
                        wcslen( ExpressionText ),
                        0, // get a scanner for tokenizing the expression in ExpressionText
                        ExpressionLocation ?
                            ExpressionLocation->m_lBegLine :
                            0,
                        ExpressionLocation ?
                            ExpressionLocation->m_lBegColumn :
                            0);

        // build a parse tree for the expression
        //
        IfFailThrow(ExpressionParser.ParseOneExpression(
                        &Scanner,
                        ErrorLog,
                        &ExpressionTree,
                        NULL, // ErrorInConstructRet
                        pFile ? pFile->GetProject()->GetProjectLevelCondCompScope() : NULL, // Dev10 #789970 Use proper conditional complation scope.
                        pFile && pFile->IsSourceFile() ? pFile->PSourceFile()->GetConditionalCompilationConstants() : NULL // Dev10 #789970 
                        ));
    }

    // the location of the start of the text that makes up the expression we are evaluating
    //
    Location *TextLocation = ExpressionSymbol->GetLocation();
    ExpressionSymbol->SetIsEvaluating(true);
    BCSYM *CastType;

    if (ExpressionSymbol->GetForcedType() == NULL) // are we supposed to cast the expression?
    {
        // no type to cast expression to
        CastType = NULL;
    }
    else // there is a cast for this expression - the type should already be bound
    {
        CastType = ExpressionSymbol->GetForcedType();
        if (CastType->GetVtype() == t_ptr)
        {
            // byref parameter; get underlying type
            CastType = CastType->PPointerType()->GetCompilerRoot();
        }
    }

    // If we're looking up constant expressions for a file, use the container as the lookup scope
    //
    // But for constant expressions in method declaration - say the optional parameter value, we need
    // to start off with the type params hash of the generic method.

    if (ReferringDeclaration->IsProc() && ReferringDeclaration->PProc()->IsGeneric())
    {
        ConstantsHash =
            ReferringDeclaration->PProc()->GetGenericParamsHash();
    }

    if (!ConstantsHash)
    {
        BCSYM_NamedRoot *ParentContext = ReferringDeclaration;

        while (ParentContext && !ParentContext->IsHash())
        {
            ParentContext = ParentContext->GetImmediateParent();
        }

        if (ParentContext)
        {
            ConstantsHash = ParentContext->PHash();
        }
    }

    if (ConstantsHash)
    {
        // Make sure all bases and named types have been resolved for the container
        // before evaluating constant expressions in it.
        // How could this happen before the container's bases and named types have been
        // resolved ? Well, it could occur if this expression evaluation is triggered
        // when evaluating an expression in another container that is dependent on this
        // expression.
        //
        BCSYM_Container *ContainerOfConstantExpression = ConstantsHash->GetContainer();

        VSASSERT( ContainerOfConstantExpression, "How can a hash be independent of a container ?");

        ResolveAllNamedTypesInContainerIfPossible(
            ContainerOfConstantExpression,
            IDECompilationCaches,
            false);     // false indicates - don't bother to check whether any of the previous steps are in progress
                        //
                        // We don't bother to ensure here because the assertion is that this should never be invoked
                        // when any of the previous steps are in progress.
                        //
                        // Assert in ResolveAllNamedTypesInContainerIfPossible to detect any rogue cases.
    }

    // OPTIONAL: 
    // Bindable sees many constant expressions, e.g. enums, attributes and parameter-defaults.
    // Of those expressions whose referring declaration is a Proc, they're all param-defaults.
    // Note: delegate declarations do not satisfy IsProc. It is "ByDesign" that you're not allowed
    // to specify optional parameters in a delegate declaration.
    bool ExprIsForAParameterDefault = ExpressionSymbol->GetReferringDeclaration()->IsProc(); 

    // Parameter defaults work like this:
    // (1) Evaluate the default expression as a constant in a context where the target type is unknown
    // (2) If the resulting constant value is "Nothing" of type Object, then use default(T)
    // (3) Otherwise, attempt a constant-conversion of the value to the type of the parameter,
    //     or to the underlying type of the parameter in case it's a nullable.
    // Note that "x as Integer? = CType(Nothing,Object) uses route 2, while "x as Integer? = CType(Nothing, Integer)
    // uses route 3.

    ConstantValue ValueOfConstant;
    ILTree::Expression *ExpressionOfConstant;
    bool ExpressionIsBad = false;
    BCSYM *ParameterType = CastType==NULL ? CastType : CastType->DigThroughAlias();
    Semantics Analyzer(TreeAllocator, ErrorLog, CompilerInstance, CompilerHost, ConstantsHash->GetSourceFile(), NULL, false);

    // (1) Evaluate the rhs of the expression as a constant in a context where the target type is unknown
    ValueOfConstant = Analyzer.InterpretConstantExpression(
                        ExpressionTree,
                        ConstantsHash,
                        NULL, // NULL target type, i.e. "in a context wher the target type is unknown"
                        false,
                        &ExpressionIsBad,
                        //If this constant is used as parameter default, then the function needs to be passed to 
                        // InterpretConstantExpression, since this constant will be delayed to check obsolete and
                        // the delayed obsolete check needs to know the context of this constant.
                        //For example, class CC1 is obsolete.
                        // 
                        //<Obsolete()>
                        //    Sub Foo(Optional ByVal x As Integer = CC1.val)
                        //
                        // If this constant is assigned to a constant field, then ReferringDeclaration refers to the 
                        // constant variable.
                        //
                        // Added for bug 482094.
                        ReferringDeclaration,
                        ExpressionSymbol->IsSyntheticExpression(),
                        &ExpressionOfConstant);

    VSASSERT(ValueOfConstant.Integral==0 || ValueOfConstant.TypeCode!=t_ref, "unexpected: the only t_ref constant possible should have value 0");

    if (ExpressionIsBad)
    {
        // If it's bad, we won't look any further...
    }
    else if (ValueOfConstant.TypeCode == t_ref && ValueOfConstant.Integral == 0)
    {
        // (2) If the resulting constant value is "Nothing" of type Object, then use default(T)
        // Note that ParameterType==NULL was used in optional parameters to mean "Object": see the comment
        // in Declare::MakeParames
        ZeroMemory(&ValueOfConstant, sizeof(ValueOfConstant)); // just to make sure there's no junk in it
        Vtypes pvtype = (ParameterType==NULL) ? t_ref : ParameterType->GetVtype();
        ValueOfConstant.SetVType((pvtype==t_ref || pvtype==t_struct || pvtype==t_generic) ? t_ref : pvtype);
    }
    else
    {
        // (3) Otherwise, attempt a constant-conversion of the value to the parameter's type (or to its
        // underlying type if it was a nullable), reporting errors along the way if necessary.

        BCSYM *TargetType = ParameterType;
        if (TargetType!=NULL && TypeHelpers::IsNullableType(TargetType))
        {
            TargetType = TypeHelpers::GetElementTypeOfNullable(TargetType);
        }

        // Incidentally, recall that we here in parameter-defaults use TargetType==NULL to indicate
        // a parameter type of "Object". In such a case, every parameter-default-expression is good
        // to go with no conversion needed. And indeed, if given a NULL TargetType, then
        // ConvertWithErrorChecking does indeed do no conversion.
        ValueOfConstant = Analyzer.ConvertConstantExpressionToConstantWithErrorChecking(
                                        ExpressionOfConstant,
                                        TargetType,
                                        ExpressionIsBad); // final parameter "ExpressionIsBad" is an _Out_ parameter
    }


    ExpressionSymbol->SetIsEvaluating(false);
    ExpressionSymbol->SetIsEvaluated(true);

    if (ExpressionIsBad && ErrorLog)
    {
        // check for ErrorLog so we only throw errors if we aren't importing meta-data
        // (which is happening when ErrorLog is NULL)
        //
        ReferringDeclaration->SetIsBad();
    }
    else
    {
        if (ValueOfConstant.TypeCode == t_string)
        {
            // Copy the spelling so that it lives as long as the symbol.
            // Microsoft 9/13/2004: Since the string already exists, and we own the calculation of its length, there's no chance
            // that the number will overflow (or else it could not have been allocated in the first place).
            WCHAR *Spelling =
                (WCHAR *)SymbolAllocator->Alloc((ValueOfConstant.String.LengthInCharacters + 1) * sizeof(WCHAR));

            if (ValueOfConstant.String.Spelling)
            {
                memcpy(
                    Spelling,
                    ValueOfConstant.String.Spelling,
                    (ValueOfConstant.String.LengthInCharacters + 1) * sizeof(WCHAR));
            }
            else
            {
                Spelling = NULL;
            }
            ValueOfConstant.String.Spelling = Spelling;
        }

        ExpressionSymbol->SetValue(ValueOfConstant);

        // Change an Object-typed constant to the type of its expression (VSW#177934).

        if (ValueOfConstant.TypeCode != t_ref &&   // Devdiv Bug[24272] 
            ReferringDeclaration->IsVariable() &&
            TypeHelpers::IsRootObjectType(ReferringDeclaration->PVariable()->GetType()))
        {
            ReferringDeclaration->PVariable()->SetType(
                CompilerHost->GetFXSymbolProvider()->GetType(ValueOfConstant.TypeCode));
        }
    }
}


/****************************************************************************
;ChaseThroughToType

Chases through arrays, pointers, etc. to find the actual type of the symbol
*****************************************************************************/
BCSYM * // the type of pType
Bindable::ChaseThroughToType
(
    BCSYM * Type // [in] the type to chase through
)
{
    for( ;; )
    {
        switch( Type->GetKind()) // figure out the access of the type behind the member
        {
            case SYM_NamedType:
                Type = Type->DigThroughNamedType();
                continue;

            case SYM_PointerType:
                Type = Type->PPointerType()->GetRoot();
                continue;
            case SYM_ArrayLiteralType:    
            case SYM_ArrayType:
                Type = Type->PArrayType()->GetRoot();
                continue;

            default:
                return Type;
        } // switch
    } // endless loop
}

bool
Bindable::FindBaseInMyGroupCollection(BCSYM_Class *Class, WellKnownAttrVals::MyGroupCollectionData* groupCollectionData, unsigned* foundIndex)
{
    unsigned i = -1;
    bool found = false;
    if (groupCollectionData)
    {
        BCSYM_Class *Base = Class->GetCompilerBaseClass();
        ULONG count = groupCollectionData->Count();
        if (count)
        {
            WellKnownAttrVals::MyGroupCollectionBase* arrayBase = groupCollectionData->Array();
            while (Base && !Base->IsObject())
            {
                STRING* BaseQName = Base->GetQualifiedName();

                for (i = 0 ; i < count; i++)
                {
                    if (StringPool::IsEqual(BaseQName, arrayBase[i].m_GroupName))
                    {
                        found = true;
                        break;
                    }
                }
                if(found)
                    break;
                Base = Base->PClass()->GetCompilerBaseClass();
            }
        }
    }
    if(foundIndex)
        *foundIndex = found ? i : -1;
    return found;
}

void
Bindable::ScanAndLoadMyGroupCollectionMembers(BCSYM_Container *Container, CompilerProject *pProject)
{
    // MetaData containers are not allowed here in order to keep the
    // loading of nested types "on demand".
    //
    VSASSERT( !DefinedInMetaData(Container),
        "Metadata containers not allowed here!!");

    if (IsClass(Container))
    {
        BCSYM_Class *Class = Container->PClass();

        if (!CanBeMyGroupCollectionMember(Class, pProject->GetCompiler()))
        {
            // filter out partial types, generics and known to fail due to new constraint
#if DEBUG
            // in debug mode and if MyGroupTrace, check if such class could be a member an debprintf why is rejected
            // partial types are ignored because their main part makes it anyhow.
            if (VBFTESTSWITCH(fMyGroupAndDefaultInst) && !Class->IsPartialType())
            {
                MyGroupCollectionInfo* arrayOfMyGroupCollectionInfo = pProject->m_daMyGroupCollectionInfo.Array();
                ULONG count = pProject->m_daMyGroupCollectionInfo.Count();
                for (unsigned i = 0 ; i < count; i++)
                {
                    BCSYM_Class *GroupClass = arrayOfMyGroupCollectionInfo[i].m_groupClass;
                    WellKnownAttrVals::MyGroupCollectionData* groupCollectionData;
                    GroupClass->GetPWellKnownAttrVals()->GetMyGroupCollectionData(&groupCollectionData);

                    BCSYM_Class *Base = Class->GetCompilerBaseClass();
                    if (FindBaseInMyGroupCollection(Class, groupCollectionData, NULL))
                    {
                        DebugPrintfCanBeMyGroupCollectionMember(Class, GroupClass, pProject->GetCompiler());
                        break;
                    }
                }
            }

#endif
            return;
        }

        MyGroupCollectionInfo* arrayOfMyGroupCollectionInfo = pProject->m_daMyGroupCollectionInfo.Array();
        ULONG count = pProject->m_daMyGroupCollectionInfo.Count();
        for (unsigned i = 0 ; i < count; i++)
        {
            BCSYM_Class *GroupClass = arrayOfMyGroupCollectionInfo[i].m_groupClass;
            WellKnownAttrVals::MyGroupCollectionData *groupCollectionData;
            GroupClass->GetPWellKnownAttrVals()->GetMyGroupCollectionData(&groupCollectionData);

            if (FindBaseInMyGroupCollection(Class, groupCollectionData, NULL))
            {
                // check for duplicates in debug mode
#ifdef DEBUG
                bool found = false;
                BCSYM_Class** arrayOfClassesInMyGroupMember = arrayOfMyGroupCollectionInfo[i].m_daMyGroupMembers.Array();
                ULONG countMem = arrayOfMyGroupCollectionInfo[i].m_daMyGroupMembers.Count();
                for (unsigned k = 0 ; k < countMem; k++)
                {
                    if (BCSYM::AreTypesEqual(arrayOfClassesInMyGroupMember[k], Class))
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    VSDEBUGPRINTIF(
                        VSFSWITCH(fMyGroupAndDefaultInst),
                        "MyGroup: duplicate member: %S found in MyGroup class: %S\n", Class->GetName(), GroupClass->GetName());
                }
                VSDEBUGPRINTIF(
                    VSFSWITCH(fMyGroupAndDefaultInst),
                    "MyGroup: adding class member '%S' to group '%S'\n",
                    Class->GetName(),
                    GroupClass->GetName());
#endif

                arrayOfMyGroupCollectionInfo[i].m_daMyGroupMembers.Add() = Class;
                // consider to break here. This would corespond to the case when a class can be member of only one group
            }
        }

        // do not itterate over the the nested types in a class. Nested classes cannot be members in
        // My* groups
        return;
    }

    // iterate Container for all nested types
    //
    else if (Container->IsNamespace())
    {
        for(BCSYM_Container *NestedType = Container->GetNestedTypes();
            NestedType;
            NestedType = NestedType->GetNextNestedType())
        {
            ScanAndLoadMyGroupCollectionMembers(NestedType, pProject);
        }
    }
}

bool
Bindable::CanBeMyGroupCollectionMember
(
    BCSYM_Class *pClass,
    Compiler *pCompiler
)
{
    Type *pType = pClass;

    return !pClass->IsPartialType() &&
           !pClass->IsGeneric() &&
           GetPublicParameterlessConstructor(pClass, pCompiler) &&
           CheckConstraintsOnNew(pType) == 0;
}

#if DEBUG
// DebugPrintf co-function for the above CanBeMyGroupCollectionMember(). Keep in sync
void
Bindable::DebugPrintfCanBeMyGroupCollectionMember
(
    BCSYM_Class *pClass,
    BCSYM_Class *pGroupClass,
    Compiler *pCompiler
)
{


    Type *pType = pClass;
    char* ms = NULL;
    if ( pClass->IsGeneric())
        ms = "MyGroup: class '%S' fails to qualify as a member in group '%S' because it is generic\n";
    else if (!GetPublicParameterlessConstructor(pClass, pCompiler))
        ms = "MyGroup: class '%S' fails to qualify as a member in group '%S' because it has no parameterless constructor\n";
    else if (CheckConstraintsOnNew(pType) != 0)
        ms = "MyGroup: class '%S' fails to qualify as a member in group '%S' because it doesn't comply with the 'new constraint'\n";
    else if ( pClass->IsPartialType())
    {
        // nop - false positive partial classes are just skipped. Only the main part of a partial needs to qualify.
        //ms = "MyGroup: class '%S' fails to qualify as a member in group '%S' because it is partial\n";
    }
    else
    {
        VSFAIL("DebugPrintfCanBeMyGroupCollectionMember out of sync");
    }
    if(ms)
    {
        VSDEBUGPRINTIF(
            VSFSWITCH(fMyGroupAndDefaultInst),
                      ms, pClass->GetName(), pGroupClass->GetName());
    }

}
#endif //DEBUG

void
Bindable::----AttributesOnAllSymbolsInContainer()
{
    // For MetaData symbols, all attributes on symbols are ----ed when they are imported itself.
    // Also validation is not required for them because we only care about well known attributes.
    //
    VSASSERT( !DefinedInMetaData(CurrentContainer()),
                "MetaData container unexpected here!!!");

    if (GetStatusOf----AttributesOnAllSymbolsInContainer() == InProgress ||
        GetStatusOf----AttributesOnAllSymbolsInContainer() == Done)
    {
        return;
    }

    SetStatusOf----AttributesOnAllSymbolsInContainer(InProgress);

    // We need to ---- attributes for all bases because we need to check if a class
    // is a valid attribute and what kinds of symbols this can be applied to.
    // Also need these so that any inheritable attributes can be inherited.
    //
    ----AttributesForAllBases();

    // Shadowing semantics needs to be done so that Properties, data members
    // shadowed by inaccessible members can be assigned to in the attribute
    // statement.
    //
    ResolveShadowingForAllAttributeClassesUsedInContainer();

    // ---- Attributes for all symbols in current container.

    BCSYM_Container *Container = CurrentContainer();

    Begin_ForAllSymbolsInContainerWithAttrs(SymbolHavingAttrs, Container, GetDeclaredAttrListHead)
    {
        VSASSERT( SymbolHavingAttrs->IsParam() || SymbolHavingAttrs->IsNamedRoot(),
                        "Attributes can only be applied on parameters and named roots!!!");

        BCSYM_NamedRoot *NamedContextOfSymbolHavingAttrs =
            GetNamedContextOfSymbolWithApplAttr(SymbolHavingAttrs);

        // Allocate space for well known attributes if not already allocated
        if (!SymbolHavingAttrs->GetPAttrVals()->GetPWellKnownAttrVals())
        {
            WellKnownAttrVals* pWellKnownAttrVals = CurrentAllocator()->Alloc<WellKnownAttrVals>();
            pWellKnownAttrVals = new (pWellKnownAttrVals) WellKnownAttrVals(CurrentAllocator());

            SymbolHavingAttrs->GetPAttrVals()->SetPWellKnownAttrVals(pWellKnownAttrVals);
        }

        Attribute::----AllAttributesOnSymbol(
            SymbolHavingAttrs,
            CurrentAllocator(),
            CurrentCompilerInstance(),
            CurrentCompilerHost(),
            CurrentErrorLog(NamedContextOfSymbolHavingAttrs),
            CurrentSourceFile(NamedContextOfSymbolHavingAttrs));

    }
    End_ForAllSymbolsInContainerWithAttrs


    if (IsClass(Container))
    {
        InheritWellKnowAttributesFromBases();

        DetermineIfAttribute(Container->PClass(), CurrentCompilerHost());

        if (IsMyGroupCollection(Container))
        {
            MyGroupCollectionInfo& ff = CurrentSourceFile(Container)->GetProject()->m_daMyGroupCollectionInfo.Add();
            ff.m_groupClass = Container->PClass();
            ff.m_daMyGroupMembers;
            CurrentSourceFile(Container)->SetDelayBindingCompletion(true);
#if DEBUG
            VSDEBUGPRINTIF(
                VSFSWITCH(fMyGroupAndDefaultInst),
                "MyGroup: adding MyGroup '%S'\n",
                Container->PClass()->GetName());
#endif
        }


#if DEBUG

        // in MyGroup trace mode tell why a generic class is ignored as group
        // make sure to update the condition here if "IsMyGroupCollection" changes
         else if (  Container->GetPWellKnownAttrVals()->HasMyGroupCollectionData() &&
                    !Container->IsPartialType() &&
                    IsGenericOrHasGenericParent(Container->PClass()))
         {
              VSDEBUGPRINTIF(
                VSFSWITCH(fMyGroupAndDefaultInst),
                "MyGroup: Class '%S' is ignored as a group because it is generic\n",
                Container->PClass()->GetName());
         }
#endif
    }

    SetStatusOf----AttributesOnAllSymbolsInContainer(Done);
}

bool
Bindable::IsMyGroupCollection
(
    BCSYM_Container *Container
)
{
    return (Container->GetPWellKnownAttrVals()->HasMyGroupCollectionData() &&
            !Container->IsPartialType() &&
            !IsGenericOrHasGenericParent(Container->PClass()));
}

BCSYM_NamedRoot *
Bindable::GetNamedContextOfSymbolWithApplAttr
(
    BCSYM *SymbolWithApplAttr
)
{
    VSASSERT( SymbolWithApplAttr->GetPAttrVals(),
                    "Expected symbol with attributes!!!");

    VSASSERT( SymbolWithApplAttr->IsParam() || SymbolWithApplAttr->IsNamedRoot(),
                    "How can any other kind of symbol have attributes on it ?");

    BCSYM_NamedRoot *NamedContextOfSymbolHavingAttrs;

    if (SymbolWithApplAttr->IsParam())
    {
        // Note:
        // In the case of partial methods, we may not have a context for
        // the parameter; in this case, defer to the context of the associated
        // parameter. This happens if we had to create a fake PAttrVals.

        NamedContextOfSymbolHavingAttrs =
            SymbolWithApplAttr->GetPAttrVals()->GetPNon----edData()->m_psymParamContext;
#if 0
        if( NamedContextOfSymbolHavingAttrs == NULL &&
            SymbolWithApplAttr->PParam()->GetCorrespondingParam() != NULL )
        {
            NamedContextOfSymbolHavingAttrs =
                SymbolWithApplAttr->PParam()->GetCorrespondingParam()->
                    GetPAttrVals()->GetPNon----edData()->m_psymParamContext;
        }
#endif
    }
    else
    {
        NamedContextOfSymbolHavingAttrs = SymbolWithApplAttr->PNamedRoot();
    }

    return NamedContextOfSymbolHavingAttrs;
}


void
Bindable::----AttributesForAllBases()
{
    // We need to ---- attributes for all bases because we need to check if a class
    // is a valid attribute and what kinds of symbols this can be applied to.
    // Also need these so that any inheritable attributes can be inherited.

    // One level of bases is enough, because ----AttributesOnAllSymbolsInContainers recursively
    // invokes this on its bases.
    BCSYM_Container *Container = CurrentContainer();

    if (!Container->IsClass() && !Container->IsInterface())
    {
        return;
    }

    BasesIter Bases(Container);

    for(BCSYM_Container *Base = Bases.GetNextBase();
        Base;
        Base = Bases.GetNextBase())
    {
        if (Base->IsBad())
        {
            continue;
        }

        ----AttributesOnAllSymbolsInContainer(Base, m_CompilationCaches);
    }
}

void
Bindable::ResolveShadowingForAllAttributeClassesUsedInContainer()
{
   // Shadowing semantics needs to be done so that Properties, data members
   // shadowed by inaccessible members can be assigned to in the attribute
   // statement.

   BCSYM_Container *Container = CurrentContainer();

   Begin_ForAllSymbolsInContainerWithAttrs(SymbolHavingAttrs, Container, GetDeclaredAttrListHead)
   {
        BCITER_ApplAttrs ApplAttrsIter(SymbolHavingAttrs);

        while (BCSYM_ApplAttr *AppliedAttribute = ApplAttrsIter.GetNext())
        {
            BCSYM *BoundAttribute = AppliedAttribute->GetAttributeSymbol();

            if (BoundAttribute && BoundAttribute->IsContainer())
            {
                ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(BoundAttribute->PContainer(), m_CompilationCaches);
            }
        }
   }
   End_ForAllSymbolsInContainerWithAttrs
}

void
Bindable::InheritWellKnowAttributesFromBases()
{
   BCSYM_Container *Container = CurrentContainer();

    if (!Container->IsClass())
    {
        return;
    }

    BCSYM_Container *Base = Container->PClass()->GetCompilerBaseClass();

    if (Base)
    {
        InheritWellKnownAttributes(Base);
    }
}


/*****************************************************************************
    ;InheritWellKnownAttributes

Recursively gathers all the well-known attributes from the base class
and applies them to this class.
*****************************************************************************/
void
Bindable::InheritWellKnownAttributes
(
    BCSYM_Container *Base
)
{
    BCSYM_Container *Derived = CurrentContainer();

    //
    // Now update this class' attributes with those from the base class.
    //
    AttrVals *BaseAttributeValues;
    BaseAttributeValues = Base->GetPAttrVals();

    // If the base class does not have any attributes, we have nothing to do.
    if (!BaseAttributeValues)
    {
        return;
    }

    //
    // 




    if (!Derived->GetPAttrVals())
    {
        const bool AllocForWellKnownAttributesToo = true;

        Derived->AllocAttrVals(
                CurrentAllocator(),
                CS_Bound,
                CurrentSourceFile(Derived),
                Derived,
                AllocForWellKnownAttributesToo);

        // 
    }

    //
    // Inherit well-known attributes from the base class.
    //
    Derived->GetPAttrVals()->GetPWellKnownAttrVals()->InheritFromSymbol(Base);

}


/*****************************************************************************
;DetermineIfAttribute

Run through this class and all its base classes and determine if the class is a custom
attribute (i.e., inherits from System.Attribute) or a security attribute
(inherits from System.Security.Permissions.SecurityAttribute).
*****************************************************************************/
void
Bindable::DetermineIfAttribute
(
    BCSYM_Class *Class,
    CompilerHost *CompilerHost
)
{
    // start out at the root of the inheritance tree (the most base class)
    BCSYM_Class *Base = Class->GetCompilerBaseClass();

    if (!Base)
    {
        return;
    }

    if (Base->IsSameContainer(CompilerHost->GetFXSymbolProvider()->GetType(FX::AttributeType)) ||
        Base->IsAttribute())
    {
        Class->SetIsAttribute(true);
    }

    // Certain platforms do not support security attributes.
    if (CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::SecurityAttributeType) &&
        (Base->IsSameContainer(CompilerHost->GetFXSymbolProvider()->GetType(FX::SecurityAttributeType)) ||
         Base->IsSecurityAttribute()))
    {
        Class->SetIsSecurityAttribute(true);
    }
}

bool
IgnoreAttributesOnSymbol
(
    BCSYM *Symbol
)
{
    // 


    // Ignore all synthetic members except property accessors and user defined event methods (since they may have valid attributes).
    return
        Symbol->IsNamedRoot() &&
        Bindable::IsSynthetic(Symbol->PNamedRoot()) &&
        !(Symbol->IsProc() &&
            (Symbol->PProc()->IsPropertyGet() ||
                Symbol->PProc()->IsPropertySet() ||
                (Symbol->PProc()->CreatedByEventDecl() && !Symbol->IsSyntheticMethod())));
}

void
Bindable::VerifyAttributesOnAllSymbolsInContainer()
{
    VSASSERT( GetStatusOfVerifyAttributesOnAllSymbolsInContainer() != InProgress,
                    "Should never have cyclic dependencies when verifying Attributes!!!");

    if (GetStatusOfVerifyAttributesOnAllSymbolsInContainer() == Done)
    {
        return;
    }

    SetStatusOfVerifyAttributesOnAllSymbolsInContainer(InProgress);

    VerifyCustomAttributesOnAllSymbolsInContainer();

    BCSYM_Container *Container = CurrentContainer();

    // check for incorrect usage of well known attributes

    // Verify attributes applied on each symbol
    Begin_ForAllSymbolsInContainerWithAttrs(SymbolToCheck, Container, GetDeclaredAttrListHead)
    {
        VSASSERT( SymbolToCheck,
                        "How can a symbol without any attributes be on this list?");

        if (IgnoreAttributesOnSymbol(SymbolToCheck))
        {
            continue;
        }

        // We want to skip any symbols that are partial declarations for methods or params.
        // This is because they share a common well-known table, and we don't want to
        // report errors twice. Today, Attribute::VerifyAttributeUsage
        // does not validate params, but tomorrow it may, so take care here.

        if( ( SymbolToCheck->IsMethodImpl() &&
              SymbolToCheck->PMethodImpl()->IsPartialMethodDeclaration() &&
              SymbolToCheck->PMethodImpl()->GetAssociatedMethod() != NULL ) ||
            ( SymbolToCheck->IsParam() &&
              SymbolToCheck->PParam()->IsPartialMethodParamDeclaration() &&
              SymbolToCheck->PParam()->GetAssociatedParam() != NULL ) )
        {
            continue;
        }

        Attribute::VerifyAttributeUsage
        (
            SymbolToCheck,
            CurrentErrorLog(GetNamedContextOfSymbolWithApplAttr(SymbolToCheck)),
            m_CompilerHost
        );
    }
    End_ForAllSymbolsInContainerWithAttrs

    // verify attributes obtained during bindable, usually through inheriting inheritable
    // attribute from the base class
    //
    Begin_ForAllSymbolsInContainerWithAttrs(SymbolToCheck, Container, GetBoundAttrListHead)
    {
        VSASSERT( SymbolToCheck,
                        "How can a symbol without any attributes be on this list?");

        if (IgnoreAttributesOnSymbol(SymbolToCheck))
        {
            continue;
        }

        // We want to skip any symbols that are partial declarations for methods or params.
        // This is because they share a common well-known table, and we don't want to
        // report errors twice. Today, Attribute::VerifyAttributeUsage
        // does not validate params, but tomorrow it may, so take care here.

        if( ( SymbolToCheck->IsMethodImpl() &&
              SymbolToCheck->PMethodImpl()->IsPartialMethodDeclaration() &&
              SymbolToCheck->PMethodImpl()->GetAssociatedMethod() != NULL ) ||
            ( SymbolToCheck->IsParam() &&
              SymbolToCheck->PParam()->IsPartialMethodParamDeclaration() &&
              SymbolToCheck->PParam()->GetAssociatedParam() != NULL ) )
        {
            continue;
        }

        Attribute::VerifyAttributeUsage
        (
            SymbolToCheck,
            CurrentErrorLog(GetNamedContextOfSymbolWithApplAttr(SymbolToCheck)),
            m_CompilerHost
        );
    }
    End_ForAllSymbolsInContainerWithAttrs


    SetStatusOfVerifyAttributesOnAllSymbolsInContainer(Done);
}


void
Bindable::VerifyCustomAttributesOnAllSymbolsInContainer()
{
    BCSYM_Container *Container = CurrentContainer();

    Begin_ForAllSymbolsInContainerWithAttrs(SymbolHavingAttrs, Container, GetDeclaredAttrListHead)
    {
        VSASSERT( SymbolHavingAttrs,
                        "How can a symbol without any attributes be on this list?");

        BCSYM_NamedRoot *NamedContext = GetNamedContextOfSymbolWithApplAttr(SymbolHavingAttrs);

        if (IgnoreAttributesOnSymbol(SymbolHavingAttrs))
        {
            continue;
        }

        VerifyCustomAttributesOnSymbol(
            SymbolHavingAttrs,
            CurrentErrorLog(NamedContext),
            CurrentSourceFile(NamedContext));
    }
    End_ForAllSymbolsInContainerWithAttrs
}


/*****************************************************************************
;VerifyCustomAttributes

Verifies that the custom attributes on pSymbol are classes and are valid to be used as
attributes and also that they can be applied to pSymbol.

Assembly- and module-level attributes are stored on the SourceFile's
UnnamedNamespace.  This is the only time we need the pSourceFile param
to be non-NULL.
*****************************************************************************/
void
Bindable::VerifyCustomAttributesOnSymbol
(
    BCSYM * pSymbol,
    ErrorTable *pErrorTable,
    SourceFile *pSourceFile
)
{
    if (!pSymbol)
    {
        return;     // Nothing to verify!
    }

    VSASSERT( pSymbol->IsNamedRoot() || pSymbol->IsParam(), "Bad symbol!");

    if (!pSymbol->GetPAttrVals())
    {
        return;
    }

    Compiler * pCompiler = pSourceFile->GetCompiler();;

    NorlsAllocator scratch(NORLSLOC);

    Declaration * pExtensionAttribute =
        Semantics::GetExtensionAttributeClass
        (
            pCompiler,
            pSourceFile->GetCompilerHost(),
            pSourceFile,
            &scratch,
            NULL
        );

    BCITER_ApplAttrs ApplAttrsIter(pSymbol);

    while (BCSYM_ApplAttr * pAttributeSymbol = ApplAttrsIter.GetNext())
    {
        if
        (
            pAttributeSymbol->GetAttributeSymbol() == pExtensionAttribute
            &&
            pSymbol->IsProc()
        )
        {
            continue;
        }

        // We need to ---- attributes for all classes used as attributes in this class
        // because we need to check if a class is a valid attribute and what kind
        // of symbols this can be applied to. Needed for validation purposes.
        //
        if (IsClass(pAttributeSymbol->GetAttributeSymbol()))
        {
            BCSYM_Class *pAppliedAttributeClass = pAttributeSymbol->GetAttributeSymbol()->PClass();

            ----AttributesOnAllSymbolsInContainer(pAppliedAttributeClass, m_CompilationCaches);
        }

        if (pAttributeSymbol->GetAttrClass()->DigThroughNamedType()->IsBad())
        {
            continue;
        }

        if (!pAttributeSymbol->GetAttrClass()->DigThroughNamedType()->IsClass() ||
            pAttributeSymbol->GetAttrClass()->DigThroughNamedType()->PClass()->IsEnum())
        {
            ReportErrorOnSymbol(
                ERRID_AttributeMustBeClassNotStruct1,
                CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                pAttributeSymbol,
                pAttributeSymbol->GetAttrClass()->GetErrorName(CurrentCompilerInstance()));

            continue;
        }

        BCSYM_Class * pAttributeClass =
            pAttributeSymbol->GetAttributeSymbol()->PClass();

        // AnthonyL: Check if the attribute class can be applied to the symbol and if multiple applications are present and valid.

        // Note that error reporting is done in IsValidAttributeClass
        //
        if (!pAttributeClass->IsValidAttributeClass(pErrorTable, pAttributeSymbol))
        {
            continue;
        }

        CorAttributeTargets Targets;
        bool AllowMultipleUse;
        bool IsInherited;
        bool IsOnAssemblyOrModule;

        //
        // See if we're dealing with an assembly- or module-level attribute
        //
        IsOnAssemblyOrModule =
            pSourceFile != NULL &&
            pSymbol == pSourceFile->GetRootNamespace();

        //
        // Check if the attribute class can be applied to the symbol.
        //

        pAttributeClass->GetPWellKnownAttrVals()->GetAttributeUsageData(
            &Targets,
            &AllowMultipleUse,
            &IsInherited);

        bool IsApplicationValid = false;

        if (( Targets & catClass) &&
            (pSymbol->IsClass() && !pSymbol->IsDelegate() && !pSymbol->IsStruct()))
        {
            IsApplicationValid = true;
        }
        else if (( Targets & catStruct) && pSymbol->IsStruct())
        {
            IsApplicationValid = true;
        }
        else if (( Targets & catEnum) && pSymbol->IsEnum())
        {
            IsApplicationValid = true;
        }
        else if (( Targets & catConstructor) &&
                ( pSymbol->IsProc() && pSymbol->PProc()->IsAnyConstructor()))
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catMethod) &&
                (pSymbol->IsProc() && !pSymbol->IsProperty() &&
                    !pSymbol->PProc()->IsAnyConstructor() &&
                    !pSymbol->IsEventDecl()))
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catProperty) && pSymbol->IsProperty())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catField) &&
                (pSymbol->IsMember() && !pSymbol->IsProc()))
        {
            IsApplicationValid = true;
        }
        // The following condition allows NonSerialized attribute being applied on event. (Bug674331)
        else if (((Targets & catEvent) || 
                       (pAttributeSymbol->GetAttrIndex() && 
                       pAttributeSymbol->GetAttrIndex()->Attributes[0].m_attrkind == attrkindNonSerialized_Void)) 
                  && 
                  pSymbol->IsEventDecl())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catInterface) && pSymbol->IsInterface())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catParameter) && pSymbol->IsParam()&& !pSymbol->PParam()->IsReturnType())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catReturnValue) && pSymbol->IsParam() && pSymbol->PParam()->IsReturnType())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catDelegate) && pSymbol->IsDelegate())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catAssembly) && pAttributeSymbol->IsAssembly())
        {
            IsApplicationValid = true;
        }
        else if ((Targets & catModule) && pAttributeSymbol->IsModule())
        {
            IsApplicationValid = true;
        }

        if ( !IsApplicationValid )
        {
            if (IsOnAssemblyOrModule)
            {
                // Special case: Target was assembly or module, but the
                // symbol the attribute is attached to has no name.
                // Emit a special error in this case.
                if (pAttributeSymbol->IsAssembly())
                {
                    ReportErrorOnSymbol(
                        ERRID_InvalidAssemblyAttribute1,
                        CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                        pAttributeSymbol,
                        pAttributeClass->GetName());
                }
                else if (pAttributeSymbol->IsModule())
                {
                    ReportErrorOnSymbol(
                        ERRID_InvalidModuleAttribute1,
                        CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                        pAttributeSymbol,
                        pAttributeClass->GetName());
                }
            }
            else
            {
                // Normal case: Attribute can't be applied to this symbol
                ReportErrorOnSymbol(
                    ERRID_InvalidAttributeUsage2,
                    CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                    pAttributeSymbol,
                    pAttributeClass->GetName(),
                    pSymbol->IsNamedRoot() ?
                        pSymbol->GetErrorName(CurrentCompilerInstance()) :
                pSymbol->PParam()->GetName());
            }
        }

        if (pAttributeClass == CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::CLSCompliantAttributeType) &&
            pSymbol->IsProc() &&
            (pSymbol->PProc()->IsPropertyGet() ||
            pSymbol->PProc()->IsPropertySet()))
        {
            // Warning and not error because this was permitted in RTM and Everett.

            // "System.CLSCompliantAttribute cannot be applied to property 'Get'/'Set'.

            ReportErrorOnSymbol(
                WRNID_CLSAttrInvalidOnGetSet,
                CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                pAttributeSymbol);
        }

        // Report error if an async method is annotated by SecurityCritical attribute or SecuritySafeCritical attribute,
        // CheckSecurityCriticalAttributeOnParentOfAysnc also reports error if an async method is in a container 
        // which is annotated by SecurityCritical or SecuritySafeCritical attribute
        if (pSymbol->IsProc() && (pSymbol->PProc()->IsAsyncKeywordUsed() || pSymbol->PProc()->IsIteratorKeywordUsed()))
        {
           if (pAttributeClass == CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::SecurityCriticalAttributeType))
           {
               ReportErrorOnSymbol(
                    ERRID_SecurityCriticalAsync,
                    CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                    pAttributeSymbol,
                    L"SecurityCritical"
                    );      
           }
        
           if (pAttributeClass == CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::SecuritySafeCriticalAttributeType))
           {
               ReportErrorOnSymbol(
                   ERRID_SecurityCriticalAsync,
                   CurrentErrorLog(pAttributeSymbol->GetAttrClass()),
                   pAttributeSymbol,
                   L"SecuritySafeCritical"
                   );              
           }
        }

        //
        // Check if multiple applications are present and valid.
        //

        if (!AllowMultipleUse)
        {
            CheckForMultipleUsesOfAttribute(
                pSymbol,
                pAttributeSymbol,
                pAttributeClass);
        }
    }

}

void
Bindable::CheckForMultipleUsesOfAttribute
(
    BCSYM *SymbolHavingAttrs,
    BCSYM_ApplAttr *AppliedAttributeSymbol,
    BCSYM_Class *AttributeClassToCheckFor
)
{
    if (AppliedAttributeSymbol->IsBad())
    {
        return;
    }

    BCSYM *SymbolWithAttributesToCheckAgainst =
        SymbolHavingAttrs->IsNamespace() ?
            SymbolHavingAttrs->PNamespace()->GetNextNamespaceInSameProject() :
            SymbolHavingAttrs;

    // For partial methods, the attribute walker invalidates the loop below, because we will
    // always find the same attribute first. Therefore, here, when validating attributes,
    // if we are the implementation, select the partial declaration as the location to start. This
    // will ensure that we don't start validating at the same attribute that we are checking.

    if( SymbolWithAttributesToCheckAgainst->IsMethodImpl() )
    {
        if( SymbolWithAttributesToCheckAgainst->PMethodImpl()->IsPartialMethodImplementation() )
        {
            SymbolWithAttributesToCheckAgainst =
                SymbolWithAttributesToCheckAgainst->PMethodImpl()->GetAssociatedMethod();
        }
    }
    else if( SymbolWithAttributesToCheckAgainst->IsParam() )
    {
        if( SymbolWithAttributesToCheckAgainst->PParam()->IsPartialMethodParamImplementation() )
        {
            SymbolWithAttributesToCheckAgainst =
                SymbolWithAttributesToCheckAgainst->PParam()->GetAssociatedParam();
        }
    }

    do
    {
       const bool LookOnlyInImmediatePartialContainer = true;
       BCITER_ApplAttrs ApplAttrsIter(SymbolWithAttributesToCheckAgainst, !LookOnlyInImmediatePartialContainer);

       while (BCSYM_ApplAttr *pAttributeSymbolTemp = ApplAttrsIter.GetNext())
        {
            if (pAttributeSymbolTemp->IsBad())
            {
                continue;
            }

            if (pAttributeSymbolTemp == AppliedAttributeSymbol)
            {
                // If reached here, then no error i.e. not applied multiple times

                // 




                if (AttributeClassToCheckFor ==
                        CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::CLSCompliantAttributeType) &&
                    SymbolHavingAttrs->IsNamespace())
                {
                    // Assembly level CLS Compliant attribute

                    BCSYM_Namespace *Namespace = SymbolHavingAttrs->PNamespace();

                    VSASSERT( Namespace->GetSourceFile(),
                                    "Metadata symbol unexpected here!!!");

                    CompilerProject *CurrentProject = Namespace->GetSourceFile()->GetProject();

                    VSASSERT( CurrentProject,
                                "How can a sourcefile not be in any project ?");

                    if ((AppliedAttributeSymbol->IsAssembly() && !CurrentProject->OutputIsModule()) ||
                        (AppliedAttributeSymbol->IsModule() && CurrentProject->OutputIsModule()))
                    {
                        bool IsCompliant;
                        Namespace->GetPWellKnownAttrVals()->GetCLSCompliantData(&IsCompliant);

                        // Propagate the Project level setting from the namespace to the Project
                        CurrentProject->SetProjectClaimsCLSCompliance(IsCompliant);

#if IDE 
                        // Needed for decompilation
                        CurrentProject->SetFileThatCausedProjectsCLSComplianceClaim(Namespace->GetCompilerFile());
#endif
                    }
                }

                return;
            }

            // Need to handle case where pAttributeCompare is not a class.  In that
            // case, an error has already been reported, so just ignore it.
            BCSYM *pAttributeCompare =
                pAttributeSymbolTemp->GetAttrClass()->DigThroughNamedType();

            BCSYM_Class *pAttributeClassCompare =
                pAttributeCompare->IsClass() ? pAttributeCompare->PClass() : NULL;

            if (AttributeClassToCheckFor == pAttributeClassCompare)
            {
                //
                // Need to handle the case where both module- and assembly-level
                // attributes end up on the same symbol.  This is not an error
                // if one is on the assembly and the other is on the module.
                //
                // Note that a given symbol can hold either assembly/module
                // attributes or attributes with "real" targets, but no symbol
                // can hold a mixture of both.  We use fAnyAssemblyOrModuleAttrs
                // to decide which case we're dealing with.
                //
                // This check will miss duplicate assembly- and
                // module-level attributes that are used in different files,
                // but ALink will catch those in MetaEmit::ALinkEmitAssemblyAttributes.
                //
                bool fAnyAssemblyOrModuleAttrs =
                    AppliedAttributeSymbol->IsAssembly() | pAttributeSymbolTemp->IsAssembly() |
                    AppliedAttributeSymbol->IsModule()   | pAttributeSymbolTemp->IsModule();

                if (!fAnyAssemblyOrModuleAttrs ||
                    (AppliedAttributeSymbol->IsModule()   && pAttributeSymbolTemp->IsModule()) ||
                    (AppliedAttributeSymbol->IsAssembly() && pAttributeSymbolTemp->IsAssembly()))
                {
                    ReportErrorOnSymbol(
                        ERRID_InvalidMultipleAttributeUsage1,
                        CurrentErrorLog(AppliedAttributeSymbol->GetAttrClass()),
                        AppliedAttributeSymbol,
                        AttributeClassToCheckFor->GetName());

                    AppliedAttributeSymbol->SetIsBad();

                    return;
                }
            }
        }

        VSASSERT( SymbolWithAttributesToCheckAgainst == SymbolHavingAttrs ||
                  SymbolWithAttributesToCheckAgainst->IsNamespace(),
                    "No other symbol besides a namespace should be traversed in this way!!!");

    } while (SymbolWithAttributesToCheckAgainst != SymbolHavingAttrs &&
             (SymbolWithAttributesToCheckAgainst =
                SymbolWithAttributesToCheckAgainst->PNamespace()->GetNextNamespaceInSameProject()));

}


/*****************************************************************************
;GetSimilarKind

Some symbols can be considered as generally equivalent for some situations -
this 'normalizes' those symbols.
*****************************************************************************/
BilKind // the 'normalized' symbol
Bindable::GetSimilarKind
(
    BilKind SymbolKind // [in] the symbol to normalize
)
{
    switch ( SymbolKind )
    {
        case SYM_MethodDecl: // method decls and method impls will be 'equivalent'
            return SYM_MethodImpl;

        default:
            return SymbolKind;
    }
}


bool // true - PossiblyDerived derives from Base / false otherwise
Bindable::DerivesFrom
(
    BCSYM_Container *PossiblyDerived, // [in] does this thing derive from Base?
    BCSYM_Container *Base // [in] the base class
)
{
    VSASSERT( Base && PossiblyDerived,
                    "Non-NULL base and derived expected!!!");

    // clearly they aren't derived from each other
    //
    if (Base->IsSameContainer(PossiblyDerived))
    {
        return false;
    }

    // classes can't derive from interfaces, etc. so these aren't related by inheritance
    if (Base->IsClass() != PossiblyDerived->IsClass())
    {
        return false;
    }

    if (Base->IsClass())
    {
        while (PossiblyDerived = PossiblyDerived->PClass()->GetCompilerBaseClass())
        {
            if (Base->IsSameContainer(PossiblyDerived))
            {
                return true;
            }
        }

        return false;
    }

    // Interface
    return PossiblyDerived->PInterface()->DerivesFrom(
                CurrentCompilerInstance(),
                Base->PInterface());
}

/**************************************************************************************************
;IsGuyShadowed

Determine whether a member was shadowed by another member
***************************************************************************************************/
bool
Bindable::IsMemberShadowed
(
    BCSYM_NamedRoot *Member, // [in] is this Member shadowed
    MembersHashTable *ShadowingMembers // [in] hash of accessible members that shadow other members
)
{
    BCSYM_NamedRoot *PossibleShadowingMember = ShadowingMembers->Find(Member->GetName());
    BCSYM_Container *ContainerOfMember = Member->GetContainer();

    while (PossibleShadowingMember)
    {
        if (DerivesFrom(PossibleShadowingMember->GetContainer(), ContainerOfMember))
        {
            return true;
        }

        PossibleShadowingMember = ShadowingMembers->FindNextOfSameName();
    }

    return false;
}

void
Bindable::ScanContainerForObsoleteUsage(bool NeedToCheckObsolete)
{
    VSASSERT( GetStatusOfScanContainerForObsoleteUsage() != InProgress,
                    "Circular dependencies unexpected here!!!");

    if (GetStatusOfScanContainerForObsoleteUsage() == Done)
    {
        return;
    }

    SetStatusOfScanContainerForObsoleteUsage(InProgress);

    for(BCSYM_Container *PartialContainer = CurrentContainer();
        PartialContainer;
        PartialContainer = PartialContainer->GetNextPartialType())
    {
        ObsoleteChecker checker(CurrentSourceFile(PartialContainer), CurrentErrorLog(PartialContainer));

        checker.CheckForLongFullyQualifiedNames(PartialContainer);

        checker.ScanContainerForObsoleteUsage(PartialContainer, NeedToCheckObsolete);
    }

    // Complete any obsolete checks that semantics helpers triggered but could not
    // be completed because attribute cracking was not guaranteed to be completed
    // at that time.
    //
    if (NeedToCheckObsolete && m_DelayedCheckManager)
    {
        m_DelayedCheckManager->CompleteDelayedObsoleteChecks();
    }

    SetStatusOfScanContainerForObsoleteUsage(Done);
}

// Complete result type accessibility checks that semantics helpers registered but
// that we could not verify due to the fact that for that we need to dig through
// named types which is not possible if we haven't resolved projectlevelimports
// and application objects.
void
Bindable::ScanContainerForResultTypeChecks
(
)
{
    if (m_DelayedCheckManager)
    {
        m_DelayedCheckManager->CompleteDelayedResultTypeChecks();
    }
}

bool
Bindable::SourceFilesOfContainerHaveErrorsInCurrentCompilerStep()
{
    VSASSERT(!CurrentContainer()->IsPartialTypeAndHasMainType(),
                "Unexepected partial type!!!");

    for(BCSYM_Container *Container = CurrentContainer();
        Container;
        Container = Container->GetNextPartialType())
    {
        ErrorTable *ErrorLog = CurrentErrorLog(Container);
        SourceFile *SourceFile = CurrentSourceFile(Container);

        if (!ErrorLog || !SourceFile)
        {
            continue;
        }

        if (ErrorLog->HasErrors() ||
            SourceFile->HasActiveErrors())
        {
            return true;
        }
    }

    return false;
}

// Use ResolveNamedTypeAndTypeArguments instead of ResolveNamedType in cases where the argument
// types of generic bindings do not appear on a type list and so will not naturally be bound.
void
Bindable::ResolveNamedTypeAndTypeArguments
(
    BCSYM_NamedType *NamedType, // [in] the named type to resolve
    TypeResolutionFlags Flags,
    NameFlags NameInterpretFlags,
    bool DisableNameLookupCaching
)
{
    // Resolve the type arguments
    //
    ResolveTypeArguments(
            NamedType->GetArgumentsArray(),
            NamedType->GetNameCount(),
            CurrentErrorLog(NamedType),
            CurrentSourceFile(NamedType->GetContext()),
            Flags,
            NameInterpretFlags,
            DisableNameLookupCaching);

    // Resolve the named type
    //
    ResolveNamedType(
        NamedType,
        Flags,
        NameInterpretFlags,
        DisableNameLookupCaching);
}

void
Bindable::ResolveTypeArguments
(
    NameTypeArguments **ArgumentsArray,
    unsigned NumberOfArguments,
    ErrorTable *ErrorLog,
    SourceFile *CurrentSourceFile,
    TypeResolutionFlags Flags,
    NameFlags NameInterpretFlags,
    bool DisableNameLookupCaching
)
{
    if (!ArgumentsArray)
    {
        return;
    }

    bool HasGenericArguments = false;

    ResolveTypeArguments(
        ArgumentsArray,
        NumberOfArguments,
        CurrentAllocator(),
        ErrorLog,
        CurrentCompilerInstance(),
        CurrentCompilerHost(),
        CurrentSourceFile,
        // Don't perform obsolete checks. Will do the
        // obsolete checking for the whole container later.
        Flags & ~TypeResolvePerformObsoleteChecks,
        m_CompilationCaches,
        NameInterpretFlags,
        DisableNameLookupCaching,
        HasGenericArguments);
}

void
Bindable::ResolveTypeArguments
(
    NameTypeArguments **ArgumentsArray,
    unsigned NumberOfArguments,
    NorlsAllocator *Allocator,     // [in] for the Binder - may be NULL if no generic bindings occur in the type
    ErrorTable *ErrorLog,          // [in] the error log to put our errors into - NULL if we are importing metadata
    Compiler *CompilerInstance,    // [in] the current context
    CompilerHost *CompilerHost,    // [in] the compilerhost
    SourceFile *CurrentSourceFile, // [in] the current file
    TypeResolutionFlags Flags,     // [in] indicates how to resolve the type
    CompilationCaches *IDECompilationCaches,
    NameFlags NameInterpretFlags,  // [in] indicates the name binding
    bool DisableNameLookupCaching,  // [in] indicates that name look up caching in semantics helpers should be disabled
    bool &HasGenericArgument
)
{
    if (!ArgumentsArray)
    {
        return;
    }

    for (unsigned ArgumentsIndex = 0; ArgumentsIndex < NumberOfArguments; ArgumentsIndex++)
    {
        NameTypeArguments *Arguments = ArgumentsArray[ArgumentsIndex];

        if (!Arguments)
        {
            continue;
        }

        for (unsigned ArgumentIndex = 0; ArgumentIndex < Arguments->m_ArgumentCount; ArgumentIndex++)
        {
            BCSYM *ArgumentType = Arguments->m_Arguments[ArgumentIndex];
            HasGenericArgument = true;

            if (!ArgumentType)
            {
                continue;
            }

            ArgumentType = ArgumentType->ChaseToNamedType();

            VSASSERT(ArgumentType, "Non-NULL result expected!!!");

            if (ArgumentType->IsNamedType())
            {
                // Resolve the type arguments
                //
                ResolveTypeArguments(
                    ArgumentType->PNamedType()->GetArgumentsArray(),
                    ArgumentType->PNamedType()->GetNameCount(),
                    Allocator,
                    ErrorLog,
                    CompilerInstance,
                    CompilerHost,
                    CurrentSourceFile,
                    Flags,
                    IDECompilationCaches,
                    NameInterpretFlags,
                    DisableNameLookupCaching,
                    HasGenericArgument);

                // Resolve the named type
                //
                ResolveNamedType(
                    ArgumentType->PNamedType(),
                    Allocator,
                    ErrorLog,
                    CompilerInstance,
                    CompilerHost,
                    CurrentSourceFile,
                    Flags,
                    IDECompilationCaches,
                    NameInterpretFlags,
                    DisableNameLookupCaching);
            }
        }
    }
}

void
Bindable::ResolveBaseType
(
    BCSYM_NamedType *Type
)
{
    // 


    ResolveNamedTypeAndTypeArguments(
        Type,
        TypeResolveNoFlags,
        NameSearchIgnoreImmediateBases,
        // disable name look up caching because a base and a member type in the same context
        // might bind to different symbols. This could happen because the base is resolved before
        // any nested classes' bases are resolved whereas member types are resolved afterward.
        //
        true);
}

UserDefinedOperators
MapToMatching
(
    UserDefinedOperators Input
)
{
    // Given an operator that requires pair-wise declaration, return its matching operator.
    switch (Input)
    {
        case OperatorIsTrue:       return OperatorIsFalse;
        case OperatorIsFalse:      return OperatorIsTrue;
        case OperatorEqual:        return OperatorNotEqual;
        case OperatorNotEqual:     return OperatorEqual;
        case OperatorLess:         return OperatorGreater;
        case OperatorLessEqual:    return OperatorGreaterEqual;
        case OperatorGreaterEqual: return OperatorLessEqual;
        case OperatorGreater:      return OperatorLess;
        default:
            VSFAIL("didn't expect this operator");
            return Input;
    }
}

void
Bindable::VerifyOperatorsInContainer()
{
    VSASSERT(GetStatusOfVerifyOperatorsInContainer() != InProgress,
             "Circular dependencies unexpected here!!!");
    VSASSERT(!CurrentContainer()->IsPartialTypeAndHasMainType(),
             "partial types aren't allowed in here");

    if (GetStatusOfVerifyOperatorsInContainer() == Done)
    {
        return;
    }

    SetStatusOfVerifyOperatorsInContainer(InProgress);

    if (CurrentContainer()->HasUserDefinedOperators() && !IsStdModule(CurrentContainer()))
    {
        NorlsAllocator Scratch(NORLSLOC);

        RelationalOperatorPool Pool(Scratch);

        Symbols
            SymbolFactory(
                CurrentCompilerInstance(),
                CurrentAllocator(),
                NULL,
                CurrentGenericBindingCache());

        BCSYM *InstanceTypeForMemberContainer =
            IsGenericOrHasGenericParent(CurrentContainer()) ?
                (BCSYM *)SynthesizeOpenGenericBinding(
                    CurrentContainer(),
                    SymbolFactory) :
                CurrentContainer();

        // Walk over every member and perform semantic checks on the operators only.
        BCITER_CHILD Members(CurrentContainer());
        while (BCSYM_NamedRoot *Member = Members.GetNext())
        {
            if (Member->IsProc() &&
                !Member->IsBad() &&
                Member->PProc()->IsUserDefinedOperatorMethod() &&
                !Member->PMethodDecl()->GetAssociatedOperatorDef()->IsBad())
            {
                VerifyOperator(
                    Member->PMethodDecl()->GetAssociatedOperatorDef(),
                    InstanceTypeForMemberContainer,
                    &Pool,
                    &SymbolFactory);
            }
        }

        // Give errors for unmatched operators that require pair-wise declaration.
        CSingleListIter<OperatorInfo> Iter(Pool.CollateUnmatchedOperators());
        while (OperatorInfo *Current = Iter.Next())
        {
            StringBuffer ErrorReplString;
            GetBasicRep(Current->Symbol, &ErrorReplString);

            ReportErrorOnSymbol(
                ERRID_MatchingOperatorExpected2,
                CurrentErrorLog(Current->Symbol),
                Current->Symbol,
                CurrentCompilerInstance()->OperatorToString(MapToMatching(Current->Symbol->GetOperator())),
                ErrorReplString.GetString());
        }
    }

    SetStatusOfVerifyOperatorsInContainer(Done);
}

void
Bindable::VerifyOperator
(
    BCSYM_UserDefinedOperator *Member,
    BCSYM *InstanceTypeForMemberContainer,
    RelationalOperatorPool *Pool,
    Symbols *SymbolFactory
)
{
    VSASSERT((OperatorUNDEF < Member->GetOperator()) && (Member->GetOperator() < OperatorMAXVALID),
             "invalid operator kind");
    VSASSERT(Member->GetType() && Member->GetRawType(),
             "all operators must have a return type");

    UserDefinedOperators Operator = Member->GetOperator();

    bool HasContainingTypeParam = false;

    // Check if one of the parameter types is the defining container.
    unsigned int ParameterCount = 0;
    BCSYM_Param *Parameter = Member->GetFirstParam();

    while (Parameter)
    {
        VSASSERT(!Parameter->IsBad() && !Parameter->IsBadParam(),
                 "if we get this far, we shouldn't see bad params");
        ParameterCount++;

        if (BCSYM::AreTypesEqual(Parameter->GetType(), InstanceTypeForMemberContainer) ||
            (TypeHelpers::IsNullableType(Parameter->GetType(), CurrentCompilerHost()) &&
             BCSYM::AreTypesEqual(TypeHelpers::GetElementTypeOfNullable(Parameter->GetType(),CurrentCompilerHost()), InstanceTypeForMemberContainer)))
        {
            HasContainingTypeParam = true;
        }

        Parameter = Parameter->GetNext();
    }

    VSASSERT((IsUnaryOperator(Operator) && ParameterCount == 1) ||
             (IsBinaryOperator(Operator) && ParameterCount == 2),
             "malformed operator - how did this survive Declared?");

    // 



    // The return type of conversion operators can also be the defining container.
    if (IsConversionOperator(Operator) &&
        (BCSYM::AreTypesEqual(Member->GetType(), InstanceTypeForMemberContainer) ||
         (TypeHelpers::IsNullableType(Member->GetType(), CurrentCompilerHost()) &&
             BCSYM::AreTypesEqual(TypeHelpers::GetElementTypeOfNullable(Member->GetType(),CurrentCompilerHost()), InstanceTypeForMemberContainer))))

    {
        HasContainingTypeParam = true;
    }

    if ((Operator == OperatorIsTrue || Operator == OperatorIsFalse) &&
        !TypeHelpers::IsBooleanType(Member->GetType()))
    {
        // Named types for intrinsics don't occur, so we have no location info for them.
        Location *ErrorLocation =
            Member->GetRawType()->GetLocation() ?
                Member->GetRawType()->GetLocation() :
                Member->GetLocation();

        ReportErrorAtLocation(
            ERRID_OperatorRequiresBoolReturnType1,
            CurrentErrorLog(Member),
            ErrorLocation,
            Member->GetName());

        Member->SetIsBad();

        if (HasContainingTypeParam)
        {
            // No more useful errors to display, so just exit.
            return;
        }
    }

    if (Operator == OperatorShiftLeft || Operator == OperatorShiftRight)
    {
        Type *SecondParamType = Member->GetFirstParam()->GetNext()->GetRawType();  // NULL???

        Type * ActualSecondParamType = SecondParamType->DigThroughNamedType();
        Type * NullableElementType = TypeHelpers::GetElementTypeOfNullable(ActualSecondParamType, m_CompilerHost);
        if
        (
            !TypeHelpers::IsIntegerType(ActualSecondParamType) &&
            (
                !NullableElementType ||
                !TypeHelpers::IsIntegerType(NullableElementType)
            )
        )
        {
            // Named types for intrinsics don't occur, so we have no location info for them.
            Location *ErrorLocation =
                SecondParamType->GetLocation() ?
                    SecondParamType->GetLocation() :
                    Member->GetLocation();

            ReportErrorAtLocation(
                ERRID_OperatorRequiresIntegerParameter1,
                CurrentErrorLog(Member),
                ErrorLocation,
                Member->GetName());

            Member->SetIsBad();

            if (HasContainingTypeParam)
            {
                // No more useful errors to display, so just exit.
                return;
            }
        }
    }

    if (!HasContainingTypeParam)
    {
        // Named types for intrinsics don't occur, so we have no location info for them.
        Location *ErrorLocation =
            IsBinaryOperator(Operator) || IsConversionOperator(Operator) ?
                Member->GetLocation() :
                Member->GetFirstParam()->GetRawType()->GetLocation() ?
                    Member->GetFirstParam()->GetRawType()->GetLocation() :
                    Member->GetLocation();

        ReportErrorAtLocation(
            IsBinaryOperator(Operator) ?
                ERRID_BinaryParamMustBeContainingType1 :
                IsConversionOperator(Operator) ?
                    ERRID_ConvParamMustBeContainingType1 :
                    ERRID_UnaryParamMustBeContainingType1,
            CurrentErrorLog(Member),
            ErrorLocation,
            InstanceTypeForMemberContainer);

        Member->SetIsBad();
        return;
    }

    // Now we can perform conversion operator specific checks.
    if (IsConversionOperator(Operator))
    {
        VSASSERT(IsConversionOperator(Member), "expected conversion operator");

        Type *TargetType = Member->GetType();
        Type *SourceType = Member->GetFirstParam()->GetType();

        // Named types for intrinsics don't occur, so we have no location info for them.
        Location *TargetLocation =
            Member->GetRawType()->GetLocation() ?
                Member->GetRawType()->GetLocation() :
                Member->GetLocation();

        Location *SourceLocation =
            Member->GetFirstParam()->GetRawType()->GetLocation() ?
                Member->GetFirstParam()->GetRawType()->GetLocation() :
                Member->GetLocation();

        VSASSERT(BCSYM::AreTypesEqual(TargetType, InstanceTypeForMemberContainer) ||
                 (TypeHelpers::IsNullableType(TargetType, CurrentCompilerHost()) &&
                  BCSYM::AreTypesEqual(TypeHelpers::GetElementTypeOfNullable(TargetType,CurrentCompilerHost()), InstanceTypeForMemberContainer)) ||
                 BCSYM::AreTypesEqual(SourceType, InstanceTypeForMemberContainer) ||
                 (TypeHelpers::IsNullableType(SourceType, CurrentCompilerHost()) &&
                  BCSYM::AreTypesEqual(TypeHelpers::GetElementTypeOfNullable(SourceType,CurrentCompilerHost()), InstanceTypeForMemberContainer)),
                 "expected containing type");

        // 
        if (TypeHelpers::IsRootObjectType(SourceType))
        {
            ReportErrorAtLocation(ERRID_ConversionFromObject, CurrentErrorLog(Member), SourceLocation);
            Member->SetIsBad();
            return;
        }

        if (TypeHelpers::IsRootObjectType(TargetType))
        {
            ReportErrorAtLocation(ERRID_ConversionToObject, CurrentErrorLog(Member), TargetLocation);
            Member->SetIsBad();
            return;
        }

        if (TypeHelpers::IsInterfaceType(SourceType))
        {
            ReportErrorAtLocation(ERRID_ConversionFromInterfaceType, CurrentErrorLog(Member), SourceLocation);
            Member->SetIsBad();
            return;
        }

        if (TypeHelpers::IsInterfaceType(TargetType))
        {
            ReportErrorAtLocation(ERRID_ConversionToInterfaceType, CurrentErrorLog(Member), TargetLocation);
            Member->SetIsBad();
            return;
        }

        if (TypeHelpers::EquivalentTypes(TypeHelpers::GetElementTypeOfNullable(TargetType,CurrentCompilerHost()),
                            TypeHelpers::GetElementTypeOfNullable(SourceType,CurrentCompilerHost())))
        {
            ReportErrorOnSymbol(ERRID_ConversionToSameType, CurrentErrorLog(Member), Member);
            Member->SetIsBad();
            return;
        }

        if ((TypeHelpers::IsClassOrRecordType(SourceType) || SourceType->IsGenericParam()) &&
             TypeHelpers::IsClassOrRecordType(TargetType) || TargetType->IsGenericParam())
        {
            const bool ChaseThroughGenericBindings = true;

            if (TypeHelpers::IsOrInheritsFrom(TargetType, SourceType, *SymbolFactory, !ChaseThroughGenericBindings, NULL))
            {
                ReportErrorOnSymbol(
                    BCSYM::AreTypesEqual(TargetType, InstanceTypeForMemberContainer) ?
                        ERRID_ConversionFromBaseType :
                        ERRID_ConversionToDerivedType,
                    CurrentErrorLog(Member),
                    Member);
                Member->SetIsBad();
                return;
            }

            if (TypeHelpers::IsOrInheritsFrom(SourceType, TargetType, *SymbolFactory, !ChaseThroughGenericBindings, NULL))
            {
                ReportErrorOnSymbol(
                    BCSYM::AreTypesEqual(TargetType, InstanceTypeForMemberContainer) ?
                        ERRID_ConversionFromDerivedType :
                        ERRID_ConversionToBaseType,
                    CurrentErrorLog(Member),
                    Member);
                Member->SetIsBad();
                return;
            }
        }

        // Interestingly, by this point all predefined conversions have been ruled out.
        VSASSERT(
            Semantics::ClassifyPredefinedConversion(TargetType, SourceType, CurrentCompilerHost()) == ConversionError,
            "surprising conversion operator");
    }

    Pool->Add(Operator, Member);

    return;
}

inline GenericBindingCache*
Bindable::CurrentGenericBindingCache()
{
    return
        m_CurrentContainerContext->GetCompilerFile() ?
           m_CurrentContainerContext->GetCompilerFile()->GetCurrentGenericBindingCache() :
            NULL;
}

ErrorTable *
Bindable::CurrentErrorLog
(
    BCSYM *Symbol,
    BCSYM_NamedRoot *NamedRootContext
)
{
    VSASSERT(Symbol, "Error table request for NULL symbol unexpected!!!");

    if (Symbol->IsNamedRoot())
    {
        return CurrentErrorLog(Symbol->PNamedRoot());
    }

    if (Symbol->IsNamedType())
    {
        return CurrentErrorLog(Symbol->PNamedType());
    }

    if (Symbol->IsArrayType())
    {
        return CurrentErrorLog(Symbol->PArrayType()->GetRawRoot(), NamedRootContext);
    }

    return CurrentErrorLog(NamedRootContext);
}
