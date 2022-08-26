//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to verify shadowing, overloading and overriding semantics.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void Bindable::ResolveShadowingOverloadingAndOverridingForContainer()
{
    BCSYM_Container *Container = CurrentContainer();

    VSASSERT( Container->IsClass() || Container->IsInterface() || Container->IsNamespace(),
                "Unexpected container kind!!!");

    // We want to let Namespaces through too for metadata containers so that type
    // clashes based on casing are weeded out during ResolveOverloadingInSameContainer
    //
    if (Container->IsNamespace() &&
        !DefinedInMetaData(Container))
    {
        SetStatusOfResolveShadowingOverloadingAndOverriding(Done);
        return;
    }

    VSASSERT( GetStatusOfResolveShadowingOverloadingAndOverriding() != InProgress,
                "How can we have cycles ?");

    if (GetStatusOfResolveShadowingOverloadingAndOverriding() == Done)
    {
        return;
    }

    SetStatusOfResolveShadowingOverloadingAndOverriding(InProgress);

    // complete shadowing, overloading, overriding for bases first
    ResolveShadowingOverloadingAndOverridingForBases();

    // Meta data containers don't have handles Clauses nor do they need constant expressions
    // to be evaluated
    //
    if (!DefinedInMetaData())
    {
        // By doing this here, any new withevents properties synthesized for the handles
        // clauses can go through the normal shadowing/overloading/overriding semantics
        //
        ValidateWithEventsVarsInHandlesListsAndSynthesizePropertiesIfRequired();

        // Need to do this before shadowing semantics because we need to know the values
        // of optional parameters beforehand when doing overriding resolution
        //
        EvaluateAllConstantDeclarationsInContainer();
    }

    MemberInfoIter MemberInfos(Container, CurrentCompilerInstance());

    // Resolve overloading in current container
    //
    ResolveOverloadingInSameContainer(&MemberInfos);

    const bool SkipBadMembers = true;
    BindableMemberInfo *MemberInfo;

    // No Shadowing semantics for namespaces or enums
    //
    if (!Container->IsNamespace() &&
        !Container->IsEnum())
    {
        if (!DefinedInMetaData())
        {
            ValidateDefaultProperties(&MemberInfos);
        }

        // Check for overloaded versions of the same function having conflicting shadowing/overloads
        // flags. Eg:
        // Class Cls1
        //   Overloads Sub foo      'report error here, because the other foo in this class is marked
        //                          'shadows
        //   End Sub
        //
        //   Shadows Sub foo(Byval arg as integer)
        //   End Sub
        // End Class
        //
        CheckForOverloadOverridesShadowsClashesInSameContainer(&MemberInfos);

        MembersHashTable MustOverrideMembersList;

        if (Container->IsClass())
        {
            GetListOfMustOverrideMembersInBasesYetToBeOverridden(Container->PClass(), &MustOverrideMembersList);
        }

        MemberInfos.Reset();
        while (MemberInfo = MemberInfos.GetNext(!SkipBadMembers))
        {
            if (!MemberInfo->IsSynthetic() &&
                MemberInfo->Member->IsProperty())
            {
                ValidateProperty(MemberInfo->Member->PProperty());
            }

            if (MemberInfo->Member->IsBad())
            {
                continue;
            }

            // Ignore constructors. Constructors cannot be overloaded, shadowed, overridden, etc.,
            // declared would have caught these errors for constructors
            //
            if (MemberInfo->Member->IsProc() &&
                MemberInfo->Member->PProc()->IsAnyConstructor())
            {
                continue;
            }

            if (IsShadowing(*MemberInfo))
            {
                ResolveShadowingForMember(MemberInfo, &MustOverrideMembersList);
            }
            else
            {
                VSASSERT( MemberInfo->Member->IsProc(), "How can a non-proc be marked as overloads or overrides ?");

                ResolveOverloadingForMemberAcrossContainers(MemberInfo, &MustOverrideMembersList);
            }
        }

        // Sometimes even when overloads is specified, we infer shadows from it. So we need to
        // propagate this shadows from any synthetic member to their source members and viceversa
        // in that order.
        //
        PropagateShadowingInfoBetweenSourceSymbolsAndTheirSyntheticSymbols(&MemberInfos);

        // Badness here indicates clash with another type in which case the base will not be bound.
        // If base is not bound and we try to resolve overriding, then we give unnecessary errors
        // which end up as noise in some scenarios. See VSWhidbey 32606.
        //
        if (!Container->IsDuplicateType())
        {
            ResolveOverridingForContainer(&MemberInfos, &MustOverrideMembersList);
        }

        VerifyClassIsMarkedAbstractIfNeeded(&MemberInfos, &MustOverrideMembersList);
    }

    SetStatusOfResolveShadowingOverloadingAndOverriding(Done);
}

void
Bindable::ResolveShadowingOverloadingAndOverridingForBases()
{
    BCSYM_Container *Container = CurrentContainer();
    BasesIter Bases(Container);

    while(BCSYM_Container *Base = Bases.GetNextBase())
    {
        ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(Base, m_CompilationCaches);
    }
}

void
Bindable::CheckForOverloadOverridesShadowsClashesInSameContainer
(
    MemberInfoIter *MemberInfos
)
{
    MemberInfos->Reset();

    // Note: The members in MemberInfos are expected to be sorted by name and then location
    while (BindableMemberInfo *CurrentMemberInfo = MemberInfos->GetNext())
    {
        // Types overloaded on arity do not have to match in terms of these specifiers, see
        // Bug VSWhidbey 209623. No need to check for namespaces too. So skip this check
        // for all containers.
        //
        if (CurrentMemberInfo->Member->IsContainer())
        {
            continue;
        }

        unsigned ShadowingInfo = 0;

        // Iterate over all members with this name to see if any of them either explicitly
        // shadow or overload
        //
        MemberInfos->StartPeeking();
        BindableMemberInfo *OtherMemberInfo = CurrentMemberInfo;
        do
        {
            if (IsShadowingExplicitly(*OtherMemberInfo))
            {
                ShadowingInfo = Shadows;
                break;
            }
            else if (IsOverloading(*OtherMemberInfo))
            {
                ShadowingInfo = Overloads;

                // don't "break" here because we still want to find out if some member by this name
                // is explicitly marked as shadows. In short, shadows dominates overloads
            }
        }
        while (OtherMemberInfo = MemberInfos->PeekAtNextOfSameName());


        // Consider - All this looping here is currently O(3n). Can we do better ?
        // There are other possible ways to make it O(2n), but code gets complicated - consider

        if (IsShadowing(ShadowingInfo)) // If at least one member is shadowing explicitly, then verify
                                        // that all the other members by that name are also shadowing
        {
            OtherMemberInfo = CurrentMemberInfo;
            do
            {
                if (!IsShadowingExplicitly(*OtherMemberInfo))
                {
                    if (!OtherMemberInfo->IsSynthetic())
                    {
                        // 




                        // "|1 '|2' must be declared 'Shadows' because another member with this name is
                        // declared 'Shadows'."
                        //
                        ReportErrorOnSymbol(
                            ERRID_MustShadow2,
                            CurrentErrorLog(OtherMemberInfo->Member),
                            OtherMemberInfo->Member,
                            StringOfSymbol(CurrentCompilerInstance(), OtherMemberInfo->Member),
                            OtherMemberInfo->GetName());
                    }

                    // Clear the overloads flag for this Member because overloads is wrong due to the error given above
                    OtherMemberInfo->ShadowingInfo &= ~(Overloads &
                                                        Overrides);

                    // Mark this Member as Shadows because that is what is implied by the above error
                    OtherMemberInfo->ShadowingInfo |= Shadows;

                    // Don't mark this bad because then it will not show up in intellisense, etc.

                }
            }
            while (OtherMemberInfo = MemberInfos->GetNextOfSameName());
        }
        else if (IsOverloading(ShadowingInfo))  // If at least one member is overloading explicitly, then verify
                                                // that all the other members by that name are also marked as overloads
        {
            OtherMemberInfo = CurrentMemberInfo;
            do
            {
                if (IsShadowingImplicitly(*OtherMemberInfo))
                {
                    if (!OtherMemberInfo->IsSynthetic())
                    {
                        // "|1 '|2' must be declared 'Overloads' because another '|2' is declared 'Overloads'/'Overrides'."
                        ReportErrorOnSymbol(
                            ERRID_MustBeOverloads2,
                            CurrentErrorLog(OtherMemberInfo->Member),
                            OtherMemberInfo->Member,
                            StringOfSymbol(CurrentCompilerInstance(), OtherMemberInfo->Member),
                            OtherMemberInfo->GetName());
                    }

                    // Clear the Shadows flag for this Member because shadows is wrong due to the error given above
                    OtherMemberInfo->ShadowingInfo &=
                        ~(Shadows |
                          ImplicitShadows);

                    // Mark this Member as Overloads because that is what is implied by the above error
                    OtherMemberInfo->ShadowingInfo |= Overloads;
                }
            }
            while (OtherMemberInfo = MemberInfos->GetNextOfSameName());
        }
        else
        {
            // skip over all members with this name
            while (MemberInfos->GetNextOfSameName());
        }
    }   // for loop iterating over all the members
}

void
Bindable::ValidateDefaultProperties
(
    MemberInfoIter *MemberInfos
)
{
    BCSYM_Container *Container = CurrentContainer();

    BCSYM_Property *DefaultProperty = Container->GetDefaultProperty();

    if (!DefaultProperty)
    {
        return;
    }

    STRING *DefaultPropertyName = DefaultProperty->GetName();

    MemberInfos->Reset();

    for(BindableMemberInfo *MemberInfo = MemberInfos->GetNext();
        MemberInfo;
        MemberInfo = MemberInfos->GetNext())
    {
        BCSYM_NamedRoot *Member = MemberInfo->Member;

        if (!Member->IsProperty() ||
            MemberInfo->IsSynthetic())
        {
            continue;
        }

        if (StringPool::IsEqual(Member->GetName(), DefaultPropertyName))
        {
            if (!Member->PProperty()->IsDefault())
            {
                // "'|1' and '|2' cannot overload each other because only one is declared 'Default'."

                StringBuffer Property1;
                StringBuffer Property2;

                GetBasicRep(DefaultProperty, &Property1);
                GetBasicRep(Member, &Property2);

                ReportErrorOnSymbol(
                    ERRID_DefaultMissingFromProperty2,
                    CurrentErrorLog(Member),
                    Member,
                    Property1.GetString(),
                    Property2.GetString());
            }
        }
        else
        {
            if (Member->PProperty()->IsDefault())
            {
                // "'Default' can be applied to only one property name in a |1."

                ReportErrorOnSymbol(
                    ERRID_DuplicateDefaultProps1,
                    CurrentErrorLog(Member),
                    Member,
                    StringOfSymbol(CurrentCompilerInstance(), CurrentContainer()));
            }
        }
    }

    BCSYM_Property *ShadowedDefaultProperty =
        GetShadowedDefaultProperty(CurrentContainer());

    if (ShadowedDefaultProperty &&
        // for default shadowing with the same name, the normal shadowing/
        // overloading warnings should be enough. No other special error
        // is needed.
        //
        !StringPool::IsEqual(DefaultPropertyName, ShadowedDefaultProperty->GetName()))
    {
        for(BCSYM_NamedRoot *CurrentDefaultProperty = DefaultProperty;
            CurrentDefaultProperty;
            CurrentDefaultProperty = CurrentDefaultProperty->GetNextOfSameName())
        {
            if (CurrentDefaultProperty->IsBad() ||
                CurrentDefaultProperty->IsShadowsKeywordUsed() ||
                !CurrentDefaultProperty->IsProperty() ||
                !CurrentDefaultProperty->PProperty()->IsDefault())
            {
                continue;
            }

            // "Default property '|1' conflicts with the default property '|2' in the base |3 '|4'. 'Shadows' of default assumed."
            ReportErrorOnSymbol(
                WRNID_DefaultnessShadowed4,
                CurrentErrorLog(CurrentDefaultProperty),
                CurrentDefaultProperty,
                CurrentDefaultProperty->GetName(),
                ShadowedDefaultProperty->GetName(),
                StringOfSymbol(CurrentCompilerInstance(), ShadowedDefaultProperty->GetContainer()),
                ShadowedDefaultProperty->GetContainer()->GetName());

        }
    }

}

BCSYM_Property *
Bindable::GetShadowedDefaultProperty
(
    BCSYM_Container *Container
)
{
    BasesIter Bases(Container);

    while (BCSYM_Container *Base = Bases.GetNextBase())
    {
        BCSYM_Property *BaseDefaultProperty = Base->GetDefaultProperty();

        if (!BaseDefaultProperty)
        {
            continue;
        }

        BCSYM_NamedRoot *AccessibleDefaultMember;
        for(AccessibleDefaultMember = BaseDefaultProperty;
            AccessibleDefaultMember;
            AccessibleDefaultMember = AccessibleDefaultMember->GetNextOfSameName())
        {
            if (AccessibleDefaultMember->IsBad() ||
                !AccessibleDefaultMember->IsProperty())
            {
                continue;
            }

            if (IsAccessible(AccessibleDefaultMember))
            {
                return AccessibleDefaultMember->PProperty();
            }
        }

        if (AccessibleDefaultMember = GetShadowedDefaultProperty(Base))
        {
            return AccessibleDefaultMember->PProperty();
        }
    }

    return NULL;
}


void
Bindable::GetListOfMustOverrideMembersInBasesYetToBeOverridden
(
    BCSYM_Class *Class,
    MembersHashTable *MustOverrideMembersList
)
{
    BCSYM_Class *Base = Class->GetCompilerBaseClass();

    if (Base)
    {
        BCITER_CHILD ContainerIter;
        ContainerIter.Init(Base);
        BCSYM_NamedRoot *Member;

        GetListOfMustOverrideMembersInBasesYetToBeOverridden(Base, MustOverrideMembersList);

        while (Member = ContainerIter.GetNext())
        {
            if (Member->IsBad() &&
                // VSWhidbey 553868: Process bad metadata members, but only when the badness is due
                // to bad overloading/shadowing in metadata.
                (!Member->IsProc() || !Member->PProc()->IsBadMetadataProcOverload()))
            {
                continue;
            }

            // check if a MustOverride method has been overridden. If so, get rid of it from the list

            if (IsOverridingMethodOrProperty(Member))
            {
                BCSYM_Proc *OverriddenProc = Member->PProc()->OverriddenProcLast();

                if (OverriddenProc && IsMustOverrideMethodOrProperty(OverriddenProc))
                {
                    for(BCSYM_NamedRoot *MustOverrideMember = MustOverrideMembersList->Find(OverriddenProc->GetName());
                        MustOverrideMember;
                        MustOverrideMember = MustOverrideMembersList->FindNextOfSameName())
                    {
                        if (OverriddenProc == MustOverrideMember)
                        {
                            MustOverrideMembersList->Remove(MustOverrideMember);
                            break;
                        }
                    }
                }
            }

            if (IsMustOverrideMethodOrProperty(Member))
            {
                MustOverrideMembersList->Add(Member);
            }
        }

    } // if (base)
}


