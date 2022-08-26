//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Algorithm
//
//  Phase 1: Information Gathering
//
//  After the biltree for a method is created it is walked during semantic the
//  semantic analysis phase.  During this phase an instance of ClosureRoot is
//  created and used during the biltree walk.  It will collect the following
//  information
//
//   - All Symbol References
//   - Lambda's within the Procedure (both normal and expression trees)
//   - Variables to be lifted
//   - Empty Blocks that need to be inserted
//   - Catch Variables
//
//  Whenever we encounter a variable that needs to be lifted, a Closure will
//  be created for the Block which owns the variable.  The one exception is
//  ShortLived Temporaries, the closures for them will be created when
//  the ShortLived Temporary is encountered but it won't lift the variable
//  until ShortLived Temporaries are fixed up.
//
//  Whenever we encounter a normal lambda, a corresponding procedure will
//  be created on the containing class.  It will be fully created but the
//  original lambda BILTREE will still be attached to the containing
//  method BILTREE.  That will be removed at a later time.
//
//  After the walk is complete all of the empty blocks which were created will
//  be inserted into the BILTREE.  This happens for both empty blocks associated
//  with Blocks and with Statement Groups.  At the same time Empty Blocks
//  associated with Statement Groups are inserted all TrackedExpressions which
//  occurr in those statements will be modified to point to the appropriate
//  blocks.
//
//  Next, work will be done for the lifting of ShortLived temporaries.  When a
//  ShortLived temporary is lifted we need to actually lift a completely
//  separate variable.  The BILTREE will be scanned and the instances where a
//  temporary will be lifted are replaced with new temporaries (one per statement
//  group).
//
//  At the end of this Phase we are left with all of the Closures we will be
//  creating and all of the lambdas.  None of these are connected at this
//  point.
//
//  Phase 2: Closure Tree Structure fixup
//
//  This phase will create the closure tree structure and assign the lambdas
//  to their owners.
//
//  First the lambdas are assigned to their owning Closure.  At this point
//  we determine if a Lambda can be optimized and if so assign it to the
//  OptimizedClosure instance.  Otherwise the Closure nearest to the Block
//  definining the Lambda gains ownership.
//
//  The Closure structured is reworked so that they exist in a tree structure
//  (1 To N) matching the relative block structure of the procedure.
//
//  Example: If we had the following procedure pseudo body it would result in
//  the following closure structure
//
//  Procedure
//    Block
//      Block (associated with Closure1)
//      Block (associated with Closure2)
//        Block
//          Block (associated with Closure3)
//
//  Closure Tree
//    Closure1
//    Closure2
//      Closure3
//
//  Notice that even though there was any empty block between Closure2 and Closure3
//  that Closure2 is still the immediate parent of Closure3.  Empty blocks don't
//  factor into the reparenting process.
//
//  The BILTREE is not altered in this phase.
//
//  Phase 3: Closure class creation
//
//  At the start of this phase we know all of the closures we will be generating
//  and the fields they will be directly lifting.
//
//  Each closure in turn will create their class code (class, class variable,
//  fields, etc).  This is done in a parent to child fashion that way every
//  closure has all of the types it needs before it creates its code.  So when
//  any closure reaches this phase it knows that it's parent is completely done
//
//  The creation goes in the following order
//    - Class creation
//    - Generic Fixup
//    - Class variable creation (the local closure variable)
//    - Lifted fields
//    - Reparents procedures
//
//  The generic fixup code only runs if the original procedure, containing class,
//  or class containing the containing class is generic.  This code will iterate
//  all of the lambdas within the closure and rebind all of the appropriate types
//  within their new generic context
//
//  When first seen during Phase #1, all lambdas are turned into a procedure
//  rooted on the containing class.  The reparenting phase will reparent these
//  procedures on the Closure class
//
//  Phase 4: Symbol Reference Fixup
//
//  The ClosureRoot will iterate through all of the symbol references in stored
//  in Phase 1.  It will find the Closure nearest to the symbol reference.  By
//  nearest it will lookup the blocks and find the nearest block with an associated
//  Closure. The closure will then fixup the symbol reference if it refers to a
//  lifted variable.
//
//  The lifted variable may not be directly reachable in the closure.  If you
//  have several levels of nested closures the Closure may have to lift via
//  proxy.  For example.
//
//  Procedure
//    Block1 ( Closure1 -> Lifts x )
//      Block2 ( Closure2 -> Lifts y )
//
//  If an expression inside Block2 used "x", Closure2 would need to lift the variable.
//  But it can't directly lift the variable since it's actually owned by Closure1.
//  Instead Closure2 must lift Closure1 and fixup the reference to x to be
//  essentially
//
//    closure2Var.Closure1.x
//
//  This is referred to as lifting by proxy
//
//  If there was no closure associated with Block2 then Closure1 would be asked
//  to do the fixup since it is the closest closure.  It would be able to directly
//  refer to x since Closure1's local variable will be in scope.  In essence it
//  will become
//
//    closure1Var.x
//
//  Phase 5: Lambda Reference Fixup
//
//  The ClosureRoot will fixup all the lambda references it found in Phase 1 to
//  point to the newly reparented procedure in the Closure class.  This will case
//  the SX_LAMBDA billtree nodes to be turned into delegate calls (SX_ADDRESSOF).
//
//  Additionally ExpressionTrees will be converted at this point as well.
//
//  Phase 6: Initialization Code
//
//  During this phase, each Closure will generate their initialization code. This
//  will mainly conist of creating the closure variable inside their associated
//  block and doing assignments for fields that are lifted by proxy.
//
//  It is not necessary to do assignment for fields that are lifted directly.  Since
//  the initial assignment contains a symbol reference to a lifted variable it will
//  be redirected to the closure.
//
//  For example if x is lifted later on
//    Sub Foo()
//      Dim x As Integer = 5
//      ...
//    End Sub
//
//  Will become
//    Sub Foo()
//      Dim closure1 As New Closure1_RandomName()
//      closure1.x = 5
//      ...
//    End Sub
//
//  At this point all local variables that were lifted are removed from the original
//  containing hash or from the temporary manager if they were temporaries
//
//  Phase 7: Goto
//
//  If any GoTo's were detected during the initial biltree walk, the bil tree will
//  be rewalked and verify that all goto calls are legal
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

class ClosureLocationComparer
{
public:
    int Compare(const Closure *left, const Closure *right)
    {
        ThrowIfNull(left);
        ThrowIfNull(right);

        Location l1 = left->GetBlock()->Loc;
        Location l2 = right->GetBlock()->Loc;

        int comp = CompareValues(l1.m_lBegLine, l2.m_lBegLine);
        if ( 0 == comp )
        {
            comp = CompareValues(l1.m_lBegColumn, l2.m_lBegColumn);
        }

        return comp;
    };
};


Variable *GetVariableFromExpression(ILTree::Expression *expr)
{
    if ( !expr || SX_SYM != expr->bilop )
    {
        return NULL;
    }

    ILTree::SymbolReferenceExpression &symRef = expr->AsSymbolReferenceExpression();
    if ( symRef.Symbol && symRef.Symbol->IsVariable() )
    {
        return symRef.Symbol->PVariable();
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
//
// Some blocks have particular semantics about variables declared as a part
// of the block.  This declaration is sometimes explicitly done by the user
// and other times done as a function of the compiler.
//
// To respect these semantics closures occ----ionally have to insert a hidden
// code block between this block and it's parent simply for housing variables
// and maintaining certain semantics with other VB features (GoTo, With, etc)
//
// This returns whether or not the statement is one of those types of blocks
//
bool IsSpecialScopeExecutableBlock(ILTree::ExecutableBlock *block)
{
    ThrowIfNull(block);
    switch ( block->bilop )
    {
        case SB_FOR:
        case SB_FOR_EACH:
        case SB_WITH:
        case SB_SELECT:
            return true;
        default:
            return false;
    }
}

void UpdateStatementList(ILTree::Statement **first, ILTree::Statement **last, ILTree::Statement *created)
{
    AssertIfNull(first);
    AssertIfNull(last);
    AssertIfNull(created);

    ILTree::Statement *createdLast = created;
    while ( createdLast->Next )
    {
        createdLast = createdLast->Next;
    }

    if ( !*first )
    {
        AssertIfTrue(*last);
        *first = created;
        *last = createdLast;
    }
    else
    {
        AssertIfNull(*last);
        created->Prev = *last;
        (*last)->Next = created;
        *last = createdLast;
    }
}

void InsertStatementList(ILTree::Statement *stmt, ILTree::Statement *toInsert)
{
    ThrowIfNull(stmt);
    ThrowIfNull(toInsert);

    // Get the front of the list inserted
    ILTree::Statement *oldNext = stmt->Next;
    stmt->Next = toInsert;
    toInsert->Prev = stmt;

    // Find the last statement
    ILTree::Statement *last = toInsert;
    while (last->Next)
    {
       last = last->Next;
    }

    last->Next = oldNext;
    if ( oldNext )
    {
        oldNext->Prev = last;
    }
}

void InsertStatementListBefore(ILTree::Statement *stmt, ILTree::Statement *toInsert)
{
    ThrowIfNull(toInsert);

    if ( stmt && stmt->Prev )
    {
        // There is a statement at the front.  Insert the new list after the
        // previous statement
        InsertStatementList(stmt->Prev, toInsert);
    }
    else if ( stmt )
    {
        // stmt is the first statement in the list.  Find the last statement
        // in toInsert and append stmt after it.  Reset stmt to point to the
        // head of toInsert
        ILTree::Statement *last = toInsert;
        while ( last->Next )
        {
            last = last->Next;
        }

        // Append stmt to the end
        last->Next = stmt;
        stmt->Prev = last;
    }
    else
    {
        // There was no original list.  Happens when you pass in a null statement
        // so toInsert is just the head of the list and there is nothing to do
    }
}

void InsertStatementListBefore(ILTree::ExecutableBlock *block, ILTree::Statement *stmt, ILTree::Statement *toInsert)
{
    ThrowIfNull(block);
    ThrowIfFalse(stmt ? stmt->Parent == block : true);
    ThrowIfNull(toInsert);
    ThrowIfFalse(toInsert->Parent == block);

    ::InsertStatementListBefore(stmt, toInsert);

    // If 'stmt' was previously the child of block, reset the reference
    if ( stmt == block->Child )
    {
        block->Child = toInsert;
    }
}


void RemoveStatementFromParent(ILTree::Statement *stmt)
{
    ThrowIfNull(stmt);
    ThrowIfNull(stmt->Parent);

    ILTree::ExecutableBlock *parent = stmt->Parent;
    if ( parent->Child == stmt )
    {
        // It's the first node.
        parent->Child = stmt->Next;
        if ( parent->Child )
        {
            parent->Child->Prev = NULL;
        }
    }
    else if ( !stmt->Next )
    {
        // It's the last node.  We've already asserted that it's
        // not the first so it will have a previous
        stmt->Prev->Next = NULL;
    }
    else
    {
        // It's a middle node.
        ILTree::Statement *next = stmt->Next;
        next->Prev = stmt->Prev;
        stmt->Prev->Next = next;
    }
}

ILTree::Statement *FindBaseOrMeConstructorCall(Variable *varMe, Procedure *proc, ILTree::ProcedureBlock *body)
{
    ThrowIfNull(varMe);
    ThrowIfNull(proc);
    ThrowIfNull(body);
    ThrowIfFalse(proc->IsInstanceConstructor());
    ThrowIfFalse(varMe->GetType()->IsClass());

    BCSYM *type = varMe->GetType();
    BCSYM *baseType = type->PClass()->GetBaseClass();

    // Dev10 #689323: Base type could be a generic type binding, dig to the underlying generic type.
    if ( baseType->IsClass() )
    {
        baseType = baseType->PClass();
    }
    
    AssertIfFalse( !baseType->IsGenericTypeBinding() ); 
    AssertIfFalse( !type->IsGenericTypeBinding() ); // this is enforced by symbol construction

    ILTree::Statement *stmt = body->Child;
    while ( stmt )
    {
        ILTree::Expression *expr = stmt->AsStatementWithExpression().Operand1;
        ILTree::CallExpression *call = NULL;
        switch ( expr->bilop )
        {
            case SX_CALL:
                call = &(expr->AsCallExpression());
                break;
            case SX_SEQ:
                {
                    ILTree::BinaryExpression &binop = expr->AsBinaryExpression();
                    if ( binop.Left && SX_CALL == binop.Left->bilop)
                    {
                        call = &binop.Left->AsCallExpression();
                    }
                }
                break;
        }

        if ( call && 
             call->MeArgument != NULL && 
             call->MeArgument->bilop == SX_SYM &&
             call->MeArgument->AsSymbolReferenceExpression().Symbol == varMe )
        {
            ThrowIfNull(call->Left);
            ThrowIfFalse(call->Left->bilop == SX_SYM);
            ILTree::SymbolReferenceExpression &symRef = call->Left->AsSymbolReferenceExpression();
            if (symRef.Symbol->IsProc() )
            {
                Procedure *curProc = symRef.Symbol->PProc();
                BCSYM_Class *pClass = curProc->GetContainingClass();
                
                if ( curProc->IsInstanceConstructor()
                    && ((pClass == baseType) || (pClass == type)) )
                {
                    return stmt;
                }
            }
        }

        stmt = stmt->Next;
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
//
// Append the generic parameters from the source to the specified array
//
void AppendGenericParameters(BCSYM *source, BCSYM **argArray, unsigned &nextIndex)
{
    AssertIfNull(source);
    AssertIfNull(argArray);

    GenericParameter *current = source->GetFirstGenericParam();
    while (current)
    {
        argArray[nextIndex] = current;
        ++nextIndex;
        current = current->GetNextParam();
    }
}

//-------------------------------------------------------------------------------------------------
//
// Start ClosureNameMangler
//

bool ClosureNameMangler::IsClosureClass(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return symbol->IsClass() &&
           StrLen(symbol->GetName()) >= CLOSURE_CLASS_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), CLOSURE_CLASS_PREFIX, CLOSURE_CLASS_PREFIX_LENGTH) == 0;
}

bool ClosureNameMangler::IsStateMachine(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return symbol->IsClass() &&
           StrLen(symbol->GetName()) >= VB_STATEMACHINE_PREFIX_LEN &&
           CompareNoCaseN(symbol->GetName(), VB_STATEMACHINE_PREFIX, VB_STATEMACHINE_PREFIX_LEN) == 0;
}

bool ClosureNameMangler::IsMoveNextProcedureOfStateMachine(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    WCHAR* wszMoveNextName = L"MoveNext";
    int moveNextLength = 8;

    return symbol->IsProc() &&
           StrLen(symbol->GetName()) >= moveNextLength &&
           CompareNoCaseN(symbol->GetName(), wszMoveNextName, moveNextLength) == 0 &&
           symbol->GetContainingClass() &&
           ClosureNameMangler::IsStateMachine(symbol->GetContainingClass());
}


bool ClosureNameMangler::IsClosureVariable(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return symbol->IsVariable() &&
           StrLen(symbol->GetName()) >= CLOSURE_VARIABLE_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), CLOSURE_VARIABLE_PREFIX, CLOSURE_VARIABLE_PREFIX_LENGTH) == 0;
}

bool ClosureNameMangler::IsLambda(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return symbol->IsProc() &&
           StrLen(symbol->GetName()) >= LAMBDA_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), LAMBDA_PREFIX, LAMBDA_PREFIX_LENGTH) == 0;
}

bool ClosureNameMangler::IsLiftedLocal(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return StrLen(symbol->GetName()) >= LIFTED_LOCAL_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), LIFTED_LOCAL_PREFIX, LIFTED_LOCAL_PREFIX_LENGTH) == 0;
}

bool ClosureNameMangler::IsLiftedNonLocal(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return StrLen(symbol->GetName()) >= LIFTED_NONLOCAL_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), LIFTED_NONLOCAL_PREFIX, LIFTED_NONLOCAL_PREFIX_LENGTH) == 0;
}

bool ClosureNameMangler::IsLiftedMe(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return StrLen(symbol->GetName()) >= LIFTED_ME_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), LIFTED_ME_PREFIX, LIFTED_ME_PREFIX_LENGTH) == 0;
}

bool ClosureNameMangler::IsMyBaseMyClassStub(BCSYM_NamedRoot *symbol)
{
    ThrowIfNull(symbol);

    return StrLen(symbol->GetName()) >= CLOSURE_MYSTUB_PREFIX_LENGTH &&
           CompareNoCaseN(symbol->GetName(), CLOSURE_MYSTUB_PREFIX, CLOSURE_MYSTUB_PREFIX_LENGTH) == 0;
}

//==============================================================================
// Encode a local variable name to the field name it will have within the
// closure.  The name will be prefixed with $VB$Local_
//==============================================================================
STRING *ClosureNameMangler::EncodeLocalVariableName(Compiler *compiler, const WCHAR *name)
{
    ThrowIfNull(compiler);
    ThrowIfNull(name);

    StringBuffer buf;
    buf.AppendPrintf(LIFTED_LOCAL_PREFIX L"_%s", name);
    return compiler->AddString(&buf);
}

//==============================================================================
// Encode a local variable name to the field name it will have within the
// closure.  The name will be prefixed with $VB$AsyncLocal_
// Since state machines lift (almost) all locals of a method, the lifted fields should
// only be shown in the debugger when the original local variable was in scope.
//==============================================================================
STRING *ClosureNameMangler::EncodeResumableVariableName(Compiler *compiler, const WCHAR *name)
{
    ThrowIfNull(compiler);
    ThrowIfNull(name);

    StringBuffer buf;
    buf.AppendPrintf(LIFTED_RESUMABLELOCAL_PREFIX L"_%s", name);
    return compiler->AddString(&buf);
}

//==============================================================================
// Encode the generic paramater into the closure specific name.  This will encode
// the name of the parameter based on it's index.  Closures only ever add generic
// params for the generics on the actual method so this won't every conflict
//==============================================================================
STRING *ClosureNameMangler::EncodeGenericParameterName(Compiler *compiler, GenericParameter *param)
{
    ThrowIfNull(compiler);
    ThrowIfNull(param);

    StringBuffer buf;
    buf.AppendPrintf(CLOSURE_GENERICPARAM_PREFIX L"%u", param->GetPosition());
    return compiler->AddString(&buf);
}

//==============================================================================
// Get the index of the encoded generic parameter
//==============================================================================
bool ClosureNameMangler::DecodeGenericParameterName(const WCHAR *name, unsigned *index)
{
    ThrowIfNull(name);
    ThrowIfNull(index);

    size_t max = CLOSURE_GENERICPARAM_PREFIX_LENGTH + MaxStringLengthForParamSize;
    size_t length = 0;
    HRESULT hr = StringCchLength(name, max, &length);
    ThrowIfFalse(SUCCEEDED(hr));

    if (length < CLOSURE_GENERICPARAM_PREFIX_LENGTH + 1)
    {
        *index = 0;
        return false;
    }

    int temp = _wtoi(name + CLOSURE_GENERICPARAM_PREFIX_LENGTH);
    *index = static_cast<unsigned>(temp);
    return true;
}

//==============================================================================
// Given an encoded variable name decode it and return the original name
//==============================================================================
STRING *ClosureNameMangler::DecodeVariableName(Compiler *compiler, const WCHAR *name)
{
    ThrowIfNull(compiler);
    ThrowIfNull(name);

    if ( StringPool::IsEqual(compiler->AddString(name), compiler->AddString(LIFTED_ME_PREFIX)))
    {
        return compiler->AddString(L"Me");
    }

    StringBuffer buf;
    const WCHAR *ptr = name;
    while ( '_' != *ptr && 0 != *ptr )
    {
        buf.AppendChar(*ptr);
        ++ptr;
    }

    if ( '_' != *ptr )
    {
        // Not an encoded name
        return NULL;
    }

    ++ptr;  // Move it past the _
    STRING *prefix = compiler->AddString(&buf);
    if ( StringPool::IsEqual(prefix, compiler->AddString(LIFTED_LOCAL_PREFIX) )
        || StringPool::IsEqual(prefix, compiler->AddString(CLOSURE_VARIABLE_PREFIX))
        || StringPool::IsEqual(prefix, compiler->AddString(LIFTED_NONLOCAL_PREFIX))
        || StringPool::IsEqual(prefix, compiler->AddString(LIFTED_RESUMABLELOCAL_PREFIX)))
    {
        return compiler->AddString(ptr);
    }
    else
    {
        // Not an encoded name
        return NULL;
    }

}

STRING *ClosureNameMangler::DecodeResumableLocalOriginalName(Compiler *compiler, const WCHAR *decodedName)
{
    ThrowIfNull(compiler);
    ThrowIfNull(decodedName || *decodedName);

    StringBuffer buf;
    const WCHAR *ptr = decodedName;
    while ( '$' != *ptr && 0 != *ptr )
    {
        buf.AppendChar(*ptr);
        ++ptr;
    }
    return compiler->AddString(&buf);
}

STRING *ClosureNameMangler::DecodeStateMachineProcedureName(BCSYM_NamedRoot * symbol, Compiler *compiler)
{
    ThrowIfNull(symbol);
    ThrowIfNull(compiler);
    
    VSASSERT(IsMoveNextProcedureOfStateMachine(symbol), "bad symbol");

    BCSYM_NamedRoot* pStateMachineClass = symbol->GetContainingClass();
    
    const WCHAR *ptr = pStateMachineClass->GetName();
    // Name is of the form $VB$StateMachine_1_MethodName
    for (int i=0; i < 2; i++)
    {
        while ( '_' != *ptr && 0 != *ptr )
        {
            ++ptr;
        }

        ThrowIfFalse( '_' == *ptr );
        ++ptr;  // Move it past the _
    }
    
    // Return the "MethodName" part.  If we are left with a lambda, decode that as well.
    unsigned count;
    if ( DecodeLambdaLocation(ptr, &count) )
    {
        StringBuffer buf;
        buf.AppendPrintf(NONLOC_LAMBDA_DISPLAY_NAME_1, count);
        STRING *name = compiler->AddString(&buf);
        return name;
    }
    else
    {
        return compiler->AddString(ptr);
    }
}

bool ClosureNameMangler::DecodeStateMachineLocation(const WCHAR *name, unsigned *count)
{
    ThrowIfNull(name);
    ThrowIfNull(count);

    return swscanf_s(name, VB_STATEMACHINE_PREFIX L"_%u", count) == 1;
}

bool ClosureNameMangler::DecodeClosureClassLocation(const WCHAR *name, unsigned *count)
{
    ThrowIfNull(name);
    ThrowIfNull(count);

    return swscanf_s(name, CLOSURE_CLASS_PREFIX L"_%u", count) == 1;
}

bool ClosureNameMangler::DecodeLambdaLocation(const WCHAR *name, unsigned *count)
{
    ThrowIfNull(name);
    ThrowIfNull(count);

    return swscanf_s(name, LAMBDA_PREFIX L"_%u", count) == 1;
}



//==============================================================================
// Encode a non-local variable name to the field name it will have within the
// closure
//==============================================================================
STRING *ClosureNameMangler::EncodeNonLocalVariableName(Compiler *compiler, const WCHAR *name)
{
    ThrowIfNull(compiler);
    ThrowIfNull(name);

    StringBuffer buf;
    buf.AppendPrintf(LIFTED_NONLOCAL_PREFIX L"_%s", name);
    return compiler->AddString(&buf);
}

STRING *ClosureNameMangler::EncodeClosureVariableName(Compiler *compiler, const WCHAR *name, unsigned count)
{
    ThrowIfNull(compiler);
    ThrowIfNull(name);

    STRING *uniqueName = Semantics::GenerateUniqueName(compiler, name, count);
    StringBuffer buf;
    buf.AppendPrintf(CLOSURE_VARIABLE_PREFIX L"_%s", uniqueName);
    return compiler->AddString(&buf);
}

STRING *ClosureNameMangler::EncodeMeName(Compiler *compiler)
{
    ThrowIfNull(compiler);
    return compiler->AddString(LIFTED_ME_PREFIX);
}

STRING *ClosureNameMangler::EncodeMyBaseMyClassStubName(Compiler *compiler, Procedure* proc, unsigned flags)
{
    ThrowIfNull(compiler);
    ThrowIfNull(proc);
    ThrowIfTrue(HasFlag(flags, SXF_SYM_MYBASE) && HasFlag(flags, SXF_SYM_MYCLASS));
    ThrowIfFalse(HasFlag(flags, SXF_SYM_MYBASE) || HasFlag(flags, SXF_SYM_MYCLASS));

    StringBuffer buf;
    buf.AppendPrintf(
            CLOSURE_MYSTUB_PREFIX L"%s_%s",
            proc->GetName(),
            HasFlag(flags, SXF_SYM_MYBASE) ? L"MyBase" : L"MyClass");
    return compiler->AddString(&buf);
}


//==============================================================================
// End ClosureNameMangler
//==============================================================================

ClosureRoot::ClosureRoot(Procedure *proc, ILTree::ProcedureBlock *body, Semantics *semantics, Symbols &transientSymbolFactory)
    : m_proc(proc),
    m_procBody(body),
    m_state(CRState_None),
    m_procContainsGoto(false),
    m_genEncCode(false),
    m_lambdaNestLevel(0),
    m_semantics(semantics),
    m_pCompiler(semantics->m_Compiler),
    m_norls(NORLSLOC),
    m_alloc(&m_norls),
    m_transientSymbolFactory(transientSymbolFactory),
    m_optClosureList(m_alloc),
    m_map(m_alloc),
    m_blockStack(m_alloc),
    m_LocalScopesToInject(NULL),
    m_frameStack(m_alloc),
    m_statementStack(m_alloc),
    m_trackedList(m_alloc),
    m_rootClosureList(m_alloc),
    m_catchMap(m_alloc),
    m_flagMap(m_alloc),
    m_realLambdaMap(m_alloc),
    m_lambdaToDataMap(m_alloc),
    m_emptyBlockForGroupMap(m_alloc),
    m_emptyBlockForBlockMap(m_alloc),
    m_shortLivedMap(m_alloc),
    m_nearestMap(m_alloc),
    m_varMap(m_alloc),
    m_mbcCallList(m_alloc)
{
    ThrowIfNull(semantics);
    ThrowIfNull(proc);
    ThrowIfNull(body);
    ThrowIfNull(GetTransientSymbolStore());

    if ( semantics->m_SourceFile && semantics->m_SourceFile->GetProject() )
    {
        m_genEncCode = semantics->m_SourceFile->GetProject()->GenerateENCableCode();
    }
}

ClosureRoot::~ClosureRoot()
{
    // Everything is allocated with a NorlsAllocator so there is nothing here to delete
}

//==============================================================================
// Start Iterator Methods
//==============================================================================

void ClosureRoot::Iterate()
{
    ChangeState(CRState_Iterating);
    BoundTreeVisitor::Visit(m_procBody);
    ChangeState(CRState_PostIterating);
}

bool ClosureRoot::StartBlock(ILTree::ExecutableBlock *block)
{
    ThrowIfNull(block);

    if ( ::IsSpecialScopeExecutableBlock(block) )
    {
        // Whenever there is one of these types of blocks, there is the potential we will
        // have to insert an empty block for scoping purposes if one of the control variables
        // is lifted.  It's very complicated and risky to insert this block on demand.
        // So instead we insert a fake empty block every time and if the block is not
        // used then we just remove it without ever adding it to the BILTREE
        //
        // It's safe to insert a block in the block stack that is not actually
        // inserted in the BILTREE.  When factoring out Lambda's the closure code will
        // snip the tree in certain places.  So no code depends on being able to
        // walk the block's via the BILTREE until after the iteration is complete.
        EmptyBlockData *data = GetOrCreateEmptyBlockDataForBlock(block, GetCurrentReferencingProcedure());
        PushBlock(data->GetEmptyBlock(), data->GetEmptyBlock()->Locals, NULL);
    }
    
    PushBlock(block, block->Locals, NULL);
    return true;
        
}

void ClosureRoot::EndBlock(ILTree::ExecutableBlock *block)
{
    PopBlock();

    if ( ::IsSpecialScopeExecutableBlock(block) )
    {
        // Extra block is pushed on for these nodes.  Go ahead and remove it
        PopBlock();
    }
}

void ClosureRoot::PopBlock(LocalScopeInjection & top)
{
    top.previousInjection = m_LocalScopesToInject;
    top.localScopeToInject = PopBlock();

    AssertIfFalse(m_blockStack.Count()>0);
    if (m_blockStack.Count()>0)
    {
        top.blockInStack = m_blockStack.Peek();
    }

    m_LocalScopesToInject = &top;
}

void ClosureRoot::PushBlock(LocalScopeInjection & top)
{
    AssertIfFalse(m_blockStack.Count()>0 && m_blockStack.Peek()==top.blockInStack);
    PushBlock(top.localScopeToInject);
    AssertIfFalse(m_LocalScopesToInject == &top);
    m_LocalScopesToInject = top.previousInjection;
}

void ClosureRoot::VisitFor(ILTree::ForBlock* block)
{
    AssertIfFalse(::IsSpecialScopeExecutableBlock(block) );

    // Dev10 #576786 Some expressions and statements, even though they are stored inside
    // ILTree::ForBlock structure, are actually evaluated in the loop initialization section and not inside
    // the loop block (see CodeGenerator::GenerateForLoopStart). So, they should be visited in context
    // of the parent block.

    LocalScopeInjection top;
    PopBlock(top);

#if DEBUG
    EmptyBlockData * emptyData = GetEmptyBlockDataForBlock(block);
    AssertIfFalse(emptyData && m_blockStack.Count()>0 && m_blockStack.Peek() && 
                emptyData->GetEmptyBlock() && emptyData->GetEmptyBlock() == m_blockStack.Peek()->GetBlock());
#endif
    
    VisitExpression(&block->ControlVariableReference);
    VisitExpression(&block->Initial);
    VisitExpression(&block->Limit);
    VisitExpression(&block->Step);
    VisitStatement(block->SignCheckStatement);

    PushBlock(top);
    
    VisitStatement(block->IncrementStatement);
    VisitExpression(&block->Condition);
}


