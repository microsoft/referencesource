//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Cache for symbols created late in the compilation process
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void TransientSymbolStore::Init(
    Compiler * pCompiler,
    _In_ NorlsAllocator * pAllocator)
{
    m_pCompiler = pCompiler;
    m_pAllocator = pAllocator;
    m_SymbolList.SetOwnership(false);
    m_SymbolList.Clear();
    m_pStartTransaction = NULL;
    m_pXmlHelperClass = NULL;
    m_pAnonymousTypeHash = NULL;
    m_pAnonymousDelegateHash = NULL;
    m_pAnonDelegateToTransientSymbolMap = NULL;
    m_ResumableMethodToStateMachineAttrMap = NULL;
    m_pClosureMethodStubHash = NULL;
    m_pAnonymousTypeNameGenerator = NULL;
    m_pAnonymousDelegateNameGenerator = NULL;
    m_pStateMachineNameGenerator = NULL;
    m_closureCount = 0;
    m_lambdaCount = 0;
    m_closureVariableCount = 0;	
#if DEBUG
    m_InTransaction = false;
    m_InMethod = false;
#endif
    m_XmlNamespaces = NULL;
}

#if DEBUG
TransientSymbolStore::~TransientSymbolStore()
{
    VSASSERT(true, "breakpoint allowed");
}
#endif

void TransientSymbolStore::Insert(
    BCSYM_NamedRoot * pNamedRoot,
    BCSYM_Proc * pOwnerProc)
{
    TransientSymbol *pTransient = new (*m_pAllocator) TransientSymbol();
    pTransient->m_pTransient = pNamedRoot;
    pTransient->m_pOwnerProc = pOwnerProc;
    m_SymbolList.InsertLast(pTransient);

    // Mark the symbol as transient
    pNamedRoot->SetIsTransient();

    // Map the anonymous delegate to the corresponding Transient symbol.
    if (pNamedRoot->IsAnonymousDelegate())
    {
        BCSYM_Class *pAnonymousDelegate = pNamedRoot->PClass();
        GetAnonDelegateToTransientSymbolMap()->TransactHashAdd(pAnonymousDelegate, pTransient);
    }
}

void TransientSymbolStore::InsertAnonymousDelegateIfNotAlreadyInserted(
    _In_ BCSYM_Class * pAnonymousDelegate,
    _In_ BCSYM_Proc * pOwnerProc)
{
    ThrowIfFalse(pAnonymousDelegate->IsAnonymousDelegate());

    TransientSymbol** ppPossibleMatch = GetAnonDelegateToTransientSymbolMap()->HashFind(pAnonymousDelegate);

    if (ppPossibleMatch)
    {
        ThrowIfNull(*ppPossibleMatch);
        return;
    }

    Insert(pAnonymousDelegate, pOwnerProc);
}

void TransientSymbolStore::StartTransaction()
{
    VSASSERT(!m_InTransaction, "Nested transactions are not supported.  Previous transaction must be committed or aborted before new can start.");
#if DEBUG
    m_InTransaction = true;
#endif
    m_pStartTransaction = m_SymbolList.GetLast();

    if (m_pAnonymousTypeHash)
    {
        m_pAnonymousTypeHash->CommitTransaction();
    }

    if (m_pAnonymousDelegateHash)
    {
        m_pAnonymousDelegateHash->CommitTransaction();
    }

    if (m_pAnonDelegateToTransientSymbolMap)
    {
        m_pAnonDelegateToTransientSymbolMap->CommitTransaction();
    }

    if (m_pClosureMethodStubHash)
    {
        m_pClosureMethodStubHash->CommitTransaction();
    }

    if (m_ResumableMethodToStateMachineAttrMap)
    {
        m_ResumableMethodToStateMachineAttrMap->CommitTransaction();
    }

    // Ensure any pending names are committed
    if (m_pAnonymousTypeNameGenerator)
    {
        m_pAnonymousTypeNameGenerator->CommitNewNames();
    }
    if (m_pAnonymousDelegateNameGenerator)
    {
        m_pAnonymousDelegateNameGenerator->CommitNewNames();
    }
    if (m_pStateMachineNameGenerator)
    {
        m_pStateMachineNameGenerator->CommitNewNames();
    }
}

