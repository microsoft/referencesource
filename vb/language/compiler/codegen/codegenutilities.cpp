//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Generate COM+ byte codes helper functions. 
//  See codegen.cpp for file organization info.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// Opcode tables
//

COMPILE_ASSERT(
    t_UNDEF + 1 == t_void &&
    t_UNDEF + 2 == t_bad &&
    t_UNDEF + 3 == t_bool &&
    t_UNDEF + 4 == t_i1 &&
    t_UNDEF + 5 == t_ui1 &&
    t_UNDEF + 6 == t_i2 &&
    t_UNDEF + 7 == t_ui2 &&
    t_UNDEF + 8 == t_i4 &&
    t_UNDEF + 9 == t_ui4 &&
    t_UNDEF + 10 == t_i8 &&
    t_UNDEF + 11 == t_ui8 &&
    t_UNDEF + 12 == t_decimal &&
    t_UNDEF + 13 == t_single &&
    t_UNDEF + 14 == t_double &&
    t_UNDEF + 15 == t_date &&
    t_UNDEF + 16 == t_char &&
    t_UNDEF + 17 == t_string &&
    t_UNDEF + 18 == t_ref &&
    t_UNDEF + 19 == t_struct &&
    t_UNDEF + 20 == t_array &&
    t_UNDEF + 21 == t_ptr &&
    t_UNDEF + 22 == t_generic);

COMPILE_ASSERT(
    SX_EQ + 1 == SX_NE &&
    SX_EQ + 2 == SX_LE &&
    SX_EQ + 3 == SX_GE &&
    SX_EQ + 4 == SX_LT &&
    SX_EQ + 5 == SX_GT);

COMPILE_ASSERT(
    SX_NEG + 1 == SX_ADD &&
    SX_NEG + 2 == SX_SUB &&
    SX_NEG + 3 == SX_MUL &&
    SX_NEG + 4 == SX_DIV &&
    SX_NEG + 5 == SX_MOD);

// maps (type) to corresponding byref load opcode
const unsigned short CodeGenerator::s_OpcodeByrefLoad[] =
{
/* t_bool     */  CEE_LDIND_I1,
/* t_i1       */  CEE_LDIND_I1,
/* t_ui1      */  CEE_LDIND_U1,
/* t_i2       */  CEE_LDIND_I2,
/* t_ui2      */  CEE_LDIND_U2,
/* t_i4       */  CEE_LDIND_I4,
/* t_ui4      */  CEE_LDIND_U4,
/* t_i8       */  CEE_LDIND_I8,
/* t_ui8      */  CEE_LDIND_I8,
/* t_decimal  */  CEE_LDOBJ,
/* t_single   */  CEE_LDIND_R4,
/* t_double   */  CEE_LDIND_R8,
/* t_date     */  CEE_LDOBJ,
/* t_char     */  CEE_LDIND_U2,
/* t_string   */  CEE_NOP,
/* t_ref      */  CEE_LDIND_REF,
/* t_struct   */  CEE_LDOBJ,
/* t_array    */  0,
/* t_ptr      */  0,
/* t_generic  */  CEE_LDOBJ,
};

// maps (type) to corresponding store byref opcode
const unsigned short CodeGenerator::s_OpcodeByrefStore[] =
{
/* t_bool     */  CEE_STIND_I1,
/* t_i1       */  CEE_STIND_I1,
/* t_ui1      */  CEE_STIND_I1,
/* t_i2       */  CEE_STIND_I2,
/* t_ui2      */  CEE_STIND_I2,
/* t_i4       */  CEE_STIND_I4,
/* t_ui4      */  CEE_STIND_I4,
/* t_i8       */  CEE_STIND_I8,
/* t_ui8      */  CEE_STIND_I8,
/* t_decimal  */  CEE_STOBJ,
/* t_single   */  CEE_STIND_R4,
/* t_double   */  CEE_STIND_R8,
/* t_date     */  CEE_STOBJ,
/* t_char     */  CEE_STIND_I2,
/* t_string   */  CEE_NOP,
/* t_ref      */  CEE_STIND_REF,
/* t_struct   */  CEE_STOBJ,
/* t_array    */  0,
/* t_ptr      */  0,
/* t_generic  */  CEE_STOBJ,
};

// maps (type) to corresponding array load opcode
const unsigned short CodeGenerator::s_OpcodeArrayLoad[] =
{
/* t_bool     */  CEE_LDELEM_I1,
/* t_i1       */  CEE_LDELEM_I1,
/* t_ui1      */  CEE_LDELEM_U1,
/* t_i2       */  CEE_LDELEM_I2,
/* t_ui2      */  CEE_LDELEM_U2,
/* t_i4       */  CEE_LDELEM_I4,
/* t_ui4      */  CEE_LDELEM_U4,
/* t_i8       */  CEE_LDELEM_I8,
/* t_ui8      */  CEE_LDELEM_I8,
/* t_decimal  */  CEE_LDOBJ,
/* t_single   */  CEE_LDELEM_R4,
/* t_double   */  CEE_LDELEM_R8,
/* t_date     */  CEE_LDOBJ,
/* t_char     */  CEE_LDELEM_U2,
/* t_string   */  CEE_NOP,
/* t_ref      */  CEE_LDELEM_REF,
/* t_struct   */  CEE_LDOBJ,
/* t_array    */  0,
/* t_ptr      */  0,
/* t_generic  */  CEE_LDELEM,
};

// maps (type) to corresponding store array opcode
const unsigned short CodeGenerator::s_OpcodeArrayStore[] =
{
/* t_bool     */  CEE_STELEM_I1,
/* t_i1       */  CEE_STELEM_I1,
/* t_ui1      */  CEE_STELEM_I1,
/* t_i2       */  CEE_STELEM_I2,
/* t_ui2      */  CEE_STELEM_I2,
/* t_i4       */  CEE_STELEM_I4,
/* t_ui4      */  CEE_STELEM_I4,
/* t_i8       */  CEE_STELEM_I8,
/* t_ui8      */  CEE_STELEM_I8,
/* t_decimal  */  CEE_STOBJ,
/* t_single   */  CEE_STELEM_R4,
/* t_double   */  CEE_STELEM_R8,
/* t_date     */  CEE_STOBJ,
/* t_char     */  CEE_STELEM_I2,
/* t_string   */  CEE_NOP,
/* t_ref      */  CEE_STELEM_REF,
/* t_struct   */  CEE_STOBJ,
/* t_array    */  0,
/* t_ptr      */  0,
/* t_generic  */  CEE_STELEM,
};

// maps (oper) to its comparison code
const unsigned short CodeGenerator::s_SxopComp[] =
{
/* SX_EQ */  CEE_CEQ,
/* SX_NE */  CEE_CEQ,
/* SX_LE */  CEE_CGT,
/* SX_GE */  CEE_CLT,
/* SX_LT */  CEE_CLT,
/* SX_GT */  CEE_CGT,
};

const unsigned short CodeGenerator::s_SxopCompUnsigned[] =
{
/* SX_EQ */  CEE_CEQ,
/* SX_NE */  CEE_CEQ,
/* SX_LE */  CEE_CGT_UN,
/* SX_GE */  CEE_CLT_UN,
/* SX_LT */  CEE_CLT_UN,
/* SX_GT */  CEE_CGT_UN,
};

// maps (oper) to its comparison code
const unsigned short CodeGenerator::s_SxopCompFloat[] =
{
/* SX_EQ */  CEE_CEQ,
/* SX_NE */  CEE_CEQ,
/* SX_LE */  CEE_CGT_UN,
/* SX_GE */  CEE_CLT_UN,
/* SX_LT */  CEE_CLT,
/* SX_GT */  CEE_CGT,
};

// maps (oper) to its comparison branch code
const unsigned short CodeGenerator::s_SxopCompBranch[] =
{
/* SX_EQ */  CEE_BEQ_S,
/* SX_NE */  CEE_BNE_UN_S,
/* SX_LE */  CEE_BLE_S,
/* SX_GE */  CEE_BGE_S,
/* SX_LT */  CEE_BLT_S,
/* SX_GT */  CEE_BGT_S,
};

const unsigned short CodeGenerator::s_SxopCompBranchUnsigned[] =
{
/* SX_EQ */  CEE_BEQ_S,
/* SX_NE */  CEE_BNE_UN_S,
/* SX_LE */  CEE_BLE_UN_S,
/* SX_GE */  CEE_BGE_UN_S,
/* SX_LT */  CEE_BLT_UN_S,
/* SX_GT */  CEE_BGT_UN_S,
};

// maps (oper) to its reversed comparison branch code
const unsigned short CodeGenerator::s_SxopCompReverseBranch[] =
{
/* SX_EQ */  CEE_BNE_UN_S,
/* SX_NE */  CEE_BEQ_S,
/* SX_LE */  CEE_BGT_S,
/* SX_GE */  CEE_BLT_S,
/* SX_LT */  CEE_BGE_S,
/* SX_GT */  CEE_BLE_S,
};

const unsigned short CodeGenerator::s_SxopCompReverseBranchUnsigned[] =
{
/* SX_EQ */  CEE_BNE_UN_S,
/* SX_NE */  CEE_BEQ_S,
/* SX_LE */  CEE_BGT_UN_S,
/* SX_GE */  CEE_BLT_UN_S,
/* SX_LT */  CEE_BGE_UN_S,
/* SX_GT */  CEE_BLE_UN_S,
};

// 

const unsigned short CodeGenerator::s_SxopCompReverseBranchFloat[] =
{
/* SX_EQ */  CEE_BNE_UN_S,
/* SX_NE */  CEE_BEQ_S,
/* SX_LE */  CEE_BGT_UN_S,
/* SX_GE */  CEE_BLT_UN_S,
/* SX_LT */  CEE_BGE_UN_S,
/* SX_GT */  CEE_BLE_UN_S,
};


// Stack effect for each opcode ...

#define Pop0    0
#define Pop1    1
#define PopI4   1
#define PopR4   1
#define PopI8   1
#define PopR8   1
#define PopI    1
#define PopRef  1
#define VarPop  0       // opcodes taking VarPop need to be adjusted manually

#define Push0   0
#define Push1   1
#define PushI4  1
#define PushR4  1
#define PushI8  1
#define PushR8  1
#define PushI   1
#define PushRef 1
#define VarPush 0       // opcodes taking VarPush need to be adjusted manually

const signed CodeGenerator::s_OpcodeStkEffPush[] =
{
    #define OPDEF(c,nam,pop,push,args,type,type2,s1,s2,flow)     (push),
    #include "opcode.def"
    #undef OPDEF
};

const signed CodeGenerator::s_OpcodeStkEffPop[] =
{
    #define OPDEF(c,nam,pop,push,args,type,type2,s1,s2,flow)     (pop),
    #include "opcode.def"
    #undef OPDEF
};

#undef Pop0
#undef PopI4
#undef PopI8
#undef PopR4
#undef PopR8
#undef PopI
#undef PopR
#undef VarPop
#undef RetPop

#undef Push0
#undef PushI4
#undef PushI8
#undef PushR4
#undef PushR8
#undef PushI
#undef PushR
#undef VarPush
#undef RetPush

const unsigned char CodeGenerator::s_OpcodeLen[] =
{
    #define OPDEF(c,nam,pop,push,oper,opcod,len,b1,b2,flow) (len),
    #include "opcode.def"
    #undef OPDEF
};

// Don't need b1, since we can infer from len above
const unsigned char CodeGenerator::s_OpcodeCode[] =
{
    #define OPDEF(c,nam,pop,push,oper,opcod,len,b1,b2,flow) (b2),
    #include "opcode.def"
    #undef OPDEF
};

/*========================================================================

Routines used to emit into code buffers

========================================================================*/

//========================================================================
// Adjust the stack given a particular opcode
//========================================================================

void CodeGenerator::AddOpcodeToCurStack
(
OPCODE opcode
)
{
    VSASSERT(opcode <= CEE_COUNT, "invalid opcode");

    // These arrays are sized such that OPCODE enum values are valid indices
#pragma prefast(push)
#pragma prefast(disable: 26010 26011, "These arrays are sized such that OPCODE enum values are valid indices")
    AddToCurStack(-s_OpcodeStkEffPop[opcode]);
    AddToCurStack(s_OpcodeStkEffPush[opcode]);
#pragma prefast(pop)

    // We handle popping items off of the stack of types here. Pushing is
    // handled as appropriate at those locations where we call EmitOpcode.
    for (int i = 0; i < s_OpcodeStkEffPop[opcode]; i++)
    {
        m_stackTypes.PopOrDequeue();
    }
}

//========================================================================
// Adjust the stack manually
//========================================================================

void CodeGenerator::AddToCurStack
(
signed c
)
{
    m_usCurStackLvl += (unsigned short)(c);
    if (m_usMaxStackLvl < m_usCurStackLvl)
    {
        m_usMaxStackLvl = m_usCurStackLvl;
    }

    // since m_usCurStackLvl is unsigned, typecast it to signed to do the assert
    VSASSERT((signed short)m_usCurStackLvl >= 0,
              "AddToCurStack: stack level less than 0" );
}

//========================================================================
// Set the passed in code block as the "current" code block, i.e. the
// one that will get any emitted code
//========================================================================

void CodeGenerator::SetAsCurrentCodeBlock
(
CODE_BLOCK * pcblk
)
{
    // Sometimes we call SetAsCurrentCodeBlock() with the same pcblk as
    // the m_pcblkCurrent. We want to do nothing in this case.
    //
    if (!pcblk || pcblk != m_pcblkCurrent)
    {
        // If the m_pcblkCurrent is non empty, that means somebody has
        // set it to something and want it to be in the chain
        //
        if (m_pcblkCurrent)
        {
            EndCodeBuffer(CEE_NOP);
        }

        // Only want to create a code buffer if none exists
        if (CodeBuf_IsNull())
        {
            CodeBuf_Create();
        }

        if (pcblk)
        {
            m_pcblkCurrent = pcblk;
        }
        else
        {
            m_pcblkCurrent = NewCodeBlock();
        }
    }
}

