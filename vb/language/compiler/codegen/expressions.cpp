//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Generate COM+ byte codes for expressions.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

extern RuntimeMemberDescriptor g_rgRTLangMembers[];
extern RuntimeClassDescriptor  g_rgRTLangClasses[];

// Can the index fit in an optimized local load?
#define SMALL_INDEX(index)      (index <= UCHAR_MAX)

// Can the index be inlined?
#define INLINE_INDEX(index)     (index < 4)

inline bool IsShortCircuitOperator(BILOP op)
{
    return op == SX_ANDALSO || op == SX_ORELSE;
}

//-------------------------------------------------------------------------------------------------
//
// Generate an expression
//
void CodeGenerator::GenerateRvalue(ILTree::PILNode ptree)
{
    VSASSERT(ptree != NULL, "GenerateRvalue: must have expr");
#if DEBUG
    if ((ptree->uFlags & SF_ERROR) && VSFSWITCH(fDumpBoundMethodTrees))
    {
        BILDUMP dump(m_pCompiler);
        dump.DumpBilTree(ptree);
    }
#endif
    VSASSERT(!(ptree->uFlags & SF_ERROR), "GenerateRvalue: didn't expect bad flag on bound tree");

    VSASSERT(m_usCurStackLvl == m_stackTypes.Count(), "GenerateRvalue: m_usCurStackLvl != m_stackTypes.Count()!");

    // Backup the hydration context and set it to NULL. Most expressions don't
    // need to be concerned with hydration; those that do will create their own
    // hydration contexts in their helper methods (e.g. GenerateCall).
    BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);
    m_currentHydrationContext = NULL;
    const BILOP Bilop = ptree->bilop;

    switch (Bilop)
    {
            /*
             * Symbol expressions
             */
    case SX_SYM:
        {
            ADDRMODE addrmode;
            GenerateAddr(ptree, &addrmode);
            GenerateLoad(ptree, &addrmode);

            // Is this a Me of a stucture?  If so, the struct must be loaded onto the stack.
            if (addrmode.addrkind == ADDR_Me && ptree->AsExpression().ResultType->GetVtype() == t_struct)
            {
                VSASSERT(ptree->AsSymbolReferenceExpression().pnamed->PVariable()->IsMe(),
                          "GenerateRvalue: sym must be Me");
                GenerateLoadStructFromAddr(ptree->AsExpression().ResultType, &ptree->Loc);
            }
        }
        break;

    case SX_ADR:
        {
            // SX_ADRs do actually want to pass along the hydration context
            // they were given.
            hydrationContextBackup.Restore();

            ADDRMODE addrmode;
            ILTree::PILNode ptreeSym = ptree->AsExpressionWithChildren().Left;

            VSASSERT(ptreeSym != NULL, "what is this the address of?");

            GenerateAddr(ptreeSym, &addrmode, ptree->uFlags & SXF_CONSTRAINEDCALL_BASEREF);

            // "Me" for a value type is already an address.
            if (addrmode.addrkind == ADDR_Me)
            {
                VSASSERT(ptreeSym->AsSymbolReferenceExpression().pnamed->PVariable()->IsMe() &&
                            ptreeSym->AsExpression().ResultType->GetVtype() == t_struct,
                          "GenerateRvalue: not a Me of t_struct.  What could this be otherwise?");
                GenerateLoad(ptreeSym, &addrmode);

                // If there's no hydration context, then the address of Me has
                // been loaded onto the evaluation stack. However, the call to
                // GenerateLoad has left the type of Me (from ptreeSym) on
                // m_stackTypes, rather than the type of a pointer to Me (from
                // ptree). We need to fix it up here.
                // If there is a hydration context, then accessing Me has been
                // deferred (because we must be compiling the MoveNext method
                // of an async, and "Me" is the managed pointer to the value-
                // type state machine). The stack hasn't actually changed, so
                // there's nothing to fix.
                if (m_currentHydrationContext == NULL)
                {
                    m_stackTypes.PopOrDequeue();
                    m_stackTypes.PushOrEnqueue(ptree->AsExpression().ResultType);
                }
            }
            else
            {
                GenerateLoadAddr(ptreeSym, &addrmode, ptree->uFlags & SXF_CONSTRAINEDCALL_BASEREF);
            }
        }
        break;

    case SX_NOTHING:
        VSASSERT(ptree->AsExpression().vtype == t_ref ||
                    ptree->AsExpression().vtype == t_array ||
                    ptree->AsExpression().vtype == t_string,
                  "GenerateRvalue: type of SX_NOTHING must be a reference type.");
        EmitOpcode(CEE_LDNULL, ptree->AsExpression().ResultType);
        break;

            /*
             * Constant expressions
             */
    case SX_CNS_STR:
        GenerateLiteralStr(ptree->AsStringConstant().Spelling, ptree->AsStringConstant().Length);
        break;

    case SX_CNS_INT:
        if (ptree->AsIntegralConstantExpression().vtype == t_date)
        {
            GenerateLiteralDate(ptree);
        }
        else
        {
            GenerateLiteralInt(
                ptree->AsIntegralConstantExpression().Value,
                ptree->AsIntegralConstantExpression().vtype == t_i8 || ptree->AsIntegralConstantExpression().vtype == t_ui8);
        }
        break;

    case SX_CNS_FLT:
        GenerateLiteralFp(ptree->AsFloatConstantExpression().Value, ptree->AsExpression().vtype);
        break;

    case SX_CNS_DEC:
        GenerateLiteralDec(ptree);
        break;

    case SX_METATYPE:
        GenerateMetaType(ptree);
        break;

            /*
             * Array expressions
             */
    case SX_INDEX:
        {
            ADDRMODE addrmode;

            GenerateAddr(ptree, &addrmode);
            GenerateLoad(ptree, &addrmode);
        }
        break;

    case SX_VARINDEX:
        GenerateLateIndex(ptree);
        break;

            /*
             * Assignment/coercions expressions
             */
    case SX_ASG:
    case SX_ASG_RESADR:
        {
            // Assignment needs to provide access to the previous hydration
            // context to some of its children.
            hydrationContextBackup.Restore();
            GenerateAssignment(ptree);
        }
        break;

    case SX_CTYPE:
        GenerateConvert(&ptree->AsBinaryExpression());
        break;

    case SX_DIRECTCAST:
        GenerateDirectCast(&ptree->AsBinaryExpression());
        break;

    case SX_TRYCAST:
        GenerateTryCast(&ptree->AsBinaryExpression());
        break;

    case SX_WIDE_COERCE:
        // A wide coerce is a place holder to represent coercions from reference types to more
        // general reference types, thus this is a nop.
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        break;

            /*
             * Instantiation expressions
             */
    case SX_NEW:
        VSASSERT(ptree->AsNewExpression().pmod,
                 "GenerateRvalue: SX_NEW must have object sym");
        GenerateNew(ptree->AsNewExpression().pmod, ptree->AsNewExpression().ptreeNew, ptree ? &ptree->Loc : NULL);
        break;

    case SX_NEW_ARRAY:
        GenerateArrayConstruction(ptree->AsExpression().ResultType->PArrayType(),
                                  ptree->AsExpressionWithChildren().Left,
                                  &ptree->Loc);
        break;

    case SX_INIT_STRUCTURE:
        GenerateInitStructure(ptree->AsInitStructureExpression().StructureType,
                              ptree->AsInitStructureExpression().StructureReference);
        break;

    case SX_IIF:
        {
            // (condition) ? Expr1 : Expr2

            VSASSERT(ptree->AsIfExpression().condition->vtype == t_bool &&
                BCSYM::AreTypesEqual(ptree->AsIfExpression().Left->ResultType, ptree->AsExpression().ResultType) &&
                BCSYM::AreTypesEqual(ptree->AsIfExpression().Right->ResultType, ptree->AsExpression().ResultType),
                     "GenerateRValue: unexpected types for IIF");

            CODE_BLOCK * pcblkTrue = NewCodeBlock();
            CODE_BLOCK * pcblkFalse = NewCodeBlock();
            CODE_BLOCK * pcblkResult = NewCodeBlock();

            // say "false" for the Reverse parameter so the false block is handled first.
            // no reason for this other than the "false" path has one more branch than the "true" path.
            GenerateCondition(ptree->AsIfExpression().condition, false, pcblkTrue, pcblkFalse);

            // push Expr1
            GenerateRvalue(ptree->AsIfExpression().Right);
            EndCodeBuffer(CEE_BR_S, pcblkResult);
            AddToCurStack(-1);
            m_stackTypes.PopOrDequeue();

            SetAsCurrentCodeBlock(pcblkTrue);
            // push Expr2
            GenerateRvalue(ptree->AsIfExpression().Left);
            //AddToCurStack(-1);
            SetAsCurrentCodeBlock(pcblkResult);
        }
        break;

            /*
             * Call expressions
             */
    case SX_CALL:
        GenerateCall(ptree);
        break;

    case SX_LATE:
        GenerateLate(ptree);
        break;

            /*
             * Operator expressions
             */

    // Type comparison
    case SX_ISTYPE:
        GenerateIsType(ptree);

        // now we need to normalize - compare result of isinst to Null to make a boolean
        EmitOpcode(CEE_LDNULL, ptree->AsExpression().ResultType);
        EmitOpcode(CEE_CGT_UN, GetSymbolForVtype(t_bool));
        break;

    // Binary operators
    case SX_POW:
    case SX_NEG:
    case SX_PLUS:
    case SX_DIV:
    case SX_MOD:
    case SX_IDIV:
    case SX_CONC:
    case SX_NOT:
    case SX_AND:
    case SX_OR:
    case SX_XOR:
    case SX_MUL:
    case SX_ADD:
    case SX_SUB:
    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
        if (ptree->AsExpression().vtype == t_ref)
        {
            GenerateObjBinOp(ptree);
        }
        else if (ptree->AsExpression().vtype == t_decimal)
        {
            GenerateDecBinOp(ptree);
        }
        else
        {
            GenerateBinOp(ptree);
        }
        break;


    // Relational operators
    case SX_IS:
    case SX_ISNOT:
        VSASSERT(ptree->AsExpressionWithChildren().Right && ptree->AsExpressionWithChildren().Right->vtype == t_ref &&
                 ptree->AsExpressionWithChildren().Left && ptree->AsExpressionWithChildren().Left->vtype == t_ref,
                 "GenerateRvalue: opnds of IS must be t_ref.");

        __fallthrough; // fall through

    case SX_EQ:
    case SX_NE:
    case SX_LE:
    case SX_LT:
    case SX_GE:
    case SX_GT:
        GenerateRelOp(ptree);
        break;

    case SX_LIKE:
        GenerateLike(ptree);
        break;

    case SX_ANDALSO:
    case SX_ORELSE:
        {
            // if we've encountered a conditional here, then we need to leave the result
            // of the condition on the stack to be consumed later.

            // We must not be a sxShortCircuitOperationOp, because the lowering semantics
            // phase must have converted those to sxCalls.

            VSASSERT(!ptree->IsUserDefinedOperator(),
                    "GenerateRValue: unexpected user defined operator for AndAlso/OrElse");

            // follow this model:
            // (expression) ? push false : push true

            VSASSERT(ptree->AsExpression().vtype == t_bool || ptree->AsExpression().vtype == t_ref,
                     "GenerateRValue: unexpected type for conditional");

            if (ptree->AsExpression().vtype == t_ref)
            {
                GenerateObjectShortCircuitExpression(ptree);
            }
            else
            {
                CODE_BLOCK * pcblkTrue = NewCodeBlock();
                CODE_BLOCK * pcblkFalse = NewCodeBlock();
                CODE_BLOCK * pcblkResult = NewCodeBlock();

                // say "false" for the Reverse parameter so the false block is handled first.
                // no reason for this other than the "false" path has one more branch than the "true" path.
                GenerateCondition(ptree, false, pcblkTrue, pcblkFalse);

                // push false
                GenerateLiteralInt(COMPLUS_FALSE);
                EndCodeBuffer(CEE_BR_S, pcblkResult);
                // this "false" will be consumed later in the pcblkResult, so adjust the stack to compensate.
                AddToCurStack(-1);
                m_stackTypes.PopOrDequeue();

                SetAsCurrentCodeBlock(pcblkTrue);
                // push true
                GenerateLiteralInt(COMPLUS_TRUE);
                SetAsCurrentCodeBlock(pcblkResult);

                // NOP required here for proper stepping on x64
                InsertNOP();
            }
        }
        break;

    case SX_IF:
        {
            CODE_BLOCK *pcblkTrue = NewCodeBlock();
            CODE_BLOCK *pcblkFalse = NewCodeBlock();

            ////
            GenerateCondition(ptree->AsExpressionWithChildren().Left, true, pcblkTrue, pcblkFalse);

            GenerateRvalue(ptree->AsExpressionWithChildren().Right);

            SetAsCurrentCodeBlock(pcblkFalse);
        }
        break;

    case SX_ARRAYLEN:
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        // CEE_LDLEN pushes a native int; we shouldn't assume we know how to
        // spill this.
        EmitOpcode(CEE_LDLEN, UNSPILLABLE_TYPE_SYMBOL);
        EmitOpcode(ptree->AsExpressionWithChildren().vtype == t_i8 ? CEE_CONV_OVF_I8 : CEE_CONV_OVF_I4,
                   ptree->AsExpressionWithChildren().ResultType);
        break;

    case SX_SYNCLOCK_CHECK:
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        GenerateCallToRuntimeHelper(SyncLockCheckMember, Symbols::GetVoidType(), &ptree->Loc);
        break;

    case SX_LIST:
    case SX_SEQ:
        //Process a sequence iteratively, otherwise long sequences
        //can result in deep recursion and ---- the stack.
        while (true)
        {
            //process left as normal.
            GenerateRvalue(ptree->AsExpressionWithChildren().Left);

            //check right to see if it is a continuation of the sequence.
            ILTree::Expression* right = ptree->AsExpressionWithChildren().Right;
            if (right)
            {
                if (right->bilop == Bilop)
                {
                    ptree = right;
                }
                else
                {
                    GenerateRvalue(right);
                    break;
                }
            }
            else
            {
                break;
            }
        }
        break;

    case SX_SEQ_OP1:
    case SX_SEQ_OP2:
    case SX_ISTRUE:
    case SX_ISFALSE:
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        if (ptree->AsExpressionWithChildren().Right)
        {
            GenerateRvalue(ptree->AsExpressionWithChildren().Right);
        }
        break;

    case SX_ASYNCSPILL:
        GenerateAsyncSpill(&ptree->AsAsyncSpillExpression());
        break;

#if DEBUG
    case SX_LAMBDA:
    case SX_AWAIT:
        VSFAIL("SX_LAMBDA/SX_AWAIT is trying to be converted to an RValue. This should have been SX_ADR/Resumable'd already!");
        __fallthrough;
#endif
    case SX_CTYPEOP:
    default:
        VSFAIL("GenerateRValue: unexpected SX node");
        VbThrow(HrMake(ERRID_CodegenError));
    }
}

//========================================================================
// Do an object comparison
//========================================================================
RuntimeMembers CodeGenerator::GetHelperForObjRelOp
(
    BILOP bilop,
    Vtypes vtypeResult
)
{
    RuntimeMembers Helper = UndefinedRuntimeMember;
    if (vtypeResult == t_bool)
    {
        switch (bilop) // when the result is boolean use these helpers, e.g. dim b as boolean = o1 < o2
        {
            case SX_EQ: Helper = ConditionalCompareObjectEqualMember; break;
            case SX_NE: Helper = ConditionalCompareObjectNotEqualMember; break;
            case SX_LT: Helper = ConditionalCompareObjectLessMember; break;
            case SX_LE: Helper = ConditionalCompareObjectLessEqualMember; break;
            case SX_GE: Helper = ConditionalCompareObjectGreaterEqualMember; break;
            case SX_GT: Helper = ConditionalCompareObjectGreaterMember; break;
        }
    }
    else
    {
        switch (bilop) // when the result is anything but boolean, use these helpers, e.g. dim o as object  o1 < o2
        {
            case SX_EQ: Helper = CompareObjectEqualMember; break;
            case SX_NE: Helper = CompareObjectNotEqualMember; break;
            case SX_LT: Helper = CompareObjectLessMember; break;
            case SX_LE: Helper = CompareObjectLessEqualMember; break;
            case SX_GE: Helper = CompareObjectGreaterEqualMember; break;
            case SX_GT: Helper = CompareObjectGreaterMember; break;
        }
    }

    return Helper;
}


/*****************************************************************************
;GenerateRelOp

Generates a relational operator in response to a inline expression, e.g. dim b as boolean = s1=s2
*****************************************************************************/
void CodeGenerator::GenerateRelOp
(
ILTree::PILNode ptree
)
{
    Vtypes vtype = ptree->AsExpressionWithChildren().Left->vtype;
    BILOP bilop = ptree->bilop;

    ILTree::PILNode ptreeOp1 = ptree->AsExpressionWithChildren().Left;
    ILTree::PILNode ptreeOp2 = ptree->AsExpressionWithChildren().Right;

    VSASSERT(ptreeOp1 && ptreeOp2, "GenerateRelOp: binary op must have Op1 and Op2.");
    VSASSERT(ptreeOp1->AsExpression().vtype == ptreeOp2->AsExpression().vtype, "GenerateRelOp: vtypes of operands must match");


    // Optimization trick: simplify the comparison if one operand is constant.
    // For example, the comparison (b = true) is redundant;  the expression (b) would suffice.

    if (vtype == t_bool &&
        (bilop == SX_EQ || bilop == SX_NE) &&
        (ptreeOp1->AsExpression().bilop == SX_CNS_INT || ptreeOp2->AsExpression().bilop == SX_CNS_INT))
    {

        ILTree::PILNode ptreeConst = (ptreeOp1->AsExpression().bilop == SX_CNS_INT) ? ptreeOp1 : ptreeOp2;
        ILTree::PILNode ptreeExpr = (ptreeOp1->AsExpression().bilop == SX_CNS_INT) ? ptreeOp2 : ptreeOp1;

        VSASSERT(ptreeExpr->AsExpression().bilop != SX_CNS_INT, "This expression should have been further simplified by the constant expression evaluator.");

        GenerateRvalue(ptreeExpr);

        // Equality to False is equivalent to a Not operation.
        // (b = false) is equivalent to (Not b)
        // (b <> true) is equivalent to (Not b)

        if ((ptreeConst->AsIntegralConstantExpression().Value && bilop == SX_NE) ||
            (!ptreeConst->AsIntegralConstantExpression().Value && bilop == SX_EQ))
        {
            // Flip the value (best accomplished with a comparison to False).
            GenerateLiteralInt(COMPLUS_FALSE);
            EmitOpcode(CEE_CEQ, GetSymbolForVtype(t_bool));
        }
        return;
    }

    GenerateRvalue(ptreeOp1);
    GenerateRvalue(ptreeOp2);

    switch (vtype)
    {
    case t_bool:
        switch (bilop)
        {
            // Since VB True is -1 but is stored as 1 in IL, relational operations on Boolean must
            // be reversed to yield the correct results. Note that = and <> do not need reversal.
            case SX_LT: bilop = SX_GT; break;
            case SX_GT: bilop = SX_LT; break;
            case SX_GE: bilop = SX_LE; break;
            case SX_LE: bilop = SX_GE; break;
        }
        break;

    case t_decimal:
        // This helper returns 0 if equal, -1 if less than, 1 if greater than.
        // Comparing this value against zero gives the correct result.
        GenerateCallToRuntimeHelper(CompareDecimalMember, GetSymbolForVtype(t_i4), &ptree->Loc);  // 
        GenerateLiteralInt(0);  // even if we can't emit the call to the runtime helper we need to generate the literal so our stack tracking doesn't get out of whack
        break;

    case t_date:
        // This helper returns 0 if equal, -1 if less than, 1 if greater than.
        // Comparing this value against zero gives the correct result.
        GenerateCallToRuntimeHelper(CompareDateMember, GetSymbolForVtype(t_i4), &ptree->Loc);
        GenerateLiteralInt(0);
        break;

    case t_string:
        // Load up the Option Text value as the third argument.
        GenerateLiteralInt((ptree->uFlags & SXF_RELOP_TEXT) ? COMPLUS_TRUE : COMPLUS_FALSE);
        // This helper returns 0 if equal, -1 if less than, 1 if greater than.
        // Comparing this value against zero gives the correct result.
        if (m_Project->GetVBRuntimeKind() == EmbeddedRuntime)
        {
            GenerateCallToRuntimeHelper(EmbeddedCompareStringMember, GetSymbolForVtype(t_i4), &ptree->Loc);
        }
        else
        {
            GenerateCallToRuntimeHelper(CompareStringMember, GetSymbolForVtype(t_i4), &ptree->Loc);
        }
        GenerateLiteralInt(0); // even if we can't emit the call to the runtime helper we need to generate the literal so our stack tracking doesn't get out of whack
        break;

    case t_ref:
        // Is and IsNot turn into opcodes for = and <> instead of helper calls.
        if (bilop == SX_IS)
        {
            bilop = SX_EQ;
        }
        else if (bilop == SX_ISNOT)
        {
            bilop = SX_NE;
        }
        else
        {
            // Load up the Option Text value as the third argument.
            GenerateLiteralInt((ptree->uFlags & SXF_RELOP_TEXT) ? COMPLUS_TRUE : COMPLUS_FALSE);
            GenerateCallToRuntimeHelper(GetHelperForObjRelOp(bilop, ptree->AsExpression().vtype), ptree->AsExpression().ResultType, &ptree->Loc);
            return;
        }
        break;

    default:
        break;
    }

    if (vtype == t_double || vtype == t_single)
    {
        EmitOpcode(MapSxop2CompFloat(bilop), GetSymbolForVtype(t_bool));
    }
    else if (IsUnsignedType(vtype))
    {
        EmitOpcode(MapSxop2CompUnsigned(bilop), GetSymbolForVtype(t_bool));
    }
    else
    {
        EmitOpcode(MapSxop2Comp(bilop), GetSymbolForVtype(t_bool));
    }

    if (bilop == SX_NE || bilop == SX_LE || bilop == SX_GE)
    {
        // There is no opcode for NE, LE, or GE comparisons, so we have to reverse the value.
        GenerateLiteralInt(COMPLUS_FALSE);
        EmitOpcode(CEE_CEQ, GetSymbolForVtype(t_bool));
    }
}