void ClosureRoot::VisitForEach(ILTree::ForEachBlock* block)
{
    AssertIfFalse(::IsSpecialScopeExecutableBlock(block) );

    // Dev10 #576786 Some expressions and statements, even though they are stored inside
    // ILTree::ForEachBlock structure, are actually evaluated in the loop initialization section and not inside
    // the loop block (see CodeGenerator::GenerateForEachLoop). So, they should be visited in context
    // of the parent block.
    LocalScopeInjection top;
    PopBlock(top);

#if DEBUG
    EmptyBlockData * emptyData = GetEmptyBlockDataForBlock(block);
    AssertIfFalse(emptyData && m_blockStack.Count()>0 && m_blockStack.Peek() && 
                emptyData->GetEmptyBlock() && emptyData->GetEmptyBlock() == m_blockStack.Peek()->GetBlock());
#endif
    VisitExpression(&block->ControlVaribleTempInitializer);    
    VisitExpression(&block->Initializer);

    PushBlock(top);
    
    VisitExpression(&block->CurrentIndexAssignment);
    VisitExpression(&block->ControlVariableReference);
    VisitStatement(block->IncrementStatement);

    VisitExpression(&block->Conditional);	
}

void ClosureRoot::VisitLoop ( ILTree::LoopBlock *block)
{
    // Dev10 #576786 Even though Conditional is stored inside
    // ILTree::LoopBlock structure, it is actually evaluated outside of the loop block. 
    // So, it should be visited in context of the parent block.
    LocalScopeInjection top;
    PopBlock(top);
    
    VisitExpression(&block->Conditional);
    
    PushBlock(top);
}

void ClosureRoot::VisitIf ( ILTree::IfBlock *block)
{
    // Dev10 #576786 Even though Conditional is stored inside
    // ILTree::IfBlock structure, it is actually evaluated outside of the block. 
    // So, it should be visited in context of the parent block.
    LocalScopeInjection top;
    PopBlock(top);
    
    VisitExpression(&block->Conditional);
    
    PushBlock(top);
}

void ClosureRoot::VisitWith ( ILTree::WithBlock *block)
{
    // Dev10 #576786 Even though WithValue is stored inside
    // ILTree::WithBlock structure, it is actually evaluated outside of the block. 
    // So, it should be visited in context of the parent block.
    LocalScopeInjection top;
    PopBlock(top);

    VisitExpression(&block->WithValue);

    PushBlock(top);

    VisitExpression(&block->TemporaryClearAssignment);
    VisitExpression(&block->RecordReference);
}

void ClosureRoot::VisitCase ( ILTree::CaseBlock *block)
{
    if (block->uFlags & SBF_CASE_CONDITION)
    {
        // Dev10 #576786 Even though Conditional is stored inside
        // ILTree::CaseBlock structure, it is actually evaluated outside of the block. 
        // So, it should be visited in context of the parent block.
        LocalScopeInjection top;
        PopBlock(top);
        
        VisitExpression(&block->Conditional);

        PushBlock(top);
    }
}

bool ClosureRoot::StartStatement(ILTree::Statement *stmt)
{
    ThrowIfNull(stmt);

    if (IsBad(stmt))
    {
        return false;
    }

    m_statementStack.PushOrEnqueue(stmt);

    switch ( stmt->bilop )
    {
    case SL_RESUME:
    case SL_GOTO:
    case SL_ON_ERR:
        // Note the procedure contains at least on goto.  We will need to
        // examine this procedure further to look for shim locations and
        // illegal gotos
        m_procContainsGoto = true;
        break;
    case SB_CATCH:
        {
            // For catch blocks we need to track the catch variable
            ILTree::CatchBlock *catchBlock = &stmt->AsCatchBlock();
            Variable *var = GetVariableFromExpression(catchBlock->CatchVariableTree);
            if ( var )
            {
                AddVariableFlag(var, CL_Catch);
                m_catchMap.SetValue(&stmt->AsCatchBlock(), var);
            }
        }
        break;
    }

    return true;
}

void ClosureRoot::EndStatement(ILTree::Statement *stmt)
{
    ThrowIfNull(stmt);

    m_statementStack.PopOrDequeue();

}

bool ClosureRoot::StartExpression(ILTree::Expression **ppExpr)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);

    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;
    switch ( expr->bilop )
    {
        case SX_SYM:
        {
            ILTree::SymbolReferenceExpression *SymbolRef = &expr->AsSymbolReferenceExpression();
            Declaration *Symbol = SymbolRef->Symbol;

            if (!Symbol)
            {
                break;
            }

            if (IsLocalParamOrMe(Symbol))
            {
                if ( InClosure() )
                {
                    // If the symbol is a local variable in an outer procedure then we need to
                    // track it for lifting.  A null procedure indicates we've hit a closure
                    // block that is not associated with an inner function (usually an
                    // expression tree).  No additional test is needed in this case.
                    Variable *var = Symbol->PVariable();
                    if ( IsLocalOrParamFromOutsideFrame(var))
                    {
                        this->LiftVariable(var, SymbolRef);
                    }
                }

                this->TrackExpression(ppExpr);
            }
            else if (Symbol->IsProc())
            {
                Procedure *ReferencedProc = Symbol->PProc();

                AssertIfFalse(NULL == SymbolRef->BaseReference);
                this->TrackExpression(ppExpr);
            }
        }
        break;
    case SX_LAMBDA:
        {
            ILTree::LambdaExpression *Lambda = &expr->AsLambdaExpression();
            if (!Lambda->GetLambdaBody() || IsBad(Lambda->GetLambdaBody()))
            {
                break;
            }

            // If we're in an expression tree, don't create any more closures
            if ( !InExpressionTree() )
            {
                // If we're not in an expression tree and we encounter a lambda then
                // we need to go ahead and push
                PushClosureFrame(ppExpr);
            }
            else
            {
                // Make sure to update the containing procedure for lambda temporaries
                // when we hit an expression tree.  It's possible for an expression tree
                // to be refactored into another procedure (as a result of being inside a lambda)
                // and it needs to have the parent reset accordingly
                Lambda->TemporaryManager->ChangeContainingProcedure(this->GetCurrentReferencingProcedure());

                // Dev10#530887: even multiline lambdas in expression-trees need parents! See the comment
                // in ClosureRoot::FixupSymbolReferences, with the same bug number. 
                // THIS IS A HACK. See comment in Semantics::IsConvertibleToExpressionTree with the same
                // bug number of an explanation.
                if (Lambda->IsStatementLambda)
                {
                    Lambda->GetStatementLambdaBody()->Parent = GetCurrentBlock();
                }

                // If we're in an expression tree throw a fake block onto the stack.
                // This exists because we have actually entered a new block (in terms
                // of locals, parameters, etc ...) in terms of closures.
                PushBlock(NULL, Lambda->GetLocalsHash(), Lambda->TemporaryManager);
            }
        }
        break;
    case SX_WIDE_COERCE:
        {
            // Store the expression tree to be converted and convert it
            // after we lift locals, but only store the top level expressions.
            ILTree::LambdaExpression *exprLambda = NULL;
            if (GetSemantics()->IsConvertibleToExpressionTree(
                    expr->AsExpressionWithChildren().ResultType,
                    expr->AsExpressionWithChildren().Left,
                    &exprLambda) && exprLambda )
            {
                if (m_lambdaNestLevel == 0)
                {
                    PushClosureFrame(ppExpr);
                }

                ThrowIfFalse(InExpressionTree());
                m_lambdaNestLevel += 1;
            }
        }
        break;
    case SX_CALL:
        {
            // If this is a MyBase/MyClass call, track it for later fixup if we are
            // in a closure
            ILTree::CallExpression &call = expr->AsCallExpression();
            if ( InClosure() && call.MeArgument && IsMyBaseMyClassSymbol(call.MeArgument) )
            {
                m_mbcCallList.AddLast(expr);
            }
        }
        break;
    case SX_DELEGATE_CTOR_CALL:
        {
            // If this is delegate constructor where MyBase/MyClass is being used to
            // access the method and we are in a closure then we need to fixup this call
            ILTree::DelegateConstructorCallExpression &delCall = expr->AsDelegateConstructorCallExpression();
            if ( InClosure()
                && delCall.ObjectArgument
                && delCall.ObjectArgument->IsExprNode()
                && IsMyBaseMyClassSymbol(&delCall.ObjectArgument->AsExpression()))
            {
                m_mbcCallList.AddLast(expr);
            }
        }
        break;
    }


    return true;
}

void ClosureRoot::EndExpression(ILTree::Expression **ppExpr)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);

    ILTree::Expression *expr = *ppExpr;
    switch ( expr->bilop )
    {
        case SX_LAMBDA:
            if ( !InExpressionTree() )
            {
                // Pop the corresponding frame if we're not in an expression tree
                PopClosureFrame();
            }
            else
            {
                // Pop the fake block we added for expression tree lambdas
                PopBlock();
            }

            break;
        case SX_WIDE_COERCE:
            if ( GetSemantics()->IsConvertibleToExpressionTree(
                expr->AsExpressionWithChildren().ResultType,
                expr->AsExpressionWithChildren().Left ) )
            {
                m_lambdaNestLevel -= 1;
                VSASSERT( m_lambdaNestLevel != 0xFFFFFFFF, "Underflow with lambda conversions to expression trees" );

                // If this is the topmost expression tree pop the frame
                if ( 0 == m_lambdaNestLevel )
                {
                    PopClosureFrame();
                }
            }
            break;
    }
}


//==============================================================================
// End Iterator Methods
//==============================================================================

//==============================================================================
// Change to the specified state
//==============================================================================
void ClosureRoot::ChangeState(ClosureRootState newState)
{
    ThrowIfFalse(newState > m_state);
    m_state = newState;
}

//==============================================================================
// Ensure the ClosureRoot is in the specified state
//==============================================================================
void ClosureRoot::EnsureInState(ClosureRootState state) const
{
    ThrowIfFalse(state == m_state);
}

//==============================================================================
// Ensure the ClosureRoot is past the specified state
//==============================================================================
void ClosureRoot::EnsurePastState(ClosureRootState state) const
{
    ThrowIfFalse( static_cast<int>(m_state) > static_cast<int>(state));
}

//==============================================================================
// Ensure the ClosureRoot is past or in the specified state
//==============================================================================
void ClosureRoot::EnsurePastOrInState(ClosureRootState state) const
{
    ThrowIfFalse( static_cast<int>(m_state) >= static_cast<int>(state));
}

//==============================================================================
// Ensure the ClosureRoot is before the specified state
//==============================================================================
void ClosureRoot::EnsureBeforeState(ClosureRootState state) const
{
    ThrowIfFalse( static_cast<int>(m_state) < static_cast<int>(state));
}

//==============================================================================
// Called when a closure block is entered.  Pushes a frame onto the stack
//==============================================================================
void ClosureRoot::PushClosureFrame(ILTree::Expression **expr)
{
    ThrowIfFalse(expr && *expr);
    EnsureInState(CRState_Iterating);
    Assume(GetCurrentBlock(), L"Cannot push a closure frame when no block is currently pushed");

    // Get the current block
    ILTree::ExecutableBlock *block = GetCurrentBlock();

    // Track the expression
    Procedure *proc = NULL;
    LambdaData *lambdaData = NULL;
    ClosureFrameType frameType;

    switch ( (*expr)->bilop)
    {
    case SX_LAMBDA:
        proc = ConvertLambdaToProcedure(&(*expr)->AsLambdaExpression());
        lambdaData = new (*GetNorlsAllocator()) LambdaData(
                TrackExpression(expr),
                proc,
                GetCurrentReferencingProcedure());
        m_realLambdaMap.SetValue(proc, lambdaData);
        m_lambdaToDataMap.SetValue(expr, lambdaData);
        frameType = FrameTypeLambda;
        break;
    case SX_WIDE_COERCE:
        {
            ThrowIfTrue(InExpressionTree());    // Cannot nest expression trees
            ThrowIfFalse(SX_LAMBDA == (*expr)->AsExpressionWithChildren().Left->bilop);

            // Dev10 #526220
            // If the SX_WIDE_COERCE node was the top level node of Lambda's body, 
            // its pointer (the value of Expr) may be copied by value into procedure 
            // created for the Lambda (see, ClosureRoot::ConvertLambdaToProcedure). 
            // The trick with patching '*expr' in ClosureRoot::FixupExpressionTrees passed in 'expr' 
            // doesn't patch that copy and we end up with lambda procedure that still contains 
            // the SX_WIDE_COERCE and SX_LAMBDA nodes in its body.
            // It looks like we are lucky and we can easily find that procedure by calling GetCurrentReferencingProcedure().
            // Let's track a copy if one was created.
            ILTree::Expression **exprToTrack = expr;
            
            Procedure * owningProc = GetCurrentReferencingProcedure();
            if (owningProc && owningProc->IsSyntheticMethod())
            {
                BCSYM_SyntheticMethod * synthProc = owningProc->PSyntheticMethod();

                if (synthProc->IsLambda())
                {
                    ILTree::ProcedureBlock * block = synthProc->GetBoundTree();

                    if (block && block->Child)
                    {
                        if (block->Child->bilop == SL_RETURN)
                        {
                            if (block->Child->AsReturnStatement().ReturnExpression == *expr)
                            {
                                exprToTrack = (ILTree::Expression **)&block->Child->AsReturnStatement().ReturnExpression;
                            }
                        }
                        else if(block->Child->bilop == SL_STMT) 
                        {
                            if (block->Child->AsStatementWithExpression().Operand1 == *expr)
                            {
                                exprToTrack = &block->Child->AsStatementWithExpression().Operand1;
                            }
                        }
                    }
                }
            }
        
            
            TrackExpression(exprToTrack);

            lambdaData = new (*GetNorlsAllocator()) LambdaData(
                TrackExpression(&(*exprToTrack)->AsExpressionWithChildren().Left),
                NULL,
                GetCurrentReferencingProcedure());
            lambdaData->SetLambdaType(LambdaType_ExpressionTree);
            m_lambdaToDataMap.SetValue(&(*exprToTrack)->AsExpressionWithChildren().Left, lambdaData);
            frameType = FrameTypeExpressionTree;
        }
        break;
    default:
        frameType = FrameTypeNone;  // Stop compiler warning
        ThrowIfFalse(false);    // It's only possible to push on a closure frame for a Lambda or an
                                // expression tree
    }

    ClosureFrame *frame = new (*GetNorlsAllocator()) ClosureFrame(frameType, block, lambdaData);
    m_frameStack.PushOrEnqueue(frame);

    // If there is now an associated procedure for this expression, push
    // the block onto the stack since it can contain locals
    if ( proc && proc->GetBoundTree())
    {
        ILTree::ProcedureBlock *body = proc->GetBoundTree();
        PushBlock(body, body->Locals, body->ptempmgr);
    }
}

void ClosureRoot::PopClosureFrame()
{
    EnsureInState(CRState_Iterating);
    ClosureFrame *last = m_frameStack.Peek();
    m_frameStack.PopOrDequeue();

    // If the last frame is a Lambda frame then make sure to pop the block associated
    // with the procedure as well
    if ( FrameTypeLambda == last->GetClosureFrameType() )
    {
        ThrowIfNull(last->GetLambdaData());
        ThrowIfNull(last->GetLambdaData()->GetProcedure()->GetBoundTree());
        ThrowIfFalse(GetCurrentBlock() == last->GetLambdaData()->GetProcedure()->GetBoundTree());
        PopBlock();
    }
}

Procedure *ClosureRoot::ConvertLambdaToProcedure(ILTree::LambdaExpression *expr)
{
    ThrowIfNull(expr);
    // Port SP1 CL 2922610 to VS10
    // If we generate a return statement, we are creating a copy of the pointer.
    // the closures tracked expression store the address of the pointer and will rewrite that memory
    // location.
    // Alternative implementation would be to build a list of extra locations to fixup in the tracked
    // expressions.
    ThrowIfTrue(!expr->IsStatementLambda && (expr->GetExpressionLambdaBody()->bilop == SX_LAMBDA ||expr->GetExpressionLambdaBody()->bilop == SX_UNBOUND_LAMBDA));

    TransientSymbolStore *store = GetTransientSymbolStore();

    // !! Do not change this order of buidling this name.
    // Other teams (FxCop) assume that lambda's will start with LAMBDA_PREFIX to identify lambda functions in its analysis
    // If you have to change this naming scheme, please contact the VB langauge PM and consider the impact of that break.
    StringBuffer buf;
    buf.AppendPrintf(LAMBDA_PREFIX L"_%u", store->IncrementLambdaCount());
    STRING *name = GetCompiler()->AddString(&buf);

    //Note: Don't use m_proc->GetContainingClass() as it doesn't deal with partial classes correctly and we can end up pointing at the wrong sourcefile
    BCSYM_Container *outerClass  = m_proc->GetPhysicalContainer(); 

    // Port SP1 CL 2955440 to VS10
    // Bug #150058 - DevDiv Bugs
    // For a field initializer Semantics::InitializeFields creates an assignment statement and puts it into 
    // constructor, which for partial classes may be linked to a different PhysicalContainer.
    // The assignment statement will have proper file stored inside SL_STMT node (see Semantics::InitializeFields).
    // If the current lambda belongs to SL_STMT with the file associated with it, let's locate the right PhysicalContainer
    // based on that information.
    if(outerClass->IsPartialType() || outerClass->GetNextPartialType())
    {
        if(this->m_statementStack.Count() > 0)
        {
            // the current statement being processed should be on top of the stack
            ILTree::Statement* pStmt = this->m_statementStack.Peek();

            AssertIfNull(pStmt);
            if (pStmt && SL_STMT == pStmt->bilop)
            {
                SourceFile * pFile = pStmt->AsStatementWithExpression().GetContainingFile();

                if(pFile)
                {
                    BCSYM_Container * pBestOuterClass = NULL;
                        
                    // Start the walk with the main type.
                    BCSYM_Container * pNewOuterClass = outerClass->GetMainType();

                    if(!pNewOuterClass) 
                    {  
                        AssertIfTrue(outerClass && outerClass->IsPartialType()); // Must be a main partial class
                        pNewOuterClass = outerClass;
                    }

                    while (pNewOuterClass)
                    {
                        if(pFile == pNewOuterClass->GetSourceFile())
                        {
                            AssertIfFalse(pNewOuterClass->HasLocation()); // Partial class is coming from the source, must have a location.
                            if(pNewOuterClass->HasLocation())
                            {
                                Location * pContainerLocation = pNewOuterClass->GetLocation();

                                AssertIfNull(pContainerLocation);
                                if(pContainerLocation && Location::CompareStartPoints(pContainerLocation, &expr->Loc) < 0)
                                {
                                    // this may be our PhysicalContainer
                                    if(!pBestOuterClass || Location::CompareStartPoints(pContainerLocation, pBestOuterClass->GetLocation()) > 0)
                                    {
                                        pBestOuterClass = pNewOuterClass;
                                    }
                                }
                            }
                        }

                        pNewOuterClass = pNewOuterClass->GetNextPartialType();
                    }

                    AssertIfNull(pBestOuterClass); // How come we didn't find it?
                    if(pBestOuterClass)
                    {
                        outerClass = pBestOuterClass;
                    }
                }
            }
        }
    }
    

    // Copy the parameters from the LambdaExpression (since they were not allocated using the transient symbol factory)
    BCSYM_Param *pFirstParam = Declared::CloneParameters(expr->FirstParameter, false, NULL, GetTransientSymbolFactory());

    bool DelegateIsSub = false;

    Declaration *InvokeMethod = GetInvokeFromDelegate(expr->ResultType, GetCompiler());
    if (InvokeMethod != NULL && !IsBad(InvokeMethod) && InvokeMethod->IsProc())
    {
        DelegateIsSub = IsSub(InvokeMethod->PProc());
    }

    // Create the procedure stub
    //
    // Notice that the ResultType is simply taken from the ResultType of the Lambda Expression.
    // There is a lifetime issue here if the ResultType is a generic binding.  The new proc
    // is being added to the transient symbol list where it will have a lifetime greater
    // than any generic binding which could be LambdaExpression->ResultType
    //
    // However we are in a chicken and egg problem here.  The lambdas need to be reparented
    // for traversal but the closure class is needed to perform the rebind.  The closure class
    // cannot be created until traversal is complete and we know which closure class is the
    // root one (and hence has the generic parameters needed for rebinding).
    //
    // To get aronud this issue the type is left as is for now.  It will be fixed up in
    // FixupLambdaReferences().  The lifetime of ->ResultType is still valid at this point
    //
    // The same goes for other metadata in the proc.  Even though the parameters are cloned
    // on the transient symbol allocator they could still be generics and need to be deeply
    // copied.  This will also occur in FixupLambdaReferences()
    BCSYM_SyntheticMethod *proc = GetTransientSymbolFactory()->AllocSyntheticMethod(false);

    Symbols::SetParent(proc, outerClass->GetUnBindableChildrenHash());

    BCSYM *pReturnType = NULL;
    BCSYM_Param *pReturnTypeParam = NULL;

    if (!DelegateIsSub)
    {
        if (expr->IsStatementLambda)
        {
            Declaration *InvokeMethod = GetInvokeFromDelegate(expr->ResultType, m_semantics->m_Compiler);
            GenericTypeBinding *DelegateBindingContext = TypeHelpers::IsGenericTypeBinding(expr->ResultType) ? expr->ResultType->PGenericTypeBinding() : NULL;
            BCSYM_Proc *DelegateProc = InvokeMethod->PProc();
            Type *DelegateReturnType = DelegateProc->GetType();
            pReturnType = ReplaceGenericParametersWithArguments(DelegateReturnType, DelegateBindingContext, m_semantics->m_SymbolCreator);
        }
        else
        {
            pReturnType = expr->GetExpressionLambdaReturnType();
            // Since delegate is not sub, we should not get void type.
            VSASSERT(pReturnType != TypeHelpers::GetVoidType(), "How come a function lambda is void?");
        }
    }

    GetTransientSymbolFactory()->GetProc(NULL,
        name,
        name,
        (CodeBlockLocation*) NULL,
        (CodeBlockLocation*) NULL,
        DECLF_Public | DECLF_Hidden | DECLF_SpecialName | DECLF_Function | DECLF_HasRetval | DECLF_AllowOptional |
              (expr->IsAsyncKeywordUsed ? DECLF_Async : 0) | (expr->IsIteratorKeywordUsed ? DECLF_Iterator : 0),
        pReturnType,
        pFirstParam,
        pReturnTypeParam,
        NULL,
        NULL,
        SYNTH_TransientSymbol,
        NULL,
        NULL,
        proc);
    
    // Dev10#680221: Always set the span for the proc so ENC can update line numbers properly
    proc->SetWholeSpan(expr->AsLambdaExpression().Loc);
    proc->SetBodySpan(expr->AsLambdaExpression().BodySpan);

    proc->SetIsLambda();
    proc->SetIsRelaxedDelegateLambda(expr->IsRelaxedDelegateLambda);

    // 
    GetSemantics()->RegisterTransientSymbol(proc);

    NorlsAllocator *treeStorage = &(GetSemantics()->m_TreeStorage);
    BILALLOC *treeAllocator = &(GetSemantics()->m_TreeAllocator);
    ILTree::Statement* lambdaBodyChildStatement = NULL;

    Variable* returnVariable = NULL;
    if (!DelegateIsSub)
    {
        // Now the symbol is generate.  Update the actual biltree of the proc
        returnVariable = Declared::MakeReturnLocal(GetCompiler(), treeStorage, proc, NULL);
        Symbols::AddSymbolToHash(expr->GetLocalsHash(), returnVariable, true, false, false);    

    }
    
    // We do not need to initialize lambdaBodyChildStatement for multiline lambdas because we do not use it generate the procedure.
    if (!expr->IsStatementLambda)
    {
        // Dev10 #526220
        // ATTENTION !!!  ATTENTION !!! ATTENTION !!!
        // if you change the shape of lambdaBodyChildStatement generated below,
        // make sure to adjust code in ClosureRoot::PushClosureFrame accordingly.
    
        if (DelegateIsSub)
        {
            // Lambda is a Sub.
            ILTree::Statement *LambdaStatement = &(treeAllocator->xAllocBilNode(SL_STMT)->AsStatement());
            LambdaStatement->Loc = expr->Loc;
            LambdaStatement->ResumeIndex = NoResume;
            LambdaStatement->AsStatementWithExpression().Operand1 = expr->GetExpressionLambdaBody();

            // Set the NoItemOnStack flag for lambda's that are inside a sub.
            // We make this a sub by generating a statement containing the lambda expression
            // and then a return statement. We have to pass this flag because only method calls
            // know how to generate a POP of the returntype of the expression is void. Other expressions
            // like 1+1 will leave a value on the stack. This flag will ensure in codegen that the stack
            // does not grow. which would result in unverifyable IL.
            SetFlag32(LambdaStatement, SXF_STMT_ENSURE_NO_ITEMS_ADDED_TO_STACK_BY_POPPING);

            ILTree::ReturnStatement *Ret = &(treeAllocator->xAllocBilNode(SL_RETURN)->AsReturnStatement());
            Ret->Loc = expr->Loc;

            Ret->Prev = LambdaStatement;
            LambdaStatement->Next = Ret;

            lambdaBodyChildStatement = LambdaStatement;
            returnVariable = NULL;
        }
        else
        {
            // Lambda is a function
            // Generate the return statement.
            ILTree::ReturnStatement *Ret = &(treeAllocator->xAllocBilNode(SL_RETURN)->AsReturnStatement());
            Ret->Loc = expr->Loc;
            Ret->ReturnExpression = GetSemantics()->MakeRValue(expr->GetExpressionLambdaBody());
            lambdaBodyChildStatement = Ret;
        }
    }

    // Create the SB_PROC node.
    ILTree::ProcedureBlock *LambdaBody = &(treeAllocator->xAllocBilNode(SB_PROC)->AsProcedureBlock());
    LambdaBody->Loc = expr->Loc;
    LambdaBody->pproc = proc;
    LambdaBody->Locals = expr->GetLocalsHash();
    LambdaBody->ptempmgr = expr->TemporaryManager;
    LambdaBody->Child = expr->IsStatementLambda ? expr->GetStatementLambdaBody(): lambdaBodyChildStatement;
    LambdaBody->ReturnVariable = returnVariable;
    LambdaBody->m_ResumableKind = expr->m_ResumableKind;
    LambdaBody->m_ResumableGenericType = expr->m_ResumableGenericType;


    // We must reparent the statement block for the statement lambda to point to the procedure.
    if (expr->IsStatementLambda)
    {
        expr->GetStatementLambdaBody()->Parent = LambdaBody;
        LambdaBody->fSeenCatch = expr->GetStatementLambdaBody()->fSeenCatch;
    }

    // When the temporary manager for the lambda was originally created, there was not a
    // corresponding BCSYM_Proc so the containing method was used instead.  Now go through
    // and make sure that each temporary is reset to have the newly created BCSYM_Proc
    // as it's parent.  This allows navigation from a BCSYM_Variable, which is a temporary,
    // to it's owning temporary manager.
    LambdaBody->ptempmgr->ChangeContainingProcedure(proc);

    // Reparent the hash on the lambda so thot we can walk from the locals to the
    // created proc
    Symbols::ClearParent(expr->GetLocalsHash());
    Symbols::SetParent(expr->GetLocalsHash(), proc);

    proc->SetBoundTree(LambdaBody); 

    if (expr->IsStatementLambda)
    {
        BackupValue<ILTree::ProcedureBlock*> backup_ProcedureTree(&m_semantics->m_ProcedureTree);
        BackupValue<BCSYM_Proc*> backup_Procedure(&m_semantics->m_Procedure);

        m_semantics->m_ProcedureTree = LambdaBody;
        m_semantics->m_Procedure = proc;
        m_semantics->CheckFlow(LambdaBody, true);
        // "true" means "only check returns". The use/assignment flow checks have already been checked
        // in the context of where the closure was written.
    }

    return proc;
}

void ClosureRoot::PushBlock(ILTree::ExecutableBlock *block, Scope *locals, TemporaryManager *tempManager)
{
    EnsureInState(CRState_Iterating);
    BlockData *data = new (*GetNorlsAllocator()) BlockData(block, locals, tempManager);
    m_blockStack.PushOrEnqueue(data);
}

void ClosureRoot::PushBlock(BlockData *data)
{
    EnsureInState(CRState_Iterating);
    m_blockStack.PushOrEnqueue(data);
}

BlockData* ClosureRoot::PopBlock()
{
    EnsureInState(CRState_Iterating);
    ThrowIfTrue(0 == m_blockStack.Count());
    return m_blockStack.PopOrDequeue();
}

ILTree::ExecutableBlock *ClosureRoot::GetCurrentBlock() const
{
    EnsureInState(CRState_Iterating);
    ConstIterator<BlockData*> it = m_blockStack.GetConstIterator();
    while ( it.MoveNext() )
    {
        if ( it.Current()->GetBlock() )
        {
            return it.Current()->GetBlock();
        }
    }

    return NULL;
}

Procedure *ClosureRoot::GetCurrentReferencingProcedure() const
{
    EnsureInState(CRState_Iterating);

    if ( m_frameStack.Count() > 0 )
    {
        // Walk the frame stack backwards until we find the first
        // Lambda and return its procedure
        ConstIterator<ClosureFrame*> it = m_frameStack.GetConstIterator();
        while ( it.MoveNext() )
        {
            ClosureFrame *cur = it.Current();
            if ( FrameTypeLambda == cur->GetClosureFrameType() )
            {
                return cur->GetLambdaData()->GetProcedure();
            }
        }
    }

    return m_proc;
}