//========================================================================
// Finish emitting into a code buffer with a jump code. Sets the current
// code block to nothing
//========================================================================

void CodeGenerator::TerminateCodeBuffer
(
    OPCODE          opcodeJump,
    void          * pvData,
    Vtypes          JumpType
)
{
    // Short jumps may get translated into long jumps
    VSASSERT(IsShortBranch(opcodeJump)    ||
             opcodeJump == CEE_NOP        ||
             opcodeJump == CEE_RET        ||
             opcodeJump == CEE_THROW      ||
             opcodeJump == CEE_RETHROW    ||
             opcodeJump == CEE_ENDFILTER  ||
             opcodeJump == CEE_ENDFINALLY ||
             opcodeJump == CEE_SWITCH,
             "must be a jump opcode or switch.");

    VSASSERT(m_pcblkCurrent != NULL,
             "EndCodeBuffer: must have current block");

    if (CodeBuf_GetSize())
    {
        // Code buffer is non-empty. Move to current code block.
        m_pcblkCurrent->pbCodeBuffer    = CodeBuf_GetBuffer();
        m_pcblkCurrent->usCodeSize      = (unsigned short) CodeBuf_GetSize();

        CodeBuf_Reset();
    }

    // Link into code list
    m_cblkCodeList.InsertLast(m_pcblkCurrent);

    AddOpcodeToCurStack(opcodeJump);

    m_pcblkCurrent->opcodeJump  = opcodeJump;
    m_pcblkCurrent->pvData      = pvData;       // can be either jump dest or switch table
    m_pcblkCurrent->JumpType    = JumpType;

    m_pcblkCurrent = NULL;
}

//========================================================================
// This function exists to enforce usage of LeaveCodeBuffer() for emitting LEAVE opcodes
//========================================================================

void CodeGenerator::EndCodeBuffer
(
    OPCODE          opcodeJump,
    void          * pvData          // default value of NULL
)
{
    VSASSERT(!IsConditionalBranch(opcodeJump) || opcodeJump == CEE_BRFALSE_S || opcodeJump == CEE_BRTRUE_S, "conditional branches must specify their condition's type;  wrong overload used");
    EndCodeBuffer(
        opcodeJump,
        pvData,
        (opcodeJump == CEE_BRFALSE_S || opcodeJump == CEE_BRTRUE_S) ?
            t_bool :
            t_UNDEF);
}

void CodeGenerator::EndCodeBuffer
(
    OPCODE          opcodeJump,
    void          * pvData,
    Vtypes          JumpType
)
{
    // Use LeaveCodeBuffer to end a code buffer with a LEAVE opcode.
    VSASSERT(opcodeJump != CEE_LEAVE_S && opcodeJump != CEE_LEAVE,
             "can't call EndCodeBuffer with a Leave!");

    VSASSERT(IsConditionalBranch(opcodeJump) ^ (JumpType == t_UNDEF), "condition type must be specified when emitting a conditional branch");

    TerminateCodeBuffer(opcodeJump, pvData, JumpType);
}

//========================================================================
// Emit a LEAVE opcode into the code buffer
//========================================================================

void CodeGenerator::LeaveCodeBuffer
(
    CODE_BLOCK *JumpDest,
    bool ExitsTry,
    bool ExitsCatch,
    bool ExitsCatchToCorrespondingTry,
    bool EmitNOPBeforeHiddenIL,
    bool BeginHiddenIL,
    Location * errorLocation
)
{
    VSASSERT(ExitsTry || ExitsCatch || m_tryListPending.GetFirst() == m_ptryOnError,
             "LEAVE not necessary when not branching out of an exception handler");

    TRY_BLOCK *ptry = m_tryListPending.GetFirst();

    // ptry can be null here when a 'for each' statement do not enclose it's block in a try.
    // see bug DevDiv109887. Consider optimize this leave into a branch opcode.
    // AssertIfNull(ptry);

    if (ExitsCatch)
    {
        // Clear the Err object if this Leave branches out of a Catch context
        // (but only if we care about setting the Err object, which we don't
        // when building the runtime).

        if (!g_CompilingTheVBRuntime)
        {
            GenerateCallToRuntimeHelper(ClearErrorMember, Symbols::GetVoidType(), errorLocation, errorLocation ? Error : Ignore); // Bug #178026 - DevDiv Bugs: log an error if we have location
        }
    }

    // Dev10#707597: We need to hide the leave instruction when exiting
    // catch for correct stepping of release binaries.
    if (BeginHiddenIL && (ExitsTry || ExitsCatch && !ExitsCatchToCorrespondingTry))
    {
        if (EmitNOPBeforeHiddenIL)
        {
            InsertNOP();
        }

        if (ptry && ptry->FinallyLocation.IsValid())
        {
            // The "nop" when exiting each "try" and "Catch" is mapped to the "Finally" statement.
            // This enables a better ENC experience without disabling any normal debugging scenarios.
            // Bug VSWhidbey 500839.

            UpdateLineTable(ptry->FinallyLocation);
            InsertNOP();
        }

        // ENC remapping requires all CEE_LEAVE opcodes branching out of a Try
        // block to be marked hidden.  It also requires an empty stack.  (VSW#159111)
        VSASSERT(!ExitsTry || m_usCurStackLvl == 0, "expected zero stack depth during LEAVE");
        StartHiddenIL();
    }

    TerminateCodeBuffer(CEE_LEAVE_S, JumpDest, t_UNDEF);
}

//========================================================================
// Is the opcode a long branch?
//========================================================================

bool CodeGenerator::IsLongBranch
(
OPCODE opcode
)
{
    switch (opcode)
    {
    case CEE_BEQ:
    case CEE_BGE:
    case CEE_BGT:
    case CEE_BLE:
    case CEE_BLT:
    case CEE_BNE_UN:
    case CEE_BGT_UN:
    case CEE_BLT_UN:
    case CEE_BGE_UN:
    case CEE_BLE_UN:
    case CEE_BRFALSE:
    case CEE_BRTRUE:
    case CEE_BR:
    case CEE_LEAVE:
        return true;

    default:
        return false;
    }
}

//========================================================================
// Is the opcode a short branch?
//========================================================================

bool CodeGenerator::IsShortBranch
(
OPCODE opcode
)
{
    switch (opcode)
    {
    case CEE_BEQ_S:
    case CEE_BGE_S:
    case CEE_BGT_S:
    case CEE_BLE_S:
    case CEE_BLT_S:
    case CEE_BNE_UN_S:
    case CEE_BGT_UN_S:
    case CEE_BLT_UN_S:
    case CEE_BGE_UN_S:
    case CEE_BLE_UN_S:
    case CEE_BRFALSE_S:
    case CEE_BRTRUE_S:
    case CEE_BR_S:
    case CEE_LEAVE_S:
        return true;

    default:
        return false;
    }
}

bool CodeGenerator::IsConditionalBranch
(
OPCODE opcode
)
{
    switch (opcode)
    {
    case CEE_BEQ_S:
    case CEE_BGE_S:
    case CEE_BGT_S:
    case CEE_BLE_S:
    case CEE_BLT_S:
    case CEE_BNE_UN_S:
    case CEE_BGT_UN_S:
    case CEE_BLT_UN_S:
    case CEE_BGE_UN_S:
    case CEE_BLE_UN_S:
    case CEE_BRFALSE_S:
    case CEE_BRTRUE_S:
    case CEE_BEQ:
    case CEE_BGE:
    case CEE_BGT:
    case CEE_BLE:
    case CEE_BLT:
    case CEE_BNE_UN:
    case CEE_BGT_UN:
    case CEE_BLT_UN:
    case CEE_BGE_UN:
    case CEE_BLE_UN:
    case CEE_BRFALSE:
    case CEE_BRTRUE:
        return true;

    default:
        return false;
    }
}


OPCODE CodeGenerator::MapShort2LongJumpOpcode
(
OPCODE opcodeShortJump
)
{
    switch (opcodeShortJump)
    {
    case CEE_BEQ_S:     return CEE_BEQ;
    case CEE_BGE_S:     return CEE_BGE;
    case CEE_BGT_S:     return CEE_BGT;
    case CEE_BLE_S:     return CEE_BLE;
    case CEE_BLT_S:     return CEE_BLT;
    case CEE_BNE_UN_S:  return CEE_BNE_UN;
    case CEE_BGT_UN_S:  return CEE_BGT_UN;
    case CEE_BLT_UN_S:  return CEE_BLT_UN;
    case CEE_BGE_UN_S:  return CEE_BGE_UN;
    case CEE_BLE_UN_S:  return CEE_BLE_UN;
    case CEE_BRFALSE_S: return CEE_BRFALSE;
    case CEE_BRTRUE_S:  return CEE_BRTRUE;
    case CEE_BR_S:      return CEE_BR;
    case CEE_LEAVE_S:   return CEE_LEAVE;

    default:
        VSFAIL("unknown opcode!");
        VbThrow(HrMake(ERRID_CodegenError));
        return CEE_NOP;
    }
}

//========================================================================
// Begins a block of hidden IL in the line table
//========================================================================

void CodeGenerator::StartHiddenIL()
{
    if (m_Project->GeneratePDB())
    {
        Location Loc;
        Loc.SetLocationToHidden();
        UpdateLineTable(Loc);
    }
}

//========================================================================
// Begins the Epilogue IL location in the line table
//========================================================================

void CodeGenerator::StartEpilogueIL()
{
    // Don't need to emit debug info for synthetic methods.
    // 

    VSASSERT(m_ptreeFunc->bilop == SB_PROC, "Expected Proc node");

    if (m_pproc->IsAsyncKeywordUsed() || m_pproc->IsIteratorKeywordUsed())
    {
        // Kick off methods should not map any source code.  Instead the MoveNext methods map the source code.
        return;
    }

    Location LineTableEntry = GetDebugEndLocation(&m_ptreeFunc->AsProcedureBlock());

    if (LineTableEntry.IsValid())
    {
        UpdateLineTable(LineTableEntry);
    }
}

//========================================================================
// Adds a new entry to the line table for a statement
//      using statement location
//========================================================================

void CodeGenerator::UpdateLineTable
(
ILTree::PILNode ptreeStmt
)
{
    VSASSERT(ptreeStmt->Sxkind() & SXK_STMT, "tree is not a statment.  why are we calling updatelinetable?");

    SourceFile *pSourceFile = NULL;
    if (ptreeStmt->bilop == SL_STMT)
    {
        pSourceFile = ptreeStmt->AsStatementWithExpression().GetContainingFile();
    }

    UpdateLineTable(ptreeStmt->Loc, pSourceFile);
}

//========================================================================
// Adds a new entry to the line table for a given line number.
//========================================================================

void CodeGenerator::UpdateLineTable
(
    Location &Loc,
    SourceFile *pSourceFile     // only required to be passed in if different from the default, else NULL
)
{
    if (!m_Project->GeneratePDB())
    {
        return;
    }

    VSASSERT(Loc.IsValid(), "UpdateLineTable: We shouldn't have an invalid location in codegen");
    VSASSERT(m_pltblLineTbl, "UpdateLineTable: no line table or inconsistent state.");
    VSASSERT(Loc.m_lBegLine <= Loc.m_lEndLine &&
             (Loc.m_lEndLine > Loc.m_lBegLine || Loc.m_lBegColumn <= Loc.m_lEndColumn),
             "UpdateLineTable: Invalid line info");

    if (Loc.m_lBegLine > HIDDEN_LINE_INDICATOR)
    {
        Loc.SetLocationToHidden();
    }

    // Remove hidden sequence points between sequence points that are equal.
    // Bug VSWhidbey 293903.
    //
    for(LINETBL *pltblCurrent = m_pltblCurrent - 1;
        pltblCurrent >= m_pltblLineTbl;
        pltblCurrent = pltblCurrent - 1)
    {
        if (pltblCurrent->lSrcLine == Loc.m_lBegLine &&
            pltblCurrent->lSrcColumn == Loc.m_lBegColumn)
        {
            m_pltblCurrent = pltblCurrent + 1;
            return;
        }

        if (pltblCurrent->lSrcLine < HIDDEN_LINE_INDICATOR)
        {
            break;
        }
    }


    // Need to check for space only when adding a new entry.
    if (m_pltblCurrent >= m_pltblEnd)
    {
        // Microsoft 9/14/2004:  Added some checks to validate values (and assuming later that 10*sizeof(LINETBL) is OK.)
        if (!VBMath::TryMultiply((m_pltblEnd - m_pltblLineTbl), sizeof(LINETBL)))
        {
            VbThrow(HrMake(ERRID_CodegenError));
        }
        unsigned long size = (unsigned long)(m_pltblEnd - m_pltblLineTbl)*sizeof(LINETBL);
        if (size + 10*static_cast<unsigned long>(sizeof(LINETBL)) < size)
        {
            VbThrow(HrMake(ERRID_CodegenError));
        }
        // Add 10 more entries
        m_pltblLineTbl = (LINETBL*) m_nraLineTable.Resize(m_pltblLineTbl, size, size + 10 * sizeof(LINETBL));
        m_pltblCurrent = (LINETBL*) ((char*)m_pltblLineTbl + size);
        m_pltblEnd     = (LINETBL*) ((char*)m_pltblLineTbl + size + 10 * sizeof(LINETBL));
    }

    m_pltblCurrent->lSrcLine          = Loc.m_lBegLine;
    m_pltblCurrent->lSrcColumn        = Loc.m_lBegColumn;
    m_pltblCurrent->lEndSrcLine       = Loc.m_lEndLine;
    m_pltblCurrent->lEndSrcColumn     = Loc.m_lEndColumn;
    m_pltblCurrent->codeaddr.pcblk    = m_pcblkCurrent;
    m_pltblCurrent->codeaddr.uOffset  = CodeBuf_GetSize();
    m_pltblCurrent->pSourceFile = pSourceFile;

    m_pltblCurrent++;
}

