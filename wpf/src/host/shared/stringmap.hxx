//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines a list convenience class, for use with a small number of
//     elements.
//
// History:
//      2005/06/19 - Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

#ifndef STRINGMAP_HXX_INCLUDED
#define STRINGMAP_HXX_INCLUDED

#include "string.hxx"
#include "miscmacros.hxx"

template<class T>
class CStringMap
{
public:
    class CStringMapNode
    {
    public:
        CStringMapNode(CStringMapNode* pNext, LPCWSTR key, T tValue)
        {
            m_pNext = pNext;
            SetKey(key);
            m_tValue = tValue;
        }
        STRING_PROP(Key);
        T GetValue() { return m_tValue; }
        CStringMapNode* GetNext() { return m_pNext; }

    private:
        CString m_strKey;
        T m_tValue;
        CStringMapNode* m_pNext;
    };

public:
    CStringMap()
    {
        m_pRoot = NULL;
        m_dwCount = 0;
    }

    ~CStringMap()
    {
        CStringMapNode* pNode = m_pRoot;

        while (pNode)
        {
            CStringMapNode* pNext = pNode->GetNext();
            delete pNode;
            pNode = pNext;
        }
    }

public:
    HRESULT Add(LPCWSTR pwzKey, T tValue)
    {
        HRESULT hr = S_OK;

        if (!pwzKey)
        {
            CKHR(E_INVALIDARG);
        }

        m_pRoot = new CStringMapNode(m_pRoot, pwzKey, tValue);
        CK_ALLOC(m_pRoot);
        ++m_dwCount;

    Cleanup:
        return hr;
    }

    HRESULT Find(LPCWSTR pwzKey, T* ptValue)
    {
        HRESULT hr = S_OK;

        CStringMapNode* pNode = m_pRoot;

        while (pNode)
        {
            if (_wcsicmp(pwzKey, pNode->GetKey()) == 0)
            {
                *ptValue = pNode->GetValue();
                break;
            }

            pNode = pNode->GetNext();
        }

        if (!pNode)
        {
            CKHR(E_FAIL);
        }

    Cleanup:
        return hr;
    }

    CStringMapNode* GetRoot() { return m_pRoot; }

    DWORD GetCount() { return m_dwCount; }

private:
    CStringMapNode* m_pRoot;
    DWORD m_dwCount;
};

#endif //STRINGMAP_HXX_INCLUDED
