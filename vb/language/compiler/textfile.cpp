//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//============================================================================
// Loads the file into an in-memory buffer.
//============================================================================
//NOTE:Microsoft,4/2000, we have three different text sources for the compiler
//one from disk file, one from IVsTextBuffer, and another from VBA buffer.
//NOTE:Microsoft,11/2000, we also have a file cache for the contents of disk files.
// this function is mimic-ed by GetCryptHash, make sure they are in sync when new text sources are added
HRESULT TextFile::GetFileText(
    _Inout_ NorlsAllocator * pnra,
    __deref_out_ecount_opt(* pcchText) WCHAR * * pwszText,
    _Out_ size_t * pcchText
    
#if IDE
    , _Out_ TriState<CComPtr<IVxTextSnapshot>>& maybeSnapshot
#endif

    )
{ 

    HRESULT hr = NOERROR;
    BSTR    bstrText = NULL;
    WCHAR*  wszFileContents = 0;
    size_t  cchFileSize = 0;
    LTI_INFO lti = LTIF_SIGNATURE | LTIF_DETECT;

    // Keeps track if we got information out of the file.  We need this to
    // ensure that we aren't trying to get information out of a buffer
    // that has been created but not yet loaded.  If the size of the
    // buffer is zero and it hasn't been modified, then we will go out to
    // the file to get its information.
    //
    bool hasFileData = false;
    bool IsDetectUTF8WithoutSig = true;

    // Always init outparams
    *pwszText = NULL;
    *pcchText = 0;

    VSASSERT(m_pfile->GetSolutionExtension() == NULL ||
        m_pfile->GetSolutionExtension()->m_extension != SolutionExtension::SolutionExtensionNone,"SolutionExtensionTemplate should already have been obtained");

    CompilerIdeLock spLock(m_TextFileCriticalSection);


#if IDE
    if (m_pfile->GetProject())
    {
        if (!GetCompilerSharedState()->GetDetectUTF8())
        {
            //Add flag here for cache
            IsDetectUTF8WithoutSig = false;
            lti = lti | LTIF_NOUTF8_NOSIG;
        }
    }
#endif

    if (m_pfile->GetProject() && m_pfile->GetProject()->GetDefaultCodePage() != 0)
    {
        LTI_SETCP(lti, m_pfile->GetProject()->GetDefaultCodePage());
    }

    // Initialize the iterator if the file was loaded.  We create a copy
    // of the buffer to minimze the time that we keep it locked.  Otherwise
    // the user could not edit at all (even in the commit buffer) while
    // this object remained alive.
    //
    // We also need to take the commit buffer into account.  The
    // text we get from pTextScanner will include the user's view of
    // the commit buffer.  We need to strip that out and replace it
    // with the text we want the compiler to see.
    //
#if IDE

    {
        RefCountedPtr<CompilerIdeLock> spLockView = m_pfile->LockView();

        if ( (m_pfile->GetSourceFileView() != NULL) &&
             (m_pfile->GetSourceFileView()->IsIVsTextBufferLoaded() == true) )
        {
            SehGuard guard;
            try
            {
                maybeSnapshot = m_pfile->GetSourceFileView()->GetSnapshotToCompile();
                if ( maybeSnapshot.HasValue() )
                {
                    CComPtr<IVxTextSnapshot> snapshot = maybeSnapshot.GetValue();
                    VxUtil::GetText(snapshot, *pnra, &wszFileContents, &cchFileSize);

                    // VxUtil::GetText returns cchFileSize including the NULL terminator.  The rest of the
                    // code in this method expects it to be the actual count of characters so decrement
                    cchFileSize--;

                    UpdateCodePageFromView();
                    hasFileData = true;
                }
            }
            catch ( SehException &ex )
            {
                hr = ex.GetHResult();
                maybeSnapshot.ClearValue();
            }
        }
    }

#endif IDE

    // check to see if we already have an in-memory buffer (given to it via CompilerProject::AddBuffer)
    if (!hasFileData && m_bstrBuffer != NULL)
    {
        if (m_dwBufLen > 0)
        {
            // Allocate the real buffer.
            wszFileContents = (WCHAR *)pnra->Alloc(m_dwBufLen);

            // Copy it
            ::memcpy((void *) wszFileContents, (void *) m_bstrBuffer, m_dwBufLen);

            cchFileSize = m_dwBufLen / sizeof(WCHAR);
        }
        else // Empty buffer case--we can just bail.
        {
            wszFileContents = (WCHAR*)pnra->Alloc(sizeof(WCHAR));
            *wszFileContents = L'\0';
            cchFileSize = 0;
        }

        hasFileData = true;
    }

    if ( !hasFileData && m_pstrFileName != NULL )
    {
        {
            if (m_pTextFileCacheRef &&
                m_pTextFileCacheRef->IsDetectUTF8WithoutSig() == IsDetectUTF8WithoutSig &&
                ReadUnicodeSourceFileCache(
                    pnra, 
                    m_pfile->GetProject()->GetSourceFileCache(),
                    &m_pTextFileCacheRef,
                    m_pstrFileName,
                    &wszFileContents, 
                    &cchFileSize))
            {
                hasFileData = true;
            }
        }

        // If the file was not loaded into a buffer or if the buffer
        // was empty, load the text from the original file on disk.
        //
        if (!hasFileData)
        {
            CHandle hMap;
            BYTE* pData = NULL;
            SehGuard guard;

            try
            {
                // If we don't already have the file open, open it.
                if (m_hFile == INVALID_HANDLE_VALUE)
                {
                    m_hFile = CreateFileW(m_pstrFileName,
                                          GENERIC_READ,
                                          FILE_SHARE_READ,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                          NULL);

                    if  (m_hFile == INVALID_HANDLE_VALUE)
                    {
                        HRESULT hr2 = GetLastHResultError();
                        VbThrowNoAssert(hr2);                            
                    }
                }

                // Get the size of the file
                DWORD dwSize = GetFileSize(m_hFile, NULL);

                if (dwSize != 0)
                {
                    hMap.Attach(CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL));
                    IfNullThrow(hMap);
                    pData = (BYTE *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
                    IfNullThrow(pData);
                    //Microsoft: moved file conversion to Unicode here
                    //so that we can cache it in this format
                    IfFailThrow(LoadTextFileFromBytes(
                                                        pnra,
                                                        pData,
                                                        dwSize,
                                                        lti,
                                                        &m_lti,
                                                        &wszFileContents,
                                                        &cchFileSize));

                    // We need the timestamp for the SourceFileCache
                    FILETIME ftTimestamp;
                    GetFileTime(m_hFile, NULL, NULL, &ftTimestamp);

                    // Cache the source file contents to the source file cache so we
                    // don't spends all of our time in CreateFile() to re-read the file.
                    WriteUnicodeSourceFileCache(wszFileContents, cchFileSize, ftTimestamp, IsDetectUTF8WithoutSig);

                    //Microsoft: we used to do the conversion here, but now moved it up so we cache Unicode
                }
                else
                {
                    wszFileContents = (WCHAR*)pnra->Alloc(sizeof(WCHAR));
                    cchFileSize = 0;
                }
            }
            catch ( SehException &ex )
            {
                hr = ex.GetHResult();
            }

            if (pData)
            {
                UnmapViewOfFile(pData);
                pData = NULL;
            }
#if IDE
            // Don't keep the file open if we're in the IDE.
            // Keeping the handle open would mess up the project system and source-code-control.
            if (m_hFile)
            {
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
            }
#endif
        }
    }

    *pwszText = wszFileContents;
    *pcchText = (DWORD)cchFileSize;

    if (FAILED(hr))
    {
        // Kill all existing errors and replace them with the "Unable to load file"
        // error.
        //
#if IDE
        if (!m_pfile->HasFileLoadError())
        {
#endif
            m_pfile->GetErrorTable()->DeleteAll();


            m_pfile->GetErrorTable()->CreateErrorWithError(ERRID_BadModuleFile1,
                                                           NULL,
                                                           hr,
                                                           m_pstrFileName);
#if IDE
            m_pfile->SetHasFileLoadError(true);

        }
#endif

        // The error has been handled.
        hr = NOERROR;
    }

    if (bstrText)
    {
        ::SysFreeString(bstrText);
    }

    return hr;

}

