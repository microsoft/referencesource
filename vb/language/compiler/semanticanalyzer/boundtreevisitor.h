//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
//  Provides a general purpose class for visiting nodes in a BILTREE structure.
//
// ATTENTION:
// This visitor doesn’t always guarantee correct block scoping in terms of execution. 
// For example, it visits condition of an IF statement in context of its block, but during execution
// that condition is evaluated before the block is entered.

class BoundTreeVisitor
{
public:
    BoundTreeVisitor() {}
    virtual ~BoundTreeVisitor() {}

    void Visit(_In_opt_ ILTree::ProcedureBlock *body); 


    // Visit methods
    void VisitBlock(_Inout_ ILTree::ExecutableBlock*);
    void VisitStatement(_Inout_ ILTree::Statement*);
    void VisitExpression(_Inout_opt_ ILTree::Expression**);

protected:
    void ProcessRightSkewedTree(ILTree::Expression* ExprLoc);


    // Overridable methods
    virtual bool StartBlock(ILTree::ExecutableBlock*) { return true; }
    virtual void EndBlock(ILTree::ExecutableBlock*) { }
    virtual bool StartBlockStatements(ILTree::ExecutableBlock*) { return true; }
    virtual void EndBlockStatements(ILTree::ExecutableBlock*) { }
    virtual bool StartStatement(ILTree::Statement* pCurrentStmt) { return !IsBad(pCurrentStmt); }
    virtual void EndStatement(ILTree::Statement*) { }
    virtual bool StartExpression(_Inout_ ILTree::Expression ** ppExprTree) { return !IsBad(*ppExprTree); }

    virtual void VisitFor( ILTree::ForBlock*);
    virtual void VisitForEach( ILTree::ForEachBlock*);
    virtual void VisitLoop ( ILTree::LoopBlock *);
    virtual void VisitIf ( ILTree::IfBlock *);
    virtual void VisitWith ( ILTree::WithBlock *);
    virtual void VisitCase ( ILTree::CaseBlock *);
    
    //when overriding EndExpression, know that it will not be called for any
    //SX_SEQ or SX_LIST node, just the head.
    virtual void EndExpression(ILTree::Expression **) { }

public:
    void ProcessStatement(_In_ ILTree::Statement *pStatement);
    void ProcessExpression(_In_ ILTree::Expression *pExpr);
    void ProcessBlock(_In_ ILTree::ExecutableBlock*);
};


//-------------------------------------------------------------------------------------------------
//
// Subclass of BoundTreeVisitor that will invoke a predicate on all matching nodes
//
class BoundTreeNodeVisitor : public BoundTreeVisitor
{
public:

    BoundTreeNodeVisitor(BILOP bilop, std::function<bool (ILTree::ILNode*)> callback) : bilop(bilop), callback(callback) {}
    BILOP bilop;
    std::function<bool (ILTree::ILNode*)> callback;

    virtual bool StartBlock(_Inout_ ILTree::ExecutableBlock* node)
    {
        return (node->bilop == bilop) ? callback(node) : true;
    }
    virtual bool StartStatement(_Inout_ ILTree::Statement* node)
    {
        return (node->bilop == bilop) ? callback(node) : true;
    }
    virtual bool StartExpression(_Inout_ ILTree::Expression** node)
    {
        return ((*node)->bilop == bilop) ? callback(*node) : true;
    }
};



