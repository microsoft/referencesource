//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  TemporaryManager class -- function and statement temps.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"


// Array used to identify by a unique name the temps used
extern const WCHAR *s_wszVOfVtype[];

TemporaryManager::TemporaryManager(
    Compiler *Compiler,
    CompilerHost *CompilerHost,
    BCSYM_Proc *ContainingProcedure,
    _In_ NorlsAllocator *TempAllocator,
    _In_opt_z_ const WCHAR *ExtraPrefix
#if IDE
    , bool IsLambdaTemporaryManager
#endif IDE
) :
    m_SymbolAllocator(Compiler, TempAllocator, NULL)
{
    m_Compiler = Compiler;
    m_CompilerHost = CompilerHost;
    m_ContainingProcedure = ContainingProcedure;
    m_TempAllocator = TempAllocator;
    m_ExtraPrefix = ExtraPrefix;

#if IDE
    m_IsLambdaTemporaryManager = IsLambdaTemporaryManager;
#endif IDE
}

TemporaryManager::TemporaryManager(_In_ TemporaryManager *src, DynamicHashTable<Temporary*,Temporary*> *copyMap) :
    m_Compiler(src->m_Compiler),
    m_CompilerHost(src->m_CompilerHost),
    m_TempAllocator(src->m_TempAllocator),
    m_Temporaries(NULL),
    m_ContainingProcedure(src->m_ContainingProcedure),
    m_SymbolAllocator(m_Compiler,m_TempAllocator, NULL),
    m_ExtraPrefix(src->m_ExtraPrefix)
#if IDE
     , m_IsLambdaTemporaryManager(src->m_IsLambdaTemporaryManager)
#endif
{
    CopyTemporaries(src, copyMap);
}

void TemporaryManager::CopyTemporaries(_In_ TemporaryManager *src, _Inout_opt_ DynamicHashTable<Temporary*,Temporary*> *copyMap)
{
    TemporaryIterator it;
    it.Init(src);

    Temporary *cur;
    while (cur=it.GetNext())
    {
        BCSYM_Variable *var = this->DoAllocateTemporary(
                cur->Symbol->GetName(),
                cur->Symbol->GetType(),
                cur->Lifetime,
                cur->Symbol->HasLocation() ? cur->Symbol->GetLocation() : NULL,
                cur->Block);
        if ( copyMap )
        {
            copyMap->SetValue(cur, var->GetTemporaryInfo());
        }
    }
}

/**********
*TemporaryManager::FreeShortLivedTemporaries
*       Resets statement temp after a single statement
***********/
void
TemporaryManager::FreeShortLivedTemporaries()
{
    Temporary *CurrentTemporary = m_Temporaries;

    while (CurrentTemporary)
    {
        if (CurrentTemporary->Lifetime == LifetimeShortLived)
        {
            CurrentTemporary->IsCurrentlyInUse = false;
        }
        CurrentTemporary = CurrentTemporary->Next;
    }
}
/**********
*TemporaryManager::LockExistingShortLivedTempsForNewPasss
*       Locks all the shortlived temps created so far. This will make sure the new fresh temps will be created during the next
*       semantic pass and no collision could hapen.
*       Save the old prefix and set the new one.
***********/
void
TemporaryManager::LockExistingShortLivedTempsForNewPass(_In_opt_z_ const WCHAR* NewPrefix, _Out_ const WCHAR *&ppwszOldPrefix)
{
    ppwszOldPrefix = m_ExtraPrefix;
    m_ExtraPrefix = NewPrefix;

    Temporary *CurrentTemporary = m_Temporaries;

    while (CurrentTemporary)
    {
        if (CurrentTemporary->Lifetime == LifetimeShortLived)
        {
            //CurrentTemporary->IsCurrentlyInUse = true;
            CurrentTemporary->IsLocked = true;
        }
        CurrentTemporary = CurrentTemporary->Next;
    }
}
void
TemporaryManager::UnlockExistingShortLivedTemps(_In_opt_z_ const WCHAR* RestoredPrefix)
{
    m_ExtraPrefix =RestoredPrefix;

    Temporary *CurrentTemporary = m_Temporaries;

    while (CurrentTemporary)
    {
        if (CurrentTemporary->Lifetime == LifetimeShortLived)
        {
            //CurrentTemporary->IsCurrentlyInUse = true;
            CurrentTemporary->IsLocked = false;
        }
        CurrentTemporary = CurrentTemporary->Next;
    }
}

