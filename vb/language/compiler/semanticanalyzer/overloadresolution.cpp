//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB overload resolution semantic analysis.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"


Declaration *
Semantics::GetNextOverloadForProcedureConsideringBaseClasses
(
    Declaration * OverloadedProcedure,
    Type * AccessingInstanceType
)
{
    Declaration * ret = OverloadedProcedure->GetNextOverload();

    if (! ret)
    {
        Location loc;
        loc.Invalidate();
        bool ignored = false;

        ret = FindMoreOverloadedProcedures(OverloadedProcedure, AccessingInstanceType, loc, ignored);
    }

    return ret;
}

bool
Semantics::HasAccessibleSharedOverload
(
    BCSYM_NamedRoot * pDecl,
    GenericBinding * pBinding,
    Type * pAccessingInstanceType
)
{
    while (pDecl)
    {
        if (pDecl->IsMember() && pDecl->PMember()->IsShared() && IsAccessible(pDecl, pBinding, pAccessingInstanceType))
        {
            return true;
        }

        pDecl = GetNextOverloadForProcedureConsideringBaseClasses(pDecl, pAccessingInstanceType);
    }

    return false;
}

Declaration *
Semantics::FindMoreOverloadedProcedures
(
    Declaration *OverloadedProcedure,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Inout_ bool &SomeCandidatesBad
)
{
    if
    (
        (
            OverloadedProcedure->IsShadowing() ||
            OverloadedProcedure->IsShadowsKeywordUsed() ||
            (
                !ViewAsProcedure(OverloadedProcedure)->IsOverloadsKeywordUsed() &&
                !ViewAsProcedure(OverloadedProcedure)->IsOverridesKeywordUsed()
            )
        ) &&
        CanBeAccessible(OverloadedProcedure, NULL /* NULL binding good enough for base member */, AccessingInstanceType)
    )
    {
        return NULL;
    }


    Declaration *Owner = OverloadedProcedure->GetParent();

    if (Owner == NULL)
    {
        Owner = OverloadedProcedure->GetImmediateParentOrBuild()->GetImmediateParentOrBuild();
        if (Owner == NULL)
        {
            return NULL;
        }
    }

    Declaration *DeclaredInBase = NULL;

    if (Owner->IsClass())
    {
        DeclaredInBase =
            LookupInBaseClasses(
                Owner->PClass(),
                OverloadedProcedure->GetName(),
                BINDSPACE_Normal);
    }

    else if (Owner->IsInterface())
    {
        DeclaredInBase =
            EnsureNamedRoot
            (
                LookupInBaseInterfaces(
                    Owner->PInterface(),
                    OverloadedProcedure->GetName(),
                    NameSearchIgnoreExtensionMethods | NameSearchIgnoreObjectMethodsOnInterfaces,
                    BINDSPACE_Normal,
                    SourceLocation,
                    SomeCandidatesBad,
                    NULL,   // No binding context required here, it is figured out later
                    -1,     // Don't care about type arity here
                    NULL,   // Don't care whether the candidates are bad for any particular
                            // reason.
                    NULL    //No "bottom most" interface result
                )
            );

        if (SomeCandidatesBad)
        {
            return NULL;
        }
    }

    if (DeclaredInBase && IsProcedure(DeclaredInBase))
    {
        return DeclaredInBase;
    }

    return NULL;
}

bool Semantics::ApplySuperTypeFilter
(
    Type * ReceiverType,
    Procedure * Procedure,
    PartialGenericBinding * pPartialGenericBinding
)
{
    ThrowIfNull(ReceiverType);
    ThrowIfNull(Procedure);
    ThrowIfNull(Procedure->GetFirstParam());
    ThrowIfNull(Procedure->GetFirstParam()->GetType());


    if
    (
        Bindable::ValidateGenericConstraintsOnPartiallyBoundExtensionMethod
        (
            Procedure,
            pPartialGenericBinding,
            GetSymbols(),
            GetCompilerHost()
        )
    )
    {

        Type * replacedType =
            ReplaceGenericParametersWithArguments
            (
                ViewAsProcedure(Procedure)->GetFirstParam()->GetType(),
                pPartialGenericBinding,
                m_SymbolCreator
            );

        return DoesReceiverMatchInstance(ReceiverType, replacedType->ChaseThroughPointerTypes());
    }
    else
    {
        return false;
    }
}

Type * Semantics::ExtractReceiverType
(
    Procedure * pProc,
    GenericBindingInfo binding,
    bool isExtensionMethod
)
{
    Type * pRawReceiver = NULL;

    if (isExtensionMethod)
    {
        Assume(pProc->GetFirstParam() && pProc->GetFirstParam()->GetType(), L"All extension methods must have at least one parameter with a valid type");
        pRawReceiver = pProc->GetFirstParam()->GetType();
    }
    else
    {
        pRawReceiver = pProc->GetParent();
    }

    return
        ReplaceGenericParametersWithArguments
        (
            pRawReceiver,
            binding,
            m_SymbolCreator
        );
}

void
Semantics::CompareReceiverTypeSpecificity
(
    Procedure * pLeftProc,
    GenericBindingInfo leftBinding,
    Procedure * pRightProc,
    GenericBindingInfo rightBinding,
    _Inout_ bool & leftWins,
    _Inout_ bool & rightWins,
    bool leftIsExtensionMethod,
    bool rightIsExtensionMethod
)
{
    Assume(pLeftProc && pRightProc, L"Both pLeftProc and pRight must be non null!");

    Type * pLeftReceiverType = NULL;
    Type * pRightReceiverType = NULL;

    pLeftReceiverType =
        ExtractReceiverType
        (
            pLeftProc,
            leftBinding,
            leftIsExtensionMethod
        )->ChaseThroughPointerTypes();

    pRightReceiverType =
        ExtractReceiverType
        (
            pRightProc,
            rightBinding,
            rightIsExtensionMethod
        )->ChaseThroughPointerTypes();

    if (!BCSYM::AreTypesEqual(pLeftReceiverType, pRightReceiverType))
    {
        if (DoesReceiverMatchInstance(pLeftReceiverType, pRightReceiverType))
        {
            leftWins = true;
        }
        else if (DoesReceiverMatchInstance(pRightReceiverType, pLeftReceiverType))
        {
            rightWins = true;
        }
        else
        {
            leftWins = rightWins = false;
        }
    }
    else if (leftIsExtensionMethod && rightIsExtensionMethod)
    {
        Assume(BCSYM::AreTypesEqual(pLeftReceiverType,pRightReceiverType), L"If neither type is a subtype of the other, and the types are not interfaces, then they must be equivalent!");

        bool bothLoose = false;

        CompareGenericityBasedOnTypeGenericParams
        (
            NULL,
            pLeftProc->GetFirstParam(),
            pLeftProc,
            leftBinding,
            false,
            pRightProc->GetFirstParam(),
            pRightProc,
            rightBinding,
            false,
            leftWins,
            rightWins,
            bothLoose,
            true, //both left and right are extension methods...
            true
        );

        if (bothLoose)
        {
            leftWins = rightWins = false;
        }
    }
}

// This method determines whether the new candidate is available. This will happen
// in three situations:
//
// 1. The new candidate is not shadowed by name and signature by an existing candidate
//    (we don't have to worry about shadows by name because that is checked in
//    FindMoreOverloadedProcedures, above).
//
// 2. Either the new candidate or an existing candidate is an expanded paramarray
//    that the other doesn't match (i.e. Sub foo(Paramarray x() As Integer) and
//    Sub foo(x As Integer)).
//
// 3. The new candidate is not overridden by an existing candidate.
//
// If case #3 fails (and in case #2 when the existing candidate is the paramarray one),
// we will delete the list item and keep walking through the list remembering to return true.
//

