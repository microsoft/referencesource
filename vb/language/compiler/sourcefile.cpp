//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the file-level compilation logic.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"
using namespace std;

//****************************************************************************
//****************************************************************************
//****************************************************************************
//
// SourceFile implementation
//
//****************************************************************************
//****************************************************************************
//****************************************************************************

//****************************************************************************
// Compilation methods
//
// The following methods should only be called from the master project
// compilation routines.  They should never be called directly.
//****************************************************************************

//============================================================================
// Ensure that all of the symbols in this file are created.
//============================================================================

bool SourceFile::_StepToBuiltSymbols()
{
    DebCheckInCompileThread(m_pCompiler);

    ErrorTable       *perrorTable = this->GetCurrentErrorTable();
    BCSYM_Container  *pcontainer = NULL;

    VSASSERT(m_cs == CS_NoState, "Bad state.");
    VSASSERT(m_step == CS_NoStep, "Bad step.");

    //
    // Otherwise bring it to the next state.
    //

    ParseTree::FileBlockStatement *ptree = NULL;
    NorlsAllocator nraDeclTrees(NORLSLOC);

#if IDE 
    // Make sure the cache is cleared and the data valid.
    SignalSourceChange();
#endif


    // Get the trees.

    IfFailThrow(GetDeclTrees(&nraDeclTrees,
        &m_nraSymbols,
        perrorTable,
        &pcontainer,
        GetLineMarkerTable(),
        &ptree));

#if IDE 
    m_HasParseErrors = perrorTable->HasErrorsThroughStep(CS_NoStep);
#endif

    // Create the symbols from the trees.
    Declared::MakeDeclared(
        GetCompilerHost(),
        &m_nraSymbols,
        this,
        GetLineMarkerTable(),
        ptree,
        perrorTable,
        &m_SymbolList);

    // If we don't have any parse trees (e.g. the VB file is in the .vbproj file but it doesn't exist),
    // MakeDeclared will do nothing, but we still need to create an empty unnamednamespace for this file.
    // VS199349
    if (!GetUnnamedNamespace())
    {
        GetCompiler()->GetUnnamedNamespace(this);
    }

    // Build a hash table from the symbols built to the locations of the source text.
    // This is used later by GetSymbolOfLocation
    BCSYM_NamedRoot *pnamed = m_SymbolList.GetFirst();
    while(pnamed)
    {
        BCSYM_NamedRoot *pnamedActual = pnamed->DigThroughAlias()->PNamedRoot();

        if (pnamedActual->HasLocation() && !pnamedActual->IsLocationInherited() && !pnamedActual->IsHidden())
        {
            AddSymbolToHashTable(pnamedActual->GetLocation(), pnamedActual);
        }

        // Delegate methods should not appear in this hash-table, if they did,
        // GetSymbolOfLocation would find them instead of the delegate itself.
        if (pnamedActual->IsContainer() && !pnamedActual->IsDelegate())
        {
            BCITER_CHILD_ALL bichild(pnamedActual->PContainer(), true);
            BCSYM_NamedRoot *pnamedChild;

            while (pnamedChild = bichild.GetNext())
            {
                // Containers need not be added to the hash table because they will be re-visited later
                // (or we already have) in the m_SymbolList list.
                if (pnamedChild->HasLocation() && !pnamedChild->IsLocationInherited() && !pnamedChild->IsHidden() && !pnamedChild->IsContainer())
                {
                    AddSymbolToHashTable(pnamedChild->GetLocation(), pnamedChild);
                }
            }
        }

        pnamed = pnamed->GetNextInSymbolList();
    }

    //
    // Nothing after this point should be able to fail.  It is now safe to move this module to the next state.
    //

    {
        CompilerIdeLock spLock(GetObjectCriticalSection());

        // Save our state information.
        m_pConditionalCompilationConstants = pcontainer;

        if (IsSolutionExtension() && GetSolutionExtension()->m_extension == SolutionExtension::MyTemplateExtension )
        {
            VSASSERT(pcontainer!= 0,"MyTemplate container should be nonnull!");
        }
        if (IsSolutionExtension() && 
            GetSolutionExtension()->m_extension == SolutionExtension::MyTemplateExtension && 
            pcontainer) // pContainer is null for MyTemplateExtension in some hosted scenarios
        {
            Declaration *StartupMyConst =
                Semantics::ImmediateLookup(
                    ViewAsScope(pcontainer),
                    STRING_CONST(GetCompiler(), StartupMyFormFactory),
                    BINDSPACE_Normal);
            if ( StartupMyConst )
            {
                ConstantValue Value =
                    StartupMyConst->PVariableWithValue()->GetExpression()->GetValue();
                GetProject()->m_StartupMyFormFactory = GetCompiler()->AddString(Value.String.Spelling);
            }
            else
            {
                GetProject()->m_StartupMyFormFactory = NULL;
            }
        }
        m_nraSymbols.Mark(&m_nramarkDeclared);

        // Merge the new errors into the main error table.
        this->MergeCurrentErrors();

        VSASSERT(m_step == CS_NoStep, "Step changed unexpectedly.");
        m_step = CS_BuiltSymbols;

#if IDE 
        // Increment change tracker.
        m_iCurrentDeclChangeIndex++;
#endif

        spLock.Unlock();
    }

    return false;
}


//============================================================================
// Bind the bases and implements named types.  This only returns a catastrophic-type of error.
//============================================================================

bool SourceFile::_StepToBoundSymbols()
{
    DebCheckInCompileThread(m_pCompiler);

    ErrorTable errors(m_pCompiler, m_pProject, NULL);

    VSASSERT(m_cs == CS_Declared, "Bad state.");
    VSASSERT(m_step == CS_BuiltSymbols, "Bad step.");

    // Do the work.
    Bindable::BindSourceFile(this, NULL);

#if IDE 

    RegisterFileSubType();

#endif

    VSASSERT(m_step == CS_BuiltSymbols, "Bad step.");

    return false;
}

void SourceFile::CompleteStepToBoundSymbols()
{
    // 


    CompilerIdeLock spLock(GetObjectCriticalSection());

    // Merge the new errors into the main error table.
    this->MergeCurrentErrors();

    VSASSERT(m_step == CS_BuiltSymbols, "Bad step.");

    // Remember where we were in the allocator, because demoting to
    // Bound can roll back to this step.
    m_nraSymbols.Mark(&m_nramarkBound);
    m_step = CS_BoundSymbols;

}


#if IDE 

#define DESIGNERATTRIBUTE_CODE L"Code"

//============================================================================
// Determine whether the designer category has changed from the category
// we have in the cache.
//============================================================================

bool SourceFile::DesignerAttributeChanged( LPCWSTR wszCategory )
{
    VSASSERT( m_pstrLastSubTypeCategory != NULL, "Need to set subtype." );

    WCHAR* szCode = DESIGNERATTRIBUTE_CODE;

    if( wszCategory == NULL )
    {
        wszCategory = szCode;
    }

    if( CompareCase( m_pstrLastSubTypeCategory, wszCategory ) == 0 )
    {
        return( false );
    }
    else
    {
        return( true );
    }
}

//============================================================================
// Handles inheritance of all well-known attributes.
//============================================================================

void SourceFile::RegisterFileSubType()
{
    DebCheckInCompileThread(m_pCompiler);

    VSASSERT(m_cs >= CS_Declared, "Bad state.");

    m_ShouldReRegisterFileSubType = false;

    // If there is no site, RegisterDesignViewAttribute can't query service for the .NET
    // Designer service, so we can skip a lot of work down below. Also, we only want to
    // do this for SourceFiles that have a project item id.
    bool fDoRegister = (! GetProject()->IsExpressionEditorProject()) && (GetItemId() != 0) && !IsSolutionExtension();

    if (!fDoRegister)
    {
        return;
    }

    // Only register the Design View of the first class we find marked with
    // the custom attribute.  If no such class is found, then we register this
    // SourceFile as "Code".
    WCHAR *pwszCategoryToRegister = NULL;
    BCSYM_Class *pClass = NULL;
    Location *pCurrentLoc = NULL;

    BCITER_Classes iterClasses;
    iterClasses.Init(this, true);


    // Look for the first class in the file that has the "DesignerCategory"
    // attribute applied to it and obtain its value.
    // We don't care about Enums, Struct, Delegates or Modules.
    while ((pClass = iterClasses.Next()) != NULL)
    {
        WCHAR *pwszCategory = NULL;

        if (IsClassOnly(pClass) && pClass->GetPWellKnownAttrVals()->GetDesignerCategoryData(&pwszCategory))
        {
            if (pCurrentLoc == NULL || pClass->GetLocation()->StartsBefore(pCurrentLoc))
            {
                pwszCategoryToRegister = pwszCategory;
                pCurrentLoc = pClass->GetLocation();
            }
        }
    }

    // PERF: Avoid sending the message if there is no attribute specified and it hasn't changed.
    // The cost on the foreground thread of calling designer service and locking the compiler
    // is pretty high and default category is Code anyway.
    if (m_pstrLastSubTypeCategory == NULL && pwszCategoryToRegister == NULL)
    {
        return;
    }

    // If we found a class to register, do it now.  If not, register as "Code". But do this only
    // if we differ from the cache of the last designer category.

    if (m_pstrLastSubTypeCategory == NULL || DesignerAttributeChanged( pwszCategoryToRegister ) )
    {
        m_pstrLastSubTypeCategory = m_pCompiler->AddString(pwszCategoryToRegister == NULL ? DESIGNERATTRIBUTE_CODE : pwszCategoryToRegister);

        // Package up the params and send via a message to the foreground thread.
        // This will be freed when the message is processed.
        RegisterDesignViewAttributeMessage *pMsg;

        // We always pass 0 for the ClassID (not the line number for the class) because the Designer
        // uses this information as a ClassID, so it should be changing as code moves around. ) indicates the
        // first class in the file. (see bug 160102)
        pMsg = new RegisterDesignViewAttributeMessage(this, 0, m_pstrLastSubTypeCategory);
        GetCompilerSharedState()->GetMessages()->RegisterDesignViewAttribute(pMsg);
    }
}

#undef DESIGNERATTRIBUTE_CODE

#endif // IDE

//============================================================================
// Check CLS compliance of members.
//============================================================================

bool SourceFile::_StepToCheckCLSCompliance()
{
    DebCheckInCompileThread(m_pCompiler);

    VSASSERT(m_cs == CS_Bound, "Bad state.");
    VSASSERT(m_step == CS_BoundSymbols, "Bad step.");

    CompilerIdeLock spLock(GetObjectCriticalSection());

    // Move to the next step.
    VSASSERT(m_step == CS_BoundSymbols, "Bad step.");
    m_step = CS_CheckCLSCompliance;

    return false;
}

//============================================================================
// Create the metadata tokens for this file.
//============================================================================

bool SourceFile::_StepToEmitTypes()
{
    DebCheckInCompileThread(m_pCompiler);

    VSASSERT(m_cs == CS_Bound, "Bad state.");
    VSASSERT(m_step == CS_CheckCLSCompliance, "Bad step.");

    CompilerIdeLock spLock(GetObjectCriticalSection());

    // Move to the next step.
    VSASSERT(m_step == CS_CheckCLSCompliance, "Bad step.");
    m_step = CS_EmitTypes;

    return false;
}

//============================================================================
// Create the metadata tokens for this file.
//============================================================================

bool SourceFile::_StepToEmitTypeMembers()
{
    DebCheckInCompileThread(m_pCompiler);

    VSASSERT(m_cs == CS_Bound, "Bad state.");
    VSASSERT(m_step == CS_EmitTypes, "Bad step.");

    CompilerIdeLock spLock(GetObjectCriticalSection());

    // Move to the next step.
    VSASSERT(m_step == CS_EmitTypes, "Bad step.");
    m_step = CS_EmitTypeMembers;

    // Remember where we were in the allocator, because demoting to
    // TypesEmitted can roll back to this step.
    m_nraSymbols.Mark(&m_nramarkTypesEmitted);

    return false;
}

//============================================================================
// Do the semantic analysis on the methods defined in this file.  Generate
// code if there are no errors.  This only returns a catastrophic-type of
// error.
//============================================================================

bool SourceFile::_StepToEmitMethodBodies()
{
    DebCheckInCompileThread(m_pCompiler);

    bool fAborted = false;

    VSASSERT(m_cs == CS_TypesEmitted, "Bad state.");
    VSASSERT(m_step == CS_EmitTypeMembers, "Bad step.");

    //
    // Tell the user what's up.
    //

#if !IDE
    if (m_pCompiler->OutputLevel() == OUTPUT_Verbose && m_pstrFileName)
    {
        StringBuffer sbuf;

        IfFailThrow(ResLoadStringRepl(STRID_Building,
                                      &sbuf,
                                      m_pstrFileName));

        GetCompilerHost()->Printf(L"%s\n", sbuf.GetString());
    }
#endif !IDE

    //
    // Nothing after this point should be able to fail.  It is now safe
    // to move this module to the next state.
    //

    //
    // Debug help.
    //

#if DEBUG

    if (VSFSWITCH(fDumpText))
    {
        DebPrintf("\n//============================================================================\n");
        DebPrintf("// Text for %S.\n", m_pstrFileName);
        DebPrintf("//============================================================================\n\n");

        Text text;
        IfFailThrow(text.Init(this));

        const WCHAR *wszText;
        size_t cch;

        text.GetText(&wszText, &cch);

        if (!wszText)
        {
            DebPrintf("None.");
        }
        else
        {
            const WCHAR *wszEnd = wszText + cch;

            for (; wszText < wszEnd; wszText++)
            {
                DebPrintf("%C", *wszText);
            }

            DebPrintf("\n");
        }
    }

#endif DEBUG

    m_pCompiler->AddErrorsAndWarnings(
        m_ErrorTable.GetErrorCount(),
        m_ErrorTable.GetWarningCount());

#if IDE 
    if (m_ShouldReRegisterFileSubType)
    {
        RegisterFileSubType();
    }
#endif

    VSASSERT(m_step == CS_EmitTypeMembers, "Bad step.");
    m_step = CS_GeneratedCode;

    return fAborted;
}

//****************************************************************************
// SourceFile implementation
//****************************************************************************

//============================================================================
// SourceFile Constructor
//============================================================================

SourceFile::SourceFile
(
    Compiler *pCompiler,
    CompilerProject *pProject
)
    : CompilerFile(pCompiler)
    , m_CodeFile(pCompiler, this)
    , m_XMLDocFile(pCompiler, this)
#if IDE 
    , m_LineMarkerTable(this)
    , m_pParseTreeService(NULL)
    , m_fNeedToFireOnStatementsChangedEvents(false)
    , m_IsCLSComplianceCheckingDone(false)
    , m_iSourceChangeIndex(1)    // Need to start in one, because load is a "change"
    , m_iCurrentDeclChangeIndex(1)
    , m_iCurrentBoundChangeIndex(1)
    , m_iCurrentBindingChangeIndex(1)
    , m_NamedRootsMarkedBadAfterTypesEmitted(false)
    , m_ShouldReRegisterFileSubType(true)
    , m_pstrLastSubTypeCategory(NULL)
    , m_IsPosted_FileInDeclared(false)
    , m_IsPosted_FileInBindable(false)
    , m_IsPosted_RegisterDesignViewAttribute(false)
    , m_pstrDesignViewAttributeCategory(NULL)
    , m_HasFileLoadError(false)
    , m_HasParseErrors(false)
    , m_isLinkedAndRegistered(false)
    ,m_ImportedExtensionMethodMetaDataLoaded(false)
#endif IDE
#if IDE
    , m_fShowErrorsInTaskList(TRUE)
    , m_pFileCodeModel(NULL)
    , m_pENCProcessor(NULL)
    , m_IsAspx(false)
    , m_fNeedToUpdateOutOfProcBuildErrorArray(false)
#endif
    , m_HaveImportsBeenResolved(FALSE)
    , m_CurrentErrorTable(pCompiler, pProject, NULL)
    , m_RootNamespace(NULL)
    , m_pstrKeyFileNameFromAttr(NULL)
    , m_pstrKeyContainerNameFromAttr(NULL)
    , m_pImportTracker(NULL)
    , m_pDaUnprocessedFriends(NULL)
{

#if IDE 

    m_fCachingTemporaryNames = false;

#endif IDE

    ClearingBindingStatus();

 }

SourceFile::~SourceFile()
{
    Invalidate(false);
}

//============================================================================
// SourceFile Invalidate - same code the destructor will call
// but sometimes we will call it before the ref count hits 0 - e.g.
// when the compilerproject is disconnected.
//============================================================================