bool ClosureRoot::InGeneric() const
{
    if ( m_proc->IsGeneric() ||
            (m_proc->GetContainingClass() && m_proc->GetContainingClass()->IsGeneric()) )
    {
        return true;
    }

    return false;
}

ILTree::ProcedureBlock *ClosureRoot::GetProcedureBody(Procedure *proc)
{
    AssertIfNull(proc);
    if ( proc == m_proc )
    {
        return m_procBody;
    }
    else
    {
        return &proc->GetBoundTree()->AsProcedureBlock();
    }
}

//==============================================================================
// Like the name says, get or create a Closure for the current block
//
// private:
//==============================================================================
Closure *ClosureRoot::GetOrCreateClosure(ILTree::ExecutableBlock *block)
{
    AssertIfNull(block);

    Closure *found = GetClosure(block);
    if ( found )
    {
        return found;
    }

    Procedure *containingProc = NULL;
    ILTree::ExecutableBlock *current = block;
    while (current)
    {
        if ( current->bilop == SB_PROC )
        {
            containingProc = current->AsProcedureBlock().pproc;
            break;
        }

        current = current->Parent;
    }

    if ( !current )
    {
        VSASSERT(false, "Could not find the containing proc");
        containingProc = m_proc;
    }

    // If there is a lambda associated with this block, we need to update
    // the count of the closure variables which exist within the lambda.
    LambdaData *lambdaData = NULL;
    if ( m_realLambdaMap.GetValue(containingProc, &lambdaData))
    {
        lambdaData->SetHasClosureVariable(true);
    }

    Closure *created = new (*GetNorlsAllocator())Closure(containingProc, block, this);
    m_map.SetValue(block, created);
    m_nearestMap.Clear();   // Clear m_nearestMap whenever a new closure is created
                            // because adding a new closure automatically invalidates
                            // this map.
    return created;
}


//==============================================================================
// Get the closure for the specified block
//
// public:
//==============================================================================
Closure *ClosureRoot::GetClosure(ILTree::ExecutableBlock *block)
{
    ThrowIfNull(block);

    Closure *found = NULL;
    if (m_map.GetValue(block, &found) )
    {
        return found;
    }

    return NULL;
}

const Closure *ClosureRoot::GetClosure(ILTree::ExecutableBlock *block) const
{
    return const_cast<ClosureRoot*>(this)->GetClosure(block);
}

//==============================================================================
// Get the closure for the body of the specified procedure
//==============================================================================
Closure *ClosureRoot::GetClosure(Procedure *proc)
{
    ThrowIfNull(proc);
    ILTree::ProcedureBlock *body = GetProcedureBody(proc);
    ThrowIfNull(proc);
    return GetClosure(body);
}

const Closure *ClosureRoot::GetClosure(Procedure *proc) const
{
    return const_cast<ClosureRoot*>(this)->GetClosure(proc);
}

//==============================================================================
// Gets the body for the passed in procedure.
//
// public:
//==============================================================================
ILTree::ProcedureBlock *ClosureRoot::GetProcedureBody(Procedure *proc) const
{
    ThrowIfNull(proc);

    if ( m_proc == proc )
    {
        return m_procBody;
    }
    else
    {
        // If it's not the root procedure then it's a bound lambda so
        // just return the bound tree
        return proc->GetBoundTree();
    }
}

//==============================================================================
// Lifts the specified variable
//
// public:
//==============================================================================
void ClosureRoot::LiftVariable(Variable *var, ILTree::SymbolReferenceExpression *symRef)
{
    ThrowIfNull(var);
    ThrowIfNull(symRef);
    ThrowIfFalse(InClosure());
    ThrowIfNull(GetCurrentBlock());

    // Get the current lambda.  In this version we don't support expression trees outside of a lambda so there must
    // be a lambda when we create a closure
    LambdaData *lambdaData = m_frameStack.Peek()->GetLambdaData();
    ThrowIfNull(lambdaData);

    // Find the current real lambda we are in and update it's variable use
    Iterator<ClosureFrame*> frameIt = m_frameStack.GetIterator();
    while ( frameIt.MoveNext() )
    {
        ClosureFrame *cur = frameIt.Current();
        if ( FrameTypeLambda == cur->GetClosureFrameType() )
        {
            LambdaData *realLambda = cur->GetLambdaData();
            if ( var->IsMe() )
            {
                realLambda->SetUsesMe(true);
            }
            else
            {
                realLambda->SetUsesLiftedVariable(true);
            }
            break;
        }
    }

    // Make sure this variable is safe to be lifted.
    this->CheckVariableSafeForLifting(lambdaData, symRef);

    // Me will be lifted on an as needed during symbol reference fixup
    if ( var->IsMe() )
    {
        return;
    }

    ILTree::ExecutableBlock *found = NULL;
    Temporary *longLivedInfo = NULL;
    if ( var->IsTemporary() )
    {
        // Temporary's have special lifting semantics we need to respect here
        Temporary *temporary = var->GetTemporaryInfo();
        ThrowIfNull(temporary);
        switch ( temporary->Lifetime )
        {
        case LifetimeShortLived:
            {
                // ShortLived temporary's are only supposed to be active for the statement in
                // which they are used.  Because we are lifting the temporary we have
                // violated this semantic and need to replace it.  This will be done at a later
                // time by adding another ShortLived temporary at later points in the BILTREE.
                // Right now we need to record information about it.
                ShortLivedTemporaryData *data = NULL;
                if ( !m_shortLivedMap.GetValue(var, &data) )
                {
                    data = new (*GetNorlsAllocator()) ShortLivedTemporaryData(temporary, m_alloc);
                    m_shortLivedMap.SetValue(var, data);
                }

                // Dev10#677779: the owning statement might be anywhere up the callstack
                ILTree::Statement *owningStatement = FindOwningStatementOfShortLivedTemporary(m_statementStack, var);
                ThrowIfNull(owningStatement);

                // Record that the current statement lifts the temporary
                data->AddStatementThatLifts(owningStatement);

                // Don't actually lift the temporary here.  When lifting a ShortLived temporary, it
                // will be scoped to the parent of the statement which did the lift.  We will create
                // a new ShortLived temporary for every instance it's lifted so this will created
                // multiple closures.
                //
                // When lifting a ShortLived temporary in a statement we need to create a hidden
                // block to ensure the semantics are correct in the presence of goto's.  Make sure
                // this block is available and the closure is associated with it.  The actual
                // lifting will occur when we fixup ShortLived temporaries
                EmptyBlockData *emptyData = GetOrCreateEmptyBlockDataForGroup(
                        owningStatement,
                        GetCurrentReferencingProcedure());
                emptyData->SetIsNeeded();
                GetOrCreateClosure(emptyData->GetEmptyBlock());
                return;
            }
            break;
        case LifetimeNone:
        case LifetimeDefaultValue:
            // LifetimeNone and LifetimeDefaultValue variables are typically used for code gen helpers.  Lifting them
            // will produce unexpected results (most likely unverifiable code).
            Assume(false, L"LifetimeNone or LifetimeDefaultValue temporaries should not be lifted");
            break;
        case LifetimeLongLived:
            longLivedInfo = temporary;
            found = temporary->Block;
            break;
        default:
            Assume(false, L"Unexpected temporary type");
            break;
        }
    }
    else
    {
        // Find the block that owns the Variable and make sure there is a closure
        // associated with it.
        Declaration *parent = var->GetImmediateParent();
        if ( parent )
        {
            Iterator<BlockData*> it = m_blockStack.GetIterator();
            LocalScopeInjection * injection = m_LocalScopesToInject;  // #576786
            while ( it.MoveNext() )
            {
                BlockData *data = it.Current();
                BlockData *blockInStack = data;
                bool leave = false;

                do
                {
                    if ( data->GetLocals() == parent )
                    {
                        ThrowIfNull(data->GetBlock());  // The only time the block is NULL is
                                                        // in the middle of expression trees and
                                                        // it's not possible to lift anything from
                                                        // an expression tree block
                        found = data->GetBlock();
                        leave = true;
                        break;
                    }

                    data = NULL;

                    // #576786 Descent into corresponding blocks from m_LocalScopesToInject.
                    // If we hit any of their locals, that local should be transferred into the parent in
                    // m_blockStack.
                    if (injection && injection->blockInStack == blockInStack)
                    {
                        data = injection->localScopeToInject;
                        injection = injection->previousInjection;
                    }
                }
                while (data);

                if (leave)
                {
                    if (data != blockInStack)
                    {
                        // #576786 How could we transfer a variable, which is not a control variable? Code below doesn't handle that.
                        switch ( found->bilop )
                        {
                        case SB_FOR:
                            if ( var != GetVariableFromExpression(found->AsForBlock().ControlVariableReference) )
                            {
                                found = NULL;
                            }
                            break;
                            
                         default:
                            found = NULL;
                            break;
                        }
                    }
                
                    break;
                }
            }

            AssertIfFalse(found || injection == NULL);
        }
    }

    // By this point we should have the block definining the variable
    ThrowIfNull(found);

    // Determine if this variable needs to be lifted into a hidden scope.  In either
    // of these cases make sure to output the warning for lifting a control variable
    bool needHiddenScope = false;
    switch ( found->bilop )
    {
    case SB_FOR:
        if ( var == GetVariableFromExpression(found->AsForBlock().ControlVariableReference) )
        {
            needHiddenScope = true;
            m_semantics->ReportSemanticError(
                    lambdaData->GetLambda()->IsPartOfQuery
                        ? WRNID_LiftControlVariableQuery
                        : WRNID_LiftControlVariableLambda,
                    symRef->Loc,
                    var);
        }
        break;
    }

    // Also if the variable is a long term temporary and we are in a compound executable
    // block, we need the hidden scope.  The reason being that compound blocks have
    // several statements which ...
    //  1) Execute outside their block (both before and after)
    //  2) Operate on temporaries associated with their block
    // Not moving these into a hidden scope causes problems rannging from Null Reference
    // Exceptions to sutble semantic bugs
    if ( longLivedInfo && ::IsSpecialScopeExecutableBlock(found) )
    {
        needHiddenScope = true;
    }

    if (needHiddenScope)
    {
        // Some variables have special semantics by which they are tied to
        // a specific block as far as the user is concerned and able to
        // manipulate.  However we for our own purposes access this variable
        // outside it's actual block to do initialization and bounds checking.
        // To get the proper semantics with closures we need to move these
        // variables into a hidden block between the block actually containing
        // the variables and it's parent.
        //
        // Ex. If this is the for loop control variable we need to actually move
        // the variable up a block.  Really the for loop control variable does
        // not live inside the for block since it's accessed outside of it
        // for initialization and terminating condition check.
        //
        // To make the use consistent we create an empty parent block which
        // holds the control variable.  It's only purpose is scoping.  This
        // is already created for us during iteration but it's not
        // actually hooked into the BILTREE unless it's used.
        EmptyBlockData *emptyData = GetEmptyBlockDataForBlock(found);
        ThrowIfNull(emptyData);
        emptyData->SetIsNeeded();
        ILTree::ExecutableBlock *empty = emptyData->GetEmptyBlock();

        if ( !var->IsTemporary() )
        {
            Symbols::RemoveSymbolFromHash(found->Locals, var);

            if ( NULL == empty->Locals )
            {
                // Create the locals hash
                CreateLocalsHash(GetCurrentReferencingProcedure(), empty);
            }
            Symbols::AddSymbolToHash(empty->Locals, var, true, false, false);
        }
        else if ( longLivedInfo )
        {
            // Make sure to update the temporary info as well if this is a
            // long term temporary
            longLivedInfo->Block = empty;
        }

        found = empty;
    }

    Closure *closure = GetOrCreateClosure(found);
    closure->EnsureVariableLifted(var);
}

TrackedExpression *ClosureRoot::TrackExpression(ILTree::Expression **ppExpr)
{
    ThrowIfNull(ppExpr);
    ThrowIfNull(*ppExpr);
    EnsureInState(CRState_Iterating);

    Procedure *refProc = GetCurrentReferencingProcedure();
    ILTree::ExecutableBlock *block = GetCurrentBlock();

    TrackedExpression *tracked = new (*GetNorlsAllocator()) TrackedExpression(
            ppExpr,
            GetCurrentBlock(),
            m_statementStack.Peek(),
            GetCurrentReferencingProcedure());
    Assume(tracked->GetReferencingProcedure() != NULL, L"There should always be a referencing procedure");
    m_trackedList.AddLast(tracked);
    return tracked;
}

VariableFlags ClosureRoot::GetVariableFlags(Variable *var) const
{
    ThrowIfNull(var);

    VariableFlags flags = CL_None;
    if ( m_flagMap.GetValue(var, &flags) )
    {
        return flags;
    }

    return CL_None;
}

void ClosureRoot::AddVariableFlag(Variable *var, VariableFlags flag)
{
    ThrowIfNull(var);

    VariableFlags old = CL_None;
    if ( m_flagMap.GetValue(var, &old) )
    {
        m_flagMap.SetValue(var, static_cast<VariableFlags>(old | flag));
    }
    else
    {
        m_flagMap.SetValue(var, flag);
    }
}

void ClosureRoot::FixupExpressionTrees()
{
    Iterator<TrackedExpression*> it = m_trackedList.GetIterator();
    while (it.MoveNext())
    {
        TrackedExpression *cur = it.Current();
        if ( SX_WIDE_COERCE != cur->GetExpression()->bilop )
        {
            continue;
        }

        ILTree::Expression **ppExpr = cur->GetRawExpression();
        ILTree::Expression *Expr = *ppExpr;

        // Before calling into the expression tree code to convert the lambda,
        // we need to set semantics to use the appropriate temporary manager
        ILTree::ProcedureBlock *body = GetProcedureBody(cur->GetReferencingProcedure());
        ProcedureState savedState;
        ProcedureState state(body);
        m_semantics->GetProcedureState(&savedState);
        m_semantics->SetProcedureState(&state);
        *ppExpr = m_semantics->ConvertLambdaToExpressionTree(
                Expr->AsExpressionWithChildren().Left,
                ExprForceRValue,
                Expr->AsExpressionWithChildren().ResultType);
        m_semantics->SetProcedureState(&savedState);
    }
}

//==============================================================================
// Is this
//  1) A local
//  2) A Paramater
//  3) Me
// private:
//==============================================================================
bool ClosureRoot::IsLocalParamOrMe(Symbol *sym)
{
    AssertIfNull(sym);
    return sym->IsLocal() ||
           (sym->IsVariable() &&
            sym->PVariable()->IsMe());
}

//==============================================================================
// Is the local variable or paramater outside the current frame
//
// private:
//==============================================================================
bool ClosureRoot::IsLocalOrParamFromOutsideFrame(Variable* var)
{
    EnsureInState(CRState_Iterating);
    ThrowIfNull(var);
    ThrowIfFalse(var->IsLocal() || var->IsMe());
    ThrowIfTrue(m_frameStack.Count() == 0);

    ILTree::ExecutableBlock *boundary = m_frameStack.Peek()->GetBlock();
    if ( var->IsMe() )
    {
        // We're in a closure so using me definately crosses a frame :)
        return true;
    }

    ILTree::ExecutableBlock *tempBlock = NULL;
    if ( var->IsTemporary() )
    {
        ILTree::ProcedureBlock *body = GetProcedureBody(var->GetImmediateParent()->PProc());
        TemporaryManager *parentTmp = body->ptempmgr;
        TemporaryManager *tempmgr = var->GetTemporaryManager();
        if ( parentTmp != tempmgr )
        {
            // This can validly happen when we are in an expression tree.  When temporaries
            // are added to a lambda temporary manager the parent is set to the containing
            // method.  For expression trees we don't create a corresponding method so
            // we cannot access the temporary manager via this method.
            //
            // This is fine because we don't ever lift a temporary (or anything else) from
            // an expression tree.
            ThrowIfFalse(InExpressionTree());
            return false;
        }

        Temporary *ptemp = var->GetTemporaryInfo();
        ThrowIfNull(ptemp);
        if ( LifetimeLongLived == ptemp->Lifetime )
        {
            // LongLived temporaries are tied to a block
            tempBlock = ptemp->Block;
        }
        else
        {
            // All other temporaries are tied to the procedure
            tempBlock = body;
        }
    }

    // Walk the block stack until we
    //  1) Hit the boundary block in which case it's outside the frame
    //  2) Find the owner of the variable
    Iterator<BlockData*> it = m_blockStack.GetIterator();
    LocalScopeInjection * injection = m_LocalScopesToInject;
    while (it.MoveNext() )
    {
        BlockData *cur = it.Current();

        // #576786 If we've reached the boundary, we shouldn't look at the blocks
        // from m_LocalScopesToInject even if they are contained within the boundary 
        // block. They can't be boundary blocks themselves and, for the purpose of boundary  
        // checking, their locals are transferred into the immediate parent block stored in m_blockStack.
        if ( cur->GetBlock() == boundary )
        {
            return true;
        }
        else
        {
            BlockData * blockInStack = cur;

            do
            {
                if ( tempBlock && cur->GetBlock() == tempBlock )
                {
                    // Temporary exists before the block
                    return false;
                }
                else
                {
                    if ( cur->GetLocals() == var->GetImmediateParent() )
                    {
                        // Locals hash is before the boundary
                        return false;
                    }
                }

                cur = NULL;

                // #576786 Let's see if the local should be transferred into the block from m_blockStack.
                if(injection && injection->blockInStack == blockInStack)
                {
                    cur = injection->localScopeToInject;
                    injection = injection->previousInjection;
                }
            }
            while (cur);
        }
    }

    AssertIfFalse(injection == NULL);

    // Shouldn't ever get here
    Assume(false, L"Could not find the owner of the variable");
    return true;
}

//==============================================================================
// This method does all of the dirty work.
//==============================================================================
void ClosureRoot::CreateClosureCode()
{
    this->FixupEmptyBlocks();

    // Fixing up ShortLivedTemporaries needs to be done before the closure tree structure
    // is created.  Fixing up the ShortLived temporaries will cause the associated closures
    // to be created and actually lift the variables.
    this->FixupShortLivedTemporaries();

    // Now fixup all of the MyBase/MyClass calls to use stubs
    this->FixupMyBaseMyClassCalls();

    // At this point the BILTREE is well formed.
    ChangeState(CRState_BiltreeWellFormed);

#ifdef DEBUG
    if (VSFSWITCH(fDumpClosures))
    {
        DebPrintf("//============================================================================\n");
        DebPrintf("// Closure Bound ILTree::ILNode (Pre Fixup)\n");
        DebPrintf("//============================================================================\n");

        BILDUMP preDump(GetCompiler());
        preDump.DumpBlockTrees(m_procBody);
    }

#endif // DEBUG

    this->FixupClosureTreeStructure();
    this->CreateClosureClassCode();

    ChangeState(CRState_SymbolFixup);
    this->FixupSymbolReferences();
    this->FixupExpressionTrees();
    this->FixupLambdaReferences();
    ChangeState(CRState_PostSymbolFixup);

    this->CreateInitCode();
    this->FixupGotos();

#ifdef DEBUG
    if (VSFSWITCH(fDumpClosures))
    {
        DebPrintf("//============================================================================\n");
        DebPrintf("// Closure Bound Trees (Post Fixup)\n");
        DebPrintf("//============================================================================\n");

        BILDUMP postDump(GetCompiler());
        postDump.DumpBlockTrees(m_procBody);

        Iterator<LambdaData*> it = m_realLambdaMap.GetValueIterator();
        while ( it.MoveNext())
        {
            postDump.DumpBlockTrees(it.Current()->GetProcedure()->GetBoundTree());
        }

        ClosureDump dump;
        dump.DumpClosures(this);
    }
#endif // DEBUG
}

EmptyBlockData *ClosureRoot::GetEmptyBlockDataForBlock(ILTree::ExecutableBlock *block)
{
    ThrowIfNull(block);
    EmptyBlockData *data = NULL;
    if ( m_emptyBlockForBlockMap.GetValue(block,&data) )
    {
        return data;
    }

    return NULL;
}

EmptyBlockData *ClosureRoot::GetOrCreateEmptyBlockDataForBlock(ILTree::ExecutableBlock *block, Procedure *proc)
{
    EmptyBlockData *data = GetEmptyBlockDataForBlock(block);
    if ( data )
    {
        return data;
    }

    Location loc = block->Loc;
    loc.m_lBegColumn += 2;  // 
    data = CreateEmptyBlockDataCore(block, loc, proc);
    m_emptyBlockForBlockMap.SetValue(block, data);
    return data;
}

EmptyBlockData *ClosureRoot::GetEmptyBlockDataForGroup(unsigned groupId)
{
    EmptyBlockData *emptyBlock = NULL;
    if ( m_emptyBlockForGroupMap.GetValue(groupId, &emptyBlock) )
    {
        return emptyBlock;
    }

    return NULL;
}

EmptyBlockData *ClosureRoot::GetOrCreateEmptyBlockDataForGroup(ILTree::Statement *stmt, Procedure *proc)
{
    ThrowIfNull(stmt);

    // If we've already created an empty block for this group then go ahead and use it
    EmptyBlockData *data = GetEmptyBlockDataForGroup(stmt->GroupId);
    if ( data )
    {
        return data;
    }

    stmt = stmt->GetFirstStatementInGroup();

    Location loc= stmt->Loc;    // 
    loc.m_lBegColumn += 1;
    data = CreateEmptyBlockDataCore(stmt, loc, proc);
    m_emptyBlockForGroupMap.SetValue(stmt->GroupId, data);
    return data;
}

EmptyBlockData *ClosureRoot::CreateEmptyBlockDataCore(ILTree::Statement *stmt, Location loc, Procedure *proc)
{
    // Create the hidden block
    ILTree::ExecutableBlock *temp = &GetSemantics()->AllocateStatement(SB_HIDDEN_CODE_BLOCK, loc, NoResume, false)->AsExecutableBlock();
    temp->EndConstructLocation = Location::GetHiddenLocation();
    temp->TerminatingResumeIndex = NoResume;
    temp->Child = stmt;
    temp->Parent = stmt->Parent;
    CreateLocalsHash(proc, temp);

    return new (*GetNorlsAllocator()) EmptyBlockData(temp, stmt);
}

//==============================================================================
// Fixup any empty blocks that need to be inserted.  An empty block can be
// inserted in 2 cases
//   1) A Block with special scoping requirements (for, for each, etc ...) lifts
//      a variable (control variable, long term temporary, etc ...)
//   2) A Statement lifts a ShortLived temporary.  To garantee sementacis are
//      me it weird query scenarios we must insert an empty block.  One example
//      is the following
//
//      Sub Foo
//        MyLabel:
//        Dim x = 5
//        Dim q = As New SomeStructure { .Max = x, From c in col Where .Max > 2 }
//
//        if SomeCondition Then
//          Goto MyLabel
//        End If
//      End Sub
//
//      The line with the query will create a ShortLived temporary which will have
//      the same initial value as q.  If we don't create an empty block and the
//      'Goto' occurs the first instance of q will have the second instance of q
//      inside the query closure.  This will cause a host of semantic issues
//
// At first glance it looks like we could do this as soon as we discover that
// we need to lift the control variable.  Unfortunately we can't because at
// the time we discover the control variable the BoundTreeVisitor is currently
// looping accross a statement list containing the loop block.  Moving it around
// while it's being iterated accross is just a bad idea and usually leads to
// an infinite loop.
//==============================================================================
void ClosureRoot::FixupEmptyBlocks()
{
    ChangeState(CRState_EmptyBlockFixup);
    if ( m_emptyBlockForGroupMap.Count() > 0 )
    {
        // unsigned int would be more natural than UINT_PTR, but HashTables require Pointer precision,
        // otherwisethe hashtable won't be able to create a hash on 64-bit
        Iterator<EmptyBlockData*> it = m_emptyBlockForGroupMap.GetValueIterator();
        while ( it.MoveNext() )
        {
            FixupEmptyBlock(it.Current());
        }

        // Have to fix up the TrackedExpression's here as well.  If an Expression we are
        // tracking occured inside a Statement that lifted a ShortLived Temporary then it's
        // possible that the Block for the TrackedExpression has changed.  Two cases
        //
        //  1) Expression occurred inside a Lambda.  In this case the block for the TrackedExpression
        //     is the ProcedureBody for the Lambda.  Refactoring the statement into a Hidden block
        //     doesn't have any effect on the expression.
        //  2) Expression did not occur inside a lambda.  Now the Block for the TrackedExpression
        //     is the Hidden Block
        //
        // You may be thinking we could associate statements with TrackedExpression instances
        // instead of Block.  It's not possible because that would cause us to loose all
        // context for nested lambdas and break scenario #1.
        Iterator<TrackedExpression*> listIt = m_trackedList.GetIterator();
        while ( listIt.MoveNext())
        {
            TrackedExpression *cur = listIt.Current();
            ILTree::Statement *stmt = cur->GetStatement();
            if ( m_emptyBlockForGroupMap.Contains(stmt->GroupId) )
            {
                ILTree::ExecutableBlock *emptyBlock = stmt->Parent;
                while ( emptyBlock->GroupId == stmt->GroupId )
                {
                    emptyBlock = emptyBlock->Parent;
                    ThrowIfNull(emptyBlock);
                }

                ThrowIfFalse(SB_HIDDEN_CODE_BLOCK == emptyBlock->bilop);
                ILTree::ExecutableBlock *originalParent = emptyBlock->Parent;
                if ( cur->GetBlock() == originalParent )
                {
                    cur->SetBlock(emptyBlock);
                }
            }
        }
    }

    Iterator<EmptyBlockData*> it2 = m_emptyBlockForBlockMap.GetValueIterator();
    while ( it2.MoveNext() )
    {
        FixupEmptyBlock(it2.Current());

        // There is no need to fixup TrackedExpression state here like there is for empty
        // blocks for statement groups.
        //
        // For Empty Blocks mapped to blocks, it's impossible for a TrackedExpression to ever
        // be associated with this EmptyBlock.  It exists purely to hold variables.  The actual
        // block the EmptyBlock is acting as a scope for will be higher on the stack and will
        // be the target for TrackedExpression instances.
    }

    ChangeState(CRState_PostEmptyBlockFixup);
}

//==============================================================================
// Fixup all of the MyBase/MyClass calls in the BILTREE to use stubs so that
// we can lift the MyBase/MyClass references
//==============================================================================
void ClosureRoot::FixupMyBaseMyClassCalls(_In_opt_ List<ILTree::Expression*, NorlsAllocWrapper> *fixupThese)
{
    if (fixupThese == NULL) fixupThese = &m_mbcCallList;

    ChangeState(CRState_MyBaseMyClassFixup);

    // Iterate over the MyBase/MyClass calls and generate the appropriate stubs
    Iterator<ILTree::Expression*> it = fixupThese->GetIterator();
    while ( it.MoveNext() )
    {
        ILTree::Expression *cur = it.Current();
        switch ( cur->bilop )
        {
            case SX_CALL:
                {
                    ILTree::CallExpression *call = &cur->AsCallExpression();
                    Procedure *proc = call->Left->AsSymbolReferenceExpression().Symbol->PProc();
                    MyBaseMyClassStubContext context(this, proc);
                    context.FixupCall(call);
                }
                break;
            case SX_DELEGATE_CTOR_CALL:
                {
                    ILTree::DelegateConstructorCallExpression *delCall = &cur->AsDelegateConstructorCallExpression();
                    Procedure *proc = delCall->Method->AsSymbolReferenceExpression().Symbol->PProc();
                    MyBaseMyClassStubContext context(this, proc);
                    context.FixupDelegateConstructor(delCall);
                }
                break;
            default:
                ThrowIfFalse(false);    // These are the only types of MyBase/MyClass we should fixup
                break;
        }
    }

    ChangeState(CRState_PostMyBaseMyClassFixup);
}

void ClosureRoot::FixupEmptyBlock(EmptyBlockData *data)
{
    ThrowIfNull(data);
    if ( !data->IsNeeded() )
    {
        return;
    }

    ILTree::ExecutableBlock *emptyBlock = data->GetEmptyBlock();
    ILTree::Statement *stmt = data->IsForBlock() ? data->GetTargetBlock() : data->GetStatement();

    // Insert the block before the statement and remove
    // the current statement from the parent's list of child statements
    ::InsertStatementListBefore(stmt->Parent, stmt, emptyBlock);
    emptyBlock->Child = stmt;
    emptyBlock->Parent = stmt->Parent;

    if (data->IsForBlock() )
    {
        // Just need to move the block as the child of the empty block
        ::RemoveStatementFromParent(stmt);
        stmt->Next = NULL;
        stmt->Prev = NULL;
        stmt->Parent = emptyBlock;
        emptyBlock->Child = stmt;
    }
    else
    {
        // We need to move all statements in the same group at this level into the empty block
        ILTree::Statement *lastAdded = NULL;
        ILTree::Statement *next;
        do
        {
            next = stmt->Next;
            ::RemoveStatementFromParent(stmt);
            stmt->Prev = lastAdded;
            stmt->Next = NULL;
            stmt->Parent = emptyBlock;
            if ( lastAdded )
            {
                lastAdded->Next = stmt;
            }

            lastAdded = stmt;
            stmt = next;
        } while ( next && next->GroupId == data->GetGroupId() );
    }
}

void ClosureRoot::FixupShortLivedTemporaries()
{
    if ( m_shortLivedMap.Count() == 0 )
    {
        return;
    }

    ChangeState(CRState_ShortLivedFixup);
    ClosureShortLivedTemporaryVisitor it(this);
    it.Visit();
    ChangeState(CRState_PostShortLivedFixup);
}