OverloadList *
Semantics::InsertIfMethodAvailable
(
    Declaration *NewCandidate,
    GenericBindingInfo GenericBindingContext,
    bool ExpandNewCandidateParamArray,
    ExpressionList *ScratchArguments,
    _In_opt_count_(ArgumentCount) ILTree::Expression **ArgumentValues,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    unsigned TypeArgumentCount,
    OverloadResolutionFlags OvrldFlags,
    _Inout_ OverloadList *Candidates,
    _Inout_ unsigned &CandidateCount,
    _Inout_ NorlsAllocator &ListStorage,
    const Location& SourceLocation,
    bool CandidateIsExtensionMethod,
    unsigned long PrecedenceLevel,
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{

    //






    Assume(CandidateIsExtensionMethod || PrecedenceLevel == 0, L"When CandidateIsExtensionMethod is false, PrecedenceLevel should always be 0");
    ThrowIfNull(NewCandidate);

    OverloadList *Current = Candidates;
    OverloadList **Previous = &Candidates;
    OverloadList *NewCandidateNode = NULL;

    Assume
    (
        !ViewAsProcedure(NewCandidate)->IsGeneric() ||
         TypeArgumentCount == 0 ||
         (
            !GenericBindingContext.IsNull() &&
            (
                GenericBindingContext.IsPartialBinding() ||
                GenericBindingContext.IsFullMethodBinding()
            )
        ),
        L"Generic method binding context expected!!!"
    );

    DBG_SWITCH_PRINTF(fDumpOverload, L"  considering %s\n", CandidateToDebugString(NewCandidate,true));



    // Need to complete type argument inference for generic methods when no type arguments
    // have been supplied. Need to complete this so early in the overload process so that
    // hid-by-sig, paramarray disambiguation etc. are done using the substitued signature.
    //
    if (TypeArgumentCount == 0 && ViewAsProcedure(NewCandidate)->IsGeneric() && !HasFlag(OvrldFlags, OvrldDisableTypeArgumentInference))
    {
        NewCandidateNode =
            new(ListStorage)
            OverloadList(
                NewCandidate,
                GenericBindingContext,
                NULL,
                ExpandNewCandidateParamArray,
                PrecedenceLevel);
        NewCandidateNode->IsExtensionMethod = CandidateIsExtensionMethod;

        // Inferring of type arguments is done when when determining the callability of this
        // procedure with these arguments by comparing them against the corresponding parameters.
        //
        // Note that although RejectUncallableProcedure needs to be invoked on the non-generics
        // candidates too, it is not done here because some of the candidates might be rejected
        // for various reasons like hide-by-sig, paramarray disambiguation, etc. and RejectUncall-
        // -ableProcedure would not have to be invoked for them. This is especially important
        // because the RejectUncallableProcedure task is expensive.
        //
        bool PreviouslyReportingErrors = m_ReportErrors;
        m_ReportErrors = false;

        RejectUncallableProcedure(
            SourceLocation,
            NewCandidateNode,
            ScratchArguments,
            ArgumentValues,
            DelegateReturnType,
            OvrldFlags,
            ppAsyncSubArgumentListAmbiguity);

        m_ReportErrors = PreviouslyReportingErrors;

        // Get the inferred generic binding for this candidate
        //
        GenericBindingContext = NewCandidateNode->Binding;
    }


    bool InferenceFailedForNewCandidate =
        IsGeneric(NewCandidate) &&
        (GenericBindingContext.IsNull() || GenericBindingContext.IsPartialBinding() || GenericBindingContext.IsGenericTypeBinding());

    while (Current)
    {
        bool InferenceFailedForExistingCandidate =
            IsGeneric(Current->Candidate) &&
            (
                Current->Binding.IsNull()||
                Current->Binding.IsPartialBinding() ||
                Current->Binding.IsGenericTypeBinding()
            );

        Declaration *ExistingCandidate = Current->Candidate;

        // If they're the same, they can't hide one another
        if (NewCandidate == ExistingCandidate)
            goto continueloop;

        DBG_SWITCH_PRINTF(fDumpOverload, L"    Comparing it against this existing candidate... %s\n", CandidateToDebugString(ExistingCandidate));

        // An overriding method hides the methods it overrides.
        // Bug VSWhidbey 385900.
        //
        if (ViewAsProcedure(ExistingCandidate)->OverriddenProc() &&
            (ViewAsProcedure(ExistingCandidate)->OverriddenProc() == NewCandidate ||
                ViewAsProcedure(ExistingCandidate)->OverriddenProc() == ViewAsProcedure(NewCandidate)->OverriddenProc()))
        {
            AssertIfFalse(ExistingCandidate->GetParent() != NewCandidate->GetParent());

            VSASSERT(IsOrInheritsFrom(ExistingCandidate->GetParent(), NewCandidate->GetParent()),
                     "expected inheritance here");

            DBG_SWITCH_PRINTF(fDumpOverload, L"      new candidate rejected: an overriding method hides the method it overrides -- http://bugcheck/bugs/VSWhidbey/385900\n");

            return Candidates;
        }

        Parameter *NewCandidateParameter = ViewAsProcedure(NewCandidate)->GetFirstParam();
        Parameter *ExistingCandidateParameter = ViewAsProcedure(ExistingCandidate)->GetFirstParam();
        unsigned CurrentArgument  = 0;

        if (CandidateIsExtensionMethod)
        {
            ThrowIfNull(NewCandidateParameter);
            NewCandidateParameter = NewCandidateParameter->GetNext();

        }

        if (Current->IsExtensionMethod)
        {
            ExistingCandidateParameter = ExistingCandidateParameter->GetNext();
        }

        if (OvrldFlags & (OvrldSomeCandidatesAreExtensionMethods))
        {
            ++CurrentArgument;
        }

        unsigned NewCandidateParamArrayMatch = 0;
        unsigned ExistingCandidateParamArrayMatch = 0;
        bool ExistingCandidateHasParamArrayParameter = false;
        bool NewCandidateHasParamArrayParameter = false;

        for (; CurrentArgument < ArgumentCount; CurrentArgument++)
        {
            bool BothLose = false;
            bool NewCandidateWins = false;
            bool ExistingCandidateWins = false;

            CompareParameterApplicability(
                NULL,
                NewCandidateParameter,
                ViewAsProcedure(NewCandidate),
                GenericBindingContext,
                ExpandNewCandidateParamArray,
                ExistingCandidateParameter,
                ViewAsProcedure(ExistingCandidate),
                Current->Binding,
                Current->ParamArrayExpanded,
                NewCandidateWins,
                ExistingCandidateWins,
                BothLose,
                CandidateIsExtensionMethod,
                Current->IsExtensionMethod);

            if (BothLose || NewCandidateWins || ExistingCandidateWins)
            {
                goto continueloop; // Go to next candidate
            }

            if (NewCandidateParameter->IsParamArray())
            {
                NewCandidateHasParamArrayParameter = true;
            }

            if (ExistingCandidateParameter->IsParamArray())
            {
                ExistingCandidateHasParamArrayParameter = true;
            }


            // If a parameter is a param array, there is no next parameter and so advancing
            // through the parameter list is bad.
            if (!NewCandidateParameter->IsParamArray() || !ExpandNewCandidateParamArray)
            {
                NewCandidateParameter = NewCandidateParameter->GetNext();
                NewCandidateParamArrayMatch++;
            }

            if (!ExistingCandidateParameter->IsParamArray() || !Current->ParamArrayExpanded)
            {
                ExistingCandidateParameter = ExistingCandidateParameter->GetNext();
                ExistingCandidateParamArrayMatch++;
            }
        }

        Procedure * NewCandidateProcedure = ViewAsProcedure(NewCandidate);
        Procedure * ExistingCandidateProcedure = ViewAsProcedure(ExistingCandidate);


        unsigned CompareFlags =
            BCSYM::CompareProcs
            (
                NewCandidateProcedure,
                GenericBindingContext,
                ExistingCandidateProcedure,
                Current->Binding,
                &m_SymbolCreator,
                false, //don't ignore overload differences
                CandidateIsExtensionMethod,
                Current->IsExtensionMethod
            );

        bool ExactSignature =
            (CompareFlags & (EQ_Shape | EQ_GenericTypeParams)) == 0;

        if
        (
            (
                NewCandidateHasParamArrayParameter &&
                ExistingCandidateHasParamArrayParameter &&
                (
                    Current->ParamArrayExpanded != ExpandNewCandidateParamArray
                )
            )
        )
        {
            //If both procedures are param array proceudres, then a non expanded
            //candidate should never shadow an expanded candidate
            goto continueloop;
        }

        bool NewCandidateWins = false;
        bool ExistingCandidateWins = false;

        if (!ExactSignature)
        {

            // If inference failed for any of the candidates, then don't compare them.
            //
            // This simple strategy besides fixing the problems associated with an inference
            // failed candidate beating a inference passing candidate also helps with better
            // error reporting by showing the inference failed candidates too.
            //
            if (InferenceFailedForNewCandidate ||InferenceFailedForExistingCandidate)
            {
                goto continueloop;
            }

            //If we have gotten to this point it means that the 2 procedures have equal specificity,
            //but signatures that do not match exactly (after generic substitution). This
            //implies that we are dealing with differences in shape due to param arrays
            //or optional arguments.
            //So we look and see if one procedure maps fewer arguments to the
            //param array than the other. The one using more, is then shadowed by the one using less.

            CompareParamarraySpecificity(
                ExpandNewCandidateParamArray,
                NewCandidateParamArrayMatch,
                Current->ParamArrayExpanded,
                ExistingCandidateParamArrayMatch,
                NewCandidateWins,
                ExistingCandidateWins);

            if (ExistingCandidateWins)
            {
                DBG_SWITCH_PRINTF(fDumpOverload, L"      new candidate rejected: $7.0 If M has fewer parameters from an expanded paramarray than N, eliminate N from the set\n");
                return Candidates;
            }
            else if (NewCandidateWins)
            {
                // Delete current item from the list and continue.
                *Previous = Current = Current->Next;
                CandidateCount--;
                DBG_SWITCH_PRINTF(fDumpOverload, L"      existing candidate rejected: $7.0 If M has fewer parameters from an expanded paramarray than N, eliminate N from the set\n");
                continue;
            }
            else if (CandidateIsExtensionMethod || Current->IsExtensionMethod)
            {
                //We have 2 methods with different shapes
                //but the parameters of both are equaly specific
                //and they both use the same # of param array parameters,
                //and at least one of the two is an extension method.
                //We now first check to see which one
                //is defined with the more derived receiver type and have that one
                //shadow the less derived one.


                ExistingCandidateWins = false;
                NewCandidateWins = false;

                //Compare Receiver Type Specificity

                CompareReceiverTypeSpecificity
                (
                    NewCandidateProcedure,
                    GenericBindingContext,
                    ExistingCandidateProcedure,
                    Current->Binding,
                    NewCandidateWins,
                    ExistingCandidateWins,
                    CandidateIsExtensionMethod,
                    Current->IsExtensionMethod
                );

                if (ExistingCandidateWins)
                {
                    DBG_SWITCH_PRINTF(fDumpOverload, L"      new candidate rejected: $7.1 If M is defined in a more derived type than N, eliminate N from the set\n");
                    return Candidates;
                }
                else if (NewCandidateWins)
                {
                    Current = *Previous = Current->Next;
                    CandidateCount--;
                    DBG_SWITCH_PRINTF(fDumpOverload, L"      existing candidate rejected: $7.1 If M is defined in a more derived type than N, eliminate N from the set\n");
                    continue;
                }
            }

            goto continueloop;
        }
        else
        {
            //The signatures of the two methods match (after generic parameter substitution).
            //This means that param array shadowing doesn't come into play.

            CompareReceiverTypeSpecificity
            (
                NewCandidateProcedure,
                GenericBindingContext,
                ExistingCandidateProcedure,
                Current->Binding,
                NewCandidateWins,
                ExistingCandidateWins,
                CandidateIsExtensionMethod,
                Current->IsExtensionMethod
            );

            if (ExistingCandidateWins)
            {
                //The existing candidate is the derived type...
                //We need to check to see if the existing candate has failed inference but the new candidate has not...

                if (InferenceFailedForExistingCandidate && !InferenceFailedForNewCandidate)
                {
                    goto continueloop;
                }
                else
                {
                    DBG_SWITCH_PRINTF(fDumpOverload, L"      new candidate rejected: $7.1 If M is defined in a more derived type than N, eliminate N from the set (and either inference succeeded for existing candidate or failed for new candidate)\n");
                    return Candidates;
                }
            }
            else if (NewCandidateWins)
            {
                //The new candidate is the derived type...
                //We need to check to see if the new candidate has failed inference but the new candidate has not...

                if (!(InferenceFailedForNewCandidate && !InferenceFailedForExistingCandidate))
                {
                    Current = *Previous = Current->Next;
                    CandidateCount--;
                    DBG_SWITCH_PRINTF(fDumpOverload, L"      existing candidate rejected: $7.1 If M is defined in a more derived type than N, eliminate N from the set (and either inference succeeded for new candidate or failed for existing candidate)\n");
                    continue;
                }
            }

            goto continueloop;
        }

        VSFAIL("unexpected code path");

continueloop:
        Previous = &Current->Next;
        Current = Current->Next;
    }

    if (NewCandidateNode == NULL)
    {
        NewCandidateNode =
            new(ListStorage)
            OverloadList(
                NewCandidate,
                GenericBindingContext,
                NULL,
                ExpandNewCandidateParamArray,
                PrecedenceLevel);

        NewCandidateNode->IsExtensionMethod = CandidateIsExtensionMethod;
    }

    NewCandidateNode->Next = Candidates;
    CandidateCount++;

    DBG_SWITCH_PRINTF(fDumpOverload, L"    added candidate %s\n", CandidateToDebugString(NewCandidate));

    return NewCandidateNode;
}

void
Semantics::CompareParamarraySpecificity
(
    bool LeftParamarrayExpanded,
    unsigned LeftParamarrayExpansionCount,
    bool RightParamarrayExpanded,
    unsigned RightParamarrayExpansionCount,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins
)
{
    if (!LeftParamarrayExpanded && RightParamarrayExpanded)
    {
        LeftWins = true;
    }
    else if (LeftParamarrayExpanded && !RightParamarrayExpanded)
    {
        RightWins = true;
    }
    else if (!LeftParamarrayExpanded && !RightParamarrayExpanded)
    {
        // In theory, this shouldn't happen, but another language could
        // theoretically define two methods with optional arguments that
        // end up being equivalent. So don't prefer one over the other.
    }
    else
    {
        // If both are expanded, then see if one uses more on actual
        // parameters than the other. If so, we prefer the one that uses
        // more on actual parameters.

        if (LeftParamarrayExpansionCount > RightParamarrayExpansionCount)
        {
            LeftWins = true;
        }
        else if (RightParamarrayExpansionCount > LeftParamarrayExpansionCount)
        {
            RightWins = true;
        }
    }
}

bool
Semantics::VerifyParameterCounts
(
    unsigned int freeTypeParameterCount,
    Procedure * pProc,
    unsigned int typeArgumentCount,
    unsigned int argumentCount,
    _Inout_ unsigned &RejectedForArgumentCount,
    _Inout_ unsigned &RejectedForTypeArgumentCount,
    _Out_ bool & hasParamArray,
    _Out_ unsigned int & maximumParameterCount
)
{
    if (typeArgumentCount && freeTypeParameterCount != typeArgumentCount)
    {
        ++RejectedForTypeArgumentCount;
        return false;
    }
    else
    {
        unsigned int requiredParameterCount = 0;
        maximumParameterCount = 0;
        hasParamArray = false;

        pProc->GetAllParameterCounts
        (
            requiredParameterCount,
            maximumParameterCount,
            hasParamArray
        );

        if
        (
            argumentCount < requiredParameterCount ||
            (
                argumentCount > maximumParameterCount &&
                ! hasParamArray
            )
        )
        {
            ++RejectedForArgumentCount;
            return false;
        }
        else
        {
            return true;
        }
    }
}


void
Semantics::CollectExtensionMethodOverloadCandidates
(
    ExtensionCallLookupResult * ExtensionCall,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    _In_opt_count_(TypeArgumentCount) Type **TypeArguments,
    Location * TypeArgumentLocations,
    unsigned TypeArgumentCount,
    _Inout_ NorlsAllocator &ListStorage,
    _Out_ unsigned &CandidateCount,
    _Out_ unsigned &RejectedForArgumentCount,
    _Out_ unsigned &RejectedForTypeArgumentCount,
    _Out_ unsigned &RejectedBySuperTypeFilterCount,
    const Location &SourceLocation,
    OverloadResolutionFlags OvrldFlags,
    Type * InvokeMethodReturnType,
    ExpressionFlags Flags,
    Type * AccessingInstanceType,
    bool & SomeCandidatesBad,
    OverloadList * & Candidates,
    OverloadList * & InstanceMethodOnlyCandidates,
    unsigned & InstanceMethodOnlyCandidateCount,
    _Inout_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    ThrowIfNull(ExtensionCall);

    //If either TypeArguments or TypeArgumentLocations is not null, then both must be not null.
    ThrowIfFalse(!TypeArguments || TypeArgumentLocations);
    ThrowIfFalse(!TypeArgumentLocations || TypeArguments);

    ExtensionCallLookupResult::iterator_type candidateIterator = ExtensionCall->GetExtensionMethods();

    ILTree::Expression *ArgumentsScratch[128], **SavedArguments = NULL;

    Candidates = NULL;
    CandidateCount = 0;
    RejectedForArgumentCount = 0;
    RejectedForTypeArgumentCount = 0;
    RejectedBySuperTypeFilterCount = 0;

    //iterate over each candidate in the unfiltered extension call
    while (candidateIterator.MoveNext())
    {

        ExtensionCallInfo current = candidateIterator.Current();

        if(
            (HasFlag(OvrldFlags, OvrldIgnoreSubs) && IsSub(current.m_pProc)) ||
            (HasFlag(OvrldFlags, OvrldIgnoreProperties) && IsProperty(current.m_pProc)) ||
            (HasFlag(OvrldFlags, OvrldIgnoreEvents) && IsEvent(current.m_pProc))
           )
        {
            continue;
        }

        GenericBindingInfo pGenericBinding;

        bool hasParamArray = false;
        unsigned int maximumParameterCount = 0;

        if
        (
            VerifyParameterCounts
            (
                current.GetFreeArgumentCount(),
                current.m_pProc,
                TypeArgumentCount,
                ArgumentCount,
                RejectedForArgumentCount,
                RejectedForTypeArgumentCount,
                hasParamArray,
                maximumParameterCount
            )
        )
        {
            if(
                (HasFlag(OvrldFlags, OvrldExactArgCount) && ArgumentCount<maximumParameterCount) ||
                (HasFlag(OvrldFlags, OvrldIgnoreParamArray) && ArgumentCount>maximumParameterCount)
               )
            {
                RejectedForArgumentCount++;
                continue;
            }

            if (TypeArgumentCount > 0 && ! current.m_pPartialGenericBinding)
            {

                pGenericBinding =
                (
                    m_SymbolCreator.GetGenericBinding
                    (
                        false,
                        current.m_pProc,
                        TypeArguments,
                        TypeArgumentCount,
                        //Extension methods are not allowed in generic types, so we will never have a parent binding.
                        NULL
                    )
                );
            }
            else if (current.m_pPartialGenericBinding)
            {
                pGenericBinding = current.m_pPartialGenericBinding;

                if (TypeArgumentCount > 0)
                {
                    pGenericBinding.ApplyExplicitArgumentsToPartialBinding
                    (
                        TypeArguments,
                        TypeArgumentLocations,
                        TypeArgumentCount,
                        this,
                        current.m_pProc
                    );

                }
            }

            if (current.m_pProc->IsGeneric() && SavedArguments == NULL && TypeArgumentCount == 0)
            {
                // Back up the arguments because the inference process later on
                // is destructive to the argument list.
                //
                SaveArguments
                (
                    ListStorage,
                    ArgumentsScratch,
                    128,
                    SavedArguments,
                    Arguments,
                    ArgumentCount
                );
            }

            // A method with a paramarray can be considered in two forms: in an
            // expanded form or in an unexpanded form (i.e. as if the paramarray
            // decoration was not specified). Weirdly, it can apply in both forms, as
            // in the case of passing Object() to ParamArray x As Object() (because
            // Object() converts to both Object() and Object).

            // Does the method apply in its unexpanded form? This can only happen if
            // either there is no paramarray or if the argument count matches exactly
            // (if it's less, then the paramarray is expanded to nothing, if it's more,
            // it's expanded to one or more parameters).

            if
            (
                !hasParamArray ||
                ArgumentCount == maximumParameterCount
            )
            {
                Candidates =
                    InsertIfMethodAvailable
                    (
                        current.m_pProc,
                        pGenericBinding,
                        false,
                        Arguments,
                        SavedArguments,
                        ArgumentCount,
                        InvokeMethodReturnType,
                        TypeArgumentCount,
                        OvrldFlags,
                        Candidates,
                        CandidateCount,
                        ListStorage,
                        SourceLocation,
                        true,
                        current.m_precedenceLevel,
                        ppAsyncSubArgumentListAmbiguity
                    );
            }

            // How about it's expanded form? It always applies if there's a paramarray.
            if (hasParamArray && !HasFlag(OvrldFlags, OvrldIgnoreParamArray))
            {
                Candidates =
                    InsertIfMethodAvailable
                    (
                        current.m_pProc,
                        pGenericBinding,
                        true,
                        Arguments,
                        SavedArguments,
                        ArgumentCount,
                        InvokeMethodReturnType,
                        TypeArgumentCount,
                        OvrldFlags,
                        Candidates,
                        CandidateCount,
                        ListStorage,
                        SourceLocation,
                        true,
                        current.m_precedenceLevel,
                        ppAsyncSubArgumentListAmbiguity
                    );
            }
        }

    }

    if (SavedArguments != NULL)
    {
        // Restore the arguments if they have been backed up because then the arguments
        // might have been modified during type parameter inference.
        //
        RestoreOriginalArguments(SavedArguments, Arguments);
    }


    if (ExtensionCall->GetInstanceMethodLookupResult())
    {
        unsigned InstanceRejectedForArgumentCount = 0;
        bool InstanceSomeCandidatesBad = false;

        InstanceMethodOnlyCandidates =
            CollectOverloadCandidates
            (
                NULL,
                ExtensionCall->GetInstanceMethodLookupResult(),
                ExtensionCall->GetInstanceMethodLookupGenericBinding(),
                Arguments,
                ArgumentCount,
                InvokeMethodReturnType,
                TypeArguments,
                TypeArgumentCount,
                Flags,
                OvrldFlags,
                ExtensionCall->GetAccessingInstanceTypeOfInstanceMethodLookupResult(),
                ListStorage,
                InstanceMethodOnlyCandidateCount,
                InstanceRejectedForArgumentCount,
                RejectedForTypeArgumentCount,
                SourceLocation,
                InstanceSomeCandidatesBad,
                ppAsyncSubArgumentListAmbiguity
            );

        Candidates =
            CollectOverloadCandidates
            (
                Candidates,
                ExtensionCall->GetInstanceMethodLookupResult(),
                ExtensionCall->GetInstanceMethodLookupGenericBinding(),
                Arguments,
                ArgumentCount,
                InvokeMethodReturnType,
                TypeArguments,
                TypeArgumentCount,
                Flags,
                OvrldFlags,
                ExtensionCall->GetAccessingInstanceTypeOfInstanceMethodLookupResult(),
                ListStorage,
                CandidateCount,
                RejectedForArgumentCount,
                RejectedForTypeArgumentCount,
                SourceLocation,
                SomeCandidatesBad,
                ppAsyncSubArgumentListAmbiguity
            );
    }

}

OverloadList *
Semantics::CollectOverloadCandidates
(
    OverloadList * Candidates,  //The candidate list to attach the results to(can be null)
    Declaration *OverloadedProcedure,
    GenericBinding *GenericBindingContext,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    _In_count_(TypeArgumentCount) Type **TypeArguments,
    unsigned TypeArgumentCount,
    ExpressionFlags Flags,
    OverloadResolutionFlags OvrldFlags,
    Type *AccessingInstanceType,
    _Inout_ NorlsAllocator &ListStorage,
    _Out_ unsigned &CandidateCount,
    _Out_ unsigned &RejectedForArgumentCount,
    _Out_ unsigned &RejectedForTypeArgumentCount,
    const Location &SourceLocation,
    _Inout_ bool &SomeCandidatesBad,
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    Type *PossiblyGenericContext =
        GenericBindingContext ?
            GenericBindingContext :
            OverloadedProcedure->GetContainer()->PNamedRoot();

    ILTree::Expression *ArgumentsScratch[128], **SavedArguments = NULL;
    RejectedForTypeArgumentCount = 0;
    RejectedForArgumentCount = 0;

    do
    {
        GenericBinding *CandidateGenericBinding = NULL;

        for (Declaration *NextProcedure = OverloadedProcedure;
             NextProcedure;
             NextProcedure = NextProcedure->GetNextOverload())
        {
            // Amazingly, non-procedures can land here if a class defines both fields
            // and methods with the same name. (This is impossible in VB, but apparently
            // possible for classes written in other languages.)

            if (!IsProcedure(NextProcedure))
            {
                continue;
            }

            Procedure *NonAliasProcedure = ViewAsProcedure(NextProcedure);

            if (IsBad(NonAliasProcedure))
            {
                continue;
            }

            // ---- out constructors or non-constructors as appropriate.
            if (HasFlag(Flags, ExprIsConstructorCall))
            {
                if (!NonAliasProcedure->IsInstanceConstructor())
                {
                    continue;
                }
            }
            else if (NonAliasProcedure->IsInstanceConstructor())
            {
                continue;
            }

            if(
                (HasFlag(OvrldFlags, OvrldIgnoreSharedMembers) && NonAliasProcedure->IsShared()) ||
                (HasFlag(OvrldFlags, OvrldIgnoreSubs) && IsSub(NonAliasProcedure)) ||
                (HasFlag(OvrldFlags, OvrldIgnoreEvents) && IsEvent(NonAliasProcedure))
               )
            {
                continue;
            }

#if 0
            // Overriding procedures are not considered as introducing new signatures,
            // so they are ignored and the methods they are overriding will be eventually
            // considered as we dig up the bases.
            //
            if (ViewAsProcedure(NextProcedure)->IsOverrides())
            {
                continue;
            }
#endif

            // ---- out inaccessible procedures.

            if (!IsAccessible(
                    NextProcedure,
                    NULL,   // Accessing through base, don't need a binding context
                    AccessingInstanceType))
            {
                continue;
            }

            // If type arguments have been supplied, ---- out procedures that don't have an
            // appropriate number of type parameters.

            if (TypeArgumentCount > 0 &&
                TypeArgumentCount != NextProcedure->GetGenericParamCount())
            {
                RejectedForTypeArgumentCount++;
                continue;
            }

            // ---- out procedures that cannot accept the number of supplied arguments.

            unsigned RequiredParameterCount = 0;
            unsigned MaximumParameterCount = 0;
            bool HasParamArray = false;

            NonAliasProcedure->GetAllParameterCounts(RequiredParameterCount, MaximumParameterCount, HasParamArray);

            unsigned ArgumentCountToUseForComparison = (OvrldFlags & OvrldSomeCandidatesAreExtensionMethods) ? ArgumentCount  - 1: ArgumentCount;

            if (ArgumentCountToUseForComparison < RequiredParameterCount ||
                (ArgumentCountToUseForComparison > MaximumParameterCount && !HasParamArray))
            {
                RejectedForArgumentCount++;
                continue;
            }

            if(
                (HasFlag(OvrldFlags, OvrldExactArgCount) && ArgumentCountToUseForComparison < MaximumParameterCount) ||
                (HasFlag(OvrldFlags, OvrldIgnoreParamArray) && ArgumentCountToUseForComparison > MaximumParameterCount)
               )
            {
                RejectedForArgumentCount++;
                continue;
            }

            if (CandidateGenericBinding == NULL)
            {
                // Derive a generic binding appropriate for referring to members in this context. If type arguments
                // are supplied explicitly, then synthesize the appropriate binding using those arguments.

                CandidateGenericBinding =
                    DeriveGenericBindingForMemberReference(PossiblyGenericContext, NextProcedure, m_SymbolCreator, m_CompilerHost);
            }

            GenericBinding *FullCandidateGenericBinding;

            if (TypeArgumentCount > 0)
            {
                FullCandidateGenericBinding =
                    m_SymbolCreator.GetGenericBinding(
                        false,
                        NextProcedure,
                        TypeArguments,
                        TypeArgumentCount,
                        CandidateGenericBinding->PGenericTypeBinding());
            }
            else
            {
                if (NextProcedure->IsGeneric() && SavedArguments == NULL)
                {
                    // Back up the arguments because the inference process later on
                    // is destructable to the argument list.
                    //
                    SaveArguments(
                        ListStorage,
                        ArgumentsScratch,
                        128,
                        SavedArguments,
                        Arguments,
                        ArgumentCount);
                }

                FullCandidateGenericBinding = CandidateGenericBinding;
            }

            // A method with a paramarray can be considered in two forms: in an
            // expanded form or in an unexpanded form (i.e. as if the paramarray
            // decoration was not specified). Weirdly, it can apply in both forms, as
            // in the case of passing Object() to ParamArray x As Object() (because
            // Object() converts to both Object() and Object).

            // Does the method apply in its unexpanded form? This can only happen if
            // either there is no paramarray or if the argument count matches exactly
            // (if it's less, then the paramarray is expanded to nothing, if it's more,
            // it's expanded to one or more parameters).

            if ((!HasParamArray || ArgumentCountToUseForComparison== MaximumParameterCount))
            {
                Candidates =
                    InsertIfMethodAvailable(
                        NextProcedure,
                        FullCandidateGenericBinding,
                        false,
                        Arguments,
                        SavedArguments,
                        ArgumentCount,
                        DelegateReturnType,
                        TypeArgumentCount,
                        OvrldFlags,
                        Candidates,
                        CandidateCount,
                        ListStorage,
                        SourceLocation,
                        false,
                        0,
                        ppAsyncSubArgumentListAmbiguity);
            }

            // How about it's expanded form? It always applies if there's a paramarray.
            if (HasParamArray && !HasFlag(OvrldFlags, OvrldIgnoreParamArray))
            {
                Candidates =
                    InsertIfMethodAvailable(
                        NextProcedure,
                        FullCandidateGenericBinding,
                        true,
                        Arguments,
                        SavedArguments,
                        ArgumentCount,
                        DelegateReturnType,
                        TypeArgumentCount,
                        OvrldFlags,
                        Candidates,
                        CandidateCount,
                        ListStorage,
                        SourceLocation,
                        false,
                        0,
                        ppAsyncSubArgumentListAmbiguity);
            }
        }

    } while (!HasFlag(Flags, ExprIsConstructorCall) &&
             (OverloadedProcedure = FindMoreOverloadedProcedures(OverloadedProcedure, AccessingInstanceType, SourceLocation, SomeCandidatesBad)));

    if (SavedArguments != NULL)
    {
        // Restore the arguments if they have been backed up because then the arguments
        // might have been modified during type parameter inference.
        //
        RestoreOriginalArguments(SavedArguments, Arguments);
    }

    return Candidates;
}

Declaration *
Semantics::VerifyLateboundCallConditions
(
    _In_ const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ Type *DelegateType,
    _In_ OverloadList *Candidates,
    _In_count_(ArgumentCount) ExpressionList *Arguments,
    unsigned ArgumentCount,
    _In_opt_ Type *DelegateReturnType,
    _Out_ bool &ResolutionFailed,
    _Out_ bool &ResolutionIsLateBound,
    OverloadResolutionFlags OvrldFlags
)
{
    if (ViewAsProcedure(Candidates->Candidate)->IsInstanceConstructor())
    {
        ReportLateBoundConstructors(
            CallLocation,
            OverloadedProcedureName,
            DelegateType,
            Candidates,
            Arguments,
            ArgumentCount,
            DelegateReturnType,
            OvrldFlags);

        ResolutionFailed = true;
        return NULL;
    }

    if (m_CompilerHost->IsStarliteHost())
    {
        ReportSemanticError(ERRID_NoStarliteOverloadResolution, CallLocation);
        ResolutionFailed = true;
        return NULL;
    }


    // Warnings have lower precedence than Option Strict and Starlite errors.
    if (!m_UsingOptionTypeStrict && WarnOptionStrict())
    {
        ReportSemanticError(WRNID_LateBindingResolution, CallLocation);
    }

    ResolutionIsLateBound = true;
    return NULL;
}

unsigned
CountArguments
(
    ExpressionList *Arguments
)
{
    unsigned Result = 0;

    for (ExpressionList *Argument = Arguments;
         Argument;
         Argument = Argument->AsExpressionWithChildren().Right)
    {
        Result++;
    }

    return Result;
}

void
SaveArguments
(
    _Inout_ NorlsAllocator &TreeStorage,
    ILTree::Expression *ArgumentsScratch[],
    unsigned ScratchCount,
    _Out_ _Deref_post_cap_(ArgumentCount) ILTree::Expression **&SavedArguments,
    ExpressionList *Arguments,
    unsigned ArgumentCount
)
{
    SavedArguments = ArgumentsScratch;
    if (ArgumentCount > 128)
    {
        typedef ILTree::Expression *ExpressionPointer;
        SavedArguments = new(TreeStorage) ExpressionPointer[ArgumentCount];
    }

    unsigned ArgumentIndex = 0;

    for (ExpressionList *Argument = Arguments; Argument; Argument = Argument->AsExpressionWithChildren().Right)
    {
        SavedArguments[ArgumentIndex++] =
            Argument->AsExpressionWithChildren().Left ?
                Argument->AsExpressionWithChildren().Left->AsArgumentExpression().Left :
                NULL;
    }
}

// Overload resolution uses other operations in the compiler that can perform
// transformations on argument trees. In order to guarantee that the original
// trees are not altered, make copies of the parts of argument trees that might
// undergo alteration.

void
MakeScratchCopiesOfArguments
(
    BILALLOC &TreeAllocator,
    _In_opt_ ILTree::Expression *SavedArguments[],
    ExpressionList *Arguments
)
{
    ThrowIfNull(SavedArguments);

    unsigned ArgumentIndex = 0;
    for (ExpressionList *Argument = Arguments; Argument; Argument = Argument->AsExpressionWithChildren().Right)
    {
        if (SavedArguments[ArgumentIndex])
        {
            // Bug VSWhidbey 208815 requires that SX_ADDRESSOF nodoes also be
            // deep copied here because InterpretDelegateBinding modifies some
            // of the sub nodes of this node.

            ILTree::ILNode *ArgumentCopy =
                (SavedArguments[ArgumentIndex]->bilop == SX_PROPERTY_REFERENCE ||
                 SavedArguments[ArgumentIndex]->bilop == SX_LATE_REFERENCE ||
                 SavedArguments[ArgumentIndex]->bilop == SX_ADDRESSOF ||
                 SavedArguments[ArgumentIndex]->bilop == SX_LAMBDA ||
                 SavedArguments[ArgumentIndex]->bilop == SX_UNBOUND_LAMBDA
                 ) ?
                    TreeAllocator.xCopyBilTreeForScratch(SavedArguments[ArgumentIndex]) :
                    TreeAllocator.xCopyBilNodeForScratch(SavedArguments[ArgumentIndex]);

            Argument->AsExpressionWithChildren().Left->AsArgumentExpression().Left = &ArgumentCopy->AsExpression();
        }
        ArgumentIndex++;
    }
}

OverloadList *
Semantics::RejectBetterTypeInferenceLevelOfLaterReleases
(
    _Inout_ OverloadList *Candidates,
    _Inout_ unsigned &CandidateCount
)
{
    OverloadList *BestCandidate = NULL;
    OverloadList *CurrentCandidate = NULL;
    TypeInferenceLevel leastInferred = TypeInferenceLevelInvalid;

    // first pass: find least relaxation level.
    CurrentCandidate = Candidates;
    while (CurrentCandidate)
    {
        if (!CurrentCandidate->NotCallable)
        {
            leastInferred = min(leastInferred, CurrentCandidate->TypeInferenceLevel);
        }

        CurrentCandidate = CurrentCandidate->Next;
    }

    // second pass: remove all candidates that have wider relaxations.
    CurrentCandidate = Candidates;
    while (CurrentCandidate)
    {
        if (!CurrentCandidate->NotCallable)
        {
            if (CurrentCandidate->TypeInferenceLevel > leastInferred)
            {
                DBG_SWITCH_PRINTF(fDumpOverload, L"  Removed candidate %s because it's %s rather than %s\n", CandidateToDebugString(CurrentCandidate), TypeInferenceLevelToDebugString(CurrentCandidate->TypeInferenceLevel), TypeInferenceLevelToDebugString(leastInferred));

                CandidateCount--;
                CurrentCandidate->NotCallable = true;
            }
            else
            {
                BestCandidate = CurrentCandidate;
            }
        }
        CurrentCandidate = CurrentCandidate->Next;
    }

    return BestCandidate;
}

OverloadList *
Semantics::RejectLessSpecificDelegateRelaxationLevels
(
    _Inout_ OverloadList *Candidates,
    _Inout_ unsigned &CandidateCount
)
{
    // It looks like this function is implementing the following two rules from $11.8.1. of the language spec:
    //
    // - If one or more arguments are AddressOf or lambda expressions, 
    //    and all of the corresponding delegate types in M match exactly, 
    //    but not all do in N, eliminate N from the set.
    //
    // - If one or more arguments are AddressOf or lambda expressions, 
    //    and all of the corresponding delegate types in M are widening conversions, 
    //    but not all are in N, eliminate N from the set.
    //

    OverloadList *BestCandidate = NULL;
    OverloadList *CurrentCandidate = NULL;
    DelegateRelaxationLevel leastRelaxed = DelegateRelaxationLevelInvalid;

    // first pass: find least relaxation level.
    CurrentCandidate = Candidates;
    while (CurrentCandidate)
    {
        if (!CurrentCandidate->NotCallable)
        {
            leastRelaxed = min(leastRelaxed, CurrentCandidate->DelegateRelaxationLevel);
        }

        CurrentCandidate = CurrentCandidate->Next;
    }

    // second pass: remove all candidates that have wider relaxations.
    CurrentCandidate = Candidates;
    while (CurrentCandidate)
    {
        if (!CurrentCandidate->NotCallable)
        {
            if (CurrentCandidate->DelegateRelaxationLevel > leastRelaxed)
            {
                DBG_SWITCH_PRINTF(fDumpOverload, L"  Removed candidate %s because it's %s rather than %s\n", CandidateToDebugString(CurrentCandidate), DelegateRelaxationLevelToDebugString(CurrentCandidate->DelegateRelaxationLevel), DelegateRelaxationLevelToDebugString(leastRelaxed));

                CandidateCount--;
                CurrentCandidate->NotCallable = true;
            }
            else
            {
                BestCandidate = CurrentCandidate;
            }
        }
        CurrentCandidate = CurrentCandidate->Next;
    }

    return BestCandidate;
}

void
RestoreOriginalArguments
(
    _In_ ILTree::Expression *SavedArguments[],
    ExpressionList *Arguments
)
{
    unsigned ArgumentIndex = 0;
    for (ExpressionList *Argument = Arguments; Argument; Argument = Argument->AsExpressionWithChildren().Right)
    {
        if (SavedArguments[ArgumentIndex])
        {
            Argument->AsExpressionWithChildren().Left->AsArgumentExpression().Left =
                SavedArguments[ArgumentIndex];
        }
        ArgumentIndex++;
    }
}



OverloadList *
Semantics::RejectUncallableProcedures
(
    const Location &CallLocation,
    _Inout_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags,
    _Inout_ unsigned &CandidateCount,
    _Out_ bool &SomeCandidatesAreGeneric,
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    OverloadList *BestCandidate = NULL;

    // The process of verifying that a procedure is callable is
    // destructive to the arguments, so use a copy of the top-level argument
    // expression nodes for testing each procedure.

    ILTree::Expression *ArgumentsScratch[128], **SavedArguments;
    SaveArguments(
        m_TreeStorage,
        ArgumentsScratch,
        128,
        SavedArguments,
        Arguments,
        ArgumentCount);

    bool PreviouslyReportingErrors = m_ReportErrors;
    m_ReportErrors = false;

    while (Candidates)
    {

        if (!Candidates->ArgumentMatchingDone)
        {
            RejectUncallableProcedure(
                CallLocation,
                Candidates,
                Arguments,
                SavedArguments,
                DelegateReturnType,
                OvrldFlags,
                ppAsyncSubArgumentListAmbiguity);
        }

        if (Candidates->NotCallable)
        {
            CandidateCount--;
        }
        else
        {
            BestCandidate = Candidates;

            if (IsGenericOrHasGenericParent(Candidates->Candidate))
            {
                SomeCandidatesAreGeneric = true;
            }

            // 





            /* else if (!RequiresSomeConversion)
            {
                // this flag is erroneously not set in some scenarios involving paramarrays.

                // This candidate is an exact match,
                // which means the audition is over.

                // 




*/
        }

        Candidates = Candidates->Next;
    }

    m_ReportErrors = PreviouslyReportingErrors;

    RestoreOriginalArguments(SavedArguments, Arguments);

    return BestCandidate;
}

void
Semantics::RejectUncallableProcedure
(
    _In_ const Location &CallLocation,
    _In_ OverloadList *Candidate,
    _In_ ExpressionList *Arguments,
    _In_ ILTree::Expression **SavedArguments,
    _In_opt_ Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags,
    AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    VSASSERT(Candidate->ArgumentMatchingDone == false, "Argument matching being done multiple times!!!");

    // The process of verifying that a procedure is callable is
    // destructive to the arguments, so use a copy of the top-level argument
    // expression nodes for testing each procedure.

    MakeScratchCopiesOfArguments(
        m_TreeAllocator,
        SavedArguments,
        Arguments);

    bool RequiresSomeConversion = false;
    ExpressionList *CopyOutArguments = NULL;
    bool ArgumentArityBad = false;

    MatchArguments4
    (
        CallLocation,
        ViewAsProcedure(Candidate->Candidate),
        NULL,
        Candidate->Binding,
        Arguments,
        DelegateReturnType,
        ExprNoFlags,
        OvrldFlags,
        CopyOutArguments,
        true,
        false,
        !Candidate->ParamArrayExpanded,
        Candidate->ParamArrayExpanded,
        Candidate->NotCallable,
        ArgumentArityBad,
        Candidate->RequiresNarrowingConversion,
        RequiresSomeConversion,
        Candidate->AllNarrowingIsFromObject,
        Candidate->AllNarrowingIsFromNumericLiteral,
        Candidate->InferenceFailed,
        Candidate->AllFailedInferenceIsDueToObject,
        false,
        Candidate->IsExtensionMethod,
        NULL,
        Candidate->DelegateRelaxationLevel,
        Candidate->TypeInferenceLevel,
        Candidate->RequiresUnwrappingNullable,
        Candidate->RequiresInstanceMethodBinding,
        Candidate->UsedDefaultForAnOptionalParameter,
        ppAsyncSubArgumentListAmbiguity
    );

    Candidate->ArgumentMatchingDone = true;
}

bool
Semantics::CandidateIsLiftedOperator
(
    _In_opt_ OverloadList * Candidate
)
{
    return
        Candidate &&
        Candidate->Candidate &&
        Candidate->Candidate->IsUserDefinedOperator() &&
        Candidate->Candidate->PUserDefinedOperator() &&
        Candidate->Candidate->PUserDefinedOperator()->GetOperatorMethod() &&
        Candidate->Candidate->PUserDefinedOperator()->GetOperatorMethod()->IsLiftedOperatorMethod();
}

bool
Semantics::IsSingleton
(
    OverloadList * Candidates
)
{
    return
        Candidates &&
        !Candidates->Candidate->IsUserDefinedOperator() &&
        (
            !Candidates->Next ||
            (
                Candidates->Next->Next == NULL &&
                Candidates->Candidate == Candidates->Next->Candidate
            )
        );
}


// SharedErrorCount -- for Dev10#575987, this is a simple structure for counting how many times a given error has occured
struct SharedErrorCount
{
    CompileError *Error;
    unsigned int Count;

    SharedErrorCount(CompileError *error, unsigned int count) :
        Error(error),
        Count(count)
    {
    }
};


// SharedErrorCounts -- for Dev10#575987, this dynamic array is used to count how many times a set of errors have occured.
// For each error, call "CountError(err)". If this err has been counted before (with same errid, location, text, ...)
// then we will increase its tally. If not, we'll start a new tally for it. The CompileErrorCounts object
// keeps track of its own memory, i.e. you're free to deallocate "err" immediately after calling CountError.
// Exception: certain errors (e.g. "Not most specific") are about the overload-binding process itself,
// so it would be wrong to share these. We have a ---- of such errors and assume that all other errors
// can be shared. (That's a more general expandable solution than having a ---- of only those errors that 
// can be shared).
class PotentiallySharedErrors : public DynamicArray<SharedErrorCount>
{
public:

    static bool HasArityErrors(ErrorTable *errors)
    {
        ErrorIterator i(errors);
        while (CompileError *error = i.Next())
        {
            if (error->m_errid == ERRID_TooManyArgs || // "Too many arguments"
                error->m_errid == ERRID_NamedArgAlsoOmitted1 || // "Parameter x already has a matching omitted argument"
                error->m_errid == ERRID_NamedArgUsedTwice1 || // "Parameter x already has a matching argument"
                error->m_errid == ERRID_NamedParamNotFound1 || // "x is not a method parameter"
                error->m_errid == ERRID_OmittedArgument1 || // "Argument not specified for parameter x"
                error->m_errid == ERRID_LambdaBindingMismatch1 || // "Nested function does not have a signature that is compatible with delegate"
                error->m_errid == ERRID_LambdaBindingMismatch2 || // "Nested sub does not have a signature that is compatible with delegate"
                error->m_errid == ERRID_LambdaNotDelegate1) // "Lambda expression cannot be converted to '|1' because '|1' is not a delegate type.")
            {
                return true;
            }
        }
        return false;
    }

    static bool IsAboutTypeInference(CompileError *error)
    {
        return (error->m_errid == ERRID_TypeInferenceFailure1 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailure2 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailure3 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicit1 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicit2 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicit3 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureAmbiguous1 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureAmbiguous2 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureAmbiguous3 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicitAmbiguous1 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicitAmbiguous2 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicitAmbiguous3 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoBest1 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoBest2 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoBest3 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicitNoBest1 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicitNoBest2 || // "Data type cannot be inferred ..."
                error->m_errid == ERRID_TypeInferenceFailureNoExplicitNoBest3); // "Data type cannot be inferred ..."
    }

    static bool PreventsSharedErrorReporting(CompileError *error)
    {
        return (error->m_errid == ERRID_NotMostSpecificOverload || // "Not most specific"
                error->m_errid == ERRID_UnboundTypeParam1); // "Type parameter cannot be inferred"
    }

    static bool AreErrorsTheSame(CompileError *newError, CompileError *existingError)
    {
        if (newError->m_errid != existingError->m_errid) return false;
        if (Location::Compare(newError->m_loc, existingError->m_loc)!=0) return false;
        if (newError->m_pCompilerProject != existingError->m_pCompilerProject) return false;
        if (newError->m_Priority != existingError->m_Priority) return false;
        if (wcscmp(newError->m_wszMessage, existingError->m_wszMessage)!=0) return false;
        return true;
    }


    void AddErrorToList(CompileError *newError)
    {
        // Given that we can share this error, we'll see if this error has already been counted.
        for (unsigned int i=0; i<Count(); i++)
        {
            CompileError *existingError = Array()[i].Error;
            if (AreErrorsTheSame(existingError, newError))
            {
                Array()[i].Count += 1;
                return;
            }
        }
        // If not, we'll start a count for it.
        // A note on memory management: This is a DynamicArray, so elements in it get copied at will with their copy-constructor.
        // Our elements are plain old data with no allocation/deallocation abilities.
        // The element's data gets allocated exactly once (here) and deallocated exactly once (in ~CompileErrorCounts).
        // As for the text-buffer, that gets allocated here and deallocated in the ~CompileError destructor.
        if (newError->m_wszMessage == NULL)
        {
            VSFAIL("Unexpected: error with no text");
            return;
        }
        size_t length = wcslen(newError->m_wszMessage);
        size_t bufferSize;
        wchar_t *wszMessage = RawStringBufferUtil::AllocateNonNullCharacterCount(length, &bufferSize);
        memcpy(wszMessage, newError->m_wszMessage, bufferSize);
        CompileError *perror = new (zeromemory) CompileError(newError->m_pCompiler,
                                                             newError->m_pCompilerProject,
                                                             wszMessage,
                                                             newError->m_errid,
                                                             &newError->m_loc,
                                                             newError->m_Priority);
        AddElement(SharedErrorCount(perror, 1));
    }


    // SomeErrorsAreUniversal: If our tally-count showed that at least some errors
    // were reported exactly "requiredCount" times, and we were allowed to share
    // errors, we return true.
    bool SomeErrorsAreUniversal(unsigned int requiredCount)
    {
        for (unsigned int i=0; i<Count(); i++)
        {
            if (Array()[i].Count == requiredCount) return true;
        }

        return false;
    }

    bool ErrorIsUniversal(CompileError *error, unsigned int requiredCount)
    {
        for (unsigned int i=0; i<Count(); i++)
        {
            if (AreErrorsTheSame(Array()[i].Error, error))
            {
                return Array()[i].Count == requiredCount;
            }
        }
        VSFAIL("Why are you asking me about an error that wasn't even in the shared error table?");
        return false;
    }

    ~PotentiallySharedErrors()
    {
        for (unsigned int i=0; i<Count(); i++)
        {
            CompileError *err = Array()[i].Error;
            delete err; // the destructor ~CompileError will deallocate its m_wszMessage.
            Array()[i].Error = NULL;
        }
    }

};


void
Semantics::ReportOverloadResolutionFailure
(
    const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ Type *DelegateType,
    _In_opt_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    unsigned ErrorId,
    ArgumentDetector Detector,
    CandidateProperty CandidateFilter,
    OverloadResolutionFlags OvrldFlags
)
{
    ILTree::Expression *ArgumentsScratch[128], **SavedArguments;
    SaveArguments(
        m_TreeStorage,
        ArgumentsScratch,
        128,
        SavedArguments,
        Arguments,
        ArgumentCount);

    StringBuffer ArgumentErrorsText;

    bool ReportRealErrors = false;

    // VSW#451640 If we only have 2 candidates and it is due to a paramarrray, try
    // to report  the real errors. Also if we only have one candidate.
    if
    (
        (OvrldFlags & (OvrldForceOverloadResolution | OvrldDontReportSingletonErrors)) != (OvrldForceOverloadResolution | OvrldDontReportSingletonErrors) &&
        (
            Candidates &&
            IsSingleton(Candidates)
        )
    )
    {
        ReportRealErrors = true;
    }

    // Dev10#575987: if every candidate generates exactly the same errors (e.g. "Thread.Start(Sub() perkyFlops)" will fail
    // both candidates each with the same error 'perkyFlops not defined'), then we'll just display those errors rather than
    // reporting overload failure. This "SharedErrorTable" is the error-table that will be populated by just the first of the
    // candidates; if all candidates ended up producing the same errors, then we'll simply merge SharedErrorTable into m_Errors.
    //
    // Dev11#46047: actually, that logic was too timid. Let's go for more aggressive logic:
    // * If some candidates had the correct arity, and amongst those there were at at least some errors shared by all candidates
    //   that weren't "cannot infer datatypes", then report just those errors.
    // * Otherwise, stick with the old detailed "Overload Resolution Failed" message.
    bool AllowErrorSharing = true;
    unsigned int NumberOfCandidatesWithCorrectArity = 0;
    PotentiallySharedErrors potentiallySharedErrors;

    while (Candidates)
    {
        if
        (
            CandidateFilter(Candidates) &&
            !CandidateIsLiftedOperator(Candidates)
        )
        {
            // We may have two versions of paramarray methods in the list. So
            // skip the first one.
            OverloadList *CurrentCandidate = Candidates->Next;

            while (CurrentCandidate)
            {
                if (CurrentCandidate->Candidate == Candidates->Candidate && CandidateFilter(CurrentCandidate))
                {
                    goto SkipCandidate;
                }
                CurrentCandidate = CurrentCandidate->Next;
            }

            MakeScratchCopiesOfArguments(
                m_TreeAllocator,
                SavedArguments,
                Arguments);

            if (!ReportRealErrors)
            {
                ErrorTable *SavedErrors = m_Errors;
                ErrorTable CandidateErrorTable(m_Compiler, NULL, NULL);
                m_Errors = &CandidateErrorTable;

                // Detect errors in the argument matching.
                (this->*Detector)
                (
                    CallLocation,
                    ViewAsProcedure(Candidates->Candidate),
                    Candidates->Binding,
                    Arguments,
                    DelegateReturnType,
                    true, //Suppress Method Names in Error Messages
                    Candidates->IsExtensionMethod,
                    OvrldFlags
                );

                m_Errors = SavedErrors;

                ErrorIterator CandidateErrorsIter(&CandidateErrorTable);
                CompileError *CandidateError;

                // Wrap each argument error in text describing the candidate method
                // to which it applies.

                BitVector<> fixedArgumentBitVector;
                if (Candidates->IsExtensionMethod)
                {
                    ThrowIfFalse(Candidates->Candidate->IsExtensionMethod());
                    //Rebuild the fixed argument bit vector for extension methods.
                    //This is lost when we convert the partial bindings to full bindings, and is also mutated when we apply explicit parameters to a partial binding.
                    //We rebuild it here, by examining the signature of the method, so that we can pass it into GetBasicRep below. This will hide "fixed" type parameters
                    //from the procedure signatures stored in methods.
                    ViewAsProcedure(Candidates->Candidate)->GenerateFixedArgumentBitVectorFromFirstParameter(&fixedArgumentBitVector, m_Compiler);
                }

                StringBuffer MethodRepresentation;
                Candidates->Candidate->GetBasicRep
                (
                    m_Compiler,
                    m_Procedure == NULL ? NULL : m_Procedure->GetContainingClass(),
                    &MethodRepresentation,
                    Candidates->Binding.GetGenericBindingForErrorText(),
                    NULL,
                    true,
                    Candidates->IsExtensionMethod ? TIP_ExtensionCall : TIP_Normal,
                    &fixedArgumentBitVector
                );

                bool reportedDetailedError = false;

                bool CandidateHasArityErrors = PotentiallySharedErrors::HasArityErrors(&CandidateErrorTable);
                if (!CandidateHasArityErrors) NumberOfCandidatesWithCorrectArity++;

                if (! (HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf) && CandidateErrorTable.GetErrorCount() > 1))
                {
                    while (CandidateError = CandidateErrorsIter.Next())
                    {
                        if (!CandidateHasArityErrors)
                        {
                            if (PotentiallySharedErrors::PreventsSharedErrorReporting(CandidateError))
                            {
                                AllowErrorSharing = false;
                            }
                            else if (!PotentiallySharedErrors::IsAboutTypeInference(CandidateError))
                            {
                                potentiallySharedErrors.AddErrorToList(CandidateError);
                            }
                        }

                        reportedDetailedError = true;

                        HRESULT ArgumentErrorDescription = 0;

                        if (Candidates->IsExtensionMethod)
                        {
                            ThrowIfFalse(Candidates->Candidate->IsExtensionMethod());
                            ArgumentErrorDescription =
                                HrMakeRepl
                                (
                                    ERRID_ExtensionMethodOverloadCandidate3,
                                    MethodRepresentation.GetString(),
                                    Candidates->Candidate->PProc()->GetContainer()->PClass()->GetQualifiedName(),
                                    CandidateError->m_wszMessage
                                );
                        }
                        else
                        {
                            ArgumentErrorDescription =
                                HrMakeRepl
                                (
                                    ERRID_OverloadCandidate2,
                                    MethodRepresentation.GetString(),
                                    CandidateError->m_wszMessage
                                );
                        }

                        // Append the description of this argument error to the list of collected
                        // argument errors.

                        HRINFO ArgumentErrorInfo;
                        GetHrInfo(ArgumentErrorDescription, &ArgumentErrorInfo);
                        ArgumentErrorsText.AppendString(ArgumentErrorInfo.m_bstrDescription);
                        ReleaseHrInfo(&ArgumentErrorInfo);
                    }
                }
                else
                {
                        reportedDetailedError = true;

                        StringBuffer IncompatibleBinding;
                        IfFailThrow(ResLoadStringRepl(ERRID_DelegateBindingMismatch, &IncompatibleBinding, NULL));

                        HRESULT ArgumentErrorDescription = 0;

                        if (Candidates->IsExtensionMethod)
                        {
                            ThrowIfFalse(Candidates->Candidate->IsExtensionMethod());
                            ArgumentErrorDescription =
                                HrMakeRepl
                                (
                                    ERRID_ExtensionMethodOverloadCandidate3,
                                    MethodRepresentation.GetString(),
                                    Candidates->Candidate->PProc()->GetContainer()->PClass()->GetQualifiedName(),
                                    IncompatibleBinding.GetString()
                                );
                        }
                        else
                        {
                            ArgumentErrorDescription =
                                HrMakeRepl
                                (
                                    ERRID_OverloadCandidate2,
                                    MethodRepresentation.GetString(),
                                    IncompatibleBinding.GetString()
                                );
                        }

                        // Append the description of this argument error to the list of collected
                        // argument errors.

                        HRINFO ArgumentErrorInfo;
                        GetHrInfo(ArgumentErrorDescription, &ArgumentErrorInfo);
                        ArgumentErrorsText.AppendString(ArgumentErrorInfo.m_bstrDescription);
                        ReleaseHrInfo(&ArgumentErrorInfo);
                }

                if (!reportedDetailedError)
                {
                    HRESULT ArgumentErrorDescription = 0;

                    if (Candidates->IsExtensionMethod)
                    {
                        ThrowIfFalse(Candidates->Candidate->IsExtensionMethod());
                        ArgumentErrorDescription =
                            HrMakeRepl
                            (
                                ERRID_ExtensionMethodOverloadCandidate2,
                                MethodRepresentation.GetString(),
                                Candidates->Candidate->PProc()->GetContainer()->PClass()->GetQualifiedName()
                            );
                    }
                    else
                    {
                        ArgumentErrorDescription =
                            HrMakeRepl
                            (
                                ERRID_OverloadCandidate1,
                                MethodRepresentation.GetString()
                            );
                    }

                    // Append the description of this argument error to the list of collected
                    // argument errors.

                    HRINFO ArgumentErrorInfo;
                    GetHrInfo(ArgumentErrorDescription, &ArgumentErrorInfo);
                    ArgumentErrorsText.AppendString(ArgumentErrorInfo.m_bstrDescription);
                    ReleaseHrInfo(&ArgumentErrorInfo);
                }
            }
            else
            {
                // Report the live errors.
                (this->*Detector)
                (
                    CallLocation,
                    ViewAsProcedure(Candidates->Candidate),
                    Candidates->Binding,
                    Arguments,
                    DelegateReturnType,
                    false, //do not supress method names in error messages
                    Candidates->IsExtensionMethod,
                    OvrldFlags
                );
            }

        }

SkipCandidate:
        Candidates = Candidates->Next;

    }

    RestoreOriginalArguments(SavedArguments, Arguments);

    if (!ReportRealErrors)
    {
        if (DelegateType == NULL && 
            AllowErrorSharing && 
            NumberOfCandidatesWithCorrectArity>=1 && 
            potentiallySharedErrors.SomeErrorsAreUniversal(NumberOfCandidatesWithCorrectArity))
        {
            ErrorTable universalErrorTable(m_Compiler, NULL, NULL);
            for (unsigned int i=0; i<potentiallySharedErrors.Count(); i++)
            {
                if (potentiallySharedErrors.Array()[i].Count < NumberOfCandidatesWithCorrectArity) continue;
                CompileError *err = potentiallySharedErrors.Array()[i].Error;
                universalErrorTable.CreateErrorArgsBase(err->m_errid, err->m_wszMessage, NULL, &err->m_loc, NULL, NULL);
            }

            VSASSERT(universalErrorTable.GetErrorCount()>0, "We should have some errors left!");

            if (m_Errors!=NULL) 
            {
                m_Errors->MergeTemporaryTable(&universalErrorTable);
            }
        }
        else if (DelegateType == NULL)
        {
            ReportSemanticError(
                ErrorId,
                CallLocation,
                // Use just the procedure's name, not its full description.
                OverloadedProcedureName,
                ArgumentErrorsText);
        }
        else
        {
            StringBuffer DelegateRepresentation;
            DelegateType->PNamedRoot()->GetBasicRep(
                m_Compiler,
                m_Procedure == NULL ? NULL : m_Procedure->GetContainingClass(),
                &DelegateRepresentation,
                    DelegateType->IsGenericBinding() ? DelegateType->PGenericBinding() : NULL,
                NULL,
                true);

            ReportSemanticError(
                ErrorId,
                CallLocation,
                // Use just the procedure's name, not its full description.
                OverloadedProcedureName,
                DelegateRepresentation.GetString(),
                ArgumentErrorsText.GetString());
        }
    }
}

