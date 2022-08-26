    //-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if IDE && ID_TEST     // Dump the XML to a file
unsigned CodeFile::m_lXMLDumpFileName = 0;
#endif

CodeFile::CodeFile(
    Compiler * pCompiler,
    SourceFile * pSourceFile)
    : m_pSourceFile(pSourceFile)
    , m_pCompiler(pCompiler)
    , m_nraEdits(NORLSLOC)
    , m_fEnableFiredChangedEvent(true)
    , m_InsertionKind(CodeModel_Unknown)
{
}

CodeFile::~CodeFile()
{
    ICodeFileEvents **rgArray   = m_daEventListeners.Array();
    ICodeFileEvents * pElement  = NULL;
    unsigned iEvent, cEvents    = m_daEventListeners.Count();

    // Normally we can't get here unless all the sinks release their
    // references right? Wrong! if ~Codefile() is called on the "this"
    // pointer outside of the delete function, we will be dumping sinks that
    // may have other references on us!
    // Calling release here is actually VERY DANGEROUS
    // to do. It depends on a particular IUnknown implementation!!!
    for (iEvent = 0; iEvent < cEvents; iEvent++)
    {
        pElement = rgArray[iEvent];

        pElement->OnDropped();
        pElement->Release();
    }

    VSASSERT(!m_spUndo, "CodeFile is being released in the middle of an Undo action!");
    m_fIsZombie = true;
}

//****************************************************************************
// IUnknown implementation - pseudo aggregation.
//****************************************************************************

STDMETHODIMP CodeFile::QueryInterface
(
    REFIID riid,
    void **ppv
)
{
#if IDE

    if (ppv == NULL)
    {
        VSFAIL("Invalid Null Pointer");
        return E_POINTER;
    }

    if (riid == IID_ICodeFile)
    {
        *ppv = static_cast<CodeFile *>(this);
    }
    else if (riid == IID_CCodeFile)
    {
        *ppv = this;
    }

    else
    {
        if (!m_pSourceFile)
        {
            return E_FAIL;
        }

        return m_pSourceFile->QueryInterface(riid, ppv);
    }

    AddRef();
    return NOERROR;

#else !IDE
    return E_NOTIMPL;
#endif !IDE
}

STDMETHODIMP_(ULONG) CodeFile::AddRef()
{
#if IDE
    return m_pSourceFile->AddRef();
#else !IDE
    return 0;
#endif !IDE
}

STDMETHODIMP_(ULONG) CodeFile::Release()
{
#if IDE
    return m_pSourceFile->Release();
#else !IDE
    return 0;
#endif !IDE
}

//*****************************************************************************
// ICodeFile
//*****************************************************************************

//
// Hook the events.
//

//=============================================================================
// Adds an event listener to the list.  Returns the cookie
// used to later unadvise the events.
//=============================================================================

STDMETHODIMP CodeFile::AdviseCodeFileEvents
(
    ICodeFileEvents *pCodeFileEvents,
    VSCOOKIE *pEventCookie
)
{
    VB_ENTRY();
    if (m_pSourceFile && m_pSourceFile->IsValidSourceFile())
    {
        m_daEventListeners.Add() = pCodeFileEvents;
        pCodeFileEvents->AddRef();

        *pEventCookie = (VSCOOKIE)pCodeFileEvents;

        // Don't fire the events right now because the shell will crash in the
        // DropDown handler because it isn't fully setup yet.
    }

    VB_EXIT();
}

//=============================================================================
// Unhook from the event source.
//=============================================================================

STDMETHODIMP CodeFile::UnadviseCodeFileEvents
(
    VSCOOKIE EventCookie
)
{
    VB_ENTRY();
    ICodeFileEvents **rgArray = m_daEventListeners.Array();
    unsigned iEvent, cEvents = m_daEventListeners.Count();

    if (m_pSourceFile && m_pSourceFile->IsValidSourceFile())
    {
        for (iEvent = 0; iEvent < cEvents; iEvent++)
        {
            if ((VSCOOKIE)rgArray[iEvent] == EventCookie)
            {
                rgArray[iEvent]->Release();

                m_daEventListeners.Remove(iEvent);
                break;
            }
        }
    }

    VB_EXIT();
}

#if ID_TEST && IDE // Dump the XML to a file
void CodeFile::DumpToFile(Compiler *pCompiler, _In_z_ WCHAR *pText, XMLDumpKind DumpKind)
{
    if (VBFTESTSWITCH(fIDEDumpXML))
    {
        WCHAR fileName[30] = {0};
        WCHAR fileNumber[5] = {0};

        _ultow_s(m_lXMLDumpFileName, fileNumber, _countof(fileNumber), 10);

        switch (DumpKind)
        {
        case Dump_Decls:
            wcscpy_s(fileName, _countof(fileName), L"DeclXMLDump_");
            break;

        case Dump_MethodBody:
            wcscpy_s(fileName, _countof(fileName), L"MethodBodyXMLDump_");
            break;

        case Dump_VBCode:
            wcscpy_s(fileName, _countof(fileName), L"VBFromXMLDump_");
            break;

        case Dump_ReadXML:
            wcscpy_s(fileName, _countof(fileName), L"XMLReadDump_");
            break;

        default:
            VSFAIL(L"Unknown DumpFile type!");
            wcscpy_s(fileName, _countof(fileName), L"XMLDump_");
        }

        wcscat_s(fileName, _countof(fileName), fileNumber);

        HRESULT hr = S_OK;
        {
            // Dump the XML we are given so that we can tell if a bug is on our side or not
            CompilerProject::DumpXMLToFile(pCompiler, fileName, DumpKind == Dump_VBCode ? L"xvb" : L"xml", pText);
        }

        ++m_lXMLDumpFileName;       // Use a unique file name
    }
}
#endif

//
// Get file information.
//

//=============================================================================
// Fills a text buffer with the XML that describes the declaration
// statements for this file.
//=============================================================================

STDMETHODIMP CodeFile::GetDeclarationStatements
(
    BSTR *pbstrText
)
{
    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        return E_FAIL;
    }

    if (pbstrText == NULL)
    {
        VSFAIL("Invalid NULL Pointer");
        return E_POINTER;
    }

    VB_ENTRY();
    StringBuffer sbText;
    sbText.SetAllocationSize(StringBuffer::AllocSize::Grande);

    // Get the XML.
    IfFailGo(XMLGen::XMLRunDeclarations(m_pCompiler, m_pSourceFile, &sbText));

    // Convert it to a BSTR and return it.
    *pbstrText = SysAllocStringLen(sbText.GetString(), (UINT)sbText.GetStringLength());

#if IDE && ID_TEST     // Dump the XML to a file
    DumpToFile(m_pCompiler, sbText.GetString(), Dump_Decls);
#endif

    VB_EXIT_LABEL();
}

//
// File modification routines.
//

//=============================================================================
// Starts an edit.  None of these edits will actually be applied until
// "EndEditTransaction" is called so you don't have to worry about
// locations changing while you are editing.
//=============================================================================

