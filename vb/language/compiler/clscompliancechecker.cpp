//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Checks for usage of non-CLSCompliant types in public exposed entities.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void CLSComplianceChecker::VerifyCLSCompliance(SourceFile * SourceFile)
{
    CLSComplianceChecker Checker(SourceFile);

    Checker.VerifyCLSComplianceForContainerAndNestedTypes(
        SourceFile->GetUnnamedNamespace());
}

void CLSComplianceChecker::VerifyNameofProjectRootNamespaceIsCLSCompliant(
    CompilerProject * Project,
    ErrorTable * ErrorLog)
{
    VSASSERT( Project,
                "NULL Project Unexpected!!!");

    VSASSERT( ErrorLog,
                "NULL ErrorLog Unexpected!!!");

    if (!Project->ProjectClaimsCLSCompliance())
    {
        return;
    }

    STRING *RootNamespace = Project->GetDefaultNamespace();

    if (!RootNamespace)
    {
        return;
    }

    Compiler *CompilerInstance = Project->GetCompiler();

    VSASSERT( CompilerInstance,
                "NULL compiler unexpected!!!");

    STRING **QualifiedNames = NULL;
    ULONG NumberOfNames = 0;
    NorlsAllocator NameStorage(NORLSLOC);

    BreakUpQualifiedName(CompilerInstance, &QualifiedNames, &NameStorage, &NumberOfNames, RootNamespace, DOT_DELIMITER);

    for(ULONG Index = 0; Index < NumberOfNames; Index++)
    {
        if (IsNameCLSCompliant(QualifiedNames[Index]))
        {
            continue;
        }

        if (NumberOfNames == 1)
        {
            // Root namespace '|1' is not CLS compliant.
            //
            ErrorLog->CreateError(
                WRNID_RootNamespaceNotCLSCompliant1,
                NULL,
                RootNamespace);
        }
        else
        {
            // Name '|1' in the root namespace '|2' is not CLS compliant.
            //
            ErrorLog->CreateError(
                WRNID_RootNamespaceNotCLSCompliant2,
                NULL,
                QualifiedNames[Index],
                RootNamespace);
        }
    }
}

void CLSComplianceChecker::VerifyCLSComplianceForContainerAndNestedTypes(BCSYM_Container * Container)
{
    // Partial types are processed when their main type is processed
    //
    if (Container->IsPartialTypeAndHasMainType())
    {
        return;
    }

    ACCESS Access = Container->GetAccess();

    // If the type is not accessible outside the assembly, then
    // don't bother checking for CLS compliance
    //
    if (!IsAccessibleOutsideAssembly(Container))
    {
        return;
    }

    // If the type is a synthetic type, then don't do any checking
    // eg: implicit delegate generated for event
    //
    if (IsSyntheticMember(Container))
    {
        return;
    }

    VerifyCLSComplianceForContainer(Container);

    BCSYM_Container *CurrentContainer = Container;
    do
    {
        for(BCSYM_Container *NestedContainer = CurrentContainer->GetNestedTypes();
            NestedContainer;
            NestedContainer = NestedContainer->GetNextNestedType())
        {
            VerifyCLSComplianceForContainerAndNestedTypes(NestedContainer);
        }

    } while(CurrentContainer = CurrentContainer->GetNextPartialType());
}

void CLSComplianceChecker::VerifyCLSComplianceForContainer(BCSYM_Container * Container)
{
    if (!Container->IsCLSCompliant())
    {
        VerifyThatMembersAreNotMarkedCLSCompliant(Container);
        return;
    }

    // If the container is part of the rootnamespace, then skip this check
    // and instead report it later in the project error table when checking
    // the project's rootnamespace.
    //
    if (!ContainerIsPartOfRootNamespace(Container))
    {
        VerifyNameIsCLSCompliant(Container);
    }

    VerifyBasesForCLSCompliance(Container);

    VerifyMembersForCLSCompliance(Container);

    VerifyEnumUnderlyingTypeForCLSCompliance(Container);
}