void ClosureRoot::FixupClosureTreeStructure()
{
    // First part is to assign all of the lambdas to the closures that
    // own them.  It is possible for this to create another closure in the case
    // of nested lambdas so it should go before we build the tree structure.
    //
    // This is done in two passes.  The first pass will move all of the lambdas
    // that cannot be optimized into the appropriate closure.  The second pass
    // will move the remaining lambdas into their optimized closure.
    //
    // We must do it this way because of the implicit dependency between lambdas
    // here.  Imagine the following hierarchy of lambdas
    //
    //   L1 = Function(x) L2(x) OrElse SomeLocal
    //     L2 = Function(x) Return True
    //       L3 = Function(x) Return SomeOtherLocal
    //
    // In the abscence of L1 and L3, L2 could be optimized.  But because it is between
    // two unoptimized lambdas it must also not be optimized.  Using two passes allows
    // us to capture this easily
    ChangeState(CRState_ParentingLambdas);

    Iterator<LambdaData*> lambdaItFirst = m_realLambdaMap.GetValueIterator();
    while ( lambdaItFirst.MoveNext() )
    {
        LambdaData *data = lambdaItFirst.Current();
        ThrowIfTrue(LambdaType_ExpressionTree == data->GetLambdaType());
        if ( LambdaType_Unknown != data->GetLambdaType() )
        {
            continue;
        }

        if ( data->UsesLiftedVariable() || data->HasClosureVariable() )
        {
            data->SetLambdaType(LambdaType_Normal);

            // Because it's not optimized none of it's parents can be either
            Procedure *curProc = data->GetParentProcedure();
            while ( curProc != m_proc )
            {
                // If it's not the proc, then it's a lambda.  Throws if it does
                // not exist.  Don't needless follow the hierarchy multiple times
                LambdaData *curData = m_realLambdaMap.GetValue(curProc);
                if ( LambdaType_Unknown != curData->GetLambdaType() )
                {
                    break;
                }

                curData->SetLambdaType(LambdaType_Normal);
                curProc = curData->GetParentProcedure();
            }
        }

        if ( data->UsesMe() )
        {
            // If a particular lambda uses me, then all of the container lambdas
            // will use me as well since they reference it
            Procedure *curProc = data->GetParentProcedure();
            while ( curProc != m_proc )
            {
                // If it's not the proc, then it's a lambda.  Throws if it does
                // not exist.  Don't needless follow the hierarchy multiple times
                LambdaData *curData = m_realLambdaMap.GetValue(curProc);
                if ( curData->UsesMe() )
                {
                    break;
                }

                curData->SetUsesMe(true);
                curProc = curData->GetParentProcedure();
            }
        }
    }

    // Now actually assign the lambdas to the appropriate closure
    Iterator<LambdaData*> lambdaIt = m_realLambdaMap.GetValueIterator();
    while ( lambdaIt.MoveNext() )
    {
        LambdaData *data = lambdaIt.Current();
        if ( LambdaType_Unknown == data->GetLambdaType() )
        {
            // Create an optimized closure to hold it
            data->SetLambdaType(LambdaType_Optimized);
            OptimizedClosure *optClosure = new (*GetNorlsAllocator()) OptimizedClosure(this);
            optClosure->SetLambda(data);
            m_optClosureList.AddLast(optClosure);
        }
        else
        {
            // It can't be optimized so find the closure nearest to the lambda and add
            // it to that closure.
            ThrowIfFalse(LambdaType_Normal == data->GetLambdaType());
            ILTree::ExecutableBlock *block = data->GetTrackedExpression()->GetBlock();
            Closure *closure = FindNearestClosure(block);
            if (!closure)
            {
                // This can occur when you have a nested lambda which lifts the parameter of
                // it's parent lambda but no other variables.
                // 






                closure = GetOrCreateClosure(m_procBody);
            }
            closure->AddLambda(data);
        }
        ThrowIfTrue(LambdaType_Unknown == data->GetLambdaType());

    }
    ChangeState(CRState_PostParentingLambdas);

    // Second part is take the set of closures and push them into a tree
    // structure based on the underlying block structure.
    //
    // This tree structure must be consistent between compiles so that ENC
    // can compare the trees.  Therefore first sort the list by location
    // in the file so that the tree will be processed in the same order
    // every time
    ChangeState(CRState_ClosureTreeBuilding);
    ConstIterator<KeyValuePair<ILTree::ExecutableBlock*,Closure*> > mapIt = m_map.GetConstIterator();
    ArrayList<Closure*,NorlsAllocWrapper> sortedList(m_alloc);
    while (mapIt.MoveNext())
    {
        sortedList.Add(mapIt.Current().Value());
    }

    ClosureLocationComparer comp;
    sortedList.Sort(comp);
    Iterator<Closure*> it = sortedList.GetIterator();
    while ( it.MoveNext() )
    {
        Closure *current = it.Current();

        // Build up it's parent
        Closure *parent = NULL;
        ILTree::ExecutableBlock *block = current->GetBlock();
        if ( block->Parent )
        {
            parent = FindNearestClosure(block->Parent);
        }
        else if ( block->bilop == SB_PROC )
        {
            parent = FindOwningClosure(block->AsProcedureBlock().pproc);
        }

        // It's okay if we don't find a parent.  That just means
        // it's a root node
        if ( parent )
        {
            parent->AddChildClosure(current);
        }
    }

    // Now find the root closures.
    ArrayListIterator<Closure*, NorlsAllocWrapper> it2(&sortedList);
    while (it2.MoveNext() )
    {
        Closure *current = it2.Current();
        if ( !current->GetParent() )
        {
            m_rootClosureList.AddLast(current);
        }
    }
    ChangeState(CRState_PostClosureTreeBuilding);
}

void ClosureRoot::FixupGotos()
{
    // If there are no 'goto' in the code then don't run this
    if ( !m_procContainsGoto )
    {
        return;
    }

    ChangeState(CRState_GotoFixup);

    // For performance reasons, the goto iterators must cache a lot of data that
    // is only relevant for a single iteration.  Building up this data with lots
    // of lambdas and goto's can cause a lot of heap buildup.  Instead rollback
    // the memory after
    // every usage
    NorlsMark mark;

    // Create a block here to scope the iterator
    m_norls.Mark(&mark);
    {
        // Be explicity that we are giving a constant referenc so it is unable to modify
        // any of our allocations and hence toast the rollback
        const ClosureRoot *constRoot = this;
        ClosureGotoVisitor it(constRoot);

        // Do the containing proc
        it.Visit(m_proc, m_procBody);
    }
    m_norls.Free(&mark);

    // Fixup all of the lambda's
    RealLambdaDataIterator mapIt(&m_realLambdaMap);
    while ( mapIt.MoveNext() )
    {
        m_norls.Mark(&mark);

        // Create an additional scope here to garantee the iterator is destroyed
        // before we rollback the memory
        {
            // Be explicity that we are giving a constant referenc so it is unable to modify
            // any of our allocations and hence toast the rollback
            const ClosureRoot *constRoot = this;
            ClosureGotoVisitor it(constRoot);
            Procedure *proc = mapIt.Current()->GetProcedure();
            it.Visit(proc, proc->GetBoundTree());
        }
        m_norls.Free(&mark);
    }

    ChangeState(CRState_PostGotoFixup);
}

void ClosureRoot::CreateInitCode()
{
    ChangeState(CRState_InitCode);

    // Call for generation on the roots.  They will control the generation of their children
    ListValueIterator<Closure*,NorlsAllocWrapper> it(&m_rootClosureList);
    while(it.MoveNext())
    {
        Closure *cur = it.Current();
        cur->CreateInitializationCode();
    }

    ChangeState(CRState_PostInitCode);
}

//==============================================================================
// Now that all of the lambdas are resting on their final closure.  Have the
// closures clean up the initial references to the lambas to point to
// the actual code
//==============================================================================
void ClosureRoot::FixupLambdaReferences()
{
    HashTableValueIterator<Procedure*,LambdaData*,NorlsAllocWrapper> it(&m_realLambdaMap);
    while (it.MoveNext())
    {
        LambdaData *lambdaData = it.Current();
        ClosureBase *owner = lambdaData->GetClosure();
        Procedure *proc = lambdaData->GetProcedure();

        // Basic sanity checks
        ThrowIfTrue(LambdaType_ExpressionTree==lambdaData->GetLambdaType());
        ThrowIfNull(proc);
        ThrowIfNull(owner);

        // First step is to update the result type of the Lambda expression.  It's
        // possible for the ResultType to be a generic binding.  Normally this
        // wouldn't be an issue but the proc is on the transient symbol list
        // so we need to extend the binding lifetime to the length of the
        // transient symbol list
        if ( proc->GetType() )
        {
            BCSYM *newType = owner->GetEquivalentTypeForMetaData(proc->GetType());
            proc->SetType(newType);
        }

        // Next update the paramaters
        Parameter *param = proc->GetFirstParam();
        while ( param )
        {
            BCSYM *newType = owner->GetEquivalentTypeForMetaData(param->GetType());
            param->SetType(newType);
            param = param->GetNext();
        }

        owner->FixupLambdaReference(lambdaData);
    }
}



//==============================================================================
// FindOwningStatementOfShortLivedTemporary - finds the statement in the stack that owns the
// given temporary variable - see Dev10#677779. If none claim to own it,
// then returns the statement at the bottom of the stack.
// This method is static and the caller passes in their statementStack.
//
// E.g.
//   Dim x As New Fred With {.a = Function()
//                                   Return .a
//                                End Function}
// There is a short-lived temporary for "x". We will often be walking the
// tree looking at temporaries. As we walk the return statement, the question
// is: which statement does the temporary belong to? It's not enough just to
// look in m_statementStack.Peek(), i.e. the bottom "Return tempx.a" of the
// statement stack. This temporary tempx happens to be higher up in the
// statement stack.
//==============================================================================
ILTree::Statement *ClosureRoot::FindOwningStatementOfShortLivedTemporary(Filo<ILTree::Statement*,NorlsAllocWrapper> &statementStack, Variable *var)
{
    ThrowIfFalse(var->IsTemporary());
    ThrowIfNull(var->GetTemporaryInfo());
    ThrowIfFalse(var->GetTemporaryInfo()->Lifetime == LifetimeShortLived);
    ThrowIfFalse(statementStack.Count() > 0);

    ConstIterator<ILTree::Statement*> it = statementStack.GetConstIterator();
    while (it.MoveNext())
    {
        ILTree::Statement *candidateStatement = it.Current();
        ThrowIfNull(candidateStatement);

        // Find the SB_HIDDEN_BLOCK and the ProcedureBlock that contains this statement
        ILTree::Statement *containingBlock = candidateStatement;
        while (containingBlock->Parent!=NULL)
        {
            containingBlock=containingBlock->Parent;
        }
        ThrowIfFalse(containingBlock->bilop == SB_PROC);
        ILTree::ProcedureBlock *procedureBlock = &(containingBlock->AsProcedureBlock());

        // If the temporary manager of the tempvar matches that of the procedure-block,
        // then we have a winner
        TemporaryManager *procedureTempManager = procedureBlock->ptempmgr;
        if (var->GetTemporaryManager() == procedureBlock->ptempmgr) 
        {
            return candidateStatement;
        }
    }

    // In the case of expression lambdas, their variable temp managers already have been
    // corrected to the Lambda$1 procedure block, so the above search didn't yield anything.
    // However, because they're expression-lambdas, the statement at the bottom of the
    // stack is necessarily the correct one:
    VSASSERT(var->GetTemporaryManager()->GetContainingProcedure()->IsSyntheticMethod() &&
             var->GetTemporaryManager()->GetContainingProcedure()->PSyntheticMethod()->IsLambda(),
             "Expected the temporary variable to come from a lambda");
    return statementStack.Peek();
}


//==============================================================================
// Find the closure nearest to the passed in block.  This will look up the bound
// tree and return the first block with an associated closure
//==============================================================================
Closure *ClosureRoot::FindNearestClosure(ILTree::ExecutableBlock *block)
{
    // Finding the nearest closure before the Biltree is well formed will
    // lead to invalid answers
    EnsurePastOrInState(CRState_BiltreeWellFormed);

    // First look in the map.  We call this method a lot for symbol fixup so we
    // cache the result to make later calls faster
    Closure *found = NULL;
    if ( m_nearestMap.GetValue(block, &found) )
    {
        return found;
    }

    // Do the search the hard way
    ILTree::ExecutableBlock *current = block;
    while (current)
    {
        Closure *closure = this->GetClosure(current);
        if ( closure )
        {
            found = closure;
            break;
        }

        if ( current->Parent )
        {
            current = current->Parent;
        }
        else if ( current->bilop == SB_PROC && m_procBody == current )
        {
            // At the very top.  Since ::GetClosure() didn't return a closure, there is no
            // root closure
            found = NULL;
            break;
        }
        else if ( current->bilop == SB_PROC )
        {
            // Variable 'current' is a proc that is not the containing method so it is a
            // lambda.  The nearest closure to a lambda is the Closure owning the Lambda.
            // So it's the Closure associated with the Block where the LambdaExpression
            // occurred.
            //
            // ::GetNearestClosure() is called before lambdas are parented so
            // do not use ::FindOwniingClosure().
            //
            // Instead just look for the block the Lambda expression occurs in.
            LambdaData *data = m_realLambdaMap.GetValue(current->AsProcedureBlock().pproc);
            TrackedExpression *tracked = data->GetTrackedExpression();
            current = tracked->GetBlock();
        }
        else
        {
            current = NULL;
        }
    }

    // Cache the result
    m_nearestMap.SetValue(block, found);
    return found;
}

Closure *ClosureRoot::FindOwningClosure(Procedure *proc) const
{
    ClosureBase *cb = FindOwningClosureBase(proc);
    if ( cb && NormalClosureContext == cb->GetGenericContextKind())
    {
        return cb->AsClosure();
    }

    return NULL;
}

ClosureBase *ClosureRoot::FindOwningClosureBase(Procedure *proc) const
{
    EnsurePastOrInState(CRState_PostParentingLambdas);  // This call will fail if the
                                                        // lambdas have not been reparented yet
    ThrowIfNull(proc);

    LambdaData *lambdaData = NULL;
    if ( m_realLambdaMap.GetValue(proc, &lambdaData) )
    {
        return lambdaData->GetClosure();
    }

    return NULL;
}

Closure *ClosureRoot::FindOwningClosure(Variable *var) const
{
    AssertIfNull(var);

    Closure *found = NULL;
    if ( !m_varMap.GetValue(var, &found))
    {
        return NULL;
    }

    return found;
}

void ClosureRoot::CreateClosureClassCode()
{
    // Firstly fixup the generics on static lambdas.
    ListValueIterator<OptimizedClosure*,NorlsAllocWrapper> optIt(&m_optClosureList);
    while ( optIt.MoveNext() )
    {
        optIt.Current()->FixupGenericTypes();
    }

    // Call for generation on the roots.  They will control the generation of their children
    ListValueIterator<Closure*,NorlsAllocWrapper> it(&m_rootClosureList);
    while (it.MoveNext())
    {
        Closure *cur = it.Current();
        ThrowIfTrue(cur->GetParent());
        cur->CreateClosureClassCode();
    }
}

//==============================================================================
// Determine if it's safe to lift this type
//==============================================================================
void ClosureRoot::CheckVariableSafeForLifting(LambdaData *lambdaData, ILTree::SymbolReferenceExpression *symRef)
{
    ThrowIfNull(lambdaData);

    Variable *var = GetVariableFromExpression(symRef);
    ThrowIfNull(var);
    Location loc = symRef->Loc;

    if ( var->IsMe() )
    {
        // It's not legal to lift Me in a closure when Me is a struct
        if ( var->GetType()->IsStruct() )
        {
            m_semantics->ReportSemanticError(
                lambdaData->GetLambda()->IsPartOfQuery
                    ? ERRID_CannotLiftStructureMeQuery
                    : ERRID_CannotLiftStructureMeLambda,
                symRef->Loc,
                symRef->Symbol);
        }
    }

    // Make sure it's not a pointer type.  Lifting a ByRef parameter
    // is not allowed.
    if ( TypeHelpers::IsPointerType(var->GetType()) )
    {
        m_semantics->ReportSemanticError(
            lambdaData->GetLambda()->IsPartOfQuery
                ? ERRID_CannotLiftByRefParamQuery1
                : ERRID_CannotLiftByRefParamLambda1,
            &loc,
            var);
    }

    // Make sure it's not a restricted type
    if (IsRestrictedType(var->GetType(), GetSemantics()->m_CompilerHost) ||
        IsRestrictedArrayType(var->GetType(), GetSemantics()->m_CompilerHost))
    {
        ErrorTable *errorTable = GetSemantics()->m_Errors;
        StringBuffer buff;

        errorTable->CreateError(
            lambdaData->GetLambda()->IsPartOfQuery
                ? ERRID_CannotLiftRestrictedTypeQuery
                : ERRID_CannotLiftRestrictedTypeLambda,
            &loc,
            errorTable->ExtractErrorName(var->GetType(), NULL, buff));
    }
}

//==============================================================================
// Iterate over all of the tracked variable references and have the responsible
// closure perform the update.  The responsible closure in this case in the
// nearest closure to the block where the reference occurred.
//==============================================================================
void ClosureRoot::FixupSymbolReferences()
{
    ListValueIterator<TrackedExpression*,NorlsAllocWrapper> it(&m_trackedList);
    while (it.MoveNext())
    {
        // Only fixing up symbol references here
        TrackedExpression* trackedRef = it.Current();
        if ( SX_SYM != trackedRef->GetExpression()->bilop)
        {
            continue;
        }

        ILTree::SymbolReferenceExpression *symRef = &(trackedRef->GetExpression()->AsSymbolReferenceExpression());
        Symbol *sym = symRef->Symbol;
        Closure *closure = FindNearestClosure(trackedRef->GetBlock());
        Procedure *refProc = trackedRef->GetReferencingProcedure();

        // Dev10#530887: consider the case "Dim x as <ExpressionTree> = Function() x"
        // Here we will fix up the symbol "x", and FindNearestClosure points to the containing procedure.
        // In the multiline case "Dim x as <ExpressionTree> = Function() : return x : End Function"
        // we should likewise fix up the symbol "x", and FindNearestClosure should again point to the
        // containing procedure. So, even for multiline lambdas in expression trees, trackedRef->GetBlock()
        // has to have a valid parent. (see other comment for this bug in ClosureRoot::StartExpression)
        
        if (!closure)
        {
            AssertIfTrue(sym->IsVariable() && ! sym->PVariable()->IsMe() ? FindOwningClosure(sym->PVariable())  : NULL);
        }
        else if ( sym->IsVariable() )
        {
            Variable *var = sym->PVariable();

            // We only need to fixup the symbol reference in 2 cases
            // 1) It's a symbol reference to a lifted variable.  In this case we need
            //    to fixup the reference in all cases (even outside lambda's) since
            //    the local will eventually be completely removed
            // 2) It's a Me reference and it's in a non-optimized lambda.  Since Me is not a local
            //    and cannot actually be removed :) we don't need to fixup the reference
            //    outside of a lambda.
            Closure *owner = FindOwningClosure(var);
            Assume(trackedRef->GetReferencingProcedure() != NULL, L"There should always be a referencing procedure");
            if ( owner )
            {
                closure->FixupVariableReference(symRef, refProc);
            }
            else if ( var->IsMe() )
            {
                LambdaData *lambdaData;
                if ( m_realLambdaMap.GetValue(refProc, &lambdaData)
                    && OptimizedClosureContext != lambdaData->GetClosure()->GetGenericContextKind() )
                {
                    closure->FixupVariableReference(symRef, refProc);
                }
            }
        }

        // We only need this list once so delete it as we are iterating
        it.Remove();
    }
}

//==============================================================================
// Returns the equivalent type for the passed in symbol when used within the
// passed in procedure
//==============================================================================
BCSYM *ClosureRoot::GetEquivalentType(BCSYM *type, Procedure *proc)
{
    ThrowIfNull(type);
    ThrowIfNull(proc);

    if ( m_proc == proc )
    {
        // Within the root procedure, the types do not change so just return the
        // original type
        return type;
    }

    // If the procedure is not the root proc then it is a lambda.  Find the
    // owning closure and ask it for the equivalent type
    ClosureBase *cls = FindOwningClosureBase(proc);
    ThrowIfNull(cls);
    return cls->GetEquivalentType(type);
}

//==============================================================================
// In some cases (SB_IF,SB_TRY for example) if no locals are declared a locals
// hash will not be created.  At this point we have a local so make sure
// locals hash is available
//==============================================================================
void ClosureRoot::CreateLocalsHash(Procedure *proc, ILTree::ExecutableBlock *block)
{
    ThrowIfNull(proc);
    ThrowIfNull(block);
    ThrowIfTrue(block->Locals);

    BCSYM_NamedRoot *parent = proc;     // Worst case the owner is the procedure
    ILTree::ExecutableBlock * curBlock = block->Parent;
    while ( curBlock && !curBlock->Locals )
    {
        curBlock = curBlock->Parent;
    }

    if ( curBlock )
    {
        parent = curBlock->Locals;
    }

    block->Locals = GetNormalSymbolFactory()->GetHashTable(NULL, parent, true, 1, NULL);
}

//==============================================================================
// Create a reference to the original class 'Me' in the original procedure.  If
// this creates a new symbol that could be pushed into a lambda you must track
// the resulting expression or it will not be properly refactored
//
// The only case where this is valid in MyBaseMyClass Fixup.  Any other case
// where this is used must be heavily scrutinized before being allowed.
//
// The risk you run is allocating a reference to 'Me' in an expression which ends
// up in a normal real lambda in which case 'Me' now refers to the closure instead
// of the original containing class.  The resulting code will not verify
//==============================================================================
ILTree::SymbolReferenceExpression *ClosureRoot::CreateReferenceToContainingClassMe()
{
    EnsureInState(CRState_MyBaseMyClassFixup);
    return CreateReferenceToContainingClassMeImpl();
}

//==============================================================================
// Create a reference to the original containing class 'Me' within the specified
// procedure
//
// Caller Beware.  You cannot call this if from a static optimized lambda
//==============================================================================
ILTree::SymbolReferenceExpression *ClosureRoot::CreateReferenceToContainingClassMeInProcedure(Procedure *inProc)
{
    ThrowIfNull(inProc);

    // Validate the state we are in
    EnsurePastOrInState(CRState_PostParentingLambdas);
    EnsureBeforeState(CRState_InitCode);    // This potentially causes Closure instances
                                            // to lift 'Me' so this cannot be done
                                            // after the initialization code is
                                            // generated


    // Create the reference
    ILTree::SymbolReferenceExpression *symRef = CreateReferenceToContainingClassMeImpl();

    // If this is just the original proc then there is no more work to be done
    if ( inProc == m_proc )
    {
        return symRef;
    }

    // Now we need to fixup the 'Me' reference within the context of
    // the lambda it's to be created in
    ClosureBase *owner = FindOwningClosureBase(inProc);
    ThrowIfNull(owner);
    if ( NormalClosureContext == owner->GetGenericContextKind() )
    {
        // For normal Closure we must also ensure that they lift 'Me'
        // since we are asking for a reference in that Context.  Calling
        // FixupVariableReference will do for both
        Closure *closure = owner->AsClosure();
        closure->FixupVariableReference(symRef, inProc);
    }
    else
    {
        // Optimized lambdas are just normal methods on the class so it doesn't require
        // any extra fixup.  Ensure it uses Me though and as a consequence is not a static
        // lambda
        ThrowIfFalse(OptimizedClosureContext == owner->GetGenericContextKind());
        OptimizedClosure *closure = owner->AsOptimizedClosure();
        LambdaData *ld = closure->GetLambdaData();
        ThrowIfFalse(ld->UsesMe());
    }

    return symRef;
}

//==============================================================================
// Create a reference to the original containing class 'Me'
//
// Never call this directly.  Always call the wrapper methods
//==============================================================================
ILTree::SymbolReferenceExpression *ClosureRoot::CreateReferenceToContainingClassMeImpl()
{
    // First step is to build the typeBinding for the ContainingClass
    ClassOrRecordType *containingClass = m_proc->GetContainingClass();
    GenericTypeBinding *typeBinding = NULL;
    if ( IsGenericOrHasGenericParent(containingClass) )
    {
        typeBinding = SynthesizeOpenGenericBinding(
                containingClass,
                *GetNormalSymbolFactory())->PGenericTypeBinding();
    }

    // Now create the symbol reference
    Location loc = Location::GetHiddenLocation();
    ILTree::SymbolReferenceExpression *symRef = GetSemantics()->AllocateSymbolReference(
        containingClass->GetMe(),
        containingClass,
        NULL,
        loc,
        typeBinding);

    return symRef;
}

//==============================================================================
//  Start GenericContext
//==============================================================================

GenericContext::GenericContext(ClosureRoot *root) :
    m_root(root),
    m_firstGenericParam(NULL),
    m_genericMap(root->GetNorlsAllocator()),
    m_bindingCacheMap(root->GetNorlsAllocator()),
    m_bindingMetaDataCacheMap(root->GetNorlsAllocator())
{
}

//==============================================================================
// Returns the equivalent type within the closure for the type passed in.  There
// are 2 scenarios where we would need to get an equivalent type.
//
// The first is for GenericTypes.  When we move lambdas into Closures the generic
// binding is altered since the generic parameters are altered.  We need to
// get the equivalent binding for the closure's lambda.
//
// The second has to do with lifetime.  Several types are allocated with the
// default Tree Allocator instead of the symbol table (Array's for instance).
// When these types are used as the type for a Closure field, we need to clone
// them with the TransientSymbol allocator so their lifetime will be long
// enough for IDE features such as ENC
//==============================================================================
BCSYM *GenericContext::GetEquivalentTypeImpl(BCSYM *type, bool isForMetaData)
{
    ThrowIfNull(type);

    // When updating this "switch" statement make sure to update ClosureDump::PrintTypeName
    // as well
    if ( type->IsGenericParam() )
    {
        return GetEquivalentGenericParameter(type->PGenericParam());
    }
    else if ( type->IsGenericTypeBinding() )
    {
        return GetEquivalentTypeBindingImpl(type->PGenericTypeBinding(), isForMetaData);
    }
    else if ( type->IsPointerType() )
    {
        BCSYM *newRoot = GetEquivalentTypeImpl(type->PPointerType()->GetRoot(), isForMetaData);
        Symbols *symbols = GetSymbolFactory(isForMetaData);
        return symbols->MakePtrType(newRoot);
    }
    else if ( type->IsNamedType() )
    {
        return GetEquivalentTypeImpl(type->PNamedType()->GetSymbol(), isForMetaData);
    }
    else if ( type->IsArrayType() )
    {
        BCSYM_ArrayType *arrayType = type->PArrayType();
        BCSYM *newRoot = GetEquivalentTypeImpl(arrayType->GetRoot(), isForMetaData);
        Symbols *symbols = GetSymbolFactory(isForMetaData);

        // Need to allocate a new type here
        type = symbols->GetArrayType( arrayType->GetRank(), newRoot);
        return type;
    }
    else
    {
        return type;
    }
}

//==============================================================================
// Create a binding for the generic symbol inside the closure
//==============================================================================
GenericBinding *GenericContext::CreateBindingImpl(Declaration *decl, bool isForMetaData)
{
    ThrowIfNull(decl);
    ThrowIfFalse(IsGenericOrHasGenericParent(decl));

    // Create a simple binding then get the equivalent binding
    GenericBinding *binding = CreateSimpleBindingImpl(decl, isForMetaData);
    return GetEquivalentBindingImpl(binding, isForMetaData);
}

//==============================================================================
// Create a generic type binding for the passed in symbol
//==============================================================================
GenericTypeBinding *GenericContext::CreateTypeBindingImpl(Declaration *decl, bool isForMetaData)
{
    GenericBinding *binding = CreateBindingImpl(decl, isForMetaData);
    ThrowIfFalse(binding->IsGenericTypeBinding());
    return binding->PGenericTypeBinding();
}

//==============================================================================
// Build the equivalent type binding in the generated closure class.
//
// public:
//==============================================================================
GenericTypeBinding* GenericContext::GetEquivalentTypeBindingImpl(GenericTypeBinding *source, bool isForMetaData)
{
    ThrowIfNull(source);

    GenericBinding *binding = GetEquivalentBindingImpl(source, isForMetaData);
    ThrowIfFalse(binding->IsGenericTypeBinding());
    return binding->PGenericTypeBinding();
}

GenericBinding *GenericContext::GetEquivalentBindingImpl(GenericBinding *source, bool isForMetaData )
{
    ThrowIfNull(source);

    // First look in the cache and see if we've already rebuilt this binding.  If
    // so don't rebuild it again.  GenericBinding instances are cached in the
    // original BILTREE so a pointer comparison is sufficient
    //
    // We need to cache here to avoid allocating the same GenericBinding over and
    // over again.  Otherwise with large numbers of queries we can quickly get
    // into memory leak situtaions.
    //
    // See DevDiv #36554 for a sample of 150 queries that will quickly lead vbc
    // to run out of memory without this caching.
    DynamicHashTable<GenericBinding*,GenericBinding*,NorlsAllocWrapper> *cacheMap = isForMetaData ?
        &m_bindingMetaDataCacheMap : &m_bindingCacheMap;
    GenericBinding *cached = NULL;
    if ( cacheMap->GetValue(source, &cached) )
    {
        return cached;
    }

    Symbols *symbols = GetSymbolFactory(isForMetaData);
    NorlsAllocator *norlsAlloc = symbols->GetNorlsAllocator();
    unsigned argCount = source->GetArgumentCount();
    BCSYM **oldArray = source->GetArguments();
    BCSYM **newArray;

    if ( 0 == argCount )
    {
        // If there are no arguments then there isn't much to update
        newArray = NULL;
    }
    else
    {
        newArray = reinterpret_cast<BCSYM**>(norlsAlloc->Alloc(sizeof(BCSYM*)*argCount));

        for ( unsigned i = 0; i < argCount; ++i)
        {
            BCSYM *curOld = oldArray[i];
            if ( curOld->IsNamedType() )
            {
                curOld = curOld->DigThroughNamedType();
            }

            newArray[i] = GetEquivalentTypeImpl(curOld, isForMetaData);
        }
    }

    GenericTypeBinding *parent = source->GetParentBinding();
    GenericBinding *binding = symbols->GetGenericBinding(
            false,
            source->GetGeneric(),
            newArray,
            argCount,
            parent ? GetEquivalentTypeBindingImpl(parent, isForMetaData) : NULL);

    // Update the cached map
    cacheMap->SetValue(source, binding);
    return binding;
}

