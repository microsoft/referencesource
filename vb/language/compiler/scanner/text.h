//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manages the compiler's view of the text of a module.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class SourceFile;
class Compiler;
struct Location;
class ColorizerState;
class Scanner;
struct Token;

// Wraps the LINEDATAEX structure and provides
// Uses CComBSTR to manage the lifetime of the line text.
typedef struct _TextLineData
{
    friend class VBTextBuffer;

public:
    _TextLineData()
    {
        m_LineData.iLength = 0;
        m_LineData.pszText = NULL;
        m_LineData.iEolType = eolCRLF;
        m_LineData.pAttributes = NULL;
        m_LineData.dwFlags = 0;
        m_LineData.dwReserved = 0;
        m_LineData.pAtomicTextChain = NULL;
    }

    // Accessors
    const WCHAR *GetText() const { return m_LineData.pszText; }
    long GetLength() const { return m_LineData.iLength; }

private:
    LINEDATAEX m_LineData;

    // Keep the text alive - ensures we don't leak GetText()
    CComBSTR m_bstrLineText;

} TextLineData;


//-------------------------------------------------------------------------------------------------
//
// Holds the actual text for a file.  The lifetime of this object controls
// the lifetime of the text.
//
struct Text
{
    NEW_CTOR_SAFE()

    Text();
    virtual ~Text() {} 

    // Prepare to get the text for a file.
    HRESULT Init(SourceFile *pfile);

    // Get the text of the file.  Returns NULL if we were unable
    // to open the file.
    //
    void GetText(__deref_out_ecount_opt(*pcch) const WCHAR **pwsz, _Out_ size_t *pcch);

    // Get the text of the file starting from the specified line (helper)
    void GetText(long iLineStart, __deref_out_ecount_opt(*pcch) const WCHAR **pwsz, _Out_ size_t *pcch);

    // Get the text for a specific range (helper).
    void GetTextOfRange(size_t oStart, size_t oEnd, __deref_out_ecount_opt(*pcch) const WCHAR **pwsz, _Out_ size_t *pcch);

#if DEBUG
    // Dump a region of text.
    void OutSpan(Location *ploc);
#endif

    HRESULT GetTextLineRange(long iLineStart, long iLineEnd, _Out_ BSTR *pbstrTextRange);

#if IDE

    TriState<CComPtr<IVxTextSnapshot>> MaybeGetCachedSnapshot()
    {
        return m_maybeSnapshot;
    }

#endif

protected:

    // The text.
    WCHAR *m_wszText;

    // The size.
    size_t m_cchText;

    // Where to store the text.
    NorlsAllocator m_nraText;

    //
    // Optimization for "OutSpan".
    //

    // Find a line in the buffer.
    const WCHAR *FindLine(long iLine);

    // Cache because I cant stand starting from the top each time.
    const WCHAR *m_wszCurrent;
    long m_iLineCurrent;

#if DEBUG
    bool m_fFileOpen;
#endif

#if IDE

    // If this particular TextEx instance is based around a IVxTextBuffer, this value
    // will store the actual IVxTextSnapshot from which we recieved the text
    TriState<CComPtr<IVxTextSnapshot>> m_maybeSnapshot;

#endif

private:

    // This class is not copy or assignment safe
    Text(const Text&);  
    Text& operator=(const Text&);

};

#if IDE 
struct TextEx : public Text
{
    TextEx() : Text() {}
    TextEx( _In_ IVxTextSnapshot* pSnapshot);
    void EnsureIndexArrayInitialized();
    virtual __override ~TextEx() { Reset(); }
    void InitForSnapshot( _In_ IVxTextSnapshot* pSnapshot );
    void InitForEmpty();
    HRESULT InitForTextOnDisk( SourceFile* pSourceFile );