void CLSComplianceChecker::VerifyThatMembersAreNotMarkedCLSCompliant(BCSYM_Container * Container)
{
    VSASSERT( !Container->IsCLSCompliant(),
                    "CLS compliant container unexpected!!!");

    // Members nested inside Non CLS Compliant Types cannot be
    // marked as CLS Compliant.
    //
    // For Top Level types, it is okay to be marked as CLS Compliant
    // even though inside a Non-CLS Compliant assembly.
    //
    if (Container->IsNamespace())
    {
        return;
    }

    BCITER_CHILD Members(Container);
    while (BCSYM_NamedRoot *Member = Members.GetNext())
    {
        if (Member->IsBad())
        {
            continue;
        }

        // If the member is not accessible outside the assembly, then
        // don't bother checking for CLS compliance
        //
        if (!IsAccessibleOutsideAssembly(Member))
        {
            continue;
        }

        // Need to do this so that we handle the operators. Operators
        // do not by themselves show up in this (bindable hash) hash
        // we are looking in, so we will find them through their functions
        // instead.
        //
        if (Member->IsProc() &&
            Member->PProc()->IsUserDefinedOperatorMethod())
        {
            Member = Member->PProc()->GetAssociatedOperatorDef();
        }

        // We don't want to report errors for synthetic members.
        // Eg: withevents property, the withevents variable _prop, etc.
        // If we don't skip errors for these, we would end up reporting
        // the same errors on both these and the original symbols from
        // which they were synthesized resulting in duplicate errors.
        //
        if (IsSyntheticMember(Member))
        {
            continue;
        }

        VerifyMemberNotMarkedCLSCompliant(Member);
    }
}

void CLSComplianceChecker::VerifyMemberNotMarkedCLSCompliant(BCSYM_NamedRoot * Member)
{
    // If explicitly marked as CLSCompliant(true), then report error
    // If the CLSCompliant attribute is inherited, then don't report
    // the error
    //
    if (IsExplicitlyMarkedCLSCompliant(Member))
    {
        // |1 '|2' cannot be marked CLS compliant because its containing type '|3' is not CLS compliant.

        ReportErrorOnSymbol(

            WRNID_CLSMemberInNonCLSType3,
            Member,
            StringOfSymbol(m_Compiler, Member),
            Member->GetName(),
            Member->GetContainer()->GetName());
    }

    if (Member->IsEventDecl() &&
        Member->PEventDecl()->IsBlockEvent())
    {
        BCSYM_EventDecl *Event = Member->PEventDecl();

        if (Event->GetProcAdd() &&
            !Event->GetProcAdd()->IsBad() &&
            IsExplicitlyMarkedCLSCompliant(Event->GetProcAdd()))
        {
            // '|1' method for event '|2' cannot be marked CLS compliant because its containing type '|3' is not CLS compliant.

            ReportErrorOnSymbol(
                WRNID_CLSEventMethodInNonCLSType3,
                Event->GetProcAdd(),
                m_Compiler->TokenToString(tkADDHANDLER),
                Member->GetName(),
                Member->GetContainer()->GetName());
        }

        if (Event->GetProcRemove() &&
            !Event->GetProcRemove()->IsBad() &&
            IsExplicitlyMarkedCLSCompliant(Event->GetProcRemove()))
        {
            // '|1' method for event '|2' cannot be marked CLS compliant because its containing type '|3' is not CLS compliant.

            ReportErrorOnSymbol(
                WRNID_CLSEventMethodInNonCLSType3,
                Event->GetProcRemove(),
                m_Compiler->TokenToString(tkREMOVEHANDLER),
                Member->GetName(),
                Member->GetContainer()->GetName());
        }

        // The RaiseEvent method is not checked because it is always
        // private and only members visible outside the assembly are
        // checked for CLS compliance.
    }

}

