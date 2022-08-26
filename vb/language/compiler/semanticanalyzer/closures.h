//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  See Closures.cpp for a description of the Closures algorithm.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class GenericContext;
class ClosureBase;
class Closure;
class OptimizedClosure;
class ClosureVisitor;
class ClosureShortLivedTemporaryVisitor;
class MyBaseMyClassStubContext;

struct TrackedExpression
{
    TrackedExpression(ILTree::Expression **expr, ILTree::ExecutableBlock *block, ILTree::Statement *stmt, Procedure *proc)
        :m_ppExpr(expr), m_proc(proc), m_block(block), m_stmt(stmt)
    {
    }

    // The block where the symbol referenced occurred
    ILTree::ExecutableBlock *GetBlock() const { return m_block; }

    // The block can change as the biltree is walked.  Right now this only happens when
    // the expression occurs inside a statement which ends up lifting a ShortLived
    // Temporary
    void SetBlock(ILTree::ExecutableBlock *block) { m_block = block; }
    ILTree::Expression *GetExpression() const { return *m_ppExpr; }
    ILTree::Expression **GetRawExpression() const { return m_ppExpr;}
    Procedure *GetReferencingProcedure() const { return m_proc;}
    ILTree::Statement *GetStatement() const { return m_stmt;}

private:

    // Do not auto generate
    TrackedExpression();
    TrackedExpression(const TrackedExpression&);
    TrackedExpression &operator=(const TrackedExpression);

    ILTree::Expression **m_ppExpr;
    Procedure *m_proc;
    ILTree::Statement *m_stmt;
    ILTree::ExecutableBlock *m_block;
};


//==============================================================================
// Information about a lambda
//==============================================================================

enum LambdaType
{
    LambdaType_Normal,
    LambdaType_Optimized,
    LambdaType_ExpressionTree,
    LambdaType_Unknown,
};

class LambdaData
{
public:
    LambdaData(_In_ TrackedExpression *tracked, Procedure *proc, Procedure *parentProc)
        : m_tracked(tracked),
        m_closure(NULL),
        m_proc(proc),
        m_parentProc(parentProc),
        m_usesLiftedVariable(false),
        m_usesMe(false),
        m_hasClosureVar(false),
        m_type(LambdaType_Unknown)
    {
        ThrowIfNull(tracked);
        ThrowIfFalse(SX_LAMBDA == tracked->GetExpression()->bilop);
        ThrowIfNull(parentProc);
    }

    ILTree::LambdaExpression *GetLambda() const { return &m_tracked->GetExpression()->AsLambdaExpression(); }
    TrackedExpression *GetTrackedExpression() const { return m_tracked; }
    Procedure *GetProcedure() const { return m_proc; }
    ClosureBase *GetClosure() const { return m_closure; }
    void SetClosure(ClosureBase *value) { m_closure = value; }
    Procedure *GetParentProcedure() const { return m_parentProc; }
    void SetParentProcedure(Procedure *parent) { m_parentProc = parent; }
    bool UsesLiftedVariable() const { return m_usesLiftedVariable; }
    void SetUsesLiftedVariable(bool value) { m_usesLiftedVariable = value; }
    bool UsesMe() const { return m_usesMe; }
    void SetUsesMe(bool value ) { m_usesMe = value;}
    bool HasClosureVariable() const { return m_hasClosureVar; }
    void SetHasClosureVariable(bool value ) { m_hasClosureVar = true; }
    LambdaType GetLambdaType() const { return m_type; }
    void SetLambdaType(LambdaType type) { m_type = type; }

private:
    // Do not auto generate
    LambdaData();
    LambdaData(const LambdaData&);
    LambdaData &operator=(const LambdaData&);

    TrackedExpression *m_tracked;
    ClosureBase *m_closure;     // Closure that owns the Lambda
    Procedure *m_proc;
    Procedure *m_parentProc;
    bool m_usesLiftedVariable;
    bool m_usesMe;
    bool m_hasClosureVar;
    LambdaType m_type;
};

//==============================================================================
// Stores data about ShortLivedTemporaries
//==============================================================================
class ShortLivedTemporaryData
{
public:
    ShortLivedTemporaryData(_In_ Temporary *info, NorlsAllocWrapper alloc)
        : m_tempInfo(info),
        m_liftedStatementList(alloc)
    {
        ThrowIfFalse(LifetimeShortLived == info->Lifetime);
    }
    ~ShortLivedTemporaryData() {}

    Temporary *GetTemporaryInfo() const { return m_tempInfo; }
    void AddStatementThatLifts(ILTree::Statement *stmt) { m_liftedStatementList.AddLast(stmt); }
    bool IsLiftedInStatement(ILTree::Statement *stmt) const { return m_liftedStatementList.Contains(stmt); }
    ListValueIterator<ILTree::Statement*,NorlsAllocWrapper> GetStatementIterator() { return ListValueIterator<ILTree::Statement*,NorlsAllocWrapper>(&m_liftedStatementList); }

private:

    // Do not auto generate
    ShortLivedTemporaryData();
    ShortLivedTemporaryData(const ShortLivedTemporaryData&);
    ShortLivedTemporaryData &operator=(const ShortLivedTemporaryData&);

    Temporary *m_tempInfo;
    List<ILTree::Statement*,NorlsAllocWrapper> m_liftedStatementList;
};