STDMETHODIMP CodeFile::StartEditTransaction()
{
    HRESULT hr = NOERROR;

#if IDE
    CComPtr<IVsTextLines> srpIVsTextLines = NULL;
    CComPtr<VBTextBuffer> srpVBTextBuffer = NULL;
    CComPtr<IVsTextManager> srpTextManager;
    CComPtr<IVsTextView> srpIVsTextView;

    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        return E_FAIL;
    }

    // We don't support nesting edits.
    VSASSERT(m_daEdits.Count() == 0, "Can't nest!");
    VSASSERT(!m_fEditing, "Already in the middle of an edit!");
    VSASSERT(!m_spUndo, "How come undo action is open?");

    if (m_fEditing)
    {
        return E_FAIL;
    }
    else
    {
        m_fEditing = true;
    }

    // Reset our transaction information to its default.  The before sequence
    // starts at -1 to ensure that its edits are sorted before the others.
    // The after sequence starts at 2 because 0 is reserved for deletes and
    // statement replacements while 1 is used for method body replacements.
    //
    m_lBeforeSequence = -1;
    m_lAfterSequence  = 2;

    // If there is a sourcefileview, use it, otherwise, use the invisible editor to do the insertion of code
    if (m_pSourceFile && m_pSourceFile->GetSourceFileView() && m_pSourceFile->GetSourceFileView()->GetVBTextBuffer())
    {
        srpVBTextBuffer = m_pSourceFile->GetSourceFileView()->GetVBTextBuffer();
        srpIVsTextLines = srpVBTextBuffer->GetIVsTextLines();
    }
    
	if(!srpIVsTextLines)
    {
        // try to grab an invisible editor
        if (m_srpInvisibleEditor == NULL)
        {
            IfFailGo(IDEHelpers::GetInvisibleEditor(m_pSourceFile, &m_srpInvisibleEditor));
        }

        // we need to get a source file to get to a text lines
        // use the invisible editor
        if (m_srpInvisibleEditor)
        {
            IfFailGo(m_srpInvisibleEditor->GetDocData(TRUE, IID_IVsTextLines, (void **)&srpIVsTextLines));
        }
        else
        {
            VSFAIL("Code Insertion Failed!");
            hr = E_FAIL;
            goto Error;
        }
    }

    if (srpIVsTextLines)
    {
        // Wrap the edit in an undo action so that the user can undo it if they want
        if (GetCompilerPackage() && GetCompilerPackage()->GetSite())
        {
            GetCompilerPackage()->GetSite()->QueryService(SID_SVsTextManager, IID_IVsTextManager, (LPVOID*) &srpTextManager);
        }

        if (srpTextManager)
        {
            srpTextManager->GetActiveView(FALSE, srpIVsTextLines, &srpIVsTextView);

            if (srpIVsTextView)
            {
                srpIVsTextView->QueryInterface(IID_IVsCompoundAction,(void **)&m_spUndo);
            }
        }

        if (m_spUndo)
        {
            StringBuffer sb;
            ResLoadStringRepl(STRID_CodeModelEdit,&sb);
            if (FAILED(m_spUndo->OpenCompoundAction(sb.GetString())))
            {
                m_spUndo = NULL;
            }
        }
    }

Error:

	if (hr != S_OK)
	{
		if (srpVBTextBuffer) // we can still succeed if we have a VBTextBuffer, we'll be missing the undo compound action
		{
			hr = S_OK;
		}
		else
		{
			m_fEditing = false;
		}
	}
        
#endif IDE

    return hr;
}


//=============================================================================
// Commits all of the edits applied to the file since the last
// call to StartEditTransaction.  This will cause each of the
// CodeFileEvents to fire at most one time. The ID for newly
// created element is returned.
//=============================================================================

STDMETHODIMP CodeFile::EndEditTransaction(BSTR *pbstrNewElementID)
{
#if IDE
    if (m_fIsZombie)
    {
        VSFAIL(L"Is CodeFile is Zombie, you can't use this CodeFile to get XML!");
        return E_FAIL;
    }

    if (!m_fEditing)
    {
        VSFAIL(L"StartEditTransaction must be called before EndEditTransaction");
        return E_FAIL;
    }

    VB_ENTRY();
    CComPtr<VBTextBuffer> srpVBTextBuffer = NULL;
    CComPtr<IVsTextLines> srpIVsTextLines = NULL;
    VSASSERT(wcslen(USZ_CRLF) == 2, "USZ_CRLF is no longer 2 characters long!");

    //Make sure that we can continue after checking SCC
    DynamicArray<STRING *> daFileNames;
    if (m_pSourceFile)
    {
        daFileNames.Add() = m_pSourceFile->GetFileName();
    }

    // We don't proceed if QueryEdit failed
    // This usually indicates the file got reloaded
    hr = IDEHelpers::QueryEdit(&daFileNames);

    if (FAILED(hr))
    {
        if (m_spUndo)
        {
            //We failed so we need to rollback any edits up to this point.
            m_spUndo->AbortCompoundAction();
            m_spUndo = NULL;
        }

        m_daEdits.Destroy();
        m_nraEdits.FreeHeap();
        m_fEditing = false;

        return hr;
    }

    // If there is a sourcefileview, use it, otherwise, use the invisible editor to do the insertion of code
    if (m_pSourceFile && m_pSourceFile->GetSourceFileView() && m_pSourceFile->GetSourceFileView()->GetVBTextBuffer())
    {
        srpVBTextBuffer = m_pSourceFile->GetSourceFileView()->GetVBTextBuffer();
        srpIVsTextLines = srpVBTextBuffer->GetIVsTextLines();
    }
    
	if (!srpIVsTextLines)
    {
        // try to grab an invisible editor
        if (m_srpInvisibleEditor == NULL)
        {
            IDEHelpers::GetInvisibleEditor(m_pSourceFile, &m_srpInvisibleEditor);
        }

        // we need to get a source file to get to a text lines
        // use the invisible editor
        if (m_srpInvisibleEditor)
        {
			m_srpInvisibleEditor->GetDocData(TRUE, IID_IVsTextLines, (void **)&srpIVsTextLines);
			if (!srpVBTextBuffer)
			{
				srpVBTextBuffer = VBTextBuffer::Create(srpIVsTextLines);
			}
        }

        VSASSERT(srpIVsTextLines || srpVBTextBuffer, "Cannot perform the edit without a view: decompilation will not happen.");
    }

    //
    // Stop the events from firing.
    //

    m_fHoldEvents = true;
    m_fFiredStatementChangedEvent = false;
    m_fFiredDeclarationStatementChangedEvent = false;

    // background thread needn't sleep since no one is typing
    m_pCompiler->SkipDelayWhenStartingNextBackgroundCompilationTask();

    // Commit the changes.
    if (m_spUndo)
    {
        m_spUndo->FlushEditActions();
    }

    if (srpVBTextBuffer)
    {
        hr = ApplyEdits(srpVBTextBuffer, pbstrNewElementID);
    }

    m_daEdits.Destroy();
    m_nraEdits.FreeHeap();

    //
    // Renable and fire the events.
    //

    m_fHoldEvents = false;

    // fire the CodeFile events only if code application was successful
    if (hr == NOERROR || hr == BUFFER_E_RELOAD_OCCURRED)
    {
        if (m_fFiredDeclarationStatementChangedEvent)
        {
            OnDeclarationStatementChanged();
        }
        else if (m_fFiredStatementChangedEvent)
        {
            OnStatementChanged();
        }
    }

    if (m_spUndo)
    {
        if (SUCCEEDED(hr))
        {
            m_spUndo->CloseCompoundAction();
            m_spUndo = NULL;
        }
        else
        {
            //We failed so we need to rollback any edits up to this point.
            m_spUndo->AbortCompoundAction();
            m_spUndo = NULL;
        }
    }

    if (m_pSourceFile && m_pSourceFile->GetSourceFileView())
    {
        m_pSourceFile->GetSourceFileView()->UpdateLineSeparators();
    }


    VB_EXIT_NORETURN();

    m_fHoldEvents = false;
    m_fEditing = false;
    return hr;

#else
    return NOERROR;
#endif IDE
}

STDMETHODIMP CodeFile::GetFileName(BSTR *pbstrFileName)
{
    HRESULT hr = NOERROR;

    if (pbstrFileName == NULL)
    {
        hr = E_POINTER;
        goto Error;
    }

    *pbstrFileName = NULL;

#if IDE
    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        hr = E_FAIL;
        goto Error;
    }

    // we need a sourcefileview to get the item id
    if (m_pSourceFile)
    {
        *pbstrFileName = ::SysAllocString(m_pSourceFile->GetFileName());

        if (pbstrFileName == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
#endif

Error:
    return hr;
}

STDMETHODIMP CodeFile::GetItemID(VSITEMID *pvsitemid)
{
    HRESULT hr = NOERROR;

    if (pvsitemid == NULL)
    {
        hr = E_POINTER;
        goto Error;
    }

    *pvsitemid = VSITEMID_NIL;

#if IDE
    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        hr = E_FAIL;
        goto Error;
    }

    // we need a sourcefileview to get the item id
    if (m_pSourceFile)
    {
        *pvsitemid = m_pSourceFile->GetItemId();
    }
#endif

Error:
    return hr;
}

