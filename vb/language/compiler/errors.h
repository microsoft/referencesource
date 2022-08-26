//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manages the parse and compile errors.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#if IDE 
class SourceFileView;
#endif

enum CompilationState;
enum CompilationSteps;

class CompilerFile;
class EditBuffer;
struct Location;
class LineMarker;
class LineMarkerTable;

class CompileError;
class ErrorTable;
class ErrorIterator;
class CompileErrorTaskItem;
class Compiler;
struct Text;

#if IDE 
    extern bool _VBPeekKeyMessages();
#endif

//============================================================================
// Cheater class to get VbErrors to destroy themselves.
//============================================================================
struct BCError : VbError
{
    ~BCError()
    {
        SysFreeString(Description);
        SysFreeString(ItemName);
        SysFreeString(FileName);
        SysFreeString(SourceText);
    }

#if IDE
    bool m_fIsWME;

    BCError(): m_fIsWME(false) { }
#endif
};

#ifndef MAX_ERRORS_DISABLED
//============================================================================
// Helper to build an array of errors from a project.  This will append
// the errors to the end of the array.
//============================================================================
const UINT BuildErrorArrayMaxErrors = 100;
const UINT BuildErrorArrayMaxWarnings = 100;
#endif
void BuildErrorArray(
     CompilerProject *pProject,
     DynamicArray<BCError> *pdaErrors,
     DynamicArray<SourceFile *> *pdaFiles
#ifndef MAX_ERRORS_DISABLED     
     , UINT MaxErrors     /* Set to zero for INFINITE */
     , UINT MaxWarnings
#endif
     );

#if IDE 
void BuildErrorArray(
    SourceFileView * pSourceFileView,
    DynamicArray<BCError > * pdaErrors
#ifndef MAX_ERRORS_DISABLED     
    , UINT MaxErrors
#endif    
    );
#endif

//****************************************************************************
// Stores a single error. Contains a LineMarker so that the error line can
// be updated. Used in a linked list contained by ErrorTable.
//****************************************************************************

class CompileError :
    public CSingleLink<CompileError>
#if FV_DEADBEEF
    , public Deadbeef<CompileError> // Must be last base class!
#endif
{
public:

    //
    // Data members
    //
    Compiler        *m_pCompiler;           // context
    CompilerProject *m_pCompilerProject;    // context
    WCHAR           *m_wszMessage;          // error message, owned by this object
    unsigned         m_errid;               // error code, zero if this is a comment item

#if IDE 

    WCHAR           *m_wszExtra;            // Extra string (value depends on the error)
    ErrorLocation m_loc;    // error location
    SourceFileView * m_pSourceFileView;

private:
    void SetExtra(_In_opt_z_ const WCHAR *wszExtra);
    void SetSourceFileView(SourceFileView * pSoruceFileView);

public:

#else !IDE
    Location m_loc;
#endif !IDE

    CompilationSteps m_step;  // compilation state attempted to reach
                              // when this error occurs

    VSTASKPRIORITY m_Priority;

    //
    // Member functions
    //

    CompileError(
        Compiler * pCompiler,
        CompilerProject * pCompilerProject,
        _In_opt_z_ WCHAR * wszMessage,
        unsigned errid,
        Location * ploc,
        VSTASKPRIORITY Priority
        IDE_ARG(_In_opt_z_ const WCHAR *wszExtra = NULL)
        IDE_ARG(_In_opt_ SourceFileView * pSourceFileView = NULL));

    ~CompileError();

    bool IsComment() const 
    {
         return (m_errid == 0); 
    }

    bool IsError() const 
    {
         return ((m_errid > 0) && (m_errid < WRNID_First)); 
    }

    bool IsWarning() const 
    {
         return (m_errid >= WRNID_First); 
    }
};

class ErrorTableDependency
{
    friend class ErrorTable;
    friend class ErrorTableDependenciesIterator;
public:
    ErrorTableDependency();

    virtual
    ~ ErrorTableDependency();

    virtual
    void NotifyTableReplacement(
        ErrorTable * pOldErrorTable,
        ErrorTable * pNewErrorTable) = 0;

    virtual
    void NotifyTableDestruction(ErrorTable * pOldErrorTable) = 0;

    bool ShouldBeRemovedFromErrorTable();
private:
    ErrorTableDependency * m_pNextDependency;
    ErrorTableDependency * m_pPreviousDependency;
    bool m_ShouldBeRemovedFromErrorTable;
};

class ErrorTableDependenciesIterator
{
public:
    ErrorTableDependenciesIterator();
    ErrorTableDependenciesIterator(ErrorTable * pErrorTable);
    ErrorTableDependency * Next();
    void Reset();
    void Init(ErrorTable * pErrorTable);
private:
    ErrorTable * m_pErrorTable;
    ErrorTableDependency * m_pNext;
};

//****************************************************************************
// Manages errors in a single file.
//****************************************************************************

