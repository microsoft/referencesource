//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manages the parse and compile errors.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//****************************************************************************
// ErrorTable helper methods
//****************************************************************************

bool CompareCompileErrorCompilationSteps(const CompileError* first, const CompileError* second)
{
    VSASSERT(first && second, "invalid arguments");
    return first->m_step < second->m_step;
}

std::list<CompileError*>::iterator GetPositionOfLastNoStepError(_In_ std::list<CompileError*>& errors)
{
    errors.sort(CompareCompileErrorCompilationSteps);

    auto positionOfLastNoStepError = errors.begin();
    for(; positionOfLastNoStepError != errors.end(); ++positionOfLastNoStepError)
    {
        if ((*positionOfLastNoStepError)->m_step > CS_NoStep)
        {
            break;
        }
    }
    return positionOfLastNoStepError;
}

#if DEBUG
bool ContainsErrorAboveNoStep(const std::list<CompileError*> & errors)
{
    auto found = std::find_if(errors.begin(), errors.end(), [](CompileError* pError)
    {
        return pError->m_step > CS_NoStep;
    });
    return found != errors.end();
}
#endif DEBUG

//****************************************************************************
// CompileError
//****************************************************************************

//=============================================================================
// Constructor
//=============================================================================

CompileError::CompileError(
    Compiler * pCompiler,
    CompilerProject * pCompilerProject,
    _In_opt_z_ WCHAR * wszMessage,
    unsigned errid,
    Location * ploc,
    VSTASKPRIORITY Priority
    IDE_ARG(_In_opt_z_ const WCHAR *wszExtra)
    IDE_ARG(_In_opt_ SourceFileView * pSourceFileView))
    : m_wszMessage(wszMessage)
    , m_errid(errid)
    , m_step(CS_NoStep)
    , m_pCompiler(pCompiler)
    , m_pCompilerProject(pCompilerProject)
    , m_Priority(Priority)
{
    if (ploc)
    {
        memcpy(&m_loc, ploc, sizeof(Location));
    }

#if IDE 
    m_wszExtra = NULL;
    m_pSourceFileView = NULL;

    SetExtra(wszExtra);
    SetSourceFileView(pSourceFileView);
#endif

#if DEBUG

    //
    // Verify that we have a legal location.
    //

    if (m_loc.IsValid())
    {
        // Quick check, nothing should be negative.
        VSASSERT(m_loc.m_lBegLine >= 0, "Bad line number.");
        VSASSERT(m_loc.m_lEndLine >= 0, "Bad line number.");
        VSASSERT(m_loc.m_lBegColumn >= 0, "Bad line number.");
        VSASSERT(m_loc.m_lEndColumn >= 0, "Bad line number.");
    }
#endif

}

#if IDE 

void CompileError::SetExtra(_In_opt_z_ const WCHAR *wszExtra)
{
    if (m_wszExtra != NULL)
    {
        VBFree(m_wszExtra);
        m_wszExtra = NULL;
    }

    if (wszExtra)
    {
        size_t cbSize;
        m_wszExtra = RawStringBufferUtil::AllocateNonNullCharacterCount(wcslen(wszExtra), &cbSize);
        if (FAILED(StringCbCopy(m_wszExtra, cbSize, wszExtra)))
        {
            VBFree(m_wszExtra);
            m_wszExtra = NULL;
        }
    }
}

void CompileError::SetSourceFileView(SourceFileView * pSourceFileView)
{
    m_pSourceFileView = pSourceFileView;
    if (m_pSourceFileView != NULL &&
        m_pSourceFileView->GetCompilerTaskProvider() != NULL)
    {
        m_pSourceFileView->GetCompilerTaskProvider()->TaskListNeedsRefresh();
    }
}
#endif IDE

//=============================================================================
// Destructor
//=============================================================================

CompileError::~CompileError()
{
#if IDE 
    m_pSourceFileView = NULL;

    if (m_wszExtra != NULL)
    {
        VBFree(m_wszExtra);
        m_wszExtra = NULL;
    }

#endif

    if (m_wszMessage != NULL)
    {
        VBFree(m_wszMessage);
        m_wszMessage = NULL;
    }
}

ErrorTable::ErrorTable() :
    m_fIsTransient(false),
    m_pFirstDependency(NULL),
    m_pLastDependency(NULL)
#if IDE 
    , m_Zombied(false)
#endif
{
    Init(NULL, NULL, NULL);
}

//****************************************************************************
// ErrorTable
//****************************************************************************
ErrorTable::ErrorTable(
    Compiler * pCompiler,
    CompilerProject * pCompilerProject,
    LineMarkerTable * plmt)
  :
    m_fIsTransient(false),
    m_pFirstDependency(NULL),
    m_pLastDependency(NULL)
#if IDE 
    , m_Zombied(false)
#endif
{
    // After base class and member CTORs have been invoked, defer to Init
    Init(pCompiler, pCompilerProject, plmt);
}


//Creates a temporary replacement table for src.
//This is an empty error table with the same
//compile, project, and critical section. The line marker
//table is not propigated, however. This is because
//the temporary table is designed to either be
//dropped or merged into the table it was
//generated from.
ErrorTable::ErrorTable(const ErrorTable &src) :
    m_fIsTransient(true),
    m_pFirstDependency(NULL),
    m_pLastDependency(NULL)
#if IDE 
    , m_Zombied(src.m_Zombied)
#endif

{
    Init(src.m_pCompiler, src.m_pCompilerProject, NULL);
}

bool ErrorTable::IsTransient()
{
    return m_fIsTransient;
}

void ErrorTable::NotifyDependenciesOfDestruction()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    ErrorTableDependenciesIterator iterator(this);
    ErrorTableDependency * pCurrent = NULL;

    while (pCurrent = iterator.Next())
    {
        pCurrent->NotifyTableDestruction(this);
    }
}

void ErrorTable::NotifyDependenciesOfMerge(ErrorTable * pDestination)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    ErrorTableDependenciesIterator iterator(this);
    ErrorTableDependency * pCurrent = NULL;

    while (pCurrent = iterator.Next())
    {
        pCurrent->NotifyTableReplacement(this, pDestination);
    }
}

void ErrorTable::RemoveDependency(ErrorTableDependency * pDependency)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    VSASSERT(pDependency, "The pDependency parameter cannot be NULL!");
    if (pDependency)
    {
        if (m_pLastDependency == pDependency)
        {
            m_pLastDependency = pDependency->m_pPreviousDependency;
        }

        if (m_pFirstDependency == pDependency)
        {
            m_pFirstDependency = pDependency->m_pNextDependency;
        }

        if (pDependency->m_pNextDependency)
        {
            pDependency->m_pNextDependency->m_pPreviousDependency = pDependency->m_pPreviousDependency;
        }

        if (pDependency->m_pPreviousDependency)
        {
            pDependency->m_pPreviousDependency->m_pNextDependency = pDependency->m_pNextDependency;
        }

        pDependency->m_pPreviousDependency = pDependency->m_pNextDependency = NULL;
    }
}

void ErrorTable::RecordDependency(ErrorTableDependency * pDependency)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    VSASSERT(pDependency, "The pDependency parameter cannot be NULL!");
    if (pDependency)
    {
        VSASSERT(! (pDependency->m_pNextDependency), "The pDependency paramter should always have a NULL next pointer");
        VSASSERT(! (pDependency->m_pPreviousDependency), "The pDependency paramter should always have a NULL next pointer");

        if (!m_pFirstDependency)
        {
            m_pFirstDependency = pDependency;
        }

        if (m_pLastDependency)
        {
            m_pLastDependency->m_pNextDependency = pDependency;
            pDependency->m_pPreviousDependency = m_pLastDependency;
        }

        m_pLastDependency = pDependency;
        pDependency->m_ShouldBeRemovedFromErrorTable = true;
    }
}

void ErrorTable::ClearDependencies()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    ErrorTableDependenciesIterator iterator(this);

    ErrorTableDependency * dep = NULL;
    while ((dep = iterator.Next()))
    {
        dep->m_ShouldBeRemovedFromErrorTable = false;
    }

    m_pFirstDependency = m_pLastDependency = NULL;
}

void ErrorTable::SpliceDependencies(ErrorTable * pOtherErrorTable)
{

    VSASSERT(pOtherErrorTable, "The pOtherErrorTable property should not be null!");

    if (pOtherErrorTable)
    {
        CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
        if (m_pLastDependency)
        {
            m_pLastDependency->m_pNextDependency = pOtherErrorTable->m_pFirstDependency;
        }

        if (pOtherErrorTable->m_pFirstDependency)
        {
            pOtherErrorTable->m_pFirstDependency->m_pPreviousDependency = m_pLastDependency;
        }

        if (!m_pFirstDependency)
        {
            m_pFirstDependency = pOtherErrorTable->m_pFirstDependency;
        }

        if (pOtherErrorTable->m_pLastDependency)
        {
            m_pLastDependency = pOtherErrorTable->m_pLastDependency;
        }
        pOtherErrorTable->m_pLastDependency = pOtherErrorTable->m_pFirstDependency = NULL;
    }
}

void ErrorTable::Init(
    Compiler * pCompiler,
    CompilerProject * pCompilerProject,
    LineMarkerTable * plmt)
{
    m_pCompiler = pCompiler;
    m_pCompilerProject = pCompilerProject;
#if IDE 
    m_pLineMarkerTable = plmt;
    m_pSourceFileView = NULL;
#endif
}


