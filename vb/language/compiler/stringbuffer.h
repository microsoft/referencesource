//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This is a string class that enables creating, appending and clearing a string
//
//-------------------------------------------------------------------------------------------------

#pragma once

class Compiler;
typedef WCHAR STRING;

class BaseWriter
{
public:
    virtual ~BaseWriter() { }

    virtual
    void AppendSTRING(_In_opt_z_ STRING * strBuffer) = 0;

    virtual
    void AppendString(_In_opt_z_ PCWCH wszBuffer) = 0;

    virtual
    void AppendChar(WCHAR wchBuffer) = 0;

    virtual
    void AppendMultiCopiesOfAWChar(
        WCHAR wchBuffer,
        size_t cchLength) = 0;

    virtual
    void AppendWithLength(
        _In_count_(cchLength)PCWCH wszBuffer,
        size_t cchLength) = 0;
};

class StringBuffer :
    public virtual BaseWriter
{
private:
    // No body is intentionally provided for this Stub.  This is a transition mechanism 
    // to guarantee that no one can accidentally pass a Compiler* and silently bind
    // to a differente .ctor
    StringBuffer(Compiler*);

public:
    NEW_CTOR_SAFE()

    DECLARE_ENUM(AllocSize)
        Normal,
        Large,
        Grande
    END_ENUM(AllocSize)

    DECLARE_ENUM(Allocator)
        None,           // m_pvDataPtr is from m_rgbBuffer or external
        VBHeap,         // m_pvDataPtr is from VBAlloc
        CoTaskMemHeap   // m_pvDataPtr is from CoTaskMemAlloc
    END_ENUM(Allocator)

    StringBuffer();
    StringBuffer(ISequentialStream * pStr);
    StringBuffer(
        _In_count_(cchBuffer)WCHAR * wszBuffer,
        size_t cchBuffer);
    StringBuffer(
        AllocSizeEnum size,
        AllocatorEnum allocType);
    StringBuffer(size_t cbInitialAllocationSize);

    ~StringBuffer();

    // See comment to GetStringLength about why GetByteLength() returns long and not size_t
    long GetByteLength() const
    {
        return (long)m_cbLen;
    }

    void Clear();

    void *GetData()
    {
        return m_pvDataPtr;
    }

    WCHAR* GetString() const
    {
        return m_wszStringPtr;
    }

    void AllocateSize(size_t cbNewSize);
    void AllocateSizeAndSetLength(size_t cbNewSize);
    void AllocateSizeAndSetLengthAndNullTerminate(size_t cbNewSize);
    
    void AppendData(
        _In_bytecount_(cbData)const void * pv,
        size_t cbData);

    virtual
    void AppendStringPoolEntry(const StringPoolEntry& entry)
    {
        return AppendSTRING(entry.GetDataNonConst());
    }

    // appends a STRING to the current buffer
    virtual
    void AppendSTRING(_In_opt_z_ STRING * strBuffer);                           
    
    // appends a string to the current buffer    
    virtual
    void AppendString(_In_opt_z_ PCWCH wszBuffer);
    
    // appends a StringBuffer to the current buffer
    void AppendString(const StringBuffer * psBuffer);

    void AppendPrintf(
        const WCHAR * format,
        ...);

    // appends a single char to the current buffer
    virtual
    void AppendChar(WCHAR wchBuffer);

    // appends a character (n) number of times to the buffer
    virtual
    void AppendMultiCopiesOfAWChar(
        WCHAR wchBuffer,
        size_t cchLength);

    // appends a string with a known length to the buffer
    virtual
    void AppendWithLength(
        _In_count_(cchLength)PCWCH wszBuffer,
        size_t cchLength);

    // appends a string with a known length to the buffer
    void AppendWithLength(
        _In_ const StringBuffer * psBuffer,
        size_t cchLength);

    // Insert text into the current buffer
    void InsertString(
        unsigned startPos,
        _In_z_ PCWCH pTextToInsert);

    // Delete the specified number of WCHARs from the current buffer
    void DelWCharsFromString(
        unsigned startPos,
        int numCharsToDelete = - 1);

    void StripTrailingBlanks();
    
    void StripTrailingBlanksAndLineBreaks();

    // Removes extra white spaces (including line breaks) from a string.
    void StripExtraWhiteSpacesAndLineBreaks();

    void CopyWithLength(
        _In_count_(cchLength)PCWCH wszBuffer,
        size_t cchLength);

    void Copy(StringBuffer *);

    WCHAR * AllocateBufferAndCopy(_Inout_ NorlsAllocator * allocator);

    // Microsoft:
    // GetStringLength() returns long for Win64 sake. Strings are never larger that 2GB,
    // and most of Win API functions and VS Environment interfaces accept long or int
    // as a string length. I intentionally didn't change other methods such as AppendWithLength
    // to accept int/long string length instead of size_t. size_t is 64 and wcslen and strlen
    // return size_t. So this allows code line AppendWithLength(..., strlen(str)) to compile.
    long GetStringLength() const
    {
        return (long)m_cbLen / sizeof (WCHAR);
    }

    void Truncate(size_t cchNewStringLength );

    void SetAllocationSize(AllocSizeEnum size)
    {
        m_allocSize = size;
    }

    AllocSizeEnum GetAllocationSize() const
    {
        return m_allocSize;
    }

private:
    enum AllocSizeBytes
    {
        normalSize = 256,
        largeSize = 1024,
        grandeSize = 2048,
    };

    size_t          m_cbLen;                        // length of the string IN BYTES
    size_t          m_cbMemory;                     // size of allocated memory
    AllocSizeEnum   m_allocSize;                    // memory allocation granularity
    unsigned        m_PowerOfTwo;                   // current power of two used for grande allocations

    AllocatorEnum   m_allocator;                    // the allocation method

    union
    {
        WCHAR*      m_wszStringPtr;                 // pointer to the buffer if any
        BYTE*       m_pvDataPtr;
    };

    BYTE m_rgbBuffer[256];                          // internal buffer array (enables stack-based string buffers).
};


