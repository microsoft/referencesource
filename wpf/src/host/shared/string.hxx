//------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines a metered string convenience class
//
// History:
//      2005/06/19 - Microsoft
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

#ifndef STRING_HXX_INCLUDED
#define STRING_HXX_INCLUDED

#include <WinInet.h> //INTERNET_MAX_URL_LENGTH

struct IStream;

class CString
{
    // Copy not implemented. We don't have a good way to respond to allocation errors.
    CString(const CString &src);
    void operator=(const CString &src);
public:
    CString(size_t maxLength = INTERNET_MAX_URL_LENGTH + 1);
    ~CString();

    LPCWSTR GetValue() const { return m_pwzValue; }
    HRESULT SetValue(__in_opt LPCWSTR pszValue);

    /// <summary>
    /// Set value from a substring
    /// </summary>
    /// <param name="pszValue">Source string, may not be null-terminated</param>
    /// <param name="length">Number of characters to copy</param>
    /// <remarks>
    /// The caller should guarantee that pszValue[0]..pszValue[length-1] is a valid
    /// and accessible range of memory.
    /// </remarks>
    HRESULT SetValue(__in_opt LPCWSTR pszValue, size_t length);

    static CString* CreateOnHeap(LPCWSTR pszValue, size_t maxLength = INTERNET_MAX_URL_LENGTH + 1);

    size_t Length() const { return m_curLength; }

    HRESULT WriteToStream(IStream* pOutputStream) const;
    HRESULT ReadFromStream(IStream* pInputStream);

    /// <summary> 
    /// Case sensitive comparison
    /// </summary> 
    inline bool operator== (LPCWSTR lpszOther) const
    {
        if (m_pwzValue != nullptr)
        {
            return wcscmp(m_pwzValue, lpszOther) == 0;
        }

        // true if both are nullptr; otherwise false
        return m_pwzValue == lpszOther;
    }

    /// <summary>
    /// Case sensitive comparison
    /// </summary> 
    inline bool operator== (const CString& strOther) const
    {
        return *this == strOther.GetValue();
    }

    /// <summary> 
    /// Cast/conversion operator to convert a <c>CString</c> instance to <c>LPCWSTR</c>
    /// </summary>
    /// <remarks> 
    /// Identical to <see cref="GetValue()" />, but more convenient in some 
    /// situations due to the potential for implicit conversions from <c>CString</c>
    /// to <c>LPCWSTR</c>
    /// </remarks>
    inline operator LPCWSTR() const
    {
        return GetValue();
    }

private:
    void Free();

    LPWSTR m_pwzValue;
    size_t m_maxLength, m_curLength;
};

// This defines a plain string property. 
#define STRING_PROP(name) \
    HRESULT Set##name(__in_opt LPCWSTR pszValue) { return m_str##name.SetValue(pszValue); } \
    LPCWSTR Get##name() const { return m_str##name.GetValue(); } \
    CString* name() { return &m_str##name; }

#endif
