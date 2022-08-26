// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
// ---------------------------------------------------------------------------

#pragma once

class TypeNameBuilder
{
public:
    HRESULT OpenGenericArguments();
    HRESULT CloseGenericArguments();
    HRESULT OpenGenericArgument();
    HRESULT CloseGenericArgument();
    HRESULT AddName(LPCWSTR szName);
    HRESULT AddName(LPCWSTR szName, LPCWSTR szNamespace);
    HRESULT AddPointer();
    HRESULT AddByRef();
    HRESULT AddSzArray();
    HRESULT AddArray(DWORD rank);
    HRESULT AddAssemblySpec(LPCWSTR szAssemblySpec);
    HRESULT ToString(BSTR* pszStringRepresentation);
    HRESULT Clear();

public:
    typedef enum
    {
        ParseStateSTART         = 0x0001,
        ParseStateNAME          = 0x0004,
        ParseStateGENARGS       = 0x0008,
        ParseStatePTRARR        = 0x0010,
        ParseStateBYREF         = 0x0020,
        ParseStateASSEMSPEC     = 0x0080,
        ParseStateERROR         = 0x0100,
    } ParseState;

public:

    TypeNameBuilder()
    {
        Clear();
    }

    void SetUseAngleBracketsForGenerics(BOOL value) { m_bUseAngleBracketsForGenerics = value; }
    void Append(LPCWSTR pStr) { m_sb.AppendString(pStr); }
    void Append(WCHAR c) { m_sb.AppendChar(c); }
    WCHAR* GetString() { return m_sb.GetString(); }

private:
    void EscapeName(LPCWSTR szName);
    void EscapeAssemblyName(LPCWSTR szName);
    void EscapeEmbeddedAssemblyName(LPCWSTR szName);
    BOOL CheckParseState(int validState) { return ((int)m_parseState & validState) != 0; }
    HRESULT Fail() { m_parseState = ParseStateERROR; return E_FAIL; }
    void PushOpenGenericArgument();
    void PopOpenGenericArgument();

private:
    ParseState m_parseState;
    StringBuffer m_sb;

    DWORD m_instNesting;
    BOOL m_bFirstInstArg;
    BOOL m_bNestedName;
    BOOL m_bHasAssemblySpec;
    BOOL m_bUseAngleBracketsForGenerics;
    Stack<int> m_stack;
};
