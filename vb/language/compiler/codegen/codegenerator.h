//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Generates COM+ byte codes from trees. 
//  See CodeGenerator.cpp for file organization info.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// invalid local variable index
#define INVALID_LCL_INDEX   USHRT_MAX

// Sentinel for an unspillable type
#define UNSPILLABLE_TYPE_SYMBOL NULL

// Errors used by the runtime
// NOTE: THESE MUST MATCH THE PREVIOUS VERSIONS
const int E_RESUMEWITHOUTERROR = 0x800a0014; // 20
const int E_INTERNALERROR = 0x800a0033; // 51

//-------------------------------------------------------------------------------------------------
//
// Addressing mode : When dealing with the address of something addressable,
// this specifies what kind of address the thing actually has.
//
enum ADDR_KIND
{
    ADDR_Local,             // A local variable
    ADDR_Array,             // An array element
    ADDR_VirtualField,      // A virtual field
    ADDR_StaticField,       // A static field (no me qualification)
    ADDR_Me,                // The "Me" qualifier
    ADDR_Byref,             // A byref parameter (except ByRef Variant, below)
    ADDR_Byval,             // A byval parameter
};

//-------------------------------------------------------------------------------------------------
//
// What kind of diagnostic to generate if a runtime helper isn't defined
//
enum MISSING_HELPER_ERROR_KIND
{
    Error,
    Ignore
};

//-------------------------------------------------------------------------------------------------
//
// The list of opcodes from opcode.def
//
typedef enum opcode_t
{
#define OPDEF(c,nam,pop,push,args,type,type2,s1,s2,flow)    c,
#include <opcode.def>
#undef OPDEF
  CEE_COUNT,        /* number of instructions and macros pre-defined */
} OPCODE;

// tableswitch indices for On Error Resume and On Error Goto 0
const int DISABLE_HANDLER = 0;
const int RESUME_NEXT = 1;
const int RESUME_NEXT_BASE = -2;

// Specify the largest tuple we want to use for stack spilling. Note that
// Tuple`8 requires another tuple as the last parameter, so it isn't useful for
// spilling.
const int MAX_TUPLE_SIZE_FOR_SPILLING = 7;

//-------------------------------------------------------------------------------------------------
//
// Stores information about the address of something
//
struct ADDRMODE
{
    ADDR_KIND     addrkind;       // Kind of address
    union
    {
        UINT              uIndex;  // The slot for locals and parameters
        mdMemberRef       memref;  // The member for fields
        BCSYM_NamedRoot * pnamed;   // The procedure for addressof
    };
};

struct SWITCH_TABLE;  // Forward

//-------------------------------------------------------------------------------------------------
//
// Represents a single block of code
//
struct CODE_BLOCK : CSingleLink<CODE_BLOCK>
{
    unsigned long     ulBlockOffset;      // block offset
    unsigned char  *  pbCodeBuffer;       // code buffer
    union
    {
        CODE_BLOCK      * pcblkJumpDest;  // jump dest or NULL
        SWITCH_TABLE    * pswTable;       // SWITCH table info, for CEE_SWITCH
        void            * pvData;         // for assignment when type not known
    };
    OPCODE            opcodeJump;         // CEE_NOP, CEE_BR, CEE_SWITCH, etc.
    Vtypes            JumpType;           // branch expression's type - used during final codegen pass
    unsigned short    usCodeSize;         // size in bytes of code buffer
    bool              fLive;              // used during final codegen pass
};

//-------------------------------------------------------------------------------------------------
//
// Represents a block of code at an address
//
struct CODE_ADDRESS
{
    CODE_BLOCK        * pcblk;              // code block
    unsigned int        uOffset;            // offset into code block
};

//-------------------------------------------------------------------------------------------------
//
// A code address in a list of locations for the Resume table
//
struct RESUME_ADDRESS : CSingleLink<RESUME_ADDRESS>
{
    CODE_ADDRESS codeaddr;
};

//-------------------------------------------------------------------------------------------------
//
// Represents a switch block
//
struct SWITCH_TABLE
{
    __int64           LowVal;             // low index for table
    __int64           HiVal;              // high index for table
    unsigned long     cEntries;           // number of entries for table
    CODE_BLOCK      * pcblkCaseElse;      // CASE ELSE code block - NULL if no CASE ELSE
    CODE_BLOCK      * pcblkFallThrough;   // opcodes following SWITCH table
    CODE_ADDRESS      pcodeaddrs[1];      // the table
};

