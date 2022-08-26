//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Cache for symbols created late in the compilation process
//
//  Transient Symbols (TS) are wrappers for symbols created after CS_BoundSymbols
//  is completed. As new symbols are created late in the compilation they usually
//  need to be wired into existing data structures, whose lifetime extends beyond
//  that of the new symbols. During decompilation we need to cleanup any dangling
//  references to these new symbols, which is where the TS comes in. It manages a
//  BCSYM* representing the newly created symbol, carries function pointers used
//  to connect & disconnect the new symbol from the existing tree, and a blob of
//  data used to do the right connections/disconnections. Each SourceFile carries
//  a HashTable per CompilationState of TS that were created & registered, and
//  when decompilation happens it will automatically do the cleanup. For details
//  on how to register/unregister a TS with a SourceFile see RegisterSymbol*.
//
//-------------------------------------------------------------------------------------------------

#pragma once

struct TransientSymbol : CSingleLink<TransientSymbol>
{
    BCSYM_NamedRoot *m_pTransient;
    BCSYM_Proc *m_pOwnerProc;
};

#define HASH_TABLE_SIZE     256

template<class Key, class Value>
class TransactedHashTable : public FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value>
{
public:
    TransactedHashTable() : 
      m_pnra(NULL), 
          m_pUncommited(NULL)
    {
    }

    void Init(_In_ NorlsAllocator *pnra)
    {
        m_pnra = pnra;
        m_pUncommited = NULL;
        FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value>::Init(pnra);
    }


    void TransactHashAdd(
        const Key &key,
        _In_ Value &val)
    {
        if (!m_pnra)
        {
            return;
        }

        FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value>::HashAdd(key, val);

        if (!m_pUncommited)
        {
            m_pUncommited = (FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value> *)m_pnra->Alloc(sizeof(FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value>));
            m_pUncommited->Init(m_pnra);
        }
        m_pUncommited->HashAdd(key, val);
    }

    void CommitTransaction()
    {
        m_pUncommited = NULL;
    }


    bool IsUncommited(
        const Key &key,
        _In_opt_ Value * val)
    {
        return m_pUncommited && (m_pUncommited->HashFind(key, val) != NULL);
    }

    void AbortTransaction()
    {
        if (m_pUncommited)
        {
            Key *pKey;
            Value *pValue;
            m_pUncommited->IterateHashKeyAndValue(&pKey, &pValue);
            while (pKey && pValue)
            {
                FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value>::HashRemove(*pKey, pValue);
                m_pUncommited->IterateHashKeyAndValue(&pKey, &pValue);
            }
            m_pUncommited = NULL;
        }
    }

private:
    NorlsAllocator *m_pnra;
    FixedSizeHashTable<HASH_TABLE_SIZE, Key, Value> *m_pUncommited;
};

//==============================================================================
// Used to store a hash for Closure Stubs
//==============================================================================
struct ClosureStubHashKey : ZeroInit<ClosureStubHashKey>
{
    BCSYM_Proc *Target;
    BCSYM_Class *StubClass;
    bool IsMyBaseStub;  // If False, it's a MyClass Stub
};

typedef TransactedHashTable<size_t,BCSYM_Class*> AnonymousTypeHashTable;
typedef TransactedHashTable<size_t,BCSYM_Class*> AnonymousDelegateHashTable;
typedef TransactedHashTable<BCSYM_Class*, TransientSymbol*> AnonDelegateToTransientSymbolMap;
typedef TransactedHashTable<ClosureStubHashKey,BCSYM_Proc*> ClosureMethodStubHashTable;
typedef TransactedHashTable<BCSYM_Proc*,BCSYM_ApplAttr*> ResumableMethodToStateMachineAttrMap;

// 

class TransientSymbolStore
{
public:

    //---------------------------------------------------------------------------
    // Creation/Initialization
    //---------------------------------------------------------------------------

    TransientSymbolStore()
    {
        Init(NULL, NULL);
    }

    TransientSymbolStore(Compiler *pCompiler, NorlsAllocator *pAllocator)
    {
        Init(pCompiler, pAllocator);
    }

#if DEBUG
    ~TransientSymbolStore();
#endif

    void Init(Compiler *pCompiler, _In_ NorlsAllocator *pAllocator);


    //---------------------------------------------------------------------------
    // Generic Symbol Storage Manipulation
    //
    //   These methods allow manipulation of the list of transient symbols.  Care
    //   is taken to not directly expose the list, to prevent callers from
    //   attempting to add or remove directly through the list rather than
    //   through the accessor methods provided here.
    //---------------------------------------------------------------------------

    NorlsAllocator *SymbolStorage()
    {
        return m_pAllocator;
    }

    unsigned long SymbolCount()
    {
        return m_SymbolList.NumberOfEntries();
    }

    CSingleListIter<TransientSymbol> SymbolIterator()
    {
        CSingleListIter<TransientSymbol> iter(&m_SymbolList);
        return iter;
    }

    void Clear()
    {
        Init(m_pCompiler, m_pAllocator);
    }

    void Insert(BCSYM_NamedRoot *pNamedRoot, BCSYM_Proc *pOwnerProc);
    void InsertAnonymousDelegateIfNotAlreadyInserted( _In_ BCSYM_Class *pAnonymousDelegate, _In_ BCSYM_Proc *pOwnerProc);