class ErrorTable :
    ZeroInit<ErrorTable>
#if FV_DEADBEEF
    , public Deadbeef<ErrorTable> // Must be last base class!
#endif
{
    friend class ErrorIterator;
    friend class ErrorTableDependenciesIterator;
public:
    NEW_CTOR_SAFE()

    //Creates a temporary replacement table for src.
    //This is an empty error table with the same
    //compile, and project. The line marker
    //table is not propigated, however. This is because
    //the temporary table is designed to either be
    //dropped or merged into the table it was
    //generated from.
    ErrorTable(const ErrorTable &src);

    ErrorTable();

    ErrorTable(
        Compiler * pCompiler,
        CompilerProject * pCompilerProject,
        LineMarkerTable * plmt);

    ~ErrorTable()
    {
        Destroy();
    }

    void Destroy()
    {
        DeleteAll();
        NotifyDependenciesOfDestruction();
        ClearDependencies();
    }

    void Init(
        Compiler * pCompiler,
        CompilerProject * pCompilerProject,
        LineMarkerTable * plmt);

    void SetCompiler(Compiler *pCompiler)
    {
        m_pCompiler = pCompiler;
    }

#if IDE 
    void KillLineMarkerTable()
    {
        DeleteAll();
        m_pLineMarkerTable = NULL;
    }

    void SetSourceFileView(SourceFileView * pSourceFileView)
    {
        m_pSourceFileView = pSourceFileView;
    }

    void DeleteAllENCErrors();
#endif

    Compiler* GetCompiler() const
    {
        return m_pCompiler;
    }

    CompilerProject* GetProject() const
    {
        return m_pCompilerProject;
    }

    //========================================================================
    // Functions to create errors.
    //========================================================================

    void __cdecl CreateError(
        ERRID errid,
        Location * ploc,
        ...);

    void __cdecl CreateErrorWithError(
        ERRID errid,
        Location * ploc,
        HRESULT hr,
        ...);

    void __cdecl CreateErrorWithSymbol(
        ERRID errid,
        BCSYM * psym,
        ...);

    void __cdecl CreateErrorWithSymbolAndError(
        ERRID errid,
        BCSYM * psym,
        HRESULT hr,
        ...);

    void __cdecl CreateErrorWithExtra(
        ERRID errid,
        _In_opt_z_ const WCHAR * wszExtra,
        Location * ploc,
        ...);

    void __cdecl CreateErrorWithPartiallySubstitutedString(
        ERRID errid,
        const WCHAR * wszPartiallyReplString,
        Location * ploc,
        ...);

    void CreateErrorArgs(
        ERRID errid,
        Location * ploc,
        _In_opt_z_ WCHAR * wszError,
        va_list ap);

    // The base error creation method.
    void CreateErrorArgsBase(
        ERRID errid,
        const WCHAR * wszPartiallyReplString,
        _In_opt_z_ const WCHAR * wszExtra,
        Location * ploc,
        _In_opt_z_ WCHAR * wszError,
        va_list ap);

    WCHAR * ExtractErrorName(
        BCSYM * Substitution,
        BCSYM_Container * ContainingContainer,
        StringBuffer &TextBuffer,
        BCSYM_GenericBinding * GenericBindingContext = NULL,
        bool FormatAsExtensionMethod = false,
        IReadonlyBitVector * FixedTypeArgumentBitVector = NULL,
        CompilerHost * CompilerHost = NULL);

    //========================================================================
    // Functions to create comment task items.
    //========================================================================

    void CreateComment(
        Location * pLocation,
        _In_z_ WCHAR * wszMessage,
        VSTASKPRIORITY Priority);

    //========================================================================
    // Error table cleanup methods.
    //========================================================================

    bool DeleteIf(std::function<bool (CompileError*)>, bool lockCriticalSection = true);
    void DeleteAll();
    void DeleteAllWithThisStateOrHigher(CompilationState state);
    void DeleteAllWithStep(CompilationSteps step);
    void DeleteSpecificError(ERRID errid, const Location * pWithinLocation = NULL);

    //========================================================================
    // Set Zombie so that we do not propogate use of the task provider.
    //========================================================================

#if IDE 
    void SetZombie()
    {
        m_Zombied = true;
    }
#endif

    //========================================================================
    // Random implementation.
    //========================================================================

    bool HasErrors();
    bool HasErrorsNoLock();
    unsigned GetErrorCount();
    bool HasWarnings();
    unsigned GetWarningCount();
    bool HasComments();

    bool HasErrorsThroughStep(CompilationSteps step);

    bool HasErrorsThroughStepNoLock(CompilationSteps step);

    bool HasThisErrorWithSymLocation(
        ERRID errid,
        BCSYM * psym);

    bool HasThisErrorWithLocation(
        ERRID errid,
        const Location &loc);

    bool HasThisErrorWithLocationAndParameters(
        ERRID errid,
        const Location * ploc,
        ...);
#if IDE 
    bool HasThisError(ERRID errid);
    bool HasErrorsAtStep(CompilationSteps step);
#endif IDE

    void MergeTemporaryTable(ErrorTable * pErrorTable);
    bool IsTransient();
    void RecordDependency(ErrorTableDependency * pDependency);
    void RemoveDependency(ErrorTableDependency * pDependency);

    // Merge pErrorTable which contains errors while trying to reach csTarget
    // compilation state. Existing errors in this table with that same state
    // will be deleted. Merging essentially *moves* CompileError's from
    // pErrorTable into this ErrorTable, while maintaining ascending order.
    //
    void Merge(
        ErrorTable * pErrorTable,
        CompilationSteps stepTarget,
        bool fPurgeExistingErrorsWithThisStep = true);

    // Merge the error table that contains the project-level conditional
    // compilation errors into this one.  This does not set any steps
    // or states but does munge the error message into something
    // a bit more user-friendly.  This resulting table then has to be
    // merged again into the main table using the above API.
    //
    void MergeProjectLevelCCErrors(
        ErrorTable * pErrorTable,
        _In_z_ STRING * pstrCC);

    void MergeProjectLevelImportErrors(ErrorTable *pErrorTable);

    // Move errors from the NoStep list to the list for a particular target step.
    void MergeNoStepErrors(CompilationSteps stepTarget);

    void MoveNoStepErrors(ErrorTable *pErrorTable);

    // Locking
    RefCountedPtr<CompilerIdeTransientLock> LockErrorTableOnStack()
    {
        return RefCountedPtr<CompilerIdeTransientLock>(new CompilerIdeTransientLock(m_csErrorTable, GetTransientLockMode()));
    }

private:
    TransientLockModeEnum GetTransientLockMode() const
    {
        return m_fIsTransient 
            ? TransientLockMode::Faked
            : TransientLockMode::Synchronized;
    }

    void NotifyDependenciesOfDestruction();
    void NotifyDependenciesOfMerge(ErrorTable * pDestination);
    void ClearDependencies();
    void SpliceDependencies(ErrorTable * pOtherErrorTable);

    // Helper for Merge
    bool MergeNoStepErrors(
        ErrorTable * pErrorTable,
        CompilationSteps stepTarget);

    // Helper for error deletion
    bool DeleteAllWithStepNoLock(CompilationSteps step);
    void TaskListNeedsRefresh();

    //
    // Data Members
    //
    // Since there can be thousands of ErrorTable instances in a large solution,
    // we don't want this structure to get too large.  With 10,000 ErrorTables,
    // every 100 bytes in fields = 1 MB.

    // Context.
    Compiler        *m_pCompiler;
    CompilerProject *m_pCompilerProject;
#if IDE 
    LineMarkerTable *m_pLineMarkerTable;
    SourceFileView * m_pSourceFileView;
    bool             m_Zombied;
#endif

    // The error/warning/comments list.
    std::list<CompileError*> m_Errors;

    // Synchronization
    CompilerIdeCriticalSection m_csErrorTable;

    // Never change this value during the lifetime of the errortable
    // or the locking mechanism will deadlock.
    const bool m_fIsTransient;

    ErrorTableDependency * m_pFirstDependency;
    ErrorTableDependency * m_pLastDependency;
};