/*****************************************************************************
;GenerateRelOpBranch

Generates a relational operator and jump expression, e.g. if s1 < s2
*****************************************************************************/
void CodeGenerator::GenerateRelOpBranch
(
    ILTree::PILNode ptree,
    bool fReverse,
    CODE_BLOCK *pcblkTrue,
    CODE_BLOCK *pcblkFalse
)
{
    Vtypes vtype = ptree->AsExpressionWithChildren().Left->vtype;
    BILOP bilop = ptree->bilop;

    ILTree::PILNode ptreeOp1 = ptree->AsExpressionWithChildren().Left;
    ILTree::PILNode ptreeOp2 = ptree->AsExpressionWithChildren().Right;

    VSASSERT(ptreeOp1 && ptreeOp2, "GenerateRelOpBranch: binary op must have Op1 and Op2.");
    VSASSERT(ptreeOp1->AsExpression().vtype == ptreeOp2->AsExpression().vtype, "GenerateRelOpBranch: vtypes of operands must match");
    VSASSERT(pcblkTrue && pcblkFalse, "GenerateRelOpBranch: code buffers for branch not valid");


    // Optimization trick: simplify the comparison if one operand is constant.
    // For example, the comparison (b = true) is redundant;  the expression (b) would suffice.

    if (vtype == t_bool &&
        (bilop == SX_EQ || bilop == SX_NE) &&
        (ptreeOp1->AsExpression().bilop == SX_CNS_INT || ptreeOp2->AsExpression().bilop == SX_CNS_INT))
    {
        ILTree::PILNode ptreeConst = (ptreeOp1->AsExpression().bilop == SX_CNS_INT) ? ptreeOp1 : ptreeOp2;
        ILTree::PILNode ptreeExpr = (ptreeOp1->AsExpression().bilop == SX_CNS_INT) ? ptreeOp2 : ptreeOp1;

        VSASSERT(ptreeExpr->AsExpression().bilop != SX_CNS_INT, "This expression should have been further simplified by the constant expression evaluator.");

        GenerateRvalue(ptreeExpr);

        // Start from the basic assumption that we branch to the True-block if the expression is true.
        // The Reverse flag, if enabled, flips the meaning so we branch to the False-block if the
        // expression is false.

        // The comparison kind can flip the branch (but not the branch destination)
        // because equality to False is equivalent to a NOT operation.
        // (b = false) is equivalent to (Not b)
        // (b <> true) is equivalent to (Not b)

        OPCODE opcode;

        if ((!ptreeConst->AsIntegralConstantExpression().Value && bilop == SX_EQ) ||
            (ptreeConst->AsIntegralConstantExpression().Value && bilop == SX_NE))
        {
            opcode = CEE_BRFALSE_S;
        }
        else
        {
            opcode = CEE_BRTRUE_S;
        }

        if (fReverse)
        {
            // Reverse the branch and the branch destination.
            opcode = (opcode == CEE_BRFALSE_S) ? CEE_BRTRUE_S : CEE_BRFALSE_S;
            EndCodeBuffer(opcode, pcblkFalse);
        }
        else
        {
            EndCodeBuffer(opcode, pcblkTrue);
        }
        return;
    }

    GenerateRvalue(ptreeOp1);
    GenerateRvalue(ptreeOp2);

    switch (vtype)
    {
    case t_bool:
        switch (bilop)
        {
            // Since VB True is -1 but is stored as 1 in IL, relational operations on Boolean must
            // be reversed to yield the correct results. Note that = and <> do not need reversal.
            case SX_LT: bilop = SX_GT; break;
            case SX_GT: bilop = SX_LT; break;
            case SX_GE: bilop = SX_LE; break;
            case SX_LE: bilop = SX_GE; break;
        }
        break;

    case t_decimal:
        // This helper returns 0 if equal, -1 if less than, 1 if greater than.
        // Comparing this value against zero gives the correct result.
        GenerateCallToRuntimeHelper(CompareDecimalMember, GetSymbolForVtype(t_i4), &ptree->Loc);  // 
        GenerateLiteralInt(0);  // even if we can't emit the call to the runtime helper we need to generate the literal so our stack tracking doesn't get out of whack
        break;

    case t_date:
        // This helper returns 0 if equal, -1 if less than, 1 if greater than.
        // Comparing this value against zero gives the correct result.
        GenerateCallToRuntimeHelper(CompareDateMember, GetSymbolForVtype(t_i4), &ptree->Loc);
        GenerateLiteralInt(0);
        break;

    case t_string:
        // Load up the Option Text value as the third argument.
        GenerateLiteralInt((ptree->uFlags & SXF_RELOP_TEXT) ? COMPLUS_TRUE : COMPLUS_FALSE);
        // This helper returns 0 if equal, -1 if less than, 1 if greater than.
        // Comparing this value against zero gives the correct result.
        if (m_Project->GetVBRuntimeKind() == EmbeddedRuntime)
        {
            GenerateCallToRuntimeHelper(EmbeddedCompareStringMember, GetSymbolForVtype(t_i4), &ptree->Loc);
        }
        else
        {
            GenerateCallToRuntimeHelper(CompareStringMember, GetSymbolForVtype(t_i4), &ptree->Loc);
        }
        GenerateLiteralInt(0); // even if we can't emit the call to the runtime helper we need to generate the literal so our stack tracking doesn't get out of whack
        break;

    case t_ref:
        // Is and IsNot turn into opcodes for = and <> instead of helper calls.
        if (bilop == SX_IS)
        {
            bilop = SX_EQ;
        }
        else if (bilop == SX_ISNOT)
        {
            bilop = SX_NE;
        }
        else
        {
            VSASSERT(ptree->AsExpression().vtype == t_bool, "must have boolean result for branch");

            RuntimeMembers Helper = UndefinedRuntimeMember;
            switch (bilop)
            {
                case SX_EQ: Helper = ConditionalCompareObjectEqualMember; break;
                case SX_NE: Helper = ConditionalCompareObjectNotEqualMember; break;
                case SX_LT: Helper = ConditionalCompareObjectLessMember; break;
                case SX_LE: Helper = ConditionalCompareObjectLessEqualMember; break;
                case SX_GE: Helper = ConditionalCompareObjectGreaterEqualMember; break;
                case SX_GT: Helper = ConditionalCompareObjectGreaterMember; break;
            }

            // Load up the Option Text value as the third argument.
            GenerateLiteralInt((ptree->uFlags & SXF_RELOP_TEXT) ? COMPLUS_TRUE : COMPLUS_FALSE);
            GenerateCallToRuntimeHelper(Helper, GetSymbolForVtype(t_bool), &ptree->Loc);

            if (fReverse)
            {
                EndCodeBuffer(CEE_BRFALSE_S, pcblkFalse);
            }
            else
            {
                EndCodeBuffer(CEE_BRTRUE_S, pcblkTrue);
            }
            return;
        }
        break;

    default:
        break;
    }

    if (fReverse)
    {
        if (IsUnsignedType(vtype) || vtype == t_double || vtype == t_single)
        {
            EndCodeBuffer(MapSxop2CompReverseBranchUnsigned(bilop), pcblkFalse, vtype);
        }
        else
        {
            EndCodeBuffer(MapSxop2CompReverseBranch(bilop), pcblkFalse, vtype);
        }
    }
    else
    {
        if (IsUnsignedType(vtype))
        {
            EndCodeBuffer(MapSxop2CompBranchUnsigned(bilop), pcblkTrue, vtype);
        }
        else
        {
            EndCodeBuffer(MapSxop2CompBranch(bilop), pcblkTrue, vtype);
        }
    }
}

//========================================================================
// Generate a binary operator
//========================================================================

void CodeGenerator::GenerateBinOp
(
ILTree::PILNode ptree
)
{
    OPCODE opcode;

    switch (ptree->bilop)
    {
    case SX_POW:
        VSASSERT(ptree->AsExpressionWithChildren().Left && ptree->AsExpressionWithChildren().Right,
                   "GenerateBinOp: sx_pow must have two trees");

        VSASSERT(ptree->AsExpression().vtype == t_double, "");

        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        GenerateRvalue(ptree->AsExpressionWithChildren().Right);
        GenerateCallToRuntimeHelper(NumericPowerMember, GetSymbolForVtype(t_double), &ptree->Loc); // Dim c As Double = a ^ b calls [mscorlib]System.Math::Pow()
        break;

    // 
    case SX_PLUS:
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        break;

    case SX_NEG:
        {
            Vtypes vtype = ptree->AsExpression().vtype;

            VSASSERT(IsNumericType(vtype) && !IsUnsignedType(vtype), "unexpected type for negation");

            // If overflow checking is on, we must subtract from zero because CEE_NEG doesn't
            // check for overflow.  Generate the zero const first.
            if ((vtype == t_i4 || vtype == t_i8) && !m_NoIntChecks)
            {
               GenerateZeroConst(vtype);
            }

            GenerateRvalue(ptree->AsExpressionWithChildren().Left);
            GenerateMathOp(SX_NEG, vtype, ptree);
        }
        break;

    case SX_IDIV:
        VSASSERT(IsIntegralType(ptree->AsExpression().vtype),
                 "GenerateBinOp: expected integral type for integer division.");
        __fallthrough;
    case SX_MUL:
    case SX_MOD:
    case SX_ADD:
    case SX_SUB:
    case SX_DIV:
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        GenerateRvalue(ptree->AsExpressionWithChildren().Right);
        GenerateMathOp(ptree->bilop, ptree->AsExpression().vtype, ptree);
        break;

    case SX_NOT:
        GenerateRvalue(ptree->AsExpressionWithChildren().Left);

        if (ptree->AsExpression().vtype == t_bool)
        {
            // Boolean Not maps 0 to 1 and non-zero to 0.
            GenerateLiteralInt(COMPLUS_FALSE);
            EmitOpcode(CEE_CEQ, GetSymbolForVtype(t_bool));
        }
        else
        {
            EmitOpcode(CEE_NOT, ptree->AsExpressionWithChildren().ResultType); // Is this really the right type?

            // Since the CLR will generate a 4-byte result from the Not operation, we
            // need to convert back to ui1 or ui2 because they are unsigned
            // 

            if (ptree->AsExpression().vtype == t_ui1 || ptree->AsExpression().vtype == t_ui2)
            {
                GenerateConvertSimpleType(t_ui4, ptree->AsExpression().vtype, false, ptree);
            }
        }
        break;

    case SX_AND:
        opcode = CEE_AND;
        goto lbl_BitwiseBinOp;

    case SX_OR:
        opcode = CEE_OR;
        goto lbl_BitwiseBinOp;

    case SX_XOR:
        opcode = CEE_XOR;
        goto lbl_BitwiseBinOp;

lbl_BitwiseBinOp:
        VSASSERT(ptree->AsExpressionWithChildren().Right, "GenerateBinOp: binop must have Op2.");
        VSASSERT(opcode != CEE_NOP,
                  "GenerateBinOp: opcode must be set before branching to label.");

        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        GenerateRvalue(ptree->AsExpressionWithChildren().Right);
        EmitOpcode(opcode, ptree->AsExpressionWithChildren().ResultType);
        break;

    case SX_CONC:
        VSASSERT(ptree->AsExpression().vtype == t_string,
                   "GenerateBinOp: concat only on strings and variants.");

        GenerateRvalue(ptree->AsExpressionWithChildren().Left);
        GenerateRvalue(ptree->AsExpressionWithChildren().Right);
        GenerateCallToRuntimeHelper(StringConcatenationMember, GetSymbolForVtype(t_string), &ptree->Loc); // [mscorlib] System.String::Concat()  In response to s1 & s2
        break;

    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
        GenerateShift(ptree);
        break;

    default:
        VSFAIL("unknown operator");
    }
}

/*========================================================================

Constant expressions

========================================================================*/

//========================================================================
// Generate a constant string
//========================================================================

void CodeGenerator::GenerateLiteralStr
(
_In_opt_count_(cch) const WCHAR * wsz,
size_t cch
)
{
    mdString str;

    if (wsz)
    {
        str = m_pmdemit->AddDataString(wsz, cch);
        EmitOpcode_U4(CEE_LDSTR, str, GetSymbolForVtype(t_string));
    }
    else
    {
        EmitOpcode(CEE_LDNULL, GetSymbolForVtype(t_string));
    }
}

//========================================================================
// Generate a constant integer
//========================================================================

COMPILE_ASSERT(
    CEE_LDC_I4_0 + 1 == CEE_LDC_I4_1 &&
    CEE_LDC_I4_0 + 2 == CEE_LDC_I4_2 &&
    CEE_LDC_I4_0 + 3 == CEE_LDC_I4_3 &&
    CEE_LDC_I4_0 + 4 == CEE_LDC_I4_4 &&
    CEE_LDC_I4_0 + 5 == CEE_LDC_I4_5 &&
    CEE_LDC_I4_0 + 6 == CEE_LDC_I4_6 &&
    CEE_LDC_I4_0 + 7 == CEE_LDC_I4_7 &&
    CEE_LDC_I4_0 + 8 == CEE_LDC_I4_8);


void
CodeGenerator::GenerateLiteralInt
(
    __int32 Value
)
{
    BCSYM* i4Symbol = GetSymbolForVtype(t_i4);

    if (Value >= 0 && Value <= 8)
    {
        EmitOpcode((opcode_t)(CEE_LDC_I4_0 + Value), i4Symbol);
    }
    else if (Value == -1)
    {
        EmitOpcode(CEE_LDC_I4_M1, i4Symbol);
    }
    else if (Value >= _I8_MIN && Value <= _I8_MAX)
    {
        EmitOpcode_U1(CEE_LDC_I4_S, Value, i4Symbol);
    }
    else
    {
        EmitOpcode_U4(CEE_LDC_I4, Value, i4Symbol);
    }
}


void
CodeGenerator::GenerateLiteralInt
(
    __int64 Value,
    bool Need64bitResult
)
{
    if (Need64bitResult)
    {
        BCSYM* i8Symbol = GetSymbolForVtype(t_i8);

        if (Value >= _I32_MIN && Value <= _UI32_MAX)
        {
            GenerateLiteralInt((__int32)Value);

            if (Value <= _I32_MAX)
            {
                EmitOpcode(CEE_CONV_I8, i8Symbol);
            }
            else
            {
                EmitOpcode(CEE_CONV_U8, i8Symbol);
            }
        }
        else
        {
            EmitOpcode_U8(CEE_LDC_I8, Value, i8Symbol);
        }
    }
    else
    {
        GenerateLiteralInt((__int32)Value);
    }
}


void CodeGenerator::GenerateLiteralDate
(
    ILTree::PILNode ptree
)
{
    __int64 Value = ptree->AsIntegralConstantExpression().Value;
    if (Value == 0)
    {
        GenerateRTField(DateConstZeroField, t_date);
    }
    else
    {
        GenerateLiteralInt(Value, true);
        GenerateCallToRuntimeHelper(DateFromConstantMember, GetSymbolForVtype(t_date), &ptree->Loc); // This one makes a call to System.DateTime::.ctor(int64) in mscorlib
    }
}


//========================================================================
// Generate a constant decimal value
//========================================================================
void CodeGenerator::GenerateLiteralDec
(
ILTree::PILNode ptree
)
{
    // use a long because we may have to emit all 4 bytes later
    long Negative = (ptree->AsDecimalConstantExpression().Value.sign & DECIMAL_NEG) == DECIMAL_NEG;

    // if we have a number which only uses the bottom 4 bytes and
    // has no fraction part, then we can generate more optimal code
    //
    if ( ptree->AsDecimalConstantExpression().Value.scale == 0 &&
         ptree->AsDecimalConstantExpression().Value.Hi32  == 0 &&
         ptree->AsDecimalConstantExpression().Value.Mid32 == 0 )
    {
        if (ptree->AsDecimalConstantExpression().Value.Lo32 == 0)
        {
            // whole value == 0 if we get here
            GenerateRTField(DecimalConstZeroField, t_decimal);
        }
        else if (ptree->AsDecimalConstantExpression().Value.Lo32 == 1)
        {
            if (Negative)
            {
                // whole value == -1 if we get here
                GenerateRTField(DecimalConstMinusOneField, t_decimal);
            }
            else
            {
                // whole value == 1 if we get here
                GenerateRTField(DecimalConstOneField, t_decimal);
            }
        }
        else
        {
            // Convert from unsigned to signed.  To do this, store into a
            // larger data type (this won't do sign extension), and then set the sign
            //
            __int64 value = ptree->AsDecimalConstantExpression().Value.Lo32;

            if (Negative)
            {
                value = -value;
            }

            // generate an 8 byte int
            GenerateLiteralInt(value, true);

            // call the Decimal constructor that takes an 8 byte int
            GenerateCallToRuntimeHelper(DecimalFromInt64Member, GetSymbolForVtype(t_decimal), &ptree->Loc); // System.Decimal::Ctor() Dim d as decimal = 13@
       }
    }
    else
    {
        // Looks like we have to do it the hard way
        // Emit all parts of the value, including Sign info and Scale info
        //
        BCSYM* i4Symbol = m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_i4);
        EmitOpcode(CEE_LDC_I4, i4Symbol);
        EmitData((char *)&(ptree->AsDecimalConstantExpression().Value.Lo32), 4);
        EmitOpcode(CEE_LDC_I4, i4Symbol);
        EmitData((char *)&(ptree->AsDecimalConstantExpression().Value.Mid32), 4);
        EmitOpcode(CEE_LDC_I4, i4Symbol);
        EmitData((char *)&(ptree->AsDecimalConstantExpression().Value.Hi32), 4);

        // Sign info
        EmitOpcode(CEE_LDC_I4, i4Symbol);
        EmitData((char *)&Negative, 4);

        // Scale info
        EmitOpcode(CEE_LDC_I4, i4Symbol);
        long temp = ptree->AsDecimalConstantExpression().Value.scale;
        EmitData((char *)&temp, 4);    // this is why we store Scale in a long
        GenerateCallToRuntimeHelper(DecimalFromConstantMember, GetSymbolForVtype(t_decimal), &ptree->Loc); // Dim d As Decimal = 13@
    }
}

//========================================================================
// Generate a constant floating point
//========================================================================

void CodeGenerator::GenerateLiteralFp
(
double dblVal,
Vtypes vtype
)
{
    float sngVal;

    switch (vtype)
    {
    case t_single:
        sngVal = (float)dblVal;
        EmitOpcode_U4(CEE_LDC_R4, *(long *)&sngVal, GetSymbolForVtype(t_single));
        break;

    case t_double:
        EmitOpcode(CEE_LDC_R8, GetSymbolForVtype(t_double));
        EmitData((char *)&dblVal, 8);
        break;

    default:
        VSFAIL("GenerateLiteralFp: Unsupported type of Fp.");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }
}

//========================================================================
// Generate a zero value
//========================================================================

void CodeGenerator::GenerateZeroConst
(
    Vtypes vtype
)
{
    switch (vtype)
    {
    case t_bool:
    case t_i1:
    case t_ui1:
    case t_i2:
    case t_ui2:
    case t_i4:
    case t_ui4:
    case t_char:
        GenerateLiteralInt(0);
        break;

    case t_i8:
    case t_ui8:
        GenerateLiteralInt(0, true);
        break;

    case t_double:
    case t_single:
        GenerateLiteralFp(0, vtype);
        break;

    default:
        VSFAIL("GenerateZeroConst: unrecoginzed type of 0 const.");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }
}


/*========================================================================

Load/store helpers

========================================================================*/

//========================================================================
// Determine what kind of addressing mode a tree requires
//========================================================================

void CodeGenerator::GenerateAddr
(
ILTree::PILNode    ptree,
ADDRMODE *  paddr,
bool        fReadOnlyAddr
)
{
    VSASSERT(ptree != NULL, "GenerateAddr: must have expr");

    switch (ptree->bilop)
    {
    case SX_SYM:
        {
            BCSYM_NamedRoot * pnamed = ptree->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PNamedRoot();
            BCSYM_Variable * pmem;
            BCSYM * ptyp;

            // address of function is handled during generation of delegate constructor call
            VSASSERT(!pnamed->IsProc(), "shouldn't have a proc symbol here");

            pmem = pnamed->PVariable();
            ptyp = pmem->GetType();

            if (ptree->AsSymbolReferenceExpression().ptreeQual != NULL)
            {
                // Load the qualifier
                if (pmem->IsShared())
                {
                    paddr->addrkind = ADDR_StaticField;
                }
                else
                {
                    GenerateRvalue(ptree->AsSymbolReferenceExpression().ptreeQual);
                    paddr->addrkind  = ADDR_VirtualField;
                }

                paddr->memref = m_pmdemit->DefineMemberRefBySymbol(pmem, ptree->AsSymbolReferenceExpression().GenericBindingContext, &ptree->Loc);
            }
            else if (pmem->GetVarkind() == VAR_Param &&
                     pmem->IsMe())
            {
                VSASSERT(pmem->GetLocalSlot() == 0,
                           "GenerateAddr: this ptr always has index 0.");

                paddr->addrkind = ADDR_Me;
                paddr->uIndex   = 0;
            }
            // Sigh, an egregious hack. Decimal constants have to be expressed as static
            // initonly fields that are intialized to a value. Rather than try and have
            // the entire compiler understand they they aren't "really" constants, we have
            // do do some faking, like here.
            else if (pmem->IsShared() ||
                     pmem->GetVarkind() == VAR_Const)
            {
                paddr->addrkind  = ADDR_StaticField;
                paddr->memref = m_pmdemit->DefineMemberRefBySymbol(pmem, ptree->AsSymbolReferenceExpression().GenericBindingContext, &ptree->Loc);
            }
            else
            {
                if (pmem->GetVarkind() == VAR_Param)
                {
                    if (pmem->GetCompilerType()->IsPointerType())
                    {
                        paddr->addrkind = ADDR_Byref;
                    }
                    else
                    {
                        paddr->addrkind = ADDR_Byval;
                    }
                }
                else
                {
                    if (pmem->IsStatic())
                    {
                        if (pmem->IsShared())
                        {
                            // Static locals are really static fields on the class
                            paddr->addrkind = ADDR_StaticField;
                            paddr->memref = m_pmdemit->DefineMemberRefBySymbol(pmem, ptree->AsSymbolReferenceExpression().GenericBindingContext, &ptree->Loc);
                        }
                        else
                        {   // Static locals are really fields on the class
                            
                            if (pmem->GetType()->GetVtype() == t_struct)
                            {
                                // If the type in question is a value type,
                                // then LDARG_0 actually pushes a managed
                                // pointer.
                                EmitOpcode(CEE_LDARG_0, UNSPILLABLE_TYPE_SYMBOL);
                            }
                            else
                            {
                                EmitOpcode(CEE_LDARG_0, pmem->GetType());
                            }
                            paddr->addrkind = ADDR_VirtualField;
                            paddr->memref = m_pmdemit->DefineMemberRefBySymbol(pmem, ptree->AsSymbolReferenceExpression().GenericBindingContext, &ptree->Loc);
                        }
                        break;
                    }
                    else
                    {
                        paddr->addrkind  = ADDR_Local;
                    }
                }

                paddr->uIndex    = pmem->GetLocalSlot();

                if (paddr->addrkind == ADDR_Byref)
                {
                    VSASSERT(ptyp->IsPointerType(), "byref is ptr");

                    // Special case: We have to generate the address here. This is because the
                    // indirect store opcode takes the address first and value second. So the
                    // load here has to be on the address generation, not on the load/store
                    // generation.
                    GenerateLoadParam(paddr->uIndex, ptyp);
                }
            }
        }
        break;

    case SX_INDEX:
        {
            GenerateIndex(ptree, paddr);

            // Now check if we are in the special case of an array of complex valuetypes.
            // If so, then the address to store and load to this array is the array element itself,
            // but only if it's a simple array (otherwise we use helpers).
            if (IsComplexValueType(ptree->AsExpression().vtype) && IsSimpleArray(ptree) &&
                ptree->AsExpression().vtype != t_generic)     // Bug VSWhidbey 545519
            {
                GenerateLoadArrayAddr(ptree, fReadOnlyAddr);
            }
        }
        break;

    default:
        VSFAIL("GenerateAddr: unexpected node type");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }
} // GenerateAddr

//========================================================================
// Load the address of a local
//========================================================================

void CodeGenerator::GenerateLoadLocalAddr
(
unsigned uIndex
)
{
    if (m_currentHydrationContext != NULL)
    {
        unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());

        if (SMALL_INDEX(uIndex))
        {
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                       CEE_LDLOCA_S,
                                                                       uIndex));
        }
        else
        {
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                       CEE_LDLOCA,
                                                                       uIndex));
        }
    }
    else
    {
        if (SMALL_INDEX(uIndex))
        {
            EmitOpcode_U1(CEE_LDLOCA_S, uIndex, UNSPILLABLE_TYPE_SYMBOL);
        }
        else
        {
            EmitOpcode_U2(CEE_LDLOCA, uIndex, UNSPILLABLE_TYPE_SYMBOL);
        }
    }
}

//========================================================================
// Load the address of a parameter
//========================================================================

void CodeGenerator::GenerateLoadParamAddr
(
unsigned uIndex
)
{
    if (m_currentHydrationContext != NULL)
    {
        unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());

        if (SMALL_INDEX(uIndex))
        {
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                       CEE_LDARGA_S,
                                                                       uIndex));
        }
        else
        {
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                       CEE_LDARGA,
                                                                       uIndex));
        }
    }
    else
    {
        if (SMALL_INDEX(uIndex))
        {
            EmitOpcode_U1(CEE_LDARGA_S, uIndex, UNSPILLABLE_TYPE_SYMBOL);
        }
        else
        {
            EmitOpcode_U2(CEE_LDARGA, uIndex, UNSPILLABLE_TYPE_SYMBOL);
        }
    }
}