/**********
*TemporaryManager::FreeTemporary
*       Frees one temporary indicating that it is not required.
***********/
void
TemporaryManager::FreeTemporary
(
    BCSYM_Variable *TemporaryToDelete
)
{
    VSASSERT(TemporaryToDelete->IsTemporary(), "Temporary expected!!!");

    // Find the temporary and decrement its use count.
    for (Temporary *Cursor = m_Temporaries; Cursor; Cursor = Cursor->Next)
    {
        VSASSERT(Cursor->Symbol, "this temporary doesn't have a symbol!");

        if (Cursor->Symbol == TemporaryToDelete)
        {
            VSASSERT(Cursor->UseCount != 0, "Inconsistency in temporary variable allocations!!!");

            Cursor->UseCount--;
            if (Cursor->UseCount == 0)
            {
                Cursor->IsCurrentlyInUse = false;
            }

            break;
        }
    }
}

BCSYM_Variable *TemporaryManager::AllocateShortLivedTemporary(BCSYM *type, const Location *textSpan, _Out_opt_ bool *existingVariableReused )
{
    return AllocateTemporary(type, LifetimeShortLived, textSpan, NULL, existingVariableReused);
}

BCSYM_Variable *TemporaryManager::AllocateLongLivedTemporary(BCSYM *type, const Location *textSpan, ILTree::ExecutableBlock *block, _Out_opt_ bool *existingVariableReused )
{
    ThrowIfNull(block);
    return AllocateTemporary(type, LifetimeLongLived, textSpan, block, existingVariableReused);
}

BCSYM_Variable *TemporaryManager::AllocateLifetimeNoneTemporary(BCSYM *type, const Location *textSpan, _Out_opt_ bool *existingVariableReused )
{
    return AllocateTemporary(type, LifetimeNone, textSpan, NULL, existingVariableReused);
}

BCSYM_Variable *TemporaryManager::AllocateDefaultValueTemporary(BCSYM *type, const Location *textSpan)
{
    return AllocateTemporary(type, LifetimeDefaultValue, textSpan, NULL, NULL);
}

BCSYM_Variable *TemporaryManager::AllocateShortLivedTemporaryNoReuse(BCSYM * pType, const Location * pTextSpan)
{
    return
        DoAllocateTemporary(
            GetNextTemporaryName(pType, LifetimeShortLived, pTextSpan),
            pType,
            LifetimeShortLived,
            pTextSpan,
            NULL);
}

/**********
*TemporaryManager::AllocateTemporary
*       Allocs temporary, looking in free stmt temp if appropriate
***********/
BCSYM_Variable *
TemporaryManager::AllocateTemporary
(
    BCSYM *TypeRequested,
    LifetimeClass Lifetime,
    const Location *TextSpan,
    ILTree::ExecutableBlock *ExecutableBlock,
    _Out_opt_ bool *ExistingVariableReused
)
{
    ThrowIfFalse(LifetimeLongLived != Lifetime || ExecutableBlock);   // If it's a LongLived temporary a block must be provided
    ThrowIfFalse(LifetimeLongLived == Lifetime || !ExecutableBlock);  // Likewise, if it's not LongLived Blocks cannot be provided
    if (ExistingVariableReused)
    {
        *ExistingVariableReused = false;
    }

    // Long lived temporaries are never reused, so don't bother searching
    if (Lifetime != LifetimeLongLived)
    {
        // Search for the first free temporary that matches the Type requested
        for (Temporary *Cursor = m_Temporaries; Cursor; Cursor = Cursor->Next)
        {
            VSASSERT(Cursor->Symbol, "this temporary doesn't have a symbol!");

            if (!Cursor->IsCurrentlyInUse &&
                !Cursor->IsLocked &&
                Cursor->Lifetime == Lifetime &&
                BCSYM::CompareReferencedTypes(
                    TypeRequested->DigThroughNamedType(),
                    (GenericBinding *)NULL,
                    Cursor->Symbol->GetType()->DigThroughNamedType(),
                    (GenericBinding *)NULL,
                    &m_SymbolAllocator) == EQ_Match)
            {
                Cursor->UseCount++;

                // Temporaries with no lifetime are never "in use", i.e., they are always available from the pool.
                if (Cursor->Lifetime != LifetimeNone && Cursor->Lifetime != LifetimeDefaultValue)
                {
                    Cursor->IsCurrentlyInUse = true;
                }

                if (ExistingVariableReused)
                {
                    *ExistingVariableReused = true;
                }

                return Cursor->Symbol;
            }
        }
    }


    // If we get here, we didn't find a free temporary
    return
        DoAllocateTemporary(
            GetNextTemporaryName(TypeRequested, Lifetime, TextSpan),
            TypeRequested,
            Lifetime,
            TextSpan,
            ExecutableBlock);
}

