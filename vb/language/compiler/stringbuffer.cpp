//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This is a string class that enables creating, appending, clearing and
//  obtaining the length of a string. Eliminates building string on the
//  local stack.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

StringBuffer::StringBuffer(
    AllocSizeEnum size,
    AllocatorEnum allocType)
{
    m_allocator = allocType;
    m_allocSize = size;
    m_PowerOfTwo = grandeSize;

    m_wszStringPtr = (WCHAR *)m_rgbBuffer;
    m_cbMemory = sizeof(m_rgbBuffer);

    Clear();
}

/******************************************************************
*StringBuffer::StringBuffer()
*Purpose:
*       Sets up a string buffer with an initial size.
*       initial data members
*Entry:
*       none
*Exit:
*
******************************************************************/
StringBuffer::StringBuffer(size_t cbInitialAllocationSize)
{
    m_allocator = Allocator::None;
    m_allocSize = AllocSize::Normal;
    m_PowerOfTwo = grandeSize;

    m_wszStringPtr = (WCHAR *)m_rgbBuffer;
    m_cbMemory = sizeof(m_rgbBuffer);
    Clear();

    AllocateSize(cbInitialAllocationSize);
}

/******************************************************************
*StringBuffer::StringBuffer()
*Purpose:
*       Default constructor
*       initial data members
*
******************************************************************/
StringBuffer::StringBuffer
(
    _In_count_(cchBuffer) WCHAR *wszBuffer,  // String to store into
    size_t cchBuffer   // Size of buffer
)
{
    m_allocator = Allocator::None;
    m_allocSize = AllocSize::Normal;
    m_PowerOfTwo = grandeSize;

    m_wszStringPtr = wszBuffer;

    // Microsoft 9/19/2004:  Overflow check; trying to avoid math to do it to minimize perf hit.
    if (cchBuffer < 0x80000000)
    {
        m_cbMemory = cchBuffer * sizeof(WCHAR);
    }
    else
    {
        VSASSERT(false,_T("Overflow in string allocation -- setting memory to zero.  Check the calling code."));
        m_cbMemory = 0;
    }

    Clear();
}

/******************************************************************
*StringBuffer::StringBuffer()
*Purpose:
*       Default constructor
*       initial data members
*Entry:
*       none
*Exit:
*
******************************************************************/
StringBuffer::StringBuffer()
{
    m_allocator = Allocator::None;
    m_allocSize = AllocSize::Normal;
    m_PowerOfTwo = grandeSize;

    m_wszStringPtr = (WCHAR *)m_rgbBuffer;
    m_cbMemory = sizeof(m_rgbBuffer);

    Clear();
}

StringBuffer::StringBuffer( ISequentialStream * pStr)
{
    m_allocator = Allocator::None;
    m_allocSize = AllocSize::Normal;
    m_PowerOfTwo = grandeSize;

    m_wszStringPtr = (WCHAR *)m_rgbBuffer;
    m_cbMemory = sizeof(m_rgbBuffer);

    Clear();

    HRESULT hr = S_OK;

    ULONG bytesRead = 0;

    do
    {
        AllocateSize(m_cbLen + sizeof(WCHAR));

        bytesRead = 0;
        hr = pStr->Read(m_pvDataPtr + m_cbLen, (ULONG)(m_cbMemory - m_cbLen - sizeof(WCHAR)), &bytesRead);

        m_cbLen += bytesRead;
    } while (hr == S_OK && bytesRead);

#pragma prefast(suppress: 26010, "No overflow here")
    m_wszStringPtr[m_cbLen / 2] = 0;
}

/******************************************************************
* StringBuffer::~StringBuffer()
* Purpose:
*       destroys the string
* Entry:
*       none
* Exit:
*
******************************************************************/
StringBuffer::~StringBuffer()
{
    Clear();
}

void StringBuffer::Clear()
{
    if ( Allocator::None != m_allocator )
    {
        if ( Allocator::VBHeap == m_allocator )
        {
            VBFree(m_pvDataPtr);
        }
        else if ( Allocator::CoTaskMemHeap  == m_allocator )
        {
            CoTaskMemFree(m_pvDataPtr);
        }
        else
        {
            VSASSERT(FALSE, "StringBuffer has invalid allocator type");
        }

        // Reset the StringBuffer to the internal buffer
        m_pvDataPtr = m_rgbBuffer;
        m_cbMemory = sizeof(m_rgbBuffer);
        m_allocator = Allocator::None;
    }

    m_cbLen = 0;
    *m_wszStringPtr = 0;
}