//-------------------------------------------------------------------------------------------------
//
// Represents a catch clause
//    
struct CATCH_BLOCK : CSingleLink<CATCH_BLOCK>
{
    CODE_BLOCK  * pcblkFilter;     // Filter for catch
    BCSYM       * psymType;        // Type for catch
    CODE_BLOCK  * pcblkCatch;      // Actual catch block
    CODE_BLOCK  * pcblkLast;       // Last block of the catch (to figure out length)
    Location      ExceptionVarLocation;   // Location of the exception variable in the catch clause.
};

//-------------------------------------------------------------------------------------------------
//
// Represents an entire try block
//
struct TRY_BLOCK : CSingleLink<TRY_BLOCK>
{
    Location                   EndTryLocation;   // Location of the End Try construct
    Location                   FinallyLocation;  // Location of the "finally" construct
    CSingleList<CATCH_BLOCK>   catchList;
    CODE_BLOCK               * pcblkTry;         // The first block in the try
    CODE_BLOCK               * pcblkFinally;     // The first block in the finally
    CODE_BLOCK               * pcblkFallThrough; // The first block after the entire construct
    bool                       IsSynthetic;      // The Try block was synthesized by Semantics.
};

//-------------------------------------------------------------------------------------------------
//
// Allows us to store the try block in another linked list
//
struct TRY_BLOCK_HOLDER : CSingleLink<TRY_BLOCK_HOLDER>
{
    TRY_BLOCK * ptry;  // A holder for a try block so we can construct another linked list of try block structs
};

//-------------------------------------------------------------------------------------------------
//
// Stores line numbers for debugging information
//
struct LINETBL
{
    long              lSrcLine;          // physical source line
    long              lSrcColumn;        // physical source column
    long              lEndSrcLine;       // physical end source line
    long              lEndSrcColumn;     // physical end source column
    CODE_ADDRESS      codeaddr;

    SourceFile        *pSourceFile;      // physical source file the code is present in
};

struct ScopeMember : public CSingleLink<ScopeMember>
{
    // Use a union here to support Constants
    union {
        LocalVariable *MemberInfo;
        ConstantVariable *ConstantInfo;
    };
    bool fConstant;
};

struct BlockScope : CSingleLink<BlockScope>
{
    CSingleList<ScopeMember> ScopeMembers;
    CSingleList<BlockScope>  ChildScopeList;

    CODE_ADDRESS             OpenAddress;
    CODE_ADDRESS             CloseAddress;
    unsigned long            OpenOffset;
    unsigned long            CloseOffset;
};

struct HydrationStep
{
    HydrationStep(unsigned short depth, OPCODE opCode, int opCodeArg) :
        StackDepth(depth),
        OpCode(opCode),
        OpCodeArg(opCodeArg),
        ItemsToPop(0)
    {
    }

    HydrationStep(unsigned short depth, OPCODE opCode, int opCodeArg, int itemsToPop) :
        StackDepth(depth),
        OpCode(opCode),
        OpCodeArg(opCodeArg),
        ItemsToPop(itemsToPop)
    {
    }

    unsigned short StackDepth;
    OPCODE OpCode;
    int OpCodeArg;
    int ItemsToPop;
};



//-------------------------------------------------------------------------------------------------
//
// IMPORTANT NOTE: There is a SET order of generation for methods in a class.
// You first generate the regular (non-constructor) methods. Then you generate
// the constructor methods. The reason for this is that the methods may contain
// static local variables, which we might need to construct in the constructor.
// Rather than generate all the trees twice, codegen will keep track of any
// static locals that it sees as you generate methods. Then, when you generate
// a constructor (or call GenerateMissingConstructors), we will have the
// complete list of static locals that we might need to construct.
//

class CodeGenerator
{
    friend class CDebugParsedExpression;

public:
    CodeGenerator(
        Compiler *pCompiler,
        _In_ NorlsAllocator *pnra,
        NorlsAllocator *pnraDebugSpace,
        MetaEmit *pmdemit,
        CompilationCaches *pCompilationCaches) :
        m_pnra(pnra),
        m_nraWrapper(m_pnra),
        m_pnraDebugSpace(pnraDebugSpace),
        m_nraLineTable(NORLSLOC),
        m_pCompiler(pCompiler),
        m_pltblLineTbl(NULL),
        m_pcblkAsyncSubCatch(NULL),
        m_ptryOnError(NULL),
        m_pCodeGenTemporaryManager(NULL),
        m_pCompilationCaches(pCompilationCaches),
        m_stackTypes(m_nraWrapper),
        m_currentHydrationContext(NULL),
        m_IsGeneratingAwait(false)
    {
        VSASSERT(pmdemit, "MetaEmit must not be NULL");
        m_pmdemit = pmdemit;
    }

    ~CodeGenerator()
    {
    }