/**********
 * Temporary &Temporarymanager::GetTemporary(Variable*)
 *  Return the Temporary struct for the specified temporary
***********/
Temporary *
TemporaryManager::GetTemporary(BCSYM_Variable *var)
{
    ThrowIfNull(var);
    ThrowIfFalse(var->IsTemporary());

    for (Temporary *cur = m_Temporaries; cur; cur = cur->Next)
    {
        if ( cur->Symbol == var )
        {
            return cur;
        }
    }

    return NULL;
}

/**********
*TemporaryManager::ChangeContainingProcedure
*       Changes the procedure containing the temporary manager.  Will also reset the
*       parent on all of the temporary variables
***********/
void
TemporaryManager::ChangeContainingProcedure(BCSYM_Proc *proc)
{
    ThrowIfNull(proc);
    if ( proc == m_ContainingProcedure )
    {
        return;
    }

    m_ContainingProcedure = proc;

    Temporary *temporary;
    TemporaryIterator tempIt;
    tempIt.Init(this, true);
    while ( (temporary = tempIt.GetNext()) )
    {
        Symbols::ClearParent(temporary->Symbol);
        Symbols::SetParent(temporary->Symbol, proc);
    }
}

/**********
*TemporaryManager::DoAllocateTemporary
*       Allocates memory for a temporary and inserts it into the list
***********/
BCSYM_Variable *
TemporaryManager::DoAllocateTemporary
(
    _In_z_ STRING *TemporaryName,
    BCSYM *TemporaryType,
    LifetimeClass Lifetime,
    const Location *TextSpan,
    ILTree::ExecutableBlock *Block
)
{
    Temporary *Return;

    Return = (Temporary *)m_TempAllocator->Alloc(sizeof(Temporary));
    Return->Symbol = m_SymbolAllocator.AllocVariable(TextSpan ? true : false, false);
    Return->Lifetime = Lifetime;
    Return->Block = Block;
    Return->IsLocked = false;

#if IDE 
    if (TemporaryType->DigThroughNamedType()->IsAnonymousType())
    {
        // Special case anonymous type since they are globally unique with no qualification.
        Return->VariableTypeName = TemporaryType->DigThroughNamedType()->PNamedRoot()->GetName();
    }
    else
    {
        StringBuffer sb;
        TemporaryType->GetBasicRep(m_Compiler,NULL,&sb);
        if (sb.GetStringLength() > 0)
        {
            Return->VariableTypeName = m_Compiler->AddString(sb.GetString());
        }
    }
#endif IDE 

    Return->UseCount = 1;

    // Temporaries with no lifetime are never "in use", i.e., they are always available from the pool.
    if (Return->Lifetime != LifetimeNone && Return->Lifetime != LifetimeDefaultValue)
    {
        Return->IsCurrentlyInUse = true;
    }

    m_SymbolAllocator.GetVariable(
        TextSpan,
        TemporaryName,
        TemporaryName,
        DECLF_Public,
        VAR_Local,
        TemporaryType,
        NULL,
        NULL,
        Return->Symbol);

    Return->Symbol->SetIsTemporary();

    Symbols::SetParent(Return->Symbol, m_ContainingProcedure);

    // Insert at beginning of list
    Return->Next = m_Temporaries;
    m_Temporaries = Return;

    // Set the temporary manager after inserting into the list because the set method will
    // assert that the temporary is actually apart of this manager
    Return->Symbol->SetTemporaryManager(this);

    return Return->Symbol;
}

/**********
*TemporaryManager::AllocateOnErrorTemporaries
*       Allocates the three temporaries used for On Error constructs.
*       Returns the three temporaries in an array.
***********/
void
TemporaryManager::AllocateOnErrorTemporaries
(
    _Out_ BCSYM_Variable *&ActiveHandlerTemporary,
    _Out_ BCSYM_Variable *&ResumeTargetTemporary
)
{
    ActiveHandlerTemporary =
        DoAllocateTemporary(
            m_Compiler->AddString(TEMP_LOCAL_ON_ERROR),
            m_CompilerHost->GetFXSymbolProvider()->GetType(t_i4),
            LifetimeLongLived);

    ResumeTargetTemporary =
        DoAllocateTemporary(
            m_Compiler->AddString(TEMP_LOCAL_RESUME),
            m_CompilerHost->GetFXSymbolProvider()->GetType(t_i4),
            LifetimeLongLived);

    return;
}

/**********
*TemporaryManager::AllocateCurrentStatementTemporary
*       Allocates the temporary used for maintained the Current Statement state.
*       Used only when a Resume statement is present.
***********/
BCSYM_Variable *
TemporaryManager::AllocateCurrentStatementTemporary()
{
    return
        DoAllocateTemporary(
            m_Compiler->AddString(TEMP_LOCAL_CUR_STMT),
            m_CompilerHost->GetFXSymbolProvider()->GetType(t_i4),
            LifetimeLongLived);
}

