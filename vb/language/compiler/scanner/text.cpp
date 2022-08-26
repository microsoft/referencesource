//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manages the compiler's view of a text module.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

Text::Text() :
    m_nraText(NORLSLOC)
{
    m_wszText = NULL;
    m_cchText = 0;
    m_wszCurrent = NULL;
    m_iLineCurrent = 0;

#if DEBUG
    m_fFileOpen = false;
#endif
}

//-------------------------------------------------------------------------------------------------
//
// Create a large buffer that holds all of the text of the file.
//
HRESULT Text::Init(SourceFile *pfile)
{
    HRESULT hr = NOERROR;
    // deallocate old text if it exists
    if (m_wszText != NULL)
    {
        m_nraText.FreeHeap();
        m_wszText = NULL;
        m_cchText = 0;
#if IDE
        m_maybeSnapshot.ClearValue();
#endif
    }

    // if it's a readony My or XML template
    SolutionExtensionData * pSolutionExtensionData = pfile->GetSolutionExtension();
    if (pSolutionExtensionData &&
        pSolutionExtensionData->m_extension != SolutionExtension::SolutionExtensionNone)
    {
        m_wszText = pSolutionExtensionData->m_psbCode->GetString();
        m_cchText = pSolutionExtensionData->m_psbCode->GetStringLength();
    }
    else
    {
#if IDE
        if (!pfile->TryGetCachedSourceFileText(&m_nraText, &m_wszText, &m_cchText))
        {
            IfFailGo(pfile->GetTextFile()->GetFileText(&m_nraText,
                                                       &m_wszText,
                                                       &m_cchText,
                                                       __ref m_maybeSnapshot));
        }
#else
        IfFailGo(pfile->GetTextFile()->GetFileText(&m_nraText,
                                                   &m_wszText,
                                                   &m_cchText));
#endif

    }
#pragma prefast(push)
#pragma prefast(disable: 26010, "Prefast is way out of line here... these two guys are not buffers")
    m_iLineCurrent = 0;
    m_wszCurrent = NULL;
#pragma prefast(pop)

#if DEBUG && IDE
    m_fFileOpen = pfile->HasSourceFileView();
#endif

Error:
    return hr;
}


//-------------------------------------------------------------------------------------------------
//
// Get the text in this buffer.
//
void Text::GetText(
    __deref_out_ecount_opt(*pcch) const WCHAR **pwsz,
    _Out_ size_t *pcch)
{
    *pwsz = m_wszText;
    *pcch = m_cchText;
}

//-------------------------------------------------------------------------------------------------
//
// Get the text starting from the specified line
//
void Text::GetText(
    long iLineStart,
    __deref_out_ecount_opt(*pcch) const WCHAR **pwsz,
    _Out_ size_t *pcch)
{
    const WCHAR *wszStart = FindLine(iLineStart);

    // It's OK for wszStart to be NULL. This will happen when the user deletes
    // the source file while it is open in the editor (we don't lock any of the
    // files in the project, so this is very possible).
    if (wszStart)
    {
        size_t length = (size_t)(m_wszText + m_cchText - wszStart);
        *pwsz = wszStart;
        *pcch = length;
    }
    else
    {
        *pwsz =  NULL;
        *pcch = 0;
    }
}

//-------------------------------------------------------------------------------------------------
//
// Get the text for a specific range (helper).
//
void Text::GetTextOfRange(
    size_t oStart,
    size_t oEnd,
    __deref_out_ecount_opt(*pcch) const WCHAR **pwsz,
    _Out_ size_t *pcch)
{
    // If m_wszText is not set, then we failed to open the file and
    // already reported an error.  Return NULL to signify this.
    //
    if (!m_wszText || m_cchText == 0)
    {
        *pwsz = NULL;
        *pcch = 0;
    }
    else
    {

#if DEBUG
        // VSW#461074
        //
        // When the source file is closed, the file on disk may change, and the background thread
        // may send down cached text spans before the IDE receives the file change notification.
        // The file change notification will decompile (fixing symbol state, etc.)
        if (m_fFileOpen)
        {
            VSASSERT(oStart <= m_cchText, "Start value overflow.");
            // comment out the line below???
            VSASSERT((oStart < m_cchText) || (oStart == oEnd), "Start value overflow.");
            VSASSERT(oEnd <= m_cchText, "End value overflow.");
            VSASSERT(oEnd >= oStart, "End value comes before start.");
        }
#endif

        // Return nothing an invalid oStart
        if (oStart > m_cchText || oStart > oEnd)
        {
            *pwsz = NULL;
            *pcch = 0;
        }
        else
        {
            *pwsz = m_wszText + oStart;
            *pcch = ((oEnd<=m_cchText)?oEnd:m_cchText) - oStart;
        }
    }
}

HRESULT Text::GetTextLineRange
(
      long iLineStart,
      long iLineEnd,
      _Out_ BSTR *pbstrTextRange
)
{
    const WCHAR *wszStart, *wsz;
    wszStart = wsz = FindLine(iLineStart);

    // It's OK for wszStart to be NULL. This will happen when the user deletes
    // the source file while it is open in the editor (we don't lock any of the
    // files in the project, so this is very possible).
    if (wszStart)
    {
        for (long iLine = iLineStart; iLine <= iLineEnd; iLine++)
        {
            while((size_t)(wsz - m_wszText) < m_cchText && *wsz)
            {
                if (IsLineBreak(*wsz))
                {
                    wsz = LineBreakAdvance(wsz);
                    break;
                }
                else
                {
                    wsz++;
                }
            }
        }

        *pbstrTextRange =  SysAllocStringLen( wszStart, (UINT)(wsz - wszStart) );
        return (*pbstrTextRange == NULL) ? E_OUTOFMEMORY : S_OK;
    }
    else
    {
        *pbstrTextRange =  NULL;
        return S_FALSE;
    }
}

#if DEBUG

//============================================================================
// Dump a region of text, used for error display.
//============================================================================