bool CLSComplianceChecker::ContainerIsPartOfRootNamespace(BCSYM_Container * Container)
{
    if (!Container->IsNamespace())
    {
        return false;
    }

    SourceFile *ContainingSourceFile = Container->GetSourceFile();

    VSASSERT( ContainingSourceFile,
                "Non source container unexpected!!!");

    // RootNamespace can be NULL for non-existent files specified in project files
    //
    BCSYM_Namespace *RootNamespace =
        ContainingSourceFile->GetRootNamespace() ?
            ContainingSourceFile->GetRootNamespace() :
            ContainingSourceFile->GetUnnamedNamespace();

    VSASSERT( RootNamespace,
                "NULL rootnamespace unexpected!!!");

    for(BCSYM_Container *NextContaineraboveRootNamespace = RootNamespace;
        NextContaineraboveRootNamespace;
        NextContaineraboveRootNamespace = NextContaineraboveRootNamespace->GetContainer())
    {
        if (NextContaineraboveRootNamespace == Container)
        {
            return true;
        }

        VSASSERT( !NextContaineraboveRootNamespace->IsSameContainer(Container),
                    "How can there be multiple instances per file for namespaces above the rootnamespace ?");
    }

    return false;
}

void CLSComplianceChecker::VerifyNameIsCLSCompliant(BCSYM_NamedRoot * NamedEntity)
{
    STRING *Name = NamedEntity->GetName();

    if (!IsNameCLSCompliant(Name))
    {
        ReportErrorOnSymbol(
            WRNID_NameNotCLSCompliant1,
            NamedEntity,
            Name);
    }
}

bool CLSComplianceChecker::IsNameCLSCompliant(_In_opt_z_ STRING * Name)
{
    if (Name &&
        Name[0] == L'_')
    {
        return false;
    }

    return true;
}

bool CLSComplianceChecker::IsSyntheticMember(BCSYM_NamedRoot * Member)
{
    return
        Member->IsSyntheticMethod() ||
        Bindable::IsSynthetic(Member);
}

bool CLSComplianceChecker::IsExplicitlyMarkedCLSCompliant(BCSYM_NamedRoot * Member)
{
    bool IsCLSCompliant, IsInherited;

    return
        Member->GetPWellKnownAttrVals()->GetCLSCompliantData(&IsCLSCompliant, &IsInherited) &&
            IsCLSCompliant &&
            !IsInherited;
}

bool CLSComplianceChecker::IsExplicitlyMarkedNonCLSCompliant(BCSYM_NamedRoot * Member)
{
    bool IsCLSCompliant, IsInherited;

    return
        Member->GetPWellKnownAttrVals()->GetCLSCompliantData(&IsCLSCompliant, &IsInherited) &&
        !IsCLSCompliant &&
        !IsInherited;
}

void CLSComplianceChecker::VerifyBasesForCLSCompliance(BCSYM_Container * Container)
{
    VSASSERT( Container->IsCLSCompliant(),
                    "Non-CLS compliant container unexpected!!!");

    if (Container->IsClass())
    {
        BCSYM *RawBaseType = Container->PClass()->GetRawBase();

        // Report an error if this inherited class is not compliant.
        //
        if (!IsTypeCLSCompliant(RawBaseType, Container))
        {
            ReportErrorOnSymbol(
                WRNID_BaseClassNotCLSCompliant2,
                Container,
                Container->GetName(),
                RawBaseType->ChaseToType()->GetErrorName(m_Compiler));
        }
    }
    else if (Container->IsInterface())
    {
        for (BCSYM_Implements *Impl =
                Container->PInterface()->GetFirstImplements();
             Impl;
             Impl = Impl->GetNext())
        {
            if (Impl->IsBad())
            {
                continue;
            }

            BCSYM *RawBaseType = Impl->GetRawRoot();

            // Report an error if this inherited interface is not compliant.
            //
            if (!IsTypeCLSCompliant(RawBaseType, Container))
            {
                ReportErrorOnSymbol(
                    WRNID_InheritedInterfaceNotCLSCompliant2,
                    Container,
                    Container->GetName(),
                    RawBaseType->ChaseToType()->GetErrorName(m_Compiler));
            }
        }
    }
}