//=============================================================================
// Gets the text location of a statement.  This will return the
// extent of the entire statement (i.e. from the start of the
// defining statement of a block to the end of the block end
// construct).
//=============================================================================

HRESULT CodeFile::GetTextLocation
(
    LPCOLESTR szId,
    LONG *plStartLine,
    LONG *plStartColumn,
    LONG *plEndLine,
    LONG *plEndColumn
)
{
    HRESULT hr = NOERROR;

    VSASSERT(szId, "Bad ID!");
    VSASSERT((plStartLine && plStartColumn && plEndLine && plEndColumn), "Bad location storage...");

    if (!szId)
    {
        return E_INVALIDARG;
    }

    if (plStartLine == NULL || plStartColumn == NULL || plEndLine == NULL || plEndColumn == NULL)
    {
        VSFAIL("Invalid NULL Pointer");
        return E_POINTER;
    }

    Location startLocation, endLocation;
    DecodeId(szId, wcslen(szId) + 1, &startLocation, &endLocation, NULL);

    if (wcschr(szId, '~'))
    {
        // Block location
        *plStartLine    = startLocation.m_lBegLine;
        *plStartColumn  = startLocation.m_lBegColumn;
        *plEndLine      = endLocation.m_lEndLine;
        *plEndColumn    = endLocation.m_lEndColumn;
    }
    else
    {
        // non-block location
        *plStartLine    = startLocation.m_lBegLine;
        *plStartColumn  = startLocation.m_lBegColumn;
        *plEndLine      = startLocation.m_lEndLine;
        *plEndColumn    = startLocation.m_lEndColumn;
    }

    return hr;
}

//=============================================================================
// Returns true if the language is case sensitive, or false if it is not.
//=============================================================================

HRESULT CodeFile::IsLanguageCaseSensitive
(
    BOOL *pflangCase
)
{
    if (pflangCase == NULL)
    {
        VSFAIL("Invalid NULL Pointer");
        return E_POINTER;
    }

    *pflangCase = false;

    return NOERROR;
}

//=============================================================================
// Returns true if the given identifier is valid in the language,
// or false if it is not.
//=============================================================================

HRESULT CodeFile::IsValidIdentifier
(
    _In_z_ LPCOLESTR szIdentifier,
    BOOL *pfIsValid
)
{
    if (szIdentifier == NULL)
    {
        VSFAIL("Invalid NULL Pointer");
        return E_INVALIDARG;
    }

    if (pfIsValid == NULL)
    {
        VSFAIL("Invalid NULL Pointer");
        return E_POINTER;
    }

    STRING *pIdentifierName = m_pCompiler->AddString(szIdentifier);
    tokens IdKeyword = TokenOfString(pIdentifierName);

    if (!Scanner::IsValidIdentiferOrBracketedIdentifier((WCHAR *)szIdentifier) || TokenIsReserved(IdKeyword))
    {
        *pfIsValid = false;
    }
    else
    {
        *pfIsValid = true;
    }

    return NOERROR;
}

//=============================================================================
// Returns true if the CodeFile is Ready to Produce statements?
// or false if it not
//=============================================================================

HRESULT CodeFile::IsReadyToProduceStatements
(
    BOOL *pfIsReadyToProduceStatements
)
{
#if IDE

    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        *pfIsReadyToProduceStatements = FALSE;
        return E_FAIL;
    }

    if (pfIsReadyToProduceStatements == NULL)
    {
        VSFAIL("Invalid NULL Pointer");
        return E_POINTER;
    }

    if (m_pSourceFile->GetCompState() >= CS_Bound)
    {
        *pfIsReadyToProduceStatements = TRUE;
    }
    else
    {
        *pfIsReadyToProduceStatements = FALSE;
    }
#else
        *pfIsReadyToProduceStatements = TRUE;
#endif IDE

    return NOERROR;
}

//*****************************************************************************
// Implementation.
//*****************************************************************************

//=============================================================================
// Fires the statement changed events.
//=============================================================================

void CodeFile::OnStatementChanged()
{
#if IDE

    // If we are already processing OnStatementChanged(), no need to re-process it.
    if (m_fFiringOnStatementEvent)
    {
        return;
    }

    if (m_fHoldEvents)
    {
        m_fFiredStatementChangedEvent = true;
    }
    else
    {
        m_fFiringOnStatementEvent = true;

        ICodeFileEvents **rgArray = m_daEventListeners.Array();
        unsigned iEvent, cEvents = m_daEventListeners.Count();

        // Set this before we fire the first event because
        // firing the event might cause someone to run that depends
        // on m_fFiredChangedEvent (VS7 196360)
        m_fFiredChangedEvent = true;

        for (iEvent = 0; iEvent < cEvents; iEvent++)
        {
            (void)rgArray[iEvent]->OnStatementChanged();
        }

        m_fFiringOnStatementEvent = false;

        if (m_fIsInBoundState)
        {
            // If we reached bound state while we were busy processing this event, go ahead
            // and fire the boundstate event now.
            //
            m_fIsInBoundState = false;
            GetCompilerSharedState()->GetMessages()->FileInBindable(m_pSourceFile);
        }

        m_fFiredStatementChangedEvent = false;
    }

#endif IDE
}

//=============================================================================
// Fires the declaration changed events.
//=============================================================================

void CodeFile::OnDeclarationStatementChanged()
{
#if IDE

    // If we are already processing OnDeclarationStatementChanged(), no need to re-process it.
    if (m_fFiringOnDeclarationEvent)
    {
        return;
    }

    if (m_fHoldEvents)
    {
        m_fFiredDeclarationStatementChangedEvent = true;
    }
    else
    {
        m_fFiringOnDeclarationEvent = true;

        ICodeFileEvents **rgArray = m_daEventListeners.Array();
        unsigned iEvent, cEvents = m_daEventListeners.Count();

        // Set this before we fire the first event because
        // firing the event might cause someone to run that depends
        // on m_fFiredChangedEvent (VS7 196360)
        m_fFiredChangedEvent = true;

        for (iEvent = 0; iEvent < cEvents; iEvent++)
        {
            (void)rgArray[iEvent]->OnDeclarationStatementChanged();
        }

        m_fFiringOnDeclarationEvent = false;

        if (m_fIsInBoundState)
        {
            // If we reached bound state while we were busy processing this event, go ahead
            // and fire the boundstate event now.
            //
            m_fIsInBoundState = false;
            GetCompilerSharedState()->GetMessages()->FileInBindable(m_pSourceFile);
        }

        m_fFiredDeclarationStatementChangedEvent = false;
    }

#endif IDE
}

//=============================================================================
// Fires the "ready to get XML" event.
//=============================================================================

void CodeFile::OnFileInBindable()
{
#if IDE

    if (m_fEnableFiredChangedEvent || m_fFiredChangedEvent)
    {
        if (m_fFiringOnDeclarationEvent || m_fFiringOnStatementEvent)
        {
            // If we get re-entered while we are firing Changed events, we only set a flag
            // and return so that we can re-fire the events later.
            //
            m_fIsInBoundState = true;
            return;
        }
        else
        {
            ICodeFileEvents **rgArray = m_daEventListeners.Array();
            unsigned iEvent, cEvents = m_daEventListeners.Count();

            for (iEvent = 0; iEvent < cEvents; iEvent++)
            {
                (void)rgArray[iEvent]->OnReadyToProduceStatements();
            }

            m_fFiredChangedEvent = false;
            m_fEnableFiredChangedEvent = false;     // only first time around we do this
        }
    }

#endif IDE
}

void CodeFile::Fire_OnDropped()
{
#if IDE

    if (m_fIsZombie)
    {
        VSFAIL("This File is already dropped");
        return;
    }

    unsigned iEvent, cEvents = m_daEventListeners.Count();
    size_t ArraySize;
    if ( !VBMath::TryMultiply(cEvents, sizeof(ICodeFileEvents*), &ArraySize) )
    {
        VSFAIL("Overflow error.");
        return;
    }

    // The OnDropped() call will call UnadviseCodeFileEvents and will shrink the array by 1,
    // which is the zerioth element. So we need to make a copy of the array first, and then
    // use that copy to unadvice clients.
    VBHeapPtr<ICodeFileEvents*> ppEventListenersCopy;
    ppEventListenersCopy.AllocateBytes(ArraySize);
    memcpy((void *)ppEventListenersCopy, (void *)m_daEventListeners.Array(), ArraySize);

    for (iEvent = 0; iEvent < cEvents; iEvent++)
    {
        VSASSERT(iEvent + m_daEventListeners.Count() == cEvents, "Soneone unadviced more than once!");

        (void)ppEventListenersCopy[iEvent]->OnDropped();
    }

    if (m_spUndo)
    {
        VSFAIL("CodeFile is being Zombied in the middle of an Undo action!");
        m_spUndo = NULL;
    }

    m_srpInvisibleEditor.Release();

    m_fIsZombie = true;

#endif IDE
}

