//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once;


class BCSYM_Member;
class BCSYM_PointerType;

//-------------------------------------------------------------------------------------------------
//
// Class to manage the allocation of BILTREE nodes and its appurtenances.
// The inline bodies are in bilinl.h and the non-inline ones are in bilalloc.cpp.
//
class BILALLOC
{
    private:
    Compiler * m_pCompiler;
    NorlsAllocator * m_pnraTree;

    // Dev10 #758830 This table captures a map from the original block to its new copy.
    // A block is kept in this map while its children are being copied.
    DynamicHashTable<ILTree::PILNode, ILTree::PILNode> m_BlocksBeingCopied;

    // Dev10 #772215 A map from old variable to the new variable for substitution.
    DynamicHashTable<Variable*,Variable*> m_varMap;
    
    // Dev10 #772215 A map from original block contained in a Lambda to its new copy.
    // A block is kept in this map until we finish copying top-most Lambda.
    DynamicHashTable<ILTree::PILNode, ILTree::PILNode> m_BlocksCopiedWithinLambda; 

    // Dev10 #772215 A map from original LABEL_ENTRY_BIL to its new copy.
    // A LABEL_ENTRY_BIL is kept in this map until we finish copying a Lambda.
    DynamicHashTable<LABEL_ENTRY_BIL *, LABEL_ENTRY_BIL *> * m_pCopiedLabelEntries; 

public:    // Accessors.

    BILALLOC() 
        : m_pCompiler(NULL)
        , m_pnraTree(NULL)
        , m_pCopiedLabelEntries(NULL)
    {

    }

    BILALLOC(Compiler* pCompiler, NorlsAllocator* pNra) 
        : m_pCompiler(pCompiler)
        , m_pnraTree(pNra)
        , m_pCopiedLabelEntries(NULL)
    {

    }

    void SetAllocator(NorlsAllocator *pnra)
    {
        m_pnraTree = pnra;
    }

    void Init(Compiler *pCompiler)
    {
        m_pCompiler = pCompiler;
    }

    // The fundamental methods
    BYTE *xAllocBilBytes(size_t cb);

    // Methods to allocate specfic types
    ILTree::PILNode xAllocBilNode(BILOP bilop);
    ILTree::PILNode xAllocUDOBilNode(BILOP bilop);
    ILTree::PILNode xAllocBilNodeOff(BILOP bilop, ILTree::PILNode ptree);

    ILTree::PILNode xAllocSxTree(BILOP bilop, ILTree::PILNode ptreeL, ILTree::PILNode ptreeR);
    ILTree::PILNode xAllocUDOSxTree(BILOP bilop, ILTree::PILNode ptreeL, ILTree::PILNode ptreeR);

    // Methods to build specific BILTREE constructs
    ILTree::PILNode xCopyBilNode(ILTree::PILNode ptreeIn);

    // Scratch copies
    ILTree::PILNode xCopyBilNodeForScratch(ILTree::PILNode ptree);
    ILTree::PILNode xCopyBilTreeForScratch(ILTree::PILNode ptree);

    // Unsafe methods
    ILTree::PILNode xCopyBilTreeUnsafe(ILTree::PILNode ptree);

    // Strongly type copy methods
    ILTree::SymbolReferenceExpression *xCopySymbolReferenceTree(ILTree::PILNode ptree);
    ILTree::LambdaExpression *xShallowCopyLambdaTree(ILTree::LambdaExpression *);
    ILTree::UnboundLambdaExpression *BILALLOC::xShallowCopyUnboundLambdaTreeForScratch(ILTree::UnboundLambdaExpression *);
    ILTree::PILNode xCopyBilTreeBeta2SpecialCase(ILTree::PILNode ptree);
    BCSYM_Hash * DeepCloneLambdaLocals(ILTree::LambdaExpression *lambdaExpr, DynamicHashTable<Variable*,Variable*> &varMap);
    BCSYM_Hash * DeepCloneBlockLocals(ILTree::ExecutableBlock *pExecBlock, DynamicHashTable<Variable*,Variable*> &varMap);

private:    
    static BCSYM_Variable* DeepCloneVariable(BCSYM_Variable *oldVar, Symbols &symbols);
    static void MapVariable(Variable *oldVar, Variable *newVar, DynamicHashTable<Variable*,Variable*> &varMap);
    static void UnmapVariables(BCSYM_Hash *oldHash, DynamicHashTable<Variable*,Variable*> &varMap);
    static void UnmapTemporaries(TemporaryManager *oldTemps, DynamicHashTable<Variable*,Variable*> &varMap);
    void DeepCloneParameters(ILTree::LambdaExpression *lambdaExpr);
    TemporaryManager * DeepCloneTemporaries(ILTree::LambdaExpression *lambdaExpr);

#if DEBUG && 0
private:
    unsigned long m_ulId;
public:
    unsigned long DebugNextId()
    {
        m_ulId += 1; return m_ulId;
    }
#endif // DEBUG

private:
    ILTree::PILNode xCopyBilNodeImpl(ILTree::PILNode, unsigned);
    ILTree::PILNode xCopyBilTreeImpl(ILTree::PILNode, unsigned, ILTree::ExecutableBlock *pParent=NULL);
    LABEL_ENTRY_BIL * xCopyLabelEntry(LABEL_ENTRY_BIL * pEntryIn, ILTree::LabelStatement *pParent = NULL);
    static BCSYM_Hash* DeepCloneLocals(BCSYM_Hash *oldHash, Symbols &symbols, DynamicHashTable<Variable*,Variable*> &varMap);

}; // class

//markmil: this used to be declared static. 
inline
void* _cdecl operator new(size_t cbSize, BILALLOC &alloc)
{
	return alloc.xAllocBilBytes(cbSize);
}