//========================================================================
// Records the current code address in a list of code addresses that
// correspond to the yield point of an await operator.  Emitted to the
// PDB later and used by the debugger for the special async stepping
// behaviour.
//========================================================================

void
CodeGenerator::RecordAwaitYieldCodeAddress
(
)
{
    if (m_Project->GeneratePDB())
    {
        CODE_ADDRESS codeaddr;
        codeaddr.pcblk    = m_pcblkCurrent;
        codeaddr.uOffset  = CodeBuf_GetSize();

        m_AwaitYieldOffsets.AddElement(codeaddr);
    }
}

//========================================================================
// Records the current code address in a list of code addresses that
// correspond to the resume point of an await operator.  Emitted to the
// PDB later and used by the debugger for the special async stepping
// behaviour.
//
// "ResumeCodeAddress" here is a bit of a misnomer since it doesn't
// actually correspond to the resume label, but instead the point after
// the stack is restored and the state machine's state is set back to 0.
// But from the debugger's point of view this is the IL offset where it
// will set a breakpoint for Step Over Await behaviour.
//========================================================================

void
CodeGenerator::RecordAwaitResumeCodeAddress
(
)
{
    if (m_Project->GeneratePDB())
    {
        CODE_ADDRESS codeaddr;
        codeaddr.pcblk    = m_pcblkCurrent;
        codeaddr.uOffset  = CodeBuf_GetSize();

        m_AwaitResumeOffsets.AddElement(codeaddr);
    }
}

//========================================================================
// Updates the resume table for a block -- if pcblk is null, then set the
// resume to go to the current statement. Otherwise, make it point at the
// beginning of the give code block
//========================================================================

unsigned short
CodeGenerator::AllocateResumeEntry
(
    CODE_BLOCK *pcblk,
    unsigned short ResumeIndex
)
{
    if (InsideValidResumeContext())
    {
        if (ResumeIndex == NoResume)
        {
            // No Resume is necessary for this statement, so skip it.
            return ResumeIndex;
        }

        // 



        // Normally, the Parser will assign sequential resume indicies.  However, it is
        // possible that some may not be sequential order (although they must be in an
        // always increasing order).  This introduces the possibility of gaps in the Resume
        // table.  Gaps are problematic because Resume Next relies on the fact that ResumeTarget + 1
        // is the index into the table for the next statement.  To fix this, we fill the gaps.
        //
        // Given a previous index I and a new entry for branch target T at index J where J > I+1,
        // we make entires I+1 and J branch to T.  For each index K where I+1 < K < J, we make the
        // entry branch to a code block which throws an internal error exception.  In theory, the
        // ResumeTarget state variable should never hold the value K.  But who knows?  ENC may have
        // a bug.  It's better to throw than to execute code incorrectly.

        int GapCount = ResumeIndex - (m_cadrResumeList.NumberOfEntries() - 1) - 1;

        VSASSERT(m_usCurStmtLocal != INVALID_LCL_INDEX, "local var used for managing resume state is not valid");

        if (GapCount < 0)
        {
            VSASSERT(ResumeIndex != 0 || pcblk == m_pcblkResumeTableFallThrough,
                     "ResumeIndex is zero! #11/17/2003#");
            VSFAIL("adding resume entry at an already existing index. : #11/14/2003#");
            VbThrow(HrMake(ERRID_CodegenError));
            return 0;
        }

        RESUME_ADDRESS *presaddr = (RESUME_ADDRESS *)m_pnra->Alloc(sizeof(RESUME_ADDRESS));

        presaddr->codeaddr.pcblk = pcblk ? pcblk : m_pcblkCurrent;
        presaddr->codeaddr.uOffset = pcblk ? 0 : CodeBuf_GetSize();
        m_cadrResumeList.InsertLast(presaddr);

        for (int i = 0; i < GapCount - 1; i++)
        {
            presaddr = (RESUME_ADDRESS *)m_pnra->Alloc(sizeof(RESUME_ADDRESS));

            presaddr->codeaddr.pcblk = m_pcblkResumeTableFallThrough;
            presaddr->codeaddr.uOffset = 0;
            m_cadrResumeList.InsertLast(presaddr);
        }

        if (GapCount > 0)
        {
            presaddr = (RESUME_ADDRESS *)m_pnra->Alloc(sizeof(RESUME_ADDRESS));

            presaddr->codeaddr.pcblk = pcblk ? pcblk : m_pcblkCurrent;
            presaddr->codeaddr.uOffset = pcblk ? 0 : CodeBuf_GetSize();
            m_cadrResumeList.InsertLast(presaddr);
        }

        VSASSERT(m_cadrResumeList.NumberOfEntries() - 1 == ResumeIndex,
                 "expected consistent index : #11/14/2003#");
    }

    return (unsigned short)(m_cadrResumeList.NumberOfEntries() - 1);
}

void
CodeGenerator::UpdateCurrentStatementLocal
(
    unsigned short ResumeIndex
)
{
    // If resume present, assign current executable line to
    // Resume local. Can't just do a simple iinc because of goto's.
    //
    if (InsideValidResumeContext())
    {
        if (ResumeIndex == NoResume)
        {
            // No Resume is necessary for this statement, so skip it.
            return;
        }

        VSASSERT(m_usCurStmtLocal != INVALID_LCL_INDEX, "local var used for managing resume state is not valid");

        GenerateLiteralInt(ResumeIndex);
        GenerateStoreLocal(m_usCurStmtLocal);
    }
}

void
CodeGenerator::UpdateResumeTable
(
    unsigned short ResumeIndex,
    bool fCanThrow,         // defaults to true
    CODE_BLOCK *pcblk       // defaults to NULL
)
{
    ResumeIndex = AllocateResumeEntry(pcblk, ResumeIndex);

    // If there is no exception possible, then don't emit an update
    if (fCanThrow)
    {
        UpdateCurrentStatementLocal(ResumeIndex);
    }
}

void CodeGenerator::AddVariableToMethod
(
    BlockScope * CurrentScope,
    BCSYM_Variable * Variable,
    unsigned long Flags
)
{
    Signature signature;
    BYTE * SignatureBuffer;
    LocalVariable * CurrentVariable;

    
    if (Variable->GetRewrittenName())
    {
        CurrentVariable = m_MethodSlots->AddResumableLocalVariable(Variable);
    }
    else
    {
        m_pmdemit->StartNewSignature(&signature);

        m_pmdemit->EncodeType(Variable->GetCompilerType(), Variable->GetRawType()->GetLocation());
        SignatureBuffer = (BYTE *)m_pnraDebugSpace->Alloc(m_pmdemit->GetSigSize());
        memcpy(SignatureBuffer, m_pmdemit->GetSignature(), m_pmdemit->GetSigSize());

        CurrentVariable = m_MethodSlots->AddVariable(Variable, Flags, SignatureBuffer, m_pmdemit->GetSigSize());

        VSASSERT(CurrentVariable->SignatureSize > 0 &&
            CurrentVariable->Signature,
            "AddVariableToMethod: this variable has no signature!");

        m_pmdemit->ReleaseSignature(&signature);

        // Once we know the slot number, we can record this information in the symbol
        Variable->SetLocalSlot(CurrentVariable->SlotNumber);
    }

    // A non-null scope implies that we are generating PDB information.
    // Therefore, we need to keep track of block scoping information.
    // If so, then add this variable to the current scope's member list.
    //
    VSASSERT(!(!!CurrentScope ^ m_Project->GeneratePDB()),
        "AddVariableToMethod: The validity of CurrentScope must follow decision to generate pdb");

    if (CurrentScope)
    {
        ScopeMember * NewScopeMember = (ScopeMember *)m_pnraDebugSpace->Alloc(sizeof(ScopeMember));
        NewScopeMember->MemberInfo = CurrentVariable;
        CurrentScope->ScopeMembers.InsertLast(NewScopeMember);
    }
}


void CodeGenerator::AddConstantToMethod
(
    BlockScope * CurrentScope,
    BCSYM_Variable * Variable
)
{
    Signature signature;
    BYTE * SignatureBuffer;
    ConstantVariable * ConstantVariable;

    m_pmdemit->StartNewSignature(&signature);
    m_pmdemit->EncodeInteger(IMAGE_CEE_CS_CALLCONV_FIELD);
    m_pmdemit->EncodeType(Variable->GetCompilerType(), Variable->GetRawType()->GetLocation());
    SignatureBuffer = (BYTE *)m_pnraDebugSpace->Alloc(m_pmdemit->GetSigSize());
    memcpy(SignatureBuffer, m_pmdemit->GetSignature(), m_pmdemit->GetSigSize());

    ConstantVariable = m_MethodSlots->AddConstant(Variable, SignatureBuffer, m_pmdemit->GetSigSize());

    VSASSERT(ConstantVariable->SignatureSize > 0 &&
              ConstantVariable->Signature,
              "AddConstantToMethod: this variable has no signature!");

    // Get a signature token for the constant.
    // We need this because constants are not encoded in the method signature. Therefore,
    // we must compute and emit the token for the constant signature right now in the IL emitted,
    // store it here, and we will refer to this in pdb emit.

    ConstantVariable->SignatureToken = m_pmdemit->ComputeSignatureToken(
        ConstantVariable->Signature,
        ConstantVariable->SignatureSize);
    VSASSERT( ConstantVariable->SignatureToken != mdTokenNil, "Why is the signature token for a constant mdTokenNil?" );

    m_pmdemit->ReleaseSignature(&signature);

    // A non-null scope implies that we are generating PDB information.
    // Therefore, we need to keep track of block scoping information.
    // If so, then add this variable to the current scope's member list.
    //
    VSASSERT(!(!!CurrentScope ^ m_Project->GeneratePDB()),
              "AddVariableToMethod: The validity of CurrentScope must follow decision to generate pdb");

    if (CurrentScope)
    {
        ScopeMember * NewScopeMember = (ScopeMember *)m_pnraDebugSpace->Alloc(sizeof(ScopeMember));
        NewScopeMember->fConstant = true;
        NewScopeMember->ConstantInfo = ConstantVariable;
        CurrentScope->ScopeMembers.InsertLast(NewScopeMember);
    }
}


/*========================================================================

Department of IL optimization. Because of the simplicity of our code
generator, which does not carry state along with it as it compile trees
into code, there are several simple cases that we can not optimize for
easily in the first pass of code generaton. So, we have a second pass
over the IL, optimizing any simple cases that we can find.

========================================================================*/

//========================================================================
// Walk the flow of the code paths and remove any unreachable blocks
//========================================================================

void CodeGenerator::OptimizeGeneratedIL
(
)
{
    CSingleListIter<CODE_BLOCK> Iter(&m_cblkCodeList);
    CODE_BLOCK * pcblk;

    MarkLiveBlocks(m_cblkCodeList.GetFirst());

    VSASSERT(m_tryList.NumberOfEntries() == m_tryholderEncounteredList.NumberOfEntries(),
              "OptimizeGeneratedIL: try lists not equal in size!");

    // When scanning through try blocks to remove the dead ones, we must
    // traverse the try blocks in the order we encountered them when generating statements
    // because MarkLiveBlocks can skip nested Try's if there is more than one.
    // This is different than the order we need to emit them as EHClauses, thus the need
    // for an "encountered" list.
    //
    if (m_tryholderEncounteredList.NumberOfEntries())
    {
        CSingleListIter<TRY_BLOCK_HOLDER> IterTryHolder(&m_tryholderEncounteredList);
        TRY_BLOCK_HOLDER * ptryholder;

        while (ptryholder = IterTryHolder.Next())
        {
            TRY_BLOCK * ptry;
            ptry = ptryholder->ptry;

            // if this try block is live, also mark any catch and finally blocks as live
            if (ptry->pcblkTry->fLive)
            {
                if (ptry->catchList.NumberOfEntries())
                {
                    CSingleListIter<CATCH_BLOCK> IterCatch(&ptry->catchList);
                    CATCH_BLOCK * pcatch;

                    while (pcatch = IterCatch.Next())
                    {
                        if (pcatch->pcblkFilter)
                        {
                            MarkLiveBlocks(pcatch->pcblkFilter);
                        }

                        MarkLiveBlocks(pcatch->pcblkCatch);
                    }
                }

                if (ptry->pcblkFinally)
                {
                    MarkLiveBlocks(ptry->pcblkFinally);

                    // 







                    MarkLiveBlocks(ptry->pcblkFallThrough);
                }
            }
            else
            {
                // Remove from try lists since this try block is dead
                m_tryList.Remove(ptry);
                m_tryholderEncounteredList.Remove(ptryholder);
            }
        }
    }

#if DEBUG
    if (m_pcblkOnErrorHandler)
    {
        VSASSERT(m_pcblkOnErrorFailure->fLive == true,
                 "OptimizeGeneratedIL: On Error failure case must be marked live!");
    }
#endif


    //
    // Fix-up any Catch or Finally block whose last codeblock is marked dead
    //

    CSingleListIter<TRY_BLOCK> IterTry(&m_tryList);
    TRY_BLOCK * ptry;

    while (ptry = IterTry.Next())
    {
        VSASSERT(ptry->pcblkTry->fLive, "Why didn't this try block get removed from the list?");

        // If this try block is live, also mark any catch and finally blocks as live
        if (ptry->catchList.NumberOfEntries())
        {
            CSingleListIter<CATCH_BLOCK> IterCatch(&ptry->catchList);
            CATCH_BLOCK * pcatch;

            while (pcatch = IterCatch.Next())
            {
                if (!pcatch->pcblkLast->fLive)
                {
                    CODE_BLOCK *pcblkCatchCur = pcatch->pcblkCatch;
                    CODE_BLOCK *pcblkLastLive = pcblkCatchCur;

                    // The entire catch handler isn't live, so fix up the last pointer.
                    // 

                    while (pcblkCatchCur != pcatch->pcblkLast)
                    {
                        if (pcblkCatchCur->fLive)
                        {
                            pcblkLastLive = pcblkCatchCur;
                        }

                        pcblkCatchCur = pcblkCatchCur->Next();
                    }

                    pcatch->pcblkLast = pcblkLastLive;

                    // a catch block MUST exit via these opcodes
                    VSASSERT(pcatch->pcblkLast->opcodeJump == CEE_LEAVE_S ||
                             pcatch->pcblkLast->opcodeJump == CEE_THROW ||
                             pcatch->pcblkLast->opcodeJump == CEE_RETHROW,
                             "OptimizeGeneratedIL: catch block has invalid jump opcode!");
                }
            }
        }

        // 

        /*
        if (ptry->pcblkFinally && !ptry->pcblkFallThrough->fLive)
        {
            // If the codeblock immediately following the try block is not live,
            // then we need to find the next live address so we can correctly calculate the length of
            // the Finally clause.
            //
            CODE_BLOCK * pcblkFallThrough = ptry->pcblkFallThrough;

            // iterate through the list until we find a live codeblock
            while (pcblkFallThrough != NULL && !pcblkFallThrough->fLive)
            {
                pcblkFallThrough = pcblkFallThrough->Next();
            }

            VSASSERT(pcblkFallThrough, "we should have found a live codeblock!");

            ptry->pcblkFallThrough = pcblkFallThrough;
        }
        */
    }


    //
    // Now remove dead blocks
    //

    Iter.Reset();
    while (pcblk = Iter.Next())
    {
        if (!pcblk->fLive)
        {
            m_cblkCodeList.Peel(pcblk);
        }
    }

    OptimizeBranches();
}