class BlockData
{
public:
    BlockData(ILTree::ExecutableBlock *block, Scope *locals, TemporaryManager *tempManager)
        : m_block(block), m_locals(locals), m_tempManager(tempManager)
    {
    }
    ~BlockData() { }

    ILTree::ExecutableBlock *GetBlock() const { return m_block; }
    Scope *GetLocals() const { return m_locals; }
    TemporaryManager *GetTemporaryManager() const { return m_tempManager; }

private:
    ILTree::ExecutableBlock *m_block;
    Scope *m_locals;
    TemporaryManager *m_tempManager;
};

//==============================================================================
// Data structure representing SB_HIDDEN_CODE_BLOCK nodes we have yet to insert
//==============================================================================
class EmptyBlockData
{
public:
    EmptyBlockData(ILTree::ExecutableBlock *block, ILTree::Statement *stmt)
        :m_emptyBlock(block), m_block(NULL), m_stmt(stmt), m_isNeeded(false), m_isForBlock(false)
    {
    }
    EmptyBlockData(ILTree::ExecutableBlock *emptyBlock, ILTree::ExecutableBlock *block)
        :m_emptyBlock(emptyBlock), m_block(block), m_stmt(NULL), m_isNeeded(false), m_isForBlock(true)
    {
    }

    ILTree::ExecutableBlock *GetEmptyBlock() const { return m_emptyBlock; }
    ILTree::Statement *GetStatement() const { ThrowIfTrue(IsForBlock()); return m_stmt; }
    ILTree::ExecutableBlock *GetTargetBlock() const { ThrowIfFalse(IsForBlock()); return m_block; }
    unsigned GetGroupId() const { return IsForBlock() ? 0 : m_stmt->GroupId;}
    bool IsNeeded() const { return m_isNeeded; }
    void SetIsNeeded() { m_isNeeded = true; }
    bool IsForBlock() const { return m_isForBlock; }
    bool IsForGroup() const { return !m_isForBlock; }

private:
    EmptyBlockData();
    EmptyBlockData(const EmptyBlockData&);
    EmptyBlockData &operator=(const EmptyBlockData);

    ILTree::ExecutableBlock *m_emptyBlock;
    ILTree::ExecutableBlock *m_block;
    ILTree::Statement *m_stmt;
    bool m_isNeeded;
    bool m_isForBlock;
};

enum VariableFlags
{
    CL_None = 0,    // No flags
    CL_Catch        = 0x0001,   // Variable is the catch variable
    CL_ClosureVar   = 0x0002,   // Variable is a closure variable
};


class ClosureNameMangler
{
public:
    static bool IsClosureClass(BCSYM_NamedRoot *);
    static bool IsStateMachine(BCSYM_NamedRoot *);
    static bool IsMoveNextProcedureOfStateMachine(BCSYM_NamedRoot *);
    static bool IsClosureVariable(BCSYM_NamedRoot *);
    static bool IsLambda(BCSYM_NamedRoot *);
    static bool IsLiftedLocal(BCSYM_NamedRoot *);
    static bool IsLiftedNonLocal(BCSYM_NamedRoot *);
    static bool IsLiftedMe(BCSYM_NamedRoot *);
    static bool IsMyBaseMyClassStub(BCSYM_NamedRoot *);
    static STRING *EncodeClosureVariableName(Compiler*, const WCHAR*, unsigned);
    static STRING *EncodeLocalVariableName(Compiler*, const WCHAR*);
    static STRING *EncodeResumableVariableName(Compiler*, const WCHAR*);
    static STRING *EncodeNonLocalVariableName(Compiler*, const WCHAR*);
    static STRING *EncodeMeName(Compiler*);
    static STRING *DecodeVariableName(Compiler*, const WCHAR*);
    static STRING *DecodeResumableLocalOriginalName(Compiler *compiler, const WCHAR *decodedName);
    static STRING *DecodeStateMachineProcedureName(BCSYM_NamedRoot*, Compiler*);
    static STRING *EncodeGenericParameterName(Compiler*, GenericParameter*);
    static STRING *EncodeMyBaseMyClassStubName(Compiler*, BCSYM_Proc*,unsigned);
    static bool DecodeClosureClassLocation(const WCHAR*, unsigned*);
    static bool DecodeLambdaLocation(const WCHAR*,unsigned*);
    static bool DecodeStateMachineLocation(const WCHAR*,unsigned*);
    static bool DecodeGenericParameterName(const WCHAR*, unsigned*);
    static bool DecodeMyBaseMyClassStubName(const WCHAR*, long*, long*);
};

#ifdef DEBUG

//==============================================================================
// Used for debugging of closures by dumping the information onto the console
//==============================================================================
class ClosureDump
{
public:
    ClosureDump() : m_indent(0) { }
    ~ClosureDump() {}
    void DumpClosures(ClosureRoot*);
    void DumpClosure(Closure*);
    void DumpFields(Closure*);
    void DumpLambdas(Closure*);
    void DumpChildren(Closure*);
    void DumpGenericMap(Closure*);
    void DumpLambda(LambdaData*);
    void Print(const WCHAR*, ...);
    void PrintAndIndent(const WCHAR*, ...);
    void PrintTypeName(BCSYM*);
    void PrintSignature(Procedure*);
    void PrintLocation(Location);
private:
    void IncreaseIndent() { ++m_indent; }
    void DecreaseIndent() { --m_indent; }