void CLSComplianceChecker::VerifyMembersForCLSCompliance(BCSYM_Container * Container)
{
    VSASSERT( Container->IsCLSCompliant(),
                    "Non-CLS compliant container unexpected!!!");

    BCITER_CHILD Members(Container);
    while (BCSYM_NamedRoot *Member = Members.GetNext())
    {
        // Need to do this so that we handle the operators. Operators
        // do not by themselves show up in this (bindable hash) hash
        // we are looking in, so we will find them through their functions
        // instead.
        //
        if (Member->IsProc() &&
            Member->PProc()->IsUserDefinedOperatorMethod())
        {
            Member = Member->PProc()->GetAssociatedOperatorDef();
        }

        if (Member->IsBad())
        {
            continue;
        }

        // Containers are checked later when checking the nested types
        //
        if (Member->IsContainer())
        {
            continue;
        }

        // If the member is not accessible outside the assembly, then
        // don't bother checking for CLS compliance
        //
        if (!IsAccessibleOutsideAssembly(Member))
        {
            continue;
        }

        // We don't want to report errors for synthetic members.
        // Eg: withevents property, the withevents variable _prop, etc.
        // If we don't skip errors for these, we would end up reporting
        // the same errors on both these and the original symbols from
        // which they were synthesized resulting in duplicate errors.
        //
        if (IsSyntheticMember(Member))
        {
            continue;
        }

        // If explicitly marked as CLSCompliant(false), then no need to
        // verify
        //
        if (IsExplicitlyMarkedNonCLSCompliant(Member))
        {
            ValidateNonCLSCompliantMemberInCLSCompliantContainer(
                Member,
                Container);

            continue;
        }

        VerifyNameIsCLSCompliant(Member);

        // Nothing more to do for enum members
        //
        if (Container->IsEnum())
        {
            continue;
        }

        if (Member->IsProc())
        {
            VerifyProcForCLSCompliance(
                Member->PProc(),
                Container);
        }
        else
        {
            VSASSERT( Member->IsVariable(), "What else can get here ?");

            if (!IsTypeCLSCompliant(Member->PVariable()->GetRawType(), Member))
            {
                ReportErrorOnSymbol(
                    WRNID_FieldNotCLSCompliant1,
                    Member,
                    Member->GetName());
            }
        }
    }

    VerifyConstraintsAreCLSCompliant(Container);
}

void CLSComplianceChecker::ValidateNonCLSCompliantMemberInCLSCompliantContainer(
    BCSYM_NamedRoot * Member,
    BCSYM_Container * ParentOfMember)
{
    VSASSERT( ParentOfMember->IsCLSCompliant(),
                    "Members of non-CLS compliant types unexpected!!!");

    // 

    VSASSERT( Member->IsMember(), "Non-member unexpected!!!");

    if (ParentOfMember->IsInterface())
    {
        // Non CLS compliant |1 is not allowed in a CLS compliant interface.

        ReportErrorOnSymbol(
            WRNID_NonCLSMemberInCLSInterface1,
            Member,
            StringOfSymbol(m_Compiler, Member));
    }
    else if (Member->IsProc() &&
             Member->PProc()->IsMustOverrideKeywordUsed())
    {
        // Non CLS compliant mustoverride member is not allowed in a CLS compliant |1.

        ReportErrorOnSymbol(
            WRNID_NonCLSMustOverrideInCLSType1,
            Member,
            StringOfSymbol(m_Compiler, ParentOfMember));
    }
}

