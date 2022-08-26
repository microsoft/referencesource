//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB Query Expression semantic analysis.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

Vtypes LookupInOperatorTables(
    BILOP Opcode,
    Vtypes Left,
    Vtypes Right);

ILTree::Expression * Semantics::AttemptQueryNameLookup(
    Location & location,
    _In_z_ Identifier *Name,
    typeChars TypeCharacter,
    NameFlags NameLookupFlags,
    ExpressionFlags Flags,
    int GenericTypeArity,
    _Out_opt_ Symbol ** pNameBinding,
    bool CheckUseOfLocalBeforeDeclaration /* = true */)
{
    Symbol * NameBinding = NULL;
    ILTree::Expression * Result = NULL;

    if (m_Lookup->IsLocalsHash())
    {
        // try bind name to a local first
        NameBinding =
            AttemptInterpretLocalReference
            (
                location,
                Name,
                NameLookupFlags,
                Flags,
                GenericTypeArity,
                &Result,
                CheckUseOfLocalBeforeDeclaration
            );
    }

    bool CheckJoinKey = false;

    //Binding to locals failed, so we now attempt to bind to the direct members of iteration variables
    if ((! NameBinding && ! Result) ||
        (CheckJoinKey = (NameBinding && m_JoinKeyBuilderList && NameBinding->IsVariable())))
    {
        ILTree::Expression * save_Result = Result;
        Symbol * save_NameBinding = NameBinding;

        NameBinding = NULL;

        Result = AttemptInterpretMemberReference(location,Name, TypeCharacter, NameLookupFlags, Flags, GenericTypeArity,
                    CheckJoinKey ? save_NameBinding->PVariable()->GetImmediateParent() : NULL);

        if(!Result && CheckJoinKey)
        {
            Result = save_Result;
            NameBinding = save_NameBinding;
            m_JoinKeyBuilderList->CheckName(NameBinding->PVariable(), Name, location);
        }
        else
        // (Bug 49307 - DevDiv Bugs) Convert Property ref into property get call
        if(Result && !IsBad(Result) &&
            HasFlag(Flags, ExprIsExplicitCallTarget) &&
            Result->bilop == SX_SYM && IsProcedure(Result->AsSymbolReferenceExpression().Symbol) &&
            IsProperty(ViewAsProcedure(Result->AsSymbolReferenceExpression().Symbol)))
        {
            Result =
                BindArgsAndInterpretCallExpressionWithNoCopyOut(
                    location,
                    Result,
                    TypeCharacter,
                    NULL,
                    ExprForceRValue,
                    OvrldNoFlags,
                    NULL);
        }
    }

    AssertIfNull(pNameBinding);
    if(pNameBinding)
    {
        *pNameBinding = NameBinding;
    }

    return Result;
}

ILTree::Expression *
Semantics::AttemptInterpretMemberReference
(
    Location & location,
    _In_z_ Identifier *Name,
    typeChars TypeCharacter,
    NameFlags NameLookupFlags,
    ExpressionFlags ExprFlags,
    int GenericTypeArity,
    BCSYM_NamedRoot * cutOffParent
)
{
    Variable * candidateVariable = NULL;

    NorlsAllocator Scratch(NORLSLOC);
    CSingleList<AttemptInterpretMemberReferenceInfo> MemberReferenceList;
    AttemptInterpretMemberReferenceInfo * memberInfo;

    ErrorTable * pTemporaryErrorTable = NULL;
    BackupValue<ErrorTable *> backup_error_table(&m_Errors);

    // !!! DANGER !!!   See the comments in TemporaryErrorTable::Restore.
    // In general, InterpretExpression can have side-effects on the SX_ tree as well (in the case of
    // unbound lambdas function(x)x), which we might have had to to merge. Luckily for us, though,
    // the only InterpretExpression that we're doing is simple name references so that doesn't apply.

    if(m_Errors)
    {
        pTemporaryErrorTable = new  ErrorTable(*m_Errors);
    }

    m_Errors = pTemporaryErrorTable;

    // scan through all variables in scope and try to bind to their members
    QueryMemberLookupVariable * currentVar;

    for(currentVar = m_QueryMemberLookupList.GetFirst();
          currentVar;
          currentVar = currentVar->Next())
    {
        AssertIfFalse(currentVar->IterationVariable->IsQueryRecord());

        if(cutOffParent)
        {
            // consider this variable only if its parent below the cutOffParent
            BCSYM_NamedRoot * parent = currentVar->IterationVariable->GetImmediateParent();

            if(parent)
            {
                parent = parent->GetImmediateParent();
            }

            while(parent && parent!=cutOffParent)
            {
                parent = parent->GetImmediateParent();
            }

            if(!parent)
            {
                continue;
            }
        }

        if (AttemptInterpretMemberReference(
                    currentVar->IterationVariable->GetType(),
                    location,
                    Name,
                    NameLookupFlags,
                    GenericTypeArity,
                    &Scratch,
                    MemberReferenceList))
        {
            candidateVariable = currentVar->IterationVariable;
            break;
        }
        else if (AttemptInterpretNestedMemberReference(
                    currentVar->IterationVariable->GetType(),
                    location,
                    Name,
                    NameLookupFlags,
                    GenericTypeArity,
                    &Scratch,
                    MemberReferenceList))
        {
            candidateVariable = currentVar->IterationVariable;
            break;
        }
    }

    backup_error_table.Restore();

    if(pTemporaryErrorTable)
    {
        delete pTemporaryErrorTable;
    }

    if (!MemberReferenceList.IsEmpty())
    {
        //We found something, so refer to what we found, and return the generated expression
        ILTree::Expression * base =
                ReferToSymbol
                (
                    location,
                    candidateVariable,
                    chType_NONE,
                    NULL,
                    NULL,
                    ExprFlags
                    );

        if(m_JoinKeyBuilderList)
        {
            m_JoinKeyBuilderList->CheckName(candidateVariable, Name, location);
        }

        for(memberInfo = MemberReferenceList.GetFirst();
              !IsBad(base) && memberInfo;
              memberInfo = memberInfo->Next())
        {
            base =
                ReferToSymbol
                (
                    location,
                    memberInfo->candidateNameBinding,
                    (memberInfo->Next() ? chType_NONE : TypeCharacter),
                    base,
                    memberInfo->candidateGenericBindingContext,
                    ExprSuppressMeSynthesis |
                    //  (Bug 53114 - DevDiv Bugs)
                     (memberInfo->Next() ?
                                                        ExprSuppressImplicitVariableDeclaration |
                                                        ExprLeadingQualifiedName
                                                     :  ExprFlags)
                );
        }

        MemberReferenceList.Clear();

        return base;
    }
    else
    {
        return NULL;
    }
}


Semantics::AttemptInterpretMemberReferenceInfo *
Semantics::AttemptInterpretMemberReference
(
    Type * lookInType,
    Location & location,
    _In_z_ Identifier *Name,
    NameFlags NameLookupFlags,
    int GenericTypeArity,
    _Inout_ NorlsAllocator * Scratch,
    _Inout_ CSingleList<AttemptInterpretMemberReferenceInfo> &MemberReferenceList
)
{
    bool NameIsBad = false;
    GenericBinding * GenericBindingContext=NULL;
    Scope * pLookup = NULL;
    GenericParameter * pGenericParameter = NULL;

    lookInType->GetBindingSource(m_CompilerHost, &pLookup, &pGenericParameter);

    Symbol * NameBinding =
        InterpretName
        (
            Name,
            pLookup,
            pGenericParameter,
            (
                NameLookupFlags |
                NameSearchIgnoreParent |
                NameSearchIgnoreImports
            ),
            ContainingClass(),
            location,
            NameIsBad,
            &GenericBindingContext,
            GenericTypeArity
        );

    if (NameBinding && ! NameIsBad && CanAccessMemberUnqualified(NameBinding))
    {
        UpdateNameLookupGenericBindingContext(NameBinding, lookInType, &GenericBindingContext);

        AttemptInterpretMemberReferenceInfo * memberInfo;

        memberInfo = new(*Scratch) AttemptInterpretMemberReferenceInfo;
        memberInfo->candidateGenericBindingContext=GenericBindingContext;
        memberInfo->candidateNameBinding = NameBinding;

        MemberReferenceList.InsertLast(memberInfo);

        return memberInfo;
    }

    return NULL;
}


bool
Semantics::AttemptInterpretNestedMemberReference
(
    Type * lookInType,
    Location & location,
    _In_z_ Identifier *Name,
    NameFlags NameLookupFlags,
    int GenericTypeArity,
    _Inout_ NorlsAllocator * pScratch,
    _Inout_ CSingleList<AttemptInterpretMemberReferenceInfo> &MemberReferenceList
)
{
    Identifier * transparentMembers[] = {STRING_CONST(m_Compiler, It1), STRING_CONST(m_Compiler, It2)};
    int i;
    NorlsMark mark;
    AttemptInterpretMemberReferenceInfo * memberInfo;
    CSingleList<AttemptInterpretMemberReferenceInfo> localMemberReferenceList;

    pScratch->Mark(&mark);

    for(i=0; i< sizeof(transparentMembers)/sizeof(transparentMembers[0]); i++)
    {
        memberInfo = AttemptInterpretMemberReference(
                    lookInType,
                    location,
                    transparentMembers[i],
                    NameLookupFlags,
                    GenericTypeArity,
                    pScratch,
                    localMemberReferenceList);

        if(memberInfo)
        {
            Type * memberType = memberInfo->candidateNameBinding->PMember()->GetType();

            if(memberInfo->candidateGenericBindingContext)
            {
                memberType = ReplaceGenericParametersWithArguments(memberType, memberInfo->candidateGenericBindingContext, m_SymbolCreator);
            }

            if (AttemptInterpretMemberReference(
                                memberType,
                                location,
                                Name,
                                NameLookupFlags,
                                GenericTypeArity,
                                pScratch,
                                localMemberReferenceList))
            {
                MemberReferenceList.Splice(&localMemberReferenceList);
                return true;
            }
            else if(AttemptInterpretNestedMemberReference(
                                memberType,
                                location,
                                Name,
                                NameLookupFlags,
                                GenericTypeArity,
                                pScratch,
                                localMemberReferenceList))
            {
                MemberReferenceList.Splice(&localMemberReferenceList);
                return true;
            }

            // cleanup the list and release the memory
            localMemberReferenceList.Clear();
            pScratch->Free(&mark);
        }
    }

    return false;
}


bool
Semantics::CanAccessMemberUnqualified
(
        Symbol * Referenced
)
{
    // Allow only public instance fields and properties

    if (!Referenced || Referenced->IsBad() || !Referenced->IsNamedRoot() || Referenced->PNamedRoot()->GetAccess() != ACCESS_Public)
        return false;

    if (Referenced->IsVariable())
    {
        Variable *ReferencedVariable = Referenced->PVariable();

        return !ReferencedVariable->IsShared();
    }
    else if (IsProcedure(Referenced))
    {
        Procedure * proc = ViewAsProcedure(Referenced);

        return proc->IsProperty() && !proc->IsShared();
    }

    return false;
}


ParseTree::IdentifierDescriptor
Semantics::Join1IdentifierDescriptor
(
)
{
    return ItIdentifierDescriptor(STRING_CONST(m_Compiler, It1));
}

ParseTree::IdentifierDescriptor
Semantics::Join2IdentifierDescriptor
(
)
{
    return ItIdentifierDescriptor(STRING_CONST(m_Compiler, It2));
}

ParseTree::IdentifierDescriptor
Semantics::ItIdentifierDescriptor
(
)
{
    return ItIdentifierDescriptor(STRING_CONST(m_Compiler, It));
}

ParseTree::IdentifierDescriptor
Semantics::ItIdentifierDescriptor
(
    _In_z_ STRING * id
)
{
    ParseTree::IdentifierDescriptor ItName;

    ItName.Name = id;
    ItName.TypeCharacter = chType_NONE;
    ItName.IsBracketed = false;
    ItName.IsBad = false;
    ItName.TextSpan.SetLocationToHidden();
    ItName.IsNullable = false;

    return ItName;
}

Type *
Semantics::InferControlVariableType
(
    Declaration * selectMethodDecl,
    _In_opt_ GenericBindingInfo *TargetBinding
)
{
    Procedure * selectMethod;

    AssertIfFalse(IsProcedure(selectMethodDecl));

    if(!IsProcedure(selectMethodDecl))
    {
        return NULL;
    }

    selectMethod = ViewAsProcedure(selectMethodDecl);

    AssertIfTrue(IsProperty(selectMethod) || IsEvent(selectMethod) || IsSub(selectMethod));

    if(IsProperty(selectMethod) || IsEvent(selectMethod) || IsSub(selectMethod))
    {
        return NULL;
    }

    bool IsExtension = selectMethod->IsExtensionMethod();

    unsigned RequiredParameterCount = 0;
    unsigned MaximumParameterCount = 0;
    bool HasParamArray = false;

    selectMethod->GetAllParameterCounts(RequiredParameterCount, MaximumParameterCount, HasParamArray);

    Parameter *parameter = selectMethod->GetFirstParam();

    if(parameter && IsExtension)
    {
        AssertIfTrue(MaximumParameterCount!=2);

        if(MaximumParameterCount!=2)
        {
            return NULL;
        }

        parameter = parameter->GetNext();
    }
    else if(MaximumParameterCount!=1)
    {
        AssertIfTrue(IsExtension);
        return NULL;
    }

    if(!parameter)
    {
        AssertIfTrue(IsExtension);
        return NULL;
    }

    Type * paramType =  parameter->GetCompilerType();
    Type * delegate = NULL;
    Type * GenericExpressionType = NULL;

    if(paramType && TargetBinding)
    {
        paramType = ReplaceGenericParametersWithArguments(paramType, *TargetBinding, m_SymbolCreator);
    }

    if (paramType &&
        TypeHelpers::IsGenericTypeBinding(paramType) &&
        (GenericExpressionType = GetFXSymbolProvider()->GetGenericExpressionType()) &&
        TypeHelpers::EquivalentTypes(paramType->PGenericTypeBinding()->GetGenericType(),
                                    GenericExpressionType))
    {
        paramType = paramType->PGenericTypeBinding()->GetArgument(0);
    }

    if(paramType && paramType->IsDelegate())
    {
        delegate = paramType;
    }

    if(!delegate)
    {
        return NULL;
    }

    // Get Invoke declaration for the delegate
    bool InvokeIsBad = false;
    GenericBinding * GenericBindingContext=NULL;
    Location loc;
    loc.Invalidate();

    Symbol *Invoke =
        InterpretName(
            STRING_CONST(m_Compiler, DelegateInvoke),
            ViewAsScope(delegate->PClass()),
            NULL,   // No Type parameter lookup
            NameSearchIgnoreParent,
            ContainingClass(),
            loc,
            InvokeIsBad,
            &GenericBindingContext,
            -1);

    if (Invoke == NULL || !IsProcedure(Invoke) || InvokeIsBad)
    {
        return NULL;
    }

    UpdateNameLookupGenericBindingContext(delegate, &GenericBindingContext);

    Procedure * invoke = ViewAsProcedure(Invoke);

    if(IsSub(invoke))
    {
        return NULL;
    }

    RequiredParameterCount = 0;
    MaximumParameterCount = 0;
    HasParamArray = false;

    invoke->GetAllParameterCounts(RequiredParameterCount, MaximumParameterCount, HasParamArray);

    if(RequiredParameterCount!=1 || MaximumParameterCount!=1)
    {
        return NULL;
    }

    parameter = invoke->GetFirstParam();
    paramType =  parameter->GetCompilerType();

    if(paramType && GenericBindingContext)
    {
        paramType =
                ReplaceGenericParametersWithArguments(paramType, GenericBindingContext, m_SymbolCreator);
    }

    // Do not allow ByRef (Bug 64295 - DevDiv Bugs)
    if (paramType && paramType->IsPointerType())
    {
        return NULL;
    }

    return paramType->DigThroughArrayLiteralType(&m_SymbolCreator);
}

Type *
Semantics::InferControlVariableType
(
    _In_ OverloadList *Candidates,
    unsigned CandidateCount,
    bool & failedDueToAnAmbiguity
)
{
    Type * vartype =NULL;

    failedDueToAnAmbiguity = false;

    if (CandidateCount == 1)
    {
        if(!Candidates->NotCallable)
        {
#if DEBUG
            AssertIfFalse(IsProcedure(Candidates->Candidate));
            Procedure * RealProc = ViewAsProcedure(Candidates->Candidate);
            AssertIfFalse(!IsEvent(RealProc) && (!RealProc->IsShared() || Candidates->IsExtensionMethod));
#endif
            vartype = InferControlVariableType(Candidates->Candidate, &Candidates->Binding);

            if(vartype && RefersToGenericParameter(vartype, Candidates->Candidate))
            {
                failedDueToAnAmbiguity = true;
                vartype = NULL;
            }
        }
    }
    else if (CandidateCount > 0)
    {
        OverloadList *Candidate = NULL;

        for (Candidate = Candidates; Candidate; Candidate = Candidate->Next)
        {
#if DEBUG
            AssertIfFalse(IsProcedure(Candidate->Candidate));
            Procedure * RealProc = ViewAsProcedure(Candidate->Candidate);
            AssertIfFalse(!IsEvent(RealProc) && (!RealProc->IsShared() || Candidate->IsExtensionMethod));
#endif

            if (!Candidate->NotCallable)
            {
                Type * type =InferControlVariableType(Candidate->Candidate, &Candidate->Binding);

                if(type)
                {
                    if(RefersToGenericParameter(type, Candidate->Candidate))
                    {
                        failedDueToAnAmbiguity = true;
                        vartype = NULL;
                        break;
                    }

                     if(!vartype)
                    {
                        vartype = type;
                    }
                    else if(!TypeHelpers::EquivalentTypes(vartype,type))
                    {
                        failedDueToAnAmbiguity = true;
                        vartype = NULL;
                        break;
                    }
                }
            }
        }
     }

    return vartype;
}


Type *
Semantics::InferControlVariableTypeFromMembers
(
    ILTree::Expression *BaseReference,
    ExpressionList *BoundArguments,
    Location SourceLoc,
    bool & failedDueToAnAmbiguity
)
{
    Procedure *TargetProcedure = NULL;
    Declaration *TargetDeclaration = NULL;

    failedDueToAnAmbiguity = false;

    Type * vartype =NULL;

    // consider only methods
    if (BaseReference->bilop == SX_SYM &&
        IsProcedure((TargetDeclaration = BaseReference->AsSymbolReferenceExpression().Symbol)) &&
        !IsProperty((TargetProcedure = ViewAsProcedure(TargetDeclaration)))
        )
    {
        GenericBinding *TargetBinding = NULL;

        TargetBinding = BaseReference->AsSymbolReferenceExpression().GenericBindingContext;

        if (TargetProcedure->IsOverloads() ||
            (IsGeneric(TargetProcedure) && (TargetBinding == NULL || TargetBinding->GetGeneric() != TargetProcedure)))
        {
            unsigned ArgumentCount = 1; //CountArguments(BoundArguments);
            bool ResolutionFailed = false;

            NorlsAllocator Scratch(NORLSLOC);
            unsigned CandidateCount = 0;
            unsigned RejectedForArgumentCount = 0;
            unsigned RejectedForTypeArgumentCount = 0;
            OverloadList *Candidates = NULL;
            Type **TypeArguments = NULL;
            Location *TypeArgumentLocations = NULL;
            unsigned TypeArgumentCount = 0;

            Candidates =
                CollectOverloadCandidates(
                    NULL,
                    TargetDeclaration,
                    TargetBinding,
                    BoundArguments,
                    ArgumentCount,
                    NULL,
                    TypeArguments,
                    TypeArgumentCount,
                    ExprNoFlags,
                    OvrldIgnoreSharedMembers | OvrldIgnoreEvents | OvrldExactArgCount | OvrldIgnoreParamArray |
                    OvrldDisableTypeArgumentInference | OvrldIgnoreSubs,
                    InstanceTypeOfReference(BaseReference->AsSymbolReferenceExpression().BaseReference),
                    Scratch,
                    CandidateCount,
                    RejectedForArgumentCount,
                    RejectedForTypeArgumentCount,
                    SourceLoc,
                    ResolutionFailed,
                    NULL);

            if (!ResolutionFailed)
            {
                vartype = InferControlVariableType(Candidates, CandidateCount, failedDueToAnAmbiguity);
            }
        }
        else if(!IsEvent(TargetProcedure) && !TargetProcedure->IsShared() &&  !IsSub(TargetProcedure))
        {
            GenericBindingInfo binding = TargetBinding;
            vartype = InferControlVariableType(TargetProcedure, &binding);

            if(vartype && RefersToGenericParameter(vartype, TargetProcedure))
            {
                failedDueToAnAmbiguity = true;
                vartype = NULL;
            }
        }
    }

    return vartype;
}

Type *
Semantics::InferControlVariableType
(
    ILTree::Expression * Source
)
{
    ParserHelper PH(&m_TreeStorage, Source->Loc);

    ParseTree::Expression  * selectCallTarget =
                    PH.CreateQualifiedExpression(
                            PH.CreateBoundExpression(Source),
                            PH.CreateNameExpression(1, STRING_CONST(m_Compiler, SelectMethod)));


    BackupValue<ErrorTable *> backup_error_table(&m_Errors);
    ErrorTable * pTemporaryErrorTable = NULL;

    if(m_Errors)
    {
        pTemporaryErrorTable = new  ErrorTable(*m_Errors);
    }

    m_Errors = pTemporaryErrorTable;

    ParseTree::ArgumentList * selectCallArgs =
                PH.CreateArgList(1, PH.CreateNothingConst());

    bool SomeArgumentsBad = false;
    ExpressionList *BoundArguments =
        InterpretArgumentList(
            selectCallArgs,
            SomeArgumentsBad,
            ExprArgumentsMustBeConstant);

    AssertIfTrue(SomeArgumentsBad);

    // try instance members first
    ILTree::Expression *BaseReference =
        InterpretExpression(
            selectCallTarget,
            (ExprIsExplicitCallTarget |
                ExprIsQueryOperator |
                ExprAccessDefaultProperty |
                ExprPropagatePropertyReference |
                ExprSuppressImplicitVariableDeclaration
                ));

    Type * vartype =NULL;

    if (!IsBad(BaseReference))
    {
        bool failedDueToAnAmbiguity = false;

        if(BaseReference->bilop != SX_EXTENSION_CALL)
        {
            vartype = InferControlVariableTypeFromMembers(BaseReference, BoundArguments, Source->Loc, failedDueToAnAmbiguity);
        }
        else
        {
            ILTree::ExtensionCallExpression * extCall = &(BaseReference->AsExtensionCallExpression());

            AssertIfFalse(BaseReference->bilop == SX_EXTENSION_CALL);

            if(!vartype && !failedDueToAnAmbiguity)
            {
                NorlsAllocator Scratch(NORLSLOC);
                unsigned CandidateCount = 0;
                unsigned RejectedForArgumentCount = 0;
                unsigned RejectedForTypeArgumentCount = 0;
                OverloadList *Candidates = NULL;
                bool ResolutionFailed = false;

                unsigned int rejectedBySuperTypeFilter = 0;

                ExpressionListHelper listHelper(this, extCall->ImplicitArgumentList);
                listHelper.Splice(BoundArguments);
                BoundArguments = listHelper.Start();

                OverloadList * InstanceMethodOnlyCandidates = NULL;
                unsigned InstanceMethodOnlyCandidateCount = 0;

                CollectExtensionMethodOverloadCandidates
                (
                    extCall->ExtensionCallLookupResult,
                    BoundArguments,
                    listHelper.Count(),
                    extCall->TypeArguments,
                    extCall->TypeArgumentLocations,
                    extCall->TypeArgumentCount,
                    Scratch,
                    CandidateCount,
                    RejectedForArgumentCount,
                    RejectedForTypeArgumentCount,
                    rejectedBySuperTypeFilter,
                    Source->Loc,
                    OvrldIgnoreEvents | OvrldExactArgCount | OvrldIgnoreParamArray |
                    OvrldDisableTypeArgumentInference | OvrldIgnoreSubs | OvrldIgnoreProperties |
                    OvrldSomeCandidatesAreExtensionMethods | OvrldIgnoreSharedMembers,
                    NULL, //no delegate return type
                    ExprNoFlags, //No expression flags
                    InstanceTypeOfReference(BaseReference->AsExtensionCallExpression().ImplicitArgumentList->Left),
                    ResolutionFailed,
                    Candidates,
                    InstanceMethodOnlyCandidates,
                    InstanceMethodOnlyCandidateCount,
                    NULL
                );

                if (!ResolutionFailed)
                {
                    //vartype = InferControlVariableType(Candidates, CandidateCount, failedDueToAnAmbiguity);
                    vartype = InferControlVariableType(InstanceMethodOnlyCandidates, InstanceMethodOnlyCandidateCount, failedDueToAnAmbiguity);

                    if (!(vartype || failedDueToAnAmbiguity))
                    {
                        vartype = InferControlVariableType(Candidates, CandidateCount, failedDueToAnAmbiguity);
                    }
                }
            }
        }
    }

    backup_error_table.Restore();

    if(pTemporaryErrorTable)
    {
        delete pTemporaryErrorTable;
    }

    return vartype;
}


ILTree::Expression *
Semantics::ToQueryableSource
(
    ILTree::Expression * Source,
    _Out_opt_ Type ** ControlVariableType,
    _Inout_opt_ QueryExpressionFlags * pQueryFlags
)
{
    Type * vartype;
    ILTree::Expression * newSource = Source;

    // first try to infer from the Source
    if(!(vartype = InferControlVariableType(newSource)))
    {
        BackupValue<ErrorTable *> backup_error_table(&m_Errors);
        ErrorTable * pTemporaryErrorTable = NULL;

        if(m_Errors)
        {
            pTemporaryErrorTable = new  ErrorTable(*m_Errors);
        }

        m_Errors = pTemporaryErrorTable;

        ParserHelper PH(&m_TreeStorage, Source->Loc);

        // try AsQueryable
        newSource =
            InterpretQueryOperatorCall(
                        PH.CreateMethodCall(
                                    PH.CreateQualifiedExpression(
                                                PH.CreateBoundExpression(Source),
                                                PH.CreateNameExpression(STRING_CONST(m_Compiler, AsQueryableMethod),Source->Loc)),
                                    NULL),
                        ExprForceRValue);

        if(IsBad(newSource) || !(vartype = InferControlVariableType(newSource)))
        {
            // try AsEnumerable

            newSource =
                InterpretQueryOperatorCall(
                            PH.CreateMethodCall(
                                        PH.CreateQualifiedExpression(
                                                    PH.CreateBoundExpression(Source),
                                                    PH.CreateNameExpression(STRING_CONST(m_Compiler, AsEnumerableMethod),Source->Loc)),
                                        NULL),
                            ExprForceRValue);

            if(IsBad(newSource) ||
                !(vartype = InferControlVariableType(newSource)))
            {
                // Need to apply Cast
                // assume control variable type is object
                vartype = GetFXSymbolProvider()->GetObjectType();

                ParseTree::CallOrIndexExpression * castCall =
                            PH.CreateMethodCall
                            (
                                PH.CreateGenericQualifiedExpression
                                (
                                    PH.CreateQualifiedExpression
                                    (
                                        PH.CreateBoundExpression(Source),
                                        PH.CreateNameExpression(STRING_CONST(m_Compiler, CastMethod),Source->Loc),
                                        Source->Loc
                                    ),
                                    PH.CreateTypeList(1, PH.CreateBoundType(vartype))
                                )
                            );


                newSource =  InterpretQueryOperatorCall( castCall, ExprForceRValue);

                if(IsBad(newSource))
                {
                    newSource = NULL;
                    vartype = NULL;
                }
            }
            else
            {
                AssertIfNull(vartype);
                AssertIfNull(newSource);
            }
        }

        backup_error_table.Restore();

        if(pTemporaryErrorTable)
        {
            delete pTemporaryErrorTable;
        }

        if(pQueryFlags && vartype)
        {
            *pQueryFlags |= AppliedAtLeastOneOperator;
        }
    }

    if(ControlVariableType)
    {
        *ControlVariableType =  vartype;
    }

    AssertIfFalse((vartype==NULL && newSource == NULL) ||(vartype!=NULL && newSource != NULL));

    return newSource;
}


ILTree::Expression *
Semantics::InterpretQueryOperatorCall
(
    _In_ ParseTree::CallOrIndexExpression * operatorCall,
    ExpressionFlags Flags
)
{
    typeChars TypeCharacter = ExtractTypeCharacter(operatorCall->Target);
    ILTree::Expression *Result = NULL;

    BackupValue<ErrorTable *> backup_error_table(&m_Errors);
    ErrorTable * pTemporaryErrorTable = NULL;

    if(m_Errors)
    {
        pTemporaryErrorTable = new  ErrorTable(*m_Errors);
    }

    m_Errors = pTemporaryErrorTable;

    ILTree::Expression *BaseReference =
        InterpretExpression
        (
            operatorCall->Target,
            (
                ExprIsQueryOperator |
                ExprIsExplicitCallTarget |
                ExprAccessDefaultProperty |
                ExprPropagatePropertyReference |
                ExprSuppressImplicitVariableDeclaration
            )
        );


    backup_error_table.Restore();

    if(pTemporaryErrorTable)
    {
        delete pTemporaryErrorTable;
    }

    Procedure *TargetProcedure = NULL;
    Declaration *TargetDeclaration = NULL;

    if
    (
        !IsBad(BaseReference) &&
        (
            (
                BaseReference->bilop == SX_SYM &&
                IsProcedure((TargetDeclaration = BaseReference->AsSymbolReferenceExpression().Symbol)) &&
                !IsProperty((TargetProcedure = ViewAsProcedure(TargetDeclaration))) &&
                !IsEvent(TargetProcedure) &&
                (TargetProcedure->IsOverloads() || (!TargetProcedure->IsShared() && !IsSub(TargetProcedure)))
            ) ||
        BaseReference->bilop == SX_OVERLOADED_GENERIC ||
        BaseReference->bilop == SX_EXTENSION_CALL)
    )
    {
        OverloadResolutionFlags OvrldFlags = OvrldIgnoreEvents | OvrldExactArgCount | OvrldIgnoreParamArray | OvrldIgnoreSubs | OvrldIgnoreProperties;

        OvrldFlags |= OvrldIgnoreSharedMembers;

        Result =
            BindArgsAndInterpretCallExpressionWithNoCopyOut(
                operatorCall->TextSpan,
                BaseReference,
                TypeCharacter,
                operatorCall->Arguments.Values,
                Flags,
                OvrldFlags | OvrldDontReportAddTypeParameterInTypeInferErrors,
                NULL);
    }

    if(!Result || IsBad(Result) )
    {
        AssertIfTrue(Result && IsBad(BaseReference));

        bool IsNameBangQualified;
        ParseTree::IdentifierDescriptor * operatorName = ExtractName(operatorCall->Target, IsNameBangQualified);

        ReportSemanticError(
            ERRID_QueryOperatorNotFound,
            BaseReference->Loc,
            operatorName ? operatorName->Name : L""
            );

        if(!Result)
        {
            Result = AllocateBadExpression(operatorCall->TextSpan);
        }
    }
    else
    {
        SetFlag32(Result, SXF_CALL_WAS_QUERY_OPERATOR);
    }

    VSASSERT(Result != NULL, "The expression was not interpreted correctly. This will crash later");

    return ApplyContextSpecificSemantics(Result, Flags, NULL);
}