void
Semantics::DetectArgumentErrors
(
    const Location &CallLocation,
    Procedure *TargetProcedure,
    GenericBindingInfo GenericBindingContext,
    ExpressionList *Arguments,
    Type *DelegateReturnType,
    bool SuppressMethodNameInErrorMessages,
    bool CandidateIsExtensionMethod,
    OverloadResolutionFlags OvrldFlags
)
{
    bool SomeArgumentsBad = false;
    bool ArgumentArityBad = false;
    bool RequiresNarrowingConversion = false;
    bool RequiresSomeConversion = false;
    bool AllNarrowingIsFromObject = true;
    bool AllNarrowingIsFromNumericLiteral = true;
    bool InferenceFailed = false;
    bool AllFailedInferenceIsDueToObject = true;
    DelegateRelaxationLevel DelegateRelaxationLevel = DelegateRelaxationLevelNone;
    TypeInferenceLevel TypeInferenceLevel = TypeInferenceLevelNone;
    ExpressionList *CopyOutArguments = NULL;
    bool RequiresUnwrappingNullable = false;
    bool RequiresInstanceMethodBinding = false;

    MatchArguments3
    (
        CallLocation,
        TargetProcedure,
        NULL,
        GenericBindingContext,
        Arguments,
        DelegateReturnType,
        ExprNoFlags,
        OvrldFlags,
        CopyOutArguments,
        // CheckValidityOnly is false so that all errors get reported,
        // not just the first one.
        false,
        false,
        false,
        false,
        SomeArgumentsBad,
        ArgumentArityBad,
        RequiresNarrowingConversion,
        RequiresSomeConversion,
        AllNarrowingIsFromObject,
        AllNarrowingIsFromNumericLiteral,
        InferenceFailed,
        AllFailedInferenceIsDueToObject,
        SuppressMethodNameInErrorMessages,
        CandidateIsExtensionMethod,
        DelegateRelaxationLevel,
        TypeInferenceLevel,
        RequiresUnwrappingNullable,
        RequiresInstanceMethodBinding
    );

    // Bug 122092: AddressOf wants just one line for inference errors, if ovrld flag is set
    // matcharguments won't report any, so do it here.
    if (InferenceFailed && HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf))
    {
        if (m_ReportErrors && m_Errors)
        {
            // For the ReportRealErrors case deep inside overload resolution it will report this one in the singleton
            // case. This is extremely hard to detect so this is the best compromise to filter it.
            if (!m_Errors->HasThisErrorWithLocation(ERRID_DelegateBindingTypeInferenceFails, CallLocation))
            {
                ReportSemanticError(
                    ERRID_DelegateBindingTypeInferenceFails,
                    CallLocation);
            }
        }
    }
}