void CLSComplianceChecker::VerifyProcForCLSCompliance(
    BCSYM_Proc * Proc,
    BCSYM_Container * ParentOfProc)
{
    // We won't do CLS Compliance checking on BeginInvoke or EndInvoke (only Invoke) to avoid
    // duplicating our efforts.
    //
    if (ParentOfProc->IsDelegate() &&
        (ParentOfProc->SimpleBind(NULL, STRING_CONST(m_Compiler, DelegateBeginInvoke)) == Proc ||
         ParentOfProc->SimpleBind(NULL, STRING_CONST(m_Compiler, DelegateEndInvoke)) == Proc))
    {
        return;
    }

    // Event specific validation
    //
    if (Proc->IsEventDecl())
    {
        BCSYM_EventDecl *Event = Proc->PEventDecl();
        BCSYM *RawDelegate = Event->GetRawDelegate();
        BCSYM *Delegate = Event->GetDelegate();

        VSASSERT(!(Event->AreParametersObtainedFromDelegate() && Event->IsDelegateFromImplements()),
                        "Event parameters obtained from the implemented event's delegate - unexpected!!!");

        // The event's delegate also needs to be validated, but only when it is not the implicitly
        // generated delegate of this event.
        //
        if (Delegate &&
            Delegate->IsDelegate() &&
            Delegate->PClass()->CreatedByEventDecl() != Event)
        {
            // If this asserts fails, this can possibly cause duplicate errors for type arguments
            // of the delegate and also for errors to be reported at the wrong locations.
            //
            VSASSERT(RawDelegate->IsNamedType() || Event->IsDelegateFromImplements(),
                        "Event delegate should not be the raw delegate type of the implemented event!!!");

            if (!IsTypeCLSCompliant(RawDelegate, Event))
            {
                ReportErrorOnSymbol(
                    WRNID_EventDelegateTypeNotCLSCompliant2,
                    RawDelegate->IsNamedType() ?
                        RawDelegate :
                        Event,
                    Event,
                    Delegate->PClass()->GetQualifiedName(true, ParentOfProc),
                    Event->GetName());
            }
        }

        // If the event's parameters are obtained from its delegate, then its parameters should not
        // be checked here because they will be validated when the delegate itself is checked.
        //
        if (Event->AreParametersObtainedFromDelegate())
        {
            return;
        }
    }


    // Check the return type.
    //
    if (!IsTypeCLSCompliant(Proc->GetRawType(), Proc))
    {
        ReportErrorOnSymbol(
            WRNID_ProcTypeNotCLSCompliant1,
            Proc,
            Proc->GetName());
    }

    // Check the parameters.
    //
    for(BCSYM_Param *Param = Proc->GetFirstParam();
        Param;
        Param = Param->GetNext())
    {
        if (!IsTypeCLSCompliant(Param->GetRawType(), Proc))
        {
            ReportErrorOnSymbol(
                WRNID_ParamNotCLSCompliant1,
                Param,
                Proc,
                Param->GetName());
        }
        else if (Param->IsOptional() && Param->IsParamWithValue())
        {
            BCSYM_Expression *OptionalValue =
                Param->PParamWithValue()->GetExpression();

            if (OptionalValue &&
                // SByte, UInt16, Uint32, Uint64 constants are not CLS compliant
                //
                (OptionalValue->GetValue().TypeCode == t_i1 ||
                    OptionalValue->GetValue().TypeCode == t_ui2 ||
                    OptionalValue->GetValue().TypeCode == t_ui4 ||
                    OptionalValue->GetValue().TypeCode == t_ui8))

            {
                ReportErrorOnSymbol(
                    WRNID_OptionalValueNotCLSCompliant1,
                    Param->PParamWithValue()->GetExpression()->HasLocation() ?
                        (BCSYM *)Param->PParamWithValue()->GetExpression() :
                        Param,
                    Proc,
                    Param->GetName());
            }
        }
    }

    VerifyOverloadsForCLSCompliance(Proc);
    VerifyConstraintsAreCLSCompliant(Proc);

}

void CLSComplianceChecker::VerifyOverloadsForCLSCompliance(BCSYM_Proc * PossiblyOverloadedProc)
{
    // Check the Overloads for overloading by Array Rank or
    // overloading on Array of Arrays

    // we don't allow bad members because they could have
    // been bad during overloads verification in bindable.
    //
    VSASSERT( !PossiblyOverloadedProc->IsBad(),
                    "Bad members unexpected!!!");

    if (!Bindable::IsMethod(PossiblyOverloadedProc) &&
        !Bindable::IsProperty(PossiblyOverloadedProc))
    {
        return;
    }

    for(BCSYM_NamedRoot *NextPossibleOverload = PossiblyOverloadedProc->GetNextOverload();
        NextPossibleOverload;
        NextPossibleOverload = NextPossibleOverload->GetNextOverload())
    {
        VSASSERT( NextPossibleOverload->IsProc(),
                    "How can a non-proc be involved in overloads ?");

        BCSYM_Proc *NextOverload = NextPossibleOverload->PProc();

        if (!IsAccessibleOutsideAssembly(NextOverload))
        {
            continue;
        }

        VerifyOverloadsForCLSCompliance(
            PossiblyOverloadedProc,
            NextOverload);
    }

    // 














}