void
Bindable::ValidateProperty
(
    BCSYM_Property *Property
)
{
    // Validate that the Property Return type and the Set Accessor's
    // Parameter type are the same.
    //

    BCSYM_Proc *SetAccessor = Property->SetProperty();

    if (!SetAccessor)
    {
        return;
    }

    // Find the parameter of the setter which happens to be the last
    // parameter of the setter because it is prepended with the
    // property parameters
    //
    BCSYM_Param *SetAccessorParam;
    for(SetAccessorParam = SetAccessor->GetFirstParam();
        SetAccessorParam &&  SetAccessorParam->GetNext();
        SetAccessorParam = SetAccessorParam->GetNext())
    {
    }

    if (!SetAccessorParam)
    {
        return;
    }

    BCSYM *PropertyReturnType = Property->GetType();
    BCSYM *SetAccessorParamType = SetAccessorParam->GetRawType();

    if (!PropertyReturnType ||
        !SetAccessorParamType ||
        PropertyReturnType->IsGenericBadNamedRoot())
    {
        return;
    }

    // Note: Even if the set parameter type is an unresolved type,
    // it is still a good idea to give this error so that the user
    // when correcting the set parameter type can type it the same
    // as the property return type.
    //
    if (!BCSYM::AreTypesEqual(PropertyReturnType,
                              SetAccessorParamType->DigThroughNamedType()))
    {
        // "'Set' parameter must have the same type as the containing property."

        ReportErrorOnSymbol(
            ERRID_SetValueNotPropertyType,
            CurrentErrorLog(Property),
            SetAccessorParamType->IsNamedType() ?   // Primitive types don't have
                SetAccessorParamType :              // location, so then use the
                SetAccessorParam);                  // Parameter location.
    }
}


void
Bindable::ResolveShadowingForMember
(
    BindableMemberInfo *ShadowingMemberInfo,
    MembersHashTable *MustOverrideMembersList
)
{
    ResolveShadowingForMember(ShadowingMemberInfo);

    // Don't report shadowing error on a member that is marked overloads or overrides
    // because this would puzzle the user.
    // So then why did this member come through Shadowing Semantics ? - this is because
    // another member of the same marked as shadows caused an error to be reported on
    // this member and this member was forced into shadowing.
    //
    if (!(ShadowingMemberInfo->Member->IsProc() &&
          (ShadowingMemberInfo->Member->PProc()->IsOverloadsKeywordUsed() ||
           ShadowingMemberInfo->Member->PProc()->IsOverridesKeywordUsed())))
    {
        GenerateErrorIfShadowingAnyMustOverrideMember(ShadowingMemberInfo, MustOverrideMembersList);
    }
}

void
Bindable::ResolveShadowingForMember
(
    BindableMemberInfo *ShadowingMemberInfo
)
{
    BCSYM_NamedRoot *ShadowingMember = ShadowingMemberInfo->Member;
    BCSYM_Container *Container = CurrentContainer();

    VSASSERT( !ShadowingMember->IsShadowing(),
                    "How can a member that has not yet been through shadowing semantics be marked as shadowing?");

    // 







    bool IsShadowing = false;
    BCSYM_NamedRoot *FirstOverloadedOrShadowedMemberFound = NULL;
    BCSYM_NamedRoot *OverloadedMemberFoundInDifferentBaseThanFirst = NULL;

    DetermineShadowingAndOverloadingForMemberAcrossContainers(
        ShadowingMemberInfo,
        Container,
        FirstOverloadedOrShadowedMemberFound,
        OverloadedMemberFoundInDifferentBaseThanFirst,
        IsShadowing);

    if (FirstOverloadedOrShadowedMemberFound)
    {
        ShadowingMember->SetIsShadowing();

        BindableMemberInfo ShadowedMemberInfo(FirstOverloadedOrShadowedMemberFound);
        bool IndicateShadowingAcrossMultipleBases =
            !!OverloadedMemberFoundInDifferentBaseThanFirst;

        GenerateWarningIfShadowingImplicitly(
            ShadowingMemberInfo,
            &ShadowedMemberInfo,
            IndicateShadowingAcrossMultipleBases);
    }

    return;
}


void
Bindable::DetermineShadowingAndOverloadingForMemberAcrossContainers
(
    BindableMemberInfo *OverloadingMemberInfo,
    BCSYM_Container *Container,
    BCSYM_NamedRoot *&FirstOverloadedOrShadowedMemberFound,
    BCSYM_NamedRoot *&OverloadedMemberFoundInDifferentBaseThanFirst,
    bool &IsShadowing
)
{
    BCSYM_NamedRoot *OverloadingMember = OverloadingMemberInfo->Member;
    BasesIter Bases(Container);

    while (BCSYM_Container *Base = Bases.GetNextBase())
    {
        BCSYM_NamedRoot *OverloadedMember;
        for (OverloadedMember =
                SimpleBindIncludingUnBindableMembers(Base, OverloadingMember);
             OverloadedMember;
             OverloadedMember =
                GetNextOfSameNameIncludingUnBindableMembers(OverloadedMember))
        {
            if (OverloadedMember->IsBad())
                continue;

            if (!IsAccessible(OverloadedMember))
                continue;

            BindableMemberInfo OverloadedMemberInfo(OverloadedMember);

            if (OverloadingMemberInfo->IsSynthetic() || OverloadedMemberInfo.IsSynthetic())
            {
                // several reasons why we would ignore,
                // eg: say the clash is because Get_Prop and Get_Prop clash for a Prop, then we
                // can ignore the clash among these because there may really be overloading among
                // their source properties.
                //

                if (!IgnoreClashingErrorsOnSyntheticMembers(OverloadingMemberInfo, &OverloadedMemberInfo))
                {
                    IsShadowing = true;
                    FirstOverloadedOrShadowedMemberFound = OverloadedMember;
                }

                return;
            }

            // Types with same name, but different arity do not hide each other, so no shadowing.
            //
            if (OverloadingMember->IsType() &&
                OverloadedMember->IsType() &&
                OverloadingMember->GetGenericParamCount() != OverloadedMember->GetGenericParamCount())
            {
                continue;
            }

            if (IsShadowingExplicitly(*OverloadingMemberInfo))
            {
                IsShadowing = true;
                FirstOverloadedOrShadowedMemberFound = OverloadedMember;
                return;
            }

            if (!BothAreMethodsOrBothAreProperties(OverloadingMember, OverloadedMember))
            {
                IsShadowing = true;
                FirstOverloadedOrShadowedMemberFound = OverloadedMember;
                return;
            }

            // OverloadedMember in base found

            if (FirstOverloadedOrShadowedMemberFound)
            {
                if (OverloadedMember != FirstOverloadedOrShadowedMemberFound)
                {
                    IsShadowing = true;
                    OverloadedMemberFoundInDifferentBaseThanFirst = OverloadedMember;

                    return;
                }
            }
            else
            {
                FirstOverloadedOrShadowedMemberFound = OverloadedMember;
            }

            // if we have determined that we are overloading a base member, then there is no need
            // to look any deeper in the bases
            break;
        }

        // if we have determined that we are overloading a base member, then there is no need
        // to look any further in the bases of the current base
        //
        if (!OverloadedMember)
        {
            DetermineShadowingAndOverloadingForMemberAcrossContainers(
                OverloadingMemberInfo,
                Base,
                FirstOverloadedOrShadowedMemberFound,
                OverloadedMemberFoundInDifferentBaseThanFirst,
                IsShadowing);

            // if we determined that this is bad or is infact shadowing something, then there is
            // no point continuing any further.
            //
            if (IsShadowing)
            {
                return;
            }
        }
    }

    return;
}


void
Bindable::GenerateWarningIfShadowingImplicitly
(
    BindableMemberInfo *ShadowingMemberInfo,
    BindableMemberInfo *ShadowedMemberInfo,
    bool IndicateShadowingAcrossMultipleBases
)
{

    // Members in standard modules are always assumed to be explicitly
    // shadowing and so no implicitly shadowing warning should be given
    // for them.
    //
    if (IsStdModule(CurrentContainer()))
    {
        return;
    }

    // if the shadowing is implicit, then give a warning
    //
    if (IsShadowingExplicitly(*ShadowingMemberInfo))
    {
        return;
    }

    VSASSERT( !IgnoreClashingErrorsOnSyntheticMembers(ShadowingMemberInfo, ShadowedMemberInfo),
                    "This should have been detected earlier when determining shadowing!!!");

    GenerateImplicitShadowingWarning(
        ShadowingMemberInfo,
        ShadowedMemberInfo,
        IndicateShadowingAcrossMultipleBases);
}

void
Bindable::GenerateImplicitShadowingWarning
(
    BindableMemberInfo *ShadowingMemberInfo,
    BindableMemberInfo *ShadowedMemberInfo,
    bool IndicateShadowingAcrossMultipleBases
)
{
    // For Non-Modules:
    //      - could be implicit if overloads/overrides is specified and clashes with a different kind of member
    //      - could be implicit if no overloads/shadows/overrides is specified
    //
    // For Modules:
    //      - could be implicit if overloads is specified and clashes with a different kind of member
    //

    BCSYM_NamedRoot *ShadowingMember = ShadowingMemberInfo->Member;
    BCSYM_NamedRoot *ShadowedMember = ShadowedMemberInfo->Member;

    VSASSERT( ShadowingMember->IsShadowing(),
                    "Why are we generating an implicit shadowing warning for a member that is not shadowing anything?");

    if (ShadowingMemberInfo->IsSynthetic() &&
        ShadowedMemberInfo->IsSynthetic())
    {
        // |1 '|2' implicitly declares '|3', which conflicts with a member implicitly declared for |4 '|5'
        // in the base |6 '|7'. So the |1 should be declared 'Shadows'.
        ReportErrorAtLocation(
            IsStdModule(CurrentContainer()) ?
                WRNID_SynthMemberShadowsSynthMemberMod7 :
                WRNID_SynthMemberShadowsSynthMember7,
            CurrentErrorLog(ShadowingMemberInfo->NamedContextOfSyntheticMember),
            ShadowingMemberInfo->GetLocationToReportError(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowingMemberInfo->SourceOfSyntheticMember),
            ShadowingMemberInfo->SourceOfSyntheticMember->GetName(),
            ShadowingMemberInfo->GetName(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowedMemberInfo->SourceOfSyntheticMember),
            ShadowedMemberInfo->SourceOfSyntheticMember->GetName(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowedMemberInfo->Member->GetContainer()),
            ShadowedMemberInfo->Member->GetContainer()->GetName());
    }
    else if (ShadowingMemberInfo->IsSynthetic())
    {
        // |1 '|2' implicitly declares '|3', which conflicts with a member in the base |4 '|5', and so the
        // |1 should be declared 'Shadows'.
        ReportErrorAtLocation(
            IsStdModule(CurrentContainer()) ?
                WRNID_SynthMemberShadowsMemberInModule5 :
                WRNID_SynthMemberShadowsMember5,
            CurrentErrorLog(ShadowingMemberInfo->NamedContextOfSyntheticMember),
            ShadowingMemberInfo->GetLocationToReportError(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowingMemberInfo->SourceOfSyntheticMember),
            ShadowingMemberInfo->SourceOfSyntheticMember->GetName(),
            ShadowingMemberInfo->GetName(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowedMemberInfo->Member->GetContainer()),
            ShadowedMemberInfo->Member->GetContainer()->GetName());

    }
    else if (ShadowedMemberInfo->IsSynthetic())
    {
        // |1 '|2' conflicts with a member implicitly declared for |3 '|4' in the base |5 '|6' and so should
        // be declared 'Shadows'.
        ReportErrorOnSymbol(
            IsStdModule(CurrentContainer()) ?
                WRNID_MemberShadowsSynthMemberInModule6 :
                WRNID_MemberShadowsSynthMember6,
            CurrentErrorLog(ShadowingMemberInfo->Member),
            ShadowingMemberInfo->Member,
            StringOfSymbol(CurrentCompilerInstance(), ShadowingMemberInfo->Member),
            ShadowingMemberInfo->GetName(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowedMemberInfo->SourceOfSyntheticMember),
            ShadowedMemberInfo->SourceOfSyntheticMember->GetName(),
            StringOfSymbol(CurrentCompilerInstance(), ShadowedMemberInfo->Member->GetContainer()),
            ShadowedMemberInfo->Member->GetContainer()->GetName());

    }
    else
    {
        if (BothAreMethodsOrBothAreProperties(ShadowingMember, ShadowedMember))
        {
            if (IndicateShadowingAcrossMultipleBases)
            {
                // "|1 '|2' conflicts with other members of the same name across the inheritance hierarchy and so should be declared 'Shadows'."
                ReportErrorOnSymbol(
                    WRNID_MustShadowOnMultipleInheritance2,
                    CurrentErrorLog(ShadowingMemberInfo->Member),
                    ShadowingMember,
                    StringOfSymbol(CurrentCompilerInstance(), ShadowingMember),
                    ShadowingMember->GetName());
            }
            else
            {
                // "|1 '|2' shadows an overloadable member declared in the base |3 '|4'.
                // or "|1 '|2' shadows an overridable member declared in the base |3 '|4'.
                // If you want to overload the base member, this member must be declared 'Overloads'."
                // If shadowing an 'overridable' declare 'overrides' when not in an interface. If in interface, you can declare
                // overloads or shadows.

                ReportErrorOnSymbol(
                      (ShadowedMember->PProc()->IsOverridableKeywordUsed() ||
                       ShadowedMember->PProc()->IsOverridesKeywordUsed())?
                        WRNID_MustOverride2 :
                        WRNID_MustOverloadBase4,
                    CurrentErrorLog(ShadowingMemberInfo->Member),
                    ShadowingMember,
                    StringOfSymbol(CurrentCompilerInstance(), ShadowingMember),
                    ShadowingMember->GetName(),
                    StringOfSymbol(CurrentCompilerInstance(), ShadowedMember->GetContainer()),
                    ShadowedMember->GetContainer()->GetName());
            }
        }
        else
        {
            // "|1 '|2' conflicts with |3 '|2' in the base |4 '|5' and so should be declared 'Shadows'."
            ReportErrorOnSymbol(
                IsStdModule(CurrentContainer()) ?
                    WRNID_MemberShadowsMemberInModule5 :
                    WRNID_OverrideType5,
                CurrentErrorLog(ShadowingMemberInfo->Member),
                ShadowingMember,
                StringOfSymbol(CurrentCompilerInstance(), ShadowingMember),
                ShadowingMember->GetName(),
                StringOfSymbol(CurrentCompilerInstance(), ShadowedMember),
                StringOfSymbol(CurrentCompilerInstance(), ShadowedMember->GetContainer()),
                ShadowedMember->GetContainer()->GetName());
        }
    }
}