    // Helpers to get the location spans for the beginning and ending lines of
    // a method or lambda.  Valid only on ProcedureBlocks of BCSYM_MethodImpl and
    // BCSYM_SyntheticMethod symbols.  Returns an invalid location if called on
    // blocks of other BCSYM_Proc types.
    static Location CodeGenerator::GetDebugBeginLocation(const ILTree::ProcedureBlock* procblock);
    static Location CodeGenerator::GetDebugEndLocation(const ILTree::ProcedureBlock* procblock);

    //
    // Method generation methods.
    //

    // Creates the IL for a method but does not emit it.
    void GenerateMethod(ILTree::PILNode ptreeFunc);

    void FixupScopeAddress(CODE_ADDRESS &Address);
    void AssignScopeOffsets(BlockScope * CurrentScope, BlockScope * ParentScope);

    // Returns the resulting size of the IL, headers and SEH information
    unsigned GetImageSize();

    // Emits the IL into a block of memory.  The memory is guarenteed
    // to be large enough.  This also emits all debug information
    // and "closes" the method.
    void EmitImage(BYTE *pbImage);

private:
    Compiler                    * m_pCompiler;          // context
    MetaEmit                    * m_pmdemit;            // PE Builder
    NorlsAllocator              * m_pnra;
    NorlsAllocWrapper             m_nraWrapper;         // Wrapper around m_pnra
    NorlsAllocator              * m_pnraDebugSpace;     // used for debug info (method slots, block scopes) which codegen generates but lives longer than codegen

    NorlsAllocator                m_nraLineTable;       // used exclusively for the line table which may grow as
                                                        // entries are added to it
    LINETBL                     * m_pltblLineTbl;       // The line table
    LINETBL                     * m_pltblCurrent;
    LINETBL                     * m_pltblEnd;

    CODE_BLOCK                  * m_pcblkAsyncSubCatch; // Async Sub generated catch block, used to record start IL offset to PDB
    DynamicArray<CODE_ADDRESS>    m_AwaitYieldOffsets;  // IL code offsets of each await operator's return instruction
    DynamicArray<CODE_ADDRESS>    m_AwaitResumeOffsets; // IL code offsets of instruction where each await resumes control

    ILTree::ILNode              * m_ptreeFunc;          // SB_PROC node for current function (note always hold pproc)
    BCSYM_Proc                  * m_pproc;              // function we're generating code for
    CompilerProject             * m_Project;            // project containing the function we're generating code for
    CSingleFunnel<CODE_BLOCK>     m_cblkCodeList;       // list of code blocks - with the option for peeling off codeblocks from the main list
    bool                          m_NoIntChecks;        // do not perform int overflow check.

    CSingleList<RESUME_ADDRESS>   m_cadrResumeList;     // list of resume addresses

    CSingleList<TRY_BLOCK>        m_tryListPending;     // a stack of pending try blocks
                                                        // i.e. one's we're in right now
    CSingleList<TRY_BLOCK>        m_tryList;            // a queue of finished try blocks

    CSingleList<TRY_BLOCK_HOLDER> m_tryholderEncounteredList; // a queue of try blocks in the order we encounter them

    bool m_IsGeneratingAwait;
                                                              // during statement generation

    // Temporary manager to manage the temporaries created during codegen.
    // Note that since these are emitted at the very end of the locals, these need not be
    // tracked during ENC.
    TemporaryManager              *m_pCodeGenTemporaryManager;
    CompilationCaches *m_pCompilationCaches;

    //
    // Member data used as "counters/cursors"
    //

    CODE_BLOCK                  * m_pcblkCurrent;       // current code buffer
    unsigned short                m_usCurStackLvl;      // current stack level
    unsigned short                m_usMaxStackLvl;      // current max stack level - we'll have to write out to the PE image what the max stack slot consumption is

    Filo<BCSYM*, NorlsAllocWrapper> m_stackTypes;       // The types of the items on the evaluation stack
    Queue<HydrationStep*, NorlsAllocWrapper>* m_currentHydrationContext;

    //
    // On Error state management members
    //

    unsigned short                m_usOnErrorLocal;     // current on error local
    unsigned short                m_usResumeLocal;      // current resume local
    unsigned short                m_usCurStmtLocal;     // current resume line local
    unsigned short                m_usCurLineLocal;     // current line number
    CODE_BLOCK                  * m_pcblkOnErrorFilter; // filter of the On Error catch block
    CODE_BLOCK                  * m_pcblkOnErrorHandler;// start of the On Error catch block
    CODE_BLOCK                  * m_pcblkOnErrorFailure;// codeblock immediately following On Error mechanism
    CODE_BLOCK                  * m_pcblkLoadForResume; // codeblock for loading the value for resume statements which the resume switch table uses
    CODE_BLOCK                  * m_pcblkLoadForResumeNext;  // codeblock for loading the value for resume next statements which the resume switch table uses
    CODE_BLOCK                  * m_pcblkResumeTableFallThrough; // codeblock immediately following the Resume table
    SWITCH_TABLE                * m_ptblswOnError;      // current on error switch
    TRY_BLOCK                   * m_ptryOnError;        // describes the main Try block used in On Error mechanism