//========================================================================
// Load the address of an element of an array
//========================================================================

void CodeGenerator::GenerateLoadArrayAddr
(
ILTree::PILNode ptreeLHS,
bool     fReadOnlyAddr
)
{
    if (m_currentHydrationContext == NULL)
    {
        // on entry           ; <a,i>
        // Normally this routine just emits the "LDELEMA" opcode:
        // LDELEMA            ; <&>
        GenerateLoadArrayAddrInternal(ptreeLHS, fReadOnlyAddr);
        return;
    }

    // If we're in dehydration mode, we have to do a "sacrificial" LDELEMA just to observe
    // any side-effects. e.g. 
    //    M(ref a[i], g(), await t)
    // The a[i] might generate a NullReferenceException or ArrayBoundsException, and if it does
    // then it should do so before invoking g(). The problem is that, since we're in dehydration
    // mode, we're merely pushing "a" and "i" onto the stack.
    //
    // Solution: we will do a "sacrificial LDELEMA" to evaluate &a[i] and flush out any exceptions,
    // and then discard the result. We'll leave "a" and "i" on the stack to be rehydrated later.
    // Note: correctness of this relies on the fact that "a" and "i" themselves are rvalues,
    // i.e. they are the top two values on the evaluation stack (rather than being stored on there
    // in dehydrated form).
    //
    // on entry           ; <a,i>
    // ... allocate short-lived temp
    // STLOC temp         ; <a>
    // DUP                ; <a,a>
    // LDLOC temp         ; <a,a,i>
    // LDELEMA            ; <a,&>  -- may produce exception
    // POP                ; <a>
    // LDLOC temp         ; <a,i>
    // ... deallocate short-lived temp
    // ... push hydration instructions
   
    // (if it were a multiply-dimensioned array, a[i,j] => <a,i,j>, then we need several short-lived temps)
    signed cDims = ptreeLHS->AsIndexExpression().DimensionCount;
    BCSYM_Variable** temps = (BCSYM_Variable**)m_pnra->Alloc(VBMath::Multiply(cDims, sizeof(BCSYM_Variable*)));

    for (signed i=cDims-1; i>=0; i--)
    {
        temps[i] = CreateCodeGenTemporary(m_stackTypes.Peek());
        GenerateStoreLocal(temps[i]->GetLocalSlot()); // will pop it from m_stackTypes
    }
    EmitOpcode(CEE_DUP, m_stackTypes.Peek());
    for (signed i=0; i<cDims; i++)
    {
        GenerateLoadLocal(temps[i]->GetLocalSlot(), temps[i]->GetType());
    }
    {
        BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);
        m_currentHydrationContext = NULL;
        GenerateLoadArrayAddrInternal(ptreeLHS, fReadOnlyAddr);
    }
    EmitOpcode(CEE_POP, Symbols::GetVoidType());
    for (signed i=0; i<cDims; i++)
    {
        GenerateLoadLocal(temps[i]->GetLocalSlot(), temps[i]->GetType());
        m_pCodeGenTemporaryManager->FreeTemporary(temps[i]);
    }
    GenerateLoadArrayAddrInternal(ptreeLHS, fReadOnlyAddr);     
}


void CodeGenerator::GenerateLoadArrayAddrInternal
(
ILTree::PILNode ptreeLHS,
bool     fReadOnlyAddr
)
{
    BCSYM_ArrayType * parr = ptreeLHS->AsIndexExpression().Left->ResultType->PArrayType();
    Vtypes vtype = ptreeLHS->AsExpression().vtype;
    signed cDims = ptreeLHS->AsIndexExpression().DimensionCount;

    if (fReadOnlyAddr)
    {
        // Emit a readonly prefix before the address. This
        // is required for the array load address when the
        // resulting address is used as the base expression
        // for a constrained call i.e. a call through a type
        // param.
        //
        VSASSERT(parr->GetRoot()->IsGenericParam() || vtype == t_struct,
                        "Readonly IL prefix requirement for non-type param access unexpected!!!");
        if (parr->GetRoot()->IsGenericParam())
        {
            if (m_currentHydrationContext != NULL)
            {
                unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
                m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                           CEE_READONLY,
                                                                           0 /* opCodeArg */));
            }
            else
            {
                EmitOpcode(CEE_READONLY, Symbols::GetVoidType());
            }
        }
    }

    // If the array is one dimension, we can use ldelem
    // Otherwise, we have to call the helper.
    //
    if (IsSimpleArray(ptreeLHS))
    {
        BCSYM * psymType = parr->GetRoot();
        mdTypeRef tr;

        tr = m_pmdemit->DefineTypeRefBySymbol(psymType, &ptreeLHS->Loc);
        if (m_currentHydrationContext != NULL)
        {
            unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                       CEE_LDELEMA,
                                                                       tr));
        }
        else
        {
            EmitOpcode_Tok(CEE_LDELEMA, tr, UNSPILLABLE_TYPE_SYMBOL);
        }
    }
    else
    {
        mdMemberRef       memref;

        memref = m_pmdemit->DefineArrayLoadAddrRef(parr, cDims, &ptreeLHS->Loc);
        
        if (m_currentHydrationContext != NULL)
        {
            unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                       CEE_CALLVIRT,
                                                                       memref,
                                                                       cDims + 1));
        }
        else
        {
            // Pop the arguments
            AddToCurStack(-cDims);
            for (int i = 0; i < cDims; i++)
            {
                m_stackTypes.PopOrDequeue();
            }

            // Pop the object
            AddToCurStack(-1);
            m_stackTypes.PopOrDequeue();

            // Emit the opcode and push the result
            EmitOpcode_Tok(CEE_CALLVIRT, memref, UNSPILLABLE_TYPE_SYMBOL);
            AddToCurStack(1);
        }
    }
}

//========================================================================
// Load a local
//========================================================================

void CodeGenerator::GenerateLoadLocal
(
unsigned uIndex,
BCSYM* pType
)
{
    if (INLINE_INDEX(uIndex))
    {
        EmitOpcode((opcode_t)(CEE_LDLOC_0 + uIndex), pType);
    }
    else if (SMALL_INDEX(uIndex))
    {
        EmitOpcode_U1(CEE_LDLOC_S, uIndex, pType);
    }
    else
    {
        EmitOpcode_U2(CEE_LDLOC, uIndex, pType);
    }
}

//========================================================================
// Load a parameter
//========================================================================

void CodeGenerator::GenerateLoadParam
(
unsigned uIndex,
BCSYM* pType
)
{
    if (m_currentHydrationContext != NULL &&
        pType->IsPointerType())
    {
        // We should never be able to get here. If we do, it indicates that
        // we're trying to access a byref parameter from within an expression
        // that contains an Await operator. Since Async methods cannot take
        // byref parameters, this should be impossible.
        //
        // On the other hand, if it turns out we do need to handle this for
        // some reason, the comment code below is included for completeness.
        VSFAIL("How are we accessing a byref parameter in a async method?");
        VbThrow(HrMake(ERRID_CodegenError));

        /*
        OPCODE deferredOpcode;
        int opcodeArgument = 0;

        if (INLINE_INDEX(uIndex))
        {
            deferredOpcode = (opcode_t)(CEE_LDARG_0 + uIndex);
        }
        else if (SMALL_INDEX(uIndex))
        {
            deferredOpcode = CEE_LDARG_S;
            opcodeArgument = uIndex;
        }
        else
        {
            deferredOpcode = CEE_LDARG;
            opcodeArgument = uIndex;
        }

        unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
        m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth, deferredOpcode, opcodeArgument));
        */
    }
    else
    {
        if (INLINE_INDEX(uIndex))
        {
            EmitOpcode((opcode_t)(CEE_LDARG_0 + uIndex), pType);
        }
        else if (SMALL_INDEX(uIndex))
        {
            EmitOpcode_U1(CEE_LDARG_S, uIndex, pType);
        }
        else
        {
            EmitOpcode_U2(CEE_LDARG, uIndex, pType);
        }
    }
}

//========================================================================
// Load an element of an array
//========================================================================

void CodeGenerator::GenerateLoadArray
(
ILTree::PILNode ptree
)
{
    VSASSERT( ptree->bilop == SX_INDEX, "GenerateLoadArray: SX_INDEX node expected" );

    BCSYM_ArrayType * parr = ptree->AsIndexExpression().Left->ResultType->PArrayType();
    Vtypes            vtype = ptree->AsExpression().vtype;
    signed            cDims = ptree->AsIndexExpression().DimensionCount;

    // If the array is one dimensional, we can use the opcode.
    // Otherwise, we have to call the helper.
    //
    if (IsSimpleArray(ptree))
    {
        if (vtype == t_generic) // Bug VSWhidbey 545519
        {
            EmitOpcode_Tok(CEE_LDELEM,
                           m_pmdemit->DefineTypeRefBySymbol(parr->GetRoot(), &ptree->Loc),
                           parr->GetRoot());
        }
        else if (IsComplexValueType(vtype))
        {
            // the address of the element is already on the stack, so we can do a direct load
            GenerateLoadStructFromAddr(parr->GetRoot(), &ptree->Loc);
        }
        else
        {
            OPCODE  opcode = MapType2Opcode_ArrayLoad(vtype);

            VSASSERT(opcode != CEE_NOP, "GenerateLoadArray: unexpected vtype");
            EmitOpcode(opcode, parr->GetRoot());
        }
    }
    else
    {
        mdMemberRef       memref;

        memref = m_pmdemit->DefineArrayLoadRef(parr, cDims, &ptree->Loc);
        
        // Pop the arguments off the stack
        AddToCurStack(-cDims);
        for (int i = 0; i < cDims; i++)
        {
            m_stackTypes.PopOrDequeue();
        }
        
        // Pop the object off the stack
        AddToCurStack(-1);
        m_stackTypes.PopOrDequeue();

        // Emit CALLVIRT. The table for CALLVIRT defines no pushes or pops,
        // as these can be variable. Hence we're handling this all ourselves.
        EmitOpcode_Tok(CEE_CALLVIRT, memref, parr->GetRoot());
        
        // Push the resulting item onto the stack
        AddToCurStack(1);
    }
}


void CodeGenerator::GenerateLoadOrElseLoadLocal(_In_opt_ ILTree::SymbolReferenceExpression *symref, _In_ BCSYM_Variable *local)
{
    // This function is to generalize over cases (like SB_FOR) where something might be
    // stored as a Variable, or (if it was lifted) as a SymbolReferenceExpression.
    // If symref is non-NULL then it generates a load from that symref.
    // Otherwise, it generates a load from the local.

    VSASSERT(local != NULL, "Must supply either a local to load from, or both a local and a symref");
    if (symref != NULL)
    {
        ADDRMODE addrmode;
        GenerateAddr(symref, &addrmode);
        GenerateLoad(symref, &addrmode);
    }
    else
    {
        GenerateLoadLocal(local->GetLocalSlot(), local->GetType());
    }
}


void CodeGenerator::GenerateStoreOrElseStoreLocal(_In_opt_ ILTree::SymbolReferenceExpression *symref, _In_ BCSYM_Variable *local)
{
    // As above, but for stores. If symref is non-NULL then it generates a store into that symref.
    // Otherwise it generates a store to the local.

    VSASSERT(local != NULL, "Must supply either a local to store to, or both a local and a symref");
    if (symref != NULL)
    {
        // We start the catch block with v on the stack.
        // Goal is to do "this.field = v" or similar
        //              [v]
        // STLOC temp   []
        // LDARG 0      [this]
        // LDLOC temp   [this,v]
        // STFLD f      []                               
        BCSYM_Variable *pTemp = CreateCodeGenTemporary(m_stackTypes.Peek());
        GenerateStoreLocal(pTemp->GetLocalSlot());
        ADDRMODE addrmode;
        GenerateAddr(symref, &addrmode);
        GenerateLoadLocal(pTemp->GetLocalSlot(), pTemp->GetType());
        GenerateStore(symref, &addrmode);
    }
    else
    {
        GenerateStoreLocal(local->GetLocalSlot());
    }
}


//========================================================================
// Store a local
//========================================================================

void CodeGenerator::GenerateStoreLocal
(
unsigned uIndex
)
{
    if (INLINE_INDEX(uIndex))
    {
        EmitOpcode((opcode_t)(CEE_STLOC_0 + uIndex), Symbols::GetVoidType());
    }
    else if (SMALL_INDEX(uIndex))
    {
        EmitOpcode_U1(CEE_STLOC_S, uIndex, Symbols::GetVoidType());
    }
    else
    {
        EmitOpcode_U2(CEE_STLOC, uIndex, Symbols::GetVoidType());
    }
}

//========================================================================
// Store a parameter
//========================================================================

void CodeGenerator::GenerateStoreParam
(
unsigned uIndex
)
{
    if (SMALL_INDEX(uIndex))
    {
        EmitOpcode_U1(CEE_STARG_S, uIndex, Symbols::GetVoidType());
    }
    else
    {
        EmitOpcode_U2(CEE_STARG, uIndex, Symbols::GetVoidType());
    }
}

//========================================================================
// Generate a store of a complex valuetype
//========================================================================

void CodeGenerator::GenerateStoreComplexValueType
(
BCSYM * ptyp,
Location *pReferencingLocation
)
{
    Vtypes vtype = ptyp->GetVtype();
    mdTypeRef typref;


    VSASSERT( IsComplexValueType(vtype), "GenerateStoreComplexValueType: unexpected vtype" );
    VSASSERT( MapType2Opcode_ByrefStore(vtype) == CEE_STOBJ &&
               (MapType2Opcode_ArrayStore(vtype) == CEE_STOBJ || MapType2Opcode_ArrayStore(vtype) == CEE_STELEM),
               "GenerateStoreComplexValueType: unexpected opcode" );

    // for structs, we use the type symbol
    if (vtype == t_struct || vtype == t_generic)
    {
        typref = m_pmdemit->DefineTypeRefBySymbol(ptyp, pReferencingLocation);
    }
    else
    {
        typref = m_pmdemit->DefineTypeRefBySymbol(
            m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(vtype),
            pReferencingLocation);
    }

    EmitOpcode_Tok(CEE_STOBJ, typref, Symbols::GetVoidType());
}

//========================================================================
// Store into a byref
//========================================================================

void CodeGenerator::GenerateStoreByRef
(
BCSYM * ptyp,
Location *pReferencingLocation
)
{
    Vtypes vtype = ptyp->GetVtype();

    // If it's a value type, we have to tell how to copy it
    if (IsComplexValueType(vtype))
    {
        GenerateStoreComplexValueType(ptyp, pReferencingLocation);
    }
    else
    {
        OPCODE  opcode = MapType2Opcode_ByrefStore(vtype);

        VSASSERT(opcode != CEE_NOP, "GenerateStoreByRef: unexpected vtype");
        EmitOpcode(opcode, Symbols::GetVoidType());
    }
}

//========================================================================
// Store an element of an array
//========================================================================

void CodeGenerator::GenerateStoreArray
(
ILTree::PILNode ptree,
Location *pReferencingLocation
)
{
    VSASSERT( ptree->bilop == SX_INDEX, "GenerateStoreArray: SX_INDEX node expected" );

    BCSYM_ArrayType * parr = ptree->AsIndexExpression().Left->ResultType->PArrayType();
    Vtypes            vtype = ptree->AsExpression().vtype;
    signed            cDims = ptree->AsIndexExpression().DimensionCount;

    // If the array is one dimensional, we can use the opcode.
    // Otherwise, we have to call the helper.
    //
    if (IsSimpleArray(ptree))
    {
        if (vtype == t_generic) // Bug VSWhidbey 545519
        {
            EmitOpcode_Tok(CEE_STELEM,
                           m_pmdemit->DefineTypeRefBySymbol(parr->GetRoot(), pReferencingLocation),
                           Symbols::GetVoidType());
        }
        else if (IsComplexValueType(vtype))
        {
            // the address of the element is already on the stack, so we can do a direct store
            GenerateStoreComplexValueType(parr->GetRoot(), pReferencingLocation);
        }
        else
        {
            OPCODE  opcode = MapType2Opcode_ArrayStore(vtype);

            VSASSERT(opcode != CEE_NOP, "GenerateStoreArray: unexpected vtype");
            EmitOpcode(opcode, Symbols::GetVoidType());  // No value pushed.
            if (opcode == CEE_STELEM_REF)
            {
                InsertNOP();
            }
        }
    }
    else
    {
        mdMemberRef memref;

        memref = m_pmdemit->DefineArrayStoreRef(parr, cDims, &ptree->Loc);
        EmitOpcode_Tok(CEE_CALLVIRT, memref, Symbols::GetVoidType()); // No value pushed.
        AddToCurStack(-cDims);
        AddToCurStack(-2);
        for (int i = 0; i < cDims + 2; i++)
        {
            m_stackTypes.PopOrDequeue();
        }
    }
}

//========================================================================
// Store a value of a particular type
//========================================================================

void CodeGenerator::GenerateStoreType
(
BCSYM    *  ptyp,
ADDRMODE *  paddrmode,
Location *  pReferencingLocation
)
{
    switch (paddrmode->addrkind)
    {
    case ADDR_Local:
        GenerateStoreLocal(paddrmode->uIndex);
        break;

    case ADDR_Byval:
        GenerateStoreParam(paddrmode->uIndex);
        break;

    case ADDR_VirtualField:
        EmitOpcode_Tok(CEE_STFLD, paddrmode->memref, Symbols::GetVoidType());
        break;

    case ADDR_StaticField:
        EmitOpcode_Tok(CEE_STSFLD, paddrmode->memref, Symbols::GetVoidType());
        break;

    case ADDR_Byref:
        GenerateStoreByRef(ptyp, pReferencingLocation);
        break;

    case ADDR_Array:
        // we handle array stores in GenerateStoreArray
        VSFAIL("GenerateStoreType: ADDR_Array addressing mode not handled here");
        VbThrow(HrMake(ERRID_CodegenError));
        break;

    default:
        VSFAIL("GenerateStoreType: unhandled addressing kind.");
        VbThrow(HrMake(ERRID_CodegenError));
        break;

    } // switch addrkind
}

//========================================================================
// Store a value
//========================================================================

void CodeGenerator::GenerateStore
(
ILTree::PILNode    ptreeLHS,
ADDRMODE *  paddrmode
)
{
    // GenerateStoreType takes only a bcsym, thus array-mode needs its own special function
    if (paddrmode->addrkind == ADDR_Array)
    {
        GenerateStoreArray(ptreeLHS, &ptreeLHS->Loc);
    }
    else
    {
        GenerateStoreType(ptreeLHS->AsExpression().ResultType, paddrmode, &ptreeLHS->Loc);
    }
}

//========================================================================
// Load a complex valuetype
//========================================================================

void CodeGenerator::GenerateLoadStructFromAddr
(
BCSYM * ptyp,
Location *pReferencingLocation
)
{
    Vtypes vtype = ptyp->GetVtype();
    mdTypeRef typref;


    VSASSERT( IsComplexValueType(vtype), "GenerateLoadStructFromAddr: unexpected vtype" );
    VSASSERT( MapType2Opcode_ByrefLoad(vtype) == CEE_LDOBJ &&
               (MapType2Opcode_ArrayLoad(vtype) == CEE_LDOBJ || MapType2Opcode_ArrayLoad(vtype) == CEE_LDELEM),
               "GenerateLoadStructFromAddr: unexpected opcode" );

    // for structs, we use the type symbol
    if (vtype == t_struct || vtype == t_generic)
    {
        typref = m_pmdemit->DefineTypeRefBySymbol(ptyp, pReferencingLocation);
    }
    else
    {
        typref = m_pmdemit->DefineTypeRefBySymbol(
            m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(vtype),
            pReferencingLocation);
    }

    EmitOpcode_Tok(CEE_LDOBJ, typref, ptyp);
}

//========================================================================
// Load into a byref
//========================================================================

void CodeGenerator::GenerateLoadByRef
(
BCSYM * ptyp,
Location *pReferencingLocation
)
{
    Vtypes vtype = ptyp->GetVtype();

    // If it's a value type, we have to tell how to load it
    if (IsComplexValueType(vtype))
    {
        GenerateLoadStructFromAddr(ptyp, pReferencingLocation);
    }
    else
    {
        OPCODE  opcode = MapType2Opcode_ByrefLoad(vtype);

        VSASSERT(opcode != CEE_NOP, "GenerateLoadByRef: unexpected vtype");
        EmitOpcode(opcode, ptyp);
    }
}

//========================================================================
// Load a value
//========================================================================

void CodeGenerator::GenerateLoad
(
ILTree::PILNode    ptreeLHS,
ADDRMODE *  paddrmode
)
{
    switch (paddrmode->addrkind)
    {
    case ADDR_Me:
        if (m_currentHydrationContext != NULL)
        {
            unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
            m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth, CEE_LDARG_0, 0 /* opCodeArg */));
        }
        else
        {
            EmitOpcode(CEE_LDARG_0, ptreeLHS->AsExpression().ResultType);
        }
        break;

    case ADDR_Local:
        GenerateLoadLocal(paddrmode->uIndex, ptreeLHS->AsExpression().ResultType);
        break;

    case ADDR_Byval:
        GenerateLoadParam(paddrmode->uIndex, ptreeLHS->AsExpression().ResultType);
        break;

    case ADDR_Array:
        GenerateLoadArray(ptreeLHS);
        break;

    case ADDR_VirtualField:
        EmitOpcode_Tok(CEE_LDFLD, paddrmode->memref, ptreeLHS->AsExpression().ResultType);
        break;

    case ADDR_StaticField:
        EmitOpcode_Tok(CEE_LDSFLD, paddrmode->memref, ptreeLHS->AsExpression().ResultType);
        break;

    case ADDR_Byref:
        GenerateLoadByRef(ptreeLHS->AsExpression().ResultType, &ptreeLHS->Loc);
        break;

    default:
        VSFAIL("GenerateLoad: Unexpected addressing mode");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }
}

//========================================================================
// Load the address of a value
//========================================================================

void CodeGenerator::GenerateLoadAddr
(
ILTree::PILNode ptreeLHS,
ADDRMODE * paddrmode,
bool     fReadOnlyAddr
)
{
    switch (paddrmode->addrkind)
    {
    case ADDR_Local:
        GenerateLoadLocalAddr(paddrmode->uIndex);
        break;

    case ADDR_Byval:
        GenerateLoadParamAddr(paddrmode->uIndex);
        break;

    case ADDR_Array:
        // if i have a struct and a simple array, then skip loading the address of the
        // element on the stack because GenerateAddr already did this for us
        if (!IsComplexValueType(ptreeLHS->AsExpression().vtype) || !IsSimpleArray(ptreeLHS) ||
            ptreeLHS->AsExpression().vtype == t_generic)      // Bug VSWhidbey 545519
        {
            GenerateLoadArrayAddr(ptreeLHS, fReadOnlyAddr);
        }
        break;

    case ADDR_VirtualField:
        {
            if (m_currentHydrationContext != NULL)
            {
                // In evaluating things like "M(ref v.f, g(), await t)", the order-of-evaluation
                // semantics say that any null-reference-exception arising from v.f will be thrown
                // before g() is executed. But since we're in dehydrated mode, we typically just
                // push "v" onto the stack so that ".f" can be rehydrated from it later.
                //
                // Solution: we will evaluate a sacrificial "LDFLDA v.f" early just to flush out
                // any NullReferenceException. Then will discard the result, leaving "v" on the stack
                // to be rehydrated later.
                //
                //                ; <v>
                // DUP            ; <v,v>
                // CEE_LDFLDA(f)  ; <v,&>  -- to shake out any NullReferenceExceptions
                // POP            ; <v>
                //
                // The correctness of this is subtle... ADDR_VirtualField can come from things like
                // (1) ref.v or (2) struct.v or (3) struct.struct.struct.v. In case (1)
                // the thing to the left is an rvalue, i.e. it will necessarily be fully hydrated
                // and "ref" will be the topmost thing on the stack. In case (2), the thing to the
                // left will be an lvalue i.e. a managed address. The only way a NULL managed
                // address could come into VB is if by passing a null managed address in ByRef from
                // some other language like C++/CLI. But async methods dont' accept ByRef parameters.
                // Therefore, the thing to the left will never be NULL, and we don't need to do the
                // sacrificial LDFLDA. Case (3) is similar.
                // 
                // So we'll only need to do this sacrificial LDFLDA in cases where the left hand
                // side is not a managed-address. This implies it will be on the evaluation stack,
                // ready for us to DUP... (i.e. it won't require any rehydration)

                if (ptreeLHS->AsSymbolReferenceExpression().BaseReference->bilop != SX_ADR)
                {
                    EmitOpcode(CEE_DUP, m_stackTypes.Peek());
                    EmitOpcode_Tok(CEE_LDFLDA, paddrmode->memref, UNSPILLABLE_TYPE_SYMBOL);
                    EmitOpcode(CEE_POP, Symbols::GetVoidType());
                }

                unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
                m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                           CEE_LDFLDA,
                                                                           paddrmode->memref));
            }
            else
            {
                EmitOpcode_Tok(CEE_LDFLDA, paddrmode->memref, UNSPILLABLE_TYPE_SYMBOL);
            }
        }
        break;

        case ADDR_StaticField:
        {
            if (m_currentHydrationContext != NULL)
            {
                unsigned short stackDepth = static_cast<unsigned short>(m_stackTypes.Count());
                m_currentHydrationContext->Add(new (*m_pnra) HydrationStep(stackDepth,
                                                                           CEE_LDSFLDA,
                                                                           paddrmode->memref));
            }
            else
            {
                EmitOpcode_Tok(CEE_LDSFLDA, paddrmode->memref, UNSPILLABLE_TYPE_SYMBOL);
            }
        }
        break;

    case ADDR_Byref:
        // Already handled in GenerateAddr
        break;

    case ADDR_Me:  // not allowed to load address of location that holds the Me pointer
    default:
        VSFAIL("GenerateLoadAddr: Unexpected addressing mode");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }
}