//-------------------------------------------------------------------------------------------------
//
// Subclass of BoundTreeVisitor that will ferret out all of the top level types in the Biltree.
//
class BoundTreeTypeVisitor :
    public BoundTreeVisitor
{
protected:

    BoundTreeTypeVisitor() { }
    ~BoundTreeTypeVisitor() { }

    // Iterator Methods
    void VisitType(_Inout_ _Deref_prepost_opt_valid_ BCSYM **);
    void VisitGenericBinding(_Inout_ _Deref_prepost_opt_valid_ GenericBinding **);
    void VisitGenericTypeBinding(_Inout_ _Deref_prepost_opt_valid_ GenericTypeBinding **);

    // Overriden Methods
    virtual bool StartBlock(_Inout_ ILTree::ExecutableBlock*);
    virtual bool StartStatement(_Inout_ ILTree::Statement *);
    virtual bool StartExpression(_Inout_ ILTree::Expression **); 

    // Overridable Methods
    virtual bool StartType(BCSYM **) { return true; }
    virtual void EndType(BCSYM **) { }
    virtual bool StartGenericBinding(GenericBinding **) { return true; }
    virtual void EndGenericBinding(GenericBinding **) { }
    virtual bool StartGenericTypeBinding(_In_opt_ GenericTypeBinding **) { return true; }
    virtual void EndGenericTypeBinding(GenericTypeBinding **) { } 

public:

    // Helper Methods
    void ProcessTemporaryManager(_Inout_opt_ TemporaryManager*);
    void ProcessParameters(_Inout_opt_ Parameter*);
    void ProcessLocalsHash(Scope*);
    void ProcessGenericBinding(GenericBinding*);
    void ProcessExpressionTypes(_In_ ILTree::Expression *expr);
    void ProcessStatementTypes(_Inout_ ILTree::Statement *statement);
    void ProcessType(_In_ BCSYM **ppType);
};




//-------------------------------------------------------------------------------------------------
//
// BoundTreeRewriter:
//
// Adapter around BoundTreeTypeVisitor with a more compositional API for rewriting bound trees.
// This one has virtual methods "RewriteExpression", "RewriteStatement", "RewriteType" that you
// can override.
//
// If will mutate statement and expression trees in-place. e.g. if you override RewriteExpression
// to turn all + unary operators into - unary operators (hey yeah it's a crazy thing to do -- don't
// ask me -- you're the one doing it!) then the top-level expression+statement objects will have the
// same identity, and also most of the way down the tree, but the thing which used to point to the
// old expression node will now point to your new one.
// This makes sense because expression trees generally aren't shared.
//
// Type trees will NOT be mutated in place, however. e.g. if there was a node with type IEnumerable<T>,
// and you override RewriteType to turn all BCSYM_GenericParam("T") into "Int32", then it will
// allocate a NEW generic type for IEnumerable with your argument in it, rather than modifying the
// old IEnumerable<T>.
// This makes sense because type trees generally are shared.
// But at least it tries to be smart about it. If a none of a type-node's-children were replaced,
// then it won't replace the parent.
//
class BoundTreeRewriter
{
public:
    BoundTreeRewriter(Symbols *transientSymbolCreator) : m_adapter(this), m_TransientSymbolCreator(transientSymbolCreator) {}

    ILTree::Statement* RewriteStatementList(_In_opt_ ILTree::Statement* statements);

protected:
    virtual ILTree::Expression* RewriteExpression(_In_ ILTree::Expression *expr); // must return an expression of the same type (up to generic parameter substitution)
    virtual ILTree::Statement* RewriteStatement(_In_ ILTree::Statement *statement); // is allowed to return 0 or more in a linked-list
    virtual BCSYM* RewriteType(_In_ BCSYM *type);  // The rewriter makes copies rather than mutating
    virtual BCSYM_GenericBinding* RewriteGenericBinding(_In_ BCSYM_GenericBinding *binding){return binding;}; // a helper for the rewriter

private:
    BoundTreeRewriter(const BoundTreeRewriter&); // C++ don't autogenerate
    BoundTreeRewriter &operator=(const BoundTreeRewriter&); // C++ don't autogenerate
    
    Symbols* m_TransientSymbolCreator;

    BCSYM_GenericTypeBinding* RewriteGenericTypeBinding(_In_ BCSYM_GenericTypeBinding *type); // a helper for the rewriter

