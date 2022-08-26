// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
// ---------------------------------------------------------------------------

#include "stdafx.h"

#include "TypeNameBuilder.h"

HRESULT TypeNameBuilder::OpenGenericArguments()
{
    if (!CheckParseState(ParseStateNAME))
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    m_parseState = ParseStateSTART;
    m_instNesting ++;
    m_bFirstInstArg = TRUE;

    if (m_bUseAngleBracketsForGenerics)
    {
        Append(L'<');
    }
    else
    {
        Append(L'[');
    }

    return hr;
}

HRESULT TypeNameBuilder::CloseGenericArguments()
{
    if (!m_instNesting)
    {
        return E_FAIL;
    }

    if (!CheckParseState(ParseStateSTART))
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    m_parseState = ParseStateGENARGS;

    m_instNesting --;

    if (m_bFirstInstArg)
    {
        m_sb.Truncate(m_sb.GetStringLength() - 1);
    }
    else
    {
        if (m_bUseAngleBracketsForGenerics)
        {
            Append(L'>');
        }
        else
        {
            Append(L']');
        }
    }

    return hr;
}

HRESULT TypeNameBuilder::OpenGenericArgument()
{
    if (!CheckParseState(ParseStateSTART))
    {
        return E_FAIL;
    }

    if (m_instNesting == 0)
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    m_parseState = ParseStateSTART;
    m_bNestedName = FALSE;

    if (!m_bFirstInstArg)
    {
        Append(L',');
    }

    m_bFirstInstArg = FALSE;

    if (m_bUseAngleBracketsForGenerics)
    {
        Append(L'<');
    }
    else
    {
        Append(L'[');
    }
    PushOpenGenericArgument();

    return hr;
}

void TypeNameBuilder::PushOpenGenericArgument()
{
    m_stack.Push(m_sb.GetStringLength());
}


HRESULT TypeNameBuilder::CloseGenericArgument()
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR | ParseStateBYREF | ParseStateASSEMSPEC))
    {
        return E_FAIL;
    }

    if (m_instNesting == 0)
    {
        return E_FAIL;
    }

    m_parseState = ParseStateSTART;

    if (m_bHasAssemblySpec)
    {
        if (m_bUseAngleBracketsForGenerics)
        {
            Append(L'>');
        }
        else
        {
            Append(L']');
        }
    }

    PopOpenGenericArgument();

    return S_OK;
}

void TypeNameBuilder::PopOpenGenericArgument()
{
    int index = m_stack.Top();
    m_stack.Pop();

    if (!m_bHasAssemblySpec)
    {
        m_sb.DelWCharsFromString(index - 1, 1);
    }

    m_bHasAssemblySpec = FALSE;
}

HRESULT TypeNameBuilder::AddName(LPCWSTR szName)
{
    if (!szName)
    {
        return E_FAIL;
    }

    if (!CheckParseState(ParseStateSTART | ParseStateNAME))
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    m_parseState = ParseStateNAME;

    if (m_bNestedName)
    {
        Append(L'+');
    }

    m_bNestedName = TRUE;

    EscapeName(szName);

    return hr;
}

bool ContainsReservedChar(LPCWSTR pTypeName)
{
    WCHAR c;
    while ((c = * pTypeName++) != L'\0')
    {
        if (CLRTypeName::IsTypeNameReservedChar(c))
        {
            return true;
        }
    }

    return false;
}

/* This method escapes szName and appends it to this TypeNameBuilder */
void TypeNameBuilder::EscapeName(LPCWSTR szName)
{
    if (ContainsReservedChar(szName))
    {
        while (*szName)
        {
            WCHAR c = *szName++;

            if (CLRTypeName::IsTypeNameReservedChar(c))
            {
                Append(L'\\');
            }

            Append(c);
        }
    }
    else
    {
        Append(szName);
    }
}

HRESULT TypeNameBuilder::AddPointer()
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR))
    {
        return E_FAIL;
    }

    m_parseState = ParseStatePTRARR;

    Append(L'*');

    return S_OK;
}