void Text::OutSpan
(
    Location *ploc
)
{
    if (ploc->IsInvalid())
    {
        VSFAIL("Invalid location");
        DebPrintf("Invalid location\n");
        return;
    }

    //
    // Print out the line(s)
    //

    {
        long iLineStart = ploc->m_lBegLine;
        long iColumnStart = ploc->m_lBegColumn;
        long iLineEnd = ploc->m_lEndLine;
        long iColumnEnd = ploc->m_lEndColumn;

        const WCHAR *wsz, *wszLine;

        long iLine;

        for (iLine = iLineStart; iLine <= iLineEnd; iLine++)
        {
            wszLine = wsz = FindLine(iLine);

            if (wszLine)
            {
                while (*wsz)
                {
                    // Don't print out the last \n.
                    if (IsLineBreak(*wsz) && iLine == iLineEnd)
                    {
                        break;
                    }

                    if (!(iLine == iLineStart && (wsz - wszLine) < iColumnStart
                      || iLine == iLineEnd && (wsz - wszLine) > iColumnEnd))
                    {
                        if (!IsLineBreak(*wsz))
                        {
                            DebPrintf("%C", *wsz);
                        }
                        else
                        {
                            DebPrintf("%C", '\n');
                        }
                    }

                    if (IsLineBreak(*wsz))
                    {
                        break;
                    }

                    wsz++;
                }
            }
        }
    }
}

#endif

//============================================================================
// Find a line in the buffer.
//============================================================================

const WCHAR *Text::FindLine
(
    long iLineFind
)
{
    const WCHAR *wszLine, *wszEnd;
    long iLine;

    // Find where to start from.
    if (m_iLineCurrent <= iLineFind && m_wszCurrent)
    {
        iLine = m_iLineCurrent;
        wszLine = m_wszCurrent;
    }
    else
    {
        iLine = 0;
        wszLine = m_wszText;
    }

    if (!wszLine)
    {
        return NULL;
    }

    // Get the last thing in the block.
    wszEnd = m_wszText + m_cchText;

    // Find the line.
    while (iLine < iLineFind)
    {
        // Get the end of the current line.
        while (wszLine != NULL && *wszLine)
        {
            if (IsLineBreak(*wszLine))
            {
                wszLine = LineBreakAdvance(wszLine);
                break;
            }
            else
            {
                wszLine++;
                VSASSERT(wszLine != wszEnd, "Bad line number.");
            }
        }

        iLine++;
    }

    // Stash it.
    m_wszCurrent = wszLine;
    m_iLineCurrent = iLineFind;

    // Return it.
    return wszLine;
}

#if IDE 

HRESULT 
TextEx::InitForTextOnDisk( SourceFile* pSourceFile )
{
    VerifyInPtr(pSourceFile);
    Reset();

    HRESULT hr = S_OK;
    IfFailGo( Text::Init(pSourceFile) );

Error:
    return hr;
}

TextEx::TextEx( _In_ IVxTextSnapshot* pSnapshot )
{
    ThrowIfNull(pSnapshot);
    InitForSnapshot(pSnapshot);
}

void
TextEx::InitForSnapshot( _In_ IVxTextSnapshot* pSnapshot )
{
    ThrowIfNull(pSnapshot);
    Reset();

    VxUtil::GetText(pSnapshot, m_nraText, &m_wszText, &m_cchText);
    m_cchText--;    // GetText return NULL terminator, m_cchText is not meant to count it
    m_maybeSnapshot.SetValue(pSnapshot);
}

//-------------------------------------------------------------------------------------------------
//
// Create a complete empty TextEx structure.  Essentially a 0 length file
//
//-------------------------------------------------------------------------------------------------
void
TextEx::InitForEmpty()
{
    Reset();
    m_cchText = 0;
    m_wszText = reinterpret_cast<WCHAR*>(m_nraText.Alloc(sizeof(WCHAR)));
    m_wszText[0] = L'\0';
}

//Transfer line/column to position
HRESULT TextEx::GetPositionOfLineIndex(size_t iLine,size_t iIndex,size_t* pPos)
{
    EnsureIndexArrayInitialized();
    HRESULT hr = NOERROR;
    IfFalseGo(pPos,E_INVALIDARG);
    IfFalseGo(iLine >= 0 && (ULONG)iLine < m_IndexArray.Count(),E_FAIL);
    UINT iLineNum = VBMath::Convert<ULONG>(iLine);

    if (m_IndexArray.Count() > (ULONG)(iLineNum + 1))
    {
        IfFalseGo(iIndex <= (m_IndexArray.Element(iLineNum+1) - m_IndexArray.Element(iLineNum)),E_FAIL);
    }
    else
    {
        IfFalseGo(iIndex <= ((size_t)m_cchText - m_IndexArray.Element(iLineNum)),E_FAIL);
    }
    *pPos = m_IndexArray.Element(iLineNum) + iIndex;
Error:
    return hr;
}

//Transfer position to line/column
HRESULT TextEx::GetLineIndexOfPosition(size_t iPos, size_t* pLine, size_t* pIndex)
{
    EnsureIndexArrayInitialized();
    HRESULT hr = NOERROR;
    bool isSet = false;
    ULONG uCount = m_IndexArray.Count();
    size_t low,high,mid;
    IfFalseGo(pLine && pIndex,E_INVALIDARG);
    IfFalseGo(iPos >= 0 && iPos < (size_t)m_cchText,E_FAIL);
    low = 0;
    high = uCount;
    mid = 0;
    while (low <= high)
    {
        mid = (low + high)/2;
        size_t iStart = m_IndexArray.Element(VBMath::Convert<UINT>(mid));
        size_t iEnd = 0;
        if (uCount == (mid+1))
        {
            iEnd = (size_t)m_cchText;
        }
        else
        {
            iEnd = m_IndexArray.Element(VBMath::Convert<UINT>(mid+1));
        }
        if (iPos >= iStart && iPos < iEnd)
        {
            *pLine = mid;
            *pIndex = iPos - iStart;
            isSet = true;
            break;
        }
        else if (iStart > iPos)
        {
            high = mid - 1;
        }
        else
        {
            low = mid + 1;
        }
    }
    if (hr == NOERROR)
    {
        VSASSERT(isSet,"Wrong position!");
        IfFalseGo(isSet,E_FAIL);
    }
Error:
    return hr;
}