#if IDE 
ILTree::Expression *
Semantics::InterpretFromItemWithContext
(
    ParseTree::FromItem * FromClause,
    ParseTree::IdentifierDescriptor * pControlVariableName,
    ParseTree::AlreadyBoundType ** pControlVariableType,
    ILTree::Expression *WithStatementContext
)
{
    if (WithStatementContext)
    {
        m_EnclosingWith = &AllocateStatement(SB_WITH, WithStatementContext->Loc, 0, false)->AsWithBlock();
        m_EnclosingWith->Loc = WithStatementContext->Loc;

        InterpretWithStatement(WithStatementContext, m_EnclosingWith);
    }

    return InterpretFromItem(FromClause, pControlVariableName, pControlVariableType);
}
#endif

Type *
Semantics::GetControlVariableType
(
    _In_ ParseTree::VariableDeclaration *ControlVariableDeclaration,
    _In_ Location & TextSpan,
    bool & TypeIsBad
)
{
    Type * TargetType = NULL;
    ParserHelper PH(&m_TreeStorage, TextSpan);

    if (ControlVariableDeclaration->Type)
    {
        ParseTree::Type * ControlVarType = ControlVariableDeclaration->Type;

        if (ControlVariableDeclaration->Variables->Element->Name.IsNullable)
        {
            if (ControlVariableDeclaration->Type->Opcode == ParseTree::Type::Nullable)
            {
                ReportSemanticError( ERRID_CantSpecifyNullableOnBoth, ControlVariableDeclaration->Variables->Element->Name.TextSpan );
                // Do not interrupt interpetation, interpret as if there was no second '?'
            }
            else if(ControlVariableDeclaration->Type->Opcode == ParseTree::Type::ArrayWithoutSizes ||
                    ControlVariableDeclaration->Type->Opcode == ParseTree::Type::ArrayWithSizes)
            {
                // Bug #90856 - DevDiv Bugs
                ReportSemanticError( ERRID_CantSpecifyArrayAndNullableOnBoth, ControlVariableDeclaration->TextSpan);
                TypeIsBad = true;
            }
            else
            {
                // make nullable type
                ControlVarType = PH.CreateNullableType(ControlVarType, ControlVarType->TextSpan);
            }
        }

        TargetType = InterpretTypeName(ControlVarType, TypeIsBad);
    }

    return TargetType;
}

ILTree::Expression *
Semantics::InterpretQueryControlVariableDeclaration
(
    _In_ ParseTree::VariableDeclaration *ControlVariableDeclaration,
    _In_ ParseTree::Expression * Source,
    bool IsLetControlVar,
    _In_ Location & TextSpan,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags
)
{
    if(pControlVariableName)
    {
        if(ControlVariableDeclaration &&
            ControlVariableDeclaration->Variables &&
            ControlVariableDeclaration->Variables->Element &&
            !ControlVariableDeclaration->Variables->Element->Name.IsBad &&
            ControlVariableDeclaration->Variables->Element->Name.Name)
        {
            *pControlVariableName = ControlVariableDeclaration->Variables->Element->Name;
            pControlVariableName->IsNullable = false;
        }
    }

    if(pControlVariableType)
    {
        *pControlVariableType = NULL;
    }

    if(pQueryFlags)
    {
        *pQueryFlags = QueryNoFlags;
    }

    if(!(ControlVariableDeclaration &&
          ControlVariableDeclaration->Variables &&
          ControlVariableDeclaration->Variables->Element &&
          !ControlVariableDeclaration->Variables->Element->Name.IsBad &&
          ControlVariableDeclaration->Variables->Element->Name.Name) ||
        !Source||
        Source->Opcode == ParseTree::Expression::SyntaxError)
    {
        return AllocateBadExpression(TextSpan);
    }

    // Microsoft:
    // See dev10 496875. When we call InterpretExpression, it binds a lambda for us.
    // Then when we call ConvertWithErrorChecking, it may relax it for us.
    // We don't what that for lambdas the user typed. So find the type first, if it
    // exists, and call InterpretExpressionWithTargetType.
    // We only want to do this in the case of let.

    Type *TargetType = NULL;
    Type * ControlVariableType=NULL;
    bool TypeIsBad = false;
    ParserHelper PH(&m_TreeStorage, TextSpan);
    ILTree::Expression * boundSource = NULL;

    if (!IsLetControlVar)
    {
        boundSource = InterpretExpression(Source,ExprForceRValue);
        if (IsBad(boundSource))
        {
             return boundSource;
        }
        TargetType = GetControlVariableType(ControlVariableDeclaration, TextSpan, TypeIsBad);

        if (TypeIsBad)
        {
            return AllocateBadExpression(ControlVariableDeclaration->Type->TextSpan);
        }
    }
    else
    {
        // For let variables, let's try to bind the target type first.
        // This is so that lets built like this:
        // Let x As Func(Of Integer) = Function() 10
        // get interpreted correctly (via InterpretExpressionWithTargetType);

        TargetType = GetControlVariableType(ControlVariableDeclaration, TextSpan, TypeIsBad);

        if (TypeIsBad)
        {
            return AllocateBadExpression(ControlVariableDeclaration->Type->TextSpan);
        }

        if (TargetType != NULL)
        {
            boundSource = InterpretExpressionWithTargetType(Source, ExprForceRValue, TargetType);
        }
        else
        {
            boundSource = InterpretExpression(Source,ExprForceRValue);
        }

        if (IsBad(boundSource))
        {
            return boundSource;
        }
    }

    // #26707
    if ((!TargetType || !IsLetControlVar) && boundSource != NULL && boundSource->ResultType == TypeHelpers::GetVoidType())
    {
        boundSource =  ConvertWithErrorChecking(boundSource, GetFXSymbolProvider()->GetObjectType(), ExprForceRValue);

        AssertIfFalse(IsBad(boundSource));

        if(IsBad(boundSource))
        {
            return boundSource;
        }
    }

    if(IsLetControlVar)
    {
        if(TargetType)
        {
            ControlVariableType = TargetType;
            // boundSource = ConvertWithErrorChecking(boundSource, TargetType, ExprForceRValue);
        }
        else
        {
            ControlVariableType = boundSource->ResultType;
        }

        if(pControlVariableType)
        {
            *pControlVariableType = PH.CreateBoundType(ControlVariableType, Source->TextSpan); // not likely to get here
        }

        return boundSource;
    }

    QueryExpressionFlags queryFlags = pQueryFlags ? (*pQueryFlags) : QueryNoFlags;
    ILTree::Expression * queryableSource = ToQueryableSource(boundSource,&ControlVariableType, &queryFlags);
    if(pQueryFlags) *pQueryFlags = queryFlags;


    if(queryableSource && IsBad(queryableSource))
    {
        return queryableSource;
    }

    AssertIfFalse(ControlVariableType || !queryableSource);

    if(!queryableSource)
    {
        ControlVariableType = NULL;
        queryableSource = boundSource;
    }

    // The Cast(Of Object) is now applied by ToQueryableSource().
    // If we don't have control variable type at this point, nothing can help us.
    if(!ControlVariableType)
    {
        ReportSemanticError(ERRID_ExpectedQueryableSource, boundSource->Loc, boundSource->ResultType);
        return AllocateBadExpression(boundSource->Loc);
    }

    if (TargetType)
    {
        AssertIfFalse(ControlVariableType);

        if(!TypeHelpers::EquivalentTypes(TargetType,ControlVariableType))
        {
            Location asTypeLocation;
            asTypeLocation.SetLocation(ControlVariableDeclaration->TextSpan.m_lBegLine, &ControlVariableDeclaration->As, 2);
            asTypeLocation.SetEnd(&ControlVariableDeclaration->TextSpan);

            // Need to apply Select for the conversion
            ParseTree::IdentifierDescriptor ControlVariableName = ControlVariableDeclaration->Variables->Element->Name;
            ControlVariableName.IsNullable = false;

            Location varAsTypeLocation = asTypeLocation;
            varAsTypeLocation.SetStart(&ControlVariableName.TextSpan);

            ILTree::LambdaExpression * boundSelector = BindSelectSelector(
                                    PH.CreateNameExpression(ControlVariableName.Name, varAsTypeLocation),
                                    ControlVariableName,
                                    PH.CreateBoundType(ControlVariableType),
                                    QueryNoFlags, TargetType);

            if(IsBad(boundSelector))
            {
                return boundSelector;
            }

            queryableSource = BindSelectOperator(queryableSource, TextSpan, asTypeLocation, boundSelector);
            ControlVariableType = TargetType;

            if(pQueryFlags)
            {
                *pQueryFlags |= AppliedAtLeastOneOperator;
            }
        }

        if(pControlVariableType)
        {
            *pControlVariableType = PH.CreateBoundType(ControlVariableType, ControlVariableDeclaration->Type->TextSpan);
        }
    }
    else
    {   // else TargetType was null, i.e. we didn't find a type for it.
        if(pControlVariableType)
        {
            *pControlVariableType = PH.CreateBoundType(ControlVariableType, Source->TextSpan);
        }
    }

    return queryableSource;
}

ILTree::Expression *
Semantics::InterpretFromItem
(
    _In_ ParseTree::FromItem * FromClause,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags
)
{
    ILTree::Expression * Source =
        InterpretQueryControlVariableDeclaration
        (
            FromClause->ControlVariableDeclaration,
            FromClause->Source,
            FromClause->IsLetControlVar,
            FromClause->TextSpan,
            pControlVariableName,
            pControlVariableType,
            pQueryFlags
        );

#if IDE 
    if (m_FromItemsMap && pControlVariableType && *pControlVariableType)
    {
        m_FromItemsMap->Add(FromClause, *pControlVariableType);
    }
#endif

    return Source;
}


bool
Semantics::CheckControlVariableDeclaration
(
    _In_ ParseTree::VariableDeclaration *ControlVariableDeclaration
)
{
    if (m_Errors && ControlVariableDeclaration &&
        ControlVariableDeclaration->Variables &&
        ControlVariableDeclaration->Variables->Element
        )
    {
        return CheckControlVariableIdentifier(ControlVariableDeclaration->Variables->Element->Name);
    }

    return true;
}


bool
Semantics::CheckControlVariableIdentifier
(
    _In_ ParseTree::IdentifierDescriptor & Name
)
{
    if (!Name.IsBad &&
        Name.Name
        )
    {
        Location  location = Name.TextSpan;
        Identifier * name = Name.Name;

        // Bug 40202 - DevDiv Bugs
        if (Name.TypeCharacter != chType_NONE)
        {
            ReportSemanticError(
                ERRID_QueryAnonymousTypeDisallowsTypeChar,
                location);

            return false; // Bug 44597 - DevDiv Bugs
        }

        return CheckControlVariableName(name, location);
    }

    return false;
}

bool
Semantics::CheckControlVariableName
(
    _In_opt_z_ Identifier * name,
    Location  & location,
    bool CheckForObjectMemberNameCollision /* = true */
)
{
    AssertIfNull(name);

    if(name)
    {
        if(CheckForObjectMemberNameCollision)
        {
            Declaration *Member = GetFXSymbolProvider()->GetObjectType()->PContainer()->GetHash()->SimpleBind(name);
            if (Member)
            {
                ReportSemanticError(ERRID_QueryInvalidControlVariableName1, location);
                return false;
            }
        }

        // Bug 42478 - DevDiv Bugs
        if (m_Procedure && m_Procedure->GetGenericParamsHash() &&
             m_Procedure->GetGenericParamsHash()->SimpleBind( name ))
        {
            ReportSemanticError( ERRID_NameSameAsMethodTypeParam1, location, name );
            return false;
        }

        // if we are building keys for a Join, we add extra variables into the scope and don't want interpretation to fail just because of that
        if(!m_JoinKeyBuilderList
            && name != STRING_CONST(m_Compiler, Group2)) // don't check compiler generated name
        {
            if (!CheckNameForShadowingOfLocals(
                    name,
                    location,
                // (Bug 44887 - DevDiv Bugs)
                    m_CreateImplicitDeclarations ? ERRID_IterationVariableShadowLocal2 : ERRID_IterationVariableShadowLocal1,
                    true /*DeferForImplicitDeclarations*/))
            {
                return false;
            }
        }
    }

    return true;
}


bool
Semantics::CheckNameForShadowingOfLocals
(
    _In_ Identifier* Name,
    Location&   Location,
    ERRID       ErrorId,
    bool        DeferForImplicitDeclarations
)
{
    // It is possible to not have an m_BlockContext when we reach this function.  While this 
    // should not happen in the core compiler, it's a possible scenario in the IDE, EE and 
    // HOSTED.  
    //
    // See DevDiv 653950 for an EE scenario where m_BlockContext will be NULL

    Symbol *NameBinding = NULL;
    ILTree::Expression * Result =
        AttemptQueryNameLookup(
            Location,
            Name,
            chType_NONE,
            NameSearchLocalsOnly,
            ExprNoFlags,
            -1,
            &NameBinding,
            false);  // #25115 do not Check Use Of Local Before Declaration

    if (NameBinding ||Result)
    {
        if (m_ReportErrors && m_Errors)
        {
            if (!m_Errors->HasThisErrorWithLocation(ErrorId, Location))
            {
                ReportSemanticError(ErrorId, Location, Name);
            }
        }

        return false;
    }
    else if (DeferForImplicitDeclarations && m_CreateImplicitDeclarations)
    {
        if (m_BlockContext)
        {
            TrackingVariablesForImplicitCollision * newTrackedVar = new(m_TreeAllocator) TrackingVariablesForImplicitCollision(
                Name,
                Location,
                ErrorId);

            m_BlockContext->TrackingVarsForImplicit.InsertFirst(newTrackedVar);
        }
    }
        
    return true;
}

Semantics::LambdaBodyInterpretJoinedFromItem::LambdaBodyInterpretJoinedFromItem
(
    _In_ Semantics * semantics,
    _In_ ParseTree::FromItem *FromItem,
    ParseTree::NewObjectInitializerExpression * ItInitializer
)
{
    AssertIfNull(semantics);
    AssertIfNull(FromItem);
    AssertIfNull(ItInitializer);

    m_Semantics = semantics;
    m_FromItem = FromItem;
    m_ControlVariableType = NULL;
    m_ItInitializer = ItInitializer;
}

ILTree::ILNode *
Semantics::LambdaBodyInterpretJoinedFromItem::DoInterpretBody()
{
    return m_Semantics->InterpretJoinedFromItem(m_FromItem, m_ItInitializer, &m_AddedName, &m_ControlVariableType);
}

ParseTree::AlreadyBoundType *
Semantics::LambdaBodyInterpretJoinedFromItem::GetControlVariableType()
{
    return m_ControlVariableType;
}

ParseTree::IdentifierDescriptor
Semantics::LambdaBodyInterpretJoinedFromItem::GetAddedName()
{
    return m_AddedName;
}

Semantics::LambdaBodyInterpretJoinedLetItem::LambdaBodyInterpretJoinedLetItem
(
    _In_ Semantics * semantics,
    _In_ ParseTree::FromItem *FromItem,
    _In_ ParseTree::NewObjectInitializerExpression * ItInitializer
)
{
    AssertIfNull(semantics);
    AssertIfNull(FromItem);
    AssertIfTrue(FromItem && !FromItem->IsLetControlVar);

    m_Semantics = semantics;
    m_FromItem = FromItem;
    m_ItInitializer = ItInitializer;
    m_AddedInitializer = NULL;
}

ILTree::ILNode *
Semantics::LambdaBodyInterpretJoinedLetItem::DoInterpretBody()
{
    ParseTree::VariableDeclaration *ControlVariableDeclaration = m_FromItem->ControlVariableDeclaration;

    ILTree::Expression * letSource = m_Semantics->InterpretFromItem(m_FromItem, &m_AddedName);

    // #27975 Do this after InterpretFromItem() because it may declare new locals for the Proc
    bool nameIsGood = m_FromItem->SuppressShadowingChecks ?
                                    true :
                                    m_Semantics->CheckControlVariableDeclaration(ControlVariableDeclaration);

    if (IsBad(letSource))
    {
        return letSource;
    }
    else if(!nameIsGood)
    {
        return m_Semantics->AllocateBadExpression(m_FromItem->TextSpan);
    }

    if(m_ItInitializer)
    {
        ParserHelper PH(&m_Semantics->m_TreeStorage, m_FromItem->TextSpan);
        m_AddedInitializer = PH.AddAssignmentInitializer(m_ItInitializer, &m_AddedName, PH.CreateBoundExpression(letSource));
        m_ItInitializer->TextSpan.SetEnd(&m_FromItem->TextSpan);
        return m_Semantics->InterpretExpression(m_ItInitializer, ExprForceRValue);
    }

    return letSource;
}

ParseTree::InitializerList *
Semantics::LambdaBodyInterpretJoinedLetItem::GetAddedInitializer()
{
    return m_AddedInitializer;
}

ParseTree::IdentifierDescriptor
Semantics::LambdaBodyInterpretJoinedLetItem::GetAddedName()
{
    return m_AddedName;
}


ParseTree::FromList *
Semantics::GetNextItemInFromList
(
    _In_opt_ ParseTree::FromList *FromItems,
    _Inout_ ParseTree::LinqExpression ** CrossJoinTo,
    _Out_ Location & CommaOrFromOrErrorTextSpan,
    _Out_ bool & AnError
)
{
    AnError = false;

    if(FromItems && FromItems->Next)
    {
        CommaOrFromOrErrorTextSpan.SetLocation(FromItems->TextSpan.m_lBegLine, &FromItems->Punctuator,0);
        return FromItems->Next;
    }

    if(*CrossJoinTo)
    {
        ParseTree::FromExpression * from = NULL;

        AssertIfFalse((*CrossJoinTo)->Opcode == ParseTree::Expression::From ||
                                (*CrossJoinTo)->Opcode == ParseTree::Expression::Let ||
                                (*CrossJoinTo)->Opcode == ParseTree::Expression::CrossJoin);


        switch((*CrossJoinTo)->Opcode)
        {
            case ParseTree::Expression::From:
            case ParseTree::Expression::Let:
                from = (*CrossJoinTo)->AsFrom();
                *CrossJoinTo = NULL;
                break;

            case ParseTree::Expression::CrossJoin:
                {
                    ParseTree::CrossJoinExpression * crossJoin = (*CrossJoinTo)->AsCrossJoin();

                    AssertIfTrue(crossJoin->Source->Opcode != ParseTree::Expression::From &&
                                          crossJoin->Source->Opcode != ParseTree::Expression::Let);

                    if(crossJoin->Source->Opcode == ParseTree::Expression::From ||
                        crossJoin->Source->Opcode == ParseTree::Expression::Let)
                    {
                        from = crossJoin->Source->AsFrom();
                        *CrossJoinTo = crossJoin->JoinTo;
                    }
                    else
                    {
                        // wrong tree configuration
                        CommaOrFromOrErrorTextSpan = crossJoin->Source->TextSpan;
                    }
                }
                break;

            default:
                // wrong tree configuration
                CommaOrFromOrErrorTextSpan = (*CrossJoinTo)->TextSpan;
                break;

        }

        if(from)
        {
            FromItems = from->FromItems;

            if(from->ImplicitLetforAggregate)
            {
                CommaOrFromOrErrorTextSpan.SetLocation(from->TextSpan.m_lBegLine, from->TextSpan.m_lBegColumn,
                                                            from->TextSpan.m_lBegLine, from->TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkAGGREGATE]-1);
            }
            else if(from->Opcode == ParseTree::Expression::Let)
            {
                CommaOrFromOrErrorTextSpan.SetLocation(from->TextSpan.m_lBegLine, from->TextSpan.m_lBegColumn,
                                                            from->TextSpan.m_lBegLine, from->TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkLET]-1);
            }
            else
            {
                AssertIfFalse(from->Opcode == ParseTree::Expression::From);
                CommaOrFromOrErrorTextSpan.SetLocation(from->TextSpan.m_lBegLine, from->TextSpan.m_lBegColumn,
                                                            from->TextSpan.m_lBegLine, from->TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkFROM]-1);
            }
        }
        else
        {
            // shouldn't get here
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                CommaOrFromOrErrorTextSpan);

            AnError = true;
            FromItems = NULL;
            *CrossJoinTo = NULL;
        }

        return FromItems;
    }

    return NULL;
}

ILTree::Expression *
Semantics::InterpretJoinedFromItem
(
    _In_ ParseTree::FromItem *FromItem,
    ParseTree::NewObjectInitializerExpression * ItInitializer,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType
)
{
    AssertIfNull(ItInitializer);
    AssertIfNull(FromItem);

    AssertIfTrue(FromItem->IsLetControlVar);
    if(FromItem->IsLetControlVar)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            FromItem->TextSpan);
        return AllocateBadExpression(FromItem->TextSpan);
    }

    ParserHelper PH(&m_TreeStorage, FromItem->TextSpan);
    ParseTree::VariableDeclaration *ControlVariableDeclaration = FromItem->ControlVariableDeclaration;

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType * ControlVariableType = NULL;

    ILTree::Expression * Source = InterpretFromItem(FromItem, &ControlVariableName, &ControlVariableType);

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    // #27975 Do this after InterpretFromItem() because it may declare new locals for the Proc
    bool nameIsGood = CheckControlVariableDeclaration(ControlVariableDeclaration);

    if (IsBad(Source))
    {
        return Source;
    }
    else if(!nameIsGood)
    {
        return AllocateBadExpression(FromItem->TextSpan);
    }

    PH.AddExpressionInitializer(ItInitializer, PH.CreateNameExpression(ControlVariableName.Name,ControlVariableName.TextSpan));

    return Source;
}


ILTree::Expression *
Semantics::BuildJoinedFromClauses
(
    _In_opt_ ParseTree::FromList *FromItems,
    ParseTree::LinqExpression * CrossJoinTo,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _Out_opt_ ParseTree::IdentifierDescriptor *pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    Location TextSpan
)
{
    AssertIfNull(ItInitializer);
    AssertIfNull(FromItems);
    AssertIfTrue(CrossJoinTo &&
                            CrossJoinTo->Opcode!=ParseTree::Expression::From &&
                            CrossJoinTo->Opcode!=ParseTree::Expression::Let &&
                            CrossJoinTo->Opcode!=ParseTree::Expression::CrossJoin);

    ILTree::Expression *Result = NULL;
    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType * ControlVariableType = NULL;
    ILTree::Expression * Source = InterpretJoinedFromItem(FromItems->Element,ItInitializer, &ControlVariableName,&ControlVariableType);

    if (IsBad(Source))
    {
        return Source;
    }

    Location CommaOrFromOrErrorTextSpan;
    bool AnError=false;
    ParseTree::FromList *nextFromItems = GetNextItemInFromList(FromItems, &CrossJoinTo, CommaOrFromOrErrorTextSpan, AnError);

    if(AnError)
    {
        return AllocateBadExpression(CommaOrFromOrErrorTextSpan);
    }

    if(nextFromItems)
    {
        Result = InterpretCrossJoin
                                (
                                    Source,
                                    ControlVariableName,
                                    ControlVariableType,
                                    QueryNoFlags,
                                    CommaOrFromOrErrorTextSpan,
                                    nextFromItems,
                                    CrossJoinTo,
                                    ItInitializer,
                                    pControlVariableName,
                                    pControlVariableType,
                                    pQueryFlags,
                                    pFollowingOperator,
                                    TextSpan
                                );

        AssertIfFalse(!pQueryFlags || IsBad(Result) || HasFlag(*pQueryFlags, AppliedAtLeastOneOperator));

        return Result;
    }

    AssertIfTrue(ERRID_InternalCompilerError);
    ReportSemanticError(
        ERRID_InternalCompilerError,
        FromItems->TextSpan);
    return AllocateBadExpression(FromItems->TextSpan);
}


static void
CleanupQueryOutputPrameters
(
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _Out_opt_ ParseTree::AlreadyBoundType ** pGroupElementType = NULL
)
{
    if(pControlVariableType)
    {
        *pControlVariableType = NULL;
    }

    if(pControlVariableName)
    {
        pControlVariableName->Name = NULL;
    }

    if(pQueryFlags)
    {
        *pQueryFlags = QueryNoFlags;
    }

    if(pRecordInitializer)
    {
        *pRecordInitializer = NULL;
    }

    if (pGroupElementType)
    {
        *pGroupElementType = NULL;
    }
}


ILTree::Expression *
Semantics::InterpretInnerJoinExpression
(
    _In_ ParseTree::InnerJoinExpression *InnerJoin,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;
    ParseTree::NewObjectInitializerExpression * ItInitializer=NULL;

    ILTree::Expression * Result =
        InterpretInnerJoinExpression
        (
            InnerJoin,
            Flags,
            NULL, //FollowingLet,
            true, //lastItemToJoin,
            ItInitializer,
            ControlVariableName,
            ControlVariableType,
            QueryFlags,
            pFollowingOperator
        );

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    if(pQueryFlags)
    {
        *pQueryFlags=QueryFlags;
    }

    if(pRecordInitializer)
    {
        *pRecordInitializer = ItInitializer;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretInnerJoinExpression
(
    _In_ ParseTree::InnerJoinExpression *InnerJoin,
    ExpressionFlags Flags,
    _In_opt_ ParseTree::FromItem *FollowingLet,
    bool lastItemToJoin,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _Out_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Deref_out_ ParseTree::AlreadyBoundType *& ControlVariableType,
    _Out_ QueryExpressionFlags & QueryFlags,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator
)
{
    ParseTree::IdentifierDescriptor ControlVariableName1;
    ParseTree::AlreadyBoundType *ControlVariableType1= NULL;
    QueryExpressionFlags QueryFlags1=QueryNoFlags;

    ParseTree::LinqOperatorExpression * op = InnerJoin;
    ParseTree::NewObjectInitializerExpression * ItInitializer1 = NULL;

    // Interpret the first source
    ILTree::Expression *Source1 = InterpretJoinOperatorSource(InnerJoin->Source, Flags, ItInitializer1, ControlVariableName1, ControlVariableType1, QueryFlags1, &op);

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            InnerJoin->Source->TextSpan); // shouldn't get here
        return AllocateBadExpression(InnerJoin->Source->TextSpan); // shouldn't get here
    }

    if(!InnerJoin->JoinTo)
    {
        return AllocateBadExpression(InnerJoin->TextSpan); // shouldn't get here, parser never leaves this NULL
    }

    Location joinTextSpan = InnerJoin->TextSpan;
    ParserHelper PH(&m_TreeStorage, joinTextSpan);

    if(!ItInitializer1)
    {
        AssertIfTrue(!IsBad(Source1) && HasFlag(QueryFlags1, ControlVarIsRecord));

        ItInitializer1 = PH.CreateAnonymousTypeInitializer(NULL, joinTextSpan, true, true);

        if(!HasFlag(QueryFlags1, ControlVarIsHidden))
        {
            PH.AddExpressionInitializer(ItInitializer1, PH.CreateNameExpression(ControlVariableName1.Name,ControlVariableName1.TextSpan));
        }
    }
    else if(!IsBad(Source1))
    {
        AssertIfFalse(HasFlag(QueryFlags1, ControlVarIsRecord));
        AssertIfFalse(HasFlag(QueryFlags1, ControlVarIsHidden));
    }

    ParseTree::InitializerList * lastLeftInitializer;

    for(lastLeftInitializer = ItInitializer1->InitialValues->InitialValues; lastLeftInitializer && lastLeftInitializer->Next;  lastLeftInitializer = lastLeftInitializer->Next)
    {
    }


    ParseTree::IdentifierDescriptor ControlVariableName2;
    ParseTree::AlreadyBoundType *ControlVariableType2 = NULL;
    QueryExpressionFlags QueryFlags2=QueryNoFlags;

    // Interpret the second source
    ILTree::Expression *Source2 = InterpretJoinOperatorSource(InnerJoin->JoinTo, Flags, ItInitializer1, ControlVariableName2, ControlVariableType2, QueryFlags2, &op);

    if(IsBad(Source1))
    {
        return Source1;
    }

    if(IsBad(Source2))
    {
        return Source2;
    }

    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            InnerJoin->JoinTo->TextSpan); // shouldn't get here
        return AllocateBadExpression(InnerJoin->JoinTo->TextSpan); // shouldn't get here
    }

    // check for duplicate control variables on the right
    bool DuplicateVars = false;

    if(lastLeftInitializer)
    {
        ParseTree::InitializerList * firstRightInitializer = lastLeftInitializer->Next;

        DuplicateVars = !JoinCheckDuplicateControlVars
                                    (
                                        ItInitializer1->InitialValues->InitialValues,
                                        firstRightInitializer,
                                        NULL,
                                        NULL,
                                        firstRightInitializer
                                    );
    }
    else
    {
        // Parse tree is built to make sure that control var has a name
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            InnerJoin->Source->TextSpan); // shouldn't get here
        return AllocateBadExpression(InnerJoin->Source->TextSpan); // shouldn't get here
        // shouldn't get here
    }

    if(DuplicateVars &&
        !HasFlag(QueryFlags1, ControlVarIsRecord) &&
        !HasFlag(QueryFlags2, ControlVarIsRecord))
    {
        return AllocateBadExpression(InnerJoin->JoinTo->TextSpan);
    }


    // interpret keys
    ILTree::LambdaExpression * boundKey1 = NULL;
    ILTree::LambdaExpression * boundKey2 = NULL;

    InterpretJoinKeys
    (
        ControlVariableName1,
        ControlVariableType1,
        QueryFlags1,
        ItInitializer1->InitialValues->InitialValues,
        ControlVariableName2,
        ControlVariableType2,
        QueryFlags2,
        lastLeftInitializer->Next,
        InnerJoin->Predicate,
        Flags,
        boundKey1,
        boundKey2
    );

    if(DuplicateVars)
    {
        return AllocateBadExpression(InnerJoin->JoinTo->TextSpan);
    }

    // interpret result selector

    ControlVariableName = ControlVariableName1;
    ControlVariableType = ControlVariableType1;
    QueryFlags = QueryFlags1;

    // Build selector that joins two control variables
    ILTree::Expression * selector =
                InterpretJoinSelector
                (
                    ControlVariableName,
                    ControlVariableType,
                    QueryFlags,
                    ControlVariableName2,
                    ControlVariableType2,
                    QueryFlags2,
                    ItInitializer1,
                    FollowingLet,
                    lastItemToJoin,
                    pFollowingOperator,
                    joinTextSpan
                );

    if(IsBad(selector))
    {
        return selector;
    }
    else if(selector->bilop != SX_LAMBDA)
    {
        AssertIfFalse(selector->bilop == SX_LAMBDA);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            joinTextSpan); // shouldn't get here
        return AllocateBadExpression(joinTextSpan); // shouldn't get here
    }

    ILTree::LambdaExpression * boundSelector = NULL;

    boundSelector = &selector->AsLambdaExpression();

    // Interpret the Join call
    if(!boundKey1 || !boundKey2 || IsBad(boundKey1) || IsBad(boundKey2))
    {
        return AllocateBadExpression(joinTextSpan);
    }

    QueryFlags |= AppliedAtLeastOneOperator;

    Location joinNameLocation;
    joinNameLocation.SetLocation(InnerJoin->TextSpan.m_lBegLine, &InnerJoin->FirstPunctuator,g_tkKwdNameLengths[tkJOIN]-1);

    ILTree::Expression *
    Result =
        InterpretQueryOperatorCall(
            PH.CreateMethodCall(
                        PH.CreateQualifiedExpression(
                                    PH.CreateBoundExpression(Source1),
                                    PH.CreateNameExpression(STRING_CONST(m_Compiler, JoinMethod), joinNameLocation),
                                    joinNameLocation),
                        PH.CreateArgList(
                                    4,
                                    PH.CreateBoundExpression(Source2),
                                    PH.CreateBoundExpression(boundKey1),
                                    PH.CreateBoundExpression(boundKey2),
                                    PH.CreateBoundExpression(boundSelector)),
                        joinTextSpan),
            ExprForceRValue);

    if(!ItInitializer)
    {
        ItInitializer = ItInitializer1;
    }
    else
    {
        PH.AppendInitializerList( ItInitializer, ItInitializer1->InitialValues->InitialValues);
        ItInitializer->TextSpan.SetEnd(&ItInitializer1->TextSpan);
    }

    return Result;
}


