//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Allocator functions for Bound Trees.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// Flags used while copying BILTREE's
//
#define BCF_NONE                0x0000
#define BCF_IN_LAMBDA           0x0001
#define BCF_SHALLOW_COPY_LAMBDA 0x0002
#define BCF_SCRATCH             0x0004
#define BCF_BETA2_SPECIAL_CASE  0x0008




void BILALLOC::DeepCloneParameters(ILTree::LambdaExpression *lambdaExpr)
{
    Symbols symbols(m_pCompiler, m_pnraTree, NULL);
    BCSYM_Param *param = Declared::CloneParameters(lambdaExpr->FirstParameter, false, NULL, &symbols);
    lambdaExpr->FirstParameter = param;
}

// Returns pointer to the old TemporaryManager
TemporaryManager * BILALLOC::DeepCloneTemporaries(ILTree::LambdaExpression *lambdaExpr)
{
    // Don't bother if there is not a temporary manager
    if ( !lambdaExpr->TemporaryManager)
    {
        return NULL;
    }

    DynamicHashTable<Temporary*,Temporary*> copyMap;
    TemporaryManager *oldMgr = lambdaExpr->TemporaryManager;
    TemporaryManager *newMgr = new (*m_pnraTree)TemporaryManager(oldMgr, &copyMap);

    HashTableIterator<Temporary*,Temporary*, VBAllocWrapper> it(&copyMap);
    while ( it.MoveNext() )
    {
        MapVariable(it.Current().Key()->Symbol, it.Current().Value()->Symbol, m_varMap);
    }

    // Set the manager field
    lambdaExpr->TemporaryManager = newMgr;

    return oldMgr;
}

#if DEBUG
static void CallConstructorForDebug(ILTree::PILNode ptree)
{
	void *pvoid = ptree;
    switch(ptree->bilop)
    {
    case SX_NAME:
        new (pvoid) struct ILTree::ArgumentNameExpression;
        break;

    case SX_ARG:
        new (pvoid) struct ILTree::ArgumentExpression;
        break;

    case SB_PROC:
        new (pvoid) struct ILTree::ProcedureBlock;
        break;

    case SB_IF_BLOCK:
        new (pvoid) struct ILTree::IfGroup;
        break;

    case SB_IF:
    case SB_ELSE_IF:
    case SB_ELSE:
        new (pvoid) struct ILTree::IfBlock;
        break;

    case SB_SELECT:
        new (pvoid) struct ILTree::SelectBlock;
        break;

    case SB_CASE:
        new (pvoid) struct ILTree::CaseBlock;
        break;

    case SB_TRY:
        new (pvoid) struct ILTree::TryBlock;
        break;

    case SB_CATCH:
        new (pvoid) struct ILTree::CatchBlock;
        break;

    case SB_FINALLY:
        new (pvoid) struct ILTree::FinallyBlock;
        break;

    case SB_FOR:
        new (pvoid) struct ILTree::ForBlock;
        break;

    case SB_FOR_EACH:
        new (pvoid) struct ILTree::ForEachBlock;
        break;

    case SB_DO:
    case SB_LOOP:
        new (pvoid) struct ILTree::LoopBlock;
        break;

    case SB_SYNCLOCK:
    case SB_USING:
    case SB_HIDDEN_CODE_BLOCK:
        new (pvoid) struct ILTree::ExecutableBlock;
        break;

    case SB_STATEMENT_LAMBDA:
        new (pvoid) struct ILTree::StatementLambdaBlock;
        break;

    case SB_WITH:
        new (pvoid) struct ILTree::WithBlock;
        break;

    case SL_LABEL:
        new (pvoid) struct ILTree::LabelStatement;
        break;

    case SL_STOP:
    case SL_END:
    case SL_DEBUGLOCNOP:
        new (pvoid) struct ILTree::Statement;
        break;

    case SL_RESUME:
        new (pvoid) struct ILTree::OnErrorStatement;
        break;

    case SL_GOTO:
        new (pvoid) struct ILTree::GotoStatement;
        break;

    case SL_ASYNCSWITCH:
        new (ptree) struct ILTree::AsyncSwitchStatement;
        break;

    case SL_EXIT:
        new (pvoid) struct ILTree::ExitStatement;
        break;

    case SL_RETURN:
        new (pvoid) struct ILTree::ReturnStatement;
        break;

    case SL_YIELD:
        new (pvoid) struct ILTree::YieldStatement;
        break;

    case SL_CONTINUE:
        new (pvoid) struct ILTree::ContinueStatement;
        break;

    case SL_ON_ERR:
        new (pvoid) struct ILTree::OnErrorStatement;
        break;

    case SL_REDIM:
        new (pvoid) struct ILTree::RedimStatement;
        break;

    case SL_STMT:
    case SL_ERROR:
    case SL_VAR_DECL:
        new (pvoid) struct ILTree::StatementWithExpression;
        break;

    case SX_CNS_INT:
        new (pvoid) struct ILTree::IntegralConstantExpression;
        break;

    case SX_CNS_DEC:
        new (pvoid) struct ILTree::DecimalConstantExpression;
        break;

    case SX_CNS_FLT:
        new (pvoid) struct ILTree::FloatConstantExpression;
        break;

    case SX_CNS_STR:
        new (pvoid) struct ILTree::StringConstantExpression;
        break;

    case SX_NEW:
        new (pvoid) struct ILTree::NewExpression;
        break;

    case SX_INIT_STRUCTURE:
        new (pvoid) struct ILTree::InitStructureExpression;
        break;

    case SX_OVERLOADED_GENERIC:
        new (pvoid) struct ILTree::OverloadedGenericExpression;
        break;

    case SX_LAMBDA:
        new (pvoid) struct ILTree::LambdaExpression;
        break;

    case SX_UNBOUND_LAMBDA:
        new (pvoid) struct ILTree::UnboundLambdaExpression;
        break;

    case SX_NOTHING:
        new (pvoid) struct ILTree::Expression;
        break;

    case SX_BOGUS:
        new (pvoid) struct ILTree::Expression;
        break;

    case SX_INDEX:
        new (pvoid) struct ILTree::IndexExpression;
        break;

    case SX_VARINDEX:
        new (pvoid) struct ILTree::VariantIndexExpression;
        break;

    case SX_LATE:
        new (pvoid) struct ILTree::LateBoundExpression;
        break;

    case SX_SYM:
        new (pvoid) struct ILTree::SymbolReferenceExpression;
        break;

    case SX_IIF:
        new (pvoid) struct ILTree::IfExpression;
        break;

    case SX_CTYPEOP:
        new (pvoid) struct ILTree::LiftedCTypeExpression;
        break;

    case SX_CALL:
        new (pvoid) struct ILTree::CallExpression;
        break;

    case SX_DELEGATE_CTOR_CALL:
        new (pvoid) struct ILTree::DelegateConstructorCallExpression;
        break;

    case SX_SEQ:
    case SX_SEQ_OP1:
    case SX_SEQ_OP2:
    case SX_AND:
    case SX_OR:
    case SX_XOR:
    case SX_ADDRESSOF:
    case SX_ASG:
    case SX_ASG_RESADR:
    case SX_ADR:
    case SX_METATYPE:
    case SX_NEW_ARRAY:
    case SX_BAD:
    case SX_CTYPE:
    case SX_DIRECTCAST:
    case SX_TRYCAST:
    case SX_WIDE_COERCE:
    case SX_ARRAYLEN:
    case SX_NOT:
    case SX_NEG:
    case SX_PLUS:
    case SX_ADD:
    case SX_SUB:
    case SX_MUL:
    case SX_DIV:
    case SX_MOD:
    case SX_POW:
    case SX_IDIV:
    case SX_CONC:
    case SX_LIKE:
    case SX_EQ:
    case SX_NE:
    case SX_LE:
    case SX_GE:
    case SX_LT:
    case SX_GT:
    case SX_IS:
    case SX_ISNOT:
    case SX_LIST:
    case SX_ISTYPE:
    case SX_IF:
    case SX_CREATE_ARRAY:
    case SX_SYNCLOCK_CHECK:
    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
    case SX_ISTRUE:
    case SX_ISFALSE:
    case SX_IIFCoalesce:
        new (pvoid) struct ILTree::BinaryExpression;
        break;

    case SX_ORELSE:
    case SX_ANDALSO:
        if( ptree->IsUserDefinedOperator() )
        {
            new (pvoid) struct ILTree::ShortCircuitBooleanOperatorExpression;
        }
        else
        {
            new (pvoid) struct ILTree::BinaryExpression;
        }
        break;

    case SX_PROPERTY_REFERENCE:
    case SX_LATE_REFERENCE:
        new (pvoid) struct ILTree::PropertyReferenceExpression;
        break;
    case SX_APPL_ATTR:
        new (pvoid) struct ILTree::AttributeApplicationExpression;
        break;
    case SX_EXTENSION_CALL:
        new (pvoid) struct ILTree::ExtensionCallExpression;
        break;
    case SX_ANONYMOUSTYPE:
        new (pvoid) struct ILTree::AnonymousTypeExpression;
        break;
    case SX_DEFERRED_TEMP:
        new(ptree) struct ILTree::DeferredTempExpression;
        break;
    case SX_NAME_NOT_FOUND:
        // don't assert on this guy: debugger is trying to eval an expr that's invalid in this scope: see Semantics::InterpretExpression
        break;
    case SX_ARRAYLITERAL:
        new (ptree) struct ILTree::ArrayLiteralExpression;
        break;
    case SX_NESTEDARRAYLITERAL:
        new (ptree) struct ILTree::NestedArrayLiteralExpression;
        break;
    case SX_COLINIT:
        new (ptree) struct ILTree::ColInitExpression;
        break;
    case SX_COLINITELEMENT:
        new (ptree) struct ILTree::ColInitElementExpression;
        break;
    case SX_AWAIT:
        new (ptree) struct ILTree::AwaitExpression;
        break;
    case SX_ASYNCSPILL:
        new (ptree) struct ILTree::AsyncSpillExpression;
        break;
    default:
        VSFAIL("Bad Type.");
    break;
    }

}
#endif // DEBUG