//defines a wrapper around an AutoPtr and BackupValue structure used for representing temporary error tables.
//When an instance of this type goes out of scope it will delete the value stored in m_tempErrorTable if it exists
//and then restore the error table pointer represented by m_backupErrorTable. If m_mergeByDefault is true
//then the temporary error table will be merged into the backup error table before it is deleted.
class TemporaryErrorTable
{
public:
    TemporaryErrorTable(
        Compiler * pCompiler,
        ErrorTable * * ppBackupErrorTable);
    ~TemporaryErrorTable();

    void SuppressMergeOnRestore();
    void EnableMergeOnRestore(unsigned index);
    bool WillMergeOnRestore(unsigned index);

    void AddTemporaryErrorTable(ErrorTable * pTemporaryErrorTable);
    void Restore();

    ErrorTable * OldErrorTable();
    ErrorTable * NewErrorTable(unsigned index);
private:
    BackupValue<ErrorTable *> m_oldErrorTable;
    ArrayList<ErrorTable *> m_newErrorTables;
    int m_mergeOnRestore;
};


//****************************************************************************
// Iterates over the contents of an error table.
//****************************************************************************

class ErrorIterator : ZeroInit<ErrorIterator>
{
public:

    ErrorIterator(ErrorTable * pErrorTable)
    : m_pErrorTable(pErrorTable),
      Initialized(false)
    {}

    CompileError *Next();

private:
    ErrorTable *m_pErrorTable;

    std::list<CompileError*>::iterator m_nextError;
    bool Initialized;
};