void CLSComplianceChecker::VerifyOverloadsForCLSCompliance(
    BCSYM_Proc * OverloadedProc,
    BCSYM_Proc * OverloadingProc)
{
    if (!OverloadedProc->GetFirstParam() || !OverloadingProc->GetFirstParam())
    {
        return;
    }

    bool ParamTypesAreArrayOfArrays = false;
    bool ParamsDifferOnlyByArrayRank = false;

    {
        BCSYM_Param *Param1;
        BCSYM_Param *Param2;
        for(Param1 = OverloadedProc->GetFirstParam(), Param2 = OverloadingProc->GetFirstParam();
            Param1 && Param2;
            Param1 = Param1->GetNext(), Param2 = Param2->GetNext())
        {
            // Don't do any checking if any of the parameters are bad
            //
            if (Param1->IsBad() || Param2->IsBad())
            {
                return;
            }

            // Note: Nothing special to do for optional and paramarrays
            // because a CLS Compliant consumer can always call them by
            // providing all the parameters.
            //

            BCSYM *Type1 = Param1->GetType();
            BCSYM *Type2 = Param2->GetType();


            if (Type1->IsArrayType() && Type2->IsArrayType())
            {
                BCSYM *ArrayElementType1 = Type1->PArrayType()->GetRoot();
                BCSYM *ArrayElementType2 = Type2->PArrayType()->GetRoot();

                if (ArrayElementType1->IsArrayType() &&
                    ArrayElementType2->IsArrayType())
                {
                    ParamTypesAreArrayOfArrays = true;
                    continue;
                }
                else if (BCSYM::AreTypesEqual(ArrayElementType1, ArrayElementType2))
                {
                    if (Type1->PArrayType()->GetRank() != Type2->PArrayType()->GetRank())
                    {
                        ParamsDifferOnlyByArrayRank = true;
                    }
                    continue;
                }

                // Signatures are different enough
            }
            else
            {
                if (BCSYM::AreTypesEqual(Type1, Type2))
                {
                    continue;
                }
            }

            // Signatures are different enough
            //
            return;
        }

        VSASSERT( !(Param1 && Param2),
                        "both non-NULLs Unexpected!!!");

        // Different number of parameters, so Signatures are different enough
        //
        if (Param1 != Param2)
        {
            return;
        }
    }

    VSASSERT( ParamTypesAreArrayOfArrays ||
              ParamsDifferOnlyByArrayRank,
                    "How did two indetical overloads get through bindable ?");


    if (ParamTypesAreArrayOfArrays ||
        ParamsDifferOnlyByArrayRank)
    {
        // 


        // "'|1' which overloads '|2' differs from it only by array of array parameter types
        // or by the rank of the the array parameter types and so is not CLS Compliant."

        StringBuffer OverloadingMember;
        StringBuffer OverloadedMember;

        OverloadingProc->GetBasicRep(m_Compiler, OverloadingProc->GetContainer(), &OverloadingMember);
        OverloadedProc->GetBasicRep(m_Compiler, OverloadedProc->GetContainer(), &OverloadedMember);

        ReportErrorOnSymbol(
            WRNID_ArrayOverloadsNonCLS2,
            OverloadingProc,
            OverloadingMember.GetString(),
            OverloadedMember.GetString());
    }
}

void CLSComplianceChecker::VerifyEnumUnderlyingTypeForCLSCompliance(BCSYM_Container * PossibleEnum)
{
    VSASSERT( PossibleEnum->IsCLSCompliant(),
                    "Non-CLS compliant container unexpected!!!");

    if (!PossibleEnum->IsEnum())
    {
        return;
    }

    BCSYM *UnderlyingType = PossibleEnum->PClass()->GetUnderlyingTypeForEnum();

    if (!IsTypeCLSCompliant(UnderlyingType, PossibleEnum))
    {
        // Underlying type '|1' of enum is not CLS Compliant.

        ReportErrorOnSymbol(
            WRNID_EnumUnderlyingTypeNotCLS1,
            PossibleEnum,
            UnderlyingType->GetErrorName(m_Compiler));
    }
}