/*************************************************
xAllocBilBytes -- allocs bytes in BILTREE allocator
*************************************************/
BYTE * BILALLOC::xAllocBilBytes(size_t cb)
{
    BYTE *pb = (BYTE *)m_pnraTree->Alloc(cb);
    return pb;
}

/*************************************************
xAllocBilNodeOff -- creates a new BILTREE node for a particular BILOPs
                   with an error reporting offset

NOTE: ONLY FOR SX nodes
*************************************************/
ILTree::PILNode BILALLOC::xAllocBilNodeOff
(
    BILOP    bilop,     // node to create
    ILTree::PILNode ptreeLoc   // get location info from this node
)
{
    ILTree::PILNode ptree;

    if( ptreeLoc->IsUserDefinedOperator() )
    {
        ptree = (ILTree::PILNode)xAllocUDOBilNode(bilop);
    }
    else
    {
        ptree = (ILTree::PILNode)xAllocBilNode(bilop);
    }

    // propagate location info
    ptree->PropagateLocFrom(ptreeLoc);

    return ptree;
}

/*************************************************
xAllocBilNode -- creates a new BILTREE node for a particular BILOP
                -- SIZE DEPENDS ON BILOP
*************************************************/
ILTree::PILNode BILALLOC::xAllocBilNode(BILOP bilop)
{
    ILTree::PILNode ptree = NULL;

    if (bilop >= 0 && bilop < _countof(ILTree::ILNode::s_uBiltreeAllocationSize))
    {
        // make sure the table elements do not grow out of 16 bits
        VSASSERT(ILTree::ILNode::s_uBiltreeAllocationSize[bilop] < (USHORT)0x1000, "ILTree size too high: #02/13/2004#");

        ptree = (ILTree::PILNode)xAllocBilBytes(ILTree::ILNode::s_uBiltreeAllocationSize[bilop]);
        ptree->bilop = bilop;

#if DEBUG
        CallConstructorForDebug(ptree);
#endif // DEBUG

#if DEBUG
        ptree->pchWhichStruct = ILTree::ILNode::s_szBiltreeWhichSubStruct[bilop];
#endif // DEBUG
    }

    return ptree;
}

/*************************************************
xAllocUDOBilNode -- creates a new BILTREE node for a particular BILOP, using the user defined operator column
                -- SIZE DEPENDS ON BILOP
*************************************************/
ILTree::PILNode BILALLOC::xAllocUDOBilNode(BILOP bilop)
{
    ILTree::PILNode ptree = NULL;

    if (bilop >= 0 && bilop < _countof(ILTree::ILNode::s_uBiltreeAllocationSize))
    {
        // make sure the table elements do not grow out of 16 bits
        VSASSERT(ILTree::ILNode::s_uUserDefinedOperatorBiltreeAllocationSize[bilop] < (USHORT)0x1000, "ILTree size too high: #02/13/2004#");

        ptree = (ILTree::PILNode)xAllocBilBytes(ILTree::ILNode::s_uUserDefinedOperatorBiltreeAllocationSize[bilop]);
        ptree->bilop = bilop;
        ptree->uFlags |= SXF_USERDEFINEDOPERATOR;

#if DEBUG
        CallConstructorForDebug(ptree);
#endif // DEBUG

#if DEBUG
        ptree->pchWhichStruct = ILTree::ILNode::s_szBiltreeWhichSubStruct[bilop];
#endif // DEBUG
    }

    return ptree;
}


/*********************************************************************
*
* Private Function:
*     Copy the trees to the allocated biltree.
**********************************************************************/

static void
InitializeSxTreeWithChildren
(
    ILTree::ILNode * ptree,
    ILTree::ILNode * ptreeL,
    ILTree::ILNode * ptreeR
)
{
    VSASSERT( ptree != NULL, "must have ptree!" );

    if (ptreeL)
    {
        VSASSERT(!ptreeL->IsStmtNode(), "must not be stmt node");

        ptree->uFlags |= SF_INHERIT(ptreeL->uFlags);
        ptree->AsExpressionWithChildren().Left = &ptreeL->AsExpression();
    }

    if (ptreeR)
    {
        VSASSERT(!ptreeR->IsStmtNode(), "must not be stmt node");

        ptree->uFlags |= SF_INHERIT(ptreeR->uFlags);
        ptree->AsExpressionWithChildren().Right = &ptreeR->AsExpression();
    }
}

/*********************************************************************
*
* Function:
*     CParser::xAllocSxTree
*
* Purpose:
*     Creates a Sx tree from the given left and right children and
*     does the appropriate flag propagation. Caller expected to manage
*     their own column info.
*
**********************************************************************/
ILTree::PILNode
BILALLOC::xAllocSxTree
(
    BILOP      bilop,          // [in] bil opcode
    ILTree::ILNode  * ptreeL,         // [in] left child
    ILTree::ILNode  * ptreeR         // [in] right child
)
{
    ILTree::ILNode *ptree = NULL;

    if (bilop >=0 && bilop < _countof(ILTree::ILNode::s_uBiltreeAllocationSize))
    {
        // make sure the table elements do not grow out of 16 bits
        VSASSERT(ILTree::ILNode::s_uBiltreeAllocationSize[bilop] < (USHORT)0x1000, "ILTree size too high : #02/13/2004#");

        ptree = (ILTree::PILNode)xAllocBilBytes(ILTree::ILNode::s_uBiltreeAllocationSize[bilop]);
        ptree->bilop = bilop;

#if DEBUG
        CallConstructorForDebug(ptree); /// so debugger can show correct structure
        ptree->pchWhichStruct = ILTree::ILNode::s_szBiltreeWhichSubStruct[bilop];
#endif // DEBUG

        InitializeSxTreeWithChildren( ptree, ptreeL, ptreeR );
    }

    return ptree;
}