    class VisitorAdapter : public BoundTreeTypeVisitor
    {
    public:
        VisitorAdapter(BoundTreeRewriter *rewriter) : m_rewriter(rewriter) {}
        BoundTreeRewriter *m_rewriter;

        virtual bool StartExpression(_Inout_ ILTree::Expression ** ppExprTree);
        virtual bool StartBlockStatements(ILTree::ExecutableBlock*);
        virtual bool StartType(_Inout_ BCSYM ** ppType);
        virtual bool StartGenericBinding(_Inout_ GenericBinding **ppBinding);

    } m_adapter;
};


#if DEBUG

//-------------------------------------------------------------------------------------------------
//
// Used in Debug builds to walk the biltree and perform some sanity checks
//
class BoundTreeDebugVisitor : public BoundTreeVisitor 
{
public:
    BoundTreeDebugVisitor(Compiler*); 
    virtual ~BoundTreeDebugVisitor();

protected:

    virtual bool StartStatement(ILTree::Statement*);
    virtual void EndStatement(ILTree::Statement*);
    virtual bool StartBlock(ILTree::ExecutableBlock*);
    virtual void EndBlock(ILTree::ExecutableBlock*);
    virtual bool StartExpression(_Inout_ ILTree::Expression **);

private:
    NorlsAllocator m_norls;
    NorlsAllocWrapper m_norlsWrapper;
    Filo<ILTree::Statement*,NorlsAllocWrapper> m_statementStack;
    Filo<ILTree::ExecutableBlock*,NorlsAllocWrapper> m_blockStack;
};

#endif 

//-------------------------------------------------------------------------------------------------
//
// Used to walk the biltree and check that none of the nodes
// are marked bad.
//
class BoundTreeBadnessVisitor : public BoundTreeVisitor 
{
public:
    BoundTreeBadnessVisitor(); 
    virtual ~BoundTreeBadnessVisitor();
    bool TreeIsBad() { return m_isBad; }

    void Visit(_In_opt_ ILTree::ExecutableBlock *block);

protected:

    virtual bool StartStatement(ILTree::Statement*);
    virtual void EndStatement(ILTree::Statement*);
    virtual bool StartBlock(ILTree::ExecutableBlock*);
    virtual void EndBlock(ILTree::ExecutableBlock*);
    virtual bool StartExpression(_Inout_ ILTree::Expression **);

private:
    bool m_isBad;
};


//-------------------------------------------------------------------------------------------------
//
// Anonymous delegates iterator
//
// Used to find and register all of the anonymous delegates that are used in the
// bound tree.
//
class AnonymousDelegatesRegisterVisitor : public BoundTreeTypeVisitor 
{
public:
    AnonymousDelegatesRegisterVisitor(Procedure *Proc, _In_ TransientSymbolStore *pTransientSymbolStore)
        : m_Procedure(Proc),
          m_pTransientSymbolStore(pTransientSymbolStore)
        {ThrowIfNull(Proc); ThrowIfNull(pTransientSymbolStore);}

    virtual ~AnonymousDelegatesRegisterVisitor(){}

protected:
    // This method overrides the StartGenericTypeBinding in the base iterator and
    // registers any anonymous delegates seen during the tree walk.
    //
    virtual bool StartGenericTypeBinding(_In_opt_ GenericTypeBinding **);

private:
    Procedure *m_Procedure;

    // Store in which to register the anonymous delegates.
    TransientSymbolStore *m_pTransientSymbolStore;
};


class SerializingVisitor :
    public BoundTreeVisitor 
{
public:
    SerializingVisitor(IAppendable<ILTree::ILNode *> * pOutput);    
protected:
    bool StartBlock(ILTree::ExecutableBlock* pBlock);
    bool StartStatement(ILTree::Statement* pStatement);
    bool StartExpression(_Inout_ ILTree::Expression ** ppExpression);
private:
    IAppendable<ILTree::ILNode *> * m_pOutput;
};