    HRESULT GetPositionOfLineIndex(size_t iLine,size_t iIndex,size_t * pPos);
    HRESULT GetLineIndexOfPosition(size_t iPos,size_t * pLine, size_t * pIndex);
    HRESULT GetLineLength(size_t iLine, size_t *piLength);

    HRESULT GetTextOfLocation(const Location * ploc, __deref_out_ecount_opt(*pcch) const WCHAR **pwsz,size_t *pcch,bool DoesLocationEncloseText = false);
    void Reset();

    long GetLineCount() 
    { 
        EnsureIndexArrayInitialized();
        return m_IndexArray.Count(); 
    }
    size_t GetSize() { return m_cchText;}

private:
    // Define an init method here with no implementation.  This will prevent the IDE from silently binding 
    // to the Text::Init method and make them choose a declarative init method
    void Init();

    void InitIndexArray();
    bool m_initedIndexArray;
    DynamicArray<size_t> m_IndexArray;  // this is lazily initialized when needed
};


struct CComVxSnapshotPoint : public _VxSnapshotPoint
{
    CComVxSnapshotPoint() 
    {
        snapshot = NULL;
    }
    CComVxSnapshotPoint( const _VxSnapshotPoint& other)
    {
        snapshot = NULL;
        CopyTo(other, __ref (*this));
    }
    ~CComVxSnapshotPoint()
    {
        Release();
    }

    void Release()
    { 
        Release( __ref (*this));
    }

    static void CopyTo( const _VxSnapshotPoint& source, _VxSnapshotPoint& dest )
    {
        Release(dest);
        dest = source;
        if ( dest.snapshot ) 
        {
            dest.snapshot->AddRef();
        }
    }

    static void Release( _VxSnapshotPoint& point )
    {
        if ( point.snapshot ) 
        {
            point.snapshot->Release();
            point.snapshot = NULL;
        }
    }

    CComVxSnapshotPoint& operator=(const _VxSnapshotPoint& other)
    {
        CopyTo(other, __ref (*this));
        return *this;
    }

private:
};


struct CComVxSnapshotSpan : public _VxSnapshotSpan
{
    CComVxSnapshotSpan() 
    {
        start.snapshot = NULL;
        start.position = 0;
        length = 0;
    }
    CComVxSnapshotSpan( 
        _In_ const _VxSnapshotPoint& startPoint,
        _In_ const _VxSnapshotPoint& endPoint)
    {
        start.snapshot = NULL;
        Initialize(startPoint, endPoint);
    }
    CComVxSnapshotSpan( const CComVxSnapshotSpan& other )
    {
        start.snapshot = NULL;
        CopyTo(other, *this);
    }

    ~CComVxSnapshotSpan()
    {
        Release();
    }

    void Initialize(
        _In_ const _VxSnapshotPoint& startPoint,
        _In_ const _VxSnapshotPoint& endPoint)
    {
        ThrowIfNull(startPoint.snapshot);
        Release();
        CComVxSnapshotPoint::CopyTo(startPoint, __ref start);
        length = endPoint.position - startPoint.position;
    }

    _VxSpan GetSpan() const 
    {
        return GetSpan(*this);
    }

    void Release() 
    {
        Release(*this);
    }

    static void Release( _VxSnapshotSpan& span )
    {
        CComVxSnapshotPoint::Release( __ref span.start );
    }

    static void CopyTo( const _VxSnapshotSpan& source, _VxSnapshotSpan& dest )
    {
        Release(dest);
        dest = source;
        if ( dest.start.snapshot )
        {
            dest.start.snapshot->AddRef();
        }
    }

    static _VxSpan GetSpan( const _VxSnapshotSpan& source)
    {
        _VxSpan span;
        span.start = source.start.position;
        span.length = source.length;
        return span;
    }

    CComVxSnapshotSpan& operator=(const CComVxSnapshotSpan& other)
    {
        CopyTo(other, *this);
        return *this;
    }

private:
};