void
Semantics::InterpretJoinKeys
(
    _In_ ParseTree::IdentifierDescriptor & ControlVariableName1,
    ParseTree::AlreadyBoundType *ControlVariableType1,
    QueryExpressionFlags QueryFlags1,
    _In_ ParseTree::InitializerList * InitializerList1,
    _In_ ParseTree::IdentifierDescriptor & ControlVariableName2,
    ParseTree::AlreadyBoundType *ControlVariableType2,
    QueryExpressionFlags QueryFlags2,
    _In_ ParseTree::InitializerList * InitializerList2,
    _In_opt_ ParseTree::Expression *Predicate,
    ExpressionFlags Flags,
    _Out_ ILTree::LambdaExpression * &boundKey1,
    _Out_ ILTree::LambdaExpression * &boundKey2
)
{
    boundKey1 = NULL;
    boundKey2 = NULL;

    if(Predicate)
    {
        ParserHelper PH(&m_TreeStorage, Predicate->TextSpan);

        // create fake Lambda that we will use to determine which side the key belongs to
        ParseTree::ParameterList * lambdaParams;

        ParseTree::IdentifierDescriptor Var1 = ControlVariableName1;
        ParseTree::IdentifierDescriptor Var2 = ControlVariableName2;

        if(HasFlag(QueryFlags1, ControlVarIsRecord))
        {
            Var1 = Join1IdentifierDescriptor();
        }

        if(HasFlag(QueryFlags2, ControlVarIsRecord))
        {
            Var2 = Join2IdentifierDescriptor();
        }

        lambdaParams = PH.CreateParameterList(
                                    Predicate->TextSpan,
                                    2,
                                    PH.CreateParameter(
                                        Var1,
                                        ControlVariableType1, true,
                                        HasFlag(QueryFlags1,ControlVarIsRecord)),
                                    PH.CreateParameter(
                                        Var2,
                                        ControlVariableType2, true,
                                        HasFlag(QueryFlags2,ControlVarIsRecord))
                                        );

        LambdaBodyBuildKeyExpressions KeyBuilder(this, Predicate, Flags, Var1.Name,  Var2.Name, InitializerList1, InitializerList2);

        // don't care about the return value here
        CreateLambdaExpression(
                    InterpretLambdaParameters(lambdaParams),
                    Predicate->TextSpan,
                    Location::GetInvalidLocation(), // BodySpan, only relevant for multiline lambdas
                    TypeHelpers::GetVoidType(),
                    &KeyBuilder,
                    false, // IsFunctionLambda
                    false, // IsAsyncKeywordUsed
                    false, // IsIteratorKeywordUsed
                    SingleLineLambda);

        if(KeyBuilder.m_Key1)
        {
            boundKey1 = Semantics::BindSelectSelector
                                (
                                    KeyBuilder.m_Key1,
                                    ControlVariableName1,
                                    ControlVariableType1,
                                    QueryFlags1
                                );

            boundKey1->ExactlyMatchExpressionResultType = false;
        }

        if(KeyBuilder.m_Key2)
        {
            boundKey2 = Semantics::BindSelectSelector
                                (
                                    KeyBuilder.m_Key2,
                                    ControlVariableName2,
                                    ControlVariableType2,
                                    QueryFlags2
                                );

            boundKey2->ExactlyMatchExpressionResultType = false;

            if(!KeyBuilder.m_KeysAreInvalid && boundKey1 && !IsBad(boundKey1) && !IsBad(boundKey2) &&
                !TypeHelpers::EquivalentTypes(boundKey1->GetExpressionLambdaBody()->ResultType, boundKey2->GetExpressionLambdaBody()->ResultType) &&
                !(((boundKey1->GetExpressionLambdaBody()->ResultType->IsAnonymousType() &&
                            boundKey2->GetExpressionLambdaBody()->ResultType->IsAnonymousType()) ||
                   (boundKey1->GetExpressionLambdaBody()->ResultType->IsAnonymousDelegate()&&
                            boundKey2->GetExpressionLambdaBody()->ResultType->IsAnonymousDelegate() &&
                            BCSYM::AreTypesEqual(boundKey1->GetExpressionLambdaBody()->ResultType, boundKey2->GetExpressionLambdaBody()->ResultType)))&& //Check for anonymous delegates added for Dev10 bug 474653                    
                   (!m_SymbolsCreatedDuringInterpretation || !m_MergeAnonymousTypeTemplates)))
            {
                AssertIfTrue(ERRID_InternalCompilerError);
                ReportSemanticError(ERRID_EqualsTypeMismatch, Predicate->TextSpan,
                    boundKey1->GetExpressionLambdaBody()->ResultType,boundKey2->GetExpressionLambdaBody()->ResultType); // shouldn't get here
                boundKey1 = NULL;    // shouldn't get here
                boundKey2 = NULL;    // shouldn't get here
            }
        }

        if(KeyBuilder.m_KeysAreInvalid)
        {
            boundKey1 = NULL;
            boundKey2 = NULL;
        }
    }
}


ILTree::Expression *
Semantics::InterpretJoinOperatorSource
(
    _In_ ParseTree::LinqExpression *Source,
    ExpressionFlags Flags,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _Out_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Deref_out_ ParseTree::AlreadyBoundType *& ControlVariableType,
    _Out_ QueryExpressionFlags & QueryFlags,
    _Inout_opt_ ParseTree::LinqOperatorExpression **pFollowingOperator
)
{
    AssertIfFalse(!pFollowingOperator ||
                            (*pFollowingOperator &&
                                ((*pFollowingOperator)->Opcode == ParseTree::Expression::InnerJoin ||
                                (*pFollowingOperator)->Opcode == ParseTree::Expression::GroupJoin)));

    ControlVariableType = NULL;

    bool lastItemToJoin = !(pFollowingOperator && *pFollowingOperator &&
                                        ((*pFollowingOperator)->Opcode == ParseTree::Expression::InnerJoin ||
                                        (*pFollowingOperator)->Opcode == ParseTree::Expression::GroupJoin));

    switch(Source->Opcode)
    {
        case ParseTree::Expression::LinqSource:
            {
                ParseTree::LinqSourceExpression * linqSource = Source->AsLinqSource();
                ILTree::Expression * Result =
                    InterpretQueryControlVariableDeclaration
                    (
                        linqSource->ControlVariableDeclaration,
                        linqSource->Source,
                        false, //IsLetControlVar
                        linqSource->TextSpan,
                        &ControlVariableName,
                        &ControlVariableType,
                        &QueryFlags
                    );

                // #27975 Do this after InterpretFromItem() because it may declare new locals for the Proc
                if(!CheckControlVariableDeclaration(linqSource->ControlVariableDeclaration))
                {
                    MakeBad(Result);
                    return Result;
                }

                if(ItInitializer && !IsBad(Result))
                {
                    ParserHelper PH(&m_TreeStorage, Source->TextSpan);
                    PH.AddExpressionInitializer(ItInitializer, PH.CreateNameExpression(ControlVariableName.Name,ControlVariableName.TextSpan));
                }

                return Result;
            }

        case ParseTree::Expression::InnerJoin:
            return
                    InterpretInnerJoinExpression
                    (
                        Source->AsInnerJoin(),
                        Flags,
                        NULL, //FollowingLet,
                        lastItemToJoin, // lastItemToJoin,
                        ItInitializer,
                        ControlVariableName,
                        ControlVariableType,
                        QueryFlags,
                        pFollowingOperator
                    );

        case ParseTree::Expression::GroupJoin:
            return
                    InterpretGroupJoinExpression
                    (
                        Source->AsGroupJoin(),
                        Flags,
                        NULL, //FollowingLet,
                        lastItemToJoin, // lastItemToJoin,
                        ItInitializer,
                        ControlVariableName,
                        ControlVariableType,
                        QueryFlags,
                        pFollowingOperator
                    );
    }

    AssertIfTrue(ItInitializer);

    if(ItInitializer)
    {
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Source->TextSpan); // shouldn't get here
        return AllocateBadExpression(Source->TextSpan); // shouldn't get here
    }

    return InterpretLinqOperatorSource(Source, Flags, pFollowingOperator, &ControlVariableName, &ControlVariableType, &QueryFlags, &ItInitializer);
}


Semantics::LambdaBodyBuildKeyExpressions::LambdaBodyBuildKeyExpressions
(
    _In_ Semantics * semantics,
    _In_ ParseTree::Expression * Predicate,
    ExpressionFlags Flags,
    _In_z_ Identifier *ControlVarName1,
    _In_z_ Identifier *ControlVarName2,
    _In_ ParseTree::InitializerList * InitializerList1,
    _In_ ParseTree::InitializerList * InitializerList2
)
{
    AssertIfNull(semantics);
    AssertIfNull(Predicate);
    AssertIfNull(ControlVarName1);
    AssertIfNull(ControlVarName2);

    m_Semantics = semantics;
    m_Predicate = Predicate;
    m_ControlVarName1 = ControlVarName1;
    m_ControlVarName2 = ControlVarName2;
    m_InitializerList1 = InitializerList1;
    m_InitializerList2 = InitializerList2;
    m_Flags = Flags;
    m_Errors = NULL;

    m_Var1 = NULL;
    m_Var2 = NULL;

    m_NameBoundToControlVar = NULL;
    m_BoundTo = NotBound;
    m_ThereWasABadNameReference = false;
    m_ThereWasANameReference = false;

    m_KeySegmentCount = 0;
    m_Prev = NULL;

    m_Key1 = NULL;
    m_Key2 = NULL;

    m_KeysAreInvalid = false;
}

ILTree::ILNode *
Semantics::LambdaBodyBuildKeyExpressions::DoInterpretBody()
{
    // fix up the list
    BackupValue<LambdaBodyBuildKeyExpressions *> backup_m_JoinKeyBuilderList(&m_Semantics->m_JoinKeyBuilderList);
    BackupValue<LambdaBodyBuildKeyExpressions *> backup_m_Prev(&m_Prev);
    m_Prev = m_Semantics->m_JoinKeyBuilderList;
    m_Semantics->m_JoinKeyBuilderList = this;

    // Get our control vars
    m_Var1 = m_Semantics->m_Lookup->SimpleBind(m_ControlVarName1);
    m_Var2 = m_Semantics->m_Lookup->SimpleBind(m_ControlVarName2);

    if(m_Semantics->m_Errors)
    {
        m_Errors = new ErrorTable(*m_Semantics->m_Errors);
    }

    BuildKey(m_Predicate);

    if(m_Errors)
    {
        if(m_Semantics->m_Errors && m_Semantics->m_ReportErrors && m_Errors->HasErrors())
        {
            m_Semantics->m_Errors->MergeTemporaryTable(m_Errors);
        }

        delete m_Errors;
    }

    Location invalid;
    invalid.Invalidate();

    // No one is interested in the result
    return m_Semantics->AllocateBadExpression(invalid);
}


void
Semantics::LambdaBodyBuildKeyExpressions::BuildKey
(
    _In_ ParseTree::Expression * Predicate
)
{
    ParseTree::BinaryExpression * binary;

    switch(Predicate->Opcode)
    {
        case ParseTree::Expression::Equals:
            {
                BackupValue<bool> backup_m_ReportErrors(&m_Semantics->m_ReportErrors);
                BackupValue<ErrorTable *> backup_m_Errors(&m_Semantics->m_Errors);

                ErrorTable * ignoreErrors = NULL;

                if(m_Semantics->m_Errors)
                {
                    ignoreErrors = new ErrorTable(*m_Semantics->m_Errors);
                    m_Semantics->m_Errors = ignoreErrors;
                }

                m_Semantics->m_ReportErrors = false;

                binary = Predicate->AsBinary();
                ILTree::Expression * Left = NULL;
                ILTree::Expression * Right = NULL;
                BoundState LeftIsBoundTo = NotBound;
                BoundState RightIsBoundTo = NotBound;
                bool LeftBadNameRef = false;
                bool RightBadNameRef = false;

                m_NameBoundToControlVar = NULL;
                m_BoundTo = NotBound;

                // Try the left side
                if(binary->Left->Opcode != ParseTree::Expression::SyntaxError)
                {
                    m_ThereWasABadNameReference = false;
                    m_ThereWasANameReference = false;

                    Left = m_Semantics->InterpretExpression(binary->Left, (m_Flags | ExprForceRValue) & ~ ExprTypeInferenceOnly);//  turn off ExprTypeInferenceOnly because an XML expr might ref a var, but InterpretXMLExpression nulls content. see 90774

                    if(m_NameBoundToControlVar)
                    {
                        AssertIfFalse(m_ThereWasANameReference);
                        LeftIsBoundTo = m_BoundTo;
                        LeftBadNameRef = m_ThereWasABadNameReference;

                        // prepare to interpret the right side
                        m_NameBoundToControlVar = NULL;

                        if(LeftIsBoundTo == BoundToLeft)
                        {
                            m_BoundTo = BoundToRight;
                        }
                        else
                        {
                            m_BoundTo = BoundToLeft;
                        }
                    }
                }

                // Now try the right side
                if(binary->Right->Opcode != ParseTree::Expression::SyntaxError)
                {
                    m_ThereWasABadNameReference = false;
                    m_ThereWasANameReference = false;

                    Right = m_Semantics->InterpretExpression(binary->Right, (m_Flags | ExprForceRValue) & ~ ExprTypeInferenceOnly); //  turn off ExprTypeInferenceOnly because an XML expr might ref a var, but InterpretXMLExpression nulls content. see 90774

                    RightBadNameRef = m_ThereWasABadNameReference;

                    if(m_ThereWasANameReference)
                    {
                        RightIsBoundTo = m_BoundTo;
                    }
                }

                // See if we need to report some errors
                if(m_Errors)
                {
                    m_Semantics->m_ReportErrors = true;
                    m_Semantics->m_Errors = m_Errors;
                }

                ParserHelper PH(&m_Semantics->m_TreeStorage, Predicate->TextSpan);
                ParseTree::Expression * exprLeft = binary->Left;
                ParseTree::Expression * exprRight = binary->Right;


                if(LeftIsBoundTo != NotBound && RightIsBoundTo != NotBound &&
                    Left && !IsBad(Left) && !LeftBadNameRef && Left->ResultType != TypeHelpers::GetVoidType() &&
                    Right && !IsBad(Right) && !RightBadNameRef && Right->ResultType != TypeHelpers::GetVoidType() &&
                    !TypeHelpers::EquivalentTypes(Left->ResultType,Right->ResultType))
                {
                    // figure out the type for the key
                    Type* ResultToUse = NULL;

                    Vtypes Result = t_bad;
                    Type * TypeLeft = Left->ResultType;
                    Type * TypeRight = Right->ResultType;
                    bool AtLeastOneTypeIsNullable = false;

                    // dig through Nullable types
                    if(TypeHelpers::IsNullableType(TypeLeft, m_Semantics->m_CompilerHost))
                    {
                        AtLeastOneTypeIsNullable = true;
                        TypeLeft = TypeHelpers::GetElementTypeOfNullable(TypeLeft, m_Semantics->m_CompilerHost);
                    }

                    if(TypeHelpers::IsNullableType(TypeRight, m_Semantics->m_CompilerHost))
                    {
                        AtLeastOneTypeIsNullable = true;
                        TypeRight = TypeHelpers::GetElementTypeOfNullable(TypeRight, m_Semantics->m_CompilerHost);
                    }

                    Vtypes VtypeLeft = TypeLeft->GetVtype();
                    Vtypes VtypeRight = TypeRight->GetVtype();

                    // Operands of type 1-dimensional array of Char are treated as if they
                    // were of type String.

                    if (TypeHelpers::IsCharArrayRankOne(TypeLeft))
                    {
                        VtypeLeft = t_string;
                    }

                    if (TypeHelpers::IsCharArrayRankOne(TypeRight))
                    {
                        VtypeRight = t_string;
                    }

                    // #77726 Avoid an assert in LookupInOperatorTables
                    if (VtypeLeft >= t_bad && VtypeLeft <= t_ref &&
                        VtypeRight >= t_bad && VtypeRight <= t_ref)
                    {
                        Result = LookupInOperatorTables(SX_EQ, VtypeLeft, VtypeRight);
                    }

                    if (Result != t_bad && Result != t_ref)
                    {
                        ResultToUse = m_Semantics->GetFXSymbolProvider()->GetType(Result);

                        // promote to Nullable type if result is a value type
                        if(AtLeastOneTypeIsNullable && TypeHelpers::IsValueType(ResultToUse))
                        {
                            ResultToUse = m_Semantics->GetFXSymbolProvider()->GetNullableIntrinsicSymbol(Result);
                        }
                    }
                    else
                    {
                        NorlsAllocator Scratch(NORLSLOC);
                        NorlsAllocWrapper AllocWrapper(&Scratch);
                        DominantTypeDataList TypeDataList(AllocWrapper);
                        TypeInferenceCollection bestType( m_Semantics, &Scratch, &AllocWrapper );

                        bestType.AddType( Left->ResultType, ConversionRequired::Any, Left );
                        bestType.AddType( Right->ResultType, ConversionRequired::Any, Right );

                        InferenceErrorReasons errorReasons = InferenceErrorReasonsOther;

                        // Ignore the option strict flag in this case to provide better error reporting.
                        // The implicit conversoin will give a more specific error than 'could not find equals'
                        bestType.FindDominantType( TypeDataList, &errorReasons, true );

                        if( TypeDataList.Count() == 1 )
                        {
                            ResultToUse = TypeDataList[0]->ResultType;
                        }
                    }

                    if( !ResultToUse )
                    {
                        if(m_Errors)
                        {
                            m_Semantics->ReportSemanticError(ERRID_EqualsTypeMismatch, Predicate->TextSpan, Left->ResultType,Right->ResultType);
                        }

                        m_KeysAreInvalid = true;
                    }
                    else
                    {
                        // see if we need to perform conversions
                        if(!TypeHelpers::EquivalentTypes(Left->ResultType,ResultToUse))
                        {
                            exprLeft = PH.CreateImplicitConversionExpression(exprLeft, ResultToUse);
                        }

                        if(!TypeHelpers::EquivalentTypes(Right->ResultType,ResultToUse))
                        {
                            exprRight = PH.CreateImplicitConversionExpression(exprRight, ResultToUse);
                        }
                    }
                }

                if(LeftIsBoundTo == NotBound && Left && !IsBad(Left) && binary->Left->Opcode != ParseTree::Expression::SyntaxError)
                {
                    if(m_Errors)
                    {
                        m_Semantics->ReportSemanticError(ERRID_EqualsOperandIsBad, binary->Left->TextSpan, GetErrorArg1(), GetErrorArg2());
                    }

                    m_KeysAreInvalid = true;
                }

                if(RightIsBoundTo == NotBound && !RightBadNameRef && Right && !IsBad(Right) && binary->Right->Opcode != ParseTree::Expression::SyntaxError)
                {
                    if(m_Errors)
                    {
                        m_Semantics->ReportSemanticError(ERRID_EqualsOperandIsBad, binary->Right->TextSpan, GetErrorArg1(), GetErrorArg2());
                    }

                    m_KeysAreInvalid = true;
                }

                // by default Left is bound to left
                if(LeftIsBoundTo == NotBound)
                {
                    if(RightIsBoundTo == BoundToLeft)
                    {
                        LeftIsBoundTo = BoundToRight;
                    }
                    else
                    {
                        LeftIsBoundTo = BoundToLeft;
                    }
                }

                // by default Right is bound to Right
                if(RightIsBoundTo == NotBound)
                {
                    if(LeftIsBoundTo == BoundToRight)
                    {
                        RightIsBoundTo = BoundToLeft;
                    }
                    else
                    {
                        RightIsBoundTo = BoundToRight;
                    }
                }

                // do not try to interpret a key with binding error

                if(LeftBadNameRef)
                {
                    exprLeft = PH.CreateSyntaxErrorExpression(exprLeft->TextSpan);
                }

                if(RightBadNameRef)
                {
                    exprRight = PH.CreateSyntaxErrorExpression(exprRight->TextSpan);
                }

                ParseTree::Expression * key1 = NULL;
                ParseTree::Expression * key2 = NULL;

                if(LeftIsBoundTo == BoundToLeft)
                {
                    AssertIfFalse(RightIsBoundTo == BoundToRight);
                    key1 = exprLeft;
                    key2 = exprRight;
                }
                else
                {
                    AssertIfFalse(LeftIsBoundTo == BoundToRight);
                    AssertIfFalse(RightIsBoundTo == BoundToLeft);
                    key2 = exprLeft;
                    key1 = exprRight;
                }

                if(!m_Key1)
                {
                    m_Key1 = key1;
                    m_Key2 = key2;
                }
                else
                {
                    // there is a bug with lambdas having the same location
                    if(m_KeySegmentCount==0)
                    {
                        m_Key1->TextSpan.SetStart(&key1->TextSpan);
                        m_Key2->TextSpan.SetStart(&key2->TextSpan);
                    }
                    else
                    {
                        m_Key1->TextSpan.SetEnd(&key1->TextSpan);
                        m_Key2->TextSpan.SetEnd(&key2->TextSpan);
                    }

                    // Add new segment into composite key
                    WCHAR SegmentName[20]=WIDE("");
                    StringCchPrintfW(SegmentName, DIM(SegmentName), WIDE("Key%u"), m_KeySegmentCount);
                    m_KeySegmentCount++;

                    STRING * segment = m_Semantics->m_Compiler->AddString(SegmentName);

                    PH.AddAssignmentInitializer
                        (
                            m_Key1->AsObjectInitializer(),
                            &PH.CreateIdentifierDescriptor(segment, key1->TextSpan),
                            key1,
                            key1->TextSpan
                        );

                    PH.AddAssignmentInitializer
                        (
                            m_Key2->AsObjectInitializer(),
                            &PH.CreateIdentifierDescriptor(segment, key2->TextSpan),
                            key2,
                            key2->TextSpan
                        );
                }

                if(ignoreErrors)
                {
                    delete ignoreErrors;
                }
            }
            break;

        case ParseTree::Expression::And:
            // create initializers
            if(!m_Key1)
            {
                ParserHelper PH(&m_Semantics->m_TreeStorage, Predicate->TextSpan);
                m_Key1 = PH.CreateAnonymousTypeInitializer(NULL, Predicate->TextSpan, true, true);
                m_Key2 = PH.CreateAnonymousTypeInitializer(NULL, Predicate->TextSpan, true, true);
            }

            binary = Predicate->AsBinary();
            BuildKey(binary->Left);
            BuildKey(binary->Right);
            break;

        case ParseTree::Expression::Like:
        case ParseTree::Expression::Is: 
        case ParseTree::Expression::IsNot: 
        case ParseTree::Expression::Equal: 
        case ParseTree::Expression::NotEqual: 
        case ParseTree::Expression::Less: 
        case ParseTree::Expression::LessEqual: 
        case ParseTree::Expression::GreaterEqual: 
        case ParseTree::Expression::Greater: 
        case ParseTree::Expression::Or:
        case ParseTree::Expression::AndAlso:
        case ParseTree::Expression::OrElse:
        case ParseTree::Expression::Xor:
        case ParseTree::Expression::SyntaxError:    // don't do anything if Synatax error: if AND is followed by EOL, then binary->Right will be SyntaxError
            break;

        default:
            m_Semantics->ReportSemanticError(
                ERRID_InternalCompilerError,
                Predicate->TextSpan); // shouldn't get here
            break; // shouldn't get here
    }
}


WCHAR* Semantics::LambdaBodyBuildKeyExpressions::GetErrorArg1()
{
    if(m_ErrorArg1.GetStringLength() == 0)
    {
        AssertIfNull(m_InitializerList1);

        if(m_InitializerList1)
        {
            ParseTree::InitializerList * list = m_InitializerList1;
            WCHAR comma[3] = L"\0 ";

            do
            {
                AssertIfFalse(list->Element && list->Element->Opcode == ParseTree::Initializer::Expression);

                if(list->Element && list->Element->Opcode == ParseTree::Initializer::Expression)
                {
                    ParseTree::ExpressionInitializer * exprInitializer;

                    exprInitializer = list->Element->AsExpression();

                    AssertIfFalse(exprInitializer->Value &&
                        exprInitializer->Value->Opcode == ParseTree::Expression::Name);

                    if(exprInitializer->Value &&
                    exprInitializer->Value->Opcode == ParseTree::Expression::Name)
                    {
                        m_ErrorArg1.AppendString(comma);
                        m_ErrorArg1.AppendPrintf(L"'%s'",exprInitializer->Value->AsName()->Name.Name);
                        comma[0] = L',';
                    }
                }

                list = list->Next;
            }
            while(list && list!=m_InitializerList2);
        }
    }

    return m_ErrorArg1.GetString();
}

WCHAR* Semantics::LambdaBodyBuildKeyExpressions::GetErrorArg2()
{
    if(m_ErrorArg2.GetStringLength() == 0)
    {
        if(m_InitializerList2)
        {
            ParseTree::InitializerList * list = m_InitializerList2;
            WCHAR comma[3] = L"\0 ";

            do
            {
                AssertIfFalse(list->Element && list->Element->Opcode == ParseTree::Initializer::Expression);

                if(list->Element && list->Element->Opcode == ParseTree::Initializer::Expression)
                {
                    ParseTree::ExpressionInitializer * exprInitializer;

                    exprInitializer = list->Element->AsExpression();

                    AssertIfFalse(exprInitializer->Value &&
                        exprInitializer->Value->Opcode == ParseTree::Expression::Name);

                    if(exprInitializer->Value &&
                    exprInitializer->Value->Opcode == ParseTree::Expression::Name)
                    {
                        m_ErrorArg2.AppendString(comma);
                        m_ErrorArg2.AppendPrintf(L"'%s'",exprInitializer->Value->AsName()->Name.Name);
                        comma[0] = L',';
                    }
                }

                list = list->Next;
            }
            while(list);
        }
        else
        {
            m_ErrorArg2.AppendPrintf(L"'%s'", m_ControlVarName2);
        }
    }

    return m_ErrorArg2.GetString();
}


void
Semantics::LambdaBodyBuildKeyExpressions::CheckName
(
    Declaration * var,
    _In_z_ Identifier * Name,
    const Location & nameTextSpan
)
{
    BoundState BoundTo = NotBound;

    if(var == m_Var1)
    {
        BoundTo = BoundToLeft;
    }
    else if(var == m_Var2)
    {
        BoundTo = BoundToRight;
    }
    else
    {
        if(m_Prev)
        {
            m_Prev->CheckName(var, Name, nameTextSpan);
        }

        return;
    }

    if(m_BoundTo == NotBound)
    {
        m_BoundTo = BoundTo;
        m_NameBoundToControlVar = Name;
        m_ThereWasANameReference = true;
    }
    else if(m_BoundTo != BoundTo)
    {
        BackupValue<bool> backup_m_ReportErrors(&m_Semantics->m_ReportErrors);
        BackupValue<ErrorTable *> backup_m_Errors(&m_Semantics->m_Errors);

        if(m_Errors)
        {
            m_Semantics->m_ReportErrors = true;
            m_Semantics->m_Errors = m_Errors;

            if(m_NameBoundToControlVar)
            {
                if(!m_ThereWasABadNameReference || !m_Errors->HasThisErrorWithLocation(ERRID_EqualsOperandIsBad, nameTextSpan))
                {
                    m_Semantics->ReportSemanticError(ERRID_EqualsOperandIsBad, nameTextSpan, GetErrorArg1(), GetErrorArg2());
                }
            }
            else
            {
                if(!m_ThereWasABadNameReference || !m_Errors->HasThisErrorWithLocation(ERRID_EqualsOperandIsBad, nameTextSpan))
                {
                    m_Semantics->ReportSemanticError(ERRID_EqualsOperandIsBad, nameTextSpan, GetErrorArg1(), GetErrorArg2());
                }
            }
        }

        m_KeysAreInvalid = true;
        m_ThereWasABadNameReference = true;
    }
    else
    {
        AssertIfFalse(m_BoundTo == BoundTo);
        m_ThereWasANameReference = true;
    }
}