//==============================================================================
// Return the equivalent generic parameter for the one passed in within the
// scope of this generic closure class
//==============================================================================
Type *GenericContext::GetEquivalentGenericParameter(GenericParameter *param)
{
    AssertIfNull(param);
    GenericContext *parent = GetParentGenericContext();
    if ( parent )
    {
        // Only our root parent actually does any parameter mapping.  Ask them
        // for the equivalent parameter.
        return parent->GetEquivalentGenericParameter(param);
    }

    Type *equiv = NULL;
    if ( !m_genericMap.GetValue(param,&equiv) )
    {
        // If the generic parameter was not mapped, then just return the original
        equiv = param;
    }
    return equiv;
}

//==============================================================================
// Copies a set of GenericParametrs into the closure and applies them to
// the passed in symbol.  Used when
//  1) Need to set Generic Paramaters on the closure class/proc
//  2) GetEquivalentBindings
//==============================================================================
void GenericContext::CopyGenericParametersImpl(Declaration *source, bool isForMethod)
{
    ThrowIfTrue(m_firstGenericParam);   // GenericParams can only be copied from a single
                                        // source (the method).  Otherwise the names will
                                        // conflict
    ThrowIfNull(source);
    ThrowIfFalse(source->IsGeneric());
    ThrowIfTrue(GetParentGenericContext()); // Only the parent should do mapping

    // When we are copying generic parameters they should be stored on the transient
    // symbol store.  They are used in both MetaData and inside the BILTREE so to
    // work it needs to have the greater lifetime (transient).
    Symbols *symbols = GetTransientSymbolFactory();
    GenericParameter *builtFirst = NULL;
    GenericParameter *builtLast = NULL;
    Location loc = Location::GetHiddenLocation();

    // First steup is to create the generic parameters.  Then once they've all
    // been created we will copy the constraints.  We need to create them first
    // since it's possible for constraints to refer to generic parameters
    GenericParameter *cur = source->GetFirstGenericParam();
    unsigned nextIndex = m_genericMap.Count();
    while (cur)
    {
        // Build up our generic param.  Assert than we are not being asked to map a
        // GenericParameter twice
        ThrowIfTrue(m_genericMap.Contains(cur));
        GenericParameter *built;
        STRING *builtName = GetNameForCopiedGenericParameter(cur);
        VSASSERT(cur->GetVariance() == Variance_None || cur->IsBadVariance(), "expected to copy from an invariant class");

        built = symbols->GetGenericParam(
            &loc,
            builtName,
            nextIndex,
            isForMethod,
            cur->GetVariance());

        // Update the mapping
        m_genericMap.SetValue(cur, built);

        // Add it to the list
        if ( !builtFirst )
        {
            builtFirst = built;
            builtLast = built;
        }
        else
        {
            builtLast->SetNextParam(built);
            builtLast = built;
        }

        ++nextIndex;
        cur = cur->GetNextParam();
    }

    // Now copy the constraints
    cur = source->GetFirstGenericParam();
    while (cur)
    {
        GenericParameter *built = m_genericMap.GetValue(cur)->PGenericParam();

        // Build up the constraints
        GenericConstraint *constraint = cur->GetConstraints();
        if ( constraint )
        {
            built->SetConstraints(CopyGenericConstraints(constraint));
        }

        cur = cur->GetNextParam();
    }

    if ( NULL == m_firstGenericParam )
    {
        m_firstGenericParam = builtFirst;
    }
}

GenericConstraint *GenericContext::CopyGenericConstraints(GenericConstraint *first)
{
    ThrowIfNull(first);
    GenericConstraint *builtFirst = NULL;
    GenericConstraint *builtLast = NULL;
    Symbols *symbols = GetTransientSymbolFactory();
    bool skipDuplicateGenericConstraints = SkipDuplicateGenericConstraints();
    HashSet<BCSYM *> rawTypeSet;

    while ( first )
    {
        GenericConstraint *built;
        if ( first->IsGenericTypeConstraint() )
        {
            GenericTypeConstraint *gtc = first->PGenericTypeConstraint();
            BCSYM *type = GetEquivalentTypeForMetaData(gtc->GetType());
            built = symbols->GetGenericTypeConstraint(gtc->GetLocation(), type);
        }
        else if ( first->IsGenericNonTypeConstraint() )
        {
            GenericNonTypeConstraint *gntc = first->PGenericNonTypeConstraint();
            built = symbols->GetGenericNonTypeConstraint(gntc->GetLocation(), gntc->GetConstraintKind());
        }
        else
        {
            ThrowIfFalse(false);
            built = first;
        }

        // Skip GenericTypeConstraint if raw type already seen
        if ( skipDuplicateGenericConstraints && built->IsGenericTypeConstraint() )
        {
            BCSYM* rawType = built->PGenericTypeConstraint()->GetRawType();
            if ( rawTypeSet.Contains(rawType) )
            {
                first = first->Next();
                continue;
            }
            rawTypeSet.Add(rawType);
        }

        if ( builtFirst )
        {
            AssertIfNull(builtLast);
            builtLast->SetNext(built);
            builtLast = built;
        }
        else
        {
            builtFirst = built;
            builtLast = builtFirst;
        }
        first = first->Next();
    }

    return builtFirst;
}

//==============================================================================
// Create a simple binding of a type to it's generic parameters
//
// private:
//==============================================================================
GenericBinding *GenericContext::CreateSimpleBindingImpl(Declaration *pType, bool isForMetaData)
{
    ThrowIfNull(pType);
    ThrowIfFalse(IsGenericOrHasGenericParent(pType));

    Symbols *symbols = GetSymbolFactory(isForMetaData);
    NorlsAllocator *norlsAlloc = symbols->GetNorlsAllocator();
    unsigned paramCount = pType->GetGenericParamCount();
    BCSYM **argArray = NULL;
    if ( paramCount )
    {
        argArray = reinterpret_cast<BCSYM**>(norlsAlloc->Alloc(sizeof(BCSYM*) * paramCount));
        unsigned index = 0;
        AppendGenericParameters(pType, argArray, index);
    }

    // Need to build the binding recursively
    GenericTypeBinding *parentBinding = NULL;
    if ( pType->HasGenericParent())
    {
        Declaration *current = pType->GetImmediateParent();
        while ( !current->IsType() )
        {
            current = current->GetImmediateParent();
        }

        ThrowIfNull(current);
        parentBinding = CreateSimpleTypeBindingImpl(current, isForMetaData);
    }

    GenericBinding *binding = symbols->GetGenericBinding(
            false,
            pType,
            argArray,
            paramCount,
            parentBinding);
    return binding;
}

GenericTypeBinding *GenericContext::CreateSimpleTypeBindingImpl(Declaration *pType, bool isForMetaData)
{
    GenericBinding *binding = CreateSimpleBindingImpl(pType, isForMetaData);
    ThrowIfFalse(binding->IsGenericTypeBinding());
    return binding->PGenericTypeBinding();
}

//==============================================================================
// Return the appropriate symbol factory.  For any type of MetaData we need to
// use the transient symbol store.  For all other types of nodes (BILTREE) we
// should use the normal symbol factory since it has a shorter lifetime.
//==============================================================================
Symbols *GenericContext::GetSymbolFactory(bool isForMetaData)
{
    return isForMetaData ?
        GetTransientSymbolFactory() :
        GetNormalSymbolFactory();
}

//==============================================================================
// Used to add all of the generic information from the class hierarchy into
// this map.  This is very useful if you need to map code or types which exist
// in a parent class into a sub class.
//
// This will essentially follow the generic hierarchy.  So if we had the following
// structure
//
// Class A(Of T)
// Class B(Of U) Inherits A(Of U)
// Class C Inherits (B Of Integer)
//
// If class C was passed into this method, the following entries would be
// added to the map
//
//  U -> Integer
//  T -> Integer
//
// To propertly translate a type that exists in either A or B, we now just need
// to call GetEquivalentType()
//==============================================================================
void GenericContext::AddClassHierarchyToMap(ClassOrRecordType *startClass)
{
    BCSYM *cur = startClass->GetBaseClass();
    while ( cur )
    {
        ThrowIfNull(cur);
        if ( cur->IsGenericTypeBinding() )
        {
            GenericTypeBinding *binding = cur->PGenericTypeBinding();
            Declaration *generic = binding->GetGeneric();
            GenericParameter *curParam = generic->GetFirstGenericParam();
            while ( curParam )
            {
                ThrowIfTrue(m_genericMap.Contains(curParam));   // Shouldn't ever remap a generic param
                                                                // more than once
                Type *target = NULL;
                Type *arg = binding->GetCorrespondingArgument(curParam);
                if ( !arg->IsGenericParam()
                        || !m_genericMap.GetValue(arg->PGenericParam(), &target) )
                {
                    target = arg;
                }
                m_genericMap.SetValue(curParam, target);
                curParam = curParam->GetNextParam();
            }

            // We've processed the generic parent at this point so the next
            // stop is it's parent
            cur = generic->PClass()->GetBaseClass();
        }
        else if ( cur->IsGeneric()
                && cur->PClass()->GetBaseClass()
                && cur->PClass()->GetBaseClass()->IsGeneric())
        {
            // Case like so
            // Class Parent(Of T)
            // ..
            // Class Child(Of T)
            //  Inherits Parent(Of T)
            //
            // If there is any difference here then it would be a generic binding inheritance
            BCSYM *baseClass = cur->PClass()->GetBaseClass();
            GenericParameter *curChildParam = cur->GetFirstGenericParam();
            GenericParameter *curParentParam = baseClass->GetFirstGenericParam();

            while ( curChildParam && curParentParam )
            {
                BCSYM *target = NULL;
                if ( !m_genericMap.GetValue(curChildParam, &target) )
                {
                    target = curChildParam;
                }

                m_genericMap.SetValue(curParentParam, target);
                curChildParam = curChildParam->GetNextParam();
                curParentParam = curParentParam->GetNextParam();
            }
            cur = baseClass;
        }
        else
        {
            ThrowIfFalse(cur->IsClass());
            cur = cur->PClass()->GetBaseClass();
        }
    }
}

Closure *GenericContext::AsClosure()
{
    ThrowIfFalse(NormalClosureContext == GetGenericContextKind());
    return reinterpret_cast<Closure*>(this);
}

OptimizedClosure *GenericContext::AsOptimizedClosure()
{
    ThrowIfFalse(OptimizedClosureContext == GetGenericContextKind());
    return reinterpret_cast<OptimizedClosure*>(this);
}

MyBaseMyClassStubContext* GenericContext::AsMyBaseMyClassStubContext()
{
    ThrowIfFalse(MyBaseMyClassStubGenericContext == GetGenericContextKind());
    return reinterpret_cast<MyBaseMyClassStubContext*>(this);
}

//==============================================================================
//  End GenericContext
//==============================================================================

//==============================================================================
// Start ClosureBase
//==============================================================================

ClosureBase::ClosureBase(ClosureRoot *root) :
    GenericContext(root)
{

}

STRING *ClosureBase::GetNameForCopiedGenericParameter(GenericParameter *param)
{
    ThrowIfNull(param);
    return ClosureNameMangler::EncodeGenericParameterName(GetCompiler(), param);
}

//==============================================================================
// End ClosureBase
//==============================================================================

Closure::Closure(Procedure *containingProc, ILTree::ExecutableBlock *block, ClosureRoot *root)
: ClosureBase(root)
, m_containingProc(containingProc)
, m_block(block)
, m_parent(NULL)
, m_closureClass(NULL)
, m_closureVariable(NULL)
, m_internalBinding(NULL)
, m_externalBinding(NULL)
, m_liftSet(root->m_alloc)
, m_childList(root->m_alloc)
, m_procList(root->m_alloc)
, m_liftedToFieldMap(root->m_alloc)
{
    ThrowIfNull(containingProc);
    ThrowIfNull(block);
    ThrowIfNull(root);
}


Closure::~Closure()
{
    // Nothing to delete.
}

void Closure::EnsureVariableLifted(Variable *var)
{
    ThrowIfNull(var);
    ThrowIfTrue(var->IsMe());       // Me is handled as needed during fixup
    ThrowIfTrue(m_closureVariable); // Should not be called after class creataion

    if ( !m_liftSet.Contains(var) )
    {
        m_liftSet.Add(var);
        m_root->m_varMap.SetValue(var, this);
    }
}

//==============================================================================
// Ensures that a variable is lifted into the closure and returns a reference
// to it.  Doesn't do any smarts with Me().
//==============================================================================
Variable *Closure::EnsureLiftedAndCreated(Variable *var)
{
    Assume(var, L"Variable should not be null");
    Assume(m_closureVariable, L"This should not be called before class created");

    Variable *toRet;
    if ( !m_liftedToFieldMap.GetValue(var, &toRet))
    {
        AssertIfTrue(m_liftSet.Contains(var));
        m_liftSet.Add(var);
        toRet = this->CreateField(var);
    }

    AssertIfNull(toRet);
    return toRet;
}


//==============================================================================
// Adds a lambda to this closure.
//==============================================================================
void Closure::AddLambda(LambdaData *data)
{
    ThrowIfTrue(LambdaType_Normal != data->GetLambdaType());
    ThrowIfNull(data);
    m_procList.AddLast(data->GetProcedure());
    data->SetClosure(this);
}

void Closure::FixupLambdaReference(LambdaData *lambdaData)
{
    ThrowIfNull(lambdaData);

    Procedure *lambdaProc = lambdaData->GetProcedure();
    TrackedExpression *tracked = lambdaData->GetTrackedExpression();

    ThrowIfNull(tracked);
    ThrowIfFalse(this->OwnsProcedure(lambdaProc));
    ThrowIfNull(m_closureVariable);

    // Create a reference to the procedure.  Typically this will be through
    // the closure variable on the stack.  However it's possible to access
    // one of my lambda's from another one of my Lambdas.  In this case
    // the base reference is Me rather than the closure variable.  Find the
    // associated proc and see if I own it.
    ILTree::Expression **ppExpr = tracked->GetRawExpression();
    ILTree::Expression *expr = *ppExpr;
    ILTree::SymbolReferenceExpression *baseRef;
    bool ownsRefProc = this->OwnsProcedure(tracked->GetReferencingProcedure());
    if ( ownsRefProc )
    {
        baseRef = CreateReferenceToThisClosure(true, expr->Loc);
    }
    else
    {
        baseRef = GetSemantics()->AllocateSymbolReference(
            m_closureVariable,
            m_closureVariable->GetType(),
            NULL,
            expr->Loc);
    }

    // Need to create a GenericBinding for our lambda based off where the lambda
    // is called from if the lambda is generic.  If the lambdaProc is not
    // generic, we still need to use the m_externalBinding for our parent
    // binding information
    Procedure *refProc = tracked->GetReferencingProcedure();
    Closure *refClosure = m_root->FindOwningClosure(refProc);
    GenericBinding *lambdaBinding;
    if ( IsGenericOrHasGenericParent(lambdaProc) )
    {
        lambdaBinding = refClosure ? refClosure->CreateBinding(lambdaProc) : m_externalBinding;
    }
    else
    {
        lambdaBinding = NULL;
    }

    ILTree::SymbolReferenceExpression *lambdaRef = GetSemantics()->AllocateSymbolReference(
        lambdaProc,
        TypeHelpers::GetVoidType(),
        baseRef,
        expr->Loc,
        lambdaBinding);

    // Create an address of operation
    ILTree::Expression *addr= GetSemantics()->AllocateExpression(
        SX_ADDRESSOF,
        TypeHelpers::GetVoidType(),
        lambdaRef,
        expr->Loc);

    // Convert the delegate call.  Make sure to update the type.  The type
    // can be a generic delegate bound to some of our paramaters.  The
    // binding depends on our location
    BCSYM *resultType = m_root->GetEquivalentType(expr->ResultType, tracked->GetReferencingProcedure());

    // Make sure to update the procedure state of semantics before doing the conversion
    ILTree::ProcedureBlock *refProcBody = m_root->GetProcedureBody(refProc);
    ProcedureState savedState;
    ProcedureState state(refProcBody);
    GetSemantics()->GetProcedureState(&savedState);
    GetSemantics()->SetProcedureState(&state);
    *ppExpr = GetSemantics()->ConvertWithErrorChecking(
        addr,
        resultType,
        ExprForceRValue);
    GetSemantics()->SetProcedureState(&savedState);
}

void Closure::AddChildClosure(Closure *child)
{
    ThrowIfNull(child);
    ThrowIfTrue(child->m_closureClass);
    ThrowIfTrue(child->m_parent);

    m_childList.AddLast(child);
    child->m_parent = this;
}

void Closure::CreateClosureClassCode()
{
    // The parents metadata should be generated before the childs
    AssertIfFalse(m_parent ? m_parent->m_closureClass : true);
    CreateClass();
    CreateGenericInfo();
    CreateClassVariable();
    CreateFields();
    ReparentProcedures();

    // If we're in a generic go ahead and fixup the generic code
    if ( m_root->InGeneric() )
    {
        FixupGenericTypes();
    }

    // Force all children to generate their metadata
    ListValueIterator<Closure*,NorlsAllocWrapper> it(&m_childList);
    while(it.MoveNext())
    {
        it.Current()->CreateClosureClassCode();
    }
}

void Closure::CreateClass()
{
    AssertIfTrue(m_closureClass);

    Location location = m_block->Loc;

    TransientSymbolStore *store = m_root->GetTransientSymbolStore();
    StringBuffer buf;
    buf.AppendPrintf(CLOSURE_CLASS_PREFIX L"_%u", store->IncrementClosureCount());
    STRING *className = GetCompiler()->AddString(&buf);

    ClassOrRecordType *closureClass = GetTransientSymbolFactory()->AllocClass(false);

    // Get the parent class
    ClassOrRecordType *parentClass;
    if ( m_parent )
    {
        AssertIfFalse(m_parent->m_closureClass);
        parentClass = m_parent->m_closureClass;
    }
    else
    {
        parentClass = GetSemantics()->ContainingClass();
    }

    GetTransientSymbolFactory()->GetClass(
        NULL,
        className,
        className,
        parentClass->GetContainingNamespace()->GetName(),
        GetRootProcedure()->GetSourceFile(),
        GetSemantics()->GetFXSymbolProvider()->GetObjectType(),
        NULL,           // Implements list
        DECLF_Friend,
        t_bad,
        GetTransientSymbolFactory()->AllocVariable(false, false),
        NULL,
        NULL,
        NULL,
        NULL,
        closureClass);

    closureClass->SetBindingDone(true);

    Scope *Hash = GetTransientSymbolFactory()->GetHashTable(className, closureClass, true, 16, NULL);
    Scope *UnbindableHash =GetTransientSymbolFactory()->GetHashTable(className, closureClass, true, 16, NULL);

    closureClass->SetHashes(Hash, UnbindableHash);

    Symbols::SetParent(closureClass, parentClass->GetUnBindableChildrenHash());
    GetSemantics()->RegisterTransientSymbol(closureClass);

    m_closureClass = closureClass;
}

void Closure::CreateClassVariable()
{

    TransientSymbolStore *store = m_root->GetTransientSymbolStore();
    STRING *name = ClosureNameMangler::EncodeClosureVariableName(
        GetCompiler(), 
        L"ClosureVariable", 
        store->IncrementClosureVariableCount());

    Variable *ClosureVariable =
        GetNormalSymbolFactory()->AllocVariable(false /* No location */, false /* No value */);

    GetNormalSymbolFactory()->GetVariable(
        NULL,
        name,
        name,
        DECLF_Public,
        VAR_Local,
        m_externalBinding ? static_cast<BCSYM*>(m_externalBinding) : static_cast<BCSYM*>(m_closureClass),
        NULL,   // No value
        NULL,   // No symbol list to add to
        ClosureVariable);
    m_closureVariable = ClosureVariable;
    m_root->AddVariableFlag(m_closureVariable, CL_ClosureVar);
}

void Closure::CreateFields()
{
    HashSetIterator<Variable*,NorlsAllocWrapper> it(&m_liftSet);
    while(it.MoveNext())
    {
        CreateField(it.Current());
    }
}

//==============================================================================
// Actually create the field in the closure
//
// private:
//==============================================================================
Variable *Closure::CreateField(Variable *var)
{
    AssertIfFalse(var);
    Assume(m_closureClass && m_closureVariable, L"Class must be created by this point");
    AssertIfTrue(m_liftedToFieldMap.Contains(var));

    // There are 3 types of variables that can be lifted into a closure and they
    // are named by a specific naming convention to give visibity for ENC, the
    // Debugger and QA.
    //
    //  1) Me -> $VB$Me
    //  2) Local Variable -> $VB$Local_OriginalName
    //  3) NonLocal Variable -> $VB$NonLocal_OriginalName
    //
    // The only type of NonLocals that are available for lifting are other closure
    // variables
    STRING *VarName = NULL;
    VariableFlags flags = m_root->GetVariableFlags(var);
    BCSYM *variableType = var->GetType();
    if (var->IsMe())
    {
        if ( var->GetType() == m_root->m_proc->GetContainingClass() )
        {
            ThrowIfTrue(m_parent);  // Only the root closure should lift the outer me
            VarName = ClosureNameMangler::EncodeMeName(GetCompiler());
        }
        else
        {
            VarName = ClosureNameMangler::EncodeNonLocalVariableName(GetCompiler(), var->GetType()->PClass()->GetName());
        }

        if ( var->GetType()->IsClass() && IsGenericOrHasGenericParent(var->GetType()->PClass()) )
        {
            variableType = CreateTypeBindingForMetaData(var->GetType()->PClass());
        }
    }
    else if ( 0 != (CL_ClosureVar & flags) )
    {
        VarName = ClosureNameMangler::EncodeNonLocalVariableName(GetCompiler(), var->GetName());
    }
    else
    {
        VarName = ClosureNameMangler::EncodeLocalVariableName(GetCompiler(), var->GetName());
    }

    BCSYM *fieldType = GetEquivalentTypeForMetaData(variableType);
    Variable *field = GetTransientSymbolFactory()->AllocVariable( false, false );    

    GetTransientSymbolFactory()->GetVariable(
        NULL, // No location
        VarName,
        VarName,
        DECLF_Public,               // Needs to be public for closures to work with DLinq.
                                    // Dlinq expression evaluator does not load non-public members.
        VAR_Member,
        fieldType,
        NULL,
        NULL,
        field);

    Symbols::AddSymbolToHash(m_closureClass->GetHash(), field, true, false, false);
    GetSemantics()->RegisterTransientSymbol(field);
    AssertIfNull(field);
    AssertIfFalse(field->GetContainingClass() == m_closureClass);

    m_liftedToFieldMap.SetValue(var, field);
    return field;
}

//==============================================================================
// Reparent the owned procedures to the Closure class.  Originally we put all
// of the methods onto the root class as a place holder.  Now we need to
// attach them to the generated class
//==============================================================================
void Closure::ReparentProcedures()
{
    AssertIfNull(m_closureClass);

    ListValueIterator<Procedure*,NorlsAllocWrapper> it(&m_procList);
    while(it.MoveNext())
    {
        Procedure *current = it.Current();

        Symbols::ClearParent(current);
        Symbols::AddSymbolToHash(m_closureClass->GetUnBindableChildrenHash(), current, true, false, false);
        current->SetIsShared(false);
    }
}

//==============================================================================
// Build up the generic information for this closure if necessary.
//==============================================================================
void Closure::CreateGenericInfo()
{
    // This is only necessary if the class or procedure is generic
    Procedure *rootProc = GetRootProcedure();
    ClassOrRecordType *rootClass = rootProc->GetContainingClass();
    if ( !rootProc->IsGeneric() && !IsGenericOrHasGenericParent(rootClass))
    {
        return;
    }

    // First build up the parameters
    CreateGenericParameters();

    // Now build up the bindings
    CreateExternalGenericBinding();
    CreateInternalGenericBinding();
}

//==============================================================================
// Builds up the generic parameters for the class
//
// private:
//==============================================================================
void Closure::CreateGenericParameters()
{
    AssertIfFalse(m_closureClass);  // Preformed after class creation

    // If we have a parent then we are no more generic that it.  We inherit
    // the necessary parameters and have no need to add any more to the mix
    if ( m_parent )
    {
        return;
    }

    // Need to build up a generic parameters and bindings
    Symbols *symbols = GetTransientSymbolFactory();

    // First build up the parameters
    Procedure *rootProc = GetRootProcedure();
    if ( rootProc->IsGeneric() )
    {
        CopyGenericParametersForType(rootProc);
        symbols->SetGenericParams(m_firstGenericParam, m_closureClass);
    }
}

//==============================================================================
// Build up the generic binding for the type when used within the containing
// lambda or procedure.
//==============================================================================
void Closure::CreateExternalGenericBinding()
{
    Symbols *symbols = GetNormalSymbolFactory();
    NorlsAllocator *norlsAlloc = symbols->GetNorlsAllocator();
    Procedure *rootProc = GetRootProcedure();
    ClassOrRecordType *rootClass = rootProc->GetContainingClass();

    unsigned paramCount = m_closureClass->GetGenericParamCount();
    BCSYM **argArray = NULL;
    if ( paramCount )
    {
        // If we are a nested closures we have no actual generic parameters since
        // we inherit them all from our parent closure class
        AssertIfFalse(m_containingProc == rootProc);
        argArray = reinterpret_cast<BCSYM**>(norlsAlloc->Alloc(sizeof(BCSYM*) * paramCount));

        // When we are used within the root procedure, we must derive our bindings
        // from the root procedure.  We inherit the class ones since we are
        // nested within it
        unsigned index = 0;

        ThrowIfFalse(rootProc->GetGenericParamCount()==paramCount);
        AppendGenericParameters(rootProc, argArray, index);
    }

    // Get the parent binding
    GenericTypeBinding *parentBinding = NULL;
    if ( m_parent )
    {
        if ( m_parent->m_containingProc == m_containingProc )
        {
            // We're a nested closure inside the same procedure as
            // our parent.  Just grab the parent binding
            parentBinding = m_parent->m_externalBinding;
        }
        else
        {
            // We're a nested closure not inside the same closure
            // as our parent.  In this case we need to grab the
            // internal binding of our parent.
            parentBinding = m_parent->m_internalBinding;
        }
    }
    else if ( IsGenericOrHasGenericParent(rootClass) )
    {
        // Use the simple binding of our parent
        parentBinding = CreateSimpleTypeBinding(rootClass);
    }

    GenericBinding *binding = symbols->GetGenericBinding(
            false,
            m_closureClass,
            argArray,
            paramCount,
            parentBinding);
    AssertIfFalse(binding->IsGenericTypeBinding());
    m_externalBinding = binding->PGenericTypeBinding();
}

//==============================================================================
// Create the internal binding.  This is the binding used for the class in
// internal methods
//
// private:
//==============================================================================
void Closure::CreateInternalGenericBinding()
{
    AssertIfFalse(m_closureClass);

    Symbols *symbols = GetNormalSymbolFactory();
    NorlsAllocator *norlsAlloc = symbols->GetNorlsAllocator();
    Procedure *rootProc = GetRootProcedure();
    ClassOrRecordType *rootClass = rootProc->GetContainingClass();

    unsigned paramCount = m_closureClass->GetGenericParamCount();
    BCSYM **argArray = NULL;
    if ( paramCount )
    {
        argArray = reinterpret_cast<BCSYM**>(norlsAlloc->Alloc(sizeof(BCSYM*) * paramCount));
        unsigned index = 0;
        AppendGenericParameters(m_closureClass, argArray, index);
    }

    // Get the parent binding
    GenericTypeBinding *parentBinding = NULL;
    if ( m_parent )
    {
        // We're a nested closure so just get the binding
        // from our parent
        parentBinding = m_parent->m_internalBinding;
    }
    else if ( IsGenericOrHasGenericParent(rootClass) )
    {
        parentBinding = CreateSimpleTypeBinding(rootClass);
    }

    GenericBinding *binding = symbols->GetGenericBinding(
            false,
            m_closureClass,
            argArray,
            m_closureClass->GetGenericParamCount(),
            parentBinding);
    AssertIfFalse(binding->IsGenericTypeBinding());
    m_internalBinding = binding->PGenericTypeBinding();
}

//==============================================================================
// Whether or not the variable is owned by this closure.  In other words is the
// variable a local variable in the associated blocks's scope that is lifted
//
// public:
//==============================================================================
bool Closure::OwnsVariable(Variable *var)
{
    ThrowIfNull(var);

    return (m_block->Locals == var->GetImmediateParent() || var->IsTemporary() )
        && m_liftSet.Contains(var);
}

//==============================================================================
// Whether or not this closure owns this procedure.  This is true if the procedure
// is a member of the closure
//==============================================================================
bool Closure::OwnsProcedure(Procedure *proc)
{
    AssertIfNull(proc);

    return this == m_root->FindOwningClosure(proc);
}