static bool
CandidateIsNotCallable
(
    _In_ OverloadList *Candidate
)
{
    return Candidate->NotCallable;
}

void
Semantics::ReportUncallableProcedures
(
    const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ Type *DelegateType,
    _In_opt_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags
)
{
    ReportOverloadResolutionFailure
    (
        CallLocation,
        OverloadedProcedureName,
        DelegateType,
        Candidates,
        Arguments,
        ArgumentCount,
        DelegateReturnType,
        HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf) ? ERRID_DelegateBindingFailure3 : ERRID_NoCallableOverloadCandidates2,
        &Semantics::DetectArgumentErrors,
        &CandidateIsNotCallable,
        OvrldFlags
    );
}

void
Semantics::DetectArgumentNarrowing
(
    const Location &CallLocation,
    Procedure *TargetProcedure,
    GenericBindingInfo GenericBindingContext,
    ExpressionList *Arguments,
    Type *DelegateReturnType,
    bool SuppressMethodNameInErrorMessages,
    bool CandidateIsExtensionMethod,
    OverloadResolutionFlags OvrldFlags
)
{
    bool SomeArgumentsBad = false;
    bool ArgumentArityBad = false;
    bool RequiresNarrowingConversion = false;
    bool RequiresSomeConversion = false;
    bool AllNarrowingIsFromObject = true;
    bool AllNarrowingIsFromNumericLiteral = true;
    bool InferenceFailed = false;
    bool AllFailedInferenceIsDueToObject = true;
    DelegateRelaxationLevel DelegateRelaxationLevel = DelegateRelaxationLevelNone;
    TypeInferenceLevel TypeInferenceLevel = TypeInferenceLevelNone;
    ExpressionList *CopyOutArguments = NULL;
    bool RequiresUnwrappingNullable = false;
    bool RequiresInstanceMethodBinding = false;

    MatchArguments3
    (
        CallLocation,
        TargetProcedure,
        NULL,
        GenericBindingContext,
        Arguments,
        DelegateReturnType,
        ExprNoFlags,
        OvrldFlags & (OvrldSomeCandidatesAreExtensionMethods),
        CopyOutArguments,
        // CheckValidityOnly is false so that all errors get reported,
        // not just the first one.
        false,
        true,
        false,
        false,
        SomeArgumentsBad,
        ArgumentArityBad,
        RequiresNarrowingConversion,
        RequiresSomeConversion,
        AllNarrowingIsFromObject,
        AllNarrowingIsFromNumericLiteral,
        InferenceFailed,
        AllFailedInferenceIsDueToObject,
        SuppressMethodNameInErrorMessages,
        CandidateIsExtensionMethod,
        DelegateRelaxationLevel,
        TypeInferenceLevel,
        RequiresUnwrappingNullable,
        RequiresInstanceMethodBinding
    );
}

static bool
CandidateIsNarrowing
(
    _In_ OverloadList *Candidate
)
{
    return !Candidate->NotCallable && Candidate->RequiresNarrowingConversion;
}

static bool
CandidateIsNarrowingFromObject
(
    _In_ OverloadList *Candidate
)
{
    return
        !Candidate->NotCallable &&
        Candidate->RequiresNarrowingConversion &&
        Candidate->AllNarrowingIsFromObject;
}

void
Semantics::ReportNarrowingProcedures
(
    const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ Type *DelegateType,
    _In_opt_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags
)
{
    ReportOverloadResolutionFailure
    (
        CallLocation,
        OverloadedProcedureName,
        DelegateType,
        Candidates,
        Arguments,
        ArgumentCount,
        DelegateReturnType,
        HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf) ? ERRID_DelegateBindingFailure3 : ERRID_NoNonNarrowingOverloadCandidates2,
        &Semantics::DetectArgumentNarrowing,
        &CandidateIsNarrowing,
        OvrldFlags
    );
}

void
Semantics::ReportLateBoundConstructors
(
    const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ Type *DelegateType,
    _In_opt_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags
)
{
    ReportOverloadResolutionFailure
    (
        CallLocation,
        OverloadedProcedureName,
        DelegateType,
        Candidates,
        Arguments,
        ArgumentCount,
        DelegateReturnType,
        HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf) ? ERRID_DelegateBindingFailure3 : ERRID_NoNonNarrowingOverloadCandidates2,
        &Semantics::DetectArgumentNarrowing,
        &CandidateIsNarrowingFromObject,
        OvrldFlags
    );
}

static bool
CandidateFailedInference
(
    _In_ OverloadList *Candidate
)
{
    return Candidate->InferenceFailed;
}

void
Semantics::ReportInferenceFailureProcedures
(
    const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ Type *DelegateType,
    _In_opt_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags
)
{
    ReportOverloadResolutionFailure
    (
        CallLocation,
        OverloadedProcedureName,
        DelegateType,
        Candidates,
        Arguments,
        ArgumentCount,
        DelegateReturnType,
        HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf) ? ERRID_DelegateBindingFailure3 : ERRID_NoCallableOverloadCandidates2,
        &Semantics::DetectArgumentErrors,
        &CandidateFailedInference,
        OvrldFlags
    );
}

void
Semantics::DetectUnspecificity
(
    const Location &CallLocation,
    Procedure *TargetProcedure,
    GenericBindingInfo GenericBindingContext,
    ExpressionList *Arguments,
    Type *DelegateReturnType,
    bool SuppressMethodNameInErrorMessages,
    bool CandidateIsExtensionMethod,
    OverloadResolutionFlags OvrldFlags
)
{
    // Bug 122092: AddressOf doesn't want any details behind the methods for
    // specificity because the top message conveys this already.
    // Not reporting an error will just list the method name without quotes.
    if (!HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf))
    {
        ReportSemanticError(ERRID_NotMostSpecificOverload, CallLocation);
    }
}

static bool
CandidateIsUnspecific
(
    _In_ OverloadList *Candidate
)
{
    return
        (!Candidate->NotCallable &&
        !Candidate->RequiresNarrowingConversion &&
        !Candidate->LessSpecific) ||
        Candidate->InferenceFailed;
}

void
Semantics::ReportUnspecificProcedures
(
    const Location &CallLocation,
    _In_z_ Identifier *OverloadedProcedureName,
    _In_opt_ OverloadList *Candidates,
    ExpressionList *Arguments,
    unsigned ArgumentCount,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags
)
{
    ReportOverloadResolutionFailure
    (
        CallLocation,
        OverloadedProcedureName,
        NULL, // This error does not need to report the delegate type in the error.
        Candidates,
        Arguments,
        ArgumentCount,
        DelegateReturnType,
        HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf) ? ERRID_AmbiguousDelegateBinding2 : ERRID_NoMostSpecificOverload2,
        &Semantics::DetectUnspecificity,
        &CandidateIsUnspecific,
        OvrldFlags
    );
}

void
Semantics::CompareNumericTypeSpecificity
(
    Type *LeftType,
    Type *RightType,
    _Out_ bool &LeftWins,
    _Out_ bool &RightWins
)
{
    // The Vtypes order is exactly the ordering of specificity
    // for the numeric types (where a lower value means more
    // specific). This function uses the ordering of Vtypes
    // to determine specificity.
    //
    // This function implements the notion that signed types are
    // preferred over unsigned types during overload resolution.

    VSASSERT(TypeHelpers::IsNumericType(LeftType) && !LeftType->IsEnum() &&
             TypeHelpers::IsNumericType(RightType) && !RightType->IsEnum(),
             "expected only numerics here: #12/10/2003#");

    Vtypes LeftVtype = LeftType->GetVtype();
    Vtypes RightVtype = RightType->GetVtype();

    // When the types are identical, neither is more specific.
    if (LeftVtype != RightVtype)
    {
        if (LeftVtype == t_i1 && RightVtype == t_ui1)
        {
            // Special case: for backwards compatibility, Byte is
            // considered more specific than SByte.
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s wins over %s - special rule for back-compat that Byte is more specific than SByte\n", TypeToDebugString(RightType), TypeToDebugString(LeftType));
            RightWins = true;
        }
        else if (LeftVtype == t_ui1 && RightVtype == t_i1)
        {
            // Special case: for backwards compatibility, Byte is
            // considered more specific than SByte.
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s wins over %s - special rule for back-compat that Byte is more specific than SByte\n", TypeToDebugString(LeftType), TypeToDebugString(RightType));
            LeftWins = true;
        }
        // This comparison depends on the ordering of the Vtype enum.
        else if (LeftVtype < RightVtype)
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s wins over %s\n", TypeToDebugString(LeftType), TypeToDebugString(RightType));
            LeftWins = true;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s wins over %s\n", TypeToDebugString(RightType), TypeToDebugString(LeftType));
            RightWins = true;
        }
    }

    return;
}

void
CompareIntegralTypeSpecificity
(
    Type *LeftType,
    Type *RightType,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins
)
{
    // The Vtypes order is exactly the ordering of specificity
    // for the numeric types (where a lower value means more
    // specific). This function uses the ordering of Vtypes
    // to determine specificity.
    //
    // This function implements the notion that signed types are
    // preferred over unsigned types during overload resolution.

    VSASSERT(TypeHelpers::IsIntegralType(LeftType) && !LeftType->IsEnum() &&
             TypeHelpers::IsIntegralType(RightType) && !RightType->IsEnum(),
             "expected only numerics here: #12/10/2003#");

    Vtypes LeftVtype = LeftType->GetVtype();
    Vtypes RightVtype = RightType->GetVtype();

    // When the types are identical, neither is more specific.
    if (LeftVtype != RightVtype)
    {
        if (LeftVtype == t_i1 && RightVtype == t_ui1)
        {
            // Special case: for backwards compatibility, Byte is
            // considered more specific than SByte.
            RightWins = true;
        }
        else if (LeftVtype == t_ui1 && RightVtype == t_i1)
        {
            // Special case: for backwards compatibility, Byte is
            // considered more specific than SByte.
            LeftWins = true;
        }
        // This comparison depends on the ordering of the Vtype enum.
        else if (LeftVtype < RightVtype)
        {
            if (!IsUnsignedType(LeftVtype) && IsUnsignedType(RightVtype))
            {
                LeftWins = true;
            }
        }
        else
        {
            if (IsUnsignedType(LeftVtype) && !IsUnsignedType(RightVtype))
            {
                RightWins = true;
            }
        }
    }

    return;
}

Type*
GetParamTypeToCompare
(
    Parameter *Parameter,
    bool ExpandParamArray
)
{
    Type *Type = GetDataType(Parameter);
    if (TypeHelpers::IsPointerType(Type))
    {
        Type = TypeHelpers::GetReferencedType(Type->PPointerType());
    }

    if (Parameter->IsParamArray() && ExpandParamArray && TypeHelpers::IsArrayType(Type))
    {
        Type = TypeHelpers::GetElementType(Type->PArrayType());
    }

    return Type;
}

void
Semantics::CompareParameterApplicability
(
    _In_opt_ ILTree::Expression *Argument,
    Parameter *LeftParameter,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    bool ExpandLeftParamArray,
    Parameter *RightParameter,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool ExpandRightParamArray,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins,
    _Inout_ bool &BothLose,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    // ParameterApplicability is in $11.8.1.1 "Applicability"

    // BothLose is offered up as a shortcut from this method, so that our caller can decide to abort its search early.
    // BothLose means "we promise you that neither parameter could ever be part of a winning method".
    //   e.g. f(A,...) and f(B,...) where A->B and B->A are widening conversions and we invoke it with
    //   some argument C which widens to both. The fact of widening conversions each way means that --
    //   regardless of the other parameters -- neither overload of f can ever win.
    // By contrast, !LeftWins && !RightWins && !BothLose means "Comparing these parameters didn't tell anything;
    //   but go on because subsequent parameters might." e.g. f(int,...) and f(int,...).

    Type *LeftType = GetParamTypeToCompare(LeftParameter, ExpandLeftParamArray);
    Type *RightType = GetParamTypeToCompare(RightParameter, ExpandRightParamArray);
    
    LeftType = ReplaceGenericParametersWithArguments(LeftType, LeftBinding, m_SymbolCreator);
    RightType = ReplaceGenericParametersWithArguments(RightType, RightBinding, m_SymbolCreator);


    return CompareParameterTypeApplicability(Argument,LeftType,LeftProcedure,RightType,RightProcedure,LeftWins,RightWins,BothLose);
}

void
Semantics::CompareParameterTypeApplicability
(
    _In_opt_ ILTree::Expression *Argument,
    Type *LeftType,
    Procedure *LeftProcedure,
    Type *RightType,
    Procedure *RightProcedure,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins,
    _Inout_ bool &BothLose
)
{
    if (TypeHelpers::IsNumericType(LeftType) && TypeHelpers::IsNumericType(RightType) &&
        !LeftType->IsEnum() && !RightType->IsEnum())
    {
        CompareNumericTypeSpecificity(LeftType, RightType, LeftWins, RightWins);
        return;
    }

    // If both the types are different only by generic method type parameters
    // with the same index position, then treat as identity
    //
    if (LeftProcedure->IsGeneric() && RightProcedure->IsGeneric())
    {
        unsigned CompareFlags =
            BCSYM::CompareReferencedTypes(
                LeftType,
                (GenericBinding *)NULL,
                RightType,
                (GenericBinding *)NULL,
                &m_SymbolCreator);


        if (CompareFlags == EQ_Match ||
            CompareFlags == EQ_GenericMethodTypeParams)
        {
            return;
        }
    }

    Procedure *OperatorMethod = NULL;
    GenericBinding *OperatorMethodGenericContext = NULL;
    bool    OperatorMethodIsLifted;

    ConversionClass LeftToRight = ClassifyConversion(RightType, LeftType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);

    if (LeftToRight == ConversionIdentity)
    {
        return;
    }

    if (LeftToRight == ConversionWidening)
    {
        if (OperatorMethod &&
            ClassifyConversion(LeftType, RightType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted) == ConversionWidening)
        {
            // Although W<-->W conversions don't exist in the set of predefined conversions,
            // it can occur with user-defined conversions.  If the two param types widen to each other
            // (necessarily by using user-defined conversions), and the argument type is known and
            // is identical to one of the parameter types, then that parameter wins.  Otherwise,
            // if the argument type is not specified, we can't make a decision and both lose.

            if (Argument && Argument->ResultType && TypeHelpers::EquivalentTypes(Argument->ResultType, LeftType))
            {
                DBG_SWITCH_PRINTF(fDumpOverload, L"    Parameter %s:%s is more applicable than %s:%s - $11.8.1.1\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
                LeftWins = true;
                return;
            }

            if (Argument && Argument->ResultType && TypeHelpers::EquivalentTypes(Argument->ResultType, RightType))
            {
                DBG_SWITCH_PRINTF(fDumpOverload, L"    Parameter %s:%s is more applicable than %s:%s - $11.8.1.1\n", CandidateToDebugString(RightProcedure), TypeToDebugString(RightType), CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType));
                RightWins = true;
                return;
            }

            DBG_SWITCH_PRINTF(fDumpOverload, L"    Both %s:%s and %s:%s are mutually more applicable, so both lose - $11.8.1.1\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
            BothLose = true;
            return;
        }

        // Enforce the special rule that zero literals widening to enum don't count if we see a non-enum
        // type. If we've gotten to this point, we should have eliminated all narrowing conversion methods,
        // which means that if we see a zero literal, the enum should go.
        if (Argument && LeftType->IsEnum() && !RightType->IsEnum() && IsIntegerZeroLiteral(Argument))
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s, since zero literals widening to enum don't count if we see a non-enum type\n", CandidateToDebugString(RightProcedure), TypeToDebugString(RightType), CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType));
            RightWins = true;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s widens to %s:%s and hence wins over it\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
            LeftWins = true;
        }

        return;
    }

    ConversionClass RightToLeft = ClassifyConversion(LeftType, RightType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);
    if (RightToLeft == ConversionWidening)
    {
        // Enforce the special rule that zero literals widening to enum don't count if we see a non-enum
        // type. If we've gotten to this point, we should have eliminated all narrowing conversion methods,
        // which means that if we see a zero literal, the enum should go.
        if (Argument && RightType->IsEnum() && !LeftType->IsEnum() && IsIntegerZeroLiteral(Argument))
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s, since zero literals widening to enum don't count if we see a non-enum type\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
            LeftWins = true;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s widens to %s:%s and hence wins over it\n", CandidateToDebugString(RightProcedure), TypeToDebugString(RightType), CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType));
            RightWins = true;
        }
        return;
    }

    //Expression type is more specific when it's return type is more specific
    Type * GenericExpressionType = GetFXSymbolProvider()->GetGenericExpressionType();
    bool LeftTypeIsExpr = (GenericExpressionType && TypeHelpers::IsGenericTypeBinding(LeftType) &&
                                                TypeHelpers::EquivalentTypes(LeftType->PGenericTypeBinding()->GetGenericType(),
                                                                            GenericExpressionType));

    bool RightTypeIsExpr = (GenericExpressionType && TypeHelpers::IsGenericTypeBinding(RightType) &&
                                                TypeHelpers::EquivalentTypes(RightType->PGenericTypeBinding()->GetGenericType(),
                                                                            GenericExpressionType));

    if(LeftTypeIsExpr && (RightTypeIsExpr || RightType->IsDelegate()))
    {
        // we've got an Expression(Of T), skip through to T
        LeftType = LeftType->PGenericTypeBinding()->GetArgument(0);
    }

    if(RightTypeIsExpr && LeftType->IsDelegate())
    {
        // we've got an Expression(Of T), skip through to T
        RightType = RightType->PGenericTypeBinding()->GetArgument(0);
    }

    if (Argument)
    {
        // Delegate type is more specific when it's return type is more specific
        if(LeftType->IsDelegate() && RightType->IsDelegate())
        {
            //First we need to make sure that the delegate argumetns are more specific....

            ILTree::Expression * newArgument = NULL;

            if(Argument->bilop == SX_LAMBDA)
            {
                ILTree::LambdaExpression * Lambda=&Argument->AsLambdaExpression();

                if(TypeHelpers::IsVoidType(Lambda->ResultType) || Lambda->ResultType->IsAnonymousDelegate())
                {
                    if (Lambda->IsStatementLambda)
                    {
                        // We need an expression whose result type is the return type of the delegate that the lambda returns.
                        // Therefore, we create a Nothing expression that contains with the necessary result type.
                        newArgument = AllocateExpression(SX_NOTHING, 
                                                         Lambda->GetStatementLambdaBody()->pReturnType, 
                                                         Lambda->Loc);
                    }
                    else
                    {
                        newArgument = Lambda->GetExpressionLambdaBody();
                    }
                }
            }

            // Port SP1 CL 2954860 to VS10
            //don't match Delegate to an Expression(Of T) unless we were given a Lambda as an argument
            bool CanCompareDelegateTypes = newArgument || LeftTypeIsExpr == RightTypeIsExpr || Argument->bilop == SX_UNBOUND_LAMBDA;

            if(CanCompareDelegateTypes)
            {
                Type * returnTypes[2]={NULL,NULL};
                Type * delegateTypes[2]={LeftType,RightType};
                for(int i=0; i<2; i++)
                {
                    Declaration *InvokeMethod = GetInvokeFromDelegate(delegateTypes[i], m_Compiler);

                    if (InvokeMethod && !IsBad(InvokeMethod) && InvokeMethod->IsProc())
                    {
                        Type *DelegateReturnType = InvokeMethod->PProc()->GetType();

                        if(DelegateReturnType && !TypeHelpers::IsBadType(DelegateReturnType))
                        {
                            GenericTypeBinding *DelegateBindingContext = TypeHelpers::IsGenericTypeBinding(delegateTypes[i]) ? delegateTypes[i]->PGenericTypeBinding() : NULL;
                            DelegateReturnType = ReplaceGenericParametersWithArguments(DelegateReturnType, DelegateBindingContext, m_SymbolCreator);

                            if(DelegateReturnType && !TypeHelpers::IsBadType(DelegateReturnType))
                            {
                                returnTypes[i] = DelegateReturnType;
                            }
                        }
                    }
                }
                if(returnTypes[0] && !TypeHelpers::IsVoidType(returnTypes[0]) && returnTypes[1] && !TypeHelpers::IsVoidType(returnTypes[1]))
                {
                    return CompareParameterTypeApplicability(newArgument,returnTypes[0],LeftProcedure,returnTypes[1],RightProcedure,LeftWins,RightWins,BothLose);
                }

           }
        }
    }

    // Bug 36231 - DevDiv Bugs
    if(Argument && Argument->ResultType)
    {
        if (TypeHelpers::EquivalentTypes(Argument->ResultType, LeftType))
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s - see http://bugcheck/bugs/DevDivBugs/36231\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
            LeftWins = true;
            return;
        }

        if (TypeHelpers::EquivalentTypes(Argument->ResultType, RightType))
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s - see http://bugcheck/bugs/DevDivBugs/36231\n", CandidateToDebugString(RightProcedure), TypeToDebugString(RightType), CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType));
            RightWins = true;
            return;
        }
    }

    DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s and %s:%s both lose at applicability\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
    BothLose = true;
    return;
}