void
Bindable::GenerateErrorIfShadowingAnyMustOverrideMember
(
    BindableMemberInfo *ShadowingMemberInfo,
    MembersHashTable *MustOverrideMembersList,
    bool fHideBySig
)
{
    // Don't mark metadata members bad. The members should still be usable.Bug VSWhidbey 336579.
    //
    // 




    if (DefinedInMetaData())
    {
        return;
    }

    BCSYM_NamedRoot *ShadowingMember = ShadowingMemberInfo->Member;

    // We don't check accessibility here. What if the MustOverride method is a Friend member
    // in a public class from another assembly, then this is inaccessible here, so the shadowing
    // member really does not shadow anything in this case. But we go ahead and give an error
    // in this case too because the user really should not be allowed to shadow mustoverride
    // methods.

    // if there is a MustOverride Member by this name in a base class that is yet to be overridden,
    // then this shadowing cannot be permitted
    //
    BCSYM_NamedRoot *MustOverrideMember = MustOverrideMembersList->Find(ShadowingMember->GetName());

    if (fHideBySig && MustOverrideMember)
    {
        Symbols
            SymbolFactory(
                CurrentCompilerInstance(),
                CurrentAllocator(),
                NULL,
                CurrentGenericBindingCache());

        BCSYM_GenericBinding *ShadowingMemberBinding =
            IsGenericOrHasGenericParent(ShadowingMember->GetParent()) ?
                SynthesizeOpenGenericBinding(ShadowingMember->GetParent(), SymbolFactory) :
                NULL;

        while (MustOverrideMember)
        {
            unsigned CompareFlags;

            if (!BothAreMethodsOrBothAreProperties(ShadowingMember, MustOverrideMember) ||
                (CanShadowByNameAndSig(
                    ShadowingMember->PProc(),
                    ShadowingMemberBinding,
                    MustOverrideMember->PProc(),
                    DeriveGenericBindingForMemberReference(ShadowingMemberBinding ? ShadowingMemberBinding : ShadowingMember->GetParent(), MustOverrideMember, SymbolFactory, CurrentCompilerHost()),
                    CompareFlags,
                    &SymbolFactory)))
            {
                    break;
            }

            MustOverrideMember = MustOverrideMembersList->FindNextOfSameName();
        }
    }

    if (MustOverrideMember)
    {
        // in order to get to the mustoverride member in the most immediate base class, get the
        // last mustoverride member by this name

        BindableMemberInfo MustOverrideMemberInfo(MustOverrideMember);

        if (IgnoreClashingErrorsOnSyntheticMembers(ShadowingMemberInfo, &MustOverrideMemberInfo))
        {
            return;
        }

        if (ShadowingMemberInfo->IsSynthetic() &&
            MustOverrideMemberInfo.IsSynthetic())
        {
            VSASSERT( StringPool::IsEqual(
                            ShadowingMemberInfo->SourceOfSyntheticMember->GetName(),
                            MustOverrideMemberInfo.SourceOfSyntheticMember->GetName()),
                      "How can we have two MustOverride synthetic members with the same name, but generated by source symbols with different names ?");

            // in this case, don't report error here, because the error will be reported on the actual
            // source symbols
            return;
        }


        StringBuffer BasicRep;
        GetBasicRep(ShadowingMember, &BasicRep);

        if (ShadowingMemberInfo->IsSynthetic())
        {

            // '|1', implicitly declared for |2 '|3', cannot shadow a 'MustOverride' method in the base |4 '|5'.
            ReportErrorAtLocation(
                ERRID_SynthMemberShadowsMustOverride5,
                CurrentErrorLog(ShadowingMemberInfo->NamedContextOfSyntheticMember),
                ShadowingMemberInfo->GetLocationToReportError(),
                BasicRep.GetString(),
                StringOfSymbol(CurrentCompilerInstance(), ShadowingMemberInfo->SourceOfSyntheticMember),
                ShadowingMemberInfo->SourceOfSyntheticMember->GetName(),
                StringOfSymbol(CurrentCompilerInstance(), MustOverrideMemberInfo.Member->GetContainer()),
                MustOverrideMemberInfo.Member->GetContainer()->GetName());

        }
        else if (MustOverrideMemberInfo.IsSynthetic())
        {

            // '|1' cannot shadow a 'MustOverride' method implicitly declared for |2 '|3' in the base |4 '|5'.
           ReportErrorOnSymbol(
               ERRID_MemberShadowsSynthMustOverride5,
               CurrentErrorLog(ShadowingMember),
               ShadowingMember,
               BasicRep.GetString(),
               StringOfSymbol(CurrentCompilerInstance(), MustOverrideMemberInfo.SourceOfSyntheticMember),
               MustOverrideMemberInfo.SourceOfSyntheticMember->GetName(),
               StringOfSymbol(CurrentCompilerInstance(), MustOverrideMemberInfo.Member->GetContainer()),
               MustOverrideMemberInfo.Member->GetContainer()->GetName());

        }
        else
        {
            // '|1' cannot shadow a method declared 'MustOverride'.
            ReportErrorOnSymbol(
                ERRID_CantShadowAMustOverride1,
                CurrentErrorLog(ShadowingMember),
                ShadowingMember,
                BasicRep.GetString());
        }

        ShadowingMemberInfo->SetIsBad();
    }
}