ILTree::Expression *
Semantics::InterpretGroupJoinExpression
(
    _In_ ParseTree::GroupJoinExpression *GroupJoin,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    _Out_opt_ ParseTree::AlreadyBoundType **pGroupElementType
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pGroupElementType);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType=NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;
    ParseTree::NewObjectInitializerExpression * ItInitializer=NULL;

    ILTree::Expression * Result =
        InterpretGroupJoinExpression
        (
            GroupJoin,
            Flags,
            NULL, //FollowingLet,
            true, //lastItemToJoin,
            ItInitializer,
            ControlVariableName,
            ControlVariableType,
            QueryFlags,
            pFollowingOperator,
            pGroupElementType
        );

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    if(pQueryFlags)
    {
        *pQueryFlags=QueryFlags;
    }

    if(pRecordInitializer)
    {
        *pRecordInitializer = ItInitializer;
    }

    return Result;
}


void
Semantics::GetNameInfoFromInitializer
(
    _In_ ParseTree::InitializerList * nextRightInitializer,
    _Out_ ParseTree::IdentifierDescriptor * &RightIdentifierDescriptor,
    _Out_ Location * &rightNameLocation
)
{
    AssertIfFalse(nextRightInitializer->Element &&
        nextRightInitializer->Element->Opcode == ParseTree::Initializer::Expression);

    if(nextRightInitializer->Element &&
        nextRightInitializer->Element->Opcode == ParseTree::Initializer::Expression)
   {
        ParseTree::ExpressionInitializer * exprInitializer;

        exprInitializer = nextRightInitializer->Element->AsExpression();

        AssertIfFalse(exprInitializer->Value &&
            exprInitializer->Value->Opcode == ParseTree::Expression::Name);

        if(exprInitializer->Value &&
            exprInitializer->Value->Opcode == ParseTree::Expression::Name)
        {
            RightIdentifierDescriptor = &exprInitializer->Value->AsName()->Name;
            rightNameLocation = &exprInitializer->Value->TextSpan;
        }
    }
}


bool
Semantics::JoinCheckDuplicateControlVars
(
    _In_opt_ ParseTree::InitializerList * firstLeftInitializer,
    ParseTree::InitializerList * stopInitializer,
    _In_opt_ ParseTree::IdentifierDescriptor * RightIdentifierDescriptor,
    Location * rightNameLocation,
    _In_opt_ ParseTree::InitializerList * nextRightInitializer
)
{
    bool DuplicateVars = false;

    for(;;)
    {
        if(RightIdentifierDescriptor)
        {
            ParseTree::InitializerList * nextLeftInitializer;
            ParseTree::IdentifierDescriptor * LeftIdentifierDescriptor=NULL;
            Location * notused = NULL;

            for(nextLeftInitializer = firstLeftInitializer; nextLeftInitializer && nextLeftInitializer != stopInitializer;  nextLeftInitializer = nextLeftInitializer->Next)
            {
                GetNameInfoFromInitializer(nextLeftInitializer, LeftIdentifierDescriptor, notused);

                AssertIfNull(LeftIdentifierDescriptor);

                if(LeftIdentifierDescriptor)
                {
                    if(StringPool::IsEqual(RightIdentifierDescriptor->Name, LeftIdentifierDescriptor->Name, false))
                    {
                        DuplicateVars = true;

                        ReportSemanticError(
                            ERRID_QueryDuplicateAnonTypeMemberName1,
                            *rightNameLocation,
                            RightIdentifierDescriptor->Name);
                        break;
                    }
                }
            }
        }

        if(nextRightInitializer)
        {
            GetNameInfoFromInitializer(nextRightInitializer, RightIdentifierDescriptor, rightNameLocation);
            nextRightInitializer = nextRightInitializer->Next;

            continue;
        }

        break;
    }

    return !DuplicateVars;
}

ILTree::Expression *
Semantics::InterpretGroupJoinExpression
(
    _In_ ParseTree::GroupJoinExpression *GroupJoin,
    ExpressionFlags Flags,
    ParseTree::FromItem *FollowingLet,
    bool lastItemToJoin,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _Out_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Deref_out_ ParseTree::AlreadyBoundType *& ControlVariableType,
    _Out_ QueryExpressionFlags & QueryFlags,
    _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    _Out_opt_ ParseTree::AlreadyBoundType **pGroupElementType
)
{
    QueryFlags = QueryNoFlags;

    ParseTree::IdentifierDescriptor ControlVariableName1;
    ParseTree::AlreadyBoundType *ControlVariableType1 = NULL;
    QueryExpressionFlags QueryFlags1=QueryNoFlags;
    ParseTree::NewObjectInitializerExpression * ItInitializer1 = NULL;

    ParseTree::LinqOperatorExpression * op = GroupJoin;

    // Interpret the first source
    ILTree::Expression *Source1 = InterpretJoinOperatorSource(GroupJoin->Source, Flags, ItInitializer1, ControlVariableName1, ControlVariableType1, QueryFlags1, &op);

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            GroupJoin->Source->TextSpan); // shouldn't get here
        return AllocateBadExpression(GroupJoin->Source->TextSpan); // shouldn't get here
    }

    if(!GroupJoin->JoinTo)
    {
        return AllocateBadExpression(GroupJoin->TextSpan); // shouldn't get here, parser never leaves this NULL
    }

    Location joinTextSpan = GroupJoin->TextSpan;
    ParserHelper PH(&m_TreeStorage, joinTextSpan);

    if(!ItInitializer1)
    {
        AssertIfTrue(!IsBad(Source1) && HasFlag(QueryFlags1, ControlVarIsRecord));

        ItInitializer1 = PH.CreateAnonymousTypeInitializer(NULL, joinTextSpan, true, true);

        if(!HasFlag(QueryFlags1, ControlVarIsHidden))
        {
            PH.AddExpressionInitializer(ItInitializer1, PH.CreateNameExpression(ControlVariableName1.Name,ControlVariableName1.TextSpan));
        }
    }
    else if(!IsBad(Source1))
    {
        AssertIfFalse(HasFlag(QueryFlags1, ControlVarIsRecord));
        AssertIfFalse(HasFlag(QueryFlags1, ControlVarIsHidden));
    }

    ParseTree::IdentifierDescriptor ControlVariableName2;
    ParseTree::AlreadyBoundType *ControlVariableType2 = NULL;
    QueryExpressionFlags QueryFlags2=QueryNoFlags;

    // Interpret the second source
    bool mustFlatten = !(GroupJoin->Projection && IsOKToNotFlattenBeforeInto(GroupJoin->Projection, false));
    ParseTree::NewObjectInitializerExpression * ItInitializer2 = NULL;

    ILTree::Expression *Source2 = InterpretJoinOperatorSource(GroupJoin->JoinTo, Flags, ItInitializer2, ControlVariableName2, ControlVariableType2, QueryFlags2,
                                                                                mustFlatten ? NULL : &op);

    if (pGroupElementType)
    {
        *pGroupElementType = ControlVariableType2;
    }

    if(IsBad(Source1))
    {
        return Source1;
    }

    if(IsBad(Source2))
    {
        return Source2;
    }

    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            GroupJoin->JoinTo->TextSpan); // shouldn't get here
        return AllocateBadExpression(GroupJoin->JoinTo->TextSpan); // shouldn't get here
    }

    // check for duplicate control variables on the right
    ParseTree::InitializerList * nextRightInitializer=NULL;
    ParseTree::IdentifierDescriptor * RightIdentifierDescriptor=NULL;
    Location * rightNameLocation=NULL;

    if(ItInitializer2)
    {
        AssertIfFalse(HasFlag(QueryFlags2, ControlVarIsRecord));
        nextRightInitializer = ItInitializer2->InitialValues->InitialValues;

        if(nextRightInitializer)
        {
            GetNameInfoFromInitializer(nextRightInitializer, RightIdentifierDescriptor, rightNameLocation);
            nextRightInitializer = nextRightInitializer->Next;
        }
    }
    else
    {
        AssertIfTrue(HasFlag(QueryFlags2, ControlVarIsRecord));
        RightIdentifierDescriptor = &ControlVariableName2;
        rightNameLocation = &ControlVariableName2.TextSpan;
    }

    bool DuplicateVars = !JoinCheckDuplicateControlVars
                                    (
                                        ItInitializer1->InitialValues->InitialValues,
                                        NULL,
                                        RightIdentifierDescriptor,
                                        rightNameLocation,
                                        nextRightInitializer
                                    );

    if(DuplicateVars &&
        !HasFlag(QueryFlags1, ControlVarIsRecord) &&
        !HasFlag(QueryFlags2, ControlVarIsRecord))
    {
        return AllocateBadExpression(GroupJoin->JoinTo->TextSpan);
    }

    // interpret keys
    ILTree::LambdaExpression * boundKey1 = NULL;
    ILTree::LambdaExpression * boundKey2 = NULL;

    InterpretJoinKeys
    (
        ControlVariableName1,
        ControlVariableType1,
        QueryFlags1,
        ItInitializer1->InitialValues->InitialValues,
        ControlVariableName2,
        ControlVariableType2,
        QueryFlags2,
        ItInitializer2 ? ItInitializer2->InitialValues->InitialValues : NULL,
        GroupJoin->Predicate,
        Flags,
        boundKey1,
        boundKey2
    );

    // Interpret the GroupJoin call
    bool keysAreBad = false;

    if(!boundKey1 || !boundKey2 || IsBad(boundKey1) || IsBad(boundKey2))
    {
        keysAreBad = true;
    }

    ILTree::Expression * Result=NULL;
    Location GroupJoinLocation = Location::GetHiddenLocation();
    GroupJoinLocation.SetLocation(GroupJoin->TextSpan.m_lBegLine, &GroupJoin->FirstPunctuator,g_tkKwdNameLengths[tkGROUP]-1);

    if(GroupJoin->HaveJoin)
    {
        Location JoinLocation = {0};
        JoinLocation.SetLocation(GroupJoin->TextSpan.m_lBegLine, &GroupJoin->Join,g_tkKwdNameLengths[tkJOIN]-1);
        GroupJoinLocation.SetEnd(&JoinLocation);
    }

    Type * ProjectionReturnType=NULL;
    ParseTree::Expression * GroupRef = GetGroupReferenceFromIntoProjection(GroupJoin->Projection);

    // If Group is the only thing that gets projected by the INTO clause and the left control var is not a record,
    // we don't need to go through the type inference for the Group
    // This is mostly due to performance reasons.
    // Check GroupRef first because GroupJoin->Projection may be NULL
    if(GroupRef && !GroupJoin->Projection->Next && !HasFlag(QueryFlags1, ControlVarIsRecord))
    {
        ParseTree::NameExpression * InternalGroupName=NULL;
        ParseTree::IdentifierDescriptor InternalGroupControlVariableName;
        InternalGroupControlVariableName = ItIdentifierDescriptor(STRING_CONST(m_Compiler, Group));
        InternalGroupControlVariableName.TextSpan = GroupRef->TextSpan;
        InternalGroupName = PH.CreateNameExpression(InternalGroupControlVariableName);

        // can't continue if we failed to interpret the keys
        if(keysAreBad)
        {
            return AllocateBadExpression(GroupJoin->TextSpan);
        }

        ParseTree::InitializerList * FirstAddedInitializer = NULL;
        bool projectionIsGood =
                        AppendIntoProjection
                                (
                                    ItInitializer1,
                                    GroupJoin->Projection,
                                    InternalGroupName,
                                    ControlVariableName2,
                                    ControlVariableType2,
                                    QueryFlags2,
                                    &FirstAddedInitializer
                                );

        if(!projectionIsGood || !FirstAddedInitializer ||
            FirstAddedInitializer->Element->Opcode != ParseTree::Initializer::Assignment)

        {
            // Trivial projection, should have succeeded
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                GroupJoin->TextSpan); // shouldn't get here
            return AllocateBadExpression(GroupJoin->TextSpan); // shouldn't get here
        }
        else
        {
            ParseTree::IdentifierDescriptor * GrName = &FirstAddedInitializer->Element->AsAssignment()->Name;

            // Make sure there is no name clash
            if(StringPool::IsEqual(GrName->Name,ControlVariableName1.Name, false))
            {
                ReportSemanticError(ERRID_QueryDuplicateAnonTypeMemberName1, GrName->TextSpan, GrName->Name);
                return AllocateBadExpression(GroupJoin->TextSpan);
            }
        }

        ItInitializer1->TextSpan.SetEnd(&GroupJoin->Projection->TextSpan);

        Result = BindGroupJoinOperator
                        (
                            Source1,
                            Source2,
                            GroupJoin->TextSpan,
                            GroupJoinLocation,
                            boundKey1,
                            boundKey2,
                            ControlVariableName1,
                            ControlVariableType1,
                            QueryFlags1,
                            InternalGroupControlVariableName,
                            ItInitializer1
                        );

        if(IsBad(Result))
        {
            return Result;
        }

        // Check names for shadowing and other restrictions
        // Check names after BindGroupJoinOperator() because it may declare new locals for the Proc
        if(!CheckIntoProjectionForShadowing(FirstAddedInitializer))
        {
            return AllocateBadExpression(GroupJoin->TextSpan);
        }

        // Adjust our ItInitializer1 so that we can return it
        ConvertIntoProjectionToExpressions(FirstAddedInitializer);

        // Control variable is the return value of the final selector, which is the last lambda passed to the GroupJoin operator.
        ProjectionReturnType = GetControlVariableTypeAfterGroupBy(Result);

        AssertIfNull(ProjectionReturnType);

        if(!ProjectionReturnType)
        {
            AssertIfNull(ProjectionReturnType);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                GroupJoin->TextSpan); // shouldn't get here
            return AllocateBadExpression(GroupJoin->TextSpan); // shouldn't get here
        }
    }
    else
    {
        // Now, it is the time to infer the type for our group.
        // To do this, we will try to bind GroupJoin method using
        // simple selector {group} and then extruct the type from
        // Lambda's return type.
        // Using Anonymous Type will ensure that we will bind to the same GroupBy
        // method with the real selector because the real selector will be an Anonymous Type too.
        Type * GroupType = NULL;

        // can't do the inference if we failed to interpret the keys
        if(!keysAreBad)
        {
            ParseTree::NewObjectInitializerExpression *inferenceProjection;
            STRING * memberName = STRING_CONST(m_Compiler, GroupJoinMethod); // we need to use VB legal name here otherwise Anonymous Type will choke
            ParseTree::IdentifierDescriptor inferenceControlVariableName = ItIdentifierDescriptor(memberName);

            inferenceProjection = PH.CreateAnonymousTypeInitializer(NULL, GroupJoin->TextSpan, true, true);
            PH.AddExpressionInitializer(inferenceProjection,  PH.CreateNameExpression(memberName, GroupJoinLocation));

            BackupValue<ErrorTable *> backup_error_table(&m_Errors);
            ErrorTable * pTemporaryErrorTable=NULL;

            if(m_Errors)
            {
                pTemporaryErrorTable = new  ErrorTable(*m_Errors);
                m_Errors = pTemporaryErrorTable;
            }

            Result = BindGroupJoinOperator
                            (
                                Source1,
                                Source2,
                                GroupJoin->TextSpan,
                                GroupJoinLocation,
                                boundKey1,
                                boundKey2,
                                ItIdentifierDescriptor(STRING_CONST(m_Compiler, Key)), // Use compiler generated name to avoid clash with inferenceControlVariableName
                                ControlVariableType1,
                                QueryFlags1,
                                inferenceControlVariableName,
                                inferenceProjection
                            );

            backup_error_table.Restore();

            if(IsBad(Result) && m_Errors)
            {
                m_Errors->MergeTemporaryTable(pTemporaryErrorTable);
            }

            if(pTemporaryErrorTable)
            {
                delete pTemporaryErrorTable;
            }

            if(!IsBad(Result))
            {
                // The group is the only member of an Anonymous Type returned by the last lambda passed to the GroupJoin operator.
                GroupType = GetTypeOfTheGroup(Result, memberName, GroupJoinLocation);

                if(!GroupType)
                {
                    AssertIfNull(GroupType);
                    ReportSemanticError( // shouldn't get here
                        ERRID_InternalCompilerError,
                        GroupJoin->TextSpan); // shouldn't get here
                }
            }
        }

        AssertIfFalse(!GroupType || !keysAreBad);

        // Now, go through INTO projection and build our real selector
        if(!GroupJoin->Projection)
        {
            return AllocateBadExpression(GroupJoin->TextSpan); // Parser should have reported an error
        }


        ParseTree::NewObjectInitializerExpression * ProjectionInitializer = NULL;


        // GroupJoin is similar to a Let (we can't merge it with the following SELECT)
        // Let's use the same logic for control variable nesting
        if(HasFlag(QueryFlags1,ControlVarIsRecord) &&
            (!GroupType || // don't do flattenning since we reported an error already
            !ShouldFlattenLetJoinSelector(lastItemToJoin, pFollowingOperator)))
        {
            QueryFlags |= ControlVarIsNested;

            Location firstItemTextSpan = ControlVariableName1.TextSpan;
            ControlVariableName1 = Join1IdentifierDescriptor();
            ControlVariableName1.TextSpan = firstItemTextSpan;

            ProjectionInitializer = PH.CreateAnonymousTypeInitializer(NULL, joinTextSpan, true, true);
            PH.AddExpressionInitializer(ProjectionInitializer, PH.CreateNameExpression(ControlVariableName1));
        }
        else
        {
            ProjectionInitializer = ItInitializer1;
        }

        // Use compiler generated name for the Group argument, user
        // shouldn't be able to reference the group inside an Aggregate function
        ParseTree::IdentifierDescriptor GroupControlVariableName;
        GroupControlVariableName = ItIdentifierDescriptor(STRING_CONST(m_Compiler, Group));
        GroupControlVariableName.TextSpan = GroupJoinLocation;
        ParseTree::NameExpression *GroupName = PH.CreateNameExpression(GroupControlVariableName);

        ParseTree::ParameterList * lambdaParams = NULL;

        if(GroupType)
        {
            lambdaParams = PH.CreateParameterList(
                                                                            GroupJoin->Projection->TextSpan,
                                                                            2,
                                                                            PH.CreateParameter(
                                                                                ControlVariableName1,
                                                                                ControlVariableType1, true,
                                                                                HasFlag(QueryFlags1,ControlVarIsRecord)),
                                                                            PH.CreateParameter(
                                                                                GroupControlVariableName,
                                                                                PH.CreateBoundType(GroupType, GroupJoinLocation), true,
                                                                                false)
                                                                            );
        }
        else
        {
            // We were not able to interpret the type of the group,
            // regardless, try to report errors for projection
            lambdaParams = PH.CreateParameterList(
                                                                            GroupJoin->Projection->TextSpan,
                                                                            1,
                                                                            PH.CreateParameter(
                                                                                ControlVariableName1,
                                                                                ControlVariableType1, true,
                                                                                HasFlag(QueryFlags1,ControlVarIsRecord))
                                                                             );
        }

        LambdaBodyInterpretAggregateProjection bodyInterpreter
                                (
                                    this,
                                    GroupName,
                                    ControlVariableName2,
                                    ControlVariableType2,
                                    QueryFlags2,
                                    GroupType ? ProjectionInitializer : NULL, // pass NULL if we only want to validate the projection and report errors
                                    GroupJoin->Projection,
                                    Flags
                                );

        ILTree::LambdaExpression * boundProjection =
                            CreateLambdaExpression(
                                        InterpretLambdaParameters(lambdaParams),
                                        GroupJoin->Projection->TextSpan,
                                        Location::GetInvalidLocation(), // BodySpan, only relevant for multiline lambdas
                                        TypeHelpers::GetVoidType(),
                                        &bodyInterpreter,
                                        true, // IsFunctionLambda
                                        false, // IsAsyncKeywordUsed
                                        false, // IsIteratorKeywordUsed
                                        SingleLineLambda);

        if(IsBad(boundProjection))
        {
            return boundProjection;
        }

        boundProjection->ExactlyMatchExpressionResultType = true;

        bool projectionIsGood = bodyInterpreter.ProjectionIsGood();

        AssertIfFalse(!projectionIsGood || (GroupType && !keysAreBad));

        if(keysAreBad || !GroupType || !projectionIsGood)
        {
            return AllocateBadExpression(GroupJoin->TextSpan);
        }

        AssertIfNull(boundProjection);

        // Check names for shadowing
        // Check names after BindSelectSelector() because it may declare new locals for the Proc
        ParseTree::InitializerList * FirstAddedInitializer = bodyInterpreter.FirstAddedInitializer();
        AssertIfNull(FirstAddedInitializer);
        if(!CheckIntoProjectionForShadowing(FirstAddedInitializer))
        {
            return AllocateBadExpression(GroupJoin->TextSpan);
        }

        ProjectionReturnType = boundProjection->GetExpressionLambdaBody()->ResultType;

        // Bind final GroupJoin call
        Result = BindGroupJoinOperator
                        (
                            Source1,
                            Source2,
                            GroupJoin->TextSpan,
                            GroupJoinLocation,
                            boundKey1,
                            boundKey2,
                            boundProjection
                        );

        if(IsBad(Result))
        {
            AssertIfTrue(IsBad(Result));
            return Result; // shouldn't get here
        }

        // Adjust our ItInitializer1 so that we can return it
        ConvertIntoProjectionToExpressions(FirstAddedInitializer);

        if(ProjectionInitializer != ItInitializer1)
        {
            if(!JoinCheckDuplicateControlVars
                                        (
                                            ItInitializer1->InitialValues->InitialValues,
                                            FirstAddedInitializer,
                                            NULL,
                                            NULL,
                                            FirstAddedInitializer
                                        ))
            {
                return AllocateBadExpression(GroupJoin->TextSpan);
            }

            PH.AppendInitializerList( ItInitializer1, FirstAddedInitializer);
        }

        ItInitializer1->TextSpan.SetEnd(&GroupJoin->Projection->TextSpan);
    }

    AssertIfFalse(ProjectionReturnType);

    if(DuplicateVars)
    {
        MakeBad(Result);
        return Result;
    }

    QueryFlags |= AppliedAtLeastOneOperator | ControlVarIsHidden | ControlVarIsRecord;
    ControlVariableName = ItIdentifierDescriptor();
    ControlVariableType = PH.CreateBoundType(ProjectionReturnType, ItInitializer1->TextSpan);

    if(!ItInitializer)
    {
        ItInitializer = ItInitializer1;
    }
    else
    {
        PH.AppendInitializerList( ItInitializer, ItInitializer1->InitialValues->InitialValues);
        ItInitializer->TextSpan.SetEnd(&ItInitializer1->TextSpan);
    }


    return Result;
}


ILTree::Expression *
Semantics::BindGroupJoinOperator
(
    ILTree::Expression * Source1,
    ILTree::Expression * Source2,
    _In_ Location & JoinTextSpan,
    _In_ Location & JoinNameLocation,
    ILTree::LambdaExpression * boundKey1,
    ILTree::LambdaExpression * boundKey2,
    _In_ ParseTree::IdentifierDescriptor &ControlVariableName1,
    ParseTree::AlreadyBoundType *ControlVariableType1,
    QueryExpressionFlags QueryFlags1,
    _In_ ParseTree::IdentifierDescriptor &groupControlVariableName,
    _In_opt_ ParseTree::NewObjectInitializerExpression *Projection
)
{
    ILTree::UnboundLambdaExpression * unboundLambda =
                                                        BuildUnboundLambdaForIntoProjection
                                                        (
                                                            ControlVariableName1,
                                                            ControlVariableType1,
                                                            QueryFlags1,
                                                            groupControlVariableName,
                                                            Projection
                                                        );

    if (IsBad(unboundLambda))
    {
        return unboundLambda; // not likely to get here
    }

    return BindGroupJoinOperator
                    (
                        Source1,
                        Source2,
                        JoinTextSpan,
                        JoinNameLocation,
                        boundKey1,
                        boundKey2,
                        unboundLambda
                    );
}


ILTree::Expression *
Semantics::BindGroupJoinOperator
(
    ILTree::Expression * Source1,
    ILTree::Expression * Source2,
    _In_ Location & JoinTextSpan,
    _In_ Location & JoinNameLocation,
    ILTree::LambdaExpression * boundKey1,
    ILTree::LambdaExpression * boundKey2,
    ILTree::Expression *ProjectionLambda
)
{
    if(!(SX_LAMBDA == ProjectionLambda->bilop || SX_UNBOUND_LAMBDA == ProjectionLambda->bilop))
    {
        AssertIfFalse(SX_LAMBDA == ProjectionLambda->bilop || SX_UNBOUND_LAMBDA == ProjectionLambda->bilop);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            JoinTextSpan); // should never get here
        return AllocateBadExpression(JoinTextSpan);// should never get here
    }

    ILTree::Expression * Result;
    ParserHelper PH(&m_TreeStorage, JoinTextSpan);

    //Expression *
    Result =
        InterpretQueryOperatorCall(
            PH.CreateMethodCall(
                        PH.CreateQualifiedExpression(
                                    PH.CreateBoundExpression(Source1),
                                    PH.CreateNameExpression(STRING_CONST(m_Compiler, GroupJoinMethod), JoinNameLocation),
                                    JoinNameLocation),
                        PH.CreateArgList(
                                    4,
                                    PH.CreateBoundExpression(Source2),
                                    PH.CreateBoundExpression(boundKey1),
                                    PH.CreateBoundExpression(boundKey2),
                                    PH.CreateBoundExpression(ProjectionLambda)),
                        JoinTextSpan),
            ExprForceRValue);

    return Result;
}


ILTree::Expression *
Semantics::InterpretJoinSelector
(
    _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
    _Inout_ QueryExpressionFlags &QueryFlags,
    _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName2,
    ParseTree::AlreadyBoundType * ControlVariableType2,
    QueryExpressionFlags QueryFlags2,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _In_opt_ ParseTree::FromItem *FollowingLet,
    bool lastItemToJoin,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    _Inout_ Location &joinTextSpan // can be modified !!!
)
{
    ParserHelper PH(&m_TreeStorage, joinTextSpan);

    ParseTree::IdentifierDescriptor newControlVariableName;
    QueryExpressionFlags newQueryFlags =  QueryNoFlags;

    // Build selector that joins two control variables
    ParseTree::Expression * SelectExpression = NULL;

    // In some scenarios, it is safe to leave Control Variable in nested state when the next operator
    // does its own projection (Select, Group By, ...). That operator have to take an Anonymous Type in
    // both cases and, since there is no way to restrict the shape of the Anonymous Type in method's
    // declaration, the operator should be insensitive to the shape of the Anonymous Type.

    // REVISIT THIS CODE IF the previous assumption about not being able to restrict the shape of the
    // Anonymous Type is no longer valid!!!

    bool shouldNest = false;

    if(!FollowingLet) // Let will handle proper nesting/flattening
    {
        if(!lastItemToJoin)
        {
            shouldNest = true;
        }
        else if(pFollowingOperator && *pFollowingOperator)
        {
            switch((*pFollowingOperator)->Opcode)
            {
                case ParseTree::Expression::GroupBy:
                    if(IsOKToNotFlattenBeforeGroupBy((*pFollowingOperator)->AsGroupBy()))
                    {
                        shouldNest = true;
                    }
                    break;

                case ParseTree::Expression::InnerJoin:
                case ParseTree::Expression::GroupJoin:
                     shouldNest = true;
                     break;
            }
        }
    }

    // rename It variables
    if(HasFlag(QueryFlags,ControlVarIsRecord))
    {
        Location firstItemTextSpan = ControlVariableName.TextSpan;
        ControlVariableName = Join1IdentifierDescriptor();
        ControlVariableName.TextSpan = firstItemTextSpan;
    }

    if(HasFlag(QueryFlags2,ControlVarIsRecord))
    {
        Location secondItemTextSpan = ControlVariableName2.TextSpan;
        ControlVariableName2 = Join2IdentifierDescriptor();
        ControlVariableName2.TextSpan = secondItemTextSpan;
    }

    if(shouldNest)
    {
        if(!HasFlag(QueryFlags,ControlVarIsRecord) && HasFlag(QueryFlags,ControlVarIsHidden))
        {
            // don't need to add the first control var into the result since it doesn't have user provided name
            SelectExpression = PH.CreateNameExpression(ControlVariableName2.Name, ControlVariableName2.TextSpan);

            newQueryFlags = QueryFlags2;
            newControlVariableName = ControlVariableName2;
        }
        else
        {
            ParseTree::NewObjectInitializerExpression * joinInitializer =
                    PH.CreateAnonymousTypeInitializer(NULL, joinTextSpan, true, true);

            // Set nesting flag
            if(HasFlag(QueryFlags,ControlVarIsRecord))
            {
                newQueryFlags |= ControlVarIsNested;
            }

            if(HasFlag(QueryFlags2,ControlVarIsRecord))
            {
                newQueryFlags |= ControlVarIsNested;
            }

            PH.AddExpressionInitializer(joinInitializer, PH.CreateNameExpression(ControlVariableName.Name,ControlVariableName.TextSpan));
            PH.AddExpressionInitializer(joinInitializer, PH.CreateNameExpression(ControlVariableName2.Name,ControlVariableName2.TextSpan));

            newQueryFlags |= QueryFlags | QueryFlags2 | ControlVarIsRecord | ControlVarIsHidden;
            newControlVariableName = ItIdentifierDescriptor();

            SelectExpression = joinInitializer;
        }
    }
    else if (FollowingLet)
    {
        joinTextSpan.SetEnd(&FollowingLet->TextSpan);

        ILTree::Expression * selector =
            InterpretLetJoinSelector
            (
                ControlVariableName,
                ControlVariableType,
                QueryFlags,
                FollowingLet,
                ItInitializer,
                lastItemToJoin,
                pFollowingOperator,
                joinTextSpan,
                &ControlVariableName2,
                ControlVariableType2,
                QueryFlags2
            );

        return selector;
    }
    else if(pFollowingOperator && *pFollowingOperator && (*pFollowingOperator)->Opcode == ParseTree::Expression::Select)
    {
        ParseTree::SelectExpression * Select = (*pFollowingOperator)->AsSelect();

        *pFollowingOperator = NULL; // indicate that we don't need to apply following Select operator

        newControlVariableName = ControlVariableName;
        ParseTree::AlreadyBoundType * newControlVariableType = ControlVariableType;
        newQueryFlags = QueryFlags;

        //Use Selector from the following Select operator
        ILTree::Expression * selector = InterpretSelectSelector
                                            (
                                                Select,
                                                newControlVariableName,
                                                newControlVariableType,
                                                newQueryFlags,
                                                &ControlVariableName,
                                                &ControlVariableType,
                                                &QueryFlags,
                                                &ItInitializer,
                                                &ControlVariableName2,
                                                ControlVariableType2,
                                                QueryFlags2
                                            );

        joinTextSpan.SetEnd(&Select->TextSpan);

        return selector;
    }
    else
    {
        // flatten new control variable
        if(
            !HasFlag(QueryFlags,ControlVarIsRecord) && HasFlag(QueryFlags,ControlVarIsHidden) &&
            (!HasFlag(QueryFlags2,ControlVarIsRecord) || !HasFlag(QueryFlags2,ControlVarIsNested)))
        {
            SelectExpression = PH.CreateNameExpression(ControlVariableName2.Name, ControlVariableName2.TextSpan);

            newQueryFlags = QueryFlags2;
            newControlVariableName = ControlVariableName2;
            AssertIfTrue(HasFlag(newQueryFlags,ControlVarIsNested));

            if(!HasFlag(newQueryFlags,ControlVarIsRecord))
            {
                ItInitializer = NULL;
            }
        }
        else
        {
            SelectExpression = ItInitializer;
            ItInitializer->TextSpan.SetEnd(&joinTextSpan);
            newQueryFlags = ControlVarIsRecord | ControlVarIsHidden;
            newControlVariableName = ItIdentifierDescriptor();
        }
    }

    ILTree::LambdaExpression * boundSelector = NULL;

    AssertIfNull(SelectExpression);
    boundSelector = BindSelectSelector(SelectExpression,
                                                                                            ControlVariableName, ControlVariableType, QueryFlags,
                                                                                            ControlVariableName2, ControlVariableType2, QueryFlags2);

    if(IsBad(boundSelector))
    {
        return boundSelector;
    }

    // prepare resulting control var data
    ControlVariableName = newControlVariableName;
    ControlVariableType = PH.CreateBoundType(boundSelector->GetExpressionLambdaBody()->ResultType, joinTextSpan);
    QueryFlags = newQueryFlags;

    return boundSelector;
}