bool
SameDestination
(
    CODE_BLOCK *Start,
    CODE_BLOCK *BranchTarget
)
{
    CODE_BLOCK *Cursor = Start;

    do
    {
        Cursor = Cursor->Next();

        if (Cursor == BranchTarget)
        {
            return true;
        }
    }
    while (Cursor && Cursor->usCodeSize == 0 && Cursor->opcodeJump == CEE_NOP);

    return false;
}

//========================================================================
// Perform simple branch optimizations
//========================================================================

void CodeGenerator::OptimizeBranches
(
)
{
    CSingleListIter<CODE_BLOCK> Iter(&m_cblkCodeList);
    CODE_BLOCK * pcblk;

    // 
    Iter.Reset();
    while (pcblk = Iter.Next())
    {
        // transform the pattern:
        // br x                   nop
        // x:           INTO      x:
        //
        if (pcblk->opcodeJump == CEE_BR_S && SameDestination(pcblk, pcblk->pcblkJumpDest))
        {
            pcblk->opcodeJump = CEE_NOP;
            continue;
        }

        // transform the pattern:
        // brfalse x              pop
        // x:           INTO      x:
        //
        if ((pcblk->opcodeJump == CEE_BRFALSE_S || pcblk->opcodeJump == CEE_BRTRUE_S) &&
            SameDestination(pcblk, pcblk->pcblkJumpDest))
        {
            pcblk->opcodeJump = CEE_POP;
            pcblk->pcblkJumpDest = NULL;
            continue;
        }

        // transform the pattern:
        //
        // <conditionalbranch> x              !<conditionalbranch> y
        // br y                     INTO      x:
        // x:
        //
        // 

        if (pcblk->Next() &&
            pcblk->Next()->usCodeSize == 0 &&
            pcblk->Next()->opcodeJump == CEE_BR_S &&
            pcblk->pcblkJumpDest == pcblk->Next()->Next() &&
            IsIntegralType(pcblk->JumpType))
        {
            OPCODE NewBranchOpcode = CEE_NOP;

            switch (pcblk->opcodeJump)
            {
                case CEE_BRTRUE_S:  NewBranchOpcode = CEE_BRFALSE_S; break;
                case CEE_BRFALSE_S: NewBranchOpcode = CEE_BRTRUE_S; break;
                case CEE_BEQ_S:     NewBranchOpcode = CEE_BNE_UN_S; break;
                case CEE_BNE_UN_S:  NewBranchOpcode = CEE_BEQ_S; break;
                case CEE_BGE_S:     NewBranchOpcode = CEE_BLT_S; break;
                case CEE_BGE_UN_S:  NewBranchOpcode = CEE_BLT_UN_S; break;
                case CEE_BGT_S:     NewBranchOpcode = CEE_BLE_S; break;
                case CEE_BGT_UN_S:  NewBranchOpcode = CEE_BLE_UN_S; break;
                case CEE_BLE_S:     NewBranchOpcode = CEE_BGT_S; break;
                case CEE_BLE_UN_S:  NewBranchOpcode = CEE_BGT_UN_S; break;
                case CEE_BLT_S:     NewBranchOpcode = CEE_BGE_S; break;
                case CEE_BLT_UN_S:  NewBranchOpcode = CEE_BGE_UN_S; break;

                default:
                    VSASSERT(pcblk->opcodeJump == CEE_BR_S || pcblk->opcodeJump == CEE_LEAVE_S,
                             "OptimizeBranches: unexpected branch opcode");
                    continue;
            }

            VSASSERT(NewBranchOpcode != CEE_NOP, "OptimizeBranches: should have chosen a new branch opcode by now");

            pcblk->opcodeJump = NewBranchOpcode;
            pcblk->pcblkJumpDest = pcblk->Next()->pcblkJumpDest;
            // advance the iterator because it's already sitting on the block
            // we want to delete.
            Iter.Next();
            m_cblkCodeList.Peel(pcblk->Next());
        }

    }  // while
}

//========================================================================
// Mark any blocks that flow from the block as live
//========================================================================

void CodeGenerator::MarkLiveBlocks
(
CODE_BLOCK *pcblk
)
{
    VSASSERT(pcblk, "CodeGenerator::MarkLiveBlocks():  must have valid code block");

    if (pcblk->fLive)
    {
        return; // We've already been here
    }

    pcblk->fLive = true; // Mark that we've been here

    VSASSERT(
        IsShortBranch(pcblk->opcodeJump)    ||
        pcblk->opcodeJump == CEE_NOP        ||
        pcblk->opcodeJump == CEE_RET        ||
        pcblk->opcodeJump == CEE_THROW      ||
        pcblk->opcodeJump == CEE_RETHROW    ||
        pcblk->opcodeJump == CEE_ENDFILTER  ||
        pcblk->opcodeJump == CEE_ENDFINALLY ||
        pcblk->opcodeJump == CEE_SWITCH,
        "CodeGenerator::MarkLiveBlocks():  must be jump, switch, or block end");

    // Now visit children
    if (IsShortBranch(pcblk->opcodeJump))
    {
checkbranch:

        // If the destination codeblock has no code in it, then attempt to collapse the jumps.
        // Also check that we aren't jumping back to the current codeblock.  If so,
        // just continue.  NOTE:  This condition represents an infinite loop in the user's code.
        if (pcblk->pcblkJumpDest->usCodeSize == 0 && pcblk->pcblkJumpDest != pcblk)
        {
            if (pcblk->opcodeJump == CEE_BR_S && pcblk->pcblkJumpDest->opcodeJump == CEE_LEAVE_S)
            {
                // Unconditional branch to leave
                pcblk->opcodeJump = CEE_LEAVE_S;
                pcblk->pcblkJumpDest = pcblk->pcblkJumpDest->pcblkJumpDest;
            }
            else if (pcblk->opcodeJump == CEE_BR_S && pcblk->pcblkJumpDest->opcodeJump == CEE_RET)
            {
                // Unconditional branch to Return
                pcblk->opcodeJump = CEE_RET;
                pcblk->pcblkJumpDest = NULL;
                // This is now a dead-end, so we're finished here.
                return;
            }
            else if (pcblk->pcblkJumpDest->opcodeJump == CEE_BR_S ||
                     (pcblk->opcodeJump == CEE_LEAVE_S && pcblk->pcblkJumpDest->opcodeJump == CEE_LEAVE_S))
            {
                // (Un)conditional branch to branch
                // leave to leave
                pcblk->pcblkJumpDest = pcblk->pcblkJumpDest->pcblkJumpDest;
                goto checkbranch;
            }
        }

        MarkLiveBlocks(pcblk->pcblkJumpDest);

        if (pcblk->opcodeJump != CEE_BR_S &&
            pcblk->opcodeJump != CEE_LEAVE_S)
        {
            MarkLiveBlocks(pcblk->Next());
        }
    }
    else if (pcblk->opcodeJump == CEE_NOP)
    {
        MarkLiveBlocks(pcblk->Next());
    }
    else if (pcblk->opcodeJump == CEE_SWITCH)
    {
        unsigned iBlk;
        SWITCH_TABLE * pswtbl = pcblk->pswTable;

        for (iBlk = 0; iBlk < pswtbl->cEntries; iBlk++)
        {
            MarkLiveBlocks(pswtbl->pcodeaddrs[iBlk].pcblk);
        }

        if (pswtbl->pcblkCaseElse)
        {
            MarkLiveBlocks(pswtbl->pcblkCaseElse);
        }

        if (pswtbl->pcblkFallThrough)
        {
            MarkLiveBlocks(pswtbl->pcblkFallThrough);
        }
    }
    // if it's not a blocking opcode, we have a problem
    else if (pcblk->opcodeJump != CEE_RET &&
             pcblk->opcodeJump != CEE_THROW &&
             pcblk->opcodeJump != CEE_RETHROW &&
             pcblk->opcodeJump != CEE_ENDFINALLY &&
             pcblk->opcodeJump != CEE_ENDFILTER)
    {
        VSFAIL("CodeGenerator::MarkLiveBlocks():  complete fall through unexpected" );
        VbThrow(HrMake(ERRID_CodegenError));
    }
}

void CodeGenerator::InsertENCRemappablePoint
(
    BCSYM *TypeOnStack
)
{
    if (!m_Project->GenerateENCableCode())
    {
        return;
    }

    // For ENC remapping to happen, before every branch to a user statement, a hidden
    // sequence point needs to be emitted. Also the stack needs to be empty, thus
    // the store to a temp and a load. The load is associated with the hidden sequence
    // point.
    //
    BCSYM_Variable *Temporary = CreateCodeGenTemporary(TypeOnStack);
    GenerateStoreLocal(Temporary->GetLocalSlot());

    StartHiddenIL();
    GenerateLoadLocal(Temporary->GetLocalSlot(),
                      Temporary->GetType());
}

void CodeGenerator::InsertENCRemappablePoint
(
    CODE_BLOCK **ppcblkTrue,
    CODE_BLOCK **ppcblkFalse,
    bool fReverse
)
{
    if (!m_Project->GenerateENCableCode())
    {
        return;
    }

    CODE_BLOCK *&pcblkTrue = *ppcblkTrue;
    CODE_BLOCK *&pcblkFalse = *ppcblkFalse;

    CODE_BLOCK *pcblkBranchOut = NewCodeBlock();

    BCSYM_Variable *pTemporary =
        CreateCodeGenTemporary(m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(t_bool));

    if (fReverse)
    {
        VSASSERT(m_pcblkCurrent == pcblkTrue, "True block expected to be current!!!");

        GenerateLiteralInt(1);
        EndCodeBuffer(CEE_BR_S, pcblkBranchOut);

        SetAsCurrentCodeBlock(pcblkFalse);
        GenerateLiteralInt(0);
        EndCodeBuffer(CEE_NOP);
    }
    else
    {
        VSASSERT(m_pcblkCurrent == pcblkFalse, "False block expected to be current!!!");

        GenerateLiteralInt(0);
        EndCodeBuffer(CEE_BR_S, pcblkBranchOut);

        SetAsCurrentCodeBlock(pcblkTrue);
        GenerateLiteralInt(1);
        EndCodeBuffer(CEE_NOP);
    }

    SetAsCurrentCodeBlock(pcblkBranchOut);
    GenerateStoreLocal(pTemporary->GetLocalSlot());

    // 2 loads, both of which branch to the same store, so reduce stack count by 1 to indicate the balance.
    AddToCurStack(-1);
    m_stackTypes.PopOrDequeue();

    StartHiddenIL();
    GenerateLoadLocal(pTemporary->GetLocalSlot(), pTemporary->GetType());

    pcblkTrue = NewCodeBlock();
    pcblkFalse = NewCodeBlock();

    if (fReverse)
    {
        EndCodeBuffer(CEE_BRFALSE_S, pcblkFalse);
        SetAsCurrentCodeBlock(pcblkTrue);
    }
    else
    {
        EndCodeBuffer(CEE_BRTRUE_S, pcblkTrue);
        SetAsCurrentCodeBlock(pcblkFalse);
    }
}

/*========================================================================

Routines used to emit the actual code from code buffers

========================================================================*/

//========================================================================
// Flattens a switch table struct into the code buffer. Should only be
// called when we are flattening the code blocks. Does NOT emit into the
// code blocks
//========================================================================