    CODE_BLOCK                  * m_pcblkEndProcLoadLocal;  // end proc block which loads the Return Value local
    CODE_BLOCK                  * m_pcblkEndProc;           // end proc block for exit function out
                                                            // of a try block or for a common exit point

    unsigned long                 m_cTotalBytes;        // size of the code block, only valid after
                                                        // a call to CalculateBlockAddresses

    BCSYM_Proc                 ** m_RuntimeCallRemapTable;

    //
    // Information used to generate proper locals while in EnC
    //

    MethodSlots                 * m_MethodSlots;   // Locals list so EnC will emit proper order
    BlockScope                  * m_MethodScope;   // The top level local member scope for the method

    //
    // Code buffering logic. The code buffer Write does not check for
    // end of buffer because we'll always call _IsAtEnd to start a
    // new buffer if we're at end. We're using wrappers for the buffering
    // logic so we can easily change its implementation.
    //

    unsigned char               * m_pbBufCur;
    unsigned char               * m_pbBufStart;
    unsigned char               * m_pbBufEnd;

    //
    // Private tables of opcode information
    //

    static const unsigned short   s_OpcodeByrefLoad[];
    static const unsigned short   s_OpcodeByrefStore[];
    static const unsigned short   s_OpcodeArrayLoad[];
    static const unsigned short   s_OpcodeArrayStore[];
    static const signed           s_OpcodeStkEffPush[];
    static const signed           s_OpcodeStkEffPop[];
    static const unsigned char    s_OpcodeLen[];
    static const unsigned char    s_OpcodeCode[];

    static const unsigned short   s_SxopComp[];
    static const unsigned short   s_SxopCompUnsigned[];
    static const unsigned short   s_SxopCompFloat[];
    static const unsigned short   s_SxopCompBranch[];
    static const unsigned short   s_SxopCompBranchUnsigned[];
    static const unsigned short   s_SxopCompReverseBranch[];
    static const unsigned short   s_SxopCompReverseBranchUnsigned[];
    static const unsigned short   s_SxopCompReverseBranchFloat[];

    struct StackRestorationManager
    {
    public:
        StackRestorationManager(CodeGenerator* pGenerator, BCSYM_Variable* pInitialStorage, int itemsToRestore, Location* pLocation) :
            m_pGenerator(pGenerator),
            m_pCurrentScratchSpace(pInitialStorage),
            m_itemsToRestore(itemsToRestore),
            m_itemsRestored(0),
            m_indexInTuple(0),
            m_pLocation(pLocation)
        {
        }

        bool Done();
        void RestoreNextItem();

    private:
        void RestoreOneAndOnlyItem();

    private:
        CodeGenerator* const m_pGenerator;
        const int m_itemsToRestore;
        int m_itemsRestored;
        BCSYM_Variable* m_pCurrentScratchSpace;
        int m_indexInTuple;
        Location* const m_pLocation;
    };

public:
    static RuntimeMembers GetHelperForDecBinOp(BILOP bilop);
    static RuntimeMembers GetHelperForObjBinOp(BILOP bilop);
    static RuntimeMembers GetHelperForStringCoerce(Vtypes vtypeFrom);
    static RuntimeMembers GetHelperFromRef(Vtypes vtypeTo);
    static RuntimeMembers GetHelperFromString(Vtypes vtypeTo);
    static RuntimeMembers GetHelperToDecimal(Vtypes vtypeFrom);
    static RuntimeMembers GetHelperFromDecimal(Vtypes vtypeTo);
    static RuntimeMembers GetHelperForObjRelOp(BILOP bilop, Vtypes vtypeResult);

private:
    //
    // CodeGenerator.cpp functions
    //

    void InitializeGenerator(ILTree::ILNode *ptreeFunc);
    void InitializeDebugInformation();
    void InitializeMethodSlots();
    void InitializeOnError();
    void StartOnErrorTryBlock();

    void AssignBlockLocalSlots(ILTree::ExecutableBlock &exblock, BlockScope *CurrentScope);
    void AssignLocalSlots();
    void AssignParameterSlots();
    void CreateLocalsSignature();