ILTree::Expression *
Semantics::InterpretCrossJoin
(
    ILTree::Expression * Source,
    _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
    _Inout_ QueryExpressionFlags &QueryFlags,
    Location CommaOrFromTextSpan,
    _In_ ParseTree::FromItem *FromItem,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _In_opt_ ParseTree::FromItem *FollowingLet,
    bool lastItemToJoin,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    Location joinTextSpan
)
{
    AssertIfTrue(FromItem->IsLetControlVar);
    if(FromItem->IsLetControlVar)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            FromItem->TextSpan);  // shouldn't get here
        return AllocateBadExpression(FromItem->TextSpan); // shouldn't get here
    }

    ParserHelper PH(&m_TreeStorage, FromItem->TextSpan);

    // Interpret selector for the second collection
    ParseTree::ParameterList * lambdaParams = PH.CreateParameterList(
                                                                    FromItem->TextSpan,
                                                                    1,
                                                                    PH.CreateParameter(
                                                                        ControlVariableName,
                                                                        ControlVariableType, true,
                                                                        HasFlag(QueryFlags, ControlVarIsRecord)));

    LambdaBodyInterpretJoinedFromItem bodyInterpreter(this, FromItem, ItInitializer);

    Location lambdaTextSpan = FromItem->TextSpan;

    ILTree::LambdaExpression * boundManySelector =
                        CreateLambdaExpression(
                                    InterpretLambdaParameters(lambdaParams),
                                    lambdaTextSpan,
                                    Location::GetInvalidLocation(), // BodySpan, only relevant for multiline lambdas
                                    TypeHelpers::GetVoidType(),
                                    &bodyInterpreter,
                                    true, // IsFunctionLambda
                                    false, // IsAsyncKeywordUsed
                                    false, // IsIteratorKeywordUsed
                                    SingleLineLambda);

    if(IsBad(boundManySelector))
    {
        return boundManySelector;
    }

    ParseTree::IdentifierDescriptor ControlVariableName2 = bodyInterpreter.GetAddedName();
    ParseTree::AlreadyBoundType * ControlVariableType2 = bodyInterpreter.GetControlVariableType();
    QueryExpressionFlags QueryFlags2 = bodyInterpreter.GetQueryFlags();

    // Build selector that joins two control variables
    ILTree::Expression * selector =
                InterpretJoinSelector
                (
                    ControlVariableName,
                    ControlVariableType,
                    QueryFlags,
                    ControlVariableName2,
                    ControlVariableType2,
                    QueryFlags2,
                    ItInitializer,
                    FollowingLet,
                    lastItemToJoin,
                    pFollowingOperator,
                    joinTextSpan
                );

    if(IsBad(selector))
    {
        return selector;
    }
    else if(selector->bilop != SX_LAMBDA)
    {
        AssertIfFalse(selector->bilop == SX_LAMBDA);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            FromItem->TextSpan); // shouldn't get here
        return AllocateBadExpression(FromItem->TextSpan); // shouldn't get here
    }

    ILTree::LambdaExpression * boundSelector = NULL;

    boundSelector = &selector->AsLambdaExpression();

    QueryFlags |= AppliedAtLeastOneOperator;

    Location selectManyNameLocation = CommaOrFromTextSpan;

    ILTree::Expression *
    Result =
        InterpretQueryOperatorCall(
            PH.CreateMethodCall(
                        PH.CreateQualifiedExpression(
                                    PH.CreateBoundExpression(Source),
                                    PH.CreateNameExpression(STRING_CONST(m_Compiler, SelectManyMethod), selectManyNameLocation),
                                    selectManyNameLocation),
                        PH.CreateArgList(
                                    2,
                                    PH.CreateBoundExpression(boundManySelector),
                                    PH.CreateBoundExpression(boundSelector)),
                        joinTextSpan),
            ExprForceRValue);

    return Result;
}


ILTree::Expression *
Semantics::InterpretCrossJoin
(
    ILTree::Expression * Source,
    ParseTree::IdentifierDescriptor ControlVariableName,
    ParseTree::AlreadyBoundType * ControlVariableType,
    QueryExpressionFlags QueryFlags,
    Location CommaOrFromTextSpan,
    _In_opt_ ParseTree::FromList *nextFromItems,
    ParseTree::LinqExpression * CrossJoinTo,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    _Out_opt_ ParseTree::IdentifierDescriptor *pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    Location TextSpan
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, NULL);


    do
    {
        ParseTree::FromList *FromItems = nextFromItems;
        Location joinTextSpan = TextSpan;
        joinTextSpan.SetEnd(&FromItems->Element->TextSpan);


        // Look ahead to see if there are more items to join
        bool AnError=false;
        Location CommaOrFromOrErrorTextSpan;
        nextFromItems = GetNextItemInFromList(nextFromItems, &CrossJoinTo, CommaOrFromOrErrorTextSpan, AnError);

        if(AnError)
        {
            return AllocateBadExpression(CommaOrFromOrErrorTextSpan);
        }

        // deal with Let variables
        if(FromItems->Element->IsLetControlVar)
        {
            Source = InterpretLetJoin(Source,
                                                    ControlVariableName,
                                                    ControlVariableType,
                                                    QueryFlags,
                                                    CommaOrFromTextSpan,
                                                    FromItems->Element,
                                                    ItInitializer,
                                                    !nextFromItems,
                                                    nextFromItems ? NULL : pFollowingOperator,
                                                    joinTextSpan);
        }
        else
        {
            ParseTree::FromItem *FollowingLet = NULL;

            // See if we need to merge this join with the following Let
            if(nextFromItems && nextFromItems->Element->IsLetControlVar)
            {
                FollowingLet = nextFromItems->Element;

                nextFromItems = GetNextItemInFromList(nextFromItems, &CrossJoinTo, CommaOrFromOrErrorTextSpan, AnError);

                if(AnError)
                {
                    return AllocateBadExpression(CommaOrFromOrErrorTextSpan); // not likely to get here
                }
            }

            Source = InterpretCrossJoin(Source,
                                                        ControlVariableName,
                                                        ControlVariableType,
                                                        QueryFlags,
                                                        CommaOrFromTextSpan,
                                                        FromItems->Element,
                                                        ItInitializer,
                                                        FollowingLet,
                                                        !nextFromItems,
                                                        nextFromItems ? NULL : pFollowingOperator,
                                                        joinTextSpan);
        }

        if(IsBad(Source))
        {
            return Source;
        }

        AssertIfFalse(HasFlag(QueryFlags, AppliedAtLeastOneOperator));

        CommaOrFromTextSpan = CommaOrFromOrErrorTextSpan;
    }
    while(nextFromItems);

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    if(pQueryFlags)
    {
        *pQueryFlags = QueryFlags;
    }

    return Source;
}


bool
Semantics::ShouldFlattenLetJoinSelector
(
    bool lastItemToJoin,
    _In_ ParseTree::LinqOperatorExpression **pFollowingOperator
)
{
    bool shouldFlatten = true;

    // In some scenarios, it is safe to leave Control Variable in nested state when the next operator
    // does its own projection (Select, Group By, ...). That operator have to take an Anonymous Type in
    // both cases and, since there is no way to restrict the shape of the Anonymous Type in method's
    // declaration, the operator should be insensitive to the shape of the Anonymous Type.

    // REVISIT THIS CODE IF the previous assumption about not being able to restrict the shape of the
    // Anonymous Type is no longer valid!!!

    if(!lastItemToJoin)
    {
        shouldFlatten = false;
    }
    else if(pFollowingOperator && *pFollowingOperator)
    {
        switch((*pFollowingOperator)->Opcode)
        {
            case ParseTree::Expression::Select:
                // Do not flatten if the next operator is Select
                shouldFlatten = false;
                break;

            case ParseTree::Expression::GroupBy:
                if(IsOKToNotFlattenBeforeGroupBy((*pFollowingOperator)->AsGroupBy()))
                {
                    shouldFlatten = false;
                }
                break;

            case ParseTree::Expression::InnerJoin:
                shouldFlatten = false;
                break;

            case ParseTree::Expression::GroupJoin:
                shouldFlatten = false;
                break;
        }
    }

    return shouldFlatten;
}


ILTree::Expression *
Semantics::InterpretLetJoinSelector
(
    _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
    _Inout_ QueryExpressionFlags &QueryFlags,
    _In_ ParseTree::FromItem *FromItem,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    bool lastItemToJoin,
    _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    Location joinTextSpan,
    _Inout_opt_ ParseTree::IdentifierDescriptor *pControlVariableName2,
    ParseTree::AlreadyBoundType * pControlVariableType2,
    QueryExpressionFlags QueryFlags2
)
{
    AssertIfFalse(FromItem->IsLetControlVar);
    AssertIfNull(ItInitializer);
    AssertIfFalse((pControlVariableName2 && pControlVariableType2) || (!pControlVariableName2 && !pControlVariableType2));


    if(!FromItem->IsLetControlVar)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            FromItem->TextSpan);
        return AllocateBadExpression(FromItem->TextSpan);
    }

    // deal with Let variables
    ParseTree::NewObjectInitializerExpression * letItInitializer = NULL;
    ParserHelper PH(&m_TreeStorage, FromItem->TextSpan);

    QueryExpressionFlags newQueryFlags =  QueryNoFlags;

    AssertIfTrue(HasFlag(QueryFlags2,ControlVarIsHidden) && !HasFlag(QueryFlags2,ControlVarIsRecord));

    if(HasFlag(QueryFlags,ControlVarIsRecord) || !HasFlag(QueryFlags,ControlVarIsHidden) || pControlVariableName2)
    {
        bool shouldFlatten = ShouldFlattenLetJoinSelector(lastItemToJoin, pFollowingOperator);

        if(!shouldFlatten)
        {
            // not the last item, build nested control variable if needed
            letItInitializer = PH.CreateAnonymousTypeInitializer(NULL, joinTextSpan, true, true);

            // rename It variable
            if(HasFlag(QueryFlags,ControlVarIsRecord))
            {
                Location firstItemTextSpan = ControlVariableName.TextSpan;
                ControlVariableName = Join1IdentifierDescriptor();
                ControlVariableName.TextSpan = firstItemTextSpan;
                newQueryFlags |= ControlVarIsNested;
            }

            // Add the first control variable only if it is a record or not hidden
            if(HasFlag(QueryFlags,ControlVarIsRecord) || !HasFlag(QueryFlags,ControlVarIsHidden))
            {
                PH.AddExpressionInitializer(letItInitializer, PH.CreateNameExpression(ControlVariableName.Name,ControlVariableName.TextSpan));
            }

            // Add the second control variable
            if(pControlVariableName2)
            {
                newQueryFlags |= QueryFlags2;

                // rename the variable
                if(HasFlag(QueryFlags2,ControlVarIsRecord))
                {
                    Location secondItemTextSpan = pControlVariableName2->TextSpan;
                    *pControlVariableName2 = Join2IdentifierDescriptor();
                    pControlVariableName2->TextSpan = secondItemTextSpan;
                    newQueryFlags |= ControlVarIsNested;
                }

                PH.AddExpressionInitializer(letItInitializer, PH.CreateNameExpression(pControlVariableName2->Name,pControlVariableName2->TextSpan));
            }

            newQueryFlags |= QueryFlags | ControlVarIsRecord | ControlVarIsHidden;
        }
        else
        {
            // the last item to join, flatten control variable if needed
            letItInitializer = ItInitializer;
            newQueryFlags = ControlVarIsRecord | ControlVarIsHidden;
        }
    }
    else if(lastItemToJoin)
    {
        // should have an empty initializer at this point
        AssertIfFalse(ItInitializer && ItInitializer->InitialValues && !ItInitializer->InitialValues->InitialValues);
        ItInitializer = NULL;
    }

    ParseTree::ParameterList * lambdaParams;

    // If we are merging a Join with the Let, add the second control variable
    if(pControlVariableName2)
    {
        lambdaParams = PH.CreateParameterList(
                                    FromItem->TextSpan,
                                    2,
                                    PH.CreateParameter(
                                        ControlVariableName,
                                        ControlVariableType, true,
                                        HasFlag(QueryFlags,ControlVarIsRecord)),
                                    PH.CreateParameter(
                                        *pControlVariableName2,
                                        pControlVariableType2, true,
                                        HasFlag(QueryFlags2,ControlVarIsRecord))
                                        );
    }
    else
    {
        lambdaParams = PH.CreateParameterList(
                                    FromItem->TextSpan,
                                    1,
                                    PH.CreateParameter(
                                        ControlVariableName,
                                        ControlVariableType, true,
                                        HasFlag(QueryFlags,ControlVarIsRecord)));
    }

    LambdaBodyInterpretJoinedLetItem bodyInterpreter(this, FromItem, letItInitializer);

    ILTree::LambdaExpression * boundSelector =
                        CreateLambdaExpression(
                                    InterpretLambdaParameters(lambdaParams),
                                    FromItem->TextSpan,
                                    Location::GetInvalidLocation(), // BodySpan, only relevant for multiline lambdas
                                    TypeHelpers::GetVoidType(),
                                    &bodyInterpreter,
                                    true, // IsFunctionLambda
                                    false, // IsAsyncKeywordUsed
                                    false, // IsIteratorKeywordUsed
                                    SingleLineLambda);

    if(IsBad(boundSelector))
    {
        return boundSelector;
    }

    boundSelector->ExactlyMatchExpressionResultType = true;

    ParseTree::IdentifierDescriptor addedIdentifier = bodyInterpreter.GetAddedName();
    ParseTree::NameExpression * addedName = PH.CreateNameExpression(addedIdentifier.Name,addedIdentifier.TextSpan);

    if(ItInitializer)
    {
        if(ItInitializer != letItInitializer)
        {
            AssertIfFalse(!lastItemToJoin || !HasFlag(newQueryFlags,ControlVarIsRecord)
                                        ||
                                        (pFollowingOperator && *pFollowingOperator &&
                                        (
                                            (*pFollowingOperator)->Opcode == ParseTree::Expression::Select ||
                                            (*pFollowingOperator)->Opcode == ParseTree::Expression::GroupBy ||
                                            (*pFollowingOperator)->Opcode == ParseTree::Expression::InnerJoin ||
                                            (*pFollowingOperator)->Opcode == ParseTree::Expression::GroupJoin))
                                   );

            PH.AddExpressionInitializer(ItInitializer, addedName);
        }
        else
        {
            AssertIfFalse(lastItemToJoin && HasFlag(newQueryFlags,ControlVarIsRecord));
            PH.ChangeToExpressionInitializer(bodyInterpreter.GetAddedInitializer(), addedName);
        }
    }

    ControlVariableType = PH.CreateBoundType(boundSelector->GetExpressionLambdaBody()->ResultType, joinTextSpan);
    QueryFlags = newQueryFlags;

    if(letItInitializer)
    {
        AssertIfFalse(HasFlag(QueryFlags,ControlVarIsRecord));
        ControlVariableName = ItIdentifierDescriptor();
    }
    else
    {
        AssertIfTrue(HasFlag(QueryFlags,ControlVarIsRecord));
        ControlVariableName = addedIdentifier;
    }

    return boundSelector;
}

ILTree::Expression *
Semantics::InterpretLetJoin
(
    ILTree::Expression * Source,
    _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
    _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
    _Inout_ QueryExpressionFlags &QueryFlags,
    Location CommaOrFromTextSpan,
    _In_ ParseTree::FromItem *FromItem,
    _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
    bool lastItemToJoin,
    _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    Location joinTextSpan
)
{
    ILTree::Expression * selector =
        InterpretLetJoinSelector
        (
            ControlVariableName,
            ControlVariableType,
            QueryFlags,
            FromItem,
            ItInitializer,
            lastItemToJoin,
            pFollowingOperator,
            joinTextSpan
        );

    if(IsBad(selector))
    {
        return selector;
    }
    else if(selector->bilop != SX_LAMBDA)
    {
        AssertIfFalse(selector->bilop == SX_LAMBDA);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            FromItem->TextSpan); // shouldn't get here
        return AllocateBadExpression(FromItem->TextSpan); // shouldn't get here
    }

    ILTree::LambdaExpression * boundSelector =  &selector->AsLambdaExpression();

    QueryFlags |= AppliedAtLeastOneOperator;

    return BindSelectOperator(Source, joinTextSpan, CommaOrFromTextSpan, boundSelector);
}