//****************************************************************************
// Functions to create errors.
//****************************************************************************

//=============================================================================
// Help create an error list.
//=============================================================================

void __cdecl ErrorTable::CreateError(
    ERRID errid,
    Location * ploc,
    ...)
{
    va_list ap;

    // Get stack pointer.
    va_start(ap, ploc);

    // Defer.
    CreateErrorArgs(errid, ploc, NULL, ap);
}

//=============================================================================
// Create an error that adds context to an existing error.
//=============================================================================

void __cdecl ErrorTable::CreateErrorWithError
(
    ERRID            errid,
    Location       * ploc,
    HRESULT          hrErr,
    ...
)
{
    va_list ap;
    HResultInfo hrInfo;

    // Get stack pointer.
    va_start(ap, hrErr);

    // Get the string for the error.
    hrInfo.Load(hrErr);

    // Defer.
    CreateErrorArgs(errid, ploc, hrInfo.GetHRINFO().m_bstrDescription, ap);
}


//=============================================================================
// Private helper for CreateErrorWithSymbol and CreateErrorWithSymbolAndError.
// Returns the Location where errors on psym should be reported.
//=============================================================================
static Location *GetErrorLocFromSymbol
(
    BCSYM *psym
)
{
    Location *ploc = NULL;

    // Get the location.
    if (psym)
    {
        if (psym->HasLocation())
        {
            ploc = psym->GetLocation();
        }
        else if (psym->IsArrayType())
        {
            return GetErrorLocFromSymbol(psym->PArrayType()->GetRawRoot());
        }
        else if (psym->IsPointerType())
        {
            return GetErrorLocFromSymbol(psym->PPointerType()->GetRawRoot()); // Dev10#498177
        }
    }

    return ploc;
}

//=============================================================================
// Create an error with a Symbol
//=============================================================================

void __cdecl ErrorTable::CreateErrorWithSymbol
(
    ERRID           errid,
    BCSYM         * psym,
    ...
)
{
    if (this == NULL)
    {
        return;
    }

    va_list ap;

    // Get stack pointer.
    va_start(ap, psym);

    // Defer.
    CreateErrorArgs(errid, GetErrorLocFromSymbol(psym), NULL, ap);
}

//=============================================================================
// Combination of CreateErrorWithError and CreateErrorWithSymbol.
//=============================================================================

void __cdecl ErrorTable::CreateErrorWithSymbolAndError
(
    ERRID           errid,
    BCSYM         * psym,
    HRESULT         hr,
    ...
)
{
    if (this == NULL)
    {
        return;
    }

    HResultInfo hrInfo;
    va_list ap;

    // Get stack pointer.
    va_start(ap, hr);

    // Get the string for the error.
    hrInfo.Load(hr);

    // Defer.
    CreateErrorArgs(
        errid, 
        GetErrorLocFromSymbol(psym), 
        hrInfo.GetHRINFO().m_bstrDescription,
        ap);
}

//=============================================================================
// Create an error with extra information and a location
//=============================================================================
void __cdecl ErrorTable::CreateErrorWithExtra
(
    ERRID errid,
    _In_opt_z_ const WCHAR *wszExtra,
    Location *ploc,
    ...
)
{
    va_list ap;

    // Get stack pointer.
    va_start(ap, ploc);

    CreateErrorArgsBase(errid, NULL, wszExtra, ploc, NULL, ap);
}

//=============================================================================
// Create an error with extra information and a location
//=============================================================================
void __cdecl ErrorTable::CreateErrorWithPartiallySubstitutedString
(
    ERRID errid,
    const WCHAR    * wszPartiallyReplString,
    Location *ploc,
    ...
)
{
    va_list ap;

    // Get stack pointer.
    va_start(ap, ploc);

    CreateErrorArgsBase(errid, wszPartiallyReplString, NULL, ploc, NULL, ap);
}

//=============================================================================
// Create an error list from another varargs helper.
//=============================================================================
void ErrorTable::CreateErrorArgs
(
    ERRID            errid,
    Location       * ploc,
    _In_opt_z_ WCHAR          * wszError,
    va_list          ap
)
{
    CreateErrorArgsBase(errid, NULL, NULL, ploc, wszError, ap);
}

//=============================================================================
// Base error creation method
//=============================================================================
void ErrorTable::CreateErrorArgsBase
(
    ERRID            errid,
    const WCHAR    * wszPartiallyReplString,
    _In_opt_z_ const WCHAR    * wszExtra,
    Location       * ploc,
    _In_opt_z_ WCHAR          * wszError,
    va_list          ap
)
{
    WCHAR           *wszMessage = NULL;
    StringBuffer sbuf;
    CompileError    *perror;

    if (ploc && 0)
    {
        ploc->DebCheck();
    }

    if (errid < WRNID_First ||
        m_pCompilerProject && m_pCompilerProject->GetWarningLevel(errid) != WARN_None)
    {
        // Create the error message.
        if (wszPartiallyReplString)
        {
            // If partially replaced string is present, use it and complete the substitutions.
            IfFailThrow(ResStringReplArgs((WCHAR *)wszPartiallyReplString, &sbuf, (WCHAR *)wszExtra, ap));
        }
        else
        {
            IfFailThrow(ResLoadStringReplArgs(errid, &sbuf, wszError, ap));
        }

        size_t bufferSize;
        wszMessage = RawStringBufferUtil::AllocateNonNullCharacterCount(sbuf.GetStringLength(), &bufferSize);
        memcpy(wszMessage, sbuf.GetString(), bufferSize);

        //
        // Create the error node.
        //

        VSTASKPRIORITY Priority = TP_NORMAL;
        if (ReportedAsError(errid,m_pCompilerProject))
        {
            Priority = TP_HIGH;
        }

        perror = new (zeromemory) CompileError(m_pCompiler,
                                               m_pCompilerProject,
                                               wszMessage,
                                               errid,
                                               ploc,
                                               Priority
                                               IDE_ARG(wszExtra)
                                               IDE_ARG(m_pCompilerProject ? NULL : m_pSourceFileView));

        CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
#if IDE
        if (FENCID(errid))
        {
            perror->m_step = CS_ENCRudeDetection;
        }
#endif
        m_Errors.push_back(perror);

        // After adding an error, the task list will need to be refreshed.
        // This call CANNOT be made while the error table is locked because of
        // possible deadlock with the IDE thread
        lock.Unlock();
        TaskListNeedsRefresh();
    }
}

//=============================================================================
// Extract the name from a symbol
//=============================================================================

WCHAR *ErrorTable::ExtractErrorName
(
    BCSYM *Substitution,
    BCSYM_Container *ContainingContext,
    StringBuffer &TextBuffer,
    BCSYM_GenericBinding *GenericBindingContext,
    bool FormatAsExtensionMethod,
    IReadonlyBitVector * FixedTypeArgumentBitVector,
    CompilerHost * CompilerHost
)
{
    Substitution = Substitution->DigThroughAlias();

    if (CompilerHost && TypeHelpers::IsNullableType(Substitution, CompilerHost))
    {
        Substitution->GetBasicRep(m_pCompiler, ContainingContext, &TextBuffer, GenericBindingContext);
        return TextBuffer.GetString();
    }

    if (Substitution->IsGenericBinding())
    {
        if (GenericBindingContext != NULL)
        {
            // Perf Impact:
            // An extremely rare error case. So okay to have an allocator here.

            NorlsAllocator TempAllocator(NORLSLOC);
            Symbols SymbolFactory(m_pCompiler, &TempAllocator, NULL);

            BCSYM_GenericBinding *ReplacedBinding =
                ReplaceGenericParametersWithArguments(
                    Substitution->PGenericBinding(),
                    GenericBindingContext,
                    SymbolFactory)->PGenericBinding();

            return ExtractErrorName(
                        ReplacedBinding->PNamedRoot(),
                        ContainingContext,
                        TextBuffer,
                        ReplacedBinding);
        }

        GenericBindingContext = Substitution->PGenericBinding();
        Substitution = Substitution->PGenericBinding()->GetGeneric();
    }

    if (Substitution->IsType() && Substitution->GetVtype() == t_array)
    {
        StringBuffer ElementTextBuffer;
        WCHAR RankSpelling[20];

        if (FAILED(StringCchPrintf(RankSpelling, DIM(RankSpelling), L"%d", Substitution->PArrayType()->GetRank())))
        {
            RankSpelling[0] = UCH_NULL;
        }

        HRESULT ArrayDescription =
            HrMakeRepl(
                ERRID_ArrayType2,
                RankSpelling,
                ExtractErrorName(Substitution->PArrayType()->GetRoot()->DigThroughAlias(), ContainingContext, ElementTextBuffer, GenericBindingContext));

        HRINFO ErrorInfo;
        GetHrInfo(ArrayDescription, &ErrorInfo);
        TextBuffer.AppendString(ErrorInfo.m_bstrDescription);
        ReleaseHrInfo(&ErrorInfo);

        return TextBuffer.GetString();
    }

    if (Substitution->IsProc())
    {
        // Turn a property method into the associated property.

        if ((Substitution->PProc()->IsPropertyGet() ||
            Substitution->PProc()->IsPropertySet()) &&
            Substitution->PProc()->GetAssociatedPropertyDef())
        {
            Substitution = Substitution->PProc()->GetAssociatedPropertyDef();
        }

        Substitution->GetBasicRep
        (
            m_pCompiler,
            ContainingContext,
            &TextBuffer,
            GenericBindingContext,
            NULL,
            true,
            FormatAsExtensionMethod ?
                TIP_ExtensionCall :
                TIP_Normal,
            FixedTypeArgumentBitVector
        );

        return TextBuffer.GetString();
    }

    if (Substitution->IsParam())
    {
        Substitution->GetBasicRep(m_pCompiler, ContainingContext, &TextBuffer, GenericBindingContext);
        return TextBuffer.GetString();
    }

    // Anonymous Types
    if (Substitution->IsAnonymousType())
    {
        if (GenericBindingContext && GenericBindingContext->HasLocation())
        {
            WCHAR Digits[MaxStringLengthForIntToStringConversion]; // e.g. long + trailing NULL
            IfFailThrow(StringCchPrintfW(Digits, DIM(Digits), L"%lu", GenericBindingContext->GetLocation()->m_lBegLine + 1));
            ResLoadStringRepl(STRID_AnonymousTypeWithLocation, &TextBuffer, Digits);
        }
        else
        {
            ResLoadStringRepl(STRID_AnonymousType, &TextBuffer);
        }

        return TextBuffer.GetString();
    }

     // Anonymous Delegates
    if (Substitution->IsAnonymousDelegate())
    {
        if (GenericBindingContext && GenericBindingContext->HasLocation())
        {
            WCHAR Digits[MaxStringLengthForIntToStringConversion]; // e.g. long + trailing NULL
            IfFailThrow(StringCchPrintfW(Digits, DIM(Digits), L"%lu", GenericBindingContext->GetLocation()->m_lBegLine + 1));
            ResLoadStringRepl(STRID_AnonymousDelegateWithLocation, &TextBuffer, Digits);
        }
        else
        {
            ResLoadStringRepl(STRID_AnonymousDelegate, &TextBuffer);
        }

        return TextBuffer.GetString();
    }

    // To avoid ambiguous error messages, report the fully qualified name if this is a container.
    // Also append the type parameters for generic types to distinguish between types overloaded
    // on arity.
    //
    if (Substitution->IsContainer() || Substitution->IsGeneric()  )
    {
        return Substitution->PContainer()->GetQualifiedName(
                    false,
                    NULL,
                    true,   // Append type parameters or type arguments
                    GenericBindingContext);
    }

    return Substitution->GetErrorName(m_pCompiler);
}


