//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Header for TemporaryManager class -- function and statement temps.
//
//-------------------------------------------------------------------------------------------------

#pragma once
class BILALLOC;

#define VBOldTemporaryPrefix        L"_Vb_"
#define VBOldTemporaryPrefixLength  4
#define VBTemporaryPrefix           L"VB$"
#define VBTemporaryPrefixLength     3

#define TEMP_LOCAL_ON_ERROR         VBTemporaryPrefix L"ActiveHandler"
#define TEMP_LOCAL_RESUME           VBTemporaryPrefix L"ResumeTarget"
#define TEMP_LOCAL_CUR_LINE         VBTemporaryPrefix L"CurrentLine"
#define TEMP_LOCAL_CUR_STMT         VBTemporaryPrefix L"CurrentStatement"

enum LifetimeClass
{
    LifetimeDefaultValue,// The temporary variable always available for conversions 'Nothing' to struct. 
                         // IT SHOULDN'T BE USED FOR ANY OTHER PURPOSE, but CType(Nothing, <value type>).
                         
    LifetimeNone,        // The temporary variable is always available.
    LifetimeShortLived,  // The temporary variable has a per-statement lifetime.
    LifetimeLongLived    // The temporary variable lives for the entire function.
};

struct Temporary
{
public:
    Temporary *Next;
    BCSYM_Variable *Symbol;
    LifetimeClass Lifetime;
    bool IsCurrentlyInUse;      // Is this temporary currently being used ?
    bool IsLocked;                // Is this shortlived temp locked?(used to separates semantic passes)
    unsigned UseCount;          // The number of times this temporary has ever been used.
    ILTree::ExecutableBlock *Block;   // For LongLived temporaries, the block they are associated with
#if IDE 
    ILTree::ILNode *ValueForTemporary;
    //EnC needs to cache temp local variables so we are able to keep their slot numbers
    //when we build delta IL. The type pointer in Symbol will be invalid after decompiling
    STRING *VariableTypeName;
#endif
};

//-------------------------------------------------------------------------------------------------
//
// Manages temporaries for a function.
//
class TemporaryManager : ZeroInit<TemporaryManager>
{
    friend class TemporaryIterator;

public:
    TemporaryManager
    (
        Compiler *Compiler,
        CompilerHost *pCompilerHost,
        BCSYM_Proc *ContainingProcedure,
        _In_ NorlsAllocator *TempAllocator,
        _In_opt_z_ const WCHAR *ExtraPrefix = NULL
#if IDE
        , bool IsLambdaTemporaryManager = false
#endif IDE
    );

    // Copy constructors.  Useful for xCopyBilTree when doing a deep copy of a
    // lambda expression
    TemporaryManager(_In_ TemporaryManager *src, DynamicHashTable<Temporary*,Temporary*> *copyMap=NULL);

    void FreeShortLivedTemporaries();
    void FreeTemporary(BCSYM_Variable *TemporaryToDelete);

    BCSYM_Variable *AllocateShortLivedTemporary(BCSYM *type, const Location *textSpan, _Out_opt_ bool *existingVariableReused = NULL);
    BCSYM_Variable *AllocateShortLivedTemporaryNoReuse(BCSYM *type, const Location *textSpan);
    BCSYM_Variable *AllocateLongLivedTemporary(BCSYM *type, const Location *textSpan, ILTree::ExecutableBlock *block, _Out_opt_ bool *existingVariableReused = NULL);
    BCSYM_Variable *AllocateLifetimeNoneTemporary(BCSYM *type, const Location *textSpan, _Out_opt_ bool *existingVariableReused = NULL);
    BCSYM_Variable *AllocateDefaultValueTemporary(BCSYM *type, const Location *textSpan);

    void AllocateOnErrorTemporaries(_Out_ BCSYM_Variable *&ActiveHandlerTemporary, _Out_ BCSYM_Variable *&ResumeTargetTemporary);
    BCSYM_Variable *AllocateCurrentStatementTemporary();
    BCSYM_Variable *AllocateCurrentLineTemporary();
    Temporary *GetTemporary(BCSYM_Variable*);
    void ChangeContainingProcedure(BCSYM_Proc *);
    BCSYM_Proc *GetContainingProcedure() const { return m_ContainingProcedure; }
    void LockExistingShortLivedTempsForNewPass(_In_opt_z_ const WCHAR* NewPrefix, _Out_ const WCHAR *&ppwszOldPrefix);
    void UnlockExistingShortLivedTemps(_In_opt_z_ const WCHAR* RestoredPrefix);

private:

    void CopyTemporaries(_In_ TemporaryManager*, _Inout_opt_ DynamicHashTable<Temporary*,Temporary*> *copyMap);
    BCSYM_Variable *AllocateTemporary(BCSYM *RequestedType, LifetimeClass Lifetime, const Location *TextSpan, ILTree::ExecutableBlock *block, _Out_opt_ bool *ExistingVariableReused = NULL);
    BCSYM_Variable *DoAllocateTemporary(_In_z_ STRING *TemporaryName, BCSYM *TemporaryType, LifetimeClass Lifetime, const Location *TextSpan = NULL, ILTree::ExecutableBlock *block = NULL);

//***********************************************************
//*** Data members
//***********************************************************

    Compiler         * m_Compiler;
    CompilerHost     * m_CompilerHost;
    NorlsAllocator   * m_TempAllocator;
    Temporary        * m_Temporaries;
    BCSYM_Proc       * m_ContainingProcedure;
    Symbols            m_SymbolAllocator;
    unsigned long      m_CountOfTemporariesOfVtype[t_max];
    const WCHAR      * m_ExtraPrefix;   // If not NULL, will be appended to the end of the default prefix.

#if IDE
    bool               m_IsLambdaTemporaryManager;
#endif IDE

//***********************************************************
//*** Private methods
//***********************************************************
    STRING *GetNextTemporaryName(Vtypes TemporaryVtype, LifetimeClass Lifetime);
    STRING *GetNextTemporaryName(BCSYM *RequestedType, LifetimeClass Lifetime, const Location *TextSpan);
};

//NOTE:Microsoft,6/16/2003,we use old temp name prefix because we may debug a PE from VB7.1/7.0
inline bool
HasTemporaryName(BCSYM_NamedRoot *Candidiate)
{
    return CompareCaseN(Candidiate->GetName(), VBTemporaryPrefix, VBTemporaryPrefixLength) == 0 ||
           CompareCaseN(Candidiate->GetName(), VBOldTemporaryPrefix, VBOldTemporaryPrefixLength) == 0;
}

inline bool
HasTemporaryName( _In_ const StringPoolEntry& entry)
{
    return CompareCaseN(entry.GetData(), VBTemporaryPrefix, VBTemporaryPrefixLength) == 0 ||
           CompareCaseN(entry.GetData(), VBOldTemporaryPrefix, VBOldTemporaryPrefixLength) == 0;
}

// *****************************************
// class TemporaryIterator -- iterates over all Temporaries for a function
// *****************************************
class TemporaryIterator
{
public:

    void Init(_In_ TemporaryManager *TemporaryManagerInstance, bool fIncludeUnusedTemporaries = false);
    Temporary *GetNext();

private:
    DynamicArray<Temporary *> Temporaries;
    ULONG Index;
};