ILTree::Expression *
Semantics::InterpretCrossJoinExpression
(
    _In_ ParseTree::CrossJoinExpression *CrossJoin,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _Inout_opt_ ParseTree::LinqOperatorExpression **pFollowingOperator
)
{
    if(CrossJoin->Source->Opcode == ParseTree::Expression::From)
    {
        return InterpretFromExpression(CrossJoin->Source->AsFrom(), CrossJoin->JoinTo, Flags,
                                                        pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pFollowingOperator);
    }

    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);
    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;
    ParseTree::NewObjectInitializerExpression * ItInitializer = NULL;
    ParseTree::LinqOperatorExpression * followingOperator = pFollowingOperator ? *pFollowingOperator : NULL;

    Location CommaOrFromOrErrorTextSpan;
    bool AnError=false;
    ParseTree::LinqExpression * CrossJoinTo = CrossJoin->JoinTo;
    ParseTree::FromList *nextFromItems = GetNextItemInFromList(NULL, &CrossJoinTo, CommaOrFromOrErrorTextSpan, AnError);

    if(AnError)
    {
        return AllocateBadExpression(CommaOrFromOrErrorTextSpan); // not likely to get here
    }

    ILTree::Expression *Source=NULL;

    if(nextFromItems &&
        (CrossJoin->Source->Opcode == ParseTree::Expression::InnerJoin ||
        CrossJoin->Source->Opcode == ParseTree::Expression::GroupJoin))
    {
        ParseTree::FromItem *FollowingLet = NULL;

        if(CrossJoin->Source->Opcode == ParseTree::Expression::GroupJoin)
        {
            Source =
                            InterpretGroupJoinExpression
                            (
                                CrossJoin->Source->AsGroupJoin(),
                                Flags,
                                FollowingLet,
                                !nextFromItems, //lastItemToJoin,
                                ItInitializer,
                                ControlVariableName,
                                ControlVariableType,
                                QueryFlags,
                                pFollowingOperator
                            );
        }
        else
        {
            AssertIfFalse(CrossJoin->Source->Opcode == ParseTree::Expression::InnerJoin);

            // See if we need to merge this join with the following Let
            if(nextFromItems->Element->IsLetControlVar)
            {
                FollowingLet = nextFromItems->Element;

                nextFromItems = GetNextItemInFromList(nextFromItems, &CrossJoinTo, CommaOrFromOrErrorTextSpan, AnError);

                if(AnError)
                {
                    return AllocateBadExpression(CommaOrFromOrErrorTextSpan); // not likely to get here
                }
            }

            Source =
                            InterpretInnerJoinExpression
                            (
                                CrossJoin->Source->AsInnerJoin(),
                                Flags,
                                FollowingLet,
                                !nextFromItems, //lastItemToJoin,
                                ItInitializer,
                                ControlVariableName,
                                ControlVariableType,
                                QueryFlags,
                                pFollowingOperator
                            );
        }

        AssertIfFalse(IsBad(Source) || HasFlag(QueryFlags, AppliedAtLeastOneOperator));

        if(IsBad(Source))
        {
            return Source;
        }

        // return if nothing left to join
        if(!nextFromItems)
        {
            AssertIfNull(FollowingLet);
            AssertIfFalse(CrossJoin->Source->Opcode == ParseTree::Expression::InnerJoin);
            AssertIfFalse(IsBad(Source) || (HasFlag(QueryFlags, ControlVarIsRecord) && ItInitializer));
            AssertIfFalse(!followingOperator || followingOperator == (*pFollowingOperator));

            if(pQueryFlags)
            {
                *pQueryFlags = QueryFlags;
            }

            if(pControlVariableType)
            {
                *pControlVariableType = ControlVariableType;
            }

            if(pControlVariableName)
            {
                *pControlVariableName = ControlVariableName;
            }

            if(pRecordInitializer)
            {
                *pRecordInitializer = ItInitializer;
            }

            return Source;
        }

        AssertIfFalse(true); // for code coverage
    }
    else
    {
        ParseTree::LinqOperatorExpression * op = CrossJoin;
        Source = InterpretLinqOperatorSource(CrossJoin->Source, Flags, &op, &ControlVariableName, &ControlVariableType, &QueryFlags, &ItInitializer);

        if (IsBad(Source))
        {
            return Source;
        }

        AssertIfNull(op);
        if(!op)
        {
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                CrossJoin->TextSpan); // shouldn't get here
            return AllocateBadExpression(CrossJoin->TextSpan); // shouldn't get here
        }
    }

    if(!ItInitializer)
    {
        AssertIfTrue(HasFlag(QueryFlags, ControlVarIsRecord));

        ParserHelper PH(&m_TreeStorage, CrossJoin->TextSpan);
        ItInitializer = PH.CreateAnonymousTypeInitializer(NULL, CrossJoin->TextSpan, true, true);

        if(!HasFlag(QueryFlags, ControlVarIsHidden))
        {
            PH.AddExpressionInitializer(ItInitializer, PH.CreateNameExpression(ControlVariableName.Name,ControlVariableName.TextSpan));
        }
    }
    else
    {
        AssertIfFalse(HasFlag(QueryFlags, ControlVarIsRecord));
        AssertIfFalse(HasFlag(QueryFlags, ControlVarIsHidden));
    }

    QueryExpressionFlags resultQueryFlags=QueryNoFlags;
    ParseTree::IdentifierDescriptor resultControlVariableName;

    ILTree::Expression *Result = InterpretCrossJoin
                                (
                                    Source,
                                    ControlVariableName,
                                    ControlVariableType,
                                    QueryFlags,
                                    CommaOrFromOrErrorTextSpan,
                                    nextFromItems,
                                    CrossJoinTo,
                                    ItInitializer,
                                    &resultControlVariableName,
                                    pControlVariableType,
                                    &resultQueryFlags,
                                    pFollowingOperator,
                                    CrossJoin->TextSpan
                                );

    AssertIfFalse(IsBad(Result) || HasFlag(resultQueryFlags, AppliedAtLeastOneOperator));

    if(pQueryFlags)
    {
        *pQueryFlags = resultQueryFlags;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = resultControlVariableName;
    }

    if(!HasFlag(resultQueryFlags, ControlVarIsRecord))
    {
        AssertIfFalse(ItInitializer == NULL || IsBad(Result));
        ItInitializer = NULL;
    }

    AssertIfFalse(!followingOperator || followingOperator == (*pFollowingOperator) || !(*pFollowingOperator));

    if(pRecordInitializer && ItInitializer && !IsBad(Result))
    {
        // do not adjust initializer's location if it comes from Select
        if(!followingOperator ||(*pFollowingOperator))
        {
            ItInitializer->TextSpan = CrossJoin->TextSpan;
        }

        *pRecordInitializer = ItInitializer;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretFromExpression
(
    _In_ ParseTree::FromExpression *From,
    ParseTree::LinqExpression * CrossJoinTo,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _Inout_opt_ ParseTree::LinqOperatorExpression **pFollowingOperator
)
{
    AssertIfTrue(CrossJoinTo &&
                            CrossJoinTo->Opcode!=ParseTree::Expression::From &&
                            CrossJoinTo->Opcode!=ParseTree::Expression::Let &&
                            CrossJoinTo->Opcode!=ParseTree::Expression::CrossJoin);

    ILTree::Expression *Result = NULL;
    ParseTree::FromList * FromClauses = From->FromItems;
    ParseTree::FromItem *FromClause = FromClauses->Element;
    ParseTree::NewObjectInitializerExpression * ItInitializer = NULL;
    ParseTree::LinqOperatorExpression * followingOperator = pFollowingOperator ? *pFollowingOperator : NULL;

    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    if(FromClause->IsLetControlVar)
    {
        Location errorTextSpan = FromClause->ControlVariableDeclaration->TextSpan;
            errorTextSpan.SetEnd(FromClause->TextSpan.m_lBegLine + FromClause->InOrEq.Line,
                                            FromClause->InOrEq.Column + g_tkKwdNameLengths[tkEQ]-1);

        ReportSemanticError(
            From->ImplicitFrom ? ERRID_AggregateStartsWithLet : ERRID_QueryStartsWithLet,
            errorTextSpan);
        return AllocateBadExpression(FromClause->TextSpan);
    }
    else if (FromClauses->Next || CrossJoinTo)
    {
        ParserHelper PH(&m_TreeStorage, FromClauses->TextSpan);
        Location joinTextSpan = FromClauses->TextSpan;

        if(CrossJoinTo)
        {
            joinTextSpan.SetEnd(&CrossJoinTo->TextSpan);
        }

        ItInitializer = PH.CreateAnonymousTypeInitializer(NULL, FromClauses->TextSpan, true, true);

        Result = BuildJoinedFromClauses(FromClauses, CrossJoinTo,
                                                        ItInitializer,
                                                        pControlVariableName,
                                                        pControlVariableType,
                                                        pQueryFlags,
                                                        pFollowingOperator,
                                                        joinTextSpan);

        AssertIfFalse(!followingOperator || followingOperator == (*pFollowingOperator) || !(*pFollowingOperator));

        if(!IsBad(Result))
        {
            Result->Loc.SetStart(&From->TextSpan);
        }

        if(pQueryFlags)
        {
            AssertIfFalse(IsBad(Result) || HasFlag(*pQueryFlags, AppliedAtLeastOneOperator));
            *pQueryFlags |= AppliedAtLeastOneOperator;

            if(!HasFlag(*pQueryFlags, ControlVarIsRecord))
            {
                AssertIfFalse(ItInitializer == NULL || IsBad(Result));
            }
        }

        if(pRecordInitializer && ItInitializer && !IsBad(Result))
        {
            // do not adjust initializer's location if it comes from Select
            if(!followingOperator ||(*pFollowingOperator))
            {
                ItInitializer->TextSpan = joinTextSpan;
            }

            *pRecordInitializer = ItInitializer;
        }
    }
    else
    {
        Result = InterpretFromItem(FromClause, pControlVariableName, pControlVariableType, pQueryFlags);

        // #27975 Do this after InterpretFromItem() because it may declare new locals for the Proc
        bool nameIsGood = CheckControlVariableDeclaration(FromClause->ControlVariableDeclaration);

        if(!nameIsGood)
        {
            return AllocateBadExpression(FromClause->TextSpan);
        }
    }

    if (!Result)
    {
        return AllocateBadExpression(FromClause->TextSpan);
    }

    return Result;
}

ILTree::Expression *
Semantics::InterpretFilterExpression
(
    _In_ ParseTree::FilterExpression *Filter,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;

    ParseTree::LinqOperatorExpression * op = Filter;
    ILTree::Expression *Source = InterpretLinqOperatorSource(Filter->Source, Flags, &op, &ControlVariableName, &ControlVariableType, &QueryFlags, pRecordInitializer);

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    if(pQueryFlags)
    {
        *pQueryFlags=QueryFlags;
    }

    if (IsBad(Source))
    {
        return Source;
    }

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Filter->TextSpan); // shouldn't get here
        return AllocateBadExpression(Filter->TextSpan); // shouldn't get here
    }

    ParseTree::Expression *FilterExpression = Filter->Predicate;

    if(FilterExpression->Opcode == ParseTree::Expression::SyntaxError)
    {
        return AllocateBadExpression(FilterExpression->TextSpan);
    }

    ParserHelper PH(&m_TreeStorage, Filter->TextSpan);

    STRING * opMethod;
    Location operatorLocation = {0};

    if(Filter->Opcode == ParseTree::Expression::Where)
    {
        operatorLocation.SetLocation(Filter->TextSpan.m_lBegLine, &Filter->FirstPunctuator,g_tkKwdNameLengths[tkWHERE]-1);
        opMethod = STRING_CONST(m_Compiler, WhereMethod);
    }
    else
    {
        tokens opToken;

        if(Filter->Opcode == ParseTree::Expression::TakeWhile)
        {
            opMethod = STRING_CONST(m_Compiler, TakeWhileMethod);
            opToken = tkTAKE;
        }
        else
        {
            AssertIfFalse(Filter->Opcode == ParseTree::Expression::SkipWhile);
            opMethod = STRING_CONST(m_Compiler, SkipWhileMethod);
            opToken = tkSKIP;
        }

        operatorLocation.SetLocation(Filter->TextSpan.m_lBegLine, &Filter->FirstPunctuator,g_tkKwdNameLengths[opToken]-1);

        Location WhileLocation = {0};
        WhileLocation.SetLocation(Filter->TextSpan.m_lBegLine, &Filter->AsWhile()->While,g_tkKwdNameLengths[tkWHILE]-1);
        operatorLocation.SetEnd(&WhileLocation);
    }


    ParseTree::LambdaExpression * predicate =
                                            PH.CreateSingleLineLambdaExpression(
                                                        PH.CreateParameterList(
                                                                    FilterExpression->TextSpan,
                                                                    1,
                                                                    PH.CreateParameter(
                                                                        ControlVariableName,
                                                                        ControlVariableType, true,
                                                                        HasFlag(QueryFlags, ControlVarIsRecord))),
                                                        FilterExpression,
                                                        FilterExpression->TextSpan,
                                                        true
                                                        );

    ILTree::LambdaExpression * boundPredicate =
            InterpretUnboundLambda(
                    InterpretLambdaExpression(predicate, ExprForceRValue));

    if(IsBad(boundPredicate))
    {
        return boundPredicate;
    }

    // Let's verify type of the predicate
    BackupValue<ErrorTable *> backup_error_table(&m_Errors);
    ErrorTable * pTemporaryErrorTable=NULL;

    if(m_Errors)
    {
        pTemporaryErrorTable = new  ErrorTable(*m_Errors);
        m_Errors = pTemporaryErrorTable;
    }

    // Since we may keep the result of the InterpretConditionalOperand
    // Let's use lambda's Lookup and TemporaryManager
    BackupValue<Scope *> Lookup_backup(&m_Lookup);
    BackupValue<TemporaryManager *> TemporaryManager_backup(&m_TemporaryManager);

    m_Lookup = boundPredicate->GetLocalsHash();
    m_TemporaryManager = boundPredicate->TemporaryManager;

    ILTree::Expression * convertedPredicate = InterpretConditionalOperand(PH.CreateBoundExpression(boundPredicate->GetExpressionLambdaBody()));

    Lookup_backup.Restore();
    TemporaryManager_backup.Restore();
    backup_error_table.Restore();

    if(IsBad(convertedPredicate))
    {
        if(m_Errors)
        {
            m_Errors->MergeTemporaryTable(pTemporaryErrorTable);
        }
    }
    else if(boundPredicate->GetExpressionLambdaBody()->ResultType == GetFXSymbolProvider()->GetObjectType())
    {
        // We are going to special case Object here due to the (Bug 54486 - DevDiv Bugs)
        // We are leaving it in Boolean state
        boundPredicate->SetExpressionLambdaBody(convertedPredicate);

        // keep any warnings that could get
        if(m_Errors)
        {
            m_Errors->MergeTemporaryTable(pTemporaryErrorTable);
        }
    }

    if(pTemporaryErrorTable)
    {
        delete pTemporaryErrorTable;
    }

    if(IsBad(convertedPredicate))
    {
        return convertedPredicate;
    }

    boundPredicate->ConvertResultTypeFlags = ExprIsOperandOfConditionalBranch;

    ILTree::Expression * Result =
        InterpretQueryOperatorCall(
                    PH.CreateMethodCall(
                                PH.CreateQualifiedExpression(
                                            PH.CreateBoundExpression(Source),
                                            PH.CreateNameExpression(opMethod, operatorLocation),
                                            operatorLocation),
                                PH.CreateArgList(
                                            1,
                                            PH.CreateBoundExpression(boundPredicate))),
                    ExprForceRValue);

    if(pQueryFlags)
    {
        *pQueryFlags |= AppliedAtLeastOneOperator;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretSkipTakeExpression
(
    _In_ ParseTree::SkipTakeExpression *SkipTake,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;

    ParseTree::LinqOperatorExpression * op = SkipTake;
    ILTree::Expression *Source = InterpretLinqOperatorSource(SkipTake->Source, Flags, &op, &ControlVariableName, &ControlVariableType, &QueryFlags, pRecordInitializer);

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    if(pQueryFlags)
    {
        *pQueryFlags=QueryFlags;
    }

    if (IsBad(Source))
    {
        return Source;
    }

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            SkipTake->TextSpan);  // shouldn't get here
        return AllocateBadExpression(SkipTake->TextSpan);  // shouldn't get here
    }

    ParseTree::Expression *CountExpression = SkipTake->Count;

    ILTree::Expression * boundCount = InterpretExpression(SkipTake->Count, Flags | ExprForceRValue);

    if(IsBad(boundCount))
    {
        return boundCount;
    }

    ParserHelper PH(&m_TreeStorage, SkipTake->TextSpan);

    tokens operatorToken;
    STRING * methodName;

    if(SkipTake->Opcode == ParseTree::Expression::Take)
    {
        operatorToken = tkTAKE;
        methodName = STRING_CONST(m_Compiler, TakeMethod);
    }
    else
    {
        AssertIfFalse(SkipTake->Opcode == ParseTree::Expression::Skip);
        operatorToken = tkSKIP;
        methodName = STRING_CONST(m_Compiler, SkipMethod);
    }

    Location operatorLocation = {0};
    operatorLocation.SetLocation(SkipTake->TextSpan.m_lBegLine, &SkipTake->FirstPunctuator,g_tkKwdNameLengths[operatorToken]-1);

    ILTree::Expression * Result =
        InterpretQueryOperatorCall(
                    PH.CreateMethodCall(
                                PH.CreateQualifiedExpression(
                                            PH.CreateBoundExpression(Source),
                                            PH.CreateNameExpression(methodName, operatorLocation),
                                            operatorLocation),
                                PH.CreateArgList(
                                            1,
                                            PH.CreateBoundExpression(boundCount))),
                    ExprForceRValue);

    if(pQueryFlags)
    {
        *pQueryFlags |= AppliedAtLeastOneOperator;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretAggregateExpression
(
    _In_ ParseTree::AggregateExpression *Aggregate,
    ExpressionFlags Flags,
    _Out_opt_ Type **pGroupType,
    _Out_opt_ ParseTree::AlreadyBoundType **pGroupElementType
)
{
    // interpret stand alone Aggregate expression
    AssertIfFalse(Aggregate->Source == NULL);

    // first interpret the group
    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;

    ILTree::Expression * Group = InterpretLinqOperatorSource(Aggregate->AggSource, Flags,NULL, &ControlVariableName, &ControlVariableType, &QueryFlags, NULL);

    if (pGroupType)
    {
        *pGroupType = (!IsBad(Group) && Group->ResultType && !Group->ResultType->IsBad()) ?
                            Group->ResultType :
                            NULL;
    }

    if (pGroupElementType)
    {
        *pGroupElementType = ControlVariableType;
    }

    if(IsBad(Group))
    {
        return Group;
    }

    ParserHelper PH(&m_TreeStorage, Aggregate->TextSpan);
    bool projectionIsGood = true;
    ILTree::Expression * boundResult = NULL;

    // see if we need to create an Anonymous Type
    if(!Aggregate->Projection)
    {
        return AllocateBadExpression(Aggregate->TextSpan); // Parser should have reported an error
    }
    else if(Aggregate->Projection->Next)
    {
        // Need to create an Anonymous Type
        // So we need to store the Group in a temporary and then build
        // an Anonymous type initializer using that temporary


        Variable *Temporary = NULL;
        ILTree::ExpressionWithChildren * assign = CaptureInShortLivedTemporary(Group, Temporary);

        ILTree::SymbolReferenceExpression * groupRef =
                     AllocateSymbolReference(
                            Temporary,
                            Temporary->GetType(),
                            NULL,
                            Group->Loc);

        // build Anonymous Type initializer
        ParseTree::NewObjectInitializerExpression * Initializer = PH.CreateAnonymousTypeInitializer(NULL, Aggregate->Projection->TextSpan, true, true);

        ParseTree::InitializerList * FirstAddedInitializer = NULL;

        projectionIsGood =
                                                AppendIntoProjection
                                                (
                                                    Initializer,
                                                    Aggregate->Projection,
                                                    PH.CreateBoundExpression(groupRef),
                                                    ControlVariableName,
                                                    ControlVariableType,
                                                    QueryFlags,
                                                    &FirstAddedInitializer
                                                );

        if(!FirstAddedInitializer)
        {
            // couldn't add anything, some errors should have been reported
            return AllocateBadExpression(Aggregate->TextSpan);
        }
        else if(!CheckIntoProjectionForShadowing(FirstAddedInitializer))
        {
            projectionIsGood = false;
        }

        // interpret the initializer
        boundResult = InterpretExpression(Initializer, Flags);

        if(IsBad(boundResult))
        {
            return boundResult;
        }
        else if(!projectionIsGood)
        {
            return AllocateBadExpression(Aggregate->TextSpan);
        }

        return
            AllocateExpression(
                SX_SEQ,
                boundResult->ResultType,
                assign,
                boundResult,
                Aggregate->TextSpan);
    }
    else
    {
        // Don't need to create an Anonymous Type
        ParseTree::Expression * expr;
        ParseTree::AggregationExpression * agg;
        Location initializerTextSpan;
        ParseTree::IdentifierDescriptor * Name;

        if(!GetItemFromIntoProjection
                    (
                        Aggregate->Projection,
                        expr,
                        agg,
                        Name,
                        initializerTextSpan
                    )
           )
        {
            return AllocateBadExpression(Aggregate->TextSpan); // Parser should have reported an error
        }

        if(!agg)
        {
                AssertIfTrue(ERRID_InternalCompilerError);
                ReportSemanticError(
                    ERRID_InternalCompilerError,
                    Aggregate->Projection->TextSpan);// not likely to get here
                return AllocateBadExpression(Aggregate->TextSpan); // not likely to get here
        }
        else if(agg->HaveLParen && !agg->HaveRParen)
        {
            return AllocateBadExpression(Aggregate->TextSpan); // Parser should have reported an error
        }
        else
        {
            AssertIfFalse(ParseTree::Expression::Aggregation == expr->Opcode && expr == agg && Name);

            // #60708
            if (Aggregate->Projection->Element->Opcode == ParseTree::Initializer::Assignment &&
                Name->TypeCharacter != chType_NONE)
            {
                ReportSemanticError(
                    ERRID_QueryAnonymousTypeDisallowsTypeChar,
                    Name->TextSpan);
            }

            if(!CheckControlVariableName(Name->Name, Name->TextSpan, true))
            {
                projectionIsGood = false;
            }

            // transform Aggregation expression into a call on the group

            ParseTree::Expression * aggCall = BuildAggregateFunctionCall
                                                                        (
                                                                            PH.CreateBoundExpression(Group),
                                                                            ControlVariableName,
                                                                            ControlVariableType,
                                                                            QueryFlags,
                                                                            agg
                                                                        );

            if(!aggCall)
            {
                // couldn't bind the argument
                return AllocateBadExpression(Aggregate->TextSpan); // BuildAggregateFunctionCall() should have reported an error
            }

            boundResult = InterpretExpression(aggCall, Flags);

            if(!projectionIsGood)
            {
                return AllocateBadExpression(Aggregate->TextSpan);
            }

            return boundResult;
        }
    }
}

ILTree::Expression *
Semantics::InterpretGroupForAggregateExpression
(
    _Inout_ ParseTree::QueryAggregateGroupExpression * AggGroup,
    ExpressionFlags Flags
)
{
    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;

    ILTree::Expression *Group = InterpretLinqOperatorSource(AggGroup->Group, Flags,NULL, &ControlVariableName, &ControlVariableType, &QueryFlags, NULL);

    if(!IsBad(Group))
    {
        AggGroup->ControlVariableType = ControlVariableType;
        AggGroup->ControlVariableName = ControlVariableName;
        AggGroup->QueryFlags = QueryFlags;
    }

    return Group;
}


Semantics::LambdaBodyInterpretAggregateProjection::LambdaBodyInterpretAggregateProjection
(
    _In_ Semantics * semantics,
    _In_ ParseTree::Expression * Group,
    _In_ ParseTree::IdentifierDescriptor &ElementControlVariableName,
    ParseTree::AlreadyBoundType *ElementControlVariableType,
    QueryExpressionFlags ElementQueryFlags,
    _In_opt_ ParseTree::NewObjectInitializerExpression * ItInitializer,
    ParseTree::InitializerList * Projection,
    ExpressionFlags Flags
)
{
    AssertIfNull(semantics);
    AssertIfNull(Group);
    AssertIfNull(ElementControlVariableType);
    AssertIfNull(Projection);

    m_Semantics = semantics;
    m_Group = Group;
    m_ElementControlVariableName = ElementControlVariableName;
    m_ElementControlVariableType = ElementControlVariableType;
    m_ElementQueryFlags = ElementQueryFlags;
    m_ItInitializer = ItInitializer;
    m_Projection = Projection;
    m_Flags = Flags;
    m_ProjectionIsGood = false;
    m_FirstAddedInitializer = NULL;
}


ILTree::ILNode *
Semantics::LambdaBodyInterpretAggregateProjection::DoInterpretBody()
{
    // Now, go through INTO projection and build our real selector
    m_ProjectionIsGood =
                                        m_Semantics->AppendIntoProjection
                                                (
                                                    m_ItInitializer,
                                                    m_Projection,
                                                    m_Group,
                                                    m_ElementControlVariableName,
                                                    m_ElementControlVariableType,
                                                    m_ElementQueryFlags,
                                                    &m_FirstAddedInitializer
                                                );

    if(m_ItInitializer)
    {
        if(!m_FirstAddedInitializer)
        {
            // couldn't add anything, some errors should have been reported
            return m_Semantics->AllocateBadExpression(m_ItInitializer->TextSpan);
        }

        return m_Semantics->InterpretExpression(m_ItInitializer, m_Flags | ExprForceRValue);
    }

    return m_Semantics->AllocateBadExpression(m_ElementControlVariableName.TextSpan);
}


ILTree::Expression *
Semantics::InterpretAggregateExpression
(
    _In_ ParseTree::AggregateExpression *Aggregate,
    ExpressionFlags Flags,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableTypeContainingGroup,
    _Out_opt_ ParseTree::AlreadyBoundType ** pGroupElementType
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pGroupElementType);

    if (pControlVariableTypeContainingGroup)
    {
        *pControlVariableTypeContainingGroup = NULL;
    }

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;
    ParseTree::NewObjectInitializerExpression * RecordInitializer=NULL;

    // Depending on how many items we have in the INTO clause,
    // we will rewrite our query and then interpret it
    //
    // FROM a in AA            FROM a in AA
    // OVER b in a.BB  ==   FROM count = (FROM b IN a.BB).Count()
    // INTO Count()
    //
    // FROM a in AA            FROM a in AA
    // OVER b in a.BB  ==   FROM GroupName = (FROM b IN a.BB)
    // INTO Count(),           Select a, Count=GroupName.Count(), Sum=GroupName.Sum(b=>b)
    //         Sum(b)
    //

    // Do we have to use an extra Select  (the second case)?
    bool extraSelect = (Aggregate->Projection && Aggregate->Projection->Next);
    ParserHelper PH(&m_TreeStorage, Aggregate->TextSpan);

    // Create our Let item
    ParseTree::IdentifierDescriptor GroupName = ItIdentifierDescriptor(STRING_CONST(m_Compiler, Group2));
    ParseTree::QueryAggregateGroupExpression * Group = NULL;
    ParseTree::FromExpression * Let = NULL;
    bool shouldFail = false;

    if(extraSelect)
    {
        // Group is going to be our Let expression
        // When we interpret it, we'll store information about the group (ControlVariableName, ControlVariableType, QueryFlags)
        // in the Group node (see Semantics::InterpretGroupForAggregateExpression() )
        Group = PH.CreateQueryAggregateGroupExpression(Aggregate->AggSource);

        GroupName.TextSpan = Aggregate->AggSource->TextSpan;

        Let =PH.CreateFromExpression(
                            PH.CreateLetFromItem(
                                        PH.CreateVariableDeclaration(GroupName),
                                        Group,
                                        Group->TextSpan
                                )
                        );
    }
    else
    {
        // create new AggregationExpression that is stand alone
        // it is going to be our Let expression
        ParseTree::AggregateExpression *newAggregate = new(m_TreeStorage) ParseTree::AggregateExpression;
        *newAggregate = *Aggregate;
        newAggregate->Source = NULL;

        // extract the name of the new control variable
        ParseTree::IdentifierDescriptor * Name = NULL;

        if(Aggregate->Projection)
        {
            ParseTree::Initializer *Operand = Aggregate->Projection->Element;

            if (Operand)
            {
                ParseTree::AssignmentInitializer * assignmentInitializer;
                ParseTree::ExpressionInitializer * expressionInitializer;

                switch (Operand->Opcode)
                {
                    case ParseTree::Initializer::Expression:
                        expressionInitializer = Operand->AsExpression();

                        if(ParseTree::Expression::Aggregation == expressionInitializer->Value->Opcode)
                        {
                                Name = &expressionInitializer->Value->AsAggregation()->AggFunc;
                        }
                        break;

                    case ParseTree::Initializer::Assignment:
                        assignmentInitializer = Operand->AsAssignment();
                        Name = &assignmentInitializer->Name;
                        break;
                }
            }
        }

        shouldFail = (Name==NULL);

        ParseTree::IdentifierDescriptor newName = Name ? *Name : ItIdentifierDescriptor();  // if Name==NULL we should report an error later, so any name will do

        // #60708
        if (newName.TypeCharacter != chType_NONE)
        {
            newName.TypeCharacter = chType_NONE; // don't want to report a duplicate error
        }

        ParseTree::FromItem * letFromItem =
                            PH.CreateLetFromItem(
                                        PH.CreateVariableDeclaration(newName),
                                        newAggregate,
                                        newAggregate->AggSource->TextSpan
                                );

        letFromItem->SuppressShadowingChecks = true; // do not report shadowing errors twice

        Let =PH.CreateFromExpression(letFromItem);
    }


    ParseTree::LinqExpression * Source =  Aggregate->Source;
    ParseTree::CrossJoinExpression * topCrossJoin = NULL;
    ParseTree::CrossJoinExpression * parentCrossJoin = NULL;
    ParseTree::CrossJoinExpression * CrossJoin = NULL;
    ParseTree::CrossJoinExpression * newCrossJoin = NULL;
    Location textSpan;

    // create new CrossJoin node for additional 'Let' and merge it into the Source
    while(ParseTree::Expression::CrossJoin == Source->Opcode)
    {
        CrossJoin = Source->AsCrossJoin();
        Source = CrossJoin->JoinTo;

        textSpan = CrossJoin->TextSpan;
        textSpan.SetEnd(&Aggregate->AggSource->TextSpan);

        newCrossJoin = PH.CreateCrossJoinExpression(CrossJoin->Source, NULL, textSpan);

        if(!topCrossJoin)
        {
            topCrossJoin = newCrossJoin;
        }
        else
        {
            AssertIfNull(parentCrossJoin);
            parentCrossJoin->JoinTo = newCrossJoin;
        }

        parentCrossJoin = newCrossJoin;
    }

    textSpan = Source->TextSpan;
    textSpan.SetEnd(&Aggregate->AggSource->TextSpan);

    Let->ImplicitLetforAggregate = true;
    Let->TextSpan.SetStart(Aggregate->TextSpan.m_lBegLine + Aggregate->FirstPunctuator.Line, Aggregate->FirstPunctuator.Column);
    newCrossJoin = PH.CreateCrossJoinExpression(Source, Let, textSpan);

    if(!topCrossJoin)
    {
        topCrossJoin = newCrossJoin;
    }
    else
    {
        AssertIfNull(parentCrossJoin);
        parentCrossJoin->JoinTo = newCrossJoin;
    }

    if(extraSelect)
    {

        // Add Select operator on top of our CrossJoin node and interpret it
        // We need this fake node just to make sure that InterpretLetJoin is aware that
        // we are going to apply Select after it (it shouldn't flatten control variable).
        // So we will not be setting Projection at this point because
        // 1) we don't need it to interpret the CrossJoin
        // 2) we can't create it because we don't have information about the group (ControlVariableName, ControlVariableType, QueryFlags).
        //     The information about the group is going to be stored in the Group node after we successfully interpreted the CrossJoin.
        textSpan.SetEnd(&Aggregate->TextSpan);
        ParseTree::SelectExpression * Select = PH.CreateSelectExpression(topCrossJoin, NULL, textSpan);

        // Interpret Source for our new Select operator
        ParseTree::LinqOperatorExpression * op = Select;
        ILTree::Expression *boundSource = InterpretLinqOperatorSource(Select->Source, Flags, &op, &ControlVariableName, &ControlVariableType, &QueryFlags, &RecordInitializer);

        if (IsBad(boundSource))
        {
            return boundSource;
        }

        if (pControlVariableTypeContainingGroup)
        {
            *pControlVariableTypeContainingGroup = ControlVariableType;
        }

        if (pGroupElementType)
        {
            *pGroupElementType = Group->ControlVariableType;
        }

        AssertIfNull(op);
        if(!op)
        {
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Aggregate->TextSpan); // shouldn't get here
            return AllocateBadExpression(Aggregate->TextSpan); // shouldn't get here
        }

        // Now apply Select Operator
        if(!RecordInitializer)
        {
            AssertIfTrue(HasFlag(QueryFlags, ControlVarIsRecord));
            RecordInitializer = PH.CreateAnonymousTypeInitializer(NULL, Aggregate->TextSpan, true, true);
        }
        else
        {
            RecordInitializer->TextSpan.SetEnd(&Aggregate->TextSpan);

            // we need to remove the last initializer, it is our group and we want to get rid of it
            ParseTree::InitializerList ** list = &RecordInitializer->InitialValues->InitialValues;

            AssertIfNull(*list);

            if(*list)
            {
                for(;;list = &(*list)->Next)
                {
                    if(!(*list)->Next)
                    {
                        *list = NULL;
                        break;
                    }
                }
            }
        }

        // Interpret our Select operator without using ParseTree::SelectExpression node

        // Now, go through INTO projection and interpret our real selector
        ParseTree::ParameterList * lambdaParams = PH.CreateParameterList(
                                                                        Aggregate->Projection->TextSpan,
                                                                        1,
                                                                        PH.CreateParameter(
                                                                            ControlVariableName,
                                                                            ControlVariableType, true,
                                                                            HasFlag(QueryFlags,ControlVarIsRecord)));

        LambdaBodyInterpretAggregateProjection bodyInterpreter
                                (
                                    this,
                                    PH.CreateNameExpression(GroupName),
                                    Group->ControlVariableName,
                                    Group->ControlVariableType,
                                    Group->QueryFlags,
                                    RecordInitializer,
                                    Aggregate->Projection,
                                    Flags
                                );

        ILTree::LambdaExpression * boundSelector =
                            CreateLambdaExpression(
                                        InterpretLambdaParameters(lambdaParams),
                                        Aggregate->Projection->TextSpan,
                                        Location::GetInvalidLocation(), // BodySpan, only relevant for multiline lambdas
                                        TypeHelpers::GetVoidType(),
                                        &bodyInterpreter,
                                        true, // IsFunctionLambda
                                        false, // IsAsyncKeywordUsed
                                        false, // IsIteratorKeywordUsed
                                        SingleLineLambda);

        if(IsBad(boundSelector))
        {
            return boundSelector;
        }

        boundSelector->ExactlyMatchExpressionResultType = true;

        // Check names for shadowing
        // Check names after binding because it may declare new locals for the Proc
        if(!bodyInterpreter.ProjectionIsGood() ||
            !CheckIntoProjectionForShadowing(bodyInterpreter.FirstAddedInitializer()))
        {
            return AllocateBadExpression(Aggregate->TextSpan);
        }


        if(pQueryFlags)
        {
            *pQueryFlags = AppliedAtLeastOneOperator | ControlVarIsHidden | ControlVarIsRecord;
        }

        if(pControlVariableName)
        {
            *pControlVariableName = ItIdentifierDescriptor();
        }

        if(pControlVariableType)
        {
            *pControlVariableType = PH.CreateBoundType(boundSelector->GetExpressionLambdaBody()->ResultType, Aggregate->Projection->TextSpan);
        }

        if(pRecordInitializer)
        {
            ConvertIntoProjectionToExpressions(bodyInterpreter.FirstAddedInitializer());
            * pRecordInitializer = RecordInitializer;
        }

        // bind SELECT
        AssertIfFalse(Aggregate->HaveInto);

        // Interpret our Select operator without using ParseTree::SelectExpression node
        Location intoNameLocation;
        intoNameLocation.SetLocation(Aggregate->TextSpan.m_lBegLine, &Aggregate->Into,g_tkKwdNameLengths[tkINTO]-1);

        return BindSelectOperator(boundSource, Aggregate->TextSpan, intoNameLocation, boundSelector);
    }
    else
    {
        // Just interpret our CrossJoin node passing it the following operator
        ILTree::Expression * Result = InterpretLinqOperatorSource(topCrossJoin, Flags, pFollowingOperator, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

        if(shouldFail && !IsBad(Result))
        {
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Aggregate->TextSpan); // shouldn't get here
            return AllocateBadExpression(Aggregate->TextSpan); // shouldn't get here
        }

        return Result;
    }
}


