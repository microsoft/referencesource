//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Class to manage mapping and unmapping files.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// Returns the file creation time of the given file.  If the file cannot be opened
// or queried, function returns false and sets OUT param to all zeros.
bool GetFileModificationTime(
    LPCWSTR pszFileName,
    FILETIME * pfiletimeModified);

// Opens file for reading and also returns the file size if pcbFile != NULL.
HANDLE OpenFileEx(
    LPCWSTR pszFileName,
    _Out_opt_ DWORD * pcbFile);

class MapFile
{
public:
    // Constructor & destructor
    MapFile()  
    {
        ClearMembers(); 
    }

    ~MapFile() 
    {
        Unmap(); 
    }

    // Call Map(wszFileName) to map file as read-only.  (Read/write NYI.)
    void Map(
        LPCWSTR wszFileName,
        ErrorTable * pErrorTable,
        ERRID errid = ERRID_UnableToOpenFile1);

    // Call Unmap to unmap the file.  This is a nop (not an error) if
    // the MapFile isn't currently mapping a file.
    void Unmap();

    // Clients should only care about m_pbMapAddr and m_cbFile,
    // (which are, respectively, where the file was mapped to, and the
    // file's length in bytes).  So we provide getters for those
    // fields, but not for m_hFile or m_hMapFile.
    PBYTE PbMapAddr() 
    {
        return m_pbMapAddr; 
    }

    DWORD CbFile()    
    {
        return m_cbFile; 
    }

    // FMapped returns true iff this MapFile is currently mapping a file
    bool FMapped()
    {
        return m_fMapped; 
    }

    // ClearMembers is used to set all of the MapFile's data members to
    // default values.  Clients should not call this directly, but we
    // need to leave it as public to alloc access to MetaEmit::AssemImport.
    void ClearMembers();

private:

    HANDLE m_hFile;
    PBYTE  m_pbMapAddr;
    HANDLE m_hMapFile;
    DWORD  m_cbFile;

    // We need a special m_fMapped member to allow mapping of
    // zero-byte-long files.  We used to use m_pbMapAddr as
    // a proxy for this, but the system won't let you map
    // empty files.
    bool   m_fMapped;
};