//=============================================================================
// Helper that converts a tree to an id.  The passed-in buffers
// must be at least cbIdSize big. This method takes in a new parse tree.
//
// The ID format is as follows:
//
//   For a non-block statement, the ID is encoded as 4 equal-length numbers
//   concatinated together.  The numbers stand for starting line, starting
//   column, end line and ending column, in that order.
//
//   A block statement has two such sets of numbers separated by a ~.  The
//   first set gives the location of the block statement itself, the
//   second one gives the location of end construct.
//
//=============================================================================

void CodeFile::MakeStatementId
(
    ParseTree::Statement *pstatement,
    _Out_z_cap_(cchId) WCHAR *wszId,
    size_t cchId
)
{
    Location loc;
    unsigned u[4];

    VSASSERT(pstatement != NULL, "Invalid NULL Pointer");
    VSASSERT(wszId != NULL, "Invalid NULL Pointer");

    // Get the locations for the pieces we're consuming.
    if (pstatement->IsBlock() &&
        pstatement->Opcode != ParseTree::Statement::Region &&
        pstatement->Opcode != ParseTree::Statement::EndRegion)
    {
        size_t cch;

        loc = pstatement->TextSpan;

        u[0] = loc.m_lBegLine;
        u[1] = loc.m_lBegColumn;
        u[2] = loc.m_lEndLine;
        u[3] = loc.m_lEndColumn;

        EncodeNumber(wszId, cchId, &cch, u, _countof(u));

        // Encode the location of the end statement if there is one.
        if (pstatement->AsBlock()->TerminatingConstruct)
        {
            ParseTree::Statement *pendStatement = pstatement->AsBlock()->TerminatingConstruct;

            loc = pendStatement->TextSpan;

            // Just use the location as-is.
            u[0] = loc.m_lBegLine;
            u[1] = loc.m_lBegColumn;
            u[2] = loc.m_lEndLine;
            u[3] = loc.m_lEndColumn;

#pragma prefast(suppress: 26015, "cch comes from EncodeNumber which guarantees this will not overflow.")
            wszId[cch++] = '~';

#pragma prefast(suppress: 22018, "cch is guaranteed smaller than cchId because it came from EncodeNumber.")
            EncodeNumber(wszId + cch, cchId - cch, &cch, u, _countof(u));
        }
    }
    else
    {
        if (pstatement->IsMethodDefinition())
        {
            size_t cch;
            loc = pstatement->TextSpan;

            u[0] = loc.m_lBegLine;
            u[1] = loc.m_lBegColumn;
            u[2] = loc.m_lEndLine;
            u[3] = loc.m_lEndColumn;

            EncodeNumber(wszId, cchId, &cch, u, _countof(u));

            // Encode the location of the end statement if there is one.
            if (pstatement->AsMethodDefinition()->TerminatingConstruct)
            {
                ParseTree::Statement *pendStatement = pstatement->AsMethodDefinition()->TerminatingConstruct;

                loc = pendStatement->TextSpan;

                // Just use the location as-is.
                u[0] = loc.m_lBegLine;
                u[1] = loc.m_lBegColumn;
                u[2] = loc.m_lEndLine;
                u[3] = loc.m_lEndColumn;

                wszId[cch++] = '~';

                EncodeNumber(wszId + cch, cchId - cch, &cch, u, _countof(u));
            }
        }
        else
        {
            // Normal statements are easy, just encode the statement's location.
            MakeStatementId(pstatement->TextSpan, wszId, cchId);
        }
    }
}

void CodeFile::MakeStatementId
(
    const Location &loc,
    _Out_z_cap_(cchId) WCHAR *wszId,
    size_t cchId
)
{
    unsigned u[4];
    size_t cch;

    VSASSERT(wszId != NULL, "Invalid NULL Pointer");

    u[0] = loc.m_lBegLine;
    u[1] = loc.m_lBegColumn;
    u[2] = loc.m_lEndLine;
    u[3] = loc.m_lEndColumn;

    EncodeNumber(wszId, cchId, &cch, u, _countof(u));
}

//=============================================================================
// Helper that converts a tree to an id.  The passed-in buffers
// must be at least cbIdSize big.
//
// The ID format is as follows:
//
//   For a non-block statement, the ID is encoded as 4 equal-length numbers
//   concatinated together.  The numbers stand for starting line, starting
//   column, end line and ending column, in that order.
//
//   A block statement has two such sets of numbers separated by a ~.  The
//   first set gives the location of the block statement itself, the
//   second one gives the location of end construct.
//
//=============================================================================

void CodeFile::MakeId
(
    ILTree::ILNode *ptree,
    _Out_z_cap_(cchId) WCHAR *wszId,
    size_t cchId
)
{
    Location loc;
    unsigned u[4];

    VSASSERT(ptree != NULL, "Invalid NULL Pointer");
    VSASSERT(wszId != NULL, "Invalid NULL Pointer");
    VSASSERT(ptree->IsStmtNode(), "Can only do this on a statement or a block.");

    loc = ptree->Loc;


    // Get the locations for the pieces we're consuming.
    if (!ptree->IsExecutableBlockNode())
    {
        size_t cch;

        // Normal statements are easy, just encode the statement's location.
        u[0] = loc.m_lBegLine;
        u[1] = loc.m_lBegColumn;
        u[2] = loc.m_lEndLine;
        u[3] = loc.m_lEndColumn;

        EncodeNumber(wszId, cchId, &cch, u, _countof(u));
    }
    else
    {
        size_t cch;

        // Block statements currently encode the entire block (minus the
        // end construct) in the tree's location.  Centis is working
        // on fixing this...for now just put all space between
        // the start of the block and the first statement as part of
        // the block declaration.
        //
        u[0] = loc.m_lBegLine;
        u[1] = loc.m_lBegColumn;

        if (ptree->AsExecutableBlock().ptreeChild)
        {
            loc = ptree->AsExecutableBlock().ptreeChild->Loc;

            u[2] = loc.m_lBegLine;
            u[3] = loc.m_lBegColumn;
        }
        else
        {
            u[2] = loc.m_lEndLine;
            u[3] = loc.m_lEndColumn;
        }

        EncodeNumber(wszId, cchId, &cch, u, _countof(u));

        // Encode the location of the end statement if there is one.
        if (ptree->AsStatement().Next)
        {
            ptree = ptree->AsStatement().Next;

            loc = ptree->Loc;

            // Just use the location as-is.
            u[0] = loc.m_lBegLine;
            u[1] = loc.m_lBegColumn;
            u[2] = loc.m_lEndLine;
            u[3] = loc.m_lEndColumn;

#pragma prefast(suppress: 26015, "cch comes from EncodeNumber which guarantees this will not overflow.")
            wszId[cch++] = '~';

#pragma prefast(suppress: 22018, "cch is guaranteed smaller than cchId because it came from EncodeNumber.")
            EncodeNumber(wszId + cch, cchId - cch, &cch, u, _countof(u));
        }
    }
}

//=============================================================================
// Encodes a set of numbers into a concatinated string.  All of the
// numbers will be encode into fields of the same size.  This will not
// use ~ or ".
//
// Returns the size of the encoded number.
//=============================================================================

#define LOWEST (0x21)
#define HIGHEST (0x7c)
#define ESCAPE ('"')
#define TO (0x7d)
#define BASE (HIGHEST - LOWEST)