    //---------------------------------------------------------------------------
    // Transaction Support
    //
    //   These methods allow a transaction context to be set up.  Changes to the
    //   symbol store are tracked during the transaction, and can later be
    //   committed or rolled back.  In addition, symbols created within the
    //   transaction scope can be iterated.
    //---------------------------------------------------------------------------

    // Keep track of transient symbols that are inserted, so that they can be removed if an abort is necessary
    void StartTransaction();
    void CommitTransaction();
    void AbortTransaction();
    void StartMethod();
    void EndMethod();
    CLinkRangeIter<TransientSymbol> TransactionSymbolIterator();
    CLinkRangeIter<TransientSymbol> MethodSymbolIterator(); // Iterates over the transient symbols generated by a method body


    //---------------------------------------------------------------------------
    // Transient Symbol Indexing Helpers
    //
    //   These methods are helpers which enable fast access to the transient
    //   symbols in this store.  Any time new indexing is added, AbortTransaction
    //   should be updated to enable rollback of the index.
    //---------------------------------------------------------------------------

    AnonymousTypeHashTable *GetAnonymousTypeHash();
    unsigned GetAnonymousTypeCount();

    AnonymousDelegateHashTable *GetAnonymousDelegateHash();
    unsigned GetAnonymousDelegateCount();

    ClosureMethodStubHashTable *GetClosureMethodStubHash();

    SequentialNameGenerator *GetAnonymousTypeNameGenerator();
    SequentialNameGenerator *GetAnonymousDelegateNameGenerator();
    SequentialNameGenerator *GetStateMachineNameGenerator();


    Scope *GetXmlNamespaces();
    void SetXmlNamespaces(Scope *pXmlnamespaces);

    BCSYM_Class *GetXmlHelperClass()
    {
        return m_pXmlHelperClass;
    }

    void SetXmlHelperClass(BCSYM_Class *pXmlHelperClass)
    {
        VSASSERT(!m_pXmlHelperClass, "Should check to see whether helper class has already been created before setting it");
        Insert(pXmlHelperClass, NULL);
        m_pXmlHelperClass = pXmlHelperClass;
    }

    unsigned GetClosureCount() { return m_closureCount.Value(); }
    unsigned IncrementClosureCount() { ++m_closureCount; return m_closureCount.Value(); }
    unsigned GetLambdaCount() { return m_lambdaCount.Value(); }
    unsigned IncrementLambdaCount() { ++m_lambdaCount; return m_lambdaCount.Value();}
    unsigned IncrementClosureVariableCount() { ++m_closureVariableCount; return m_closureVariableCount.Value();}

    ResumableMethodToStateMachineAttrMap *GetResumableMethodToStateMachineAttrMap();

private:
    // Do not auto-generate
    TransientSymbolStore( const TransientSymbolStore& );
    TransientSymbolStore& operator=(const TransientSymbolStore&) ;

    AnonDelegateToTransientSymbolMap *GetAnonDelegateToTransientSymbolMap();

    Compiler *m_pCompiler;
    NorlsAllocator *m_pAllocator;
    CSingleList<TransientSymbol> m_SymbolList;
    TransientSymbol *m_pStartTransaction;
    TransientSymbol *m_pStartMethod;
    BCSYM_Class *m_pXmlHelperClass;
    AnonymousTypeHashTable *m_pAnonymousTypeHash;

    // The hash that is used to unify anonymous delegates across a
    // project.
    //
    // Note that an anonymous delegate might be in the m_pAnonymousDelegateHash,
    // but may not have been inserted into m_SymbolList in order to prevent
    // the delegate from being emitted to the final assembly.
    //      See Bug Devdiv 78381 for an example of such a scenario.
    //
    AnonymousDelegateHashTable *m_pAnonymousDelegateHash;


    // This hash is used to map anonymous delegates' BCSYM_Class symbols
    // to the corresponding transient symbol object that is used to
    // track it in the list represented by m_SymbolList.
    //
    // Note that an anonymous delegate might be in the m_pAnonymousDelegateHash,
    // but may not have been inserted into m_SymbolList in order to prevent
    // the delegate from being emitted to the final assembly. In this case,
    // this map returns NULL for the transient symbol.
    //      See Bug Devdiv 78381 for an example of such a scenario.
    //
    AnonDelegateToTransientSymbolMap *m_pAnonDelegateToTransientSymbolMap;


    // This hash is used to map resumable proc to state machine attribute.
    // It is not a good idea to attach the attribute to the proc directly, since 
    // the proc is created during binding and the attribute is created during 
    // semantics analyze, attaching the attribute to the proc requires "restoring"
    // the attribute list to bound during decompiling. 
    ResumableMethodToStateMachineAttrMap *m_ResumableMethodToStateMachineAttrMap;

    ClosureMethodStubHashTable *m_pClosureMethodStubHash;
    Scope * m_XmlNamespaces; // hash of all namespaces
    SequentialNameGenerator *m_pAnonymousTypeNameGenerator;                             // Allocates and tracks anonymous type suffix
    SequentialNameGenerator *m_pAnonymousDelegateNameGenerator;
    SequentialNameGenerator *m_pStateMachineNameGenerator;

    // Closure Counters
    SafeInt<unsigned> m_closureCount;
    SafeInt<unsigned> m_lambdaCount;
    SafeInt<unsigned> m_closureVariableCount;

#if DEBUG
    bool m_InTransaction;
    bool m_InMethod;
#endif
};