//=============================================================================
// Create a comment task item and link it directly to the comment list.
//=============================================================================

void ErrorTable::CreateComment
(
    Location *pLocation,
    _In_z_ WCHAR *wszText,
    VSTASKPRIORITY Priority
)
{
    CompileError    *perror;
    RawStringBuffer wszMessage;

#pragma prefast(suppress: 26018, "iswspace returns 0 fro the null character")
    while (iswspace(*wszText))
    {
        wszText++;
    }

    wszMessage.AllocateNonNullCharacterCount(wcslen(wszText));
    memcpy(wszMessage.GetData(), wszText, wszMessage.GetBufferByteSize());

    //
    // Create the error node.
    //

    perror = new (zeromemory) CompileError(m_pCompiler, m_pCompilerProject, wszMessage.Detach(), 0, pLocation, Priority);

    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    // Link it at the end of the nostep list.
    m_Errors.push_back(perror);

    // After adding an error, the task list will need to be refreshed.
    // This call CANNOT be made while the error table is locked because of
    // possible deadlock with the IDE thread
    lock.Unlock();
    TaskListNeedsRefresh();
}

bool ErrorTable::DeleteIf
(
    std::function<bool (CompileError*)> predicate,
    bool lockCriticalSectionAndRefreshTaskList /* = true */
)
{
    bool fTaskListNeedsRefresh = false;

    {
        RefCountedPtr<CompilerIdeTransientLock> spLock;
        if (lockCriticalSectionAndRefreshTaskList)
        {
            spLock = LockErrorTableOnStack() ;
        }

        // Need to store the removed items for deletion since invalidating the location
        // may affect the result of the passed in predicate
        std::list<CompileError*> removedItems;
        
        // Delete the line markers.
        m_Errors.remove_if( [&] (CompileError* pError) -> bool
        {
            if (predicate(pError))
            {
                pError->m_loc.Invalidate();
                removedItems.push_back(pError);
                fTaskListNeedsRefresh = true;
                return true;
            }
            return false;
        });

#if IDE 
        if (m_pLineMarkerTable)
        {
            m_pLineMarkerTable->RemoveInvalidErrorLocations();
        }
#endif IDE

        // Delete the elements themselves.
        std::for_each(removedItems.begin(), removedItems.end(), [] (CompileError* pError)
        {
            delete pError;
        });
    }

    if (lockCriticalSectionAndRefreshTaskList && fTaskListNeedsRefresh)
    {
        // After deleting errors, the task list will need to be refreshed.
        // This call CANNOT be made while the error table is locked because of
        // possible deadlock with the IDE thread
        TaskListNeedsRefresh();
    }

    return fTaskListNeedsRefresh;
}

//=============================================================================
// Delete all CompileError's and their corresponding LineMarker's.
//=============================================================================

void ErrorTable::DeleteAll()
{
    auto predicate = [](CompileError*) { return true; };
    DeleteIf(predicate);
}

//=============================================================================
// Delete all errors (and corresponding line markers) that have occurred in
// attempt to reach the given compilation state, or any state after this state.
//=============================================================================

void ErrorTable::DeleteAllWithThisStateOrHigher(CompilationState state)
{
    bool fTaskListNeedsRefresh = false;
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    // Delete the errors for all steps that are part of this state
    CompilationSteps step;
    for (step = CS_NoStep; step < CS_MAXSTEP; step = (CompilationSteps)(step + 1))
    {
        if (MapStepToState(step) >= state && step != CS_ENCRudeDetection)
        {
            if (DeleteAllWithStepNoLock(step))
            {
                fTaskListNeedsRefresh = true;
            }
        }
    }

    lock.Unlock();
    if (fTaskListNeedsRefresh)
    {
        // After deleting errors, the task list will need to be refreshed.
        // This call CANNOT be made while the error table is locked because of
        // possible deadlock with the IDE thread
        TaskListNeedsRefresh();
    }
}


//=============================================================================
// Delete all errors (and corresponding line markers) that have occurred in
// attempt to reach the given compilation step.
//=============================================================================

void ErrorTable::DeleteAllWithStep
(
    CompilationSteps step
)
{
    auto predicate = [=](CompileError* pError) { return pError->m_step == step; };
    DeleteIf(predicate);
}

#if IDE
//=============================================================================
// Delete all ENC(edit and continue) errors from ENCID_First to ENCID_Last.
//=============================================================================
void ErrorTable::DeleteAllENCErrors()
{
    auto predicate = [=](CompileError* pError) 
    { 
        return FENCID(pError->m_errid) && pError->m_step == CS_ENCRudeDetection; 
    };
    DeleteIf(predicate);
}

#endif

//=============================================================================
// Delete all instances of this specific error.
//=============================================================================

void ErrorTable::DeleteSpecificError
(
    ERRID errid,
    const Location * pWithinLocation
)
{
    auto predicate = [=](CompileError* pError) 
    { 
        return pError->m_errid == errid &&
            (pWithinLocation == NULL || pWithinLocation->ContainsInclusive(pError->m_loc));
    };
    DeleteIf(predicate);
}

//=============================================================================
// Does this table contain any errors?  Ignore comments.
//=============================================================================

bool ErrorTable::HasErrors()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    return HasErrorsNoLock();
}

bool ErrorTable::HasErrorsNoLock()
{
    auto found = std::find_if(m_Errors.begin(), m_Errors.end(), [this](CompileError* pError)
    {
        return ReportedAsError(pError->m_errid, GetProject());
    });

    return found != m_Errors.end();
}

//=============================================================================
// Get the total number of errors, again, ignore comments.
//=============================================================================

unsigned ErrorTable::GetErrorCount()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    unsigned errorCount = 0;

    std::for_each(m_Errors.begin(), m_Errors.end(), [&] (CompileError* pError)
    {
        if (ReportedAsError(pError->m_errid, GetProject()))
        {
            errorCount++;
        }
    });

    return errorCount;
}

//=============================================================================
// Does this table contain any Warnings?  Ignore comments.
//=============================================================================

bool ErrorTable::HasWarnings()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    auto found = std::find_if(m_Errors.begin(), m_Errors.end(), [this](CompileError* pError)
    {
        return pError->IsWarning() && !ReportedAsError(pError->m_errid, GetProject());
    });

    return found != m_Errors.end();
}

//=============================================================================
// Get the total number of warnings, again, ignore comments.
//=============================================================================

unsigned ErrorTable::GetWarningCount()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    unsigned warningCount = 0;

    std::for_each(m_Errors.begin(), m_Errors.end(), [&] (CompileError* pError)
    {
        if (pError->IsWarning() && !ReportedAsError(pError->m_errid, GetProject()))
        {
            warningCount++;
        }
    });

    return warningCount;
}

//=============================================================================
// Get the total number of comments.
//=============================================================================

bool ErrorTable::HasComments()
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    
    auto found = std::find_if(m_Errors.begin(), m_Errors.end(), [](CompileError* pError)
    {
        return pError->IsComment();
    });

    return found != m_Errors.end();
}

//=============================================================================
// Does this table have any errors below a specific step?
//=============================================================================

bool ErrorTable::HasErrorsThroughStep
(
    CompilationSteps step
)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    return HasErrorsThroughStepNoLock(step);
}

