//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements a metered string convenience class
//
// History:
//      2005/06/19 - Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#include "Precompiled.hxx"
#include "String.hxx"
#include <wchar.h>
#include <StrSafe.h>
#include <Objidl.h> //IStream
#include <algorithm>
#include <iterator>


///////////////////////////////////////////////////////////////////////////////////////////
// CString

CString::CString(size_t maxLength)
{
    m_maxLength = maxLength;
    m_pwzValue = NULL;
    m_curLength = 0;
}

CString::~CString()
{
    Free();
}

HRESULT CString::SetValue(__in_opt LPCWSTR pwzSource)
{
    HRESULT hr = S_OK;

    Free();

    if (pwzSource != NULL)
    {
        CKHR(StringCchLengthW(pwzSource, m_maxLength, &m_curLength));
        m_pwzValue = new wchar_t[m_curLength+1];
        if (!m_pwzValue)
        {
            m_curLength = 0;
            CKHR(E_OUTOFMEMORY);
        }
        wcscpy_s(m_pwzValue, m_curLength+1, pwzSource);
    }

Cleanup:
   
    return hr;
}

/// <summary>
/// Set value from a substring
/// </summary>
/// <param name="pwzSource">Source string, may not be null-terminated</param>
/// <param name="length">Number of characters to copy</param>
/// <remarks>
/// Reading from a non-null terminated string can lead to errors, for e.g., an access
/// violation. It is the responsibility of the caller to pass a valid source buffer and 
/// <paramref name="length" /> parameters. 
/// </remarks>
HRESULT CString::SetValue(__in_opt LPCWSTR pwzSource, size_t length)
{
    HRESULT hr = S_OK;

    Free();
    if (pwzSource != nullptr)
    {
        if (length + 1 > m_maxLength)
        {
            // The requested substring is too long
            //  Mimic the behavior of StringCchLength and return
            //  STRSAFE_E_INVALID_PARAMETER
            CKHR(STRSAFE_E_INVALID_PARAMETER);
        }

        m_curLength = length;
        m_pwzValue = new wchar_t[length + 1];
        if (m_pwzValue == nullptr)
        {
            m_curLength = 0;
            CKHR(E_OUTOFMEMORY);
        }

        // String copying functions like wcscpy_s and StringCchCopyW require 
        // the source string buffer to be null-terminated. The best we can do when 
        // working with a (potentially) non-null terminated buffer is to treat it as if
        // it were an array
        // 
        // Copy from half-open range [pwzSource, pwzSource + length) onto the buffer m_pwzValue, 
        // then null-terminate the buffer
        std::copy(pwzSource, pwzSource + length, stdext::checked_array_iterator<wchar_t*>(m_pwzValue,length + 1));
        m_pwzValue[length] = L'\0';
    }

Cleanup:
    return hr;
}

/*static*/ CString* CString::CreateOnHeap(LPCWSTR pszValue, size_t maxLength)
{
    CString *p = new CString(maxLength);
    if(p)
    {
        if(FAILED(p->SetValue(pszValue)))
        {
            delete p;
            p = 0;
        }
    }
    return p;
}

void CString::Free()
{
    if (m_pwzValue)
    {
        delete [] m_pwzValue;
        m_pwzValue = NULL;
    }
    m_curLength = 0;
}

HRESULT CString::WriteToStream(IStream* pOutputStream) const
{
    HRESULT hr = S_OK;
   
    ULONG bytesWritten = 0;

    CKHR(pOutputStream->Write((void*)&m_curLength, sizeof(m_curLength), &bytesWritten));

    if(m_curLength)
    {
        ASSERT(m_pwzValue);
        CKHR(pOutputStream->Write((void*)m_pwzValue, (ULONG) m_curLength * sizeof(WCHAR), &bytesWritten));
    }

Cleanup:
    return hr;
}

HRESULT CString::ReadFromStream(IStream* pInputStream)
{
    HRESULT hr = S_OK;

    Free();

    size_t length = 0;
    ULONG bytesRead = 0;

    CKHR(pInputStream->Read((void*)&length, sizeof(length), &bytesRead));

    if (length)
    {
        ULONG charsToRead = (ULONG) (length > m_maxLength ? m_maxLength : length);
    
        m_pwzValue = new wchar_t[charsToRead + 1];
        CK_ALLOC(m_pwzValue);

        CKHR(pInputStream->Read((void*)m_pwzValue, charsToRead * sizeof(WCHAR), &bytesRead));
        m_pwzValue[charsToRead] = L'\0';

        m_curLength = charsToRead;
    }

Cleanup:
    if (FAILED(hr))
    {
        Free();
    }
    return hr;
}
