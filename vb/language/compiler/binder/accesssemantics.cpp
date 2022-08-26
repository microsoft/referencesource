//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to verify access exposure.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

bool Bindable::VerifyAccessExposure(
    BCSYM_NamedRoot *Member,
    BCSYM *RawType,
    DynamicArray<BCSYM_Container *> *&ContainersWithAccessError,  // [In] expect NULL, [out] Non-NULL if any access exposure errors, caller needs to delete it
    DynamicArray<BCSYM *> *&TypesExposed,                         // [In] expect NULL, [out] Non-NULL if any access exposure errors, caller needs to delete it
    bool TypeIsParentBinding)
{
    bool IsTypeNotExposed = true;
    bool AreTypeArgumentsNotExposed = true;
    bool SkipVerifyingMainType = false;

    BCSYM *ChasedType = RawType ? ChaseThroughToType(RawType) : NULL;
    BCSYM_GenericTypeBinding *ParentGenericBinding = NULL;

    DynamicArray<BCSYM_Container *> *ContainersWithAccessErrorForParentBindings = NULL;

    if (ChasedType && !ChasedType->IsBad())
    {
        if (ChasedType->IsGenericTypeBinding())
        {
            BCSYM_GenericTypeBinding *GenericBinding = ChasedType->PGenericTypeBinding();

            // Verify the generic type and each of the type arguments.

            unsigned ArgumentCount = GenericBinding->GetArgumentCount();
            BCSYM **TypeArguments = NULL;
            bool DigThroughTypeArguments = false;

            // If possible, get the NamedTypes for the type arguments to give better error
            // location.
            //
            // This is only possibly for the leaf generic binding and when a named type is
            // passed in.
            //
            if (RawType->IsNamedType() && !TypeIsParentBinding)
            {
                BCSYM_NamedType *NamedType = RawType->PNamedType();

                // In order for the type arguments to correspond to the leaf binding, get the
                // type arguments corresponding to the last name in the list of names in the
                // named type.
                //
                if (NamedType->GetArgumentsArray() &&
                    NamedType->GetArgumentsArray()[NamedType->GetNameCount() - 1])
                {
                    TypeArguments =
                        NamedType->GetArgumentsArray()[NamedType->GetNameCount() - 1]->m_Arguments;
                }
            }

            if (!TypeArguments)
            {
                TypeArguments = GenericBinding->GetArguments();

                // Dig through type arguments in this case because the named type if any might not
                // be specified at the use sites. This can happen under either of the following
                // situations:
                // - the binding is a cached binding shared by several use sites
                // - the binding corresponds to a generic alias - Bug VSWhidbey #167282
                //
                DigThroughTypeArguments = true;
            }

            VSASSERT(!ArgumentCount || TypeArguments, "Inconsistency in type arguments information!!!");

            for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                if (!VerifyAccessExposure(
                        Member,
                        DigThroughTypeArguments ?
                            ChaseThroughToType(TypeArguments[ArgumentIndex]) :
                            TypeArguments[ArgumentIndex],
                        ContainersWithAccessError,
                        TypesExposed))
                {
                    AreTypeArgumentsNotExposed = false;
                }
            }

            ChasedType = GenericBinding->GetGenericType();

            if (ChasedType->IsBad())
            {
                return IsTypeNotExposed && AreTypeArgumentsNotExposed;
            }

            DynamicArray<BCSYM *> **Dummy = NULL;

            // Are the parent bindings expanding the access of any type illegally ?
            //
            ParentGenericBinding = GenericBinding->GetParentBinding();
            if (ParentGenericBinding)
            {
                if (!VerifyAccessExposure(
                        Member,
                        ParentGenericBinding,
                        ContainersWithAccessErrorForParentBindings,
                        *Dummy,
                        true))      // true indicates that the binding being passed in is a parent binding.
                {
                    IsTypeNotExposed = false;
                    delete Dummy;
                }
            }

            if (TypeIsParentBinding)
            {
                SkipVerifyingMainType = true;
            }
        }
        else if (ChasedType->IsGenericParam())
        {
            return IsTypeNotExposed;
        }


        VSASSERT( ChasedType->IsContainer(), "What else can a chased type be besides container ?");

        BCSYM_Container *ContainerWithAccessError = NULL;

        if (!SkipVerifyingMainType &&
            !VerifyAccessExposure(
                Member,
                ChasedType->PContainer(),
                ParentGenericBinding,
                ContainerWithAccessError) &&
            !ContainersWithAccessErrorForParentBindings)
        {
            IsTypeNotExposed = false;
        }

        if (IsTypeNotExposed)
        {
            return IsTypeNotExposed && AreTypeArgumentsNotExposed;
        }

        // If parent bindings cause errors, then find the nearest enclosing container
        // outside which this type is being exposed invalidly
        //
        if (ContainersWithAccessErrorForParentBindings)
        {
            BCSYM_Container *EnclosingContainer;
            for (EnclosingContainer = Member->IsContainer() ?
                                                            Member->PContainer() :
                                                            Member->GetContainer();
                EnclosingContainer;
                EnclosingContainer = EnclosingContainer->GetContainer())
            {
                if (EnclosingContainer->IsSameContainer(ContainerWithAccessError))
                {
                    break;
                }

                if (ContainersWithAccessErrorForParentBindings)
                {
                    unsigned Index = 0;
                    for (Index = 0;
                        Index < ContainersWithAccessErrorForParentBindings->Count();
                        Index++)
                    {
                        if (EnclosingContainer->IsSameContainer(
                                ContainersWithAccessErrorForParentBindings->Element(Index)))
                        {
                            break;
                        }
                    }

                    if (Index < ContainersWithAccessErrorForParentBindings->Count())
                    {
                        break;
                    }
                }
            }

            ContainerWithAccessError = EnclosingContainer;
            delete ContainersWithAccessErrorForParentBindings;
        }


        // Allocate the dynamic arrays needed if not already allocated to hold
        // the types involved in this error.

        if (&ContainersWithAccessError)
        {
            if (!ContainersWithAccessError)
            {
                ContainersWithAccessError = new DynamicArray<BCSYM_Container *> ();
            }

            // Add the container outside which this type is being exposed illegally
            //
            ContainersWithAccessError->AddElement(ContainerWithAccessError);
        }

        if (&TypesExposed)
        {
            if (!TypesExposed)
            {
                TypesExposed = new DynamicArray<BCSYM *> ();
            }

            // Add the type being exposed
            //
            // Add the raw type where possible for better error information.
            //
            BCSYM *TypeExposed =
                RawType->IsNamedType() && !TypeIsParentBinding ?
                    RawType :
                    ChaseThroughToType(RawType);

            TypesExposed->AddElement(TypeExposed);
        }
    }

    return IsTypeNotExposed && AreTypeArgumentsNotExposed;
}