    int m_indent;
};

#endif



struct LocalScopeInjection
{
    BlockData* blockInStack;
    BlockData* localScopeToInject;
    LocalScopeInjection * previousInjection;

    LocalScopeInjection()
    {
        blockInStack = NULL;
        localScopeToInject = NULL;
        previousInjection = NULL;
    }
};
    


//==============================================================================
// The ClosureRoot goes through several states.  Certain actions are only
// valid after a specific state occurred so this enum is used to track the
// state and give meaniningful asserts where valid
//
// Whenever this enum changes, examine all calls to
//
//   ClosureRoot::EnsureState()
//   ClosureRoot::EnsurePastState()
//
// Make sure they are still respected
//==============================================================================
enum ClosureRootState
{
    // No state, between calls to the constructor and when we are actually
    // doing anything
    CRState_None,

    // Used to distinguish when the ClosureRoot is iterating the Biltree during
    // it's initial information gathering phase.
    CRState_Iterating,
    CRState_PostIterating,

    // Distinguishing when EmptyBlocks are being inserted into the tree
    CRState_EmptyBlockFixup,
    CRState_PostEmptyBlockFixup,

    // Distinguishing when ShortLived temporaries are being fixed up in the Biltree
    CRState_ShortLivedFixup,
    CRState_PostShortLivedFixup,

    // Distinguishing the phase where we are fixing up the MyBase/MyClass calls
    // to use stubs so that it can be lifted properly
    CRState_MyBaseMyClassFixup,
    CRState_PostMyBaseMyClassFixup,

    // Valid when the Biltree enters the well formed state.  This occurs after
    // the initial Biltree is fixed up with all of the closure specific changes
    // but before it is split apart into lambdas and symbols are fixed up
    CRState_BiltreeWellFormed,

    // Distinguishing when lambdas are being assigned to their owning Closures
    CRState_ParentingLambdas,
    CRState_PostParentingLambdas,

    // Distinguishing when Closures are being built into a tree structure
    CRState_ClosureTreeBuilding,
    CRState_PostClosureTreeBuilding,

    // Distinguishing when symbols are being fixed up.
    CRState_SymbolFixup,
    CRState_PostSymbolFixup,

    // Distinguishing when generating Initialization Code
    CRState_InitCode,
    CRState_PostInitCode,

    // Distinguishing when fixing up the Goto references
    CRState_GotoFixup,
    CRState_PostGotoFixup,

    // Valid when the ClosureRoot is finished
    CRState_Finished,
};

//==============================================================================
// Root structure maintaining list of Closures for a procedure.  It's not possible
// to just use the closure structure because it's possible for a procedure to have
// two top level closures that are not related.
//
// This also gives us a good place for quick lookup
//==============================================================================
class ClosureRoot : public BoundTreeVisitor 
{
    friend GenericContext;
    friend ClosureBase;
    friend Closure;
    friend MyBaseMyClassStubContext;
    friend OptimizedClosure;
    friend class ClosureGenericVisitor;
    friend ClosureShortLivedTemporaryVisitor;
#ifdef DEBUG
    friend class ClosureDump;
#endif

    // Helpful typedefs
public:
    NEW_CTOR_SAFE()

    typedef HashTableValueIterator<Procedure*,LambdaData*,NorlsAllocWrapper> RealLambdaDataIterator;
    typedef HashTableValueConstIterator<Procedure*,LambdaData*,NorlsAllocWrapper> RealLambdaDataConstIterator;
    typedef HashTableValueIterator<Variable*,ShortLivedTemporaryData*,NorlsAllocWrapper> ShortLivedTemporaryDataIterator;
    typedef HashTableValueConstIterator<Variable*,ShortLivedTemporaryData*,NorlsAllocWrapper> ShortLivedTemporaryDataConstIterator;

private:

    enum ClosureFrameType
    {
        FrameTypeNone,              // Used for asserting error conditions
        FrameTypeLambda,
        FrameTypeExpressionTree,
    };

    class ClosureFrame
    {
    public:
        // Immediate outer block where the frame was created
        ILTree::ExecutableBlock *GetBlock() const { return m_block; }
        ClosureFrameType GetClosureFrameType() const { return m_type; }
        LambdaData *GetLambdaData() const { return m_data; }

        ClosureFrame(ClosureFrameType frameType, ILTree::ExecutableBlock *block, LambdaData *data)
            : m_type(frameType), m_block(block), m_data(data)
        {
            ThrowIfTrue(frameType == FrameTypeNone);
            ThrowIfNull(block);
        }

    private:
        // Do not autogenerate
        ClosureFrame();
        ClosureFrame(const ClosureFrame&);
        ClosureFrame &operator=(const ClosureFrame&);

        ClosureFrameType m_type;
        ILTree::ExecutableBlock *m_block;
        LambdaData *m_data;
    };

public:

    ClosureRoot(Procedure *proc, ILTree::ProcedureBlock *body, Semantics *semantics, Symbols &transientSymbolsFactory);
    ~ClosureRoot();

    void Iterate();
    void CreateClosureCode();