bool ErrorTable::HasErrorsThroughStepNoLock
(
    CompilationSteps step
)
{
    auto found = std::find_if(m_Errors.begin(), m_Errors.end(), [=](CompileError* pError)
    {
        return ReportedAsError(pError->m_errid, GetProject()) && pError->m_step <= step;
    });

    return found != m_Errors.end();
}

#if IDE 
bool ErrorTable::HasErrorsAtStep(CompilationSteps step)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    auto found = std::find_if(m_Errors.begin(), m_Errors.end(), [=](CompileError* pError)
    {
        return ReportedAsError(pError->m_errid, GetProject()) && pError->m_step == step;
    });

    return found != m_Errors.end();
}
#endif IDE

bool ErrorTable::HasThisErrorWithLocation(ERRID errid, const Location & loc)
{
    ErrorIterator ei(this);
    CompileError * pError = NULL;
    bool found = false;

    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    while (pError = ei.Next())
    {
        if
        (
            errid == pError->m_errid &&
            loc.IsEqual(&pError->m_loc)
        )
        {
            found = true;
            break;
        }
    }

    return found;
}

bool ErrorTable::HasThisErrorWithLocationAndParameters(
    ERRID errid,
    const Location * ploc,
    ...)
{
    ErrorIterator ei(this);
    CompileError * pError = NULL;
    bool found = false;

    va_list ap;

    // Get stack pointer.
    va_start(ap, ploc);


    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    while (pError = ei.Next())
    {
        if
        (
            errid == pError->m_errid &&
            ploc->IsEqual(&pError->m_loc)
        )
        {
            StringBuffer sbuf;

            IfFailThrow(ResLoadStringReplArgs(errid, &sbuf, NULL, ap));

            if (wcscmp(pError->m_wszMessage, sbuf.GetString()) == 0)
            {
                found = true;
                break;
            }
        }
    }

    return found;
}


// check if the table already has an error with the same errid and the same location as
// the given psym.
// Can solve most of multiple error reporting. But not all.
// Warning: The error messages are not really checked. Two errors with the same errid and location
//          but different arguments shows as identical.
//
bool ErrorTable::HasThisErrorWithSymLocation(ERRID errid, BCSYM *psym)
{
    VSASSERT(psym, "The provided symbol should not be null!");
    if (psym)
    {
        Location * pLoc = GetErrorLocFromSymbol(psym);
        if (pLoc)
        {
            return HasThisErrorWithLocation(errid, *pLoc);
        }
    }
    return false;
}

#if IDE 
bool ErrorTable::HasThisError(ERRID errid)
{
    ErrorIterator ei(this);
    CompileError * pError = NULL;
    bool found = false;

    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    while (pError = ei.Next())
    {
        if( errid == pError->m_errid)
        {
            found = true;
            break;
        }
    }

    return found;
}
#endif IDE


void ErrorTable::MergeTemporaryTable(ErrorTable * pErrorTable)
{

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!  DANGER    DANGER    DANGER   DANGER   DANGER   DANGER   DANGER   DANGER   DANGER   !!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    // Merging error tables is all about side effects and immutable data. Sometimes when we
    // invoke a function (e.g. MatchArguments), we want it to be a pure function that doesn't
    // alter the error table and doesn't alter the SX tree. One way to prevent side effects
    // is by disabling error-reporting (pushing "m_ReportErrors=false"). Another way to
    // prevent side-effects is by putting errors into a temporary error table. Another way
    // is to do an xBilCopyTree to copy part of the SX_tree so that any mutations we do on it
    // are just done on a temporary copy.
    // 
    // Merging error tables is about saving work: we experiment down one path immutably,
    // and experiment down another path immutably, and if it turns out that one path was
    // better then we adopt the effects it caused. (by merging its error tables).
    //
    // The trouble is that there's no real architecture in place to make sure that side-effect-free
    // on the error-table and on the tree to together.
    //
    // An example to consider: f({function(x)x}). This does overload resolution; each candidate overload
    // calls MatchArguments, which calls PassArgumentByval, which calls ConvertWithErrorChecking, 
    // which might in general report errors but is being supporessed. It calls ConvertArrayLiteralElement,
    // which calls ConvertUnboundLambda, which has the side-effect of modifying the SX_tree by
    // inserting "As Object" for the x so that the conversion can proceed. Once overload resolution
    // has happened and the types have been figured out, then conversion happens "for real" and this
    // time the error-reporting and SX_tree modifications are for real.
    //
    // If ever you're tempted to merge error tables, please make sure that you've thought holistically
    // about side-effects in the SX_ tree as well.
    //

    VSASSERT(pErrorTable, "Cannot merge a null error table");
    VSASSERT(!ContainsErrorAboveNoStep(pErrorTable->m_Errors), "Bad table to merge.");

    if (pErrorTable)
    {
        CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
        m_Errors.splice(m_Errors.end(), pErrorTable->m_Errors);
        pErrorTable->NotifyDependenciesOfMerge(this);
        if (IsTransient())
        {
            SpliceDependencies(pErrorTable);
        }
        else
        {
            pErrorTable->ClearDependencies();
        }
    }
}

void ErrorTable::MoveNoStepErrors
(
    ErrorTable *pErrorTable
)
{
    if (!pErrorTable)
    {
        return;
    }

    auto positionOfLastNoStepError = GetPositionOfLastNoStepError(__ref pErrorTable->m_Errors);

    m_Errors.splice(m_Errors.begin(), pErrorTable->m_Errors, pErrorTable->m_Errors.begin(), positionOfLastNoStepError);
}

//
// NOTE: This is a private function, so this method does not lock the error table
//       Its callers should if necessary
//
// This function returns true if any errors were merged into the table, false otherwise
//
bool ErrorTable::MergeNoStepErrors
(
    ErrorTable *pErrorTable,
    CompilationSteps stepTarget
)
{
#if IDE 
    CompilationState state = MapStepToState(stepTarget);
#endif IDE

    bool fMergedErrors = false;

    auto positionOfLastNoStepError = GetPositionOfLastNoStepError(__ref pErrorTable->m_Errors);

    // Only run the loop if there was an error at CS_NoStep
    if (positionOfLastNoStepError != pErrorTable->m_Errors.begin())
    {
        // Set the steps.
        std::for_each(pErrorTable->m_Errors.begin(), pErrorTable->m_Errors.end(), [&] (CompileError* pError)
        {
            if (pError->m_step == CS_NoStep)
            {
#if IDE 
                // Create the associated line marker
                if (m_pLineMarkerTable)
                {
                    m_pLineMarkerTable->AddErrorLocation(&pError->m_loc, state);
                }
#endif IDE

                fMergedErrors = true;
                pError->m_step = stepTarget;
            }
        });

        // Merge their list into ours
        m_Errors.splice(m_Errors.end(), pErrorTable->m_Errors, pErrorTable->m_Errors.begin(), positionOfLastNoStepError);
    }

    return fMergedErrors;
}

//=============================================================================
// Merge pErrorTable which contains errors while trying to reach csTarget
// compilation state. Existing errors in this table with that same state
// will be deleted. Merging essentially *moves* CompileError's from pErrorTable
// into this ErrorTable, while maintaining ascending order.
//=============================================================================

void ErrorTable::Merge
(
    ErrorTable *pErrorTable,
    CompilationSteps stepTarget,
    bool fPurgeExistingErrorsWithThisStep
)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    bool fTaskListNeedsRefresh = false;

#if IDE 
    CompilationState state = MapStepToState(stepTarget);
#endif

    // 




    // Delete all errors currently in our list that have occurred
    // in a previous attempt to reach csTarget compilation state
    //

    if (fPurgeExistingErrorsWithThisStep)
    {
        fTaskListNeedsRefresh = DeleteAllWithStepNoLock(stepTarget);
    }

    VSASSERT(!ContainsErrorAboveNoStep(pErrorTable->m_Errors), "Bad table to merge.");

    // Note Microsoft 2/28/01
    // The use of error tables in the compilation model assumes that compiling
    // a file generates all and only the errors for that file. That assumption
    // is incorrect during the MakeEvaluated step of Bindable, because
    // evaluating a constant expression in one file can trigger the evaluation
    // of a constant expression in another file, and the second evaluation can
    // have an error.
    //
    // This situation manifests itself in errors appearing in the NoStep list
    // of error tables into which merging occurs. I have mitigated the problem by
    // merging in the NoStep errors of the target error table as well as those of
    // the source error table. Consider investigating for a better solution.

    if (MergeNoStepErrors(pErrorTable, stepTarget))
    {
        fTaskListNeedsRefresh = true;
    }

#if IDE 
    fTaskListNeedsRefresh = fTaskListNeedsRefresh ||
                            !pErrorTable->m_Errors.empty();
#endif IDE

    MergeNoStepErrors(this, stepTarget);

    lock.Unlock();
    if (fTaskListNeedsRefresh)
    {
        // After merging errors, the task list will need to be refreshed.
        // This call CANNOT be made while the error table is locked because of
        // possible deadlock with the IDE thread
        TaskListNeedsRefresh();
    }
}

void ErrorTable::MergeNoStepErrors
(
    CompilationSteps stepTarget
)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    MergeNoStepErrors(this, stepTarget);
}