    MethodSlots * GetMethodSlots(BCSYM_Proc * pproc);
    void AddVariableToMethod(BlockScope * CurrentScope, BCSYM_Variable * Variable, unsigned long Flags);
    void AddConstantToMethod(BlockScope * CurrentScope, BCSYM_Variable * Variable);

    BCSYM_Variable* CreateCodeGenTemporary(BCSYM *pType);

    void GenerateReturn(ILTree::PILNode ptreeReturn);
    void GenerateResumeTable();
    void GenerateOnErrorHandler();
    void GenerateEpilogue();
    void GenerateMissingConstructor(BCSYM_Container *pcon);
    void GenerateMissingSharedConstructor(BCSYM_Container *pcon);

    //
    // Utilities.cpp functions
    //

    void AddOpcodeToCurStack(OPCODE opcode);
    void AddToCurStack(signed c);
    void SetAsCurrentCodeBlock(CODE_BLOCK *pcblk);
    void TerminateCodeBuffer(OPCODE uJumpCode, void *pvData, Vtypes JumpType);
    void EndCodeBuffer(OPCODE uJumpCode, void *pvData = NULL);
    void EndCodeBuffer(OPCODE uJumpCode, void *pvData, Vtypes JumpType);
    void LeaveCodeBuffer(CODE_BLOCK * JumpDest, bool ExitsTry, bool ExitsCatch, bool ExitsCatchToCorrespondingTry, bool EmitNOPBeforeHiddenIL = false, bool BeginHiddenIL = true, Location * errorLocation = NULL);
    bool IsLongBranch(OPCODE opcode);
    bool IsShortBranch(OPCODE opcode);
    bool IsConditionalBranch(OPCODE opcode);
    OPCODE MapShort2LongJumpOpcode(OPCODE opcodeShortJump);
    void StartHiddenIL();
    void StartEpilogueIL();
    void UpdateLineTable(Location &Loc, SourceFile *pSourceFile = NULL);
    void UpdateLineTable(ILTree::PILNode ptreeStmt);
    void RecordAwaitYieldCodeAddress();
    void RecordAwaitResumeCodeAddress();
    unsigned short AllocateResumeEntry(CODE_BLOCK *pcblk, unsigned short ResumeIndex);
    void UpdateCurrentStatementLocal(unsigned short ResumeIndex);
    void UpdateResumeTable(unsigned short ResumeIndex, bool fCanThrow = true, CODE_BLOCK * pcblk = NULL);
    void InsertNOP(bool ForceEmit = false);
    void InsertENCRemappablePoint();
    void InsertENCRemappablePoint(BCSYM *TypeOnStack);
    void InsertENCRemappablePoint(CODE_BLOCK **ppcblkTrue, CODE_BLOCK **ppcblkFalse, bool fReverse);
    bool WithinFunction();
    void CollateSwitchTable(CODE_BLOCK * pcblkSwitch);
    void OptimizeGeneratedIL();
    void OptimizeBranches();
    void MarkLiveBlocks(CODE_BLOCK *pcblk);
    unsigned long CalculateBlockAddresses();
    void EmitEHClauses();
    bool ShouldEmitDebugInfoForProc();
    void EmitDebugInformation();
    void EmitByteCodes(BYTE *pbImage);
    MethodDebugInfo *CreateLineTable(CSingleList<MethodDebugInfo> *pMethodDebugInfoList, unsigned long ulNoLines);
    bool AddToLineTable(CSingleList<MethodDebugInfo> *pMethodDebugInfoList, unsigned long ulLine, unsigned long ulColumn,
                        unsigned long ulOffset, unsigned long ulEndLine, unsigned long ulEndColumn, SourceFile *pPhysicalSourceFile);
    AsyncMethodDebugInfo *CreateAsyncMethodDebugInfo(unsigned long ulNoAwaits, BCSYM_Proc *pOriginalProc);
    void LogRuntimeHelperNotFoundError( RuntimeMembers RuntimeHelperMethod, Location *ErrorLocation, MISSING_HELPER_ERROR_KIND messageKind);
    BCSYM* GetSymbolForVtype(Vtypes vtype);
    BCSYM* GetSymbolForFXType(FX::TypeName fxType);
    bool TrySpillAndStoreEvaluationStack(_In_ ILTree::AsyncSpillExpression* pSpill, _Out_opt_ BCSYM** ppSpilledType, _Out_opt_ unsigned int* pSpillCount);
    bool TrySpillEvaluationStackCore(_In_ Location* pLocation, _Out_opt_ BCSYM_Variable** pTemporaryStorage, _Out_opt_ unsigned int* pSpillCount);
    void RestoreEvaluationStack(_In_ ILTree::AsyncSpillExpression* pSpill, _In_ BCSYM* pSpilledType, _In_ unsigned int spillCount);
    bool StackContainsRestrictedTypes(_In_ Location* pLocation);
    bool StackContainsUnspillableTypes();
    void RehydrateManagedPointers(_In_ Location* pLocation);