class AppendStringBuffer
{
public:
    AppendStringBuffer(
        _In_ StringBuffer * pStringBuffer,
        bool autoRollback = true)
    {
        m_pStringBuffer = pStringBuffer;
        InternalInit(autoRollback);
    }

    ~AppendStringBuffer()
    {
        if ( m_autoRollback )
        {
            Rollback();
        }
    }

    void Commit()
    {
        // We will not rollback changes made to the string buffer
        InternalInit(false);
    }

    void Rollback()
    {
        // Roll back the string buffer to the previous size
        m_pStringBuffer->Truncate( m_cchRollback );
        InternalInit(false);
    }

    operator StringBuffer*()
    {
        return m_pStringBuffer;
    }

    StringBuffer* operator->()
    {
        return m_pStringBuffer;
    }

    StringBuffer* operator&()
    {
        return m_pStringBuffer;
    }

private:
    void InternalInit(bool autoRollback)
    {
        m_cchRollback = m_pStringBuffer->GetStringLength();
        m_autoRollback = autoRollback;
    }

    StringBuffer*   m_pStringBuffer;
    size_t          m_cchRollback;
    bool            m_autoRollback;
};

//Defines a replacement for the string buffer class
//that immedietly outputs to a file, in UTF8 format
class UTF8FileWriter :
    public virtual BaseWriter
{
public:
    UTF8FileWriter(
        HANDLE hFile,
        Compiler * pCompiler);

    virtual
    void AppendSTRING(_In_opt_z_ STRING * strBuffer);

    virtual
    void AppendString(_In_opt_z_ PCWCH wszBuffer);

    virtual
    void AppendChar(WCHAR wchBuffer);

    virtual
    void AppendMultiCopiesOfAWChar(
        WCHAR wchBuffer,
        size_t cchLength);

    virtual
    void AppendWithLength(
        _In_count_(cchLength)PCWCH wszBuffer,
        size_t cchLength);

private:
    void AppendData(
        const WCHAR * pv,
        size_t cbSize);

private:
    NorlsAllocator m_alloc;
    HANDLE m_hFile;
};
