//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#pragma warning(disable : 4200)

#define cbIdSize 42

class SourceFile;

namespace ILTree
{
	class ILNode;
}
//-------------------------------------------------------------------------------------------------
//
// Enumration of Insertion Type
//
enum CodeInsertionKind
{
    CodeModel_None,
    CodeModel_Unknown,
    CodeModel_Variable,
    CodeModel_Method,
    CodeModel_Part
};

//-------------------------------------------------------------------------------------------------
//
// The different kinds of edits.we support.
//
enum CodeEditKind
{
    AddAtStartOfFile,
    AddAtEndOfFile,
    AddBefore,
    AddBeforeEnd,
    AddAfter,
    AddAfterBegining,
    DeleteDeclaration,
    ReplaceDeclaration,
    ReplaceBody
};

#if ID_TEST && IDE // Dump the XML to a file
// Enumeration of Insertion Type
enum XMLDumpKind {Dump_Decls, Dump_MethodBody, Dump_VBCode, Dump_ReadXML};
#endif

//=============================================================================
// CodeFile
//
// Implements the per-file XML extensibility.
//=============================================================================

// 


class _declspec(uuid("DF0753ED-5F2A-4175-A4A9-8C8664571434"))
CodeFile : public ICodeFile, ZeroInit<CodeFile>
{
public:
    NEW_MUST_ZERO()

    CodeFile(Compiler *pCompiler, SourceFile *pSourceFile);
    ~CodeFile();

    //========================================================================
    // IUnknown
    //========================================================================

    // We don't use ATL for this object because it makes debugging a royal
    // pain in the ----.

    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //========================================================================
    // ICodeFile
    //========================================================================

    //
    // Hook the events.
    //

    // Adds an event listner to the list.  Returns the cookie
    // used to later unadvise the events.
    //
    STDMETHOD(AdviseCodeFileEvents)(ICodeFileEvents *pCodeFileEvents,
        VSCOOKIE *pdwEventCookie);

    // Unhook from the event source.
    STDMETHOD(UnadviseCodeFileEvents)(VSCOOKIE dwEventCookie);

#if ID_TEST && IDE     // Dump the XML to a file
    static void CodeFile::DumpToFile(Compiler *pCompiler, _In_z_ WCHAR *pText, XMLDumpKind DumpKind);
#endif ID_TEST


    //
    // Get file information.
    //

    // Get one single chunk of XML that models all of the statements in
    // the entire file, both declaration and method body statements
    // if applicable.
    //
	STDMETHOD(GetAllStatements)(BSTR *pbstrText)
	{
		VSFAIL("ICodeFile::GetAllStatements is deprecated!");
		return E_NOTIMPL;
	}

    // Fills a text buffer with the XML that describes all of the
    // declaration statements in this file.
    //
    STDMETHOD(GetDeclarationStatements)(BSTR *pbstrText);

    // Fills a text buffer with the XML that describes all of the
    // types defined in this file.  This does not include any of the
    // non-types defined in the file, including namespace constructs.
    // Also, the types should be presented in one flat list with
    // no nesting.
    //
	STDMETHOD(GetTypeDeclarationStatements)(BSTR *pbstrText)
	{
		VSFAIL("ICodeFile::GetTypeDeclarationStatements is deprecated!");
		return E_NOTIMPL;
	}

    // Fills a text buffer with the XML that fully describes a
    // type, including the header for the type, its members and
    // any contained type (and its members).  It does not include
    // method bodies.  Identify the type via its Id which will
    // be invalidated whenever any CodeFileEvent is fired.
    //
	STDMETHOD(GetTypeMemberStatements)(
		LPCOLESTR szId, 
		BSTR *pbstrText)
	{
		VSFAIL("ICodeFile::GetTypeMemberStatements is deprecated!");
		return E_NOTIMPL;
	}

    // Fills a text buffer with the XML that fully describes a
    // type, (Class, Enum, Delegate, Interface, Struct) including the header
    // for the type, its members and any contained type (and its members).
    // It does not include method bodies.  Give the fully-qualified name for the
    // type in the szFullName param.
    //
    STDMETHOD(GetTypeFromFullName)(
		_In_z_ WCHAR *pwszFullName,
        BSTR *pbstrText)
	{
		VSFAIL("ICodeFile::GetTypeFromFullName is deprecated!");
		return E_NOTIMPL;
	}

    // Fill a text buffer with the XML that describes a method body.
    // The identifier of the method is the value of the "methodbodyid"
    // attribute of the method declaration.  This attribute is invalidated
    // whenever any CodeFileEvent is fired.
    //
    STDMETHOD(GetMethodBodyStatements)(LPCOLESTR szId,
        BSTR *pbstrText)
	{
		VSFAIL("ICodeFile::GetMethodBodyStatements is deprecated!");
		return E_NOTIMPL;
	}

    //
    // File modification routines.
    //

    // Starts an edit.  None of these edits will actually be applied until
    // "EndEditTransaction" is called so you don't have to worry about
    // locations changing while you are editing.
    //
    STDMETHOD(StartEditTransaction)();

    STDMETHOD(AddAfterDeclarationStatement)(
        LPCOLESTR szDeclarationId,
        LPCOLESTR szDeclarationXML)
    {
        VSFAIL("ICodeFile::AddAfterDeclarationStatement is deprecated!");
        return E_NOTIMPL;
    }

    STDMETHOD(AddAfterBeginConstruct)(
        LPCOLESTR szDeclarationId,
        LPCOLESTR szDeclarationXML)
    {
        VSFAIL("ICodeFile::AddAfterBeginConstruct is deprecated!");
        return E_NOTIMPL;
    }

    STDMETHOD(AddBeforeEndConstruct)(
        LPCOLESTR szDeclarationId,
        LPCOLESTR szDeclarationXML)
    {
        VSFAIL("ICodeFile::AddBeforeEndConstruct is deprecated!");
        return E_NOTIMPL;
    }

    STDMETHOD(ReplaceDeclarationStatement)(
        LPCOLESTR szDeclarationId,
        LPCOLESTR szDeclarationXML)
    {
        VSFAIL("ICodeFile::ReplaceDeclarationStatement is deprecated!");
        return E_NOTIMPL;
    }

    STDMETHOD(DeleteDeclarationStatement)(LPCOLESTR szDeclarationId)
    {
        VSFAIL("ICodeFile::DeleteDeclarationStatement is deprecated!");
        return E_NOTIMPL;
    }

    STDMETHOD(ReplaceMethodBody)(
        LPCOLESTR szMethodBodyId,
        LPCOLESTR szMethodBodyText)
    {
        VSFAIL("ICodeFile::ReplaceMethodBody is deprecated!");
        return E_NOTIMPL;
    }

    // Commits all of the edits applied to the file since the last
    // call to StartEditTransaction.  This will cause each of the
    // CodeFileEvents to fire at most one time. The ID for newly
    // created element is returned.
    //
    STDMETHOD(EndEditTransaction)(BSTR *pbstrNewElementID);

    // Gets the text location of a statement.  This will return the
    // extent of the entire statement (i.e. from the start of the
    // defining statement of a block to the end of the block end
    // construct).
    //
    STDMETHOD(GetTextLocation)(LPCOLESTR szMethodId,
        LONG *plStartLine,
        LONG *plStartColumn,
        LONG *plEndLine,
        LONG *plEndColumn);

    // Returns true if the language is case sensitive, or false if it is not.
    //
    STDMETHOD(IsLanguageCaseSensitive)(BOOL *pflangCase);

    // Returns true if the given identifier is valid in the language,
    // or false if it is not.
    //
    STDMETHOD(IsValidIdentifier)(_In_z_ LPCOLESTR szIdentifier, BOOL *pfIsValid);

    // Are we ready to produce XML? (Bound state)
    STDMETHOD(IsReadyToProduceStatements)(BOOL *pfIsReadyToProduceStatements);

    STDMETHOD(GetItemID)(VSITEMID *pvsitemid);

    STDMETHOD(GetFileName)(BSTR *pbstrFileName);

    //
    // Implementation.
    //

    // Fires the statement changed events.
    void OnStatementChanged();

    // Fires the declaration changed events.
    void OnDeclarationStatementChanged();

    // Fires the "ready to get XML" event.
    void OnFileInBindable();

    // Fires the zombie event.
    void Fire_OnDropped();

    // Fires the "ready to get XML" event.
    void SetFileDirty() { m_fFiredChangedEvent = true; }

    // Helper that converts a tree to an id.  The passed-in buffers
    // must be at least cbIdSize big.
    //

    // Fires the "ready to get XML" event.
    SourceFile *GetSourceFile() { return m_pSourceFile; }

    // Generates an ID based on the location info from an old parsetree
    static void MakeId
    (
        ILTree::ILNode *ptree,
        _Out_z_cap_(cchId) WCHAR *wszId,
        size_t cchId
    );

    // Generates an ID based on the location info from a new statement parsetree
    static void MakeStatementId
    (
        ParseTree::Statement *pstatement,
        _Out_z_cap_(cchId) WCHAR *wszId,
        size_t cchId
    );

    static void MakeStatementId
    (
        const Location &loc,
        _Out_z_cap_(cchId) WCHAR *wszId,
        size_t cchId
    );


private:
    // Structure to hold the edits as they're applied but before they're committed.
    struct Edit
    {
        // The location of the statement itself.
        Location m_loc;

        // The location of the end construct for the statement.  This will
        // be the same location if this statement is not a block.
        //
        Location m_locEnd;

        // The sequence number of the edit.  This will be a positive number
        // for additions after the current statement, a negative number
        // for additions before the current statement and zero if the
        // current statement is being replaced or deleted.
        //
        // This identifies the order that we need to process the edits associated
        // with any given statement.
        //
        long m_lSequence;

        // The edit being performed.
        CodeEditKind m_EditKind;

        CodeInsertionKind m_InsertionKind; // Used for VB edits.

        // The text of the edit, null terminated.  This member has no meaning
        // if this is a Delete edit.
        //
        WCHAR m_wszText[0];
    };

    // Handles encoding/decoding a number to its base-64 string.
    static
    void EncodeNumber
    (
        _Pre_cap_(cchBuffer) _Pre_invalid_ _Post_z_count_x_(*cchUsed + 1) wchar_t *wszBuffer, 
        size_t cchBuffer,
        _Out_ size_t *cchUsed,
        _In_count_(NumberCount) unsigned u[],
        unsigned NumberCount
    );

    static void DecodeNumber
    (
        _In_count_(cchBuffer) const WCHAR *wszBuffer,
        size_t cchBuffer,
        _Out_cap_(NumberCount) unsigned u[],
        unsigned NumberCount
    );

    public:
        // Unwraps an id into the defining statement location and the block location.
        static void DecodeId
        (
            _In_count_(cchEncodedId) _Pre_z_ const WCHAR *wszId,
            size_t cchEncodedId,
            Location *ploc,
            Location *plocEnd,
            Location *plocExtra = NULL
        );

        // Add a VB edit to the array.
        void AddVBEdit(_In_z_ const WCHAR *wszId, CodeEditKind ek, CodeInsertionKind InsertionKind, _In_z_ const WCHAR *wszText);

    private:
        // Applies the edits in the array to the text buffer.
        static int _cdecl SortEdits(const void *arg1, const void *arg2);
        void InsertionSortEdits();

        // Add an edit to the array.
        void AddEdit(_In_z_ const WCHAR *wszId, CodeEditKind ek, CodeInsertionKind InsertionKind, _In_opt_z_ const WCHAR *wszText, bool IsVbCode = false);

        // Applies the edits in the array to the text buffer.
        HRESULT ApplyEdits(VBTextBuffer* pIVsTextLines, BSTR *pbstrNewElementID);

        // Context
        Compiler *m_pCompiler;

        // Containing file.
        SourceFile *m_pSourceFile;

        // List of event listeners.
        DynamicArray<ICodeFileEvents *> m_daEventListeners;

        // Makes sure that we only fire the codefile events once
        // when committing an edit.
        //
        bool m_fHoldEvents;
        bool m_fFiredStatementChangedEvent;
        bool m_fFiredDeclarationStatementChangedEvent;

        // We only want to fire the "in bindable" event after we've
        // actually fired a changed event.
        //
        bool m_fFiredChangedEvent;

        // Set to true when we fire OnDeclarationStatementChanged or OnStatementChanged events
        //
        bool m_fFiringOnDeclarationEvent;
        bool m_fFiringOnStatementEvent;

        // Did the background thread reach bound state?
        bool m_fIsInBoundState;

        // We also want to be able to disable m_fFiredChangedEvent at will. We do this
        // so that we fire the m_fFiredChangedEvent once at project load time w/o the
        // file actually changing
        //
        bool m_fEnableFiredChangedEvent;

        // Used to control access to the Editing features of the CodeModel
        bool m_fEditing;

        // Used to indicate that CodeFile is gone, and that we can't generate XML.
        bool m_fIsZombie;

        //
        // The rest of the members only "live" while an edit transaction is in process.
        //

        // The memory allocator that contains the list of edits.
        NorlsAllocator m_nraEdits;

        // The array of edits applied in this transaction.  We use a dynamic array
        // instead of a list to make it easier to sort the edits before we
        // apply them.
        //
        DynamicArray<Edit *> m_daEdits;

        // The current "before" sequence number.
        long m_lBeforeSequence;

        // The current "after" sequence number.
        long m_lAfterSequence;

        CComPtr<IVsCompoundAction> m_spUndo;

        // Use the InvisibleEditor to do this if there is no SourceFileView. This InvisibleEditor creates a SourseFileView for us
        // so it can be used until we are done with it.
        CComPtr<IVsInvisibleEditor>         m_srpInvisibleEditor;

        // Santity check for our ref counts.
        ULONG m_ulRefs;

        // Stores the type of code insertion so that we can determine to insert blank lines or not.
        CodeInsertionKind m_InsertionKind;

#if IDE && ID_TEST     // Dump the XML to a file
        // A unique file name for dumping XML.
        static unsigned m_lXMLDumpFileName;
#endif
};
#define IID_CCodeFile __uuidof(CodeFile)

#pragma warning(default : 4200)