    Procedure *GetCurrentReferencingProcedure() const;
    const Closure *GetClosure(ILTree::ExecutableBlock*) const;
    Closure *GetClosure(ILTree::ExecutableBlock*) ;
    const Closure *GetClosure(Procedure*) const;
    Closure *GetClosure(Procedure*) ;
    Closure *GetOrCreateClosure(ILTree::ExecutableBlock *block);
    ILTree::ProcedureBlock *GetProcedureBody(Procedure*) const;
    RealLambdaDataIterator GetRealLambdaDataIterator() { return RealLambdaDataIterator(&m_realLambdaMap); }
    RealLambdaDataConstIterator GetRealLambdaDataIterator() const { return RealLambdaDataConstIterator(&m_realLambdaMap); }
    ShortLivedTemporaryDataIterator GetShortLivedTemporaryDataIterator() { return ShortLivedTemporaryDataIterator(&m_shortLivedMap); }
    ShortLivedTemporaryDataConstIterator GetShortLivedTemporaryDataIterator() const { return ShortLivedTemporaryDataConstIterator(&m_shortLivedMap); }
    LambdaData *GetLambdaData(ILTree::Expression **) const;

    bool InClosure() const { return m_frameStack.Count() > 0; }
    bool InExpressionTree() const { return InClosure() && FrameTypeExpressionTree == m_frameStack.Peek()->GetClosureFrameType(); }

    Compiler *GetCompiler() const { return m_pCompiler; }
    NorlsAllocator *GetNorlsAllocator() const { return const_cast<NorlsAllocator*>(&m_norls); }
    Semantics* GetSemantics() const { return m_semantics; }
    Symbols *GetTransientSymbolFactory() const { return &m_transientSymbolFactory; }
    Symbols *GetNormalSymbolFactory() const { return &m_semantics->m_SymbolCreator; }
    TransientSymbolStore *GetTransientSymbolStore() const { return m_semantics->m_SymbolsCreatedDuringInterpretation; }

    BCSYM *GetEquivalentType(BCSYM *, Procedure*);
    ILTree::SymbolReferenceExpression *CreateReferenceToContainingClassMeInProcedure(Procedure*);
    ILTree::SymbolReferenceExpression *CreateReferenceToContainingClassMe();
    void CreateLocalsHash(Procedure*, ILTree::ExecutableBlock*);
protected:

    // Overridden from BoundTreeVisitor
    virtual bool StartBlock(ILTree::ExecutableBlock*);
    virtual void EndBlock(ILTree::ExecutableBlock*);
    virtual bool StartStatement(ILTree::Statement *);
    virtual void EndStatement(ILTree::Statement *);
    virtual bool StartExpression(ILTree::Expression **);
    virtual void EndExpression(ILTree::Expression **);
    
    virtual void VisitFor(ILTree::ForBlock*);
    virtual void VisitForEach(ILTree::ForEachBlock*);
    virtual void VisitLoop ( ILTree::LoopBlock *);
    virtual void VisitIf ( ILTree::IfBlock *);
    virtual void VisitWith ( ILTree::WithBlock *);
    virtual void VisitCase ( ILTree::CaseBlock *);

private:

    void PushClosureFrame(ILTree::Expression**);
    void PopClosureFrame();
    void PushBlock(ILTree::ExecutableBlock*,Scope*,TemporaryManager*);
    void PushBlock(BlockData *);
    void PushBlock(LocalScopeInjection &);
    BlockData* PopBlock();
    void PopBlock(LocalScopeInjection &);
    void LiftVariable(Variable *, ILTree::SymbolReferenceExpression *);
    ILTree::ExecutableBlock *GetCurrentBlock() const;

    void ChangeState(ClosureRootState);
    void EnsureInState(ClosureRootState) const;
    void EnsurePastState(ClosureRootState) const;
    void EnsurePastOrInState(ClosureRootState) const;
    void EnsureBeforeState(ClosureRootState) const;

public:
    void FixupMyBaseMyClassCalls(_In_opt_ List<ILTree::Expression*, NorlsAllocWrapper> *fixupThese = NULL);
private:
    void FixupEmptyBlocks();
    void FixupEmptyBlock(EmptyBlockData*);
    void FixupShortLivedTemporaries();
    void FixupClosureTreeStructure();
    void FixupLambdaReferences();
    void FixupExpressionTrees();
    void CreateClosureClassCode();
    void FixupSymbolReferences();
    void FixupGotos();
    void CreateInitCode();
    bool InGeneric() const;
    bool IsLocalParamOrMe(Symbol*);
    bool IsLocalOrParamFromOutsideFrame(Variable*);

    static ILTree::Statement *FindOwningStatementOfShortLivedTemporary(Filo<ILTree::Statement*,NorlsAllocWrapper> &statementStack, Variable *tempVar);

