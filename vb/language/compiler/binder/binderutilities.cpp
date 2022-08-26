//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Contains utilities and iterators specific to Bindable.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------Start of BindableMemberInfo implementation-----------------------------

Bindable::BindableMemberInfo::BindableMemberInfo(BCSYM_NamedRoot *Member)
{
    this->Member = Member;

    if (Member)
    {
        Bindable::IsSynthetic(
            Member,
            &SourceOfSyntheticMember,
            &NamedContextOfSyntheticMember,
            &syntheticContext);

        ShadowingInfo =
            Bindable::IsOverloadingOverridingOrShadowing(Member);
    }
    else
    {
        SourceOfSyntheticMember = NULL;
        NamedContextOfSyntheticMember = NULL;
    }
}

STRING * Bindable::BindableMemberInfo::GetName()
{
    if (Member)
    {
        return Member->GetName();
    }

    return NULL;
}

bool
Bindable::BindableMemberInfo::IsSynthetic()
{
    return (SourceOfSyntheticMember != NULL);
}

void
Bindable::BindableMemberInfo::SetIsBad()
{
    if (!Member)
        return;

    if (SourceOfSyntheticMember)
    {
        SourceOfSyntheticMember->SetIsBad();
    }

    Member->SetIsBad();
}

Location*
Bindable::BindableMemberInfo::GetLocationToReportError()
{
    if (SourceOfSyntheticMember)
    {
        if (syntheticContext == HandlesOfBaseWithEvents)
        {
            if (Member->IsProperty())
            {
                if (Member->PProperty()->CreatedByHandlesClause())
                {
                    return Member->PProperty()->CreatedByHandlesClause()->GetLocationOfWithEventsVar();
                }
            }
            else
            {
                VSASSERT( Member->IsSyntheticMethod(),
                                "What other members can be synthesized for handling a base withevents ?");
                VSASSERT( Member->PProc()->GetAssociatedPropertyDef(),
                            "what other synthetic method besides a Propety_Get and a Property_Set can be synthesized for handling a base withevents?");

                if (Member->PProc()->GetAssociatedPropertyDef()->CreatedByHandlesClause())
                {
                    return Member->PProc()->GetAssociatedPropertyDef()->
                                CreatedByHandlesClause()->GetLocationOfWithEventsVar();
                }
            }
        }

        return SourceOfSyntheticMember->GetLocation();
    }

    return Member->GetLocation();
}


//-------------------End of BindableMemberInfo implementation-----------------------------


//-------------------Start of MemberInfoIter implementation-------------------------------

int __cdecl SortSymbolsByNameAndOperatorAndLocation
(
    const void *arg1,
    const void *arg2
)
{
    // 


    BCSYM_NamedRoot *Member1 = *(BCSYM_NamedRoot **)arg1;
    BCSYM_NamedRoot *Member2 = *(BCSYM_NamedRoot **)arg2;

    VSASSERT(Member1 && Member2, "How can the symbols to be compared be NULL?");

    // First sort by name.
    int i = SortSymbolsByName(arg1, arg2);

    // Now sort by operator kind.
    if (i == 0)
    {
        bool Member1IsOperator = IsUserDefinedOperator(Member1);
        bool Member2IsOperator = IsUserDefinedOperator(Member2);

        if (Member1IsOperator)
        {
            if (Member2IsOperator)
            {
                UserDefinedOperators Operator1 = Member1->PUserDefinedOperator()->GetOperator();
                UserDefinedOperators Operator2 = Member2->PUserDefinedOperator()->GetOperator();

                i = (Operator1 == Operator2) ? 0 : (Operator1 > Operator2) ? 1 : -1;
            }
            else
            {
                i = 1;
            }
        }
        else if (Member2IsOperator)
        {
            i = -1;
        }
    }

    if (i == 0)
    {
        i = SortSymbolsByLocation(arg1, arg2);
    }

    return i;
}