void CodeFile::EncodeNumber
(
    _Pre_cap_(cchBuffer) _Pre_invalid_ _Post_z_count_x_(*cchUsed + 1) wchar_t *wszBuffer, 
    size_t cchBuffer,
    _Out_ size_t *cchUsed,
    _In_count_(NumberCount) unsigned u[],
    unsigned NumberCount
)
{
    VSASSERT(wszBuffer != NULL, "Invalid NULL Pointer");
    VSASSERT(u != NULL, "Invalid NULL Pointer");

    wszBuffer[0] = 0;
    *cchUsed = 0;

    if (NumberCount != 4 || cchBuffer < 21)
    {
        return;
    }

    // Buffers for each number.
    WCHAR wsz[4][6];
    unsigned cchMax = 1;
    unsigned i, ich, cch, ch;

#if DEBUG
    // Save the array for future checking.
    unsigned u2[4];

    memcpy(u2, u, sizeof(u2));
#endif DEBUG

    // Fill out each number string.
    for (i = 0; i < 4; i++)
    {
        cch = 1;
        wsz[i][5] = 0;

        for (ich = 0; ich < 5; ich++)
        {
            ch = u[i] % BASE + LOWEST;
            u[i] /= BASE;

            if (u[i])
            {
                cch++;
            }

            if (ch == ESCAPE)
            {
                ch = TO;
            }

            wsz[i][4 - ich] = ch;
        }

        VSASSERT(u[i] == 0, "Number too big to fit?");

        if (cch > cchMax)
        {
            cchMax = cch;
        }
    }

    // Concatenate the last cchMax character of each buffer into the
    // final buffer.
    //
    for (i = 0; i < 4; i++)
    {
#pragma prefast(suppress: 26011 26014, "cchMax cannot be greater than 5 and the buffer is guaranteed to be at least 21 elements long")
        memcpy(wszBuffer + (i * cchMax), wsz[i] + (5 - cchMax), cchMax * sizeof(WCHAR));
    }

    *cchUsed = 4 * cchMax;
#pragma prefast(suppress: 26014 26044, "cchMax cannot be greater than 5 and the buffer is guaranteed to be at least 21 elements long")
    wszBuffer[*cchUsed] = 0;

#if DEBUG

    // Make sure that we'll get the same value when decoding.
    DecodeNumber(wszBuffer, *cchUsed, u, NumberCount);

    for (i = 0; i < 4; i++)
    {
        VSASSERT(u[i] == u2[i], "Bad decoding.");
    }

#endif DEBUG
}

//=============================================================================
// Decodes a set of numbers from a concatenated string.
//=============================================================================

void CodeFile::DecodeNumber
(
    _In_count_(cchBuffer) const WCHAR *wszBuffer,
    size_t cchBuffer,
    _Out_cap_(NumberCount) unsigned u[],
    unsigned NumberCount
)
{
    if (NumberCount != 4 || cchBuffer % 4 != 0)
    {
        VSFAIL("Invalid Encoding");

        return;
    }

    const size_t cchNumber = cchBuffer / NumberCount;

    unsigned i, ch;
    size_t ich;

    for (i = 0; i < NumberCount; i++)
    {
        for (ich = 0; ich < cchNumber; ich++)
        {
#pragma prefast(suppress: 26014, "The number of loop iterations is bounded by cchBuffer / NumberCount")
            ch = *(wszBuffer++);

            if (ch == TO)
            {
                ch = ESCAPE;
            }

            ch -= LOWEST;

            u[i] = u[i] * BASE + ch;
        }
    }
}

//****************************************************************************
// This static function will convert XML string into valid VB string. It can
// be used by any client who originally used XMLGen to XMl for a file.
// We are using the Private-User-Area (0xE000 - 0xE01F) for characters from
// 0x0000 to 0x001F excluding UCH_TAB, UCH_LF, and UCH_CR. When converting
// from XML to VB code, we need to restore the original character back.
//
// Please note: The calles is responsible for releasing the memory allocated by
// this function, which is point to by the out param pOutText.
//****************************************************************************
static HRESULT DecodePrivateUseCharacters(
    _In_z_ const WCHAR *pInText,
    _In_z_ const WCHAR *wszElementType,
    _Deref_out_z_ WCHAR **pOutText)
{
    HRESULT hr = NOERROR;
    if (!pInText || pOutText == NULL)
    {
        return E_INVALIDARG;
    }
    StringBuffer DecodedBuffer;
    if (wcslen(pInText) + 32 < 32 || 
        !VBMath::TryMultiply((wcslen(pInText) + 32), sizeof(WCHAR)))
    {
        return E_FAIL;
    }
    DecodedBuffer.AllocateSize((wcslen(pInText) + 32) * sizeof(WCHAR));
    const WCHAR *EndOfString = wcsrchr(pInText, UCH_NULL);
    while (*pInText)
    {
        if ((EndOfString - pInText) >= 4 && *pInText == L'&')
        {
            const WCHAR *NextCharacter = pInText + 1;
            if ((EndOfString - NextCharacter) >= 5 && !wcsncmp(NextCharacter, L"quot;", 5))
            {
                if (TagMatchesCase(wszElementType, ELEMENT_String))
                {
                    DecodedBuffer.AppendString(L"\"\"");          // In VB, Double quotes need to be escaped by using double quotes.
                }
                else
                {
                    DecodedBuffer.AppendChar(L'\"');
                }
                pInText += 6;
                continue;
            }
            if ((EndOfString - NextCharacter) >= 5 && !wcsncmp(NextCharacter, L"apos;", 5))
            {
                DecodedBuffer.AppendChar(L'\'');
                pInText += 6;
                continue;
            }
            if ((EndOfString - NextCharacter) >= 4 && !wcsncmp(NextCharacter, L"amp;", 4))
            {
                DecodedBuffer.AppendChar(L'&');
                pInText += 5;
                continue;
            }
            if (!wcsncmp(NextCharacter, L"lt;", 3))
            {
                DecodedBuffer.AppendChar(L'<');
                pInText += 4;
                continue;
            }
            if (!wcsncmp(NextCharacter, L"gt;", 3))
            {
                DecodedBuffer.AppendChar(L'>');
                pInText += 4;
                continue;
            }
        }
        if (*pInText >= START_OF_ENCODING_PRIVATE_USER_AREA && *pInText <= END_OF_ENCODING_PRIVATE_USER_AREA)
        {
            DecodedBuffer.AppendChar(*pInText & ENCODING_MASK);   // Special private-use-character
        }
        else if (*pInText == CHAR_DoubleQuote && (TagMatchesCase(wszElementType, ELEMENT_String) || TagMatchesCase(wszElementType, ELEMENT_Char)))
        {
            DecodedBuffer.AppendString(L"\"\"");          // In VB, Double quotes need to be escaped by using double quotes.
        }
        else
        {
            DecodedBuffer.AppendChar(*pInText);           // Normal character, just go ahead and copy it
        }
        ++pInText;
    }
    size_t outStringLength = DecodedBuffer.GetStringLength();
    *pOutText = new (zeromemory) WCHAR[outStringLength + 1];
    memcpy(*pOutText, DecodedBuffer.GetString(), outStringLength * sizeof(WCHAR));
    (*pOutText)[outStringLength] = UCH_NULL;
    return hr;
}
//=============================================================================
// Unwraps an id into the defining statement location and the block location.
//
// The first location gives the location of the defining statement.  The
// second location gives the location of the entire block, including the
// defining statement and the end construct.
//=============================================================================