HRESULT TextEx::GetLineLength(size_t iLine, size_t* piLength)
{
    EnsureIndexArrayInitialized();

    HRESULT hr = S_OK;

    if (iLine < 0 || (ULONG)iLine >= m_IndexArray.Count() || !piLength)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        UINT iLineNum = VBMath::Convert<ULONG>(iLine);
        if (m_IndexArray.Count() > (ULONG)(iLineNum + 1))
        {
            *piLength = m_IndexArray.Element(iLineNum + 1) - m_IndexArray.Element(iLineNum);
        }
        else
        {
            *piLength = (size_t) m_cchText - m_IndexArray.Element(iLineNum);
        }
    }

    return hr;
}

//Build index array to speed up getting code text by given line/column
//or position
void TextEx::InitIndexArray()
{
    if (m_wszText && m_cchText)
    {
        VSASSERT(!m_initedIndexArray, "already initialized");
        m_initedIndexArray = true;
        const WCHAR * wszText = m_wszText;
        size_t iIndex = 0;
        m_IndexArray.Add() = iIndex;

        while (iIndex < m_cchText)
        {
            if (IsLineBreak(*wszText))
            {
                const WCHAR * wsz = LineBreakAdvance(wszText);
                iIndex += (size_t)(wsz - wszText);
                m_IndexArray.Add() = iIndex;
                wszText = wsz;
            }
            else
            {
                wszText++;
                iIndex++;
            }
        }
    }
}

//Get code text based on our location.
//
// DoesLocationEncloseText = false
// "Foo" has a location of (0,0)-(0,2) which needs to be mapped to (0,0)-(0,3) to properly get the text
//
// DoesLocationEncloseText = true
// "Foo" has a location of (0,0)-(0,3) - no adjustment is needed
HRESULT TextEx::GetTextOfLocation(const Location * ploc, __deref_out_ecount_opt(*pcch) const WCHAR **pwsz,size_t *pcch,bool DoesLocationEncloseText)
{
    VerifyInPtr(ploc);
    HRESULT hr = S_OK;

    // Guard against the compiler giving us negative indexes.  Will cause the size_t conversion to fail
    if ( ploc->IsAnyPartNegative() )
    {
        return E_FAIL;
    }

    size_t oStart = 0;
    size_t oEnd = 0;
    size_t iSLine = VBMath::Convert<size_t>(ploc->m_lBegLine);
    size_t iSIndex = VBMath::Convert<size_t>(ploc->m_lBegColumn);
    size_t iELine = VBMath::Convert<size_t>(ploc->m_lEndLine);
    size_t iEIndex = VBMath::Convert<size_t>(ploc->m_lEndColumn);
 
    if (!DoesLocationEncloseText)
    {
        iEIndex++;
    }

    IfFailGo(GetPositionOfLineIndex(iSLine,iSIndex,&oStart));
    IfFailGo(GetPositionOfLineIndex(iELine,iEIndex,&oEnd));
    GetTextOfRange(oStart,oEnd,pwsz,pcch);
Error:
    return hr;
}

//Clean up
void TextEx::Reset()
{
    m_wszText = NULL;
    m_cchText = 0;
    m_wszCurrent = NULL;
    m_iLineCurrent = 0;
    m_IndexArray.Reset();
    m_initedIndexArray = false;
    m_nraText.FreeHeap();
	m_maybeSnapshot.ClearValue();
}

void
TextEx::EnsureIndexArrayInitialized()
{
    if (m_initedIndexArray)
    {
        return;
    }

    if (IsLineBlank(m_wszText,m_cchText))
    {
        m_initedIndexArray = true;  // Text is immutable so this won't change, don't check again 
        return;
    }

    InitIndexArray();
}

bool VxUtil::TryConvertVsTextLinesToVxTextBuffer( _In_ IVsTextLines* pLines, _Out_ CComPtr<IVxTextBuffer>& spRet )
{
    HRESULT hr = S_OK;
    CComPtr<IVsUserData> spData;
    CComVariant variant;
    CComPtr<IUnknown> spUnk;
    GUID id = {0xbe120c41,0xd969,0x42a4,{0xa4,0xdd,0x91,0x26,0x65,0xa5,0xbf,0x13}};

    if ( SUCCEEDED( ComUtil::QI(pLines,&spData) ) 
        && SUCCEEDED( spData->GetData(id, &variant) )
        && VT_UNKNOWN == variant.vt
        && SUCCEEDED( ComUtil::QI(variant.punkVal, &spRet) ) )
    {
        return true;
    }
    
    return false;
}

//-------------------------------------------------------------------------------------------------
//
// Copy the contents of the BSTR to the buffer in question.  Have to handle the case where a 
// BSTR is NULL
//
//-------------------------------------------------------------------------------------------------
void 
VxUtil::CopyBstrToBuffer(
    _In_ const CComBSTR& source,
    _In_ WCHAR* pBuffer,
    _In_ size_t bufferLength)
{
    ThrowIfNull(pBuffer);

    if ( 0 == source.Length() )
    {
        IfFailThrow( StringCchCopy(pBuffer, bufferLength, L"") );
    }
    else
    {
        IfFailThrow( StringCchCopy(pBuffer, bufferLength, source) );
    }
}

void 
VxUtil::CopyBstrToBuffer(
    _In_ const CComBSTR& source,
    _In_ NorlsAllocator& norls,
    _In_ WCHAR** ppBuffer,
    _In_ size_t* pBufferLength)
{
    SafeInt<size_t> length = source.Length();
    length++;   // Null terminator

    *ppBuffer = norls.AllocArray<WCHAR>(length.Value());
    *pBufferLength = length.Value();
    VxUtil::CopyBstrToBuffer(source, *ppBuffer, *pBufferLength);
}

//-------------------------------------------------------------------------------------------------
//
// Get all of the text from the IVxTextSnapshot
//
// pTextCch will point to the character count INCLUDING the null terminator
//-------------------------------------------------------------------------------------------------
void
VxUtil::GetText(
    _In_ IVxTextSnapshot* pSnapshot, 
    _In_ NorlsAllocator& norls,
    _Deref_out_ WCHAR** ppText,
    _Deref_out_ size_t* pTextCch)
{
    ThrowIfNull(pSnapshot);
    ThrowIfNull(ppText);
    ThrowIfNull(pTextCch);

    CComBSTR text;
    IfFailThrow( pSnapshot->GetText_3(&text) );
    VxUtil::CopyBstrToBuffer(text, norls, ppText, pTextCch);
}