void CodeGenerator::CollateSwitchTable
(
CODE_BLOCK * pcblkSwitch
)
{
    SWITCH_TABLE  * pswTable;
    unsigned long   ulFallThroughAddr;
    unsigned long   cEntries;

    pswTable = pcblkSwitch->pswTable;
    cEntries = pswTable->cEntries;

#if DEBUG
    {
        // Verify that all entries of SWITCH table are set
        for (unsigned i = 0; i < cEntries; i++)
        {
            VSASSERT( pswTable->pcodeaddrs[i].pcblk != NULL,
                       "CollateSwitchTable: each switch table entry must have a jump offset." );
        }
    }
#endif

    // write the CEE_SWITCH opcode
    CodeBuf_WritePcode(CEE_SWITCH);

    // write the size of the table - 4 byte unsigned field
    CodeBuf_Write32((int)cEntries);

    // get address of fall through code block
    ulFallThroughAddr = pswTable->pcblkFallThrough->ulBlockOffset;

    // emit all the entries. Entries are relative to address of
    // instruction following switch instruction
    for (unsigned i = 0; i < cEntries; i++)
    {
        CodeBuf_Write32(CodeAddress(&(pswTable->pcodeaddrs[i])) - ulFallThroughAddr);
    }

    return;
}

//========================================================================
// Iterate through all the code blocks and determine their offset
// addresses. This is a multi-pass job. First, we go through and assume
// all jumps are going to be short jumps and lay out the blocks. Then we
// go back through and make any jumps to long that have to be. This may
// cause other jumps to become long. We keep iterating until there are no
// changes
//========================================================================

unsigned long CodeGenerator::CalculateBlockAddresses
(
)
{
    CODE_BLOCK *pcblk;
    unsigned long cTotalBytes;
    bool fModified = true;

    CSingleListIter<CODE_BLOCK>  Iter(&m_cblkCodeList);

    while (fModified)
    {
        fModified = false;
        cTotalBytes = 0;

        // Iterate through code blocks and set the block offsets
        Iter.Reset();
        while (pcblk = Iter.Next())
        {
            pcblk->ulBlockOffset = cTotalBytes;

            cTotalBytes += pcblk->usCodeSize;

            // Add the size of the jump byte code
            // SWITCH table is a special case
            if (pcblk->opcodeJump == CEE_SWITCH)
            {
                // Add size of SWITCH opcode + 4 byte unsigned "size of table" field
                cTotalBytes += CBytesOpcode(CEE_SWITCH) + sizeof(unsigned long);
                cTotalBytes += pcblk->pswTable->cEntries * sizeof(long);
            }
            else if (pcblk->opcodeJump == CEE_THROW ||
                     pcblk->opcodeJump == CEE_RETHROW ||
                     pcblk->opcodeJump == CEE_RET  ||
                     pcblk->opcodeJump == CEE_ENDFILTER ||
                     pcblk->opcodeJump == CEE_ENDFINALLY ||
                     pcblk->opcodeJump == CEE_POP)
            {
                cTotalBytes += CBytesOpcode(pcblk->opcodeJump);
            }
            else if (pcblk->opcodeJump != CEE_NOP)
            {
                cTotalBytes += CBytesBranchOpcodeOperand(pcblk->opcodeJump);
            }

        } // while

        // Now, check to make sure opcodes are OK
        Iter.Reset();
        while (pcblk = Iter.Next())
        {
            signed long lJumpOffset;

            if (IsShortBranch(pcblk->opcodeJump))
            {
                lJumpOffset = pcblk->pcblkJumpDest->ulBlockOffset -
                                (pcblk->ulBlockOffset +
                                 pcblk->usCodeSize +
                                 CBytesBranchOpcodeOperand(pcblk->opcodeJump));

                if (lJumpOffset < SCHAR_MIN || lJumpOffset > SCHAR_MAX)
                {
                    // Gotta make it a long jump
                    pcblk->opcodeJump = MapShort2LongJumpOpcode(pcblk->opcodeJump);
                    fModified = true;
                }
            }
        }
    }

    m_cTotalBytes = cTotalBytes;

    return cTotalBytes;
}

//========================================================================
// Emit all the EH clause information
//========================================================================

void CodeGenerator::EmitEHClauses
(
)
{
    CSingleListIter<TRY_BLOCK> IterTry(&m_tryList);
    TRY_BLOCK * ptry;

    IterTry.Reset();
    while (ptry = IterTry.Next())
    {
        VSASSERT(ptry->pcblkTry->fLive, "Try codeblock is not marked live!");

        CSingleListIter<CATCH_BLOCK> IterCatch(&ptry->catchList);
        CATCH_BLOCK * pcatch;
        unsigned ulStartOffset = ptry->pcblkTry->ulBlockOffset;
        unsigned ulEndOffset;

        if (ptry->catchList.NumberOfEntries())
        {
            pcatch = ptry->catchList.GetFirst();

            if (pcatch->pcblkFilter)
            {
                VSASSERT(pcatch->pcblkFilter->fLive, "Filter codeblock is not marked live!");
                ulEndOffset = pcatch->pcblkFilter->ulBlockOffset;
            }
            else
            {
                VSASSERT(pcatch->pcblkCatch->fLive, "Catch codeblock is not marked live!");
                ulEndOffset = pcatch->pcblkCatch->ulBlockOffset;
            }
        }
        else
        {
            VSASSERT(ptry->pcblkFinally, "Try without Catch or Finally!");
            VSASSERT(ptry->pcblkFinally->fLive, "Finally codeblock is not marked live!");
            ulEndOffset = ptry->pcblkFinally->ulBlockOffset;
        }

        IterCatch.Reset();
        while (pcatch = IterCatch.Next())
        {
            // a catch block MUST exit via these opcodes
            VSASSERT(pcatch->pcblkLast->opcodeJump == CEE_LEAVE_S ||
                     pcatch->pcblkLast->opcodeJump == CEE_LEAVE ||
                     pcatch->pcblkLast->opcodeJump == CEE_THROW ||
                     pcatch->pcblkLast->opcodeJump == CEE_RETHROW,
                     "EmitEHClauses: catch block has invalid jump opcode!");

            CorExceptionFlag   flags;
            mdTypeRef          typref;
            unsigned long      ulHandlerLength;

            if (pcatch->pcblkFilter)
            {
                flags = (CorExceptionFlag)
                        (COR_ILEXCEPTION_CLAUSE_FILTER |
                        COR_ILEXCEPTION_CLAUSE_OFFSETLEN);
            }
            else
            {
                flags = (CorExceptionFlag)
                        (COR_ILEXCEPTION_CLAUSE_NONE |
                        COR_ILEXCEPTION_CLAUSE_OFFSETLEN);
            }

            if (pcatch->psymType)
            {
                typref = m_pmdemit->DefineTypeRefBySymbol(pcatch->psymType, &pcatch->ExceptionVarLocation);
            }
            else
            {
                // the filter will itself verify the exception's type is correct
                if (pcatch->pcblkFilter)
                {
                    typref = mdTypeRefNil;
                }
                else
                {
                    typref = m_pmdemit->DefineTypeRefBySymbol(m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ExceptionType), NULL);
                }
            }

            if (pcatch->pcblkLast->Next())
            {
                VSASSERT(pcatch->pcblkLast->Next()->fLive, "Catch boundary block is not marked live!");
                ulHandlerLength = pcatch->pcblkLast->Next()->ulBlockOffset -
                                  pcatch->pcblkCatch->ulBlockOffset;
            }
            else
            {
                // There's no block past the end
                ulHandlerLength = m_cTotalBytes - pcatch->pcblkCatch->ulBlockOffset;
            }

            m_pmdemit->AddEHClause(flags,
                                   ulStartOffset,
                                   ulEndOffset - ulStartOffset,
                                   pcatch->pcblkCatch->ulBlockOffset,
                                   ulHandlerLength,
                                   pcatch->pcblkFilter ?
                                      pcatch->pcblkFilter->ulBlockOffset :
                                      0,
                                   typref);
        }

        if (ptry->pcblkFinally)
        {
            VSASSERT(ptry->pcblkFallThrough->fLive, "FallThrough codeblock is not marked live!");

            // Finally encompasses try and all catches
            m_pmdemit->AddEHClause((CorExceptionFlag)
                                        (COR_ILEXCEPTION_CLAUSE_FINALLY |
                                        COR_ILEXCEPTION_CLAUSE_OFFSETLEN),
                                    ulStartOffset,
                                    ptry->pcblkFinally->ulBlockOffset -
                                        ulStartOffset,
                                    ptry->pcblkFinally->ulBlockOffset,
                                    ptry->pcblkFallThrough->ulBlockOffset -
                                        ptry->pcblkFinally->ulBlockOffset,
                                    0,
                                    0);
        }
    }
}

//========================================================================
// Decides if we should Emit the debug information about a method
//========================================================================

bool CodeGenerator::ShouldEmitDebugInfoForProc()
{
    if (m_Project->GeneratePDB())
    {
        // if the Method isn't hidden and it's not a method decl
        if (!m_pproc->IsHidden() && m_pproc->IsMethodDecl())
        {
            return true;
        }
        // Or is the Form Main or Lambda
        if (m_pproc->IsSyntheticMethod() &&
            (m_pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_FormMain ||
             m_pproc->PSyntheticMethod()->IsResumable() ||
             m_pproc->PSyntheticMethod()->IsLambda()))
        {
            return true;
        }
        // or if it's a constructor (we might have instance variable we initialize)
        if (m_pproc->IsAnyConstructor())
        {
            // Check to see if there's any non hidden lines of code in the constructor

            // If there's only one entry
            if (m_pltblLineTbl+1 == m_pltblCurrent)
            {
                // and it's a hidden line
                if (m_pltblLineTbl->lSrcLine == HIDDEN_LINE_INDICATOR)
                {
                    // Then we don't need to generate code
                    return false;
                }
            }
            return true;
        }
    }
    return false;

}

//========================================================================
// Emits the debug information about a method
//========================================================================

void CodeGenerator::EmitDebugInformation
(
)
{
    //Clear the DebugInfoList regardless of whether we emitt the code.
    if (m_pproc->IsSyntheticMethod())
    {
        m_pproc->PSyntheticMethod()->GetDebugInfoList()->Clear();
    }
    else if (m_pproc->IsMethodImpl())
    {
        m_pproc->PMethodImpl()->GetDebugInfoList()->Clear();
    }

    if (ShouldEmitDebugInfoForProc())
    {
        bool hasSeenSequencePoint = false;
        // Verify that something has been written to the line table
        if (m_pltblCurrent > m_pltblLineTbl)
        {
#if DEBUG
            // Dumps the line table header, if the debug switch dump.linetable is on.
            // The actual lines are dumped in AddToLineTable
            if (VSFSWITCH(fDumpLineTable)) {
                DebPrintf("-------------------------------------------------------------------\n");
                DebPrintf("          Line Number/IL Offset for Method : %S\n", m_pproc->GetEmittedName());
                DebPrintf("-------------------------------------------------------------------\n\n");
                DebPrintf("     Row   Column  EndRow  EndColumn  ILOffset\n");
            }
#endif

            LINETBL       * pltbl;

            // The number of lines is usually more than required because there are some duplicate entires.
            if (m_pproc->IsSyntheticMethod())
            {
                CreateLineTable(m_pproc->PSyntheticMethod()->GetDebugInfoList(),
                                (ULONG32)(m_pltblCurrent - m_pltblLineTbl));
            }
            else if (m_pproc->IsMethodImpl())
            {
                // Also the +2 is for the lines containing "Sub/Function foo()" and "End Sub/Function".
                CreateLineTable(m_pproc->PMethodImpl()->GetDebugInfoList(),
                                (ULONG32)(m_pltblCurrent - m_pltblLineTbl)+2);
            }


            // Add the locations of the lines in the function body
            LINETBL *ptblNext = NULL;

            for (pltbl = m_pltblLineTbl; pltbl < m_pltblCurrent; pltbl = ptblNext)
            {
                ptblNext = pltbl + 1;

                // Don't emit line information for dead blocks
                if (m_Project->GenerateOptimalIL() && !pltbl->codeaddr.pcblk->fLive)
                {
                    continue;
                }

                // Check for duplicate offsets, must be done here.
                for(LINETBL *pltblDup = pltbl + 1; pltblDup < m_pltblCurrent; pltblDup++)
                {
                    // Bug VSWhidbey 137946
                    if (m_Project->GenerateOptimalIL() && !pltblDup->codeaddr.pcblk->fLive)
                    {
                        ptblNext = pltblDup + 1;
                        continue;
                    }

                    if (CodeAddress(&pltbl->codeaddr) == CodeAddress(&(pltblDup)->codeaddr))
                    {
                        pltbl = pltblDup;
                        ptblNext = pltblDup + 1;
                        continue;
                    }

                    break;
                }

                // All offsets must be strictly in increasing order, so remove any that aren't
                if (pltbl > m_pltblLineTbl && (CodeAddress(&pltbl->codeaddr) < CodeAddress(&(pltbl-1)->codeaddr)))
                {
                    continue;
                }

                if (m_pproc->IsSyntheticMethod() || m_pproc->IsMethodImpl())
                {
                    CSingleList<MethodDebugInfo> *pDebugInfoList = m_pproc->IsSyntheticMethod() ?
                                                                    m_pproc->PSyntheticMethod()->GetDebugInfoList() :
                                                                    m_pproc->PMethodImpl()->GetDebugInfoList();

                    ULONG ulAddress = CodeAddress(&pltbl->codeaddr);
                    if(ulAddress > 0 && !hasSeenSequencePoint)
                    {
                        ulAddress = 0;
                    }
                    hasSeenSequencePoint |=
                        AddToLineTable(pDebugInfoList,
                                        pltbl->lSrcLine+1,
                                        pltbl->lSrcColumn+1,
                                        ulAddress,
                                        pltbl->lEndSrcLine+1,
                                        pltbl->lEndSrcColumn+1,
                                        pltbl->pSourceFile);
                }
            }

            // Dump the line table if debug switch for the same is on
            DBG_SWITCH_PRINTF(fDumpLineTable, "\n\n")
        }

        BCSYM_Class *pContainingClass = m_pproc->GetContainingClass();
        if (pContainingClass && pContainingClass->IsStateMachineClass())
        {
            BCSYM_Proc *pOriginalResumableProc = pContainingClass->GetOriginalResumableProc();

            if (pOriginalResumableProc->IsAsyncKeywordUsed())
            {
                VSASSERT(m_AwaitYieldOffsets.Count() == m_AwaitResumeOffsets.Count(), "Mismatch between number of await yield and await resume code addresses");

                ULONG cLiveAwaitOffsets = 0;
                for (ULONG i = 0; i < m_AwaitYieldOffsets.Count(); i++)
                {
                    if (m_AwaitYieldOffsets.Element(i).pcblk->fLive)
                    {
                        cLiveAwaitOffsets++;
                    }
                }

                AsyncMethodDebugInfo *pAsyncMethodDebugInfo = CreateAsyncMethodDebugInfo(cLiveAwaitOffsets, pOriginalResumableProc);

                for (ULONG iSrc = 0, iDst = 0; iSrc < m_AwaitYieldOffsets.Count(); iSrc++)
                {
                    // Discard await debug info for dead blocks when optimizations are turned on
                    if (m_AwaitYieldOffsets.Element(iSrc).pcblk->fLive)
                    {
                        VSASSERT(m_AwaitResumeOffsets.Element(iSrc).pcblk->fLive, "Await resume point block must be alive if yield point block is alive!");
                        pAsyncMethodDebugInfo->m_rglYieldOffsets[iDst] = (unsigned int)CodeAddress(&m_AwaitYieldOffsets.Element(iSrc));
                        pAsyncMethodDebugInfo->m_rglBreakpointOffsets[iDst] = (unsigned int)CodeAddress(&m_AwaitResumeOffsets.Element(iSrc));
                        iDst++;
                    }
                }

                if (m_pcblkAsyncSubCatch != NULL)
                {
                    pAsyncMethodDebugInfo->m_catchHandlerOffset = m_pcblkAsyncSubCatch->ulBlockOffset;
                }

                if (m_pproc->IsSyntheticMethod())
                {
                    m_pproc->PSyntheticMethod()->SetAsyncDebugInfo(pAsyncMethodDebugInfo);
                }
                else
                {
                    m_pproc->PMethodImpl()->SetAsyncDebugInfo(pAsyncMethodDebugInfo);
                }
            }
        }

        if (m_ptreeFunc->AsProcedureBlock().pproc->IsMethodImpl())
        {
            m_ptreeFunc->AsProcedureBlock().pproc->PMethodImpl()->SetMethodScope(m_MethodScope);
        }
        else if (m_ptreeFunc->AsProcedureBlock().pproc->IsSyntheticMethod())
        {
            m_ptreeFunc->AsProcedureBlock().pproc->PSyntheticMethod()->SetMethodScope(m_MethodScope);
        }
    }
}