void CodeFile::DecodeId
(
    _In_count_(cchEncodedId) _Pre_z_ const WCHAR *EncodedID,
    size_t cchEncodedId,
    Location *ploc,
    Location *plocEnd,
    Location *plocExtra
)
{
    WCHAR* wszId = NULL;

    // Restore any Unicode characters that we may encoded while generating XML
    // as well as any entity reference characters that may have been used
    // in the ID.
    if FAILED(DecodePrivateUseCharacters(EncodedID, ATTRIB_VALUE_Id, &wszId))
    {
        VSFAIL(L"DecodePrivateUseCharacters failed");
        return;
    }

    const WCHAR *pchDivider = NULL;
    const WCHAR *pchSecondDivider = NULL;
    const WCHAR *pchEnd;

    // Chunk the string.
    pchEnd = wszId + wcslen(wszId);
    pchDivider = wcschr(wszId, '~');

    // Init out parameters
    if (plocExtra)
    {
        plocExtra->Invalidate();
    }

    if (pchDivider)
    {
        pchSecondDivider = wcschr(pchDivider + 1, '~');

        if (pchSecondDivider)
        {
            pchSecondDivider++;
        }
    }

    VSASSERT(pchEnd && pchEnd != wszId, "Bad input.");

    // Is this a block?
    if (pchDivider)
    {
        unsigned u1[4], u2[4];
        memset(u1, 0, sizeof(u1));
        memset(u2, 0, sizeof(u2));

        DecodeNumber(wszId, pchDivider - wszId, u1, _countof(u1));

        pchDivider++;

        // Check for a second '~' in there, and if there is, encode the thrid portion
        if (pchSecondDivider)
        {
            DecodeNumber(pchDivider, pchSecondDivider - pchDivider - 1, u2, _countof(u2));
        }
        else
        {
            DecodeNumber(pchDivider, pchEnd - pchDivider, u2, _countof(u2));
        }

        ploc->m_lBegLine    = u1[0];
        ploc->m_lBegColumn  = u1[1];
        ploc->m_lEndLine    = u1[2];
        ploc->m_lEndColumn  = u1[3];

        plocEnd->m_lBegLine     = u2[0];
        plocEnd->m_lBegColumn   = u2[1];
        plocEnd->m_lEndLine     = u2[2];
        plocEnd->m_lEndColumn   = u2[3];

        if (plocExtra)
        {
            if (pchSecondDivider)
            {
                unsigned u3[4];
                memset(u3, 0, sizeof(u3));

                DecodeNumber(pchSecondDivider, pchEnd - pchSecondDivider, u3, _countof(u3));

                plocExtra->m_lBegLine    = u3[0];
                plocExtra->m_lBegColumn  = u3[1];
                plocExtra->m_lEndLine    = u3[2];
                plocExtra->m_lEndColumn  = u3[3];
            }
            else
            {
                plocExtra->Invalidate();
            }
        }
    }
    else            // Handle a normal statement.
    {
        unsigned u[4];
        memset(u, 0, sizeof(u));

        DecodeNumber(wszId, pchEnd - wszId, u, _countof(u));

        ploc->m_lBegLine = plocEnd->m_lBegLine = u[0];
        ploc->m_lBegColumn = plocEnd->m_lBegColumn = u[1];
        ploc->m_lEndLine = plocEnd->m_lEndLine = u[2];
        ploc->m_lEndColumn = plocEnd->m_lEndColumn = u[3];
    }

    delete[] wszId;
}


//=============================================================================
// Add an edit to the array.
//=============================================================================
void CodeFile::AddVBEdit
(
    _In_z_ const WCHAR *wszId,
    CodeEditKind ek,
    CodeInsertionKind InsertionKind,
    _In_z_ const WCHAR *wszText
)
{
#if IDE
    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        return;
    }

    if (!wszText)
    {
        VSASSERT(ek == DeleteDeclaration, "Invalid text supplied for edit action that is not a DeleteDeclaration.");
        wszText = L"";
    }

    size_t cbSizeText = 0;
    Edit *pEdit;

    // Allocate space for the edit structure.
    if (ek != DeleteDeclaration)
    {
        cbSizeText = (wcslen(wszText) + 1) * sizeof(WCHAR);
    }
    
    // Note: For 64 bit it is possible to allocate slightly more than necessary because the structure
    // end by an [0] array. For now, it doesn?t seem to be worth to  change.
    pEdit = (Edit *)m_nraEdits.Alloc(sizeof(Edit) + cbSizeText);

    // Fill in the edit kind.
    pEdit->m_EditKind = ek;
    pEdit->m_InsertionKind = InsertionKind;
    DecodeId(wszId, wcslen(wszId) + 1, &pEdit->m_loc, &pEdit->m_locEnd, NULL);

    // Fill in the sequence.
    switch(ek)
    {
    case AddBefore:
    case AddBeforeEnd:
        pEdit->m_lSequence =  m_lBeforeSequence--;
        break;

    case AddAfter:
    case AddAfterBegining:
        pEdit->m_lSequence = m_lAfterSequence++;
        break;

    case AddAtEndOfFile:
        if (m_pSourceFile && m_pSourceFile->GetSourceFileView() && m_pSourceFile->GetSourceFileView()->GetTextLines())
        {
            IVsTextLines *pIVsTextLines = m_pSourceFile->GetSourceFileView()->GetTextLines();
            long iLines = 0;
            long iColumns = 0;

            pIVsTextLines->GetLineCount(&iLines);

            iLines = max(0, iLines - 1);
            pIVsTextLines->GetLengthOfLine(iLines, &iColumns);

            pEdit->m_loc.SetLocation(iLines, iColumns-1);
            pEdit->m_lSequence = m_lAfterSequence++;
        }
        else
        {
            VSFAIL("How can we not have a SourceFileView and IVSTextLines()?");
        }
        break;

    case AddAtStartOfFile:
        pEdit->m_loc.SetLocation(0,0);
        pEdit->m_lSequence = m_lAfterSequence++;
        break;

    case ReplaceBody:
        // This actually replaces the gunk inside of a block, not the block itself.
        // Munge the locations to reflect this.
        //
        pEdit->m_loc.m_lBegLine = pEdit->m_loc.m_lEndLine;
        pEdit->m_loc.m_lBegColumn = pEdit->m_loc.m_lEndColumn;
        pEdit->m_loc.m_lEndLine = pEdit->m_locEnd.m_lBegLine;
        pEdit->m_loc.m_lEndColumn = pEdit->m_locEnd.m_lBegColumn;

        pEdit->m_locEnd = pEdit->m_loc;
        pEdit->m_lSequence = 0;
        break;

    case ReplaceDeclaration:
        break;

    case DeleteDeclaration:
        pEdit->m_lSequence = 0;
        break;

    default:
        VSFAIL("Bad enum value.");
    }

    // Copy the text.
    memcpy(pEdit->m_wszText, wszText, cbSizeText);


    // Add this to the end of the edit array.
    m_daEdits.Add() = pEdit;
#endif IDE
}

//=============================================================================
// Helper to sort edits.
//=============================================================================

int _cdecl CodeFile::SortEdits
(
    const void *arg1,
    const void *arg2
)
{
    Edit *pEdit1, *pEdit2;

    pEdit1 = ((Edit *)arg1);
    pEdit2 = ((Edit *)arg2);

    // Compare the locations.
    int i = Location::CompareStartPoints(&pEdit1->m_loc, &pEdit2->m_loc);

    if (i == 0)
    {
        if (pEdit1->m_lSequence < pEdit2->m_lSequence)
        {
            i = -1;
        }
        else if (pEdit2->m_lSequence < pEdit1->m_lSequence)
        {
            i = 1;
        }
    }

    return i;
}

//=============================================================================
// Sorts edits by insertion sort
//=============================================================================
void CodeFile::InsertionSortEdits()
{
    unsigned iEdit = 1; // rgEdits[0:iEdit-1] are sorted
    unsigned cEdits = m_daEdits.Count();
    unsigned j;
    Edit **rgEdits = m_daEdits.Array();
    Edit *peditToPlace = NULL;      // next edit to sort into array
    Edit *peditTemp = NULL;

    // put the iEdit'th element in place
    while (iEdit < cEdits)
    {
        // nothing to do if the next is bigger than iEdit
        // otherwise we have to put rgEdit[iEdit] into place
        if (SortEdits(rgEdits[iEdit-1], rgEdits[iEdit]) > 0)
        {
            // invariant:
            j = iEdit;
            peditToPlace = rgEdits[j];
            while(j > 0 && SortEdits(rgEdits[j-1], peditToPlace) > 0)
            {
                rgEdits[j] = rgEdits[j-1];
                j--;
            }
            rgEdits[j] = peditToPlace;
        }
        iEdit++;
    }

    // now that they are sorted, we must remove all but the last replace in the same location
    // ISSUE: overlapping locations for replace
    iEdit = 0;
    while (iEdit + 1 < cEdits)
    {
        // this round we complare [iEdit] and [iEdit + 1]
        if (rgEdits[iEdit]->m_EditKind == rgEdits[iEdit + 1]->m_EditKind &&
            rgEdits[iEdit]->m_loc.IsEqual(&rgEdits[iEdit + 1]->m_loc) &&
            rgEdits[iEdit]->m_locEnd.IsEqual(&rgEdits[iEdit + 1]->m_locEnd))
        {
            // this is a single edit, so we leave it alone
            switch (rgEdits[iEdit]->m_EditKind)
            {
                case ReplaceDeclaration:
                    // remove iEdit, since iEdit +1 will just overwrite it anyway
                    m_daEdits.Remove(iEdit);

                    // lowering cEdits makes progress in the loop, we have a new iEdit value
                    // so we can't increment that
                    cEdits--;
                    break;

                case AddAtStartOfFile:
                case AddAtEndOfFile:
                case AddBefore:
                case AddBeforeEnd:
                case AddAfter:
                case AddAfterBegining:
                case DeleteDeclaration:
                default:
                    iEdit++;
                    break;
            }

        }
        else
        {
            iEdit++;
        }

    }
    return;
}
//=============================================================================
// Applies the edits in the array to the text buffer.
//=============================================================================