    Closure *FindNearestClosure(ILTree::ExecutableBlock *);
    Closure *FindOwningClosure(Procedure *) const;
    Closure *FindOwningClosure(Variable *) const;
    ClosureBase *FindOwningClosureBase(Procedure *) const;
    VariableFlags GetVariableFlags(Variable *) const;
    void AddVariableFlag(Variable *, VariableFlags);
    ClosureFrame *GetCurrentClosureFrame() const { return m_frameStack.Peek(); }
    ILTree::ProcedureBlock *GetProcedureBody(Procedure*);
    TrackedExpression *TrackExpression(ILTree::Expression**);
    EmptyBlockData *GetOrCreateEmptyBlockDataForGroup(ILTree::Statement*, Procedure *);
    EmptyBlockData *GetOrCreateEmptyBlockDataForBlock(ILTree::ExecutableBlock*, Procedure*);
    EmptyBlockData *CreateEmptyBlockDataCore(ILTree::Statement*, Location loc, Procedure*);
    EmptyBlockData *GetEmptyBlockDataForGroup(unsigned);
    EmptyBlockData *GetEmptyBlockDataForBlock(ILTree::ExecutableBlock*);
    Procedure *ConvertLambdaToProcedure(ILTree::LambdaExpression*);
    void CheckVariableSafeForLifting(LambdaData *, ILTree::SymbolReferenceExpression*);
    ILTree::SymbolReferenceExpression *CreateReferenceToContainingClassMeImpl();


    // Do not autogenerate
    ClosureRoot();
    ClosureRoot(const ClosureRoot&);
    ClosureRoot &operator=(const ClosureRoot&);

    // Outermost proc the root is associated with
    Procedure *m_proc;
    ILTree::ProcedureBlock *m_procBody;
    ClosureRootState m_state;
    bool m_procContainsGoto;
    bool m_genEncCode;
    unsigned m_lambdaNestLevel;
    Semantics *m_semantics;
    Compiler *m_pCompiler;
    NorlsAllocator m_norls;
    NorlsAllocWrapper m_alloc;
    Symbols &m_transientSymbolFactory;
    List<OptimizedClosure*,NorlsAllocWrapper> m_optClosureList;
    Filo<BlockData*,NorlsAllocWrapper> m_blockStack;
    LocalScopeInjection* m_LocalScopesToInject; // #576786 stack of scopes to inject
    Filo<ClosureFrame*,NorlsAllocWrapper> m_frameStack;
    Filo<ILTree::Statement*,NorlsAllocWrapper> m_statementStack;
    List<TrackedExpression*,NorlsAllocWrapper> m_trackedList;
    List<Closure*,NorlsAllocWrapper> m_rootClosureList;
    DynamicHashTable<ILTree::CatchBlock*,Variable*,NorlsAllocWrapper> m_catchMap;
    DynamicHashTable<ILTree::ExecutableBlock*,Closure*,NorlsAllocWrapper> m_map;
    DynamicHashTable<Variable*,VariableFlags,NorlsAllocWrapper> m_flagMap;

    // Tracks Procedure created from Lambda -> Source Lambda
    DynamicHashTable<Procedure*,LambdaData*,NorlsAllocWrapper> m_realLambdaMap;

    // ** Raw Lambda Expression in BILTREE -> LambdaData
    DynamicHashTable<ILTree::Expression**,LambdaData*,NorlsAllocWrapper> m_lambdaToDataMap;
    // unsigned int would be more natural than UINT_PTR, but HashTables require Pointer precision,
    // otherwisethe hashtable won't be able to create a hash on 64-bit
    DynamicHashTable<UINT_PTR,EmptyBlockData*,NorlsAllocWrapper> m_emptyBlockForGroupMap;
    DynamicHashTable<ILTree::ExecutableBlock*,EmptyBlockData*,NorlsAllocWrapper> m_emptyBlockForBlockMap;
    DynamicHashTable<Variable*,ShortLivedTemporaryData*,NorlsAllocWrapper> m_shortLivedMap;
    DynamicHashTable<ILTree::ExecutableBlock*,Closure*,NorlsAllocWrapper> m_nearestMap;
    DynamicHashTable<Variable*,Closure*,NorlsAllocWrapper> m_varMap;
    List<ILTree::Expression*,NorlsAllocWrapper> m_mbcCallList;    // Tracks MyBase/MyClass calls
};


enum GenericContextKind
{
    NormalClosureContext,
    OptimizedClosureContext,
    MyBaseMyClassStubGenericContext,
};

//==============================================================================
// GenericContext
//
// Holds methods common to GenericContext's which represent a method or set of
// methods in the code Closures must translate.
//==============================================================================
class GenericContext
{
public:
    Compiler *GetCompiler() const { return m_root->m_pCompiler; }
    Semantics *GetSemantics() const { return m_root->m_semantics; }
    NorlsAllocator *GetNorlsAllocator() const { return m_root->GetNorlsAllocator(); }
    TransientSymbolStore *GetTransientSymbolStore() const { return m_root->GetTransientSymbolStore(); }

    // Generic Helpers
    BCSYM *GetEquivalentType(BCSYM *type)
        { return GetEquivalentTypeImpl(type, false); }
    BCSYM *GetEquivalentTypeForMetaData(BCSYM *type)
        { return GetEquivalentTypeImpl(type, true); }
    GenericTypeBinding *GetEquivalentTypeBinding(GenericTypeBinding *binding)
        { return GetEquivalentTypeBindingImpl(binding, false); }
    GenericTypeBinding *GetEquivalentTypeBindingForMetaData(GenericTypeBinding *binding)
        { return GetEquivalentTypeBindingImpl(binding, true); }
    GenericBinding *GetEquivalentBinding(GenericBinding *binding)
        { return GetEquivalentBindingImpl(binding, false); }
    GenericBinding *GetEquivalentBindingForMetaData(GenericBinding *binding)
        { return GetEquivalentBindingImpl(binding, true); }
    GenericBinding *CreateBinding(Declaration *decl)
        { return CreateBindingImpl(decl, false); }
    GenericBinding *CreateBindingForMetaData(Declaration *decl)
        { return CreateBindingImpl(decl, true); }
    GenericTypeBinding *CreateTypeBinding(Declaration *decl)
        { return CreateTypeBindingImpl(decl, false); }
    GenericTypeBinding *CreateTypeBindingForMetaData(Declaration *decl)
        { return CreateTypeBindingImpl(decl, true); }
    Type *GetEquivalentGenericParameter(GenericParameter*);