void
Semantics::CompareParameterGenericDepth
(
    _In_opt_ ILTree::Expression *Argument,
    Parameter *LeftParameter,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    bool ExpandLeftParamArray,
    Parameter *RightParameter,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool ExpandRightParamArray,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins,
    _Inout_ bool &BothLose,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    // Generic parameter depth is $11.8.1.3 "Specific parameter types"

    Type *LeftType = GetParamTypeToCompare(LeftParameter, ExpandLeftParamArray);
    Type *RightType = GetParamTypeToCompare(RightParameter, ExpandRightParamArray);

    // $7.11: Generic-parameter-depth tie-breaker is only used between two candidates whose
    // substituted parameter types are identical to each other.
    //
    // 



    Type *LeftSubstitutedType = ReplaceGenericParametersWithArguments(LeftType, LeftBinding, m_SymbolCreator);
    Type *RightSubstitutedType = ReplaceGenericParametersWithArguments(RightType, RightBinding, m_SymbolCreator);
    if (!TypeHelpers::EquivalentTypes(LeftSubstitutedType, RightSubstitutedType))
    {
#if DEBUG
        // EquivalentTypes is cheaper than doing the "ClassifyConversion==Identity" test.
        // But ClassifyConversion is used elsewhere, so it's nice to have a sanity check that they're the same.
        Procedure *OperatorMethod = NULL;
        GenericBinding *OperatorMethodGenericContext = NULL;
        bool    OperatorMethodIsLifted;
        ConversionClass LeftToRight = ClassifyConversion(RightSubstitutedType, LeftSubstitutedType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);
        OperatorMethod = NULL;
        OperatorMethodGenericContext = NULL;
        ConversionClass RightToLeft = ClassifyConversion(LeftSubstitutedType, RightSubstitutedType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);
        VSASSERT(LeftToRight != ConversionIdentity && RightToLeft != ConversionIdentity, "Unexpected: ConversionIdentity should be symmetric, and true if and only if AreTypesEqual");
#endif
        BothLose = true;
        return;
    }

    CompareParameterTypeGenericDepth(LeftType,RightType,LeftWins,RightWins);

#if DEBUG
    if (LeftWins)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s has better (deeper) generic depth than %s:%s - $11.8.1.3\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
    }
    else if (RightWins)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s has better (deeper) generic depth than %s:%s - $11.8.1.3\n", CandidateToDebugString(RightProcedure), TypeToDebugString(RightType), CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType));
    }
    else if (BothLose)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s and %s:%s both lose the generic-parameter-depth. (??? HOW???) - $11.8.1.3\n", CandidateToDebugString(LeftProcedure), TypeToDebugString(LeftType), CandidateToDebugString(RightProcedure), TypeToDebugString(RightType));
    }
#endif
}

void
Semantics::CompareParameterTypeGenericDepth
(
    Type *LeftType,
    Type *RightType,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins
)
{
    if (LeftType == RightType)
    {
        return; // shortcut
    }
    else if (LeftType->IsGenericParam() && !RightType->IsGenericParam())
    {
        RightWins = true;
        return;
    }
    else if (RightType->IsGenericParam() && !LeftType->IsGenericParam())
    {
        LeftWins = true;
        return;
    }
    else if (LeftType->IsArrayType() && RightType->IsArrayType() &&
             LeftType->PArrayType()->GetRank() == RightType->PArrayType()->GetRank())
    {
        return CompareParameterTypeGenericDepth(LeftType->PArrayType()->GetRoot(), RightType->PArrayType()->GetRoot(), LeftWins, RightWins);
    }
    else if (LeftType->IsGenericTypeBinding() && RightType->IsGenericTypeBinding() &&
             LeftType->PGenericTypeBinding()->GetArgumentCount() == RightType->PGenericTypeBinding()->GetArgumentCount())
    {
        bool NestedLeftWinsAny=false, NestedRightWinsAny=false;

        // Now we compare all arguments. Note: this algorithm could run right-to-left, or left-to-right,
        // and I'm doing it right-to-left just to save calling "GetArgumentCount" each time. ---- efficiency, I know.
        for (int i=LeftType->PGenericTypeBinding()->GetArgumentCount()-1; i>=0; i--)
        {
            Type *LeftArg = LeftType->PGenericTypeBinding()->GetArgument(i);
            Type *RightArg = RightType->PGenericTypeBinding()->GetArgument(i);
            CompareParameterTypeGenericDepth(LeftArg, RightArg, NestedLeftWinsAny, NestedRightWinsAny);

            if (NestedLeftWinsAny && NestedRightWinsAny)
            {
                return; // shortcut
            }
        }

        // nb. here we know that we don't have both LeftArgWinsAny && RightArgWinsAny
        if (NestedLeftWinsAny)
        {
            LeftWins = true;
            return;
        }
        else if (NestedRightWinsAny)
        {
            RightWins = true;
            return;
        }
        else
        {
            return;
        }
    }

    return;
}


// 
void
Semantics::CompareParameterIntegralSpecificity
(
    ILTree::Expression *Argument,
    Parameter *LeftParameter,
    Procedure *LeftProcedure,
    GenericBinding *LeftBinding,
    bool ExpandLeftParamArray,
    Parameter *RightParameter,
    Procedure *RightProcedure,
    GenericBinding *RightBinding,
    bool ExpandRightParamArray,
    _Inout_ bool &LeftWins,
    _Inout_ bool &RightWins,
    _Inout_ bool &BothLose
)
{
    Type *LeftType = GetParamTypeToCompare(LeftParameter, ExpandLeftParamArray);
    Type *RightType = GetParamTypeToCompare(RightParameter, ExpandRightParamArray);

    LeftType = ReplaceGenericParametersWithArguments(LeftType, LeftBinding, m_SymbolCreator);
    RightType = ReplaceGenericParametersWithArguments(RightType, RightBinding, m_SymbolCreator);

    if (TypeHelpers::IsIntegralType(LeftType) && TypeHelpers::IsIntegralType(RightType) &&
        !LeftType->IsEnum() && !RightType->IsEnum())
    {
        CompareIntegralTypeSpecificity(LeftType, RightType, LeftWins, RightWins);
        return;
    }
}


void
Semantics::CompareGenericityBasedOnMethodGenericParams
(
    ILTree::Expression *Argument,
    Parameter *LeftParameter,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    bool ExpandLeftParamArray,
    Parameter *RightParameter,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool ExpandRightParamArray,
    _Out_ bool &LeftIsLessGeneric,
    _Out_ bool &RightIsLessGeneric,
    _Out_ bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    CompareTypeGenericityBasedOnMethodGenericParams
    (
        Argument,
        GetParamTypeToCompare(LeftParameter, ExpandLeftParamArray),
        LeftProcedure,
        LeftBinding,
        GetParamTypeToCompare(RightParameter, ExpandRightParamArray),
        RightProcedure,
        RightBinding,
        LeftIsLessGeneric,
        RightIsLessGeneric,
        SignatureMismatch,
        LeftIsExtensionMethod,
        RightIsExtensionMethod
    );
}

void
Semantics::CompareTypeGenericityBasedOnMethodGenericParams
(
    Type *LeftParameterType,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Type *RightParameterType,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool &LeftIsLessGeneric,
    bool &RightIsLessGeneric,
    bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod

)
{
    CompareTypeGenericityBasedOnMethodGenericParams(
        NULL,
        LeftParameterType,
        LeftProcedure,
        LeftBinding,
        RightParameterType,
        RightProcedure,
        RightBinding,
        LeftIsLessGeneric,
        RightIsLessGeneric,
        SignatureMismatch,
        LeftIsExtensionMethod,
        RightIsExtensionMethod
    );
}

void
Semantics::CompareTypeGenericityBasedOnMethodGenericParams
(
    ILTree::Expression *Argument,
    Type *LeftParameterType,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Type *RightParameterType,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    _Out_ bool &LeftIsLessGeneric,
    _Out_ bool &RightIsLessGeneric,
    _Out_ bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    SignatureMismatch = false;

    if (CompareGenericityIsSignatureMismatch(Argument, LeftParameterType, LeftBinding, RightParameterType, RightBinding))
    {
        // The signatures of the two methods are not identical and so the "least generic" rule
        // does not apply.

        SignatureMismatch = true;
        return;
    }

    // Only references to generic parameters of the procedures count. For the purpose of this
    // function, references to generic parameters of a type do not make a procedure more generic.

    if (RefersToGenericParameter(LeftParameterType, LeftProcedure,LeftBinding.GetFixedTypeArgumentBitVector()))
    {
        if (!RefersToGenericParameter(RightParameterType, RightProcedure, RightBinding.GetFixedTypeArgumentBitVector()))
        {
            RightIsLessGeneric = true;
        }
    }
    else if (RefersToGenericParameter(RightParameterType, RightProcedure, RightBinding.GetFixedTypeArgumentBitVector()))
    {
        LeftIsLessGeneric = true;
    }
}
bool
Semantics::CompareGenericityIsSignatureMismatch
(
    ILTree::Expression *Argument,
    Type *LeftParameterType,
    GenericBindingInfo LeftBinding,
    Type *RightParameterType,
    GenericBindingInfo RightBinding
)
{
    bool SignatureMismatch = false;

    Type * leftType = ReplaceGenericParametersWithArguments(LeftParameterType, LeftBinding, m_SymbolCreator);
    Type * rightType = ReplaceGenericParametersWithArguments(RightParameterType, RightBinding, m_SymbolCreator);

    if (!TypeHelpers::EquivalentTypes(
            leftType,
            rightType))
    {
        SignatureMismatch = true;

        Type * GenericExpressionType = GetFXSymbolProvider()->GetGenericExpressionType();
        bool LeftTypeIsExpr = (GenericExpressionType && TypeHelpers::IsGenericTypeBinding(leftType) &&
                                                    TypeHelpers::EquivalentTypes(leftType->PGenericTypeBinding()->GetGenericType(),
                                                                                GenericExpressionType));

        bool RightTypeIsExpr = (GenericExpressionType && TypeHelpers::IsGenericTypeBinding(rightType) &&
                                                    TypeHelpers::EquivalentTypes(rightType->PGenericTypeBinding()->GetGenericType(),
                                                                                GenericExpressionType));

        if(LeftTypeIsExpr && (RightTypeIsExpr || rightType->IsDelegate()))
        {
            // we've got an Expression(Of T), skip through to T
            leftType = leftType->PGenericTypeBinding()->GetArgument(0);
        }

        if(RightTypeIsExpr && leftType->IsDelegate())
        {
            // we've got an Expression(Of T), skip through to T
            rightType = rightType->PGenericTypeBinding()->GetArgument(0);
        }

        if(leftType->IsDelegate() && rightType->IsDelegate())
        {
            ILTree::Expression * newArgument = NULL;

            if(Argument && Argument->bilop == SX_LAMBDA)
            {
                ILTree::LambdaExpression * Lambda=&Argument->AsLambdaExpression();

                newArgument = Lambda->GetExpressionLambdaBody();
            }

            bool CanCompareDelegateTypes = newArgument || LeftTypeIsExpr == RightTypeIsExpr || (Argument != NULL && Argument->bilop == SX_UNBOUND_LAMBDA);

            // don't match Delegate to an Expression(Of T) unless we were given a Lambda as an argument
            if(CanCompareDelegateTypes)
            {
                Procedure * invokeMethods[2]={NULL,NULL};
                Type * delegateTypes[2]={leftType,rightType};

                for(int i=0; i<2; i++)
                {
                    Declaration *InvokeMethod = GetInvokeFromDelegate(delegateTypes[i], m_Compiler);

                    if (InvokeMethod && !IsBad(InvokeMethod) && InvokeMethod->IsProc())
                    {
                        invokeMethods[i] = ViewAsProcedure(InvokeMethod);
                    }
                }

                unsigned CompareFlags =
                                    BCSYM::CompareProcs(
                                        invokeMethods[0],
                                        TypeHelpers::IsGenericTypeBinding(delegateTypes[0]) ? delegateTypes[0]->PGenericTypeBinding() : NULL,
                                        invokeMethods[1],
                                        TypeHelpers::IsGenericTypeBinding(delegateTypes[1]) ? delegateTypes[1]->PGenericTypeBinding() : NULL,
                                        &m_SymbolCreator);


                if((CompareFlags & (EQ_Shape | EQ_Byref  | EQ_Return)) == 0)
                {
                    // signatures are the same
                    SignatureMismatch = false;
                }
            }
        }
    }

    return SignatureMismatch;
}


void
Semantics::CompareGenericityBasedOnTypeGenericParams
(
    ILTree::Expression *Argument,
    Parameter *LeftParameter,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    bool ExpandLeftParamArray,
    Parameter *RightParameter,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool ExpandRightParamArray,
    _Out_ bool &LeftIsLessGeneric,
    _Out_ bool &RightIsLessGeneric,
    _Out_ bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    CompareTypeGenericityBasedOnTypeGenericParams(
        Argument,
        GetParamTypeToCompare(LeftParameter, ExpandLeftParamArray),
        LeftProcedure,
        LeftBinding,
        GetParamTypeToCompare(RightParameter, ExpandRightParamArray),
        RightProcedure,
        RightBinding,
        LeftIsLessGeneric,
        RightIsLessGeneric,
        SignatureMismatch,
        LeftIsExtensionMethod,
        RightIsExtensionMethod);
}

void
Semantics::CompareTypeGenericityBasedOnTypeGenericParams
(
    Type *LeftParameterType,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Type *RightParameterType,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    _Out_ bool &LeftIsLessGeneric,
    _Out_ bool &RightIsLessGeneric,
    _Out_ bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    CompareTypeGenericityBasedOnTypeGenericParams(
        NULL,
        LeftParameterType,
        LeftProcedure,
        LeftBinding,
        RightParameterType,
        RightProcedure,
        RightBinding,
        LeftIsLessGeneric,
        RightIsLessGeneric,
        SignatureMismatch,
        LeftIsExtensionMethod,
        RightIsExtensionMethod);
}

bool
Semantics::RefersToGenericTypeParameter
(
    bool ProcedureIsExtensionMethod,
    Procedure * Procedure,
    Type * Type,
    GenericBindingInfo Binding
)
{
    if (ProcedureIsExtensionMethod)
    {
        AutoPtr<NegatedBitVector> autoBitVector;

        IReadonlyBitVector * pBitVector = Binding.GetFixedTypeArgumentBitVector();

        if (pBitVector)
        {
            autoBitVector.Attach(new (zeromemory) NegatedBitVector(pBitVector));
            pBitVector = autoBitVector;
        }

        return
            RefersToGenericParameter(Type, Procedure,pBitVector);
    }
    else
    {
        return RefersToGenericParameter(Type, NULL, NULL, NULL, 0, NULL, true /*IgnoreGenericMethodParams*/);
    }
}

void
Semantics::CompareTypeGenericityBasedOnTypeGenericParams
(
    ILTree::Expression *Argument,
    Type *LeftParameterType,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Type *RightParameterType,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    _Out_ bool &LeftIsLessGeneric,
    _Out_ bool &RightIsLessGeneric,
    _Out_ bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    SignatureMismatch = false;

    if (CompareGenericityIsSignatureMismatch(Argument, LeftParameterType, LeftBinding, RightParameterType, RightBinding))
    {
        // The signatures of the two methods are not identical and so the "least generic" rule
        // does not apply.

        SignatureMismatch = true;
        return;
    }

    // Only references to generic parameters of the generic types count. For the purpose of this
    // function, references to generic parameters of a method do not make a procedure more generic.
    //
    const bool IgnoreGenericMethodParams = true;

    if (RefersToGenericTypeParameter(LeftIsExtensionMethod, LeftProcedure, LeftParameterType, LeftBinding))
    {
        if (!RefersToGenericTypeParameter(RightIsExtensionMethod, RightProcedure, RightParameterType, RightBinding))
        {
            RightIsLessGeneric = true;
        }
    }
    else if (RefersToGenericTypeParameter(RightIsExtensionMethod, RightProcedure, RightParameterType, RightBinding))
    {
        LeftIsLessGeneric = true;
    }
}

Procedure *
Semantics::LeastGenericProcedure
(
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool ConsiderReturnTypesToo,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    // If both the procedures are neither generic nor members of generic types, then they cannot be
    // disambiguated by their genericity. So early exit for this case which would be quite common.
    //
    if (!IsGenericOrHasGenericParent(LeftProcedure) && !IsGenericOrHasGenericParent(RightProcedure))
    {
        return NULL;
    }

    bool SignatureMismatch = false;

    Procedure *LeastGeneric =
        LeastGenericProcedure(
            LeftProcedure,
            LeftBinding,
            RightProcedure,
            RightBinding,
            &Semantics::CompareTypeGenericityBasedOnMethodGenericParams,
            ConsiderReturnTypesToo,
            SignatureMismatch,
            LeftIsExtensionMethod,
            RightIsExtensionMethod);

    if (LeastGeneric == NULL && !SignatureMismatch)
    {
        LeastGeneric =
            LeastGenericProcedure(
                LeftProcedure,
                LeftBinding,
                RightProcedure,
                RightBinding,
                &Semantics::CompareTypeGenericityBasedOnTypeGenericParams,
                ConsiderReturnTypesToo,
                SignatureMismatch,
                LeftIsExtensionMethod,
                RightIsExtensionMethod);
    }

    return LeastGeneric;
}

Procedure *
Semantics::LeastGenericProcedure
(
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    ParameterTypeCompare Compare,
    bool ConsiderReturnTypesToo,
    _Out_ bool &SignatureMismatch,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
)
{
    bool LeftWinsAtLeastOnce = false;
    bool RightWinsAtLeastOnce = false;
    SignatureMismatch = false;

    Parameter *LeftParameter = LeftProcedure->GetFirstParam();
    Parameter *RightParameter = RightProcedure->GetFirstParam();

    while (LeftParameter && RightParameter)
    {
        (this->*Compare)(
            GetParamTypeToCompare(LeftParameter, false),
            LeftProcedure,
            LeftBinding,
            GetParamTypeToCompare(RightParameter, false),
            RightProcedure,
            RightBinding,
            LeftWinsAtLeastOnce,
            RightWinsAtLeastOnce,
            SignatureMismatch,
            LeftIsExtensionMethod,
            RightIsExtensionMethod);

        if (SignatureMismatch ||
            (LeftWinsAtLeastOnce && RightWinsAtLeastOnce))
        {
            return NULL;
        }

        LeftParameter = LeftParameter->GetNext();
        RightParameter = RightParameter->GetNext();
    }

    VSASSERT(!(LeftWinsAtLeastOnce && RightWinsAtLeastOnce), "Least generic method logic is confused.");

    if (LeftParameter || RightParameter)
    {
        // The procedures have different numbers of parameters, and so don't have matching signatures.
        return NULL;
    }

    // For operators, return types also influence overload resolution. So check the return types to
    // determine the genericity. This code is general so that in the future this could be used
    //
    if (ConsiderReturnTypesToo)
    {
        // Currently only operators require this. So assert for non-operators.
        VSASSERT(LeftProcedure->IsUserDefinedOperator() &&
                 RightProcedure->IsUserDefinedOperator(), "Return type comparison unexpected for non-operators!!!");

        Type *LeftReturnType = LeftProcedure->GetType() ;
        Type *RightReturnType = RightProcedure->GetType();

        if (!!LeftReturnType != !!RightReturnType)
        {
            SignatureMismatch = true;
            return NULL;
        }

        if (LeftReturnType)
        {
            (this->*Compare)(
                LeftReturnType,
                LeftProcedure,
                LeftBinding,
                RightReturnType,
                RightProcedure,
                RightBinding,
                LeftWinsAtLeastOnce,
                RightWinsAtLeastOnce,
                SignatureMismatch,
                LeftIsExtensionMethod,
                RightIsExtensionMethod);

            if (SignatureMismatch ||
                (LeftWinsAtLeastOnce && RightWinsAtLeastOnce))
            {
                return NULL;
            }
        }
    }

    if (LeftWinsAtLeastOnce)
    {
        return LeftProcedure;
    }

    if (RightWinsAtLeastOnce)
    {
        return RightProcedure;
    }

    return NULL;
}