/**********
*TemporaryManager::AllocateCurrentLineTemporary
*       Allocates the temporary used for maintained the Current Line state.
*       Used only when a Line number and On Error are present.
***********/
BCSYM_Variable *
TemporaryManager::AllocateCurrentLineTemporary()
{
    return
        DoAllocateTemporary(
            m_Compiler->AddString(TEMP_LOCAL_CUR_LINE),
            m_CompilerHost->GetFXSymbolProvider()->GetType(t_i4),
            LifetimeLongLived);
}

/****************************
TemporaryManager::GetNextTemporaryName -- gets next temp name (_t1, etc.)
*****************************/
STRING *
TemporaryManager::GetNextTemporaryName
(
    Vtypes TemporaryVtype,
    LifetimeClass Lifetime
)
{
    if (TemporaryVtype >= 0 && TemporaryVtype < _countof(m_CountOfTemporariesOfVtype))
    {
        StringBuffer NameBuffer;
        WCHAR CharacterBuffer[32];  // VS Everett security bug #515087

        NameBuffer.AppendString(VBTemporaryPrefix);

        if (m_ExtraPrefix)
        {
            NameBuffer.AppendString(m_ExtraPrefix);
            NameBuffer.AppendChar(L'$');
        }

        NameBuffer.AppendString(s_wszVOfVtype[TemporaryVtype]);
        NameBuffer.AppendChar(L'$');
        NameBuffer.AppendChar(
            (Lifetime == LifetimeLongLived) ? L'L' : (Lifetime == LifetimeShortLived) ? L'S' : L'N');

        _ultow_s(m_CountOfTemporariesOfVtype[TemporaryVtype]++, CharacterBuffer, _countof(CharacterBuffer), 10);
        NameBuffer.AppendString(CharacterBuffer);

        return m_Compiler->AddString(&NameBuffer);
    }

    return NULL;
}

STRING *TemporaryManager::GetNextTemporaryName
(
    BCSYM *RequestedType,
    LifetimeClass Lifetime,
    const Location *TextSpan
)
{
    STRING *TempName = NULL;
#if IDE
    if (!GetCompilerSharedState()->IsInBackgroundThread() &&
        !m_IsLambdaTemporaryManager &&      // No ENC for lambdas
        m_ContainingProcedure &&
        m_ContainingProcedure->GetSourceFile() &&
        m_ContainingProcedure->GetSourceFile()->GetProject() &&
        m_ContainingProcedure->GetSourceFile()->GetProject()->GetENCBuilder())
    {
        TempName = m_ContainingProcedure->GetSourceFile()->GetCachedTemporaryName(m_ContainingProcedure, RequestedType, Lifetime, TextSpan);
    }
#endif IDE
    if (!TempName)
    {
        TempName = GetNextTemporaryName(RequestedType->GetVtype(), Lifetime);
#if IDE
        if (!GetCompilerSharedState()->IsInBackgroundThread() &&
            !m_IsLambdaTemporaryManager &&      // No ENC for lambdas
            m_ContainingProcedure &&
            m_ContainingProcedure->GetSourceFile() &&
            m_ContainingProcedure->GetSourceFile()->GetProject() &&
            m_ContainingProcedure->GetSourceFile()->GetProject()->GetENCBuilder())
        {
            // Make sure TempName doesn't collide with an existing cached temporary name.
            while (m_ContainingProcedure->GetSourceFile()->IsCachedTemporaryName(m_ContainingProcedure, TempName))
            {
                TempName = GetNextTemporaryName(RequestedType->GetVtype(), Lifetime);
            }

            m_ContainingProcedure->GetSourceFile()->CacheNewTemporaryName(m_ContainingProcedure, TempName);
        }
#endif IDE
    }
    return TempName;
}

/****************************
TemporaryIterator::Init -- init iterator over temps in TEMPMGR
*****************************/
void
TemporaryIterator::Init
(
    _In_ TemporaryManager *TemporaryManagerInstance,
    bool fIncludeUnusedTemporaries
)
{
    Temporary *Temp = TemporaryManagerInstance->m_Temporaries;
    while (Temp)
    {
        if (fIncludeUnusedTemporaries || Temp->UseCount > 0)
        {
            Temporaries.Add() = Temp;
        }
        Temp = Temp->Next;
    }
    Index = Temporaries.Count();
}

/****************************
TemporaryIterator::GetNext --
*****************************/
Temporary *
TemporaryIterator::GetNext()
{
    Temporary *Return = NULL;

    if (Index > 0)
    {
        Return = Temporaries.Element(Index - 1);
        --Index;
    }

    return Return;
}