int __cdecl SortSymbolsByNameAndOperatorAndExternalAccess
(
    const void *arg1,
    const void *arg2
)
{
    BCSYM_NamedRoot *Member1 = *(BCSYM_NamedRoot **)arg1;
    BCSYM_NamedRoot *Member2 = *(BCSYM_NamedRoot **)arg2;

    VSASSERT(Member1 && Member2, "How can the symbols be compared be NULL?");

    // We add locations to all members imported from COM typelibs, since we may need the ordering data later if we
    // need to embed No-PIA local types. In other situations, we expect that metadata members do not have locations.
    AssertIfTrue(Member1->HasLocation() && !Member1->GetContainingProject()->GetAssemblyIdentity()->IsImportedFromCOMTypeLib());
    AssertIfTrue(Member2->HasLocation() && !Member2->GetContainingProject()->GetAssemblyIdentity()->IsImportedFromCOMTypeLib());

    // First sort by name.
    int i = SortSymbolsByNameAndOperatorAndLocation(arg1, arg2);

    // Now sort by access external to this assembly.
    if (i == 0)
    {
#pragma prefast(suppress: 26010 26011, "If access is outside of the array limits, then the whole symbol is busted")
        i = MapAccessToAccessOutsideAssembly[Member1->GetAccess()] - MapAccessToAccessOutsideAssembly[Member2->GetAccess()];
    }

    return i;
}

int __cdecl SortSymbolsByNameAndExternalAccess
(
    const void *arg1,
    const void *arg2
)
{
    BCSYM_NamedRoot *Member1 = *(BCSYM_NamedRoot **)arg1;
    BCSYM_NamedRoot *Member2 = *(BCSYM_NamedRoot **)arg2;

    VSASSERT(Member1 && Member2, "How can the symbols be compared be NULL?");

    // We add locations to all members imported from COM typelibs, since we may need the ordering data later if we
    // need to embed No-PIA local types. In other situations, we expect that metadata members do not have locations.
    // Exception: as per the comment in MetaData::ImportChildren, we also get locations for members from PIAs.
    AssertIfTrue(Member1->HasLocation() && !Member1->GetContainingProject()->GetAssemblyIdentity()->IsImportedFromCOMTypeLib() && !Member1->GetContainingProject()->GetAssemblyIdentity()->IsPrimaryInteropAssembly());
    AssertIfTrue(Member2->HasLocation() && !Member2->GetContainingProject()->GetAssemblyIdentity()->IsImportedFromCOMTypeLib() && !Member2->GetContainingProject()->GetAssemblyIdentity()->IsPrimaryInteropAssembly());


    // First sort by name.
    int i = SortSymbolsByName(arg1, arg2);

    // Now sort by access external to this assembly.
    if (i == 0)
    {
#pragma prefast(suppress: 26010 26011, "If access is outside of the array limits, then the whole symbol is busted")
        i = MapAccessToAccessOutsideAssembly[Member1->GetAccess()] - MapAccessToAccessOutsideAssembly[Member2->GetAccess()];
    }

    return i;
}