void UpdateErrorContext
(
    _In_ CompileError *pError,
    _In_z_ WCHAR *pstrCCW,
    _In_z_ WCHAR *wszCCMax
)
{
    StringBuffer sb, sbConstant;

    ERRID errid;

    // Figure out if we have a specific constant we can point out.  Do this by
    // wandering the "extent" of the message until we either reach the beginning/end
    // of the string or we hit some terminators ':'.
    //
    if (pError->m_loc.IsValid())
    {
        WCHAR *wszStart, *wszEnd, *wszItemStart, *wszItemEnd;

        VSASSERT(pError->m_loc.m_lBegLine == pError->m_loc.m_lEndLine, "Bad location.");
        VSASSERT(pError->m_loc.m_lBegColumn < (long)wcslen(pstrCCW), "Bad location");
        VSASSERT(pError->m_loc.m_lEndColumn < (long)wcslen(pstrCCW), "Bad location");
        IfFalseThrow(pError->m_loc.m_lBegLine >= 0 && pError->m_loc.m_lBegColumn >= 0 && pError->m_loc.m_lEndColumn >= 0);

        // Find the right line
        unsigned TargetLineDistance = pError->m_loc.m_lBegLine > 0 ? pError->m_loc.m_lBegLine  : 0;

#pragma prefast(push)
#pragma prefast(disable: 26001 26018, "Limits of string are correctly checked")
        while (TargetLineDistance && *pstrCCW)
        {
            if (*pstrCCW == '\r' || *pstrCCW == '\n')
            {
                TargetLineDistance--;
                if (*pstrCCW == '\r' && *(pstrCCW + 1) == '\n')
                    pstrCCW++;
            }

            pstrCCW++;
        }

        VSASSERT(!TargetLineDistance, "Bad location.");

        // Get the extent of the error.
        wszStart = pstrCCW + pError->m_loc.m_lBegColumn;

        if (wszStart > wszCCMax)
        {
            wszStart = wszCCMax;
        }

        wszItemStart = wszStart +1;

        if (pstrCCW + pError->m_loc.m_lEndColumn + 1 < wszCCMax)
        {
            wszEnd = wszItemEnd = pstrCCW + pError->m_loc.m_lEndColumn + 1;
        }
        else
        {
            wszEnd = wszItemEnd = wszCCMax;
        }


        // From the first character backwards to find the beginning.
        while (wszStart > pstrCCW && *wszStart != L'\r' && *wszStart != L'\n')
        {
            wszStart--;
        }

        // Adjust, if needed.
        if (*wszStart == L'\r' || *wszStart == L'\n')
        {
            wszStart++;
        }

        // From the last character, find the end.
        while (wszEnd < wszCCMax && *wszEnd != '\r' && *wszEnd != '\n')
        {
            wszEnd++;
        }

        // Save the string.
        //sbConstant.AppendWithLength(wszStart, wszEnd - wszStart);
        sbConstant.AppendWithLength(wszStart, wszItemStart - wszStart);
        sbConstant.AppendWithLength(L" ^^ ", 4);
        if (wszItemEnd > wszItemStart)
        {
            sbConstant.AppendWithLength(wszItemStart, wszItemEnd - wszItemStart);
        }
        sbConstant.AppendWithLength(L" ^^ ",4);
        sbConstant.AppendWithLength(wszItemEnd, wszEnd - wszItemEnd);
#pragma prefast(pop)
    }

    // Create the error message.
    if (sbConstant.GetStringLength())
    {
        errid = ERRID_ProjectCCError1;
    }
    else
    {
        errid = ERRID_ProjectCCError0;
    }

    IfFailThrow(ResLoadStringRepl(errid, &sb, pError->m_wszMessage, sbConstant.GetString()));

    // Free the old message.
    size_t bufferByteSize;
    VBFree(pError->m_wszMessage);
    pError->m_wszMessage = NULL;
    pError->m_wszMessage = RawStringBufferUtil::AllocateNonNullCharacterCount(
        sb.GetStringLength(),
        &bufferByteSize);


    // Copy it.
#pragma prefast(suppress: 26017, "cbMessage.Value() hasn't changed since the allocation");
    memcpy(pError->m_wszMessage, sb.GetString(), bufferByteSize);
}

//=============================================================================
// Merge the error table that contains the project-level conditional
// compilation errors into this one.  This does not set any steps
// or states but does munge the error message into something
// a bit more user-friendly.  This resulting table then has to be
// merged again into the main table using the above API.
//=============================================================================

void ErrorTable::MergeProjectLevelCCErrors
(
    ErrorTable *pErrorTable,
    _In_z_ STRING *pstrCC
)
{
    WCHAR *wszCCMax = pstrCC + StringPool::StringLength(pstrCC);

    // b158947. using ++pstrCC to iterate on the string and then  StringLength(pstrCC) in one of the asserts bellow
    // trigers an assert in StringLength when the pspelling signature is verified. StringLength() cannot work on a part
    // of the original pstrCC. Take the pstrCC in a WCHAR* pstrCCW and use wcslen().
    WCHAR *pstrCCW = pstrCC;

    bool fTaskListNeedsRefresh = false;
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());

    VSASSERT(!ContainsErrorAboveNoStep(pErrorTable->m_Errors), "Bad table to merge.");

    // Walk the list of errors, adding additional context to the message
    // without disturbing the error number.
    //
    std::for_each(pErrorTable->m_Errors.begin(), pErrorTable->m_Errors.end(), [&] (CompileError* pError)
    {
        UpdateErrorContext(pError, pstrCCW, wszCCMax);
    });

    fTaskListNeedsRefresh = !pErrorTable->m_Errors.empty();

    m_Errors.splice(m_Errors.begin(), pErrorTable->m_Errors);

    lock.Unlock();
    if (fTaskListNeedsRefresh)
    {
        // After merging errors, the task list will need to be refreshed.
        // This call CANNOT be made while the error table is locked because of
        // possible deadlock with the IDE thread
        TaskListNeedsRefresh();
    }
}

//=============================================================================
// Merge the error table that contains the project-level conditional
// compilation errors into this one.  This does not set any steps
// or states but does munge the error message into something
// a bit more user-friendly.  This resulting table then has to be
// merged again into the main table using the above API.
//=============================================================================

void ErrorTable::MergeProjectLevelImportErrors
(
    ErrorTable *pErrorTable
)
{
    CompilerIdeTransientLock lock(m_csErrorTable, GetTransientLockMode());
    bool fTaskListNeedsRefresh = false;

    VSASSERT(!ContainsErrorAboveNoStep(pErrorTable->m_Errors), "Bad table to merge.");

    // Walk the list of errors, adding additional context to the message
    // without disturbing the error number.
    //

    std::for_each(pErrorTable->m_Errors.begin(), pErrorTable->m_Errors.end(), [&] (CompileError* pError)
    {
        VSASSERT(!pError->IsComment(), "shouldn't have any comments in the project level imports errors");
        // Customized project-level import error. No need to modify.
        //
        if (pError->m_errid != WRNID_UndefinedOrEmpyProjectNamespaceOrClass2 && !pError->IsComment())
        {
            StringBuffer sbModifiedError;
            StringBuffer sbPartialImport;

            // Modify the error as follows:
            //      Error in project-level imports '|1' at '|2' : |3

            // HACK ALERT: Microsoft - For project level imports, the start line is the index corresponding
            // to the error causing imports in the project's imports list.
            //
            unsigned ImportIndex = pError->m_loc.m_lBegLine;

            WCHAR *ImportString = m_pCompilerProject->GetOriginalImportStringByIndex(ImportIndex);
            sbPartialImport.AppendWithLength(ImportString, pError->m_loc.m_lEndColumn + 1);


            IfFailThrow(
                ResLoadStringRepl(
                ERRID_GeneralProjectImportsError3,
                &sbModifiedError,
                ImportString,
                sbPartialImport.GetString(),
                pError->m_wszMessage));

            // Free the old message.
            size_t bufferByteSize;
            VBFree(pError->m_wszMessage);
            pError->m_wszMessage = NULL;
            pError->m_wszMessage = RawStringBufferUtil::AllocateNonNullCharacterCount(
                sbModifiedError.GetStringLength(),
                &bufferByteSize);

            // Copy it.
#pragma prefast(suppress: 26017, "cbMessage.Value() hasn't changed since the allocation");
            memcpy(pError->m_wszMessage, sbModifiedError.GetString(), bufferByteSize);

            fTaskListNeedsRefresh = true;
        }
    });
    
    m_Errors.splice(m_Errors.begin(), pErrorTable->m_Errors);

    lock.Unlock();
    if (fTaskListNeedsRefresh)
    {
        // After merging errors, the task list will need to be refreshed.
        // This call CANNOT be made while the error table is locked because of
        // possible deadlock with the IDE thread
        TaskListNeedsRefresh();
    }
}

//=============================================================================
// Delete all errors (and corresponding line markers) that have occurred in
// attempt to reach the given compilation step.
//
// This function returns true if any errors were deleted, false otherwise
//=============================================================================

bool ErrorTable::DeleteAllWithStepNoLock
(
    CompilationSteps step
)
{
    auto predicate = [=](CompileError* pError) { return pError->m_step == step; };
    bool fDeletedErrors = DeleteIf(predicate, false /* takeLock */);
    return fDeletedErrors;
}

void ErrorTable::TaskListNeedsRefresh()
{
#if IDE
    // Do nothing if we are zombied!

    if (m_Zombied)
    {
        return;
    }

    // Notify the compiler project that the task list needs to be udpated
    if (m_pCompilerProject != NULL &&
        m_pCompilerProject->GetCompilerTaskProvider() != NULL)
    {
        m_pCompilerProject->GetCompilerTaskProvider()->TaskListNeedsRefresh();
    }

    if (m_pSourceFileView != NULL &&
        m_pSourceFileView->GetCompilerTaskProvider() != NULL)
    {
        m_pSourceFileView->GetCompilerTaskProvider()->TaskListNeedsRefresh();
    }
#endif IDE
}