ILTree::SymbolReferenceExpression *Closure::CreateReferenceToThisClosure(bool useMe, Location loc)
{
    ILTree::SymbolReferenceExpression *baseRef;
    if ( useMe )
    {
        baseRef = GetSemantics()->AllocateSymbolReference(
            m_closureClass->GetMe(),
            m_closureClass,
            NULL,
            loc);
    }
    else
    {
        baseRef = GetSemantics()->AllocateSymbolReference(
            m_closureVariable,
            m_closureVariable->GetType(),
            NULL,
            loc);
    }

    return baseRef;
}

//==============================================================================
// Update this symbol reference so that it will point the the new lifted variable
//==============================================================================
void Closure::FixupVariableReference(ILTree::SymbolReferenceExpression *symRef, Procedure *refProc)
{
    ThrowIfNull(symRef);
    ThrowIfNull(refProc);
    ThrowIfFalse(symRef->Symbol->IsVariable());

    Variable *var = symRef->Symbol->PVariable();
    Symbol *type = symRef->ResultType;
    *symRef = *CreateReferenceToLiftedVariable(var, refProc, symRef->Loc);

    // Dev10 #683075
    // We can not assume that ResultType of the SymbolReferenceExpression matches the type of the variable.
    // For example, 'ReDim' does a trick of changing ResultType from 'Object' to 'Object()'.
    if (var->GetType() != type)
    {
        SetResultType(*symRef, m_root->GetEquivalentType(type, refProc));
    }
}

//==============================================================================
// Create a reference to a field in the closure in the specified procedure.  This
// will do all of the necessary lifting and proxying via other closures.
//
// "var" in this case refers to the original lifted variable
//
// "refProc" must be a lambda on the Closure or the containing proc
//
// public:
//==============================================================================
ILTree::SymbolReferenceExpression *Closure::CreateReferenceToLiftedVariable(Variable *var, Procedure *refProc, Location loc)
{
    ThrowIfNull(var);
    ThrowIfNull(refProc);
    ThrowIfNull(m_closureVariable);
    ThrowIfNull(m_closureClass);

    bool isMyProc = false;
    if ( refProc )
    {
        isMyProc = OwnsProcedure(refProc);
    }
    Assume(isMyProc || refProc == m_containingProc, L"Should be one or the other");

    ILTree::SymbolReferenceExpression *symRef;
    Closure *varOwner;
    if ( var->IsMe() )
    {
        if ( m_parent )
        {
            if ( m_containingProc == refProc &&
                (m_parent->m_containingProc == m_containingProc || m_parent->OwnsProcedure(m_containingProc) ))
            {
                // Parent is directly visible because either
                //   1) The closure variable for my parent is in the same procedure my closure variable is in
                //   2) My parent owns the lambda my closure variable is in
                // In either case my Parent is directly visible and it's variables can be directly accessed from
                // the referencing proc.  A child closure can never lift 'Me' so trying to do so here will often
                // result in non-optimal code.  Instead defer to our parent and let them make the decision.
                //
                // Take the following example
                //
                // Class Bar
                //   Dim z As Integer
                //
                //   Sub Case1
                //     Dim x = 5
                //     If True Then
                //       Dim y = 6
                //       Me.z = 10
                //       Dim l = Function() x + y
                //     End If
                //   End Sub
                // End Class
                //
                // In this case two closures will be created.  The first for the proc lifting 'x' called 'closure1'
                // and the second for the if block lifting 'y' called 'closure2'.  The symbol 'Me' in the line
                // 'Me.z = 10' must be fixed up.  Optimally this should not actually lift 'Me' , however 'closure2'
                // isn't really in a positon to decide so we should defer to our parent who knows better.
                return m_parent->CreateReferenceToLiftedVariable(var, refProc, loc);
            }
            else
            {
                // This is a Me. reference and this is not a root closure.  Need to
                // refer to me on the outer most parent
                symRef = CreateReferenceToParentLiftedVariable(
                    GetRootParent(),
                    var,
                    isMyProc ? m_internalBinding : m_externalBinding,
                    CreateReferenceToThisClosure(isMyProc, loc),
                    refProc);
            }
        }
        else
        {
            // This is the root closure.  We lift me directly so just use the variable
            Variable *meField = EnsureLiftedAndCreated(var);
            symRef = GetSemantics()->AllocateSymbolReference(
                        meField,
                        meField->GetType(),
                        CreateReferenceToThisClosure(isMyProc, loc),
                        loc,
                        isMyProc ? m_internalBinding : m_externalBinding);
        }
    }
    else if ( this->OwnsVariable(var))
    {
        // If we own this variable then it was added during the metadata
        // generation phase.  Grab the existing variable and change the reference
        Variable *fieldInClosure = EnsureLiftedAndCreated(var);

        // For calculating the type it's important to note where we are.
        // 1) InMyProc - We're in my proc so just use the type of the lifted
        //    field, nothing really special
        // 2) Not InMyProc - Now we're in the containing procedure.  When lifting
        //    a variable we do not actually change it's type.  The only item we
        //    really change is it's generic binding.  That being said we should
        //    just use the original type.
        //
        //    This is especially important when lifting a field who's type is a
        //    generic param.  Assume we had the following method declaration
        //
        //    Sub Foo(Of T)
        //      Dim x As T
        //      ... Do something to lift x
        //      Dim val = x Is Nothing
        //    End Sub
        //
        //    In this case the type of x within Foo is still the generic param T
        //    even though inside the closure it's a different named generic param
        Symbol *type = isMyProc ? fieldInClosure->GetType() : var->GetType();

        // Bug #96148 - DevDiv Bugs
        if (type && TypeHelpers::IsPointerType(type))
        {
            // This occurs for references to symbols for Byref arguments.
            type = TypeHelpers::GetReferencedType(type->PPointerType());
        }

        symRef = GetSemantics()->AllocateSymbolReference(
            fieldInClosure,
            type,
            CreateReferenceToThisClosure(isMyProc, loc),
            loc,
            isMyProc ? m_internalBinding : m_externalBinding);
    }
    else if (m_containingProc == refProc &&
            (varOwner = m_root->FindOwningClosure(var)) &&
            (varOwner->m_containingProc == m_containingProc || varOwner->OwnsProcedure(m_containingProc)))
    {
        // We don't own this variable but the owning closure variable is visible
        // because it is either ...
        // 1) In the same procedure is my closure variable.  In which case it's
        //    in the chain of my parents and is directly visible
        // 2) Owns the procedure my class variable is in.  In which case it's
        //    visible via 'Me'
        symRef = varOwner->CreateReferenceToLiftedVariable(var, refProc, loc);
    }
    else
    {
        // We don't own this variable so it's owned by a different closure.  Find
        // that closure and make sure it's lifted into this closure.  We will then
        // generate a reference into the owning closure
        ThrowIfNull(m_parent);

        // Every lifted symbol is owned by some closure.  This is created during
        // the initialization phase.
        Closure *owner = m_root->FindOwningClosure(var);
        AssertIfNull(owner);
        symRef = CreateReferenceToParentLiftedVariable(
            owner,
            var,
            isMyProc ? m_internalBinding : m_externalBinding,
            CreateReferenceToThisClosure(isMyProc, loc),
            refProc);
    }

    return symRef;
}

//==============================================================================
// Create a reference to a variable stored in a parent closure.  Lifts the appropriate
// closures into this closure when needed
//==============================================================================
ILTree::SymbolReferenceExpression *Closure::CreateReferenceToParentLiftedVariable(Closure *parent, Variable *var, GenericBinding *varBinding, ILTree::SymbolReferenceExpression *baseRef, Procedure *referencingProc)
{
    AssertIfNull(parent);
    AssertIfNull(var);
    AssertIfFalse(parent->m_closureVariable);
    AssertIfNull(referencingProc);

    // A note about the referencing proc.  This is the proc containing the symbol reference we are trying
    // to fixup.  It's only important with respect to generic binding.  If the proc is the containing
    // proc for a closure it must use it's external binding, otherwise it's internal binding.

    if ( parent->m_containingProc == m_containingProc )
    {
        // We can only directly lift variables from closures when the closure variables for
        // me and my parent are in the same procedure.  Otherwise it would be impossible
        // to create a direct assignment statement
        Variable *parentField = EnsureLiftedAndCreated(parent->m_closureVariable);
        Variable *liftedField = parent->EnsureLiftedAndCreated(var);

        // See comment in Closure::FixupVariableReference() under the line
        //  else if ( this->OwnsVariable(var))
        // for a detailed explanation of why the type is computed this way
        Symbol *liftedType = (GetRootProcedure() == referencingProc )
            ? var->GetType()
            : liftedField->GetType();
        GenericTypeBinding *parentBinding = (parent->m_containingProc == referencingProc)
            ? parent->m_externalBinding
            : parent->m_internalBinding;
        return GetSemantics()->AllocateSymbolReference(
            liftedField,
            liftedType,
            GetSemantics()->AllocateSymbolReference(
                parentField,
                parentField->GetType(),
                baseRef,
                baseRef->Loc,
                varBinding),
            baseRef->Loc,
            parentBinding);
    }
    else
    {
        // Harder case.  We need to do a recursive lift. First find the closure that we are
        // associated with.  Get the closure that owns the procedure we are associated with
        Closure *nextClosure = m_root->FindOwningClosure(m_containingProc);
        AssertIfNull(nextClosure);

        // Get the binding for the closure
        GenericTypeBinding *nextBinding = nextClosure->m_containingProc == referencingProc
            ? nextClosure->m_externalBinding
            : nextClosure->m_internalBinding;

        // Make sure that closure is lifted into us.  This closure will appear as Me inside
        // of our closure class since it owns our procedure.  However this is a recursive call.
        // baseRef is started by a child closure and at this point it has gotten access to
        // this closure instance.  It's not possible to tag on Me here so instead we need to
        // lift this variable and tag on the member field
        Variable *nextVar = EnsureLiftedAndCreated(nextClosure->m_closureClass->GetMe());

        // Now create a new base reference to the lifted var based on the current
        // base reference
        baseRef = GetSemantics()->AllocateSymbolReference(
            nextVar,
            nextVar->GetType(),
            baseRef,
            baseRef->Loc,
            varBinding);

        if ( nextClosure != parent )
        {
            // Ask our owner to create the reference
            return nextClosure->CreateReferenceToParentLiftedVariable(
                parent,
                var,
                nextBinding,
                baseRef,
                referencingProc);
        }
        else
        {
            // Return the field ref directly
            Variable *liftedField = nextClosure->EnsureLiftedAndCreated(var);
            return GetSemantics()->AllocateSymbolReference(
                liftedField,
                liftedField->GetType(),
                baseRef,
                baseRef->Loc,
                nextBinding);
        }
    }
}

//==============================================================================
// Create an assignment between the two listed variables
//==============================================================================
ILTree::Statement *Closure::CreateMemberAssignment(Variable *targetVar, GenericTypeBinding *targetVarBinding, Variable *sourceVar)
{
    AssertIfNull(sourceVar);
    AssertIfNull(targetVar);
    AssertIfFalse(targetVar->GetContainingClass() == m_closureClass);

    Location hiddenLoc = Location::GetHiddenLocation();

    ILTree::SymbolReferenceExpression *source =
        GetSemantics()->AllocateSymbolReference(
            sourceVar,
            sourceVar->GetType(),
            NULL,
            hiddenLoc);

    ILTree::SymbolReferenceExpression *target =
        GetSemantics()->AllocateSymbolReference(
            targetVar,
            targetVar->GetType(),
            GetSemantics()->AllocateSymbolReference(
                m_closureVariable,
                m_closureVariable->GetType(),
                NULL,
                hiddenLoc),
            hiddenLoc,
            targetVarBinding);

    ILTree::ExpressionWithChildren *initExpr =
        (ILTree::ExpressionWithChildren *)(GetSemantics()->AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            target,
            source,
            hiddenLoc));

    ILTree::Statement *initStmt = GetSemantics()->AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
    initStmt->AsStatementWithExpression().Operand1 = initExpr;
    initStmt->Parent = m_block;

    return initStmt;
}

//==============================================================================
// Create an instance of the closure on the specified block using the
// passed in argument list
//
// private:
//==============================================================================
ILTree::Statement *Closure::CreateInstance(ILTree::ExecutableBlock *block, bool useCopyConstructor)
{
    AssertIfNull(block);
    AssertIfNull(m_closureVariable);

    Semantics &semantics = *GetSemantics();
    Location hiddenLoc = Location::GetHiddenLocation();

    ExpressionList *ctorArgList = NULL;
    if ( useCopyConstructor )
    {
        if ( !m_copyConstructor )
        {
            CreateCopyConstructor();
        }

        // The argument for the copy constructor is the actual variable so it
        // can re-init
        ctorArgList = semantics.AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                semantics.AllocateExpression(
                    SX_ARG,
                    m_internalBinding ? static_cast<BCSYM*>(m_internalBinding) : static_cast<BCSYM*>(m_closureClass),
                    semantics.AllocateSymbolReference(
                        m_closureVariable,
                        m_closureVariable->GetType(),
                        NULL,
                        hiddenLoc),
                    hiddenLoc),
                hiddenLoc);
    }
    else
    {
        if ( !m_emptyConstructor )
        {
            CreateEmptyConstructor();
        }
    }

    // Generate the "<ClosureLocal> = New <ClosureClass>" statement
    ILTree::Expression *source =
        semantics.CreateConstructedInstance(
            m_closureVariable->GetType(),
            hiddenLoc,
            hiddenLoc,
            ctorArgList,
            false,  // No arguments are bad
            ExprNoFlags);

    ILTree::SymbolReferenceExpression *target =
        semantics.AllocateSymbolReference(
            m_closureVariable,
            m_closureVariable->GetType(),
            NULL,
            hiddenLoc);

    ILTree::ExpressionWithChildren *initExpr =
        reinterpret_cast<ILTree::ExpressionWithChildren*>(semantics.AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            target,
            source,
            hiddenLoc));

    ILTree::Statement *initStmt = semantics.AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
    initStmt->Parent = block;
    initStmt->AsStatementWithExpression().Operand1 = initExpr;
    return initStmt;
}

//==============================================================================
// Generate the initialization code for the closure class that will go at
// the start of the block
//==============================================================================
void Closure::CreateInitializationCode()
{
    AssertIfNull(m_closureClass);
    AssertIfNull(m_closureVariable);

    Semantics &semantics = *GetSemantics();
    ILTree::Statement *firstStmt = NULL;
    ILTree::Statement *lastStmt = NULL;
    bool liftedCatchException = false;

    Location hiddenLoc = Location::GetHiddenLocation();

    // Make sure the block has a locals hash.
    if ( !m_block->Locals )
    {
        m_root->CreateLocalsHash(m_containingProc, m_block);
    }

    // Add the symbols to the hash
    Symbols::AddSymbolToHash(m_block->Locals, m_closureVariable, true, false, false);

    ILTree::Statement *initStmt;
    if ( m_block->bilop == SB_FOR ||
        m_block->bilop == SB_FOR_EACH ||
        m_block->bilop == SB_LOOP ||
        m_root->m_procContainsGoto )
    {
        // Insert the for/foreach/loop/goto shim.  Instead of running the normal constructor on the
        // closure we need to run the copy closure.  This will ensure that iterations
        // of the loop or goto jumps will maintain the previous values of uninitialized
        // variables and will logically separate
        initStmt = CreateInstance(m_block, true);
    }
    else
    {
        initStmt = CreateInstance(m_block, false);
    }

    // If we are generating ENCable code then make sure to generate both
    // constructors so we don't cause a rude edit later on
    if (m_root->m_genEncCode )
    {
        if ( !m_copyConstructor )
        {
            CreateCopyConstructor();
        }
        if ( !m_emptyConstructor)
        {
            CreateEmptyConstructor();
        }
    }

    if (m_block->bilop == SB_FOR_EACH)
    {
        // Assert that there is only one closure associated to ForEach block;
        ThrowIfFalse(m_block->AsForEachBlock().InitializeClosure == NULL);
        m_block->AsForEachBlock().InitializeClosure = initStmt;
    }
    else
    {
        // Update the statements
        UpdateStatementList(&firstStmt, &lastStmt, initStmt);
    }

    // Generate the "<ClosureLocal>.<SomeArg> = <SomeArg>" statements.
    HashTableConstIterator<Variable*,Variable*,NorlsAllocWrapper> it(&m_liftedToFieldMap);
    while (it.MoveNext())
    {
        Variable *liftedVar = it.Current().Key();
        Variable *closureFieldVar = it.Current().Value();

        // Get the flags
        VariableFlags liftedVarFlags = m_root->GetVariableFlags(liftedVar);
        VariableFlags closureFieldVarFlags = m_root->GetVariableFlags(closureFieldVar);

        // This work is not necessary (and causes bugs) when this is a closure
        // we've lifted.
        if ( liftedVarFlags & CL_ClosureVar )
        {
            continue;
        }

        // If the liftedVariable is ever used as a catch variable we need to go through
        // all of those catch blocks and insert the initialization code wherever our
        // catch variable is lifted.
        if ( liftedVarFlags & CL_Catch )
        {
            FixupCatchStatements(liftedVar);
        }

        ILTree::Statement *created = NULL;
        if ( liftedVar->GetVarkind() == VAR_Param )
        {
            created = CreateMemberAssignment(closureFieldVar, m_externalBinding, liftedVar);
        }
        else if ( liftedVar->IsLocal() )
        {
            AssertIfFalse(liftedVar->GetImmediateParent() &&
                        (liftedVar->GetImmediateParent()->IsHash() ||
                        liftedVar->GetImmediateParent()->IsProc())); // For temporaries.

            // Handle case of lifting the special function return local.
            if ( liftedVar->GetVarkind() == VAR_FunctionResult)
            {
                ILTree::ProcedureBlock *containingTree = m_root->GetProcedureBody(m_containingProc);
                containingTree->ReturnVariable = closureFieldVar;
                containingTree->LiftedReturnVarRef = CreateReferenceToLiftedVariable(
                        liftedVar,
                        m_containingProc,
                        Location::GetHiddenLocation());
            }

            Declaration *immediateParent = liftedVar->GetImmediateParent();
            if ( immediateParent && immediateParent->IsHash() )
            {
                Symbols::RemoveSymbolFromHash(immediateParent->PHash(), liftedVar);
            }
            else if ( liftedVar->IsTemporary())
            {
                liftedVar->GetTemporaryManager()->FreeTemporary(liftedVar);
            }
        }

        if ( created )
        {
            // Constructors must be special cased here.  Before calling the base class constructor
            // we cannot access any uninitiliazed base class data or Me.  However it's legal to access
            // non-base class data (locals, params, etc ...).  The closure must be initialized before
            // calling the base class constructor though because it's possible to lift a parameter
            // to the constructor that is also passed to the base class constructor Ex.
            //
            // Class Derived
            //      Sub New(ByVal x As Integer)
            //          MyBase.New(x)
            //          ...
            //          ' Lift x
            //      End Sub
            // End Class
            //
            // This is all well and good unless Me is forced to be lifted.  Without a shim we would
            // be assigning ClosureVar.LifteMe = Me before we called the base class constructor.  This
            // will produce unverifiable code.  Instead what we need to do is move the Me assignment
            // just after the base class call.
            //
            // This will produce verifiable code.  Also any situtaion where Me is accessed outside of
            // the closure before it is assigned to the Me field will be illegal.  Because that means
            // the access to Me is occurring before the call to MyBase.New()
            //
            // If inside a struct, don't special case the constructor since we've already emitted
            // ERRID_CannotLiftStructureMeLambda in CheckVariableSafeForLifting and baseStmt==NULL
            if ( liftedVar->IsMe()
                    && SB_PROC == m_block->bilop
                    && m_block->AsProcedureBlock().pproc->IsInstanceConstructor() 
                    && !liftedVar->GetType()->IsStruct() )
            {
                // Find the first call block with Me as the base
                ILTree::ProcedureBlock &body = m_block->AsProcedureBlock();
                ILTree::Statement *baseStmt = FindBaseOrMeConstructorCall(liftedVar, body.pproc, &body);

                ThrowIfNull(baseStmt);  // How could there not be a base ctor call
                InsertStatementList(baseStmt, created);
            }
            else
            {
                UpdateStatementList(&firstStmt, &lastStmt, created);
            }
        }
    }

    // For typical locals which are assigned values, we update the symbol reference
    // to point to the lifted field inside our structure thereby eliminating the
    // need to generate a new assignment.  In the case where we lift a parent closure
    // we must actually generate the assignment since this is new code.
    Closure *parent = this->GetParent();
    while ( parent )
    {
        AssertIfNull(parent->m_closureVariable);
        Variable *myField;
        if ( m_liftedToFieldMap.GetValue(parent->m_closureVariable, &myField) )
        {
            ILTree::Statement *created;
            if ( parent->m_containingProc == m_containingProc )
            {
                // If the closure variables have the same containing procedure then a
                // direct assignment is possible
                created = CreateMemberAssignment(myField, m_externalBinding, parent->m_closureVariable);
            }
            else
            {
                // Otherwise we are not in scope.  We can only directly lift variables that
                // are either in the same procedure or when it's our parent.  This case is
                // when it's the owner of the procedure we live in so assign from me
                AssertIfFalse(parent->OwnsProcedure(m_containingProc));
                created = CreateMemberAssignment( myField, m_externalBinding, parent->m_closureClass->GetMe());
            }
            UpdateStatementList(&firstStmt, &lastStmt, created);
        }

        parent = parent->GetParent();
    }

    AssertIfNull(lastStmt || SB_FOR_EACH == m_block->bilop );
    AssertIfNull(firstStmt || SB_FOR_EACH == m_block->bilop);

    // If our block is a catch block then we need to make sure the closure is
    // initialized before any filter code is run because it is possible to
    // access lifted variables inside a filter block and the filter block is
    // techinally a part of the catch block so the catch closure is responsible
    // for the lifted variables;
    if ( SB_CATCH == m_block->bilop )
    {
        // It's imperative that we insert our initialization code at the start
        // of ExceptionStore.  It's very possible that the catch variable
        // for this block is also a lifted variable.  In which case this
        // closure will lift it (directly or by proxy).  So we must initialize
        // our closure before the assignment from the exception temporary into
        // the lifted variable occurs
        ILTree::CatchBlock *catchStmt = &m_block->AsCatchBlock();
        ::InsertStatementListBefore(catchStmt->ExceptionStore, firstStmt);
        catchStmt->ExceptionStore = firstStmt;
    }
    else if(lastStmt && firstStmt)
    {
        lastStmt->Next = m_block->Child;
        if ( m_block->Child )
        {
            m_block->Child->Prev = lastStmt;
        }
        m_block->Child = firstStmt;
    }

    // Now force all of our children to generate their initialization code
    ListValueIterator<Closure*,NorlsAllocWrapper> itChild(&m_childList);
    while (itChild.MoveNext())
    {
        itChild.Current()->CreateInitializationCode();
    }
}

//==============================================================================
// Called when the closure lifts a variable used 1 or more times as a catch
// variable.  This will shim all of the catch blocks where the variable is used
// as a catch block.
//
// The shim is necessary because special steps must be taken when a non-local
// variable is used as the catch field.  The special steps are necessary to
// deal with limitations in IL.
//
// private
//==============================================================================
void Closure::FixupCatchStatements(Variable *liftedVar)
{
    ThrowIfNull(liftedVar);

    HashTableIterator<ILTree::CatchBlock*,Variable*,NorlsAllocWrapper> catchIt(&m_root->m_catchMap);
    while ( catchIt.MoveNext() )
    {
        ILTree::CatchBlock *catchStmt = catchIt.Current().Key();
        Variable *catchVar = catchIt.Current().Value();

        if ( catchVar == liftedVar )
        {
            ThrowIfTrue(catchStmt->ExceptionTemporary);
            ThrowIfTrue(catchStmt->ExceptionStore);

            // Find the nearest closure to the catch statement.  Should not be null
            // because at the very least it should resolve to this closure
            Closure *cls = m_root->FindNearestClosure(catchStmt);
            ThrowIfNull(cls);

            // Find the procedure for the catch statement.
            //
            // Cannot use cls->m_containingProc here.  The nearest closure may be the
            // not be in the same procedure as the catch statement (in which case
            // it would be the owner of the procedure holding the catch statement).
            // Using cls->m_containingProc in that case can yield incorrect generic
            // bindings.
            ILTree::ExecutableBlock *cur = catchStmt->Parent;
            while ( SB_PROC != cur->bilop )
            {
                cur = cur->Parent;
            }
            ILTree::ProcedureBlock *procBody = &cur->AsProcedureBlock();
            Procedure *proc = procBody->pproc;

            // Allocate the exception temporary.  As insane as it sounds it is possible for
            // the exception to be a generic so make sure to convert the type.
            ThrowIfNull(procBody->ptempmgr);
            Symbol *catchVarType = m_root->GetEquivalentType(catchVar->GetType(), proc);
            catchStmt->ExceptionTemporary = procBody->ptempmgr->AllocateShortLivedTemporary(catchVarType, &catchStmt->Loc);

            // Now create an assignment statement
            Location hiddenLoc = Location::GetHiddenLocation();
            ILTree::Expression *assign = GetSemantics()->AllocateExpression(
                    SX_ASG,
                    TypeHelpers::GetVoidType(),
                    cls->CreateReferenceToLiftedVariable(catchVar, proc, hiddenLoc),
                    GetSemantics()->AllocateSymbolReference(
                        catchStmt->ExceptionTemporary,
                        catchVarType,
                        NULL,
                        hiddenLoc),
                    hiddenLoc);
            ILTree::Statement *stmt = GetSemantics()->AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
            stmt->AsStatementWithExpression().Operand1 = assign;
            stmt->Parent = catchStmt;
            catchStmt->ExceptionStore = stmt;
        }
    }
}

void Closure::FixupGenericTypes()
{
    ClosureGenericVisitor genIt(m_root,this);
    ListValueIterator<Procedure*,NorlsAllocWrapper> it(&m_procList);

    while (it.MoveNext())
    {
        Procedure *proc = it.Current();
        genIt.Visit(proc, proc->GetBoundTree());
    }
}

//==============================================================================
// Create an empty constructor
//
// private:
//==============================================================================
void Closure::CreateEmptyConstructor()
{
    ThrowIfFalse(m_closureClass);
    ThrowIfTrue(m_emptyConstructor);

	Procedure *constructor = Declared::BuildSyntheticConstructor(
			GetTransientSymbolFactory(), 
			GetCompiler(), 
			NULL, 
			Declared::SyntheticConstructorType::Instance);
    Symbols::SetCode(constructor->PSyntheticMethod(), WIDE("\r\n"), 2);
    Symbols::AddSymbolToHash(m_closureClass->GetHash(), constructor, true, false, false);
    m_emptyConstructor = constructor;
}