Bindable::MemberInfoIter::MemberInfoIter
(
    BCSYM_Container *Container,
    Compiler *CompilerInstance
)
{
    // Resolve overloading in current container
    BCITER_CHILD_ALL Members(Container);
    BCSYM_NamedRoot *Member;
    unsigned MemberCount = Members.GetCount();

    m_CompilerInstance = CompilerInstance;

    BindableMemberInfo *MemberInfos = CreateMemberInfosForMembers(MemberCount);
    ArrayBuffer<BCSYM_NamedRoot*> MembersArray(MemberCount);

    // Create a MemberInfo array and store the info. about whether
    // shadowing/overloading/overriding were specified on each member.
    // Why do we need this info. Although it is present on each of the
    // Members? Because we are going to change this information based
    // on the errors that occur and we don't want these changes reflected
    // in the Members themselves. Also this make it easy to avoid redundant
    // computing for other info that is needed commonly for overriding,
    // overloading and shadowing. Eg. of common information - is a member synthetic
    // and if so what is its source symbol?
    //

    // The sorting might seem slow here, but since this is used across several
    // tasks in shadowing, overloading, overriding, by keeping them sorted
    // and also reusing this information we recoup the extra time spent sorting here.
    //
    unsigned CurrentMemberIndex = 0;
    for(Member = Members.GetNext();
        Member;
        Member = Members.GetNext())
    {
#pragma prefast(suppress: 26017, "Number of loop iterations is constrained by number of elements")
        MembersArray[CurrentMemberIndex++] = Member;

        VSASSERT( CurrentMemberIndex <= MemberCount,
                        "How did more members show up than allocated space ?!!");
    }

    MemberCount = CurrentMemberIndex;

    if (Bindable::DefinedInMetaData(Container) && !TypeHelpers::IsEmbeddableInteropType(Container))
    {
        // For metadata container, sort by access too. This simplifies the solution for Bug VSWhidbey 401153.
        //
        // The reason this simplied the solution for Bug VSWhibdye 401153 is as follows:
        //
        // When 2 different kinds of members (method vs. field, etc.) have the same name (different casing -
        // possible in c# - Non CLS compliant), then VB will only allow the most accessible members to be
        // accessed. If no one kind of member is more accessible than the other, then they are marked bad
        // along with ambiguity errors which will be reported upon their use. Also note that we will allow
        // access to all the overloads of the most accessible member, even if the overloads are less accessible
        // or equal in accessibility than some of the ruled out members. So for this purpose, it is easier
        // if we can start with the most accessible member first and see what other members the most accessible
        // member makes inaccessible.

        // Sort by name and then location
        qsort(
            MembersArray.GetData(),
            MemberCount,
            sizeof(BCSYM_NamedRoot *),
            Container->HasUserDefinedOperators() ?         // Perf tweak:  consider operator kind in the sort only if the container has operators.
                SortSymbolsByNameAndOperatorAndExternalAccess :
                SortSymbolsByNameAndExternalAccess);
    }
    else
    {
        // Sort by name and then location
        qsort(
            MembersArray.GetData(),
            MemberCount,
            sizeof(BCSYM_NamedRoot *),
            Container->HasUserDefinedOperators() ?         // Perf tweak:  consider operator kind in the sort only if the container has operators.
                SortSymbolsByNameAndOperatorAndLocation :
                SortSymbolsByNameAndLocation);
    }

    for(CurrentMemberIndex = 0;
        CurrentMemberIndex < MemberCount;
        CurrentMemberIndex++)
    {
        BindableMemberInfo *MemberInfo =
            new((void*)(&MemberInfos[CurrentMemberIndex])) BindableMemberInfo(MembersArray[CurrentMemberIndex]);
    }


    m_MemberInfos = MemberInfos;
    m_NumberOfMembers = MemberCount;

    Reset();
}

Bindable::BindableMemberInfo*
Bindable::MemberInfoIter::GetNext
(
    bool fSkipBad
)
{
    BindableMemberInfo *MemberInfo = GetNextMemberInfo(fSkipBad);

    if (MemberInfo)
    {
        m_CurrentMemberName = MemberInfo->GetName();
        m_CurrentMemberInfo = MemberInfo;
    }

    return MemberInfo;
}

Bindable::BindableMemberInfo*
Bindable::MemberInfoIter::GetNextOfSameName
(
    bool fSkipBad
)
{
    return GetNextMemberInfoOfSameName(m_NextMemberIndex, fSkipBad);
}

void
Bindable::MemberInfoIter::StartPeeking()
{
    m_NextPeekIndex = m_NextMemberIndex;
}

Bindable::BindableMemberInfo*
Bindable::MemberInfoIter::PeekAtNextOfSameName
(
    bool fSkipBad
)
{
    return GetNextMemberInfoOfSameName(m_NextPeekIndex, fSkipBad);
}