HRESULT TypeNameBuilder::AddByRef()
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR))
    {
        return E_FAIL;
    }

    m_parseState = ParseStateBYREF;

    Append(L'&');

    return S_OK;
}

HRESULT TypeNameBuilder::AddSzArray()
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR))
    {
        return E_FAIL;
    }

    m_parseState = ParseStatePTRARR;

    Append(L"[]");

    return S_OK;
}

HRESULT TypeNameBuilder::AddArray(DWORD rank)
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR))
    {
        return E_FAIL;
    }

    m_parseState = ParseStatePTRARR;

    if (rank <= 0)
    {
        return E_INVALIDARG;
    }

    if (rank == 1)
    {
        Append(L"[*]");
    }
    else if (rank > 64)
    {
        // Only taken in an error path, runtime will not load arrays of more than 32 dimentions
        WCHAR wzDim[128];
        _snwprintf_s(wzDim, 128, _TRUNCATE, L"[%d]", rank);
        Append(wzDim);
    }
    else
    {
        WCHAR* wzDim = new WCHAR[rank+3];

        if (wzDim == NULL) // allocation failed, do it the long way (each Append -> memory realloc)
        {
            Append(L'[');
            for(unsigned int i = 1; i < rank; i++)
            {
                Append(L',');
            }
            Append(L']');
        }
        else             // allocation OK, do it the fast way
        {
            WCHAR* pwz = wzDim+1;
            *wzDim = '[';
            for(unsigned int i = 1; i < rank; i++, pwz++)
            {
                *pwz=',';
            }
            *pwz = ']';
            *(++pwz) = 0;
            Append(wzDim);
            delete [] wzDim;
        }
    }

    return S_OK;
}

HRESULT TypeNameBuilder::AddAssemblySpec(LPCWSTR szAssemblySpec)
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR | ParseStateBYREF))
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    m_parseState = ParseStateASSEMSPEC;

    if (szAssemblySpec && *szAssemblySpec)
    {
        Append(L", ");

        if (m_instNesting > 0)
        {
            EscapeEmbeddedAssemblyName(szAssemblySpec);
        }
        else
        {
            EscapeAssemblyName(szAssemblySpec);
        }

        m_bHasAssemblySpec = TRUE;
        hr = S_OK;
    }

    return hr;
}

void TypeNameBuilder::EscapeAssemblyName(LPCWSTR szName)
{
    Append(szName);
}

void TypeNameBuilder::EscapeEmbeddedAssemblyName(LPCWSTR szName)
{
    LPCWSTR itr = szName;
    bool bContainsReservedChar = false;

    while (*itr)
    {
        if (L']' == *itr++)
        {
            bContainsReservedChar = true;
            break;
        }
    }

    if (bContainsReservedChar)
    {
        itr = szName;
        while (*itr)
        {
            WCHAR c = *itr++;
            if (c == ']')
            {
                Append(L'\\');
            }

            Append(c);
        }
    }
    else
    {
        Append(szName);
    }
}

HRESULT TypeNameBuilder::ToString(BSTR* pszStringRepresentation)
{
    if (!CheckParseState(ParseStateNAME | ParseStateGENARGS | ParseStatePTRARR | ParseStateBYREF | ParseStateASSEMSPEC))
    {
        return E_FAIL;
    }

    if (m_instNesting)
    {
        return E_FAIL;
    }

    *pszStringRepresentation = SysAllocString(m_sb.GetString());

    return S_OK;
}

HRESULT TypeNameBuilder::Clear()
{
    m_sb.Clear();
    m_bNestedName = FALSE;
    m_instNesting = 0;
    m_bFirstInstArg = FALSE;
    m_parseState = ParseStateSTART;
    m_bHasAssemblySpec = FALSE;
    m_bUseAngleBracketsForGenerics = FALSE;
    m_stack.Reset();

    return S_OK;
}