//========================================================================
// Generates code to duplicate an addressing value on the stack
//========================================================================

void CodeGenerator::GenerateDupLvalue
(
ILTree::PILNode   ptreeLHS,
ADDRMODE * paddrmode
)
{
    // Generate the appropriate dup followed by a load
    switch (paddrmode->addrkind)
    {
    case ADDR_Array:
        VSASSERT(ptreeLHS->bilop == SX_INDEX,
                   "better be an array!");

        GenerateIndex(ptreeLHS, paddrmode);
        break;

    case ADDR_VirtualField:
    case ADDR_Byref:
        BCSYM* pTypeOnTopOfStack = m_stackTypes.Peek();
        EmitOpcode(CEE_DUP, pTypeOnTopOfStack);
        break;
    }
}

/*========================================================================

Array expressions

========================================================================*/

//========================================================================
// Generate the indexes for an array operation
//========================================================================

void CodeGenerator::GenerateIndex
(
ILTree::PILNode ptreeIndex,
ADDRMODE *paddrmode
)
{
    ILTree::PILNode   ptreeArray = ptreeIndex->AsIndexExpression().Left;
    ILTree::PILNode   ptreeListCursor;
    unsigned   iDim;

    VSASSERT(ptreeArray, "GenerateIndex: need an array expression");

    // Special case. Because there is no special opcode for an
    // array store, we have to call a "pretend" function on the
    // array. To do that, though, we need the actual object to
    // call on.
    GenerateRvalue(ptreeArray);

    // Now load up the indices. Make sure to subtract from lower bound.
    // Traverse the list of SX_LIST nodes and generate the expressions
    //
    iDim = 0;
    ptreeListCursor = ptreeIndex->AsIndexExpression().Right;

    while (ptreeListCursor)
    {
        ILTree::PILNode ptreeDimExpr;

        VSASSERT(ptreeListCursor->bilop == SX_LIST,
                   "GenerateIndex: node in AsIndexExpression.ptreeList must be SX_LIST");

        ptreeDimExpr = ptreeListCursor->AsExpressionWithChildren().Left;

        GenerateRvalue(ptreeDimExpr);

        ptreeListCursor = ptreeListCursor->AsExpressionWithChildren().Right;
        iDim += 1;

    } // while

    paddrmode->addrkind = ADDR_Array;

    VSASSERT(iDim == ptreeIndex->AsIndexExpression().DimensionCount,
               "GenerateIndex: dimension mismatch.");
}

//========================================================================
// Generates code to construct an array
//========================================================================

void CodeGenerator::GenerateArrayConstruction
(
BCSYM_ArrayType * parray,
ILTree::PILNode pSizeList,
Location *pReferencingLocation
)
{
    VSASSERT(pSizeList->bilop == SX_LIST, "Malformed array size list.");
    GenerateRvalue(pSizeList);

    signed cDims = parray->GetRank();

    if (cDims == 1)
    {
        mdTypeRef tr = m_pmdemit->DefineTypeRefBySymbol(parray->GetRoot(), pReferencingLocation);
        EmitOpcode_Tok(CEE_NEWARR, tr, parray);
    }
    else
    {
        mdMemberRef memref = m_pmdemit->DefineArrayCtorRef(parray, pReferencingLocation);
        AddToCurStack(-cDims);
        for (int i = 0; i < cDims; i++)
        {
            m_stackTypes.PopOrDequeue();
        }
        EmitOpcode_Tok(CEE_NEWOBJ, memref, parray);
    }
}

/*========================================================================

Assignment expressions

========================================================================*/

//========================================================================
// Clone an expression's value for correct object semantics.
//========================================================================

void CodeGenerator::GenerateClone
(
ILTree::PILNode ptree
)
{
    // Only Object to Object assignment or arguments should be cloned.
    // No cloning should be done if we're building the runtime because that will ---- up the latebinding code.

    if (ptree->AsExpression().ResultType == 
        m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType) && !g_CompilingTheVBRuntime)
    {
        // There are a series of object operations which we know don't require a call to GetObjectValue.
        // These operations are math and logic operators.

        switch (ptree->bilop)
        {
        case SX_POW:
        case SX_NEG:
        case SX_PLUS:
        case SX_DIV:
        case SX_MOD:
        case SX_IDIV:
        case SX_CONC:
        case SX_NOT:
        case SX_AND:
        case SX_OR:
        case SX_XOR:
        case SX_MUL:
        case SX_ADD:
        case SX_SUB:
        case SX_SHIFT_LEFT:
        case SX_SHIFT_RIGHT:
            break;

        case SX_NOTHING:
            break;

        case SX_CTYPE:
        case SX_WIDE_COERCE:
            // A coerce to object implies a newly-boxed value type,
            // so don't do a copy because a copy is done implicitly while boxing.
            // A wide coerce implies a reference coercion to a more general reference type.
            // No need to do a copy in this case either because it won't contain a boxed value type.
            // Coercing to object is a nop for reference types, so make sure a coerce node doesn't exist.
            VSASSERT(
                ptree->AsExpression().bilop == SX_WIDE_COERCE ||
                ptree->AsExpressionWithChildren().Left->vtype != t_ref,
                "GenerateGetObjectValue: unexpected coercion to object");
            break;

        default:
            // Do correct object semantics. This call will do a shallow copy
            // if the object is a boxed value type. Otherwise, it will return the reference itself.
            GenerateCallToRuntimeHelper(GetObjectValue, GetSymbolForFXType(FX::ObjectType), &ptree->Loc);
            break;
        }
    }
}


//========================================================================
// Generate an assignment expression
//========================================================================

void CodeGenerator::GenerateAssignment
(
ILTree::PILNode ptree
)
{
    ADDRMODE    addrmode;
    unsigned    uFlags = ptree->uFlags;
    Vtypes      vtypeOp1;
    BCSYM     * ptypAsg;
    ILTree::PILNode    ptreeLHS = ptree->AsExpressionWithChildren().Left;
    ILTree::PILNode    ptreeRHS = ptree->AsExpressionWithChildren().Right;

    VSASSERT(ptree && (ptree->bilop == SX_ASG || ptree->bilop == SX_ASG_RESADR),
              "GenerateAssignment: must be an assignment node.");

    VSASSERT(ptreeLHS && ptreeRHS,
              "GenerateAssignment: must have Op1 and Op2.");

    BackupValue<Queue<HydrationStep*, NorlsAllocWrapper>*> hydrationContextBackup(&m_currentHydrationContext);
    m_currentHydrationContext = ptree->AsExpression().ContainsAsyncSpill ? new (*m_pnra) Queue<HydrationStep*, NorlsAllocWrapper>(m_nraWrapper) : NULL;

    vtypeOp1 = ptreeLHS->AsExpression().vtype;
    ptypAsg = ptreeLHS->AsExpression().ResultType;

    // Need to first load the "addr" part of the lvalue
    GenerateAddr(ptreeLHS, &addrmode);

    // Now load the RHS.
    GenerateRvalue(ptreeRHS);

    // If necessary, clone the RHS value for correct object semantics.
    if (!(ptree->uFlags & SXF_ASG_SUPPRESS_CLONE))
    {
        GenerateClone(ptreeRHS);
    }

    // OK, we've taken care of all the child expressions. Now, rehydrate any
    // managed pointers before we actually store the result.
    RehydrateManagedPointers(&ptree->Loc);

    GenerateStore(ptreeLHS, &addrmode);

    // if type of SX_ASG is not void, load up the LHS
    if (ptree->AsExpression().vtype != t_void)
    {
        if (ptree->bilop == SX_ASG)
        {
            // We are generating the loading of a value here - therefore no dehydration/rehydration
            // should take place. Null out the hydration context, which will be restored from the backup
            // at the end of this method.
            m_currentHydrationContext = NULL;
            GenerateAddr(ptreeLHS, &addrmode);
            GenerateLoad(ptreeLHS, &addrmode);
        }
        else
        {
            VSASSERT(ptree->bilop == SX_ASG_RESADR, "has to be one or the other!");

            // If we need the value of the SX_ASG_RESADR, then dehydration/rehydration needs
            // to take place in the context of the parent.
            hydrationContextBackup.Restore();
            GenerateAddr(ptreeLHS, &addrmode);
            GenerateLoadAddr(ptreeLHS, &addrmode);
        }
    }
}

//========================================================================
// Emit a CONV opcode to convert to a type
//========================================================================

#define Err  CEE_ILLEGAL
#define NOP  CEE_NOP

#define I1   CEE_CONV_I1
#define U1   CEE_CONV_U1
#define I2   CEE_CONV_I2
#define U2   CEE_CONV_U2
#define I4   CEE_CONV_I4
#define U4   CEE_CONV_U4
#define I8   CEE_CONV_I8
#define U8   CEE_CONV_U8
#define R4   CEE_CONV_R4
#define R8   CEE_CONV_R8

#define OI1  CEE_CONV_OVF_I1
#define OU1  CEE_CONV_OVF_U1
#define OI2  CEE_CONV_OVF_I2
#define OU2  CEE_CONV_OVF_U2
#define OI4  CEE_CONV_OVF_I4
#define OU4  CEE_CONV_OVF_U4
#define OI8  CEE_CONV_OVF_I8
#define OU8  CEE_CONV_OVF_U8

#define OI1U CEE_CONV_OVF_I1_UN
#define OU1U CEE_CONV_OVF_U1_UN
#define OI2U CEE_CONV_OVF_I2_UN
#define OU2U CEE_CONV_OVF_U2_UN
#define OI4U CEE_CONV_OVF_I4_UN
#define OU4U CEE_CONV_OVF_U4_UN
#define OI8U CEE_CONV_OVF_I8_UN
// #define OU8U CEE_CONV_OVF_U8_UN  - never used

const unsigned ConversionTableSize = t_date - t_bad + 1;

//                                     Target              Source
const OPCODE SimpleTypeConversionTable[ConversionTableSize][ConversionTableSize] =
{
// From->   bad    bool   i1     ui1    i2     ui2    i4     ui4    i8     ui8    dec    r4     r8
/* bad  */ {Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err },
/* bool */ {Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err },
/* i1   */ {Err ,  Err ,  Err ,  I1  ,  I1  ,  I1  ,  I1  ,  I1  ,  I1  ,  I1  ,  Err ,  I1  ,  I1  },
/* ui1  */ {Err ,  Err ,  U1  ,  Err ,  U1  ,  U1  ,  U1  ,  U1  ,  U1  ,  U1  ,  Err ,  U1  ,  U1  },
/* i2   */ {Err ,  Err ,  NOP ,  NOP ,  Err ,  I2  ,  I2  ,  I2  ,  I2  ,  I2  ,  Err ,  I2  ,  I2  },
/* ui2  */ {Err ,  Err ,  U2  ,  NOP ,  U2  ,  Err ,  U2  ,  U2  ,  U2  ,  U2  ,  Err ,  U2  ,  U2  },
/* i4   */ {Err ,  Err ,  NOP ,  NOP ,  NOP ,  NOP ,  Err ,  NOP ,  I4  ,  I4  ,  Err ,  I4  ,  I4  },
/* ui4  */ {Err ,  Err ,  NOP ,  NOP ,  NOP ,  NOP ,  NOP ,  Err ,  U4  ,  U4  ,  Err ,  U4  ,  U4  },
/* i8   */ {Err ,  Err ,  I8  ,  U8  ,  I8  ,  U8  ,  I8  ,  U8  ,  Err ,  NOP ,  Err ,  I8  ,  I8  },
/* ui8  */ {Err ,  Err ,  I8  ,  U8  ,  I8  ,  U8  ,  I8  ,  U8  ,  NOP ,  Err ,  Err ,  U8  ,  U8  },
/* dec  */ {Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err },
/* r4   */ {Err ,  Err ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  Err ,  R4  ,  R4  },
/* r8   */ {Err ,  Err ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  Err ,  R8  ,  R8  },
};

//                                             Target              Source
const OPCODE SimpleTypeOverflowConversionTable[ConversionTableSize][ConversionTableSize] =
{
// From->   bad    bool   i1     ui1    i2     ui2    i4     ui4    i8     ui8    dec    r4     r8
/* bad  */ {Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err },
/* bool */ {Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err },
/* i1   */ {Err ,  Err ,  Err ,  OI1U,  OI1 ,  OI1U,  OI1 ,  OI1U,  OI1 ,  OI1U,  Err ,  OI1 ,  OI1 },
/* ui1  */ {Err ,  Err ,  OU1 ,  Err ,  OU1 ,  OU1U,  OU1 ,  OU1U,  OU1 ,  OU1U,  Err ,  OU1 ,  OU1 },
/* i2   */ {Err ,  Err ,  NOP ,  NOP ,  Err ,  OI2U,  OI2 ,  OI2U,  OI2 ,  OI2U,  Err ,  OI2 ,  OI2 },
/* ui2  */ {Err ,  Err ,  OU2 ,  NOP ,  OU2 ,  Err ,  OU2 ,  OU2U,  OU2 ,  OU2U,  Err ,  OU2 ,  OU2 },
/* i4   */ {Err ,  Err ,  NOP ,  NOP ,  NOP ,  NOP ,  Err ,  OI4U,  OI4 ,  OI4U,  Err ,  OI4 ,  OI4 },
/* ui4  */ {Err ,  Err ,  OU4 ,  NOP ,  OU4 ,  NOP ,  OU4 ,  Err ,  OU4 ,  OU4U,  Err ,  OU4 ,  OU4 },
/* i8   */ {Err ,  Err ,  I8  ,  U8  ,  I8  ,  U8  ,  I8  ,  U8  ,  Err ,  OI8U,  Err ,  OI8 ,  OI8 },
/* ui8  */ {Err ,  Err ,  OU8 ,  U8  ,  OU8 ,  U8  ,  OU8 ,  U8  ,  OU8 ,  Err ,  Err ,  OU8 ,  OU8 },
/* dec  */ {Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err ,  Err },
/* r4   */ {Err ,  Err ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  R4  ,  Err ,  R4  ,  R4  },
/* r8   */ {Err ,  Err ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  R8  ,  Err ,  R8  ,  R8  },
};

#undef Err
#undef NOP

#undef I1
#undef U1
#undef I2
#undef U2
#undef I4
#undef U4
#undef I8
#undef U8
#undef R4
#undef R8

#undef OI1
#undef OU1
#undef OI2
#undef OU2
#undef OI4
#undef OU4
#undef OI8
#undef OU8

#undef OI1U
#undef OU1U
#undef OI2U
#undef OU2U
#undef OI4U
#undef OU4U
#undef OI8U
// #undef OU8U - never used

void CodeGenerator::GenerateConvertSimpleType
(
    Vtypes SourceType,
    Vtypes ResultType,
    bool PerformOverflowChecking,
    ILTree::PILNode ptree // [in] for location information
)
{
    VSASSERT(IsSimpleType(SourceType) && IsSimpleType(ResultType), "Expected simple types.");

    OPCODE opcode = CEE_ILLEGAL;

    int ResultIndex = ResultType - t_bad;
    int SourceIndex = SourceType - t_bad;

    if (ResultIndex >= 0 && ResultIndex < ConversionTableSize &&
        SourceIndex >= 0 && SourceIndex < ConversionTableSize)
    {
        opcode = PerformOverflowChecking ?
            SimpleTypeOverflowConversionTable[ResultIndex][SourceIndex] :
            SimpleTypeConversionTable[ResultIndex][SourceIndex];
    }

    VSASSERT(opcode != CEE_ILLEGAL, "invalid vtype");

    if (SourceType == t_single || SourceType == t_double)
    {
        if (IsIntegralType(ResultType))
        {
            if (SourceType == t_single)
            {
                GenerateConvertSimpleType(t_single, t_double, false, ptree);
            }
            GenerateCallToRuntimeHelper(NumericRoundMember, GetSymbolForVtype(t_double), &ptree->Loc); // Dim d3 As Double = 3.5 : Dim i As Integer = CInt(d3) -> [mscorlib]System.Math::Round(float64)
        }
    }
    else if (SourceType == t_ui4 || SourceType == t_ui8)
    {
        if (ResultType == t_single || ResultType == t_double)
        {
            EmitOpcode(CEE_CONV_R_UN, GetSymbolForVtype(ResultType));
        }
    }

    if (opcode != CEE_NOP)
    {
        EmitOpcode(opcode, GetSymbolForVtype(ResultType));
    }
}

//========================================================================
// Coerce an intrinsic number to another number
//========================================================================

void CodeGenerator::GenerateConvertIntrinsic
(
    ILTree::PILNode ptreeTo,
    ILTree::PILNode ptreeFrom,
    Vtypes vtypeTo,
    Vtypes vtypeFrom
)
{
    VSASSERT(IsSimpleType(vtypeTo) && IsSimpleType(vtypeFrom),
             "GenerateConvertIntrinsic: coercion must be between numbers");
    VSASSERT(vtypeTo != vtypeFrom || vtypeTo == t_single || vtypeTo == t_double,
             "GenerateConvertIntrinsic: shouldn't do coercion if non-floating types are the same");

    // Generate the expression to convert.
    GenerateRvalue(ptreeFrom);

    if (vtypeTo == t_bool)
    {
        GenerateZeroConst(ptreeFrom->AsExpression().vtype);

        // using cgt.un is optimal, but doesn't work in the case of floating point values
        if (vtypeFrom == t_single || vtypeFrom == t_double)
        {
            EmitOpcode(CEE_CEQ, GetSymbolForVtype(t_bool));
            GenerateLiteralInt(COMPLUS_FALSE);
            EmitOpcode(CEE_CEQ, GetSymbolForVtype(t_bool));
        }
        else
        {
            EmitOpcode(CEE_CGT_UN, GetSymbolForVtype(t_bool));
        }

        return;
    }

    if (vtypeFrom == t_bool)
    {
        // First, normalize to -1
        GenerateZeroConst(t_bool);
        EmitOpcode(CEE_CGT_UN, GetSymbolForVtype(t_bool));
        EmitOpcode(CEE_NEG, GetSymbolForVtype(t_i4));

        if (vtypeTo != t_i4)
        {
            // Convert to the target type, but don't do overflow checking.  This results in unsigned types
            // getting their max value.
            GenerateConvertSimpleType(t_i4, vtypeTo, false, ptreeFrom);
        }
        return;
    }

    if (vtypeFrom == t_double || vtypeFrom == t_single)
    {
        if (IsIntegralType(vtypeTo))
        {
            if (vtypeFrom == t_single)
            {
                // If converting from an intermediate value, we need to guarantee that
                // the intermediate value keeps the precision of its type.  The JIT will try to
                // promote the precision of intermediate values if it can, and this can lead to
                // incorrect results (VS#241243).

                switch (ptreeFrom->bilop)
                {
                case SX_ADD:
                case SX_SUB:
                case SX_MUL:
                case SX_DIV:
                case SX_MOD:
                case SX_POW:
                case SX_NEG:
                case SX_PLUS:
                    EmitOpcode(CEE_CONV_R4, GetSymbolForVtype(t_single));
                    break;

                default:
                    // no intermediate value so no need for the forced convert
                    break;
                }
            }
        }
    }

    GenerateConvertSimpleType(vtypeFrom, vtypeTo, !m_NoIntChecks, ptreeTo); // doesn't emit a runtime helper.  Emits a conversion opcode (e.g. Dim p as system.UIntPtr = 0 : dim i as integer = p, comes through here)
}

