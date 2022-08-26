//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manages the text of a file, whether it is currently stored on disk
//  or is stored in a text buffer.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class SourceFile;
class TextFileCacheRef;
class TextFileCache;

//-------------------------------------------------------------------------------------------------
//
// Memory mapped file I/O helpers.
//
struct TextFile : ZeroInit<TextFile>
{
    TextFile()
    {
        m_hFile = INVALID_HANDLE_VALUE;
        m_pTextFileCacheRef = NULL;
    }

    ~TextFile()
    {
        CloseFile();
    }

    void Init(
        Compiler * pCompiler,
        SourceFile * pfile,
        _In_z_ STRING * pstrFileName)
    {
        CompilerIdeLock spLock(m_TextFileCriticalSection);
        m_pCompiler = pCompiler;
        m_pfile = pfile;
        m_pstrFileName = pstrFileName;
    }


    // Get the text for a file.
    HRESULT GetFileText(
        _Inout_ NorlsAllocator * pnra,
        __deref_out_ecount_opt(* pcchText) WCHAR * * pwszText,
        _Out_ size_t * pcchText

    // This is UGLY, but I can't think of a better way to do it
#if IDE
        , _Out_ TriState<CComPtr<IVxTextSnapshot>>& maybeSnapshot
#endif
        );



    HRESULT SetBuffer(
        _In_opt_bytecount_(dwLen)WCHAR * wszBuffer,
        DWORD dwLen)
    {
        CompilerIdeLock spLock(m_TextFileCriticalSection);
        if (wszBuffer != NULL)
            m_bstrBuffer = ::SysAllocStringLen(wszBuffer, dwLen / sizeof(WCHAR));
        else
            m_bstrBuffer = NULL;
        m_dwBufLen  = dwLen;

        return NOERROR;
    }
    void ReleaseBuffer()
    {
        if (m_bstrBuffer != NULL)           
        {
            ::SysFreeString(m_bstrBuffer);
            m_bstrBuffer = NULL;
            m_dwBufLen = 0;
        }
    }

    bool isVBAInMemoryBuffer()
    {
        return (m_bstrBuffer != NULL && m_dwBufLen > 0);
    }

    void ForceCloseFile()
    {
        CloseFile();
    }

    LTI_INFO GetLTI()
    {
        return m_lti;
    }


#if IDE
    void UpdateCodePageFromView();
#endif

    HRESULT GetCryptHash(
        _Out_opt_ void * pvHashValue,
        DWORD cbHashSize);

#if IDE
    bool DifferentCacheAndDiskTimestamps() const ;

    TextFileCacheRef* GetTextFileCacheRef()
    {
        return m_pTextFileCacheRef;
    }
#endif IDE

    static bool ReadUnicodeSourceFileCache(
        _Inout_ NorlsAllocator * pnra,
        _In_ TextFileCache* pTextFileCache,
        _Inout_ TextFileCacheRef * * ppTextFileCacheRef,
        _In_ STRING *pstrFileName,
        _Out_ _Deref_post_cap_(*pcchFileSize) WCHAR ** pFileContents,
        _Out_ size_t * pcchFileSize);

private:
    static
    HRESULT LoadTextFileFromBytes(
        _Inout_ NorlsAllocator * pnra,  // allocator
        const BYTE * pData,  // text file bytes
        DWORD dwSize,  // text file number of bytes
        LTI_INFO lti,  // Instructions for format, detection
        _Out_opt_ LTI_INFO * plti,  // [out] format used
        __deref_out_ecount_opt(* pcchFileSize)WCHAR * * pwszFileContents,  // [out] unicode characters
        _Out_ size_t * pcchFileSize); // [out] number of unicode characters


private:

    // Context.
    Compiler *m_pCompiler;

    // The owning source file.
    SourceFile *m_pfile;

    // The name of the file.
    STRING *m_pstrFileName;


    // Close the file handle for the file.
    void CloseFile();

    // The handle to the file.  INVALID_HANDLE_VALUE if the file is
    // not open.
    //
    HANDLE m_hFile;

    // Host-provided (VBA) in-memory buffer
    BSTR m_bstrBuffer;

    // length of buffer in bytes
    DWORD m_dwBufLen;
    LTI_INFO m_lti;      // LTI info of the file


    void LockTextFile()
    {
        m_TextFileCriticalSection.Lock();
    }

    void UnlockTextFile()
    {
        m_TextFileCriticalSection.Unlock();
    }

    CompilerIdeCriticalSection m_TextFileCriticalSection;