/*********************************************************************
*
* Function:
*     CParser::xAllocUDOSxTree
*
* Purpose:
*     Creates a Sx tree from the given left and right children and
*     does the appropriate flag propagation. Caller expected to manage
*     their own column info.
*     This uses the user defined operator column.
*
**********************************************************************/
ILTree::PILNode
BILALLOC::xAllocUDOSxTree
(
    BILOP      bilop,          // [in] bil opcode
    ILTree::ILNode  * ptreeL,         // [in] left child
    ILTree::ILNode  * ptreeR         // [in] right child
)
{
    ILTree::ILNode *ptree = NULL;

    if (bilop >=0 && bilop < _countof(ILTree::ILNode::s_uBiltreeAllocationSize))
    {
        // make sure the table elements do not grow out of 16 bits
        VSASSERT(ILTree::ILNode::s_uUserDefinedOperatorBiltreeAllocationSize[bilop] < (USHORT)0x1000, "ILTree size too high : #02/13/2004#");

        ptree = (ILTree::PILNode)xAllocBilBytes(ILTree::ILNode::s_uUserDefinedOperatorBiltreeAllocationSize[bilop]);
        ptree->bilop = bilop;
        ptree->uFlags |= SXF_USERDEFINEDOPERATOR;

#if DEBUG
        CallConstructorForDebug(ptree); /// so debugger can show correct structure
        ptree->pchWhichStruct = ILTree::ILNode::s_szBiltreeWhichSubStruct[bilop];
#endif // DEBUG

        InitializeSxTreeWithChildren( ptree, ptreeL, ptreeR );
    }

    return ptree;
}

/************************
CopyBilNode
    Copy a tree node; must have no children

    Note: BCSYM_Types, BCSYMs and string constants are NOT copied;
          so, they are shared
************************/
ILTree::PILNode BILALLOC::xCopyBilNode(ILTree::PILNode ptreeIn)
{
    return xCopyBilNodeImpl(ptreeIn, BCF_NONE);
}

ILTree::PILNode BILALLOC::xCopyBilNodeForScratch(ILTree::PILNode ptree)
{
    return xCopyBilNodeImpl(ptree, BCF_SCRATCH);
}

ILTree::PILNode BILALLOC::xCopyBilNodeImpl(ILTree::PILNode ptreeIn, unsigned flags)
{
    ILTree::PILNode ptreeOut = NULL;

    if (ptreeIn->bilop >= 0 && ptreeIn->bilop < _countof(ILTree::ILNode::s_uBiltreeAllocationSize))
    {
        ptreeOut = xAllocBilNodeOff(ptreeIn->bilop, ptreeIn);

        //
        // Must do a memcpy, not a structure copy because the the node is
        // most likely NOT the size of a BILTREE
        //
        if( ptreeIn->IsUserDefinedOperator() )
        {
            memcpy(ptreeOut, ptreeIn, ILTree::ILNode::s_uUserDefinedOperatorBiltreeAllocationSize[ptreeIn->bilop]);
        }
        else
        {
            memcpy(ptreeOut, ptreeIn, ILTree::ILNode::s_uBiltreeAllocationSize[ptreeIn->bilop]);
        }

#if DEBUG

        // If the scratch flag is set, make sure we set this node as being for scratch
        if (HasFlag(flags, BCF_SCRATCH))
        {
            ptreeOut->IsScratch = true;
        }
#endif // DEBUG
    }

    return ptreeOut;
}