//========================================================================
// Do a non number-to-number coercion
//========================================================================
RuntimeMembers CodeGenerator::GetHelperFromRef
(
    Vtypes vtypeTo  // Type to convert to
)
{
    // This is used by the Expression Evaluator as well as CodeGen.  Keep that
    // in mind if changing this.

    switch (vtypeTo)
    {
    case t_bool:    return ObjectToBooleanMember;
    case t_i1:      return ObjectToSignedByteMember;
    case t_ui1:     return ObjectToByteMember;
    case t_i2:      return ObjectToShortMember;
    case t_ui2:     return ObjectToUnsignedShortMember;
    case t_i4:      return ObjectToIntegerMember;
    case t_ui4:     return ObjectToUnsignedIntegerMember;
    case t_i8:      return ObjectToLongMember;
    case t_ui8:     return ObjectToUnsignedLongMember;
    case t_decimal: return ObjectToDecimalMember;
    case t_single:  return ObjectToSingleMember;
    case t_double:  return ObjectToDoubleMember;
    case t_date:    return ObjectToDateMember;
    case t_char:    return ObjectToCharMember;
    case t_string:  return ObjectToStringMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

//========================================================================
// Do a string-to-number coercion
//========================================================================
RuntimeMembers CodeGenerator::GetHelperFromString
(
    Vtypes vtypeTo  // Type to convert to
)
{
    switch (vtypeTo)
    {
    case t_bool:    return StringToBooleanMember;
    case t_i1:      return StringToSignedByteMember;
    case t_ui1:     return StringToByteMember;
    case t_i2:      return StringToShortMember;
    case t_ui2:     return StringToUnsignedShortMember;
    case t_i4:      return StringToIntegerMember;
    case t_ui4:     return StringToUnsignedIntegerMember;
    case t_i8:      return StringToLongMember;
    case t_ui8:     return StringToUnsignedLongMember;
    case t_decimal: return StringToDecimalMember;
    case t_single:  return StringToSingleMember;
    case t_double:  return StringToDoubleMember;
    case t_date:    return StringToDateMember;
    case t_char:    return StringToCharMember;
    case t_array:   return StringToCharArrayMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

// Helper to determine which runtime call to make to convert to a string
RuntimeMembers CodeGenerator::GetHelperForStringCoerce
(
    Vtypes vtypeFrom  // Type to convert from
)
{
    switch (vtypeFrom)
    {
    case t_bool:    return BooleanToStringMember;
    case t_i1:
    case t_i2:
    case t_i4:      return IntegerToStringMember;
    case t_ui1:     return ByteToStringMember;
    case t_ui2:
    case t_ui4:     return UnsignedIntegerToStringMember;  // 
    case t_i8:      return LongToStringMember;
    case t_ui8:     return UnsignedLongToStringMember;
    case t_decimal: return DecimalToStringMember;
    case t_single:  return SingleToStringMember;
    case t_double:  return DoubleToStringMember;
    case t_date:    return DateToStringMember;
    case t_char:    return CharToStringMember;
    case t_ref:     return ObjectToStringMember;
    case t_array:   return CharArrayToStringMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

RuntimeMembers CodeGenerator::GetHelperToDecimal
(
    Vtypes vtypeFrom
)
{
    switch (vtypeFrom)
    {
    case t_bool:    return BooleanToDecimalMember;
    case t_i1:
    case t_ui1:
    case t_i2:
    case t_ui2:
    case t_i4:      return IntegerToDecimalMember;
    case t_ui4:     return UnsignedIntegerToDecimalMember;
    case t_i8:      return LongToDecimalMember;
    case t_ui8:     return UnsignedLongToDecimalMember;
    case t_single:  return SingleToDecimalMember;
    case t_double:  return DoubleToDecimalMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

RuntimeMembers CodeGenerator::GetHelperFromDecimal
(
    Vtypes vtypeTo
)
{
    switch (vtypeTo)
    {
    case t_bool:    return DecimalToBooleanMember;
    case t_i1:      return DecimalToSignedByteMember;
    case t_ui1:     return DecimalToByteMember;
    case t_i2:      return DecimalToShortMember;
    case t_ui2:     return DecimalToUnsignedShortMember;
    case t_i4:      return DecimalToIntegerMember;
    case t_ui4:     return DecimalToUnsignedIntegerMember;
    case t_i8:      return DecimalToLongMember;
    case t_ui8:     return DecimalToUnsignedLongMember;
    case t_single:  return DecimalToSingleMember;
    case t_double:  return DecimalToDoubleMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}


//========================================================================
// Do a non number-to-number coercion
//========================================================================
void CodeGenerator::GenerateConvertClass
(
ILTree::PILNode ptreeTo,
ILTree::PILNode ptreeFrom,
Vtypes vtypeTo,
Vtypes vtypeFrom
)
{
    RuntimeMembers rtHelper = UndefinedRuntimeMember;

    // Attempt to optimize the coercion.
    if (vtypeTo == t_bool && vtypeFrom == t_ref)
    {
        switch (ptreeFrom->bilop)
        {
        case SX_EQ:
        case SX_NE:
        case SX_LE:
        case SX_LT:
        case SX_GE:
        case SX_GT:
            // The result of the comparison is known to be boolean, so force it to be.
            ptreeFrom->AsExpression().vtype = t_bool;
            GenerateRelOp(ptreeFrom);
            return;

        default:
            // Cannot do any optimization, continue as usual.
            break;
        }
    }

    bool ConversionFromNothing = ptreeFrom->bilop == SX_NOTHING;

    if (vtypeFrom == t_ref && vtypeTo == t_struct && ConversionFromNothing)
    {
        // Special case the conversion from Nothing to struct. Don't generate the CEE_LDNULL opcode.
    }
    else
    {
        GenerateRvalue(ptreeFrom);
    }

    if (vtypeTo == t_i4 && vtypeFrom == t_char)
    {
        // Conversions from Char to Integer require no additional work.  This is a special case which
        // comes from Semantics when it optimizes AscW calls.
        // 
        return;
    }

    if (vtypeTo == t_generic)
    {
        VSASSERT(vtypeFrom == t_string || vtypeFrom == t_ref || vtypeFrom == t_array,
                "valuetype to generic parameter type unexpected!!!");

        GenerateCastClass(ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
        return;
    }

    if (vtypeFrom == t_generic)
    {
        VSASSERT(vtypeTo == t_string || vtypeTo == t_ref || vtypeTo == t_array,
                 "conversion from generic type parameter to value type unexpected!!!");

        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(ptreeFrom->AsExpression().ResultType, &ptreeFrom->Loc);
        EmitOpcode_Tok(CEE_BOX, typref, ptreeTo->AsExpression().ResultType);
        return;
    }

    // Handle all coercions to t_ref. Keep in mind that t_ref is a catch-
    // all for every reference type. It doesn't just mean System.Object
    if (vtypeTo == t_ref)
    {
        // String, ref and array are all reference types. String and array
        // can't be downcast, but may be cast to interfaces.
        if (vtypeFrom == t_string || vtypeFrom == t_ref || vtypeFrom == t_array)
        {
            GenerateCastClass(ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
        }
        else
        {
            VSASSERT(ptreeTo->AsExpression().ResultType ==
                        m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType) ||
                     ptreeTo->AsExpression().ResultType ==
                         m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ValueTypeType) ||
                     ptreeTo->AsExpression().ResultType ==
                         m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::EnumType),
                     "coercion makes no sense!");

            mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(ptreeFrom->AsExpression().ResultType, &ptreeFrom->Loc);
            EmitOpcode_Tok(CEE_BOX, typref, GetSymbolForFXType(FX::ObjectType));
        }

        return;
    }

    // string = UI1|i2|i4|r4|r8|bool|date|cy|dec
    if (vtypeTo == t_string)
    {
        VSASSERT(vtypeFrom != t_array || ptreeFrom->AsExpression().ResultType->PArrayType()->GetRoot()->GetVtype() == t_char,
                 "GenerateConvertClass: must be array of char to coerce to string.");

        if (vtypeFrom == t_ref &&
            ptreeFrom->AsExpression().ResultType != m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType))
        {
            GenerateCastClass(ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
            return;
        }
        else
        {
            rtHelper = GetHelperForStringCoerce(vtypeFrom);
        }
    }

    // decimal = integer, R8
    else if (vtypeTo == t_decimal && vtypeFrom != t_string && vtypeFrom != t_ref)
    {
        VSASSERT(vtypeFrom != t_char, "Char to Decimal is illegal. We should not be getting here.");

        rtHelper = GetHelperToDecimal(vtypeFrom);
    }


    // Handle all coercions from t_ref.
    if (vtypeFrom == t_ref)
    {
        if (vtypeTo == t_struct)
        {
            // Conversions from references types to structures should be equivalent to
            // conversions from Nothing if the reference is Nothing.  Perform the check
            // and do the conversion.

            CODE_BLOCK *pcblkUnbox = NULL;
            CODE_BLOCK *pcblkResult = NULL;

            if (!ConversionFromNothing)
            {
                pcblkUnbox = NewCodeBlock();
                pcblkResult = NewCodeBlock();

                // If the Reference is not nothing, branch directly to the Unbox
                BCSYM* pTypeOnTopOfStack = m_stackTypes.Peek();
                EmitOpcode(CEE_DUP, pTypeOnTopOfStack);
                EndCodeBuffer(CEE_BRTRUE_S, pcblkUnbox);

                SetAsCurrentCodeBlock(0);

                // The reference is nothing, so we need to load the special "Nothing" temp for the
                // struct we're converting to.
                //
                // But first Pop off the Null reference cause we don't need it anymore.
                EmitOpcode(CEE_POP, Symbols::GetVoidType());
            }

            // Find the special Nothing temp in the temp pool and load it onto the stack.
            // 

            TemporaryIterator Iterator;
            Iterator.Init(m_ptreeFunc->AsProcedureBlock().ptempmgr);

            Temporary *Current;
            while (Current = Iterator.GetNext())
            {
                if (Current->Lifetime == LifetimeDefaultValue &&
                    TypeHelpers::EquivalentTypes(Current->Symbol->GetType(), ptreeTo->AsExpression().ResultType))
                {
                    GenerateLoadLocal(Current->Symbol->GetLocalSlot(), Current->Symbol->GetType());
                    break;
                }
            }

            if (Current == NULL)
            {
                VSFAIL("Expected to find special 'Nothing' temp!");
                VbThrow(HrMake(ERRID_CodegenError));
            }

            if (!ConversionFromNothing)
            {
                EndCodeBuffer(CEE_BR_S, pcblkResult);

                SetAsCurrentCodeBlock(pcblkUnbox);

                // Unbox the reference to get the struct from inside the object.
                mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
                EmitOpcode_Tok(CEE_UNBOX, typref, UNSPILLABLE_TYPE_SYMBOL);
                EmitOpcode_Tok(CEE_LDOBJ, typref, ptreeTo->AsExpression().ResultType);

                SetAsCurrentCodeBlock(pcblkResult);
                // Basic block merge points on x64 JIT need explicit sequence points (via NOP) in order
                // for the instructions before the next sequence point to map to the previous sequence
                // point.  This should also be the case on x86 JIT, but it happens to work regardless
                // by accident.
                //
                // This ensures that the values of variables show up immediately after stepping over
                // their assignment on x64 specifically when the target type is a struct larger than
                // 64 bits, e.g. Guid.  Ex:
                //
                //      Dim obj As Object = Guid.NewGuid()
                //      Dim guid = CType(obj, Guid) ' Step passed this line and observe value of guid in watch window
                //
                InsertNOP();
            }
            return;
        }
        else if (vtypeTo == t_array)
        {
            // If we're converting to array of characters, we've got to call the special
            // helper (in case it's a string). Otherwise, just convert to the specific array type.
            if (TypeHelpers::IsCharArrayRankOne(ptreeTo->AsExpression().ResultType))
            {
                rtHelper = ObjectToCharArrayMember;
                GenerateCallToRuntimeHelper(rtHelper, ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
                // 






            }
            GenerateCastClass(ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
            return;
        }
        else
        {
            rtHelper = GetHelperFromRef(vtypeTo);
        }
    }

    else if (vtypeFrom == t_array && vtypeTo != t_string)
    {
        // Arrays can only be case to reference types or other arrays.
        VSASSERT(vtypeTo == t_array, "has to be covariance!");

        GenerateCastClass(ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
        return;
    }

    //  Handle calls from string to UI1|i2|i4|r4|r8|bool|date|cy|dec
    else if (vtypeFrom == t_string)
    {
        VSASSERT(vtypeTo != t_array ||
                  ptreeTo->AsExpression().ResultType->PArrayType()->GetRoot()->GetVtype() == t_char,
                  "GenerateConvertClass: must be array of char to coerce from string.");

        VSASSERT(rtHelper == UndefinedRuntimeMember, "hmm, didn't expected this.");  // 

        rtHelper = GetHelperFromString(vtypeTo);
    }

    // ui1|i2|i4|r4|r8 = decimal
    else if (vtypeFrom == t_decimal && vtypeTo != t_string)
    {
        VSASSERT(vtypeTo != t_char, "Decimal to Char is illegal. We should not be getting here.");
        rtHelper = GetHelperFromDecimal(vtypeTo);
    }

    // By this point, types should never match
    VSASSERT(vtypeFrom != vtypeTo, "GenerateConvertClass: should not see coerce node to same types");

    VSASSERT(rtHelper != UndefinedRuntimeMember, "unhandled coercion!");
    GenerateCallToRuntimeHelper(rtHelper, ptreeTo->AsExpression().ResultType, &ptreeTo->Loc);
}

/*=======================================================================================
GenerateDirectCast

Generates code for the DirectCast expression.
Produces IL to perform casts over inhertance and interface implementation relationships.
=======================================================================================*/
void
CodeGenerator::GenerateDirectCast
(
    ILTree::BinaryExpression *ptreeTo
)
{
    // 

    VSASSERT(ptreeTo->bilop == SX_DIRECTCAST, "GenerateDirectCast: must have cast node");

    ILTree::Expression *ptreeFrom = ptreeTo->Left;

    Vtypes vtypeFrom = ptreeFrom->vtype;
    Vtypes vtypeTo = ptreeTo->vtype;

    // Attempt to optimize the coercion.
    if (vtypeTo == t_bool && vtypeFrom == t_ref)
    {
        switch (ptreeFrom->bilop)
        {
        case SX_EQ:
        case SX_NE:
        case SX_LE:
        case SX_LT:
        case SX_GE:
        case SX_GT:
            // The result of the comparison is known to be boolean,
            // so force it to be.
            ptreeFrom->vtype = t_bool;
            GenerateRelOp(ptreeFrom);
            return;

        default:
            // Cannot do any optimization, continue as usual.
            break;
        }
    }

    GenerateRvalue(ptreeFrom);

    if (vtypeFrom == t_generic)
    {
        VSASSERT(vtypeTo == t_string || vtypeTo == t_ref || vtypeTo == t_array,
                 "conversion from generic parameter type to value type unexpected!!");

        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(ptreeFrom->ResultType, &ptreeFrom->Loc);
        EmitOpcode_Tok(CEE_BOX, typref, GetSymbolForFXType(FX::ObjectType));

        // Note: A GenerateCastClass is not needed here because no narrowing conversions
        // are allowed from a generic parameter type directly. If any are even possible,
        // they have to go through object.
    }
    else if (vtypeTo == t_generic)
    {
        VSASSERT(vtypeFrom == t_string || vtypeFrom == t_ref || vtypeFrom == t_array,
                 "conversion from value type to generic parameter type unexpected!!");

        GenerateCastClass(ptreeTo->ResultType, &ptreeTo->Loc);
    }
    else if (TypeHelpers::IsReferenceType(ptreeFrom->ResultType))
    {
        if (TypeHelpers::IsReferenceType(ptreeTo->ResultType))
        {
            GenerateCastClass(ptreeTo->ResultType, &ptreeTo->Loc);
        }
        else
        {
            mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(ptreeTo->ResultType, &ptreeTo->Loc);

            // unbox the return value to get the struct from inside the object
            EmitOpcode_Tok(CEE_UNBOX, typref, UNSPILLABLE_TYPE_SYMBOL);
            EmitOpcode_Tok(CEE_LDOBJ, typref, ptreeTo->ResultType);
        }
    }
    else
    {
        VSASSERT(ptreeTo->ResultType == m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType) ||
                 ptreeTo->ResultType == m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ValueTypeType) ||
                 ptreeTo->ResultType == m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::EnumType),
                 "DirectCast makes no sense!");

        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(ptreeFrom->ResultType, &ptreeFrom->Loc);
        EmitOpcode_Tok(CEE_BOX, typref, GetSymbolForFXType(FX::ObjectType));
    }

    return;
}

/*=======================================================================================
GenerateTryCast

Generates code for the TryCast expression.
Produces IL to perform safe casts (casts which do not throw InvalidCastExceptions) over
inhertance and interface implementation relationships.
=======================================================================================*/
void
CodeGenerator::GenerateTryCast
(
    ILTree::BinaryExpression *Target
)
{
    VSASSERT(Target->bilop == SX_TRYCAST, "GenerateTryCast: must have cast node");

    ILTree::Expression *Source = Target->Left;

    GenerateRvalue(Source);

    if (Source->vtype == t_generic)
    {
        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(Source->ResultType, &Source->Loc);
        EmitOpcode_Tok(CEE_BOX, typref, GetSymbolForFXType(FX::ObjectType));

        typref = m_pmdemit->DefineTypeRefBySymbol(Target->ResultType, &Target->Loc);
        EmitOpcode_Tok(CEE_ISINST, typref, Target->ResultType);
    }
    else if (Target->vtype == t_generic)
    {
        VSASSERT(TypeHelpers::IsReferenceType(Target->ResultType), "Target must be a class-constrained generic.");
        VSASSERT(TypeHelpers::IsReferenceType(Source->ResultType), "Source must be a reference type.");

        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(Target->ResultType, &Target->Loc);
        EmitOpcode_Tok(CEE_ISINST, typref, Target->ResultType);

        typref = m_pmdemit->DefineTypeRefBySymbol(Target->ResultType, &Target->Loc);
        VSASSERT(typref != mdTypeRefNil, "invalid generic type param!");
        EmitOpcode_Tok(CEE_UNBOX_ANY, typref, Target->ResultType); // As noted above, the target must be a class-constrained generic.
    }

    else if (TypeHelpers::IsReferenceType(Source->ResultType))
    {
        VSASSERT(TypeHelpers::IsReferenceType(Target->ResultType), "Can't cast to a valuetype using TryCast!");

        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(Target->ResultType, &Target->Loc);
        EmitOpcode_Tok(CEE_ISINST, typref, Target->ResultType);
    }
    else
    {
        VSASSERT(Target->ResultType == m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType) ||
                 Target->ResultType == m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ValueTypeType) ||
                 Target->ResultType == m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::EnumType),
                 "DirectCast makes no sense!");

        mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(Source->ResultType, &Source->Loc);
        EmitOpcode_Tok(CEE_BOX, typref, GetSymbolForFXType(FX::ObjectType));
    }

    return;
}

//========================================================================
// Generate a coerce expression
//========================================================================

void CodeGenerator::GenerateConvert
(
    ILTree::BinaryExpression *ptreeTo
)
{
    ILTree::Expression *ptreeFrom;
    Vtypes vtypeFrom, vtypeTo;

    VSASSERT(ptreeTo && ptreeTo->bilop == SX_CTYPE, "GenerateConvert: must have conversion node");
    VSASSERT(ptreeTo->Left, "GenerateConvert: must have from expr");

    ptreeFrom = ptreeTo->Left;

    // Get the types to coerce from and to
    vtypeFrom = ptreeFrom->vtype;
    vtypeTo = ptreeTo->vtype;

    VSASSERT(vtypeTo != t_ptr, "GenerateConvert: coercion doesn't make sense");

    // Get the base type of pointer
    if (vtypeFrom == t_ptr)
    {
        VSFAIL("can this be reached?");
        vtypeFrom = ptreeFrom->AsExpression().ResultType->PPointerType()->GetRoot()->GetVtype();
    }

    // All number to number coercions (no runtime functions required)
    if (IsSimpleType(vtypeFrom) && IsSimpleType(vtypeTo))
    {
        GenerateConvertIntrinsic(ptreeTo, ptreeFrom, vtypeTo, vtypeFrom);
        return;
    }

    // All other coercions
    GenerateConvertClass(ptreeTo, ptreeFrom, vtypeTo, vtypeFrom);
}

//========================================================================
// Generate a cast expression
//========================================================================

void
CodeGenerator::GenerateCastClass
(
    BCSYM *ptypTo,
    Location *pReferencingLocation
)
{
    mdTypeRef typref = mdTypeRefNil;

    // Don't need to cast to the Object intrinsic type.
    if (ptypTo->IsObject())
    {
        return;
    }

    if (ptypTo->IsGenericParam())
    {
        typref = m_pmdemit->DefineTypeRefBySymbol(ptypTo->DigThroughAlias(), pReferencingLocation);
        VSASSERT(typref != mdTypeRefNil, "invalid generic type param!");
        EmitOpcode_Tok(CEE_UNBOX_ANY, typref, ptypTo);
        return;
    }

    typref = m_pmdemit->DefineTypeRefBySymbol(ptypTo->DigThroughAlias(), pReferencingLocation);
    VSASSERT(typref != mdTypeRefNil, "invalid class!");
    EmitOpcode_Tok(CEE_CASTCLASS, typref, ptypTo);
}

/*========================================================================

Instantiation expressions

========================================================================*/

//========================================================================
// Find the default constructor (the first one that takes no args).
// If no constructors are defined and it's a basic class, then assume
// there will be a generated one.
//========================================================================

void CodeGenerator::GetDefCtorRef
(
BCSYM *pcon,
mdMemberRef *pmemref,
Location *pReferencingLocation
)
{
    BCSYM_Proc *pproc = NULL;

    *pmemref = mdMemberRefNil;

    VSASSERT(pcon->IsClass(), "then what is it?");

    pproc = pcon->PClass()->GetFirstInstanceConstructor(m_pCompiler);

    while (pproc != NULL)
    {

        if (!pproc->GetFirstParam())
        {
            break;
        }

        pproc = pproc->GetNextInstanceConstructor();
    }

    if (pproc != NULL)
    {
        *pmemref = m_pmdemit->DefineMemberRefBySymbol(pproc, pcon->IsGenericTypeBinding() ? pcon->PGenericTypeBinding() : NULL, pReferencingLocation);
        VSASSERT(*pmemref != mdMemberRefNil, "must exist.");
    }
    else
    {
        VSASSERT(pcon->IsStruct(), "Nothing else does not have a constructor.");

        // If we get here, we are dealing with a struct.
        // Do nothing since the default constuctor for non-VB structs is actually
        // not a constructor at all but instead zero-initialized memory.
        // A memref of mdMemberRefNil signals to the caller not to generate
        // a constructor call.
    }
}


// =======================================================================
// Generate a Delegate constructor expression
//
//    Because the verifier requires us to use the dup opcode for a virtual
//    delegate, we need to special-case delegate construction with its own
//    node type
// =======================================================================

void CodeGenerator::GenerateDelegateConstructorArgs
(
    ILTree::PILNode ptree,
    signed *SubStack
)
{
    VSASSERT(ptree->bilop == SX_DELEGATE_CTOR_CALL,
             "GenerateDelegateConstructorArgs: no delegate ctor node to use!");

    VSASSERT(ptree->AsDelegateConstructorCallExpression().Method->bilop == SX_SYM,
             "GenerateDelegateConstructorArgs: need a symbol for the Method argument");

    ILTree::PILNode ObjectArgument = ptree->AsDelegateConstructorCallExpression().ObjectArgument;

    // Generate the Object part of the constructor call
    GenerateRvalue(ObjectArgument);

    BCSYM_NamedRoot * MethodSymbol = ptree->AsDelegateConstructorCallExpression().Method->AsSymbolReferenceExpression().pnamed;

    // Get the token for the Method part of the constructor call
    mdMemberRef MethodToken =
        m_pmdemit->DefineMemberRefBySymbol(
            MethodSymbol,
            ptree->AsDelegateConstructorCallExpression().Method->AsSymbolReferenceExpression().GenericBindingContext,
            &ptree->AsDelegateConstructorCallExpression().Method->Loc);

    // If Method is shared, or if object is MyBase or MyClass, or if method is declared in a value type,
    // use LDFTN opcode. Otherwise use LDVIRTFTN.
    // Special-casing value types is required for primitive types (Int32, Double etc), because using LDVIRTFTN
    // for methods declared in them will produce unverifiable code due to a verifier bug. And it doesn't hurt
    // for other value types.
    BCSYM_Proc *pProc = MethodSymbol->DigThroughAlias()->PProc();
    if
    (
        ObjectArgument->bilop == SX_NOTHING ||
        (
            ObjectArgument->bilop == SX_SYM &&
            ObjectArgument->uFlags & (SXF_SYM_MYBASE | SXF_SYM_MYCLASS)
        ) ||
        HasFlag32(ptree, SXF_CALL_WAS_EXTENSION_CALL) ||
        (pProc->GetContainingClass() != nullptr && pProc->GetContainingClass()->IsStruct())
    )
    {
        VSASSERT(ObjectArgument->bilop != SX_NOTHING || pProc->IsShared(),
                 "GenerateDelegateConstructorArgs: If no object operand, this must be shared");

        EmitOpcode_Tok(CEE_LDFTN, MethodToken, UNSPILLABLE_TYPE_SYMBOL); // LDFTN actually pushes a native int.
    }
    else
    {
        VSASSERT(!pProc->IsShared(),
                 "GenerateDelegateConstructorArgs: If we have an object operand, this must not be shared");

        // The DUP opcode is required by the verifier - the object operand and the reference used
        // to obtain the function pointer must identical
        BCSYM* pTypeOnTopOfStack = m_stackTypes.Peek();
        EmitOpcode(CEE_DUP, pTypeOnTopOfStack);
        EmitOpcode_Tok(CEE_LDVIRTFTN, MethodToken, UNSPILLABLE_TYPE_SYMBOL); // LDFTN actually pushes a native int.
    }

    *SubStack = 2;  // a delegate constructor consumes two items from the stack
}


//========================================================================
// Generate a New expression
//========================================================================

void CodeGenerator::GenerateNew
(
BCSYM * pmod,
ILTree::PILNode ptree,
Location *Location // [in] the ptree we get doesn't have the location of the expression in it
)
{
    mdMemberRef memref = mdMemberRefNil;
    signed sSubStack = 0;

    VSASSERT(!pmod->IsStruct(), "GenerateNew: didn't expect structure here.");  // SX_NEW should not be used on structures.

    if (pmod->IsGenericParam())
    {
        VSASSERT(!ptree, "constructor call tree unexpected!!!");

        BCSYM * pTypeParamType = pmod;

        // 



        if (!m_Project->GetCompilerHost()->IsStarliteHost())
        {
            // Generate the equivalent of the following code:
            //      System.Activator.CreateInstance(Of T)()

            BCSYM_NamedRoot *pGenericCreateInstance = m_Project->GetCompilerHost()->GetSymbolForRuntimeMember(GenericCreateInstanceMember, m_pCompilationCaches);
            BCSYM *pTypeArguments[1] = {pTypeParamType};
            Symbols SymbolFactory(m_pCompiler, m_pnra, NULL);

            // Generated for: Sub bar(Of T As New)()
            // Dim x As T = New T 'calls [mscorlib]System.Activator::CreateInstance
            GenerateCallToRuntimeHelper( GenericCreateInstanceMember, 
                pmod,
                Location,
                Error,
                pGenericCreateInstance ? // this can be NULL if System.Activator::CreatInstance(<T>) isn't defined in mscorlib.dll
                    SymbolFactory.GetGenericBinding( false /* IsBad = false */, pGenericCreateInstance, pTypeArguments, 1, NULL, true) :
                    NULL ); // Alloc and copy type arguments
        }
        else
        {
            // Generate the equivalent of the following code:
            //      DirectCast(System.Activator.CreateInstance(GetType(T)), T)

            // get the type object
            GenerateTypeObject(pTypeParamType, Location);

            // create the instance
            GenerateCallToRuntimeHelper(CreateInstanceMember, GetSymbolForFXType(FX::ObjectType), Location, Error);

            // cast back to the type param
            GenerateCastClass(pTypeParamType, NULL);
        }

        return;
    }

    VSASSERT(ptree, "GenerateNew: must have something to call!");

    if (ptree->bilop == SX_DELEGATE_CTOR_CALL)
    {
        BCSYM_Proc *pproc = ptree->AsDelegateConstructorCallExpression().Constructor->AsSymbolReferenceExpression().pnamed->PProc();
        GenerateDelegateConstructorArgs(ptree, &sSubStack);
        memref =
            m_pmdemit->DefineMemberRefBySymbol(
                pproc,
                ptree->AsDelegateConstructorCallExpression().Constructor->AsSymbolReferenceExpression().GenericBindingContext,
                &ptree->AsDelegateConstructorCallExpression().Constructor->Loc);
#if DEBUG
        DumpCall(pproc);
#endif
    }
    else
    {
        VSASSERT(ptree->bilop == SX_CALL, "GenerateNew: must have a call node here");

        if (ptree->AsCallExpression().Left)
        {
            BCSYM_Proc *pproc = ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->PProc();

            GenerateCallArg(ptree->AsCallExpression().Right, &sSubStack);
            memref =
                m_pmdemit->DefineMemberRefBySymbol(
                    pproc,
                    ptree->AsCallExpression().Left->AsSymbolReferenceExpression().GenericBindingContext,
                    &ptree->AsCallExpression().Left->Loc);
#if DEBUG
            DumpCall(pproc);
#endif
        }
        else
        {
            GetDefCtorRef(pmod, &memref, &ptree->Loc);
        }
    }

    VSASSERT(memref != mdMemberRefNil, "GenerateNew: no constructor!");
    AddToCurStack(-sSubStack);
    for (int i = 0; i < sSubStack; i++)
    {
        m_stackTypes.PopOrDequeue();
    }
    EmitOpcode_Tok(CEE_NEWOBJ, memref, pmod);
}


void CodeGenerator::GenerateInitStructure
(
BCSYM * StructureType,
ILTree::PILNode StructureReference
)
{
    VSASSERT(StructureType->IsStruct() || StructureType->IsGenericParam(), "Expected structure");

    GenerateRvalue(StructureReference);

    // Although VB and C# don't allow parameterless constructors on structures, other languages do (MC++).
    // We should call the constructor if it exists.  (VS #335900)

    mdMemberRef memref = mdMemberRefNil;
    if (!StructureType->IsGenericParam())
    {
        GetDefCtorRef(StructureType, &memref, &StructureReference->Loc);
    }

    if (memref != mdMemberRefNil)
    {
        // Found a parameterless constructor, so call it.
        EmitOpcode_Tok(CEE_CALL, memref, Symbols::GetVoidType());
        AddToCurStack(-1);
        m_stackTypes.PopOrDequeue();
    }
    else
    {
        EmitOpcode_Tok(CEE_INITOBJ,
                       m_pmdemit->DefineTypeRefBySymbol(StructureType, &StructureReference->Loc),
                       Symbols::GetVoidType());
    }
}


/*********************************************************************
*
* Call expressions
*
**********************************************************************/


//========================================================================
// Generate a latebound expression call
//========================================================================

void CodeGenerator::GenerateLate
(
ILTree::PILNode ptree
)
{
    RuntimeMembers rtHelper = UndefinedRuntimeMember;

    // Load the object of the Late call, null if there isn't one
    if (ptree->AsLateBoundExpression().Left)
    {
        VSASSERT(ptree->AsLateBoundExpression().LateClass == NULL,
                 "GenerateLate: LateClass cannot be provided if Me is provided");

        GenerateRvalue(ptree->AsLateBoundExpression().Left);
    }
    else
    {
        EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ObjectType));
    }

    // Load the Type object for the class in which the static method resides, null if there isn't one
    if (ptree->AsLateBoundExpression().LateClass)
    {
        VSASSERT(ptree->AsLateBoundExpression().Left == NULL,
                 "GenerateLate: Me cannot be provided if LateClass is provided");

        GenerateTypeObject(ptree->AsLateBoundExpression().LateClass, &ptree->Loc);
    }
    else
    {
        EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::TypeType));
    }

    // Load method/property as constant String
     if (ptree->AsLateBoundExpression().LateIdentifier)
    {
        GenerateRvalue(ptree->AsLateBoundExpression().LateIdentifier);
    }
    else
    {
        // default property; load a NULL
        EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ObjectType));
    }

    // Load the arguments in a paramarray
    GenerateRvalue(ptree->AsLateBoundExpression().Right);

    switch (SXF_LATE_ENUM(ptree->uFlags))
    {
    case SXE_LATE_GET:
    case SXE_LATE_CALL:

        if (ptree->AsLateBoundExpression().TypeArguments)
        {
            GenerateRvalue(ptree->AsLateBoundExpression().TypeArguments);
        }
        else
        {
            EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ObjectType));
        }

        // Load the array of booleans (if needed) which keeps track of the byref parameters
        if (ptree->AsLateBoundExpression().AssignmentInfoArrayParam)
        {
            GenerateRvalue(ptree->AsLateBoundExpression().AssignmentInfoArrayParam);
        }
        else
        {
            EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ObjectType));
        }
        rtHelper = (SXF_LATE_ENUM(ptree->uFlags) == SXE_LATE_GET) ? LateGetMember : LateCallMember;

        if (rtHelper == LateCallMember)
        {
            GenerateLiteralInt(COMPLUS_TRUE);
        }

        break;

    case SXE_LATE_SET:
        // LateSet devolves into a property set (which can't take byref params) or a late index set (which also
        // can't take byref params)
        VSASSERT(ptree->AsLateBoundExpression().AssignmentInfoArrayParam == NULL,
                 "GenerateLate: LateSet can't have byref params, so why do we need them?");

        if (ptree->AsLateBoundExpression().TypeArguments)
        {
            GenerateRvalue(ptree->AsLateBoundExpression().TypeArguments);
        }
        else
        {
            EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ObjectType));
        }

        // This Set may be the "copy-back" of a latebound expression being passed as a byref param,
        // in which case we call a special helper which determines if the Set will
        // work (since the latebound expression may result in a ReadOnly property or a method).

        if (ptree->uFlags & SXF_LATE_OPTIMISTIC || ptree->uFlags & SXF_LATE_RVALUE_BASE)
        {
            GenerateLiteralInt(ptree->uFlags & SXF_LATE_OPTIMISTIC ? COMPLUS_TRUE : COMPLUS_FALSE);
            GenerateLiteralInt(ptree->uFlags & SXF_LATE_RVALUE_BASE ? COMPLUS_TRUE : COMPLUS_FALSE);

            rtHelper = LateSetComplexMember;
        }
        else
        {
            rtHelper = LateSetMember;
        }
        break;

    default:
        VSFAIL("GenerateLate: invalid rtHelper");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }

    GenerateCallToRuntimeHelper(rtHelper, ptree->AsExpression().ResultType, &ptree->Loc);

    // 
    if (rtHelper == LateCallMember)
    {
        EmitOpcode(CEE_POP, Symbols::GetVoidType());
    }
}