//This procedure should be kept in sync with MostSpecificProcedure.
//In particular, if you add any new criteria to MostSpecificProcedure that will
//cause one procedure to be more specific to another, then that same criteria
//should be added to this procedure, such that the new condition will cause it to return false.
//Conversly, any new criteria added here that cause a value of false to be returned
//should be added to MostSpecificProcedure to cause it to either return a procedure or
//to set BothLoose to true. Any criteria added here that causes true to be returned
//should be added to MostSpecificProcedure in a way that causes it to return null.
bool
Semantics::AreProceduresEquallySpecific
(
    OverloadList *Left,
    OverloadList *Right,
    ExpressionList *Arguments,
    OverloadResolutionFlags OvrldFlags,
    unsigned & LeftParamArrayCount,
    unsigned & RightParamArrayCount
)
{
    bool BothLose = false;

    bool LeftWinsAtLeastOnce = false;
    bool RightWinsAtLeastOnce = false;

    Procedure *LeftProcedure = ViewAsProcedure(Left->Candidate);
    Procedure *RightProcedure = ViewAsProcedure(Right->Candidate);

    Parameter *LeftParameter = LeftProcedure->GetFirstParam();
    Parameter *RightParameter = RightProcedure->GetFirstParam();

    ILTree::Expression *RemainingArguments = Arguments;

    if (OvrldFlags & OvrldSomeCandidatesAreExtensionMethods)
    {
        Assume(RemainingArguments, L"Cannot skip non existant argument!");
        RemainingArguments = RemainingArguments->AsExpressionWithChildren().Right;
    }

    if (Left->IsExtensionMethod)
    {
        Assume(LeftParameter,  L"Cannot skip non-existant parameter");
        LeftParameter = LeftParameter->GetNext();
    }

    if (Right->IsExtensionMethod)
    {
        Assume(RightParameter, L"Cannot skip non-existant parameter");
        RightParameter = RightParameter->GetNext();
    }

    // Compare the parameters that match the supplied positional arguments

    for (int phase=0; phase<2; phase++)
    {
        // Here we are concerned with tie-breaker rules.
        // The first phase embodies $11.8.1:6 "Eliminate the less applicable members
        //   followed by 7.9/7.10/7.11 "delegate relaxation level"
        //   and then by 7.8 "type inference level".
        //   Yes, the implementation and the spec are completely out of whack.
        // The second phase embodies $11.8.1:7.13 "Eliminate the less specific parameter types"
        //
        // It happened to be easier to leave the implementation untouched and to repeat
        // 7.9/7.10/7.11/7.8 an additional useless time in the second phase... easier because
        // we could do a "for" loop rather than a huge factoring-out.

        ParameterCompare Compare = (phase==0)
                                   ? &Semantics::CompareParameterApplicability
                                   : &Semantics::CompareParameterGenericDepth;



        while (RemainingArguments &&
               (RemainingArguments->AsExpressionWithChildren().Left == NULL ||
                !HasFlag32(RemainingArguments->AsExpressionWithChildren().Left, SXF_ARG_NAMED)))
        {
            // Compare parameters only for supplied arguments.

            if (RemainingArguments->AsExpressionWithChildren().Left)
            {
                (this->*Compare)(
                    RemainingArguments->AsExpressionWithChildren().Left->AsArgumentExpression().Left,
                    LeftParameter,
                    LeftProcedure,
                    Left->Binding,
                    Left->ParamArrayExpanded,
                    RightParameter,
                    RightProcedure,
                    Right->Binding,
                    Right->ParamArrayExpanded,
                    LeftWinsAtLeastOnce,
                    RightWinsAtLeastOnce,
                    BothLose,
                    Left->IsExtensionMethod,
                    Right->IsExtensionMethod);

                if ((BothLose || LeftWinsAtLeastOnce || RightWinsAtLeastOnce))
                {
                    return false;
                }
            }

            // If a parameter is a param array, there is no next parameter and so advancing
            // through the parameter list is bad. (However, it is necessary to advance
            // through the argument list.)

            if (!LeftParameter->IsParamArray() || !Left->ParamArrayExpanded)
            {
                ++LeftParamArrayCount;
                LeftParameter = LeftParameter->GetNext();
            }

            if (!RightParameter->IsParamArray() || !Right->ParamArrayExpanded)
            {
                ++RightParamArrayCount;
                RightParameter = RightParameter->GetNext();
            }

            RemainingArguments = RemainingArguments->AsExpressionWithChildren().Right;
        }

        // Compare the parameters that match the named arguments.

        while (RemainingArguments)
        {
            ILTree::Expression *Argument = RemainingArguments->AsExpressionWithChildren().Left;

            if (HasFlag32(Argument, SXF_ARG_NAMED))
            {
                unsigned LeftParameterIndex, RightParameterIndex;
                LeftProcedure->GetNamedParam(
                    Argument->AsArgumentExpression().Name->AsArgumentNameExpression().Name,
                    &LeftParameter,
                    &LeftParameterIndex);
                RightProcedure->GetNamedParam(
                    Argument->AsArgumentExpression().Name->AsArgumentNameExpression().Name,
                    &RightParameter,
                    &RightParameterIndex);
            }
            else
            {
                // It is valid for a single unnamed argument to follow named
                // arguments in a property assignment. This argument matches the
                // last parameter.

                LeftParameter = LeftProcedure->GetLastParam();
                RightParameter = RightProcedure->GetLastParam();
            }

            (this->*Compare)(
                Argument->AsArgumentExpression().Left,
                LeftParameter,
                LeftProcedure,
                Left->Binding,
                true,
                RightParameter,
                RightProcedure,
                Right->Binding,
                true,
                LeftWinsAtLeastOnce,
                RightWinsAtLeastOnce,
                BothLose,
                Left->IsExtensionMethod,
                Right->IsExtensionMethod);

            if (BothLose || LeftWinsAtLeastOnce || RightWinsAtLeastOnce)
            {
                return false;
            }

            RemainingArguments = RemainingArguments->AsExpressionWithChildren().Right;
        }

        // Still not able to disabiguate, use Relaxation level as disabiguator
        if (Left->DelegateRelaxationLevel < Right->DelegateRelaxationLevel)
        {
            return false;
        }
        else if (Right->DelegateRelaxationLevel < Left->DelegateRelaxationLevel)
        {
            return false;
        }

        // Do TypeInference after since we want a better type inference win over
        // a delegate relaxation.
        // Still not able to disabiguate, use Type Inference level as disabiguator
        if (Left->TypeInferenceLevel < Right->TypeInferenceLevel)
        {
            return false;
        }
        else if (Right->TypeInferenceLevel < Left->TypeInferenceLevel)
        {
            return false;
        }

    }

    return true;
}

//This procedure should be kept in sync with AreProceduresEquallySpecific.
//See the commetns on that procedure for details.
OverloadList *
Semantics::MostSpecificProcedureOfTwo
(
    _In_ OverloadList *Left,
    _In_ OverloadList *Right,
    ExpressionList *Arguments,
    ParameterCompare Compare,
    _Out_ bool &BothLose,
    bool ContinueWhenBothLose,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod,
    OverloadResolutionFlags OvrldFlags
)
{
    BothLose = false;

    bool LeftWinsAtLeastOnce = false;
    bool RightWinsAtLeastOnce = false;

    Procedure *LeftProcedure = ViewAsProcedure(Left->Candidate);
    Procedure *RightProcedure = ViewAsProcedure(Right->Candidate);

    Parameter *LeftParameter = LeftProcedure->GetFirstParam();
    Parameter *RightParameter = RightProcedure->GetFirstParam();

    ILTree::Expression *RemainingArguments = Arguments;

    if (OvrldFlags & OvrldSomeCandidatesAreExtensionMethods)
    {
        Assume(RemainingArguments, L"Cannot skip non existant argument!");
        RemainingArguments = RemainingArguments->AsExpressionWithChildren().Right;
    }

    if (LeftIsExtensionMethod)
    {
        Assume(LeftParameter,  L"Cannot skip non-existant parameter");
        LeftParameter = LeftParameter->GetNext();
    }

    if (RightIsExtensionMethod)
    {
        Assume(RightParameter, L"Cannot skip non-existant parameter");
        RightParameter = RightParameter->GetNext();
    }

    // Compare the parameters that match the supplied positional arguments

    while (RemainingArguments &&
           (RemainingArguments->AsExpressionWithChildren().Left == NULL ||
            !HasFlag32(RemainingArguments->AsExpressionWithChildren().Left, SXF_ARG_NAMED)))
    {
        // Compare parameters only for supplied arguments.

        if (RemainingArguments->AsExpressionWithChildren().Left)
        {
            (this->*Compare)(
                RemainingArguments->AsExpressionWithChildren().Left->AsArgumentExpression().Left,
                LeftParameter,
                LeftProcedure,
                Left->Binding,
                Left->ParamArrayExpanded,
                RightParameter,
                RightProcedure,
                Right->Binding,
                Right->ParamArrayExpanded,
                LeftWinsAtLeastOnce,
                RightWinsAtLeastOnce,
                BothLose,
                LeftIsExtensionMethod,
                RightIsExtensionMethod);

            if ((BothLose && !ContinueWhenBothLose) ||
                (LeftWinsAtLeastOnce && RightWinsAtLeastOnce))
            {
                return NULL;
            }
        }

        // If a parameter is a param array, there is no next parameter and so advancing
        // through the parameter list is bad. (However, it is necessary to advance
        // through the argument list.)

        if (!LeftParameter->IsParamArray())
        {
            LeftParameter = LeftParameter->GetNext();
        }

        if (!RightParameter->IsParamArray())
        {
            RightParameter = RightParameter->GetNext();
        }

        RemainingArguments = RemainingArguments->AsExpressionWithChildren().Right;
    }

    // Compare the parameters that match the named arguments.

    while (RemainingArguments)
    {
        ILTree::Expression *Argument = RemainingArguments->AsExpressionWithChildren().Left;

        if (HasFlag32(Argument, SXF_ARG_NAMED))
        {
            unsigned LeftParameterIndex, RightParameterIndex;
            LeftProcedure->GetNamedParam(
                Argument->AsArgumentExpression().Name->AsArgumentNameExpression().Name,
                &LeftParameter,
                &LeftParameterIndex);
            RightProcedure->GetNamedParam(
                Argument->AsArgumentExpression().Name->AsArgumentNameExpression().Name,
                &RightParameter,
                &RightParameterIndex);
        }
        else
        {
            // It is valid for a single unnamed argument to follow named
            // arguments in a property assignment. This argument matches the
            // last parameter.

            LeftParameter = LeftProcedure->GetLastParam();
            RightParameter = RightProcedure->GetLastParam();
        }

        (this->*Compare)(
            Argument->AsArgumentExpression().Left,
            LeftParameter,
            LeftProcedure,
            Left->Binding,
            true,
            RightParameter,
            RightProcedure,
            Right->Binding,
            true,
            LeftWinsAtLeastOnce,
            RightWinsAtLeastOnce,
            BothLose,
            LeftIsExtensionMethod,
            RightIsExtensionMethod);

        if ((BothLose && !ContinueWhenBothLose) ||
            (LeftWinsAtLeastOnce && RightWinsAtLeastOnce))
        {
            return NULL;
        }

        RemainingArguments = RemainingArguments->AsExpressionWithChildren().Right;
    }

    VSASSERT(!(LeftWinsAtLeastOnce && RightWinsAtLeastOnce), "Most specific method logic is confused.");

    if (LeftWinsAtLeastOnce)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s wins\n", CandidateToDebugString(Left));
        return Left;
    }

    if (RightWinsAtLeastOnce)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s wins\n", CandidateToDebugString(Right));
        return Right;
    }



    // Still not able to disabiguate, use Relaxation level as disabiguator
    if (Left->DelegateRelaxationLevel < Right->DelegateRelaxationLevel)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s - $7.8/7.10/7.11\n", CandidateToDebugString(Left), DelegateRelaxationLevelToDebugString(Left->DelegateRelaxationLevel), CandidateToDebugString(Right), DelegateRelaxationLevelToDebugString(Right->DelegateRelaxationLevel));
        return Left;
    }
    else if (Right->DelegateRelaxationLevel < Left->DelegateRelaxationLevel)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s - $7.8/7.10/7.11\n", CandidateToDebugString(Right), DelegateRelaxationLevelToDebugString(Right->DelegateRelaxationLevel), CandidateToDebugString(Left), DelegateRelaxationLevelToDebugString(Left->DelegateRelaxationLevel));
        return Right;
    }

    // Do TypeInference after since we want a better type inference win over
    // a delegate relaxation.
    // Still not able to disabiguate, use Type Inference level as disabiguator
    if (Left->TypeInferenceLevel < Right->TypeInferenceLevel)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s - $7.8\n", CandidateToDebugString(Left), TypeInferenceLevelToDebugString(Left->TypeInferenceLevel), CandidateToDebugString(Right), TypeInferenceLevelToDebugString(Right->TypeInferenceLevel));
        return Left;
    }
    else if (Right->TypeInferenceLevel < Left->TypeInferenceLevel)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"    %s:%s wins over %s:%s - $7.8\n", CandidateToDebugString(Right), TypeInferenceLevelToDebugString(Right->TypeInferenceLevel), CandidateToDebugString(Left), TypeInferenceLevelToDebugString(Left->TypeInferenceLevel));
        return Right;
    }

    return NULL;
}

OverloadList *
Semantics::MostSpecificProcedure
(
    _Inout_ OverloadList *Candidates,
    _Inout_ unsigned &CandidateCount,
    ExpressionList *Arguments,
    bool LookAtSafeConversions,
    OverloadResolutionFlags OvrldFlags
)
{

    for (int phase=0; phase<2; phase++)
    {
        // See the comments in "AreProceduresEquallySpecific" for an explanation of the two phases. In short:
        // The first phase embodies $11.8.1:6 "applicability", 7.9/7.10/7.11 "delegate relaxation", 7.8 "type inference level"
        // The second phase embodies $11.8.1:7.3 "specificity", followed by the same 7.9/7.10/7.11/7.8 (but redundant this time)

        DBG_SWITCH_PRINTF(fDumpOverload, phase==0 ? "  Judging applicability $11.8.1:6...\n"
                                                  : "  Judging specificity $11.8.1:7.3...\n");

        OverloadList *CurrentCandidate = Candidates;

        while (CurrentCandidate)
        {
            if (!CurrentCandidate->NotCallable &&
                (!CurrentCandidate->RequiresNarrowingConversion || (LookAtSafeConversions && CurrentCandidate->RequiresNarrowingConversion && CurrentCandidate->AllNarrowingIsFromNumericLiteral)))
            {
                bool CurrentCandidateIsBest = true;

                DBG_SWITCH_PRINTF(fDumpOverload, L"  Start with %s\n", CandidateToDebugString(CurrentCandidate,true));

                OverloadList *Contender = Candidates;

                while (Contender)
                {
                    if (!Contender->NotCallable &&
                        (!Contender->RequiresNarrowingConversion || (LookAtSafeConversions && CurrentCandidate->RequiresNarrowingConversion && CurrentCandidate->AllNarrowingIsFromNumericLiteral)) &&
                        (Contender->Candidate != CurrentCandidate->Candidate ||
                         Contender->ParamArrayExpanded != CurrentCandidate->ParamArrayExpanded))
                    {
                        bool Dummy;

                        DBG_SWITCH_PRINTF(fDumpOverload, L"    Compare with %s\n", CandidateToDebugString(Contender,true));

                        OverloadList *BestOfTheTwo =
                            MostSpecificProcedureOfTwo
                            (
                                CurrentCandidate,
                                Contender,
                                Arguments,
                                phase==0 ? &Semantics::CompareParameterApplicability : &Semantics::CompareParameterGenericDepth,
                                Dummy,
                                true, /* continue when both sides lose */// Bug VSWhidbey
                                CurrentCandidate->IsExtensionMethod,
                                Contender->IsExtensionMethod,
                                OvrldFlags
                            );

                        if (BestOfTheTwo == CurrentCandidate)
                        {
                            if (!Contender->LessSpecific)
                            {
                                DBG_SWITCH_PRINTF(fDumpOverload, L"      Rejected contender %s\n", CandidateToDebugString(Contender));

                                Contender->LessSpecific = true;
                                CandidateCount--;
                            }
                        }
                        else
                        {
                            if (BestOfTheTwo == Contender &&
                                !CurrentCandidate->LessSpecific)
                            {
                                DBG_SWITCH_PRINTF(fDumpOverload, L"      Rejected current candidate %s\n", CandidateToDebugString(CurrentCandidate));

                                CurrentCandidate->LessSpecific = true;
                                CandidateCount--;
                            }
#if DEBUG
                            else if (CurrentCandidateIsBest)
                            {
                                DBG_SWITCH_PRINTF(fDumpOverload, L"      Neither current candidate %s nor contender %s won, nor are best\n", CandidateToDebugString(CurrentCandidate), CandidateToDebugString(Contender));
                            }
                            else
                            {
                                DBG_SWITCH_PRINTF(fDumpOverload, L"      Neither current candidate %s nor contender %s won\n", CandidateToDebugString(CurrentCandidate), CandidateToDebugString(Contender));
                            }
                           
#endif

                            // The current candidate can't be the most specific.
                            CurrentCandidateIsBest = false;

                            // Because the current candidate can't win, terminating
                            // the loop can save some time. However, more accurate
                            // diagnostics result if the loop completes. For example,
                            // for three candidates A, B, and C, if neither A nor B
                            // is most specific, but B is more specific than C,
                            // then excluding C gives a sharper diagnostic.
                            //
                            // break;
                        }
                    }

                    Contender = Contender->Next;
                }

                if (CurrentCandidateIsBest)
                {
                    VSASSERT(CandidateCount == 1 || LookAtSafeConversions, "Surprising overload candidate remains.");

                    return CurrentCandidate;
                }
            }

            CurrentCandidate = CurrentCandidate->Next;
        }

    }

    return NULL;
}

Declaration *
Semantics::ResolveOverloadedCall
(
    _In_ const Location &CallLocation,
    _In_ Declaration *OverloadedProcedure,
    _In_ ExpressionList *Arguments,
    _In_opt_ Type *DelegateType,
    _In_opt_ Type *DelegateReturnType,
    _Inout_ GenericBinding *&GenericBindingContext,
    _In_count_(TypeArgumentCount) Type **TypeArguments,
    unsigned TypeArgumentCount,
    ExpressionFlags Flags,
    OverloadResolutionFlags OvrldFlags,
    _In_ Type *AccessingInstanceType,
    _Out_ bool &ResolutionFailed,
    _Out_ bool &ResolutionIsLateBound,
    _Out_ bool &ResolutionIsAmbiguous
)
{
    ExpressionList **LastArgumentTarget = NULL;
    ExpressionList *LastArgument = NULL;

    if (HasFlag(Flags, ExprIsPropertyAssignment))
    {
        // For a property assignment, the last argument is the source value and
        // does not match any parameter of the property. Remove it from the
        // argument list.

        for (LastArgumentTarget = &Arguments;
            (*LastArgumentTarget)->AsExpressionWithChildren().Right;
            LastArgumentTarget = &(*LastArgumentTarget)->AsExpressionWithChildren().Right)
        {
        }

        LastArgument = *LastArgumentTarget;
        *LastArgumentTarget = NULL;
    }

    AsyncSubAmbiguityFlagCollection *pAsyncSubArgumentListAmbiguity = NULL;

    Declaration *Result =
        CollectCandidatesAndResolveOverloading
        (
            CallLocation,
            OverloadedProcedure,
            Arguments,
            DelegateType,
            DelegateReturnType,
            GenericBindingContext,
            TypeArguments,
            TypeArgumentCount,
            Flags,
            OvrldFlags,
            AccessingInstanceType,
            ResolutionFailed,
            ResolutionIsLateBound,
            ResolutionIsAmbiguous,
            &pAsyncSubArgumentListAmbiguity
        );

    if (LastArgument)
    {
        *LastArgumentTarget = LastArgument;
    }

    ReportAndDeleteAsyncSubArgumentListAmbiguity(&pAsyncSubArgumentListAmbiguity, Result != NULL && !IsBad(Result));

    return Result;
}

Declaration *
Semantics::CollectCandidatesAndResolveOverloading
(
    _In_ const Location &CallLocation,
    _In_ Declaration *OverloadedProcedure,
    _In_ ExpressionList *Arguments,
    _In_opt_ Type *DelegateType,
    _In_opt_ Type *DelegateReturnType,
    _Inout_ GenericBinding *&GenericBindingContext, // See comment below
    _In_count_(TypeArgumentCount) Type **TypeArguments,
    unsigned TypeArgumentCount,
    ExpressionFlags Flags,
    OverloadResolutionFlags OvrldFlags,
    _In_ Type *AccessingInstanceType,
    _Out_ bool &ResolutionFailed,
    _Out_ bool &ResolutionIsLateBound,
    _Out_ bool &ResolutionIsAmbiguous,
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    // Input:  OverloadedProcedure is the most derived method with the correct name,
    //         and GenericBindingContext is for the type containing it
    // Output: * We might return the winning candidate and GenericBindingContext is its type binding (or method binding if it's generic)
    //         * We might fail to find a candidate and return NULL
    //         * We might fail, but because of "OvrldDontReportSingletonErrors" we return it with the original GenericBindingContext (even if it's generic)
    //           and with ResolutionFailed=true.

    ResolutionFailed = false;
    ResolutionIsLateBound = false;

    unsigned ArgumentCount = CountArguments(Arguments);

    NorlsAllocator Scratch(NORLSLOC);
    unsigned CandidateCount = 0;
    unsigned RejectedForArgumentCount = 0;
    unsigned RejectedForTypeArgumentCount = 0;

    DBG_SWITCH_PRINTF(fDumpOverload, L"\n\n=== COLLECT CANDIDATES: %s\n", OverloadedProcedure->GetName());

    OverloadList * Candidates =
        CollectOverloadCandidates
        (
            NULL,
            OverloadedProcedure,
            GenericBindingContext,
            Arguments,
            ArgumentCount,
            DelegateReturnType,
            TypeArguments,
            TypeArgumentCount,
            Flags,
            OvrldFlags,
            AccessingInstanceType,
            Scratch,
            CandidateCount,
            RejectedForArgumentCount,
            RejectedForTypeArgumentCount,
            CallLocation,
            ResolutionFailed,
            ppAsyncSubArgumentListAmbiguity
        );

    if (ResolutionFailed)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Resolution failed.\n");
        return NULL;
    }

    DBG_SWITCH_PRINTF(fDumpOverload, L"  CandidateCount = %u\n", CandidateCount);

    if (CandidateCount == 1 &&  ! Candidates->InferenceFailed && ! (OvrldFlags & OvrldForceOverloadResolution) )
    {
        if (Candidates->Binding.IsGenericTypeBinding())
        {
            // Dev11#306832: if the winning candidate had a generic type binding, we need to report that.
            // (the reporting is also being done in the CandidateCount>1 case below, with "GenericBindingContext = ResultGenericBindingContext;"
            GenericBindingContext = Candidates->Binding.PGenericBinding();
        }
        return Candidates->Candidate;
    }

    if (CandidateCount == 0)
    {

        if (!GetNextOverloadForProcedureConsideringBaseClasses(OverloadedProcedure, AccessingInstanceType))
        {
            ExpressionList * ignored = NULL;
            bool ignoredBool[10] = {0};
            DelegateRelaxationLevel DelegateRelaxationLevel = DelegateRelaxationLevelNone;
            TypeInferenceLevel TypeInferenceLevel = TypeInferenceLevelNone;

            if (HasFlag(OvrldFlags, OvrldDontReportSingletonErrors))
            {
                ResolutionFailed = true;
                ResolutionIsLateBound = false;
                return OverloadedProcedure;

                // NB. Normally our caller expects us to modify GenericBindingContext appropriate to our returned overload,
                // e.g. the class containing it, or its generic arguments if it was a generic overload.
                // But down this path, where there was no winner and we're merely returning the single overload
                // that was given to us, there's no such thing as "generic binding of its arguments".
                // So callers must be hardened that if they ask for OvrldDontReportSingletonErrors,
                // then they can get back either the GenericTypeBinding that they put in (in case of this failure)
                // or the correct GenericMethodBinding (in case of success down other branches of the method).
            }

            if (! RejectedForArgumentCount && ! RejectedForTypeArgumentCount)
            {
                ReportSemanticError
                (
                    ERRID_InaccessibleSymbol2,
                    CallLocation,
                    OverloadedProcedure,
                    OverloadedProcedure->GetAccessConsideringAssemblyContext(ContainingContainer())
                );
            }


            Location dummy;
             
            //We have a singleton method, so we use the singleton error messages
            MatchArguments1
            (
                CallLocation,
                ViewAsProcedure(OverloadedProcedure),
                NULL,
                GenericBindingContext,
                Arguments,
                DelegateReturnType,
                Flags,
                OvrldFlags,
                ignored,
                false, // CheckValidityOnly
                false,
                false,
                false,
                ignoredBool[0],
                ignoredBool[1],
                ignoredBool[2],
                ignoredBool[3],
                ignoredBool[4],
                ignoredBool[5],
                ignoredBool[6],
                ignoredBool[7],
                false,
                false,
                NULL,
                DelegateRelaxationLevel,
                TypeInferenceLevel,
                ignoredBool[8],
                ignoredBool[9],
                // For resolve overloading, we only need to give the type of line number, 
                // not the actually value. 
                &dummy
            );
        }
        else
        {
            unsigned ErrorId = ERRID_NoViableOverloadCandidates1;

            if (RejectedForArgumentCount != 0)
            {
                ErrorId =
                    HasFlag(Flags, ExprIsLHSOfObjectInitializer) ?
                        ERRID_NoZeroCountArgumentInitCandidates1 :
                        ERRID_NoArgumentCountOverloadCandidates1;
            }
            else if (RejectedForTypeArgumentCount != 0)
            {
                ErrorId = ERRID_NoTypeArgumentCountOverloadCand1;
            }

            ReportSemanticError(
                ErrorId,
                CallLocation,
                OverloadedProcedure->GetErrorName(m_Compiler));
        }
        ResolutionFailed = true;
        return NULL;
    }

    // 

    GenericBinding *ResultGenericBindingContext = NULL;

    Declaration *Result =
        ResolveOverloading
        (
            CallLocation,
            Candidates,
            CandidateCount,
            Arguments,
            ArgumentCount,
            DelegateType,
            DelegateReturnType,
            ResolutionFailed,
            OvrldFlags,
            ResolutionIsLateBound,
            ResolutionIsAmbiguous,
            ResultGenericBindingContext,
            NULL,
            false,
            NULL,
            0,
            NULL,
            ppAsyncSubArgumentListAmbiguity
        );

    if (!ResolutionFailed &&
        ResolutionIsLateBound &&
        AccessingInstanceType->IsInterface())
    {
        ReportSemanticError(
            ERRID_LateboundOverloadInterfaceCall1,
            CallLocation,
            OverloadedProcedure->GetErrorName(m_Compiler));

        ResolutionFailed = true;
        ResolutionIsLateBound = false;
        return NULL;
    }

    GenericBindingContext = ResultGenericBindingContext;

    return Result;
}