void
Bindable::MemberInfoIter::Reset()
{
    m_NextMemberIndex = 0;
    m_NextPeekIndex = 0;
    m_CurrentMemberName = NULL;
    m_CurrentMemberInfo = NULL;
}

Bindable::MemberInfoIter::~MemberInfoIter()
{
    FreeMemberInfos();
}

Bindable::BindableMemberInfo*
Bindable::MemberInfoIter::CreateMemberInfosForMembers
(
    unsigned NumberOfMembers
)
{
    return VBAllocator::AllocateArray<BindableMemberInfo>(NumberOfMembers);
}

void
Bindable::MemberInfoIter::FreeMemberInfos()
{
    VBFree(m_MemberInfos);
}


Bindable::BindableMemberInfo*
Bindable::MemberInfoIter::GetNextMemberInfo
(
    bool fSkipBad
)
{
    BindableMemberInfo *MemberInfo;

    do
    {
        if (m_NextMemberIndex >= m_NumberOfMembers)
        {
            return NULL;
        }

        MemberInfo = &m_MemberInfos[m_NextMemberIndex++];

    } while (fSkipBad && MemberInfo->Member->IsBad());

    return MemberInfo;
}

// Assumes MemberInfos are sorted by name
Bindable::BindableMemberInfo*
Bindable::MemberInfoIter::GetNextMemberInfoOfSameName
(
    unsigned &CurrentMemberIndex,
    bool fSkipBad
)
{
    BindableMemberInfo *MemberInfo;

    do
    {
        if (CurrentMemberIndex >= m_NumberOfMembers)
        {
            return NULL;
        }

        MemberInfo = &m_MemberInfos[CurrentMemberIndex];

        if (!StringPool::IsEqual(MemberInfo->GetName(), m_CurrentMemberName))
        {
            return NULL;
        }

        if (m_CurrentMemberInfo && !NamesCanOverload(MemberInfo->Member, m_CurrentMemberInfo->Member))
        {
            return NULL;
        }

        CurrentMemberIndex++;   // NOTE: this is changing iterator state.

    } while (fSkipBad && MemberInfo->Member->IsBad());

    return MemberInfo;
}

//-------------------End of MemberInfoIter implementation-------------------------------

//-------------------Start of BasesIter Implemenation-----------------------------------

Bindable::BasesIter::BasesIter
(
    BCSYM_Container *Container
)
{
    m_ClassOrInterface = Container;
    m_IteratorStarted = false;
}

BCSYM_Container*
Bindable::BasesIter::GetNextBase
(
    BCSYM_GenericTypeBinding **BaseGenericBinding
)
{
    BCSYM *Base;

    if (!m_IteratorStarted)
    {
        m_IteratorStarted = true;
        Base = GetFirst();
    }
    else
    {
        Base = GetNext();
    }

    if (BaseGenericBinding)
    {
        if (Base && Base->IsGenericTypeBinding())
        {
            *BaseGenericBinding = Base->PGenericTypeBinding();
        }
        else
        {
            *BaseGenericBinding = NULL;
        }
    }

    return Base->PContainer();
}

BCSYM*
Bindable::BasesIter::GetFirst()
{
    BCSYM *Base = NULL;
    m_LinkToCurrentBase = NULL;

    if (m_ClassOrInterface->IsClass())
    {
        Base = m_ClassOrInterface->PClass()->GetBaseClass();

        if (Base &&
            (Base->IsBad() || !Base->IsClass()))
        {
            Base = NULL;
        }
    }
    else if (m_ClassOrInterface->IsInterface())
    {
        m_LinkToCurrentBase = m_ClassOrInterface->PInterface()->GetFirstImplements();

        Base = GetBaseInterface(m_LinkToCurrentBase);
    }

    return Base;
}