void StringBuffer::AppendPrintf(
    const WCHAR * format,
    ...)
{
    ThrowIfNull(format);

    va_list arglist;
    size_t cbSize = 100;
    bool done = false;
    while (!done)
    {
        VBHeapPtr<WCHAR> buffer;
        buffer.AllocateBytes(cbSize);

        va_start(arglist, format);
        HRESULT hr = ::StringCbVPrintf(
                buffer,
                cbSize,
                format,
                arglist);
        va_end(arglist);

        if ( FAILED(hr))
        {
            ThrowIfFalse(STRSAFE_E_INSUFFICIENT_BUFFER == hr);
            cbSize *= 2;
        }
        else
        {
            size_t cbLen;
            hr = ::StringCbLength(buffer, cbSize, &cbLen);
            ThrowIfFalse(SUCCEEDED(hr));
            AppendData(buffer, cbLen);
            done = true;
        }
    }
}


/******************************************************************
* void StringBuffer::AppendString(WCHAR* wszBuffer)
* Purpose:
*       appends a string to the current buffer
* Entry:
*       *wszBuffer : pointer to a string to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendString(_In_opt_z_ PCWCH wszBuffer)
{
    if (wszBuffer == NULL)
        return;

    AppendData(wszBuffer, wcslen(wszBuffer) * sizeof(WCHAR));
}

/******************************************************************
* void StringBuffer::AppendString(STRING* strBuffer)
* Purpose:
*       appends a STRING to the current buffer
* Entry:
*       strBuffer : pointer to a STRING to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendSTRING(_In_opt_z_ STRING * strBuffer)
{
    if (strBuffer == NULL)
        return;

    AppendData(strBuffer, StringPool::StringLength(strBuffer) * sizeof(WCHAR));
}

/******************************************************************
* void StringBuffer::AppendString(StringBuffer* psBuffer)
* Purpose:
*       appends a the string from a StringBuffer to the current buffer
* Entry:
*       psBuffer : pointer to a StringBuffer to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendString(const StringBuffer * psBuffer)
{
    VSASSERT( psBuffer != NULL, "Caller must supply a StringBuffer" );
    VSASSERT(wcslen(psBuffer->m_wszStringPtr)*sizeof(WCHAR) == psBuffer->GetByteLength(), "String buffer byte length not equal to string length!");
    AppendData( psBuffer->m_wszStringPtr, psBuffer->GetByteLength());
}

/******************************************************************
* void StringBuffer::AppendWithLength(WCHAR* wszBuffer)
* Purpose:
*       appends a string to the current buffer
* Entry:
*       *wszBuffer : pointer to a string to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendWithLength(
    const StringBuffer * psBuffer,
    size_t cchLength)
{
    VSASSERT( psBuffer != NULL, "Caller must supply a StringBuffer" );

    // Microsoft 9/19/2004:  Overflow check; trying to avoid math to do it to minimize perf hit.
    if (cchLength < 0x80000000)
    {
        AppendData( psBuffer->m_wszStringPtr, cchLength * sizeof( WCHAR ) );
    }
    else
    {
        VSFAIL(_T("Overflow in string allocation.  Check the calling code."));
    }
}

/******************************************************************
* void StringBuffer::AppendWithLength(PCWCH wszBuffer, size_t cchLength)
* Purpose:
*       appends a string to the current buffer
* Entry:
*       *wszBuffer : pointer to a string to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendWithLength(
    _In_count_(cchLength)PCWCH wszBuffer,
    size_t cchLength)
{
    // Microsoft 9/19/2004:  Overflow check; trying to avoid math to do it to minimize perf hit.
    if (cchLength < 0x80000000)
    {
        AppendData(wszBuffer, cchLength * sizeof(WCHAR));
    }
    else
    {
        VSFAIL(_T("Overflow in string allocation.  Check the calling code."));
    }
}


/******************************************************************
* void StringBuffer::Truncate(size_t cchNewStringLength )
* Purpose:
*       Truncates the string to the number of characters.
* Entry:
*       *wszBuffer : pointer to a string to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::Truncate(size_t cchNewStringLength)
{
    if (cchNewStringLength < (size_t)GetStringLength())
    {
        m_cbLen = cchNewStringLength * sizeof(WCHAR);
        *(WCHAR*)(m_pvDataPtr + m_cbLen) = '\0';
    }
}

/******************************************************************
* void StringBuffer::AllocateSize( size_t cchSize )
* Purpose:
* Entry:
* Exit:
*
******************************************************************/
void StringBuffer::AllocateSize(size_t cbNewSize)
{
    BYTE*   pvResult;

    // The buffer size should be large enough for the requested bytes plus a null character.
    cbNewSize += sizeof(WCHAR);
    if (cbNewSize < sizeof(WCHAR) || cbNewSize >= 0x80000000)
    {
        VSFAIL(_T("Overflow in string allocation.  Check the calling code."));
    }

    //  Do we have enough space?
    if ( cbNewSize > m_cbMemory )
    {
        // Round the allocation size up to nearest chunk, depending
        // on how aggressive we want to be with this string buffer.
        size_t cbChunkSize;

        if ( m_allocSize == AllocSize::Grande )
        {
            IfFalseThrow(m_PowerOfTwo < 0x80000000);
            cbChunkSize = m_PowerOfTwo;
            m_PowerOfTwo *= 2;
        }
        else if ( m_allocSize == AllocSize::Large )
            cbChunkSize = largeSize;
        else
            cbChunkSize = normalSize;

        cbNewSize = (size_t)(((UINT)cbNewSize + ((UINT)cbChunkSize - 1)) & ~((UINT)cbChunkSize - 1));

        // The source buffer could be the stack or an allocated buffer.
        // If we reallocate using a heap, we'll have to copy the data over.
        bool fCopyOldDataPtr = false;

        // Allocate or reallocate according to the correct heap
        switch ( m_allocator )
        {
        default:
            // 
            VSASSERT(FALSE, "Unknown StringBuffer allocation type. Using default allocator.");
            // We're messed up; this is really bad, but not fatal. Attempt recovery.
            m_allocator = Allocator::None;

            __fallthrough; // Fall thru to case Allocator::None.

        case Allocator::None:
            pvResult = (BYTE *)VBAlloc(cbNewSize);
            m_allocator = Allocator::VBHeap;
            fCopyOldDataPtr = true;
            break;

        case Allocator::VBHeap:
            pvResult = (BYTE *)VBRealloc(m_pvDataPtr, cbNewSize);
            break;

        case Allocator::CoTaskMemHeap:
            pvResult = (BYTE *)CoTaskMemRealloc(m_pvDataPtr, cbNewSize);
            break;
        }

        // Check the allocation
        IfNullThrow(pvResult);

        // If we just switched from the stack version to a heap version,
        // we need to copy the string we had on the stack to the heap version.
        if ( fCopyOldDataPtr )
        {
            memcpy( pvResult, m_pvDataPtr, m_cbMemory );
        }

        // Update internal state
        m_pvDataPtr = pvResult;
        m_cbMemory = cbNewSize;
    }
}