    //
    // CodeGenerator.inl functions
    //

    Vtypes ActualVmType(Vtypes vtype);
    OPCODE MapType2Opcode_ByrefLoad(Vtypes vtype);
    OPCODE MapType2Opcode_ByrefStore(Vtypes vtype);
    OPCODE MapType2Opcode_ArrayLoad(Vtypes vtype);
    OPCODE MapType2Opcode_ArrayStore(Vtypes vtype);
    OPCODE MapSxop2Comp(BILOP sxop);
    OPCODE MapSxop2CompUnsigned(BILOP sxop);
    OPCODE MapSxop2CompFloat(BILOP sxop);
    OPCODE MapSxop2CompBranch(BILOP sxop);
    OPCODE MapSxop2CompBranchUnsigned(BILOP sxop);
    OPCODE MapSxop2CompReverseBranch(BILOP sxop);
    OPCODE MapSxop2CompReverseBranchUnsigned(BILOP sxop);
    OPCODE MapSxop2CompReverseBranchFloat(BILOP sxop);
    unsigned short OpcodeRefEncoding(OPCODE opcode);
    SWITCH_TABLE *AllocSwitchTable(unsigned long cEntries);
    unsigned long CodeAddress(_In_ CODE_ADDRESS * pcodeaddr);
    CODE_BLOCK *NewCodeBlock();
    unsigned IsComplexValueType(Vtypes vtype);
    unsigned IsSimpleArray(ILTree::ILNode * ptreeIndex);
    bool InsideValidResumeContext();
    bool NeedLineNumber();
    void CodeBuf_Create();
    void CodeBuf_Create(BYTE * pbBuf, unsigned long cbBytes);
    unsigned CodeBuf_IsAtEnd();
    unsigned CodeBuf_IsNull();
    void CodeBuf_Reset();
    unsigned CodeBuf_IsEnoughSize(unsigned cbLen);
    unsigned CodeBuf_GetSize();
    void CodeBuf_Write08(char ch);
    void CodeBuf_Write16(int val);
    void CodeBuf_Write32(int val);
    void CodeBuf_Write64(__int64 val);
    BYTE *CodeBuf_GetBuffer();
    void CodeBuf_WriteData(_In_bytecount_(cbLen) const char * pbBuf, unsigned long cbLen);
    void CodeBuf_WritePcode(OPCODE opcode);
    unsigned CBytesOpcode(OPCODE opcode);
    unsigned CBytesBranchOpcodeOperand(OPCODE opcode);
    void EmitInt08(int ch);
    void EmitInt16(int val);
    void EmitInt32(int val);
    void EmitInt64(__int64 val);
    void EmitTok(int val);
    void EmitData(_In_bytecount_(cb) char * pb, int cb);
    void EmitOpcode(OPCODE pc, BCSYM* pTypePushedOnStack);
    void EmitOpcode_U1(OPCODE pc, int val, BCSYM* pTypePushedOnStack);
    void EmitOpcode_U2(OPCODE pc, int val, BCSYM* pTypePushedOnStack);
    void EmitOpcode_U4(OPCODE pc, long val, BCSYM* pTypePushedOnStack);
    void EmitOpcode_U8(OPCODE pc, __int64 val, BCSYM* pTypePushedOnStack);
    void EmitOpcode_Tok(OPCODE pc, long val, BCSYM* pTypePushedOnStack);
    #define EmitOpcode_I1(pc,val,type) EmitOpcode_U1(pc,val,type)
    #define EmitOpcode_I2(pc,val,type) EmitOpcode_U2(pc,val,type)
    #define EmitOpcode_I4(pc,val,type) EmitOpcode_U4(pc,val,type)
    #define EmitOpcode_I8(pc,val,type) EmitOpcode_U8(pc,val,type)

    //
    // Statements.cpp
    //