void 
VxUtil::GetText( 
    _In_ const _VxSnapshotSpan& snapshotSpan,
    _In_ NorlsAllocator& norls,
    _Deref_out_ WCHAR** ppText,
    _Deref_out_ size_t* pTextCch)
{
    CComBSTR text;
    _VxSpan span = CComVxSnapshotSpan::GetSpan(snapshotSpan);
    IfFailThrow( snapshotSpan.start.snapshot->GetText(span, &text) );
    VxUtil::CopyBstrToBuffer(text, norls, ppText, pTextCch);
}

HRESULT 
VxUtil::ConvertToPoint(
    _In_ IVxTextSnapshot* pSnapshot,
    _In_ int line,
    _In_ int column,
    _Out_ CComVxSnapshotPoint& point)
{
    VerifyInPtr(pSnapshot);
    HRESULT hr = S_OK;
    CComPtr<IVxTextSnapshotLine> spLine;
    int length;

    IfFailGo( pSnapshot->GetLineFromLineNumber(line, &spLine) );
    IfFailGo( spLine->GetStart(&point) );

    if ( column > 0 )
    {
        // Make sure that we have a valid column
        IfFailGo( spLine->GetLength(&length) );
        IfTrueGo( column > length, E_INVALIDARG );
        point.position += column;
    }

Error:
    return hr;
}

//-------------------------------------------------------------------------------------------------
//
// Convert the line column span to an actual span
//
//-------------------------------------------------------------------------------------------------
HRESULT 
VxUtil::ConvertToSpan(
    _In_ IVxTextSnapshot* pSnapshot,
    _In_ int startLine,
    _In_ int startColumn,
    _In_ int endLine,
    _In_ int endColumn,
    _Out_ CComVxSnapshotSpan& span)
{
    VerifyInPtr(pSnapshot);
    HRESULT hr = S_OK;

    CComVxSnapshotPoint startPoint, endPoint;
    IfFailGo( ConvertToPoint(pSnapshot, startLine, startColumn, __ref startPoint) );
    IfFailGo( ConvertToPoint(pSnapshot, endLine, endColumn, __ref endPoint) );
    
    span.Initialize(startPoint, endPoint);
Error:
    return hr;
}

HRESULT 
VxUtil::ConvertToSpan(
    _In_ IVxTextSnapshot* pSnapshot,
    _In_ int startLine,
    _In_ int startColumn,
    _In_ int endLine,
    _Out_ CComVxSnapshotSpan& span)
{
    VerifyInPtr(pSnapshot);
    HRESULT hr = S_OK;
    CComVxSnapshotPoint startPoint, endPoint;

    CComPtr<IVxTextSnapshotLine> spLine;
    IfFailGo( pSnapshot->GetLineFromLineNumber(endLine, &spLine) );
    IfFailGo( spLine->GetEnd(&endPoint) );
    IfFailGo( ConvertToPoint(pSnapshot, startLine, startColumn, __ref startPoint) );
    span.Initialize(startPoint, endPoint);

Error:
    return hr;
}

HRESULT 
VxUtil::GetSpanForLine(
    _In_ IVxTextSnapshot* pSnapshot,
    _In_ int line,
    _Out_ CComVxSnapshotSpan& span)
{
    ThrowIfNull(pSnapshot);

    HRESULT hr = S_OK;
    CComVxSnapshotPoint startPoint, endPoint;
    CComPtr<IVxTextSnapshotLine> spLine;

    IfFailGo( pSnapshot->GetLineFromLineNumber(line, &spLine) );
    IfFailGo( spLine->GetStart(&startPoint) );
    IfFailGo( spLine->GetEnd(&endPoint) );
    span.Initialize(startPoint, endPoint);

Error:
    return hr;
}

HRESULT 
VxUtil::CreateTrackingSpan(
    _In_ IVxTextSnapshot* pSnapshot,
    _In_ int startLine,
    _In_ int startColumn,
    _In_ int endLine,
    _In_ int endColumn,
    _In_ _VxSpanTrackingMode trackingMode, 
    _Out_ CComPtr<IVxTrackingSpan>& trackingSpan)
{
    ThrowIfNull(pSnapshot); 
    HRESULT hr = S_OK;

    CComVxSnapshotSpan span;
    IfFailGo( ConvertToSpan(pSnapshot, startLine, startColumn, endLine, endColumn, __ref span) );
    IfFailGo( pSnapshot->CreateTrackingSpan(span.GetSpan(), trackingMode, &trackingSpan) );

Error:
    return hr;
}

HRESULT 
VxUtil::ConvertToTextSpan( 
    _In_ const _VxSnapshotSpan& vss, 
    _Out_ TextSpan& textSpan )
{
    HRESULT hr = S_OK; 

    CComPtr<IVxTextSnapshotLine> spLine;
    CComVxSnapshotPoint startPoint;
    int lineNumber;
    int lineNumberEnd;
    IfFailRet( vss.start.snapshot->GetLineFromPosition(vss.start.position, &spLine) );
    IfFailRet( spLine->GetLineNumber(&lineNumber) );
    IfFailRet( spLine->GetStart(&startPoint) );
    textSpan.iStartLine = lineNumber;
    textSpan.iStartIndex = vss.start.position - startPoint.position;

    spLine.Release();
    IfFailRet( vss.start.snapshot->GetLineFromPosition(vss.start.position+vss.length, &spLine) );
    IfFailRet( spLine->GetLineNumber(&lineNumberEnd) );
    if (lineNumber != lineNumberEnd)    // small optimization if span on same line #
    {
        startPoint.Release();
        IfFailRet( spLine->GetStart(&startPoint) );
    }
    textSpan.iEndLine = lineNumberEnd;
    textSpan.iEndIndex = (vss.start.position+vss.length)-(startPoint.position);
    return S_OK;
}

HRESULT 
VxUtil::ConvertToLocation(
    _In_ const _VxSnapshotSpan& vss, 
    _Out_ Location& loc)
{
    TextSpan span;
    HRESULT hr = ConvertToTextSpan(vss, __ref span);
    if ( FAILED(hr) )
    {
        return hr;
    }

    loc = Location::Create(span);
    return S_OK;
}

CComPtr<IVxTextSnapshot> 
VxUtil::GetCurrentSnapshot( _In_ IVxTextBuffer* pBuffer )
{
    ThrowIfNull(pBuffer);
    CComPtr<IVxTextSnapshot> spSnapshot; 
    IfFailThrow( pBuffer->GetCurrentSnapshot(&spSnapshot) );
    return spSnapshot;
}