void StringBuffer::AllocateSizeAndSetLength(size_t cbNewSize)
{
    AllocateSize(cbNewSize);
    m_cbLen = cbNewSize;
}

void 
StringBuffer::AllocateSizeAndSetLengthAndNullTerminate(size_t cbNewSize)
{
    AllocateSizeAndSetLength(cbNewSize);
    GetString()[GetStringLength()] = '\0';
}

/******************************************************************
* void StringBuffer::CopyWithLength(PCWCH wszBuffer, size_t cchLength)
* Purpose:
*       appends a string to the current buffer
* Entry:
*       *wszBuffer : pointer to a string to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::CopyWithLength(
    _In_count_(cchLength)PCWCH wszBuffer,
    size_t cchLength)
{
    // Microsoft 9/19/2004:  Overflow check; trying to avoid math to do it to minimize perf hit.
    IfFalseThrow(cchLength < 0x80000000);
    AllocateSize( cchLength * sizeof(WCHAR) );

    //  Copy it in.
    m_cbLen = cchLength * sizeof( WCHAR );

    wcsncpy_s( (PWCH) m_pvDataPtr, m_cbMemory/sizeof( WCHAR ), wszBuffer, cchLength );

    //  Zero terminate.
    *(WCHAR*)(m_pvDataPtr + m_cbLen) = '\0';
}

/******************************************************************
* void StringBuffer::AppendChar(WCHAR chBuffer)
* Purpose:
*       appends a single WCHARacter to the current buffer
* Entry:
*       chBuffer : single WCHARacter to be appended to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendChar(WCHAR wchBuffer)
{
    AppendData(&wchBuffer, sizeof(WCHAR));
}

/******************************************************************
* void StringBuffer::AppendMultiCopiesOfChar(WCHAR wchBuffer, size_t cchLength )
* Purpose:
*       appends a character (cchLength) number of times to the buffer
* Entry:
*       chBuffer : single WCHARacter to be appended (cchLength) number of times
*       to StringBuffer
* Exit:
*
******************************************************************/
void StringBuffer::AppendMultiCopiesOfAWChar(
    WCHAR wchBuffer,
    size_t cchLength)
{
    // Microsoft 9/19/2004:  Overflow check.
    IfFalseThrow(cchLength < 0x80000000 && m_cbLen + (cchLength + 1) * sizeof(WCHAR) >= m_cbLen);
    if (cchLength > 0)
    {
        // Grow the buffer by cchLength * sizeof(WCHAR) bytes
        AllocateSize(m_cbLen + (cchLength + 1) * sizeof(WCHAR));

        long StringLength = GetStringLength();  // original string length

        // Append the data to the buffer
        size_t i;
        for (i = 0; i < cchLength; ++i)
        {
            m_wszStringPtr[StringLength + i] = wchBuffer;
        }

        // NULL terminate the string
        m_wszStringPtr[StringLength + i] = UCH_NULL;

        // update the m_cbLen count
        m_cbLen += cchLength * sizeof( WCHAR );
    }
}