class VxUtil
{
public: 
    static void CopyBstrToBuffer(
            _In_ const CComBSTR& source,
            _In_ WCHAR* pBuffer,
            _In_ size_t bufferLength);
    static void CopyBstrToBuffer(
            _In_ const CComBSTR& source,
            _In_ NorlsAllocator& norls,
            _In_ WCHAR** ppBuffer,
            _In_ size_t* pBufferLength);
    static void GetText( 
            _In_ IVxTextSnapshot* pSnapshot, 
            _In_ NorlsAllocator& norls,
            _Deref_out_ WCHAR** ppText,
            _Deref_out_ size_t* pTextCch);
    static void GetText( 
            _In_ const _VxSnapshotSpan& snapshotSpan,
            _In_ NorlsAllocator& norls,
            _Deref_out_ WCHAR** ppText,
            _Deref_out_ size_t* pTextCch);
    static HRESULT ConvertToPoint(
            _In_ IVxTextSnapshot* pSnapshot,
            _In_ int line,
            _In_ int column,
            _Out_ CComVxSnapshotPoint& point);
    static HRESULT ConvertToSpan(
            _In_ IVxTextSnapshot* pSnapshot,
            _In_ int startLine,
            _In_ int startColumn,
            _In_ int endLine,
            _In_ int endColumn,
            _Out_ CComVxSnapshotSpan& span);
    static HRESULT ConvertToSpan(
            _In_ IVxTextSnapshot* pSnapshot,
            _In_ int startLine,
            _In_ int startColumn,
            _In_ int endLine,
            _Out_ CComVxSnapshotSpan& span);
    static HRESULT GetSpanForLine(
            _In_ IVxTextSnapshot* pSnapshot,
            _In_ int line,
            _Out_ CComVxSnapshotSpan& span);
    static HRESULT CreateTrackingSpan(
            _In_ IVxTextSnapshot* pSnapshot,
            _In_ int startLine,
            _In_ int startColumn,
            _In_ int endLine,
            _In_ int endColumn,
            _In_ _VxSpanTrackingMode trackingMode, 
            _Out_ CComPtr<IVxTrackingSpan>& span);
    static HRESULT ConvertToTextSpan( 
            _In_ const _VxSnapshotSpan& vss, 
            _Out_ TextSpan& textSpan );
    static HRESULT ConvertToLocation(
            _In_ const _VxSnapshotSpan& vss, 
            _Out_ Location& loc);
    static CComPtr<IVxTextSnapshot> GetCurrentSnapshot( _In_ IVxTextBuffer* pBuffer );

    static bool TryConvertVsTextLinesToVxTextBuffer( 
            _In_ IVsTextLines* pLines, 
            _Out_ CComPtr<IVxTextBuffer>& spRet );
    static int GetVersionNumber( _In_ IVxTextSnapshot* pSnapshot );
    static int GetVersionNumber( _In_ IVxTextVersion* pVersion );
    static bool IsInert( _In_ IVxTextBuffer* pBuffer );
    static TextSpan ConvertToTextSpan( _In_ const TextSpan2& span );
    static TextSpan2 ConvertToTextSpan2( _In_ const TextSpan& span );
};

//-------------------------------------------------------------------------------------------------
// VBTextBuffer
// Wraps either an IVsTextLines or an IVxTextBuffer instance.
// Methods exposed match the IVsTextLines interface in order to reduce the 
// amount of code churn. 
class VBTextBuffer :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IUnknown
{
protected:
    VBTextBuffer() 
    {
    }

    ~VBTextBuffer()
    {
    }

public:
    BEGIN_COM_MAP(VBTextBuffer)
        COM_INTERFACE_ENTRY(IUnknown)
    END_COM_MAP()

    // Lock the buffer to a particular version of the text.
    // Must be set to NULL in order to get the live text
    void CacheSnapshot(IVxTextSnapshot *pTextSnapshot) 
    { 
        m_pTextSnapshot = pTextSnapshot; 
    }

