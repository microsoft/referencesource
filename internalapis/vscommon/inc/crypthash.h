//--------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation Inc.
// All rights reserved
//
// CryptHash.h
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma once
#endif

#ifndef __CRYPTHASH_H__
#define __CRYPTHASH_H__

// Class definition
//
class CryptHash 
{
public:
    // Create a CryptHash object with the usual two-stage construction technique
    //
    static bool WINAPI CreateCryptHash(OUT CryptHash **);
    static bool WINAPI CreateCryptHashAlgorithm(OUT CryptHash **, IN ALG_ID AlgorithmId, HCRYPTPROV hProv = NULL);

    // Accumulate more bytes into the hash. pvBuf contains the source code.
    //
    bool SetCryptHashData(IN LPCVOID pvBuf, IN size_t cbBuf);

    // Query the size of the hash 
    //
    DWORD GetCryptHashSize() const;

    // Copy the hash bytes to the client buffer
    //
    bool GetCryptHash(OUT void *pvHash, IN DWORD cbHash) const;

    // Verify the incoming hash against a target buffer of bytes
    // returning 1 it matches, 0 it doesn't, or -1 for indeterminate.
    //
    int CompareCryptHash(IN DWORD cbHash,IN LPCVOID pvHash,IN size_t cbBuf,IN LPCVOID pvBuf);

    // Reset
    //
    bool Reset();

    // Close off and release this object
    //
    void Close();

private:
    HCRYPTPROV  m_hProv;                    // crypto provider handle
    HCRYPTHASH  m_hHash;                    // crypto hash key handle
    ALG_ID      m_AlgorithmId;
    bool        m_bOwnsProv;

    CryptHash(ALG_ID AlgorithmId = 0, HCRYPTPROV hProv = NULL) : 
        m_hProv(hProv),
        m_hHash(NULL), 
        m_AlgorithmId(AlgorithmId),
        m_bOwnsProv(hProv == NULL)
    {
    }
    ~CryptHash() 
    {
        UnInit();
    }

    bool Init();
    void UnInit();
};

#endif // __CRYPTHASH_H__