void TransientSymbolStore::CommitTransaction()
{
    VSASSERT(m_InTransaction, "Transaction was never started.");
#if DEBUG
    m_InTransaction = false;
#endif
    m_pStartTransaction = NULL;

    if (m_pAnonymousTypeHash)
    {
        m_pAnonymousTypeHash->CommitTransaction();
    }

    if (m_pAnonymousDelegateHash)
    {
        m_pAnonymousDelegateHash->CommitTransaction();
    }

    if (m_pAnonDelegateToTransientSymbolMap)
    {
        m_pAnonDelegateToTransientSymbolMap->CommitTransaction();
    }

    if (m_ResumableMethodToStateMachineAttrMap)
    {
        m_ResumableMethodToStateMachineAttrMap->CommitTransaction();
    }

    if (m_pClosureMethodStubHash)
    {
        m_pClosureMethodStubHash->CommitTransaction();
    }

    if (m_pAnonymousTypeNameGenerator)
    {
        m_pAnonymousTypeNameGenerator->CommitNewNames();
    }

    if (m_pAnonymousDelegateNameGenerator)
    {
        m_pAnonymousDelegateNameGenerator->CommitNewNames();
    }

    if (m_pStateMachineNameGenerator)
    {
        m_pStateMachineNameGenerator->CommitNewNames();
    }
}

void TransientSymbolStore::AbortTransaction()
{
    VSASSERT(m_InTransaction, "Transaction was never started.");

    if (!m_pStartTransaction)
    {
        // Symbol list started out empty, so remove all symbols
        m_pStartTransaction = m_SymbolList.GetFirst();
    }
    else
    {
        // Remove all symbols after the starting mark
        m_pStartTransaction = m_pStartTransaction->Next();
    }

    if (m_pStartTransaction)
    {
        // Truncate end of list
        m_SymbolList.RemoveRange(m_pStartTransaction, m_SymbolList.GetLast());
        m_pStartTransaction = NULL;
    }

    // Remove anonymous types from hash
    if (m_pAnonymousTypeHash)
    {
        m_pAnonymousTypeHash->AbortTransaction();
    }

    // Remove anonymous delegates from hash
    if (m_pAnonymousDelegateHash)
    {
        m_pAnonymousDelegateHash->AbortTransaction();
    }

    // Remove anonymous delegates mapping from hash
    if (m_pAnonDelegateToTransientSymbolMap)
    {
        m_pAnonDelegateToTransientSymbolMap->AbortTransaction();
    }

    if (m_ResumableMethodToStateMachineAttrMap)
    {
        m_ResumableMethodToStateMachineAttrMap->AbortTransaction();
    }

    // Remove stubs from the hash
    if (m_pClosureMethodStubHash)
    {
        m_pClosureMethodStubHash->AbortTransaction();
    }

    // Discard anonymous names
    if (m_pAnonymousTypeNameGenerator)
    {
        m_pAnonymousTypeNameGenerator->AbortNewNames();
    }

    if (m_pAnonymousDelegateNameGenerator)
    {
        m_pAnonymousDelegateNameGenerator->AbortNewNames();
    }

    if (m_pStateMachineNameGenerator)
    {
        m_pStateMachineNameGenerator->AbortNewNames();
    }

#if DEBUG
    m_InTransaction = false;
#endif
}
void TransientSymbolStore::StartMethod() {
    VSASSERT(!m_InMethod, "Nested transient methods are not supported.");
#if DEBUG
    m_InMethod = true;
#endif
    m_pStartMethod = m_SymbolList.GetLast();
}

void TransientSymbolStore::EndMethod()
{
    VSASSERT(m_InMethod, "TMethod was never started.");
#if DEBUG
    m_InMethod = false;
#endif
    m_pStartMethod = NULL;
}

CLinkRangeIter<TransientSymbol> TransientSymbolStore::TransactionSymbolIterator()
{
    VSASSERT(m_InTransaction, "Transaction was never started.");
    TransientSymbol *pStart;

    if (m_pStartTransaction)
    {
        pStart = m_pStartTransaction->Next();
    }
    else
    {
        pStart = m_SymbolList.GetFirst();
    }

    CLinkRangeIter<TransientSymbol> iter(pStart, m_SymbolList.GetLast());
    return iter;
}

CLinkRangeIter<TransientSymbol> TransientSymbolStore::MethodSymbolIterator()
{
    VSASSERT(m_InMethod, "Method was never started.");
    TransientSymbol *pStart;

    if (m_pStartMethod)
    {
        pStart = m_pStartMethod->Next();
    }
    else
    {
        pStart = m_SymbolList.GetFirst();
    }

    CLinkRangeIter<TransientSymbol> iter(pStart, m_SymbolList.GetLast());
    return iter;
}