    void GenerateBlockStatementList(ILTree::ExecutableBlock &exblock);
    void GenerateStatementList(ILTree::PILNode ptree);
    void GenerateForEachLoop(ILTree::PILNode ptree);
    void GenerateLoop(ILTree::LoopBlock *ptree);
    void GenerateElse(ILTree::PILNode ptree);
    void GenerateLabel(ILTree::PILNode ptree);
    void GenerateExitSelect(ILTree::PILNode ptree);
    void GenerateExitLoop(ILTree::PILNode ptree);
    void GenerateContinue(ILTree::PILNode ptree);
    void GeneratePrint(ILTree::PILNode ptree);
    void GenerateForLoopStart(ILTree::PILNode ptree);
    void GenerateForLoopEnd( ILTree::PILNode  ptree );
    bool SkipInitialForLoopCheck(ILTree::PILNode ptree);
    void GenerateForLoop(ILTree::PILNode ptreeFor);
    void GenerateGotoLabel(LABEL_ENTRY_BIL *pLabel, bool ExitsTry, bool ExitsCatch, bool ExitsCatchToCorrespondingTry);
    void GenerateAsyncSwitch(ILTree::PILNode ptree);
    void GenerateOnError(ILTree::PILNode ptree);
    void GenerateTry(ILTree::PILNode ptree);
    void GenerateCatchFilter(ILTree::CatchBlock *ptreeCatch);
    void GenerateCatch(ILTree::CatchBlock *ptreeCatch);
    void GenerateFinally(ILTree::PILNode ptree);
    void GenerateSelect(ILTree::PILNode ptree);
    void GenerateEndSelect(ILTree::PILNode ptree);
    void GenerateCase(ILTree::PILNode ptree);
    void GenerateResume(ILTree::PILNode ptree);
    void GenerateObjectForLoopStart(ILTree::PILNode ptree);
    void GenerateObjectForLoopEnd(ILTree::PILNode ptree);

    //
    // Expressions.cpp
    //