#if IDE

void 
TextFile::UpdateCodePageFromView()
{
    if ( m_pfile->GetSourceFileView() )
    {
        // The high WORD for m_lti may still have bits set so we must ignore these to do a
        // Code Page comparison.
        m_lti = m_pfile->GetSourceFileView()->GetCodePageInfo();
    }
}

#endif


//=============================================================================
// This method checks whether the text file is cached and if so, it returns
// an array to the text buffer and its size (in bytes).
//=============================================================================
bool TextFile::ReadByteSourceFileCache(
    _Inout_ NorlsAllocator * pnra,
    _In_ TextFileCache* pTextFileCache,
    _Inout_ TextFileCacheRef * * ppTextFileCacheRef,
    _In_ STRING *pstrFileName,
    _Out_ BYTE * * pFileContents,
    _Out_ size_t * pcbFileSize)
{
    HRESULT     hr = E_FAIL;
    BYTE*       pFileContentsOut = NULL;
    DWORD      cbFileSize = 0;

    if (ppTextFileCacheRef && *ppTextFileCacheRef)
    {
        NorlsMark mark = pnra->Mark();
        SehGuard guard;
        try
        {
            cbFileSize = (*ppTextFileCacheRef)->GetSize();

            // Get a temp-buffer to read from the cache
            if (cbFileSize > 0)
            {
                pFileContentsOut = (BYTE*)pnra->Alloc(cbFileSize + 1);
                pFileContentsOut[cbFileSize] = 0; // ensure this string is null terminated
            }
            else
            {
                pFileContentsOut = (BYTE*)pnra->Alloc(1);
                pFileContentsOut[0] = 0;
            }

            // We call TextFileCache::Read even if cbFileSize is zero becuase
            // it will validate the timestamp and throw an error if the
            // cache is invalid.
            cbFileSize = pTextFileCache->Read(pstrFileName, *ppTextFileCacheRef, pFileContentsOut, cbFileSize);

            // Return value
            hr = S_OK;
        }
        catch ( SehException& ex )
        {
            hr = ex.GetHResult();

            // We want to avoid vb's exception filter; Just return the hresult.
            if (pFileContentsOut)
            {
                pnra->Free(&mark);
                pFileContentsOut = NULL;
                cbFileSize = 0;
            }

            delete *ppTextFileCacheRef;
            *ppTextFileCacheRef = NULL;
        }
    }

    // Out params
    *pFileContents = pFileContentsOut;
    *pcbFileSize = cbFileSize;

    return SUCCEEDED(hr);
}
//=============================================================================
// This method is for use by consumers of the cache who deal with Unicode files.
//=============================================================================