class ExtensionMethodBindingChangeDetector :
    protected BoundTreeVisitor 
{
public:
    ExtensionMethodBindingChangeDetector();
    bool DidExtensionMethodBindingsChange
    (
        _In_opt_ ILTree::ProcedureBlock * pBodyWithImports, 
        _In_opt_ ILTree::ProcedureBlock * pBodyWithoutImports, 
        _Inout_ NorlsAllocator * pNorls
    );
protected:
    bool StartBlock(ILTree::ExecutableBlock* pBlock);
    bool StartStatement(ILTree::Statement* pStatement);
    bool StartExpression(_Inout_ ILTree::Expression ** ppExpression); 
    void EndBlock(ILTree::ExecutableBlock* pBlock);
    void EndStatement(ILTree::Statement* pStatement);
    void CheckRemainingNodes();
private:
    //WARNING:
    //please do not see this method and think that it can be repurposed for anything, because
    //chances are it can't. This method is only suitable for use in the AddImports extension method binding collision checks.
    //In particular, it works because the 2 methods symbols being compared are guaranteed to have been obtained from the same
    //method in the same class in the same project, and so there are no cross project identity issues. However, in general, equivalent types
    //may have different pointers if they are accessed via different projects.
    bool CompareSymbolNodes(_In_opt_ ILTree::SymbolReferenceExpression * pSymbolReference, _In_opt_ ILTree::SymbolReferenceExpression * pOtherSymbolReference );
    bool CompareGeneralNodes(ILTree::ILNode * pThisTree);

    IConstIterator<ILTree::ILNode *> * m_pWithoutImportsTreeIterator;
    bool m_ret;
    bool m_nextExpressionIsExtensionMethod;
};

class LowerBoundTreeVisitor : public BoundTreeVisitor 
{
public:

    LowerBoundTreeVisitor(_In_ Semantics *semantics);
    ~LowerBoundTreeVisitor();

    void Visit(_In_opt_ ILTree::ProcedureBlock* BoundBody);
    void Visit(_Inout_opt_ ILTree::Expression** ppExpr);
    Compiler *GetCompiler() const { return m_Compiler; }
    Semantics* GetSemantics() const { return m_semantics; }
protected:

    // Overridden from BoundTreeVisitor 
    //virtual bool StartBlock(Block*);
    //virtual void EndBlock(Block*);
    //virtual bool StartStatement(Statement *);
    virtual void EndStatement(ILTree::Statement *);
    virtual bool StartExpression(_Inout_ ILTree::Expression **);
        
    //virtual void EndExpression(Expression **);
private:

    // Do not autogenerate
    LowerBoundTreeVisitor();
    LowerBoundTreeVisitor(const LowerBoundTreeVisitor&);
    LowerBoundTreeVisitor &operator=(const LowerBoundTreeVisitor&);

    bool VisitShortCircuitBooleanOperator(_Inout_opt_ ILTree::Expression **ppExpr);
    void VisitIsTrueIsFalseNode(_Inout_opt_ ILTree::Expression ** ppExpr);
    void VisitAnonymousType( _Inout_opt_ ILTree::Expression** ppExpr );
    void VisitCallExpression (_Inout_ ILTree::Expression ** ppExpr);
    bool OptimizeNullableOperand(_Inout_ ILTree::Expression *&Operand);
    void VisitLiftedIsOrIsNotOperator(_Inout_ ILTree::Expression **ppExpr);
    void VisitLiftedIntrinsicUnaryOperator(_Inout_ ILTree::Expression **ppExpr);
    void VisitLiftedIntrinsicBinaryOperator(_Inout_ ILTree::Expression **ppExpr);
    void VisitCollectionInitializer(ILTree::Expression ** ppExpr);
    void VisitArrayLiteral(ILTree::Expression ** ppExpr);

    bool IsEmpty(ILTree::ArrayLiteralExpression * pLiteral);