//========================================================================
// Generate variant indexing operation
//========================================================================

void CodeGenerator::GenerateLateIndex
(
ILTree::PILNode ptree
)
{
    RuntimeMembers rtHelper = UndefinedRuntimeMember;

    // Load the object of the late index call
    GenerateRvalue(ptree->AsVariantIndexExpression().Left);

    GenerateRvalue(ptree->AsVariantIndexExpression().Right);

    switch (SXF_VARINDEX_ENUM(ptree->uFlags))
    {
    case SXE_VARINDEX_GET:
        rtHelper = LateIndexGetMember;
        break;

    case SXE_VARINDEX_SET:
        // This Set may be the "copy-back" of a latebound expression being passed as a byref param,
        // in which case we call a special helper which determines if the Set will
        // work (since the latebound expression may result in a ReadOnly property or a method).
        //
        if (ptree->uFlags & SXF_LATE_OPTIMISTIC || ptree->uFlags & SXF_LATE_RVALUE_BASE)
        {
            GenerateLiteralInt(ptree->uFlags & SXF_LATE_OPTIMISTIC ? COMPLUS_TRUE : COMPLUS_FALSE);
            GenerateLiteralInt(ptree->uFlags & SXF_LATE_RVALUE_BASE ? COMPLUS_TRUE : COMPLUS_FALSE);

            rtHelper = LateIndexSetComplexMember;
        }
        else
        {
            rtHelper = LateIndexSetMember;
        }
        break;
    }

    GenerateCallToRuntimeHelper(rtHelper, ptree->AsExpression().ResultType, &ptree->Loc);
}

//========================================================================
// Generate the arguments for a call
//========================================================================

void
CodeGenerator::GenerateCallArg
(
    ILTree::PILNode ptreeList,
    signed *psSubStack
)
{
    signed sSubStack = 0;

    while (ptreeList)
    {
        VSASSERT(ptreeList->bilop == SX_LIST,
                 "GenerateCallArg: each arg must be SX_LIST");

        ILTree::PILNode ptreeArg = ptreeList->AsExpressionWithChildren().Left;
        GenerateRvalue(ptreeArg);

        if (!(ptreeList->uFlags & SXF_LIST_SUPPRESS_CLONE))
        {
            // If necessary, clone the Argument value for correct object semantics.
            GenerateClone(ptreeArg);
        }

        // keep track of the stack effect
        sSubStack++;
        if (sSubStack == 0)
        {
            VbThrow(HrMake(ERRID_CodegenError)); // Overflow
        }

        ptreeList = ptreeList->AsExpressionWithChildren().Right;
    } // while ptreeList


    if (*psSubStack + sSubStack < *psSubStack) // sSubStack is known to be positive
    {
        VbThrow(HrMake(ERRID_CodegenError)); // Overflow
    }
    *psSubStack += sSubStack;

}


//========================================================================
// Generate a call expression
//========================================================================

void
CodeGenerator::GenerateCall
(
    ILTree::PILNode ptree
)
{
    signed        sSubStack = 0;
    OPCODE        opcode = CEE_NOP;
    ILTree::PILNode      ptreeThis;
    BCSYM_Proc  * pproc;
    mdMemberRef   memref;
    BCSYM       * ptypRet = NULL;
    signed        cParams = 0;

    VSASSERT(ptree->AsCallExpression().Left &&
             ptree->AsCallExpression().Left->bilop == SX_SYM,
             "GenerateCall: not a runtime call, must have a proc sym");

    VSASSERT(m_currentHydrationContext == NULL, "GenerateRValue should have set the hydration context to NULL before calling GenerateCall.");
    m_currentHydrationContext = ptree->AsExpression().ContainsAsyncSpill ? new (*m_pnra) Queue<HydrationStep*, NorlsAllocWrapper>(m_nraWrapper) : NULL;

    pproc = ptree->AsCallExpression().Left->AsSymbolReferenceExpression().pnamed->DigThroughAlias()->PProc();

    // If pproc is a partial declaration, this must mean that it has no implementation.
    // Therefore, just make this call a NOOP.

    if( pproc->IsPartialMethodDeclaration() )
    {
        VSASSERT(
            pproc->GetAssociatedMethod() == NULL,
            "How come a partial that has an implementation made it to code gen?"
            );
        return;
    }

    ptreeThis = ptree->AsCallExpression().ptreeThis;

    // generate code for the object
    if (ptreeThis)
    {
        VSASSERT(!pproc->IsDllDeclare(), "dll calls are always static!");

        GenerateRvalue(ptreeThis);

        // 
        if (!(ptree->AsCallExpression().Left->uFlags & SXF_SYM_NONVIRT) &&
            ptreeThis->AsExpression().vtype == t_ptr &&
            !ptreeThis->AsExpression().ResultType->PPointerType()->GetRoot()->IsGenericParam() &&
            !TypeHelpers::IsValueType(ptreeThis->AsExpression().ResultType->PPointerType()->GetRoot()))
        {
            VSFAIL("This condition shouldn't occur");
            VbThrow(HrMake(ERRID_CodegenError));
        }

        if ((sSubStack += 1) < 1)
        {
            VbThrow(HrMake(ERRID_CodegenError)); // Overflow
        }
    }

    GenerateCallArg(ptree->AsCallExpression().Right, &cParams);

    // OK, we've taken care of all the child expressions. Now, rehydrate any
    // managed pointers before we actually call the method.
    RehydrateManagedPointers(&ptree->Loc);

    sSubStack += cParams;
    BCSYM_GenericBinding *pBinding = ptree->AsCallExpression().Left->AsSymbolReferenceExpression().GenericBindingContext;
    if (ptreeThis &&
        ptreeThis->uFlags & SXF_CONSTRAINEDCALL_BASEREF)
    {
        VSASSERT(ptreeThis->AsExpression().ResultType->IsPointerType() &&
                 (TypeHelpers::IsGenericParameter(ptreeThis->AsExpression().ResultType->PPointerType()->GetRoot()) ||
                  TypeHelpers::IsValueType(ptreeThis->AsExpression().ResultType->PPointerType()->GetRoot())),
                    "Inconsistency in constrained call tree!!!");

        EmitOpcode_Tok(
            CEE_CONSTRAINED,
            m_pmdemit->DefineTypeRefBySymbol(ptreeThis->AsExpression().ResultType->PPointerType()->GetRoot(), &ptree->AsCallExpression().Left->Loc),
            Symbols::GetVoidType()); // Does not modify the stack

        opcode = CEE_CALLVIRT;

        //constrained call on valuetypes should go via overridden proc, if any
        if (pproc->OverriddenProc() && TypeHelpers::IsValueType(pproc->GetContainer()))
        {
            pproc = pproc->OverriddenProc();
            pBinding = NULL;
            VSASSERT(pproc == m_Project->GetCompilerHost()->RemapRuntimeCall(pproc, m_pCompilationCaches), "Why a remaped runtime call is invoked via constrained base call");
        }
    }
    else if (ptree->AsCallExpression().Left->uFlags & SXF_SYM_NONVIRT)
    {
        opcode = CEE_CALL;
    }
    else
    {
        opcode = CEE_CALLVIRT;
        VSASSERT(ptree->AsCallExpression().ptreeThis,
                  "GenerateNormalCall: virtual calls must have this object.");
    }

    pproc = m_Project->GetCompilerHost()->RemapRuntimeCall(pproc, m_pCompilationCaches);

#if DEBUG
    DumpCall(pproc);
#endif

    memref =
        m_pmdemit->DefineMemberRefBySymbol(
            pproc,
            pBinding,
            &ptree->AsCallExpression().Left->Loc);

    EmitOpcode_Tok(opcode, memref, Symbols::GetVoidType());

    // CEE_call: Pop off the arguments
    AddToCurStack(-sSubStack);
    for (int i = 0; i < sSubStack; i++)
    {
        m_stackTypes.PopOrDequeue();
    }

    ptypRet = pproc->GetCompilerType();
    if (ptypRet != NULL)
    {
        AddToCurStack(1);
        m_stackTypes.PushOrEnqueue(ptree->AsExpression().ResultType);

        if (ptree->AsExpression().vtype == t_void)
        {
            // no consumer of return value, pop off stack
            EmitOpcode(CEE_POP, Symbols::GetVoidType());
        }
    }
    else
    {
        // The IP always points to the location where execution will return
        // So we generate a NOP here so that the IP is still associated with
        // the call statement and the user can double click in the callstack
        // window and take him to the calling line.
        InsertNOP();
    }
}

/*========================================================================

Binary operator expressions

========================================================================*/

//========================================================================
// Generate the TypeOf ... Is ... expression
//========================================================================

void CodeGenerator::GenerateIsType
(
ILTree::PILNode ptree
)
{
    ILTree::ILNode   * ptreeObj = ptree->AsExpressionWithChildren().Left;
    BCSYM     * ptyp = ptree->AsExpressionWithChildren().Right->ResultType;
    mdTypeRef   typref;


    VSASSERT(ptree->bilop == SX_ISTYPE, "GenerateIsType: SX_ISTYPE node expected");
    VSASSERT(ptree->AsExpression().vtype == t_bool, "GenerateIsType: TypeOf..Is should be boolean expression");
    VSASSERT(ptree->AsExpressionWithChildren().Right->bilop == SX_NOTHING, "GenerateIsType: SX_NOTHING node expected");

    VSASSERT(ptreeObj->AsExpression().vtype == t_ref ||
             ptreeObj->AsExpression().vtype == t_string ||
             ptreeObj->AsExpression().vtype == t_array,
             "GenerateIsType: t_ref expected");

    // load object reference onto the stack
    GenerateRvalue(ptreeObj);

    // then check if it matches the type
    // note:  isinst returns the instance of the class casted to the target type if <object>
    // inherits from or implements <target>.  otherwise, it returns NULL.
    typref = m_pmdemit->DefineTypeRefBySymbol(ptyp->DigThroughAlias(), &ptree->Loc);
    EmitOpcode_Tok(CEE_ISINST, typref, ptyp);
}

//========================================================================
// Load a type reflection object given a type
//========================================================================

void CodeGenerator::GenerateTypeObject
(
    BCSYM *ptyp,
    Location *pLoc
)
{
    mdTypeRef   typref = m_pmdemit->DefineTypeRefBySymbol(ptyp->DigThroughAlias(), pLoc);
    // LDTOKEN pushes a RuntimeFieldHandle, RuntimeTypeHandle, or
    // RuntimeMethodHandle, which are struct types. Since it's going to be
    // consumed immediately, we don't bother to figure out and push the proper
    // type here.
    EmitOpcode_Tok(CEE_LDTOKEN, typref, UNSPILLABLE_TYPE_SYMBOL);

    GenerateCallToRuntimeHelper(GetTypeFromHandleMember, GetSymbolForFXType(FX::TypeType), pLoc);
}

//========================================================================
// Load a member reflection object given a member.
// This is used for expression trees, because we need to load the
// appropriate reflection object based on a member.
//========================================================================

void CodeGenerator::GenerateMemberObject
(
    ILTree::PILNode Tree
)
{
    ThrowIfNull( Tree );

    ILTree::PILNode TargetTree = Tree->AsExpressionWithChildren().Left;

    ThrowIfFalse( TargetTree->bilop == SX_SYM );

    BCSYM_NamedRoot *Symbol = TargetTree->AsSymbolReferenceExpression().pnamed;
    BCSYM_GenericBinding *Binding = TargetTree->AsSymbolReferenceExpression().GenericBindingContext;
    Location *Location = &Tree->Loc;
    BCSYM *ResultType = Tree->AsExpression().ResultType;
    mdMemberRef MemberRef;

    // Tesla #206 properly handle generic methods in generic types

    BCSYM_GenericTypeBinding *parentBinding = Binding &&
        !Binding->IsGenericTypeBinding() &&
        Symbol->IsProc() ? Binding->GetParentBinding() : NULL;

    MemberRef = m_pmdemit->DefineMemberRefBySymbol(
        Symbol->DigThroughAlias()->PNamedRoot(),
        Binding,
        Location);

    // LDTOKEN pushes a RuntimeFieldHandle, RuntimeTypeHandle, or
    // RuntimeMethodHandle, which are struct types. Since it's going to be
    // consumed immediately, we don't bother to figure out and push the proper
    // type here.
    EmitOpcode_Tok(CEE_LDTOKEN, MemberRef, UNSPILLABLE_TYPE_SYMBOL);

    // If this is a fake member added to implement some Windows Runtime interface then
    // get the original member's generic binding from the interface. If the interface is 
    // generic then we need to emit a call to a different overload of GetMethodFromHandle
    if (Symbol->DigThroughAlias()->PNamedRoot() &&
        Symbol->DigThroughAlias()->PNamedRoot()->IsProc() &&
        Symbol->DigThroughAlias()->PNamedRoot()->PProc()->IsFakeWindowsRuntimeMember())
    {
        Binding = Symbol->DigThroughAlias()->PNamedRoot()->PProc()->GetImplementsList()->GetGenericBindingContext();
    }

    mdTypeRef TypeRef;

    if (Binding && Binding->IsGenericTypeBinding())
    {
        TypeRef = m_pmdemit->DefineTypeRefBySymbol(Binding, Location);

        // LDTOKEN pushes a RuntimeFieldHandle, RuntimeTypeHandle, or
        // RuntimeMethodHandle, which are struct types. Since it's going to be
        // consumed immediately, we don't bother to figure out and push the proper
        // type here.
        EmitOpcode_Tok(CEE_LDTOKEN, TypeRef, UNSPILLABLE_TYPE_SYMBOL);

        if (Symbol->IsProc())
        {
            GenerateCallToRuntimeHelper(GetMethodFromHandleGenericMember, GetSymbolForFXType(FX::MethodBaseType), &Tree->Loc); // jtw need test case
        }
        else
        {
            GenerateCallToRuntimeHelper(GetFieldFromHandleGenericMember, GetSymbolForFXType(FX::FieldInfoType), &Tree->Loc); // jtw need test case
        }
    }
    else
    {
        if (Symbol->IsProc())
        {
            // Tesla #206 properly handle generic methods in generic types
            if(parentBinding)
            {
                TypeRef = m_pmdemit->DefineTypeRefBySymbol(parentBinding, Location);

                // LDTOKEN pushes a RuntimeFieldHandle, RuntimeTypeHandle, or
                // RuntimeMethodHandle, which are struct types. Since it's
                // going to be consumed immediately, we don't bother to figure
                // out and push the proper type here.
                EmitOpcode_Tok(CEE_LDTOKEN, TypeRef, UNSPILLABLE_TYPE_SYMBOL);

                GenerateCallToRuntimeHelper(GetMethodFromHandleGenericMember, GetSymbolForFXType(FX::MethodBaseType), &Tree->Loc); // jtw need test case
            }
            else
            {
                GenerateCallToRuntimeHelper(GetMethodFromHandleMember, GetSymbolForFXType(FX::MethodBaseType), &Tree->Loc); // jtw need test case
            }
        }
        else
        {
            GenerateCallToRuntimeHelper(GetFieldFromHandleMember, GetSymbolForFXType(FX::FieldInfoType), &Tree->Loc); // jtw need test case
        }
    }

    GenerateCastClass(ResultType, Location);
}

//========================================================================
// Generate the MetaType expression
//========================================================================