    // Pure Virtual
    virtual GenericContextKind GetGenericContextKind() const = 0;

    // Casting Functions
    Closure *AsClosure();
    OptimizedClosure *AsOptimizedClosure();
    MyBaseMyClassStubContext *AsMyBaseMyClassStubContext();

protected:
    GenericContext(ClosureRoot*);
    virtual ~GenericContext() { }

    // Pure Virtual
    virtual STRING *GetNameForCopiedGenericParameter(GenericParameter*) = 0;
    virtual GenericContext *GetParentGenericContext() const = 0;

    // Methods
    Symbols *GetSymbolFactory(bool);
    Symbols *GetTransientSymbolFactory() const { return m_root->GetTransientSymbolFactory(); }
    Symbols *GetNormalSymbolFactory() const { return m_root->GetNormalSymbolFactory(); }
    void CopyGenericParametersForMethod(Declaration *source) { CopyGenericParametersImpl(source, true); }
    void CopyGenericParametersForType(Declaration *source) { CopyGenericParametersImpl(source, false); }
    void AddClassHierarchyToMap(ClassOrRecordType *);
    GenericBinding *CreateSimpleBinding(Declaration *decl)
        { return CreateSimpleBindingImpl(decl, false); }
    GenericBinding *CreateSimpleBindingForMetaData(Declaration *decl)
        { return CreateSimpleBindingImpl(decl, true); }
    GenericTypeBinding *CreateSimpleTypeBinding(Declaration *decl)
        { return CreateSimpleTypeBindingImpl(decl, false); }
    GenericTypeBinding *CreateSimpleTypeBindingForMetaData(Declaration *decl)
        { return CreateSimpleTypeBindingImpl(decl, true); }

private:
    void CopyGenericParametersImpl(Declaration*, bool);
    GenericConstraint *CopyGenericConstraints(GenericConstraint *);
    BCSYM *GetEquivalentTypeImpl(BCSYM*, bool);
    GenericBinding *GetEquivalentBindingImpl(GenericBinding*, bool);
    GenericTypeBinding *GetEquivalentTypeBindingImpl(GenericTypeBinding*, bool);
    GenericBinding *CreateBindingImpl(Declaration *, bool);
    GenericTypeBinding *CreateTypeBindingImpl(Declaration *, bool);
    GenericBinding *CreateSimpleBindingImpl(Declaration*,bool);
    GenericTypeBinding *CreateSimpleTypeBindingImpl(Declaration*, bool);

protected:

    virtual bool SkipDuplicateGenericConstraints() { return false; };

    // Fields
    ClosureRoot *m_root;
    GenericParameter *m_firstGenericParam;
    DynamicHashTable<GenericParameter*,Type*,NorlsAllocWrapper> m_genericMap;
    DynamicHashTable<GenericBinding*,GenericBinding*,NorlsAllocWrapper> m_bindingCacheMap;
    DynamicHashTable<GenericBinding*,GenericBinding*,NorlsAllocWrapper> m_bindingMetaDataCacheMap;
};

//==============================================================================
// ClosureBase
//
// Holds methods common to 'Closure' and 'OptimizedClosure' instances.
//==============================================================================
class ClosureBase : public GenericContext
{
public:

    // Pure Virtual
    virtual void FixupLambdaReference(LambdaData*) = 0;

protected:
    ClosureBase(ClosureRoot *);
    virtual __override ~ClosureBase() { }


    // Overrides
    virtual __override STRING* GetNameForCopiedGenericParameter(GenericParameter*);

};

class Closure : public ClosureBase
{
    friend ClosureVisitor;
#ifdef DEBUG
    friend class ClosureDump;
#endif

public:
    Closure(Procedure *, ILTree::ExecutableBlock *, ClosureRoot *);
    virtual __override ~Closure();

    ILTree::ExecutableBlock *GetBlock() const { return m_block; }
    Closure *GetParent() const { return m_parent; }
    Closure *GetRootParent() { return m_parent ? m_parent->GetRootParent() : this; }
    ClassOrRecordType *GetClosureClass() const { return m_closureClass; }

    void EnsureVariableLifted(Variable *);
    void AddLambda(LambdaData *);
    void AddChildClosure(Closure *);
    void CreateClosureClassCode();
    void CreateInitializationCode();
    bool OwnsProcedure(Procedure *);
    bool OwnsVariable(Variable *);
    void FixupVariableGenericType(Variable*);
    void FixupVariableReference(ILTree::SymbolReferenceExpression *symRef, Procedure *referencingProc);
    void FixupGenericTypes();
    bool IsLifted(Variable* var) const { ThrowIfNull(var); return m_liftSet.Contains(var); }

    virtual void FixupLambdaReference(LambdaData *);
    virtual GenericContextKind GetGenericContextKind() const { return NormalClosureContext; }
protected:

    virtual GenericContext *GetParentGenericContext() const { return m_parent; }

private:

    Procedure *GetRootProcedure() const { return m_root->m_proc; }
    Variable *EnsureLiftedAndCreated(Variable *);

    void CreateClass();
    void CreateClassVariable();
    void CreateFields();
    Variable *CreateField(Variable *var);
    void CreateMeField();
    void ReparentProcedures();
    void CreateGenericInfo();
    void CreateGenericParameters();
    void CreateExternalGenericBinding();
    void CreateInternalGenericBinding();
    ILTree::Statement *CreateMemberAssignment(Variable *target, GenericTypeBinding *targetBinding, Variable *source);
    ILTree::Statement *CreateInstance(ILTree::ExecutableBlock *, bool);
    ILTree::SymbolReferenceExpression *CreateReferenceToLiftedVariable(Variable*, Procedure*, Location);
    ILTree::SymbolReferenceExpression *CreateReferenceToThisClosure(bool useMe, Location loc);
    ILTree::SymbolReferenceExpression *CreateReferenceToParentLiftedVariable(Closure *, Variable*, GenericBinding *, ILTree::SymbolReferenceExpression *baseRef, Procedure *referencingProc);
    void CreateCopyConstructor();
    void CreateEmptyConstructor();
    void FixupCatchStatements(Variable *);

    // Do not Generate
    Closure();
    Closure(const Closure&);
    Closure& operator=(const Closure&);

    Procedure *m_containingProc;
    ILTree::ExecutableBlock *m_block;
    Closure *m_parent;
    ClassOrRecordType *m_closureClass;
    Procedure *m_emptyConstructor;
    Procedure *m_copyConstructor;
    Variable *m_closureVariable;
    GenericTypeBinding *m_externalBinding;
    GenericTypeBinding *m_internalBinding;
    List<Closure*,NorlsAllocWrapper> m_childList;
    List<Procedure*,NorlsAllocWrapper> m_procList;
    HashSet<Variable*,NorlsAllocWrapper> m_liftSet;
    DynamicHashTable<Variable*,Variable*,NorlsAllocWrapper> m_liftedToFieldMap;
};

//==============================================================================
// OptimizedClosure
//
// Holds lambda's which are optimized as private methods on the containing class.  
// This includes both closures which lift absolutely nothing and are generated as 
// private static methods, and closures which only "lift" the variable Me and are
// generated as private instance methods.  
//
// There will be a single instance of OptimizedClosure for every optimized lambda.
//
//==============================================================================
class OptimizedClosure : public ClosureBase
{
#ifdef DEBUG
    friend class ClosureDump;
#endif

public:
    OptimizedClosure(ClosureRoot *root)
        :ClosureBase(root), m_lambdaData(NULL)
    {
    }

    virtual __override ~OptimizedClosure()
    {
        // 'ClosureRoot' actually owns the 'LambdaData' instance so it does
        // not need to be freed here
    }

    void FixupGenericTypes();
    const LambdaData *GetLambdaData() const { return m_lambdaData; }
    LambdaData *GetLambdaData() { return m_lambdaData; }
    void SetLambda(LambdaData*);
    virtual void FixupLambdaReference(LambdaData*);
    virtual GenericContextKind GetGenericContextKind() const { return OptimizedClosureContext; }
protected:
    virtual GenericContext *GetParentGenericContext() const { return NULL; }

private:
    GenericBinding *CreateLambdaBinding(ClosureBase *owner);

    // Do not auto generate
    OptimizedClosure();
    OptimizedClosure(const OptimizedClosure&);
    OptimizedClosure &operator=(const OptimizedClosure&);

    LambdaData* m_lambdaData;
};

//==============================================================================
// MyBaseMyClassStubContext
//
// Used when the Closure code needs to generate method stubs in the containing
// class that are not directly lambda related.
//
// This class is specifically geared to be of use to only a single method.  Each
// method in a class can be independently generic in completely unrelated manners.
// This class is built to deal with a specific generic context.
//
// It's very light weight and all the real work will only happen when we generate
// the stub.  All subsequent operations just grab it from a hash table
//==============================================================================
class MyBaseMyClassStubContext : public GenericContext
{
public:
    MyBaseMyClassStubContext(ClosureRoot*,Procedure *);

    void FixupCall(ILTree::CallExpression *call);
    void FixupDelegateConstructor(ILTree::DelegateConstructorCallExpression *del);
    virtual GenericContextKind GetGenericContextKind() const { return MyBaseMyClassStubGenericContext; }
protected:

    // Overrides
    virtual STRING* GetNameForCopiedGenericParameter(GenericParameter*);
    virtual GenericContext *GetParentGenericContext() const { return NULL; }
    virtual bool SkipDuplicateGenericConstraints() { return true; };

private:
    Procedure *CreateMethodStubForMyBaseMyClass(_Inout_ STRING*, unsigned);
    void UpdateSymbolToStubProcedure(ILTree::SymbolReferenceExpression *, unsigned);
    void FixupStubGenericSignature(Procedure*);
    void GrabGenericInfo();

    // Fields
    ClassOrRecordType *m_stubClass;
    Procedure *m_originalProc;
};