int
VxUtil::GetVersionNumber( _In_ IVxTextSnapshot* pSnapshot )
{
    ThrowIfNull(pSnapshot);
    CComPtr<IVxTextVersion> spVersion;
    IfFailThrow( pSnapshot->GetVersion(&spVersion) );
    return VxUtil::GetVersionNumber(spVersion);
}

int
VxUtil::GetVersionNumber( _In_ IVxTextVersion* pVersion )
{
    ThrowIfNull(pVersion);
    int number;
    IfFailThrow( pVersion->GetVersionNumber(&number) );
    return number;
}

//-------------------------------------------------------------------------------------------------
//
// Determine if this buffer is in fact an inert buffer
//
//-------------------------------------------------------------------------------------------------
bool
VxUtil::IsInert( _In_ IVxTextBuffer* pBuffer )
{
    ThrowIfNull(pBuffer);

    CComPtr<IVxContentType> spContentType;
    CComBSTR typeName = L"inert";
    LONG isType = FALSE;

    if ( SUCCEEDED( pBuffer->GetContentType(&spContentType) ) && 
        spContentType &&
        SUCCEEDED( spContentType->IsOfType(typeName, &isType) ))
    {
      return isType != 0;
    }

    return false;
}

TextSpan
VxUtil::ConvertToTextSpan( _In_ const TextSpan2& span )
{
    TextSpan newSpan;
    newSpan.iStartLine = span.iStartLine;
    newSpan.iStartIndex = span.iStartIndex;
    newSpan.iEndLine = span.iEndLine;
    newSpan.iEndIndex = span.iEndIndex;
    return newSpan;
}

TextSpan2 
VxUtil::ConvertToTextSpan2( _In_ const TextSpan& span )
{
    TextSpan2 newSpan;
    newSpan.iStartLine = span.iStartLine;
    newSpan.iStartIndex = span.iStartIndex;
    newSpan.iEndLine = span.iEndLine;
    newSpan.iEndIndex = span.iEndIndex;
    return newSpan;
}

// Returns a BSTR for the specified range of text
STDMETHODIMP VBTextBuffer::GetLineText
( 
    /* [in] */ long iStartLine,
    /* [in] */ CharIndex iStartIndex,
    /* [in] */ long iEndLine,
    /* [in] */ CharIndex iEndIndex,
    /* [out] */ __RPC__deref_out_opt BSTR *pbstrBuf
)
{
    VerifyOutPtr(pbstrBuf);
    return m_spShimBuffer->GetLineText(m_pTextSnapshot, iStartLine, iStartIndex, iEndLine, iEndIndex, pbstrBuf);
}

// Returns a BSTR for the specified line
STDMETHODIMP VBTextBuffer::GetLineText
(
    /* [in] */ long lLine,
    /* [out] */ __RPC__deref_out BSTR *pbstrText
)
{
    VerifyOutPtr(pbstrText);
    return m_spShimBuffer->GetCachedLineText(m_pTextSnapshot, lLine, pbstrText);
}

// Returns a WCHAR* for the specified range of text
// Caller must have allocated the memory before calling.
STDMETHODIMP VBTextBuffer::CopyLineText
( 
    /* [in] */ long iStartLine,
    /* [in] */ CharIndex iStartIndex,
    /* [in] */ long iEndLine,
    /* [in] */ CharIndex iEndIndex,
    /* [in] */ __RPC__in LPWSTR pszBuf,
    /* [out][in] */ __RPC__inout long *pcchBuf
)
{
    HRESULT hr = E_UNEXPECTED;

    CComBSTR bstrLineText;
    IfFailRet(GetLineText(iStartLine, iStartIndex, iEndLine, iEndIndex, &bstrLineText));

    if (pszBuf)
    {
        // Copy the string to the pre-allocated buffer
        memcpy( pszBuf, bstrLineText, (bstrLineText.Length() * sizeof( WCHAR )) );
    }

    if (pcchBuf)
    {
        *pcchBuf = bstrLineText.Length();
    }

    return hr;
}

// Replace the specified span of text with new text.
STDMETHODIMP VBTextBuffer::ReplaceLines
( 
    /* [in] */ long iStartLine,
    /* [in] */ CharIndex iStartIndex,
    /* [in] */ long iEndLine,
    /* [in] */ CharIndex iEndIndex,
    /* [in] */ __RPC__in LPCWSTR pszText,
    /* [in] */ long iNewLen,
    /* [out] */ __RPC__out TextSpan *pChangedSpan
)
{
    VerifyInPtr(pszText);
    return ReplaceLinesEx(rtfDefault, iStartLine, iStartIndex, iEndLine, iEndIndex, pszText, iNewLen, pChangedSpan);
}

STDMETHODIMP VBTextBuffer::CreateLineMarker
( 
    /* [in] */ long iMarkerType,
    /* [in] */ long iStartLine,
    /* [in] */ CharIndex iStartIndex,
    /* [in] */ long iEndLine,
    /* [in] */ CharIndex iEndIndex,
    /* [in] */ __RPC__in_opt IVsTextMarkerClient *pClient,
    /* [out] */ __RPC__deref_out_opt IVsTextLineMarker **ppMarker
)
{
    if (m_pTextLines)
    {
        // Delegate to the SHIM'd IVsTextLines implementation
        return m_pTextLines->CreateLineMarker(iMarkerType, iStartLine, iStartIndex, iEndLine, iEndIndex, pClient, ppMarker);
    }

    // Not required by a hostable IDE feature.
    return E_NOTIMPL;
}

STDMETHODIMP VBTextBuffer::EnumMarkers
( 
    /* [in] */ long iStartLine,
    /* [in] */ CharIndex iStartIndex,
    /* [in] */ long iEndLine,
    /* [in] */ CharIndex iEndIndex,
    /* [in] */ long iMarkerType,
    /* [in] */ DWORD dwFlags,
    /* [out] */ __RPC__deref_out_opt IVsEnumLineMarkers **ppEnum
)
{
    if (m_pTextLines)
    {
        // Delegate to the SHIM'd IVsTextLines implementation
        return m_pTextLines->EnumMarkers(iStartLine, iStartIndex, iEndLine, iEndIndex, iMarkerType, dwFlags, ppEnum);
    }

    // Not required by a hostable IDE feature.
    return E_NOTIMPL;
}