//==============================================================================
// Create a copy constructor for the closure.  Does a simple member assignment
// between the passed in closure and this closure.  Null is a valid value
// for the other closure
//
// private:
//==============================================================================
void Closure::CreateCopyConstructor()
{
    ThrowIfFalse(m_closureClass);
    ThrowIfTrue(m_copyConstructor);

    Symbols *transientSymbols = GetTransientSymbolFactory();
    Semantics *semantics = GetSemantics();
    Location hiddenLoc;
    hiddenLoc.SetLocationToHidden();

    // Build the constructor signature
    // Sub New(ByVal other As Closure)
    Procedure *constructor = Declared::BuildSyntheticConstructor(
        transientSymbols, 
        GetCompiler(), 
        NULL, 
        Declared::SyntheticConstructorType::Instance);
    Parameter *firstParam = NULL;
    Parameter *lastParam = NULL;
    Parameter *otherParam = transientSymbols->GetParam(
            &hiddenLoc,
            GetCompiler()->AddString(L"other"),
            m_internalBinding
                ? static_cast<BCSYM*>(GetEquivalentTypeForMetaData(m_internalBinding))
                : static_cast<BCSYM*>(m_closureClass),
            PARAMF_ByVal,
            NULL,
            &firstParam,
            &lastParam,
            false);
    constructor->SetFirstParam(firstParam);
    Symbols::AddSymbolToHash(m_closureClass->GetHash(), constructor, true, false, false);

    // Create the variable for the parameter
    BCSYM_Variable *otherLocal = transientSymbols->AllocVariable( false, false );
    SymbolList builtLocals;
    transientSymbols->GetVariable( NULL,
             otherParam->GetName(),
             otherParam->GetName(),
             DECLF_Dim | DECLF_Public,
             VAR_Param,
             m_internalBinding
                ? static_cast<BCSYM*>(m_internalBinding)
                : static_cast<BCSYM*>(m_closureClass),
             NULL, // don't send initializer info
             &builtLocals,
             otherLocal);

    // Create the block
    ILTree::Statement *bodyStmt = semantics->AllocateStatement(SB_PROC, hiddenLoc, NoResume, false);
    ILTree::ProcedureBlock *body = &(bodyStmt->AsProcedureBlock());
    body->Locals = transientSymbols->GetHashTable(GetCompiler()->AddString(L"."), constructor, true, 1, &builtLocals);
    body->LocalsCount = 1;
    body->pproc = constructor;
    body->ptempmgr = new(semantics->m_TreeAllocator) TemporaryManager(
            GetCompiler(),
            semantics->m_CompilerHost,
            constructor,
            &semantics->m_TreeStorage);
    constructor->SetBoundTree(body);

    // Call the base (object) constructor
    ILTree::Statement *baseCallStmt = semantics->AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
    ILTree::StatementWithExpression &baseCall = baseCallStmt->AsStatementWithExpression();
    BCSYM *baseCtor = m_closureClass->GetBaseClass()->PClass()->GetFirstInstanceConstructorNoRequiredParameters(GetCompiler());
    baseCall.Operand1 = semantics->InterpretCallExpressionWithNoCopyout(
            hiddenLoc,
            semantics->ReferToSymbol(
                hiddenLoc,
                baseCtor,
                chType_NONE,
                CreateReferenceToThisClosure(true, hiddenLoc),
                NULL,
                ExprIsConstructorCall | ExprIsExplicitCallTarget),
            chType_NONE,
            NULL,
            false,
            ExprIsConstructorCall | ExprResultNotNeeded,
            NULL);
    baseCallStmt->Parent = body;
    body->Child = baseCallStmt;

    // Build the block for the if statement
    ILTree::Statement *ifBlkStmtRaw = semantics->AllocateStatement(SB_IF_BLOCK, hiddenLoc, NoResume, false);
    ILTree::IfGroup *ifBlk = &(ifBlkStmtRaw->AsIfGroup());
    ifBlk->Parent = body;
    ifBlk->EndConstructLocation = hiddenLoc;
    InsertStatementList(baseCallStmt, ifBlk);

    // Create the If statement
    // if other isnot Nothing Then
    //   ...
    // End If
    ILTree::Statement *ifStmtRaw = semantics->AllocateStatement(SB_IF, hiddenLoc, NoResume, false);
    ILTree::IfBlock *ifStmt = &(ifStmtRaw->AsIfBlock());
    ifStmt->Parent = ifBlk;
    ifBlk->Child = ifStmt;

    // Build the if condition
    // other IsNot Nothing
    ILTree::Expression *ifCond = semantics->AllocateExpression(
            SX_ISNOT,
            semantics->GetFXSymbolProvider()->GetBooleanType(),
            semantics->AllocateSymbolReference( otherLocal, otherLocal->GetType(), NULL, hiddenLoc, m_internalBinding),
            semantics->AllocateExpression( SX_NOTHING, m_closureClass, hiddenLoc),
            hiddenLoc);
    ifStmt->Conditional = ifCond;

    // Now generate the member assignments
    ILTree::Statement *firstStmt = NULL;
    ILTree::Statement *lastStmt = NULL;

    // Add an assignment for each variable
    HashTableIterator<Variable*,Variable*,NorlsAllocWrapper> it(&m_liftedToFieldMap);
    while ( it.MoveNext())
    {
        Variable *oldVar = it.Current().Key();
        Variable *newVar = it.Current().Value();

        // If this is a closure variable don't bother with the assignment
        // since initialization post constructor will take care of assigning
        // the closure variables
        if ( 0 != (CL_ClosureVar & m_root->GetVariableFlags(oldVar)) )
        {
            continue;
        }

        ILTree::SymbolReferenceExpression *left = semantics->AllocateSymbolReference(
                newVar,
                newVar->GetType(),
                CreateReferenceToThisClosure(true, hiddenLoc),
                hiddenLoc,
                m_internalBinding);

        ILTree::SymbolReferenceExpression *right = semantics->AllocateSymbolReference(
                newVar,
                newVar->GetType(),
                semantics->AllocateSymbolReference(
                    otherLocal,
                    otherLocal->GetType(),
                    NULL,
                    hiddenLoc),
                hiddenLoc,
                m_internalBinding);

        ILTree::ExpressionWithChildren *initExpr =
            (ILTree::ExpressionWithChildren *)(semantics->AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                left,
                right,
                hiddenLoc));

        ILTree::Statement *initStmt = GetSemantics()->AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
        initStmt->AsStatementWithExpression().Operand1 = initExpr;
        initStmt->Parent = ifStmt;
        UpdateStatementList(&firstStmt, &lastStmt, initStmt);
    }

    // Add the init code into the if block
    ifStmt->Child = firstStmt;

    // Finished
    m_copyConstructor = constructor;
}

//==============================================================================
// End Closure
//==============================================================================

//==============================================================================
// Start OptimizedClosure
//==============================================================================

void OptimizedClosure::FixupGenericTypes()
{
    ClosureGenericVisitor genIt(m_root, this);      
    Procedure *proc = m_lambdaData->GetProcedure();
    genIt.Visit(proc, proc->GetBoundTree());
}

void OptimizedClosure::SetLambda(LambdaData *lambdaData)
{
    ThrowIfNull(lambdaData);
    ThrowIfTrue(lambdaData->HasClosureVariable());
    ThrowIfTrue(lambdaData->UsesLiftedVariable());
    ThrowIfTrue(m_lambdaData);

    // We own the lambda now
    lambdaData->SetClosure(this);
    m_lambdaData = lambdaData;

    // Reparent this back onto the original class
    Procedure *lambdaProc = lambdaData->GetProcedure();

    // Symbols::ClearParent(lambdaProc);
    // Port SP1 CL 2922610 to VS10
    // jtw: This gets called for field initializers, e.g. private foo as integer = Function() i
    // Field initializers get rolled into a constructor method on the class.  So this parent is going to be wrong
    // because we will pick up the container for the ctor instead of the container where the lambda was defined
    // Symbols::SetParent(lambdaProc, m_root->m_proc->GetPhysicalContainer()->GetUnBindableChildrenHash());

    // Port SP1 CL 2955440 to VS10
    BCSYM_Container *pNewOuterClass = m_root->m_proc->GetPhysicalContainer();

    // Bug #150058 - DevDiv Bugs
    // If we already connected to a different partial declaration of the same class, do not change the parent.
    BCSYM_Container *pOldOuterClass = lambdaProc->GetPhysicalContainer();
    BCSYM_Container *pOldMainClass;
    BCSYM_Container *pNewMainClass;

    if(pOldOuterClass && pOldOuterClass->IsPartialTypeAndHasMainType())
    {
        pOldMainClass = pOldOuterClass->GetMainType();
    }
    else
    {
        pOldMainClass = pOldOuterClass;
    }

    if(pNewOuterClass && pNewOuterClass->IsPartialTypeAndHasMainType())
    {
        pNewMainClass = pNewOuterClass->GetMainType();
    }
    else
    {
        pNewMainClass = pNewOuterClass;
    }

    if (pOldMainClass != pNewMainClass)
    {
        Symbols::ClearParent(lambdaProc); 
        Symbols::SetParent(lambdaProc, pNewOuterClass->GetUnBindableChildrenHash());
    }

    // If the proc is generic make sure to copy the generic parameters.
    // m_root->m_proc() is set during when the closure root is built and contains the lambda
    if ( m_root->m_proc->IsGeneric())
    {
        // 


        CopyGenericParametersForMethod(m_root->m_proc);
        Symbols *symbols = GetTransientSymbolFactory();
        symbols->SetGenericParams(m_firstGenericParam, lambdaProc);
    }

    // Make it private.  Also make it static if it doesn't lift Me
    lambdaProc->SetAccess(ACCESS_Private);
    if ( !lambdaData->UsesMe())
    {
        lambdaProc->SetIsShared(true);
    }
}

void OptimizedClosure::FixupLambdaReference(LambdaData *data)
{
    ThrowIfNull(m_lambdaData);
    ThrowIfFalse(m_lambdaData == data);

    Procedure *lambdaProc = m_lambdaData->GetProcedure();
    Procedure *containingProc = m_root->m_proc;
    ClassOrRecordType *containingClass = containingProc->GetContainingClass();
    TrackedExpression *tracked = m_lambdaData->GetTrackedExpression();

    // We need to build up the GenericTypeBinding based on where the lambda
    // is being used from
    GenericBinding *binding = NULL;
    if ( tracked->GetReferencingProcedure() == containingProc )
    {
        // Called from within the original containing method

        // #1 Build up the binding information if nec----ary.  Default to the typeBinding
        // if the method itself is not Generic
        GenericTypeBinding *typeBinding = IsGenericOrHasGenericParent(containingClass)
                    ? CreateSimpleTypeBinding(containingClass)
                    : NULL;
        binding = typeBinding;
        if ( lambdaProc->GetGenericParamCount() > 0 )
        {
            // When we are used within the root procedure, we must derive our bindings
            // from the root procedure.
            Symbols *symbols = GetNormalSymbolFactory();
            NorlsAllocator *norlsAlloc = symbols->GetNorlsAllocator();
            unsigned paramCount = lambdaProc->GetGenericParamCount();
            BCSYM **argArray = reinterpret_cast<BCSYM**>(norlsAlloc->Alloc(sizeof(BCSYM*) * paramCount));
            unsigned index = 0;

            // The parent procedure is the outer most procedure.  Just append the generic
            // parameters for the binding
            ThrowIfNull(containingProc->GetGenericParamCount()==paramCount);
            AppendGenericParameters(containingProc, argArray, index);
            binding = symbols->GetGenericBinding(
                    false,
                    lambdaProc,
                    argArray,
                    paramCount,
                    typeBinding);
        }
    }
    else
    {
        // Called from within another lambda
        ClosureBase *owner = m_root->FindOwningClosureBase(tracked->GetReferencingProcedure());
        ThrowIfNull(owner);
        binding = CreateLambdaBinding(owner);
    }

    ILTree::Expression **ppExpr = tracked->GetRawExpression();
    ILTree::Expression *expr = *ppExpr;

    // Create the base reference if necessary
    ILTree::SymbolReferenceExpression *baseRef = NULL;
    if ( m_lambdaData->UsesMe() )
    {
        // It's safe to call CreateReferenceToContainingClassMe here because since this is
        // an instance lambda, the caller is gauranteed to be as well
        baseRef = m_root->CreateReferenceToContainingClassMeInProcedure(tracked->GetReferencingProcedure());
    }

    ILTree::SymbolReferenceExpression *lambdaRef = GetSemantics()->AllocateSymbolReference(
        lambdaProc,
        TypeHelpers::GetVoidType(),
        baseRef,
        expr->Loc,
        binding);

    // Create an address of operation
    ILTree::Expression *addr= GetSemantics()->AllocateExpression(
        SX_ADDRESSOF,
        TypeHelpers::GetVoidType(),
        lambdaRef,
        expr->Loc);

    // Convert the delegate call.  Make sure to update the type.  The type
    // can be a generic delegate bound to some of our paramaters.  The
    // binding depends on our location
    BCSYM *resultType = m_root->GetEquivalentType(expr->ResultType, tracked->GetReferencingProcedure());
    *ppExpr = GetSemantics()->ConvertWithErrorChecking(
        addr,
        resultType,
        ExprForceRValue);
}

//==============================================================================
// This occurs when we need to create a binding of our Lambda inside the
// context of another closure.  When a new binding needs to be created we
// typically do the following.
//
// This occurs within a lambda.  Find the owning closure and create a binding.  The
// problem here is that to create a binding the process is the following.
//
//  1) Create a GenericBinding where all GenericParameter are mapped to themselves.
//  2) Get the equivalent binding for #1 in the context of the lambda
//
// Right now the owning closure lacks sufficient knowledge to do so.  Imagine we're
// translating the following method.
//
//  Sub Foo(Of T)
//      Dim col = New QueryableCollection(Of T)(...)
//      Dim q = From c in col Return (From j in col)
//      ...
//  End Sub
//
// This will roughly translate into
//      col.Select(c => col.Select( j=>{j}) )
//
// This will cause two closures to be generated.  One optimized and one normal closure.
//
//  Normal
//  Class NormalClosure(Of CLS0)
//      Maps T -> CLS0
//      Lambda1()(ByVal arg As CLS0) As QueryableCollection(Of AnonymousType(Of CLS0))
//  End Class
//
//  Optimized Closure
//      Maps T -> CLS1
//      Lambda2(Of CLS1)(ByVal arg As CLS1) As QueryableCollection(Of AnonymousType(Of CLS1))
//
//  So what we will do is allow the owning closure to create our parent binding but
//  manually create our own binding in the context of the closure
//==============================================================================
GenericBinding *OptimizedClosure::CreateLambdaBinding(ClosureBase *context)
{
    ThrowIfNull(context);

    GenericTypeBinding *parentBinding = NULL;
    ClassOrRecordType *containingClass = m_root->m_proc->GetContainingClass();
    if ( IsGenericOrHasGenericParent(containingClass) )
    {
        // It's safe to ask the context closure to create the parent binding here.  We
        // don't remap any of the generic parameters outside of the containing
        // method so the Closure has enough information to create this binding.
        parentBinding = context->CreateTypeBinding(containingClass);
    }

    // If the lambda itself is not generic then we don't need to create a further
    // binding of the proc
    Procedure *lambdaProc = m_lambdaData->GetProcedure();
    unsigned paramCount = lambdaProc->GetGenericParamCount();
    if ( !paramCount)
    {
        return parentBinding;
    }

    // Build up a map of what we need to map when creating the binding.  In this
    // case we map
    //  T -> CLS1
    // And the context maps
    //  T -> CLS0
    //
    // So we should be mapping
    //  CLS1 -> CLS0
    //
    // Right now it's not possible for a lambda to contain a partial binding.  By
    // that we mean T->Integer instead of another generic parameter.  But even if
    // it did both lambdas with map T to the same bound value so we only need to
    // worry about re-mapping generic parameters which map to other generic parameters
    DynamicHashTable<GenericParameter*,Type*,NorlsAllocWrapper> map(m_root->GetNorlsAllocator());
    HashTableIterator<GenericParameter*,Type*,NorlsAllocWrapper> it(&m_genericMap);
    while ( it.MoveNext() )
    {
        Type *leftType = it.Current().Value();
        if ( leftType->IsGenericParam() )
        {
            GenericParameter *left = leftType->PGenericParam();
            Type *right = context->GetEquivalentGenericParameter(it.Current().Key());
            map.SetValue(left, right);
        }
    }

    // Now create the binding
    Symbols *symbols = GetNormalSymbolFactory();
    NorlsAllocator *norlsAlloc = symbols->GetNorlsAllocator();
    BCSYM **argArray = reinterpret_cast<BCSYM**>(norlsAlloc->Alloc(sizeof(BCSYM*) * paramCount));
    unsigned index = 0;

    GenericParameter *cur = lambdaProc->GetFirstGenericParam();
    while ( cur )
    {
        ThrowIfFalse(index < paramCount);   // Make sure there's not a mismatch
        argArray[index] = map.GetValue(cur);
        ++index;
        cur = cur->GetNextParam();
    }

    GenericBinding *binding = symbols->GetGenericBinding(
            false,
            lambdaProc,
            argArray,
            paramCount,
            parentBinding);
    return binding;
}

//==============================================================================
// End OptimizedClosure
//==============================================================================

//==============================================================================
// Start MyBaseMyClassStubContext
//==============================================================================

MyBaseMyClassStubContext::MyBaseMyClassStubContext(ClosureRoot *root, Procedure *proc) :
    GenericContext(root),
    m_stubClass(NULL),
    m_originalProc(proc)
{
    ThrowIfNull(proc);

    m_stubClass = m_root->m_proc->GetContainingClass();

    GrabGenericInfo();
}

//==============================================================================
// Grab all of the Generic information for our context.  This is a little bit
// more complex that normal lambdas because there are a lot more cases.
//
// In addition to having to deal with generic methods we also have to deal with
// generic hieararchy's.  With lambda's this is not an issue because they all
// exist on the same level.  With MyBase/MyClass overrides this is not true.
//
// Imagine we have the following scenario.
//
// Class Parent(Of T)
//  Overrideable Sub GetName(ByVal val AS T)
//      ..
//  End Sub
// End Class
//
// Class Child
//   Inherits Parent(Of Integer)
//
//   Overrides Sub GetName(ByVal val As Integer)
//      ..
//   End Sub
//
// End Class
//
// When a call to MyBase.GetName() needs to be stubbed out, 'm_originalProc' will
// refere to the GetName() in Parent() rather than the GetName() in Child.
// However MyClass.GetName() will grab Child.GetName().
//
// This has a lot of implications for bindings when there is a partial bind
// in the hierarchy.  Our Generic map needs to know to map T->Integer in this case.
// So we will troll through the class hierarchy and create the appropriate
// mapping.
//==============================================================================
void MyBaseMyClassStubContext::GrabGenericInfo()
{
    // Troll through the hierarchy mapping
    ClassOrRecordType *originalProcClass = m_originalProc->GetContainingClass();
    if ( originalProcClass != m_stubClass )
    {
        AddClassHierarchyToMap(m_stubClass);
    }

    // If the oroginal procedure was generic copy over the generic parameters.
    // This will create a mapping from the original parameters to the new
    //
    // So if we had
    //  Sub Original(Of T1,U1)
    //
    // We are creating
    //  Sub Stub(Of T2, U2)
    //
    // The map will be
    //  T1 -> T2
    //  U1 -> U2
    if ( m_originalProc->IsGeneric() )
    {
        CopyGenericParametersForMethod(m_originalProc);
    }
}

//==============================================================================
// When copying a MethodStub we just use the original name
//==============================================================================
STRING *MyBaseMyClassStubContext::GetNameForCopiedGenericParameter(GenericParameter *param)
{
    ThrowIfNull(param);
    return param->GetName();
}

//==============================================================================
// Fixup the passed in MyBase/MyClass method call to call a non-virtual stub
//==============================================================================
void MyBaseMyClassStubContext::FixupCall(ILTree::CallExpression *call)
{
    ThrowIfNull(call);
    ThrowIfNull(call->MeArgument);
    ThrowIfFalse(IsMyBaseMyClassSymbol(call->MeArgument));

    ILTree::SymbolReferenceExpression *callProcRef = &call->Left->AsSymbolReferenceExpression();

    // Validate the call arguments.  This context can only work on valid
    // calls against the procedure we were created for
    ThrowIfNull(callProcRef->Symbol);
    ThrowIfFalse(callProcRef->Symbol->IsProc());
    ThrowIfFalse(callProcRef->Symbol->PProc() == m_originalProc);

    // First step is to update the procedure symbol to point to the stub
    UpdateSymbolToStubProcedure(callProcRef, call->MeArgument->uFlags);

    // Now we have created a Stub on the stub class.  We need to update the MeArgument
    // of the call to point to the m_stubClass.  MyBaseMyClass fixup occurs when the
    // BILTREE has not been split into it's lambdas so we're still in the original
    // proc.
    //
    // Even though we've essentially added a new Symbol to the tree we don't have to
    // retrack this symbol.  The origial pass took a double pointer to call->MeArgument
    // and we've just updated the value.  The code will still pass over call->MeArgument
    // if it is necessary for it to be updated
    call->MeArgument = m_root->CreateReferenceToContainingClassMe();
}

//==============================================================================
// Fixup the delegate constructor of a MyBase/MyClass method to call a
// non-virtual stub
//==============================================================================
void MyBaseMyClassStubContext::FixupDelegateConstructor(ILTree::DelegateConstructorCallExpression *call)
{
    ThrowIfNull(call);
    ThrowIfFalse(IsMyBaseMyClassSymbol(&call->ObjectArgument->AsExpression()));
    ThrowIfNull(call->Method);

    ILTree::SymbolReferenceExpression *meSymRef = &call->ObjectArgument->AsSymbolReferenceExpression();
    ILTree::SymbolReferenceExpression *procSymRef = &call->Method->AsSymbolReferenceExpression();
    ThrowIfNull(procSymRef->Symbol == m_originalProc);

    // First step is to update the procedure symbol to point to the stub
    UpdateSymbolToStubProcedure(procSymRef, meSymRef->uFlags);

    // Now update the me argument.
    // See comment in MyBaseMyClass::FixupCall as to the semantis of this
    call->ObjectArgument = m_root->CreateReferenceToContainingClassMe();
}

//==============================================================================
// Given a pointer pointer to a symbol to the original proc, make it change to
// be a symbol referencing the stub proc
//
// Update the existing SymbolReference instead of creating a new one.  The new
// one would not be tracked by the ClosuresRoot and hence would not be
// rewritten if needed
//==============================================================================
void MyBaseMyClassStubContext::UpdateSymbolToStubProcedure(ILTree::SymbolReferenceExpression *symbolRef, unsigned meFlags)
{
    ThrowIfNull(symbolRef);

    // First check and see if we have created the stub already.
    TransientSymbolStore *store = GetTransientSymbolStore();
    ClosureMethodStubHashTable *hash = store->GetClosureMethodStubHash();

    ClosureStubHashKey key;
    key.Target = m_originalProc;
    key.StubClass = m_stubClass;
    key.IsMyBaseStub = meFlags & SXF_SYM_MYBASE;
    Procedure **temp = hash->HashFind(key);
    Procedure *stubProc;
    if ( temp )
    {
        stubProc = *temp;
    }
    else
    {
        STRING *stubName = ClosureNameMangler::EncodeMyBaseMyClassStubName(
                GetCompiler(),
                m_originalProc,
                meFlags);
        stubProc = CreateMethodStubForMyBaseMyClass(stubName, meFlags);
        hash->HashAdd(key, stubProc);
    }

    ThrowIfNull(stubProc);

    // Set the call to invoke the stub
    symbolRef->Symbol = stubProc;

    // Now we have created the stub.  We need to recreate the binding.  The original
    // binding was
    //  X -> T1
    //  Y -> U1
    //
    // We now need to create
    //  X -> T2
    //  Y -> U2
    //
    // Grabbing the equivalent binding will accomplish this.
    //
    // However we need to be careful about the parent binding.  It needs to be built
    // for the stubClass instead of the target class
    GenericBinding *binding;
    if ( symbolRef->GenericBindingContext && stubProc->IsGeneric() )
    {
        // Get the equivalent binding and reset the parent
        binding = GetEquivalentBinding(symbolRef->GenericBindingContext);

        if ( IsGenericOrHasGenericParent(m_stubClass) )
        {
            binding->SetParentBinding(CreateSimpleTypeBinding(m_stubClass));
        }
        else
        {
            binding->SetParentBinding(NULL);
        }
    }
    else if ( IsGenericOrHasGenericParent(m_stubClass) )
    {
        // Dev10#682896: the containing class can be generic, even if the base class
        // isn't. In that case, we need to have the generic type binding to emit the
        // call to the stub, otherwise we end up generating incorrect IL with an open
        // generic type reference.
        binding = CreateSimpleTypeBinding(m_stubClass);
    }
    else
    {
        binding = NULL;
    }

    symbolRef->GenericBindingContext = binding;
}

//==============================================================================
// Create a stub method for the passed in virtual method.  This is used so that
// we can make the original MyBase/MyClass call without having to
// worry about the verification issue
//
// Don't worry about fixing up the metadata issues here.  They will be resolved
// in FixupStubGenericSignature
//==============================================================================
Procedure *MyBaseMyClassStubContext::CreateMethodStubForMyBaseMyClass(_Inout_ STRING *stubName, unsigned callMeFlags)
{
    ThrowIfNull(m_originalProc);
    ThrowIfFalse(callMeFlags & (SXF_SYM_MYBASE | SXF_SYM_MYCLASS));

    // It's very important here to use the containing method's class as the
    // stub class.  It's possible that m_originalProc is in a parent class
    // which is part of a compiled assembly.  To ensure this will work we need
    // to generate the stub in the class for the proc we are currently
    // compiling (we know we have the source for that).
    Symbols *transientSymbols = GetTransientSymbolFactory();
    Symbols *normalSymbols = GetNormalSymbolFactory();
    bool isFunction = m_originalProc->GetType() != NULL;
    Location hiddenLoc = Location::GetHiddenLocation();

    // Clone the parameters inte tho transient symobls.  Make sure to give the parameters
    // unique names since we will be binding against it later.  It is possible to produce
    // verifiable code that has parameters with the same name.
    Parameter *firstParam = NULL;
    Parameter *lastParam = NULL;
    SafeInt<unsigned> paramCount = 0;
    for ( Parameter *curParam = m_originalProc->GetFirstParam();
            curParam;
            curParam = curParam->GetNext() )
    {
        StringBuffer buf;
        buf.AppendPrintf(L"p%u", paramCount.Value());
        transientSymbols->GetParam(
                &hiddenLoc,
                GetCompiler()->AddString(&buf),
                curParam->GetType(),    // We'll remap this shortly if needed
                Symbols::CalculateInitialParamFlags(curParam),
                NULL,
                &firstParam,
                &lastParam,
                curParam->IsReturnType());
        paramCount += 1;
    }

    // Remove any virtual items from the decl flags
    DECLFLAGS declFlags = m_originalProc->GetDeclFlags();
    ClearFlag(declFlags, DECLF_MustOverride);
    ClearFlag(declFlags, DECLF_OverridesKeywordUsed);
    ClearFlag(declFlags, DECLF_Overridable);

    Procedure *stubProc = transientSymbols->AllocSyntheticMethod(false);
    transientSymbols->GetProc(
            NULL,
            stubName,
            stubName,
            (CodeBlockLocation*) NULL,
            (CodeBlockLocation*) NULL,
            declFlags,
            m_originalProc->GetType(),
            firstParam,
            NULL,
            NULL,
            NULL,
            SYNTH_TransientSymbol,
            NULL,
            NULL,
            stubProc);
    Symbols::SetParent(stubProc, m_stubClass->GetUnBindableChildrenHash());
    GetSemantics()->RegisterTransientSymbol(stubProc);

    // Update the generic signature if necessary
    if ( IsGenericOrHasGenericParent(m_originalProc) )
    {
        FixupStubGenericSignature(stubProc);
    }

    // Now we have the properly parented proc.  Generate the method stub
    Semantics *semantics = GetSemantics();
    NorlsAllocator &treeStorage = semantics->m_TreeStorage;
    BILALLOC &treeAllocator = semantics->m_TreeAllocator;

    // Create the main proc node
    ILTree::ProcedureBlock *body = &(treeAllocator.xAllocBilNode(SB_PROC)->AsProcedureBlock());
    body->Loc= hiddenLoc;
    body->pproc = stubProc;
    body->ptempmgr = new (treeStorage) TemporaryManager(
            GetCompiler(),
            semantics->m_CompilerHost,
            stubProc,
            &semantics->m_TreeStorage);
    Scope *stubLocals = NULL;
    Variable *retVariable = NULL;
    Declared::MakeLocalsFromParams(
            GetCompiler(),
            &treeStorage,
            stubProc,
            semantics->m_Errors,
            isFunction,
            &retVariable,
            &stubLocals);
    body->Locals = stubLocals;
    body->ReturnVariable = isFunction ? retVariable : NULL;
    body->m_ResumableKind = ILTree::NotResumable;
    body->m_ResumableGenericType = NULL;
    stubProc->SetBoundTree(body);

    ILTree::Statement *&main = body->Child;
    ILTree::CallExpression *call = &(treeAllocator.xAllocBilNode(SX_CALL)->AsCallExpression());

    if ( stubProc->GetType() )
    {
        // If the stubProc has a return type then we need to generate a return statement
        main = &(treeAllocator.xAllocBilNode(SL_RETURN)->AsReturnStatement());
        main->AsReturnStatement().ReturnExpression = call;
    }
    else
    {
        // Otherwise, just generate a normal plain statement
        main = &(treeAllocator.xAllocBilNode(SL_STMT)->AsStatement());
        main->AsStatementWithExpression().Operand1 = call;
    }
    main->Loc = hiddenLoc;
    main->ResumeIndex = NoResume;

    call->MeArgument = semantics->AllocateSymbolReference(
            m_stubClass->GetMe(),
            m_stubClass,
            NULL,
            hiddenLoc,
            NULL);
    call->MeArgument->uFlags = callMeFlags;

    // Create the base binding for the generic method.
    GenericTypeBinding *targetBinding;
    if ( HasFlag(callMeFlags, SXF_SYM_MYBASE) )
    {
        ClassOrRecordType *originalClass = m_originalProc->GetContainingClass();
        targetBinding = IsGenericOrHasGenericParent(originalClass)
            ? CreateTypeBinding(originalClass)
            : NULL;
    }
    else
    {
        // MyClass.
        targetBinding = IsGenericOrHasGenericParent(m_stubClass)
            ? CreateSimpleTypeBinding(m_stubClass)
            : NULL;
    }

    // Generate the GenericBinding for the original proc if it's generic.  It
    // will have identical Generic Parameters (in both order and type) as the stub
    // proc so do a simple map
    GenericBinding *originalBinding;
    if ( m_originalProc->IsGeneric() )
    {
        NorlsAllocator *normalNorls = normalSymbols->GetNorlsAllocator();
        unsigned argCount = m_originalProc->GetGenericParamCount();
        unsigned index = 0;
        BCSYM **argArray = reinterpret_cast<BCSYM**>(normalNorls->Alloc(sizeof(BCSYM*)*argCount));
        ::AppendGenericParameters(m_originalProc, argArray, index);

        originalBinding = normalSymbols->GetGenericBinding(
                false,
                stubProc,
                argArray,
                argCount,
                targetBinding);
    }
    else
    {
        originalBinding = targetBinding;
    }

    call->Left = semantics->AllocateSymbolReference(
            m_originalProc,
            m_originalProc->GetType() ? m_originalProc->GetType() : TypeHelpers::GetVoidType(),
            NULL,
            hiddenLoc,
            originalBinding);
    SetFlag32(call->Left, SXF_SYM_NONVIRT);

    // Now build up the arguments.  Because it's legal to have
    ILTree::ExpressionWithChildren *cur = call;
    for ( BCSYM_Param *curParam = stubProc->GetFirstParam(); curParam; curParam = curParam->GetNext())
    {
        // Get the variable
        Declaration *declParam = stubLocals->SimpleBind(curParam->GetName());
        ThrowIfNull(declParam);
        ThrowIfFalse(declParam->IsVariable());
        Variable *varParam = declParam->PVariable();

        // Get the binding.  Simple bindings are sufficient here as the type of
        // the variable is derived from the type of the parameter.  These were
        // fixed before we created the locals so now a simple binding is all
        // that is needed.
        GenericTypeBinding *varParamBinding =
            varParam->GetType()->IsGeneric() && varParam->GetType()->IsNamedRoot()
            ? CreateSimpleTypeBinding(varParam->GetType()->PNamedRoot())
            : NULL;

        ILTree::ExpressionWithChildren*next = &(treeAllocator.xAllocBilNode(SX_LIST)->AsExpressionWithChildren());
        next->Left = semantics->AllocateSymbolReference(
                varParam,
                varParam->GetType(),
                NULL,
                hiddenLoc,
                varParamBinding);

        // If it's a by ref, then grab the address
        if ( curParam->IsByRefKeywordUsed() )
        {
            next->Left = semantics->AllocateExpression(
                    SX_ADR,
                    varParam->GetType(),
                    next->Left,
                    hiddenLoc);
        }

        cur->Right = next;
        cur = next;
    }

    return stubProc;
}