//========================================================================
// Do the actual byte code emit
//========================================================================

void CodeGenerator::EmitByteCodes
(
BYTE *pbImage
)
{
    VSASSERT(m_usCurStackLvl == 0,
             "EmitByteCodes: stack level must be 0 at end");
    VSASSERT(m_stackTypes.Count() == 0,
             "There should be nothing left on m_stackTypes at this point!");
    VSASSERT(m_tryListPending.NumberOfEntries() == 0,
             "EmitByteCodes: no open try blocks at end");

    // Write the header and exception information to the PE Builder.
    BYTE * pbIL =
        m_pmdemit->EmitImage(
            pbImage,
            m_cTotalBytes,
            m_usMaxStackLvl,
            m_pmdemit->GetSignature(),
            m_pmdemit->GetSigSize());

    // write out the byte codes

    // Collate the code blocks and write into the provide buffer.
    CodeBuf_Create(pbIL, m_cTotalBytes);

    // Iterate through code blocks
    CSingleListIter<CODE_BLOCK> CodeBlockIter(&m_cblkCodeList);


    while (CODE_BLOCK * pcblk = CodeBlockIter.Next())
    {
        if (pcblk->usCodeSize)
        {
            CodeBuf_WriteData((const char *)pcblk->pbCodeBuffer, pcblk->usCodeSize);
        }

        if (pcblk->opcodeJump == CEE_SWITCH)
        {
            CollateSwitchTable(pcblk);
        }
        else if (pcblk->opcodeJump == CEE_THROW ||
                 pcblk->opcodeJump == CEE_RETHROW ||
                 pcblk->opcodeJump == CEE_RET ||
                 pcblk->opcodeJump == CEE_ENDFILTER ||
                 pcblk->opcodeJump == CEE_ENDFINALLY ||
                 pcblk->opcodeJump == CEE_POP)
        {
            CodeBuf_WritePcode(pcblk->opcodeJump);
        }
        else if (pcblk->opcodeJump != CEE_NOP)
        {
            long lJumpOffset;

            // Note: usCodeSize does not include jump instruction itself.
            // The offset is from the beginning of the instruction
            // *following* the branch.
            lJumpOffset = pcblk->pcblkJumpDest->ulBlockOffset -
                            (pcblk->ulBlockOffset +
                             pcblk->usCodeSize +
                             CBytesBranchOpcodeOperand(pcblk->opcodeJump));

            VSASSERT(!IsShortBranch(pcblk->opcodeJump) ||
                      (lJumpOffset >= SCHAR_MIN && lJumpOffset <= SCHAR_MAX),
                      "wrong jump size!");

            CodeBuf_WritePcode(pcblk->opcodeJump);

            if (IsShortBranch(pcblk->opcodeJump))
            {
                CodeBuf_Write08((char)lJumpOffset);
            }
            else
            {
                CodeBuf_Write32(lJumpOffset);
            }
        }

    } // while
}

//============================================================================
// Allocates space for the line table. The size of the line numbers and
// offsets is assumed to be 4 bytes (unsigned long).
//============================================================================
#pragma optimize( "", off)
MethodDebugInfo *CodeGenerator::CreateLineTable
(
    CSingleList<MethodDebugInfo> *pMethodDebugInfoList,
    unsigned long  ulNoLines
)
{
    MethodDebugInfo *pMethodDebugInfo = (MethodDebugInfo *)m_pnraDebugSpace->Alloc(sizeof(MethodDebugInfo));

    pMethodDebugInfoList->InsertLast(pMethodDebugInfo);

    pMethodDebugInfo->m_pstrDocumentName = NULL;

    // Reset to default values
    if (ulNoLines)
    {
        if (ulNoLines > (size_t) -1 / (sizeof(unsigned int) * 5))
        {
            VbThrow(HrMake(ERRID_CodegenError));
        }

        pMethodDebugInfo->m_rglLineTable      = (unsigned int *)m_pnraDebugSpace->Alloc(ulNoLines * sizeof(unsigned int) * 5);
        pMethodDebugInfo->m_rglOffsetTable    = pMethodDebugInfo->m_rglLineTable + ulNoLines;
        pMethodDebugInfo->m_rglColumnTable    = pMethodDebugInfo->m_rglOffsetTable + ulNoLines;
        pMethodDebugInfo->m_rglEndLineTable   = pMethodDebugInfo->m_rglColumnTable + ulNoLines;
        pMethodDebugInfo->m_rglEndColumnTable = pMethodDebugInfo->m_rglEndLineTable + ulNoLines;
        pMethodDebugInfo->m_ulNoLines         = ulNoLines;
        pMethodDebugInfo->m_ulCurrNoLines     = 0;
    }
    else
    {
        pMethodDebugInfo->m_rglLineTable      = NULL;
        pMethodDebugInfo->m_rglOffsetTable    = NULL;
        pMethodDebugInfo->m_rglColumnTable    = NULL;
        pMethodDebugInfo->m_rglEndLineTable   = NULL;
        pMethodDebugInfo->m_rglEndColumnTable = NULL;
        pMethodDebugInfo->m_ulNoLines         = 0;
        pMethodDebugInfo->m_ulCurrNoLines     = 0;
    }

    return pMethodDebugInfo;
}
#pragma optimize( "", on)

//============================================================================
// Adds a new (line#, offset) pair to the line table.
//============================================================================

bool CodeGenerator::AddToLineTable
(
    CSingleList<MethodDebugInfo> *pMethodDebugInfoList,
    unsigned long ulLine,
    unsigned long ulColumn,
    unsigned long ulOffset,
    unsigned long ulEndLine,
    unsigned long ulEndColumn,
    SourceFile *pPhysicalSourceFile
)
{
    // The physical source file is the file the code for these lines is physically present in.
    // Code for a method could come from different source files depending, eg: the member
    // init code in constructors when the members are present in different partial types
    // located in different files.
    //
    SourceFile *pDefaultFile = m_pproc->GetSourceFile();
    SourceFile *pSourceFile = pPhysicalSourceFile ? pPhysicalSourceFile : pDefaultFile;
    MethodDebugInfo * pMethodDebugInfo = pMethodDebugInfoList->GetFirst();

    VSASSERT(pMethodDebugInfo && pMethodDebugInfo->m_rglLineTable, "bad line table!");
    VSASSERT((pMethodDebugInfo->m_ulCurrNoLines < pMethodDebugInfo->m_ulNoLines), "line table size exceeded!");

#if !IDE

    // Map the line number to its true location in the source.
    if (pSourceFile->IsMappedFile() && ulLine < HIDDEN_LINE_INDICATOR)
    {
        STRING *pstrDocumentName = NULL;
        bool hasDebugInfo = false;
        long lMappedLine;

        lMappedLine = pSourceFile->GetMappedLineInfo(ulLine - 1, &hasDebugInfo, &pstrDocumentName)+1;

        if (hasDebugInfo)
        {
            ULONG ulNoLines = pMethodDebugInfo->m_ulNoLines;
            while (pMethodDebugInfo)
            {
                if ((!pMethodDebugInfo->m_pstrDocumentName && pSourceFile == pDefaultFile) ||
                    StringPool::IsEqual(pMethodDebugInfo->m_pstrDocumentName, pstrDocumentName))
                {
                    break;
                }
                pMethodDebugInfo = pMethodDebugInfo->Next();
            }

            if (pMethodDebugInfo == NULL)
            {
                pMethodDebugInfo = CreateLineTable(pMethodDebugInfoList, ulNoLines);
            }

            VSASSERT(pMethodDebugInfo, "Invalid state");

            if (!pMethodDebugInfo->m_pstrDocumentName)
            {
                pMethodDebugInfo->m_pstrDocumentName = pstrDocumentName;
            }

            if (StringPool::IsEqual(pMethodDebugInfo->m_pstrDocumentName, pstrDocumentName))
            {
                pMethodDebugInfo->m_pstrDocumentName = pstrDocumentName;
                ulEndLine = ulEndLine - ulLine + lMappedLine;
                ulLine = lMappedLine;
            }
            else
            {
                // Don't add this to the table at all.
                return false;
            }
        }
        else
        {
            // Don't add this to the table at all.
            return false;
        }
    }
    else
#endif !IDE
    {
        if (pSourceFile != pDefaultFile)
        {
            ULONG ulNoLines = pMethodDebugInfo->m_ulNoLines;
            while (pMethodDebugInfo)
            {
                if (StringPool::IsEqual(pMethodDebugInfo->m_pstrDocumentName, pSourceFile->GetFileName()))
                {
                    break;
                }
                pMethodDebugInfo = pMethodDebugInfo->Next();
            }

            if (!pMethodDebugInfo)
            {
                // ulNoLines is the total for the whole method and so is the upper limit.
                // There is wastage from using ulNoLines, but is okay because this is
                // possible only for the constructor of partial classes. Constructors
                // are usually not very large nor will there be too many partial components
                // for a class. Keeping track of the exact number requires a lot more
                // work and is not worth the gain.
                //
                pMethodDebugInfo = CreateLineTable(pMethodDebugInfoList, ulNoLines);
                pMethodDebugInfo->m_pstrDocumentName = pSourceFile->GetFileName();
            }
        }
    }

    // Dumps the sequence point, if the debug switch dump.linetable is on.
    // Header for each method is printed in EmitDebugInformation
    // (Another way to get this information is "ildasm /linenum")
    DBG_SWITCH_PRINTF(fDumpLineTable, "%8d, %3d, %8d, %3d,   %5d  (0x%x)\n", ulLine, ulColumn, ulEndLine, ulEndColumn+1, ulOffset, ulOffset);

    // Save the line# and offset.
    pMethodDebugInfo->m_rglLineTable[pMethodDebugInfo->m_ulCurrNoLines] = ulLine;
    pMethodDebugInfo->m_rglOffsetTable[pMethodDebugInfo->m_ulCurrNoLines] = ulOffset;
    pMethodDebugInfo->m_rglColumnTable[pMethodDebugInfo->m_ulCurrNoLines] = ulColumn;
    pMethodDebugInfo->m_rglEndLineTable[pMethodDebugInfo->m_ulCurrNoLines] = ulEndLine;
    // End column from parser is inclusive
    // e.g. 'End Sub' is column 1 to 7 in the parser
    //      and column 1 to 8 in the PDB world
    pMethodDebugInfo->m_rglEndColumnTable[pMethodDebugInfo->m_ulCurrNoLines] = ulEndColumn+1;

    pMethodDebugInfo->m_ulCurrNoLines++;
    return true;
}

//============================================================================
// Allocates an AsyncMethodDebugInfo and space for its yield and breakpoint
// offsets arrays.
//============================================================================