//**************************************************************************************
// The error table iterator.
//**************************************************************************************

CompileError *ErrorIterator::Next()
{
    CompileError *pReturn = NULL;

    if (!Initialized)
    {
        Initialized = true;
        m_nextError = m_pErrorTable->m_Errors.begin();
    }

    if (m_nextError != m_pErrorTable->m_Errors.end())
    {
        pReturn = *m_nextError;
        ++m_nextError;
    }

    return pReturn;
}

//****************************************************************************
// Helper to build an array of errors from a project.  This will append
// the errors to the end of the array.
//****************************************************************************

class BuildErrorArrayHelper
{
public:
    typedef DynamicArray<BCError>       ErrorArray;
    typedef DynamicArray<SourceFile*>   SourceFileArray;

    BuildErrorArrayHelper(
        ErrorArray*         pdaErrors,          // external error array to build
        SourceFileArray*    pdaFiles,           // external files array to build
        bool                ProcessErrors,      // should we process errors?
        bool                ProcessWarnings,    // should we process warnings?
        bool                ProcessComments,    // should we process comments?
#ifndef MAX_ERRORS_DISABLED
        UINT                MaxErrors,        // the maxiumum number of errors
        UINT                MaxWarnings,          // the maximun number of warnings
#endif
        CompilerProject     *pProject);  // project

#if IDE
    void AddErrorsToErrorArray(SourceFile* pSourceFile, DynamicArray<CompileError *>* pdaErrors, DynamicArray<const WCHAR *>* pdaErrorMessages);
    void AddWMEErrorsToErrorArray(SourceFile* pSourceFile, DynamicArray<CompileError *>* pdaErrors);
#endif IDE

    void AddErrorsToErrorArray(ErrorTable *pErrorTable, CompilerFile *pFile = NULL);

private:

    void AddError(
        unsigned            errid,
        DWORD               severity,
        SourceFile*         pSourceFile,
        const VbError*      pTemplate);

    bool TreatWarningsAsErrors(CompileError *  pError) const
    {
        return (m_pProject->GetWarningLevel(pError->m_errid) == WARN_AsError);
    }

#ifndef MAX_ERRORS_DISABLED
    bool CheckForMaxErrors() const { return (ProcessErrors() && (m_MaxErrors != 0) && (m_ErrorCount > m_MaxErrors)); }
    bool CheckForMaxWarnings() const { return (ProcessWarnings() && (m_MaxWarnings != 0) && (m_WarningCount > m_MaxWarnings)); }
#endif
    bool ProcessErrors() const { return m_ProcessErrors; }
    bool ProcessWarnings() const { return m_ProcessWarnings; }
    bool ProcessComments() const { return m_ProcessComments; }

    void UpdateErrorCounts(CompileError* pError, SourceFile* pSourceFile, VbError* pVbError);

    ErrorArray*                 m_pdaErrors;        // external error array that we're adding to
    SourceFileArray*            m_pdaFiles;         // external file array that we're adding to
    WARNING_LEVEL               m_WarningLevel;     // the warning level for the project context
    UINT                        m_ErrorCount;       // the current error count (can include warnings - when treated as errors)
    UINT                        m_WarningCount;     // the current warning count
    bool                        m_ProcessErrors;    // whether we're processing the error table's errors
    bool                        m_ProcessWarnings;  // whether we're processing the error table's warnings
    bool                        m_ProcessComments;  // whether we're processing the error table's comments
    bool                        m_AddedWarningAsError;  // did we add the 'warning treated as error' error
    CompilerProject*            m_pProject;             // project context, if any
#ifndef MAX_ERRORS_DISABLED
    UINT                        m_MaxErrors;          // the maximum number of errors allowed
    UINT                        m_MaxWarnings;    // the maximum number of warnings allowed
    bool                        m_AddedMaxError;        // did we add the 'max errors' error
    bool                        m_AddedMaxWarnings;     // did we add the 'max warnings' warning
#endif
};
BuildErrorArrayHelper::BuildErrorArrayHelper
(
    ErrorArray*         pdaErrors,
    SourceFileArray*    pdaFiles,
    bool                ProcessErrors,
    bool                ProcessWarnings,
    bool                ProcessComments,
#ifndef MAX_ERRORS_DISABLED
    UINT                MaxErrors,
    UINT                MaxWarnings,
#endif
    CompilerProject     *pProject
)
{
    m_pdaErrors = pdaErrors;
    m_pdaFiles = pdaFiles;
    m_WarningLevel = pProject->GetDefaultWarningLevel();
    m_ProcessErrors = ProcessErrors;
    m_ProcessWarnings = ProcessWarnings;
    m_ProcessComments = ProcessComments;
    m_ErrorCount = 0;
    m_WarningCount = 0;
    m_AddedWarningAsError = false;
    m_pProject = pProject;
#ifndef MAX_ERRORS_DISABLED
    m_MaxErrors = MaxErrors;
    m_MaxWarnings = MaxWarnings;
    m_AddedMaxError = false;
    m_AddedMaxWarnings = false;
#endif
}

#if IDE

void BuildErrorArrayHelper::AddErrorsToErrorArray
(
    SourceFile* pSourceFile,
    DynamicArray<CompileError *>* pdaErrors,
    DynamicArray<const WCHAR *>* pdaErrorMessages
)
{
    VSASSERT(pdaErrors->Count() == pdaErrorMessages->Count(), "Error array should be the same size as the error message array");

    STRING* pstrFileName = pSourceFile->IsSolutionExtension() ?
                                pSourceFile->GetSolutionExtension()->m_pstrName :
                                pSourceFile->GetFileName();

    for (ULONG i = 0; i < pdaErrors->Count(); i++)
    {
        CompileError* pError = pdaErrors->Array()[i];

        if (pError->IsComment() && !ProcessComments())
        {
            continue;
        }

        if (pError->IsWarning() && !ProcessWarnings())
        {
            continue;
        }

        if (pError->IsError() && !ProcessErrors())
        {
            continue;
        }

        if (m_pdaFiles)
        {
            m_pdaFiles->AddElement(pSourceFile);
        }

        // Add the error
        VbError& vbError = m_pdaErrors->Add();
        vbError.dwSeverity = pError->m_Priority;
        vbError.dwErrId = pError->m_errid;
        vbError.dwBeginLine = pError->m_loc.m_lBegLine + 1;
        vbError.dwEndLine = pError->m_loc.m_lEndLine + 1;
        vbError.dwBeginCol = pError->m_loc.m_lBegColumn + 1;
        vbError.dwEndCol = pError->m_loc.m_lEndColumn + 1;
        vbError.Description = ::SysAllocString(pdaErrorMessages->Array()[i]);
        vbError.ItemName = ::SysAllocString(pstrFileName);
        vbError.FileName = ::SysAllocString(pstrFileName);
        vbError.SourceText = ::SysAllocString(L"");

        UpdateErrorCounts(pError, pSourceFile, &vbError);
    }
}

void BuildErrorArrayHelper::AddWMEErrorsToErrorArray
(
    SourceFile* pSourceFile,
    DynamicArray<CompileError *>* pdaErrors
)
{
    STRING* pstrFileName = pSourceFile->IsSolutionExtension() ?
                                pSourceFile->GetSolutionExtension()->m_pstrName :
                                pSourceFile->GetFileName();

    for (ULONG i = 0; i < pdaErrors->Count(); i++)
    {
        CompileError* pError = pdaErrors->Array()[i];

        if (m_pdaFiles)
        {
            m_pdaFiles->AddElement(pSourceFile);
        }

        BCError& bcError = m_pdaErrors->Add();
        bcError.m_fIsWME = true;
        bcError.dwSeverity = pError->m_Priority;
        bcError.dwErrId = pError->m_errid;
        bcError.dwBeginLine = pError->m_loc.m_lBegLine + 1;
        bcError.dwEndLine = pError->m_loc.m_lEndLine + 1;
        bcError.dwBeginCol = pError->m_loc.m_lBegColumn + 1;
        bcError.dwEndCol = pError->m_loc.m_lEndColumn + 1;
        bcError.Description = ::SysAllocString(pError->m_wszMessage);
        bcError.ItemName = ::SysAllocString(pstrFileName);
        bcError.FileName = ::SysAllocString(pstrFileName);
        bcError.SourceText = ::SysAllocString(L"");
    }
}

#endif IDE

void BuildErrorArrayHelper::UpdateErrorCounts(CompileError* pError, SourceFile* pSourceFile, VbError* pVbError)
{
    // Increment the error count. Include warnings if they count as errors.
    // Increment the warning count

    if (pError->IsError())
    {
        m_ErrorCount++;
    }
    else if (pError->IsWarning())
    {
        if ( TreatWarningsAsErrors(pError))
        {
            m_ErrorCount++;
        }
        else
        {
            m_WarningCount++;
        }
    }

    // Check if we need to add the "Warning treated as error" error.
    if (pError->IsWarning() &&
        TreatWarningsAsErrors(pError) &&
        !m_AddedWarningAsError)
    {
        AddError(ERRID_WarningTreatedAsError, TP_HIGH, pSourceFile, pVbError);
        m_AddedWarningAsError = true;
        m_ErrorCount++;             // This counts as an error
    }

#ifndef MAX_ERRORS_DISABLED
    // Check if we've gone past MaxErrors
    if (!m_AddedMaxError && CheckForMaxErrors())
    {
        AddError(ERRID_MaximumNumberOfErrors, TP_HIGH, NULL, NULL);
        m_AddedMaxError = true;
        m_ProcessErrors = false;    // Don't process any any more errors
        m_ProcessWarnings = false;  // Don't process any more warnings
    }
    // Check if we've gone past MaxWarnings
    if (!m_AddedMaxWarnings && CheckForMaxWarnings())
    {
       AddError(WRNID_MaximumNumberOfWarnings, TP_NORMAL, NULL, NULL);
       m_AddedMaxWarnings = true;
       m_ProcessWarnings = false;  // Don't process any more warnings
    }
#endif
}