AnonymousTypeHashTable *TransientSymbolStore::GetAnonymousTypeHash()
{
    if (!m_pAnonymousTypeHash)
    {
        m_pAnonymousTypeHash = new (*m_pAllocator) AnonymousTypeHashTable();
        m_pAnonymousTypeHash->Init(m_pAllocator);
    }
    return m_pAnonymousTypeHash;
}

unsigned
TransientSymbolStore::GetAnonymousTypeCount()
{
    if ( !m_pAnonymousTypeHash )
    {
        return 0;
    }

    return m_pAnonymousTypeHash->NumEntries();
}

AnonymousDelegateHashTable *TransientSymbolStore::GetAnonymousDelegateHash()
{
    if (!m_pAnonymousDelegateHash)
    {
        m_pAnonymousDelegateHash = new (*m_pAllocator) AnonymousDelegateHashTable();
        m_pAnonymousDelegateHash->Init(m_pAllocator);
    }
    return m_pAnonymousDelegateHash;
}

unsigned
TransientSymbolStore::GetAnonymousDelegateCount()
{
    if (!m_pAnonymousDelegateHash)
    {
        return 0;
    }

    return m_pAnonymousDelegateHash->NumEntries();
}

AnonDelegateToTransientSymbolMap *TransientSymbolStore::GetAnonDelegateToTransientSymbolMap()
{
    if (!m_pAnonDelegateToTransientSymbolMap)
    {
        m_pAnonDelegateToTransientSymbolMap = new (*m_pAllocator) AnonDelegateToTransientSymbolMap();
        m_pAnonDelegateToTransientSymbolMap->Init(m_pAllocator);
    }
    return m_pAnonDelegateToTransientSymbolMap;
}

ResumableMethodToStateMachineAttrMap* TransientSymbolStore::GetResumableMethodToStateMachineAttrMap()
{

    if (!m_ResumableMethodToStateMachineAttrMap)
    {
        m_ResumableMethodToStateMachineAttrMap = new (*m_pAllocator) ResumableMethodToStateMachineAttrMap();
        m_ResumableMethodToStateMachineAttrMap->Init(m_pAllocator);
    }
    return m_ResumableMethodToStateMachineAttrMap;
}


ClosureMethodStubHashTable *TransientSymbolStore::GetClosureMethodStubHash()
{
    if (!m_pClosureMethodStubHash)
    {
        m_pClosureMethodStubHash = new (*m_pAllocator) ClosureMethodStubHashTable();
        m_pClosureMethodStubHash->Init(m_pAllocator);
    }
    return m_pClosureMethodStubHash;
}

SequentialNameGenerator *TransientSymbolStore::GetAnonymousTypeNameGenerator()
{
    if (!m_pAnonymousTypeNameGenerator)
    {
        m_pAnonymousTypeNameGenerator = new (*m_pAllocator) SequentialNameGenerator(m_pCompiler, m_pAllocator);
        m_pAnonymousTypeNameGenerator->Init(VB_ANONYMOUSTYPE_PREFIX);
    }
    return m_pAnonymousTypeNameGenerator;
}

SequentialNameGenerator *TransientSymbolStore::GetAnonymousDelegateNameGenerator()
{
    if (!m_pAnonymousDelegateNameGenerator)
    {
        m_pAnonymousDelegateNameGenerator = new (*m_pAllocator) SequentialNameGenerator(m_pCompiler, m_pAllocator);
        m_pAnonymousDelegateNameGenerator->Init(VB_ANONYMOUSTYPE_DELEGATE_PREFIX);
    }
    return m_pAnonymousDelegateNameGenerator;
}

SequentialNameGenerator *TransientSymbolStore::GetStateMachineNameGenerator()
{
    if (!m_pStateMachineNameGenerator)
    {
        m_pStateMachineNameGenerator = new (*m_pAllocator) SequentialNameGenerator(m_pCompiler, m_pAllocator);
        m_pStateMachineNameGenerator->Init(VB_STATEMACHINE_PREFIX);
    }
    return m_pStateMachineNameGenerator;
}

Scope *TransientSymbolStore::GetXmlNamespaces()
{
    return m_XmlNamespaces;
}

void TransientSymbolStore::SetXmlNamespaces(Scope *pXmlNamespaces)
{
    m_XmlNamespaces = pXmlNamespaces;
}
