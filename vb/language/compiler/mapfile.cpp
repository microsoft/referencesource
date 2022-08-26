//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of class to manage mapping and unmapping files.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//============================================================================
// Returns the file modification time of the given file.  If the file cannot be opened
// or queried, function returns false and sets OUT param to all zeros.
//============================================================================
bool GetFileModificationTime(
    LPCWSTR pszFileName,
    FILETIME * pfiletimeModified)
{
    bool bRet;
    WIN32_FIND_DATA find_data;

    // Assume failure
    bRet = false;
    memset(pfiletimeModified, 0, sizeof(*pfiletimeModified));

    HANDLE h = ::FindFirstFile(pszFileName, &find_data);
    if (h != INVALID_HANDLE_VALUE)
    {
        ::FindClose(h);
        *pfiletimeModified = find_data.ftLastWriteTime;
        bRet = true;
    }

    return bRet;
}


//============================================================================
// Opens file for reading and also returns the file size if pcbFile != NULL.
//============================================================================
HANDLE OpenFileEx(
    LPCWSTR pszFileName,
    _Out_opt_ DWORD * pcbFile)
{
    HANDLE hfile;

    hfile = CreateFileW(
                    pszFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL);

    if (hfile == INVALID_HANDLE_VALUE || !pcbFile)
    {
        return hfile;
    }

    *pcbFile = GetFileSize(hfile, NULL);

    if (*pcbFile == INVALID_FILE_SIZE)
    {
        // GetFileSize failed
        CloseHandle(hfile);
        hfile = INVALID_HANDLE_VALUE;
    }

    return hfile;
}

//============================================================================
// Maps the given file.  Currently, only read-only mappings are supported.
//============================================================================
void MapFile::Map(
    LPCWSTR wszFileName,
    ErrorTable * pErrorTable,
    ERRID errid)
{
    HRESULT hr = NOERROR;

    // Make sure this MapFile isn't already in use
    VSASSERT(m_hFile     == INVALID_HANDLE_VALUE, "MapFile already in use.");
    VSASSERT(m_pbMapAddr == NULL, "MapFile already in use.");
    VSASSERT(m_hMapFile  == NULL, "MapFile already in use.");
    VSASSERT(m_cbFile    == 0,    "MapFile already in use.");

    m_hFile = OpenFileEx(wszFileName, &m_cbFile);
    IfTrueGoLast(m_hFile == INVALID_HANDLE_VALUE);

    if (m_cbFile == 0)
    {
        // File is empty.  No other work to do.
    }
    else
    {
        m_hMapFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        IfFalseGoLast(m_hMapFile);

        m_pbMapAddr = (PBYTE)MapViewOfFile(m_hMapFile, FILE_MAP_READ, 0, 0, 0);
        IfFalseGoLast(m_pbMapAddr);
    }

    m_fMapped = true;

    return;

Error:

    // Unmap the file so MapFile.FMapped() can be called to see if the mapping failed.
    Unmap();

    if (pErrorTable)
    {
        pErrorTable->CreateErrorWithError(errid,
                                          NULL,
                                          hr,
                                          wszFileName);
    }
    else
    {
        VbThrow(hr);
    }
}

//============================================================================
// Unmaps the given MapFile.  This is a nop (not an error) if
// the MapFile isn't currently mapping a file.
//============================================================================
void MapFile::Unmap()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hFile);
    }

    if (m_hMapFile)
    {
        CloseHandle(m_hMapFile);
    }

    if (m_pbMapAddr)
    {
        UnmapViewOfFile(m_pbMapAddr);
    }

    // Reset all members to pristine state
    ClearMembers();
}

//============================================================================
// Private method used to set all of the MapFile's data members to
// default values.  Clients should not call this directly.
//============================================================================
void MapFile::ClearMembers()
{
    m_hFile     = INVALID_HANDLE_VALUE;
    m_pbMapAddr = NULL;
    m_hMapFile  = NULL;
    m_cbFile    = 0;
    m_fMapped   = false;
}