bool
Bindable::CanShadowByNameAndSig
(
    BCSYM_Proc *Member,
    BCSYM_GenericBinding *MemberBinding,
    BCSYM_Proc *OtherMember,
    BCSYM_GenericBinding *OtherMemberBinding,
    unsigned &CompareFlags,
    Symbols *SymbolFactory
)
{
    VSASSERT(BothAreMethodsOrBothAreProperties(Member, OtherMember), "Unexpected symbol mismatch during hidebysig compare!!!");

    CompareFlags = BCSYM::CompareProcs(Member, MemberBinding, OtherMember, OtherMemberBinding, SymbolFactory);

    if (CompareFlags & (EQ_Shape | EQ_GenericTypeParams | EQ_GenericMethodTypeParamCount))    // are they different enough ?
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool
Bindable::CanOverride
(
    BCSYM_Proc *Member,
    BCSYM_GenericBinding *MemberBinding,
    BCSYM_Proc *OtherMember,
    BCSYM_GenericBinding *OtherMemberBinding,
    unsigned &CompareFlags,
    Symbols *SymbolFactory
)
{
    CompareFlags =
        BCSYM::CompareProcs(Member, MemberBinding, OtherMember, OtherMemberBinding, SymbolFactory);

    if (CompareFlags & (EQ_Shape | EQ_GenericTypeParams | EQ_GenericMethodTypeParamCount))     // are they different enough ?
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool
Bindable::CompareConstraints
(
    BCSYM_NamedRoot *OverriddenMember,
    BCSYM_GenericBinding *OverriddenMemberBinding,
    BCSYM_NamedRoot *OverridingMember,
    BCSYM_GenericBinding *OverridingMemberBinding,
    Symbols *SymbolFactory
)
{
    // Assumption: Duplicate constraints have been marked bad by this time.

    BCSYM_GenericParam *GenericParam1;
    BCSYM_GenericParam *GenericParam2;

    for(GenericParam1 = OverriddenMember->GetFirstGenericParam(),
            GenericParam2 = OverridingMember->GetFirstGenericParam();
        GenericParam1 && GenericParam2;
        GenericParam1 = GenericParam1->GetNextParam(),
            GenericParam2 = GenericParam2->GetNextParam())
    {
        if (!CompareConstraints(
                GenericParam1,
                OverriddenMemberBinding,
                GenericParam2,
                OverridingMemberBinding,
                SymbolFactory))
        {
            return false;
        }
    }

    return true;
}

bool
Bindable::CompareConstraints
(
    BCSYM_GenericParam *GenericParam1,
    BCSYM_GenericBinding *MemberBinding1,
    BCSYM_GenericParam *GenericParam2,
    BCSYM_GenericBinding *MemberBinding2,
    Symbols *SymbolFactory
)
{
    return
        CompareConstraintsHelper(
            GenericParam1,
            MemberBinding1,
            GenericParam2,
            MemberBinding2,
            SymbolFactory) &&
        CompareConstraintsHelper(
            GenericParam2,
            MemberBinding2,
            GenericParam1,
            MemberBinding1,
            SymbolFactory);
}

bool
Bindable::CompareConstraintsHelper
(
    BCSYM_GenericParam *GenericParam1,
    BCSYM_GenericBinding *MemberBinding1,
    BCSYM_GenericParam *GenericParam2,
    BCSYM_GenericBinding *MemberBinding2,
    Symbols *SymbolFactory
)
{
    // Verify that every constraint on param1 is also present on param2.
    //
    // Also at the end, verify that the number of constraints on param1
    // is the same as the number of constraints on param2.

    for(BCSYM_GenericConstraint *Constraint1 = GenericParam1->GetConstraints();
        Constraint1;
        Constraint1 = Constraint1->Next())
    {
        if (Constraint1->IsBadConstraint())
        {
            continue;
        }

        BCSYM *ConstraintType1 = NULL;

        if (Constraint1->IsGenericTypeConstraint())
        {
            ConstraintType1 = Constraint1->PGenericTypeConstraint()->GetType();

            if (!ConstraintType1 || ConstraintType1->IsBad())
            {
                continue;
            }

            ConstraintType1 =
                ReplaceGenericParametersWithArguments(
                    ConstraintType1,
                    MemberBinding1,
                    *SymbolFactory);
        }

        VSASSERT(!Constraint1->IsGenericTypeConstraint() || ConstraintType1, "Constraint matching lost!!!");

        BCSYM_GenericConstraint *Constraint2;
        for(Constraint2 = GenericParam2->GetConstraints();
            Constraint2;
            Constraint2 = Constraint2->Next())
        {
            if (Constraint2->IsBadConstraint())
            {
                continue;
            }

            // Match the new constraints

            if (Constraint1->IsGenericTypeConstraint() != Constraint2->IsGenericTypeConstraint())
            {
                continue;
            }

            if (Constraint1->IsGenericNonTypeConstraint())
            {
                if (Constraint1->PGenericNonTypeConstraint()->GetConstraintKind() ==
                        Constraint2->PGenericNonTypeConstraint()->GetConstraintKind())
                {
                    break;
                }

                continue;
            }

            // Match the type constraints

            VSASSERT(ConstraintType1 && Constraint2->IsGenericTypeConstraint(), "Constraint matching lost!!!");

            BCSYM *ConstraintType2 = Constraint2->PGenericTypeConstraint()->GetType();

            if (!ConstraintType2 || ConstraintType2->IsBad())
            {
                continue;
            }

            ConstraintType2 =
                ReplaceGenericParametersWithArguments(
                    ConstraintType2,
                    MemberBinding2,
                    *SymbolFactory);

            if (ONLY_THESE_FLAGS_ARE_SET(
                    EQ_Match | EQ_GenericMethodTypeParams,
                    BCSYM::CompareReferencedTypes(ConstraintType1, (GenericBinding *)NULL, ConstraintType2, (GenericBinding *)NULL, NULL)))
            {
                break;
            }
        }

        if (!Constraint2)
        {
            // no matching constraint found
            //
            return false;
        }
    }

    return true;
}

bool
Bindable::IsVirtual
(
    BCSYM_Proc *Proc
)
{
    if (Proc->IsOverridableKeywordUsed() ||
        Proc->IsOverridesKeywordUsed() ||
        Proc->IsMustOverrideKeywordUsed() ||
        Proc->IsImplementing())
    {
        return true;
    }

    return false;
}

bool
Bindable::IsOverridable
(
    BCSYM_Proc *Proc
)
{
    if (Proc->IsOverridableKeywordUsed() ||
        Proc->IsOverridesKeywordUsed() ||
        Proc->IsMustOverrideKeywordUsed())
    {
        return true;
    }

    return false;

}


bool
Bindable::BothAreMethodsOrBothAreProperties
(
    BCSYM_NamedRoot *Member1,
    BCSYM_NamedRoot *Member2
)
{
    if ((IsMethod(Member1) &&
            (IsMethod(Member2))) ||
        (IsProperty(Member1) &&
            (IsProperty(Member2))))
    {
        return true;
    }

    return false;
}

void
Bindable::ResolveOverloadingForMemberAcrossContainers
(
    BindableMemberInfo *OverloadingMemberInfo,
    MembersHashTable *MustOverrideMembersList
)
{
    ResolveOverloadingForMemberAcrossContainers(OverloadingMemberInfo);

    if (!OverloadingMemberInfo->Member->IsBad() &&
        !IsOverriding(*OverloadingMemberInfo))
    {
        GenerateErrorIfShadowingAnyMustOverrideMember(
            OverloadingMemberInfo,
            MustOverrideMembersList,
            true /* HideBySig */);
    }
}

// Emitted names are compatible in all cases but one:  differing conversion operators.
// In this case, differing conversion operators are allowed to overload each other even though
// their emitted names ("op_Implict" and "op_Explicit") are different.
//
bool
Bindable::EmittedNamesAreCompatible
(
    BCSYM_NamedRoot *Member1,
    BCSYM_NamedRoot *Member2
)
{
    return
        !IsConversionOperator(Member1) ||
        !IsConversionOperator(Member2) ||
        Member1->PUserDefinedOperator()->GetOperator() == Member2->PUserDefinedOperator()->GetOperator();
}

void
Bindable::ResolveOverloadingForMemberAcrossContainers
(
    BindableMemberInfo *OverloadingMemberInfo
)
{

    // The following assertion ensures that the input must be an event from meta file, a method or a property
    VSASSERT( 
        IsMethod(OverloadingMemberInfo->Member) || 
        IsProperty(OverloadingMemberInfo->Member) || 
        (IsEvent(OverloadingMemberInfo->Member) && m_CurrentContainerContext != NULL && m_CurrentContainerContext->GetCompilerFile()->IsMetaDataFile()),
        "How can a non-proc be considered for overloading/hide by sig ?");

    BCSYM_Proc *OverloadingMember = OverloadingMemberInfo->Member->PProc();
    BCSYM_Container *Container = CurrentContainer();

    bool IsShadowing = false;
    BCSYM_NamedRoot *FirstOverloadedOrShadowedMemberFound = NULL;
    BCSYM_NamedRoot *OverloadedMemberFoundInDifferentBaseThanFirst = NULL;

    DetermineShadowingAndOverloadingForMemberAcrossContainers(
        OverloadingMemberInfo,
        Container,
        FirstOverloadedOrShadowedMemberFound,
        OverloadedMemberFoundInDifferentBaseThanFirst,
        IsShadowing);

    if (!FirstOverloadedOrShadowedMemberFound)
    {
        VSASSERT( !IsShadowing,
                        "How can we shadow a NULL member ?");

        return;
    }

    if (OverloadedMemberFoundInDifferentBaseThanFirst)
    {
        // "Overloading methods declared in multiple base interfaces is not valid."

        VSASSERT( Container->IsInterface(),
                        "How can a non-interface have multiple bases ?");

        VSASSERT( IsShadowing,
                        "How can this be determined to not be shadowing ?");

        ReportErrorOnSymbol(
            ERRID_CantOverloadOnMultipleInheritance,
            CurrentErrorLog(OverloadingMember),
            OverloadingMember);

        OverloadingMemberInfo->Member->SetIsShadowing();
        return;
    }

    if (IsShadowing)
    {
        OverloadingMember->SetIsShadowing();

        BindableMemberInfo ShadowedMemberInfo(FirstOverloadedOrShadowedMemberFound);
        const bool IndicateShadowingAcrossMultipleBases = true;

        GenerateImplicitShadowingWarning(
            OverloadingMemberInfo,
            &ShadowedMemberInfo,
            !IndicateShadowingAcrossMultipleBases);
    }
    else
    {
        BCSYM_NamedRoot *OverloadedMember = FirstOverloadedOrShadowedMemberFound;

        VSASSERT( IsMethod(OverloadedMember) || IsProperty(OverloadedMember),
                        "How can a non-proc be overloaded (hide by sig) ?");

        Symbols::SetOverloads(OverloadingMember);

        if (EmittedNamesAreCompatible(OverloadingMember, OverloadedMember))
        {
            Symbols::SetEmittedName(OverloadingMember, OverloadedMember->GetEmittedName());
        }
    }

    return;
}

void
Bindable::PropagateShadowingInfoBetweenSourceSymbolsAndTheirSyntheticSymbols
(
    MemberInfoIter *MemberInfos
)
{
    // Sometimes even when overloads is specified, we infer shadows from it. So we need to
    // propagate this shadows from any synthetic member to their source members and viceversa
    // in that order. So the order of the loops here is important.
    //

    // Propagate shadowing info. from synthetic members to their source symbols
    MemberInfos->Reset();
    while(BindableMemberInfo *MemberInfo = MemberInfos->GetNext())
    {
        if (!MemberInfo->IsSynthetic() )
        {
            continue;
        }

        // Synthetic Members get here

        VSASSERT( MemberInfo->SourceOfSyntheticMember,
                    "How can a synthetic symbols not have a corresponding source symbol ?");

        if (MemberInfo->Member->IsShadowing())
        {
            MemberInfo->SourceOfSyntheticMember->SetIsShadowing();
        }
    }

    // Propagate shadowing info. from source members to their source symbols
    MemberInfos->Reset();
    while(BindableMemberInfo *MemberInfo = MemberInfos->GetNext())
    {
        if (!MemberInfo->IsSynthetic())
        {
            continue;
        }

        // Synthetic Members get here

        VSASSERT( MemberInfo->SourceOfSyntheticMember,
                    "How can a synthetic symbols not have a corresponding source symbol ?");

        if (MemberInfo->SourceOfSyntheticMember->IsShadowing())
        {
            MemberInfo->Member->SetIsShadowing();
        }

        if (MemberInfo->SourceOfSyntheticMember->IsUserDefinedOperator() &&
            MemberInfo->SourceOfSyntheticMember->PProc()->IsOverloads())
        {
            VSASSERT(
                MemberInfo->Member->PMethodDecl()->IsUserDefinedOperatorMethod() &&
                MemberInfo->Member->PMethodDecl()->GetAssociatedOperatorDef() == MemberInfo->SourceOfSyntheticMember,
                "operator method doesn't match its source symbol");
            Symbols::SetOverloads(MemberInfo->Member->PProc());
            Symbols::SetEmittedName(MemberInfo->Member, MemberInfo->SourceOfSyntheticMember->GetEmittedName());
        }
    }

}


void
Bindable::ResolveOverridingForContainer
(
    MemberInfoIter *MemberInfos,
    MembersHashTable *MustOverrideMembersList
)
{
    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    BCSYM_GenericBinding *BindingForContainer =
        IsGenericOrHasGenericParent(CurrentContainer()) ?
            SynthesizeOpenGenericBinding(CurrentContainer(), SymbolFactory) :
            NULL;

    MemberInfos->Reset();
    while (BindableMemberInfo *MemberInfo = MemberInfos->GetNext(false /* don't skip bad members */))
    {
        // VSWhidbey 553868: Consider metadata members that were marked as bad during overloads validation.
        //
        if (MemberInfo->Member->IsBad() &&
            (!MemberInfo->Member->IsProc() ||
             !MemberInfo->Member->PProc()->IsBadMetadataProcOverload()))
        {
            continue;
        }

        if (!MemberInfo->Member->IsProc() ||
            !MemberInfo->Member->PProc()->IsOverridesKeywordUsed() ||
            MemberInfo->Member->IsUserDefinedOperator() ||  // should I do this?
            MemberInfo->Member->PProc()->IsPartialMethodDeclaration() ) // Bug #112817 - DevDiv Bugs
        {
            continue;
        }

        ResolveOverridingForMember(MemberInfo, BindingForContainer, MustOverrideMembersList, &SymbolFactory);
    }
}

void
Bindable::ResolveOverridingForMember
(
    BindableMemberInfo *OverridingMemberInfo,
    BCSYM_GenericBinding *BindingForContainer,
    MembersHashTable *MustOverrideMembersList,
    Symbols *SymbolFactory
)
{
    VSASSERT(CurrentContainer()->IsClass(), "How can a non-class have overrides and get here ?");

    BCSYM_Proc *OverriddenMember = NULL;
    BCSYM_Proc *InAccessibleVirtualMethodInIntermediateClass = NULL;
    DynamicArray< BCSYM_Proc* > InAccessibleOverridingFriendMethods;
    unsigned CompareFlags = 0;
    bool ConstraintsMatch = true;
    DynamicArray<BCSYM_Proc *> AmbiguousOverriddenMembers;

    // Some times when resolving hide by sig (ResolveOverloadingForMemberAcrossContainers),
    // we might determine that shadowing is the better option in which case the OverriddenMember
    // is NULL - i.e. nothing to override
    //
    if (!OverridingMemberInfo->Member->IsShadowing() ||
        DefinedInMetaData())    // VSWhidbey 553868. Don't consider shadowing for metadata members.
    {
        CompareFlags =
            FindMemberOverriddenByOverridingMember(
                OverridingMemberInfo,
                BindingForContainer,
                CurrentContainer(),
                OverriddenMember,
                InAccessibleVirtualMethodInIntermediateClass,
                InAccessibleOverridingFriendMethods,
                ConstraintsMatch,
                AmbiguousOverriddenMembers,
                SymbolFactory);
    }

    BindableMemberInfo OverriddenMemberInfo(OverriddenMember);

    VerifyOverridingSemantics(
        OverridingMemberInfo,
        &OverriddenMemberInfo,
        InAccessibleOverridingFriendMethods,
        CompareFlags,
        ConstraintsMatch,
        AmbiguousOverriddenMembers,
        SymbolFactory);

    // Mark as overriding and set Overridden method even if there has been an error. This will help with
    // Error correction.

    if (OverriddenMember && AmbiguousOverriddenMembers.Count() == 0)
    {
        // keep track of the fact that OverridingMember is overriding something
        OverridingMemberInfo->Member->PProc()->SetOverriddenProc(OverriddenMember);

        // Dev10 #735384 Check if we are improperly using embedded types
        if (!DefinedInMetaData())
        {
            BCSYM_Proc * pOverridingProc = OverridingMemberInfo->Member->PProc();
            ErrorTable * pErrorTable = CurrentErrorLog(pOverridingProc);

            if (pErrorTable != NULL && pOverridingProc->HasLocation())
            {
                CompilerProject * pUseProject = pOverridingProc->GetContainingProject();
                CompilerProject * pImportedFromProject = OverriddenMember->GetContainingProject();

                if (pUseProject != NULL && pImportedFromProject != NULL && pUseProject != pImportedFromProject &&
                    pUseProject->NeedToCheckUseOfEmbeddedTypes())
                {
                    if (!pOverridingProc->IsBad())
                    {
                        TypeHelpers::CheckProcForUseOfEmbeddedTypes(
                            pOverridingProc,            
                            pUseProject,
                            pUseProject, 
                            pErrorTable,
                            pOverridingProc->GetLocation());
                    }

                    if (!OverriddenMember->IsBad())
                    {
                        TypeHelpers::CheckProcForUseOfEmbeddedTypes(
                            OverriddenMember,            
                            pUseProject,
                            pImportedFromProject, 
                            pErrorTable,
                            pOverridingProc->GetLocation());
                    }
                }
            }
        }
    
        // Bug VSWhidbey 525377. Mark metadata properties that override
        // withevents properties as withevents properties.
        //
        if (DefinedInMetaData() &&
            OverriddenMember->IsProperty() &&
            OverriddenMember->CreatedByWithEventsDecl() &&
            !OverridingMemberInfo->Member->CreatedByWithEventsDecl())
        {
            OverridingMemberInfo->Member->SetMemberThatCreatedSymbol(OverriddenMember->CreatedByWithEventsDecl());
        }

        if (EmittedNamesAreCompatible(OverridingMemberInfo->Member, OverriddenMember))
        {
            // Overriding method needs to have the same emitted name as the overridden method - #26831
            //
            // Ideally we don't need to do this here because HidebySig semantics should have taken care
            // of this. But sometimes we might unexpected have casing differences between the one
            // we are overriding and other members in intermediate classes that we overload (hidebysig).
            // We have an unresolved issue here. But to guarantee that the assembly produced is
            // verifiable, we force the case to that of the overridden method.
            // UNRESOLVED ISSUE:Microsoft - do we want to emit methodimpl if we are overloading (hidebysig)
            // against methods with a different case than the one we are overriding? or give an error?
            //
            Symbols::SetEmittedName(OverridingMemberInfo->Member, OverriddenMember->GetEmittedName());
        }

        // Emit methodimpls when there is a IntermediateMetaDataContainer for more robust versioning.
        // We revert to the everett behavior and no longer emit methodimpl for the following condition
        // because the VB compiler cannot import such metadata currently - see bug 553748. This needs
        // to be enable for orcas.
        //HaveIntermediateClassDefinedInDifferentAssemblyOrModule(
        //    CurrentContainer()->PClass(),
        //    OverriddenMember->GetContainer()->PClass())))

        if (IsMethod(OverridingMemberInfo->Member) &&
            InAccessibleVirtualMethodInIntermediateClass)
        {
            OverridingMemberInfo->Member->PProc()->SetOverridesRequiresMethodImpl(true);
        }

        // Remove from the MustOverrideMembersList if present
        if (OverriddenMember->IsMustOverrideKeywordUsed())
        {
            MustOverrideMembersList->Remove(OverriddenMember);
        }
    }
}

bool
Bindable::HaveIntermediateClassDefinedInDifferentAssemblyOrModule
(
    BCSYM_Class *DerivedClass,
    BCSYM_Class *BaseClass
)
{
    for(BCSYM_Class *IntermediateClass = DerivedClass->GetCompilerBaseClass();
        IntermediateClass && !BaseClass->IsSameContainer(IntermediateClass);
        IntermediateClass = IntermediateClass->GetCompilerBaseClass())
    {
        // Different Assembly ?
        if (DerivedClass->GetContainingProject() != IntermediateClass->GetContainingProject())
        {
            return true;
        }

        // Same Assembly, but in a different .Net Module?
        if (DefinedInMetaData(IntermediateClass))
        {
            return true;
        }

    }

    return false;
}

// Return the compare flags between the
unsigned
Bindable::FindMemberOverriddenByOverridingMember
(
    BindableMemberInfo *OverridingMemberInfo,
    BCSYM_GenericBinding *OverridingMemberBinding,
    BCSYM_Container *Container,
    BCSYM_Proc *&OverriddenMember,
    BCSYM_Proc *&InAccessibleVirtualMethodInIntermediateClass,
    DynamicArray<BCSYM_Proc *> &InAccessibleOverridingFriendMethods,
    bool &OverriddenMemberConstraintsMatch,
    DynamicArray<BCSYM_Proc *> &AmbiguousOverriddenMembers,
    Symbols *SymbolFactory
)
{
    VSASSERT( IsMethod(OverridingMemberInfo->Member) || IsProperty(OverridingMemberInfo->Member) || IsEvent(OverridingMemberInfo->Member),
                    "How can a non-proc be considered for overriding?");

    VSASSERT( CurrentContainer()->IsClass(), "How can a non-class get here?");

    BCSYM_Class *Base = Container->PClass()->GetCompilerBaseClass();
    unsigned OverriddenMemberCompareFlags = 0;

    if (!Base)
    {
        return OverriddenMemberCompareFlags;
    }

    BCSYM_Proc *OverridingMember = OverridingMemberInfo->Member->PProc();
    bool BaseMembersShadowed = false;
    bool OverriddenMemberIsAccessible = false;

    for (BCSYM_NamedRoot *PossibleOverriddenMember =
            SimpleBindIncludingUnBindableMembers(Base, OverridingMember);
         PossibleOverriddenMember;
         PossibleOverriddenMember =
            GetNextOfSameNameIncludingUnBindableMembers(PossibleOverriddenMember))
    {
        if (PossibleOverriddenMember->IsBad())
        {
            // VSWhidbey 553868: Consider metadata members that were marked as bad during overloads validation.
            //
            if (!PossibleOverriddenMember->IsProc() ||
                !PossibleOverriddenMember->PProc()->IsBadMetadataProcOverload())
            {
                continue;
            }
        }

        if (!PossibleOverriddenMember->IsProc())
        {
            VSASSERT( DefinedInMetaData() ||
                      !IsAccessible(PossibleOverriddenMember) ||
                      (PossibleOverriddenMember->IsVariable() &&
                           PossibleOverriddenMember->PVariable()->IsWithEvents()),
                        "How can an accessible non-proc get here? This should have been caught by hidebysig semantics!");

            continue;
        }

        // VSWhidbey 553868: Consider case sensitiveness for metadata members.
        //
        if (DefinedInMetaData() &&
            !StringPool::IsEqualCaseSensitive(OverridingMember->GetName(), PossibleOverriddenMember->GetName()))
        {
            continue;
        }

        unsigned CompareFlags;

        bool PossibleOverriddenMemberIsAccessible = IsAccessible(PossibleOverriddenMember);

        BCSYM_GenericBinding *PossibleOverriddenMemberBinding =
                DeriveGenericBindingForMemberReference(
                    OverridingMemberBinding ?
                        (Type *)OverridingMemberBinding :
                        CurrentContainer(),
                    PossibleOverriddenMember,
                    *SymbolFactory,
                    CurrentCompilerHost());

        if (CanOverride(
                OverridingMember,
                OverridingMemberBinding,
                PossibleOverriddenMember->PProc(),
                PossibleOverriddenMemberBinding,
                CompareFlags,
                SymbolFactory))
        {
            // If we have a frend we keep track of any method that overrides another method.

            if (!PossibleOverriddenMemberIsAccessible &&
                PossibleOverriddenMember->GetAccess() == ACCESS_Friend &&
                PossibleOverriddenMember->PProc()->IsOverrides())
            {
                InAccessibleOverridingFriendMethods.AddElement( PossibleOverriddenMember->PProc() );
            }

            if (!PossibleOverriddenMemberIsAccessible &&
                !InAccessibleVirtualMethodInIntermediateClass &&
                IsMethod(PossibleOverriddenMember) &&
                IsVirtual(PossibleOverriddenMember->PProc()))
            {
                // store this so that we know whether to emit a MethodImpl for the overrides later on
                InAccessibleVirtualMethodInIntermediateClass = PossibleOverriddenMember->PProc();
            }

            // Note: Synthetic Members cannnot override non-synthetic members and viceversa
            //
            if (IsSynthetic(PossibleOverriddenMember) != OverridingMemberInfo->IsSynthetic() &&
                !(DefinedInMetaData() &&                             // Bug VSWhidbey 525447
                  PossibleOverriddenMember->CreatedByWithEventsDecl()))
            {
                continue;
            }

            if (!PossibleOverriddenMemberIsAccessible)
            {
                if (OverriddenMember == NULL)
                {
                    // store this in case we don't find any other possible member.
                    // Then we will need to give the inaccessible overrides error with respect to this.
                    //
                    OverriddenMember = PossibleOverriddenMember->PProc();

                    OverriddenMemberCompareFlags = CompareFlags;

                    OverriddenMemberConstraintsMatch =
                        CompareConstraints(
                            OverridingMember,
                            OverridingMemberBinding,
                            PossibleOverriddenMember->PProc(),
                            PossibleOverriddenMemberBinding,
                            SymbolFactory);
                }

                continue;
            }

            if (OverriddenMember && OverriddenMemberIsAccessible)
            {
                if (AmbiguousOverriddenMembers.Count() == 0)
                {
                    AmbiguousOverriddenMembers.AddElement(OverriddenMember);
                }

                BCSYM_Proc *AmbiguousProc = PossibleOverriddenMember->PProc();
                AmbiguousOverriddenMembers.AddElement(AmbiguousProc);
            }
            else
            {
                OverriddenMember = PossibleOverriddenMember->PProc();
                OverriddenMemberIsAccessible = true;
                OverriddenMemberCompareFlags = CompareFlags;

                OverriddenMemberConstraintsMatch =
                    CompareConstraints(
                        OverridingMember,
                        OverridingMemberBinding,
                        PossibleOverriddenMember->PProc(),
                        PossibleOverriddenMemberBinding,
                        SymbolFactory);
            }

            // If member of a generic type, then continue looking to detect ambiguous overrides
            // that could potentially occur due to method signature unification, else return this
            // member.
            //
            if (!OverriddenMember->HasGenericParent())
            {
                return OverriddenMemberCompareFlags;
            }
            else
            {
                // Don't dig into bases to find more ambiguous members
                //
                BaseMembersShadowed = true;
            }
        }

        if (PossibleOverriddenMember->IsShadowing() &&
            PossibleOverriddenMemberIsAccessible &&
            !DefinedInMetaData())   // VSWhidbey 553868: Consider shadowed members too for metadata.
        {
            BaseMembersShadowed = true;
        }
    }

    if (!BaseMembersShadowed)
    {
        OverriddenMemberCompareFlags =
            FindMemberOverriddenByOverridingMember(
                OverridingMemberInfo,
                OverridingMemberBinding,
                Base,
                OverriddenMember,
                InAccessibleVirtualMethodInIntermediateClass,
                InAccessibleOverridingFriendMethods,
                OverriddenMemberConstraintsMatch,
                AmbiguousOverriddenMembers,
                SymbolFactory);
    }

    return OverriddenMemberCompareFlags;
}


void
Bindable::VerifyOverridingSemantics
(
    BindableMemberInfo *OverridingMemberInfo,
    BindableMemberInfo *OverriddenMemberInfo,
    DynamicArray<BCSYM_Proc *> &InAccessibleOverridingFriendMethods,
    unsigned CompareFlags,
    bool ConstraintsMatch,
    DynamicArray<BCSYM_Proc *> &AmbiguousOverriddenMembers,
    Symbols *SymbolFactory
)
{
    BCSYM_Proc *OverridingMember = OverridingMemberInfo->Member->PProc();
    BCSYM_Proc *OverriddenMember = OverriddenMemberInfo->Member->PProc();

    VSASSERT( !OverriddenMember ||
              DefinedInMetaData() ||
              OverridingMemberInfo->IsSynthetic() == OverriddenMemberInfo->IsSynthetic(),
                "Non-Synthetic cannot override Synthetic members and viceversa!!");

    VSASSERT(AmbiguousOverriddenMembers.Count() == 0 ||
             AmbiguousOverriddenMembers.Count() >= 2,
                "Ambiguity unexpected when only one overridden member found!!!");

    if (CompareFlags & EQ_Bad)
    {
        // Don't bother reporting an error when comparing an already bad member
        return;
    }

    // Synthetic methods overriding any member should not be verified.
    // Instead errors will be reported when verifying overriding for the entities they were created for.
    //
    // && OverriddenMemberInfo->IsSynthetic()) - Not needed because the above assert condition should never occur.
    //

    // B----ically, synthetic overloads are ignored. Still, get and set property accessors can have a more restictive access
    // than the property. For such cases the verification is forced
    bool forceSynthetics = false;
    if (OverriddenMember &&
        (   (OverriddenMember->IsPropertySet() && OverridingMember->IsPropertySet()) || // checking both members is redundant, but better keep safe
            (OverriddenMember->IsPropertyGet() && OverridingMember->IsPropertyGet()) ) )
    {
        // only those accessors that have a specific more restrictive access flag count here
        forceSynthetics = (OverriddenMember->GetAccess() != OverriddenMember->GetAssociatedPropertyDef()->GetAccess()) ||
            (OverridingMember->GetAccess() != OverridingMember->GetAssociatedPropertyDef()->GetAccess());
    }

    if (!forceSynthetics && OverridingMemberInfo->IsSynthetic())
    {
        return;
    }

    ErrorTable *ErrorLog = CurrentErrorLog(OverridingMember);
    Compiler *CompilerInstance = CurrentCompilerInstance();

    if (AmbiguousOverriddenMembers.Count() > 0)
    {
        // Ignore synthetic members. This error will be reported on the actual non-synthetic member separately.
        //
        if (OverridingMemberInfo->IsSynthetic())
        {
            return;
        }

        StringBuffer AmbiguousSignatures;

        BCSYM_GenericTypeBinding *Base =
            DeriveGenericBindingForMemberReference(
                CurrentContainer(),
                AmbiguousOverriddenMembers.Element(0),
                *SymbolFactory,
                CurrentCompilerHost());

        // "Member '|1.|2' that matches this signature cannot be overridden because the class '|1' contains multiple members with
        // this same name and signature: |3"

        ReportErrorOnSymbol(
            ERRID_AmbiguousOverrides3,
            CurrentErrorLog(OverridingMember),
            OverridingMember,
            AmbiguousOverriddenMembers.Element(0)->GetName(),
            GetQualifiedErrorName(Base),
            GetSigsForAmbiguousOverridingAndImplErrors(AmbiguousOverriddenMembers, AmbiguousSignatures));

        // Don't mark metadata members bad for this reason. They should still be callable
        //
        if (!DefinedInMetaData(CurrentContainer()))
        {
            OverridingMemberInfo->SetIsBad();
        }

        return;
    }

    if (OverriddenMember == NULL)
    {
        // 30284  , "|1 '|2' cannot be declared 'Overrides' because it does not override a |1 in a base |3."
        ReportErrorOnSymbol(
            ERRID_OverrideNotNeeded3,
            CurrentErrorLog(OverridingMember),
            OverridingMember,
            StringOfSymbol(CompilerInstance, OverridingMember),
            OverridingMember->GetName(),
            StringOfSymbol(CompilerInstance, CurrentContainer()));

        return;
    }


    VSASSERT( !(CompareFlags & EQ_Shape), "How can non matching members have been picked ?");

    VSASSERT( DefinedInMetaData() ||
              OverridingMemberInfo->IsSynthetic() == OverriddenMemberInfo->IsSynthetic(),
                    "Synthetic member overriding non-synthetic or viceversa. How can this be ?");

    // If the number of possible errors increases, this array will need to be enlarged.
    ERRID Errors[15];
    unsigned ErrorCount = 0;
    bool IsOverriddenMemberAccessible = true;

    if (!IsAccessible(OverriddenMemberInfo->Member))
    {
        // 31417  , "'|1' cannot override '|2' because it is not accessible in this context."
        Errors[ErrorCount++] = ERRID_CannotOverrideInAccessibleMember;

        IsOverriddenMemberAccessible = false;
    }
    else if (OverriddenMember->IsNotOverridableKeywordUsed())
    {
        // 30267  , "'|1' cannot override '|2' because it is declared 'NotOverridable'."
        Errors[ErrorCount++] = ERRID_CantOverrideNotOverridable2;

    }
    else if (!IsOverridable(OverriddenMember))
    {
        // 31086  , "'|1' cannot override '|2' because it is not declared 'Overridable'."
        Errors[ErrorCount++] = ERRID_CantOverride4;
    }
    else if (InAccessibleOverridingFriendMethods.Count() > 0 )
    {
        // We have a set of inaccessible friend methods that are "inbetween" the overriding method and
        // the overridden method. We need to see if any of these methods actually override overridden method,
        // and if it does, then we need to display an error.

        BCSYM_Proc* OverriddenMemberFinal = NULL;

        if( OverriddenMember->OverriddenProc() != NULL )
        {
            OverriddenMemberFinal = OverriddenMember->OverriddenProc();
        }
        else
        {
            OverriddenMemberFinal = OverriddenMember;
        }

        for( UINT i = 0; i < InAccessibleOverridingFriendMethods.Count(); i += 1 )
        {
            BCSYM_Proc* pProc = InAccessibleOverridingFriendMethods.Element( i );

            if( pProc->OverriddenProc() == OverriddenMemberFinal )
            {
                // 30980  , "'|1' in class '|2' cannot override '|3' in class '|4' because an intermediate class '|5' overrides it but is inaccessible.")

                StringBuffer ErrorReplString1, ErrorReplString2;

                GetBasicRep(OverridingMember, &ErrorReplString1);
                GetBasicRep(
                    OverriddenMember,
                    &ErrorReplString2,
                    NULL,
                    DeriveGenericBindingForMemberReference(
                        CurrentContainer(),
                        OverriddenMember,
                        *SymbolFactory,
                        CurrentCompilerHost()));

                ReportErrorOnSymbol(
                    ERRID_InAccessibleOverridingMethod5,
                    CurrentErrorLog(OverridingMember),
                    OverridingMember,
                    ErrorReplString1.GetString(),
                    OverridingMember->GetParent()->GetName(),
                    ErrorReplString2.GetString(),
                    OverriddenMember->GetParent()->GetName(),
                    pProc->GetParent()->GetName());

                break;
            }
        }
    }
    else
    {
        if (CompareFlags & EQ_Byref)
        {
            // 30398  , "'|1' cannot override '|2' because they differ by a parameter that is marked as 'ByRef'
            // versus 'ByVal'."
            Errors[ErrorCount++] = ERRID_OverrideWithByref2;
        }

        if (CompareFlags & EQ_Optional)
        {
            // 30308  , "'|1' cannot override '|2' because they differ by optional parameters."
            Errors[ErrorCount++] = ERRID_OverrideWithOptional2;
        }

        if (CompareFlags & (EQ_Return | EQ_GenericTypeParamsForReturn))
        {
            // 30437  , "'|1' cannot override '|2' because they differ by their return types."
            Errors[ErrorCount++] = ERRID_InvalidOverrideDueToReturn2;
        }

        if (CompareFlags & EQ_Property)
        {
            // 30362  , "'|1' cannot override '|2' because they differ by 'ReadOnly' or 'WriteOnly'."
            Errors[ErrorCount++] = ERRID_OverridingPropertyKind2;
        }

        if (CompareFlags & EQ_ParamDefaultVal)
        {
            // 30307  , "'|1' cannot override '|2' because they differ by the default values of optional parameters."
            Errors[ErrorCount++] = ERRID_OverrideWithDefault2;
        }

        if (CompareFlags & EQ_ParamArrayVsArray)
        {
            // 30309  , "'|1' cannot override '|2' because they differ by parameters declared 'ParamArray'."
            Errors[ErrorCount++] = ERRID_OverrideWithArrayVsParamArray2;
        }

        if (CompareFlags & EQ_OptionalTypes)
        {
            // 30697  , "'|1' cannot override '|2' because they differ by the types of optional parameters."
            Errors[ErrorCount++] = ERRID_OverrideWithOptionalTypes2;
        }

        if (!ConstraintsMatch)
        {
            // 32077  , "'|1' cannot override '|2' because they differ by their type parameter constraints."
            Errors[ErrorCount++] = ERRID_OverrideWithConstraintMismatch2;
        }
    }

    // No Access mismatch error should be given if the Overridden member is not accessible
    // because asking the user to change the access of the Overriding member to match the
    // access of the overridden member is not correct in this case.
    //
    if (IsOverriddenMemberAccessible)
    {
        // Access mismatch error
        if (OverriddenMember->GetAccess() != OverridingMember->GetAccess())
        {
            if (!
                    (OverriddenMember->GetAccess() == ACCESS_ProtectedFriend &&
                    OverridingMember->GetAccess() == ACCESS_Protected &&
                    OverriddenMember->GetContainingProject() != OverridingMember->GetContainingProject())
                )
            {
                // 30266  , "'|1' cannot override '|2' because they have different access levels."
                Errors[ErrorCount++] = ERRID_BadOverrideAccess2;
            }
        }
        else if (OverriddenMember->GetAccess() == ACCESS_ProtectedFriend &&
                OverridingMember->GetAccess() == ACCESS_ProtectedFriend &&
                OverriddenMember->GetContainingProject() != OverridingMember->GetContainingProject())
        {
            // 31538  , "Method '|1' cannot override '|2' because the access modifier 'Protected Friend' grants additional scope."
            Errors[ErrorCount++] = ERRID_FriendAssemblyBadAccessOverride2;
        }
    }

    if (ErrorCount > 0)
    {
        StringBuffer ErrorReplString1, ErrorReplString2;

        GetBasicRep(OverridingMember, &ErrorReplString1);
        GetBasicRep(
            OverriddenMember,
            &ErrorReplString2,
            NULL,
            DeriveGenericBindingForMemberReference(CurrentContainer(), OverriddenMember, *SymbolFactory, CurrentCompilerHost()));

        unsigned i = 0;
        do
        {
            ReportErrorOnSymbol(
                Errors[i],
                CurrentErrorLog(OverridingMember),
                OverridingMember,
                ErrorReplString1.GetString(),
                ErrorReplString2.GetString());

        } while (++i < ErrorCount);


        // Don't mark metadata members bad for this reason. They should still be callable
        //
        if (!DefinedInMetaData(CurrentContainer()))
        {
            OverridingMemberInfo->SetIsBad();
        }
    }
}


bool
Bindable::IsMustOverrideMethodOrProperty
(
    BCSYM_NamedRoot *Member
)
{
    if (Member->IsProc() && !Member->PProc()->IsUserDefinedOperatorMethod())  // this can't be the right place
    {
        // Only Classes/Structures can have MustOverride methods
        // Interface methods although marked as such are not MustOverride methods in VB sense
        //
        if (Member->GetContainer()->IsClass() &&
            Member->PProc()->IsMustOverrideKeywordUsed())
        {
            return true;
        }
    }

    return false;
}

bool
Bindable::IsOverridingMethodOrProperty
(
    BCSYM_NamedRoot *Member
)
{
    if (Member->IsProc() && !Member->PProc()->IsUserDefinedOperatorMethod())   // this can't be the right place
    {
        // Only Classes/Structures can have MustOverride methods
        // Interface methods although marked as such are not MustOverride methods in VB sense
        if (Member->GetContainer()->IsClass() &&
            Member->PProc()->IsOverridesKeywordUsed())
        {
            return true;
        }
    }

    return false;
}


bool
Bindable::IsAccessible
(
    BCSYM_NamedRoot *Member,
    BCSYM_GenericBinding *MemberBindingContext,
    BCSYM_Container *Context,
    BCSYM *TypeOfAccessingInstance
)
{
    if (!Context)
    {
        Context = CurrentContainer();
    }

    if (!TypeOfAccessingInstance)
    {
        TypeOfAccessingInstance = Context;
    }

    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    return Semantics::IsAccessible(
                Member,
                MemberBindingContext,
                Context,
                TypeOfAccessingInstance,
                SymbolFactory,
                CurrentCompilerHost());
}

bool
Bindable::IsAccessibleOutsideAssembly
(
    BCSYM_NamedRoot *Member,
    bool ConsiderFriend
)
{
    bool bRet = (Member->GetAccess() == ACCESS_Protected ||
            Member->GetAccess() == ACCESS_ProtectedFriend ||
            Member->GetAccess() == ACCESS_Public);

    if( ConsiderFriend && !bRet )
    {
        bRet = Member->GetAccess() == ACCESS_Friend;
    }

    return( bRet );
}

bool Bindable::ResolveOverloadingShouldSkipBadMember(int errid)
{
    switch (errid)
    {
        case ERRID_MetadataMembersAmbiguous3:
        case ERRID_UnreferencedAssembly3:
        case ERRID_UnreferencedAssemblyBase3:
        case ERRID_UnreferencedAssemblyEvent3:
        case ERRID_UnreferencedAssemblyImplements3:
        case ERRID_UnreferencedModule3:
        case ERRID_UnreferencedModuleBase3:
        case ERRID_UnreferencedModuleEvent3:
        case ERRID_UnreferencedModuleImplements3:
        case ERRID_IndirectUnreferencedAssembly4:
        case ERRID_IndirectUnreferencedAssembly3:
        case ERRID_IndirectUnreferencedProject3:
        case ERRID_IndirectUnreferencedProject2:
        case ERRID_SymbolFromUnreferencedProject3:
        case ERRID_AbsentReferenceToPIA1:
        case ERRID_NestingViolatesCLS1:
            return false;
        default:
            return true;
    }
}


void
Bindable::ResolveOverloadingInSameContainer
(
    MemberInfoIter *MemberInfos
)
{
    bool IsMetadataContainer = DefinedInMetaData(CurrentContainer());

    MemberInfos->Reset();

    const bool SkipBadMembers = true;

    while (BindableMemberInfo *MemberInfo = MemberInfos->GetNext(!SkipBadMembers))
    {
        BCSYM_NamedRoot *Member = MemberInfo->Member;
        unsigned CompareFlags;

        if (Member->IsDllDeclare() &&
            Member->HasGenericParent())
        {
            ReportErrorOnSymbol(
                ERRID_DeclaresCantBeInGeneric,
                CurrentErrorLog(Member),
                Member);

            Member->SetIsBad();
            continue;
        }

        if
        (
            Member->IsBad() &&
            ResolveOverloadingShouldSkipBadMember(Member->GetErrid())
        )  // Allow bad metadata members to continue so that
           // other clashes again them are also detected.
        {
            continue;
        }


        if (!IsMetadataContainer)
        {
            if (Member->IsEventDecl())
            {
                //Ok, this is ugly, but it's the simplest way to deal with it.
                //Events that are declared "Event x as delegate" cannot be fully built
                //until the delegate type that they are supposed to be is resolved.
                //Once the delegate is known, we have to go back and set the parameters
                //of the event.
                //
                BuildEvent(Member->PEventDecl());
            }
        }


        // For metadata members:
        //
        // When 2 different kinds of members (method vs. field, etc.) have the same name (different casing -
        // possible in c# - Non CLS compliant), then VB will only allow the most accessible members to be
        // accessed. If no one kind of member is more accessible than the other, then they are marked bad
        // along with ambiguity errors which will be reported upon their use. Also note that we will allow
        // access to all the overloads of the most accessible member, even if the overloads are less accessible
        // or equal in accessibility than some of the ruled out members. So for this purpose, it is easier
        // if we can start with the most accessible member first and see what other members the most accessible
        // member makes inaccessible. So the iterator does the sort based on access too for metadata members.

        MemberInfos->StartPeeking();
        while (BindableMemberInfo *OtherMemberInfo =
                    MemberInfos->PeekAtNextOfSameName())
        {
            BCSYM_NamedRoot *OtherMember = OtherMemberInfo->Member;

            if (MemberInfo->IsSynthetic() || OtherMemberInfo->IsSynthetic())
            {
                ReportSyntheticMemberClashInSameClass(
                    MemberInfo,
                    OtherMemberInfo);
            }
            else
            {
                // Before checking for overloads, we first check to see whether these two methods are related
                // via partial methods. If they are, we do not mark them as overloading each other, but we
                // set up partial links in the two symbols. If they are not, then we fall through to the
                // overloading logic.

                unsigned PartialCompareFlags = 0;

                if (DoSignaturesMatchAsPartialMethods(Member, OtherMember, PartialCompareFlags))
                {
                    // This means that Member and OtherMember could participate in a partial relationship.
                    // But we must validate that this is ok.

                    // First, check if they are both partial declarations. This is bad, because
                    // we only allow one partial declaration.

                    if( Member->PProc()->IsPartialMethodDeclaration() &&
                        OtherMember->PProc()->IsPartialMethodDeclaration() )
                    {
                        ReportErrorOnSymbol(
                            ERRID_OnlyOnePartialMethodAllowed2,
                            CurrentErrorLog( OtherMember ),
                            OtherMember->PProc(),
                            OtherMember->GetName(),
                            OtherMember->GetName()
                            );

                        MarkOverloadedMemberBad( OtherMember );
                        continue;
                    }

                    BCSYM_NamedRoot *Partial = NULL;
                    BCSYM_NamedRoot *Implementing = NULL;

                    if( Member->PProc()->IsPartialMethodDeclaration() )
                    {
                        Partial = Member;
                        Implementing = OtherMember;
                    }
                    else
                    {
                        VSASSERT( OtherMember->PProc()->IsPartialMethodDeclaration(), "Why is this not a partial decl" );
                        Partial = OtherMember;
                        Implementing = Member;
                    }

                    // This method will do further verification of the partial methods, and will
                    // report errors if needed.

                    if( AttemptToLinkPartialMethods( Partial->PProc(), Implementing->PProc(), PartialCompareFlags ) )
                    {
                        continue;
                    }
                }
                else if (CanOverload(Member, OtherMember, CompareFlags))
                {
                    // Note that types can also overload on arity

                    VSASSERT(!Member->IsProc() || OtherMember->IsProc(), "How can a proc overload a non-proc ?");

                    if (Member->IsProc() &&
                        OtherMember->IsProc())
                    {
                        Symbols::SetOverloads(Member->PProc());
                        Symbols::SetOverloads(OtherMember->PProc());

                        if (EmittedNamesAreCompatible(OtherMember, Member))
                        {
                            Symbols::SetEmittedName(OtherMember, Member->GetEmittedName());
                        }
                    }

                    continue;
                }
                else
                {
                    ReportOverloadsError(
                        Member,
                        OtherMember,
                        CompareFlags);
                }
            }

            // Well seems to be no if we want to be consistent on the symbol on
            // which we generate errors. For some errors, we need to generate
            // errors on the first occuring symbol and for others on the
            // second occuring symbol. So we need to check for badness here.
            //
            if
            (
                Member->IsBad() &&
                ResolveOverloadingShouldSkipBadMember(Member->GetErrid())
            )  // Allow bad metadata members to continue so that
               // other clashes again them are also detected.
            {
                break;
            }

        }   // loop over OtherMemberIndex looking for clashes
    }   // loop over all Members in the container
}

void
Bindable::ReportSyntheticMemberClashInSameClass
(
    BindableMemberInfo *MemberInfo,
    BindableMemberInfo *OtherMemberInfo
)
{
    Compiler *CompilerInstance = CurrentCompilerInstance();
    BCSYM_Container *Container = CurrentContainer();

    VSASSERT(MemberInfo->IsSynthetic() || OtherMemberInfo->IsSynthetic(),
                    "How did we get here when none of the clashing members are synthetic?");

    if (IgnoreClashingErrorsOnSyntheticMembers(MemberInfo, OtherMemberInfo))
    {
        return;
    }

    if (MemberInfo->IsSynthetic() && OtherMemberInfo->IsSynthetic())
    {
        // |1 '|2' implicitly defines '|3', which conflicts with a member implicitly declared for |4 '|5' in |6 '|7'.
        ReportErrorAtLocation(
            ERRID_SynthMemberClashesWithSynth7,
            CurrentErrorLog(MemberInfo->NamedContextOfSyntheticMember),
            MemberInfo->GetLocationToReportError(),
            StringOfSymbol(CompilerInstance, MemberInfo->SourceOfSyntheticMember),
            MemberInfo->SourceOfSyntheticMember->GetName(),
            MemberInfo->GetName(),
            StringOfSymbol(CompilerInstance, OtherMemberInfo->SourceOfSyntheticMember),
            OtherMemberInfo->SourceOfSyntheticMember->GetName(),
            StringOfSymbol(CompilerInstance, Container),
            Container->GetName());

    }
    else if (MemberInfo->IsSynthetic())
    {
        // |1 '|2' implicitly defines '|3', which conflicts with a member of the same name in |4 '|5'.
        ReportErrorAtLocation(
            ERRID_SynthMemberClashesWithMember5,
            CurrentErrorLog(MemberInfo->NamedContextOfSyntheticMember),
            MemberInfo->GetLocationToReportError(),
            StringOfSymbol(CompilerInstance, MemberInfo->SourceOfSyntheticMember),
            MemberInfo->SourceOfSyntheticMember->GetName(),
            MemberInfo->GetName(),
            StringOfSymbol(CompilerInstance, Container),
            Container->GetName());

    }
    else  // (OtherMemberInfo->IsSynthetic())
    {
        // |1 '|2' conflicts with a member implicitly declared for |3 '|4' in |5 '|6'.
        ReportErrorOnSymbol(
            ERRID_MemberClashesWithSynth6,
            CurrentErrorLog(MemberInfo->Member),
            MemberInfo->Member,
            StringOfSymbol(CompilerInstance, MemberInfo->Member),
            MemberInfo->GetName(),
            StringOfSymbol(CompilerInstance, OtherMemberInfo->SourceOfSyntheticMember),
            OtherMemberInfo->SourceOfSyntheticMember->GetName(),
            StringOfSymbol(CompilerInstance, Container),
            Container->GetName());
    }

    MarkOverloadedMemberBad(MemberInfo->Member);
    if (MemberInfo->SourceOfSyntheticMember)
    {
        MarkOverloadedMemberBad(MemberInfo->SourceOfSyntheticMember);
    }
}

bool
Bindable::IgnoreClashingErrorsOnSyntheticMembers
(
    BindableMemberInfo *MemberInfo1,
    BindableMemberInfo *MemberInfo2
)
{
    const bool Ignore = true;
    const bool DontIgnore = false;

    // If a synthetic member has the same name as the source member that generated it, then don't report errors on the
    // synthetic member. Anyway the clashing error will be reported on the source member.

    if (MemberInfo1->IsSynthetic() &&
        StringPool::IsEqual(
            MemberInfo1->Member->GetName(),
            MemberInfo1->SourceOfSyntheticMember->GetName()))
    {
        return Ignore;
    }


    // If a synthetic member has the same name as the source member that generated it, then don't report errors on the
    // synthetic member. Anyway the clashing error will be reported on the source member.

    if (MemberInfo2->IsSynthetic() &&
        StringPool::IsEqual(
            MemberInfo2->Member->GetName(),
            MemberInfo2->SourceOfSyntheticMember->GetName()))
    {
        return Ignore;
    }


    // If both the synthetic members that have the same name are generated by the same named source members, then don't report
    // errors on the synthetic members. Anyway the clashing error will be reported on the source symbols.
    if (MemberInfo1->IsSynthetic() &&
        MemberInfo2->IsSynthetic() &&
        StringPool::IsEqual(
            MemberInfo1->SourceOfSyntheticMember->GetName(),
            MemberInfo2->SourceOfSyntheticMember->GetName()))
    {
        return Ignore;
    }


    // For clashes involving metadata members, ignore synthetic clashes if atleast one of them
    // is unbindable. This will help VB apps be more robust with respect to metadata patterns
    // that do not follow VB standards. For an example, see bug VSWhidbey 187219.
    //
    if ((DefinedInMetaData(MemberInfo1->Member->GetContainer()) ||
            DefinedInMetaData(MemberInfo2->Member->GetContainer())) &&
        (MemberInfo1->Member->GetImmediateParent()->PHash()->IsUnBindableChildrenHash() ||
            MemberInfo2->Member->GetImmediateParent()->PHash()->IsUnBindableChildrenHash()))
    {
        return Ignore;
    }


    return DontIgnore;
}

void
Bindable::MarkOverloadedMemberBad
(
    BCSYM_NamedRoot *Member
)
{
    Member->SetIsBad();

    // this is so that semantics knows to ignore these members
    // and move on to the next overloads.
    //
    Symbols::MakeUnbindable(Member);
}

void
Bindable::MarkFakeWindowsRuntimeMemberBad
(
    _In_ BCSYM_NamedRoot *Member,
    Compiler *pCompiler
)
{
    // We land up in this method if we have a fake WinRT method\prop\event that
    // is clashing with a different kind of member. If we have a default property, 
    // instead of making it unbindable, we can simply rename it to something that's not
    // user callable. This is because default properties can be called without invoking
    // their name. So keeping the member bindable and changing it's name lets
    // the user use the indexer directly.
    // For eg: if foo.Item is a method that clashes with a default property called foo.Item then
    // after renaming the indexer to foo.Item$NonUserCallableName$, there won't be a clash
    // anymore and the user can invoke foo(10).The uncallable name should never be emitted to
    // IL since we will simply emit calls to get_Item() which is not being renamed here.
    if (Member->IsProperty() && Member->PProperty()->IsDefault())
    {
        StringBuffer newIndexerName;
        newIndexerName.AppendString(Member->GetName());
        newIndexerName.AppendString(L"$NonUserCallableName$");
        Member->SetName(pCompiler->AddString(&newIndexerName));
    }
    else
    {
        MarkOverloadedMemberBad(Member);
    }
}

void
MarkMetadataProcAsBadOverload
(
    BCSYM_NamedRoot *Member
)
{
    if (Member->IsProc())
    {
        Member->PProc()->SetBadMetadataProcOverload();
    }
}

void
Bindable::ReportOverloadsError
(
    BCSYM_NamedRoot *Member,
    BCSYM_NamedRoot *OtherMember,
    unsigned CompareFlags
)
{
    BCSYM_NamedRoot *SymbolThatOccursFirst = Member;
    BCSYM_NamedRoot *SymbolThatOccursSecond = OtherMember;

    if (CompareFlags & EQ_Bad)
    {
        // Don't bother reporting an error when comparing an already bad member
        return;
    }


    // Metadata member naming allows more flexibility than VB. So we need the handle
    // them specially here. The different cases here are:
    //
    // - Nested Types, Members (fields and Methods) and (Properties and Events) have
    //   distinct binding spaces in metadata due to which for eg. a type and a member
    //   could have the same name. But for CLS Compliance, this kind of name clashing
    //   is not permitted among public members ?
    //
    // - Members could have names which differ in case only as long as both are not public
    //
    if (DefinedInMetaData())
    {
        // Setting the interfering private member as bad prevents such symbols from
        // interfering in other parts of bindable and later on in semantics during
        // name binding.

        // 





        // Note: Microsoft
        // For friend assemblies, we have to allow friend modifiers through, but we will do
        // it only if the current project has declared friends.

        bool bHasFriends = false;

        if( SymbolThatOccursFirst->GetContainingProject() != NULL &&
            SymbolThatOccursFirst->GetContainingProject()->HasFriends() )
        {
            bHasFriends = true;
        }

        if (!IsAccessibleOutsideAssembly(SymbolThatOccursFirst, bHasFriends))
        {
            MarkOverloadedMemberBad(SymbolThatOccursFirst);
            MarkMetadataProcAsBadOverload(SymbolThatOccursFirst);
            return;
        }

        if (!IsAccessibleOutsideAssembly(SymbolThatOccursSecond, bHasFriends))
        {
            MarkOverloadedMemberBad(SymbolThatOccursSecond);
            MarkMetadataProcAsBadOverload(SymbolThatOccursSecond);
            return;
        }

        // If a WinRT type implements a collection interface, we fake up members on that type.
        // If there is a case where a method\property\event has the same name as a different kind of member
        // but has a different signature, then we can land up here. Proceeding further will mark the symbol as 
        // ambiguous. In that case, simply mark the fake member as bad so that the real member is
        // bindable without conflict.
        BCSYM_NamedRoot *pFakeIndexer = NULL;

        if (SymbolThatOccursFirst->IsProc() &&
            SymbolThatOccursFirst->PProc()->IsFakeWindowsRuntimeMember())
        {
            MarkFakeWindowsRuntimeMemberBad(SymbolThatOccursFirst, CurrentCompilerInstance());
            return;
        }
        else if (SymbolThatOccursSecond->IsProc() && 
                 SymbolThatOccursSecond->PProc()->IsFakeWindowsRuntimeMember())
        {
            MarkFakeWindowsRuntimeMemberBad(SymbolThatOccursSecond, CurrentCompilerInstance());
            return;
        }

        VSASSERT(SymbolThatOccursFirst->GetAccess() == ACCESS_Protected ||
                 SymbolThatOccursFirst->GetAccess() == ACCESS_ProtectedFriend ||
                 SymbolThatOccursFirst->GetAccess() == ACCESS_Public ||
                 ( SymbolThatOccursFirst->GetAccess() == ACCESS_Friend && bHasFriends ),
                    "Unexpected access level for metadata member!!!");

        VSASSERT(SymbolThatOccursSecond->GetAccess() == ACCESS_Protected ||
                 SymbolThatOccursFirst->GetAccess() == ACCESS_ProtectedFriend ||
                 SymbolThatOccursSecond->GetAccess() == ACCESS_Public ||
                 ( SymbolThatOccursFirst->GetAccess() == ACCESS_Friend && bHasFriends ),
                    "Unexpected access level for metadata member!!!");

        if (SymbolThatOccursFirst->GetAccess() == SymbolThatOccursSecond->GetAccess())
        {
            // Ambiguous members. Neither is more accessible than the other, so no way
            // to disambiguate.

            SymbolThatOccursFirst->SetIsBad();
            SymbolThatOccursFirst->SetErrid(ERRID_MetadataMembersAmbiguous3);
            MarkMetadataProcAsBadOverload(SymbolThatOccursFirst);

            SymbolThatOccursSecond->SetIsBad();
            SymbolThatOccursSecond->SetErrid(ERRID_MetadataMembersAmbiguous3);
            MarkMetadataProcAsBadOverload(SymbolThatOccursSecond);
        }
        // Prefer the most accessible member
        // Unless we have friends, in which case we prefer protected over friend.
        else if (SymbolThatOccursFirst->GetAccess() < SymbolThatOccursSecond->GetAccess())
        {
            if( bHasFriends &&
                SymbolThatOccursFirst->GetAccess() == ACCESS_Protected &&
                SymbolThatOccursSecond->GetAccess() == ACCESS_Friend )
            {
                MarkOverloadedMemberBad(SymbolThatOccursSecond);
                MarkMetadataProcAsBadOverload(SymbolThatOccursSecond);
            }
            else
            {
                MarkOverloadedMemberBad(SymbolThatOccursFirst);
                MarkMetadataProcAsBadOverload(SymbolThatOccursFirst);
            }
        }
        else
        {
            if( bHasFriends &&
                SymbolThatOccursSecond->GetAccess() == ACCESS_Protected &&
                SymbolThatOccursFirst->GetAccess() == ACCESS_Friend )
            {
                MarkOverloadedMemberBad(SymbolThatOccursFirst);
                MarkMetadataProcAsBadOverload(SymbolThatOccursFirst);
            }
            else
            {
                MarkOverloadedMemberBad(SymbolThatOccursSecond);
                MarkMetadataProcAsBadOverload(SymbolThatOccursSecond);
            }
        }

        return;
    }

    Compiler *CompilerInstance = CurrentCompilerInstance();

    // report the error on the symbol that comes first in user code
    if (SortSymbolsByLocation(&Member, &OtherMember) == 1)
    {
        SymbolThatOccursFirst = OtherMember;
        SymbolThatOccursSecond = Member;
    }

    if (CurrentContainer()->IsEnum())
    {
        // For Enum members, give more specific simpler errors.
        //
        ReportEnumMemberClashError(SymbolThatOccursFirst, SymbolThatOccursSecond);
        return;
    }

    // the basic shapes of the symbols are different - eg: variable vs. method
    if (CompareFlags & EQ_Shape)
    {
        // "'|1' is already declared as '|2' in this |3."
        // Special case - report error on the symbol that occurs later

        StringBuffer BasicRepForSymbol;
        GetBasicRep(SymbolThatOccursFirst, &BasicRepForSymbol);

        ReportErrorOnSymbol(
            ERRID_MultiplyDefinedType3,
            CurrentErrorLog(SymbolThatOccursSecond),
            SymbolThatOccursSecond,
            SymbolThatOccursSecond->GetErrorName(CurrentCompilerInstance()),
            BasicRepForSymbol.GetString(),
            StringOfSymbol(CompilerInstance, CurrentContainer()));

        MarkOverloadedMemberBad(SymbolThatOccursSecond);

        return;
    }

    // If ever the number of bad flags in the Equality enum increases, this array will need to be enlarged.
    ERRID Errors[15];
    unsigned ErrorCount = 0;

    if (CompareFlags & EQ_GenericTypeParams)
    {
        // "'|1' and '|2' cannot overload each other because they could be identical for some type parameter substitutions.
        Errors[ErrorCount++] = ERRID_OverloadsMayUnify2;
    }
    else if ((CompareFlags & (EQ_Match | EQ_ParamName | EQ_Flags | EQ_GenericMethodTypeParams | EQ_GenericMethodTypeParamsForReturn)) == CompareFlags)  // if only these flags are set
    {
        // " '|1' has multiple definitions with identical signatures."
        Errors[ErrorCount++] = ERRID_DuplicateProcDef1;
    }
    else
    {
        if (CompareFlags & EQ_Byref)
        {
            // "'|1' and '|2' cannot overload each other because they differ only by parameters declared 'ByRef'
            // or 'ByVal'."
            Errors[ErrorCount++] = ERRID_OverloadWithByref2;
        }

        if (CompareFlags & (EQ_Return | EQ_GenericTypeParamsForReturn))
        {
            // "'|1' and '|2' cannot overload each other because they differ only by return types."
            Errors[ErrorCount++] = ERRID_OverloadWithReturnType2;
        }

        if (CompareFlags & EQ_Property)
        {
            // "'|1' and '|2' cannot overload each other because they differ only by 'ReadOnly' or 'WriteOnly'."
            Errors[ErrorCount++] = ERRID_OverloadingPropertyKind2;
        }

        if (CompareFlags & EQ_Default)
        {
            // "'|1' has multiple definitions with identical signatures."
            Errors[ErrorCount++] = ERRID_DuplicateProcDef1;
        }

        if (CompareFlags & EQ_ParamArrayVsArray)
        {
            // "'|1' and '|2' cannot overload each other because they differ only by parameters declared 'ParamArray'."
            Errors[ErrorCount++] = ERRID_OverloadWithArrayVsParamArray2;
        }

        if (CompareFlags & EQ_Optional)
        {
            // "'|1' and '|2' cannot overload each other because they differ only by optional parameters."
            Errors[ErrorCount++] = ERRID_OverloadWithOptional2;
        }
        else if (CompareFlags & EQ_ParamDefaultVal)
        {
            // "'|1' and '|2' cannot overload each other because they differ only by the default values of optional parameters."
            Errors[ErrorCount++] = ERRID_OverloadWithDefault2;
        }

        if (CompareFlags & EQ_OptionalTypes)
        {
            // "'|1' and '|2' cannot overload each other because they differ only by the types of optional parameters."
            Errors[ErrorCount++] = ERRID_OverloadWithOptionalTypes2;
        }
    }

    if (ErrorCount)
    {
        // Report all the overloading errors found
        // Report all in this case. That seems bad, but we want to report all errors so that they don't start showing up
        // later when the user starts fix up code to correct the errors.
        //
        // avoid the reporting on solution extension file.
        if (SymbolThatOccursFirst->GetSourceFile()->IsSolutionExtension())
        {
            BCSYM_NamedRoot *temp = SymbolThatOccursFirst;
            SymbolThatOccursFirst = SymbolThatOccursSecond;
            SymbolThatOccursSecond = temp;
        }
        StringBuffer ErrorReplString1, ErrorReplString2;
        GetBasicRep(SymbolThatOccursFirst, &ErrorReplString1);
        GetBasicRep(SymbolThatOccursSecond, &ErrorReplString2);

        for(unsigned i=0; i<ErrorCount; i++)
        {
            // Report error on the symbol that occurs first

            ReportErrorOnSymbol(
                Errors[i],
                CurrentErrorLog(SymbolThatOccursFirst),
                SymbolThatOccursFirst,
                ErrorReplString1.GetString(),
                ErrorReplString2.GetString());
        }

        MarkOverloadedMemberBad(SymbolThatOccursFirst);
    }
}

void
Bindable::ReportEnumMemberClashError
(
    BCSYM_NamedRoot *SymbolThatOccursFirst,
    BCSYM_NamedRoot *SymbolThatOccursSecond
)
{
    VSASSERT(CurrentContainer()->IsEnum(),
                "Non-Enum members unexpected!!!");

    // For Enum member clashes involving the implicitly generated
    // reserved member Value__, give a specific error explaining
    // the problem and also ensure that the error is always given
    // on the user defined member.
    //
    bool ClashWithReservedMember = false;

    if (IsEnumMemberReservedMember(SymbolThatOccursSecond))
    {
        // Swap the first and second members to ensure that we always
        // report the error on the user defined member.
        //
        BCSYM_NamedRoot *TempSymbol = SymbolThatOccursSecond;
        SymbolThatOccursSecond = SymbolThatOccursFirst;
        SymbolThatOccursFirst = TempSymbol;

        ClashWithReservedMember = true;
    }
    else if (IsEnumMemberReservedMember(SymbolThatOccursFirst))
    {
        ClashWithReservedMember = true;
    }


    if (ClashWithReservedMember)
    {
        // "'|1' conflicts with the reserved member by this name that is implicitly declared in all enums."

        ReportErrorOnSymbol(
            ERRID_ClashWithReservedEnumMember1,
            CurrentErrorLog(SymbolThatOccursSecond),
            SymbolThatOccursSecond,
            SymbolThatOccursSecond->GetName());
    }
    else
    {
        // "'|1' is already declared in this |2."

        ReportErrorOnSymbol(
            ERRID_MultiplyDefinedEnumMember2,
            CurrentErrorLog(SymbolThatOccursSecond),
            SymbolThatOccursSecond,
            SymbolThatOccursSecond->GetName(),
            StringOfSymbol(CurrentCompilerInstance(), CurrentContainer()));
    }

    MarkOverloadedMemberBad(SymbolThatOccursSecond);

    return;
}

bool
Bindable::IsEnumMemberReservedMember
(
    BCSYM_NamedRoot *Member
)
{
    VSASSERT( Member->GetContainer() &&
              Member->GetContainer()->IsEnum(),
                    "Non-Enum member unexpected!!!");

    if (Member->IsSpecialName() &&
        Member->IsVariable())
    {
        return true;
    }

    return false;
}

void
Bindable::VerifyClassIsMarkedAbstractIfNeeded
(
    MemberInfoIter *MemberInfos,
    MembersHashTable *MustOverrideMembersInBases
)
{
    BCSYM_Container *Container = CurrentContainer();

    if (!IsClass(Container))
    {
        VSASSERT( (MustOverrideMembersInBases->GetNumberOfEntries() == 0),
                        "How can a non-class have abstract members that need to be overridden by bases?");
        return;
    }

    BCSYM_Class *Class = Container->PClass();

    // if there are no MustOverride Members that need to be overridden or the class is marked as abstract, return
    if (Class->IsMustInherit())
    {
        return;
    }

    MemberInfos->Reset();

    // Report error if there are any mustoverride members in this class
    while (BindableMemberInfo *MemberInfo = MemberInfos->GetNext())
    {
        if (IsMustOverrideMethodOrProperty(MemberInfo->Member))
        {
            // '|1' must be declared 'MustInherit' because it contains methods declared 'MustOverride'."
            ReportErrorOnSymbol(
                ERRID_MustOverridesInClass1,
                CurrentErrorLog(Class),
                Class,
                Class->GetName());

            break;
        }
    }

    // Report error if there are any mustoverride members in the base classes that are yet to be overridden

    // Get all the mustoverride members in the bases that have not yet been overridden

    if ((MustOverrideMembersInBases->GetNumberOfEntries() == 0) ||
        !VBMath::TryMultiply<size_t>(sizeof(BCSYM_NamedRoot), MustOverrideMembersInBases->GetNumberOfEntries()))
    {
        return;
    }

    VBHeapPtr<BCSYM_NamedRoot*> MustOverrideMembers(MustOverrideMembersInBases->GetNumberOfEntries());
    int Count = 0;
    MustOverrideMembersInBases->ResetHashIterator();

    for (BCSYM_NamedRoot *MustOverrideMember = MustOverrideMembersInBases->GetNext();
         MustOverrideMember;
         MustOverrideMember = MustOverrideMembersInBases->GetNext())
    {
        if (IsSynthetic(MustOverrideMember))
            continue;

        if (IsMustOverrideMethodOrProperty(MustOverrideMember)) 
        {
            MustOverrideMembers[Count++] = MustOverrideMember;
        }
    }

    VSASSERT( Count > 0,
                "How can there be no MustOverride non-synthetic member when there is at least one mustoverride member in the list ?");

    // create the error message
    // sort the members in container and file order
    //extern int _cdecl SortSymbolsByLocation(const void *arg1, const void *arg2);

    qsort(MustOverrideMembers, Count, sizeof(BCSYM_NamedRoot *), SortSymbolsByBaseClassesAndLocation);

    StringBuffer ListOfMustOverridesMethodOrProperty;
    StringBuffer MustOverridesEventName;
    BCSYM_GenericBinding *CurrentBaseBindingContext = NULL;
    BCSYM_Container *CurrentBaseContainer = NULL;
    Symbols SymbolFactory(CurrentCompilerInstance(), CurrentAllocator(), NULL, CurrentGenericBindingCache());

    for (int i = 0; i < Count; i++)
    {
        BCSYM_Container *MemberParent = MustOverrideMembers[i]->GetContainer();
        if (CurrentBaseContainer != MemberParent)
        {
            CurrentBaseContainer = MemberParent;

            CurrentBaseBindingContext =
                IsGenericOrHasGenericParent(CurrentBaseContainer) ?
                    DeriveGenericBindingForMemberReference(
                        Class,
                        MustOverrideMembers[i],
                        SymbolFactory,
                        CurrentCompilerHost()) :
                    NULL;
        }

        if (MustOverrideMembers[i]->IsEventDecl())
        {
            GetBasicRep(
                MustOverrideMembers[i],
                &MustOverridesEventName,
                Class,
                CurrentBaseBindingContext);

            ReportErrorOnSymbol(
                ERRID_MustInheritEventNotOverridden,
                CurrentErrorLog(Class),
                Class,
                MustOverridesEventName.GetString(),
                CurrentBaseContainer->PNamedRoot()->GetQualifiedName(true, NULL, true, CurrentBaseBindingContext),
                Class->GetName());

            continue;
        }

        ListOfMustOverridesMethodOrProperty.AppendString(L"\n    ");
        ListOfMustOverridesMethodOrProperty.AppendSTRING(CurrentBaseContainer->PNamedRoot()->GetQualifiedName(true, NULL, true, CurrentBaseBindingContext));
        ListOfMustOverridesMethodOrProperty.AppendChar(L' ');
        ListOfMustOverridesMethodOrProperty.AppendChar(L':');
        ListOfMustOverridesMethodOrProperty.AppendChar(L' ');
        GetBasicRep(
            MustOverrideMembers[i],
            &ListOfMustOverridesMethodOrProperty,
            Class,
            CurrentBaseBindingContext);
    }

    // "Class '|1' must either be declared 'MustInherit' or override the following inherited 'MustOverride'
    // member(s): |2."
    if (ListOfMustOverridesMethodOrProperty.GetStringLength() > 0 )
    {
        ReportErrorOnSymbol(
            ERRID_BaseOnlyClassesMustBeExplicit2,
            CurrentErrorLog(Class),
            Class,
            Class->GetName(),
            ListOfMustOverridesMethodOrProperty.GetString());
    }
    // Note - don't mark the class bad because it can still be used in all contexts except creation of new instances
}


// to be used to sort by base classes, not interfaces!!
int __cdecl SortSymbolsByBaseClassesAndLocation
(
    const void *arg1,
    const void *arg2
)
{
    BCSYM_NamedRoot *Member1 = *(BCSYM_NamedRoot **)arg1;
    BCSYM_NamedRoot *Member2 = *(BCSYM_NamedRoot **)arg2;

    VSASSERT( Member1 && Member2, "How can the symbols to be compared be NULL ?");

    VSASSERT( Member1->GetContainer() &&
              Member1->GetContainer()->IsClass() &&
              Member2->GetContainer() &&
              Member2->GetContainer()->IsClass(),
                    "This function to be used to sort by base classes only!! How can non-class containers show up ?");

    BCSYM_Class *Class1 = Member1->GetContainer()->PClass();
    BCSYM_Class *Class2 = Member2->GetContainer()->PClass();

    if (Class1 == Class2)
    {
        // secondary key to sort by is location
        return SortSymbolsByLocation(arg1, arg2);
    }

    // if a member is in a more derived class than the other, then this member is considered smaller
    if (TypeHelpers::IsOrInheritsFrom(Class1, Class2))
    {
        return -1;
    }

    VSASSERT( TypeHelpers::IsOrInheritsFrom(Class2, Class1),
                "How can the 2 classes here not have an inheritance relationship ?");

    return 1;
}

Bindable::ShadowingOverloadingOverridingInfo
Bindable::IsOverloadingOverridingOrShadowing
(
    BCSYM_NamedRoot *Member
)
{
    if (Member->IsBad())
    {
        return (ShadowingOverloadingOverridingInfo)0;
    }

    unsigned MemberInfo = 0;

    BCSYM_Container *Container = Member->GetContainer();

    if (Member->IsProc() &&
        Member->PProc()->IsOverloadsKeywordUsed())
    {
        MemberInfo |= Overloads;
    }

    if (Member->IsShadowsKeywordUsed())
    {
        MemberInfo |= Shadows;
    }


    // Can a member override and also shadow ? No.
    // So, if only Overrides is specified, then we should treat it as though it has an
    // implicit overloads ?
    if (Member->IsProc() && Member->PProc()->IsOverridesKeywordUsed())
    {
        MemberInfo |= Overrides |
                      Overloads;
    }

    // if no specifier is specified, then we implicitly shadow
    if (MemberInfo == 0)
    {
        MemberInfo = Shadows |
                    ImplicitShadows;
    }


    VSASSERT( (MemberInfo == Overloads) ||
              (MemberInfo == (Overloads | Overrides)) ||
              (MemberInfo == Shadows) ||
              (MemberInfo == (Shadows | ImplicitShadows)),
                    "How can this be ? Declared should have got rid of the faulty cases!" );


    return (ShadowingOverloadingOverridingInfo)MemberInfo;
}

//*************************************************************************************************
// Custom Name Binding Functions
//
// Every container has two hashes, one for unbindable members and one for normal members.
// At times, Bindable must consider all members in a container (even across partial types), so
// these two hashes must be 'merged'.  The functions below wrap SimpleBind and GetNextOfSameName
// and effectively merge all members of a container together before performing the name lookup.
//
// There are three levels of abstraction happening in the wrappers:
//
//    1)  Merge normal hash and unbindable hash into set of members T.
//    2)  Merge all sets T of partial types into set of members U.
//    3)  Find set W, where W is a subset of U, composed of names that overload each other.
//
// Name binding is then performed on set W.
//
// NOTE:  Calculation of set W considers the fact that two members may have the same name yet
// cannot overload each other.  The function NamesCanOverload essentially chooses the correct
// members for set W.
//
// Have placed these here and not in BCSYM_Container because except for Bindable we don't want any
// other part of the compiler to ever use these even accidentally.  If ever these are required by
// other parts of the compiler, then place them in BCSYM_Container.
//
//*************************************************************************************************

bool
Bindable::NamesCanOverload
(
    BCSYM_NamedRoot *Member1,
    BCSYM_NamedRoot *Member2
)
{
    VSASSERT(StringPool::IsEqual(Member1->GetName(), Member2->GetName()), "names must be equal!");

    // Determines if Member1 and Member2 belong to the same set W (see above).
    //
    // Operators complicate matters in that two symbols with the same name may not actually be
    // able to overload each other.
    //
    // Given two symbols with the same name, the rules are:
    //
    //     1. Non-operators can overload non-operators.
    //     2. Operators can overload operators given identical operator kinds.
    //      2a. The exception to rule 2 concerns conversion operators.  Conversion operators
    //          can overload other conversion operators.
    //     3. Otherwise, the two members cannot overload each other.
    //
    // This function detects these conditions.
    //
    // Rule 2 exists to stop OperatorNegate '-' from overloading OperatorMinus '-'.
    // Rule 2a exists to allow OperatorWiden to overload OperatorNarrow.
    // Rule 3 exists to stop Function 'Not' from overloading OperatorNot.

    bool Member1IsOperator = Member1->IsUserDefinedOperator();
    bool Member2IsOperator = Member2->IsUserDefinedOperator();

    if (Member1IsOperator || Member2IsOperator)
    {
        if (Member1IsOperator && Member2IsOperator)
        {
            UserDefinedOperators Operator1 = Member1->PUserDefinedOperator()->GetOperator();
            UserDefinedOperators Operator2 = Member2->PUserDefinedOperator()->GetOperator();

            return
                (Operator1 == Operator2) ||
                (IsConversionOperator(Operator1) && IsConversionOperator(Operator2));
        }

        return false;
    }
    else
    {
        return true;
    }
}

//
// Performs a simple bind on set U (see above).
//
BCSYM_NamedRoot *
Bindable::SimpleBindIncludingUnBindableMembers_Helper
(
    BCSYM_Container *Container,
    _In_z_ STRING *Name
)
{
    BCSYM_NamedRoot *Member = NULL;

    BCSYM_Hash *Hash = Container->GetHash();
    if (Hash)
    {
        Member = Hash->SimpleBind(Name);
    }

    if (!Member)
    {
        Hash = Container->GetUnBindableChildrenHash();
        if (Hash)
        {
            Member = Hash->SimpleBind(Name);

            // Ignore partial types tied to main types
            //
            if (Member &&
                Member->IsPartialTypeAndHasMainType())
            {
                return GetNextOfSameNameIncludingUnBindableMembers_Helper(Member);
            }
        }
    }

    return Member;
}

//
// Gets next member with the same name in set U (see above).
//
BCSYM_NamedRoot *
Bindable::GetNextOfSameNameIncludingUnBindableMembers_Helper
(
    BCSYM_NamedRoot *Member
)
{
    BCSYM_NamedRoot *PrevMember = Member;
    BCSYM_NamedRoot *NextMember;

    do
    {
        NextMember = PrevMember->GetNextOfSameName();

        // If not already looking at the UnBindableHash
        if (!NextMember &&
            !PrevMember->GetImmediateParent()->PHash()->IsUnBindableChildrenHash())
        {
            BCSYM_Hash *Hash = PrevMember->GetContainer()->GetUnBindableChildrenHash();
            if (Hash)
            {
                NextMember = Hash->SimpleBind(PrevMember->GetName());
            }
        }
    }
    // Ignore Partial Types with associated main types
    //
    while (NextMember &&
           NextMember->IsPartialTypeAndHasMainType() &&
           (PrevMember = NextMember));

    return NextMember;
}

//
// Performs a simple bind on set W (see above).
//
BCSYM_NamedRoot *
Bindable::SimpleBindIncludingUnBindableMembers
(
    BCSYM_Container *Container,
    BCSYM_NamedRoot *SourceMember
)
{
    STRING *Name = SourceMember->GetName();
    BCSYM_NamedRoot *Member = SimpleBindIncludingUnBindableMembers_Helper(Container, Name);

    while (Member && !NamesCanOverload(SourceMember, Member))
    {
        Member = GetNextOfSameNameIncludingUnBindableMembers_Helper(Member);
    }

    return Member;
}

//
// Gets next member with the same name in set W (see above).
//
BCSYM_NamedRoot *
Bindable::GetNextOfSameNameIncludingUnBindableMembers
(
    BCSYM_NamedRoot *Member
)
{
    BCSYM_NamedRoot *NextMember = Member;
    do
    {
        NextMember = GetNextOfSameNameIncludingUnBindableMembers_Helper(NextMember);
    }
    while (NextMember && !NamesCanOverload(Member, NextMember));

    return NextMember;
}

//*************************************************************************************************
// END Custom Name Binding Functions
//*************************************************************************************************