bool
Semantics::ContainsNonNarrowingCallableInstanceMethods
(
    OverloadList * Candidates
)
{
    while (Candidates)
    {
        if
        (
            (!Candidates->RequiresNarrowingConversion || Candidates->AllNarrowingIsFromNumericLiteral) && //the method is non narrowing
            !(Candidates->DelegateRelaxationLevel == DelegateRelaxationLevelNarrowing) && //and the method does not require narrowing delegate relaxation...
            !Candidates->IsExtensionMethod &&   //and it's not an extension method
            (!Candidates->NotCallable || Candidates->RequiresInstanceMethodBinding) //and it's callable or requires instance method binding.
        )
        {
            //therefore we return true
            return true;
        }
        Candidates = Candidates->Next;
    }

    //There are no non-narrarowing callable instance methods in the list, so we return false
    return false;
}


enum ShadowState
{
    ShadowStateNone = 0,
    ShadowStateLeft = 1,
    ShadowStateRight = 2
};

Semantics::ShadowState
Semantics::ShadowBasedOnParamArrayUsage
(
    OverloadList * Left,
    OverloadList * Right,
    ExpressionList * Arguments,
    OverloadResolutionFlags OvrldFlags,
    unsigned LeftParamArrayCount,
    unsigned RightParamArrayCount
)
{
    ShadowState Ret = ShadowStateNone;

    bool LeftWins = false;
    bool RightWins = false;

    CompareParamarraySpecificity
    (
        Left->ParamArrayExpanded,
        LeftParamArrayCount,
        Right->ParamArrayExpanded,
        RightParamArrayCount,
        LeftWins,
        RightWins
    );

    ThrowIfTrue(LeftWins && RightWins);

    if (LeftWins)
    {
        Ret = ShadowStateRight;
    }

     if (RightWins)
     {
        Ret = ShadowStateLeft;
     }

     return Ret;
}


Semantics::ShadowState
Semantics::ShadowBasedOnReceiverTypeSpecificity
(
    OverloadList * Left,
    OverloadList * Right,
    ExpressionList * Arguments,
    OverloadResolutionFlags OvrldFlags
)
{
    bool leftWins = false;
    bool rightWins = false;

    CompareReceiverTypeSpecificity
    (
        ViewAsProcedure(Left->Candidate),
        Left->Binding,
        ViewAsProcedure(Right->Candidate),
        Right->Binding,
        leftWins,
        rightWins,
        Left->IsExtensionMethod,
        Right->IsExtensionMethod
    );

    if (leftWins)
    {
        return ShadowStateRight;
    }
    else if (rightWins)
    {
        return ShadowStateLeft;
    }
    else
    {
        return ShadowStateNone;
    }
}


bool
Semantics::IsMethodInGenericClass
(
    OverloadList * Candidate
)
{
    ThrowIfNull(Candidate);

    Procedure * pProc = ViewAsProcedure(Candidate->Candidate);

    return
        Candidate->Candidate->HasGenericParent() ||
        (
            Candidate->IsExtensionMethod &&
            pProc->IsGeneric() &&
            RefersToGenericTypeParameter
            (
                true,
                pProc,
                pProc->GetFirstParam()->GetType(),
                Candidate->Binding
            )
        );
}


Semantics::ShadowState
Semantics::ShadowBasedOnGenericity
(
    OverloadList * Left,
    OverloadList * Right,
    ExpressionList * Arguments,
    OverloadResolutionFlags OvrldFlags
)
{
    OverloadList * LeastGeneric = NULL;

    bool SignatureMismatch = false;

    if (Left->RequiresNarrowingConversion == Right->RequiresNarrowingConversion)
    {

        if (Left->Candidate->IsGeneric() || Right->Candidate->IsGeneric())
        {
            LeastGeneric =
                MostSpecificProcedureOfTwo
                (
                    Left,
                    Right,
                    Arguments,
                    &Semantics::CompareGenericityBasedOnMethodGenericParams,
                    SignatureMismatch,
                    false,
                    Left->IsExtensionMethod,
                    Right->IsExtensionMethod,
                    OvrldFlags
                );
        }

        if  ( IsMethodInGenericClass(Left) ||IsMethodInGenericClass(Right))
        {
            if (!LeastGeneric)
            {
                LeastGeneric =
                    MostSpecificProcedureOfTwo
                    (
                        Left,
                        Right,
                        Arguments,
                        &Semantics::CompareGenericityBasedOnTypeGenericParams,
                        SignatureMismatch,
                        false,
                        Left->IsExtensionMethod,
                        Right->IsExtensionMethod,
                        OvrldFlags
                    );
            }
        }

        if (LeastGeneric)
        {
            if (LeastGeneric == Left)
            {
                return ShadowStateRight;
            }
            else
            {
                return ShadowStateLeft;
            }
        }
    }

    return ShadowStateNone;
}


Semantics::ShadowState
Semantics::ShadowBasedOnInstanceMethodVsExtensionMethodStatus
(
    OverloadList * Left,
    OverloadList * Right,
    ExpressionList * Arguments,
    OverloadResolutionFlags OvrldFlags
)
{
    if (Left->IsExtensionMethod && !Right->IsExtensionMethod)
    {
        return ShadowStateLeft;
    }
    else if (Right->IsExtensionMethod  && !Left->IsExtensionMethod)
    {
        return ShadowStateRight;
    }
    else
    {
        return ShadowStateNone;
    }
}

Semantics::ShadowState
Semantics::ShadowBasedOnExtensionMethodScope
(
    OverloadList * Left,
    OverloadList * Right,
    ExpressionList * Arguments,
    OverloadResolutionFlags OvrldFlags
)
{
    if (Left->IsExtensionMethod && Right->IsExtensionMethod)
    {
        if (Left->PrecedenceLevel < Right->PrecedenceLevel)
        {
            return ShadowStateRight;
        }
        else if (Right->PrecedenceLevel < Left->PrecedenceLevel)
        {
            return ShadowStateLeft;
        }
    }
    return ShadowStateNone;
}

Semantics::ShadowState
Semantics::ShadowBasedOnSubOfFunction
(
    OverloadList * Left,
    OverloadList * Right,
    ExpressionList * Arguments,
    OverloadResolutionFlags OvrldFlags
)
{
    bool preferSub = HasFlag(OvrldFlags, OvrldPreferSubOverFunction);
    bool preferFunction = HasFlag(OvrldFlags, OvrldPreferFunctionOverSub);

    VSASSERT(!(preferSub && preferFunction), "preferSub and preferFunction should never be set at the same time");
    if (preferSub || preferFunction)
    {
        bool leftIsSub = IsSub(ViewAsProcedure(Left->Candidate));
        bool rightIsSub = IsSub(ViewAsProcedure(Right->Candidate));

        if (leftIsSub != rightIsSub)
        {
            if (preferSub)
            {
                if (leftIsSub)
                {
                    VSASSERT(!rightIsSub, "should imply from else branch of leftIsSub == rightIsSub");
                    return ShadowStateRight;
                }
                else if (rightIsSub)
                {
                    VSASSERT(!leftIsSub, "should imply from else branch of leftIsSub == rightIsSub");
                    return ShadowStateLeft;
                }
            }
            else
            {
                VSASSERT(preferFunction, "Must be prefer function, implies from preferSub || preferFunction");
                if (!leftIsSub)
                {
                    VSASSERT(rightIsSub, "should imply from else branch of leftIsSub == rightIsSub");
                    return ShadowStateRight;
                }
                else if (!rightIsSub)
                {
                    VSASSERT(leftIsSub, "should imply from else branch of leftIsSub == rightIsSub");
                    return ShadowStateLeft;
                }
            }
        }
    }

    return ShadowStateNone;
}