BCSYM*
Bindable::BasesIter::GetNext()
{
    if (m_LinkToCurrentBase)
    {
        m_LinkToCurrentBase = m_LinkToCurrentBase->GetNext();
        return GetBaseInterface(m_LinkToCurrentBase);
    }

    return NULL;
}

BCSYM*
Bindable::BasesIter::GetBaseInterface
(
    BCSYM_Implements *&LinkToBase
)
{
    BCSYM *Base = NULL;

    do
    {
        do
        {
            if (!LinkToBase)
            {
                return NULL;
            }

            if (!LinkToBase->IsBad())
            {
                break;
            }

            LinkToBase = LinkToBase->GetNext();

        } while(true);

        Base = LinkToBase->GetInterfaceIfExists();

        if (Base && !Base->IsBad())
        {
            break;
        }

        LinkToBase = LinkToBase->GetNext();

    } while(true);

    return Base;
}

//-------------------End of BasesIter Implemenation-----------------------------------

//-------------------Start of DelayedCheckManager Implemenation-----------------------------------

void
Bindable::DelayedCheckManager::CompleteDelayedObsoleteChecks()
{
    CDoubleListForwardIter<DelayedObsoleteCheckInfo> DelayedObsoleteChecks(&m_ListOfDelayedObsoleteChecks);

    while (DelayedObsoleteCheckInfo *DelayedCheck = DelayedObsoleteChecks.Next())
    {
        bool performObsoleteCheck = true;

        if (DelayedCheck->m_ContextOfSymbolUsage !=NULL)
        {

            performObsoleteCheck = 
                !ObsoleteChecker::IsObsoleteOrHasObsoleteContainer(DelayedCheck->m_ContextOfSymbolUsage, true);
        }
            
        if (performObsoleteCheck)
        {
            ObsoleteChecker::CheckObsoleteAfterEnsuringAttributesAre----ed(
                DelayedCheck->m_PossibleObsoleteSymbol,
                &DelayedCheck->m_LocationOfSymbolUsage,
                m_ContainerContext,
                DelayedCheck->m_ErrorLogToReportObsoleteError);
        }
    }
}

void
Bindable::DelayedCheckManager::CompleteDelayedResultTypeChecks()
{
    CDoubleListForwardIter<DelayedReturnTypeCheckInfo> DelayedResultTypeChecks(&m_ListOfDelayedReturnTypeChecks);

    while (DelayedReturnTypeCheckInfo *DelayedCheck = DelayedResultTypeChecks.Next())
    {
        Symbols ScratchSymbolCreator(m_Compiler, &m_Allocator, NULL);

        Semantics::CheckResultTypeAccessible(
                DelayedCheck->m_MemberToCheck,
                &DelayedCheck->m_LocationOfSymbolUsage,
                m_ContainerContext,
                ScratchSymbolCreator,
                m_CompilerHost,
                DelayedCheck->m_ErrorLogToReportObsoleteError);
    }
}

//-------------------End of DelayedCheckManager Implemenation-----------------------------------

//-------------------Start of RelationalOperatorPool Implementation--------------------------------

Bindable::OperatorInfo *
Bindable::RelationalOperatorPool::Search
(
    CSingleList<OperatorInfo> &SearchList,
    BCSYM_UserDefinedOperator *SymbolToMatch
)
{
    CSingleListIter<OperatorInfo> Iter(&SearchList);
    while (OperatorInfo *Current = Iter.Next())
    {
        // 



        unsigned CompareFlags = BCSYM::CompareProcs(
            Current->Symbol, 
            (GenericBinding *)NULL, 
            SymbolToMatch, 
            (GenericBinding *)NULL, 
            NULL);

        if (!(CompareFlags & (EQ_Shape | EQ_Return | EQ_GenericTypeParams | EQ_GenericTypeParamsForReturn)))
        {
            return Current;
        }
    }

    return NULL;
}