void CodeGenerator::GenerateMetaType
(
    ILTree::PILNode ptree
)
{
    VSASSERT(ptree->bilop == SX_METATYPE, "GenerateMetaType: SX_METATYPE node expected");
    VSASSERT(ptree->AsExpression().vtype == t_ref, "GenerateMetaType: MetaType should be ref expression");

    ILTree::PILNode TargetTree = ptree->AsExpressionWithChildren().Left;

    if (TargetTree->bilop == SX_NOTHING)
    {
        GenerateTypeObject(TargetTree->AsExpression().ResultType, &ptree->Loc);
    }
    else
    {
        // Microsoft: For expression trees, we need to be able to generate meta type information
        // for SX_SYM that represents runtime fields and methods. This is because quite often
        // expression trees require type information for a VB runtime method.

        VSASSERT(TargetTree->bilop == SX_SYM, "GenerateMetaType: SX_SYM node expected");

        BCSYM_NamedRoot *pnamed = TargetTree->AsSymbolReferenceExpression().pnamed;

        VSASSERT(pnamed->IsProc() || pnamed->IsVariable(), "GenerateMetaType: Must be a field or proc!");

        GenerateMemberObject(ptree);
    }
}

//========================================================================
// Generate binary math operation
//========================================================================

void CodeGenerator::GenerateMathOp
(
    BILOP op,
    Vtypes vtype,
    ILTree::PILNode pTree // [in] for passing along location information
)
{
    // All arithmetic is done on either 4-byte or 8-byte (or floating-point) values.
    // Arithmetic operations don't perform overflow checking by default.  When overflow
    // checking is necessary, we select the arithmetic opcode that peforms the right
    // kind of overflow checking (checking unsigned ranges versus signed ranges).  To
    // get 1-byte and 2-byte results, we peform a conversion after the math op, which
    // will do the overflow checking as well.

    // 

    switch (op)
    {
        case SX_MUL:
            if ((vtype == t_i4 || vtype == t_i8) && !m_NoIntChecks)
            {
                EmitOpcode(CEE_MUL_OVF, GetSymbolForVtype(vtype));
            }
            else if ((vtype == t_ui4 || vtype == t_ui8) && !m_NoIntChecks)
            {
                EmitOpcode(CEE_MUL_OVF_UN, GetSymbolForVtype(vtype));
            }
            else
            {
                EmitOpcode(CEE_MUL,GetSymbolForVtype(vtype));
            }
            break;

        case SX_MOD:
            if (IsUnsignedType(vtype))
            {
                EmitOpcode(CEE_REM_UN, GetSymbolForVtype(vtype));
            }
            else
            {
                EmitOpcode(CEE_REM, GetSymbolForVtype(vtype));
            }
            break;

        case SX_ADD:
            if ((vtype == t_i4 || vtype == t_i8) && !m_NoIntChecks)
            {
                EmitOpcode(CEE_ADD_OVF, GetSymbolForVtype(vtype));
            }
            else if ((vtype == t_ui4 || vtype == t_ui8) && !m_NoIntChecks)
            {
                EmitOpcode(CEE_ADD_OVF_UN, GetSymbolForVtype(vtype));
            }
            else
            {
                EmitOpcode(CEE_ADD, GetSymbolForVtype(vtype));
            }
            break;

        case SX_SUB:
            if ((vtype == t_i4 || vtype == t_i8) && !m_NoIntChecks)
            {
                EmitOpcode(CEE_SUB_OVF, GetSymbolForVtype(vtype));
            }
            else if ((vtype == t_ui4 || vtype == t_ui8) && !m_NoIntChecks)
            {
                EmitOpcode(CEE_SUB_OVF_UN, GetSymbolForVtype(vtype));
            }
            else
            {
                EmitOpcode(CEE_SUB, GetSymbolForVtype(vtype));
            }
            break;

        case SX_IDIV:
        case SX_DIV:
            if (IsUnsignedType(vtype))
            {
                EmitOpcode(CEE_DIV_UN, GetSymbolForVtype(vtype));
            }
            else
            {
                EmitOpcode(CEE_DIV, GetSymbolForVtype(vtype));
            }
            break;

        case SX_NEG:
            // CEE_NEG with overflow checking doesn't exist so we must do subtraction
            // from zero.  See GenerateBinOp()
            if ((vtype == t_i4 || vtype == t_i8) && !m_NoIntChecks)
            {
                // By the time we get here, the zero constant has been generated, so just do the subtraction
                EmitOpcode(CEE_SUB_OVF, GetSymbolForVtype(vtype));
            }
            else
            {
                EmitOpcode(CEE_NEG, GetSymbolForVtype(vtype));
            }
            break;
    }

    // The result of the math operation has either 4 or 8 byte width.
    // For 1 and 2 byte widths, convert the value back to the original type.

    if (vtype == t_i1 || vtype == t_ui1 || vtype == t_i2 || vtype == t_ui2)
    {
        GenerateConvertSimpleType(IsUnsignedType(vtype) ? t_ui4 : t_i4, vtype, !m_NoIntChecks, pTree);
    }
}

void
CodeGenerator::GenerateShift
(
    ILTree::PILNode ptree
)
{
    Vtypes ResultType = ptree->AsExpression().vtype;

    VSASSERT(ResultType == ptree->AsExpressionWithChildren().Left->vtype,
        "GenerateBinOp: result type should follow type of left operand");
    VSASSERT(ptree->AsExpressionWithChildren().Right->vtype == t_i4,
        "GenerateBinOp: right operand must be type Integer");

    GenerateRvalue(ptree->AsExpressionWithChildren().Left);
    GenerateRvalue(ptree->AsExpressionWithChildren().Right);

    if (ptree->bilop == SX_SHIFT_LEFT)
    {
        EmitOpcode(CEE_SHL, ptree->AsExpression().ResultType);
    }
    else if (IsUnsignedType(ResultType))
    {
        EmitOpcode(CEE_SHR_UN, ptree->AsExpression().ResultType);
    }
    else
    {
        EmitOpcode(CEE_SHR, ptree->AsExpression().ResultType);
    }

    // The result of the math operation has either 4 or 8-byte width.
    // For 1 and 2-byte widths, convert the value back to the original type.

    if (ResultType == t_i1 || ResultType == t_ui1 || ResultType == t_i2 || ResultType == t_ui2)
    {
        GenerateConvertSimpleType(IsUnsignedType(ResultType) ? t_ui4 : t_i4, ResultType, false, ptree);
    }
}

RuntimeMembers CodeGenerator::GetHelperForDecBinOp
(
BILOP bilop
)
{
    RuntimeMembers rtHelper;
    switch (bilop)
    {
    case SX_NEG: rtHelper = DecimalNegateMember; break;
    case SX_MOD: rtHelper = DecimalModuloMember; break;
    case SX_ADD: rtHelper = DecimalAddMember; break;
    case SX_SUB: rtHelper = DecimalSubtractMember; break;
    case SX_MUL: rtHelper = DecimalMultiplicationMember; break;
    case SX_DIV: rtHelper = DecimalDivisionMember; break;

    case SX_EQ:
    case SX_NE:
    case SX_LE:
    case SX_GE:
    case SX_LT:
    case SX_GT:
    case SX_POW:
    case SX_NOT:
    case SX_AND:
    case SX_OR:
    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
    default:
        rtHelper = UndefinedRuntimeMember;
        VSFAIL("GenerateDecBinOp: unrecognized node.");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }

    return rtHelper;
}

//========================================================================
// Generate binary operator for decimal
//========================================================================

void CodeGenerator::GenerateDecBinOp
(
ILTree::PILNode ptree
)
{
    GenerateRvalue(ptree->AsExpressionWithChildren().Left);

    if (ptree->AsExpressionWithChildren().Right)
    {
        GenerateRvalue(ptree->AsExpressionWithChildren().Right);
    }

    if (ptree->bilop != SX_PLUS)
    {
        RuntimeMembers rtHelper = GetHelperForDecBinOp(ptree->bilop);
        GenerateCallToRuntimeHelper(rtHelper, ptree->AsExpression().ResultType, &ptree->Loc);
    }
}

//========================================================================
// Generate Like function call
//========================================================================

void CodeGenerator::GenerateLike
(
ILTree::PILNode ptree
)
{
    

    VSASSERT( ptree->bilop == SX_LIKE,
               "GenerateLike: ptree must be SX_LIKE" );
    VSASSERT( ptree->AsExpression().vtype == t_bool || ptree->AsExpression().vtype == t_ref,
               "GenerateLike: SX_LIKE must be type t_bool or t_ref" );
    VSASSERT( ptree->AsExpressionWithChildren().Left && ptree->AsExpressionWithChildren().Right,
               "GenerateLike: must have op1 and op2" );
    VSASSERT( ptree->AsExpressionWithChildren().Left->vtype == ptree->AsExpressionWithChildren().Left->vtype,
               "GenerateLike: lhs and rhs of SX_LIKE must be same type" );


    GenerateRvalue(ptree->AsExpressionWithChildren().Left);
    GenerateRvalue(ptree->AsExpressionWithChildren().Right);

    if (ptree->uFlags & SXF_RELOP_TEXT)
    {
        GenerateLiteralInt(1);
    }
    else
    {
        GenerateLiteralInt(0);
    }

    RuntimeMembers rtHelper;
    BCSYM* pRuntimeHelperReturnType;

    if (ptree->AsExpressionWithChildren().Left->vtype == t_ref)
    {
        rtHelper = LikeObjectMember;
        pRuntimeHelperReturnType = GetSymbolForFXType(FX::ObjectType);
    }
    else
    {
        rtHelper = LikeStringMember;
        pRuntimeHelperReturnType = GetSymbolForVtype(t_bool);
    }

    GenerateCallToRuntimeHelper(rtHelper, pRuntimeHelperReturnType, &ptree->Loc);
}

/*========================================================================

Variant operator expressions

========================================================================*/
RuntimeMembers CodeGenerator::GetHelperForObjBinOp
(
    BILOP bilop
)
{
    RuntimeMembers rtHelper = UndefinedRuntimeMember;

    switch (bilop)
    {
    case SX_PLUS:        rtHelper = PlusObjectMember; break;
    case SX_NEG:         rtHelper = NegateObjectMember; break;
    case SX_NOT:         rtHelper = NotObjectMember; break;
    case SX_AND:         rtHelper = AndObjectMember; break;
    case SX_OR:          rtHelper = OrObjectMember; break;
    case SX_XOR:         rtHelper = XorObjectMember; break;
    case SX_ADD:         rtHelper = AddObjectMember; break;
    case SX_SUB:         rtHelper = SubtractObjectMember; break;
    case SX_MUL:         rtHelper = MultiplyObjectMember; break;
    case SX_DIV:         rtHelper = DivideObjectMember; break;
    case SX_POW:         rtHelper = ExponentObjectMember; break;
    case SX_MOD:         rtHelper = ModObjectMember; break;
    case SX_IDIV:        rtHelper = IntDivideObjectMember; break;
    case SX_SHIFT_LEFT:  rtHelper = LeftShiftObjectMember; break;
    case SX_SHIFT_RIGHT: rtHelper = RightShiftObjectMember; break;
    case SX_CONC:        rtHelper = ConcatenateObjectMember; break;
    default:
        rtHelper = UndefinedRuntimeMember;
        VSFAIL("GenerateObjBinOp: unrecognized node.");
        VbThrow(HrMake(ERRID_CodegenError));
        break;
    }

    return rtHelper;
}

//========================================================================
// Generate object binary operator
//========================================================================

void CodeGenerator::GenerateObjBinOp
(
ILTree::PILNode ptree
)
{
    RuntimeMembers rtHelper;

    GenerateRvalue(ptree->AsExpressionWithChildren().Left);

    if (ptree->AsExpressionWithChildren().Right)
    {
        GenerateRvalue(ptree->AsExpressionWithChildren().Right);
    }

    rtHelper = GetHelperForObjBinOp(ptree->bilop);

    GenerateCallToRuntimeHelper(rtHelper, ptree->AsExpression().ResultType, &ptree->Loc);
}

/*========================================================================

Language runtime routines

========================================================================*/

//========================================================================
// Load a RT field. Assumes object already loaded
//========================================================================

void CodeGenerator::GenerateRTField
(
RuntimeMembers rtHelper,
Vtypes type
)
{
    RuntimeMemberDescriptor  * prtdesc;
    OPCODE                 opcode;
    mdMemberRef            memref;

    VerifyRTLangEnum(rtHelper);

    prtdesc = &(g_rgRTLangMembers[rtHelper]);

    VSASSERT(prtdesc->usFlags & RTF_FIELD,
               "GenerateRTField: rtHelper must be a field.");

    if (prtdesc->usFlags & RTF_VIRTUAL)
    {
        opcode = CEE_LDFLD;
    }
    else
    {
        VSASSERT(prtdesc->usFlags & RTF_STATIC,
                   "GenerateRTField: rt field must be static or virtual");

        opcode = CEE_LDSFLD;
    }

    memref = m_pmdemit->DefineRTMemberRef(rtHelper);
    EmitOpcode_Tok(opcode, memref, GetSymbolForVtype(type));
}

//========================================================================
// Throws a runtime exception
//========================================================================

void CodeGenerator::GenerateRTException
(
    HRESULT hrErr, // id of the error to throw
    Location *pLoc // location info of the statement that generated this call
)
{
    GenerateLiteralInt(hrErr);
    // Construct the runtime exception
    GenerateCallToRuntimeHelper(CreateErrorMember, GetSymbolForFXType(FX::ExceptionType), pLoc);
    EndCodeBuffer(CEE_THROW);
    SetAsCurrentCodeBlock(0);
}


/*****************************************************************************
;LogRuntimeHelperNotFoundError

Gets the appropriate error id when a given runtime helper method is not available
*****************************************************************************/
void CodeGenerator::LogRuntimeHelperNotFoundError(
    RuntimeMembers RuntimeHelperMethod, // the helper that we couldn't find
    Location *ErrorLocation, // what to squiggle if the runtime helper isn't defined
    MISSING_HELPER_ERROR_KIND ErrorKind // what level of diagnostic to issue
)
{
    if ( ErrorKind == Ignore )
    {
        // Sometimes we don't log an error, such as when calling helpers in try/catch blocks to set the err object.
        // In those cases, we prefer to log direct attempts to use the err object rather than log an error on what
        // otherwise is a benign try/catch block.
        return;
    }

    StringBuffer NameOfMissingType;
    NameOfMissingType.AppendString( g_rgRTLangClasses[ g_rgRTLangMembers[ RuntimeHelperMethod ].rtParent ].wszClassName);
    NameOfMissingType.AppendChar( L'.' );
    NameOfMissingType.AppendString( g_rgRTLangMembers[ RuntimeHelperMethod ].wszName);
    m_pmdemit->GetBuilder()->GetErrorTable()->CreateError( ERRID_MissingRuntimeHelper, ErrorLocation, NameOfMissingType.GetString());
}

/*****************************************************************************
;GenerateCallToRuntimeHelper

Attempts to generate a call to the specified runtime helper.  It the runtime helper is not defined for the
targeted library, an error is generated on the statement that generated the call to the helper method.
For instance, if s1 < s2 where s1,s2 are strings was encountered but the string runtime helpers to do the
conversion were not available on the targeted platform, we'd squiggle s1<s2
*****************************************************************************/
void CodeGenerator::GenerateCallToRuntimeHelper
(
    RuntimeMembers HelperMethodToCall, // the runtime method that we need to generate a call to
    BCSYM* pResultType, // The type of the object returned by the call
    Location *ErrorLocation, // what to squiggle if the runtime helper isn't defined
    MISSING_HELPER_ERROR_KIND ErrorKind, // [defaults to Error] what level of diagnostic (error, warning, ignore)
    BCSYM_GenericBinding *GenericBindingContext // [defaults to NULL]
)
{
    OPCODE opcode;
    mdMemberRef memref = 0;

    // Assumes arg list and object of call (if needed) has already been generated

    RuntimeMemberDescriptor *HelperMethodDescriptor = &( g_rgRTLangMembers[ HelperMethodToCall ] );

    VSASSERT( HelperMethodDescriptor->usFlags & RTF_METHOD, "GenerateCallToRuntimeHelper: must be calling a runtime method" );
    if (HelperMethodDescriptor->usFlags & RTF_CONSTRUCTOR && !(HelperMethodDescriptor->usFlags & RTF_NONVIRTUAL))
    {
        opcode = CEE_NEWOBJ;
    }
    else if (HelperMethodDescriptor->usFlags & (RTF_STATIC | RTF_NONVIRTUAL))
    {
        opcode = CEE_CALL;
    }
    else if (HelperMethodDescriptor->usFlags & RTF_VIRTUAL)
    {
        opcode = CEE_CALLVIRT;
    }
    else
    {
        VSFAIL("GenerateCallToRuntimeHelper: RT function must have call type flag set");
        VbThrow(HrMake(ERRID_CodegenError));
    }

    if ( GenericBindingContext == NULL )
    {
        memref = m_pmdemit->DefineRTMemberRef( HelperMethodToCall );
        if ( memref == mdTokenNil )
        {
            LogRuntimeHelperNotFoundError( HelperMethodToCall, ErrorLocation, ErrorKind );  // the runtime helper isn't defined (happens on some platforms such as Silverlight)
        }
    }
    else
    {
        BCSYM_NamedRoot *RuntimeSymbol = m_Project->GetCompilerHost()->GetSymbolForRuntimeMember(HelperMethodToCall, m_pCompilationCaches);
        if ( RuntimeSymbol )
        {
            // Note that this assert could also fail for a non-generic method in a generic type. But this
            // is intentional because we don't expect such uses currently and want to be warned for such cases.
            VSASSERT(RuntimeSymbol->IsGeneric(), "Generic binding for a non-generic method unexpected!!!");
            memref = m_pmdemit->DefineMemberRefBySymbol(RuntimeSymbol, GenericBindingContext, NULL);
        }
        else
        {
            LogRuntimeHelperNotFoundError( HelperMethodToCall, ErrorLocation, ErrorKind );  // the runtime helper isn't defined (happens on some platforms such as Silverlight)
        }
    }
#if DEBUG
        DumpCall(m_Project->GetCompilerHost()->GetSymbolForRuntimeMember(HelperMethodToCall, m_pCompilationCaches)->PProc());
#endif

    // Even if we couldn't find the runtime symbol to emit a call to, we still need to adjust the stack
    // so that the stack tracking we do in code gen doesn't get all out of whack.  We won't be emitting
    // code so it is somewhat moot but we'll assert all over the place if we don't keep it in line with what
    // is expected based on the arguments pushed as arguments for this helper call, etc.

    #pragma warning (push)
    #pragma warning (disable:6001) // If opcode wasn't initialized, we would have thrown an exception above.  memref may be 0 in the event the runtime helper isn't defined in the runtime library we are going against.
    EmitOpcode_Tok(opcode, memref, Symbols::GetVoidType());
    #pragma warning (pop)

    // We need to adjust the stack to reflect arguments pushed for the runtime helper method and any return from the runtime helper method
    if (HelperMethodDescriptor->usFlags & (RTF_NONVIRTUAL | RTF_VIRTUAL))
    {
        AddToCurStack(-1);
        m_stackTypes.PopOrDequeue();
    }

     // Args
    Vtypes *HelperMethodArgs = HelperMethodDescriptor->vtypArgs;
    while (*HelperMethodArgs != t_UNDEF)
    {
        if (*HelperMethodArgs == t_ptr)
        {
            AddToCurStack(-1);
            m_stackTypes.PopOrDequeue();
            HelperMethodArgs += 2;
        }
        else
        {
            AddToCurStack(-1);
            m_stackTypes.PopOrDequeue();
            HelperMethodArgs++;
        }
    }

    
    if (opcode == CEE_NEWOBJ)
    {
        VSASSERT(HelperMethodDescriptor->vtypRet == t_void, "Might be about to push the same type twice.");
        // m_usCurStackLevel has been incremented by EmitOpcode_Tok already,
        // but we can't push a t_ref onto m_stackTypes until after we've popped
        // the arguments off.
        m_stackTypes.PushOrEnqueue(pResultType);
    }

    // Return
    if (HelperMethodDescriptor->vtypRet != t_void)
    {
        AddToCurStack(1);
        m_stackTypes.PushOrEnqueue(pResultType);
    }
    else if (HelperMethodToCall != ClearErrorMember &&
             HelperMethodToCall != SetErrorMember &&
             HelperMethodToCall != SetErrorMemberWithLineNum)
    {
        // The IP always points to the location where execution will return
        // So we generate a NOP here so that the IP is still associated with
        // the call statement and the user can double click in the callstack
        // window and take hime to the calling line.
        InsertNOP();
    }
}

/*========================================================================

Conditional and branch routines

========================================================================*/

//========================================================================
// Generates a branch instruction. Assumes arguments pushed.
//========================================================================

void
CodeGenerator::GenerateConditionalBranch
(
    ILTree::PILNode ptreeCond,
    bool fReverse,
    CODE_BLOCK *pcblkTrue,
    CODE_BLOCK *pcblkFalse
)
{
    VSASSERT(ptreeCond->AsExpression().vtype == t_bool, "conditional branch using non-boolean result");

    switch (ptreeCond->bilop)
    {
    case SX_NOT:
        GenerateConditionalBranch(ptreeCond->AsExpressionWithChildren().Left, !fReverse, pcblkFalse, pcblkTrue);
        return;

    case SX_ISTYPE:
        // The result of an IsInst opcode leaves a reference on the stack.
        // We can use this reference as the boolean result of the TypeOf..Is expression.
        GenerateIsType(ptreeCond);

        if (fReverse)
        {
            EndCodeBuffer(CEE_BRFALSE_S, pcblkFalse);
        }
        else
        {
            EndCodeBuffer(CEE_BRTRUE_S, pcblkTrue);
        }
        break;

    case SX_IS:
    case SX_ISNOT:
        // If comparing against Nothing, we can use the reference itself
        // as the result of the comparison.
        if (ptreeCond->AsExpressionWithChildren().Left->bilop == SX_NOTHING ||
            ptreeCond->AsExpressionWithChildren().Right->bilop == SX_NOTHING)
        {
            if (ptreeCond->AsExpressionWithChildren().Left->bilop == SX_NOTHING)
            {
                GenerateRvalue(ptreeCond->AsExpressionWithChildren().Right);
            }
            else
            {
                GenerateRvalue(ptreeCond->AsExpressionWithChildren().Left);
            }

            if (fReverse)
            {
                EndCodeBuffer((ptreeCond->bilop == SX_IS) ? CEE_BRTRUE_S : CEE_BRFALSE_S , pcblkFalse);
            }
            else
            {
                EndCodeBuffer((ptreeCond->bilop == SX_IS) ? CEE_BRFALSE_S : CEE_BRTRUE_S , pcblkTrue);
            }

            break;
        }
        __fallthrough; // fall through if not comparing against Nothing

    case SX_EQ:
    case SX_NE:
    case SX_LE:
    case SX_LT:
    case SX_GE:
    case SX_GT:
        // Relational operator. Do special handling
        GenerateRelOpBranch(ptreeCond, fReverse, pcblkTrue, pcblkFalse);
        break;

    case SX_CNS_INT:
        if (m_Project->GenerateOptimalIL())
        {
            // Change the conditional branch into an unconditional branch if the condition
            // is known at compile time.  Unreachable code elimination will clean up the mess.

            bool Value = ptreeCond->AsIntegralConstantExpression().Value;

            if (fReverse)
            {
                if (Value == false)
                {
                    EndCodeBuffer(CEE_BR_S, pcblkFalse);
                }
                else
                {
                    InsertNOP();  // This allows breakpoints on the condition.
                }
            }
            else
            {
                if (Value == false)
                {
                    InsertNOP();  // This allows breakpoints on the condition.
                }
                else
                {
                    EndCodeBuffer(CEE_BR_S, pcblkTrue);
                }
            }
            break;
        }
        __fallthrough; // Fall through when not optimizing IL.

    default:
        // Not a relop. Just generate the value and do a boolean jump.
        GenerateRvalue(ptreeCond);

        if (fReverse)
        {
            EndCodeBuffer(CEE_BRFALSE_S, pcblkFalse);
        }
        else
        {
            EndCodeBuffer(CEE_BRTRUE_S, pcblkTrue);
        }
        break;
    }

    if (fReverse)
    {
        SetAsCurrentCodeBlock(pcblkTrue);
    }
    else
    {
        SetAsCurrentCodeBlock(pcblkFalse);
    }
}