    // We keep a TextFileCacheRef for data that has been cached to
    // the compiler package's SourceFileCache. Just opening a file is a very
    // expensive operation. We use the cache to prevent opening and closing
    // the handle every time we need the file contents.
    static bool ReadByteSourceFileCache(
        _Inout_ NorlsAllocator * pnra,
        _In_ TextFileCache* pTextFileCache,
        _Inout_ TextFileCacheRef * * ppTextFileCacheRef,
        _In_ STRING *pstrFileName,
        _Out_ BYTE * * pFileContents,
        _Out_ size_t * pcbFileSize);
    
    void WriteByteSourceFileCache(
        const BYTE * pFileContents,
        size_t cbFileSize,
        const FILETIME &timestamp,
        const bool IsDetectUTF8WithoutSig = true);

    void WriteUnicodeSourceFileCache(
        _In_count_(cchFileSize)const WCHAR * pFileContents,
        size_t cchFileSize,
        const FILETIME &timestamp,
        const bool IsDetectUTF8WithoutSig);
    
    void ClearSourceFileCache();

    TextFileCacheRef*   m_pTextFileCacheRef;

};

//
// The TemporaryFile class provides a temp file with
// exclusive read/write access and delete-on-close semantics.
//
class TemporaryFile
{
public:
    static HRESULT GetUniqueTemporaryFileName(StringBuffer& sb, _In_z_ PCWSTR wszPrefix);

public:
    TemporaryFile();
    ~TemporaryFile();

    void Open(LPCWSTR wszPrefix);
    void Close();
    HANDLE GetHandle() const;
    bool IsValidHandle() const;

private:
    HANDLE              m_hFile;
};

//
// The TextFileCacheRef object points to an entry in
// the TextFileCache. The offset, size and timestamp
// of the entry are maintained to ensure that the
// element in the caches matches the reference.
//
class TextFileCacheRef
{
public:
    NEW_MUST_ZERO()

    TextFileCacheRef();

    TextFileCacheRef(
        LONG offset,
        DWORD size,
        const FILETIME &lastWriteTime,
        const bool IsDetectUTF8WithoutSig = true);

    LONG GetOffset() const 
    {
         return m_Offset; 
    }
    
    void SetUnicode(bool fUnicode) 
    {
         m_IsUnicode = fUnicode;
    }
    
    bool IsUnicode() 
    {
         return m_IsUnicode;
    }

    void SetDetectUTF8WithoutSig(bool DetectUTF8) 
    {
         m_DetectUTF8WithoutSig = DetectUTF8;
    }
    
    bool IsDetectUTF8WithoutSig() 
    {
         return m_DetectUTF8WithoutSig; 
    }
    
    bool HasCryptHash() 
    {
         return m_HasCryptHash; 
    }
    
    void * GetCryptHash() 
    {
         return (void *)&m_bCryptHash; 
    }
    
    void SetCryptHash(void * pvHashValue) 
    {
         memcpy(m_bCryptHash, pvHashValue, CRYPT_HASHSIZE); 
         m_HasCryptHash = true; 
    }
    
    DWORD GetSize() const 
    {
         return m_Size; 
    }
    
    FILETIME GetTimestamp() const 
    {
         return m_Timestamp; 
    }
    
    bool operator ==(const TextFileCacheRef &e) const;
    
    bool operator !=(const TextFileCacheRef &e) const;

private:
    LONG                m_Offset;   // bytes
    DWORD               m_Size;     // bytes
    BYTE                m_bCryptHash[CRYPT_HASHSIZE];
    FILETIME            m_Timestamp;
    bool                m_IsUnicode:1;
    bool                m_DetectUTF8WithoutSig:1;
    bool                m_HasCryptHash:1;
};


//
// The TextFileCache manages a temporary file that contains the
// cached contents of text files. Each entry in the temp file
// contains a header that consists of the size and timestamp of
// the entry, followed by the actual bits.
//
class TextFileCache
{
public:

    TextFileCache(Compiler * pCompiler) 
    {
         m_pCompiler = pCompiler; 
    }

    ~TextFileCache()
    {
        Close();
    }

    bool IsOpen();
    void Close();

    TextFileCacheRef * Append(
        LPVOID lpBuffer,
        DWORD cbBytes,
        const FILETIME &timestamp,
        const bool IsDetectUTF8WithoutSig);

    DWORD Read(
        const TextFileCacheRef * pElement,
        LPVOID lpBuffer,
        DWORD cbBytes);

    DWORD Read(
        LPCWSTR pszFileName,
        const TextFileCacheRef * pElement,
        LPVOID lpBuffer,
        DWORD cbBytes);

    static const FILETIME NullFileTime;

private:

    void EnsureOpen();
    struct EntryHeader
    {
        EntryHeader();
        EntryHeader(
            DWORD size,
            const FILETIME &lastWriteTime);

        DWORD           m_Size;
        FILETIME        m_Timestamp;
    };

    Compiler*           m_pCompiler;
    TemporaryFile       m_TempFile;
    CompilerIdeCriticalSection m_CriticalSection;
};