/*
    Return true if no error,
    else false if error due to access exposure
*/
bool
Bindable::VerifyAccessExposure
(
    BCSYM_NamedRoot *Member,
    BCSYM_Container *Type,
    BCSYM_GenericTypeBinding *TypeGenericBindingContext,
    BCSYM_Container *&ContainerWithAccessError
)
{
    // since the entire heirarchy is not built up in the debugger case,
    // there is no point proceeding any further.
    //
    VSASSERT(Member->GetImmediateParent() && Member->GetImmediateParent()->IsHash(),
                "Member should exist and be present in a hash!!!");

    ContainerWithAccessError = NULL;

    VSASSERT( !Member->IsPartialTypeAndHasMainType() &&
                !Type->IsPartialTypeAndHasMainType(),
                    "Partial types unexpected. Should not have been bound to!!!");

    VSASSERT( !Type->IsNamespace(),
                    "How can a namespace be a type!!!");

    // Short cut for intrinsic types
    //
    if (Type->IsIntrinsicType() || Type->IsObject())
    {
        return true;
    }

    if (MemberIsOrNestedInType(Member, Type))
    {
        return true;
    }

    // We're going to do a "fail-fast" here...
    // First test: "If we WERE within the assembly, and access didn't work, then fail-fast."
    // Next test: "Okay, if access-inside would work, then let's do the more accurate test."

    if (!VerifyAccessExposureWithinAssembly(
            Member,
            Type,
            TypeGenericBindingContext,
            ContainerWithAccessError))
    {
        return false;
    }

    return VerifyAccessExposureOutsideAssembly(
                Member,
                Type,
                TypeGenericBindingContext);
}


bool
Bindable::MemberIsOrNestedInType
(
    BCSYM_NamedRoot *Member,
    BCSYM_Container *Type
)
{
     VSASSERT( !Member->IsPartialTypeAndHasMainType() &&
                !Type->IsPartialTypeAndHasMainType(),
                    "Partial types unexpected. Should not have been bound to!!!");

    VSASSERT( !Type->IsNamespace(),
                    "How can a namespace be a type!!!");

    if (Member == Type)
    {
        return true;
    }

    if (Member->IsContainedBy(Type))
    {
        return true;
    }

    return false;
}

bool
Bindable::VerifyAccessExposureWithinAssembly
(
    BCSYM_NamedRoot *Member,
    BCSYM_Container *Type,
    BCSYM_GenericTypeBinding *TypeGenericBindingContext,
    BCSYM_Container *&ContainerWithAccessError
)
{
    const bool IsOutsideAssemblyContext = true;
    bool SeenThroughInheritance = false;

    // NOTE: If we're called here it doesn't mean that we ARE within the assembly.
    // It's merely asking the question, "If we were within the assembly, then can we verify access?"

    return
        VerifyAccessExposureHelper(
            Member,
            Type,
            TypeGenericBindingContext,
            ContainerWithAccessError,
            SeenThroughInheritance,
            !IsOutsideAssemblyContext); // Is not outside assembly
}