HRESULT CodeFile::ApplyEdits(VBTextBuffer* pIVsTextLines, BSTR *pbstrNewElementID)
{
    HRESULT hr = NOERROR;

#if IDE
    if (m_fIsZombie)
    {
        VSFAIL("CodeFile is Zombie, you can't use this CodeFile to get XML!");
        return E_FAIL;
    }

    m_InsertionKind = CodeModel_Unknown;
    StringBuffer sbCode;

    if (pbstrNewElementID)
    {
        *pbstrNewElementID = NULL;
    }

    SourceFileView *pSourceFileView = SourceFileView::GetOrCreate(m_pSourceFile, pIVsTextLines->GetIVsTextLines());
    bool IgnoreCommitFlag = false;
    
    if (pSourceFileView && pSourceFileView->IsExceptionUnwindInProgress())
    {
        return OLECMDERR_E_CANCELED;
    }

    if (pSourceFileView)
    {
        IgnoreCommitFlag = pSourceFileView->SetIgnoreCommit(true);
    }

    //
    // Prepare the array to be processed.
    //

    // Sort the edit list.
    // we need a stable sort, this list is always small and all the edits tend to be clustered
    // at a few locations, so insertion sort is good
    InsertionSortEdits();

    // Walk the array looking for delete edits.  Remove any edits on statements
    // that are contained inside of the deleted statement.
    //
    unsigned iEdit, iEditStore = 0, cEdits = m_daEdits.Count();
    Edit **rgEdits = m_daEdits.Array();

    for (iEdit = 0; iEdit < cEdits; iEdit++)
    {
        rgEdits[iEditStore++] = rgEdits[iEdit];

        if (rgEdits[iEdit]->m_EditKind == DeleteDeclaration)
        {
            while (iEdit < (cEdits - 1))
            {
                if (!rgEdits[iEdit + 1]->m_loc.IsBetweenInclusive(&rgEdits[iEdit]->m_loc, &rgEdits[iEdit]->m_locEnd))
                {
                    break;
                }

                iEdit++;
            }
        }

    }

    cEdits = iEditStore;

#if ID_DEBUG

    //
    // Make sure that we're not trying to delete or replace the same statement twice.
    //

    for (iEdit = 0; iEdit < cEdits; iEdit++)
    {
        if (rgEdits[iEdit]->m_EditKind == DeleteDeclaration)
        {
            unsigned iEditCurrent;

            for(iEditCurrent = iEdit + 1; iEditCurrent < cEdits; iEditCurrent++)
            {
                if (!rgEdits[iEditCurrent].m_loc.IsEqual(rgEdits[iEdit]->m_loc))
                {
                    break;
                }

                VSASSERT(rgEdits[iEditCurrent].m_lSequence != rgEdits[iEdit]->m_lSequence, "Multiple deletes on the same statement!");
            }
        }
    }

#endif ID_DEBUG

    //
    // Run the array.
    //
    CodeEditKind EditKind = AddAtStartOfFile;
    Location loc;
    StringBuffer sbBuffer;

    // Walk the array backwards, applying each edit.
    for (iEdit = cEdits; iEdit > 0;)
    {
        iEdit--;
        sbBuffer.Clear();

        sbCode.Clear();
        TextLineData ld;
        EditKind = rgEdits[iEdit]->m_EditKind;

        unsigned InsertionLineNumber = rgEdits[iEdit]->m_loc.m_lBegLine;

        if (EditKind == AddBefore ||
            EditKind == AddBeforeEnd ||
            EditKind == AddAtStartOfFile)
        {
            if (SUCCEEDED(pIVsTextLines->GetLineDataEx(gldeDefault, InsertionLineNumber, 0, 0, &ld, NULL)))
            {
                if (!IsLineBlank(ld.GetText(), rgEdits[iEdit]->m_loc.m_lBegColumn))
                {
                    // This code handles this case:
                    //
                    //    Class a
                    //        Sub foo()
                    //
                    //        End Sub : End Class
                    //
                    // Before we can insert the new element, we need to create a new line first
                    // so we do that by insertion a new line followed by spaces in order to
                    // align the End Class, then we can process the insertion as normal.

                    StringBuffer InsertionString;
                    InsertionString.AppendWithLength(USZ_CRLF, CCH_CRLF);

                    pIVsTextLines->ReleaseLineDataEx(&ld);

                    IfFailGo(pIVsTextLines->ReplaceLinesEx(rtfKeepMarkers | rtfClientSuppressFormatting,
                        InsertionLineNumber, rgEdits[iEdit]->m_loc.m_lBegColumn,
                        InsertionLineNumber, rgEdits[iEdit]->m_loc.m_lBegColumn,
                        InsertionString.GetString(), InsertionString.GetStringLength(), NULL));

                    ++InsertionLineNumber;
                    ++rgEdits[iEdit]->m_loc.m_lBegLine;
                    ++rgEdits[iEdit]->m_loc.m_lEndLine;

                    if (pSourceFileView)
                    {
                        pSourceFileView->ClearParseTreeCacheIfNeeded();
                    }
                }
                else
                {
                    pIVsTextLines->ReleaseLineDataEx(&ld);
                }
            }

            if (InsertionLineNumber > 0)
            {
                --InsertionLineNumber;
            }
            else
            {
                VSASSERT(EditKind != AddBeforeEnd, "How can an insertion after an End Construct happen on line 0?");
            }
        }

        bool IsPreviousLineBlank = false;

        if (EditKind == AddBefore ||
            EditKind == AddBeforeEnd ||
            EditKind == AddAfterBegining ||
            EditKind == AddAfter ||
            EditKind == AddAtStartOfFile ||
            EditKind == AddAtEndOfFile)
        {
            // If the insertion type is anything else, we don't care about the previous line.
            if (SUCCEEDED(pIVsTextLines->GetLineDataEx(gldeDefault, InsertionLineNumber, 0, 0, &ld, NULL)))
            {
                if (IsLineBlank(ld.GetText(), ld.GetLength()))
                {
                    IsPreviousLineBlank = true;
                }
                else
                {
                    IsPreviousLineBlank = false;
                }

                pIVsTextLines->ReleaseLineDataEx(&ld);
            }
        }

        switch(EditKind)
        {
        case AddAtStartOfFile:
        case AddBefore:
        case AddBeforeEnd:
            {

                    m_InsertionKind = rgEdits[iEdit]->m_InsertionKind;

                IfFailGo(pIVsTextLines->ReplaceLinesEx(rtfKeepMarkers | rtfClientSuppressFormatting,
                    rgEdits[iEdit]->m_loc.m_lBegLine, 0,
                    rgEdits[iEdit]->m_loc.m_lBegLine, 0,
                    m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? L"\r\n\r\n" : L"\r\n",
                    m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? 4 : 2,
                    NULL));

                loc.m_lBegLine = loc.m_lEndLine = rgEdits[iEdit]->m_loc.m_lBegLine +
                                 (m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? 1 : 0);


                TextLineData ld2;
                pIVsTextLines->GetLineDataEx(gldeDefault, loc.m_lBegLine, 0, rgEdits[iEdit]->m_loc.m_lBegColumn, &ld2, NULL);

                if (ld2.GetLength() < rgEdits[iEdit]->m_loc.m_lBegColumn)
                {
                    IDEHelpers::InsertToColumn(&sbBuffer, 0, rgEdits[iEdit]->m_loc.m_lBegColumn, 0, false, 0);
                }

                pIVsTextLines->ReleaseLineDataEx(&ld2);
                loc.m_lBegColumn = loc.m_lEndColumn = 0;

                break;
            }

        case AddAfterBegining:
            {

                    m_InsertionKind = rgEdits[iEdit]->m_InsertionKind;

                // Do the actual insertion... Insert 2 CRLF so things will look pretty
                IfFailGo(pIVsTextLines->ReplaceLinesEx(rtfKeepMarkers | rtfClientSuppressFormatting,
                    rgEdits[iEdit]->m_loc.m_lEndLine + 1, 0,
                    rgEdits[iEdit]->m_loc.m_lEndLine + 1, 0,
                    m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? L"\r\n\r\n" : L"\r\n",
                    m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? 4 : 2,
                    NULL));

                loc.m_lBegLine = loc.m_lEndLine = rgEdits[iEdit]->m_loc.m_lEndLine +
                                 (m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? 2 : 1);

                TextLineData ld2;
                pIVsTextLines->GetLineDataEx(gldeDefault, loc.m_lBegLine, 0, rgEdits[iEdit]->m_loc.m_lBegColumn, &ld2, NULL);

                if (ld2.GetLength() < rgEdits[iEdit]->m_loc.m_lBegColumn)
                {
                    IDEHelpers::InsertToColumn(&sbBuffer, 0, rgEdits[iEdit]->m_loc.m_lBegColumn, 0, false, 0);
                }

                pIVsTextLines->ReleaseLineDataEx(&ld2);

                loc.m_lBegColumn = loc.m_lEndColumn = 0;
                break;
            }

        case AddAfter:
        case AddAtEndOfFile:
            {

                    m_InsertionKind = rgEdits[iEdit]->m_InsertionKind;

                // Do the actual insertion... Insert 2 CRLF so things will look pretty
                IfFailGo(pIVsTextLines->ReplaceLinesEx(rtfKeepMarkers | rtfClientSuppressFormatting,
                    rgEdits[iEdit]->m_loc.m_lEndLine, rgEdits[iEdit]->m_loc.m_lEndColumn + 1,
                    rgEdits[iEdit]->m_loc.m_lEndLine, rgEdits[iEdit]->m_loc.m_lEndColumn + 1,
                    m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? L"\r\n\r\n" : L"\r\n",
                    m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? 4 : 2,
                    NULL));

                loc.m_lBegLine = loc.m_lEndLine = rgEdits[iEdit]->m_loc.m_lEndLine +
                                 (m_InsertionKind == CodeModel_Method && !IsPreviousLineBlank ? 2 : 1);

                TextLineData ld2;
                pIVsTextLines->GetLineDataEx(gldeDefault, loc.m_lBegLine, 0, rgEdits[iEdit]->m_loc.m_lBegColumn, &ld2, NULL);

                if (ld2.GetLength() < rgEdits[iEdit]->m_loc.m_lBegColumn)
                {
                    IDEHelpers::InsertToColumn(&sbBuffer, 0, rgEdits[iEdit]->m_loc.m_lBegColumn, 0, false, 0);
                }

                pIVsTextLines->ReleaseLineDataEx(&ld2);

                loc.m_lBegColumn = loc.m_lEndColumn = 0;
                break;
            }

        case DeleteDeclaration:
            {
                loc.m_lBegLine = rgEdits[iEdit]->m_loc.m_lBegLine;
                loc.m_lBegColumn = rgEdits[iEdit]->m_loc.m_lBegColumn;
                loc.m_lEndLine = rgEdits[iEdit]->m_locEnd.m_lEndLine;
                loc.m_lEndColumn = rgEdits[iEdit]->m_locEnd.m_lEndColumn + 1;

                long cLinesInFile = 0;

                pIVsTextLines->GetLineCount(&cLinesInFile);

                if (cLinesInFile > rgEdits[iEdit]->m_locEnd.m_lEndLine &&
                    loc.m_lEndLine == loc.m_lBegLine)
                {
                    TextLineData ld2;

                    if (SUCCEEDED(pIVsTextLines->GetLineDataEx(gldeDefault, rgEdits[iEdit]->m_locEnd.m_lEndLine, 0, 0, &ld2, NULL)))
                    {
                        // If we are deleting all text on the line, then go ahead and delete the whole line, including any
                        // leading tabs or spaces.
                        if (IsLineBlank(ld2.GetText(), rgEdits[iEdit]->m_loc.m_lBegColumn))
                        {
                            loc.m_lBegColumn = 0;
                        }

                        // If we are deleting the whole line, go ahead and delete the line break too.
                        if (IsLineBreak(ld2.GetText()[loc.m_lEndColumn]) && loc.m_lBegColumn == 0)
                        {
                            loc.m_lEndLine++;
                            loc.m_lEndColumn = 0;
                        }

                        pIVsTextLines->ReleaseLineDataEx(&ld2);
                    }
                }

                break;
            }

        case ReplaceDeclaration:




                loc = rgEdits[iEdit]->m_loc;
                loc.m_lEndColumn++;

                break;

        case ReplaceBody:


            loc = rgEdits[iEdit]->m_loc;
            break;

        default:
            VSFAIL("Bad edit kind.");
        }

        WCHAR *pTextEdit = NULL;
        ULONG cchTextEdit = 0;

        if (EditKind != DeleteDeclaration)
        {
            sbBuffer.AppendString(rgEdits[iEdit]->m_wszText);
            pTextEdit = sbBuffer.GetString();
            cchTextEdit = sbBuffer.GetStringLength();
        }

        IfFailGo(pIVsTextLines->ReplaceLinesEx(rtfKeepMarkers | rtfClientSuppressFormatting,
            loc.m_lBegLine, loc.m_lBegColumn,
            loc.m_lEndLine, loc.m_lEndColumn,
            pTextEdit, cchTextEdit, NULL));
    }

    if (pSourceFileView)
    {
        pSourceFileView->SetIgnoreCommit(IgnoreCommitFlag);
        pSourceFileView->PurgeChangeInput();
    }
    else
    {
        VSFAIL("Decompilation can't happen without a view");
    }

    // After we've inserted the code, we go ahead and get the parsetree for the thingy we just inserted,
    // then we use the location information to generate the ID for that element.
    if (EditKind == AddBeforeEnd ||
        EditKind == AddAfter ||
        EditKind == AddAfterBegining)
    {
        // If there is no SourceFileView, we're not going to return an ID.
        ParseTree::StatementList *pStatementList = NULL;

        VB_ENTRY_NOHR();
        pStatementList = m_pSourceFile->GetParseTreeService()->GetStatementForLocation(&loc);
        VB_EXIT_NORETURN();

        if (SUCCEEDED(hr) && pStatementList && pStatementList->Element && pbstrNewElementID && !(*pbstrNewElementID))
        {
            ParseTree::Statement *pBeginConstruct = pStatementList->Element;

            WCHAR wszId[cbIdSize + 1];
            MakeStatementId(pBeginConstruct, wszId, _countof(wszId));  // Use the CodeFile ID generator service

            WCHAR *entityString = NULL;
            XMLGen::CheckForEntityReferences(wszId, &entityString, NO_METHODS);

            // Convert it to a BSTR and return it.
            *pbstrNewElementID = SysAllocStringLen(entityString, (UINT)wcslen(entityString));

#if DEBUG
            WCHAR* UnEncodedID = NULL;
            DecodePrivateUseCharacters(entityString, ATTRIB_VALUE_Id, &UnEncodedID);
            VSASSERT(!CompareNoCase(wszId, UnEncodedID), "ID encoding/decoding failed!");
            delete[] UnEncodedID;
#endif DEBUG

            delete[] entityString;
        }
    }

Error:
#endif IDE

    return hr;
}