void CLSComplianceChecker::VerifyConstraintsAreCLSCompliant(BCSYM_NamedRoot * ContainerOrProc)
{
    VSASSERT(ContainerOrProc->IsContainer() || ContainerOrProc->IsProc(),
                "Symbol kind unexpected during generic constraint CLS compliance checking!!!");

    // Check the Generic parameter constraint types too for CLS Compliance.
    // If not a generic type or method, it will not have any generic
    // parameters.

    for(BCSYM_GenericParam *GenericParam = ContainerOrProc->GetFirstGenericParam();
        GenericParam;
        GenericParam = GenericParam->GetNextParam())
    {
        for (BCSYM_GenericTypeConstraint *Constraint = GenericParam->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            BCSYM *ConstraintType = Constraint->GetRawType();

            if (IsTypeCLSCompliant(ConstraintType, GenericParam))
            {
                continue;
            }

            ReportErrorOnSymbol(
                WRNID_GenericConstraintNotCLSCompliant1,
                Constraint,
                GenericParam,
                ConstraintType->ChaseToType()->GetErrorName(m_Compiler));
        }
    }
}

bool CLSComplianceChecker::IsTypeCLSCompliant(
    BCSYM * RawType,
    BCSYM_NamedRoot * NamedContext)
{
    BCSYM *ActualType = RawType->ChaseToType();

    if (!ActualType ||
        (ActualType->IsNamedRoot() &&
         ActualType->PNamedRoot()->IsBadNamedRoot()))
    {
        return true;
    }

    // Return true for generic params because its constraints are anyway verified separately
    //
    if (ActualType->IsGenericParam())
    {
        return true;
    }

    VSASSERT( ActualType->IsContainer(),
                    "What else can a chased type be ?");

    if (ActualType->IsGenericBinding())
    {
        NameTypeArguments **RawTypeArgumentsSets =
            RawType->IsNamedType() ? RawType->PNamedType()->GetArgumentsArray() : NULL;

        unsigned CountOfRawTypeArgumentsSets =
            RawType->IsNamedType() ? RawType->PNamedType()->GetNameCount() : 0;

        NameTypeArguments *CurrentRawTypeArgumentsSet = NULL;
        unsigned CurrentRawTypeArgumentsSetIndex = CountOfRawTypeArgumentsSets;

        for(BCSYM_GenericTypeBinding *GenericBinding = ActualType->PGenericTypeBinding();
            GenericBinding;
            GenericBinding = GenericBinding->GetParentBinding())
        {
            if (RawTypeArgumentsSets && CurrentRawTypeArgumentsSetIndex > 0)
            {
                CurrentRawTypeArgumentsSet = RawTypeArgumentsSets[--CurrentRawTypeArgumentsSetIndex];

                if (CurrentRawTypeArgumentsSet &&
                    CurrentRawTypeArgumentsSet->m_BoundGenericBinding != GenericBinding)
                {
                    // Lost the one to one mapping between actual type and raw type, so
                    // invalidate and do not use the raw type argument locations.

                    CurrentRawTypeArgumentsSet = NULL;
                }
            }

            unsigned GenericArgumentCount = GenericBinding->GetArgumentCount();
            for(unsigned Index = 0; Index < GenericArgumentCount; Index++)
            {
                BCSYM *ArgumentType = GenericBinding->GetArgument(Index);

                if (!IsTypeCLSCompliant(ArgumentType, NamedContext))
                {
                    Location *ErrorLocation = NULL;

                    if (CurrentRawTypeArgumentsSet &&
                        CurrentRawTypeArgumentsSet->m_ArgumentCount > 0)
                    {
                        ErrorLocation = CurrentRawTypeArgumentsSet->GetArgumentLocation(Index);
                    }

                    if (!ErrorLocation)
                    {
                        ErrorLocation =
                            RawType->IsNamedType() ?
                                RawType->GetLocation() :
                                NamedContext->GetLocation();
                    }

                    ReportErrorAtLocation(
                        WRNID_TypeNotCLSCompliant1,
                        ErrorLocation,
                        NamedContext,
                        ArgumentType->ChaseToType()->GetErrorName(m_Compiler));
                }
            }
        }

        return IsTypeCLSCompliant(ActualType->PGenericTypeBinding()->GetGeneric(), NamedContext);
    }

    return ActualType->PContainer()->IsCLSCompliant();
}