void SourceFile::Invalidate(bool SkipDemoteNodesCheck /* = true */)
{
    CheckInMainThread();

#if IDE 

#if DEBUG
    if (GetCompilerPackage() && !SkipDemoteNodesCheck)
    {
        for(CommitEditNode *pNode = GetCompilerSharedState()->GetCommitEditList()->m_pHeadNode;
            pNode;
            pNode = pNode->m_pNext)
        {
            VSASSERT(pNode->m_pFile != this, "Presence of project being deleted in decompilation list unexpected!!!");
        }
    }
#endif DEBUG

    if (GetCompilerPackage())
    {
        GetCompilerPackage()->OnSourceFileRemovedFromProject(this);
    }

    if ( m_isLinkedAndRegistered && GetCompilerPackage() )
    {
        GetCompilerPackage()->UnregisterLinkedFile(m_pstrFileName);
        m_isLinkedAndRegistered = false;
    }

    // Verify the FileCodeModel Object was correctly released.
    if (m_pFileCodeModel)
    {
        m_pFileCodeModel->Invalidate();
        m_pFileCodeModel->Release();
        m_pFileCodeModel = NULL;
    }

    if (m_pENCProcessor != NULL)
    {
        delete m_pENCProcessor;
        m_pENCProcessor = NULL;
    }

    ClearOutOfProcBuildErrors();

    //  Free the parse tree services and cache.
    //
    if (m_pParseTreeService)
    {
        delete m_pParseTreeService;
        m_pParseTreeService = NULL;
    }

    // If we are currently connected to a SourceFileView, disconnect now because
    // the SourceFile is completely invalid at this point.  Otherwise it's possible
    // for the SourceFileView and hence the managed object model to continue to hold
    // onto a SourceFile that is essentially destroyed
    if ( GetSourceFileView() )
    {
        GetSourceFileView()->SetSourceFile(NULL);
        SetSourceFileView(NULL);
    }

    m_LineMarkerTable.RemoveAllDeclLocations();

    m_ErrorTable.KillLineMarkerTable();

    // When we invalidate a file, we want to clear the errors from
    // the error table, otherwise, the error table may point back to the task
    // list to refresh, which ----s up because we will have been deleted.

    m_ErrorTable.DeleteAll();
    m_ErrorTable.SetZombie();
    m_CurrentErrorTable.DeleteAll();
    m_CurrentErrorTable.SetZombie();

#endif IDE

    if(m_pDaUnprocessedFriends)
    {
        delete m_pDaUnprocessedFriends;
        m_pDaUnprocessedFriends = NULL;
    }
}

//============================================================================
// Initializes the file.
//============================================================================

HRESULT SourceFile::Create
(
    CompilerProject *pProject,
    _In_z_ const WCHAR *wszFileName,
    _In_opt_bytecount_(dwBufLen) const WCHAR *wszBuffer,             // in-memory buffer (NULL except in VBA case)
    _In_ DWORD dwBufLen,                     // length of in-memory buffer
    _In_ VSITEMID itemid,
    _Out_ SourceFile **ppfile,
    _In_opt_ bool fWatchFile,  //= true, // file system watcher
    _In_opt_ SolutionExtensionEnum nSolutionExtension //= SolutionExtensionNone // if it's a My template 
)
{

    AssertIfNull(pProject);

    HRESULT hr = NOERROR;
    Compiler *pCompiler = pProject->GetCompiler();
    CheckInMainThread();
    SourceFile *pfile;

    //
    // Create the instance.
    //

#if IDE 

    // Use an external allocator because a SourceFile can
    // live beyond its containing compiler.
    //
    pfile = new (zeromemory )SourceFile(pCompiler, pProject);

    //  Create the Parse Tree Service and Cache.
    //
    pfile->m_pParseTreeService = new ParseTreeService( pfile );

    //  Initially there is no view of the file.
    //
    pfile->m_pSourceFileView = NULL;

#else //!IDE

    pfile = new (zeromemory) SourceFile(pCompiler, pProject);

#endif !IDE

    IfFailGo(pfile->Init(FILE_Source, pProject));

    //
    // Store information about the file.
    //

    pfile->m_pstrFileName = pfile->m_pCompiler->AddString(wszFileName);

#if IDE 
    // Set the itemid.  This may be zero.
    pfile->m_itemid = itemid;
#endif IDE

    // Initialize the text file.
    pfile->m_TextFile.Init(pCompiler, pfile, pfile->m_pstrFileName);

    if (dwBufLen > 0)
    {
        // if it's a MY or XMLHelper template, it's CONST readonly, so we will the SolutionExtensionData instance
        if (nSolutionExtension == SolutionExtension::SolutionExtensionNone) 
        {
            // Give text file the in-memory buffer (usually NULL/0 except in VBA case)
            pfile->m_TextFile.SetBuffer((WCHAR *)wszBuffer, dwBufLen);
        }
    }

#if IDE 

    // Add a reference to ourseves so we'll never go away because
    // of a refcount decrementation.
    //
    pfile->AddRef();


    if (fWatchFile)
    {
        //  Watch the file.
        //
        g_pCompilerPackage->SetIgnoreFileChangeNotifications(true);
        IfFailGo(pfile->WatchFile());
    }

    // Hostable Editor check
    if (!pProject->IsExpressionEditorProject() && GetCompilerPackage()->GetSite())
    {
        CComObject<CFileCodeModel> *pComFileCodeModel;
        hr  = CComObject<CFileCodeModel>::CreateInstance(&pComFileCodeModel);
        if (SUCCEEDED(hr))
        {
            pComFileCodeModel->AddRef();

            hr = pComFileCodeModel->Init(pfile);
            if (FAILED(hr))
            {
                VSFAIL("Couldn't create FileCodeModel Object");

                pComFileCodeModel->Invalidate();
                pComFileCodeModel->Release();
                pComFileCodeModel = NULL;
            }

            pfile->m_pFileCodeModel = pComFileCodeModel;
            pComFileCodeModel = NULL;
        }
    }

    pProject->AddFileToTree(pfile);

    if ( pfile->IsLinkedFile() )
    {
        GetCompilerPackage()->RegisterLinkedFile(pfile->m_pstrFileName);
        pfile->m_isLinkedAndRegistered = true;
    }

#endif // IDE 

    pProject->ClearFileCompilationArray();

    *ppfile = pfile;
    pfile = NULL;

Error:

#if IDE 
    if (fWatchFile)
    {
        g_pCompilerPackage->SetIgnoreFileChangeNotifications(false);
    }

    if (pfile)
    {
        // 

        delete pfile;
    }

#else !IDE

    if (pfile)
    {
        delete pfile;
    }

#endif IDE

    RRETURN( hr );
}

#if IDE 

//============================================================================
// Remove the source file from the project linked list and release all
// if it's resources and its connection.  Add it to a "debug"
// list that will ensure that the final release happens.

// Might want to make a zombie call to the codefile to ensure it releases
// any event sinks which might be holding on to additional references.
// This object should really go away right at this point and not later.
//============================================================================

void SourceFile::ReleaseFileFromProject(bool SkipDemoteNodesCheck)
{
    VSASSERT(m_cRefs > 0, "The project should still have a reference to the file.");

    // Codefile references passed thru to the sourcefile should also
    // be passed thru to the compilerproject. In such a case you should
    // not see this call in the destructor of the compilerproject
    // since we can't be there in the first place. However, I know
    // that's not happening and so we zombie the codefile so any
    // refs (by xml comsumers) on the codefile can be dropped.
    m_CodeFile.Fire_OnDropped();

    // Round about way of asking if a file is currently marked for decompilation
    // and in the decompilation list
    //
    VSASSERT(SkipDemoteNodesCheck || !this->IsNeedToFireOnStatementsChangedEvents(),
                "File pending decompilation cannot be released!!!");

    Invalidate(SkipDemoteNodesCheck);

    // Can't call GetCompilerHost() from SourceFile destructor since it may be
    // invalid at that time.
    if (GetCompilerHost()->GetPreferredSourceFile() == this)
    {
        GetCompilerHost()->SetPreferredSourceFile(NULL);
    }

    // Have to unlink now because there may be some background thread
    // MSG_FileInBindable messages pending and if this is being called
    // from the destructor then the compilerfile will still be alive
    // after the compilerproject is gone!! since the compilerfile has
    // a back pointer to the project we'll GP fault.
    UnlinkFromProject();

#if DEBUG
    // Keep track fo this class.to make sure it eventually goes away.
    s_FileLeakDetection.Add(this);
#endif

    Release();
}

//============================================================================
// Rename the source file from the project
//============================================================================
void SourceFile::RenameFileFromProject(LPCWSTR wszNewFileName)
{
    CheckInMainThread();
    if (wszNewFileName)
    {
        //  If we're being watched, stop it.
        bool IsFileWatched = IsWatched();

        if (IsFileWatched)
        {
            UnwatchFile();
        }

        m_LineMarkerTable.RemoveAllDeclLocations();

        AssertIfNull(m_pProject);
#if IDE 
        m_pProject->RemoveFileFromTree(this);
        if ( m_isLinkedAndRegistered )
        {
            GetCompilerPackage()->UnregisterLinkedFile(m_pstrFileName);
        }
#endif

        m_pstrFileName = m_pCompiler->AddString(wszNewFileName);

        m_TextFile.Init(m_pCompiler, this, m_pstrFileName);

        // Tell the code file that things have changed
        GetCodeFile()->SetFileDirty();

        //  Watch the file.
        if (IsFileWatched)
        {
            WatchFile();
        }
#if IDE 
        m_pProject->AddFileToTree(this);
        if ( m_isLinkedAndRegistered )
        {
            GetCompilerPackage()->RegisterLinkedFile(m_pstrFileName);
        }
#endif
        m_pProject->ClearFileCompilationArray();
    }
}


//============================================================================
// GetInternalFileCodeModel:
//
//    Gets the internal file code model.
//
//    Returns:
//        - True if the function succeeded,
//        - False if the trees were not available.
//============================================================================
HRESULT
SourceFile::GetInternalFileCodeModel
(
    CFileCodeModel **ppFileCodeModel //[Out] will contain the cfilecodemodel object.
)
{
    VerifyOutPtr(ppFileCodeModel);

    HRESULT hr = E_FAIL;

    *ppFileCodeModel = NULL;

    if (m_pFileCodeModel)
    {
        *ppFileCodeModel = m_pFileCodeModel;
        (*ppFileCodeModel )->AddRef();

        hr = S_OK;
    }

    return hr;
}

//============================================================================
// GetFileCodeModel:
//
//    Gets the file code model.
//
//    Returns:
//        - True if the function succeeded,
//        - False if the trees were not available.
//============================================================================
HRESULT
SourceFile::GetFileCodeModel
(
    FileCodeModel **ppFileCodeModel //[Out] will contain the cfilecodemodel object.
)
{
    VerifyOutPtr(ppFileCodeModel);

    HRESULT hr = E_FAIL;

    *ppFileCodeModel = NULL;

    if (m_pFileCodeModel)
    {
        hr = m_pFileCodeModel->QueryInterface(
            IID_FileCodeModel,
            (void **) ppFileCodeModel);
    }

    return hr;
}

//============================================================================
// GetCurrentDeclTrees:
//
//    Gets the current declaration trees. Make sure that the file is in
//    CS_Declared before calling this function.
//
//    Returns:
//        - True if the function succeeded,
//        - False if the trees were not available.
//============================================================================
bool
SourceFile::GetCurrentDeclTrees
(
    NorlsAllocator *pAllocator,
    ParseTree::FileBlockStatement **ppFileBlockStatement //[out] Contains the declaration trees.
)
{
    AssertIfNull(ppFileBlockStatement);

    // Get and cache the parse trees...
    IfFailThrow(GetDeclTrees(
        pAllocator,
        pAllocator,
        NULL, //&Errors,
        NULL,
        NULL,
        ppFileBlockStatement));

    return *ppFileBlockStatement != NULL;
}

//============================================================================
// GetCurrentDeclChangeIndex:
//
//    Returns the change index that corresponds to the current
//    state of the file.
//============================================================================
ULONG
SourceFile::GetCurrentDeclChangeIndex
(
)
{
    return m_iCurrentDeclChangeIndex;
}

//============================================================================
// GetCurrentBoundChangeIndex:
//
//    Returns the change index that corresponds to the current
//    bound state of the file.
//============================================================================
ULONG
SourceFile::GetCurrentBoundChangeIndex
(
)
{
    return m_iCurrentBoundChangeIndex;
}

//============================================================================
// GetCurrentBindingChangeIndex:
//
//    Returns the current binding change index
//  This index is incremented whenever a file is decompiled or it moves from declared to bound
//============================================================================
ULONG
SourceFile::GetCurrentBindingChangeIndex
(
)
{
    return m_iCurrentBindingChangeIndex;
}

//============================================================================
// IncrementCurrentBindingChangeIndex:
//
//    Sets the current binding change index
//    This index is incremented whenever a file is decompiled or it moves from declared to bound
//============================================================================
void
SourceFile::IncrementCurrentBindingChangeIndex()
{
    m_iCurrentBindingChangeIndex++;
}

//============================================================================
// GetSourceChangeIndex:
//
//    Returns the change index that corresponds to the current
//    source file changes.
//============================================================================
ULONG
SourceFile::GetSourceChangeIndex
(
)
{
    return m_iSourceChangeIndex;
}

//============================================================================
// HasParseErrors:
//
//    Returns whether the file had any parse errors the last time it was processed..
//============================================================================
bool
SourceFile::HasParseErrors
(
)
{
    return m_HasParseErrors;;
}

//============================================================================
// SignalSourceChange:
//
//    Used to notify us of a change in the source file.
//    We increment the tracker and release the current parse trees if any.
//============================================================================
void
SourceFile::SignalSourceChange
(
)
{
    m_iSourceChangeIndex++;

    if (m_pProject)
    {
        m_pProject->IncrementSourceChangeIndex();
    }
}

//-------------------------------------------------------------------------------------------------
//
// Transfers FileCodeModel to a new file.
//
void SourceFile::TransferFileCodeModelTo(SourceFile * pNewFile)
{
    AssertIfNull(pNewFile);
    // Release and invalidate any existing FileCodeModel on the new file.
    if (pNewFile->m_pFileCodeModel)
    {
        pNewFile->m_pFileCodeModel->Invalidate();
        RELEASE(pNewFile->m_pFileCodeModel);
    }

    // Transfter code model to the new file.
    if (m_pFileCodeModel)
    {
        pNewFile->m_pFileCodeModel = m_pFileCodeModel;
        m_pFileCodeModel = NULL;
        pNewFile->m_pFileCodeModel->SetSourceFile(pNewFile);
    }
}

void SourceFile::CopyMetaToken(NorlsAllocator *pnra)
{
    m_mdENCTokenHashTable.clear();
    m_mdENCTokenHashTable = m_mdTokenHashTable;

#if DEBUG
    if (VSFSWITCH(fENCDumpMetaTokens) && ::IsDebuggerPresent())
    {
        ::OutputDebugString(L"<MetaTokens filename=\"");
        ::OutputDebugString(m_pstrFileName);
        ::OutputDebugString(L"\">\n");
    }

    for ( auto it = m_mdTokenHashTable.begin(); it != m_mdTokenHashTable.end(); it++ )
    {
        auto key = it->first;
        auto mdtk = it->second;
        WCHAR buffer[MaxStringLengthForParamSize];
        _ltow_s(mdtk, buffer, _countof(buffer), 16);
        ::OutputDebugString(buffer);
        ::OutputDebugString(L"\n");
    }

    if (VSFSWITCH(fENCDumpMetaTokens) && ::IsDebuggerPresent())
    {
        ::OutputDebugString(L"</MetaTokens>\n");
    }
#endif
}

void SourceFile::GetMethodLocation
(
    ENCBuilder *pENCBuilder,
    bool fUpdate
)
{
    if (pENCBuilder)
    {
#if DEBUG
        if (VSFSWITCH(fENCDumpMethodLocations) && ::IsDebuggerPresent())
        {
            ::OutputDebugString(L"<GetMethodLocation filename=\"");
            ::OutputDebugString(m_pstrFileName);
            ::OutputDebugString(L"\">\n");
        }
#endif
        for ( auto it = m_SymbolListHashTable.begin(); it != m_SymbolListHashTable.end(); ++it )
        {
            BCSYM_NamedRoot * pNamed = it->second;
            if (pNamed && pNamed->HasLocation())
            {
                // Skip partial method declarations so that they don't affect position of the
                // partial method implementation itself.
                if (pNamed->IsMethodImpl() && !pNamed->PProc()->IsPartialMethodDeclaration())
                {
                    pENCBuilder->AddMethodLocation(pNamed, fUpdate);
                }
                else if (pNamed->IsVariableWithValue() && pNamed->PVariable()->GetVarkind() == VAR_Member)
                {
                    pENCBuilder->AddFieldLocation(pNamed, fUpdate);
                }
            }
        }
#if DEBUG
        if (VSFSWITCH(fENCDumpMethodLocations) && ::IsDebuggerPresent())
        {
            ::OutputDebugString(L"</GetMethodLocation>\n");
        }
#endif
    }
}