void
Bindable::RelationalOperatorPool::AddMatchingOperator
(
    CSingleList<OperatorInfo> &AddList,
    CSingleList<OperatorInfo> &SearchList,
    BCSYM_UserDefinedOperator *Symbol
)
{
    OperatorInfo *Found = Search(SearchList, Symbol);

    if (Found)
    {
        // 
        SearchList.Remove(Found);
    }
    else
    {
        Bindable::OperatorInfo *NewEntry = new(m_Allocator) OperatorInfo;
        NewEntry->Symbol = Symbol;
        AddList.InsertFirst(NewEntry);
    }
}

void
Bindable::RelationalOperatorPool::Add
(
    UserDefinedOperators Operator,
    BCSYM_UserDefinedOperator *Symbol
)
{
    VSASSERT(!IsCollated, "Adding to the pool after it has already been collated!");

    switch (Operator)
    {
        case OperatorIsTrue:       AddMatchingOperator(m_IsTrue, m_IsFalse, Symbol); break;
        case OperatorIsFalse:      AddMatchingOperator(m_IsFalse, m_IsTrue, Symbol); break;
        case OperatorEqual:        AddMatchingOperator(m_Equal, m_NotEqual, Symbol); break;
        case OperatorNotEqual:     AddMatchingOperator(m_NotEqual, m_Equal, Symbol); break;
        case OperatorLess:         AddMatchingOperator(m_Less, m_Greater, Symbol); break;
        case OperatorLessEqual:    AddMatchingOperator(m_LessEqual, m_GreaterEqual, Symbol); break;
        case OperatorGreaterEqual: AddMatchingOperator(m_GreaterEqual, m_LessEqual, Symbol); break;
        case OperatorGreater:      AddMatchingOperator(m_Greater, m_Less, Symbol); break;

#if DEBUG
        case OperatorUNDEF: __fallthrough;
        case OperatorMAXVALID: __fallthrough;
        case OperatorNot2: __fallthrough;
        case OperatorOr2: __fallthrough;
        case OperatorAnd2: __fallthrough;
        case OperatorShiftLeft2: __fallthrough;
        case OperatorShiftRight2: __fallthrough;
        case OperatorMAX:
            VSFAIL("unexpected operator kind");
            __fallthrough;
#endif
        default:
            // do nothing
            break;
    }
}

CSingleList<Bindable::OperatorInfo> *
Bindable::RelationalOperatorPool::CollateUnmatchedOperators()
{
    VSASSERT(!IsCollated, "Collating the pool after it has already been collated!");
#if DEBUG
    IsCollated = true;
#endif

    CSingleList<OperatorInfo> *Result = new(m_Allocator) CSingleList<OperatorInfo>;

    Result->Splice(&m_IsTrue);
    Result->Splice(&m_IsFalse);
    Result->Splice(&m_Equal);
    Result->Splice(&m_NotEqual);
    Result->Splice(&m_Less);
    Result->Splice(&m_LessEqual);
    Result->Splice(&m_GreaterEqual);
    Result->Splice(&m_Greater);

    return Result;
}

//-------------------End of RelationalOperatorPool Implemenation-----------------------------------

//-------------------Start of MemberBindingHashTable Implementation--------------------------------

BCSYM_GenericTypeBinding*
Bindable::MemberBindingHashTable::GetUnifyingMemberBinding
(
    BCSYM_NamedRoot *Member,
    BCSYM_GenericTypeBinding *Binding,
    Symbols *SymbolFactory,
    Compiler *CompilerInstance
)
{
    for(BCSYM_GenericTypeBinding **BindingInHash = m_Hash.HashFind(Member);
        BindingInHash;
        BindingInHash = m_Hash.FindNext())
    {
        if (Binding == *BindingInHash ||
            (Binding &&
                *BindingInHash &&
                Bindable::CanInterfacesUnify(Binding, *BindingInHash, SymbolFactory, CompilerInstance)))
        {
            return *BindingInHash;
        }
    }

    return NULL;
}

//-------------------End of MemberBindingHashTable Implementation--------------------------------