/************************
CopyBilTree
    Deep tree copy

    Note: BCSYM_Types, BCSYMs and string constants are NOT copied;
          so, they are shared.

    Note: Actually, we do copy some symbols. In particular we copy
          parameter symbols for unbound lambda parameters because they are
          mutated during generic type inference

************************/
ILTree::PILNode BILALLOC::xCopyBilTreeImpl(ILTree::PILNode ptreeIn, unsigned flags, ILTree::ExecutableBlock *pParent)
{
    ILTree::PILNode ptreeOut;
    UINT kind;

    VSASSERT(ptreeIn != NULL, "");

    ptreeOut = xCopyBilNodeImpl(ptreeIn, flags);

    kind = ILTree::Sxkind(ptreeOut->bilop);

    if (kind & SXK_SMPOP)
    {
        // operator: copy children
        // Dev10 #514423: special-case the SX_LIST node to avoid painfully-long recursion

        if (ptreeOut->AsExpressionWithChildren().Left != NULL)
        {
            ptreeOut->AsExpressionWithChildren().Left = &((xCopyBilTreeImpl(ptreeOut->AsExpressionWithChildren().Left, flags))->AsExpression());
        }

        ILTree::Expression ** rightNode = &(ptreeOut->AsExpressionWithChildren().Right);
        while (*rightNode != NULL && (*rightNode)->bilop == SX_LIST)
        {
            *rightNode = &((xCopyBilNodeImpl(*rightNode, flags))->AsExpression());

            if ((*rightNode)->AsExpressionWithChildren().Left != NULL)
            {
                (*rightNode)->AsExpressionWithChildren().Left = &((xCopyBilTreeImpl((*rightNode)->AsExpressionWithChildren().Left, flags))->AsExpression());
            }

            rightNode = &((*rightNode)->AsExpressionWithChildren().Right);
        }
        if (*rightNode != NULL)
        {
            *rightNode = &((xCopyBilTreeImpl(*rightNode, flags))->AsExpression());
        }
    }

    if (kind & SXK_SPECIAL)
    {
      // check special operators
        switch (ptreeOut->bilop)
        {
        case SX_SYM        :

            {
                ILTree::SymbolReferenceExpression &symRef = ptreeOut->AsSymbolReferenceExpression();

                if (symRef.ptreeQual != NULL)
                {
                    symRef.ptreeQual = xCopyBilTreeImpl(symRef.ptreeQual, flags);
                }

                // Dev10 #772215 Check if we need to remap variable reference
                if ( symRef.Symbol && symRef.Symbol->IsVariable() )
                {
                    Variable *oldVar = symRef.Symbol->PVariable();
                    Variable *newVar = NULL;
                    if ( m_varMap.GetValue(oldVar, &newVar) )
                    {
                        symRef.Symbol = newVar;
                    }
                }
            }
            break;

        case SX_AWAIT     :
            {
                ILTree::AwaitExpression &await = ptreeOut->AsAwaitExpression();
                if (await.GetAwaiterDummy != NULL) await.GetAwaiterDummy = &xCopyBilTreeImpl(await.GetAwaiterDummy, flags)->AsExpression();
                if (await.IsCompletedDummy != NULL) await.IsCompletedDummy = &xCopyBilTreeImpl(await.IsCompletedDummy, flags)->AsExpression();
                if (await.GetResultDummy != NULL) await.GetResultDummy = &xCopyBilTreeImpl(await.GetResultDummy, flags)->AsExpression();
            }
            break;

        case SX_IIF       :
            if (ptreeOut->AsIfExpression().condition != NULL)
                ptreeOut->AsIfExpression().condition = &xCopyBilTreeImpl(ptreeOut->AsIfExpression().condition, flags)->AsExpression();
            break;

        case SX_CALL       :
            if (ptreeOut->AsCallExpression().ptreeThis != NULL)
                ptreeOut->AsCallExpression().ptreeThis = xCopyBilTreeImpl(ptreeOut->AsCallExpression().ptreeThis, flags);
            break;

        case SX_NEW:
            if (ptreeOut->AsNewExpression().ptreeNew != NULL)
                ptreeOut->AsNewExpression().ptreeNew = xCopyBilTreeImpl(ptreeOut->AsNewExpression().ptreeNew, flags);
            break;

        case SX_DELEGATE_CTOR_CALL:
            if (ptreeOut->AsDelegateConstructorCallExpression().Constructor != NULL)
                ptreeOut->AsDelegateConstructorCallExpression().Constructor = xCopyBilTreeImpl(ptreeOut->AsDelegateConstructorCallExpression().Constructor, flags);

            if (ptreeOut->AsDelegateConstructorCallExpression().ObjectArgument != NULL)
                ptreeOut->AsDelegateConstructorCallExpression().ObjectArgument = xCopyBilTreeImpl(ptreeOut->AsDelegateConstructorCallExpression().ObjectArgument, flags);

            if (ptreeOut->AsDelegateConstructorCallExpression().Method != NULL)
                ptreeOut->AsDelegateConstructorCallExpression().Method = xCopyBilTreeImpl(ptreeOut->AsDelegateConstructorCallExpression().Method, flags);

            break;

        case SX_INIT_STRUCTURE:
            if (ptreeOut->AsInitStructureExpression().StructureReference != NULL)
                ptreeOut->AsInitStructureExpression().StructureReference = &xCopyBilTreeImpl(ptreeOut->AsInitStructureExpression().StructureReference,flags)->AsExpression();
            break;

        case SX_OVERLOADED_GENERIC:
            if (ptreeOut->AsOverloadedGenericExpression().BaseReference != NULL)
                ptreeOut->AsOverloadedGenericExpression().BaseReference = &xCopyBilTreeImpl(ptreeOut->AsOverloadedGenericExpression().BaseReference, flags)->AsExpression();
            break;

        case SX_LAMBDA:

            {
                ILTree::LambdaExpression & lambdaExpr = ptreeOut->AsLambdaExpression();
                
                if (lambdaExpr.GetLambdaBody() != NULL)
                {
                    BCSYM_Hash * pClonedLocalsHash = NULL;
                    TemporaryManager * pClonedTemporaryManager = NULL; 
                    DynamicHashTable<LABEL_ENTRY_BIL *, LABEL_ENTRY_BIL *> copiedLabelEntries;
                    BackupValue<DynamicHashTable<LABEL_ENTRY_BIL *, LABEL_ENTRY_BIL *>*> save_m_pCopiedLabelEntries(&m_pCopiedLabelEntries);
                    m_pCopiedLabelEntries = &copiedLabelEntries;
    
                    if (HasFlag(flags, BCF_SHALLOW_COPY_LAMBDA))
                    {
                        // Dev10 #772215 If we have at least one Long-Lived temporary in the current TemporaryManager,
                        // we need to clone it and remap all references for contained temporaries because Long-Lived
                        // temporaries are linked to blocks and we are going to copy those blocks.
                        if (lambdaExpr.TemporaryManager != NULL)
                        {
                            TemporaryIterator it;
                            it.Init(lambdaExpr.TemporaryManager);

                            Temporary *cur;
                            while (cur=it.GetNext())
                            {
                                if (cur->Lifetime == LifetimeLongLived)
                                {
                                    pClonedTemporaryManager = DeepCloneTemporaries(&lambdaExpr);
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        // We need to take additional steps to ensure that a lambda is truly deep copied.
                        DeepCloneParameters(&lambdaExpr);
                        pClonedLocalsHash = DeepCloneLambdaLocals(&lambdaExpr, m_varMap);
                        pClonedTemporaryManager = DeepCloneTemporaries(&lambdaExpr);
                    }

                    unsigned newFlags = flags | BCF_IN_LAMBDA;
                    
                    if (lambdaExpr.IsStatementLambda)
                    {
                        lambdaExpr.SetStatementLambdaBody(&xCopyBilTreeImpl(lambdaExpr.GetStatementLambdaBody(), newFlags)->AsStatementLambdaBlock() );
                        lambdaExpr.GetStatementLambdaBody()->pOwningLambdaExpression = &lambdaExpr;
                    }
                    else
                    {
                        lambdaExpr.SetExpressionLambdaBody(&xCopyBilTreeImpl(lambdaExpr.GetExpressionLambdaBody(), newFlags)->AsExpression());
                    }

                    // Dev10 #772215 Link newly created Long-Lived temporaries to the new parent blocks.
                    if (pClonedTemporaryManager != NULL)
                    {
                        AssertIfFalse(lambdaExpr.TemporaryManager != pClonedTemporaryManager);
                        TemporaryIterator it;
                        it.Init(lambdaExpr.TemporaryManager);

                        Temporary *cur;
                        while (cur=it.GetNext())
                        {
                            if (cur->Lifetime == LifetimeLongLived)
                            {
                                ILTree::PILNode pCopyOfOriginalParentBlock = NULL;

                                ThrowIfFalse(m_BlocksCopiedWithinLambda.GetValue(cur->Block, &pCopyOfOriginalParentBlock));
                                cur->Block = &pCopyOfOriginalParentBlock->AsExecutableBlock();
                            }
                        }
                    }

                    // Remove locals scoped to the original Lambda from the replacement map.
                    if (pClonedLocalsHash != NULL)
                    {
                        UnmapVariables(pClonedLocalsHash, m_varMap);
                    }

                    // Remove locals contained in the original TemporaryManager from the replacement map.
                    if (pClonedTemporaryManager != NULL)
                    {
                        UnmapTemporaries(pClonedTemporaryManager, m_varMap);
                    }

                    // Dev10 #772215 If this is the top-most Lambda, clean up the m_BlocksCopiedWithinLambda map.
                    if (!HasFlag(flags, BCF_IN_LAMBDA))
                    {
                        m_BlocksCopiedWithinLambda.Clear();
                    }

#if DEBUG
                    HashTableIterator<LABEL_ENTRY_BIL *, LABEL_ENTRY_BIL *, VBAllocWrapper> labelsIterator = copiedLabelEntries.GetIterator();

                    while (labelsIterator.MoveNext())
                    {
                        KeyValuePair<LABEL_ENTRY_BIL *, LABEL_ENTRY_BIL *> current = labelsIterator.Current();

                        if (current.Key()->pLabelParent == NULL)
                        {
                            AssertIfFalse(current.Value()->pLabelParent == NULL);
                        }
                        else
                        {
                            AssertIfFalse(current.Value()->pLabelParent != NULL);
                        }
                    }
#endif 
                }
            }
            break;
        case SX_UNBOUND_LAMBDA:
        {
            Symbols symbols(m_pCompiler, m_pnraTree,NULL);
            ptreeOut->AsUnboundLambdaExpression().FirstParameter = Declared::CloneParameters(ptreeOut->AsUnboundLambdaExpression().FirstParameter, false, NULL, &symbols);
            break;
        }

        case SX_ANONYMOUSTYPE:
            ptreeOut->AsAnonymousTypeExpression().BoundMembers = ( ILTree::BoundMemberInfoList* )xAllocBilBytes(
                sizeof( ILTree::BoundMemberInfoList ) * ptreeOut->AsAnonymousTypeExpression().BoundMembersCount );

            for( ULONG i = 0; i < ptreeOut->AsAnonymousTypeExpression().BoundMembersCount; i += 1 )
            {
                ptreeOut->AsAnonymousTypeExpression().BoundMembers[ i ].BoundExpression = &xCopyBilTreeImpl(
                    ptreeIn->AsAnonymousTypeExpression().BoundMembers[ i ].BoundExpression, flags )->AsExpression();

                if( ptreeIn->AsAnonymousTypeExpression().BoundMembers[ i ].Temp != NULL )
                {
                    ptreeOut->AsAnonymousTypeExpression().BoundMembers[ i ].Temp = &xCopyBilTreeImpl(
                        ptreeIn->AsAnonymousTypeExpression().BoundMembers[ i ].Temp, flags )->AsExpression();
                }

                ptreeOut->AsAnonymousTypeExpression().BoundMembers[ i ].Property =
                    ptreeIn->AsAnonymousTypeExpression().BoundMembers[ i ].Property;

                VSASSERT(
                    ptreeOut->AsAnonymousTypeExpression().BoundMembers[ i ].Property != NULL,
                    "Why is the property NULL?" );
                VSASSERT(
                    ptreeOut->AsAnonymousTypeExpression().BoundMembers[ i ].BoundExpression != NULL,
                    "Why is the expression NULL?" );
            }
            break;

        case SX_LATE:
        {
            ILTree::LateBoundExpression &lateExpr = ptreeOut->AsLateBoundExpression();
            if ( lateExpr.LateIdentifier )
            {
                lateExpr.LateIdentifier = &xCopyBilTreeImpl(lateExpr.LateIdentifier, flags)->AsExpression();
            }
            if ( lateExpr.AssignmentInfoArrayParam )
            {
                lateExpr.AssignmentInfoArrayParam = xCopyBilTreeImpl(lateExpr.AssignmentInfoArrayParam, flags);
            }
            if ( lateExpr.TypeArguments )
            {
                lateExpr.TypeArguments = &xCopyBilTreeImpl(lateExpr.TypeArguments, flags)->AsExpression();
            }
            break;
        }

        case SX_ARG:
        {
            ILTree::ArgumentExpression &argExpr = ptreeOut->AsArgumentExpression();
            if ( argExpr.Name )
            {
                argExpr.Name = &xCopyBilTreeImpl(argExpr.Name, flags)->AsArgumentNameExpression();
            }
            break;
        }
        case SX_EXTENSION_CALL:
        {
            ILTree::ExtensionCallExpression &extCall = ptreeOut->AsExtensionCallExpression();
            if ( extCall.ImplicitArgumentList )
            {
                extCall.ImplicitArgumentList = &xCopyBilTreeImpl(extCall.ImplicitArgumentList, flags)->AsExpressionWithChildren();
            }

            break;
        }
        case SX_ARRAYLITERAL:
        {
            ILTree::ArrayLiteralExpression & arrayLit = ptreeOut->AsArrayLiteralExpression();
            if (arrayLit.Dims)
            {
                unsigned * pDimsCopy = m_pnraTree->AllocArray<unsigned>(arrayLit.Rank);
                memcpy(pDimsCopy, arrayLit.Dims, sizeof(unsigned)*arrayLit.Rank);
                arrayLit.Dims = pDimsCopy;
            }

            if (arrayLit.ElementList)
            {
                arrayLit.ElementList = &xCopyBilTreeImpl(arrayLit.ElementList, flags)->AsExpressionWithChildren();
            }
            break;
        }
        case SX_NESTEDARRAYLITERAL:
        {
            ILTree::NestedArrayLiteralExpression & nestedLit = ptreeOut->AsNestedArrayLiteralExpression();

            if (nestedLit.ElementList)
            {
                nestedLit.ElementList = &xCopyBilTreeImpl(nestedLit.ElementList, flags)->AsExpressionWithChildren();
            }
            break;
        }
        case SX_COLINIT:
        {
            ILTree::ColInitExpression & colInit = ptreeOut->AsColInitExpression();

            if (colInit.Elements)
            {
                colInit.Elements = & xCopyBilTreeImpl(colInit.Elements, flags)->AsExpressionWithChildren();
            }

            if (colInit.NewExpression)
            {
                colInit.NewExpression = &xCopyBilTreeImpl(colInit.NewExpression, flags)->AsExpression();
            }

            if (colInit.ResultTemporary)
            {
                colInit.ResultTemporary = &xCopyBilTreeImpl(colInit.ResultTemporary, flags)->AsSymbolReferenceExpression();
            }
            break;
        }
        case SX_COLINITELEMENT:
        {
            ILTree::ColInitElementExpression & colInitElement = ptreeOut->AsColInitElementExpression();

            if (colInitElement.CallExpression)
            {
                colInitElement.CallExpression = &xCopyBilTreeImpl(colInitElement.CallExpression, flags)->AsExpression();
            }

            if (colInitElement.CopyOutArguments)
            {
                colInitElement.CopyOutArguments = &xCopyBilTreeImpl(colInitElement.CallExpression, flags)->AsExpression();
            }
            break;
        }

        case SX_ASYNCSPILL:
            VSASSERT(false, "Why are we copying SX_AsyncSpill?");
            __fallthrough;

        case SX_PROPERTY_REFERENCE:
        case SX_INDEX:
        case SX_VARINDEX:
        case SX_DEFERRED_TEMP:
        case SX_CTYPEOP:
            break;    // Nothing to do.
        default:
            ThrowIfFalse(false); // new operator? add it to the list"
        } // switch
    }

    if (kind & SXK_STMT)
    {
        ILTree::Statement *pStatement = &ptreeOut->AsStatement();

        if (pParent)
        {
            pStatement->Parent = pParent;
        }

        // check special operators
        switch (ptreeOut->bilop)
        {
            case SL_LABEL:
            {
                ILTree::LabelStatement & labelStmt = ptreeOut->AsLabelStatement();
                
                labelStmt.Label = xCopyLabelEntry(labelStmt.Label, &labelStmt);
                break;
            }
            
            case SL_GOTO:
            {
                ILTree::GotoStatement & gotoStmt = ptreeOut->AsGotoStatement();
                
                gotoStmt.Label = xCopyLabelEntry(gotoStmt.Label);
                break;
            }
            
            case SL_RESUME:
            case SL_ON_ERR:
            {
                ILTree::OnErrorStatement & onErrorStmt = ptreeOut->AsOnErrorStatement();
                
                onErrorStmt.Label = xCopyLabelEntry(onErrorStmt.Label);
                break;
            }

            case SL_STOP:
            case SL_END:
            {
                break;
            }
            case SL_EXIT:
            {
                ILTree::ExitStatement *pExit = &ptreeOut->AsExitStatement();

                if (pExit->ExitedStatement != NULL)
                {
                    // Dev10 #758830 we need to locate previously created copy of the original ExitedStatement
                    ILTree::PILNode pCopyOfExitedStatement = NULL;

                    ThrowIfFalse(m_BlocksBeingCopied.GetValue(pExit->ExitedStatement, &pCopyOfExitedStatement));
                    pExit->ExitedStatement = pCopyOfExitedStatement;
                }
                    
                break;
            }
            case SL_RETURN:
            {
                ILTree::ReturnStatement *pReturn = &ptreeOut->AsReturnStatement();

                pReturn->ReturnExpression = pReturn->ReturnExpression ? xCopyBilTreeImpl(pReturn->ReturnExpression, flags) : NULL;
                break;
            }
            case SL_YIELD:
            {
                ILTree::YieldStatement *pYield = &ptreeOut->AsYieldStatement();

                pYield->YieldExpression = pYield->YieldExpression ? &xCopyBilTreeImpl(pYield->YieldExpression, flags)->AsExpression() : NULL;

                break;
            }
            case SL_CONTINUE:
            {
                ILTree::ContinueStatement *pContinue = &ptreeOut->AsContinueStatement();

                if (pContinue->EnclosingLoopStatement != NULL)
                {
                    // Dev10 #758830 we need to locate previously created copy of the original EnclosingLoopStatement
                    ILTree::PILNode pCopyOfEnclosingLoopStatement = NULL;

                    ThrowIfFalse(m_BlocksBeingCopied.GetValue(pContinue->EnclosingLoopStatement, &pCopyOfEnclosingLoopStatement));
                    pContinue->EnclosingLoopStatement = pCopyOfEnclosingLoopStatement;
                }
                break;
            }
            case SL_STMT:
            case SL_ERROR:
            case SL_VAR_DECL:
            {
                ILTree::StatementWithExpression *pStmt = &ptreeOut->AsStatementWithExpression();

                pStmt->ptreeOp1 = pStmt->ptreeOp1 ? xCopyBilTreeImpl(pStmt->ptreeOp1, flags) : NULL;
                break;
            }
            case SL_REDIM:
            {
                ILTree::RedimStatement *pReDim = &ptreeOut->AsRedimStatement();

                pReDim->ptreeOp1 = pReDim->ptreeOp1 ? xCopyBilTreeImpl(pReDim->ptreeOp1, flags) : NULL;

                pReDim->Operand2 = pReDim->Operand2 ? &xCopyBilTreeImpl(pReDim->Operand2, flags)->AsExpression() : NULL;
                break;
            }
            case SL_DEBUGLOCNOP:
            {
                break;
            }
           default:
                if (!(ptreeOut->IsExecutableBlockNode()))
                {
                    // If the operator is an executable block it should be handled in the next if block.
                    ThrowIfFalse(false); // new operator? add it to the list"
                }
                break;
        }
    }

    if (ptreeOut->IsExecutableBlockNode())
    {
        ILTree::ExecutableBlock *pExecutableBlock = &ptreeOut->AsExecutableBlock();
        BCSYM_Hash * pClonedLocalsHash = NULL;

        AssertIfFalse(HasFlag(flags, BCF_IN_LAMBDA)); // Do we ever clone a block that is not inside a Lambda being cloned?

        if (!HasFlag(flags, BCF_IN_LAMBDA) || !HASFLAG(flags, BCF_SHALLOW_COPY_LAMBDA))
        {
            pClonedLocalsHash = DeepCloneBlockLocals( pExecutableBlock, m_varMap);
        }

        // Dev10 #758830: Add this block to the map, 
        // so that when we copy children or grand children
        // we could link their copies to the new block 
        ThrowIfTrue(m_BlocksBeingCopied.Contains(ptreeIn));
        m_BlocksBeingCopied.SetValue(ptreeIn, pExecutableBlock);

        // Dev10 #772215 keep track of blocks copied within a Lambda.
        if (HasFlag(flags, BCF_IN_LAMBDA))
        {
            ThrowIfTrue(m_BlocksCopiedWithinLambda.Contains(ptreeIn));
            m_BlocksCopiedWithinLambda.SetValue(ptreeIn, pExecutableBlock);
        }

        ILTree::Statement *pBlockChild = pExecutableBlock->Child;
        bool initialChildCopied = false;
        ILTree::Statement *pPreviousStatement = NULL;

        // [Try] node points to the associated [Finally] node. 
        // We need to keep track of copy of the [Try] node to patch the pointer to refer to the copy of the [Finally] node.
        // Both nodes are going to be on the same list, but we run into the [Try] node earlier.
        ILTree::TryBlock *pTryBlockToPatch = NULL; 
        
        while(pBlockChild)
        {
            ILTree::Statement *pOriginalBlockChild = pBlockChild;
            pBlockChild = &xCopyBilTreeImpl( pBlockChild, flags, &ptreeOut->AsExecutableBlock())->AsStatement();

            // Remember [Try] node to patch it later.
            if (pBlockChild->bilop == SB_TRY)
            {
                ThrowIfTrue(pTryBlockToPatch); // Previous [Try] node is left unpatched!
            
                ILTree::TryBlock *pTryBlock = &pBlockChild->AsTryBlock();

                if (pTryBlock->FinallyTree != NULL)
                {
                    pTryBlockToPatch = pTryBlock;
                }
                else 
                {
                    pTryBlockToPatch = NULL;
                }
            }
            else if (pTryBlockToPatch != NULL && pBlockChild->bilop == SB_FINALLY)
            {
                // Patch corresponding [Try] node.
                AssertIfFalse(pTryBlockToPatch->FinallyTree == pOriginalBlockChild);

                if (pTryBlockToPatch->FinallyTree == pOriginalBlockChild)    
                {
                    pTryBlockToPatch->FinallyTree = &pBlockChild->AsFinallyBlock();
                    pTryBlockToPatch = NULL;
                }
            }

            pBlockChild->Prev = pPreviousStatement;

            if (pPreviousStatement)
            {
                pPreviousStatement->Next = pBlockChild;
            }

            if (!initialChildCopied)
            {
                pExecutableBlock->ptreeChild = pBlockChild;
                initialChildCopied = true;
            }

            pPreviousStatement = pBlockChild;
            pBlockChild = pBlockChild->Next;
        }

        ThrowIfTrue(pTryBlockToPatch); // [Try] node is left unpatched!

        // Dev10 #758830: Remove this block from the map 
        m_BlocksBeingCopied.Remove(ptreeIn);

        // check special operators
        switch (ptreeOut->bilop)
        {
            case SB_IF:
            case SB_ELSE_IF:
            case SB_ELSE:
            {
                ILTree::IfBlock *pIfBlock = &ptreeOut->AsIfBlock();

                pIfBlock->Conditional = pIfBlock->Conditional ? &xCopyBilTreeImpl( pIfBlock->Conditional, flags)->AsExpression() : NULL;
                break;
            }
            case SB_SELECT:
            {
                ILTree::SelectBlock *pSelectBlock = &ptreeOut->AsSelectBlock();
                pSelectBlock->SelectorCapture = pSelectBlock->SelectorCapture ? xCopyBilTreeImpl( pSelectBlock->SelectorCapture, flags) : NULL;

                // Dev10 #772215 Check if we need to remap variable reference
                if ( pSelectBlock->SelectorTemporary != NULL )
                {
                    Variable *newVar = NULL;
                    if ( m_varMap.GetValue(pSelectBlock->SelectorTemporary, &newVar) )
                    {
                        pSelectBlock->SelectorTemporary = newVar;
                    }
                }
                break;
            }
            
            case SB_CASE:
            {
                ILTree::CaseBlock * pCaseBlock = &ptreeOut->AsCaseBlock();
                
                if ((pCaseBlock->uFlags & SBF_CASE_CONDITION) && pCaseBlock->Conditional != NULL)
                {
                    pCaseBlock->Conditional = &xCopyBilTreeImpl( pCaseBlock->Conditional , flags)->AsExpression();
                }

                break;
            }
            
            case SB_CATCH:
            {
                ILTree::CatchBlock *pCatchBlock = &ptreeOut->AsCatchBlock();

                pCatchBlock->CatchVariableTree = pCatchBlock->CatchVariableTree ? &xCopyBilTreeImpl(pCatchBlock->CatchVariableTree, flags)->AsExpression() : NULL;
                pCatchBlock->ExceptionStore = pCatchBlock->ExceptionStore ? &xCopyBilTreeImpl(pCatchBlock->ExceptionStore, flags)->AsStatement() : NULL;
                pCatchBlock->WhenExpression = pCatchBlock->WhenExpression ? &xCopyBilTreeImpl(pCatchBlock->WhenExpression, flags)->AsExpression() : NULL;
                
                // Dev10 #772215 Check if we need to remap variable reference
                if ( pCatchBlock->ExceptionTemporary != NULL )
                {
                    Variable *newVar = NULL;
                    if ( m_varMap.GetValue(pCatchBlock->ExceptionTemporary, &newVar) )
                    {
                        pCatchBlock->ExceptionTemporary = newVar;
                    }
                }
                break;
            }
            case SB_FOR:
            {
                ILTree::ForBlock *pFor = &ptreeOut->AsForBlock();

                pFor->Condition = pFor->Condition ? &xCopyBilTreeImpl(pFor->Condition, flags)->AsExpression() : NULL;
                pFor->ControlVariableReference = pFor->ControlVariableReference ? &xCopyBilTreeImpl(pFor->ControlVariableReference, flags)->AsExpression() : NULL;
                pFor->IncrementStatement = pFor->IncrementStatement ? &xCopyBilTreeImpl(pFor->IncrementStatement, flags)->AsStatement() : NULL;
                pFor->Initial = pFor->Initial ? &xCopyBilTreeImpl(pFor->Initial, flags)->AsExpression() : NULL;
                pFor->Limit = pFor->Limit ? &xCopyBilTreeImpl( pFor->Limit, flags)->AsExpression() : NULL;
                pFor->SignCheckStatement = pFor->SignCheckStatement ? &xCopyBilTreeImpl(pFor->SignCheckStatement, flags)->AsStatement() : NULL;
                pFor->Step = pFor->Step ? &xCopyBilTreeImpl(pFor->Step, flags)->AsExpression() : NULL;
                pFor->LiftedLimitTemporary = pFor->LiftedLimitTemporary ? &xCopyBilTreeImpl(pFor->LiftedLimitTemporary, flags)->AsSymbolReferenceExpression() : NULL;
                pFor->LiftedStepTemporary = pFor->LiftedStepTemporary ? &xCopyBilTreeImpl(pFor->LiftedStepTemporary, flags)->AsSymbolReferenceExpression() : NULL;


                // Dev10 #772215 Check if we need to remap variable references
                if ( pFor->EnumeratorTemporary != NULL )
                {
                    Variable *newVar = NULL;
                    if ( m_varMap.GetValue(pFor->EnumeratorTemporary, &newVar) )
                    {
                        pFor->EnumeratorTemporary = newVar;
                    }
                }

                if ( pFor->StepTemporary != NULL )
                {
                    Variable *newVar = NULL;
                    if ( m_varMap.GetValue(pFor->StepTemporary, &newVar) )
                    {
                        pFor->StepTemporary = newVar;
                    }
                }

                if ( pFor->LimitTemporary != NULL )
                {
                    Variable *newVar = NULL;
                    if ( m_varMap.GetValue(pFor->LimitTemporary, &newVar) )
                    {
                        pFor->LimitTemporary = newVar;
                    }
                }

                break;
            } 
            case SB_FOR_EACH:
            {
                ILTree::ForEachBlock *pForEach = &ptreeOut->AsForEachBlock();

                pForEach->Conditional = pForEach->Conditional ? &xCopyBilTreeImpl(pForEach->Conditional, flags)->AsExpression() : NULL;

                pForEach->Collection = pForEach->Collection ? &xCopyBilTreeImpl(pForEach->Collection, flags)->AsExpression() : NULL;
                pForEach->ControlVariableReference = pForEach->ControlVariableReference ? &xCopyBilTreeImpl(pForEach->ControlVariableReference, flags)->AsExpression() : NULL;
                pForEach->CurrentIndexAssignment = pForEach->CurrentIndexAssignment ? &xCopyBilTreeImpl(pForEach->CurrentIndexAssignment, flags)->AsExpression() : NULL;
                pForEach->IncrementStatement = pForEach->IncrementStatement ? &xCopyBilTreeImpl(pForEach->IncrementStatement, flags)->AsStatement() : NULL;
                pForEach->Initializer = pForEach->Initializer ? &xCopyBilTreeImpl(pForEach->Initializer, flags)->AsExpression() : NULL;                
                pForEach->InitializeClosure = pForEach->InitializeClosure ? &xCopyBilTreeImpl(pForEach->InitializeClosure, flags)->AsStatement() : NULL;
                pForEach->ControlVaribleTempInitializer = pForEach->ControlVaribleTempInitializer ? &xCopyBilTreeImpl(pForEach->ControlVaribleTempInitializer , flags)->AsExpression() : NULL;

                break;
            }
            case SB_DO:
            case SB_LOOP:
            {
                ILTree::LoopBlock *pDo = &ptreeOut->AsLoopBlock();

                pDo->Conditional = pDo->Conditional ? &xCopyBilTreeImpl(pDo->Conditional, flags)->AsExpression() : NULL;
                break;
            }
            case SB_USING:
            case SB_SYNCLOCK:
            case SB_HIDDEN_CODE_BLOCK:
            case SB_STATEMENT_LAMBDA:
            case SB_TRY:
            case SB_FINALLY:
            case SB_IF_BLOCK:
            {
                break;
            }
            case SB_WITH:
            {
                ILTree::WithBlock *pWith = &ptreeOut->AsWithBlock();

                pWith->ObjectBeingInitialized = pWith->ObjectBeingInitialized ? &xCopyBilTreeImpl(pWith->ObjectBeingInitialized, flags)->AsExpression() : NULL;
                pWith->ptreeWith = pWith->ptreeWith ? xCopyBilTreeImpl(pWith->ptreeWith, flags) : NULL;
                pWith->RecordReference = pWith->RecordReference ? &xCopyBilTreeImpl(pWith->RecordReference, flags)->AsExpression() : NULL;
                pWith->TemporaryClearAssignment = pWith->TemporaryClearAssignment ? &xCopyBilTreeImpl(pWith->TemporaryClearAssignment, flags)->AsExpression() : NULL;
                break;
            }
            default:
                ThrowIfFalse(false); // new operator? add it to the list"
            
        }

        // Remove locals scoped to the original block from the replacement map.
        if (pClonedLocalsHash != NULL)
        {
            UnmapVariables(pClonedLocalsHash, m_varMap);
        }
    }
    return ptreeOut;
}

LABEL_ENTRY_BIL * BILALLOC::xCopyLabelEntry(LABEL_ENTRY_BIL * pEntryIn, ILTree::LabelStatement *pParent)
{
    ThrowIfNull(m_pCopiedLabelEntries);
    
    if (pEntryIn == NULL)
    {
        return NULL;
    }

    LABEL_ENTRY_BIL * pEntryOut = NULL;

    if (!m_pCopiedLabelEntries->GetValue(pEntryIn, &pEntryOut))
    {
        pEntryOut = new (*this) LABEL_ENTRY_BIL;
        *pEntryOut = *pEntryIn;

        pEntryOut->pLabelParent = NULL;

        m_pCopiedLabelEntries->SetValue(pEntryIn, pEntryOut);
    }

    ThrowIfNull(pEntryOut);

    if (pParent)
    {
        pEntryOut->pLabelParent = pParent;
    }

    return pEntryOut;
}

ILTree::SymbolReferenceExpression *BILALLOC::xCopySymbolReferenceTree(ILTree::PILNode ptree)
{
    ThrowIfNull(ptree);
    ThrowIfFalse(SX_SYM == ptree->bilop);
    return &xCopyBilTreeImpl(ptree, BCF_NONE)->AsSymbolReferenceExpression();
}

//==============================================================================
// Performs a shallow copy of the lambda.  It will do a full copy of the
// actual expression tree but it will not copy the symbols, locals and
// temporaries.  So while you will have two independent BILTREE's, they will
// refer to the same symbols
//==============================================================================
ILTree::LambdaExpression *BILALLOC::xShallowCopyLambdaTree(ILTree::LambdaExpression *expr)
{
    ThrowIfNull(expr);
    ILTree::LambdaExpression * retval = &xCopyBilTreeImpl(expr, BCF_SHALLOW_COPY_LAMBDA)->AsLambdaExpression();
    AssertIfFalse(m_varMap.Count() == 0);
    AssertIfFalse(m_BlocksCopiedWithinLambda.Count() == 0);
    AssertIfFalse(m_BlocksBeingCopied.Count() == 0);

    return retval;
}

ILTree::UnboundLambdaExpression *BILALLOC::xShallowCopyUnboundLambdaTreeForScratch(ILTree::UnboundLambdaExpression *expr)
{
    ThrowIfNull(expr);
    ILTree::UnboundLambdaExpression * retval = &xCopyBilTreeImpl(expr, BCF_SHALLOW_COPY_LAMBDA|BCF_SCRATCH)->AsUnboundLambdaExpression();
    AssertIfFalse(m_varMap.Count() == 0);
    AssertIfFalse(m_BlocksCopiedWithinLambda.Count() == 0);
    AssertIfFalse(m_BlocksBeingCopied.Count() == 0);

    return retval;
}

ILTree::PILNode BILALLOC::xCopyBilTreeForScratch(ILTree::PILNode ptree)
{
    ILTree::PILNode retval = xCopyBilTreeImpl(ptree, BCF_SCRATCH);
    AssertIfFalse(m_varMap.Count() == 0);
    AssertIfFalse(m_BlocksCopiedWithinLambda.Count() == 0);
    AssertIfFalse(m_BlocksBeingCopied.Count() == 0);

    return retval;
}

//==============================================================================
// Copy the biltree completely with no regard to side effects.  This method
// can be unsafe if it's use is not to explicitly copy every side effect in the
// source tree
//==============================================================================
ILTree::PILNode BILALLOC::xCopyBilTreeUnsafe(ILTree::PILNode ptree)
{
    ILTree::PILNode retval = xCopyBilTreeImpl(ptree, BCF_NONE);
    AssertIfFalse(m_varMap.Count() == 0);
    AssertIfFalse(m_BlocksCopiedWithinLambda.Count() == 0);
    AssertIfFalse(m_BlocksBeingCopied.Count() == 0);

    return retval;
}


ILTree::PILNode BILALLOC::xCopyBilTreeBeta2SpecialCase(ILTree::PILNode ptree)
{
    ILTree::PILNode retval = xCopyBilTreeImpl(ptree, BCF_BETA2_SPECIAL_CASE);
    AssertIfFalse(m_varMap.Count() == 0);
    AssertIfFalse(m_BlocksCopiedWithinLambda.Count() == 0);
    AssertIfFalse(m_BlocksBeingCopied.Count() == 0);

    return retval;
}

    //==============================================================================
    // Copy the locals.  Make sure to add the map entries
    // Returns pointer to the old locals hash
    //==============================================================================
    BCSYM_Hash * BILALLOC::DeepCloneLambdaLocals(ILTree::LambdaExpression *lambdaExpr, DynamicHashTable<Variable*,Variable*> &varMap)
    {
        // Don't bother if we don't have a locals hash
        if ( !lambdaExpr->GetLocalsHash())
        {
            return NULL;
        }

        BCSYM_Hash *oldHash = lambdaExpr->GetLocalsHash();

        Symbols symbols(m_pCompiler, m_pnraTree, NULL);
        BCSYM_Hash *newHash = DeepCloneLocals(oldHash, symbols, varMap);
        
        // Update the actual hash value
        lambdaExpr->SetLocalsHash(newHash);

        return oldHash;
    }

    // Returns pointer to the old locals hash
    BCSYM_Hash * BILALLOC::DeepCloneBlockLocals(ILTree::ExecutableBlock *pExecBlock, DynamicHashTable<Variable*,Variable*> &varMap)
    {
        // Don't bother if we don't have a locals hash
        if ( !pExecBlock->Locals)
        {
            return NULL;
        }

        BCSYM_Hash *oldHash = pExecBlock->Locals;

        Symbols symbols(m_pCompiler, m_pnraTree, NULL);
        BCSYM_Hash *newHash = DeepCloneLocals(oldHash, symbols, varMap);
        
        // Update the actual hash value
        pExecBlock->Locals = newHash;

        return oldHash;
    }

    BCSYM_Hash* BILALLOC::DeepCloneLocals(BCSYM_Hash *oldHash, Symbols &symbols, DynamicHashTable<Variable*,Variable*> &varMap)
    {
        BCSYM_Hash *newHash = symbols.GetHashTable(NULL, oldHash->GetImmediateParent(), true, 1, NULL);

        BCITER_HASH_CHILD hashIt(oldHash);
        Declaration *decl;
        while (decl = hashIt.GetNext())
        {
            if ( decl->IsVariable())
            {
                Variable *oldVar = decl->PVariable();
                Variable *newVar = DeepCloneVariable(oldVar, symbols);
                Symbols::AddSymbolToHash(newHash, newVar, true, false, false);
                MapVariable(oldVar, newVar, varMap);
            }
        }

        return newHash;
    }

    // Remove variables contained in oldHash from the varMap hash table.
    void BILALLOC::UnmapVariables(BCSYM_Hash *oldHash, DynamicHashTable<Variable*,Variable*> &varMap)
    {
        BCITER_HASH_CHILD hashIt(oldHash);
        Declaration *decl;
        while (decl = hashIt.GetNext())
        {
            if ( decl->IsVariable())
            {
                Variable *oldVar = decl->PVariable();
                bool removed = varMap.Remove(oldVar);
                AssertIfFalse(removed);
            }
        }
    }

    // Remove variables contained in oldTemps from the varMap hash table.
    void BILALLOC::UnmapTemporaries(TemporaryManager *oldTemps, DynamicHashTable<Variable*,Variable*> &varMap)
    {
        TemporaryIterator it;
        it.Init(oldTemps);

        Temporary *cur;
        while (cur=it.GetNext())
        {
            Variable *oldVar = cur->Symbol;
            bool removed = varMap.Remove(oldVar);
            AssertIfFalse(removed);
        }
    }

    BCSYM_Variable* BILALLOC::DeepCloneVariable(BCSYM_Variable *oldVar, Symbols &symbols)
    {
        Variable *newVar = symbols.AllocVariable(
                oldVar->HasLocation(),
                oldVar->IsVariableWithValue());
        symbols.GetVariable(
                oldVar->HasLocation() ? oldVar->GetLocation() : NULL,
                oldVar->GetName(),
                oldVar->GetName(),
                oldVar->GetDeclFlags(),
                oldVar->GetVarkind(),
                oldVar->GetType(),
                oldVar->IsVariableWithValue() ? oldVar->PVariableWithValue()->GetExpression() : NULL,
                NULL,
                newVar);
        newVar->SetIsLambdaMember(oldVar->IsLambdaMember());
        newVar->SetIsQueryRecord(oldVar->IsQueryRecord());

        return newVar;
    }
    
    //-------------------------------------------------------------------------------------------------
    //
    // It's ok to map variables by pointers even when we are in several depths of
    // nested lambdas.  They will have different pointer values so the map
    // will work out
    //
    void BILALLOC::MapVariable(Variable *oldVar, Variable *newVar, DynamicHashTable<Variable*,Variable*> &varMap)
    {
        ThrowIfNull(oldVar);
        ThrowIfNull(newVar);
        ThrowIfTrue(varMap.Contains(oldVar));
        varMap.SetValue(oldVar, newVar);
    }