bool TextFile::ReadUnicodeSourceFileCache(
    _Inout_ NorlsAllocator * pnra,
    _In_ TextFileCache* pTextFileCache,
    _Inout_ TextFileCacheRef * * ppTextFileCacheRef,
    _In_ STRING *pstrFileName,
    _Out_ _Deref_post_cap_(*pcchFileSize) WCHAR ** pFileContents,
    _Out_ size_t * pcchFileSize)
{
    size_t  cbFileSize;
    bool    retval;

    retval = ReadByteSourceFileCache(pnra, pTextFileCache, ppTextFileCacheRef, pstrFileName, (BYTE**)pFileContents, &cbFileSize);
    *pcchFileSize = cbFileSize / sizeof(WCHAR);
    if (retval == true) {  // confirm that cache text was Unicode
        VSASSERT(ppTextFileCacheRef && *ppTextFileCacheRef && (*ppTextFileCacheRef)->IsUnicode(), "ReadUnicodeSourceFileCache on a non-Unicode file");
    }
    return retval;
}

//=============================================================================
// This stores the provided array in the cache (file).  It expects a BYTE array
// and the size in bytes.
// ============================================================================

void TextFile::WriteByteSourceFileCache(
    const BYTE * pFileContents,
    size_t cbFileSize,
    const FILETIME &timestamp,
    const bool IsDetectUTF8WithoutSig)
{
    // Get rid of any stale cache pointer
    ClearSourceFileCache();

    // Try to write the new data for this file.
    SehGuard guard;
    try
    {
        m_pTextFileCacheRef = m_pfile->GetProject()->GetSourceFileCache()->Append(
            (LPVOID)pFileContents, (DWORD)cbFileSize, timestamp, IsDetectUTF8WithoutSig);
#if IDE
        AssertIfNull(m_pTextFileCacheRef);
#endif
    }
    catch ( SehException& )
    {
        // We can eat any errors that occur. It isn't critical if the
        // cache cannot be written. TextFile will just get the file from disk next time.
        VSASSERT(FALSE, "Unable to write source file cache.");
    }
}

//=============================================================================
//This is for use by consumers who store Unicode files.  Pass a WCHAR* and the
//size of the array in Unicode characters.
//=============================================================================

void TextFile::WriteUnicodeSourceFileCache(
    _In_count_(cchFileSize)const WCHAR * pFileContents,
    size_t cchFileSize,
    const FILETIME &timestamp,
    const bool IsDetectUTF8WithoutSig)
{
    WriteByteSourceFileCache((BYTE*)pFileContents, cchFileSize * sizeof(WCHAR), timestamp, IsDetectUTF8WithoutSig);
#if IDE
    if (m_pTextFileCacheRef != nullptr)
    {
#endif
        AssertIfNull(m_pTextFileCacheRef);
        m_pTextFileCacheRef->SetUnicode(true);
#if IDE
    }
#endif
}

void TextFile::ClearSourceFileCache()
{
    delete m_pTextFileCacheRef;
    m_pTextFileCacheRef = NULL;
}

#if IDE
bool TextFile::DifferentCacheAndDiskTimestamps() const 
{
    bool fDiffTimeStamps = false;
    FILETIME ftdisk , ftcache;

    if (m_pTextFileCacheRef)
    {
        ftcache =  m_pTextFileCacheRef->GetTimestamp();
        if (GetFileModificationTime(m_pstrFileName , &ftdisk))
        {
            if (ftcache.dwLowDateTime != ftdisk.dwLowDateTime ||
                ftcache.dwHighDateTime != ftdisk.dwHighDateTime)
            {
                fDiffTimeStamps = true;
            }
        }
    }

    return fDiffTimeStamps;
}
#endif IDE