bool CLSComplianceChecker::IsAccessibleOutsideAssembly(BCSYM_NamedRoot * Member)
{
    // Namespaces are always public
    //
    if (Member->IsNamespace())
    {
        return true;
    }

    ACCESS AccessOfMember = Member->GetAccess();

    if (AccessOfMember == ACCESS_Private ||
        AccessOfMember == ACCESS_Friend)
    {
        return false;
    }


    if (AccessOfMember == ACCESS_Protected ||
        AccessOfMember == ACCESS_ProtectedFriend)
    {
        BCSYM_Container *ContainerOfMember = Member->GetContainer();

        VSASSERT( ContainerOfMember,
                    "How can a non-namespace entity not be enclosed in another container ?");

        // If the containing class is notinheritable, then the protected members are
        // not accessible outside the assembly since nobody can inherit the class.
        //
        // Confirmed this with Microsoft and jsmiller
        //
        if (ContainerOfMember->IsClass() &&
            ContainerOfMember->PClass()->IsNotInheritable())
        {
            return false;
        }

        return true;
    }

    VSASSERT( AccessOfMember == ACCESS_Public,
                    "What else can the access be ?");

    return true;
}


void CLSComplianceChecker::ReportErrorOnSymbol(
    ERRID ErrID,
    BCSYM_NamedRoot * SymbolToReportErrorOn,
    _In_opt_z_ STRING * ErrorReplString1,
    _In_opt_z_ STRING * ErrorReplString2,
    _In_opt_z_ STRING * ErrorReplString3,
    _In_opt_z_ STRING * ErrorReplString4,
    _In_opt_z_ STRING * ErrorReplString5)
{
    ReportErrorOnSymbol(
            ErrID,
            SymbolToReportErrorOn,
            SymbolToReportErrorOn,      // the Named root Context
            ErrorReplString1,
            ErrorReplString2,
            ErrorReplString3,
            ErrorReplString4,
            ErrorReplString5);
}


void CLSComplianceChecker::ReportErrorOnSymbol(
    ERRID ErrID,
    BCSYM * SymbolToReportErrorOn,
    BCSYM_NamedRoot * NamedContext,
    _In_opt_z_ STRING * ErrorReplString1,
    _In_opt_z_ STRING * ErrorReplString2,
    _In_opt_z_ STRING * ErrorReplString3,
    _In_opt_z_ STRING * ErrorReplString4,
    _In_opt_z_ STRING * ErrorReplString5)
{
    ErrorTable *ErrorLog = NamedContext->GetErrorTableForContext();

    if (ErrorLog)
    {
        ErrorLog->CreateErrorWithSymbol(
            ErrID,
            SymbolToReportErrorOn,
            ErrorReplString1,
            ErrorReplString2,
            ErrorReplString3,
            ErrorReplString4,
            ErrorReplString5);
    }
}

void CLSComplianceChecker::ReportErrorAtLocation(
    ERRID ErrID,
    Location * ErrorLocation,
    BCSYM_NamedRoot * NamedContext,
    _In_opt_z_ STRING * ErrorReplString1,
    _In_opt_z_ STRING * ErrorReplString2,
    _In_opt_z_ STRING * ErrorReplString3,
    _In_opt_z_ STRING * ErrorReplString4,
    _In_opt_z_ STRING * ErrorReplString5)
{
    ErrorTable *ErrorLog = NamedContext->GetErrorTableForContext();

    if (ErrorLog)
    {
        ErrorLog->CreateError(
            ErrID,
            ErrorLocation,
            ErrorReplString1,
            ErrorReplString2,
            ErrorReplString3,
            ErrorReplString4,
            ErrorReplString5);
    }
}