    IVxTextSnapshot *GetCachedSnapshot()  
    { 
        return m_pTextSnapshot;   
    }

    CComPtr<IVxTextSnapshot> GetCachedOrCurrentSnapshot() 
    { 
        CComPtr<IVxTextSnapshot> pSnapshot = m_pTextSnapshot;
        
        if (!pSnapshot && m_pTextBuffer)
        {
            m_pTextBuffer->GetCurrentSnapshot(&pSnapshot);
        }

        return pSnapshot ; 
    }

    // Non hostable editor features still need direct access to the IVsTextLines
    IVsTextLines *GetIVsTextLines() 
    { 
        return m_pTextLines; 
    }

    // Begin IVsTextLine methods
    STDMETHOD(GetLineText)( 
        /* [in] */ long iStartLine,
        /* [in] */ CharIndex iStartIndex,
        /* [in] */ long iEndLine,
        /* [in] */ CharIndex iEndIndex,
        /* [out] */ __RPC__deref_out_opt BSTR *pbstrBuf);

    STDMETHOD(GetLineText)(
        /* [in] */ long lLine,
        /* [out] */ __RPC__deref_out BSTR *pbstrText);

    STDMETHOD(CopyLineText)( 
        /* [in] */ long iStartLine,
        /* [in] */ CharIndex iStartIndex,
        /* [in] */ long iEndLine,
        /* [in] */ CharIndex iEndIndex,
        /* [in] */ __RPC__in LPWSTR pszBuf,
        /* [out][in] */ __RPC__inout long *pcchBuf);

    STDMETHOD(ReplaceLines)( 
        /* [in] */ long iStartLine,
        /* [in] */ CharIndex iStartIndex,
        /* [in] */ long iEndLine,
        /* [in] */ CharIndex iEndIndex,
        /* [in] */ __RPC__in LPCWSTR pszText,
        /* [in] */ long iNewLen,
        /* [out] */ __RPC__out TextSpan *pChangedSpan);

    STDMETHOD(CreateLineMarker)( 
        /* [in] */ long iMarkerType,
        /* [in] */ long iStartLine,
        /* [in] */ CharIndex iStartIndex,
        /* [in] */ long iEndLine,
        /* [in] */ CharIndex iEndIndex,
        /* [in] */ __RPC__in_opt IVsTextMarkerClient *pClient,
        /* [out] */ __RPC__deref_out_opt IVsTextLineMarker **ppMarker);

    STDMETHOD(EnumMarkers)( 
        /* [in] */ long iStartLine,
        /* [in] */ CharIndex iStartIndex,
        /* [in] */ long iEndLine,
        /* [in] */ CharIndex iEndIndex,
        /* [in] */ long iMarkerType,
        /* [in] */ DWORD dwFlags,
        /* [out] */ __RPC__deref_out_opt IVsEnumLineMarkers **ppEnum);

    STDMETHOD(GetPairExtents)( 
        /* [in] */ __RPC__in const TextSpan *pSpanIn,
        /* [out] */ __RPC__out TextSpan *pSpanOut);

    STDMETHOD(GetLineDataEx)( 
        /* [in] */ DWORD dwFlags,
        /* [in] */ long iLine,
        /* [in] */ long iStartIndex,
        /* [in] */ long iEndIndex,
        /* [out] */ __RPC__out TextLineData *pTextLineData,
        /* [in] */ __RPC__in MARKERDATA *pMarkerData);

    STDMETHOD(ReleaseLineDataEx)( 
        /* [in] */ __RPC__in TextLineData *pLineData);

    STDMETHOD(ReplaceLinesEx)( 
        /* [in] */ DWORD dwFlags,
        /* [in] */ long iStartLine,
        /* [in] */ CharIndex iStartIndex,
        /* [in] */ long iEndLine,
        /* [in] */ CharIndex iEndIndex,
        /* [in] */ __RPC__in LPCWSTR pszText,
        /* [in] */ long iNewLen,
        /* [out] */ __RPC__out TextSpan *pChangedSpan);