//============================================================================
// Close the file handle for the file.
//============================================================================

void TextFile::CloseFile()
{
    CompilerIdeLock spLock(m_TextFileCriticalSection);

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
    if (m_bstrBuffer != NULL)           // VBA in-memory buffer
        ::SysFreeString(m_bstrBuffer);

    ClearSourceFileCache();

    spLock.Unlock();
}

//============================================================================
// LoadTextFileFromBytes
//============================================================================
HRESULT TextFile::LoadTextFileFromBytes(
    _Inout_ NorlsAllocator * pnra,  // allocator
    const BYTE * pData,  // text file bytes
    DWORD dwSize,  // text file number of bytes
    LTI_INFO lti,  // Instructions for format, detection
    _Out_opt_ LTI_INFO * plti,  // [out] format used
    __deref_out_ecount_opt(* pcchFileSize)WCHAR * * pwszFileContents,  // [out] unicode characters
    _Out_ size_t * pcchFileSize /* [out] number of unicode characters */)
{
    HRESULT     hr = S_OK;
    LTI_INFO    ltiOut = 0;
    WCHAR       *wszFileContentsOut = NULL;
    size_t      cchFileSizeOut = 0;

    if (dwSize != 0 && pData != NULL)
    {
        NorlsMark   mark;

        pnra->Mark(&mark);
        IfFailThrow(GetImageFormat2(lti, dwSize, pData, &ltiOut));
        IfFailThrow(TextImageUnicodeSize(ltiOut, dwSize, pData, (DWORD*)&cchFileSizeOut));

        if (cchFileSizeOut == 0)
        {
            // Bug VSWhidbey 386072
            //
            wszFileContentsOut = (WCHAR*)pnra->Alloc(sizeof(WCHAR));
            *wszFileContentsOut = L'\0';
            cchFileSizeOut = 0;
            hr = S_OK;
        }
        else
        {
            wszFileContentsOut = (WCHAR*)pnra->Alloc(VBMath::Multiply(
                cchFileSizeOut, 
                sizeof(WCHAR)));
            IfFailThrow(TextImageToUnicode(ltiOut, false, dwSize, pData, (DWORD)cchFileSizeOut, wszFileContentsOut));
        }
    }
    else
    {
        wszFileContentsOut = (WCHAR*)pnra->Alloc(sizeof(WCHAR));
        *wszFileContentsOut = L'\0';
        cchFileSizeOut = 0;
        hr = S_OK;
    }

    *pwszFileContents = wszFileContentsOut;
    *pcchFileSize = cchFileSizeOut;

    if (plti != NULL)
        *plti = ltiOut;

    return hr;
}


// Compute MD5 hash for PDB check sum by using vscommon\crypthash.lib
// this function mimics GetTextFile(), make sure they are in sync when new text sources are added