OverloadList *
Semantics::ApplyShadowingSemantics
(
    OverloadList * Candidates,
    unsigned & CandidateCount,
    ExpressionList *Arguments,
    OverloadResolutionFlags OvrldFlags
)
{
    OverloadList * BestCandidate = NULL;

    for (OverloadList * left = Candidates; left; left = left->Next)
    {
        if (! left->NotCallable && ! left->InferenceFailed)
        {
            for (OverloadList * right = left->Next; right; right = right->Next)
            {
                unsigned leftParamArrayCount = 0;
                unsigned rightParamArrayCount = 0;

                if (!right->NotCallable && !right->InferenceFailed)
                {
                    if
                    (
                        AreProceduresEquallySpecific
                        (
                            left,
                            right,
                            Arguments,
                            OvrldFlags,
                            leftParamArrayCount,
                            rightParamArrayCount
                        )
                    )
                    {
                        ShadowState shadowState = ShadowBasedOnParamArrayUsage(left, right, Arguments, OvrldFlags, leftParamArrayCount, rightParamArrayCount);

                        if (shadowState == ShadowStateNone)
                        {
                            shadowState = ShadowBasedOnReceiverTypeSpecificity(left, right, Arguments, OvrldFlags);
                        }

                        if (shadowState == ShadowStateNone)
                        {
                            shadowState = ShadowBasedOnGenericity(left, right, Arguments, OvrldFlags);
                        }

                        if (shadowState == ShadowStateNone)
                        {
                            shadowState = ShadowBasedOnInstanceMethodVsExtensionMethodStatus(left, right, Arguments, OvrldFlags);
                        }

                        if (shadowState == ShadowStateNone)
                        {
                            shadowState = ShadowBasedOnExtensionMethodScope(left, right,  Arguments, OvrldFlags);
                        }

                        if (shadowState == ShadowStateNone)
                        {
                            // Dev10 #792284 Shadow based on a use of Default values for optional parameters.
                            // A candidate for which all arguments were explicitly given
                            // is better than the one for which default values were used in lieu of explicit arguments
                            if (!left->UsedDefaultForAnOptionalParameter && right->UsedDefaultForAnOptionalParameter)
                            {
                                shadowState = ShadowStateRight;
                            }
                            else if (left->UsedDefaultForAnOptionalParameter && !right->UsedDefaultForAnOptionalParameter)
                            {
                                shadowState = ShadowStateLeft;
                            }
                        }

                        if (shadowState == ShadowStateNone)
                        {
                            shadowState = ShadowBasedOnSubOfFunction(left, right, Arguments, OvrldFlags);
                        }

                        if (shadowState != ShadowStateNone)
                        {
                            --CandidateCount;

                            if (shadowState == ShadowStateLeft)
                            {
                                DBG_SWITCH_PRINTF(fDumpOverload, L"  Reject candidate %s because it's shadowed by %s... $2 eliminate all members from the set that are inaccessible\n", CandidateToDebugString(left), CandidateToDebugString(right));

                                left->NotCallable = true;
                                BestCandidate = right;
                                break;

                            }
                            if (shadowState == ShadowStateRight)
                            {
                                DBG_SWITCH_PRINTF(fDumpOverload, L"  Reject candidate %s because it's shadowed by %s... $2 eliminate all members from the set that are inaccessible\n", CandidateToDebugString(right), CandidateToDebugString(left));

                                right->NotCallable = true;
                                BestCandidate = left;
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
    return BestCandidate;
}


Declaration *
Semantics::ResolveOverloading
(
    _In_ const Location &CallLocation,
    _In_ OverloadList * Candidates,
    unsigned CandidateCount,
    _In_ ExpressionList *Arguments,
    unsigned ArgumentCount,
    _In_opt_ Type *DelegateType,
    _In_opt_ Type *DelegateReturnType,
    _Out_ bool &ResolutionFailed,
    OverloadResolutionFlags OvrldFlags,
    _Out_ bool &ResolutionIsLateBound,
    _Out_ bool &ResolutionIsAmbiguous,
    _Inout_ GenericBinding *&GenericBindingContext,
    _In_opt_ OutputParameter<OverloadList *> MatchingCandidate,
    bool CandidatesAreOperators,
    _In_opt_ OverloadList * InstanceMethodOnlyCandidates,
    unsigned InstanceMethodOnlyCandidateCount,
    _Out_opt_ bool * pIsBadSingleton,
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    // Port SP1 CL 2967550 to VS10
    // Microsoft:
    // we don't want to set this flag when we do overload resolution.
    BackupValue<bool> backupLambda(&m_methodHasLambda);

    ResolutionFailed = false;
    ResolutionIsLateBound = false;
    GenericBindingContext = NULL;

    bool SomeCandidatesAreGeneric = false;
    GenericBinding *BestGenericBindingContext = NULL;

    OverloadList * BestCandidate = NULL;

    // Port SP1 CL 2967675 to VS10

    if (pIsBadSingleton)
    {
        *pIsBadSingleton = false;
    }

    DBG_SWITCH_PRINTF(fDumpOverload, L"=== RESOLVE OVERLOADING: %s\n", Candidates->Candidate->GetName());

    if (InstanceMethodOnlyCandidates && InstanceMethodOnlyCandidateCount)
    {
        bool SomeInstanceMethodOnlyCandidatesAreGeneric = false;

        OverloadList * BestInstanceMethodOnlyCandidate =
            RejectUncallableProcedures
            (
                CallLocation,
                InstanceMethodOnlyCandidates,
                Arguments,
                ArgumentCount,
                DelegateReturnType,
                OvrldFlags,
                InstanceMethodOnlyCandidateCount,
                SomeInstanceMethodOnlyCandidatesAreGeneric,
                ppAsyncSubArgumentListAmbiguity
            );

        if (ContainsNonNarrowingCallableInstanceMethods(InstanceMethodOnlyCandidates))
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"  Rejecting all extension candidates: $5. If any instance methods remain in the set, eliminate all extension methods from the set.\n");
 
            Candidates = InstanceMethodOnlyCandidates;
            CandidateCount = InstanceMethodOnlyCandidateCount;
            SomeCandidatesAreGeneric = SomeInstanceMethodOnlyCandidatesAreGeneric;
            BestCandidate = BestInstanceMethodOnlyCandidate;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"  (we can't reject extension methods yet, since all the instance methods involve narrowing)\n");

            BestCandidate =
                RejectUncallableProcedures
                (
                    CallLocation,
                    Candidates,
                    Arguments,
                    ArgumentCount,
                    DelegateReturnType,
                    OvrldFlags,
                    CandidateCount,
                    SomeCandidatesAreGeneric,
                    ppAsyncSubArgumentListAmbiguity
                );
        }
    }
    else
    {
        BestCandidate =
            RejectUncallableProcedures
            (
                CallLocation,
                Candidates,
                Arguments,
                ArgumentCount,
                DelegateReturnType,
                OvrldFlags,
                CandidateCount,
                SomeCandidatesAreGeneric,
                ppAsyncSubArgumentListAmbiguity
            );
    }

    DBG_SWITCH_PRINTF(fDumpOverload, L"  CandidateCount = %u\n", CandidateCount);

    if (CandidateCount == 1 && BestCandidate && !BestCandidate->InferenceFailed)
    {
        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    Identifier *OverloadedMemberName = NULL;

    OverloadedMemberName = Candidates->Candidate->GetErrorName(m_Compiler);

    if (CandidateCount == 0)
    {
        // If one or more arguments have bad type no overload is found.
        // Avoid reporting errors on all the overloads when arguments of bad type are passed.
        // Report the bad types instead, the error is not on matching, it is about bad types.
        ILTree::Expression *Argument = Arguments;
        bool badArgumentTypeSeen = false;
        while (Argument &&
            (Argument->AsExpressionWithChildren().Left == NULL ||
            !HasFlag32(Argument->AsExpressionWithChildren().Left, SXF_ARG_NAMED)))
        {
            if (Argument->AsExpressionWithChildren().Left)
            {
                Type* ArgumentType = Argument->AsExpressionWithChildren().Left->AsArgumentExpression().Left->ResultType;
                if ( TypeHelpers::IsBadType(ArgumentType))
                {
                    ReportBadType(ArgumentType, Argument->Loc);
                    badArgumentTypeSeen = true;
                }
            }
            Argument = Argument->AsExpressionWithChildren().Right;
        }

        if(!badArgumentTypeSeen)
        {
            if (Candidates->Next || Candidates->InferenceFailed || ! (OvrldFlags & OvrldReturnUncallableSingletons))
            {
                ReportUncallableProcedures
                (
                    CallLocation,
                    OverloadedMemberName,
                    DelegateType,
                    Candidates,
                    Arguments,
                    ArgumentCount,
                    DelegateReturnType,
                    OvrldFlags
                );
            }
            else
            {
                VSASSERT((OvrldFlags & OvrldReturnUncallableSingletons), L"This should not be true!");
                
                //Because OvrldReturnUncallableSingletons is true, we return the method.
                //However,we set ResolutionFailed to true so that who ever we return to is aware of the fact that
                //the method is not callable. 
                GenericBindingContext = Candidates->Binding.PGenericBinding();
                MatchingCandidate = Candidates;
                ResolutionFailed = true;

                // Port SP1 CL 2967675 to VS10
                if (pIsBadSingleton)
                {
                    *pIsBadSingleton = true;
                }
                return Candidates->Candidate;
            }
        }

        ResolutionFailed = true;
        MatchingCandidate = NULL;

        return NULL;
    }

    if (CandidateCount  > 1)
    {
        BestCandidate =
            ApplyShadowingSemantics
            (
                Candidates,
                CandidateCount,
                Arguments,
                OvrldFlags
            );
    }

    if (CandidateCount == 1 && BestCandidate && !BestCandidate->InferenceFailed)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Picked winner %s due to shadowing\n", CandidateToDebugString(BestCandidate));

        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    // See if type inference failed for all candidates and it failed from
    // Object. For this scenario, in non-strict mode, treat the call
    // as latebound.

    unsigned InferenceFailedFromObjectCount = 0;
    OverloadList *Candidate = NULL;

    for (Candidate = Candidates; Candidate; Candidate = Candidate->Next)
    {
        if (!Candidate->NotCallable && Candidate->InferenceFailed)
        {
            DBG_SWITCH_PRINTF(fDumpOverload, L"  Rejected candidate %s because its inference failed\n", CandidateToDebugString(Candidate));

            // Remove candidates with inference failures from the list.
            Candidate->NotCallable = true;
            CandidateCount--;

            if (Candidate->AllFailedInferenceIsDueToObject && ! Candidate->IsExtensionMethod)
            {
                InferenceFailedFromObjectCount++;
            }
        }
    }

    if (CandidateCount == 0)
    {
        if (InferenceFailedFromObjectCount > 0 && !m_UsingOptionTypeStrict)
        {
            MatchingCandidate = NULL;
            return VerifyLateboundCallConditions(
                CallLocation,
                OverloadedMemberName,
                DelegateType,
                Candidates,
                Arguments,
                ArgumentCount,
                DelegateReturnType,
                ResolutionFailed,
                ResolutionIsLateBound,
                OvrldFlags);
        }

        ReportInferenceFailureProcedures
        (
            CallLocation,
            OverloadedMemberName,
            DelegateType,
            Candidates,
            Arguments,
            ArgumentCount,
            DelegateReturnType,
            OvrldFlags
        );

        ResolutionFailed = true;
        MatchingCandidate = NULL;
        return NULL;
    }

    // We need to filter out candidates who's arguments are less specific relaxations on delegates
    // to remain backwards compattible and remain consistent when flipping strict on/off.
    BestCandidate =
        RejectLessSpecificDelegateRelaxationLevels(
            Candidates,
            CandidateCount);

    if (CandidateCount == 1 && BestCandidate)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Picked winner %s due to relaxing less specific delegate relaxation levels\n", CandidateToDebugString(BestCandidate));

        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    // We need to filter out candidates who's arguments are less specific relaxations on delegates
    // to remain backwards compattible and remain consistent when flipping strict on/off.
    BestCandidate =
        RejectBetterTypeInferenceLevelOfLaterReleases(
            Candidates,
            CandidateCount);

    if (CandidateCount == 1 && BestCandidate)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Picked winner %s due to rejecting better type inference level of later releases\n", CandidateToDebugString(BestCandidate));

        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    // All candidates should have valid types at this point.

    // See if only one does not require narrowing.

    // In non-strict mode only: If all candidates require narrowing,
    // but one does so only from Object, pick that candidate. If more than
    // one candidate narrows only from object, treat the call as late
    // bound. (This rule is necessary to make overloaded methods available
    // to typeless programs.)

    unsigned NarrowOnlyFromObjectCount = 0;
    OverloadList *BestNarrowingCandidate = NULL;
    BestCandidate = NULL;

    //Look through the candidate set for lifted operators that require narrowing conversions whose
    //source operators also require narrowing conversions. In that case, we only want to keep one method in
    //the set. If the source operator requires nullables to be unwrapped than we disgard it and keep the lifted operator.
    //If it does not than we disgard the lifted operator and keep the source operator. This will prevent the presence of lifted operators
    //from causing overload resolution conflicts where there otherwise wouldn't be one. However, if the source operator only requires narrowing
    //conversions from numeric literals than we keep both in the set, because the conversion in that case is not really narrowing.
    if (CandidatesAreOperators)
    {
        for (Candidate = Candidates; Candidate; Candidate = Candidate->Next)
        {
            ThrowIfFalse(Candidate->Candidate->IsUserDefinedOperator());
            ThrowIfNull(Candidate->Candidate->PUserDefinedOperator()->GetOperatorMethod());

            if
            (
                ! Candidate->NotCallable &&
                Candidate->RequiresNarrowingConversion&&
                Candidate->Candidate->PUserDefinedOperator()->GetOperatorMethod()->IsLiftedOperatorMethod()
            )
            {

                for (OverloadList * Candidate2 = Candidates; Candidate2; Candidate2 = Candidate2->Next)
                {
                    ThrowIfFalse(Candidate2->Candidate->IsUserDefinedOperator());
                    ThrowIfNull(Candidate2->Candidate->PUserDefinedOperator()->GetOperatorMethod());

                    if
                    (
                        ! Candidate2->NotCallable &&
                        Candidate2->RequiresNarrowingConversion &&
                        ! Candidate2->Candidate->PUserDefinedOperator()->GetOperatorMethod()->IsLiftedOperatorMethod() &&
                        Candidate2->Candidate->PUserDefinedOperator()->GetOperatorMethod() == Candidate->Candidate->PUserDefinedOperator()->GetOperatorMethod()->PLiftedOperatorMethod()->GetActualProc() &&
                        !Candidate2->AllNarrowingIsFromNumericLiteral
                    )
                    {
                        --CandidateCount;

                        if
                        (
                            Candidate2->RequiresUnwrappingNullable
                        )
                        {
                            Candidate2->NotCallable = true;
                            BestCandidate = Candidate;
                            break;
                        }
                        else
                        {
                            Candidate->NotCallable = true;
                            BestCandidate = Candidate2;
                            break;
                        }
                    }
                }
            }
        }
    }


    //If we only have 1 candidate then BestCandidate will point to that one narrowing candidate.
    if (CandidateCount == 1 && BestCandidate)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Picked winner %s, even though it's narrowing, because it's the only candidate left\n", CandidateToDebugString(BestCandidate));

        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    BestCandidate = NULL;

    for (Candidate = Candidates;
         Candidate;
         Candidate = Candidate->Next)
    {
        if (!Candidate->NotCallable)
        {
            if (Candidate->RequiresNarrowingConversion)
            {
                DBG_SWITCH_PRINTF(fDumpOverload, L"  Rejected candidate %s because it requires narrowing\n", CandidateToDebugString(Candidate));

                CandidateCount--;

                if (Candidate->AllNarrowingIsFromObject && !Candidate->IsExtensionMethod)
                {
                    NarrowOnlyFromObjectCount++;
                    BestNarrowingCandidate = Candidate;
                }
            }
            else
            {
                BestCandidate = Candidate;
            }
        }
    }

    if (CandidateCount == 1 && BestCandidate)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Picked winner %s because it's the only non-narrowing candidate left\n", CandidateToDebugString(BestCandidate));

        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    if (CandidateCount == 0)
    {
        if (!HasFlag(OvrldFlags, OvrldIgnoreLateBound) && NarrowOnlyFromObjectCount > 0 && !m_UsingOptionTypeStrict)
        {
            if (NarrowOnlyFromObjectCount == 1)
            {
                GenericBindingContext = BestNarrowingCandidate->Binding.PGenericBinding();
                return BestNarrowingCandidate->Candidate;
            }

            MatchingCandidate = NULL;
            return VerifyLateboundCallConditions(
                CallLocation,
                OverloadedMemberName,
                DelegateType,
                Candidates,
                Arguments,
                ArgumentCount,
                DelegateReturnType,
                ResolutionFailed,
                ResolutionIsLateBound,
                OvrldFlags);
        }

        // Although all candidates narrow, there may be a best choice when factoring in narrowing of
        // numeric literals.  Attempt to find that choice now.


        BestCandidate =
            MostSpecificProcedure
            (
                Candidates,
                CandidateCount,
                Arguments,
                true,/* look at safe conversions */
                OvrldFlags
            );

        if (BestCandidate)
        {
            GenericBindingContext = BestCandidate->Binding.PGenericBinding();
            MatchingCandidate = BestCandidate;
            return BestCandidate->Candidate;
        }

        ReportNarrowingProcedures
        (
            CallLocation,
            OverloadedMemberName,
            DelegateType,
            Candidates,
            Arguments,
            ArgumentCount,
            DelegateReturnType,
            OvrldFlags
        );

        ResolutionFailed = true;
        MatchingCandidate = NULL;
        return NULL;
    }

    BestCandidate = MostSpecificProcedure(Candidates, CandidateCount, Arguments, false, OvrldFlags);

    if (BestCandidate)
    {
        DBG_SWITCH_PRINTF(fDumpOverload, L"  Picked winner %s due to being most applicable/specific\n", CandidateToDebugString(BestCandidate));

        VSASSERT(CandidateCount == 1, "If LookAtSafeConversions=false, then candidate count must be 1");
        GenericBindingContext = BestCandidate->Binding.PGenericBinding();
        MatchingCandidate = BestCandidate;
        return BestCandidate->Candidate;
    }

    ReportUnspecificProcedures
    (
        CallLocation,
        OverloadedMemberName,
        Candidates,
        Arguments,
        ArgumentCount,
        DelegateReturnType,
        OvrldFlags
    );

    ResolutionIsAmbiguous = true;
    ResolutionFailed = true;
    MatchingCandidate = NULL;
    return NULL;
}

void
Semantics::ReportNoExtensionCallCandidatesError
(
    unsigned RejectedForArgumentCount,
    unsigned RejectedForTypeArgumentCount,
    _In_opt_ ILTree::ExtensionCallExpression * pExtensionCall,
    ExpressionList *pBoundArguments,
    unsigned boundArgumentsCount,
    OverloadResolutionFlags OvrldFlags,
    const Location & callLocation
)
{
    ThrowIfNull(pExtensionCall);
    ThrowIfNull(pExtensionCall->ExtensionCallLookupResult);

    if (m_ReportErrors)
    {
        if (!pExtensionCall->ExtensionCallLookupResult->IsOverloads() &&
            (!HasFlag(OvrldFlags, OvrldExactArgCount) || !RejectedForArgumentCount))
        {
            Procedure * pProc= pExtensionCall->ExtensionCallLookupResult->GetFirstExtensionMethod();

            if (RejectedForArgumentCount)
            {
                ILTree::Expression * pIgnored = NULL;
                bool ignoredBool[10] = {0};
                DelegateRelaxationLevel DelegateRelaxationLevel = DelegateRelaxationLevelNone;
                TypeInferenceLevel TypeInferenceLevel = TypeInferenceLevelNone;

                GenericBindingInfo genericBinding = pExtensionCall->ExtensionCallLookupResult->GetPartialGenericBindingForFirstExtensionMethod();

                MatchArguments3
                (
                    callLocation,
                    pProc,
                    NULL,
                    genericBinding,
                    pBoundArguments,
                    NULL,
                    ExprNoFlags,
                    OvrldFlags,
                    pIgnored,
                    false,
                    false,
                    false,
                    false,
                    ignoredBool[0],
                    ignoredBool[1],
                    ignoredBool[2],
                    ignoredBool[3],
                    ignoredBool[4],
                    ignoredBool[5],
                    ignoredBool[6],
                    ignoredBool[7],
                    false,
                    true,
                    DelegateRelaxationLevel,
                    TypeInferenceLevel,
                    ignoredBool[8],
                    ignoredBool[9]
                );
            }
            else if (RejectedForTypeArgumentCount)
            {

                Location errorLocation;
                errorLocation.SetStart(&pExtensionCall->TypeArgumentLocations[0]);
                errorLocation.SetEnd(&pExtensionCall->TypeArgumentLocations[pExtensionCall->TypeArgumentCount - 1]);

                PartialGenericBinding * pPartialBinding = pExtensionCall->ExtensionCallLookupResult->GetPartialGenericBindingForFirstExtensionMethod();
                unsigned long freeTypeArgumentCount = pProc->GetGenericParamCount() - (pPartialBinding ? pPartialBinding->m_pFixedTypeArgumentBitVector->SetCount() : 0);

                RESID errorID = 0;

                //Here we need to select between "too many type parameters", "too few type parameters", and "foo is not generic"
                //error messages. We also need to select between the extension method version and the instance method version of each error.
                //The extension method error messages all have 2 arguments, and the instance method error messages all have 1.
                //In both cases the first argument is always the procedure name. For extension
                //method errors the second argument is always the name of the container that defined the extension method.
                //This allows us to only make a single call to ReportSemanticError below and just switch between the ERRID to use
                //here.
                if (pExtensionCall->TypeArgumentCount > freeTypeArgumentCount)
                {
                    if (freeTypeArgumentCount > 0)
                    {
                        errorID = ERRID_TooManyGenericArguments2;
                    }
                    else
                    {
                        errorID = ERRID_TypeOrMemberNotGeneric2;
                    }
                }
                else
                {
                    ThrowIfFalse(pExtensionCall->TypeArgumentCount < freeTypeArgumentCount);
                    errorID = ERRID_TooFewGenericArguments2;
                }

                if (m_ReportErrors)
                {
                    StringBuffer textBuffer;

                    ReportSemanticError
                    (
                        errorID,
                        errorLocation,
                        ExtractErrorName
                        (
                            pProc,
                            textBuffer,
                            true,
                            pPartialBinding ? pPartialBinding->m_pFixedTypeArgumentBitVector : NULL
                        ),
                        pProc->GetContainer()->GetQualifiedName()
                    );
                }
            }
        }
        else
        {
            RESID errorID = 0;

            if (RejectedForArgumentCount)
            {
                errorID = ERRID_NoArgumentCountOverloadCandidates1;
            }
            else if (RejectedForTypeArgumentCount)
            {
                errorID = ERRID_NoTypeArgumentCountOverloadCand1;
            }
            else
            {
                errorID = ERRID_NoViableOverloadCandidates1;
            }

            ReportSemanticError
            (
                errorID,
                callLocation,
                pExtensionCall->ExtensionCallLookupResult->GetFirstExtensionMethod()->GetErrorName(m_Compiler)
            );
        }
    }
}


Procedure *
Semantics::ResolveExtensionCallOverloading
(
    _In_opt_ ILTree::ExtensionCallExpression * ExtCall,
    ExpressionList * BoundArguments,
    unsigned BoundArgumentsCount,
    GenericBindingInfo  &GenericBindingContext,
    ExpressionFlags Flags,
    OverloadResolutionFlags OvrldFlags,
    const Location & CallLocation,
    Type *DelegateType,
    Type * InvokeMethodReturnType,
    bool & SomeCandidatesBad,
    bool & ResultIsExtensionMethod,
    bool & IsBadSingleton,
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    ThrowIfNull(ExtCall);

    NorlsAllocator Scratch(NORLSLOC);
    unsigned int RejectedBySuperTypeFilter = 0;
    OverloadList * Candidates = NULL;
    OverloadList * InstanceMethodOnlyCandidates = NULL;
    unsigned CandidateCount = 0;
    unsigned InstanceMethodOnlyCandidateCount = NULL;
    unsigned RejectedForArgumentCount = 0;
    unsigned RejectedForTypeArgumentCount = 0;

    CollectExtensionMethodOverloadCandidates
    (
        ExtCall->ExtensionCallLookupResult,
        BoundArguments,
        BoundArgumentsCount,
        ExtCall->TypeArguments,
        ExtCall->TypeArgumentLocations,
        ExtCall->TypeArgumentCount,
        Scratch,
        CandidateCount,
        RejectedForArgumentCount,
        RejectedForTypeArgumentCount,
        RejectedBySuperTypeFilter,
        CallLocation,
        OvrldFlags,
        InvokeMethodReturnType,
        Flags,
        InstanceTypeOfReference(ExtCall->ImplicitArgumentList->Left),
        SomeCandidatesBad,
        Candidates,
        InstanceMethodOnlyCandidates,
        InstanceMethodOnlyCandidateCount,
        ppAsyncSubArgumentListAmbiguity
    );

    bool ResolutionFailed = false;
    bool ResolutionIsLateBound = false;
    bool ResolutionIsAmbiguous = false;

    if (CandidateCount > 0 )
    {
        bool ignored = false;

        OverloadList * candidate = NULL;

        GenericBinding * pTmpBinding = GenericBindingContext.PGenericBinding();

        Procedure * Result =
            ViewAsProcedureHandlingNulls
            (
                ResolveOverloading
                (
                    CallLocation,
                    Candidates,
                    CandidateCount,
                    BoundArguments,
                    BoundArgumentsCount,
                    DelegateType,
                    InvokeMethodReturnType,
                    ResolutionFailed,
                    OvrldFlags,
                    ResolutionIsLateBound,
                    ResolutionIsAmbiguous,
                    pTmpBinding,
                    &candidate,
                    false,
                    InstanceMethodOnlyCandidates,
                    InstanceMethodOnlyCandidateCount,
                    &IsBadSingleton,
                    ppAsyncSubArgumentListAmbiguity
                )
            );

        GenericBindingContext = pTmpBinding;

        if (candidate)
        {
            ResultIsExtensionMethod = candidate->IsExtensionMethod;
        }

        if (Result && ! ResolutionFailed && ! ResolutionIsLateBound)
        {
            if (candidate)
            {
                if (! candidate->Binding.IsNull() && m_ReportErrors)
                {
                    ThrowIfFalse(!candidate->IsExtensionMethod || candidate->Binding.IsFullMethodBinding());

                    Bindable::CheckGenericConstraints
                    (
                        candidate->Binding.PGenericBinding(),
                        candidate->Binding.GetTypeArgumentLocations(),
                        NULL,
                        m_Errors,
                        m_CompilerHost,
                        m_Compiler,
                        &m_SymbolCreator,
                        m_CompilationCaches
                    );
                }


                CheckObsolete(candidate->Candidate, ExtCall->Loc);
            }

            return Result;
        }
        // Port SP1 CL 2967675 to VS10
        else if (Result && IsBadSingleton && (OvrldFlags & OvrldReturnUncallableSingletons))
        {
            return Result;
        }
        else if ( ResolutionIsLateBound )
        {
            ReportSemanticError(ERRID_ExtensionMethodCannotBeLateBound, CallLocation);
        }
    }
    else if (!ExtCall->ExtensionCallLookupResult->IsOverloads() && (OvrldFlags & OvrldReturnUncallableSingletons) && RejectedForArgumentCount)
    {
        // Port SP1 CL 2967675 to VS10
        IsBadSingleton = true;
        ResolutionFailed = true;
        ResultIsExtensionMethod = true;

        GenericBindingContext = ExtCall->ExtensionCallLookupResult->GetPartialGenericBindingForFirstExtensionMethod();

        if
        (
            ExtCall->TypeArgumentCount &&
            !RejectedForTypeArgumentCount &&
            ExtCall->TypeArgumentCount == GenericBindingContext.FreeTypeArgumentCount()
        )
        {
            ThrowIfFalse(GenericBindingContext.IsPartialBinding());

            //Apply any explict type arguments to our partial binding so that
            //we can generate better error messages.
            GenericBindingContext.ApplyExplicitArgumentsToPartialBinding
            (
                ExtCall->TypeArguments,
                ExtCall->TypeArgumentLocations,
                ExtCall->TypeArgumentCount,
                this,
                ExtCall->ExtensionCallLookupResult->GetFirstExtensionMethod()
            );
        }

        return
            ExtCall->ExtensionCallLookupResult->GetFirstExtensionMethod();
    }
    else
    {

        ReportNoExtensionCallCandidatesError
        (
            RejectedForArgumentCount,
            RejectedForTypeArgumentCount,
            ExtCall,
            BoundArguments,
            BoundArgumentsCount,
            OvrldFlags,
            CallLocation
        );
    }

    return NULL;
}

ILTree::Expression *
Semantics::ResolveExtensionCallOverloadingAndReferToResult
(
    ILTree::ExtensionCallExpression * ExtCall,
    ExpressionList *BoundArguments,
    unsigned BoundArgumentsCount,
    ExpressionFlags Flags,
    OverloadResolutionFlags OvrldFlags,
    const Location &CallLocation,
    bool & ResultIsExtensionMethod
)
{
    GenericBindingInfo GenericBindingContext;

    // Port SP1 CL 2967675 to VS10
    bool IgnoreIsBadSingleton = false;
    bool IgnoreSomeCandidatesBad = false;
    bool ShouldMakeBad = false;

    //We do not support the OvrldReturnUncallableSingletons flag 
    ThrowIfFalse(!(OvrldFlags &OvrldReturnUncallableSingletons));

    AsyncSubAmbiguityFlagCollection *pAsyncSubArgumentListAmbiguity = NULL;

    Procedure * pProc =
        ResolveExtensionCallOverloading
        (
            ExtCall,
            BoundArguments,
            BoundArgumentsCount,
            GenericBindingContext,
            Flags,
            OvrldFlags,
            CallLocation,
            NULL, // no delegate invoke method,
            NULL, //no delegate return type,
            IgnoreSomeCandidatesBad,
            ResultIsExtensionMethod,
            IgnoreIsBadSingleton,
            &pAsyncSubArgumentListAmbiguity
        );

    ReportAndDeleteAsyncSubArgumentListAmbiguity(&pAsyncSubArgumentListAmbiguity, pProc != NULL && !IsBad(pProc));

    if (pProc )
    {

        ILTree::Expression * BaseReference = BoundArguments->AsExpressionWithChildren().Left->AsArgumentExpression().Left;;

        if (!ResultIsExtensionMethod && !ExtCall->ImplicitMeErrorID && !IsBad(BaseReference))
        {
            if (! (pProc->IsShared() && HasFlag32(ExtCall, SXF_EXTENSION_CALL_ME_IS_SYNTHETIC)))
            {
                //Overload resolution picked an instance method.
                //As a result, we need to extract out the receiver argument and
                //use that as the base reference before we generate a symbol reference
                BaseReference = MakeRValue(BoundArguments->AsExpressionWithChildren().Left->AsArgumentExpression().Left);
            }
            else
            {
                BaseReference = NULL;
            }
        }
        else
        {
            if (IsBad(BaseReference) && ! ExtCall->ImplicitMeErrorID)
            {
                ShouldMakeBad = true;
            }
            else if (ExtCall->ImplicitMeErrorID && (ResultIsExtensionMethod || ! pProc->IsShared()))
            {
                ReportSemanticError
                (
                    ExtCall->ImplicitMeErrorID,
                    BoundArguments->AsExpressionWithChildren().Left->AsArgumentExpression().Left->Loc
                );
                ShouldMakeBad = true;
            }
            BaseReference = NULL;
        }

        ILTree::Expression * Result =
            AllocateSymbolReference
            (
                pProc,
                TypeHelpers::GetVoidType(),
                BaseReference,
                CallLocation,
                GenericBindingContext.PGenericBinding()
            );


        if (ShouldMakeBad)
        {
            Result = MakeBad(Result);
        }

        return Result;
    }
    else
    {
        return NULL;
    }
 }



void Semantics::AddAsyncSubArgumentAmbiguity(
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity,
    _In_ ILTree::Expression *Argument,
    AsyncSubAmbiguityFlags AsyncSubArgumentAmbiguity
)
{
    // This function is used in warning when the user passed an "Async Sub" argument
    // but should probably have passed "Async Function".
    //
    // The "ppAsyncSubArgumentListAmbiguity" initially points to a NULL hashset.
    // But the first time we're asked to store something in it, we lazily allocate
    // the hashset and store the thing in it.
    //
    // What do we store? Well, for each argument, we store whether the argument
    // matched an "Action" parameter, and also whether the argument missed matching
    // a "Func" parameter. We're strictly "additive", i.e. we merge any new information
    // in with the old.
    //
    // NOTE: The keys in the hashset are ArgumentExpressions (or ParamarrayArgumentExpressions).
    // Think of this as a holder for the actual argument expression.
    // It represents the argument (perhaps named) in the list of arguments passed to the method.
    // It has a pointer to the actual Expression itself.

    if (ppAsyncSubArgumentListAmbiguity == NULL || AsyncSubArgumentAmbiguity == 0)
    {
        return;
    }

    if (*ppAsyncSubArgumentListAmbiguity == NULL)
    {
        *ppAsyncSubArgumentListAmbiguity = new AsyncSubAmbiguityFlagCollection();
    }

    auto pAsyncSubArgumentListAmbiguity = *ppAsyncSubArgumentListAmbiguity;

    if (pAsyncSubArgumentListAmbiguity->Contains(Argument))
    {
        AsyncSubAmbiguityFlags oldAsyncSubArgumentAmbiguity = pAsyncSubArgumentListAmbiguity->GetValue(Argument);
        AsyncSubAmbiguityFlags newAsyncSubArgumentAmbiguity = oldAsyncSubArgumentAmbiguity | AsyncSubArgumentAmbiguity;
        if (oldAsyncSubArgumentAmbiguity != newAsyncSubArgumentAmbiguity)
        {
            pAsyncSubArgumentListAmbiguity->SetValue(Argument, newAsyncSubArgumentAmbiguity);
        }
        return;
    }

    pAsyncSubArgumentListAmbiguity->SetValue(Argument, AsyncSubArgumentAmbiguity);

}


void Semantics::ReportAndDeleteAsyncSubArgumentListAmbiguity(
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity,
    bool MakeReport
)
{
    // Over time we might have built up a list of ambiguities where the user passed an "Async Sub"
    // argument but should probably have passed "Async Function".
    //
    // In this function we delete the list we've built up.
    // We might also report on those ambiguities with a warning.
    // (Note: the "MakeReport" flag is used to control the reporting. Typically there'll be no need
    // to report ambiguities if overload resolution failed to find any candidates).


    if (ppAsyncSubArgumentListAmbiguity == NULL || *ppAsyncSubArgumentListAmbiguity == NULL)
    {
        return;
    }

    auto pAsyncSubArgumentListAmbiguity = *ppAsyncSubArgumentListAmbiguity;

    if (MakeReport)
    {
        HashTableIterator<ILTree::Expression*,AsyncSubAmbiguityFlags,VBAllocWrapper> ambiguity(pAsyncSubArgumentListAmbiguity);
        while (ambiguity.MoveNext())
        {
            ILTree::Expression *ambiguousArgument = ambiguity.Current().Key();
            int info = ambiguity.Current().Value();
            if (info != (FoundAsyncOverloadWithAction | FoundAsyncOverloadWithFunc))
            {
                continue;
            }

            Location loc = GetLocationForErrorReporting(ambiguousArgument);
            ReportSemanticError(WRNID_AsyncSubCouldBeFunction, loc);
        }
    }

    delete pAsyncSubArgumentListAmbiguity;
    *ppAsyncSubArgumentListAmbiguity = NULL;
}


Location Semantics::GetLocationForErrorReporting(_In_ ILTree::Expression *pExpr)
{
    // Typically the "squiggly location" of an expression is the whole expression.
    // But this makes for a crummy experience in the case of multiline lambdas expressions,
    // where it's not nice to squiggly the entire multiline thing. So in that case
    // we merely squiggly the "Async Sub(...)" bit of it.

    if (pExpr == NULL)
    {
        VSFAIL("Expected non-NULL expression");
        return Location::GetInvalidLocation();
    }

    if (pExpr->bilop == SX_ARG && pExpr->AsArgumentExpression().Left != NULL)
    {
        pExpr = pExpr->AsArgumentExpression().Left;
    }

    if (pExpr->bilop != SX_UNBOUND_LAMBDA)
    {
        return pExpr->Loc;
    }

    auto pLambdaExpr = &pExpr->AsUnboundLambdaExpression();
    // Let's pick out the location of just the "Async Sub(..)" or "Async Function(...)" here:
    Location loc = pLambdaExpr->Loc;
    loc.m_lEndLine = pLambdaExpr->BodySpan.m_lBegLine;
    loc.m_lEndColumn = pLambdaExpr->BodySpan.m_lBegColumn;
    return loc;
}