ILTree::Expression *
Semantics::InterpretGroupByClause
(
    _In_ ParseTree::GroupByExpression *Group,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _Out_opt_ ParseTree::AlreadyBoundType ** pGroupElementType
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pGroupElementType);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags=QueryNoFlags;

    ParseTree::LinqOperatorExpression * op = Group;
    ILTree::Expression *Source = InterpretLinqOperatorSource(Group->Source, Flags, &op, &ControlVariableName, &ControlVariableType, &QueryFlags, NULL);

    if (IsBad(Source))
    {
        return Source;
    }

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Group->TextSpan);
        return AllocateBadExpression(Group->TextSpan);
    }


    ParserHelper PH(&m_TreeStorage, Group->TextSpan);


    // interpret Element selector
    ParseTree::IdentifierDescriptor ElementControlVariableName;
    ParseTree::AlreadyBoundType *ElementControlVariableType = NULL;
    QueryExpressionFlags ElementQueryFlags=QueryNoFlags;
    ILTree::LambdaExpression * ElementSelector = NULL;
    ILTree::Expression *  selector;
    bool elementIsBad = false;

    if(Group->Element)
    {
        selector = InterpretInitializerSelector
                                            (
                                                Group->Element,
                                                false, // it is OK to not name element of the group
                                                ControlVariableName,
                                                ControlVariableType,
                                                QueryFlags,
                                                &ElementControlVariableName,
                                                &ElementControlVariableType,
                                                &ElementQueryFlags,
                                                NULL
                                            );


        if(IsBad(selector))
        {
            elementIsBad = true;
        }
        else if(selector->bilop != SX_LAMBDA)
        {
            AssertIfFalse(selector->bilop == SX_LAMBDA);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Group->Element->TextSpan); // shouldn't get here
            elementIsBad = true;  // shouldn't get here
        }
        else
        {
            ElementSelector = &selector->AsLambdaExpression();
        }

        // Continue interpretation even if we failed to interpret the element
        // in order to report more errors if they are present
    }
    else
    {
        // if Element isn't explicitly specified, it is the current control variable
        ElementControlVariableName = ControlVariableName;
        ElementControlVariableType = ControlVariableType;
        ElementQueryFlags = QueryFlags;
    }

    if (pGroupElementType)
    {
        *pGroupElementType = ElementControlVariableType;
    }

    // Interpret Key selector
    ParseTree::NewObjectInitializerExpression *ProjectionInitializer = NULL;
    
    ParseTree::IdentifierDescriptor KeyControlVariableName = {0};
    ParseTree::AlreadyBoundType *KeyControlVariableType = NULL;
    QueryExpressionFlags KeyQueryFlags=QueryNoFlags;
    ILTree::LambdaExpression * KeySelector = NULL;
    bool keyIsBad = false;

    if(!Group->Key)
    {
        keyIsBad = true; // parser should have reported a syntax error
    }
    else
    {
        unsigned ErrCntOld = m_Errors ? m_Errors->GetErrorCount() : 0;

        selector = InterpretInitializerSelector
                                            (
                                                Group->Key,
                                                true, // Key must always be named or the name should be inferred
                                                ControlVariableName,
                                                ControlVariableType,
                                                QueryFlags,
                                                &KeyControlVariableName,
                                                &KeyControlVariableType,
                                                &KeyQueryFlags,
                                                &ProjectionInitializer
                                            );


        unsigned ErrCntNew = m_Errors ? m_Errors->GetErrorCount() : 0;

        if(IsBad(selector) ||
            ErrCntOld<ErrCntNew) // avoid reporting duplicate errors later
        {
            keyIsBad = true;
        }
        else if(selector->bilop != SX_LAMBDA)
        {
            AssertIfFalse(selector->bilop == SX_LAMBDA);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Group->Key->TextSpan);// shouldn't get here
            keyIsBad = true; // shouldn't get here
        }
        // Check restricted types
        else if (IsRestrictedType(KeyControlVariableType->BoundType, m_CompilerHost) ||
            IsRestrictedArrayType(KeyControlVariableType->BoundType, m_CompilerHost))
        {
            ReportSemanticError(
                ERRID_RestrictedType1,
                Group->Key->TextSpan,
                KeyControlVariableType->BoundType);

            keyIsBad = true;
        }
        else
        {
            KeySelector = &selector->AsLambdaExpression();

            // rename key if it uses compiler generated name
            if(HasFlag(KeyQueryFlags, ControlVarIsHidden))
            {
                KeyControlVariableName = ItIdentifierDescriptor(STRING_CONST(m_Compiler, Key));
                KeyControlVariableName.TextSpan = Group->Key->TextSpan;
                AssertIfFalse(HasFlag(KeyQueryFlags, ControlVarIsRecord) && ProjectionInitializer);
            }
            else if(!ProjectionInitializer)
            {
                AssertIfFalse(!HasFlag(KeyQueryFlags, ControlVarIsRecord) && !HasFlag(KeyQueryFlags, ControlVarIsHidden));

                ProjectionInitializer = PH.CreateAnonymousTypeInitializer(NULL, Group->Key->TextSpan, true, true);
                PH.AddExpressionInitializer(ProjectionInitializer, PH.CreateNameExpression(KeyControlVariableName.Name, KeyControlVariableName.TextSpan));
            }
        }
    }

    ILTree::Expression * Result=NULL;
    Location GroupByLocation = Location::GetHiddenLocation();
    GroupByLocation.SetLocation(Group->TextSpan.m_lBegLine, &Group->FirstPunctuator,g_tkKwdNameLengths[tkGROUP]-1);

    if(!ElementSelector && Group->HaveBy)
    {
        Location ByLocation = {0};
        ByLocation.SetLocation(Group->TextSpan.m_lBegLine, &Group->By,g_tkKwdNameLengths[tkBY]-1);
        GroupByLocation.SetEnd(&ByLocation);
    }

    // See if customer named the group and validate our Projection

    QueryExpressionFlags GroupQueryFlags=QueryNoFlags;
    ParseTree::NameExpression * InternalGroupName=NULL;
    ParseTree::IdentifierDescriptor InternalGroupControlVariableName;

    ParseTree::Expression * GroupRef = GetGroupReferenceFromIntoProjection(Group->Projection);

    Location IntoLocation=GroupByLocation;
    InternalGroupControlVariableName = ItIdentifierDescriptor(STRING_CONST(m_Compiler, Group));

    if(GroupRef)
    {
        InternalGroupControlVariableName.TextSpan = GroupRef->TextSpan;
    }
    else
    {
        InternalGroupControlVariableName.TextSpan = IntoLocation;
    }

    InternalGroupName = PH.CreateNameExpression(InternalGroupControlVariableName);

    Type * ProjectionReturnType=NULL;

    // If Group is the only thing that gets projected by the INTO clause and the key is not a record,
    // we don't need to go through the type inference for the Group
    // This is mostly due to performance reasons.
    // Check GroupRef first because Group->Projection may be NULL
    if(GroupRef && !Group->Projection->Next && !HasFlag(KeyQueryFlags, ControlVarIsRecord))
    {
        // can't continue if we failed to interpret the element of the group or the key
        if(elementIsBad || keyIsBad)
        {
            return AllocateBadExpression(Group->TextSpan);
        }

        ParseTree::InitializerList * FirstAddedInitializer = NULL;
        bool projectionIsGood =
                        AppendIntoProjection
                                (
                                    ProjectionInitializer,
                                    Group->Projection,
                                    InternalGroupName,
                                    ElementControlVariableName,
                                    ElementControlVariableType,
                                    ElementQueryFlags,
                                    &FirstAddedInitializer
                                );

        if(!projectionIsGood || !FirstAddedInitializer ||
            FirstAddedInitializer->Element->Opcode != ParseTree::Initializer::Assignment)
        {
            // Trivial projection, should have succeeded
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Group->TextSpan); // shouldn't get here
            return AllocateBadExpression(Group->TextSpan); // shouldn't get here
        }
        else
        {
            ParseTree::IdentifierDescriptor * GrName = &FirstAddedInitializer->Element->AsAssignment()->Name;

            // Make sure there is no name clash
            if(StringPool::IsEqual(GrName->Name,KeyControlVariableName.Name, false))
            {
                ReportSemanticError(ERRID_QueryDuplicateAnonTypeMemberName1, GrName->TextSpan, GrName->Name);
                return AllocateBadExpression(Group->TextSpan);
            }
        }

        ProjectionInitializer->TextSpan.SetEnd(&Group->Projection->TextSpan);

        Result = BindGroupByOperator
                        (
                            Source,
                            Group->TextSpan,
                            GroupByLocation,
                            ElementSelector,
                            KeySelector,
                            KeyControlVariableName,
                            KeyControlVariableType,
                            KeyQueryFlags,
                            InternalGroupControlVariableName,
                            ProjectionInitializer
                        );

        if(IsBad(Result))
        {
            return Result;
        }

        // Check names for shadowing and other restrictions
        // Check names after BindGroupByOperator() because it may declare new locals for the Proc
        if(!CheckIntoProjectionForShadowing(FirstAddedInitializer))
        {
            return AllocateBadExpression(Group->TextSpan);
        }

        // Adjust our ProjectionInitializer so that we can return it
        if(pRecordInitializer)
        {
            ConvertIntoProjectionToExpressions(FirstAddedInitializer);
        }

        // Control variable is the return value of the final selector, which is the last lambda passed to the GroupBy operator.
        ProjectionReturnType = GetControlVariableTypeAfterGroupBy(Result);

        AssertIfNull(ProjectionReturnType);

        if(!ProjectionReturnType)
        {
            AssertIfNull(ProjectionReturnType);
            ReportSemanticError( // shouldn't get here
                ERRID_InternalCompilerError,
                Group->TextSpan);
            return AllocateBadExpression(Group->TextSpan);
        }
    }
    else
    {
        // Now, it is the time to infer the type for our group.
        // To do this, we will try to bind GroupBy method using
        // simple selector {group} and then extruct the type from
        // Lambda's return type.
        // Using Anonymous Type will ensure that we will bind to the same GroupBy
        // method with the real selector because the real selector will be an Anonymous Type too.
        Type * GroupType = NULL;

        // can't do the inference if we failed to interpret the element of the group or the key
        if(!elementIsBad && !keyIsBad)
        {
            ParseTree::NewObjectInitializerExpression *inferenceProjection;
            STRING * memberName = STRING_CONST(m_Compiler, GroupByMethod); // we need to use VB legal name here otherwise Anonymous Type will choke
            ParseTree::IdentifierDescriptor inferenceControlVariableName = ItIdentifierDescriptor(memberName);
            inferenceControlVariableName.TextSpan = InternalGroupControlVariableName.TextSpan;

            inferenceProjection = PH.CreateAnonymousTypeInitializer(NULL, Group->TextSpan, true, true);
            PH.AddExpressionInitializer(inferenceProjection,  PH.CreateNameExpression(memberName, InternalGroupControlVariableName.TextSpan));

            BackupValue<ErrorTable *> backup_error_table(&m_Errors);
            ErrorTable * pTemporaryErrorTable=NULL;

            if(m_Errors)
            {
                pTemporaryErrorTable = new  ErrorTable(*m_Errors);
                m_Errors = pTemporaryErrorTable;
            }

            ParseTree::IdentifierDescriptor iKey = ItIdentifierDescriptor(STRING_CONST(m_Compiler, Key)); // Use compiler generated name to avoid clash with inferenceControlVariableName
            iKey.TextSpan = KeyControlVariableName.TextSpan;
            Result = BindGroupByOperator
                            (
                                Source,
                                Group->TextSpan,
                                GroupByLocation,
                                ElementSelector,
                                KeySelector,
                                ItIdentifierDescriptor(STRING_CONST(m_Compiler, Key)), // Use compiler generated name to avoid clash with inferenceControlVariableName
                                KeyControlVariableType,
                                KeyQueryFlags,
                                inferenceControlVariableName,
                                inferenceProjection
                            );

            backup_error_table.Restore();

            if(IsBad(Result) && m_Errors)
            {
                m_Errors->MergeTemporaryTable(pTemporaryErrorTable);
            }

            if(pTemporaryErrorTable)
            {
                delete pTemporaryErrorTable;
            }

            if(!IsBad(Result))
            {
                // The group is the only member of an Anonymous Type returned by the last lambda passed to the GroupBy operator.
                 GroupType = GetTypeOfTheGroup(Result, memberName, InternalGroupControlVariableName.TextSpan);

                if(!GroupType)
                {
                    AssertIfNull(GroupType);
                    ReportSemanticError( // shouldn't get here
                        ERRID_InternalCompilerError,
                        Group->TextSpan); // shouldn't get here
                }
            }
        }

        // can't continue if we failed to interpret the element of the group and failed to infer the type of the group
        if(elementIsBad && !GroupType)
        {
            return AllocateBadExpression(Group->TextSpan);
        }

        AssertIfFalse(!GroupType ||(!keyIsBad && ProjectionInitializer));

        // Now, go through INTO projection and build our real selector
        ParseTree::InitializerList * FirstAddedInitializer = NULL;
        bool projectionIsGood =
                                            AppendIntoProjection
                                                    (
                                                        GroupType ? ProjectionInitializer : NULL, // pass NULL if we only want to validate the projection and report errors
                                                        Group->Projection,
                                                        InternalGroupName,
                                                        ElementControlVariableName,
                                                        !elementIsBad ? ElementControlVariableType : NULL, // pass NULL to not try to interpret Agg arguments
                                                        ElementQueryFlags,
                                                        &FirstAddedInitializer
                                                    );

        if(!FirstAddedInitializer)
        {
            // couldn't add anything, some errors should have been reported
            return AllocateBadExpression(Group->TextSpan);
        }

        AssertIfFalse(!projectionIsGood || (GroupType && !elementIsBad && !keyIsBad));

        // interpret our final selector
        ProjectionInitializer->TextSpan.SetEnd(&Group->Projection->TextSpan);

        ILTree::LambdaExpression * boundProjection =
                                BindSelectSelector(
                                    ProjectionInitializer,
                                    KeyControlVariableName,
                                    KeyControlVariableType,
                                    KeyQueryFlags,
                                    InternalGroupControlVariableName,
                                    PH.CreateBoundType(GroupType, IntoLocation),
                                    GroupQueryFlags
                                    );

        if(IsBad(boundProjection))
        {
            return boundProjection;
        }
        else if(elementIsBad || keyIsBad || !GroupType || !projectionIsGood)
        {
            return AllocateBadExpression(Group->TextSpan);
        }

        // Check names for shadowing
        // Check names after BindSelectSelector() because it may declare new locals for the Proc
        if(!CheckIntoProjectionForShadowing(FirstAddedInitializer))
        {
            return AllocateBadExpression(Group->TextSpan);
        }

        ProjectionReturnType = boundProjection->GetExpressionLambdaBody()->ResultType;

        // Bind final GroupBy call
        Result = BindGroupByOperator
                        (
                            Source,
                            Group->TextSpan,
                            GroupByLocation,
                            ElementSelector,
                            KeySelector,
                            boundProjection
                        );

        if(IsBad(Result))
        {
            AssertIfTrue(IsBad(Result));
            return Result; // shouldn't get here
        }

        // Adjust our ProjectionInitializer so that we can return it
        if(pRecordInitializer)
        {
            ConvertIntoProjectionToExpressions(FirstAddedInitializer);
        }
    }

    AssertIfFalse(ProjectionReturnType);

    if(pQueryFlags)
    {
        *pQueryFlags = AppliedAtLeastOneOperator | ControlVarIsHidden | ControlVarIsRecord;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ItIdentifierDescriptor();
    }

    if(pControlVariableType)
    {
        *pControlVariableType = PH.CreateBoundType(ProjectionReturnType, ProjectionInitializer->TextSpan);
    }

    if(pRecordInitializer)
    {
        * pRecordInitializer = ProjectionInitializer;
    }

    return Result;
}


void
Semantics::ConvertIntoProjectionToExpressions
(
    ParseTree::InitializerList * FirstAddedInitializer
)
{
    ParseTree::InitializerList *Initializers;
    ParseTree::AssignmentInitializer * assignmentInitializer;

    ParserHelper PH(&m_TreeStorage, FirstAddedInitializer->TextSpan);

    for (Initializers = FirstAddedInitializer;
         Initializers;
         Initializers = Initializers->Next)
    {
        ParseTree::Initializer *Operand = Initializers->Element;

        AssertIfNull(Operand);

        if (ParseTree::Initializer::Assignment ==  Operand->Opcode)
        {
            assignmentInitializer = Operand->AsAssignment();
            PH.ChangeToExpressionInitializer(Initializers, PH.CreateNameExpression( assignmentInitializer->Name));
        }
    }
}


bool
Semantics::CheckIntoProjectionForShadowing
(
    ParseTree::InitializerList * FirstAddedInitializer
)
{
    ParseTree::InitializerList *Initializers;
    ParseTree::AssignmentInitializer * assignmentInitializer;
    ParseTree::ExpressionInitializer * expressionInitializer;
    ParseTree::Expression * expr;
    ParseTree::IdentifierDescriptor * Name;
    bool result = true;

    for (Initializers = FirstAddedInitializer;
         Initializers;
         Initializers = Initializers->Next)
    {
        ParseTree::Initializer *Operand = Initializers->Element;

        AssertIfNull(Operand);

        Name = NULL;

        switch (Operand->Opcode)
        {
            case ParseTree::Initializer::Expression:
                expressionInitializer = Operand->AsExpression();
                expr = expressionInitializer->Value;

                if(ParseTree::Expression::Name == expressionInitializer->Value->Opcode)
                {
                    Name = &expr->AsName()->Name;
                }
                break;

            case ParseTree::Initializer::Assignment:
                assignmentInitializer = Operand->AsAssignment();
                Name = &assignmentInitializer->Name;
                break;
        }

        if (Name)
        {
            if(!CheckControlVariableName(Name->Name, Name->TextSpan, false))
            {
                result = false;
            }
        }
        else
        {
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Initializers->TextSpan); // shouldn't get here
            result = false; // shouldn't get here
        }
    }

    return result;
}

bool
Semantics::AppendIntoProjection
(
    _In_opt_ ParseTree::NewObjectInitializerExpression *ProjectionInitializer,
    ParseTree::InitializerList * Projection,
    _In_ ParseTree::Expression * GroupReference,
    _In_ ParseTree::IdentifierDescriptor &ElementControlVariableName,
    ParseTree::AlreadyBoundType *ElementControlVariableType,
    QueryExpressionFlags ElementQueryFlags,
    _Out_opt_ ParseTree::InitializerList ** pFirstAddedInitializer
)
{
    ParseTree::InitializerList * FirstAddedInitializer = NULL;
    bool result = (Projection!=NULL);

    if(result)
    {
        ParseTree::InitializerList *Initializers;
        ParseTree::Expression * expr;
        ParseTree::AggregationExpression * agg;

        ParserHelper PH(&m_TreeStorage, Projection->TextSpan);

        ParseTree::InitializerList * newInitializer = NULL;
        Location initializerTextSpan;
        ParseTree::IdentifierDescriptor * Name;

        for (Initializers = Projection;
             Initializers;
             Initializers = Initializers->Next)
        {
            newInitializer = NULL;

            if(!GetItemFromIntoProjection
                        (
                            Initializers,
                            expr,
                            agg,
                            Name,
                            initializerTextSpan
                        )
               )
            {
                result =  false; // Parser should have reported an error
                continue;
            }

            if(!expr || (ParseTree::Expression::GroupRef!= expr->Opcode && !agg))
            {
                    AssertIfTrue(ERRID_InternalCompilerError);
                    ReportSemanticError(
                        ERRID_InternalCompilerError,
                        Initializers->TextSpan);// not likely to get here
                    result = false; // not likely to get here
                    continue;
            }
            else
            if(ParseTree::Expression::GroupRef== expr->Opcode)
            {
                AssertIfFalse(!agg);
                AssertIfFalse(GroupReference != expr);

                if(ProjectionInitializer)
                {
                    ParseTree::IdentifierDescriptor GrName;

                    if(!Name)
                    {
                        GrName=PH.CreateIdentifierDescriptor(m_Compiler->TokenToString(tkGROUP), initializerTextSpan);
                        Name = &GrName;
                    }
                    else if (Name->TypeCharacter != chType_NONE)
                    {
                        ReportSemanticError(
                            ERRID_QueryAnonymousTypeDisallowsTypeChar,
                            Name->TextSpan);

                        GrName = *Name;
                        Name = &GrName;
                        Name->TypeCharacter = chType_NONE;
                    }

                    newInitializer = PH.AddAssignmentInitializer( ProjectionInitializer, Name, GroupReference, initializerTextSpan);
                }
            }
            else if(agg->HaveLParen && !agg->HaveRParen)
            {
                    result = false; // Parser should have reported a syntax error
                    continue;
            }
            else
            {
                AssertIfFalse(ParseTree::Expression::Aggregation == expr->Opcode && expr == agg && Name);

                // transform Aggregation expression into a call on the group

                ParseTree::Expression * aggCall = BuildAggregateFunctionCall
                                                                            (
                                                                                GroupReference,
                                                                                ElementControlVariableName,
                                                                                ElementControlVariableType,
                                                                                ElementQueryFlags,
                                                                                agg
                                                                            );

                if(!aggCall)
                {
                    // couldn't bind argument
                    result = false;
                    continue;
                }

                ParseTree::IdentifierDescriptor newName = *Name;

                if (Initializers->Element->Opcode == ParseTree::Initializer::Expression)
                {
                    newName.TypeCharacter =  chType_NONE; // Don't want to report duplicate errors if the name is inferred
                }


                if(ProjectionInitializer)
                {
                    newInitializer = PH.AddAssignmentInitializer( ProjectionInitializer, &newName, aggCall, initializerTextSpan);
                }
            }

            if(!newInitializer)
            {
                result = false;
            }
            else
            if(!FirstAddedInitializer)
            {
                FirstAddedInitializer = newInitializer;
            }
        }
    }

    AssertIfFalse(!result || FirstAddedInitializer);

    if(pFirstAddedInitializer)
    {
        *pFirstAddedInitializer = FirstAddedInitializer;
    }

    return  result;
}

// returns false if an error is already reported
bool
Semantics::GetItemFromIntoProjection
(
    ParseTree::InitializerList * Initializers,
    _Out_ ParseTree::Expression *& expr,
    _Out_ ParseTree::AggregationExpression * & agg,
    _Out_ ParseTree::IdentifierDescriptor * &Name,
    _Out_ Location & initializerTextSpan
)
{
    ParseTree::Initializer *Operand = Initializers->Element;
    ParseTree::AssignmentInitializer * assignmentInitializer;
    ParseTree::ExpressionInitializer * expressionInitializer;

    expr = NULL;
    agg = NULL;
    Name = NULL;

    if (Operand)
    {
        switch (Operand->Opcode)
        {
            case ParseTree::Initializer::Expression:
                expressionInitializer = Operand->AsExpression();

                if(ParseTree::Expression::GroupRef == expressionInitializer->Value->Opcode)
                {
                    expr = expressionInitializer->Value;
                    initializerTextSpan = expr->TextSpan;
                }
                else if(ParseTree::Expression::Aggregation == expressionInitializer->Value->Opcode)
                {
                    expr = expressionInitializer->Value;
                    initializerTextSpan = expr->TextSpan;
                    agg = expr->AsAggregation();
                    Name = &agg->AggFunc;
                }
                else if(ParseTree::Expression::SyntaxError == expressionInitializer->Value->Opcode)
                {
                    return  false; // Parser should have reported an error
                }

                break;

            case ParseTree::Initializer::Assignment:
                assignmentInitializer = Operand->AsAssignment();
                initializerTextSpan = assignmentInitializer->TextSpan;

                Name = &assignmentInitializer->Name;

                if(assignmentInitializer->Initializer && ParseTree::Initializer::Expression == assignmentInitializer->Initializer->Opcode)
                {
                    expressionInitializer = assignmentInitializer->Initializer->AsExpression();

                    if(ParseTree::Expression::Aggregation == expressionInitializer->Value->Opcode)
                    {
                        expr = expressionInitializer->Value;
                        agg = expr->AsAggregation();
                    }
                    else if(ParseTree::Expression::GroupRef == expressionInitializer->Value->Opcode)
                    {
                        expr = expressionInitializer->Value;
                    }
                    else if(ParseTree::Expression::SyntaxError == expressionInitializer->Value->Opcode)
                    {
                        return  false; // Parser should have reported an error
                    }
                }
                break;
        }
    }

    return true;
}


ParseTree::Expression *
Semantics::GetGroupReferenceFromIntoProjection
(
    ParseTree::InitializerList * Projection
)
{
    ParseTree::InitializerList *Initializers;
    ParseTree::ExpressionInitializer * expressionInitializer;

    for (Initializers = Projection;
         Initializers;
         Initializers = Initializers->Next)
    {
        ParseTree::Initializer *Operand = Initializers->Element;

        if (Operand)
        {
            expressionInitializer = NULL;

            if(ParseTree::Initializer::Expression == Operand->Opcode)
            {
                expressionInitializer = Operand->AsExpression();
            }
            else if(ParseTree::Initializer::Assignment == Operand->Opcode)
            {
                ParseTree::AssignmentInitializer * assignmentInitializer;
                assignmentInitializer = Operand->AsAssignment();

                if(ParseTree::Initializer::Expression == assignmentInitializer->Initializer->Opcode)
                {
                    expressionInitializer = assignmentInitializer->Initializer->AsExpression();
                }
            }

            if(expressionInitializer && ParseTree::Expression::GroupRef == expressionInitializer->Value->Opcode)
            {
                return expressionInitializer->Value;
            }
        }
    }

    return NULL;
}


bool
Semantics::IsOKToNotFlattenBeforeGroupBy
(
    _In_ ParseTree::GroupByExpression *Group
)
{
    // It is OK to not flatten when (all true)
    // 1) There is an Element selector or Group is not returned, i.e. source element is not returned by the GroupBy
    // 2) There are no more than two Lambdas use source element as an argument

    if(!Group->Element)
    {
        if(!Group->Key || !Group->Projection)
        {
            return false; // not likely to get here
        }

        return IsOKToNotFlattenBeforeInto(Group->Projection, true);
    }
    //else
    //{
        // When there is an Element selector, there are only two Lambds: Element selector and Key selector
    //}

    return true;
}


bool
Semantics::IsOKToNotFlattenBeforeInto
(
    ParseTree::InitializerList *Projection,
    bool IsGroupBy
)
{
    // It is OK to not flatten when (all true)
    // 1) Group is not returned
    // 2) There are no more than two Lambdas use source element as an argument

    ParseTree::InitializerList *Initializers;
    ParseTree::AssignmentInitializer * assignmentInitializer;
    ParseTree::ExpressionInitializer * expressionInitializer;
    ParseTree::Expression * expr;
    ParseTree::AggregationExpression * agg;
    bool GroupIsProjected = false;
    int countOfLambdasInProjection = 0;

    for (Initializers = Projection;
         Initializers;
         Initializers = Initializers->Next)
    {
        ParseTree::Initializer *Operand = Initializers->Element;

        expr = NULL;
        agg = NULL;

        if (Operand)
        {
            switch (Operand->Opcode)
            {
                case ParseTree::Initializer::Expression:
                    expressionInitializer = Operand->AsExpression();

                    if(ParseTree::Expression::Aggregation == expressionInitializer->Value->Opcode)
                    {
                            expr = expressionInitializer->Value;
                            agg = expr->AsAggregation();
                    }
                    break;

                case ParseTree::Initializer::Assignment:
                    assignmentInitializer = Operand->AsAssignment();

                    if(ParseTree::Initializer::Expression == assignmentInitializer->Initializer->Opcode)
                    {
                        expressionInitializer = assignmentInitializer->Initializer->AsExpression();

                        if(ParseTree::Expression::Aggregation == expressionInitializer->Value->Opcode)
                        {
                            expr = expressionInitializer->Value;
                            agg = expr->AsAggregation();
                        }
                    }
                    break;
            }
        }

        if(agg)
        {
            if(agg->Argument)
            {
                countOfLambdasInProjection++;
            }
        }
        else
        {
            GroupIsProjected = true;
        }
    }

    if(GroupIsProjected)
    {
        return false;
    }
    else if(IsGroupBy)
    {
        if (countOfLambdasInProjection>1) // Key selector is one Lambda, this limits us to only one Lambda in Projection
        {
            return false;
        }
    }
    else if (countOfLambdasInProjection>2) // this limits us to only two Lambdas in Projection
    {
        return false;
    }

    return true;
}


ParseTree::Expression *
Semantics::BuildAggregateFunctionCall
(
    _In_opt_ ParseTree::Expression * GroupReference,
    _In_ ParseTree::IdentifierDescriptor &ElementControlVariableName,
    ParseTree::AlreadyBoundType *ElementControlVariableType,
    QueryExpressionFlags ElementQueryFlags,
    _In_ ParseTree::AggregationExpression * agg
)
{
    ParserHelper PH(&m_TreeStorage, agg->TextSpan);

    ParseTree::ArgumentList * arguments=NULL;

    if(agg->Argument)
    {
        if(!ElementControlVariableType)
        {
            // the caller simply wanted us to verify Aggregate calls without arguments
            return NULL; // not likely to get in this situation
        }

        ILTree::LambdaExpression * aggParam =
                                BindSelectSelector(
                                    agg->Argument,
                                    ElementControlVariableName,
                                    ElementControlVariableType,
                                    ElementQueryFlags);

        if(IsBad(aggParam))
        {
            return NULL;
        }

        // should allow conversions
        aggParam->ExactlyMatchExpressionResultType = false;

        arguments = PH.CreateArgList(
                                1,
                                PH.CreateBoundExpression(aggParam));

    }

    if(agg->AggFunc.IsBad)
    {
        return NULL;
    }

    ParseTree::IdentifierDescriptor AggFunc = agg->AggFunc;

    if (AggFunc.TypeCharacter != chType_NONE)
    {
        ReportSemanticError(
            ERRID_TypeCharOnAggregation,
            AggFunc.TextSpan);

        AggFunc.TypeCharacter = chType_NONE;
    }

    ParseTree::CallOrIndexExpression * aggCall =
                                                                        PH.CreateMethodCall(
                                                                                PH.CreateQualifiedExpression(
                                                                                            GroupReference,
                                                                                            PH.CreateNameExpression(AggFunc),
                                                                                            agg->AggFunc.TextSpan),
                                                                                arguments);


    return PH.CreateQueryOperatorCall(aggCall);
}


Type *
Semantics::GetControlVariableTypeAfterGroupBy
(
    ILTree::Expression * GroupByCall
)
{
    // Control variable is the return value of the final selector, which is the last lambda passed to the GroupBy operator.

    // find the last argument for the GroupBy and get our type from there
    if(GroupByCall && SX_CALL == GroupByCall->bilop)
    {
        ILTree::CallExpression &call = GroupByCall->AsCallExpression();
        ExpressionList *Argument;

        for (Argument = call.Right;
             Argument && SX_LIST == Argument->bilop && Argument->AsExpressionWithChildren().Right;
             Argument = Argument->AsExpressionWithChildren().Right)
        {
        }

        if(Argument && SX_LIST == Argument->bilop)
        {
            ILTree::Expression * arg = Argument->AsExpressionWithChildren().Left;

            // the arg must be a Lambda or a Widenning of a Lambda
            while(arg && SX_WIDE_COERCE == arg->bilop)
            {
                arg = arg->AsExpressionWithChildren().Left;
            }

            if(arg && SX_LAMBDA == arg->bilop)
            {
                ILTree::LambdaExpression & Lambda = arg->AsLambdaExpression();

                if(Lambda.GetExpressionLambdaBody())
                {
                    return Lambda.GetExpressionLambdaBody()->ResultType;
                }
            }
        }
    }

    return NULL; // not likely to get here
}


Type *
Semantics::GetTypeOfTheGroup
(
    ILTree::Expression * GroupByCall,
    _In_z_ STRING * memberName,
    Location & memberNameTextSpan
)
{
    // Get the Anonymous Type returned by our lambda
    return GetTypeOfTheGroup(
        GetControlVariableTypeAfterGroupBy(GroupByCall),
        memberName,
        memberNameTextSpan);
}

Type *
Semantics::GetTypeOfTheGroup
(
    Type* ProjectionReturnType,
    _In_z_ STRING * memberName,
    Location & memberNameTextSpan
)
{
    // The group is a member of an Anonymous Type returned by the last lambda passed to the GroupBy operator.
    // And the name of the member is passed in memberName.

    Type * GroupType = NULL;

    // Get Group's type from the property
    if(ProjectionReturnType)
    {
        bool NameIsBad = false;
        GenericBinding * GenericBindingContext=NULL;
        Scope * pLookup = NULL;
        GenericParameter * pGenericParameter = NULL;

        ProjectionReturnType->GetBindingSource(m_CompilerHost, &pLookup, &pGenericParameter);

        Symbol * NameBinding =
            InterpretName
            (
                memberName,
                pLookup,
                pGenericParameter,
                (
                    NameSearchIgnoreParent |
                    NameSearchIgnoreImports
                ),
                ContainingClass(),
                memberNameTextSpan,
                NameIsBad,
                &GenericBindingContext,
                -1
            );

        if (NameBinding && ! NameIsBad)
        {
            if (IsProcedure(NameBinding))
            {
                Procedure * proc = ViewAsProcedure(NameBinding);

                if(proc->IsProperty() && !proc->IsShared())
                {
                    GroupType = proc->GetType();

                    UpdateNameLookupGenericBindingContext(NameBinding, ProjectionReturnType, &GenericBindingContext);

                    GroupType = ReplaceGenericParametersWithArguments(GroupType, GenericBindingContext, m_SymbolCreator);
                }
            }
        }
    }

    return GroupType;
}

ILTree::UnboundLambdaExpression *
Semantics::BuildUnboundLambdaForIntoProjection
(
    _In_ ParseTree::IdentifierDescriptor &keyControlVariableName,
    ParseTree::AlreadyBoundType *keyControlVariableType,
    QueryExpressionFlags keyQueryFlags,
    _In_ ParseTree::IdentifierDescriptor &groupControlVariableName,
    _In_opt_ ParseTree::NewObjectInitializerExpression *Projection
)
{
    ParserHelper PH(&m_TreeStorage, Projection->TextSpan);


    ParseTree::ParameterList * parameters =
                                                PH.CreateParameterList(
                                                            Projection->TextSpan,
                                                            2,
                                                            PH.CreateParameter(
                                                                        keyControlVariableName,
                                                                        keyControlVariableType,
                                                                        Projection->TextSpan,
                                                                        true, HasFlag(keyQueryFlags, ControlVarIsRecord)),
                                                            PH.CreateParameter(
                                                                        groupControlVariableName,
                                                                        NULL,  // Don't know the type yet
                                                                        Projection->TextSpan,
                                                                        true, false));


    ParseTree::LambdaExpression * selector =
                                    PH.CreateSingleLineLambdaExpression(
                                                parameters,
                                                Projection,
                                                Projection->TextSpan,
                                                true
                                        );

    selector->MethodFlags |=  DECLF_AllowParamTypingInconsistency;

    ILTree::UnboundLambdaExpression * unboundLambda = InterpretLambdaExpression(selector, ExprForceRValue);

    if (IsBad(unboundLambda))
    {
        return unboundLambda; // not likely to get here
    }

    unboundLambda->ExactlyMatchExpressionResultType = true;

    return  unboundLambda;
}

ILTree::Expression *
Semantics::BindGroupByOperator
(
    ILTree::Expression * Source,
    _In_ Location & GroupByTextSpan,
    _In_ Location & GroupByNameLocation,
    ILTree::LambdaExpression * boundElementSelector,
    ILTree::LambdaExpression * boundKeySelector,
    _In_ ParseTree::IdentifierDescriptor &keyControlVariableName,
    ParseTree::AlreadyBoundType *keyControlVariableType,
    QueryExpressionFlags keyQueryFlags,
    _In_ ParseTree::IdentifierDescriptor &groupControlVariableName,
    _In_opt_ ParseTree::NewObjectInitializerExpression *Projection
)
{
    ILTree::UnboundLambdaExpression * unboundLambda =
                                                        BuildUnboundLambdaForIntoProjection
                                                        (
                                                            keyControlVariableName,
                                                            keyControlVariableType,
                                                            keyQueryFlags,
                                                            groupControlVariableName,
                                                            Projection
                                                        );

    if (IsBad(unboundLambda))
    {
        return unboundLambda; // not likely to get here
    }

    return BindGroupByOperator
                    (
                        Source,
                        GroupByTextSpan,
                        GroupByNameLocation,
                        boundElementSelector,
                        boundKeySelector,
                        unboundLambda
                    );
}