    void GetDefCtorRef(BCSYM *pmod, mdMemberRef *pmemref, Location *pReferencingLocation);
    void GenerateRvalue(ILTree::PILNode ptree);
    void GenerateAddr(ILTree::PILNode ptree, ADDRMODE *paddr, bool fReadOnlyAddr = false);
    void GenerateDupLvalue(ILTree::PILNode ptreeLHS, ADDRMODE *paddr);
    void GenerateStoreType(BCSYM *ptyp, ADDRMODE *paddrmode, Location *pReferencingLocation);
    void GenerateStore(ILTree::PILNode ptreeLHS, ADDRMODE *paddrmode);
    void GenerateLoad(ILTree::PILNode ptreeLHS, ADDRMODE *paddrmode);
    void GenerateLoadAddr(ILTree::PILNode ptreeLHS, ADDRMODE *paddrmode, bool fReadOnlyAddr = false);
    void GenerateLiteralInt(__int32 Value);
    void GenerateLiteralInt(__int64 Value, bool Need64bitResult);
    void GenerateLiteralDate(ILTree::PILNode ptree);
    void GenerateLiteralFp(double dblVal, Vtypes vtype);
    void GenerateLiteralDec(ILTree::PILNode ptree);
    void GenerateZeroConst(Vtypes vtype);
    void GenerateCall(ILTree::PILNode ptree);
    void GenerateCallArg(ILTree::PILNode ptreeList, signed *psStack);
    void GenerateRTField(RuntimeMembers rtHelper, Vtypes vtype);
    void GenerateCallToRuntimeHelper(RuntimeMembers HelperToCall,
                                     BCSYM* pResultType,
                                     Location *Loc,
                                     MISSING_HELPER_ERROR_KIND errorKind = Error,
                                     BCSYM_GenericBinding *pGenericBindingContext = NULL);
    void GenerateLoadLocal(unsigned uIndex, BCSYM* pType);
    void GenerateLoadParam(unsigned uIndex, BCSYM* pType);
    void GenerateLoadLocalAddr(unsigned uIndex);
    void GenerateLoadParamAddr(unsigned uIndex);
    void GenerateLoadArrayAddr(ILTree::PILNode ptreeLHS, bool fReadOnlyAddr);
    void GenerateLoadArrayAddrInternal(ILTree::PILNode ptreeLHS, bool fReadOnlyAddr);
    void GenerateLoadArray(ILTree::PILNode ptreeLHS);
    void GenerateLoadStructFromAddr(BCSYM * ptyp, Location *pReferencingLocation);
    void GenerateLoadByRef(BCSYM * ptyp, Location *pReferencingLocation);
    void GenerateLoadOrElseLoadLocal(_In_opt_ ILTree::SymbolReferenceExpression *symref, _In_ BCSYM_Variable *local);
    void GenerateStoreOrElseStoreLocal(_In_opt_ ILTree::SymbolReferenceExpression *symref, _In_ BCSYM_Variable *local);
    void GenerateStoreLocal(unsigned uIndex);
    void GenerateStoreParam(unsigned uIndex);
    void GenerateStoreComplexValueType(BCSYM * ptyp, Location *pReferencingLocation);
    void GenerateStoreByRef(BCSYM * ptyp, Location *pReferencingLocation);
    void GenerateStoreArray(ILTree::PILNode ptreeLHS, Location *pReferencingLocation);
    void GenerateConditionalBranch(ILTree::PILNode ptreeCond, bool fReverse, CODE_BLOCK *pcblkTrue, CODE_BLOCK *pcblkFalse);
    void GenerateCondition(ILTree::PILNode ptreeCond, bool fReverse, CODE_BLOCK *pcblkTrue, CODE_BLOCK *pcblkFalse);
    void GenerateConditionWithENCRemapPoint(ILTree::PILNode ptreeCond, bool fReverse, CODE_BLOCK **ppcblkTrue, CODE_BLOCK **ppcblkFalse);
    void GenerateIndex(ILTree::PILNode ptreeIndex, ADDRMODE *paddrmode);
    void GenerateConvertSimpleType(Vtypes SourceType, Vtypes ResultType, bool PerformOverflowChecking, ILTree::PILNode tree);
    void GenerateConvertIntrinsic(ILTree::PILNode ptreeTo, ILTree::PILNode ptreeFrom, Vtypes vtypeTo, Vtypes vtypeFrom);
    void GenerateConvertClass(ILTree::PILNode ptreeTo, ILTree::PILNode ptreeFrom, Vtypes vtypeTo, Vtypes vtypeFrom);
    void GenerateConvert(ILTree::BinaryExpression *ptree);
    void GenerateDirectCast(ILTree::BinaryExpression *ptree);
    void GenerateTryCast(ILTree::BinaryExpression *Target);
    void GenerateLogicalOr(ILTree::PILNode ptree, bool fReverse, CODE_BLOCK *pcblkTrue, CODE_BLOCK *pcblkFalse);
    void GenerateLogicalAnd(ILTree::PILNode ptree, bool fReverse, CODE_BLOCK *pcblkTrue, CODE_BLOCK *pcblkFalse);
    void GenerateObjBinOp(ILTree::PILNode ptree);
    void GenerateDecBinOp(ILTree::PILNode ptree);
    void GenerateRelOp(ILTree::PILNode ptree);
    void GenerateRelOpBranch(ILTree::PILNode ptree, bool fReverse, CODE_BLOCK *pcblkTrue, CODE_BLOCK *pcblkFalse);
    void GenerateBinOp(ILTree::PILNode ptree);
    void GenerateLateIndex(ILTree::PILNode ptree);
    void GenerateRedim(ILTree::PILNode ptree);
    void GenerateArrayConstruction(BCSYM_ArrayType * parray, ILTree::PILNode pSizeList, Location *pReferencingLocation);
    void GenerateTypeObject(BCSYM *ptyp, Location *pLoc);
    void GenerateMemberObject(ILTree::PILNode Tree);
    void GenerateMetaType(ILTree::PILNode ptree);
    void GenerateMathOp(BILOP op, Vtypes vtype, ILTree::PILNode pTree);
    void GenerateShift(ILTree::PILNode ptree);
    void GenerateRTException(HRESULT hrErr, Location *pLoc);
    void GenerateLike(ILTree::PILNode ptree);
    void GenerateClone(ILTree::PILNode ptree);
    void GenerateAssignment(ILTree::PILNode ptree);
    void GenerateCastClass(BCSYM *ptypTo, Location *pReferencingLocation);
    void GenerateLate(ILTree::PILNode ptree);
    void GenerateIsType(ILTree::PILNode ptree);
    void GenerateNew(BCSYM *pmod, ILTree::PILNode ptree, Location *Location);
    void GenerateDelegateConstructorArgs(ILTree::PILNode ptree, signed *SubStack);
    void GenerateInitStructure(BCSYM *StructureType, ILTree::PILNode StructureReference);
    void GenerateLiteralStr(_In_opt_count_(cch) const WCHAR *wsz, size_t cch);
    void GenerateObjectOr(ILTree::PILNode ptree, CODE_BLOCK *pcblkDoneWithExpression, bool fLastTerm);
    void GenerateObjectAnd(ILTree::PILNode ptree, CODE_BLOCK *pcblkDoneWithExpression, bool fLastTerm);
    void GenerateObjectShortCircuitExpression(ILTree::PILNode ptree);
    void GenerateBoxedBoolean(bool Value);
    void GenerateRTSetErrorMember(Location *pLoc, MISSING_HELPER_ERROR_KIND errorKind);
    void GenerateAsyncSpill(_In_ ILTree::AsyncSpillExpression* pSpill);

#if DEBUG
    void DumpCall(BCSYM_Proc *pproc);
    void DumpBeginMethod();
    void DumpEndMethod();
#endif
};

#include "CodeGenerator.inl"