/******************************************************************
* void StringBuffer::AppendData(void *pvData, size_t cbData)
* Purpose:
*       appends some data to the buffer
* Entry:
*       pvData - the data to add
*       cbData - the length of the data
* Exit:
*       None.
*
******************************************************************/

void StringBuffer::AppendData(
    _In_bytecount_(cbData)const void * pvData,
    size_t cbData)
{
    IfFalseThrow(m_cbLen + cbData >= m_cbLen);
    AllocateSize( m_cbLen + cbData );

    // Append the data.
    memcpy(m_pvDataPtr + m_cbLen, pvData, cbData);
    m_cbLen += cbData;

    // Zero terminate.
    *(WCHAR *)(m_pvDataPtr + m_cbLen) = 0;
}

/******************************************************************
* void StringBuffer::Copy( StringBuffer* psBuffer )
* Purpose:
*       Copy a StringBuffer to the target ( overwrite )
* Entry:
*       psBuffer - Source StringBuffer
* Exit:
*       None.
*
******************************************************************/

void StringBuffer::Copy(StringBuffer * psBuffer)
{
    CopyWithLength( psBuffer->GetString(), psBuffer->GetStringLength() );
}

/******************************************************************
* WCHAR* StringBuffer::AllocateBufferAndCopy( NorlsAllocator* allocator)
* Purpose:
*       Allocate appropriate buffer with the passed in allocator
*       and copy the contents of the StringBuffer to the that block
* Entry:
*       allocator - A NorlsAllocator
* Exit:
*       Pointer to allocated buffer.
*
******************************************************************/
WCHAR * StringBuffer::AllocateBufferAndCopy(_Inout_ NorlsAllocator * allocator)
{
    WCHAR *Result = NULL;

    unsigned length = GetStringLength() + 1; // make room for the null terminator

    if (GetStringLength() + 1 >= 0 && length >= (unsigned)(GetStringLength() + 1))
    {
        if (VBMath::TryMultiply(length, sizeof(WCHAR)))
        {
            Result = (WCHAR*)allocator->Alloc(VBMath::Multiply(length, sizeof(WCHAR)));
            Result[length - 1] = L'\0'; // At least guarantee NULL termination

            wcscpy_s(Result, length, GetString());
        }
    }

    return Result;
}