AsyncMethodDebugInfo *CodeGenerator::CreateAsyncMethodDebugInfo
(
    unsigned long ulNoAwaits,
    BCSYM_Proc *pOriginalProc
)
{
    AsyncMethodDebugInfo *pAsyncMethodDebugInfo = (AsyncMethodDebugInfo *)m_pnraDebugSpace->Alloc(sizeof(AsyncMethodDebugInfo));

    pAsyncMethodDebugInfo->m_pKickoffProc = pOriginalProc;
    pAsyncMethodDebugInfo->m_ulNoAwaits = ulNoAwaits;
    pAsyncMethodDebugInfo->m_catchHandlerOffset = (unsigned int)-1;

    if (ulNoAwaits)
    {
        pAsyncMethodDebugInfo->m_rglYieldOffsets = (unsigned int *)m_pnraDebugSpace->Alloc(ulNoAwaits * sizeof(unsigned int) * 2);
        pAsyncMethodDebugInfo->m_rglBreakpointOffsets = pAsyncMethodDebugInfo->m_rglYieldOffsets + ulNoAwaits;
    }
    else
    {
        pAsyncMethodDebugInfo->m_rglYieldOffsets = NULL;
        pAsyncMethodDebugInfo->m_rglBreakpointOffsets = NULL;
    }

    return pAsyncMethodDebugInfo;
}

BCSYM* CodeGenerator::GetSymbolForVtype(Vtypes vtype)
{
    return m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(vtype);
}

BCSYM* CodeGenerator::GetSymbolForFXType(FX::TypeName fxType)
{
    return m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetType(fxType);
}

bool CodeGenerator::StackContainsRestrictedTypes(_In_ Location* pLocation)
{
    ArrayListReverseIterator<BCSYM*, NorlsAllocWrapper> iterator = m_stackTypes.GetIterator();
    while (iterator.MoveNext())
    {
        if (IsRestrictedType(iterator.Current(), m_Project->GetCompilerHost()) ||
            IsRestrictedArrayType(iterator.Current(), m_Project->GetCompilerHost()))
        {
            ErrorTable* pErrors = m_pmdemit->GetBuilder()->GetErrorTable();
            StringBuffer textBuffer;
            pErrors->CreateError(ERRID_AsyncRestrictedType1,
                                 pLocation,
                                 pErrors->ExtractErrorName(iterator.Current(), NULL, textBuffer));
            return true;
        }
    }

    return false;
}

bool CodeGenerator::StackContainsUnspillableTypes()
{
    ArrayListReverseIterator<BCSYM*, NorlsAllocWrapper> iterator = m_stackTypes.GetIterator();
    while (iterator.MoveNext())
    {
        if (iterator.Current() == NULL ||
            iterator.Current()->IsPointerType() ||
            iterator.Current()->IsVoidType() ||
            iterator.Current()->IsBad())
        {
            return true;
        }
    }

    return false;
}

bool CodeGenerator::TrySpillAndStoreEvaluationStack(ILTree::AsyncSpillExpression* pSpill, BCSYM** ppSpilledItem, unsigned int* pSpillCount)
{
    // Initialize out parameters to defaults.
    *ppSpilledItem = NULL;
    *pSpillCount = 0;

    // Spill the stack to a local. If this fails, we've already reported an
    // error--just bail out.
    BCSYM_Variable* pScratchSpace;
    unsigned int spillCount;
    if (!TrySpillEvaluationStackCore(&pSpill->Loc, &pScratchSpace, &spillCount))
    {
        return false;
    }

    // If there was nothing to spill, we're done.
    if (spillCount == 0)
    {
        return true;
    }

    // Generate the address information needed for the stack storage location.
    ADDRMODE addr;
    GenerateAddr(pSpill->StackStorageReference, &addr);

    // Push the item to spill from scratch space onto the stack.
    GenerateLoadLocal(pScratchSpace->GetLocalSlot(), pScratchSpace->GetType());

    // If we're only spilling a single item, we may need to box it.
    BCSYM* pTypeOnTopOfStack = m_stackTypes.Peek();
    if (pTypeOnTopOfStack->GetVtype() != t_ref &&
        pTypeOnTopOfStack->GetVtype() != t_string &&
        pTypeOnTopOfStack->GetVtype() != t_array)
    {
        EmitOpcode_Tok(CEE_BOX,
                        m_pmdemit->DefineTypeRefBySymbol(pTypeOnTopOfStack, &pSpill->Loc),
                        GetSymbolForFXType(FX::ObjectType));
    }

    // Store the item in the field.
    GenerateStore(pSpill->StackStorageReference, &addr);

    // Save the actual type of the spilled item.
    *ppSpilledItem = pScratchSpace->GetType();
    *pSpillCount = spillCount;
    if (pScratchSpace->IsTemporary())
    {
        m_pCodeGenTemporaryManager->FreeTemporary(pScratchSpace);
    }

    return true;
}

bool CodeGenerator::TrySpillEvaluationStackCore(_In_ Location* pLocation, _Out_opt_ BCSYM_Variable** ppLocalStorage, _Out_opt_ unsigned int* pSpillCount)
{
    VSASSERT(m_stackTypes.Count() == m_usCurStackLvl, "Stack counts do not match!");

    // 0.) Initialize out parameters to defaults

    *ppLocalStorage = NULL;
    *pSpillCount = 0;

    // 1.) If there's nothing on the stack, we don't need to spill.

    if (m_stackTypes.Count() == 0)
    {
        return true;
    }

    // 2.) Check if the evaluation stack has unspillable types on it.

    // Some types can only end up on the stack if there's a logic failure in
    // resumable method lowering or pointer dehydration. This should never
    // happen, so report an internal compiler error.
    if (StackContainsUnspillableTypes())
    {
        VSFAIL("There's an unspillable type on the evaluation stack; how did this happen?");
        m_pmdemit->GetBuilder()->GetErrorTable()->CreateError(ERRID_InternalCompilerError, pLocation);
        return false;
    }

    // The restricted types (ArgIterator, TypedReference, and
    // RuntimeArgumentHandle) can't be detected until now*. We've already
    // emitted an error to educate the user, so simply bail out.
    //
    // *Actually, these could be detected earlier, but if we had the time to do
    // that level of analysis we would have handled stack spilling as a series
    // of expression tree rewrites, rather than here in code gen.
    if (StackContainsRestrictedTypes(pLocation))
    {
        return false;
    }

    // 3.) Collapse the stack into chained tuples, if necessary.

    // We're going to modify the stack as we spill it, so record the current
    // size first.
    *pSpillCount = m_stackTypes.Count();

    Symbols symbolFactory(m_pCompiler, m_pnra, NULL);
    BCSYM* typeArgs[MAX_TUPLE_SIZE_FOR_SPILLING];

    // If we need to spill more than one item, repeatedly collapse the items
    // into tuples until we only have one item remaining.
    while (m_stackTypes.Count() > 1)
    {
        int tupleElements = min(MAX_TUPLE_SIZE_FOR_SPILLING, m_stackTypes.Count());

        // Collect the tuple type arguments.
        ArrayListReverseConstIterator<BCSYM*, NorlsAllocWrapper> typeIterator = m_stackTypes.GetConstIterator();
        typeIterator.MoveNext();
        for (int i = tupleElements - 1; i >= 0; i--, typeIterator.MoveNext())
        {
            typeArgs[i] = typeIterator.Current();
        }

        // Obtain the instantiated tuple type.
        BCSYM_NamedRoot* pTupleSymbol = m_Project->GetCompilerHost()->GetFXSymbolProvider()->GetTupleSymbol(tupleElements, typeArgs, &symbolFactory);
        VSASSERT(pTupleSymbol->IsClass(), "How is the tuple not a class?");

        // Find the tuple constructor
        BCSYM_Proc *pConstructor = NULL;
        for(pConstructor = pTupleSymbol->PClass()->GetFirstInstanceConstructor(m_pCompiler);
            pConstructor != NULL;
            pConstructor = pConstructor->GetNextInstanceConstructor())
        {
            if (pConstructor->GetParameterCount() == tupleElements &&
                pConstructor->GetAccess() == ACCESS_Public)
            {
                break;
            }
        }

        VSASSERT(pConstructor != NULL, "How come we couldn't find the right tuple constructor?");

        mdMemberRef constructorRef = m_pmdemit->DefineMemberRefBySymbol(pConstructor,
                                                                        pTupleSymbol->IsGenericTypeBinding() ? pTupleSymbol->PGenericTypeBinding() : NULL,
                                                                        pLocation);

        // Adjust the stack and emit the opcode to create the tuple.
        AddToCurStack(-tupleElements);
        for (int i = 0; i < tupleElements; i++)
        {
            m_stackTypes.PopOrDequeue();
        }
        EmitOpcode_Tok(CEE_NEWOBJ, constructorRef, pTupleSymbol);
    }

    // 4. Put the collapsed stack items into a local.

    // Create a local as scratch space.
    BCSYM_Variable* pLocalStorage = CreateCodeGenTemporary(m_stackTypes.Peek());

    // Move the item on top of the stack into the scratch space. 
    GenerateStoreLocal(pLocalStorage->GetLocalSlot());

    *ppLocalStorage = pLocalStorage;

    VSASSERT(m_stackTypes.Count() == m_usCurStackLvl, "Stack counts do not match!");

    return true;
}

void CodeGenerator::RestoreEvaluationStack(ILTree::AsyncSpillExpression* pSpill, BCSYM* pSpilledType, unsigned int spillCount)
{
    VSASSERT(pSpilledType != NULL || spillCount == 0, "Need a symbol if we actually spilled something!");
    VSASSERT(m_stackTypes.Count() == m_usCurStackLvl, "Stack counts do not match!");

    // 0.) If no items were spilled, there's nothing for us to do.

    if (spillCount == 0)
    {
        return;
    }

    VSASSERT(m_stackTypes.Count() == 0, "Items on the stack before restoring!");
    Location* pReferencingLocation = &pSpill->Loc;

    // 1.) Pull the spilled item or tuple out of the storage field. If we only
    // spilled a single item, this will complete the restoration.

    ADDRMODE addrForStackLoad;
    GenerateAddr(pSpill->StackStorageReference, &addrForStackLoad);
    GenerateLoad(pSpill->StackStorageReference, &addrForStackLoad);

    // Cast from object to the actual type. We use CEE_UNBOX_ANY rather than
    // calling the GenerateCastClass helper here because we may have stored a
    // single value type or generic type rather than a tuple.
    mdTypeRef typeRef = m_pmdemit->DefineTypeRefBySymbol(pSpilledType->DigThroughAlias(), pReferencingLocation);
    EmitOpcode_Tok(CEE_UNBOX_ANY, typeRef, pSpilledType);

    // Store the spilled state in a local.
    BCSYM_Variable* pStackStorageLocal = CreateCodeGenTemporary(pSpilledType);
    GenerateStoreLocal(pStackStorageLocal->GetLocalSlot());

    // 2.) Repeatedly pull items out of the tuple and back onto the stack.

    StackRestorationManager restorationManager(this, pStackStorageLocal, spillCount, pReferencingLocation);
    while (!restorationManager.Done())
    {
        restorationManager.RestoreNextItem();
    }

    // 3. Null out the stack storage field.

    // Generate the address information needed for the stack storage location.
    ADDRMODE addrForStackNull;
    GenerateAddr(pSpill->StackStorageReference, &addrForStackNull);

    // Push the item to spill from scratch space onto the stack.
    EmitOpcode(CEE_LDNULL, GetSymbolForFXType(FX::ObjectType));

    // Store the item in the field.
    GenerateStore(pSpill->StackStorageReference, &addrForStackNull);

    if (pStackStorageLocal->IsTemporary())
    {
        m_pCodeGenTemporaryManager->FreeTemporary(pStackStorageLocal);
    }

    VSASSERT(m_stackTypes.Count() == spillCount, "Wrong number of items on the stack!");
    VSASSERT(m_stackTypes.Count() == m_usCurStackLvl, "Stack counts do not match!");
}