bool SourceFile::IsCachedTemporaryName(BCSYM_Proc *pProc, _In_z_ STRING *psTempName)
{
    if (GetCompilerSharedState()->IsInBackgroundThread())
    {
        return false;
    }

    ENCBuilder *pENCBuilder = GetProject()->GetENCBuilder();
    if (pENCBuilder && pProc && psTempName)
    {
        SymbolHash hash;
        if (FAILED(pENCBuilder->GetSymbolHashKey(pProc, &hash)))
        {
            return false;
        }

        TempLocalImage **ppTempLocalImage = NULL;

        // The reason for the m_fCachingTemporaryNames check is that the TemporaryManager is 
        // an IDE aware class and will call into this method to ensure that we are not 
        // owerwriting cached temporaries
        if (m_fCachingTemporaryNames && (ppTempLocalImage = pENCBuilder->GetCachedTempLocalLayout(hash)) != NULL)
        {
            // VSWhidbey[451206] Function has changed by ENCBuilder before, return TempName based on the list cached by ENCBuilder.
            for (ULONG i = 0; i < (*ppTempLocalImage)->daTempLocalName.Count(); ++i)
            {
                if (psTempName == (*ppTempLocalImage)->daTempLocalName.Element(i))
                {
                    return true;
                }
            }
        }
        else
        {
            TemporaryManager* pTempMgr;
            if ( MapUtil::TryGetValue(m_ENCTemporaryManagers, hash, __ref pTempMgr) )
            {
                TemporaryIterator tempIter;
                tempIter.Init(pTempMgr, true);
                Temporary *pTemp = NULL;
                while (pTemp = tempIter.GetNext())
                {
                    if (pTemp->Symbol->GetName() == psTempName)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void SourceFile::CacheNewTemporaryName(BCSYM_Proc *pProc, _In_z_ STRING *psNewTempName)
{
    ENCBuilder *pENCBuilder = GetProject()->GetENCBuilder();
    if (pENCBuilder && pProc && psNewTempName)
    {
        SymbolHash hash;
        if (SUCCEEDED(pENCBuilder->GetSymbolHashKey(pProc, &hash)))
        {
            TemporaryManager* pTempMgr;
            if ( MapUtil::TryGetValue(m_ENCTemporaryManagers, hash, __ref pTempMgr) )
            {
                MultimapUtil::InsertPair(__ref m_ENCUsedTemporaryName, pTempMgr, psNewTempName);
            }
        }
    }
}

void 
SourceFile::GetENCTemporaryManagers( std::vector<TemporaryManager*>& managers )
{
    if ( 0 == m_ENCTemporaryManagers.size() ) 
    {
        return;
    }

    for ( auto it = m_ENCTemporaryManagers.begin(); it != m_ENCTemporaryManagers.end(); it++ )
    {
        managers.push_back(it->second);
    }
}

void SourceFile::InitENCTemporaryManagers(NorlsAllocator *pnra)
{
    if (pnra)
    {
        m_ENCTemporaryManagers.clear();
        m_ENCUsedTemporaryName.clear();
        m_fCachingTemporaryNames = false;
    }
}

//-------------------------------------------------------------------------------------------------
//
// This method will interperet the passed in procedure which is a part of the file.  The goal of
// this interpretation call is to calculate the locals for this method.  The set of temporaries
// generated are cached.  Once an ENC edit is applied later on we need to make sure that the 
// temporaries are generated in the same order and of the same type to ensure everything will
// line up on the CLR side.  
//
// *** WARNING ***
//
// This method caches the full TemporaryManager instance and will use it accross a decompilation
// boundary.  This is VERY dangerous as the Type field contained within the Temporary variable 
// structure and the associated procedure will not survive a decompilation.  The code is careful
// or lucky (take your pick) and does not access these fields so it is safe as written. 
//
// This should be rewritten to only store the subset of the temporary manager which is valid for
// the lifetime of what we store
//
//-------------------------------------------------------------------------------------------------
void SourceFile::CacheTemporaryManager
(
    _In_ ENCBuilder *pENCBuilder,
    _In_ const SymbolHash &hash,
    _In_ BCSYM_Proc *pProc,
    _In_ Text *pText
)
{
    ThrowIfNull(pENCBuilder);
    ThrowIfNull(pText);
    ThrowIfNull(pProc);

    HRESULT hr = S_OK;
    ErrorTable Errors(m_pCompiler,GetProject(),NULL);
    Cycles cycle(m_pCompiler, pENCBuilder->GetSessionNorlsAllocator(), &Errors, ERRID_SubNewCycle1, ERRID_SubNewCycle2);
    ILTree::ILNode *ptree = NULL;
    // Handle cycles.
    if (pProc->IsInstanceConstructor())
    {
        cycle.AddSymbolToVerify(pProc);
    }

    m_fCachingTemporaryNames = true;
    pENCBuilder->ResetCachedTempLocalLayoutAccess(hash);

    NorlsAllocator nra(NORLSLOC);
    TransientSymbolStore transients(m_pCompiler, &nra);

    IfFailThrow(GetBoundMethodBodyTrees(
        pProc,
        pENCBuilder->GetSessionNorlsAllocator(),
        &Errors,
        &cycle,
        NULL,
        &transients,
        pText,
        pENCBuilder->GetENCCompilationCaches(),
        gmbMergeAnonymousTypes,
        &ptree));

    m_fCachingTemporaryNames = false;

    cycle.FindCycles();
    if (ptree && !Errors.HasErrors() && ptree->bilop == SB_PROC && ptree->AsProcedureBlock().ptempmgr)
    {
        SymbolHash hashProc;
        if (SUCCEEDED(pENCBuilder->GetSymbolHashKey(pProc, &hashProc)))
        {
            if ( !MapUtil::ContainsKey(m_ENCTemporaryManagers, hashProc) )
            {
                m_ENCTemporaryManagers[hashProc] = ptree->AsProcedureBlock().ptempmgr;
            }
        }

        if (transients.SymbolCount() > 0)
        {
            pENCBuilder->CacheClosureVariables(&transients, &ptree->AsProcedureBlock());
        }
    }
}

//-------------------------------------------------------------------------------------------------
//
// This function is used to get the previous name of the temporary variable with the specified 
// Type, life time and location within the procedure.  This is important as we want to keep 
// temporaries as consistent as possible between ENC changes so that we don't loose values
//
// This function is *** VERY *** dangerous.  The TemporaryManager instance used here is in a very
// odd state.  The actual memory for the values is still alive because it's from the ENC session
// allocator.  However the underlying symbols which are referenced from the class and the 
// contained Temporary instances are not guaranteed to be alive and in fact are almost certainly
// no longer valid do to a decompilation.
//
// Even more odd, the Symbol member on the Temporary structure is partially valid at this point.  The
// actual Symbol lives on the ENC session allocator and hence is alive and can be talked to, but
// the Type pointed to by the Symbol must be considered dead
//
//-------------------------------------------------------------------------------------------------
STRING *
SourceFile::GetCachedTemporaryName
(
    BCSYM_Proc *pProc,
    BCSYM *pType,
    LifetimeClass Lifetime,
    const Location *pTextSpan
)
{
    if (GetCompilerSharedState()->IsInBackgroundThread())
    {
        return NULL;
    }

    STRING *TempName = NULL;
    ENCBuilder *pENCBuilder = GetProject()->GetENCBuilder();
    if (pENCBuilder && pProc && pType)
    {
        SymbolHash hash;

        if (FAILED(pENCBuilder->GetSymbolHashKey(pProc, &hash)))
        {
            return NULL;
        }

        TempLocalImage **ppTempLocalImage = NULL;
        if (m_fCachingTemporaryNames && (ppTempLocalImage = pENCBuilder->GetCachedTempLocalLayout(hash)) != NULL)
        {
            // VSWhidbey[451206] Function has changed by ENCBuilder before, return TempName based on the list cached by ENCBuilder.
            TempName = (*ppTempLocalImage)->daTempLocalName.Element((*ppTempLocalImage)->ulReadIndex++);
        }
        else
        {
            auto it = m_ENCTemporaryManagers.find(hash);
            if (it != m_ENCTemporaryManagers.end() )
            {
                TemporaryManager *pTempMgr = it->second;
                Temporary *pTemp = NULL;
                TemporaryIterator tempIter;
                tempIter.Init(pTempMgr, true);
                StringBuffer sb;
                pType->GetBasicRep(m_pCompiler,NULL,&sb);
                STRING *TypeName = NULL;
                if (sb.GetStringLength() > 0)
                {
                    TypeName = m_pCompiler->AddString(sb.GetString());
                }
                if (TypeName)
                {
                    while (pTemp = tempIter.GetNext())
                    {
                        STRING *psTempName = pTemp->Symbol->GetName();
                        if (StringPool::IsEqual(TypeName, pTemp->VariableTypeName) &&
                            Lifetime == pTemp->Lifetime &&
                            pTemp->Symbol && 
                            !MultimapUtil::ContainsPair(m_ENCUsedTemporaryName, pTempMgr, psTempName) )
                        {
                            if (Lifetime == LifetimeLongLived)
                            {
                                if (pTemp->Symbol->HasLocation() &&
                                    pTextSpan &&
                                    pTemp->Symbol->GetLocation()->IsOverlappedInclusive(pTextSpan))
                                {
                                    TempName = psTempName;
                                    MultimapUtil::InsertPair(__ref m_ENCUsedTemporaryName, pTempMgr, psTempName);
                                    break;
                                }
                            }
                            else
                            {
                                TempName = psTempName;
                                MultimapUtil::InsertPair( __ref m_ENCUsedTemporaryName, pTempMgr, psTempName);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return TempName;
}

#endif IDE

void SourceFile::SetFileContainsAssemblyKeyFileAttr
(
    bool fContains,
    _In_opt_z_ STRING *pstrKeyFileName
)
{
    VSASSERT(m_pProject && !m_pProject->IsMetaData(), "Source project expected!!!");

    if (fContains)
    {
        m_pstrKeyFileNameFromAttr = pstrKeyFileName;
        m_pProject->SetFileWithAssemblyKeyFileAttr(this, pstrKeyFileName);
    }
    else if (this == m_pProject->GetFileWithAssemblyKeyFileAttr())
    {
        m_pstrKeyFileNameFromAttr = NULL;
        m_pProject->SetFileWithAssemblyKeyFileAttr(NULL, NULL);
    }
}

void SourceFile::SetFileContainsAssemblyKeyNameAttr
(
    bool fContains,
    _In_opt_z_ STRING *pstrKeyContainerName
)
{
    VSASSERT(m_pProject && !m_pProject->IsMetaData(), "Source project expected!!!");

    if (fContains)
    {
        m_pstrKeyContainerNameFromAttr = pstrKeyContainerName;
        m_pProject->SetFileWithAssemblyKeyNameAttr(this, pstrKeyContainerName);
    }
    else if (this == m_pProject->GetFileWithAssemblyKeyNameAttr())
    {
        m_pstrKeyContainerNameFromAttr = NULL;
        m_pProject->SetFileWithAssemblyKeyNameAttr(NULL, NULL);
    }
}

void SourceFile::SetFileContainsAssemblyVersionAttr
(
    bool fContains,
    _In_opt_z_ STRING *pstrVersion
)
{
    VSASSERT(m_pProject && !m_pProject->IsMetaData(), "Source project expected!!!");

    if (fContains)
    {
        m_pProject->SetFileWithAssemblyKeyVersionAttr(this, pstrVersion);
    }
    else if (this == m_pProject->GetFileWithAssemblyVersionAttr())
    {
        m_pProject->SetFileWithAssemblyKeyVersionAttr(NULL, NULL);
    }
}

//============================================================================
// Tries to find the BCSYM associated with a Location.
//
// Return: BCSYM if found, or NULL if not.
//============================================================================
HRESULT SourceFile::GetSymbolOfLocation
(
    Location *pLoc,
    BCSYM_NamedRoot **ppsym
)
{
    BCSYM_NamedRoot* pFound;
    if ( !MultimapUtil::TryGetFirstValue(m_SymbolListHashTable, *pLoc, pFound) )
    {
        pFound = NULL;
    }
    *ppsym = pFound;
    return NOERROR;
}

#if DEBUG
//============================================================================
// Dumps the contents of the hash table to Debug output.
//============================================================================
void SourceFile::DumpSymbolsHashTable()
{
    DebPrintf("\n>>>>>>>> Start HashTable Dump for file %S <<<<<<<<", GetFileName() ? GetFileName() : L"");
    for ( auto it = m_SymbolListHashTable.begin(); it != m_SymbolListHashTable.end(); ++it )
    {
        BCSYM_NamedRoot *pNamed = it->second;
        DebPrintf("\n\t{Location (%d,%d) to (%d,%d), Symbol Name = \"%S\"}",
            pNamed->GetLocation()->m_lBegLine,
            pNamed->GetLocation()->m_lBegColumn,
            pNamed->GetLocation()->m_lEndLine,
            pNamed->GetLocation()->m_lEndColumn,
            pNamed->GetName());
    }

    DebPrintf("\n>>>>>>>> End HashTable Dump for file %S   <<<<<<<<\n", GetFileName() ? GetFileName() : L"");
}

//============================================================================
// Dumps the contents of the hash table to Debug output.
//============================================================================
void SourceFile::DumpSymbolsHashTableStats()
{
    DebPrintf("\n>>>>>>>> Start HashTable States Dump for file %S <<<<<<<<", GetFileName() ? GetFileName() : L"");
    DumpSymbolsHashTable();
    DebPrintf("\n>>>>>>>>  End HashTable States Dump for file %S  <<<<<<<<\n", GetFileName() ? GetFileName() : L"");
}
#endif DEBUG

//============================================================================
// Get the procedure symbol whose scope contains a line
//============================================================================

HRESULT
SourceFile::GetMethodContainingLine
(
    long lLineNum,
    BCSYM_MethodImpl** pMethodFound
)
{
    HRESULT hr = NOERROR;

    *pMethodFound = NULL;

    if ( m_cs < CS_Declared )
    {
        RRETURN( E_FAIL );
    }

    BCITER_Classes iterClasses;
    iterClasses.Init( this, true );

    BCSYM_Class* pClass;
    while ( (pClass = iterClasses.Next()) != NULL )
    {
        BCITER_CHILD iter;
        iter.Init(pClass, false /*fBindToUnbindableMethods*/, true /*fLookOnlyInCurrentPartialType*/);

        BCSYM_NamedRoot* pnamed;
        while ( (pnamed = iter.GetNext()) != NULL )
        {
            BCSYM_MethodImpl* pMethodImpl;
            if ( pnamed->IsMethodImpl() )
            {
                pMethodImpl = pnamed->PMethodImpl();

                if (DoesMethodImplContainLine(lLineNum, pMethodImpl))
                {
                    *pMethodFound = pMethodImpl;
                    break;
                }
            }
            else if (pnamed->IsProperty())
            {
                if (pnamed->PProperty()->GetProperty() && !pnamed->PProperty()->GetProperty()->IsSyntheticMethod())
                {
                    if (pnamed->PProperty()->GetProperty()->IsMethodImpl())
                    {
                        pMethodImpl = pnamed->PProperty()->GetProperty()->PMethodImpl();

                        if (DoesMethodImplContainLine(lLineNum, pMethodImpl))
                        {
                            *pMethodFound = pMethodImpl;
                            break;
                        }
                    }
                }

                if (pnamed->PProperty()->SetProperty() && !pnamed->PProperty()->SetProperty()->IsSyntheticMethod())
                {
                    if (pnamed->PProperty()->SetProperty()->IsMethodImpl())
                    {
                        pMethodImpl = pnamed->PProperty()->SetProperty()->PMethodImpl();
                        if (DoesMethodImplContainLine(lLineNum, pMethodImpl))
                        {
                            *pMethodFound = pMethodImpl;
                            break;
                        }
                    }
                }
            }
            else if (pnamed->IsEventDecl() && pnamed->PEventDecl()->IsBlockEvent())
            {
                if (pnamed->PEventDecl()->GetProcAdd() && !pnamed->PEventDecl()->GetProcAdd()->IsSyntheticMethod())
                {
                    if (pnamed->PEventDecl()->GetProcAdd()->IsMethodImpl())
                    {
                        pMethodImpl = pnamed->PEventDecl()->GetProcAdd()->PMethodImpl();

                        if (DoesMethodImplContainLine(lLineNum, pMethodImpl))
                        {
                            *pMethodFound = pMethodImpl;
                            break;
                        }
                    }
                }

                if (pnamed->PEventDecl()->GetProcRemove() && !pnamed->PEventDecl()->GetProcRemove()->IsSyntheticMethod())
                {
                    if (pnamed->PEventDecl()->GetProcRemove()->IsMethodImpl())
                    {
                        pMethodImpl = pnamed->PEventDecl()->GetProcRemove()->PMethodImpl();

                        if (DoesMethodImplContainLine(lLineNum, pMethodImpl))
                        {
                            *pMethodFound = pMethodImpl;
                            break;
                        }
                    }
                }

                if (pnamed->PEventDecl()->GetProcFire() && !pnamed->PEventDecl()->GetProcFire()->IsSyntheticMethod())
                {
                    if (pnamed->PEventDecl()->GetProcFire()->IsMethodImpl())
                    {
                        pMethodImpl = pnamed->PEventDecl()->GetProcFire()->PMethodImpl();

                        if (DoesMethodImplContainLine(lLineNum, pMethodImpl))
                        {
                            *pMethodFound = pMethodImpl;
                            break;
                        }
                    }
                }
            }
        }

        if (!*pMethodFound)
        {
            // Also, loop over partial method declarations.
            iter.Init(pClass, true /*fBindToUnbindableMethods*/, true /*fLookOnlyInCurrentPartialType*/);
            while ((pnamed = iter.GetNext()) != NULL)
            {
                if (pnamed->IsMethodImpl())
                {
                    BCSYM_MethodImpl* pMethodImpl = pnamed->PMethodImpl();
                    if (pMethodImpl->IsPartialMethodDeclaration() &&
                        DoesMethodImplContainLine(lLineNum, pMethodImpl))
                    {
                        *pMethodFound = pMethodImpl;
                        break;
                    }
                }
            }
        }
    }

    RRETURN( hr );
}

bool
SourceFile::DoesMethodImplContainLine
(
    long lLine,
    BCSYM_MethodImpl *pMethodImpl
)
{
    if (pMethodImpl)
    {
        const CodeBlockLocation* pCodeBlock = pMethodImpl->GetProcBlock();

        if ((lLine >= pCodeBlock->m_lBegLine) && (lLine <= pCodeBlock->m_lEndLine))
        {
            return true;
        }
    }

    return false;
}

#if IDE
class  SourceFileTextCache
{
    struct SourceFileTextCacheNode : ZeroInit<SourceFileTextCacheNode>
    {
        // SourceFile version when cached.
        ULONG m_lCachedSourceChangeIndex;

        // The text.
        WCHAR *m_wszText;

        // The size.
        size_t m_cchText;

        // Where to store the text.
        NorlsAllocator m_nraText;

        SourceFileTextCacheNode() : m_nraText(NORLSLOC) {}

    private:
        // We do not want this copied
        SourceFileTextCacheNode( const SourceFileTextCacheNode& );
        SourceFileTextCacheNode& operator=(const SourceFileTextCacheNode& );
    };

    typedef std::map<SourceFile*, RefCountedPtr<SourceFileTextCacheNode>> CacheMap;

    // m_cachedNodes holds on to sourcefile text so we don't need to block the UI thread while
    // reading the text from disk.  The cache is cleared if its size ever becomes > 10 elements 
    // or if 5 seconds have elapsed since the last entry was added.  It's intended to be used
    // by UI thread functionality (e.g. Solution Navigator) that needs to bind method information.
    CacheMap m_cachedNodes;
    unsigned m_lastInsertionTime;
    CTinyLock m_lock;

    CacheMap::iterator GetCachedPosition(_In_ SourceFile* pSourceFile)
    {
        CTinyGate gate(&m_lock);
        CacheMap::iterator it = m_cachedNodes.find(pSourceFile);
        if (it != m_cachedNodes.end())
        {
            if (it->second->m_lCachedSourceChangeIndex == pSourceFile->GetSourceChangeIndex())
            {
                return it;
            }
            else
            {
                m_cachedNodes.erase(it); // This text is stale, delete it from the cache.
            }
        }
        return m_cachedNodes.end(); // not found
    }

public:
    
    SourceFileTextCache() :m_lastInsertionTime(0) {}

    void ClearCacheIfStale()
    {
        unsigned elapsed = GetTickCount() - m_lastInsertionTime;

        CTinyGate gate(&m_lock);
        const unsigned MaxCacheDuration = 5000; 
        if (elapsed > MaxCacheDuration && !m_cachedNodes.empty())
        {
            m_cachedNodes.clear();
        }
    }

    bool TryGetCachedSourceFileText(
        _In_ SourceFile* pSourceFile,
        _In_ NorlsAllocator * pnra,
        __deref_out_ecount_opt(* pcchText) WCHAR * * pwszText,
        _Out_ size_t * pcchText)
    {
        CTinyGate gate(&m_lock);
        auto it = GetCachedPosition(pSourceFile);
        if (it != m_cachedNodes.end())
        {
            *pcchText = it->second->m_cchText;
            *pwszText = (WCHAR*) pnra->Alloc(sizeof(WCHAR) * (*pcchText + 1));
            ::memcpy(*pwszText, it->second->m_wszText, sizeof(WCHAR) * (*pcchText));
            (*pwszText)[*pcchText] = 0; // ensure this string is null terminated
            return true;
        }
        return false;
    }

    CComPtr<IVsTask> CacheText(_In_ SourceFile* pSourceFile)
    {
        CComPtr<IVsTask> spTask;
        CTinyGate gate(&m_lock);
        m_lastInsertionTime = GetTickCount();
        if ( (pSourceFile->GetSourceFileView() != NULL) &&
             (pSourceFile->GetSourceFileView()->IsIVsTextBufferLoaded() == true) )
        {
            return spTask; // this file is open so we don't need to cache it
        }
        if (GetCachedPosition(pSourceFile) != m_cachedNodes.end())
        {
            return spTask; // this file is already cached
        }

        ULONG lCachedSourceChangeIndex = pSourceFile->GetSourceChangeIndex();
        CComPtr<CompilerProject> spProject = pSourceFile->GetProject();  // Keep the project alive until we finish reading the cache.
        TextFileCacheRef* pRef = pSourceFile->GetTextFile()->GetTextFileCacheRef();
        if (pRef)
        {
            // copy in case the original is deleted.
            TextFileCacheRef* pFileCacheRef = new (zeromemory) TextFileCacheRef(pRef->GetOffset(), pRef->GetSize(), pRef->GetTimestamp(), pRef->IsDetectUTF8WithoutSig());
            pFileCacheRef->SetUnicode(true);

            CComPtr<IVsTaskSchedulerService> spTaskSchedulerService = GetCompilerPackage()->GetTaskSchedulerService();
            CComPtr<IVsTaskBody> spTaskBody;
            if (SUCCEEDED(VsTaskLibraryHelper::CreateTaskBody([=](VsTaskBody_Params) -> HRESULT
            {
                auto node = RefCountedPtr<SourceFileTextCacheNode>(new SourceFileTextCacheNode());
                node->m_lCachedSourceChangeIndex = lCachedSourceChangeIndex; // pSourceFile might be dead by now, used the cached value

                // C++ doesn't seem to allow passing and deleting pFileCacheRef directly.
                TextFileCacheRef* pNonHoistedRef = pFileCacheRef;
                bool succeeded = TextFile::ReadUnicodeSourceFileCache(
                    &node->m_nraText,
                    spProject->GetSourceFileCache(),
                    &pNonHoistedRef,
                    NULL, // filename not needed
                    &node->m_wszText,
                    &node->m_cchText);

                delete pNonHoistedRef;

                if (succeeded)
                {
                    CTinyGate gate(&m_lock); // store the text in the cache
                    const int MaxCacheSize = 10;
                    if (m_cachedNodes.size() > MaxCacheSize)
                    {
                        m_cachedNodes.clear();
                    }

                    m_cachedNodes[pSourceFile] = node;
                    m_lastInsertionTime = GetTickCount();
                }

                return S_OK;
            }, &spTaskBody)))
            {
                IfFailThrow(spTaskSchedulerService->CreateTask(VSTC_BACKGROUNDTHREAD, spTaskBody, &spTask));
                IfFailThrow(spTask->Start());
            }
            else
            {
                delete pFileCacheRef;
            }
        }
        return spTask;
    }
};

SourceFileTextCache g_SourceFileTextCache;  // one and only SourceFileTextCache

bool SourceFile::TryGetCachedSourceFileText(
    _In_ NorlsAllocator * pnra,
    __deref_out_ecount_opt(* pcchText) WCHAR * * pwszText,
    _Out_ size_t * pcchText)
{
    return g_SourceFileTextCache.TryGetCachedSourceFileText(this, pnra, pwszText, pcchText);
}

CComPtr<IVsTask> SourceFile::CacheText()
{
    return g_SourceFileTextCache.CacheText(this);
}

void SourceFile::ClearSourceFileCachedTextIfStale()
{
    g_SourceFileTextCache.ClearCacheIfStale();
}
#endif

HRESULT
SourceFile::ParseCodeBlock
(
    Text *ptext,
    NorlsAllocator *pnra,
    ErrorTable *perrortable,
    BCSYM_Container* pConditionalCompilationConstants,
    const CodeBlockLocation* pCodeBlock,
    const CodeBlockLocation* pProcBlock,   // passed only when method def is requested (need sig location for codegen)
    ParseTree::Statement::Opcodes MethodBodyKind,
    ParseTree::MethodBodyStatement** pptree,
    MethodDeclKind methodDeclKind
)
{
    VB_ENTRY();
    Text text;
    IfFalseGo(pptree,E_UNEXPECTED);
    *pptree = NULL;
    if (ptext == NULL)
    {
        IfFailGo(text.Init(this));
        ptext = &text;
    }
    IfFailGo(::ParseCodeBlock(m_pCompiler,
                              ptext,
                              this,
                              pnra,
                              perrortable,
                              pConditionalCompilationConstants,
                              pCodeBlock,
                              pProcBlock,
                              MethodBodyKind,
                              pptree, 
                              methodDeclKind,
                              false));

    VB_EXIT_NORETURN();
Error:
    RRETURN(hr);
}

HRESULT SourceFile::GetDeclTrees
(
    NorlsAllocator *pTreeAllocator, // [in] allocator to hold the declaration trees
    NorlsAllocator *pConstantsTree, // [in] allocator to hold the constants
    ErrorTable *pErrorTable,
    BCSYM_Container **ppcontainerConditionalConstants,
    LineMarkerTable *LineMarkerTableForConditionals,
    ParseTree::FileBlockStatement **ppParseTree
)
{
    VSASSERT( pTreeAllocator && pConstantsTree, "Expecting allocators for parse tree storage" );

    VB_ENTRY();
    ParseTree::FileBlockStatement *ptree = NULL;
    Text text;
    // Initialize the iterator.
    IfFailGo(text.Init(this));

    IfFailGo(::GetDeclTrees(m_pCompiler,
                            &text,
                            this,
                            pTreeAllocator,
                            pConstantsTree,
                            pErrorTable,
                            ppcontainerConditionalConstants,
                            LineMarkerTableForConditionals,
                            &ptree));
    *ppParseTree = ptree;

    VB_EXIT_GO();

Error:
    RRETURN( hr );
}

//============================================================================
// Create the parse trees for the body of this method.
//============================================================================

HRESULT SourceFile::GetUnboundMethodBodyTrees
(
    BCSYM_Proc *pproc,
    NorlsAllocator *pnra,
    ErrorTable *perrortable,
    Text *ptext,
    ParseTree::MethodBodyStatement **pptree
)
{
    ParseTree::MethodBodyStatement *ptree;

    VSASSERT(pnra != NULL, "Expecting NRA for parse tree storage");

    NorlsAllocator *pnraTrees = pnra;

    //
    // Generate the parse trees for this method.
    //
    VB_ENTRY();

    // determine the method kind.
    // 

    ParseTree::Statement::Opcodes MethodBodyKind = ParseTree::Statement::SyntaxError;

    if (pproc->GetType())
    {
        if (pproc->IsPropertyGet())
        {
            MethodBodyKind = ParseTree::Statement::PropertyGetBody;
        }
        else if (pproc->IsUserDefinedOperatorMethod())
        {
            MethodBodyKind = ParseTree::Statement::OperatorBody;
        }
        else
        {
            MethodBodyKind = ParseTree::Statement::FunctionBody;
        }
    }
    else
    {
        if (pproc->IsPropertySet())
        {
            MethodBodyKind = ParseTree::Statement::PropertySetBody;
        }
        else if (pproc->CreatedByEventDecl())
        {
            BCSYM_EventDecl *Event = pproc->CreatedByEventDecl();

            if (pproc == Event->GetProcAdd())
            {
                MethodBodyKind = ParseTree::Statement::AddHandlerBody;
            }
            else if (pproc == Event->GetProcRemove())
            {
                MethodBodyKind = ParseTree::Statement::RemoveHandlerBody;
            }
            else
            {
                VSASSERT(pproc == Event->GetProcFire(), "Event method not connected to event!!!");

                MethodBodyKind = ParseTree::Statement::RaiseEventBody;
            }
        }
        else
        {
            MethodBodyKind = ParseTree::Statement::ProcedureBody;
        }
    }


    if (pproc->IsMethodImpl())
    {
        hr = ParseCodeBlock(ptext,
                            pnraTrees,
                            perrortable,
                            m_pConditionalCompilationConstants,
                            pproc->PMethodImpl()->GetCodeBlock(),
                            pproc->PMethodImpl()->GetProcBlock(),
                            MethodBodyKind,
                            &ptree,
                            pproc->IsAsyncKeywordUsed() ? AsyncKind :
                                pproc->IsIteratorKeywordUsed() ? IteratorKind :
                                    NormalKind);
    }
    else
    {
        VSASSERT(pproc->IsSyntheticMethod(), "What else?");
        BCSYM_SyntheticMethod * SyntheticMethod = pproc->PSyntheticMethod();

        ParseTree::StatementList *pparsedCode = SyntheticMethod->GetParsedCode();
        if (pparsedCode != NULL)
        {
            ParseTree::MethodBodyStatement *Proc = new(*pnraTrees) ParseTree::MethodBodyStatement;
            Proc->Opcode = MethodBodyKind;
            Proc->IsFirstOnLine = true;
            Proc->HasProperTermination = true;
            Proc->Children = pparsedCode;
            Proc->BodyTextSpan = Location::GetHiddenLocation();
            Proc->TextSpan = Location::GetHiddenLocation();
            Proc->LocalsCount = SyntheticMethod->GetLocalsCount();

            ptree = Proc;

            hr = S_OK;
        }
        else
        {
            Scanner tsBody(m_pCompiler,
                               SyntheticMethod->GetCode(),
                               SyntheticMethod->GetCodeSize(),
                               0,
                               HIDDEN_LINE_INDICATOR,
                               0);

            VSASSERT(GetProject() && GetProject()->GetCompilerHost(), "Unexpected!");

            Parser parse(pnraTrees, m_pCompiler, GetCompilerHost(), m_pProject->IsXMLDocCommentsOn(), m_pProject->GetCompilingLanguageVersion());

            IfFailGo(parse.ParseMethodBody(&tsBody,
                                           perrortable,
                                           GetProject()->GetProjectLevelCondCompScope(),
                                           m_pConditionalCompilationConstants,
                                           MethodBodyKind,
                                           &ptree));
        }
    }

    VB_EXIT_GO();

#if DEBUG
    if (VSFSWITCH(fDumpUnboundMethodTrees))
    {
        DebPrintf("//============================================================================\n");
        DebPrintf("// Parse trees for %S.\n", pproc->GetErrorName(m_pCompiler));
        DebPrintf("//============================================================================\n");

        ParseTree::DumpStatement(ptree, m_pCompiler);
    }

#endif DEBUG

    // Return the results.
    *pptree = ptree;

Error:
    RRETURN(hr);
}

//============================================================================
// Create the bound trees for the body of this method.
//============================================================================

HRESULT SourceFile::GetBoundMethodBodyTrees
(
    BCSYM_Proc *pproc,
    NorlsAllocator *pnra,
    ErrorTable *perrortable,
    class Cycles *pcycles,
    CallGraph *pCallTracking,
    TransientSymbolStore *pCreatedDuringInterpretation,
    Text *ptext,
    CompilationCaches *pCompilationCaches,
    DWORD dwFlags,
    ILTree::ILNode **pptree,
    ParseTree::MethodBodyStatement **ppUnboundTree,
    AlternateErrorTableGetter AltErrTablesForConstructor,
    DynamicArray<BCSYM_Variable *> *pENCMembersToRefresh
)
{
    bool MergeAnonymousTypes = dwFlags & gmbMergeAnonymousTypes;

    HRESULT hr = S_OK;

#if IDE 
    BoundMethodBodyNode *CachedBoundMethodBody = NULL;

    if ((dwFlags & gmbUseCache) && GetProject() && !(dwFlags & gmbLowerTree))
    {
        VSASSERT(!pCreatedDuringInterpretation || 
            pCreatedDuringInterpretation == GetProject()->GetBoundMethodBodyTransients(), 
            "If using the project method body cache, transient symbols are stored in the project transient symbol store.");

        // Look in our cache for the bound method body
        // boundMethodBodies is an STL dictionary.  If a value doesn't exist for the key, it inserts one
        // and calls the default constructor on it.  CachedBoundMethodBody therefore is always the address of the 
        // BoundMethodBodyNode stored in the dictionary.
        auto boundMethodBodies = GetProject()->GetBoundMethodBodies();
        CachedBoundMethodBody = &((*boundMethodBodies)[pproc]);

        if (CachedBoundMethodBody->pBound)
        {
            // Make sure the cached item is still valid
            if (CachedBoundMethodBody->lCachedBoundChangeIndex == m_iCurrentBoundChangeIndex &&
                CachedBoundMethodBody->lCachedSourceChangeIndex == m_iSourceChangeIndex &&
                CachedBoundMethodBody->MergeAnonymousTypes == MergeAnonymousTypes)
            {
                *pptree = CachedBoundMethodBody->pBound;

                if (ppUnboundTree)
                {
                    *ppUnboundTree = CachedBoundMethodBody->pUnbound;
                }

                return S_OK;
            }
        }

        // Get the project-level transient symbol cache and start a transaction so that transient symbols allocated by this method body can be tracked
        pCreatedDuringInterpretation = GetProject()->GetBoundMethodBodyTransients();
        pCreatedDuringInterpretation->StartTransaction();

        // When using the cache we must use a NRLS that lives as long as the project remains at or above Bound
        pnra = GetProject()->GetBoundProjectAllocator();
    }

    m_ENCUsedTemporaryName.clear();
#endif

    ILTree::ProcedureBlock *pBound = NULL;
    ParseTree::MethodBodyStatement *pUnbound;

    //
    // Get the parse trees for this method body.
    //

    IfFailGo(GetUnboundMethodBodyTrees(pproc, pnra, perrortable, ptext, &pUnbound));

    // No tree will be returned if the file cannot be loaded.
    if (pUnbound)
    {
        //
        // Bind the trees.
        //
        ThrowIfFalse((dwFlags & gmbIDEBodies) || pCreatedDuringInterpretation);  // One of these must be true otherwise parts of the
                                                                        // compiler will crash
#if IDE

        // Set the Project being compiled here in order to ensure NoPIA type unification occurs properly
        // during interpretation.  This is only necessary for the IDE thread as the background thread 
        // is responsible for properly updating this during an actual compilation.  This is a thread local
        // variable so there is no conflict between the threads when updating
        std::auto_ptr<UpdateProjectBeingCompiled> spUpdateProject;
        if ( GetCompilerSharedState()->IsInMainThread() )
        {
            spUpdateProject.reset(new UpdateProjectBeingCompiled(m_pCompiler, GetProject()));
        }

#endif

        Semantics Analyzer(pnra,
                           perrortable,
                           m_pCompiler,
                           GetCompilerHost(),
                           this,
                           pCreatedDuringInterpretation,
                           dwFlags & gmbIDEBodies,
                           dwFlags & gmbIncludeBadExpressions);

#if IDE 
        CompilationCaches *pTempCompilationCaches = NULL;
        if (pCompilationCaches)
        {
            VSASSERT(!CachedBoundMethodBody, "We can't cache bound method data unless prna matches pCompilationCaches's norls allocator.");
            Analyzer.SetIDECompilationCaches(pCompilationCaches);
        }
        else if (CachedBoundMethodBody)
        {
            // When caching the bound method body, we need to use the bound method body allocator
            // to ensure our symbols live until the bound method body cache is cleared.
            pTempCompilationCaches = new CompilationCaches(pnra);
            Analyzer.SetIDECompilationCaches(pTempCompilationCaches);
        }
#endif

        pBound = Analyzer.InterpretMethodBody(pproc,
                                              pUnbound,
                                              pcycles,
                                              pCallTracking,
                                              NULL,
                                              AltErrTablesForConstructor,
                                              pENCMembersToRefresh,
                                              MergeAnonymousTypes);

        if (dwFlags & gmbLowerTree)
        {
                Analyzer.LowerBoundTree(&pBound->AsProcedureBlock());

        }

#if IDE 
        delete pTempCompilationCaches;
#endif

        //
        // Debug stuff.
        //

#if DEBUG
        if (VSFSWITCH(fDumpBoundMethodTrees))
        {
            BILDUMP dump(m_pCompiler);
            dump.DumpBlockTrees(pBound);
        }

#endif DEBUG

        //
        // Set the sig location for CodeGen
        // sig location starts with the first specifier and ends with the right parenthesis
        // public sub goo()
        // |--------------|
        // Since this location is not expicitly recorded in the tree, make it up from other locations and punctuators available.
        if (pproc->IsMethodImpl() && pUnbound->Definition)
        {
            ParseTree::MethodDefinitionStatement *pDef = pUnbound->Definition;
            long startProcLine = pproc->PMethodImpl()->GetProcBlock()->m_lBegLine;
            long startProcCol = pproc->PMethodImpl()->GetProcBlock()->m_lBegColumn;
            if (pDef->Specifiers)
            {
                pBound->SigLocation.m_lBegLine = pDef->Specifiers->TextSpan.m_lBegLine;
                pBound->SigLocation.m_lBegColumn = pDef->Specifiers->TextSpan.m_lBegColumn;
            }
            else
            {
                pBound->SigLocation.m_lBegLine = pDef->MethodKind.Line + startProcLine;
                pBound->SigLocation.m_lBegColumn = pDef->MethodKind.Column;
            }
            // End location is trickier:
            // "sub foo" is legal and equivalent to "Sub foo()". In this case the end of signature is
            // the end of the name (if any, for example 'get' has no name)
            long endSigLine;
            long endSigCol;
            if (pDef->ReturnType)
            {
                endSigLine = pDef->ReturnType->TextSpan.m_lEndLine;
                endSigCol = pDef->ReturnType->TextSpan.m_lEndColumn;
            }
            else
            {
                endSigLine = pDef->RightParen.Line + startProcLine;
                endSigCol = pDef->RightParen.Column;
                if (endSigLine == startProcLine && endSigCol==0)
                {
                    if (pDef->Name.Name)
                    {
                        endSigLine = pDef->Name.TextSpan.m_lEndLine;
                        endSigCol = pDef->Name.TextSpan.m_lEndColumn;
                    }
                    else
                    {
                        endSigLine = pDef->TextSpan.m_lEndLine;
                        endSigCol = pDef->TextSpan.m_lEndColumn;
                    }

                }
            }

            pBound->SigLocation.m_lEndLine = endSigLine;
            pBound->SigLocation.m_lEndColumn = endSigCol;
        }

    }

    //
    // Return the results.
    //
    *pptree = pBound;

    if (ppUnboundTree)
    {
        *ppUnboundTree = pUnbound;
    }

Error:

#if IDE 
    if (CachedBoundMethodBody)
    {
        if (SUCCEEDED(hr))
        {
            // Add the results to our cache
            CachedBoundMethodBody->pBound = pBound;
            CachedBoundMethodBody->pUnbound = pUnbound;
            CachedBoundMethodBody->TransientSymbolIter = pCreatedDuringInterpretation->TransactionSymbolIterator();
            CachedBoundMethodBody->lCachedBoundChangeIndex = m_iCurrentBoundChangeIndex;
            CachedBoundMethodBody->lCachedSourceChangeIndex = m_iSourceChangeIndex;
            CachedBoundMethodBody->MergeAnonymousTypes = MergeAnonymousTypes;

            // Commit transient symbols created during binding of this method body
            pCreatedDuringInterpretation->CommitTransaction();
        }
        else
        {
            // Rollback transient symbols created during binding of this method body
            pCreatedDuringInterpretation->AbortTransaction();
        }
    }

#endif

    RRETURN( hr );
}

//============================================================================
// Create the bound trees for the body of this lambda.
//============================================================================

HRESULT SourceFile::GetBoundLambdaBodyTrees
(
    _In_ ParseTree::LambdaBodyStatement *pUnbound,
    NorlsAllocator *pnra,
    ErrorTable *perrortable,
    TransientSymbolStore *pCreatedDuringInterpretation,
    CompilationCaches *pCompilationCaches,
    DWORD dwFlags,
    BCSYM_Class *pContainingClass,
    _Out_ ILTree::StatementLambdaBlock **pptree
)
{
    HRESULT hr = S_OK;

    ILTree::StatementLambdaBlock *pBoundLambdaBlock = NULL;
    
    // No tree will be returned if the file cannot be loaded.
    if (pUnbound)
    {
        //
        // Bind the trees.
        //
        ThrowIfFalse((dwFlags & gmbIDEBodies) || pCreatedDuringInterpretation);  // One of these must be true otherwise parts of the
                                                                        // compiler will crash
        Semantics Analyzer(pnra,
                           perrortable,
                           m_pCompiler,
                           GetCompilerHost(),
                           this,
                           pCreatedDuringInterpretation,
                           dwFlags & gmbIDEBodies,
                           dwFlags & gmbIncludeBadExpressions);

#if IDE
        if (pCompilationCaches)
        {
            Analyzer.SetIDECompilationCaches(pCompilationCaches);
        }
#endif
    
        BCSYM_Hash *pLookup =
        Analyzer.m_SymbolCreator.GetHashTable(
            NULL,
            pContainingClass,
            true,
            2,
            NULL);

        Analyzer.InitializeInterpretationState(
            NULL,
            pLookup,
            NULL,       // No cycle detection
            NULL,       // No call graph
            pCreatedDuringInterpretation,       // transient symbols
            true,       // preserve extra semantic information
            true,       // perform obsolete checks
            true,     // can interpret statements
            true,       // can interpret expressions
            false,  // can merge anonymous type templates
            NULL,       // Applied attribute context
            false);     // reset closures information

        // We are analyzing the field level lambda in the context of the constructor for the containing class because we require a
        // BCSYM_Proc and we do not have one. When we are analyzing the body of a method we always use the proc for
        // that method. Thus, we are arbitrarily using the BCSYM_Proc of the constructor of the containing class. The lambda
        // will be spit into this constructor.
        BCSYM_Proc *pConstructor = (pContainingClass->IsStdModule() || pContainingClass->IsStruct()) ?
            pContainingClass->GetSharedConstructor(pContainingClass->GetCompiler()) :
            pContainingClass->GetFirstInstanceConstructor(pContainingClass->GetCompiler());

        Analyzer.m_Procedure = pConstructor;

        ILTree::UnboundLambdaExpression *pUnboundLambda = Analyzer.InterpretLambdaExpression(pUnbound->pOwningLambdaExpression, 0);

        Type* TargetType = Analyzer.InferLambdaType(pUnboundLambda, pUnboundLambda->Loc);

        if (TargetType && !IsBad(pUnboundLambda))
        {
            ILTree::Expression *pResult = NULL;
            pResult = Analyzer.ConvertWithErrorChecking(pUnboundLambda, TargetType, 0);

            if ((pResult->bilop == SX_LAMBDA) &&
                pResult->AsLambdaExpression().IsStatementLambda)
            {
                pBoundLambdaBlock = pResult->AsLambdaExpression().GetStatementLambdaBody();
            }
        }

        //
        // Debug stuff.
        //

#if DEBUG
        if (VSFSWITCH(fDumpBoundMethodTrees))
        {
            BILDUMP dump(m_pCompiler);
            dump.DumpBlockTrees(pBoundLambdaBlock);
        }

#endif DEBUG

    }

    //
    // Return the results.
    //
    *pptree = pBoundLambdaBlock;

    RRETURN( hr );
}


// Compute MD5 hash for PDB check sum by using vscommon\crypthash.lib
HRESULT SourceFile::GetCryptHash(void *pvHashValue,DWORD cbHashSize)
{
    return this->GetTextFile()->GetCryptHash(pvHashValue,cbHashSize);
}

#if IDE 

bool 
SourceFile::IsLinkedFile()
{
    if ( !m_pProject )
    {
        return false;
    }

    return ::IsLinkedFile(m_pProject->GetHierarchy(), GetItemId());
}

bool
SourceFile::IsLinkedFileSource()
{
    return !IsLinkedFile() && GetCompilerPackage()->IsLinkedFileOrSource(m_pstrFileName);
}

bool
SourceFile::IsLinkedFileOrSource()
{
    return IsLinkedFile() || IsLinkedFileSource();
}

//============================================================================
// Get the IVsHierarchy that contains this project.
//============================================================================

IVsHierarchy *SourceFile::GetHierarchy()
{
    return m_pProject->GetHierarchy();
}

#endif IDE

#if !IDE // The mapped line helpers are only for the command line compiler

//============================================================================
// Given a line in 'this' source file, set its mapped location
//============================================================================

void SourceFile::SetMappedLineInfo(
    ParseTree::ExternalSourceDirective * pMap // [in] the line mapping information for this source file
)
{
    ParseTree::ExternalSourceDirective *pMapCount = pMap;

    // Rip through the list of #ExternalSource symbols, get the count, and throw them
    // in the m_pLineMappingTable. Assert as you go to make sure invariant of sequentialness is maintained

    m_NumMappedEntries = 0;

    // figure out how many of these there are in this module - don't count the blank ones.
    while ( pMapCount )
    {
        // The parser uses the situation: LastLine < FirstLine to indicate that an #ExternalSource was followed immediately by #End ExternalSource
        //  ( i.e. they enclosed nothing ).  Only count directives that actually enclose something.
        if ( !pMapCount->hasErrors && pMapCount->FirstLine <= pMapCount->LastLine )
        {
            m_NumMappedEntries++;
        }
        pMapCount = pMapCount->Next;
    }

    if ( m_NumMappedEntries == 0 )
    {
        m_pLineMappingTable = NULL; // Because we used a no-release allocator, we aren't stranding anything here.
        return; // nothing to do
    }

    // Farm out an array of pointers that we can use for binary search - make copies because the parse tree will disappear and we need to hang on to this info for error mapping, etc.
    m_pLineMappingTable = (ILTree::ExternalSourceDirective*) SymbolStorage()->Alloc( sizeof( ILTree::ExternalSourceDirective ) * m_NumMappedEntries );

    // Copy the elements in the linked list we were given into the table
    for ( unsigned idx(0); pMap; pMap = pMap->Next )
    {
        #if DEBUG // An essential assumption behind the Mapping code is that the list be in ascending order because it uses a binary search on the array
        if ( idx > 0 ) // The parser claims to send us things in sorted order - verify
        {
            VSASSERT( pMap->FirstLine > m_pLineMappingTable[ idx -1 ].FirstLine, "The List must be sorted in ascending order" );
        }
        #endif

        if ( pMap->hasErrors || pMap->LastLine < pMap->FirstLine )
        {
            continue; // The parser uses this situation to indicate that an #ExternalSource was followed immediately by #End ExternalSource. Ignore these.
        }

        m_pLineMappingTable[ idx ].FirstLine = pMap->FirstLine;
        m_pLineMappingTable[ idx ].LastLine = pMap->LastLine;
        m_pLineMappingTable[ idx ].ExternalSourceFileStartLine = pMap->ExternalSourceFileStartLine - 1;
        m_pLineMappingTable[ idx++ ].ExternalSourceFileName = m_pCompiler->AddString( pMap->ExternalSourceFileName );
    }
}

//============================================================================
// Given a line in 'this' source file, get its mapped location.  What this is used for is when this sourcefile
// contains code that was extracted from a another file.  For instance, a sourcefile may contain script
// blocks that were generated from a XSP page.  In order to debug into that page, we need to know where the
// code we are debugging lives at inside the other file.  This function provides that information in that given a
// line number in 'this' SourceFile, it returns the line and filename of the file where the source was generated
// from.
//
// If we are asked to map a line that doesn't fall within a #ExternalSource directive, one of two things happens.  If there are
// #ExternalSource directives in the file, then we just return the line they asked for with a flag indicating that no debug info
// should be generated for the line.  This is because the XSP guys don't want the user debugging into generated code that
// didn't explicitly come from the XSP page.
// If there are no #ExternalSource directives in the file, then we just return the line they asked for and a flag indicating that
// debug information should be generated for the line.
//
// Returns the mapped line ( zero based )
//============================================================================
long SourceFile::GetMappedLineInfo(
    long LineToMap, // [in] the line for which you want mapping information ( zero based )
    bool *pShouldHaveLineDebugInfo, // [out] indicates whether the mapped line should have debug info.
    _Deref_opt_out_z_ STRING **ppMapsToFile // [optional out] path of the file where the source for LineToMap lives
)
{
    if ( m_NumMappedEntries > 0 ) // If there is something to do
    {
        unsigned LowerBound = 0, UpperBound = m_NumMappedEntries - 1;
        // Do a binary search on the array ( the range pairs in the array are assumed to be in sorted order )
        // VSWhidbey#56614,Microsoft: Changed the while loop from true to UpperBound >= LowerBound
        //    This is doing a binary search and the code below does not check that LowerBound is less
        //    than UpperBound.  This bug ran into a situation where LowerBound was greater than UpperBound
        //    and thus we were accessing the array with a rather large index.  On x86 we have been "lucky"
        //    and have been able to access the memory location.  But, on 64-bit we explode with an AV.
        while ( UpperBound >= LowerBound )
        {
            unsigned TryThisIdx = LowerBound + ( UpperBound - LowerBound ) / 2;
            if ( LineToMap < m_pLineMappingTable[ TryThisIdx ].FirstLine ) // look left
            {
                if ( TryThisIdx > 0 )
                {
                    UpperBound = TryThisIdx-1;
                }
                else
                {
                    break; // we don't have a mapped pair for this line
                }
            }
            else if ( LineToMap > m_pLineMappingTable[ TryThisIdx ].LastLine ) // look right
            {
                if ( TryThisIdx < m_NumMappedEntries )
                {
                    LowerBound = TryThisIdx + 1;
                }
                else
                {
                    break; // not found
                }
            }
            else // found it
            {
                *pShouldHaveLineDebugInfo = TRUE;
                if ( ppMapsToFile )
                {
                    *ppMapsToFile = m_pLineMappingTable[ TryThisIdx ].ExternalSourceFileName;
                }
                return m_pLineMappingTable[ TryThisIdx ].ExternalSourceFileStartLine + LineToMap - m_pLineMappingTable[ TryThisIdx ].FirstLine;
            }
        } // while
    } // if ( m_NumMappedEntries > 0 )

    // We didn't find a mapping for the line - so we reflect the line back.  Such lines should only have debug information if there
    // were no #ExternalSource directives in this file.
    *pShouldHaveLineDebugInfo = ( m_NumMappedEntries == 0 );
    if ( ppMapsToFile )
    {
        *ppMapsToFile = GetFileName(); // report the file name though so normal error reporting has a file name to go with its location info
    }
    return LineToMap;
}

//============================================================================
// Given a line in 'this' source file, get the name of the file from which that code came from.
//============================================================================
STRING * SourceFile::GetMappedFileName(
    long ForCodeAtLine, // [in] Line # ( zero based )
    bool *pShouldHaveDebug // [optional out ] whether there should be debug information for ForCodeAtLine ( see notes heading SourceFile::GetMappedLineInfo(), above
)
{
    STRING * pFileName;
    bool ShouldHaveDebug;

    GetMappedLineInfo( ForCodeAtLine, &ShouldHaveDebug, &pFileName );
    if ( pShouldHaveDebug )
    {
        *pShouldHaveDebug = ShouldHaveDebug;
    }
    return pFileName;
}

bool SourceFile::IsMappedFile()
{
    return (m_NumMappedEntries > 0);
}
#endif !IDE

#if IDE 
void SourceFile::ClearCLSComplianceState()
{
    SourceFile *pSourceFile = this;

    if (pSourceFile->GetUnnamedNamespace())
    {
        ClearCLSComplianceState(pSourceFile->GetUnnamedNamespace());
    }

    pSourceFile->SetIsCLSComplianceCheckingDone(false);
}

void SourceFile::ClearCLSComplianceState
(
    BCSYM_Container *pContainer
)
{
    pContainer->SetIsCLSCompliant(false);
    pContainer->SetHasCLSComplianceBeenDetermined(false);

    for(BCSYM_Container *pNestedContainer = pContainer->GetNestedTypes();
        pNestedContainer;
        pNestedContainer = pNestedContainer->GetNextNestedType())
    {
        ClearCLSComplianceState(pNestedContainer);
    }
}


//****************************************************************************
// Decompilation methods
//
// The following methods should only be called from the master project
// decompilation routines and themselves.  They should never be called directly.
//****************************************************************************

//============================================================================
// Throw away our compiled code and associated dependencies.
//============================================================================

void SourceFile::_DemoteToTypesEmitted
(
    bool IsRemoveDependencies
)
{
    DebCheckNoBackgroundThreads(m_pCompiler);

    // NOTE: All of the logic in this method must appear inside of this
    //  : if statement.  The only side effect of calling a demotion
    //  : method must be that the file in question gets demoted to a new
    //  : state.
    //
    if (m_cs > CS_TypesEmitted)
    {
        _DemoteFromPossiblyIncompleteCompiledState();

        // Update the project tables.
        m_pProject->_ProcessDemotion(this, CS_TypesEmitted);
    }

    // Clear any errors that were added on the way up
    m_CurrentErrorTable.DeleteAll();

#if !defined( VBDBGEE ) 
    // Record the decompilation.
    if( g_pCompilerPackage->IsDecompilationTestHookSet() )
    {
        g_pCompilerPackage->GetDecompilationTestHook()->AddFileDecompilationState( GetProject(), this, m_cs );
    }
#endif
}

void SourceFile::_DemoteFromPossiblyIncompleteCompiledState
(
)
{
    if (this->NamedRootsMarkedBadAfterTypesEmitted())
    {
        // For files going below bound, these flags are reset anyway during the
        // decompilation from bound to declared. So no need to reset them in
        // this case.
        //
        if (m_DecompilationState >= CS_Bound)
        {
            // Reset the has error flags for all symbols in this file.
            for(BCSYM_NamedRoot *pnamed = m_SymbolList.GetFirst(); pnamed; pnamed = pnamed->GetNextInSymbolList())
            {
                // Just clear the flags we set while going to Compile
                Symbols::ResetFlags(pnamed, CS_TypesEmitted);
            }
        }

        this->SetNamedRootsMarkedBadAfterTypesEmitted(false);
    }

    // Mark this file as being in TypesEmitted state.
    m_cs = CS_TypesEmitted;

    GetXMLDocFile()->DemoteXMLDocFile(CS_TypesEmitted);

    m_mdTransientTokenHashTable.clear();

    // Undo any allocations done after TypesEmitted.
    m_nraSymbols.Free(&m_nramarkTypesEmitted);

    // Clear any errors that were added on the way up
    m_CurrentErrorTable.DeleteAll();

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S demoted to CS_TypesEmitted.\n", m_pstrFileName);

    VSASSERT(m_cs == CS_TypesEmitted, "State changed unexpectedly.");

    m_step = CS_EmitTypeMembers;
}

void SourceFile::_DemoteFromPossiblyIncompleteTypesEmittedState
(
)
{
    // Mark this file as being in Bound state.
    m_cs = CS_Bound;

    GetXMLDocFile()->DemoteXMLDocFile(CS_Bound);

    // Throw away any compiled state.
    m_pProject->m_Builder.RemoveFileMetadata(this);

    m_step = CS_BoundSymbols;
}

void SourceFile::_DemoteToBound
(
)
{
    DebCheckNoBackgroundThreads(m_pCompiler);

    // NOTE: All of the logic in this method must appear inside of this
    //  : if statement.  The only side effect of calling a demotion
    //  : method must be that the file in question gets demoted to a new
    //  : state.
    //
    if (m_cs > CS_Bound)
    {
        // Make sure we're already no greater than TypesEmitted state.
        // This demotion will invalidate the declaration cache
        // if it actually performs a demotion.
        _DemoteToTypesEmitted(false);

        _DemoteFromPossiblyIncompleteTypesEmittedState();

        // Flush ParseTreeService objects that may reference symbols
        if (m_pParseTreeService)
        {
            m_pParseTreeService->ClearProcLocalSymbolLookups();
        }

        // Undo any allocations done after Bound.
        m_nraSymbols.Free(&m_nramarkBound);

        VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S demoted to CS_Bound.\n", m_pstrFileName);

        // Update the project tables.
        m_pProject->_ProcessDemotion(this, CS_Bound);    
    }

    // Clear any errors that were added on the way up
    m_CurrentErrorTable.DeleteAll();

#if !defined( VBDBGEE ) 
    // Record the decompilation.
    if( g_pCompilerPackage->IsDecompilationTestHookSet() )
    {
        g_pCompilerPackage->GetDecompilationTestHook()->AddFileDecompilationState( GetProject(), this, m_cs );
    }
#endif
}

//============================================================================
// Unbind our named types and mark all expressions as being unevaluated.
//============================================================================

void SourceFile::_DemoteToDeclared
(
    bool IsRemoveDependencies
)
{
    DebCheckNoBackgroundThreads(m_pCompiler);

    // NOTE: IsRemoveDependencies is true if the fle is coming down only till declared,
    // but is false if we it is going down all the way to NoState. We use this to our
    // advantage in that when going all the way to NoState, we don't bother cleaning
    // the symbols, but just clear any state stored in the sourcefile.
    //

    // Has the file been bound
    // if not, has it been partially bound when binding a different sourcefile
    // if so unbind the sourcefile.
    //
    if (m_cs <= CS_Declared)
    {
        if (NeedToDecompileFileFromPartiallyBoundState())
        {
            VSASSERT( HasBindingStarted(),
                            "Why are we trying to decompile to CS_Declared if we have not gone beyond declared ?");

            VSASSERT( m_cs == CS_Declared,
                        "How can partially bound files have any other state besides declared ?");

            DemoteToDeclaredHelper(!IsRemoveDependencies);
        }

        // Clear the error table of any errors generated when going to bound
        //
        GetCurrentErrorTable()->DeleteAll();

        return;
    }

    DemoteToDeclaredHelper(!IsRemoveDependencies);

#if !defined( VBDBGEE ) 
    // Record the decompilation.
    if( g_pCompilerPackage->IsDecompilationTestHookSet() )
    {
        g_pCompilerPackage->GetDecompilationTestHook()->AddFileDecompilationState( GetProject(), this, m_cs );
    }
#endif

    if(m_pDaUnprocessedFriends)
    {
        delete m_pDaUnprocessedFriends;
        m_pDaUnprocessedFriends = NULL;
    }

    // Update the project tables.
    m_pProject->_ProcessDemotion(this, CS_Declared);

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S demoted to CS_Declared.\n", m_pstrFileName);

    m_step = CS_BuiltSymbols;
    SetContainsExtensionMethods(false);
    SetExtensionAttributeApplication(NULL);
    SetImportedExtensionMethodMetaDataLoaded(false);
}

void SourceFile::DemoteToDeclaredHelper
(
    bool IsGoingDownBelowDeclared
)
{
    if (!IsGoingDownBelowDeclared)
    {
        // Demote the XMLDoc info if needed.
        GetXMLDocFile()->DemoteXMLDocFile(CS_Declared);
    }

    // Flush ParseTreeService objects that may reference symbols
    if (m_pParseTreeService)
    {
        m_pParseTreeService->ClearProcLocalSymbolLookups();
    }

    // Make sure we're already no greater than bindable state.
    // This demotion will invalidate the declaration cache
    // if it actually performs a demotion.
    _DemoteToBound();

    Bindable::UnBindSourceFile(
        this,
        // Unbind the containers in the sourcefile only if we are not going down all the way below declared
        // because otherwise the symbols will anyway get thrown away eventually.
        //
        !IsGoingDownBelowDeclared);

    // Mark this file as being in Declared state.
    m_cs = CS_Declared;
    m_iCurrentBoundChangeIndex++;

    // clear the bindings cache
    m_DeclaredToBoundGenericBindingCache.ClearCache();

    // Undo any allocations that Bound did.
    m_nraSymbols.Free(&m_nramarkDeclared);

    VSASSERT(m_cs == CS_Declared, "State changed unexpectedly.");
}


//============================================================================
// Throw away all symbols and symbol-related accessories.
//============================================================================

void SourceFile::_DemoteToNoState()
{
    DebCheckNoBackgroundThreads(m_pCompiler);

    // NOTE: All of the logic in this method must appear inside of this
    //  : if statement.  The only side effect of calling a demotion
    //  : method must be that the file in question gets demoted to a new
    //  : state.
    //
    if (m_cs > CS_NoState)
    {
        // Make sure we're already in declared state.
        _DemoteToDeclared(false);

        // Remove the namespaces this file declared from the circular list. Do this
        // after demoting to Declared because _DemoteToDeclared needs the UnnamedNamespace.
        RemoveNamespaces();

        // Mark this file as being in NoState.
        m_cs = CS_NoState;

        // Increment change tracker.
        m_iCurrentDeclChangeIndex++;

        // Signal that file is no longer in declared state.
        m_eventDeclaredState.Unsignal();

        if (GetCompilerPackage())
        {
            GetCompilerPackage()->OnFileDemotedToNoState(this);
        }

        //// Decompile here.

        DiscardStaticLocalsLists();
        GetXMLDocFile()->DemoteXMLDocFile(CS_NoState);

        // Disassociate the file from its current root namespace
        SetRootNamespace(NULL);

        // Throw the symbols away along with their line markers.
        m_LineMarkerTable.RemoveAllDeclLocations();

        // Reset the file load error flag if we had a file load error previously.
        SetHasFileLoadError(false);

        m_nraSymbols.FreeHeap();

        m_SymbolList.Clear();

        m_SymbolListHashTable.clear();

        VSASSERT(m_cs == CS_NoState, "State changed unexpectedly.");

        // Update the project tables.
        m_pProject->_ProcessDemotion(this, CS_NoState);

        VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S demoted to CS_NoState.\n", m_pstrFileName);
    }
    m_CurrentErrorTable.DeleteAll();
    m_ErrorTable.DeleteAll();
    
#if !defined( VBDBGEE ) 
    // Record the decompilation.
    if( g_pCompilerPackage->IsDecompilationTestHookSet() )
    {
        g_pCompilerPackage->GetDecompilationTestHook()->AddFileDecompilationState( GetProject(), this, m_cs );
    }
#endif

    m_step = CS_NoStep;
}


/**************************************************************************************************
;DiscardStaticLocalsLists

Throw away the lists of static locals on each class
***************************************************************************************************/
void SourceFile::DiscardStaticLocalsLists()
{
    SymbolList *FileSymbols = this->GetNamespaceLevelSymbolList();
    for ( BCSYM_NamedRoot *FileSymbol = FileSymbols->GetFirst(); FileSymbol; FileSymbol = FileSymbol->GetNextInSymbolList())
    {
        BCSYM *RealDeal = FileSymbol->IsAlias() ? FileSymbol->DigThroughAlias() : FileSymbol;
        if ( RealDeal->IsClass())
        {
            RealDeal->PClass()->ClearBackingFieldsList();
        }
    }
}

//============================================================================
// Block until the compiler brings this file to  declared state.
//============================================================================

HRESULT SourceFile::WaitUntilState(CompilationState csState, WaitStatePumpingEnum ShouldPump, WaitStateCancelEnum CanCancel)
{
    CompilerSharedState *pCompilerSharedState = GetCompilerSharedState();
    VB_ENTRY();

    if (!pCompilerSharedState || m_pCompiler == NULL)
    {
        return E_FAIL;
    }

    if (pCompilerSharedState->IsInWaitUntilState())
    {
        // Re-entrancy. To avoid re-entrant bugs we return E_PENDING to the caller.
        return E_PENDING;
    }

    WaitStateGuard waitStateGuard;

    if ( GetCompState() < csState )
    {
        if (GetCompilerPackage() && !GetCompilerPackage()->IsSolutionBuild())
        {
            m_pCompiler->StartBackgroundThreadIfNeeded();

            if (pCompilerSharedState->IsBackgroundCompilationBlocked() || pCompilerSharedState->IsBackgroundCompilationStopping())
            {
                return E_FAIL;
            }
        }
        else
        {
            // We're in the middle of a Soln build and someone wants bound information for a project
            // The project being built may not be this one so we need to ensure it gets built.
            if (pCompilerSharedState->IsCompilingSpecificProject())
            {
                if (pCompilerSharedState->IsBackgroundCompilationBlocked()  && !GetCompilerPackage()->GetCompileToDiskPending())
                {
                    // The background thread should never be stopped during an explicit Build
                    // We must throw here otherwise we will have a hang.
                    VbThrow(E_FAIL);
                }

                // Make sure the current CompileOneProject finishes
                if (FAILED(hr = m_pCompiler->WaitForBackgroundThread(WaitStatePumping::PumpingAllowed, CanCancel)))
                {
                    return hr;
                }

                //If the project that was just built was actually us we'll now be above Bound
                if (GetCompState() >= csState )
                {
                    return hr;
                }
            }

            // Set a flag to avoid the optimization that skips CompileAllProjects during
            // a soln build.
            pCompilerSharedState->SetBackgroundCompileRequested(true);
        }

        // This call will ensure the background compiler will get our project to
        // the desired state as fast as possible by skipping unrelated projects
        if (m_pProject)
        {
            DynamicArray<CompilerProject *> pProjects;
            pProjects.AddElement(m_pProject);
            m_pProject->GetCompilerHost()->OptimizeProjectCompilationListHelper(&pProjects, csState);
        }

        ChangeCursor<IDC_WAITCURSOR> waitCursor;

        // Wait for our state to be signaled
        switch (csState)
        {
            case CS_Declared:
                hr = m_eventDeclaredState.WaitForSignal(ShouldPump, CanCancel);
                break;

            default:
                VSFAIL("Invalid CompilationState passed to WaitUntilState");
                break;
        }

        // We're now finshed with the Preferred Project so we clear it to prevent
        // subsequent CompileAllProjects doing the optimization.
        if (m_pProject && m_pProject->GetCompilerHost())
        {
            m_pProject->GetCompilerHost()->SetProjectToWaitOn(NULL);
            m_pProject->GetCompilerHost()->SetProjectCompilationStateToWaitOn(CS_NoState);
        }
    }

    VB_EXIT_NORETURN();

    RRETURN( hr );
}


HRESULT
SourceFile::WaitUntilDeclaredState
(
)
{
    return WaitUntilState(CS_Declared, WaitStatePumping::PumpingAllowed, WaitStateCancel::CancelAllowed);
}

bool SourceFile::GetAlternateNavigationTarget
(
    _In_z_ const WCHAR *wszRQName,
    BSTR *pbstrAlternateFilename,
    Location *plocAlternate
)
{
    AssertIfNull(wszRQName);
    AssertIfNull(pbstrAlternateFilename);
    AssertIfNull(plocAlternate);

    if (!IsSolutionExtension() &&
        GetHierarchy() &&
        GetItemId() != VSITEMID_NIL &&
        GetItemId() != 0)
    {
        CComQIPtr<IVsSymbolicNavigationNotify> spSymbolicNavigationNotify(GetHierarchy());

        if (spSymbolicNavigationNotify)
        {
            // Clients expect to see VSITEMID_NIL for files that are not part of the project
            VSITEMID itemIdSourceFile = GetItemId();
            if (IDEHelpers::IsNonMemberFile(this))
            {
                itemIdSourceFile = VSITEMID_NIL;
            }

            TextSpan tsNavigate;
            TS_SET(tsNavigate, 0, 0, 0, 0);

            CComPtr<IVsHierarchy> spHierarchy;
            VSITEMID itemId = VSITEMID_NIL;
            BOOL fWouldNavigate = FALSE;

            if (SUCCEEDED(spSymbolicNavigationNotify->QueryNavigateToSymbol(
                    GetHierarchy(),
                    itemIdSourceFile,
                    wszRQName,
                    &spHierarchy,
                    &itemId,
                    &tsNavigate,
                    &fWouldNavigate)))
            {
                if (fWouldNavigate)
                {
                    // tsNavigate includes the right edge of the span while the Locations shouldn't
                    IDEHelpers::MapTextSpanToLocation2(tsNavigate, plocAlternate);

                    return SUCCEEDED(IDEHelpers::GetFileName(spHierarchy, itemId, pbstrAlternateFilename));
                }
            }
        }
    }

    return false;
}

bool SourceFile::FireOnBeforeNavigateToSymbol
(
    BCSYM_NamedRoot *pNamed
)
{
    AssertIfNull(pNamed);

    if (CRQName::HasRQName(pNamed))
    {
        return FireOnBeforeNavigateToSymbol(CRQName::BuildRQName(GetCompiler(), pNamed));
    }

    return false;
}

bool SourceFile::FireOnBeforeNavigateToSymbol
(
    _In_z_ const WCHAR *wszRQName
)
{
    AssertIfNull(wszRQName);

    if (!IsSolutionExtension() &&
        GetHierarchy() &&
        GetItemId() != VSITEMID_NIL &&
        GetItemId() != 0)
    {
        CComQIPtr<IVsSymbolicNavigationNotify> spSymbolicNavigationNotify(GetHierarchy());

        if (spSymbolicNavigationNotify)
        {
            // Clients expect to see VSITEMID_NIL for files that are not part of the project
            VSITEMID itemId = GetItemId();
            if (IDEHelpers::IsNonMemberFile(this))
            {
                itemId = VSITEMID_NIL;
            }

            BOOL fNavigationHandled = FALSE;

            if (SUCCEEDED(spSymbolicNavigationNotify->OnBeforeNavigateToSymbol(GetHierarchy(), itemId, wszRQName, &fNavigationHandled)))
            {
                return fNavigationHandled;
            }
        }
    }

    return false;
}

bool SourceFile::QueryNavigateTo
(
    long iStartLine,
    long iStartIndex,
    long iEndLine,
    long iEndIndex,
    bool fRemapInAspx
)
{
    bool fCanNavigate = false;

    if (!IsSolutionExtension())
    {
        // If the file is closed, we can navigate.  Defer to SourceFileView if the file is open.
        SourceFileView *pSourceFileView = GetSourceFileView();
        if (pSourceFileView)
        {
            fCanNavigate = pSourceFileView->QueryNavigateTo(iStartLine,iStartIndex,iEndLine,iEndIndex,fRemapInAspx);
        }
        else
        {
            fCanNavigate = (m_itemid != VSITEMID_NIL) && (m_itemid != 0);
        }
    }

    return fCanNavigate;
}

HRESULT SourceFile::NavigateTo
(
    long iStartLine,
    long iStartIndex,
    long iEndLine,
    long iEndIndex,
    bool fRemapInAspx
)
{
    HRESULT hr = NOERROR;
    CComPtr<IUnknown> srpDocData = NULL;
    CComPtr<IVsProject> srpVsProject = NULL;
    CComPtr<IVsWindowFrame> srpVsWindowFrame = NULL;
    SourceFileView* pSourceFileView = GetSourceFileView();

    if (GetHierarchy())
    {
        bool isOpenItem = false;

        if (!pSourceFileView)
        {
            isOpenItem = true;
        }
        else if (!pSourceFileView->HasCodeWindow())
        {
            if (pSourceFileView->GetBufferCoordinator() == NULL)
            {
                isOpenItem = true;
                if (pSourceFileView->GetTextLines())
                {
                    pSourceFileView->GetTextLines()->QueryInterface(IID_IUnknown,(void **)&srpDocData);
                }
            }
        }
        if (isOpenItem && GetItemId() != VSITEMID_NIL)
        {
            IfFailGo(GetHierarchy()->QueryInterface(IID_IVsProject, (void **)&srpVsProject));
            if (srpDocData)
            {
                IfFailGo(srpVsProject->OpenItem(GetItemId(), LOGVIEWID_Code,srpDocData,&srpVsWindowFrame));
            }
            else
            {
                IfFailGo(srpVsProject->OpenItem(GetItemId(), LOGVIEWID_Code,DOCDATAEXISTING_UNKNOWN,&srpVsWindowFrame));
            }
            if (srpVsWindowFrame)
            {
                IfFailGo(srpVsWindowFrame->Show());
            }
         }
    }

    pSourceFileView = GetSourceFileView();

    //Is this location in hidden code?
    if (QueryNavigateTo(iStartLine, iStartIndex, iEndLine, iEndIndex + 1, fRemapInAspx))
    {
        if (pSourceFileView && (pSourceFileView->HasCodeWindow() || pSourceFileView->GetBufferCoordinator()))
        {
            hr = pSourceFileView->NavigateTo(iStartLine,iStartIndex,iEndLine,iEndIndex,fRemapInAspx);
        }
    }
    else
    {
        IDEHelpers::ShowMessageBox(STRID_FSR_HiddenDefinition);
        hr = S_OK;
    }

Error:
    return hr;
}

/***************************************************************************************************************************

    Search all files in all open projects for the SourceFile for the specified moniker ( the full path ).

***************************************************************************************************************************/
HRESULT
SourceFile::FindSourceFileByMoniker
(
    /*[in] */ LPCOLESTR     pszMkDocument,  //  Moniker file name to search for.
    /*[out]*/ SourceFile**  ppSourceFile    //  Source file found.
)
{
    HRESULT hr = NOERROR;
    bool bFound = false;
    CompilerFile *pCompilerFile = NULL;

    AllFilesIterator fileIter(GetCompilerPackage());

    // Loop through each file in Solution
    while ((pCompilerFile = fileIter.Next()) && !bFound)
    {
        if (pCompilerFile->IsSourceFile())
        {
            SourceFile* psf = pCompilerFile->PSourceFile();
            STRING* pstrFileName = psf->GetFileName();

            //  Is it the SourceFile we're looking for then rememeber it and break out of the loop.
            //
            if ((pstrFileName != NULL) && CompareFilenames(pstrFileName, pszMkDocument) == 0)
            {
                *ppSourceFile = psf;
                bFound = true;
            }
        }
    }

    if (!bFound)
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT
SourceFile::FindSourceFileByMoniker
(
    /*[in] */ IVsHierarchy *pHier,          //  project to search for.
    /*[in] */ LPCOLESTR     pszMkDocument,  //  Moniker file name to search for.
    /*[out]*/ SourceFile**  ppSourceFile    //  Source file found.
)
{
    HRESULT hr = NOERROR;

    bool bFound = false;
    CompilerProject *pCompilerProject = NULL;
    CComPtr<IVsHierarchy> spHierarchy(pHier);

    AllProjectsIterator projectIter(GetCompilerPackage());

    while (!bFound && (pCompilerProject = projectIter.Next()))
    {
        if (spHierarchy.IsEqualObject(pCompilerProject->GetHierarchy()))
        {
            // 



            FileIterator  fileIter;
            fileIter.Init( pCompilerProject );
            // Loop through each file in project
            CompilerFile* pCompilerFile;
            while ( (pCompilerFile = fileIter.Next()) && !bFound )
            {
                if ( pCompilerFile->IsSourceFile() )
                {
                    SourceFile* psf = pCompilerFile->PSourceFile();
                    STRING* pstrFileName = psf->GetFileName();

                    //  Is it the SourceFile we're looking for then rememeber it and break out of the loop.
                    if ( (pstrFileName != NULL) &&
                         CompareFilenames( pstrFileName, pszMkDocument ) == 0)
                    {
                        *ppSourceFile = psf;
                        bFound = true;
                    }
                }
            }
        }
    }

    if ( !bFound )
    {
        hr = E_FAIL;
    }
    return hr;
}

bool SourceFile::HasSourceFileView()
{
    if ( IsSourceFile() && (GetSourceFileView() != NULL) )
    {
        return true;
    }

    return false;
}

long SourceFile::GetNavigationColumn
(
    long iLine
)
{
    long lColumn = 0;

    if (m_pParseTreeService)
    {
        LANGPREFERENCES2 *pPreferences = GetCompilerPackage()->GetLangPreferences();

        if (pPreferences)
        {
            long lIndentDepth = m_pParseTreeService->GetLineIndentDepth(iLine);
            long lTabSize = 1;

            // Get the indentation size.
            if (pPreferences->IndentStyle != vsIndentStyleNone)
            {
                lTabSize = pPreferences->uIndentSize;
            }

            lColumn = lIndentDepth * lTabSize;
        }
    }

    return lColumn;
}

#endif //IDE

//************************************************************************
// ;AddSymbolFromHashTable
//
// Add a symbol to the hashed list of symbols for this file.
// NOTE: We are using the FixedSizeHashTable which adds symbols
// to the tail of the list when collisions occur. This is very
// important because when we move symbols from one bucket to the other
// we want the last symbol inserted to be the last removed.
// See VS213488
//************************************************************************
void SourceFile::AddSymbolToHashTable
(
    Location *pLoc,
    BCSYM_NamedRoot *SymbolToAdd
)
{
    VSASSERT(SymbolToAdd && pLoc->IsValid() && SymbolToAdd->GetTrackedLocation(), "Bad symbol or location");

#if IDE
    VSASSERT( GetCompilerSharedState()->IsInMainThread() ? GetCompilerSharedState()->IsBackgroundCompilationBlocked() : true,"if we're in main thrd, bgd should be blocked");
#endif IDE

    //note: there can be duplicates, so we need multimap: symbols with same loc and 2 diff syms. This sample

    //        Shared Widening Operator CType(ByVal arg As Integer) As TypeMembersTests
    //            Return Nothing
    //        End Operator
    // results in these 2 syms at the same loc:
    //  MethodImpl op_Implicit
    //  UserDefinedOperator op_Implicit

    MultimapUtil::InsertPair( m_SymbolListHashTable, *pLoc, SymbolToAdd);
    SymbolToAdd->GetTrackedLocation()->fIsInSourceFileHashTable = true;
}

#if IDE
//************************************************************************
// ;RemoveSymbolFromHashTable
//
// Remove a symbol from the hashed list of symbols for this file, and
// return that symbol.
//************************************************************************
BCSYM_NamedRoot *SourceFile::RemoveSymbolFromHashTable
(
    TrackedLocation *pLoc
)
{
    VSASSERT(pLoc->fIsInSourceFileHashTable, "location not in hashtable");

    auto it = MultimapUtil::FindPair(m_SymbolListHashTable, *pLoc, pLoc->m_pSymbolForLocation);
    ThrowIfTrue(it == m_SymbolListHashTable.end());

    BCSYM_NamedRoot* SymbolRemoved = it->second;
    VSASSERT(SymbolRemoved->GetTrackedLocation(), "Bad symbol or location");

    m_SymbolListHashTable.erase(it);
    SymbolRemoved->GetTrackedLocation()->fIsInSourceFileHashTable = false;

    return SymbolRemoved;
}

void SourceFile::InitENCProcessor( _In_ ENCBuilder* pBuilder )
{
    ThrowIfNull(pBuilder);

    m_pENCProcessor = new ENCProcessor(this, pBuilder);
    SetENCChange(false, false);
    m_fENCEntered = false;
    m_encBreakReason = ENC_BREAK_NORMAL;
}

void SourceFile::InitENCProcessor(ENCProcessor *pENCProcessor)
{
    if (pENCProcessor)
    {
        pENCProcessor->UpdateSourceFile(this);
        m_pENCProcessor = pENCProcessor;
    }
}

void SourceFile::StopDebugging()
{
    if (m_pENCProcessor != NULL)
    {
        m_pENCProcessor->StopDebugging();
        m_mdENCTokenHashTable.clear();
        m_ENCTemporaryManagers.clear();
        m_ENCUsedTemporaryName.clear();
        SetENCChange(false, false);
        m_fENCEntered = false;
        delete m_pENCProcessor;
        m_pENCProcessor = NULL;
    }
}

ENCSTATUS SourceFile::GetENCStatus()
{
    if (HasSourceFileView())
    {
        GetSourceFileView()->PurgeChangeInput();
    }

    //Check if we have any compiler errors
    if (m_pProject)
    {
        if (CheckENCCompilerError())
        {
            return ENC_RawError;
        }
        else if (m_fENCRude || m_ErrorTable.HasErrorsAtStep(CS_ENCRudeDetection))
        {
            return ENC_RudeError;   // Compile errors are considered rude.
        }
        else
        {
            return m_fENCChange ? ENC_GenDelta : ENC_NoChange;
        }
    }

    return ENC_NoChange;    // We can't currently handle ENC without a project.
}

void SourceFile::BreakModeCommit(bool fCommitOtherFiles)
{

    if (HasSourceFileView() && GetSourceFileView()->IsAspxHiddenFile())
    {
        // VSWhidbey[173730] Snippet Editor should not process Break mode commit.
        // VSWhidbey[175659] Venus should not process Break mode commit.
        return;
    }

    if (GetProject() && GetProject()->GetENCBuilder())  // VSWhidbey 175661 Only do ENC check if project supports ENC
    {
        //Check if we have any ENC errors (rude edit error)
        //VERIFY:Microsoft,10/2001,do we modify an active statement. If so, generate an ENC error
        //compare parse trees to figure out which edits are allow to continue.
        VSASSERT(m_pENCProcessor,"Why do we not have a EnC Processor?");
        if (m_pENCProcessor)
        {
            bool fWasRude = m_fENCRude;

            SetENCChange(true, false);  // VSWhidbey[409039]
            if (!m_pENCProcessor->CompareStatements())
            {
                m_fENCChange = true;

                if (m_ErrorTable.HasErrorsAtStep(CS_ENCRudeDetection))
                {
                    m_fENCRude = true;
                }
            }

            if (fWasRude != m_fENCRude)
            {
                GetCompilerSharedState()->RefreshTaskList();
            }
        }

        // VSWhidbey 128954 Need to inform other source file views of this change.
        if (fCommitOtherFiles)
        {
            SourceFileIterator sourcefiles(GetProject());
            SourceFile *pSourceFile;
            while ((pSourceFile = sourcefiles.Next()) != NULL)
            {
                if (pSourceFile != this)
                {
                    pSourceFile->BreakModeCommit(false);
                }
            }
        }
    }
    else
    {
        //Consider:Microsoft,10/2001,how to handle ENC without a vb project
    }
}

void SourceFile::SetENCChange(bool fIsDirty, bool fIsRude)
{
    m_fENCChange = fIsDirty;
    m_fENCRude = fIsRude;
}

void SourceFile::SetENCChange(DBGMODE dbgmode)
{
    if ((dbgmode == DBGMODE_Design) && m_fENCEntered)
    {
        SetENCChange(true, true);
    }
    else if (dbgmode & DBGMODE_Break)
    {
        // VSWhidbey[379262] Need to load Conditional Compilation Symbols in case we enter this code path while in CS_NoState
        BackgroundCompilerLock compilerLock(GetProject());
        ParseTreeServiceLock parseTreeServiceLock(m_pParseTreeService);

        bool fInitProjectLevelCondCompScope = false;

        if (GetProject()->GetProjectLevelCondCompScope() == NULL)
        {
            fInitProjectLevelCondCompScope = SUCCEEDED(GetProject()->CreateProjectLevelCondCompSymbols(NULL));
        }

        if (HasSourceFileView() && GetSourceFileView()->GetVBTextMarkerManager())
        {
            GetSourceFileView()->GetVBTextMarkerManager()->UpdateAllMarkers();
        }

        BreakModeCommit(true);

        if (fInitProjectLevelCondCompScope)
        {
            GetProject()->ClearProjectLevelCondCompSymbols();
        }
    }
}

//-------------------------------------------------------------------------------------------------
//
// This method is called on both the enter and exit of an ENC session on the source file.  The isEnter
// parameter is used to differentiate between the two events.  In the case of an exti of an ENC session,
// the ENC_BREAKSTATE_REASON will be ENC_BREAK_NORMAL.
//
// NOTE:Microsoft,10/2001,it should be called at the begin/end of ENC session
//-------------------------------------------------------------------------------------------------
void SourceFile::ResetENCState(ENC_BREAKSTATE_REASON encBreakReason, bool isEnter)
{
    if (m_pENCProcessor)
    {
        // For performance reasons we don't cache the ENC tree when entering ENC session.
        // However for this to work, we must be notified if the file changes in any way outside of the Editor,
        // because we would lose opportunity to cache the old parse tree.
        //
        // Microsoft: I can't make heads or tails of the above comment.  It mentions specifically
        // about entering an ENC session but the original state of the below code didn't 
        // differentiate between entering and leaving an ENC session.  The differientation was added
        // by me along with this comment
        if ( isEnter )
        {
            m_pENCProcessor->OnEnterSession();
        }
        else
        {
            m_pENCProcessor->OnLeaveSession();
        }
    }

    m_fENCEntered = isEnter;
    m_encBreakReason = encBreakReason;

    if (HasSourceFileView() && GetSourceFileView()->GetVBTextMarkerManager())
    {
        if (!isEnter)
        {
            // Defer the actual delete until the next time we enter an ENC session
            // or the ENCBuilder is destroyed
            // VSWhidbey [71428] we want to defer the deletion to prevent flickering.
            GetSourceFileView()->GetVBTextMarkerManager()->MarkDeleteReadOnlyMarkers();
        }
    }

    SetENCChange(false, false);

    // When leaving an ENC session we should reset the temporary manager state
    if ( !isEnter )
    {
        ResetTemporaryManagers();
    }
}

bool SourceFile::CheckENCCompilerError()
{
    bool fHasErr = false;
    if (GetProject())
    {
        fHasErr = m_ErrorTable.HasErrorsThroughStep(CS_BoundSymbols);
        if (fHasErr &&
            !m_ErrorTable.HasErrorsThroughStep(CS_BoundSymbols) &&
            m_ErrorTable.HasThisError(ERRID_StartupCodeNotFound1))
        {
            fHasErr = false;
        }
    }
    return fHasErr;
}

void SourceFile::CompileMethodBodiesWithTracking
(
    BCSYM_Container *Container
)
{
    while(Container)
    {
        if(Container->IsClass())
        {
            // look for member procedures if it's a class
            BCITER_CHILD  iter;
            iter.Init( Container, false, true, true );

            for(BCSYM_NamedRoot *Child = iter.GetNext();
                Child;
                Child = iter.GetNext())
            {
                if(Child->IsProc() && Child->PProc()->IsMethodImpl())
                {
                    NorlsAllocator nra(NORLSLOC);
                    ILTree::ILNode *ptreeWithoutExtraImports = NULL;
                    ILTree::ILNode * ptreeWithExtraImports = NULL;

                    ErrorTable errors1(m_pCompiler, m_pProject, NULL);
                    Cycles     cycle1(m_pCompiler, &nra, &errors1, ERRID_SubNewCycle1, ERRID_SubNewCycle2);

                    BackupValue<bool> backupUseExtraImportFlag(&m_shouldSemanticsUseExtraImportForExtensionMethods);
                    BackupValue<DynamicArray<ImportTrackerEntry> *> backupImportTracker(&m_pImportTracker);

                    m_shouldSemanticsUseExtraImportForExtensionMethods = false;
                    // this should hook us up with everything we need.
                    IfFailThrow(GetBoundMethodBodyTrees(
                        Child->PProc(),
                        &nra,
                        &errors1,
                        &cycle1,
                        NULL,
                        NULL,
                        NULL,
                        NULL, // Compilation Caches
                        gmbIDEBodies,
                        &ptreeWithoutExtraImports));

                    ErrorTable errors2(m_pCompiler, m_pProject, NULL);
                    Cycles cycle2(m_pCompiler, &nra, &errors2, ERRID_SubNewCycle1, ERRID_SubNewCycle2);

                    m_shouldSemanticsUseExtraImportForExtensionMethods = true;
                    m_pImportTracker = NULL;
                    // clears the extension method caches, see Dev10 804837
                    GetCompilerPackage()->ClearCompilationCaches();  

                    IfFailThrow(GetBoundMethodBodyTrees(
                        Child->PProc(),
                        &nra,
                        &errors2,
                        &cycle2,
                        NULL,
                        NULL,
                        NULL,
                        NULL, // Compilation Caches
                        gmbIDEBodies,
                        &ptreeWithExtraImports));

                    // clear caches again to make sure we're in a clean state after leaving this method
                    GetCompilerPackage()->ClearCompilationCaches();
                    backupUseExtraImportFlag.Restore();
                    backupImportTracker.Restore();

                    ExtensionMethodBindingChangeDetector bindingChangeDetector;

                    // GetBoundMethodBodyTrees may return NULL if the file is gone, but this should not happen normally.
                    // If it does, we'll just ignore this procedure
                    VSASSERT(ptreeWithoutExtraImports && ptreeWithExtraImports, "why are we not able to find the file");

                    if
                    (
                        ptreeWithoutExtraImports && 
                        ptreeWithExtraImports &&
                        bindingChangeDetector.DidExtensionMethodBindingsChange
                        (
                            &(ptreeWithExtraImports->AsProcedureBlock()),
                            &(ptreeWithoutExtraImports->AsProcedureBlock()),
                            &nra
                        )
                     )
                    {
                        ImportTrackerEntry & entry = GetImportTracker()->Add();
                        entry.location.Invalidate();
                        entry.fIsCollision = false;
                        entry.pstrCorrection = NULL;
                        entry.fIsExtensionMethodBindingChange = true;
                        entry.newSymbolBound = NULL;

                        //We found an extension method binding failure, so we stop.
                        //This is because we do not offer the ability to correct changed bindings
                        //when extension method bindings change, as doing so would require refactoring
                        //extension method calls into shared method calls, which was too expensive
                        //to implement in Orcas. Because we can't do anything with the modified bindings, there
                        //is no purpose in calculating them, hence why we return immedietly.
                        //
                        //WARNING: The code in CheckAddImportsCollision is dependant on this behavior.
                        //In particular, it will determine wether or not extension method bindings changed
                        //by simply looking at the last element in the tracker list and checking if its fIsExtensionMethodBindingChange
                        //flag is true. If you change this code to no-longer short circut then you need to make appropriate changes to
                        //CheckAddImportsCollision as well.
                        return;
                    }
                }
            }
        }

        if(Container->GetNestedTypes())
        {
            Container = Container->GetNestedTypes();
        }
        else if(Container->GetNextNestedType())
        {
            Container = Container->GetNextNestedType();
        }
        else
        {
            while(Container && !Container->GetNextNestedType())
            {
                if(Container->GetParent() && Container->GetParent()->IsContainer())
                {
                    Container = Container->GetParent()->PContainer();
                }
                else
                {
                    Container = NULL;
                }
            }

            if(Container)
            {
                Container = Container->GetNextNestedType();
            }
        }
    }
}

HRESULT SourceFile::RecompileWithImportTracking
(
    ImportedTarget                   *pExtraTrackedImport,
    DynamicArray<ImportTrackerEntry> *pImportTracker,
    ULONG                            &BoundChangeIndex
)
{
    HRESULT hr = S_OK;

    // decompile if we've gone above declared at all
    if(m_pProject->GetCompState() > CS_Declared ||
       m_pProject->IsBoundCompilationInProgress())
    {
        EditFilter::DecompileFileNoCommit(this, CS_Declared);
        GetCompilerPackage()->CommitEditListDemotions(true);
    }

    __try
    {
        // set us up for extra tracking stuffs
        m_pExtraTrackedImport = pExtraTrackedImport;
        m_pImportTracker = pImportTracker;

        __try
        {
            m_pCompiler->StartBackgroundThread();
            hr = m_pProject->WaitUntilBound();
        }
        __finally
        {
            m_pCompiler->StopBackgroundThread();
        }

        if (SUCCEEDED(hr))
        {
            BoundChangeIndex = m_pProject->GetCurrentBoundChangeIndex();
            CompileMethodBodiesWithTracking( this->GetUnnamedNamespace() );
        }
    }
    __finally
    {
        m_pImportTracker = NULL;
        m_pExtraTrackedImport = NULL;
    }

    return hr;
}


int _cdecl SourceFile::CompareOutOfProcVbcBuildErrorInfo
(
    const void* pInfo1,
    const void* pInfo2
)
{
    return CompareOutOfProcBuildError(((OutOfProcVbcBuildErrorInfo*) pInfo1)->m_errWithOrigLoc.m_pError, ((OutOfProcVbcBuildErrorInfo*) pInfo2)->m_errWithOrigLoc.m_pError);
}

int _cdecl SourceFile::CompareWinMDExpBuildError
(
    const void* pError1,
    const void* pError2
)
{
    return CompareOutOfProcBuildError(((CompileErrorWithOriginalLocation*) pError1)->m_pError, ((CompileErrorWithOriginalLocation*) pError2)->m_pError);
}

int SourceFile::CompareCompileErrorToOutOfProcVbcBuildErrorInfo
(
    const CompileError* pError,
    const OutOfProcVbcBuildErrorInfo* pInfo
)
{
    return CompareOutOfProcBuildError(pError, pInfo->m_errWithOrigLoc.m_pError);
}

int SourceFile::CompareOutOfProcBuildError
(
    const CompileError* pError1,
    const CompileError* pError2
)
{
    // Sort by start location

    int iCompare = CompareValues(pError1->m_loc.m_lBegLine, pError2->m_loc.m_lBegLine);

    if (iCompare == 0)
    {
        iCompare = CompareValues(pError1->m_loc.m_lBegColumn, pError2->m_loc.m_lBegColumn);
    }

    return iCompare;

}

// This ensures that invalid out of proc errors are removed and that
// the remaining errors are sorted
void SourceFile::EnsureOutOfProcBuildErrorArrayUpToDate()
{
    if (m_fNeedToUpdateOutOfProcBuildErrorArray)
    {
        // First remove all items with invalid locations
        m_LineMarkerTable.RemoveInvalidOutOfProcBuildErrorLocations();

        for (ULONG i = m_daOutOfProcVbcBuildErrors.Count(); i > 0; i--)
        {
            CompileError* pError = m_daOutOfProcVbcBuildErrors.Element(i - 1).m_errWithOrigLoc.m_pError;

            if (pError->m_loc.IsInvalid())
            {
                m_daOutOfProcVbcBuildErrors.Remove(i - 1);
                delete pError;
            }
        }

        for (ULONG i = m_daWinMDExpBuildErrors.Count(); i > 0; i--)
        {
            CompileError* pError = m_daWinMDExpBuildErrors.Element(i - 1).m_pError;

            if (pError->m_loc.IsInvalid())
            {
                m_daWinMDExpBuildErrors.Remove(i - 1);
                delete pError;
            }
        }

        // Now sort the out of proc vbc errors by the current location
        qsort(m_daOutOfProcVbcBuildErrors.Array(), m_daOutOfProcVbcBuildErrors.Count(), sizeof(OutOfProcVbcBuildErrorInfo), CompareOutOfProcVbcBuildErrorInfo);

        // And sort the WinMDExp errors
        qsort(m_daWinMDExpBuildErrors.Array(), m_daWinMDExpBuildErrors.Count(), sizeof(CompileErrorWithOriginalLocation), CompareWinMDExpBuildError);

        m_fNeedToUpdateOutOfProcBuildErrorArray = false;
    }
}

HRESULT SourceFile::AddWinMDExpBuildError(BSTR bstrErrorMessage, long errorId, VSTASKPRIORITY priority, long startLine, long startColumn, long endLine, long endColumn)
{
    HRESULT hr = S_OK;
    CompileErrorWithOriginalLocation errWithOrigLoc;

    IfFailGo(AddOutOfProcBuildError(bstrErrorMessage, errorId, priority, startLine, startColumn, endLine, endColumn, errWithOrigLoc));

    m_daWinMDExpBuildErrors.AddElement(errWithOrigLoc);

Error:
    return hr;
}

HRESULT SourceFile::AddOutOfProcVbcBuildError(BSTR bstrErrorMessage, long errorId, VSTASKPRIORITY priority, long startLine, long startColumn, long endLine, long endColumn)
{
    HRESULT hr = S_OK;
    OutOfProcVbcBuildErrorInfo info;
    info.m_fShowInTaskList = true;

    IfFailGo(AddOutOfProcBuildError(bstrErrorMessage, errorId, priority, startLine, startColumn, endLine, endColumn, info.m_errWithOrigLoc));

    m_daOutOfProcVbcBuildErrors.AddElement(info);

Error:
    return hr;
}

HRESULT SourceFile::AddOutOfProcBuildError(BSTR bstrErrorMessage, long errorId, VSTASKPRIORITY priority, long startLine, long startColumn, long endLine, long endColumn, _Out_ CompileErrorWithOriginalLocation& errWithOrigLoc)
{
    HRESULT hr = S_OK;

    TextData data;
    CComBSTR bstrText;
    TextDataPool& pool = GetCompilerPackage()->GetTextDataPool();
    IfFalseGo( pool.TryGetOrCreateForCurrentText(this, __ref data), E_FAIL );

    IfFailGo(data.GetText().GetTextLineRange(startLine, startLine, &bstrText));

    // Squiggle the token starting at the line and column of the error
    // if ending line and column values are not given, other wise use
    // the given span.
    Location locError;
    if (endLine != -1 && endColumn != -1)
    {
        locError.SetLocation(startLine, startColumn, endLine, endColumn);
    }
    else
    {
        locError.SetLocation(startLine, startColumn);

        if (bstrText.Length() > 0)
        {
            Scanner scanner(GetCompiler(), bstrText, bstrText.Length(), 0, startLine, 0);
            Token* pToken = TokenUtil::FindTokenStartingAtLocation(scanner.GetNextLine(), locError);

            if (pToken != NULL)
            {
                locError = pToken->GetLocation();
            }
        }
    }

    errWithOrigLoc.m_pError = new (zeromemory) CompileError(
                                       GetCompiler(),
                                       GetProject(),
                                       RawStringBufferUtil::Copy(bstrErrorMessage),
                                       errorId,
                                       &locError,
                                       priority);
    errWithOrigLoc.m_originalLocation = locError;

    // Add the location to the line marker table, so that its location is updated as
    // edits are made
    m_LineMarkerTable.AddOutOfProcBuildErrorLocation(&errWithOrigLoc.m_pError->m_loc);

    // Defer the invalidation check and sorting work until needed later
    m_fNeedToUpdateOutOfProcBuildErrorArray = true;

    GetProject()->GetCompilerTaskProvider()->TaskListNeedsRefresh();

Error:
    return hr;
}

bool SourceFile::ClearOutOfProcBuildErrors()
{
    bool fAnyErrorsRemoved = false;

    for (ULONG i = 0; i < m_daOutOfProcVbcBuildErrors.Count(); i++)
    {
        delete m_daOutOfProcVbcBuildErrors.Element(i).m_errWithOrigLoc.m_pError;
        fAnyErrorsRemoved = true;
    }
    m_daOutOfProcVbcBuildErrors.Reset();

    for (ULONG i = 0; i < m_daWinMDExpBuildErrors.Count(); i++)
    {
        delete m_daWinMDExpBuildErrors.Element(i).m_pError;
        fAnyErrorsRemoved = true;
    }
    m_daWinMDExpBuildErrors.Reset();

    m_LineMarkerTable.RemoveOutOfProcBuildErrorLocations();

    return fAnyErrorsRemoved;
}

void SourceFile::SignalOutOfProcBuildErrorLocationsUpdated()
{
    // Actual update work will be deferred until needed
    m_fNeedToUpdateOutOfProcBuildErrorArray = true;
}

bool SourceFile::HasOutOfProcBuildErrors()
{
    EnsureOutOfProcBuildErrorArrayUpToDate();

    return m_daOutOfProcVbcBuildErrors.Count() > 0 || m_daWinMDExpBuildErrors.Count() > 0;
}

void SourceFile::GetVisibleErrorsConsideringOutOfProcBuildErrors
(
    DynamicArray<CompileError *>* pdaErrorsToReport,
    DynamicArray<const WCHAR *>* pdaErrorMessages,
    DynamicArray<CompileError *>* pdaWMEErrorsToReport
)
{
    EnsureOutOfProcBuildErrorArrayUpToDate();

    // Go through in-proc compiler's error table and compare to the
    // out of proc vbc compile errors and collapse duplicates
    ErrorIterator errorIterator(GetErrorTable());

    while (CompileError* pError = errorIterator.Next())
    {
        pdaErrorsToReport->AddElement(pError);

        // See if any out of proc build errors match pError
        long lIndex = 0;
        BSearchEx<CompileError, OutOfProcVbcBuildErrorInfo>(
                pError,
                m_daOutOfProcVbcBuildErrors.Array(),
                m_daOutOfProcVbcBuildErrors.Count(),
                CompareCompileErrorToOutOfProcVbcBuildErrorInfo,
                &lIndex);

        // Advance to the last error that starts on the same location as the in proc error
        ULONG startIndex = lIndex;
        while (startIndex < m_daOutOfProcVbcBuildErrors.Count() &&
               m_daOutOfProcVbcBuildErrors.Element(startIndex).m_errWithOrigLoc.m_pError->m_loc.m_lBegLine == pError->m_loc.m_lBegLine &&
               m_daOutOfProcVbcBuildErrors.Element(startIndex).m_errWithOrigLoc.m_pError->m_loc.m_lBegColumn == pError->m_loc.m_lBegColumn)
        {
            startIndex++;
        }

        // If an out of proc error matches the in proc error, use the out of proc error message
        // for the in proc error and prevent the out of proc error from showing up in the task list
        const WCHAR* wszMessage = pError->m_wszMessage;

        for (ULONG i = startIndex; i > 0; i--)
        {
            OutOfProcVbcBuildErrorInfo& info = m_daOutOfProcVbcBuildErrors.Element(i - 1);

            // An out of proc error matches an in proc error if the start location and
            // error id match
            if (info.m_errWithOrigLoc.m_pError->m_loc.m_lBegLine == pError->m_loc.m_lBegLine &&
                info.m_errWithOrigLoc.m_pError->m_loc.m_lBegColumn == pError->m_loc.m_lBegColumn)
            {
                if (info.m_errWithOrigLoc.m_pError->m_errid == pError->m_errid)
                {
                    wszMessage = info.m_errWithOrigLoc.m_pError->m_wszMessage;
                    info.m_fShowInTaskList = false;
                }
            }
            else
            {
                // Stop because the array is sorted and no more items will match the start location
                break;
            }
        }

        pdaErrorMessages->AddElement(wszMessage);
    }

    // Add the out of proc errors that haven't been replaced by an in proc error
    for (ULONG i = 0; i < m_daOutOfProcVbcBuildErrors.Count(); i++)
    {
        OutOfProcVbcBuildErrorInfo info = m_daOutOfProcVbcBuildErrors.Element(i);

        if (info.m_fShowInTaskList)
        {
            pdaErrorsToReport->AddElement(info.m_errWithOrigLoc.m_pError);
            pdaErrorMessages->AddElement(info.m_errWithOrigLoc.m_pError->m_wszMessage);
        }
    }

    // Simply add the WinMDExp errors to the list, no need to check for duplicates
    for (ULONG i = 0; i < m_daWinMDExpBuildErrors.Count(); i++)
    {
        CompileError* pError = m_daWinMDExpBuildErrors.Element(i).m_pError;
        pdaWMEErrorsToReport->AddElement(pError);
    }
}

void SourceFile::RevertOutOfProcBuildErrorLocations()
{
    for (ULONG i = 0; i < m_daOutOfProcVbcBuildErrors.Count(); i++)
    {
        CompileErrorWithOriginalLocation& errWithOrigLoc = m_daOutOfProcVbcBuildErrors.Element(i).m_errWithOrigLoc;
        errWithOrigLoc.m_pError->m_loc.SetLocation(&errWithOrigLoc.m_originalLocation);
        errWithOrigLoc.m_pError->m_loc.m_oBegin = 0;
        errWithOrigLoc.m_pError->m_loc.m_oEnd = 0;
    }

    for (ULONG i = 0; i < m_daWinMDExpBuildErrors.Count(); i++)
    {
        CompileErrorWithOriginalLocation& errWithOrigLoc = m_daWinMDExpBuildErrors.Element(i);
        errWithOrigLoc.m_pError->m_loc.SetLocation(&errWithOrigLoc.m_originalLocation);
        errWithOrigLoc.m_pError->m_loc.m_oBegin = 0;
        errWithOrigLoc.m_pError->m_loc.m_oEnd = 0;
    }

    GetProject()->GetCompilerTaskProvider()->TaskListNeedsRefresh();
}

void SourceFile::SaveOutOfProcBuildErrorLocations()
{
    for (ULONG i = 0; i < m_daOutOfProcVbcBuildErrors.Count(); i++)
    {
        CompileErrorWithOriginalLocation& errWithOrigLoc = m_daOutOfProcVbcBuildErrors.Element(i).m_errWithOrigLoc;
        errWithOrigLoc.m_originalLocation.SetLocation(&errWithOrigLoc.m_pError->m_loc);
    }

    for (ULONG i = 0; i < m_daWinMDExpBuildErrors.Count(); i++)
    {
        CompileErrorWithOriginalLocation& errWithOrigLoc = m_daWinMDExpBuildErrors.Element(i);
        errWithOrigLoc.m_originalLocation.SetLocation(&errWithOrigLoc.m_pError->m_loc);
    }
}

#endif IDE