/******************************************************************
* DelWCharsFromString
* Purpose: Remove a section from the current buffer.  Leaves the size of the current
*               buffer the same.  That may seem odd but remember that the buffer is
*               the one on the stack when strings are smaller...
******************************************************************/
void StringBuffer::DelWCharsFromString
(
    unsigned int startPos,  // the index (0-based) where we want to start deleting
    int numChars2Delete     // the number of characters to delete.  If you supply -1
                            //  (the default) then the deletion is from startPos to the end
                            //  of the string.
)
{
    size_t stringLen = m_cbLen / sizeof( WCHAR );

    // parm validation
    if ( startPos > stringLen || ( numChars2Delete > 0 && numChars2Delete + startPos > stringLen )
         || ( numChars2Delete < 0 && numChars2Delete != -1 ))
    {
        VSASSERT( FALSE, "Bad parameters to DelWCharsFromString" );
        return;
    }

    // The default is to delete from startPos to the end of the string
    if ( numChars2Delete == -1 )
    {
        m_cbLen = startPos * sizeof( WCHAR ); // length of the string is from 0 to startpos since we are whacking the rest
#pragma warning(disable:22008)//,"startPos is actually well-bounded")
        m_wszStringPtr[ startPos ] = UCH_NULL; // null terminate to whack the remainder of the string
#pragma warning(default:22008)//,"startPos is actually well-bounded")
        return;
    }

    // Slide the text down to effect the deletion
    for ( size_t i = startPos; i < stringLen - numChars2Delete; i++ )
    {
        m_wszStringPtr[ i ] = m_wszStringPtr[ i + numChars2Delete ];
    }
    m_wszStringPtr[ stringLen - numChars2Delete ] = UCH_NULL; // null terminate
    m_cbLen -= numChars2Delete * sizeof( WCHAR ); // length of the string changes but buffer size  
                                                  // remains the same ( don't update m_cbMemory )
}

/******************************************************************
* StripTrailingBlanks
* Purpose:
*       String any trailing blanks from the buffer
******************************************************************/
void StringBuffer::StripTrailingBlanks()
{
    size_t stringLen = m_cbLen / sizeof(WCHAR); 

    // Check to see if there's any trailing blanks
    while (stringLen != 0 && IsBlank(m_wszStringPtr[stringLen-1]))
    {
        m_cbLen -= sizeof(WCHAR);
        stringLen--;
        m_wszStringPtr[stringLen] = UCH_NULL;
    }
}

/******************************************************************
* StripTrailingBlanksAndLineBreaks
* Purpose:
*       String any trailing blanks and line breaks from the buffer
******************************************************************/
void StringBuffer::StripTrailingBlanksAndLineBreaks()
{
    size_t stringLen = m_cbLen / sizeof(WCHAR);

    // Check to see if there's any trailing blanks
    while (stringLen != 0 &&
            (IsBlank(m_wszStringPtr[stringLen-1])  ||
             IsLineBreak(m_wszStringPtr[stringLen-1])))
    {
        m_cbLen -= sizeof(WCHAR);
        stringLen--;
        m_wszStringPtr[stringLen] = UCH_NULL;
    }
}

//============================================================================
// Removes extra white spaces from the stiring. This is done is place, so
// no new memory is allocated. This is done so if we have this comment:
//
// '@<param name='x'>Hello
// '@there</param>
//
// The the tooltip will not look like this "Hellothere", instead, "Hello there".
//============================================================================
void StringBuffer::StripExtraWhiteSpacesAndLineBreaks()
{
    if (!m_wszStringPtr)
    {
        return;
    }

    long CopyFrom   = 0;
    long CopyTo     = 0;

    bool NeedsASpace = false;
    bool LeadingSpace = true;

    while (m_wszStringPtr[CopyFrom] != UCH_NULL)
    {
        while (IsWhitespace(m_wszStringPtr[CopyFrom]))
        {
            if (!LeadingSpace)
            {
                NeedsASpace = true;
            }

            CopyFrom++;
            m_cbLen -= sizeof(WCHAR);
        }

        if (m_wszStringPtr[CopyFrom] != UCH_NULL)
        {
            LeadingSpace = false;

            if (NeedsASpace)
            {
                m_wszStringPtr[CopyTo++] = UCH_SPACE;
                m_cbLen += sizeof(WCHAR);
                NeedsASpace = false;
            }
            else
            {
                m_wszStringPtr[CopyTo++] = m_wszStringPtr[CopyFrom++];
            }
        }
    }

    m_wszStringPtr[CopyTo] = UCH_NULL;
}