#if IDE
// helper class used in AddErrorsToErrorArray 
struct LocationComparer 
{
    bool operator() (const Location& lhs, const Location& rhs) const
    {
        return Location::Compare(lhs,rhs) < 0;
    }
};

#endif IDE

void BuildErrorArrayHelper::AddErrorsToErrorArray
(
    ErrorTable*     pErrorTable,
    CompilerFile*   pFile
)
{
    ErrorIterator   errors(pErrorTable);
    CompileError *  pError;
#if !IDE
    Text            text;
#endif !IDE
    SourceFile *    pSourceFile = NULL;
    CComBSTR            bstrDescription; 
    CComBSTR            bstrItem;
    CComBSTR            bstrFileName;
    CComBSTR            bstrText;

    RefCountedPtr<CompilerIdeTransientLock> spLock = pErrorTable->LockErrorTableOnStack();

    // Check if we have any work to do
    if (!(
        (ProcessErrors() && pErrorTable->HasErrors()) ||
        (ProcessWarnings() && pErrorTable->HasWarnings()) ||
        (ProcessComments() && pErrorTable->HasComments()) ))
    {
        return;
    }

    // We'll need to report the source file text
    if (pFile && pFile->IsSourceFile() && pFile->GetFileName())
    {
        pSourceFile = pFile->PSourceFile();

#if !IDE
        // Get the text if it is a source file.
        if (pSourceFile)
        {
            IfFailThrow(text.Init(pSourceFile));
        }
#endif
    }

    STRING *pstrFileName = NULL;

    if (pFile)
    {
        pstrFileName = pFile->IsSolutionExtension() ? pFile->GetSolutionExtension()->m_pstrName : pFile->GetFileName();
    }

#if IDE
    // used to avoid duplicating task list comments
    std::set<Location, LocationComparer> commentLocations; 
#endif

    // Examine each error in turn
    while (pError = errors.Next())
    {
        if (pError->IsComment() && !ProcessComments())
            continue;

        if (pError->IsWarning() && !ProcessWarnings())
            continue;

        if (pError->IsError() && !ProcessErrors())
            continue;

#if IDE
        // if pError->m_loc was already in commentLocations, the second
        // value in the pair returned by insert will be false
        if (pError->IsComment() && !commentLocations.insert( pError->m_loc ).second)
            continue;

        if (pError->m_pCompilerProject &&
            pError->m_pCompilerProject->GetCompilerTaskProvider() &&
            pSourceFile &&
            pError->m_step > pSourceFile->GetCompStep())
        {
            // Errors in the task list are stale, and we should not try to provide quick fixes yet.
            pError->m_pCompilerProject->GetCompilerTaskProvider()->SetErrorsAreStale();
        }
#endif

        // Add the error to the table.
        VbError *pVbError = &m_pdaErrors->Add();

        if (m_pdaFiles)
        {
            m_pdaFiles->Add() = pSourceFile;
        }

        // Save the error message.
        bstrDescription = pError->m_wszMessage;

        if (pstrFileName)
        {
            // Save the item name (filename)
            bstrItem = pstrFileName;

#if !IDE
            // Map the file naem.
            if (pFile->IsSourceFile() && !pFile->IsSolutionExtension())
            {
                if (pError->m_loc.IsValid())
                {
                    pstrFileName = pSourceFile->GetMappedFileName(pError->m_loc.m_lBegLine);
                }
            }
#endif !IDE

            // Save it.
            bstrFileName = pstrFileName;

            // We can't get the file text for the error in the IDE case since the error location might
            // be out of date (see the expected behavior in VSW#51592).  This doesn't hurt anything because
            // the IDE does not use this field.
#if !IDE
            // Save the text.
            if (pSourceFile && pError->m_loc.IsValid())
            {
                IfFailThrow(text.GetTextLineRange(pError->m_loc.m_lBegLine, pError->m_loc.m_lEndLine, &bstrText));
            }
#endif !IDE
        }

        // Fill in the info
        pVbError->dwSeverity = pError->m_Priority;
        pVbError->dwErrId = pError->m_errid;

        if (pError->m_loc.IsValid())
        {
#if !IDE
            if (pSourceFile)
            {
                bool debugAvail;
                pVbError->dwBeginLine = pSourceFile->GetMappedLineInfo( pError->m_loc.m_lBegLine, &debugAvail ) + 1;
                pVbError->dwEndLine = pSourceFile->GetMappedLineInfo( pError->m_loc.m_lEndLine, &debugAvail ) + 1;
                pVbError->dwBeginCol = pError->m_loc.m_lBegColumn + 1;
                pVbError->dwEndCol = pError->m_loc.m_lEndColumn + 1;
            }
            else
#endif !IDE
            {
                pVbError->dwBeginLine = pError->m_loc.m_lBegLine + 1;
                pVbError->dwEndLine = pError->m_loc.m_lEndLine + 1;
                pVbError->dwBeginCol = pError->m_loc.m_lBegColumn + 1;
                pVbError->dwEndCol = pError->m_loc.m_lEndColumn + 1;
            }
        }

#if DEBUG
        if (pVbError->dwBeginLine)
        {
            VSASSERT(pVbError->dwEndLine, "Bad error location.");
            VSASSERT(pVbError->dwBeginCol, "Bad error location.");
            VSASSERT(pVbError->dwEndCol, "Bad error location.");
        }
        else
        {
            VSASSERT(!pVbError->dwEndLine, "Bad error location.");
            VSASSERT(!pVbError->dwBeginCol, "Bad error location.");
            VSASSERT(!pVbError->dwEndCol, "Bad error location.");
        }
#endif DEBUG

        pVbError->Description = bstrDescription.Detach();
        pVbError->ItemName = bstrItem.Detach();
        pVbError->FileName = bstrFileName.Detach();
        pVbError->SourceText = bstrText.Detach();

        UpdateErrorCounts(pError, pSourceFile, pVbError);
    }
}


void BuildErrorArrayHelper::AddError
(
    unsigned        errid,
    DWORD           severity,
    SourceFile*     pSourceFile,
    const VbError*  pTemplate
)
{
    WCHAR   wszDescription[2*CCH_MAX_LOADSTRING];
    VbErrorWrapper tempError;
    VbError *pVbError = NULL;
    LPCWSTR pszOldDescription = NULL;

    // If there is an error template, copy all but the description.
    if (pTemplate)
    {
        tempError.Copy(pTemplate);

        if ( tempError.Description )
        {
            ::SysFreeString(tempError.Description);
            tempError.Description = NULL;
        }

        // Keep track of this so we can append it on the end
        pszOldDescription = pTemplate->Description;
    }

    // Get the new error description, id and severity
    StringBuffer sbDescription(wszDescription, sizeof(wszDescription)/sizeof(wszDescription[0]));
    IfFailThrow(ResLoadStringRepl(errid, &sbDescription, pszOldDescription));
    tempError.Description = ComUtil::SafeSysAllocString(sbDescription.GetString());
    tempError.dwErrId = errid;
    tempError.dwSeverity = severity;

    // Add the new error to the error array and the source file array
    pVbError = &m_pdaErrors->Add();
    if (m_pdaFiles)
    {
        m_pdaFiles->Add() = pSourceFile;
    }

    // Fill up the error item.
    tempError.Detach(pVbError);
}

#if IDE 
bool _VBPeekKeyMessages()
{
    MSG msg;
    return ::PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE) != 0;
}
#endif