bool
Bindable::VerifyAccessExposureOutsideAssembly
(
    BCSYM_NamedRoot *Member,
    BCSYM_Container *Type,
    BCSYM_GenericTypeBinding *TypeGenericBindingContext
)
{
    ACCESS MemberAccessOutsideAssembly = GetEffectiveAccessOutsideAssembly(Member);

    VSASSERT( MemberAccessOutsideAssembly != ACCESS_Friend,
                    "How can the access be friend outside the assembly ?");


    if (MemberAccessOutsideAssembly == ACCESS_Private)
    {
        return true;
    }

    ACCESS TypeAccessOutsideAssembly = GetEffectiveAccessOutsideAssembly(Type);

    if (TypeAccessOutsideAssembly == ACCESS_Private)
    {
        return false;
    }

    if (MemberAccessOutsideAssembly == ACCESS_Public)
    {
        if (TypeAccessOutsideAssembly == ACCESS_Public)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    VSASSERT( MemberAccessOutsideAssembly == ACCESS_Protected,
                    "What else can the Member access be outside the assembly ?");

    if (TypeAccessOutsideAssembly == ACCESS_Public)
    {
        return true;
    }

    VSASSERT( TypeAccessOutsideAssembly == ACCESS_Protected,
                "What else can the Type access be outside the assembly ?");

    const bool IsOutsideAssemblyContext = true;
    BCSYM_Container *DummyArgument;
    bool TypeSeenThroughInheritance = false;

    VerifyAccessExposureHelper(
        Member,
        Type,
        TypeGenericBindingContext,
        DummyArgument,
        TypeSeenThroughInheritance,
        IsOutsideAssemblyContext);

    return TypeSeenThroughInheritance;
}


/*
four cases:
1: Member is not protected, non of its enclosing scopes are protected

2: Member is not protected, but some of its enclosing scopes are protected

3: Member is protected, non of its enclosing scopes are protected

4: Member is protected, some of its enclosing scopes are also protected

*/

/*
    Return true if no error,
    else false if error due to access exposure
*/
bool
Bindable::VerifyAccessExposureHelper
(
    BCSYM_NamedRoot *Member,
    BCSYM_Container *Type,
    BCSYM_GenericTypeBinding *TypeGenericBindingContext,
    BCSYM_Container *&ContainerWithAccessError,
    bool &SeenThroughInheritance,
    bool IsOutsideAssembly
)
{
    // NOTE: The IsOutsideAssembly flag doesn't say whether Type is inside or outside the current assembly.
    // It merely asks the hypothetical "Suppose Type were to be in/outside the current assembly...
    // then could we verify access?


    SeenThroughInheritance = false;
    BCSYM_Container *ExposingContainer = NULL;
        
    if (GetAccessInAssemblyContext(Member, IsOutsideAssembly) == ACCESS_Private)
    {
        // Bug #173443 - DevDiv Bugs: Continue checking for nested types because 
        // the fact that the enclosing type is private doesn't mean that it is OK to expose the nested type.
        if(!(Member->IsContainer() && IsTypeNestedIn(Type, Member->PContainer())))
        {
            return true;
        }

        ExposingContainer = Member->PContainer();
    }
    else
    {
        ACCESS StopAtAccess = ACCESS_Protected;

        ExposingContainer =
            FindEnclosingContainerWithGivenAccess(
                Member,
                StopAtAccess,
                IsOutsideAssembly);
    }

    BCSYM_Container *ParentOfExposingContainer;

    if (GetAccessInAssemblyContext(Member, IsOutsideAssembly) > ACCESS_Protected)
    {
        if (GetAccessInAssemblyContext(ExposingContainer, IsOutsideAssembly) != ACCESS_Protected)
        {
            // Case 1

            ParentOfExposingContainer = ExposingContainer->GetContainer();

            if (!IsTypeAccessible(Type, TypeGenericBindingContext, ParentOfExposingContainer))
            {
                ContainerWithAccessError = ParentOfExposingContainer;
                return false;
            }

            return true;

        }
        else    // GetAccessInAssemblyContext(ExposingContainer, IsOutsideAssembly) == ACCESS_Protected
        {
            // Case 2
            ParentOfExposingContainer = ExposingContainer->GetContainer();

            if (!IsTypeAccessible(Type, TypeGenericBindingContext, ParentOfExposingContainer))
            {
                ContainerWithAccessError = ParentOfExposingContainer;
                return false;
            }

            return VerifyAccessExposureHelper(
                        ExposingContainer,
                        Type,
                        TypeGenericBindingContext,
                        ContainerWithAccessError,
                        SeenThroughInheritance,
                        IsOutsideAssembly);
        }
    }
    else    // GetAccessInAssemblyContext(Member, IsOutsideAssembly) == ACCESS_Protected
    {
        if (GetAccessInAssemblyContext(ExposingContainer, IsOutsideAssembly) != ACCESS_Protected)
        {
            // Case 3

            if (CanBeAccessedThroughInheritance(Type, TypeGenericBindingContext, Member->GetContainer(), IsOutsideAssembly))
            {
                SeenThroughInheritance = true;
                return true;
            }

            ParentOfExposingContainer = ExposingContainer->GetContainer();

            if (IsTypeAccessible(Type, TypeGenericBindingContext, ParentOfExposingContainer))
            {
                return true;
            }

            ContainerWithAccessError = ParentOfExposingContainer;
            return false;
        }
        else    // GetAccessInAssemblyContext(ExposingContainer, IsOutsideAssembly) == ACCESS_Protected
        {
            // Case 4

            if (CanBeAccessedThroughInheritance(Type, TypeGenericBindingContext, Member->GetContainer(), IsOutsideAssembly))
            {
                SeenThroughInheritance = true;
                return true;
            }

            ParentOfExposingContainer = ExposingContainer->GetContainer();

            if (!IsTypeAccessible(Type, TypeGenericBindingContext, ParentOfExposingContainer))
            {
                ContainerWithAccessError = ParentOfExposingContainer;
                return false;
            }

            return VerifyAccessExposureHelper(
                        ExposingContainer,
                        Type,
                        TypeGenericBindingContext,
                        ContainerWithAccessError,
                        SeenThroughInheritance,
                        IsOutsideAssembly);

        }
    }
}

/*
    This function finds the inner most enclosing scope whose Access
    is lesser than or equal to the given access "StopAtAccess".

[in] Member - for which the enclosing scope has to be found
[in] StopAtAccess - the enclosing scope's access has to be lesser than
    It returns the actual access level at which it stopped.

*/
BCSYM_Container *
Bindable::FindEnclosingContainerWithGivenAccess
(
    BCSYM_NamedRoot *Member,
    ACCESS StopAtAccess,
    bool IsOutsideAssembly
)
{

    VSASSERT( !Member->IsNamespace(),
                    "How can a Member be a namespace ?");

    BCSYM_Container *EnclosingContainer;

    if (Member->IsContainer())
    {
        EnclosingContainer = Member->PContainer();

        if (!EnclosingContainer->GetContainer()->IsNamespace())
        {
            EnclosingContainer = EnclosingContainer->GetContainer();
        }
    }
    else
    {
        EnclosingContainer = Member->GetContainer();
    }

    for(;
        EnclosingContainer && !EnclosingContainer->GetContainer()->IsNamespace();
        EnclosingContainer = EnclosingContainer->GetContainer())
    {
        ACCESS EnclosingContainerAccess = GetAccessInAssemblyContext(EnclosingContainer, IsOutsideAssembly);

        if (EnclosingContainerAccess <= StopAtAccess)
        {
            break;
        }
    }

    VSASSERT( !EnclosingContainer->IsNamespace(),
                    "Should never return namespace here!!!");

    return EnclosingContainer;
}

bool
Bindable::CanBeAccessedThroughInheritance
(
    BCSYM_NamedRoot *Type,
    BCSYM_GenericTypeBinding *TypeGenericBindingContext,
    BCSYM_Container *Container,
    bool IsOutsideAssembly
)
{
    // NOTE: The IsOutsideAssembly flag doesn't say whether Type is inside or outside the current assembly.
    // It merely asks the hypothetical "Suppose Type were to be in/outside the current assembly...
    // then could it be accessed through inheritance?

    if (GetAccessInAssemblyContext(Type, IsOutsideAssembly) == ACCESS_Private)
    {
        return false;
    }

    BCSYM *ContainerOfType =
        TypeGenericBindingContext ?
            (BCSYM *)TypeGenericBindingContext :
            Type->GetContainer();

    if (!ContainerOfType || ContainerOfType->IsNamespace())
    {
        return false;
    }

    // 

    Symbols
        SymbolAllocator(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    BCSYM *Context;

    if (ContainerOfType->IsGenericBinding() &&
        IsGenericOrHasGenericParent(Container))
    {
        Context =
            SynthesizeOpenGenericBinding(
                Container,
                SymbolAllocator);
    }
    else
    {
        Context = Container;
    }

    // 

    if (TypeHelpers::IsOrInheritsFrom(Context, ContainerOfType, SymbolAllocator, false, NULL))
    {
        return true;
    }
    else if (GetAccessInAssemblyContext(Type, IsOutsideAssembly) != ACCESS_Protected)
    {
        return
            CanBeAccessedThroughInheritance(
                ContainerOfType->PContainer(),
                TypeGenericBindingContext ?
                    TypeGenericBindingContext->GetParentBinding() :
                    NULL,
                Container,
                IsOutsideAssembly);
    }

    return false;
}

ACCESS
Bindable::GetAccessInAssemblyContext
(
    BCSYM_NamedRoot *Member,
    bool IsOutsideAssemblyContext
)
{
    ACCESS AccessOfMember = Member->GetAccess();

    if (IsOutsideAssemblyContext)
    {
#pragma prefast(suppress: 26010 26011, "If access is outside of the array limits, then the whole symbol is busted")
        AccessOfMember = MapAccessToAccessOutsideAssembly[AccessOfMember];
    }

    return AccessOfMember;
}

ACCESS
Bindable::GetEffectiveAccessOutsideAssembly
(
    BCSYM_NamedRoot *Member
)
{
#pragma prefast(suppress: 26010 26011, "If access is outside of the array limits, then the whole symbol is busted")
    ACCESS EffectiveAccess = MapAccessToAccessOutsideAssembly[Member->GetAccess()];

    if (EffectiveAccess == ACCESS_Private)
    {
        return EffectiveAccess;
    }

    for (BCSYM_Container *EnclosingContainer = Member->GetContainer();
         EnclosingContainer;
         EnclosingContainer = EnclosingContainer->GetContainer())
    {
        ACCESS AccessOfContainer =
#pragma prefast(suppress: 26010 26011, "If access is outside of the array limits, then the whole symbol is busted")
            MapAccessToAccessOutsideAssembly[EnclosingContainer->GetAccess()];


        if (AccessOfContainer < EffectiveAccess)
        {
            EffectiveAccess = AccessOfContainer;
        }

        if (EffectiveAccess == ACCESS_Private)
        {
            return EffectiveAccess;
        }
    }

    return EffectiveAccess;
}

bool
Bindable::IsTypeAccessible
(
    BCSYM_NamedRoot *Type,
    BCSYM_GenericTypeBinding *TypeGenericBindingContext,
    BCSYM_Container *ContainerContext
)
{
    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    BCSYM_GenericTypeBinding *BindingContextForContainerOfType = TypeGenericBindingContext;

    for (BCSYM_NamedRoot *ContainerOfType = Type;
         ContainerOfType;
         ContainerOfType =
                ContainerOfType->GetContainer())
    {
        if (ContainerContext->IsSameContainer(ContainerOfType) ||
            ContainerOfType->IsNamespace())
        {
            return true;
        }

        if (!Semantics::IsAccessible(
                ContainerOfType,
                BindingContextForContainerOfType,
                ContainerContext,
                NULL,
                SymbolFactory,
                CurrentCompilerHost()))
        {
            return false;
        }

        BindingContextForContainerOfType =
            BindingContextForContainerOfType ?
                BindingContextForContainerOfType->GetParentBinding() :
                NULL;
    }

    return true;
}

/*
    Return true if no error,
    else false if error due to access exposure
*/
bool
Bindable::VerifyAccessExposureOfBaseClassOrInterface
(
    BCSYM_Container *ClassOrInterface,
    BCSYM *RawBase
)
{
    VSASSERT( ClassOrInterface->IsClass() || ClassOrInterface->IsInterface(),
                    "Expected class or interface!!!");

    DynamicArray<BCSYM_Container *> *ContainersWithAccessError = NULL;
    DynamicArray<BCSYM *> *TypesExposed = NULL;

    VerifyAccessExposure(
        ClassOrInterface,
        RawBase,
        ContainersWithAccessError,
        TypesExposed);

    if (TypesExposed)
    {
        VSASSERT( ContainersWithAccessError, "Inconsistency in access exposure data!!!");

        VSASSERT( TypesExposed->Count() &&
                  (TypesExposed->Count() == ContainersWithAccessError->Count()),
                        "Inconsistency in access exposure data!!!");

        BCSYM *ChasedBase = ChaseThroughToType(RawBase);

        for (unsigned Index = 0; Index < TypesExposed->Count(); Index++)
        {
            BCSYM_Container *ContainerAtWhichAccessErrorOccurs = ContainersWithAccessError->Element(Index);

            BCSYM *ExposedRawType = TypesExposed->Element(Index);
            BCSYM *ExposedChasedType = ChaseThroughToType(ExposedRawType);

            BCSYM *SymbolToReportErrorOn =
                ExposedRawType->IsNamedType() ?
                    ExposedRawType :
                    RawBase;

            VSASSERT( ExposedChasedType, "NULL type unexpected!!!");

            if (ContainerAtWhichAccessErrorOccurs)
            {
                if (ExposedChasedType == ChasedBase)
                {
                    // "'|1' cannot inherit from |2 '|3' because it expands the access of the base |2 to |4 '|5'."
                    //

                    VSASSERT(SymbolToReportErrorOn->IsNamedType(), "Bad error symbol !!!");

                    ReportErrorOnSymbol(
                        ERRID_InheritanceAccessMismatch5,
                        CurrentErrorLog(SymbolToReportErrorOn->PNamedType()),
                        SymbolToReportErrorOn,
                        ClassOrInterface->GetName(),
                        StringOfSymbol(CurrentCompilerInstance(), ChasedBase),
                        GetQualifiedErrorName(ChasedBase),
                        StringOfSymbol(CurrentCompilerInstance(), ContainerAtWhichAccessErrorOccurs),
                        GetQualifiedErrorName(ContainerAtWhichAccessErrorOccurs));
                }
                else // generic type argument is being exposed
                {
                    // "'|1' cannot inherit from |2 '|3' because it expands the access of type '|4' to |5 '|6'."
                    //

                    VSASSERT(SymbolToReportErrorOn->IsNamedType(), "Bad error symbol !!!");

                    ReportErrorOnSymbol(
                        ERRID_InheritsTypeArgAccessMismatch7,
                        CurrentErrorLog(SymbolToReportErrorOn->PNamedType()),
                        SymbolToReportErrorOn,
                        ClassOrInterface->GetName(),
                        StringOfSymbol(CurrentCompilerInstance(), ChasedBase),
                        GetQualifiedErrorName(ChasedBase),
                        GetQualifiedErrorName(ExposedChasedType),
                        StringOfSymbol(CurrentCompilerInstance(), ContainerAtWhichAccessErrorOccurs),
                        GetQualifiedErrorName(ContainerAtWhichAccessErrorOccurs));
                }
            }
            else
            {
                if (ExposedChasedType == ChasedBase)
                {
                    // "'|1' cannot inherit from |2 '|3' because it expands the access of the base |2 outside the assembly."
                    //

                    VSASSERT(SymbolToReportErrorOn->IsNamedType(), "Bad error symbol !!!");

                    ReportErrorOnSymbol(
                        ERRID_InheritanceAccessMismatchOutside3,
                        CurrentErrorLog(SymbolToReportErrorOn->PNamedType()),
                        SymbolToReportErrorOn,
                        ClassOrInterface->GetName(),
                        StringOfSymbol(CurrentCompilerInstance(), ChasedBase),
                        GetQualifiedErrorName(ChasedBase));
                }
                else // generic type argument is being exposed
                {
                    // "'|1' cannot inherit from |2 '|3' because it expands the access of type '|4' outside the assembly."
                    //

                    VSASSERT(SymbolToReportErrorOn->IsNamedType(), "Bad error symbol !!!");

                    ReportErrorOnSymbol(
                        ERRID_InheritsTypeArgAccessMismatchOutside5,
                        CurrentErrorLog(SymbolToReportErrorOn->PNamedType()),
                        SymbolToReportErrorOn,
                        ClassOrInterface->GetName(),
                        StringOfSymbol(CurrentCompilerInstance(), ChasedBase),
                        GetQualifiedErrorName(ChasedBase),
                        GetQualifiedErrorName(ExposedChasedType));
                }
            }
        }

        delete TypesExposed;
        delete ContainersWithAccessError;

        return false;
    }

    return true;
}

/*****************************************************************************
;VerifyAccessExposureForMember

Determine if a type is being illegally exposed from a container, e.g. a public
member is defined in a public class that has a private or friend type.  A friend
UDT may embed a private one, etc.  Ensure that the types of a member are not
more restrictive than their container or the member, whichever is more restrictive.
*****************************************************************************/
void
Bindable::VerifyAccessExposureForMember
(
    BCSYM_Member    *Member // [in] the member we are checking to see if it exposes a more restrictive type
)
{
    BCSYM_Container *Container = CurrentContainer();

    // Test the member against its container, e.g. a class member against its class,
    // a UDT member against its record, the return val of a func against the func access

    // no point doing access decl semantics on the guts of an Enum
    if (Container->IsEnum())
    {
        return;
    }

    if (Member->IsProc())
    {
        VerifyAccessExposureForGenericParameters(Member->PProc());
    }

    // Control here how we log errors on delegates that expose types so
    // we don't get multiple errors
    //
    // The problem with Delegates is that what we get is a class symbol with several
    // methods on it - more than one of which duplicates the signature of the Delegate
    // declaration. So we would get this error several times but for the bellow 'if' at the
    // entrance of this function that gets us out of here unless we are looking at
    // "Invoke".  More, we want to log a meaningful error on the Delegate declaration
    // instead of an error on the Invoke method which the user can't see anyway if we have
    // a problem with the Delegate (e.g. Delegate Function foo(x as RestrictedType) as RestrictedType)
    //
    if (Container->IsDelegate() &&
        !StringPool::IsEqual(
            Member->GetName(),
            STRING_CONST(CurrentCompilerInstance(), DelegateInvoke)))
    {
        return;
    }

    // the type behind Member, e.g. the return type of the proc, the type of a UDT member, etc.
    //
    BCSYM *TypeBehindMember =
        Member->IsEventDecl() ?
            Member->PEventDecl()->GetRawDelegate() :
            Member->GetRawType();

    VerifyAccessExposureForMemberType(Member, TypeBehindMember);

    // If we are looking at a procedure, make sure none of the parameter types are being exposed illegally
    if (!Member->IsHidden() &&
        Member->IsProc() &&
        // If this is an "Event as Delegate", the Event took on the signature of the delegate.
        // Don't do signature access work - we logged the appropriate error when processing the
        // delegate declaration #228546
        !(Member->IsEventDecl() &&
            Member->PEventDecl()->AreParametersObtainedFromDelegate()))
    {

        BCITER_Parameters ParamIter;
        ParamIter.Init(Member->PProc());
        BCSYM_Param *Param = ParamIter.PparamNext();

        while (Param)
        {
            VerifyAccessExposureForParameterType(
                Member,
                Param,
                Param->GetRawType());

            Param = ParamIter.PparamNext();
        }
    }
}

void
Bindable::VerifyAccessExposureForGenericParameters
(
    BCSYM_Container *PossibleGenericContainer
)
{
    VerifyAccessExposureForGenericParametersHelper(
        PossibleGenericContainer,
        PossibleGenericContainer->GetFirstGenericParam());
}

void
Bindable::VerifyAccessExposureForGenericParameters
(
    BCSYM_Proc *PossibleGenericProc
)
{
    VerifyAccessExposureForGenericParametersHelper(
        PossibleGenericProc,
        PossibleGenericProc->GetFirstGenericParam());
}

void
Bindable::VerifyAccessExposureForGenericParametersHelper
(
    BCSYM_NamedRoot *ProcOrContainer,
    BCSYM_GenericParam *ListOfGenericParams
)
{
    VSASSERT( ProcOrContainer->IsContainer() || ProcOrContainer->IsProc(),
                    "Unexpected generic symbol kind!!!");

    for (BCSYM_GenericParam *GenericParam = ListOfGenericParams;
         GenericParam;
         GenericParam = GenericParam->GetNextParam())
    {
        for (BCSYM_GenericTypeConstraint *Constraint = GenericParam->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            VerifyAccessExposureForMemberType(ProcOrContainer, Constraint->GetRawType());
        }
    }
}

void
Bindable::VerifyAccessExposureForMemberType
(
    BCSYM_NamedRoot *Member,        // [in] the member we are checking to see if it exposes a more restrictive type
    BCSYM *TypeBehindMember         // [in] the type we are checking to see if it exposes a more restrictive type
)
{
    DynamicArray<BCSYM_Container *> *ContainersWithAccessError = NULL;
    DynamicArray<BCSYM *> *TypesExposed = NULL;

    VerifyAccessExposure(
        Member,
        TypeBehindMember,
        ContainersWithAccessError,
        TypesExposed);

    if (TypesExposed)
    {
        VSASSERT( ContainersWithAccessError, "Inconsistency in access exposure data!!!");

        VSASSERT( TypesExposed->Count() &&
                  (TypesExposed->Count() == ContainersWithAccessError->Count()),
                        "Inconsistency in access exposure data!!!");

        BCSYM *SymbolToReportErrorOn = NULL;
        bool AccessMismatchWithAccessOfImplementedEventDelegate = false;

        if (Member->IsEventDecl() &&
            Member->PEventDecl()->IsDelegateFromImplements())
        {
            SymbolToReportErrorOn = Member;
            AccessMismatchWithAccessOfImplementedEventDelegate = true;
        }

        BCSYM_Container *Container = CurrentContainer();

        // This is needed when we find a error on the Delegate Invoke.
        // When this happens, the error has to be report for the delegate
        // and not the invoke which is not in user code.
        //
        STRING *MemberNameToReportInError =
                Container->IsDelegate() ?
                    Container->GetName() :
                    Member->GetName();

        BCSYM *ChasedType = ChaseThroughToType(TypeBehindMember);

        for (unsigned Index = 0; Index < TypesExposed->Count(); Index++)
        {
            BCSYM_Container *ContainerAtWhichAccessErrorOccurs = ContainersWithAccessError->Element(Index);

            BCSYM *ExposedRawType = TypesExposed->Element(Index);
            BCSYM *ExposedChasedType = ChaseThroughToType(ExposedRawType);

            VSASSERT( ExposedChasedType, "NULL type unexpected!!!");

            if (!AccessMismatchWithAccessOfImplementedEventDelegate)
            {
                SymbolToReportErrorOn =
                    ExposedRawType->IsNamedType() ?
                        ExposedRawType :
                        TypeBehindMember;
            }

            if (ContainerAtWhichAccessErrorOccurs)
            {
                if (AccessMismatchWithAccessOfImplementedEventDelegate)
                {
                    if (ChasedType == ExposedChasedType)
                    {
                        // "'|1' cannot expose the underlying delegate type '|2' of the event it is implementing to |3 '|4' through |5 '|6'."
                        //
                        ReportErrorOnSymbol(
                            ERRID_AccessMismatchImplementedEvent6,
                            CurrentErrorLog(SymbolToReportErrorOn, Member),
                            SymbolToReportErrorOn,
                            MemberNameToReportInError,
                            GetQualifiedErrorName(ExposedChasedType),
                            StringOfSymbol(CurrentCompilerInstance(), ContainerAtWhichAccessErrorOccurs),
                            GetQualifiedErrorName(ContainerAtWhichAccessErrorOccurs),
                            StringOfSymbol(CurrentCompilerInstance(), Container),
                            Container->GetName());
                    }
                    else
                    {
                        // 


                        // "'|1' cannot expose type '|2' used in the underlying delegate type '|3' of the event it is implementing to |4 '|5' through |6 '|7'."
                        //
                        ReportErrorOnSymbol(
                            ERRID_AccessMismatchTypeArgImplEvent7,
                            CurrentErrorLog(SymbolToReportErrorOn, Member),
                            SymbolToReportErrorOn,
                            MemberNameToReportInError,
                            GetQualifiedErrorName(ExposedChasedType),
                            GetQualifiedErrorName(ChasedType),
                            StringOfSymbol(CurrentCompilerInstance(), ContainerAtWhichAccessErrorOccurs),
                            GetQualifiedErrorName(ContainerAtWhichAccessErrorOccurs),
                            StringOfSymbol(CurrentCompilerInstance(), Container),
                            Container->GetName());
                    }
                }
                else
                {
                    ReportErrorOnSymbol(
                        ERRID_AccessMismatch6,
                        CurrentErrorLog(SymbolToReportErrorOn, Member),
                        SymbolToReportErrorOn,
                        MemberNameToReportInError,
                        GetQualifiedErrorName(ExposedChasedType),
                        StringOfSymbol(CurrentCompilerInstance(), ContainerAtWhichAccessErrorOccurs),
                        GetQualifiedErrorName(ContainerAtWhichAccessErrorOccurs),
                        StringOfSymbol(CurrentCompilerInstance(), Container),
                        Container->GetName());
                }
            }
            else
            {
                if (AccessMismatchWithAccessOfImplementedEventDelegate)
                {
                    if (ChasedType == ExposedChasedType)
                    {
                        // "'|1' cannot expose the underlying delegate type '|2' of the event it is implementing outside the project through |3 '|4'."
                        //
                        ReportErrorOnSymbol(
                                ERRID_AccessMismatchImplementedEvent4,
                                CurrentErrorLog(SymbolToReportErrorOn, Member),
                                SymbolToReportErrorOn,
                                MemberNameToReportInError,
                                GetQualifiedErrorName(ExposedChasedType),
                                StringOfSymbol(CurrentCompilerInstance(), Container),
                                Container->GetName());
                    }
                    else
                    {
                        // 

                        // "'|1' cannot expose type '|2' used in the underlying delegate type '|3' of the event it is implementing outside the project through |4 '|5'."
                        //
                        ReportErrorOnSymbol(
                            ERRID_AccessMismatchTypeArgImplEvent5,
                            CurrentErrorLog(SymbolToReportErrorOn, Member),
                            SymbolToReportErrorOn,
                            MemberNameToReportInError,
                            GetQualifiedErrorName(ExposedChasedType),
                            GetQualifiedErrorName(ChasedType),
                            StringOfSymbol(CurrentCompilerInstance(), Container),
                            Container->GetName());
                    }
                }
                else
                {
                    // "'|1' cannot expose type '|2' outside the project through |3 '|4'."
                    //
                    ReportErrorOnSymbol(
                        ERRID_AccessMismatchOutsideAssembly4,
                        CurrentErrorLog(SymbolToReportErrorOn, Member),
                        SymbolToReportErrorOn,
                        MemberNameToReportInError,
                        GetQualifiedErrorName(ExposedChasedType),
                        StringOfSymbol(CurrentCompilerInstance(), Container),
                        Container->GetName());
                }
            }
        }

        delete TypesExposed;
        delete ContainersWithAccessError;
    }

}

void
Bindable::VerifyAccessExposureForParameterType
(
    BCSYM_Member *Member,   // [in] the member we are checking to see if it exposes a more restrictive type
    BCSYM_Param *Param,        // [in] the param we are checking
    BCSYM *TypeBehindParam     // [in] the type we are checking
)
{
    DynamicArray<BCSYM_Container *> *ContainersWithAccessError = NULL;
    DynamicArray<BCSYM *> *TypesExposed = NULL;

    VerifyAccessExposure(
        Member,
        TypeBehindParam,
        ContainersWithAccessError,
        TypesExposed);

    if (TypesExposed)
    {
        VSASSERT( ContainersWithAccessError, "Inconsistency in access exposure data!!!");

        VSASSERT( TypesExposed->Count() &&
                  (TypesExposed->Count() == ContainersWithAccessError->Count()),
                        "Inconsistency in access exposure data!!!");

        for (unsigned Index = 0; Index < TypesExposed->Count(); Index++)
        {
            BCSYM_Container *ContainerAtWhichAccessErrorOccurs = ContainersWithAccessError->Element(Index);

            BCSYM *ExposedRawType = TypesExposed->Element(Index);
            BCSYM *ExposedChasedType = ChaseThroughToType(ExposedRawType);

            VSASSERT( ExposedChasedType, "NULL type unexpected!!!");

            BCSYM *SymbolToReportErrorOn =
                ExposedRawType->IsNamedType() ?
                    ExposedRawType :
                    TypeBehindParam;

            BCSYM_Container *Container = CurrentContainer();

            if (ContainerAtWhichAccessErrorOccurs)
            {
                // "'|1' cannot expose type '|2' to the scope of |3 '|4' through |5 '|6'."
                //
                ReportErrorOnSymbol(
                    ERRID_AccessMismatch6,
                    CurrentErrorLog(Member),
                    SymbolToReportErrorOn,
                    Param->GetName(),
                    GetQualifiedErrorName(ExposedChasedType),
                    StringOfSymbol(CurrentCompilerInstance(), ContainerAtWhichAccessErrorOccurs),
                    GetQualifiedErrorName(ContainerAtWhichAccessErrorOccurs),
                    StringOfSymbol(CurrentCompilerInstance(), Container),
                    Container->GetName());
            }
            else
            {
                // "'|1' cannot expose type '|2' outside the project through |3 '|4'."
                //
                ReportErrorOnSymbol(
                    ERRID_AccessMismatchOutsideAssembly4,
                    CurrentErrorLog(Member),
                    SymbolToReportErrorOn,
                    Param->GetName(),
                    GetQualifiedErrorName(ExposedChasedType),
                    StringOfSymbol(CurrentCompilerInstance(), Container),
                    Container->GetName());
            }

            Member->PNamedRoot()->SetIsBad();
        }

        delete TypesExposed;
        delete ContainersWithAccessError;
    }
}