STDMETHODIMP VBTextBuffer::GetPairExtents
( 
    /* [in] */ __RPC__in const TextSpan *pSpanIn,
    /* [out] */ __RPC__out TextSpan *pSpanOut
)
{
    if (m_pTextLines)
    {
        // Delegate to the SHIM'd IVsTextLines implementation
        return m_pTextLines->GetPairExtents(pSpanIn, pSpanOut);
    }

    // Not required by a hostable IDE feature.
    return E_NOTIMPL;
}

STDMETHODIMP VBTextBuffer::GetLineDataEx
( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ long iLine,
    /* [in] */ long iStartIndex,
    /* [in] */ long iEndIndex,
    /* [out] */ __RPC__out TextLineData *pLineData,
    /* [in] */ __RPC__in MARKERDATA *pMarkerData
)
{
    VerifyOutPtr(pLineData);

    if (m_pTextLines )
    {
        // Delegate to the SHIM'd IVsTextLines implementation
        return m_pTextLines->GetLineDataEx(dwFlags, iLine, iStartIndex, iEndIndex, &pLineData->m_LineData, pMarkerData);
    }
    else if (m_pTextBuffer)
    {
        HRESULT hr = S_OK;

        // Grab the current snapshot if it hasn't already been set
        CComPtr<IVxTextSnapshot> pTextSnapshot(m_pTextSnapshot);
        if (!pTextSnapshot)
        {
            m_pTextBuffer->GetCurrentSnapshot(&pTextSnapshot);
        }

        // We only support the default case in the new editor
        ThrowIfFalse(gldeDefault==dwFlags);

        CComPtr<IVxTextSnapshotLine> pTextLine;
        int LineLength = 0;

        IfFailRet(pTextSnapshot->GetLineFromLineNumber(iLine, &pTextLine));
        if (pLineData->m_bstrLineText)
        {
            pLineData->m_bstrLineText.Empty();
            pLineData->m_LineData.pszText = NULL;
            pLineData->m_LineData.iLength = 0;
        }
        IfFailRet(pTextLine->GetText(&pLineData->m_bstrLineText));
        IfFailRet(pTextLine->GetLength(&LineLength));

        ThrowIfFalse(iStartIndex <= LineLength);

        // Return the string at the requested line offset.
        pLineData->m_LineData.pszText  = &(pLineData->m_bstrLineText.m_str[iStartIndex * sizeof(WCHAR)]);
        pLineData->m_LineData.iLength  = (iEndIndex - iStartIndex) > 0 ? iEndIndex - iStartIndex : LineLength;

        // 
        pLineData->m_LineData.pAttributes = NULL;
        pLineData->m_LineData.iEolType = eolCRLF;
        pLineData->m_LineData.dwFlags = 0;
        pLineData->m_LineData.dwReserved = 0;
        pLineData->m_LineData.pAtomicTextChain = NULL;

        return hr;
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP VBTextBuffer::ReleaseLineDataEx
( 
    /* [in] */ __RPC__in TextLineData *pLineData
)
{
    if (m_pTextLines)
    {
        VerifyInPtr(pLineData);
        // Delegate to the SHIM'd IVsTextLines implementation
        return m_pTextLines->ReleaseLineDataEx(&pLineData->m_LineData);
    }

    // In the m_TextBuffer case TextLineData will correctly release the BSTR for the line data.
    return S_OK;
}

//static
HRESULT VBTextBuffer::ConvertSpan( _In_ const _VxSnapshotSpan& vss, _Out_ TextSpan& textSpan )
{
    return VxUtil::ConvertToTextSpan(vss, textSpan);
}

HRESULT VBTextBuffer::TrackingSpanToTextSpan
(
    _In_ CComPtr<IVxTrackingSpan> & spTrackingSpan, 
    _Deref_out_ TextSpan &textSpan
)
{
    CComPtr<IVxTextSnapshot> pTextSnapshot;
    m_pTextBuffer->GetCurrentSnapshot(&pTextSnapshot); // don't get cached snapshot

    HRESULT hr = TrackingSpanToTextSpanForSnapshot(spTrackingSpan, textSpan, pTextSnapshot);
    return hr;
}

HRESULT VBTextBuffer::TrackingSpanToTextSpanForCachedSnapshot
(
    _In_ CComPtr<IVxTrackingSpan> & spTrackingSpan, 
    _Deref_out_ TextSpan &textSpan
)
{
    CComPtr<IVxTextSnapshot> pTextSnapshot = GetCachedOrCurrentSnapshot();
    HRESULT hr = TrackingSpanToTextSpanForSnapshot(spTrackingSpan, textSpan, pTextSnapshot);
    return hr;
}

HRESULT VBTextBuffer::TrackingSpanToTextSpanForSnapshot
(
    _In_ CComPtr<IVxTrackingSpan> & spTrackingSpan, 
    _Deref_out_ TextSpan &textSpan,
    _In_ CComPtr<IVxTextSnapshot> pTextSnapshot
)
{
    HRESULT hr = E_FAIL;

    CComVxSnapshotSpan postSnapshotSpan;

    IfFailGo( spTrackingSpan->GetSpan(pTextSnapshot, &postSnapshotSpan) );
    IfFailGo( VBTextBuffer::ConvertSpan(postSnapshotSpan, textSpan) );
Error:;
    return hr;
}

// works for forward/backward and 0 length span
HRESULT VBTextBuffer::CreateTrackingSpan
(
    _In_ const TextSpan *pSpan, 
    _Deref_out_ CComPtr<IVxTrackingSpan> & spTrackingSpan,
    _In_opt_ _VxSpanTrackingMode  SpanTrackingMode /*= VxSpanTrackingModeEdgeInclusive*/
)
{
    VerifyInPtr(pSpan);
    HRESULT hr = E_FAIL;
    _VxSpan vxSpan;

    long lPositionStart;
    long lPositionEnd;
    
    IfFailRet(GetPositionOfLineIndex(pSpan->iStartLine, pSpan->iStartIndex, &lPositionStart));
    
    IfFailRet(GetPositionOfLineIndex(pSpan->iEndLine, pSpan->iEndIndex, &lPositionEnd));

    vxSpan.start = lPositionStart;
    vxSpan.length = lPositionEnd - lPositionStart;  // could be negative for backward span

    CComPtr<IVxTextSnapshot> pTextSnapshot = GetCachedOrCurrentSnapshot();

    IfFailRet( pTextSnapshot->CreateTrackingSpan(vxSpan, SpanTrackingMode, &spTrackingSpan) );

    return hr;
}

HRESULT 
VBTextBuffer::CreateSelectionTracking(
    _In_ const TextSpan& span, 
    _Deref_out_ CComPtr<ISelectionTracking>& spSelectionTracking)
{
    auto span2 = VxUtil::ConvertToTextSpan2(span);
    return m_spShimBuffer->CreateSelectionTracking(
            m_pTextSnapshot,
            span2,
            &spSelectionTracking);
}

STDMETHODIMP VBTextBuffer::ReplaceLinesEx
( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ long iStartLine,
    /* [in] */ CharIndex iStartIndex,
    /* [in] */ long iEndLine,
    /* [in] */ CharIndex iEndIndex,
    /* [in] */ __RPC__in LPCWSTR pszText,
    /* [in] */ long iNewLen,
    /* [out] */ __RPC__out TextSpan *pChangedSpan
)
{
    // if the caller wants to suppress prettylisting, we need to set/restore the flag on SourceFileView, if it exists
    BackupValue<bool> saveIgnoreCommit(0);
    if (m_pTextLines && (dwFlags & rtfClientSuppressFormatting))  // Note: doesn't work for hostable editor 
    {
        SourceFileView * pSourceFileView = SourceFileView::GetFromIVsTextLines(m_pTextLines);
        if (pSourceFileView)
        {
            saveIgnoreCommit.Init(&pSourceFileView->m_fIgnoreCommit);
            pSourceFileView->m_fIgnoreCommit = true;
        }
    }

    TextSpan2 span;
    HRESULT hr = m_spShimBuffer->ReplaceLinesEx(
            m_pTextSnapshot,
            dwFlags,
            iStartLine,
            iStartIndex,
            iEndLine,
            iEndIndex,
            const_cast<LPWSTR>(pszText),
            iNewLen,
            &span);
    if ( SUCCEEDED(hr) && pChangedSpan )
    {
        *pChangedSpan = VxUtil::ConvertToTextSpan(span);
    }

    return hr;
}

STDMETHODIMP VBTextBuffer::CreateTextPoint
( 
    /* [in] */ long iLine,
    /* [in] */ CharIndex iIndex,
    /* [out] */ __RPC__deref_out_opt IDispatch **ppTextPoint
)
{
    HRESULT hr = E_NOTIMPL;
    
    if (m_pTextLines)
    {
        // Delegate to the SHIM'd IVsTextLines implementation
        hr =  m_pTextLines->CreateTextPoint(iLine, iIndex, ppTextPoint);
    }

    // Not required by a hostable IDE feature.
    return hr;
}

STDMETHODIMP VBTextBuffer::GetLengthOfLine
( 
    /* [in] */ long iLine,
    /* [out] */ __RPC__out long *piLength
)
{
    VerifyOutPtr(piLength);
    return m_spShimBuffer->GetLengthOfLine(m_pTextSnapshot, iLine, piLength);
}

STDMETHODIMP VBTextBuffer::GetLineCount
( 
    /* [out] */ __RPC__out long *piLineCount
)
{
    VerifyOutPtr(piLineCount);
    return m_spShimBuffer->GetLineCount(m_pTextSnapshot, piLineCount);
}

STDMETHODIMP VBTextBuffer::GetSize
( 
    /* [out] */ __RPC__out long *piLength
)
{
    VerifyOutPtr(piLength);
    return m_spShimBuffer->GetSize(m_pTextSnapshot, piLength);
}

STDMETHODIMP VBTextBuffer::GetPositionOfLineIndex
(
    long Line,
    long Index,
    long *pPosition 
)
{
    VerifyOutPtr(pPosition);
    return m_spShimBuffer->GetPositionOfLineIndex(m_pTextSnapshot, Line, Index, pPosition);
}

STDMETHODIMP VBTextBuffer::GetLineIndexOfPosition(
    long Position,
    long *pLine,                     // OUT - Line number for the provided position.
    long *pIndex)                    // OUT - Offset for the line
{
    VerifyOutPtr(pLine);
    VerifyOutPtr(pIndex);
    return m_spShimBuffer->GetLineIndexOfPosition(m_pTextSnapshot, Position, pLine, pIndex);
}

bool VBTextBuffer::IsTextSpanReadOnly(TextSpan tsCheck)
{
    if (m_pTextLines && !m_pTextBuffer)
    {
        // Check for a readonly marker
        return CVBViewFilterBase::MarkerHasTextSpan(m_pTextLines,MARKER_READONLY,EM_DEFAULT,&tsCheck);
    }
    else if (m_pTextBuffer)
    {
        // Grab the current snapshot if it hasn't already been set
        CComPtr<IVxTextSnapshot> pTextSnapshot(m_pTextSnapshot);
        if (!pTextSnapshot)
        {
            m_pTextBuffer->GetCurrentSnapshot(&pTextSnapshot);
        }

        //Turn the TextSpan into a _VxTextSpan
        _VxSpan  VxSpan;
        CComVxSnapshotPoint Point0;
        CComVxSnapshotPoint Point1;
        int Pos, Length = 0;
        long Res = 0;

        // Find the start position
        CComPtr<IVxTextSnapshotLine> pTextLine;
        IfFailThrow(pTextSnapshot->GetLineFromLineNumber(tsCheck.iStartLine, &pTextLine));
        pTextLine->GetStart(&Point0);
        VxSpan.start = Point0.position;

        // Find the end position
        pTextLine = NULL;
        IfFailThrow(pTextSnapshot->GetLineFromLineNumber(tsCheck.iEndLine, &pTextLine));
        IfFailThrow(pTextLine->GetStart(&Point1));
        IfFailThrow(pTextLine->GetLength(&Length));
        Pos = Point1.position + Length;

        // Set the Length of the span (End position - Start Position)
        ThrowIfFalse(Pos >= VxSpan.start);
        VxSpan.length = Pos - VxSpan.start;

        // Now see if the span is read only
        IfFailThrow(m_pTextBuffer->IsReadOnly(VxSpan, &Res));

        return Res != 0;
    }

    VBFatal(L"Tried to call IsTextSpanReadOnly with no buffer");
}

bool VBTextBuffer::IsLineBlank(long lLine, ULONG charIndexToEndAt /* = ULONG_MAX */)
{
    HRESULT hr = E_UNEXPECTED;

    CComBSTR bstrText;
    IfFailThrow(GetLineText(lLine, &bstrText));
    
    for (size_t i = 0; i < min(bstrText.Length(), charIndexToEndAt); i++)
    {
        if (!IsBlank(bstrText[i]))
        {
            return false;
        }
    }

    return true;
}

bool VBTextBuffer::IsXMLDocCommentLine(long lLine)
{
    CComBSTR bstrText;
    IfFailThrow(GetLineText(lLine, &bstrText));

    return IDEHelpers::IsXMLDocCommentLine(bstrText, bstrText.Length(), NULL);
}

HRESULT VBTextBuffer::GetColorStateAtStartOfLine(
    long lineNumber, 
    long * pState)
{
    VB_ENTRY();
    IfFalseThrow(lineNumber >= 0);
    IfNullThrow(pState);

    m_bufferColorStates.GetActiveSnapshotColorInfo(GetCachedOrCurrentSnapshot())->GetColorStateAtStartOfLine(lineNumber, pState);

    VB_EXIT();
}


HRESULT VBTextBuffer::InvalidateColorState(_In_opt_ ChangeInput * pChangeInput /* = NULL */, _In_opt_ TextSpan *pSpanOfReplace /* = NULL */)
{
    VB_ENTRY();

    // We intentionally use active snapshot color info instead of color info for current buffer snapshot to detect situations where a client
    // tries to invalidate color states and has VBTextBuffer cached snapshot set to non-current snapshot. It is a clear indication of an error
    // in the code
    
	
	RefCountedPtr<SnapshotColorInfoBase> spColorInfo = m_bufferColorStates.GetActiveSnapshotColorInfo(GetCachedOrCurrentSnapshot());

	if (spColorInfo->IsMutable())
	{
		static_cast<CurrentSnapshotColorInfo*>(spColorInfo.GetData())->InvalidateColorState(pChangeInput, pSpanOfReplace);
	}
	else
	{
		VSASSERT(false, "InvalidateColorState must never be called on non-current snapshot");
	}

    VB_EXIT();
}

CComPtr<IVxTextBuffer> 
VBTextBuffer::GetVxTextBuffer()
{
    if ( m_pTextBuffer )
    {
        return m_pTextBuffer;
    }
    
    CComPtr<IVxTextBuffer> spBuffer;
    if ( VxUtil::TryConvertVsTextLinesToVxTextBuffer(m_pTextLines, __ref spBuffer) )
    {
        return spBuffer;
    }

    return NULL;
}

CComPtr<VBTextBuffer> 
VBTextBuffer::Create( _In_ IVsTextLines* pLines )
{
    CComPtr<VBTextBuffer> spBuffer;
    IfFailThrow( ComUtil::CreateWithRef( __ref spBuffer ) );
    spBuffer->m_pTextLines = pLines;

    // If the IVsTextLines is associateod with a SourceFileView we need to use the appropriate
    // IManagedEditorService*
    SourceFileView * pSourceFileView = SourceFileView::GetFromIVsTextLines(pLines);
    spBuffer->m_spEditorService = pSourceFileView 
        ? pSourceFileView->GetManagedEditorService()
        : GetCompilerPackage()->GetManagedEditorService();

    spBuffer->ReloadVxTextBuffer();
    return spBuffer;
}

CComPtr<VBTextBuffer> 
VBTextBuffer::Create( 
    _In_ IVxTextBuffer* pTextBuffer ,
    _In_ IManagedEditorService* pService)
{
    ThrowIfNull(pService);
    ThrowIfNull(pTextBuffer);

    CComPtr<VBTextBuffer> spBuffer;
    IfFailThrow( ComUtil::CreateWithRef( __ref spBuffer) );
    spBuffer->m_pTextBuffer = pTextBuffer;
    spBuffer->m_bufferColorStates.Initialize(pTextBuffer);
    spBuffer->m_spEditorService = pService;
    IfFailThrow( pService->CreateShimBuffer(pTextBuffer, &(spBuffer->m_spShimBuffer)) );

    return spBuffer;
}

//-------------------------------------------------------------------------------------------------
//
// The new editor will update the underlying IVxTextBuffer backing the IVsTextLines instance on
// certain events.  This call will force the VBTextBuffer instance to reload the buffer
//
//-------------------------------------------------------------------------------------------------
void
VBTextBuffer::ReloadVxTextBuffer()
{
    bool changed = false;
    CComPtr<IVxTextBuffer> spBuffer;
    if ( m_pTextLines && VxUtil::TryConvertVsTextLinesToVxTextBuffer(m_pTextLines, __ref spBuffer) )
    {
        // only set m_pTextBuffer if spBuffer is not the 'Inert' buffer
        // this way we will use m_pTextLines until we are called again and get a real buffer
        if ( !VxUtil::IsInert(spBuffer) )
        {
            m_pTextBuffer = spBuffer;
            m_bufferColorStates.GetCurrentSnapshotColorInfo()->SetTextBuffer(spBuffer);
			m_bufferColorStates.SetCurrentSnapshotAsParsed();
            changed = true;
        }
    }

    if ( changed || !m_spShimBuffer )
    {
        m_spShimBuffer.Release();
        if ( m_pTextBuffer )
        {
            IfFailThrow( m_spEditorService->CreateShimBuffer( m_pTextBuffer, &m_spShimBuffer) );
        }
        else 
        {
            ThrowIfFalse(m_pTextLines); // Must have one or the other
            IfFailThrow( m_spEditorService->CreateTextLinesShimBuffer( m_pTextLines, &m_spShimBuffer) );
        }
    }
}


//-------------------------------------------------------------------------------------------------
//
// Update the snapshot to use as parsed to calculate scanner color state.
//
//-------------------------------------------------------------------------------------------------
void
VBTextBuffer::UpdateParsedSnapshot()
{
    m_bufferColorStates.SetCurrentSnapshotAsParsed();
}

#endif IDE