    STDMETHOD(CreateTextPoint)( 
        /* [in] */ long iLine,
        /* [in] */ CharIndex iIndex,
        /* [out] */ __RPC__deref_out_opt IDispatch **ppTextPoint);

    STDMETHOD(GetLengthOfLine)( 
        /* [in] */ long iLine,
        /* [out] */ __RPC__out long *piLength);
    
    STDMETHOD(GetLineCount)( 
        /* [out] */ __RPC__out long *piLineCount);

    STDMETHOD (GetSize)( 
        /* [out] */ __RPC__out long *piLength);

    STDMETHOD(GetPositionOfLineIndex)(
        long Line,
        long Index,
        long *pPosition);

    STDMETHOD(GetLineIndexOfPosition)(
        long Position,
        long *pLine,      
        long *pIndex);
// End IVsTextLine methods

    bool IsTextSpanReadOnly(TextSpan tsCheck);

    bool IsLineBlank(long lLine, ULONG charIndexToEndAt = ULONG_MAX);

    bool IsXMLDocCommentLine(long lLine);

    HRESULT GetColorStateAtStartOfLine(
        _In_ long lineNumber, 
        _Deref_out_ long * pState);

    HRESULT InvalidateColorState(_In_opt_ ChangeInput * pChangeInput = NULL, _In_opt_ TextSpan *pSpanOfReplace = NULL);

    CComPtr<IVxTextBuffer> GetVxTextBuffer();
    void ReloadVxTextBuffer();

    static 
    CComPtr<VBTextBuffer> Create( _In_ IVsTextLines* pLines );
    static 
    CComPtr<VBTextBuffer> Create( 
        _In_ IVxTextBuffer* pTextBuffer,
        _In_ IManagedEditorService* pService);

    HRESULT CreateTrackingSpan(
        _In_ const TextSpan *pSpan, 
        _Deref_out_ CComPtr<IVxTrackingSpan> & spTrackingSpan,
        _In_opt_ _VxSpanTrackingMode SpanTrackingMode= VxSpanTrackingModeEdgeInclusive
        );

    HRESULT CreateSelectionTracking(
        _In_ const TextSpan& span, 
        _Deref_out_ CComPtr<ISelectionTracking>& spSelectionTracking);

    HRESULT VBTextBuffer::TrackingSpanToTextSpan
    (
        _In_ CComPtr<IVxTrackingSpan> & spTrackingSpan, 
        _Deref_out_ TextSpan &textSpan
    );
    HRESULT VBTextBuffer::TrackingSpanToTextSpanForCachedSnapshot
    (
        _In_ CComPtr<IVxTrackingSpan> & spTrackingSpan, 
        _Deref_out_ TextSpan &textSpan
    );
    
    static
    HRESULT VBTextBuffer::ConvertSpan( _In_ const _VxSnapshotSpan& vss, _Out_ TextSpan& textSpan );

    void UpdateParsedSnapshot();
private:

    HRESULT VBTextBuffer::TrackingSpanToTextSpanForSnapshot
    (
        _In_ CComPtr<IVxTrackingSpan> & spTrackingSpan, 
        _Deref_out_ TextSpan &textSpan,
        _In_ CComPtr<IVxTextSnapshot> pTextSnapshot
    );
    void UpdateShimBuffer();

    CComPtr<IManagedEditorService> m_spEditorService;
    CComPtr<IVsTextLines> m_pTextLines;
    CComPtr<IVxTextBuffer> m_pTextBuffer;
    CComPtr<IVxTextSnapshot> m_pTextSnapshot;
    CComPtr<IShimBuffer> m_spShimBuffer;
    BufferColorStateManager m_bufferColorStates;
};

#endif IDE