    ParseTree::ArgumentList *
    CreatePreBoundArgumentList    
    (
        ParserHelper & ph,
        ILTree::ExpressionWithChildren * pArguments
    );
    

    bool m_iterating;
    Semantics *m_semantics;
    Compiler *m_Compiler;
    CompilerHost *m_CompilerHost;
    BCSYM_Proc *m_pEnclosingProc;
};

//==============================================================================
// Subclass of DeferredTempIterator that will ferret out all special SX_DEFERRED_TEMP nodes
// in the Biltree
//==============================================================================    
class DeferredTempIterator : public BoundTreeVisitor
{
public:

    DeferredTempIterator(_In_ Semantics *semantics);
    ~DeferredTempIterator();
    void Iterate();
    void Iterate( _Inout_ ILTree::Expression** ppExpr );

    Semantics* GetSemantics() const { return m_semantics; }

protected:

    virtual bool StartExpression(_Inout_ ILTree::Expression **);
    virtual void EndExpression(ILTree::Expression **);
private:

    ILTree::Expression *GetTempForDeferredTemp(_In_ ILTree::DeferredTempExpression *deferredTemp);
    ILTree::Statement *InitTemp(_In_ Variable *var, _In_ ILTree::Expression *value);
    void LinkLongLivedTemps();
    void LinkTempExpressions( _Inout_ ILTree::Expression** ppExpr );

    Semantics* m_semantics;
    DynamicHashTable<size_t, BCSYM_Variable *,NorlsAllocWrapper> *m_Vars;
    // Temporaries that need to be added to the method body before processing closures
    ILTree::Statement *m_InitStmts;
    // The last deferred temporary statement
    ILTree::Statement *m_LastInitStmt;

};

class MultilineLambdaReturnExpressionVisitor : public BoundTreeVisitor
{
public:

    MultilineLambdaReturnExpressionVisitor
    (
        ArrayList<ILTree::Expression*> *,
        Semantics*,
        NorlsAllocator*,
        NorlsAllocWrapper*
    );

    ~MultilineLambdaReturnExpressionVisitor();
    
    void VisitReturnExpressions(ILTree::StatementLambdaBlock*);
    void VisitYieldExpressions(ILTree::StatementLambdaBlock*);

protected:
    
    // Overridden methods
    virtual bool StartBlock(ILTree::ExecutableBlock*);
    virtual bool StartExpression(ILTree::Expression**);
    virtual void EndStatement(ILTree::Statement*);

private:

    bool m_VisitYields;
    ILTree::StatementLambdaBlock *m_pLambdaBlock;
    ArrayList<ILTree::Expression*> *m_expressionList;
    Semantics *m_pSemantics;
};



// Dev10 #735384
// Visitor to check expressions for improper use of embedded types.
class CheckUsageOfEmbeddedTypesVisitor : public BoundTreeVisitor
{
private:

    ErrorTable * m_pErrorTable;
    CompilerProject * m_pProject;

public:     
    CheckUsageOfEmbeddedTypesVisitor(CompilerProject * pProject, ErrorTable * pErrorTable)
    {
        ThrowIfNull(pProject);
        ThrowIfNull(pErrorTable);
        m_pProject = pProject;
        m_pErrorTable = pErrorTable;
    }


protected:
    
    // Overridden methods
    virtual bool StartExpression(ILTree::Expression**);
};

class VariableSubstitutionVisitor : public BoundTreeVisitor
{

public:
    VariableSubstitutionVisitor();
    
    bool ContainsReference(ILTree::Expression ** ppExp, Variable *OldVar);
    void ReplaceReference(ILTree::Expression ** ppExp, Variable *OldVar, Variable *NewVar);

protected:
    virtual bool StartExpression(ILTree::Expression **);

private:

    Variable *m_NewVar;
    Variable *m_OldVar;    
    bool m_foundOldVar;
    bool m_SearchVarOnly;

};