HRESULT TextFile::GetCryptHash(
    _Out_opt_ void * pvHashValue,
    DWORD cbHashSize)
{
    HRESULT hr = NOERROR;
    HRESULT hrCrypt = E_FAIL;
    HRESULT hrPersistent = NOERROR;
    void *pvText = NULL;
    size_t cbText = 0;
    BYTE* pFileContents = NULL;
    CryptHash *psh = NULL;
    HANDLE hMap = NULL;
    BYTE* pData = NULL;
    bool newHFile = false;
    bool fFileViewAvailable = false;
    DWORD cbHashActualSize = 0;

    IfFalseRet(pvHashValue && (cbHashSize == CRYPT_HASHSIZE),E_INVALIDARG);

    // Keeps track if we got information out of the file.  We need this to
    // ensure that we aren't trying to get information out of a buffer
    // that has been created but not yet loaded.  If the size of the
    // buffer is zero and it hasn't been modified, then we will go out to
    // the file to get its information.
    //

    // Always init outparams
    //*pvHashValue = NULL;


    CompilerIdeLock spLock(m_TextFileCriticalSection);
    SehGuard guard;
    try
    {

        // Initialize the iterator if the file was loaded.  We create a copy
        // of the buffer to minimze the time that we keep it locked.  Otherwise
        // the user could not edit at all (even in the commit buffer) while
        // this object remained alive.
        //
        // We also need to take the commit buffer into account.  The
        // text we get from pTextScanner will include the user's view of
        // the commit buffer.  We need to strip that out and replace it
        // with the text we want the compiler to see.
        //
#if IDE
        hrPersistent = SourceFileView::GetCheckSum(m_pfile, (BYTE *)pvHashValue, cbHashSize, &cbHashActualSize, &fFileViewAvailable);

        if (FAILED(hrPersistent))
        {
            return hrPersistent;
        }

        if (fFileViewAvailable)
        {
            VSASSERT(cbHashSize == cbHashActualSize, "Unexpected crypt hash size!!!");

            // Hash already calculated and available, so leave
            hrCrypt = NOERROR;
            return hrCrypt;
        }

#endif IDE

        // if there is a solution extension, such as MyTemplate or XMLHelper, 
        // then the text is readonly and we share the same one.
        SolutionExtensionData * pSolutionExtensionData = m_pfile->GetSolutionExtension();
        if (pSolutionExtensionData)
        {
            if (pSolutionExtensionData->m_extension !=SolutionExtension::SolutionExtensionNone)
            {
                pvText = pSolutionExtensionData->m_psbCode->GetString();
                cbText = pSolutionExtensionData->m_psbCode->GetStringLength();
            }
        }

// For VS10 we may still need to support the VSA Buffer scenario but for SPD work we do not need this.

        // check to see if we already have an in-memory buffer (given to it via CompilerProject::AddBuffer)
        // for such cases of text files there is no real disk file to step in. For now, the internal Unicode buffer hash
        // is computed.
        // Consider to to convert to the disk file format if necessary in the future. Make sure to extend AddBuffer()
        // to provide the LTI of the disk file to be
        if (!pvText && m_bstrBuffer != NULL)
        {
            if (m_dwBufLen > 0)
            {
                pvText = (void *) m_bstrBuffer;
                cbText = m_dwBufLen;
            }
            else // Empty buffer case--we can just bail.
            {
                pvText = (void*)("");
                cbText = 0;
            }

        }

        if ( !pvText && m_pstrFileName != NULL )
        {
            {
                if (m_pTextFileCacheRef &&
                    m_pTextFileCacheRef->HasCryptHash() &&
                    pvHashValue &&
                    cbHashSize >= CRYPT_HASHSIZE)
                {
                    memcpy(pvHashValue, m_pTextFileCacheRef->GetCryptHash(), CRYPT_HASHSIZE);
                    hrCrypt = NOERROR;
                    return hrCrypt;
                }
            }

            // If the file was not loaded into a buffer or if the buffer
            // was empty, load the text from the original file on disk.
            //
            if (!pvText)
            {
                SehGuard guard;
                try
                {
                    // If we don't already have the file open, open it.
                    if (m_hFile == INVALID_HANDLE_VALUE)
                    {
                        m_hFile = CreateFileW(m_pstrFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
                        IfFalseThrow(m_hFile != INVALID_HANDLE_VALUE);
                        newHFile = true;
                    }

                    // Get the size of the file
                    DWORD dwSize = GetFileSize(m_hFile, NULL);

                    if (dwSize != 0)
                    {
                        hMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
                        IfNullThrow(hMap);
                        pData = (BYTE *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
                        IfNullThrow(pData);

                        pvText = pData;
                        cbText = dwSize;

                    }
                    else
                    {
                        pvText = (void*)("");
                        cbText = 0;
                    }
                }
                catch ( SehException &ex )
                {
                    hr = ex.GetHResult();
                }
            }
        }

        // compute the hash
        if (SUCCEEDED(hr) && pvText && CryptHash::CreateCryptHashAlgorithm(&psh, CALG_MD5))
        {
            if (psh->SetCryptHashData((LPCVOID)pvText,cbText))
            {
                psh->GetCryptHash(pvHashValue,cbHashSize);
                hrCrypt = NOERROR;
            }
            psh->Close();


            if (m_pTextFileCacheRef && pvHashValue)
            {
                m_pTextFileCacheRef->SetCryptHash(pvHashValue);
            }
        }


    }
    catch ( SehException &ex )
    {
        hr = ex.GetHResult();
    }


    if (pData)
    {
        UnmapViewOfFile(pData);
        pData = NULL;
    }
    if (hMap)
    {
        CloseHandle(hMap);
        hMap = NULL;
    }

// 
#if IDE
    // Don't keep the file open if we're in the IDE.
    // Keeping the handle open would mess up the project system and source-code-control.
    if (newHFile && m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
#endif

    if (FAILED(hrPersistent))
    {
        // reading from PersistentLines failed
        hr = hrPersistent;
    }
    else if ( FAILED(hrCrypt))
    {
        // the file can be read but the crypt op failed
        hr = hrCrypt;
    }
     else if (FAILED(hr))
    {
        // read file failure
        // Kill all existing errors and replace them with the "Unable to load file"
        // error.
        m_pfile->GetErrorTable()->DeleteAll();


        m_pfile->GetErrorTable()->CreateErrorWithError(ERRID_BadModuleFile1,
                                                       NULL,
                                                       hr,
                                                       m_pstrFileName);

        // The error has been handled.
        // no crypt op has been applied
        // keep hr to be returned
    }

    // else: persistent, crypt,  file read, all successful
    return hr;

}

//============================================================================
// TemporaryFile::TemporaryFile
//============================================================================
TemporaryFile::TemporaryFile()
{
    m_hFile = INVALID_HANDLE_VALUE;
}

//============================================================================
// TemporaryFile::~TemporaryFile
// The destructor closes the temp file if it is open.
//============================================================================
TemporaryFile::~TemporaryFile()
{
    Close();
}

//============================================================================
// TemporaryFile::GetUniqueTemporaryFileName
// Appends a new file name to the given StringBuffer.  This name has the
// optional given prefix followed by a string representation of a new GUID
// created for the purpose of this file name and finally the suffix ".TMP"
//
// No characters are appended to the given StringBuffer if the call fails.
// (i.e. if hr returned by CoCreateGuid is an error)
//============================================================================
HRESULT TemporaryFile::GetUniqueTemporaryFileName(StringBuffer& sb, _In_z_ PCWSTR wszPrefix)
{
    GUID guidTempFileName;
    HRESULT hr = CoCreateGuid(&guidTempFileName);

    if (SUCCEEDED(hr))
    {
        // No need to check wszPrefix for NULL because AppendString will
        // handle it correctly.
        sb.AppendString(wszPrefix);

        sb.AppendPrintf(L"%X%X%X%X%X%X%X%X%X%X%X",
                        guidTempFileName.Data1,
                        guidTempFileName.Data2,
                        guidTempFileName.Data3,
                        guidTempFileName.Data4[0], guidTempFileName.Data4[1],
                        guidTempFileName.Data4[2], guidTempFileName.Data4[3],
                        guidTempFileName.Data4[4], guidTempFileName.Data4[5],
                        guidTempFileName.Data4[6], guidTempFileName.Data4[7]);

        sb.AppendString(L".TMP");
    }

    return hr;
}

//============================================================================
// TemporaryFile::Open
// Creates and opens an empty temporary file.
// If the file is already opened, the function is a no-op.
// If the file cannot be created, an os error is thrown.
// Right before the file is closed (via TemporaryFile::Close()) it is marked
// for deletion on close.
//============================================================================
void TemporaryFile::Open
(
    LPCWSTR wszPrefix       // The desired prefix for the temp filename
)
{
    HRESULT hr = S_OK;
    if (!IsValidHandle())
    {
        DWORD       dwSizeOfTempPath;

#pragma warning (push)
#pragma warning (disable:6309 6387) // This call is correct
        dwSizeOfTempPath = GetTempPath(0, NULL);
#pragma warning (pop)

        if (dwSizeOfTempPath)
        {
            TEMPBUF(wszTempPath, WCHAR *, (dwSizeOfTempPath+1) * sizeof(WCHAR));
            IfNullThrow(wszTempPath);

            GetTempPath(dwSizeOfTempPath, wszTempPath);

            StringBuffer sb;
            sb.AppendString(wszTempPath);
            hr = GetUniqueTemporaryFileName(sb, wszPrefix);
            IfFailThrow(hr);

            if (sb.GetStringLength() >= MAX_PATH)
            {
                // Although we could insert "\\?\" at the beginning of
                // the path to make it work with CreateFileW we throw
                // a buffer overflow error instead to be consistent
                // with MetaEmit::ALinkCreateWin32ResFile, which cannot
                // support temp paths longer than MAX_PATH.
                IfFailThrow(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            }

            PCWSTR wszTempFile = sb.GetString();

            // We request DELETE permission here because we're going
            // to mark the file to be deleted on close by setting the
            // FileDispositionInfo in TemporaryFile::Close().
            // We could get almost the same effect by specifying
            // FILE_FLAG_DELETE_ON_CLOSE here, but it doesn't mark
            // the file for deletion before AntiVirus checkers get
            // their callback which can result in unnecessary file
            // scans.
            m_hFile = CreateFileW(wszTempFile,
                    GENERIC_READ | GENERIC_WRITE | DELETE,
                    0, NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
        }

        if (!IsValidHandle())
        {
            IfFailThrow(GetLastHResultError());
        }
    }

}


//============================================================================
// TemporaryFile::GetHandle
// Returns the OS file handle for the temp file.
//============================================================================
HANDLE TemporaryFile::GetHandle() const
{
    return m_hFile;
}

//============================================================================
// TemporaryFile::IsValidHandle
// Returns whether the OS file handle for the temp file is valid.
//============================================================================
bool TemporaryFile::IsValidHandle() const
{
    return m_hFile != INVALID_HANDLE_VALUE;
}

//============================================================================
// TemporaryFile::Close
// Closes the temp file. The OS will delete the temp file on close.
//============================================================================
void TemporaryFile::Close()
{
    if (IsValidHandle())
    {
        FILE_DISPOSITION_INFO fileInfo;
        fileInfo.DeleteFile = TRUE;

        // We call SetFileInformationByHandle here to ensure the file is already marked for
        // deletion before closing the handle. Using FILE_FLAG_DELETE_ON_CLOSE doesn't mark the
        // file for deletion before AntiVirus checkers get their callback which can result in unnecessary
        // file scans.
        if (!SetFileInformationByHandle(
            m_hFile,
            FileDispositionInfo,
            (LPVOID)&fileInfo,
            sizeof(fileInfo)))
        {
            IfFailThrow(GetLastHResultError());
        }

        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

//============================================================================
// TextFileCacheRef::TextFileCacheRef
// Initializes a blank TextFileCacheRef.
//============================================================================
TextFileCacheRef::TextFileCacheRef()
{
    m_Offset = 0;
    m_Size = 0;
    m_IsUnicode = false;
    m_HasCryptHash = false;
    m_DetectUTF8WithoutSig = true;
    m_Timestamp = TextFileCache::NullFileTime;
}

//============================================================================
// TextFileCacheRef::TextFileCacheRef
//============================================================================
TextFileCacheRef::TextFileCacheRef(
    LONG offset,
    DWORD size,
    const FILETIME &timestamp,
    const bool IsDetectUTF8WithoutSig)
{
    m_Offset = offset;
    m_Size = size;
    m_IsUnicode = false;
    m_HasCryptHash = false;
    m_DetectUTF8WithoutSig = IsDetectUTF8WithoutSig;
    m_Timestamp = timestamp;
}

//============================================================================
// TextFileCacheRef::operator==
//============================================================================
bool TextFileCacheRef::operator ==(const TextFileCacheRef &e) const
{
    return (m_Offset == e.m_Offset &&
            m_Size == e.m_Size &&
            m_Timestamp.dwLowDateTime == e.m_Timestamp.dwLowDateTime &&
            m_Timestamp.dwHighDateTime == e.m_Timestamp.dwHighDateTime);
}

//============================================================================
// TextFileCacheRef::operator!=
//============================================================================
bool TextFileCacheRef::operator !=(const TextFileCacheRef &e) const
{
    return !(*this == e);
}

//============================================================================
// TextFileCache::EntryHeader::EntryHeader
//============================================================================
TextFileCache::EntryHeader::EntryHeader(
    DWORD size,
    const FILETIME &timestamp)
{
    m_Size = size;
    m_Timestamp = timestamp;
};

//============================================================================
// TextFileCache::EntryHeader::EntryHeader
//============================================================================
TextFileCache::EntryHeader::EntryHeader()
{
    m_Size = 0;
    m_Timestamp = TextFileCache::NullFileTime;
};

//============================================================================
// TextFileCache::NullFileTime
//============================================================================
const FILETIME TextFileCache::NullFileTime = {0, 0};

//-------------------------------------------------------------------------------------------------
//
// Determine if the underlying file for the cache is currently open
//
//-------------------------------------------------------------------------------------------------
bool
TextFileCache::IsOpen()
{
    return m_TempFile.IsValidHandle();
}

//============================================================================
// TextFileCache::EnsureOpen
// Opens the temporary file for the TextFileCache.
// Will throw an error if the file cannot be opened.
//
// The TextFileCache is now loaded on demand for performance reasons.  The 
// key scenario is the EE.  The EE uses CompilerProject as a place and
// context in which to perform an evaluation.  However intstead of reading
// source code from a file, it directly hands parse trees to the Semantics
// engine.  This renders the creation of a temporary file un-necessary.
//
// The creation of a temporary file turns out to be a significant cost for
// debugger stepping scenarios.  In particular in calculating the contents
// of the stack window.  The VB EE acounts for around 10% of the entire scenario
// with the creation of the temorpary file accountig for 3-5% of the EE's
// 10%.  So now we delay the creation of the temporary file until it's 
// actually needed
//
//============================================================================
void
TextFileCache::EnsureOpen()
{
    // Protect access to the temp file
    CompilerIdeLock lock(m_CriticalSection);
    if ( !IsOpen() )
    {
        m_TempFile.Open(L"vbc");    
    }
}

//============================================================================
// TextFileCache::Close
// Closes the temporary file for the TextFileCache.
//============================================================================
void TextFileCache::Close()
{
    // Protect access to the temp file
    CompilerIdeLock lock(m_CriticalSection);
    m_TempFile.Close();
}

//============================================================================
// TextFileCache::Append
// Appends a block to the file cache. The bock consists of a header
// that contains the timestamp and length of the block, followed
// by the actual data bytes.
// A new TextFileCacheRef is returned which is used to read this block
// back out of the cache.
//============================================================================
TextFileCacheRef * TextFileCache::Append(
    LPVOID lpBuffer,
    DWORD cbBytes,
    const FILETIME &ftTimestamp,
    const bool IsDetectUTF8WithoutSig)
{
    EnsureOpen();
    TextFileCacheRef* pTextFileCacheRef = NULL;
    HRESULT hr = S_OK;

    {
        // Protect access to the temp file
        CompilerIdeLock lock(m_CriticalSection);

        LONG                newOffset;
        DWORD               bytesWritten;
        EntryHeader         header(cbBytes, ftTimestamp);

        // Seek to the end of the cache file
        newOffset = SetFilePointer(m_TempFile.GetHandle(), 0, NULL, FILE_END);
        if ((newOffset == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
        {
            VbThrow(GetLastHResultError());
        }
#if IDE
        if (newOffset < 0)
        {
            // Dev11 344896
            // Our file size went over 2GB.  Let's throw away this temp file.  A new one will be created
            // and all the text files will be reread from disk until they've each been placed in the
            // new cache.  This will cause some asserts, but should only occur in stress scenarios.
            m_TempFile.Close();
            VbThrowNoAssert(E_FAIL);
        }
#endif
        // Write the header element
        if (!WriteFile(m_TempFile.GetHandle(), (LPVOID)&header, sizeof(header), &bytesWritten, NULL))
        {
            VbThrow(GetLastHResultError());
        }

        // Write the file text
        if (!WriteFile(m_TempFile.GetHandle(), lpBuffer, cbBytes, &bytesWritten, NULL))
        {
            VbThrow(GetLastHResultError());
        }

        // Success. Allocate the output object
        pTextFileCacheRef = new (zeromemory) TextFileCacheRef(newOffset, cbBytes, ftTimestamp, IsDetectUTF8WithoutSig);
        IfNullThrow(pTextFileCacheRef);
    }

    if (FAILED(hr))
    {
        VbThrow(hr);
    }

    return pTextFileCacheRef;
}

//============================================================================
// TextFileCache::Read
// Reads a block of data out of the cache file. This version doesn't test
// whether the block is out-of-date with respect to the file that it
// came from.
//============================================================================
DWORD TextFileCache::Read(
    const TextFileCacheRef * pRef,
    LPVOID lpBuffer,
    DWORD cbBytes)
{
    return Read(NULL, pRef, lpBuffer, cbBytes);
}

//============================================================================
// TextFileCache::Read
// Reads a block of data out of the cache file.
// If wszFileName is non-null, it will check whether the last-write-time for
// the file is equal to the value specified in pRef. If not, it will
// throw an error.
// The header block in the cache is also tested against pElelement. If the
// size or timesamp don't macth, it will throw an error.
//============================================================================
DWORD TextFileCache::Read
(
    LPCWSTR                 wszFileName,
    const TextFileCacheRef* pRef,
    LPVOID                  lpBuffer,
    DWORD                   cbBytes
)
{
    EnsureOpen();
    DWORD bytesRead = 0;
    HRESULT hr = S_OK;

    {
        // Protext access to the temp file
        CompilerIdeLock lock(m_CriticalSection);

        LONG newOffset;

        // Check if the cache timestamp is curent
        if (wszFileName)
        {
            FILETIME ftCache = pRef->GetTimestamp();
            FILETIME ftCurrent;
            GetFileModificationTime(wszFileName, &ftCurrent);

            if (ftCurrent.dwLowDateTime != ftCache.dwLowDateTime ||
                ftCurrent.dwHighDateTime != ftCache.dwHighDateTime)
            {
                VbThrowNoAssert(E_FAIL);
            }
        }

        // Find and reaad the cache element.
        newOffset = SetFilePointer(m_TempFile.GetHandle(), pRef->GetOffset(), NULL, FILE_BEGIN);
        if ((newOffset == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
        {
            VbThrow(GetLastHResultError());
        }

        // Read in the header
        EntryHeader header;
        if (!ReadFile(m_TempFile.GetHandle(),&header, sizeof(header), &bytesRead, NULL))
        {
            VbThrow(GetLastHResultError());
        }

        TextFileCacheRef thisRef(newOffset, header.m_Size, header.m_Timestamp);

        // Verify that the data matches the requested TextFileCacheRef
        if (thisRef != *pRef)
        {
            VbThrow(E_FAIL);
        }

        if (cbBytes > 0)
        {
            // Verify that the caller's buffer is large enough
            if (cbBytes < header.m_Size)
            {
                VbThrow(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
            }

            // Read the data
            if (!ReadFile(m_TempFile.GetHandle(), lpBuffer, header.m_Size, &bytesRead, NULL))
            {
                VbThrow(GetLastHResultError());
            }
        }
        else
        {
            bytesRead = 0;
        }
    }

    // re-throw errors to the caller
    if (FAILED(hr))
    {
        VbThrow(hr);
    }

    return bytesRead;
}