void CodeGenerator::RehydrateManagedPointers(Location* pLoc)
{
    VSASSERT(m_stackTypes.Count() == m_usCurStackLvl, "Stack counts do not match!");

    //
    // Spill the stack.
    //

    // 0.) Check that we actually have items to rehydrate. If there's no
    // context, or nothing to do in this context, then just return now.
    if (m_currentHydrationContext == NULL ||
        m_currentHydrationContext->Count() == 0)
    {
        return;
    }

    // 1.) Figure out how many items we need to push into local. We only need
    // to remove items down to the stack level required by the first
    // rehydration step. Otherwise we'll pop stuff off the stack just to push
    // it back on.

    QueueIterator<HydrationStep*, NorlsAllocWrapper> rehydrationIterator(m_currentHydrationContext);
    bool moreItemsToRehydrate = rehydrationIterator.MoveNext();
    unsigned short requiredStackDepth = rehydrationIterator.Current()->StackDepth;

    // 2.) Dump the stack contents out into locals.
    Filo<BCSYM_Variable*, NorlsAllocWrapper> temporaries(m_nraWrapper);
    while (m_stackTypes.Count() > requiredStackDepth)
    {
        temporaries.PushOrEnqueue(CreateCodeGenTemporary(m_stackTypes.Peek()));
        GenerateStoreLocal(temporaries.Peek()->GetLocalSlot());
    }

    //
    // Restore the stack, rehydrating as we go.
    //

    // We're only done when we've restored all the spilled items, and we've
    // executed all the deferred opcodes to rehydrate the managed pointers.
    // Note that there may not be any spilled items to restore even though
    // there are pointers to rehydrate; the set of deferred opcodes may be
    // entirely CEE_LDARG (for parameters that already contain addresses),
    // CEE_LDLOCA (for taking the address of a local), or other instructions
    // that push a value but do not pop one.

    bool moreTemporariesToRestore = temporaries.Count() > 0;
    
    short delta = 0;

    while (moreTemporariesToRestore ||
           moreItemsToRehydrate)
    {
        while (moreTemporariesToRestore &&
               (!moreItemsToRehydrate ||
                (static_cast<unsigned short>(m_stackTypes.Count()) < (rehydrationIterator.Current()->StackDepth + delta))))
        {
            BCSYM_Variable* pTemporary = temporaries.PopOrDequeue();
            GenerateLoadLocal(pTemporary->GetLocalSlot(), pTemporary->GetType());
            m_pCodeGenTemporaryManager->FreeTemporary(pTemporary);

            moreTemporariesToRestore = temporaries.Count() > 0;
        }

        while (moreItemsToRehydrate &&
               m_stackTypes.Count() == (rehydrationIterator.Current()->StackDepth + delta))
        {
            int stackDepthBeforeHydration = m_stackTypes.Count();

            OPCODE opCode = rehydrationIterator.Current()->OpCode;
            int opCodeArg = rehydrationIterator.Current()->OpCodeArg;

            switch (rehydrationIterator.Current()->OpCode)
            {
            // CEE_READONLY serves as metadata on the address-generating
            // instruction that follows it. As such, it doesn't actually modify
            // the instruction stack. See CodeGenerator::GenerateLoadArrayAddr
            // for the one place we emit CEE_READONLY.
            case CEE_READONLY:
                EmitOpcode(opCode, Symbols::GetVoidType());
                break;

            case CEE_LDARG_0:
            case CEE_LDARG_1:
            case CEE_LDARG_2:
            case CEE_LDARG_3:
                EmitOpcode(opCode, UNSPILLABLE_TYPE_SYMBOL);
                break;

            case CEE_LDLOCA_S:
            case CEE_LDARGA_S:
            case CEE_LDARG_S:
                EmitOpcode_U1(opCode, opCodeArg, UNSPILLABLE_TYPE_SYMBOL);
                break;
            
            case CEE_LDLOCA:
            case CEE_LDARGA:
            case CEE_LDARG:
                EmitOpcode_U2(opCode, opCodeArg, UNSPILLABLE_TYPE_SYMBOL);
                break;
        
            case CEE_LDELEMA:
            case CEE_LDFLDA:
            case CEE_LDSFLDA:
                EmitOpcode_Tok(opCode, opCodeArg, UNSPILLABLE_TYPE_SYMBOL);
                break;

            case CEE_CALLVIRT:
                {
                    int itemsToPop = rehydrationIterator.Current()->ItemsToPop;

                    AddToCurStack(-itemsToPop);
                    for (int i = 0; i < itemsToPop; i++)
                    {
                        m_stackTypes.PopOrDequeue();
                    }

                    EmitOpcode_Tok(CEE_CALLVIRT, opCodeArg, UNSPILLABLE_TYPE_SYMBOL);
                    AddToCurStack(1);
                }
                break;

            default:
                VSFAIL("oh ----");
                break;
            }

            int stackDepthAfterHydration = m_stackTypes.Count();

            delta = delta + (stackDepthAfterHydration - stackDepthBeforeHydration);

            moreItemsToRehydrate = rehydrationIterator.MoveNext();
        }
    }

    VSASSERT(m_stackTypes.Count() == m_usCurStackLvl, "Stack counts do not match!");
}

bool CodeGenerator::StackRestorationManager::Done()
{
    return m_itemsRestored == m_itemsToRestore;
}

void CodeGenerator::StackRestorationManager::RestoreNextItem()
{
    VSASSERT(!Done(), "We're done restoring items!");

    // Special case the restoration of a single item.
    if (m_itemsToRestore == 1)
    {
        RestoreOneAndOnlyItem();
        return;
    }

    // m_indexInTuple is set to -1 to indicate that we just unpacked a chained
    // tuple onto the stack. We need to store it in a new temporary.
    if (m_indexInTuple == -1)
    {
        // Create a local as scratch space.
        m_pCurrentScratchSpace = m_pGenerator->CreateCodeGenTemporary(m_pGenerator->m_stackTypes.Peek());

        // Store the tuple in the scratch space, since we'll need to access it
        // multiple times.
        m_pGenerator->GenerateStoreLocal(m_pCurrentScratchSpace->GetLocalSlot());

        m_indexInTuple = 0;
    }

    // Load the tuple onto the stack
    m_pGenerator->GenerateLoadLocal(m_pCurrentScratchSpace->GetLocalSlot(), m_pCurrentScratchSpace->GetType());

    // Bind to the appropriate ItemX property, and then find the get_ItemX method
    BCSYM_GenericBinding* pCurrentTupleType = m_pGenerator->m_stackTypes.Peek()->PGenericBinding();
    Symbols symbolFactory(m_pGenerator->m_pCompiler, m_pGenerator->m_pnra, NULL /* pLineMarkerTable */);
    BCSYM_NamedRoot* pProc = pCurrentTupleType->SimpleBind(&symbolFactory, (STRING_CONST(m_pGenerator->m_pCompiler, TupleItem1 + m_indexInTuple)))->PProperty()->GetProperty();
    mdMemberRef getItemToken = m_pGenerator->m_pmdemit->DefineMemberRefBySymbol(pProc, pCurrentTupleType, m_pLocation);
            
    // For calls, we need to explicitly manage the stack, so pop the
    // call target off of the stack.
    m_pGenerator->m_stackTypes.PopOrDequeue();
    m_pGenerator->EmitOpcode_Tok(CEE_CALLVIRT, getItemToken, pCurrentTupleType->GetArgument(m_indexInTuple));

    // If we just pulled out the last item from the current tuple but we still
    // have more to restore, then its part of the tuple chain. We need to
    // recurse to get the actual next item.
    if (m_indexInTuple == pCurrentTupleType->GetGenericParamCount() - 1 &&
        m_itemsRestored < m_itemsToRestore - 1)
    {
        m_indexInTuple = -1;
        RestoreNextItem();
    }
    else
    {
        m_indexInTuple = m_indexInTuple + 1;
        m_itemsRestored = m_itemsRestored + 1;
    }
}

void CodeGenerator::StackRestorationManager::RestoreOneAndOnlyItem()
{
    // If we're restoring a single item, we just need to load it from the
    // storage location to the stack.

    m_pGenerator->GenerateLoadLocal(m_pCurrentScratchSpace->GetLocalSlot(), m_pCurrentScratchSpace->GetType());
    m_itemsRestored = m_itemsRestored + 1;
}

#if DEBUG
void
CodeGenerator::DumpCall
(
    BCSYM_Proc *pproc
)
{
    if (VSFSWITCH(fDumpCallGraph))
    {
        StringBuffer TextBuffer;
        pproc->GetBasicRep(m_pCompiler, NULL, &TextBuffer);

        STRING *ContainerQualifiedName = pproc->GetContainer()->GetQualifiedEmittedName();
        DebPrintf("    <CALL ID='%S::%S'>\n", ContainerQualifiedName, TextBuffer.GetString());

        unsigned NameCount = m_pCompiler->CountQualifiedNames(ContainerQualifiedName);
        STRING **Names = (STRING **)m_pnra->Alloc(sizeof(STRING *) * NameCount);
        m_pCompiler->SplitQualifiedName(ContainerQualifiedName, NameCount, Names);

#pragma warning(disable:22009)//,"Debug-only code.")
        for (unsigned i = 0; i < NameCount; i++)
        {
            DebPrintf("        <CONTEXT NAME='%S'/>\n", Names[i]);
        }
#pragma warning(default:22009)//,"Debug-only code.")

        DebPrintf("        <METHOD NAME='%S' ACCESS='%S'/>\n", TextBuffer.GetString(), StringOfAccess(m_pCompiler, pproc->GetAccess()));

        DebPrintf("    </CALL>\n");
    }
}

void
CodeGenerator::DumpBeginMethod()
{
    if (VSFSWITCH(fDumpCallGraph))
    {
        StringBuffer TextBuffer;
        m_pproc->GetBasicRep(m_pCompiler, NULL, &TextBuffer);

        STRING *ContainerQualifiedName = m_pproc->GetContainer()->GetQualifiedEmittedName();
        unsigned NameCount = m_pCompiler->CountQualifiedNames(ContainerQualifiedName);
        STRING **Names = (STRING **)m_pnra->Alloc(sizeof(STRING *) * NameCount);
        m_pCompiler->SplitQualifiedName(ContainerQualifiedName, NameCount, Names);

        DebPrintf("<CALLER ID='%S::%S'>\n", ContainerQualifiedName, TextBuffer.GetString());
#pragma warning(disable:22009)//,"Debug-only code.")
        for (unsigned i = 0; i < NameCount; i++)
        {
            DebPrintf("    <CONTEXT NAME='%S'/>\n", Names[i]);
        }
#pragma warning(default:22009)//,"Debug-only code.")

        DebPrintf("    <METHOD NAME='%S' ACCESS='%S'/>\n", TextBuffer.GetString(), StringOfAccess(m_pCompiler, m_pproc->GetAccess()));
    }
}

void
CodeGenerator::DumpEndMethod()
{
    if (VSFSWITCH(fDumpCallGraph))
    {
        DebPrintf("</CALLER>\n");
    }
}
#endif


/* static */
Location CodeGenerator::GetDebugBeginLocation(const ILTree::ProcedureBlock* procblock)
{
    Location loc = Location::GetInvalidLocation();
    BCSYM_Proc* pproc = procblock->pproc;

    if (pproc->IsMethodImpl())
    {
        // Combining ProcBlock and CodeBlock locations gives us the begin Sub location
        // The ProcBlock is covers from Sub Foo to End Sub inclusive
        // The CodeBlock is everything in between
        // If a better SigLocation is available in the tree, use that one.
        if (procblock->SigLocation.IsInvalid())
        {
            loc.SetLocation(
                pproc->PMethodImpl()->GetProcBlock()->m_lBegLine,
                pproc->PMethodImpl()->GetProcBlock()->m_lBegColumn,
                pproc->PMethodImpl()->GetCodeBlock()->m_lBegLine,
                pproc->PMethodImpl()->GetCodeBlock()->m_lBegColumn-1);
        }
        else
        {
            loc.SetLocation(
                procblock->SigLocation.m_lBegLine,
                procblock->SigLocation.m_lBegColumn,
                procblock->SigLocation.m_lEndLine,
                procblock->SigLocation.m_lEndColumn);
        }
    }
    else if (pproc->IsSyntheticMethod())
    {
        BCSYM_SyntheticMethod *proc = pproc->PSyntheticMethod();
        Location WholeSpan = proc->GetWholeSpan();
        Location BodySpan = proc->GetBodySpan();
        if (WholeSpan.IsValid() && BodySpan.IsValid())
        {
            // <4 Sub() <5                     <4..4> WholeSpan
            //    Console.WriteLine("hello")   <5..5> BodySpan
            // 5> End Sub 4>
            // We want to generate a LineTableEntry for just the "Sub()", i.e. <4..<5
            loc.SetLocation(WholeSpan.m_lBegLine, WholeSpan.m_lBegColumn, BodySpan.m_lBegLine, BodySpan.m_lBegColumn);
        }
    }
    // 

    else if (pproc->IsSyntheticMethod() && (pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_FormMain))
    {
        Location *pLocation = pproc->GetContainingClass()->GetLocation();

        loc.SetLocation(pLocation->m_lBegLine,
                        pLocation->m_lBegColumn,
                        pLocation->m_lEndLine,
                        pLocation->m_lEndColumn);
    }

    return loc;
}

/* static */
Location CodeGenerator::GetDebugEndLocation(const ILTree::ProcedureBlock* procblock)
{
    Location loc = Location::GetInvalidLocation();
    BCSYM_Proc* pproc = procblock->pproc;

    if (pproc->IsMethodImpl())
    {
        // Combining ProcBlock and CodeBlock locations gives us the End Sub/Function/Property location.
        // The ProcBlock covers from Sub Foo to End Sub inclusive
        // The CodeBlock is everything in between
        //
        loc.SetLocation(pproc->PMethodImpl()->GetCodeBlock()->m_lEndLine,
                        pproc->PMethodImpl()->GetCodeBlock()->m_lEndColumn,
                        pproc->PMethodImpl()->GetProcBlock()->m_lEndLine,
                        pproc->PMethodImpl()->GetProcBlock()->m_lEndColumn - 1);
    }
    else if (pproc->IsSyntheticMethod())
    {
        BCSYM_SyntheticMethod *proc = pproc->PSyntheticMethod();
        Location WholeSpan = pproc->PSyntheticMethod()->GetWholeSpan();
        Location BodySpan = pproc->PSyntheticMethod()->GetBodySpan();
        // Multiline:
        // <4 Sub() <5                     <4..4> WholeSpan
        //    Console.WriteLine("hello")   <5..5> BodySpan
        // 5> End Sub 4>
        //
        // Singleline:
        // <4 Sub() <5 Console.WriteLine("hello") 5> 4>

        // For multiline, we want to generate a Location for just the "End Sub", i.e. 5>..4>
        // Our dirty trick for distinguishing multiline from singleline is to check that 5> and 4> are
        // separate points. 
        bool isSingleLine = (Location::CompareEndPoints(WholeSpan,BodySpan) == 0);

        if (WholeSpan.IsValid() && BodySpan.IsValid() && !isSingleLine)
        {
            loc.SetLocation(BodySpan.m_lEndLine, BodySpan.m_lEndColumn, WholeSpan.m_lEndLine, WholeSpan.m_lEndColumn);
        }
    }

    return loc;
}