//==============================================================================
// ClosureGenericVisitor
//
// Used to fixup all of the generic references in lifted lambdas
//==============================================================================
class ClosureGenericVisitor : public BoundTreeTypeVisitor 
{
public:
    ClosureGenericVisitor(ClosureRoot*, ClosureBase*);
    virtual ~ClosureGenericVisitor() {}

    void Visit(Procedure *proc, ILTree::ProcedureBlock *body);

protected:
    virtual bool StartExpression(ILTree::Expression **);

    virtual bool StartType(BCSYM **);
    virtual bool StartGenericBinding(GenericBinding **);
    virtual bool StartGenericTypeBinding(GenericTypeBinding **);

private:
    // Do not auto generate
    ClosureGenericVisitor();
    ClosureGenericVisitor(const ClosureGenericVisitor &);
    ClosureGenericVisitor &operator=(const ClosureGenericVisitor&);

    ClosureRoot *m_root;
    ClosureBase *m_closure;
};

//==============================================================================
// ClosuresGotoIterator
//
// Used to shim the gotos when there is a closure and log compiler errors
// where appropriate
//
// Uses a separate allocator since it doesn't create any long term data
//==============================================================================
class ClosureGotoVisitor : public BoundTreeVisitor
{
public:
    ClosureGotoVisitor(const ClosureRoot*);
    virtual ~ClosureGotoVisitor(); 

    void Visit(Procedure *proc, ILTree::ProcedureBlock *body);


protected:
    virtual bool StartStatement(ILTree::Statement *);

private:
    // Do not auto generate
    ClosureGotoVisitor();
    ClosureGotoVisitor(const ClosureGotoVisitor&);
    ClosureGotoVisitor& operator=(const ClosureGotoVisitor&);

    bool IsDescendantOf(ILTree::ExecutableBlock*,ILTree::ExecutableBlock*);
    bool IsClosureInScope(ILTree::ExecutableBlock*);

    // This must be const.  We share an allocator which the root will rollback
    // after iterations so we need to gaurantee we don't modify the ClosureRoot
    // and hence cause any additional allocations which would then unexpectedly
    // be rolled back.
    const ClosureRoot *m_root;
    NorlsAllocator *m_norls;
    DynamicHashTable<LABEL_ENTRY_BIL*,ILTree::LabelStatement*,NorlsAllocWrapper> m_labelMap;
    List<ILTree::GotoStatement*,NorlsAllocWrapper> m_gotoList;
    DynamicHashTable<ILTree::ExecutableBlock*,DynamicHashTable<ILTree::ExecutableBlock*,bool,NorlsAllocWrapper>*,NorlsAllocWrapper> m_hierarchyMap;
    DynamicHashTable<ILTree::ExecutableBlock*,LambdaData*,NorlsAllocWrapper> m_realLambdaMap;
    DynamicHashTable<ILTree::ExecutableBlock*,bool,NorlsAllocWrapper> m_hasClosureInScopeMap;
};

class ClosureShortLivedTemporaryVisitor : public BoundTreeVisitor
{
    struct GroupData
    {
        GroupData(NorlsAllocator *norls) : VariableMap(norls) {}
        DynamicHashTable<Variable*,Variable*,NorlsAllocWrapper> VariableMap;
    };

public:
    ClosureShortLivedTemporaryVisitor(ClosureRoot *root);
    ~ClosureShortLivedTemporaryVisitor(); 

    void Visit();

protected:
    virtual bool StartStatement(ILTree::Statement *);
    virtual void EndStatement(ILTree::Statement *);
    virtual bool StartExpression(ILTree::Expression **);

private:
    // Do not auto generate
    ClosureShortLivedTemporaryVisitor();
    ClosureShortLivedTemporaryVisitor(const ClosureShortLivedTemporaryVisitor&);
    ClosureShortLivedTemporaryVisitor& operator=(const ClosureShortLivedTemporaryVisitor&);

    ClosureRoot *m_root;
    NorlsAllocator *m_norls;
    Filo<ILTree::Statement*,NorlsAllocWrapper> m_statementStack;
    // unsigned int would be more natural than UINT_PTR, but HashTables require Pointer precision, otherwise
    // the hashtable won't work on 64-bit
    DynamicHashTable<UINT_PTR, GroupData*,NorlsAllocWrapper> m_groupMap;
    DynamicHashTable<Variable*, bool,NorlsAllocWrapper> m_normalMap;
};

//==============================================================================
// Closure iterator.
//==============================================================================
class ClosureVisitor 
{
public:
    ClosureVisitor(_In_ Closure *start)
        :m_start(start), 
        m_norls(NORLSLOC),
        m_stack(&m_norls)
    {
        AssertIfNull(m_start);
        Reset();
    }

    void Reset()
    {
        m_stack.Clear();
        m_stack.AddLast(m_start);
    }

    Closure *GetNext()
    {
        if ( m_stack.Count() == 0)
        {
            return 0;
        }

        Closure *toRet = m_stack.GetFirst()->Data();
        m_stack.InefficentRemove(m_stack.GetFirst());

        if ( toRet->m_childList.Count() > 0 )
        {
            ListValueIterator<Closure*,NorlsAllocWrapper> it(&toRet->m_childList);
            while (it.MoveNext() )
            {
                m_stack.AddLast(it.Current());
            }
        }

        return toRet;
    }

private:

    Closure *m_start;
    NorlsAllocator m_norls;
    List<Closure*,NorlsAllocWrapper> m_stack;
};