/******************************************************************
* InsertString
* Purpose:
*       Insert a string into the current buffer.  The current buffer will grow if
*       necessary to accomodate the insertion.
******************************************************************/
void StringBuffer::InsertString
(
    unsigned int startPos,  // the index (0-based) where we want to insert psBuffer
    _In_z_ PCWCH pTextToInsert     // the string to insert
)
{
    if (startPos * sizeof( WCHAR ) > m_cbLen || 
        pTextToInsert == NULL || 
         !VBMath::TryMultiply(startPos, sizeof( WCHAR )))
    {
        VSASSERT( FALSE, "Bad arguments to InsertString" );
        return;
    }

    size_t cBytesInOrigString = m_cbLen;
    size_t cBytesInTextToInsert = wcslen( pTextToInsert ) * sizeof( WCHAR );

    //  Do we have enough space?
    if (m_cbLen + cBytesInTextToInsert < m_cbLen)
    {
        VSASSERT( FALSE, "Bad arguments to InsertString" );
        return;
    }
    AllocateSize( m_cbLen + cBytesInTextToInsert );
    m_cbLen += cBytesInTextToInsert; // update new length count

    // Slide the existing text over to make room for the insertion
    memmove( &m_pvDataPtr[ startPos * sizeof( WCHAR ) + cBytesInTextToInsert ], &m_pvDataPtr[ startPos * sizeof( WCHAR) ], cBytesInOrigString - startPos * sizeof(WCHAR)); // m_pvDataPtr is a byte array so adjust for WCHAR sizes

    // insert the text
    memcpy( &m_pvDataPtr[ startPos * sizeof( WCHAR) ], pTextToInsert, cBytesInTextToInsert );

    //  Zero terminate.
    VSASSERT(m_cbLen == cBytesInTextToInsert + cBytesInOrigString, "Buffer length is messed up in StringBuffer::InsertString!");
    *(WCHAR*)(m_pvDataPtr + m_cbLen) = '\0';
}

UTF8FileWriter::UTF8FileWriter(
    HANDLE hFile,
    Compiler * pCompiler) :
    m_hFile(hFile),
    m_alloc(NORLSLOC)
{
}

void UTF8FileWriter::AppendSTRING(_In_opt_z_ STRING * strBuffer)
{
    if (strBuffer == NULL)
    {
        return;
    }

    AppendData(strBuffer, StringPool::StringLength(strBuffer));
}

void UTF8FileWriter::AppendString(_In_opt_z_ PCWCH wszBuffer)
{
    if (wszBuffer)
    {
        AppendData(wszBuffer, wcslen(wszBuffer));
    }
}

void UTF8FileWriter::AppendChar(WCHAR wchBuffer)
{
    AppendData(&wchBuffer, 1);
}

void UTF8FileWriter::AppendMultiCopiesOfAWChar(
    WCHAR wchBuffer,
    size_t cchLength)
{
    if (cchLength > 0)
    {
        size_t i;
        for (i = 0; i < cchLength; ++i)
        {
            AppendChar(wchBuffer);
        }
    }
}

void UTF8FileWriter::AppendWithLength(
    _In_count_(cchLength)PCWCH wszBuffer,
    size_t cchLength)
{
    if (wszBuffer)
    {
        if (cchLength < 0x80000000)
        {
            AppendData(wszBuffer, cchLength);
        }
        else
        {
            VSFAIL(_T("Overflow in string allocation.  Check the calling code."));
        }
    }
}

void UTF8FileWriter::AppendData(
    const WCHAR * pv,
    size_t length)
{
    if (pv)
    {
        ULONG UTF8StringLength;
        BYTE *UTF8String;

        ConvertUnicodeToUTF8(pv, (ULONG)length, &m_alloc, &UTF8String, &UTF8StringLength);

        ULONG BytesWritten = 0;

        if (!WriteFile(m_hFile, UTF8String, UTF8StringLength, &BytesWritten, NULL))
        {
            DWORD dwLastError = GetLastError();
            throw HRESULT_FROM_WIN32(dwLastError);
        }
        else
        {
            m_alloc.FreeHeap();
        }
    }
}