void BuildErrorArray
(
    CompilerProject *pProject,
    DynamicArray<BCError> *pdaErrors,
    DynamicArray<SourceFile *> *pdaFiles
#ifndef MAX_ERRORS_DISABLED    
    , UINT MaxErrors
    , UINT MaxWarnings
#endif
)
{
    FileIterator iter(pProject);
    CompilerFile *pFile;
    BuildErrorArrayHelper helper(
        pdaErrors, 
        pdaFiles, 
        true, 
        true, 
        true, 
#ifndef MAX_ERRORS_DISABLED
        MaxErrors, 
        MaxWarnings,
#endif
        pProject);

    helper.AddErrorsToErrorArray(pProject->GetErrorTable());

    while (pFile = iter.Next())
    {

#if IDE 
        if (!(pFile->IsSourceFile()) || ((SourceFile *)pFile)->FShowErrorsInTaskList())
        {
            if (pFile->IsSourceFile() &&
                pFile->PSourceFile()->HasOutOfProcBuildErrors())
            {
                DynamicArray<CompileError *> daErrors;
                DynamicArray<const WCHAR *> daErrorMessages;
                DynamicArray<CompileError *> daWMEErrors;
                pFile->PSourceFile()->GetVisibleErrorsConsideringOutOfProcBuildErrors(&daErrors, &daErrorMessages, &daWMEErrors);

                helper.AddErrorsToErrorArray(pFile->PSourceFile(), &daErrors, &daErrorMessages);
                helper.AddWMEErrorsToErrorArray(pFile->PSourceFile(), &daWMEErrors);
            }
            else
            {
                helper.AddErrorsToErrorArray(pFile->GetErrorTable(), pFile);
            }
        }
#else
        helper.AddErrorsToErrorArray(pFile->GetErrorTable(), pFile);
#endif

#if IDE 
        if (!GetCompilerSharedState()->IsCompilingSpecificProject()) // Don't pump messages if this is a build command (VS320361)
        {
            if (GetCompilerSharedState()->IsRefreshingTaskList() && _VBPeekKeyMessages())
            {
                GetCompilerSharedState()->SetErrorsAborted(true);
                return;
            }
        }
#endif
    }

#if DEBUG
    unsigned i, c = pdaErrors->Count();
    BCError *rgError = pdaErrors->Array();

    for (i = 0; i < c; i++)
    {
        if (rgError[i].dwBeginLine)
        {
            VSASSERT(rgError[i].dwEndLine, "Bad error location.");
            VSASSERT(rgError[i].dwBeginCol, "Bad error location.");
            VSASSERT(rgError[i].dwEndCol, "Bad error location.");
        }
        else
        {
            VSASSERT(!rgError[i].dwEndLine, "Bad error location.");
            VSASSERT(!rgError[i].dwBeginCol, "Bad error location.");
            VSASSERT(!rgError[i].dwEndCol, "Bad error location.");
        }
    }
#endif DEBUG
}

#if IDE 
void BuildErrorArray
(
    SourceFileView *pSourceFileView,
    DynamicArray<BCError> *pdaErrors
#ifndef MAX_ERRORS_DISABLED     
    , UINT MaxErrors
#endif    
)
{
    if (pSourceFileView)
    {
        ErrorTable * pErrorTable = pSourceFileView->GetErrorTable();

        ErrorIterator   errors(pErrorTable);
        CompileError *  pError;

        CComBSTR bstrDescription;
        CComBSTR bstrItem;
        CComBSTR bstrFileName;
        CComBSTR bstrText;
        STRING * strFileNameCache = NULL;

        RefCountedPtr<CompilerIdeTransientLock> spLock = pErrorTable->LockErrorTableOnStack();

        // Check if we have any work to do
        if (!(pErrorTable->HasErrors() || pErrorTable->HasWarnings() || pErrorTable->HasComments()))
        {
            return;
        }

        strFileNameCache = pSourceFileView->GetFileName();
        UINT ErrorCount = 0;

        // Examine each error in turn
        while (pError = errors.Next())
        {
            // Add the error to the table.
            VbError *pVbError = &pdaErrors->Add();

            // Save the error message.
            bstrDescription = pError->m_wszMessage;
            if (strFileNameCache)
            {
                bstrFileName = strFileNameCache;
            }

            if (bstrFileName)
            {
                // Save the item name (filename)
                bstrItem = bstrFileName;

#if !IDE
                   // Save the text.
                if (pError->m_loc.IsValid())
                {
                    IfFailThrow(pSourceFileView->GetTextLines()->GetLineText(pError->m_loc.m_lBegLine,
                                                                             pError->m_loc.m_lBegColumn,
                                                                             pError->m_loc.m_lEndLine,
                                                                             pError->m_loc.m_lEndColumn + 1,
                                                                             &bstrText));
                }
#endif
            }

            // Fill in the info
            pVbError->dwSeverity = pError->m_Priority;
            pVbError->dwErrId = pError->m_errid;

            if (pError->m_loc.IsValid())
            {
                pVbError->dwBeginLine = pError->m_loc.m_lBegLine + 1;
                pVbError->dwEndLine = pError->m_loc.m_lEndLine + 1;
                pVbError->dwBeginCol = pError->m_loc.m_lBegColumn + 1;
                pVbError->dwEndCol = pError->m_loc.m_lEndColumn + 1;
            }
#if DEBUG
            if (pVbError->dwBeginLine)
            {
                VSASSERT(pVbError->dwEndLine, "Bad error location.");
                VSASSERT(pVbError->dwBeginCol, "Bad error location.");
                VSASSERT(pVbError->dwEndCol, "Bad error location.");
            }
            else
            {
                VSASSERT(!pVbError->dwEndLine, "Bad error location.");
                VSASSERT(!pVbError->dwBeginCol, "Bad error location.");
                VSASSERT(!pVbError->dwEndCol, "Bad error location.");
            }
#endif DEBUG

            pVbError->Description = bstrDescription.Detach();
            pVbError->ItemName = bstrItem.Detach();
            pVbError->FileName = bstrFileName.Detach();
            pVbError->SourceText = bstrText.Detach();

            ErrorCount++;

#ifndef MAX_ERRORS_DISABLED
            // Check if we've gone past MaxErrors.
            // No warnings are expected in FileView scenario
            if (ErrorCount > MaxErrors)
            {
                VbError tempError;
                memset(&tempError, 0, sizeof(tempError));
                StringBuffer sbDescription;
                IfFailThrow(ResLoadStringRepl(ERRID_MaximumNumberOfErrors, &sbDescription, NULL));
                tempError.Description = ComUtil::SafeSysAllocString(sbDescription.GetString());
                tempError.dwErrId = ERRID_MaximumNumberOfErrors;
                tempError.dwSeverity = TP_HIGH;
                pVbError = &pdaErrors->Add();
                *pVbError = tempError;
                break;
            }
#endif          
            if (GetCompilerSharedState()->IsRefreshingTaskList() && _VBPeekKeyMessages())
            {
                GetCompilerSharedState()->SetErrorsAborted(true);
                break;
            }
        }
    }
}
#endif

ErrorTableDependenciesIterator::ErrorTableDependenciesIterator
(
) :
    m_pErrorTable(NULL),
    m_pNext(NULL)
{
    Init(NULL);
}

ErrorTableDependenciesIterator::ErrorTableDependenciesIterator
(
    ErrorTable * pErrorTable
) :
    m_pErrorTable(NULL),
    m_pNext(NULL)
{
    Init(pErrorTable);
}

ErrorTableDependency * ErrorTableDependenciesIterator::Next()
{
    ErrorTableDependency * pRet = m_pNext;
    if (m_pNext)
    {
        m_pNext = m_pNext->m_pNextDependency;
    }
    return pRet;
}

void ErrorTableDependenciesIterator::Reset()
{
    Init(m_pErrorTable);
}

void ErrorTableDependenciesIterator::Init(ErrorTable * pErrorTable)
{
    m_pErrorTable = pErrorTable;

    if (m_pErrorTable)
    {
        m_pNext = m_pErrorTable->m_pFirstDependency;
    }
}

ErrorTableDependency::ErrorTableDependency() :
    m_pNextDependency(NULL),
    m_pPreviousDependency(NULL),
    m_ShouldBeRemovedFromErrorTable(false)
{
}

ErrorTableDependency::~ErrorTableDependency()
{
    m_pNextDependency = NULL;
    m_pPreviousDependency = NULL;
    m_ShouldBeRemovedFromErrorTable = false;
}

bool ErrorTableDependency::ShouldBeRemovedFromErrorTable()
{
    return m_ShouldBeRemovedFromErrorTable;
}

TemporaryErrorTable::TemporaryErrorTable
(
    Compiler * pCompiler,
    ErrorTable ** ppBackupErrorTable
)  :
    m_newErrorTables(VBAllocWrapper(), 0),
    m_oldErrorTable(ppBackupErrorTable)
{
}

TemporaryErrorTable::~TemporaryErrorTable()
{
    Restore();
}

void TemporaryErrorTable::SuppressMergeOnRestore()
{
    m_mergeOnRestore = -1;
}

void TemporaryErrorTable::EnableMergeOnRestore(unsigned index)
{
    ThrowIfFalse(index < m_newErrorTables.Count());
    ThrowIfFalse((int)index >= 0);
    m_mergeOnRestore = (int)index;
}

bool TemporaryErrorTable::WillMergeOnRestore(unsigned index)
{
    return m_mergeOnRestore >= 0 && index < m_newErrorTables.Count() && (unsigned)m_mergeOnRestore == index;
}

void TemporaryErrorTable::AddTemporaryErrorTable(ErrorTable * pTemporaryErrorTable)
{
    ThrowIfNull(pTemporaryErrorTable);

    m_newErrorTables.Add(pTemporaryErrorTable);
}

void TemporaryErrorTable::Restore()
{
    if (m_mergeOnRestore >= 0 && (unsigned)m_mergeOnRestore < m_newErrorTables.Count() && OldErrorTable() && NewErrorTable((unsigned)m_mergeOnRestore))
    {
        OldErrorTable()->MergeTemporaryTable(NewErrorTable((unsigned)m_mergeOnRestore));
    }
    m_oldErrorTable.Restore();

    ArrayListIterator<ErrorTable *> iterator(&m_newErrorTables);

    while (iterator.MoveNext())
    {
        delete iterator.Current();
    }

    m_mergeOnRestore = -1;
    m_newErrorTables.Clear();
}

ErrorTable * TemporaryErrorTable::OldErrorTable()
{
    return m_oldErrorTable.OldValue();
}

ErrorTable * TemporaryErrorTable::NewErrorTable(unsigned index)
{
    ThrowIfFalse(index < m_newErrorTables.Count());
    return m_newErrorTables[index];
}