//========================================================================
// Generates a short-circuit OR
//========================================================================

void CodeGenerator::GenerateLogicalOr
(
ILTree::PILNode ptree,
bool fReverse,
CODE_BLOCK *pcblkTrue,
CODE_BLOCK *pcblkFalse
)
{
    CODE_BLOCK * pcblkNewFalse;
    ILTree::PILNode ptreeL = ptree->AsExpressionWithChildren().Left;
    ILTree::PILNode ptreeR = ptree->AsExpressionWithChildren().Right;

    pcblkNewFalse = NewCodeBlock();

    if (!IsShortCircuitOperator(ptreeL->bilop))
    {
        // On true of ptreeL, jump to true and
        // set new false block to be current so we can generate the ptreeR
        //
        GenerateConditionalBranch(ptreeL, false, pcblkTrue, pcblkNewFalse);
    }
    else
    {
        CODE_BLOCK * pcblkNewTrue;

        pcblkNewTrue = NewCodeBlock();

        if (ptreeL->bilop == SX_ORELSE)
        {
            GenerateLogicalOr(ptreeL, true, pcblkNewTrue, pcblkNewFalse);
        }
        else
        {
            VSASSERT(ptreeL->bilop == SX_ANDALSO,
                     "GenerateLogicalOr: only ANDALSO or ORELSE can be short cirucuited.");

            GenerateLogicalAnd(ptreeL, true, pcblkNewTrue, pcblkNewFalse);
        }

        VSASSERT(m_pcblkCurrent == pcblkNewTrue,
                 "current should be set to the new true block.");

        // Generate a branch to the actual True block so we can generate
        // the ptreeR in pcblkNewFalse
        //
        EndCodeBuffer(CEE_BR_S, pcblkTrue);
        SetAsCurrentCodeBlock(pcblkNewFalse);
    }

    VSASSERT(m_pcblkCurrent == pcblkNewFalse,
             "expecting to generate ptreeR in pcblkNewFalse.");

    // Now generate ptreeR in new code block
    if (!IsShortCircuitOperator(ptreeR->bilop))
    {
        // On false of ptreeR, jump out to the passed in false block
        // and set the passed in true block to be current.
        //
        GenerateConditionalBranch(ptreeR, fReverse, pcblkTrue, pcblkFalse);
    }
    else
    {
        if (ptreeR->bilop == SX_ORELSE)
        {
            GenerateLogicalOr(ptreeR, fReverse, pcblkTrue, pcblkFalse);
        }
        else
        {
            VSASSERT(ptreeR->bilop == SX_ANDALSO,
                     "GenerateLogicalOr: logop can only be ANDALSO or ORELSE.");

            GenerateLogicalAnd(ptreeR, fReverse, pcblkTrue, pcblkFalse);
        }
    }
}

//========================================================================
// Generates a short circuit and
//========================================================================

void CodeGenerator::GenerateLogicalAnd
(
ILTree::PILNode ptree,
bool fReverse,
CODE_BLOCK * pcblkTrue,
CODE_BLOCK * pcblkFalse
)
{
    CODE_BLOCK * pcblkNewTrue;
    ILTree::PILNode ptreeL = ptree->AsExpressionWithChildren().Left;
    ILTree::PILNode ptreeR = ptree->AsExpressionWithChildren().Right;

    pcblkNewTrue = NewCodeBlock();

    if (!IsShortCircuitOperator(ptreeL->bilop))
    {
        // On false of ptreeL, jump to false and
        // set new true block to be current so we can generate the ptreeR
        //
        GenerateConditionalBranch(ptreeL, true, pcblkNewTrue, pcblkFalse);
    }
    else
    {
        if (ptreeL->bilop == SX_ORELSE)
        {
            GenerateLogicalOr(ptreeL, true, pcblkNewTrue, pcblkFalse);
        }
        else
        {
            VSASSERT(ptreeL->bilop == SX_ANDALSO,
                     "GenerateLogicalAnd: only ANDALSO or ORELSE can be short circuited.");

            GenerateLogicalAnd(ptreeL, true, pcblkNewTrue, pcblkFalse);
        }
    }

    VSASSERT(m_pcblkCurrent == pcblkNewTrue,
             "must be a new true block for ptreeR");

    // Now generate ptreeR in new code block
    if (!IsShortCircuitOperator(ptreeR->bilop))
    {
        // On false of ptreeR, jump out to the passed in false block
        // and set the passed in true block to be current.
        //
        GenerateConditionalBranch(ptreeR, fReverse, pcblkTrue, pcblkFalse);
    }
    else
    {
        if (ptreeR->bilop == SX_ORELSE)
        {
            GenerateLogicalOr(ptreeR, fReverse, pcblkTrue, pcblkFalse);
        }
        else
        {
            VSASSERT(ptreeR->bilop == SX_ANDALSO,
                     "GenerateLogicalOr: only be ANDALSO or ORELSE can be short circuited.");

            GenerateLogicalAnd(ptreeR, fReverse, pcblkTrue, pcblkFalse);
        }
    }
}

//========================================================================
// Generates a general condition expression (ptreeCond is not necessarily
// a relop or a logop)
//========================================================================

void
CodeGenerator::GenerateCondition
(
    ILTree::PILNode ptreeCond,
    bool fReverse,
    CODE_BLOCK *pcblkTrue,
    CODE_BLOCK *pcblkFalse
)
{
    if (IsShortCircuitOperator(ptreeCond->bilop))
    {
        if (ptreeCond->bilop == SX_ORELSE)
        {
            GenerateLogicalOr(ptreeCond, fReverse, pcblkTrue, pcblkFalse);
        }
        else
        {
            GenerateLogicalAnd(ptreeCond, fReverse, pcblkTrue, pcblkFalse);
        }
    }
    else
    {
        GenerateConditionalBranch(ptreeCond, fReverse, pcblkTrue, pcblkFalse);
    }
}

void
CodeGenerator::GenerateConditionWithENCRemapPoint
(
    ILTree::PILNode ptreeCond,         // [in]
    bool fReverse,              // [in]
    CODE_BLOCK **ppcblkTrue,    // [in, out]
    CODE_BLOCK **ppcblkFalse    // [in, out]
)
{
    if (m_Project->GenerateENCableCode())
    {
        if (ptreeCond->AsExpression().ResultType->GetVtype() == t_bool)
        {
            GenerateRvalue(ptreeCond);

            InsertENCRemappablePoint(ptreeCond->AsExpression().ResultType);

            if (fReverse)
            {
                EndCodeBuffer(CEE_BRFALSE_S, *ppcblkFalse);
                SetAsCurrentCodeBlock(*ppcblkTrue);
            }
            else
            {
                EndCodeBuffer(CEE_BRTRUE_S, *ppcblkTrue);
                SetAsCurrentCodeBlock(*ppcblkFalse);
            }
        }
        else
        {
            GenerateCondition(ptreeCond, fReverse, *ppcblkTrue, *ppcblkFalse);

            InsertENCRemappablePoint(ppcblkTrue, ppcblkFalse, fReverse);
        }
    }
    else
    {
        GenerateCondition(ptreeCond, fReverse, *ppcblkTrue, *ppcblkFalse);
    }
}

//========================================================================
// Generates a short-circuit Object OR
//========================================================================

void
CodeGenerator::GenerateObjectOr
(
    ILTree::PILNode ptree,
    CODE_BLOCK *pcblkExpressionIsTrue,
    bool fLastTerm
)
{
    VSASSERT(ptree->AsExpression().vtype == t_ref,
             "GenerateObjectOr: if this isn't t_ref, we're using the wrong short circuiting code!");

    if (!IsShortCircuitOperator(ptree->bilop))
    {
        VSASSERT(ptree->AsExpression().vtype == t_ref,
                 "GenerateObjectOr: if this isn't t_ref, than what is it doing here?");

        GenerateRvalue(ptree);
        GenerateCallToRuntimeHelper(ObjectToBooleanMember, ptree->AsExpression().ResultType, &ptree->Loc);

        // If this is not the last term, then make the check to break early.
        // If this is the last term, the answer we are looking for is actually the result of
        // the last term's evaluation, so we will branch directly to the end of the expression
        // but this branch is generated by the first caller
        //
        if (!fLastTerm)
        {
            EndCodeBuffer(CEE_BRTRUE_S, pcblkExpressionIsTrue);
            SetAsCurrentCodeBlock(0);
        }
    }
    else if (ptree->bilop == SX_ORELSE)
    {
        GenerateObjectOr(ptree->AsExpressionWithChildren().Left, pcblkExpressionIsTrue, false);
        GenerateObjectOr(ptree->AsExpressionWithChildren().Right, pcblkExpressionIsTrue, fLastTerm);
    }
    else
    {
        VSASSERT(ptree->bilop == SX_ANDALSO,
                 "GenerateObjectOr: only ANDALSO or ORELSE can be short circuited.");

        CODE_BLOCK *pcblkSubExpressionIsFalse = NewCodeBlock();

        GenerateObjectAnd(ptree->AsExpressionWithChildren().Left, pcblkSubExpressionIsFalse, false);
        GenerateObjectAnd(ptree->AsExpressionWithChildren().Right, pcblkSubExpressionIsFalse, fLastTerm);

        // branch away early if sub expression is true
        EndCodeBuffer(CEE_BR_S, pcblkExpressionIsTrue);
        // otherwise keep going
        SetAsCurrentCodeBlock(pcblkSubExpressionIsFalse);
    }
}

//========================================================================
// Generates a short-circuit Object And
//========================================================================

void
CodeGenerator::GenerateObjectAnd
(
    ILTree::PILNode ptree,
    CODE_BLOCK *pcblkExpressionIsFalse,
    bool fLastTerm
)
{
    VSASSERT(ptree->AsExpression().vtype == t_ref,
             "GenerateObjectAnd: if this isn't t_ref, we're using the wrong short circuiting code!");

    if (!IsShortCircuitOperator(ptree->bilop))
    {
        VSASSERT(ptree->AsExpression().vtype == t_ref,
                 "GenerateObjectAnd: if this isn't t_ref, than what is it doing here?");

        GenerateRvalue(ptree);
        GenerateCallToRuntimeHelper(ObjectToBooleanMember, ptree->AsExpression().ResultType, &ptree->Loc);

        // If this is not the last term, then make the check to break early.
        // If this is the last term, the answer we are looking for is actually the result of
        // the last term's evaluation, so we will branch directly to the end of the expression
        // but this branch is generated by the first caller
        //
        if (!fLastTerm)
        {
            EndCodeBuffer(CEE_BRFALSE_S, pcblkExpressionIsFalse);
            SetAsCurrentCodeBlock(0);
        }
    }
    else if (ptree->bilop == SX_ORELSE)
    {
        CODE_BLOCK *pcblkSubExpressionIsTrue = NewCodeBlock();

        GenerateObjectOr(ptree->AsExpressionWithChildren().Left, pcblkSubExpressionIsTrue, false);
        GenerateObjectOr(ptree->AsExpressionWithChildren().Right, pcblkSubExpressionIsTrue, fLastTerm);

        // branch away early if sub expression is false
        EndCodeBuffer(CEE_BR_S, pcblkExpressionIsFalse);
        // otherwise keep going
        SetAsCurrentCodeBlock(pcblkSubExpressionIsTrue);
    }
    else
    {
        VSASSERT(ptree->bilop == SX_ANDALSO,
                 "GenerateObjectAnd: only ANDALSO or ORELSE can be short circuited.");

        GenerateObjectAnd(ptree->AsExpressionWithChildren().Left, pcblkExpressionIsFalse, false);
        GenerateObjectAnd(ptree->AsExpressionWithChildren().Right, pcblkExpressionIsFalse, fLastTerm);
    }
}

//========================================================================
// Generates a short-circuit Object expression
// After this function finishes, either a boxed True or a boxed False
// will be on the stack.
//========================================================================

void
CodeGenerator::GenerateObjectShortCircuitExpression
(
    ILTree::PILNode ptree
)
{
    CODE_BLOCK *pcblkEndOfIt = NewCodeBlock();
    CODE_BLOCK *pcblkLoadTrue = NewCodeBlock();
    CODE_BLOCK *pcblkLoadFalse = NewCodeBlock();

    VSASSERT(ptree->AsExpression().vtype == t_ref,
             "GenerateObjectShortCircuitExpression: this isn't an object!");
    VSASSERT(ptree->bilop == SX_ORELSE || ptree->bilop == SX_ANDALSO,
             "GenerateObjectShortCircuitExpression: this isn't a short-circuitable operator!");

    if (ptree->bilop == SX_ORELSE)
    {
        GenerateObjectOr(ptree->AsExpressionWithChildren().Left, pcblkLoadTrue, false);
        GenerateObjectOr(ptree->AsExpressionWithChildren().Right, pcblkLoadTrue, false);

        // Because the recursion algorithm will not generate an early check
        // for the last term, we generate it now.  The reason we do this is
        // because only an unconditional branch is needed - if the last term ever gets
        // evaluated, then the result of that evaluation is the answer we want.
        //
        //AddToCurStack(-1);  // consumed later
        //EndCodeBuffer(CEE_BR_S, pcblkEndOfIt);

        SetAsCurrentCodeBlock(pcblkLoadFalse);
        GenerateBoxedBoolean(false);
        AddToCurStack(-1); // consumed later
        m_stackTypes.PopOrDequeue();
        EndCodeBuffer(CEE_BR_S, pcblkEndOfIt);

        SetAsCurrentCodeBlock(pcblkLoadTrue);
        GenerateBoxedBoolean(true);
    }
    else
    {
        GenerateObjectAnd(ptree->AsExpressionWithChildren().Left, pcblkLoadFalse, false);
        GenerateObjectAnd(ptree->AsExpressionWithChildren().Right, pcblkLoadFalse, false);

        // Because the recursion algorithm will not generate an early check
        // for the last term, we generate it now.  The reason we do this is
        // because only an unconditional branch is needed - if the last term ever gets
        // evaluated, then the result of that evaluation is the answer we want.
        //
        //AddToCurStack(-1);  // consumed later
        //EndCodeBuffer(CEE_BR_S, pcblkEndOfIt);

        SetAsCurrentCodeBlock(pcblkLoadTrue);
        GenerateBoxedBoolean(true);
        AddToCurStack(-1); // consumed later
        m_stackTypes.PopOrDequeue();
        EndCodeBuffer(CEE_BR_S, pcblkEndOfIt);

        SetAsCurrentCodeBlock(pcblkLoadFalse);
        GenerateBoxedBoolean(false);
    }

    SetAsCurrentCodeBlock(pcblkEndOfIt);

    // NOP is required here in the non-object case to create a sequence point for proper stepping on x64.
    // Stepping in the object case appears to behave correctly on x64, but for consistency and just in
    // case we also put a NOP here.
    InsertNOP();
}

void
CodeGenerator::GenerateBoxedBoolean
(
    bool Value
)
{
    mdTypeRef typref = m_pmdemit->DefineTypeRefBySymbol(
        m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_bool),
        NULL);
    GenerateLiteralInt(Value ? COMPLUS_TRUE : COMPLUS_FALSE);
    EmitOpcode_Tok(CEE_BOX, typref, GetSymbolForFXType(FX::ObjectType));
}

void
CodeGenerator::GenerateAsyncSpill
(
    _In_ ILTree::AsyncSpillExpression* pSpill
)
{
    VSASSERT(pSpill != NULL, "AsyncSpillExpression cannot be null!");
    VSASSERT(pSpill->IsCompletedCondition != NULL, "The IsCompleted condition must be defined!");
    VSASSERT(pSpill->StateAssignment != NULL, "The state assignment must be defined!");
    // pSpill->CreateAwaiterArray might be null if it's not needed...
    VSASSERT(pSpill->AwaitXAssignment != NULL, "Expect not NULL" ); 
    VSASSERT(pSpill->OnCompletedCall != NULL, "The OnCompleted call must be defined!");
    // pSpill->OnCompletedExtraCondition will only be non-null in late-bound awaits
    // pSpill->OnCompletedExtraIfTrue will only be non-null in late-bound awaits
    // pSpill->OnCompletedExtraIfFalse1 will only be non-null in late-bound awaits
    // pSpill->OnCompletedExtraIfFalse2 will only be non-null in late-bound awaits
    VSASSERT(pSpill->DoFinallyBodiesAssignment != NULL, "The DoFinallyBodies assignment must be defined!");
    VSASSERT(pSpill->StateResetAssignment != NULL, "The state assignment must be defined!");
    VSASSERT(pSpill->RestoreAwaiterStatements != NULL, "The state reset assignment must be defined!");
    VSASSERT(pSpill->StackStorageReference != NULL, "The stack storage field must be defined!");
    VSASSERT(pSpill->ResumeLabel != NULL, "The Resume label must be defined!");

    BackupValue<bool> backup( &m_IsGeneratingAwait );
    m_IsGeneratingAwait = true;
    //
    // We generate this:
    //
    // ...                                        | the "current block" as of entry to this function
    // If <IsCompletedCondition> Then             +--
    //     SPILL(<StackStorageReference>)         |
    //     <StateAssignment>                      | pIfBodyBlock1
    //     PDB-AWAIT-YIELD-POINT                  |
    //     <CreateAwaiterArray>                   | 
    //     <AwaitXAssignment>                     |
    //     <OnCompletedCall>                      |
    //     If <OnCompletedExtraCondition> Then    +--
    //       <OnCompletedExtraIfTrue>             | pExtraIfTrueBlock
    //     Else                                   +--
    //       <OnCompletedExtraIfFalse1>           | pExtraIfFalseBlock
    //       <OnCompletedExtraIfFalse2>           | 
    //     End If                                 +--
    //     <DoFinallyBodiesAssignment>            | pIfBodyBlock2
    //     Return                                 | 
    // <ResumeLabel>:                             +---
    //     PDB-AWAIT-RESUME-POINT                 |
    //     <StateResetAssignment>                 | ResumeLabel->TargetCodeBlock
    //     <RestoreAwaiterStatements>             |
    //     UNSPILL(<StackStorageReference>)       |
    // End If                                     +--
    // ...                                        | pAfterIfBlock; we leave this as the current one

    // We need to make sure there is no current hydration context, to avoid
    // dehydrating parts of the spilling operation.
    VSASSERT(m_currentHydrationContext == NULL, "We should not have a hydration context here; this can only end badly.");

    CODE_BLOCK* pIfBodyBlock1 = NewCodeBlock();
    CODE_BLOCK* pIfBodyBlock2 = NewCodeBlock();
    CODE_BLOCK* pAfterIfBlock = NewCodeBlock();
    CODE_BLOCK* pExtraIfTrueBlock = NULL;
    CODE_BLOCK* pExtraIfFalseBlock = NULL;
    if (pSpill->OnCompletedExtraCondition != NULL)
    {
        pExtraIfTrueBlock = NewCodeBlock();
        pExtraIfFalseBlock = NewCodeBlock();
    }

    VSASSERT(pSpill->ResumeLabel->TargetCodeBlock != NULL, "The code block for the Resume label should have been created already!");
    GenerateCondition(pSpill->IsCompletedCondition,
                      false /* fReverse */,
                      pAfterIfBlock,
                      pIfBodyBlock1);

    // The call to GenerateCondition has left pIfBodyBlock1 as the current block.

    // Emit code to spill the stack.
    BCSYM* pSpilledType;
    unsigned int itemsSpilled;

    int stackDepthForErrorCase = 0;
    bool spilledSuccessfully = TrySpillAndStoreEvaluationStack(pSpill, &pSpilledType, &itemsSpilled);
    if (!spilledSuccessfully)
    {
        // The stack couldn't be spilled. An error has been reported, but the
        // stack still needs to be zeroed out; otherwise we'll hit an assert
        // later.
        stackDepthForErrorCase = m_stackTypes.Count();

        m_usCurStackLvl = 0;
        m_stackTypes.Clear();
    }


    GenerateRvalue(pSpill->StateAssignment);

    // The only safe point for the debugger to obtain the async method identity
    // is right before the OnCompleted call.  Thus we record this code address
    // as the yield point of the await operation.
    //
    // The following comment only applies if we use the cached delegate as the
    // identity object of the async method, which currently we do not.  It is
    // still correct that we must have the yield point before the OnCompleted
    // call executes, but currently we place the yield point before checking
    // if we need to create the delegate because creating the delegate will
    // make a copy of the state machine + builder and we get the identity
    // from the builder.  If we got the identity later than this, we would
    // get it from the wrong (original) copy of the builder and in the void
    // method case it will lazily create the identity object on the wrong
    // builder.  We need to create it before copying the state machine and
    // builder.
    //
    // Since the debugger can break at any point in the IL, it is important to
    // mark the OnCompleted call as the point where prior to it a step over
    // await is required instead of a normal step to the next sequence point.
    // Note that the OnCompleted call can cause the continuation delegate to
    // execute immediately either synchronously or on another thread, and
    // therefore we cannot perform a step over await once this call has been
    // made.  Put another way, the resume point breakpoint is only guaranteed
    // to be hit correctly if it is set _before_ OnCompleted is called.
    //
    // Additionally, the only safe point for the debugger to obtain the async
    // method identity is also right before the OnCompleted call.
    RecordAwaitYieldCodeAddress();

    if (pSpill->CreateAwaiterArray != NULL)
    {
        GenerateRvalue(pSpill->CreateAwaiterArray);
    }

    GenerateRvalue(pSpill->AwaitXAssignment);

    GenerateRvalue(pSpill->OnCompletedCall);

    if (pSpill->OnCompletedExtraCondition != NULL)
    {
        GenerateCondition(pSpill->OnCompletedExtraCondition, true, pExtraIfTrueBlock, pExtraIfFalseBlock);
        // pExtraIfTrue is now the current block
        GenerateRvalue(pSpill->OnCompletedExtraIfTrue);
        EndCodeBuffer(CEE_BR_S, pIfBodyBlock2);
        SetAsCurrentCodeBlock(pExtraIfFalseBlock);
        GenerateRvalue(pSpill->OnCompletedExtraIfFalse1);
        GenerateRvalue(pSpill->OnCompletedExtraIfFalse2);
    }
    SetAsCurrentCodeBlock(pIfBodyBlock2);
    
    GenerateRvalue(pSpill->DoFinallyBodiesAssignment);

    // Generate return.
    // We can't just return from within a try block. We need to do a leave so
    // that the finally block will run.
    bool UseLeave = m_tryListPending.NumberOfEntries() > 0;
    if (UseLeave)
    {
        LeaveCodeBuffer(m_pcblkEndProc,
            pSpill->uFlags & SLF_EXITS_TRY,
            false /* ExitsCatch */,
            false /* ExitsCatchToCorrespondingTry */,
            false /* EmitNOPBeforeHiddenIL */,
            false /* BeginHiddenIL */);
    }
    else
    {
        EndCodeBuffer(CEE_BR_S, m_pcblkEndProc);
    }

    // Leaving/Ending a code buffer ends the current code block.
    // Continue with the Resume block.
    SetAsCurrentCodeBlock(pSpill->ResumeLabel->TargetCodeBlock);

    RecordAwaitResumeCodeAddress();

    // Emit code for expression c.
    GenerateRvalue(pSpill->StateResetAssignment);

    GenerateRvalue(pSpill->RestoreAwaiterStatements);

    if (spilledSuccessfully)
    {
        // Emit code to restore the stack.
        RestoreEvaluationStack(pSpill, pSpilledType, itemsSpilled);
    }
    else
    {
        // If the type list is null, we've reported an error and dumped the
        // type stack. In order to avoid unrelated asserts, push the proper
        // number of dummy values on the stack.
        for (short i = 0; i < stackDepthForErrorCase; i++)
        {
            m_stackTypes.PushOrEnqueue(NULL);
        }

        m_usCurStackLvl = stackDepthForErrorCase;
    }

    // Execute the rest of the code starting with a new block
    SetAsCurrentCodeBlock(pAfterIfBlock);

}