//==============================================================================
// The original proc here was generic so go ahead and fixup that definition
// here.
//
// We only need to do this if there is a generic in the hierarchy.  For normal
// closure support we have to fixup with or without generics because several of
// the types we are creating are based off of local types.  Those types are tied
// to the tree allocator.  All of our types here are based off of the original
// stub type which is on the longer allocator.  Therefore the lifetime issue
// does not apply.
//
// However the correctness issue does apply which is why we have to fix this
// for generics.
//==============================================================================
void MyBaseMyClassStubContext::FixupStubGenericSignature(Procedure *stubProc)
{
    ThrowIfNull(stubProc);
    ThrowIfFalse(IsGenericOrHasGenericParent(m_originalProc));

    Symbols *transientSymbols = GetTransientSymbolFactory();

    // Update the parameters if the proc itself is generic
    if ( m_originalProc->IsGeneric() )
    {
        ThrowIfNull(m_firstGenericParam);
        transientSymbols->SetGenericParams(m_firstGenericParam, stubProc);
    }

    // Fixup the parameter types
    Parameter *param = stubProc->GetFirstParam();
    while ( param )
    {
        BCSYM *newType = GetEquivalentTypeForMetaData(param->GetType());
        param->SetType(newType);
        param = param->GetNext();
    }

    // Fixup the proc type
    if ( stubProc->GetType() )
    {
        BCSYM *newType = GetEquivalentTypeForMetaData(stubProc->GetType());
        stubProc->SetType(newType);
    }
}

//==============================================================================
// End MyBaseMyClassStubContext
//==============================================================================

//==============================================================================
// Start ClosureGenericVisitor
//==============================================================================

ClosureGenericVisitor::ClosureGenericVisitor(ClosureRoot *root, ClosureBase *closure) :
    m_root(root),
    m_closure(closure)
{
    ThrowIfNull(root);
    ThrowIfNull(closure);
}

//==============================================================================
// At the point this code is run the SX_LAMBDA nodes are still in the bound
// tree.  When a lambda is converted into a sub proc the closure owning that
// sub proc is responsible for converting it's BILTREE.  In the case
// of expression trees though, they are not converted into sub procs
// and we must descend.
//
// protected:
//==============================================================================
bool ClosureGenericVisitor::StartExpression(ILTree::Expression **ppExpr)
{
    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;

    switch ( expr->bilop )
    {
        case SX_LAMBDA:
            {
                LambdaData *data;
                if ( m_root->m_lambdaToDataMap.GetValue(ppExpr, &data)
                    && LambdaType_ExpressionTree != data->GetLambdaType())
                {
                    return false;
                }
            }
            break;
    }

    // Not a lambda we need to skip so call the base routine
    return BoundTreeTypeVisitor::StartExpression(ppExpr);
}

//==============================================================================
// Update the encountered type.  The GenericContext code is responsible for
// descending into the types so don't let the base recursively analyze this type
//==============================================================================    
bool ClosureGenericVisitor::StartType(BCSYM **type)
{
    *type = m_closure->GetEquivalentType(*type);
    return false;
}

//==============================================================================
// Update the encountered binding.  The GenericContext code is responsible for
// descending into the binding so don't let the base recursively analyze the binding
//==============================================================================    
bool ClosureGenericVisitor::StartGenericBinding(GenericBinding **ppBinding)
{
    *ppBinding = m_closure->GetEquivalentBinding(*ppBinding);
    return false;
}

bool ClosureGenericVisitor::StartGenericTypeBinding(GenericTypeBinding **ppBinding)
{
    *ppBinding = m_closure->GetEquivalentTypeBinding(*ppBinding);
    return false;
}

//==============================================================================
// Convert the parameters of the procedure and then truly iterate
//
// public:
//==============================================================================
void ClosureGenericVisitor::Visit(Procedure *proc, ILTree::ProcedureBlock *body)
{
    ThrowIfNull(proc);
    ThrowIfNull(body);

    // Do not update the metadata of the proc here.  That operation is taken care
    // of in ClosureRoot::FixupLambdaReferences.  We need to do it there because the
    // metadata needs to be fixed up regardless of whether or not the method/class
    // is generic
    BoundTreeVisitor::Visit(body);
}

//==============================================================================
// End ClosureGenericVisitor
//==============================================================================

//==============================================================================
// Start ClosureGototerator
//==============================================================================

ClosureGotoVisitor::ClosureGotoVisitor(const ClosureRoot *root) :
    m_root(root),
    m_norls(root->GetNorlsAllocator()),
    m_labelMap(m_norls),
    m_gotoList(m_norls),
    m_hierarchyMap(m_norls),
    m_realLambdaMap(m_norls),
    m_hasClosureInScopeMap(m_norls)
{

}

ClosureGotoVisitor::~ClosureGotoVisitor()
{
    // Nothing to do.
}

void ClosureGotoVisitor::Visit(Procedure *proc, ILTree::ProcedureBlock *body)
{
    ThrowIfNull(proc);
    ThrowIfNull(body);
    ThrowIfTrue(m_realLambdaMap.Count() > 0);   // This is a once use iterator

    // Run through the tree.  Unlike the generic iterator there is no need
    // to stop descending on SX_LAMBDA's because they have been removed
    // by this phase
    BoundTreeVisitor::Visit(body);

    // To properly deterine if a goto is legal or not we need to know
    // in which blocks closures are defined.  'Goto' checking is the
    // very last item done in Closures so by this time it's possible for
    // Closures to be optimized away and hence no longer associated with
    // their block.
    //
    // However the determination is not done on the final state of the
    // closure but rather what the user typed.  Lambda's while possible
    // optimized out of a closure are not thrown away and we know the
    // original block they are declared in.  Create a new mapping that
    // will be consistent with what the user typed
    //
    // Build up a second map to cache checks and prevent redundant tree
    // walks.  Walking back up the tree is expensive and hurts
    // performance scenarios
    ClosureRoot::RealLambdaDataConstIterator lambdaIt = m_root->GetRealLambdaDataIterator();
    while (lambdaIt.MoveNext())
    {
        LambdaData *data = lambdaIt.Current();
        ThrowIfTrue(LambdaType_ExpressionTree == data->GetLambdaType());

        // If there is no lifted variables then we don't need to associate the closure to the block, therefore 
        // this block can be a destination of GoTo.
        // This is added for fixing bug 662416
        if (data->GetClosure()->GetGenericContextKind() != OptimizedClosureContext)
        {
            m_realLambdaMap.SetValue(data->GetTrackedExpression()->GetBlock(), data);
            m_hasClosureInScopeMap.SetValue(data->GetTrackedExpression()->GetBlock(), true);
        }
    }

    // No go through and shim the legal gotos and report the illegal ones
    ListValueIterator<ILTree::GotoStatement*,NorlsAllocWrapper> it(&m_gotoList);
    while ( it.MoveNext() )
    {
        ILTree::GotoStatement *gotoStmt = it.Current();
        ThrowIfNull(gotoStmt->Parent);

        // Find the block for the label statement
        ILTree::LabelStatement *labelStmt;
        if ( !m_labelMap.GetValue(gotoStmt->Label, &labelStmt))
        {
            AssertIfFalse(false);
            continue;
        }

        // If we don't have a closure associated in some way
        // with this label then the goto is fine
        bool foundClosure = IsClosureInScope(labelStmt->Parent);
        if ( !foundClosure)
        {
            continue;
        }

        // See if this is a legal goto reference.  To be legal
        // the label must essentially be in scope at the location
        // of the goto
        bool isLegal = gotoStmt->Parent == labelStmt->Parent ?
            true :
            IsDescendantOf(gotoStmt->Parent, labelStmt->Parent);

        if ( !isLegal)
        {
            // If it's not found then it's an error
            Semantics &semantics = *m_root->GetSemantics();
            ErrorTable *errorTable = semantics.m_Errors;
            errorTable->CreateError(
                    ERRID_CannotGotoNonScopeBlocksWithClosure,
                    &gotoStmt->Loc,
                    labelStmt->Label->Name);
        }
    }
}

bool ClosureGotoVisitor::IsClosureInScope(ILTree::ExecutableBlock *block)
{
    // We only care about labels where there is closure
    // assocatied with the label block or some parent block.
    // Use the map if the value is already cached

    if ( m_realLambdaMap.Contains(block) )
    {
        return true;
    }

    bool foundClosure = false;
    if ( !m_hasClosureInScopeMap.GetValue(block, &foundClosure) )
    {
        if ( block->Parent )
        {
            foundClosure = IsClosureInScope(block->Parent);
        }
        else
        {
            foundClosure = false;
        }

        m_hasClosureInScopeMap.SetValue(block, foundClosure);
        return foundClosure;
    }

    return foundClosure;
}

bool ClosureGotoVisitor::IsDescendantOf(ILTree::ExecutableBlock *child, ILTree::ExecutableBlock *possibleParent)
{
    if ( !child->Parent )
    {
        return false;
    }

    if ( child->Parent == possibleParent )
    {
        return true;
    }

    DynamicHashTable<ILTree::ExecutableBlock*,bool,NorlsAllocWrapper> *childMap = NULL;
    if ( !m_hierarchyMap.GetValue(possibleParent, &childMap) )
    {
        childMap = new (*m_norls)DynamicHashTable<ILTree::ExecutableBlock*,bool,NorlsAllocWrapper>(m_norls);
        m_hierarchyMap.SetValue(possibleParent, childMap);
    }

    bool value = false;
    if ( childMap->GetValue(child, &value) )
    {
        return value;
    }
    else
    {
        value = IsDescendantOf(child->Parent, possibleParent);
        childMap->SetValue(child, value);
        return value;
    }
}

bool ClosureGotoVisitor::StartStatement(ILTree::Statement *stmt)
{
    if (IsBad(stmt))
    {
        return false;
    }
    
    switch ( stmt->bilop )
    {
    case SL_LABEL:
        {
            ILTree::LabelStatement *label = &(stmt->AsLabelStatement());
            m_labelMap.SetValue(label->Label, label);
        }
        break;
    case SL_GOTO:
        m_gotoList.AddLast(&(stmt->AsGotoStatement()));
        break;
    case SL_RESUME:
    case SL_ON_ERR:
        {
            // Make sure this is an On Error Goto ... statement.
            // On Error Resume Next will present with a NULL label statement
            ILTree::OnErrorStatement &errStmt = stmt->AsOnErrorStatement();
            if ( errStmt.Label )
            {
                m_root->GetSemantics()->ReportSemanticError(ERRID_CannotUseOnErrorGotoWithClosure, stmt->Loc);
            }
        }
        break;
    }

    return true;
}

//==============================================================================
// End ClosureGototerator
//==============================================================================

//==============================================================================
// Start ClosureTemporaryterator
//==============================================================================
ClosureShortLivedTemporaryVisitor::ClosureShortLivedTemporaryVisitor(ClosureRoot *root)
    : m_root(root),
    m_norls(root->GetNorlsAllocator()),
    m_statementStack(m_norls),
    m_groupMap(m_norls),
    m_normalMap(m_norls)
{
    ClosureRoot::ShortLivedTemporaryDataIterator shortIt = root->GetShortLivedTemporaryDataIterator();
    NorlsAllocator *norls = m_root->GetNorlsAllocator();
    while ( shortIt.MoveNext() )
    {
        ShortLivedTemporaryData *data = shortIt.Current();
        Temporary *temporary = data->GetTemporaryInfo();
        TemporaryManager *mgr = temporary->Symbol->GetTemporaryManager();
        Variable *tempVar = temporary->Symbol;

        // It's possible that the only use of the ShortLived temporary is the lift statement.
        // In that case we should remove it at the end of the iterator from the temporary
        // manager.  Use a simple map to detect whether or not it was used.
        m_normalMap.SetValue(tempVar, false);

        // When creating the new ShortLived temporaries we don't want the old ones
        // to be re-used (hence the whole point if this iterator :) ) so make sure
        // these temporaries are marked as being in use
        temporary->IsCurrentlyInUse = true;

        // Populate the set with all of the statements that lift variables
        ListValueIterator<ILTree::Statement*,NorlsAllocWrapper> stmtIt = data->GetStatementIterator();
        while (stmtIt.MoveNext() )
        {
            ILTree::Statement *stmt = stmtIt.Current();
            GroupData *groupData = NULL;
            if ( !m_groupMap.GetValue(stmt->GroupId, &groupData) )
            {
                groupData = new (*m_norls) GroupData(m_norls);
                m_groupMap.SetValue(stmt->GroupId, groupData);
            }

            // Make sure we have a mapping for this variable
            if ( !groupData->VariableMap.Contains(tempVar))
            {
                Location loc = tempVar->HasLocation() ? *tempVar->GetLocation() : Location::GetHiddenLocation();

                // Port SP1 CL 2955440 to VS10
                // Bug #167074 - DevDiv Bugs: Make sure we do not reuse any existing temporary
                Variable *newTmp = mgr->AllocateShortLivedTemporaryNoReuse(
                        tempVar->GetType(),
                        &loc);

                // The owning closure for the ShortLived temporary is parent of the statement where it's
                // lifted.  By now this will be a hidden block
                // Dev10#677779: note that lifting is done on a "per-group" basis, i.e. on the parent
                // of the first statement in the current group
                ILTree::Statement *groupStmt = stmt->GetFirstStatementInGroup();
                ILTree::ExecutableBlock *parent = groupStmt->Parent;
                ThrowIfFalse(parent!=NULL && SB_HIDDEN_CODE_BLOCK == parent->bilop);
                Closure *closure = m_root->GetClosure(parent);
                ThrowIfNull(closure);
                closure->EnsureVariableLifted(newTmp);  // The closure now lifts this variable
                groupData->VariableMap.SetValue(tempVar, newTmp);
            }
        }
    }
}

ClosureShortLivedTemporaryVisitor::~ClosureShortLivedTemporaryVisitor()
{
    // Nothing to free up since everything is allocated on the NorlsAllocator
}

void ClosureShortLivedTemporaryVisitor::Visit()
{
    BoundTreeVisitor::Visit(m_root->m_procBody);

    // Check for Temporaries which were lifted but not used anywhere else.  These
    // are no longer needed and should be removed
    HashTableIterator<Variable*,bool,NorlsAllocWrapper> it(&m_normalMap);
    while ( it.MoveNext() )
    {
        if ( !it.Current().Value())
        {
            // The temporary wasn't used outside of the lift condition so
            // remove it from the temporary manager
            Variable *ptemp = it.Current().Key();
            ptemp->GetTemporaryManager()->FreeTemporary(ptemp);
        }
    }

}

bool ClosureShortLivedTemporaryVisitor::StartStatement(ILTree::Statement *stmt)
{
    if (IsBad(stmt))
    {
        return false;
    }

    m_statementStack.PushOrEnqueue(stmt);
    return true;
}

void ClosureShortLivedTemporaryVisitor::EndStatement(ILTree::Statement *stmt)
{
    m_statementStack.PopOrDequeue();
}

bool ClosureShortLivedTemporaryVisitor::StartExpression(ILTree::Expression **ppExpr)
{
    if (IsBad(*ppExpr))
    {
        return false;
    }
        
    ILTree::Expression *expr = *ppExpr;
    if ( SX_SYM == expr->bilop )
    {
        Variable *tempVar = GetVariableFromExpression(expr);
        if ( tempVar && tempVar->IsTemporary() )
        {
            // Alright, we've found a temporary variable.  Firstly see if we're in a
            // statement that lifts a ShortLived temporary variable.
            bool isTempLifted = false;

            // Dev10#677779: owning statement might be anywhere in the statement stack
            ILTree::Statement *stmt = (tempVar->GetTemporaryInfo()->Lifetime == LifetimeShortLived)
                                      ? m_root->FindOwningStatementOfShortLivedTemporary(m_statementStack, tempVar)
                                      : m_statementStack.Peek();
            ThrowIfNull(stmt);

            // Question: what happens in this loop when we come to a temporary
            // with LifeTimeLongLived or LifeTimeLong? ...
            // I don't know! It appears not to enter any of the following if blocks.
            // And everything seems to be working okay.

            GroupData *groupData = NULL;
            if ( m_groupMap.GetValue(stmt->GroupId, &groupData) )
            {
                // See if this temporary is lifted
                Variable *newTemp = NULL;
                if ( groupData->VariableMap.GetValue(tempVar, &newTemp) )
                {
                    expr->AsSymbolReferenceExpression().Symbol = newTemp;
                    isTempLifted = true;
                }
            }

            // Now if this temporary was not lifted make sure to update the usage map so
            // we know it was used outside of the initial lift
            if ( !isTempLifted && m_normalMap.Contains(tempVar) )
            {
                m_normalMap.SetValue(tempVar, true);
            }
        }
    }

    return true;
}

//==============================================================================
// End ClosureTemporaryterator
//==============================================================================
#ifdef DEBUG

//==============================================================================
// Start ClosureDump
//==============================================================================

void ClosureDump::DumpClosures(ClosureRoot *root)
{
    ThrowIfNull(root);

    PrintAndIndent(L"=========================================================================\n");
    PrintAndIndent(L"Begin Closure Dump\n");
    PrintAndIndent(L"=========================================================================\n");

    ClassOrRecordType *type = root->m_proc->GetContainingClass();
    PrintAndIndent(L"Class: ");
    PrintTypeName(type);
    Print(L"\n");

    PrintAndIndent(L"Procedure: ");
    PrintSignature(root->m_proc);
    Print(L"\n");

    IncreaseIndent();
    ListValueIterator<Closure*,NorlsAllocWrapper> it(&root->m_rootClosureList);
    while ( it.MoveNext() )
    {
        Closure *cur = it.Current();
        DumpClosure(cur);
    }

    // Dump the optimized closures
    if ( root->m_optClosureList.Count() > 0 )
    {
        PrintAndIndent(L"Optimized Closure\n");
        IncreaseIndent();
        ListValueIterator<OptimizedClosure*,NorlsAllocWrapper> optIt(&root->m_optClosureList);
        while ( optIt.MoveNext() )
        {
            DumpLambda(optIt.Current()->m_lambdaData);
        }
        DecreaseIndent();
    }

    DecreaseIndent();

    PrintAndIndent(L"=========================================================================\n");
    PrintAndIndent(L"End Closure Dump\n");
    PrintAndIndent(L"=========================================================================\n");
}

void ClosureDump::DumpClosure(Closure *closure)
{
    ThrowIfNull(closure);

    // Print the closure name
    ClassOrRecordType *type = closure->m_closureClass;
    PrintAndIndent(L"Closure: ");
    PrintTypeName(type);
    Print(L"\n");

    IncreaseIndent();

    // Print out the address (used as an id)
    PrintAndIndent(L"Address: 0x%x\n", closure);

    // Print out the containing proc info
    PrintAndIndent(L"Container: ");
    PrintTypeName(closure->m_containingProc->GetContainingClass());
    Print(L"::");
    PrintSignature(closure->m_containingProc);
    Print(L"\n");

    // Print out the information for the associated block
    PrintAndIndent(L"ILTree::ExecutableBlock: ");
    switch ( closure->m_block->bilop )
    {
        case SB_PROC:
            Print(L"SB_PROC");
            break;
        case SB_IF_BLOCK:
            Print(L"SB_IF_BLOCK");
            break;
        case SB_IF:
            Print(L"SB_IF");
            break;
        case SB_ELSE_IF:
            Print(L"SB_ELSE_IF");
            break;
        case SB_ELSE:
            Print(L"SB_ELSE");
            break;
        case SB_SELECT:
            Print(L"SB_SELECT");
            break;
        case SB_CASE:
            Print(L"SB_CASE");
            break;
        case SB_TRY:
            Print(L"SB_TRY");
            break;
        case SB_CATCH:
            Print(L"SB_CATCH");
            break;
        case SB_FINALLY:
            Print(L"SB_FINALLY");
            break;
        case SB_FOR:
            Print(L"SB_FOR");
            break;
        case SB_FOR_EACH:
            Print(L"SB_FOR_EACH");
            break;
        case SB_DO:
            Print(L"SB_DO");
            break;
        case SB_SYNCLOCK:
            Print(L"SB_SYNCLOCK");
            break;
        case SB_USING:
            Print(L"SB_USING");
            break;
        case SB_LOOP:
            Print(L"SB_LOOP");
            break;
        case SB_WITH:
            Print(L"SB_WITH");
            break;
        case SB_HIDDEN_CODE_BLOCK:
            Print(L"SB_HIDDEN_CODE_BLOCK");
            break;
        case SB_STATEMENT_LAMBDA:
            Print(L"SB_STATEMENT_LAMBDA");
            break;
        default:
            Print(L"!!!Unknown block!!!");
            break;
    }
    Print(L" ");
    PrintLocation(closure->m_block->Loc);
    Print(L"\n");

    DumpGenericMap(closure);
    DumpFields(closure);
    DumpLambdas(closure);
    DumpChildren(closure);
    DecreaseIndent();
}

void ClosureDump::DumpChildren(Closure *closure)
{
    ThrowIfNull(closure);
    if ( 0 == closure->m_childList.Count() )
    {
        return;
    }

    PrintAndIndent(L"Children\n");

    IncreaseIndent();
    ListValueIterator<Closure*,NorlsAllocWrapper> it(&closure->m_childList);
    while (it.MoveNext())
    {
        DumpClosure(it.Current());
    }

    DecreaseIndent();
}

void ClosureDump::DumpFields(Closure *closure)
{
    ThrowIfNull(closure);
    if ( 0 == closure->m_liftedToFieldMap.Count() )
    {
        return;
    }

    PrintAndIndent(L"Fields\n");
    IncreaseIndent();

    HashTableIterator<Variable*,Variable*,NorlsAllocWrapper> it(&closure->m_liftedToFieldMap);
    while ( it.MoveNext() )
    {
        Variable *oldVar = it.Current().Key();
        Variable *newVar = it.Current().Value();

        PrintAndIndent(L"");
        PrintTypeName(oldVar->GetType());
        Print(L" %s -> ", oldVar->GetName());
        PrintTypeName(newVar->GetType());
        Print(L" %s\n", newVar->GetName());
    }

    DecreaseIndent();
}

void ClosureDump::DumpLambdas(Closure *closure)
{
    ThrowIfNull(closure);
    if ( 0 == closure->m_procList.Count() )
    {
        return;
    }

    PrintAndIndent(L"Lambdas\n");
    IncreaseIndent();

    ListValueIterator<Procedure*,NorlsAllocWrapper> it(&closure->m_procList);
    while ( it.MoveNext() )
    {
        LambdaData *data = closure->m_root->m_realLambdaMap.GetValue(it.Current());
        DumpLambda(data);
    }

    DecreaseIndent();
}

void ClosureDump::DumpGenericMap(Closure *closure)
{
    ThrowIfNull(closure);
    if ( 0 == closure->m_genericMap.Count() )
    {
        return;
    }
    PrintAndIndent(L"Generic Map\n");

    IncreaseIndent();
    HashTableIterator<GenericParameter*,Type*,NorlsAllocWrapper> it(&closure->m_genericMap);
    while ( it.MoveNext())
    {
        PrintAndIndent(L"%s->", it.Current().Key()->GetName());
        PrintTypeName(it.Current().Value());
        Print(L"\n");
    }
    DecreaseIndent();
}

void ClosureDump::DumpLambda(LambdaData* lambdaData)
{
    ThrowIfNull(lambdaData);
    switch ( lambdaData->GetLambdaType() )
    {
        case LambdaType_Optimized:
        case LambdaType_Normal:
            ThrowIfNull(lambdaData->GetProcedure());
            PrintAndIndent(L"Lambda: %s\n", lambdaData->GetProcedure()->GetName());
            break;
        case LambdaType_Unknown:
            PrintAndIndent(L"Lambda: <unknown>\n");
            break;
        case LambdaType_ExpressionTree:
            PrintAndIndent(L"Lambda: Expression ILTree::ILNode\n");
            break;
    }
    IncreaseIndent();

    // Spit out the signature
    PrintAndIndent(L"Signature: ");
    PrintSignature(lambdaData->GetProcedure());
    Print(L"\n");

    // Address Information
    PrintAndIndent(L"Address: 0x%x\n", lambdaData);

    // Spit out the owning closure
    ClosureBase *owner = lambdaData->GetClosure();
    PrintAndIndent(L"Closure Address: 0x%x\n", owner);

    // Spit out the containing proc
    TrackedExpression *tracked = lambdaData->GetTrackedExpression();
    Procedure *containingProc = tracked->GetReferencingProcedure();
    PrintAndIndent(L"Container: %s\n", containingProc->GetName());

    DecreaseIndent();
}

void ClosureDump::PrintLocation(Location loc)
{
    if ( loc.IsHidden() )
    {
        Print(L"Hidden");
    }
    else if ( loc.IsInvalid() )
    {
        Print(L"Invalid");
    }
    else
    {
        Print(L"%d:%d %d:%d (Line/Col)",
                loc.m_lBegLine,
                loc.m_lBegColumn,
                loc.m_lEndLine,
                loc.m_lEndColumn);
    }
}

void ClosureDump::PrintTypeName(BCSYM *type)
{
    ThrowIfNull(type);

    if ( type->IsGenericParam())
    {
        GenericParameter *gParam = type->PGenericParam();
        if ( gParam->IsGenericMethodParam() )
        {
            Print(L"!!");
        }
        else
        {
            Print(L"!");
        }

        Print(L"%s", type->PGenericParam()->GetName());
    }
    else if ( type->IsGenericTypeBinding() )
    {
        GenericTypeBinding *binding = type->PGenericTypeBinding();
        Print(L"%s<", binding->GetGeneric()->GetName());

        BCSYM **args = binding->GetArguments();
        for ( unsigned i = 0; i < binding->GetArgumentCount(); ++i )
        {
            PrintTypeName(args[i]);
            if ( i + 1 < binding->GetArgumentCount() )
            {
                Print(L",");
            }
        }
        Print(L">");
    }
    else if ( type->IsPointerType() )
    {
        Print(L"*");
        PrintTypeName(type->PPointerType()->GetRoot());
    }
    else if ( type->IsClass() )
    {
        ClassOrRecordType *cType = type->PClass();
        Print(L"%s", cType->GetName());

        if ( cType->IsGeneric() )
        {
            Print(L"<");
            GenericParameter *param = cType->GetFirstGenericParam();
            while ( param)
            {
                Print(L"%s", param->GetName());
                param = param->GetNextParam();
                if ( param )
                {
                    Print(L",");
                }
            }
            Print(L">");
        }
    }
    else if ( type->IsNamedRoot() )
    {
        Print(L"%s", type->PNamedRoot()->GetName());
    }
    else if ( type->IsNamedType() )
    {
        PrintTypeName(type->PNamedType()->GetSymbol());
    }
    else if ( type->IsSimpleType() )
    {
        BCSYM_SimpleType *simple = type->PSimpleType();
        switch ( simple->GetVtype() )
        {
            case t_bool:
                Print(L"Boolean");
                break;
            case t_i1:
                Print(L"SByte");
                break;
            case t_ui1:
                Print(L"Byte");
                break;
            case t_i2:
                Print(L"Int16");
                break;
            case t_ui2:
                Print(L"UInt16");
                break;
            case t_i4:
                Print(L"Int32");
                break;
            case t_ui4:
                Print(L"UInt32");
                break;
            case t_i8:
                Print(L"UInt64");
                break;
            case t_ui8:
                Print(L"UInt64");
                break;
            case t_decimal:
                Print(L"Decimal");
                break;
            case t_single:
                Print(L"Single");
                break;
            case t_double:
                Print(L"Double");
                break;
            case t_date:
                Print(L"DateTime");
                break;
            case t_char:
                Print(L"Char");
                break;
            case t_string:
                Print(L"String");
                break;
            case t_UNDEF:
                Print(L"!UNDEF!");
                break;
            case t_array:
                PrintTypeName(simple->PArrayType()->GetRoot());
                Print(L"()");
                break;
            default:
                Print(L"<Unknown Simple>");
                break;
        }
    }
    else
    {
        Print(L"<Unknown Type>");
    }
}

void ClosureDump::PrintSignature(Procedure *proc)
{
    ThrowIfNull(proc);

    if ( proc->GetType() )
    {
        PrintTypeName(proc->GetType());
        Print(L" ");
    }
    else
    {
        Print(L"void ");
    }

    Print(L"%s", proc->GetName());
    if ( proc->IsGeneric() )
    {
        Print(L"<");
        GenericParameter *gParam = proc->GetFirstGenericParam();
        while ( gParam )
        {
            Print(L"%s", gParam->GetName());
            gParam = gParam->GetNextParam();
            if ( gParam )
            {
                Print(L",");
            }
        }

        Print(L">");
    }

    Print(L"(");
    Parameter *param = proc->GetFirstParam();
    while ( param )
    {
        PrintTypeName(param->GetType());
        Print(L" %s", param->GetName());
        param = param->GetNext();
        if ( param )
        {
            Print(L",");
        }
    }
    Print(L")");
}

void ClosureDump::Print(const WCHAR *format, ...)
{
    va_list argList;
    va_start(argList, format);
    DebVPrintf(format, argList);
    va_end(argList);
}

void ClosureDump::PrintAndIndent(const WCHAR *format, ...)
{
    for ( int i = 0; i < m_indent; ++i)
    {
        DebPrintf(L"  ");
    }

    va_list argList;
    va_start(argList, format);
    DebVPrintf(format, argList);
    va_end(argList);
}


//==============================================================================
// End ClosureDump
//==============================================================================

#endif //DEBUG