ILTree::Expression *
Semantics::BindGroupByOperator
(
    ILTree::Expression * Source,
    _In_ Location & GroupByTextSpan,
    _In_ Location & GroupByNameLocation,
    ILTree::LambdaExpression * boundElementSelector,
    ILTree::LambdaExpression * boundKeySelector,
    ILTree::Expression *ProjectionLambda
)
{
    if(!(SX_LAMBDA == ProjectionLambda->bilop || SX_UNBOUND_LAMBDA == ProjectionLambda->bilop))
    {
        AssertIfFalse(SX_LAMBDA == ProjectionLambda->bilop || SX_UNBOUND_LAMBDA == ProjectionLambda->bilop);
        ReportSemanticError( // should never get here
            ERRID_InternalCompilerError,
            GroupByTextSpan);
        return AllocateBadExpression(GroupByTextSpan);
    }

    ILTree::Expression * Result;
    ParserHelper PH(&m_TreeStorage, GroupByTextSpan);

    ParseTree::ArgumentList * arguments=
                        PH.CreateArgList(
                                    1,
                                    PH.CreateBoundExpression(boundKeySelector));

    ParseTree::ArgumentList * last = arguments;

    if(boundElementSelector)
    {
        last = PH.AddArgument(last, PH.CreateBoundExpression(boundElementSelector));
    }

    last = PH.AddArgument(last, PH.CreateBoundExpression(ProjectionLambda));

    Result =
        InterpretQueryOperatorCall(
            PH.CreateMethodCall(
                        PH.CreateQualifiedExpression(
                                    PH.CreateBoundExpression(Source),
                                    PH.CreateNameExpression(STRING_CONST(m_Compiler, GroupByMethod), GroupByNameLocation),
                                    GroupByNameLocation),
                        arguments),
            ExprForceRValue);

    if (IsBad(Result))
    {
        return Result;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretSelectExpression
(
    _In_ ParseTree::SelectExpression *Select,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer
)
{

    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags = QueryNoFlags;

    // optimization to use SelectMany Selector when possible instead of an extra Select operator
    ParseTree::LinqOperatorExpression * selectAsOperator = Select;
    ILTree::Expression *Source = InterpretLinqOperatorSource(Select->Source, Flags, &selectAsOperator,  &ControlVariableName, &ControlVariableType, &QueryFlags, pRecordInitializer);

    if (IsBad(Source))
    {
        return Source;
    }

    if(!selectAsOperator)
    {
        // the Select was merged with the last SelectMany
        if(pControlVariableType)
        {
            *pControlVariableType = ControlVariableType;
        }

        if(pControlVariableName)
        {
            *pControlVariableName = ControlVariableName;
        }

        if(pQueryFlags)
        {
            *pQueryFlags = QueryFlags;
        }

        return Source;
    }

    ILTree::Expression *Result = NULL;

    ILTree::Expression * selector = InterpretSelectSelector
                                        (
                                            Select,
                                            ControlVariableName,
                                            ControlVariableType,
                                            QueryFlags,
                                            pControlVariableName,
                                            pControlVariableType,
                                            pQueryFlags,
                                            pRecordInitializer
                                        );


    if(IsBad(selector))
    {
        return selector;
    }
    else if(selector->bilop != SX_LAMBDA)
    {
        AssertIfFalse(selector->bilop == SX_LAMBDA);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Select->TextSpan);
        return AllocateBadExpression(Select->TextSpan);
    }

    ILTree::LambdaExpression * boundSelector = &selector->AsLambdaExpression();

    Location selectNameLocation = {0};
    selectNameLocation.SetLocation(Select->TextSpan.m_lBegLine, &Select->FirstPunctuator,g_tkKwdNameLengths[tkSELECT]-1);

    Result = BindSelectOperator(Source, Select->TextSpan, selectNameLocation, boundSelector);

    if(pQueryFlags)
    {
        *pQueryFlags |= AppliedAtLeastOneOperator;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretSelectSelector
(
    _In_ ParseTree::SelectExpression *Select,
    ParseTree::IdentifierDescriptor ControlVariableName,
    ParseTree::AlreadyBoundType *ControlVariableType,
    QueryExpressionFlags QueryFlags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _In_opt_ ParseTree::IdentifierDescriptor *pControlVariableName2,
    ParseTree::AlreadyBoundType *ControlVariableType2,
    QueryExpressionFlags QueryFlags2
)
{
    ParseTree::InitializerList * Projection = Select->Projection;

    if(!Projection)
    {
        return AllocateBadExpression(Select->TextSpan);
    }

    return InterpretInitializerSelector
                (
                    Projection,
                    Select->ForceNameInferenceForSingleElement,
                    ControlVariableName,
                    ControlVariableType,
                    QueryFlags,
                    pControlVariableName,
                    pControlVariableType,
                    pQueryFlags,
                    pRecordInitializer,
                    pControlVariableName2,
                    ControlVariableType2,
                    QueryFlags2
                );
}

ILTree::Expression *
Semantics::InterpretInitializerSelector
(
    _In_ ParseTree::InitializerList * Projection,
    bool ForceNameInferenceForSingleElement,
    ParseTree::IdentifierDescriptor ControlVariableName,
    ParseTree::AlreadyBoundType *ControlVariableType,
    QueryExpressionFlags QueryFlags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
    _In_opt_ ParseTree::IdentifierDescriptor *pControlVariableName2,
    ParseTree::AlreadyBoundType *ControlVariableType2,
    QueryExpressionFlags QueryFlags2
)
{
    ParserHelper PH(&m_TreeStorage, Projection->TextSpan);
    ILTree::LambdaExpression * boundSelector = NULL;
    bool SingleExpressionCase=false;
    ParseTree::IdentifierDescriptor * pNewControlVariableName=NULL;
    NorlsAllocator Scratch(NORLSLOC);
    ULONG FieldCount = 0;
    MemberInfoList *MemberInfos = NULL;
    ParseTree::Expression *SelectorExpression=NULL;

    //
    // SELECT clause
    //
    if( !Projection->Next &&
        Projection->Element) // single element case
    {
        SingleExpressionCase = true;

        if(pRecordInitializer)
        {
            *pRecordInitializer = NULL;
        }

        ParseTree::Initializer *Initializer = Projection->Element;
        bool IsNameBangQualified = false;
        bool IsXMLNameRejectedAsBadVBIdentifier = false;
        ParseTree::XmlNameExpression *XMLNameInferredFrom = NULL;

        switch (Initializer->Opcode)
        {
            case ParseTree::Initializer::Expression:
                pNewControlVariableName =
                    ExtractAnonTypeMemberName(
                        Initializer->AsExpression()->Value,
                        IsNameBangQualified,
                        IsXMLNameRejectedAsBadVBIdentifier,
                        XMLNameInferredFrom);

                SelectorExpression = Initializer->AsExpression()->Value;

                if(ForceNameInferenceForSingleElement && ParseTree::Expression::SyntaxError != SelectorExpression->Opcode &&
                    !(pNewControlVariableName && !pNewControlVariableName->IsBad && pNewControlVariableName->Name))
                {
                    if (IsXMLNameRejectedAsBadVBIdentifier && XMLNameInferredFrom)
                    {
                        ReportSemanticError(ERRID_QueryAnonTypeFieldXMLNameInference,
                                                        XMLNameInferredFrom->TextSpan);
                    }
                    else
                    {
                        ReportSemanticError(ERRID_QueryAnonymousTypeFieldNameInference,
                                                        SelectorExpression->TextSpan);
                    }

                    return AllocateBadExpression(Projection->TextSpan);
                }

                break;

            case ParseTree::Initializer::Assignment:
                pNewControlVariableName = &Initializer->AsAssignment()->Name;
                Initializer = Initializer->AsAssignment()->Initializer;

                // the initializer must be an ParseTree::Initializer::Expression, that is how parser works
                if(Initializer &&
                    ParseTree::Initializer::Expression == Initializer->Opcode)
                {
                    SelectorExpression = Initializer->AsExpression()->Value;
                }
                break;
        }
    }
    else // build AnonymousType
    {
        ParseTree::NewObjectInitializerExpression *SelectExpression;

        SelectExpression = PH.CreateAnonymousTypeInitializer(Projection, Projection->TextSpan, true, true);

        if(pRecordInitializer)
        {
            *pRecordInitializer = PH.CreateAnonymousTypeInitializer(NULL, Projection->TextSpan, true, true);
        }

        MemberInfos = InitializeMemberInfoList(Scratch, SelectExpression->InitialValues, true, &FieldCount);

        if (FieldCount > 0 && MemberInfos == NULL)
        {
            return AllocateBadExpression(Projection->TextSpan);
        }

        SelectorExpression = SelectExpression;
    }

    AssertIfFalse(!pNewControlVariableName || SingleExpressionCase);
    AssertIfFalse(MemberInfos || SingleExpressionCase);


    if(!SelectorExpression)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Projection->TextSpan);
        return AllocateBadExpression(Projection->TextSpan);
    }

    if(!pControlVariableName2)
    {
        boundSelector = BindSelectSelector(SelectorExpression, ControlVariableName, ControlVariableType, QueryFlags);
    }
    else
    {
        boundSelector = BindSelectSelector(SelectorExpression, ControlVariableName, ControlVariableType, QueryFlags,
                                                                *pControlVariableName2, ControlVariableType2, QueryFlags2);
    }

    AssertIfNull(boundSelector);

    if(IsBad(boundSelector))
    {
        return boundSelector;
    }

    // Check names for shadowing
    // #27975 Check names after BindSelectSelector() because it may declare new locals for the Proc
    if(SingleExpressionCase)
    {
        // It is OK if we failed to infer the name, but it is not OK to infer a bad name
        if(pNewControlVariableName && !CheckControlVariableIdentifier(*pNewControlVariableName))
        {
            return AllocateBadExpression(Projection->TextSpan);
        }
    }
    else
    if (MemberInfos)
    {
        MemberInfoList *Info = MemberInfos;
        for (ULONG i = 0; i < FieldCount; ++i)
        {
            CheckControlVariableName(Info->Name->Name, Info->Name->TextSpan, false);

            if(pRecordInitializer)
            {
                PH.AddExpressionInitializer(*pRecordInitializer, PH.CreateNameExpression(Info->Name->Name, Info->Name->TextSpan));
            }

            Info = Info->Next;
        }
    }

    if(pControlVariableType)
    {
        *pControlVariableType = PH.CreateBoundType(boundSelector->GetExpressionLambdaBody()->ResultType, Projection->TextSpan);
    }

    if(pControlVariableName)
    {
        if(pNewControlVariableName)
        {
            *pControlVariableName = *pNewControlVariableName;
        }
        else
        {
            *pControlVariableName = ItIdentifierDescriptor();
        }
    }

    if(pQueryFlags)
    {
        if(pNewControlVariableName)
        {
            *pQueryFlags = (QueryNoFlags);
        }
        else if(SingleExpressionCase)
        {
            *pQueryFlags = (ControlVarIsHidden);
        }
        else
        {
            *pQueryFlags = (ControlVarIsRecord | ControlVarIsHidden);
        }
    }

    return boundSelector;
}


ILTree::Expression *
Semantics::InterpretOrderByExpression
(
    _In_ ParseTree::OrderByExpression *Order,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    ParseTree::IdentifierDescriptor ControlVariableName;
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags = QueryNoFlags;

    ParseTree::LinqOperatorExpression * op = Order;
    ILTree::Expression *Source = InterpretLinqOperatorSource(Order->Source, Flags, &op , &ControlVariableName, &ControlVariableType, &QueryFlags, pRecordInitializer);

    if(pControlVariableType)
    {
        *pControlVariableType = ControlVariableType;
    }

    if(pControlVariableName)
    {
        *pControlVariableName = ControlVariableName;
    }

    if(pQueryFlags)
    {
        *pQueryFlags=QueryFlags;
    }

    if (IsBad(Source))
    {
        return Source;
    }

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Order->TextSpan);
        return AllocateBadExpression(Order->TextSpan);
    }

    ParserHelper PH(&m_TreeStorage, Order->TextSpan);
    bool IsFirstOrderBy = true;
    ILTree::Expression *Result = Source;
    ParseTree::OrderByList * OrderByClauses = Order->OrderByItems;

    VSASSERT(ControlVariableType, "Unexpected!");

    Location OrderByOrCommaLocation = {0};
    OrderByOrCommaLocation.SetLocation(Order->TextSpan.m_lBegLine, &Order->FirstPunctuator,g_tkKwdNameLengths[tkORDER]-1);

    if(Order->WithBy)
    {
        Location ByLocation = {0};
        ByLocation.SetLocation(Order->TextSpan.m_lBegLine, &Order->By,g_tkKwdNameLengths[tkBY]-1);
        OrderByOrCommaLocation.SetEnd(&ByLocation);
    }

    while (OrderByClauses)
    {
        ParseTree::OrderByItem *OrderBy = OrderByClauses->Element;
        STRING *MethodName = NULL;

        if (IsFirstOrderBy)
        {
            if (OrderBy->IsDescending)
            {
                MethodName = STRING_CONST(m_Compiler, OrderByDescendingMethod);
            }
            else
            {
                MethodName = STRING_CONST(m_Compiler, OrderByMethod);
            }
        }
        else
        {
            if (OrderBy->IsDescending)
            {
                MethodName = STRING_CONST(m_Compiler, ThenByDescendingMethod);
            }
            else
            {
                MethodName = STRING_CONST(m_Compiler, ThenByMethod);
            }
        }

        ParseTree::LambdaExpression * keySelector =
                                                    PH.CreateSingleLineLambdaExpression(
                                                                PH.CreateParameterList(
                                                                            OrderBy->OrderExpression->TextSpan,
                                                                            1,
                                                                            PH.CreateParameter(
                                                                                        ControlVariableName,
                                                                                        ControlVariableType, true,
                                                                                        HasFlag(QueryFlags, ControlVarIsRecord))),
                                                                OrderBy->OrderExpression,
                                                                OrderBy->OrderExpression->TextSpan,
                                                                true
                                                        );

        ILTree::LambdaExpression * boundKeySelector =
                InterpretUnboundLambda(
                        InterpretLambdaExpression(keySelector, ExprForceRValue));

        if(IsBad(boundKeySelector))
        {
            return boundKeySelector;
        }

        // #26707
        if (boundKeySelector->GetExpressionLambdaBody()->ResultType == TypeHelpers::GetVoidType())
        {
            boundKeySelector->SetExpressionLambdaBody(
                ConvertWithErrorChecking(
                    boundKeySelector->GetExpressionLambdaBody(),
                    GetFXSymbolProvider()->GetObjectType(),
                    ExprForceRValue
                    )
                );

            AssertIfFalse(IsBad(boundKeySelector->GetExpressionLambdaBody()));

            if(IsBad(boundKeySelector->GetExpressionLambdaBody()))
            {
                MakeBad(boundKeySelector);
                return boundKeySelector;
            }
        }

        Result =
            InterpretQueryOperatorCall(
                        PH.CreateMethodCall(
                                    PH.CreateQualifiedExpression(
                                                PH.CreateBoundExpression(Result),
                                                PH.CreateNameExpression(MethodName,OrderByOrCommaLocation),
                                                OrderByOrCommaLocation),
                                    PH.CreateArgList(
                                                1,
                                                PH.CreateBoundExpression(boundKeySelector))),
                        ExprForceRValue);

        if(IsBad(Result))
        {
            break;
        }

        if(pQueryFlags)
        {
            *pQueryFlags |= AppliedAtLeastOneOperator;
        }

        IsFirstOrderBy = false;

        if(OrderByClauses->Next)
        {
            OrderByOrCommaLocation.SetLocation(OrderByClauses->TextSpan.m_lBegLine, &OrderByClauses->Punctuator,g_tkKwdNameLengths[tkComma]-1);
        }

        OrderByClauses = OrderByClauses->Next;
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretLinqQuery
(
    _In_ ParseTree::Expression *Source,
    ExpressionFlags Flags
)
{
    if(ParseTree::Expression::Aggregate == Source->Opcode &&
        Source->AsAggregate()->Source == NULL)
    {
        // Stand alone OVER
        return InterpretAggregateExpression(Source->AsAggregate(), Flags);
    }

    ParseTree::IdentifierDescriptor ControlVariableName;
    ZeroMemory(&ControlVariableName, sizeof(ControlVariableName));
    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    QueryExpressionFlags QueryFlags = QueryNoFlags;

    ILTree::Expression *Result = InterpretLinqOperatorSource(Source, Flags, NULL, &ControlVariableName, &ControlVariableType, &QueryFlags);
    AssertIfTrue(!IsBad(Result) && HasFlag(QueryFlags, ControlVarIsNested));

    if (!HasFlag(QueryFlags, AppliedAtLeastOneOperator) &&
        !IsBad(Result))
    {
        // Apply implicit Select
        ParserHelper PH(&m_TreeStorage, Source->TextSpan);
        ParseTree::Expression *SelectExpression;

        SelectExpression = PH.CreateNameExpression(ControlVariableName.Name,ControlVariableName.TextSpan);

        ILTree::LambdaExpression * boundSelector = BindSelectSelector(SelectExpression, ControlVariableName, ControlVariableType, QueryFlags);

        if(IsBad(boundSelector))
        {
            return boundSelector;
        }

        Result = BindSelectOperator(Result, Source->TextSpan, Source->TextSpan, boundSelector);
    }

    return Result;
}


ILTree::Expression *
Semantics::InterpretLinqOperatorSource
(
    _In_ ParseTree::Expression *Source,
    ExpressionFlags Flags,
    _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator, //when *pFollowingOperator is changed to NULL, the following operator should be skiped because it was replaced with its equivalent
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer
)
{
    CleanupQueryOutputPrameters(pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    switch(Source->Opcode)
    {
    case ParseTree::Expression::InnerJoin:
        return InterpretInnerJoinExpression(Source->AsInnerJoin(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pFollowingOperator);

    case ParseTree::Expression::GroupJoin:
        return InterpretGroupJoinExpression(Source->AsGroupJoin(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pFollowingOperator);

    case ParseTree::Expression::CrossJoin:
        return InterpretCrossJoinExpression(Source->AsCrossJoin(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pFollowingOperator);

    case ParseTree::Expression::From:
        return InterpretFromExpression(Source->AsFrom(), NULL, Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer, pFollowingOperator);

    case ParseTree::Expression::Where:
        return InterpretFilterExpression(Source->AsWhere(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::SkipWhile:
    case ParseTree::Expression::TakeWhile:
        return InterpretFilterExpression(Source->AsWhile(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::Take:
    case ParseTree::Expression::Skip:
        return InterpretSkipTakeExpression(Source->AsSkipTake(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::GroupBy:
        return InterpretGroupByClause(Source->AsGroupBy(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::Aggregate:
        return InterpretAggregateExpression(Source->AsAggregate(), Flags, pFollowingOperator, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::Select:
        return InterpretSelectExpression(Source->AsSelect(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::Distinct:
        return InterpretDistinctExpression(Source->AsDistinct(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    case ParseTree::Expression::OrderBy:
        return InterpretOrderByExpression(Source->AsOrderBy(), Flags, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    default:
            AssertIfTrue(ERRID_InternalCompilerError);
            ReportSemanticError(
                ERRID_InternalCompilerError,
                Source->TextSpan);  // shouldn't get here
            return AllocateBadExpression(Source->TextSpan); // shouldn't get here

    }
}


ILTree::Expression *
Semantics::InterpretDistinctExpression
(
    ParseTree::DistinctExpression *Distinct,
    ExpressionFlags Flags,
    _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
    _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
    _Out_opt_ QueryExpressionFlags * pQueryFlags,
    _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer
)
{
    ParseTree::LinqOperatorExpression * op = Distinct;
    ILTree::Expression *Result = InterpretLinqOperatorSource(Distinct->Source, Flags,  &op, pControlVariableName, pControlVariableType, pQueryFlags, pRecordInitializer);

    if (IsBad(Result))
    {
        return Result;
    }

    AssertIfNull(op);
    if(!op)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        ReportSemanticError(
            ERRID_InternalCompilerError,
            Distinct->TextSpan);
        return AllocateBadExpression(Distinct->TextSpan);
    }

    //
    // DISTINCT
    //

    Location distinctNameLocation = {0};
    distinctNameLocation.SetLocation(Distinct->TextSpan.m_lBegLine, &Distinct->FirstPunctuator,g_tkKwdNameLengths[tkDISTINCT]-1);

    ParserHelper PH(&m_TreeStorage, Distinct->TextSpan);

    Result =
        InterpretQueryOperatorCall(
                    PH.CreateMethodCall(
                                PH.CreateQualifiedExpression(
                                            PH.CreateBoundExpression(Result),
                                            PH.CreateNameExpression(STRING_CONST(m_Compiler, DistinctMethod),distinctNameLocation),
                                            distinctNameLocation),
                                NULL),
                    ExprForceRValue);

    if(pQueryFlags)
    {
        *pQueryFlags |= AppliedAtLeastOneOperator;
    }

    return Result;
}


ILTree::Expression *
Semantics::BindSelectOperator
(
    ILTree::Expression * Source,
    _In_ Location & selectTextSpan,
    _In_ Location & selectNameLocation,
    ILTree::LambdaExpression * boundSelector
)
{
    ParserHelper PH(&m_TreeStorage, selectTextSpan);
    ILTree::Expression * Result;

    Result =
        InterpretQueryOperatorCall(
            PH.CreateMethodCall(
                        PH.CreateQualifiedExpression(
                                    PH.CreateBoundExpression(Source),
                                    PH.CreateNameExpression(STRING_CONST(m_Compiler, SelectMethod), selectNameLocation),
                                    selectNameLocation),
                        PH.CreateArgList(
                                    1,
                                    PH.CreateBoundExpression(boundSelector))),
            ExprForceRValue);

    if (IsBad(Result))
    {
        return Result;
    }

    return Result;
}


ILTree::LambdaExpression *
Semantics::BindSelectSelector
(
    _In_ ParseTree::Expression * SelectExpression,
    _In_ ParseTree::IdentifierDescriptor &ControlVariableName,
    ParseTree::AlreadyBoundType *ControlVariableType,
    QueryExpressionFlags QueryFlags,
    Type * TargetType /*= NULL*/
)
{
    ParserHelper PH(&m_TreeStorage, SelectExpression->TextSpan);

    ParseTree::ParameterList * parameters =
                                                PH.CreateParameterList(
                                                            SelectExpression->TextSpan,
                                                            1,
                                                            PH.CreateParameter(
                                                                        ControlVariableName,
                                                                        ControlVariableType,
                                                                        SelectExpression->TextSpan,
                                                                        true, HasFlag(QueryFlags, ControlVarIsRecord)));

    return BindSelectSelector(SelectExpression, parameters, TargetType);

}

ILTree::LambdaExpression *
Semantics::BindSelectSelector
(
    _In_ ParseTree::Expression * SelectExpression,
    ParseTree::ParameterList * Parameters,
    Type * TargetType /*= NULL*/
)
{
    ParserHelper PH(&m_TreeStorage, SelectExpression->TextSpan);

    ParseTree::LambdaExpression * selector =
                                    PH.CreateSingleLineLambdaExpression(
                                                Parameters,
                                                SelectExpression,
                                                SelectExpression->TextSpan,
                                                true
                                        );

    ILTree::LambdaExpression * boundSelector =
            InterpretUnboundLambda(
                    InterpretLambdaExpression(selector, ExprForceRValue),
                    TargetType);

    boundSelector->ExactlyMatchExpressionResultType = true;
    // #26707
    if (!IsBad(boundSelector) && boundSelector->GetExpressionLambdaBody()->ResultType == TypeHelpers::GetVoidType())
    {
        boundSelector->SetExpressionLambdaBody(
            ConvertWithErrorChecking(
                boundSelector->GetExpressionLambdaBody(),
                GetFXSymbolProvider()->GetObjectType(),
                ExprForceRValue
                )
            );

        AssertIfFalse(IsBad(boundSelector->GetExpressionLambdaBody()));

        if(IsBad(boundSelector->GetExpressionLambdaBody()))
        {
            MakeBad(boundSelector);
        }
    }


    return boundSelector;
}


ILTree::LambdaExpression *
Semantics::BindSelectSelector
(
    _In_ ParseTree::Expression * SelectExpression,
    _In_ ParseTree::IdentifierDescriptor &ControlVariableName1,
    ParseTree::AlreadyBoundType *ControlVariableType1,
    QueryExpressionFlags QueryFlags1,
    _In_ ParseTree::IdentifierDescriptor &ControlVariableName2,
    ParseTree::AlreadyBoundType *ControlVariableType2,
    QueryExpressionFlags QueryFlags2,
    Type * TargetType /*= NULL*/
)
{
    ParserHelper PH(&m_TreeStorage, SelectExpression->TextSpan);

    ParseTree::ParameterList * parameters =
                                                PH.CreateParameterList(
                                                            SelectExpression->TextSpan,
                                                            2,
                                                            PH.CreateParameter(
                                                                        ControlVariableName1,
                                                                        ControlVariableType1,
                                                                        SelectExpression->TextSpan,
                                                                        true, HasFlag(QueryFlags1, ControlVarIsRecord)),
                                                            PH.CreateParameter(
                                                                        ControlVariableName2,
                                                                        ControlVariableType2,
                                                                        SelectExpression->TextSpan,
                                                                        true, HasFlag(QueryFlags2, ControlVarIsRecord)));

    return BindSelectSelector(SelectExpression, parameters, TargetType);

}


#if IDE 

void
Semantics::GetQueryGroupTypeAndElement
(
    ParseTree::Expression *ExpressionWithGroup,
    Scope *Lookup,
    ILTree::Expression *WithStatementContext,
    Type **GroupType,
    Type **GroupElementType
)
{
    if (GroupType)
    {
        *GroupType = NULL;
    }

    if (GroupElementType)
    {
        *GroupElementType = NULL;
    }

    InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,
        NULL,      // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        true,      // preserve extra semantic information
        true,      // perform obsolete checks
        false,     // cannot interpret statements
        true,
        m_MergeAnonymousTypeTemplates,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        false       // reset closures information
    );

    // Setup the with context
    if (WithStatementContext)
    {
        m_EnclosingWith = &AllocateStatement(SB_WITH, WithStatementContext->Loc, 0, false)->AsWithBlock();
        m_EnclosingWith->Loc = WithStatementContext->Loc;

        InterpretWithStatement(WithStatementContext, m_EnclosingWith);
    }

    // Dummy location for the parse trees being created
    Location locDummy = {0};
    locDummy.SetLocation(0, 0);

    STRING *pstrGroupName = STRING_CONST(m_Compiler, Group2);

    ParserHelper PH(&m_TreeStorage);

    ParseTree::AlreadyBoundType *ControlVariableType = NULL;
    ParseTree::AlreadyBoundType *BoundGroupElementType = NULL;

    switch (ExpressionWithGroup->Opcode)
    {
        case ParseTree::Expression::GroupJoin:
        {
            BackupValue<ParseTree::InitializerList *> backupValue(&ExpressionWithGroup->AsGroupJoin()->Projection);

            // Swap out projection with the name expression VB$Group2
            ExpressionWithGroup->AsGroupJoin()->Projection =
                PH.CreateInitializerListFromExpression(
                    &PH.CreateIdentifierDescriptor(pstrGroupName,locDummy),
                    PH.CreateGroupReference(locDummy));

            InterpretGroupJoinExpression(ExpressionWithGroup->AsGroupJoin(), ExprNoFlags, NULL, &ControlVariableType, NULL, NULL, NULL, &BoundGroupElementType);

            break;
        }

        case ParseTree::Expression::GroupBy:
        {
            BackupValue<ParseTree::InitializerList *> backupValue(&ExpressionWithGroup->AsGroupBy()->Projection);

            // Swap out projection with the name expression VB$Group2
            ExpressionWithGroup->AsGroupBy()->Projection =
                PH.CreateInitializerListFromExpression(
                    &PH.CreateIdentifierDescriptor(pstrGroupName,locDummy),
                    PH.CreateGroupReference(locDummy));

            InterpretGroupByClause(ExpressionWithGroup->AsGroupBy(), ExprNoFlags, NULL, &ControlVariableType, NULL, NULL, &BoundGroupElementType);

            break;
        }

        case ParseTree::Expression::Aggregate:
        {
            BackupValue<ParseTree::InitializerList *> backupValue(&ExpressionWithGroup->AsAggregate()->Projection);

            // Create a projection with two synatx error elements so that InterpretAggregateExpression
            // will send back the GroupType / GroupElementType
            ExpressionWithGroup->AsAggregate()->Projection =
                PH.CreateInitializerListFromExpression(
                    PH.CreateSyntaxErrorExpression(locDummy));

            ExpressionWithGroup->AsAggregate()->Projection->Next =
                PH.CreateInitializerListFromExpression(
                    PH.CreateSyntaxErrorExpression(locDummy));

            if (ExpressionWithGroup->AsAggregate()->Source == NULL)
            {
                // Note:  GroupType is directly set by the function (no ControlVariableType)
                InterpretAggregateExpression(ExpressionWithGroup->AsAggregate(), ExprNoFlags, GroupType, &BoundGroupElementType);
            }
            else
            {
                InterpretAggregateExpression(ExpressionWithGroup->AsAggregate(), ExprNoFlags, NULL, NULL, NULL, NULL, NULL, &ControlVariableType, &BoundGroupElementType);
            }
            break;
        }

        default:
            VSFAIL("Unexpected parse tree opcode!");
            break;
    }

    // Pull the group type from the ControlVariableType by looking for Group2
    if (GroupType &&
        ControlVariableType &&
        ControlVariableType->BoundType)
    {
        Type *TypeFromControlVarible = GetTypeOfTheGroup(ControlVariableType->BoundType, pstrGroupName, locDummy);

        if (TypeFromControlVarible && !TypeFromControlVarible->IsBad())
        {
            *GroupType = TypeFromControlVarible;
        }
    }

    if (GroupElementType &&
        BoundGroupElementType &&
        BoundGroupElementType->BoundType &&
        !BoundGroupElementType->BoundType->IsBad())
    {
        *GroupElementType = BoundGroupElementType->BoundType;
    }
}

#endif IDE

