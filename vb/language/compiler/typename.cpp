#include "stdafx.h"

#define IfFalseReturn(P) if (!P) return FALSE;

HRESULT CLRTypeName::ParseTypeName(LPCWSTR szTypeName, DWORD* pError, CLRTypeName** ppTypeName)
{
    if (!ppTypeName || !pError)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    *ppTypeName = NULL;
    *pError = (DWORD)-1;

    CLRTypeName* pTypeName = new CLRTypeName(szTypeName, pError);

    if (!pTypeName)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pTypeName->AddRef();

        if (*pError != (DWORD)-1)
        {
            pTypeName->Release();
            hr = S_FALSE;
        }
        else
        {
            *ppTypeName = pTypeName;
        }
    }

    return hr;
}

INT32 CLRTypeName::GetCharacterInfoHelper(WCHAR c, INT32 CharInfoType)
{
    unsigned short result=0;
    if (!GetStringTypeEx(LOCALE_USER_DEFAULT, CharInfoType, &(c), 1, &result)) {
        _ASSERTE(!"This should not happen, verify the arguments passed to GetStringTypeEx()");
    }
    return(INT32)result;
}

BOOL CLRTypeName::NativeIsWhiteSpace(WCHAR c)
{
    if (c <= (WCHAR) 0x7F) // common case
    {
        BOOL result = (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t') || (c == '\f') || (c == (WCHAR) 0x0B);

        ASSERT(result == ((GetCharacterInfoHelper(c, CT_CTYPE1) & C1_SPACE) != 0), "");

        return result;
    }

    // GetCharacterInfoHelper costs around 160 instructions
    return((GetCharacterInfoHelper(c, CT_CTYPE1) & C1_SPACE)!=0);
}

ULONG CLRTypeName::AddRef()
{
    m_count++;

    return m_count;
}

DWORD CLRTypeName::Release()
{
    m_count--;

    DWORD dwCount = m_count;
    if (dwCount == 0)
    {
        delete this;
    }

    return dwCount;
}

CLRTypeName::~CLRTypeName()
{
    for (ULONG i = m_genericArguments.Count(); i > 0; i--)
    {
        CLRTypeName* pGenericArgument = m_genericArguments.Element(i - 1);
        m_genericArguments.Remove(i - 1);
        pGenericArgument->Release();
    }

    for (ULONG i = m_names.Count(); i > 0; i--)
    {
        StringBuffer* pBuffer = m_names.Element(i - 1);
        m_names.Remove(i - 1);
        delete pBuffer;
    }
}

CLRTypeName* CLRTypeName::AddGenericArgument()
{
    CLRTypeName* pGenArg = new CLRTypeName();
    pGenArg->AddRef();

    pGenArg->m_bIsGenericArgument = TRUE;
    m_genericArguments.AddElement(pGenArg);
    return pGenArg;
}

CLRTypeName::TypeNameParser::TypeNameTokens CLRTypeName::TypeNameParser::LexAToken()
{
    if (m_nextToken == TypeNameIdentifier)
    {
        return TypeNamePostIdentifier;
    }

    if (m_nextToken == TypeNameEnd)
    {
        return TypeNameEnd;
    }

    if (*m_itr == L'\0')
    {
        return TypeNameEnd;
    }

    if (NativeIsWhiteSpace(*m_itr))
    {
        m_itr++;
        return LexAToken();
    }

    WCHAR c = *m_itr;
    m_itr++;
    switch(c)
    {
        case L',': return TypeNameComma;
        case L'[': return TypeNameOpenSqBracket;
        case L']': return TypeNameCloseSqBracket;
        case L'&': return TypeNameAmperstand;
        case L'*': return TypeNameAstrix;
        case L'+': return TypeNamePlus;
        case L'\\':
            m_itr--;
            return TypeNameIdentifier;
    }

    ASSERT(!CLRTypeName::IsTypeNameReservedChar(c), "");

    m_itr--;
    return TypeNameIdentifier;
}

BOOL CLRTypeName::TypeNameParser::GetIdentifier(StringBuffer* sszId, CLRTypeName::TypeNameParser::TypeNameIdentifiers identifierType)
{
    ASSERT(m_currentToken == TypeNameIdentifier && m_nextToken == TypeNamePostIdentifier, "");

    sszId->Clear();

    LPCWSTR start = m_currentItr;
    DynamicArray<LPCWSTR> m_escape;

    if (identifierType == TypeNameId)
    {
        do
        {
            switch (* m_currentItr ++)
            {
                case L',':
                case L'[':
                case L']':
                case L'&':
                case L'*':
                case L'+':
                case L'\0':
                    goto done;

                case L'\\':
                    m_escape.AddElement(m_currentItr - 1);

                    if (!CLRTypeName::IsTypeNameReservedChar(*m_currentItr) || *m_currentItr == '\0')
                    {
                        return FALSE;
                    }

                    m_currentItr++;
                    break;

                default:
                    break;
            }
        }
        while(true);

done:
        m_currentItr--;
    }
    else if (identifierType == TypeNameFusionName)
    {
        while(*m_currentItr != L'\0')
        {
            m_currentItr++;
        }
    }
    else if (identifierType == TypeNameEmbeddedFusionName)
    {
        for (; (*m_currentItr != L'\0') && (*m_currentItr != L']'); m_currentItr++)
        {
            if (*m_currentItr == L'\\')
            {
                if (*(m_currentItr + 1) == L']')
                {
                    m_escape.AddElement(m_currentItr);
                    m_currentItr ++;
                    continue;
                }
            }

            if (*m_currentItr == '\0')
            {
                return FALSE;
            }
        }
        if (*m_currentItr == L'\0')
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    sszId->AppendWithLength(start, (size_t)(m_currentItr - start));

    for (ULONG i = m_escape.Count(); i > 0; i--)
    {
        sszId->DelWCharsFromString((unsigned)(m_escape.Element(i - 1) - start), 1);
    }

    m_itr = m_currentItr;
    m_nextToken = LexAToken();
    return TRUE;
}

BOOL CLRTypeName::TypeNameParser::START()
{
    NextToken();
    NextToken();
    return AQN();
}

// FULLNAME ',' ASSEMSPEC
// FULLNAME
// /* empty */
BOOL CLRTypeName::TypeNameParser::AQN()
{
    IfFalseReturn(TokenIs(TypeNameAQN));

    if (TokenIs(TypeNameEnd))
    {
        return TRUE;
    }

    IfFalseReturn(FULLNAME());

    if (TokenIs(TypeNameComma))
    {
        NextToken();
        IfFalseReturn(ASSEMSPEC());
    }

    IfFalseReturn(TokenIs(TypeNameEnd));

    return TRUE;
}

// fusionName
BOOL CLRTypeName::TypeNameParser::ASSEMSPEC()
{
    IfFalseReturn(TokenIs(TypeNameASSEMSPEC));

    GetIdentifier(m_pTypeName->GetAssembly(), TypeNameFusionName);

    NextToken();

    return TRUE;
}

// NAME GENPARAMS QUALIFIER
BOOL CLRTypeName::TypeNameParser::FULLNAME()
{
    IfFalseReturn(TokenIs(TypeNameFULLNAME));
    IfFalseReturn(NAME());

    IfFalseReturn(GENPARAMS());

    IfFalseReturn(QUALIFIER());

    return TRUE;
}

// *empty*
// '[' GENARGS ']'
BOOL CLRTypeName::TypeNameParser::GENPARAMS()
{
    if (!TokenIs(TypeNameGENPARAM))
    {
        return TRUE;
    }

    if (!NextTokenIs(TypeNameGENARGS))
    {
        return TRUE;
    }

    NextToken();
    IfFalseReturn(GENARGS());

    IfFalseReturn(TokenIs(TypeNameCloseSqBracket));
    NextToken();

    return TRUE;
}

// GENARG
// GENARG ',' GENARGS
BOOL CLRTypeName::TypeNameParser::GENARGS()
{
    IfFalseReturn(TokenIs(TypeNameGENARGS));

    IfFalseReturn(GENARG());

    if (TokenIs(TypeNameComma))
    {
        NextToken();
        IfFalseReturn(GENARGS());
    }

    return TRUE;
}

// '[' EAQN ']'
// FULLNAME
BOOL CLRTypeName::TypeNameParser::GENARG()
{
    IfFalseReturn(TokenIs(TypeNameGENARG));

    CLRTypeName* pEnclosingTypeName = m_pTypeName;
    m_pTypeName = m_pTypeName->AddGenericArgument();
    {
        if (TokenIs(TypeNameOpenSqBracket))
        {
            NextToken();
            IfFalseReturn(EAQN());

            IfFalseReturn(TokenIs(TypeNameCloseSqBracket));
            NextToken();
        }
        else
        {
            IfFalseReturn(FULLNAME());
        }
    }
    m_pTypeName = pEnclosingTypeName;

    return TRUE;
}

// FULLNAME ',' EASSEMSPEC
// FULLNAME
BOOL CLRTypeName::TypeNameParser::EAQN()
{
    IfFalseReturn(TokenIs(TypeNameEAQN));

    IfFalseReturn(FULLNAME());

    if (TokenIs(TypeNameComma))
    {
        NextToken();
        IfFalseReturn(EASSEMSPEC());
    }

    return TRUE;
}

// embeddedFusionName
BOOL CLRTypeName::TypeNameParser::EASSEMSPEC()
{

    GetIdentifier(m_pTypeName->GetAssembly(), TypeNameEmbeddedFusionName);

    NextToken();

    return TRUE;
}

// *empty*
// '&'
// '*' QUALIFIER
// ARRAY QUALIFIER
BOOL CLRTypeName::TypeNameParser::QUALIFIER()
{
    if (!TokenIs(TypeNameQUALIFIER))
    {
        return TRUE;
    }

    if (TokenIs(TypeNameAmperstand))
    {
        m_pTypeName->SetByRef();

        NextToken();
    }
    else if (TokenIs(TypeNameAstrix))
    {
        m_pTypeName->SetPointer();

        NextToken();
        IfFalseReturn(QUALIFIER());
    }
    else
    {
        IfFalseReturn(ARRAY());
        IfFalseReturn(QUALIFIER());
    }

    return TRUE;
}

// '[' RANK ']'
// '[' '*' ']'
BOOL CLRTypeName::TypeNameParser::ARRAY()
{
    IfFalseReturn(TokenIs(TypeNameARRAY));

    NextToken();

    if (TokenIs(TypeNameAstrix))
    {
        m_pTypeName->SetArray(1);

        NextToken();
    }
    else
    {
        DWORD dwRank = 1;
        IfFalseReturn(RANK(&dwRank));

        if (dwRank == 1)
        {
            m_pTypeName->SetSzArray();
        }
        else
        {
            m_pTypeName->SetArray(dwRank);
        }
    }

    IfFalseReturn(TokenIs(TypeNameCloseSqBracket));
    NextToken();

    return TRUE;
}

// *empty*
// ',' RANK
BOOL CLRTypeName::TypeNameParser::RANK(DWORD* pdwRank)
{
    if (!TokenIs(TypeNameRANK))
    {
        return TRUE;
    }

    NextToken();
    *pdwRank = *pdwRank + 1;
    IfFalseReturn(RANK(pdwRank));

    return TRUE;
}

// id
// id '+' NESTNAME
BOOL CLRTypeName::TypeNameParser::NAME()
{
    IfFalseReturn(TokenIs(TypeNameNAME));

    GetIdentifier(m_pTypeName->AddName(), TypeNameId);

    NextToken();
    if (TokenIs(TypeNamePlus))
    {
        NextToken();
        IfFalseReturn(NESTNAME());
    }

    return TRUE;
}

// id
// id '+' NESTNAME
BOOL CLRTypeName::TypeNameParser::NESTNAME()
{
    IfFalseReturn(TokenIs(TypeNameNESTNAME));

    GetIdentifier(m_pTypeName->AddName(), TypeNameId);

    NextToken();
    if (TokenIs(TypeNamePlus))
    {
        NextToken();
        IfFalseReturn(NESTNAME());
    }

    return TRUE;
}

HRESULT CLRTypeName::GetNameCount(DWORD* pCount)
{
    if (!pCount)
    {
        return E_INVALIDARG;
    }

    *pCount = m_names.Count();

    return S_OK;
}

HRESULT CLRTypeName::GetNames(DWORD count, BSTR* bszName, DWORD* pFetched)
{
    HRESULT hr = S_OK;

    if (!pFetched)
    {
        return E_INVALIDARG;
    }

    *pFetched = m_names.Count();

    if (m_names.Count() > count)
    {
        return S_FALSE;
    }

    if (!bszName)
    {
        return E_INVALIDARG;
    }

    for (ULONG i = 0; i < m_names.Count(); i ++)
    {
        bszName[i] = SysAllocString(m_names.Element(i)->GetString());
    }

    return hr;
}

HRESULT CLRTypeName::GetTypeArgumentCount(DWORD* pCount)
{

    if (!pCount)
    {
        return E_INVALIDARG;
    }

    *pCount = m_genericArguments.Count();

    return S_OK;
}

HRESULT CLRTypeName::GetTypeArguments(DWORD count, CLRTypeName** ppArguments, DWORD* pFetched)
{
    if (!pFetched)
    {
        return E_INVALIDARG;
    }

    *pFetched = m_genericArguments.Count();

    if (m_genericArguments.Count() > count)
    {
        return S_FALSE;
    }

    if (!ppArguments)
    {
        return E_INVALIDARG;
    }

    for (ULONG i = 0; i < m_genericArguments.Count(); i ++)
    {
        ppArguments[i] = m_genericArguments.Element(i);
        ppArguments[i]->AddRef();
    }

    return S_OK;
}

HRESULT CLRTypeName::GetModifierLength(DWORD* pCount)
{
    if (pCount == NULL)
    {
        return E_INVALIDARG;
    }

    *pCount = m_signature.Count();

    return S_OK;
}

HRESULT CLRTypeName::GetModifiers(DWORD count, DWORD* pModifiers, DWORD* pFetched)
{
    if (!pFetched)
    {
        return E_INVALIDARG;
    }

    *pFetched = m_signature.Count();

    if (m_signature.Count() > count)
    {
        return S_FALSE;
    }

    if (!pModifiers)
    {
        return E_INVALIDARG;
    }

    for (ULONG i = 0; i < m_signature.Count(); i ++)
    {
        pModifiers[i] = m_signature.Element(i);
    }

    return S_OK;
}

HRESULT CLRTypeName::GetAssemblyName(BSTR* pszAssemblyName)
{
    HRESULT hr = S_OK;

    if (pszAssemblyName == NULL)
    {
        return E_INVALIDARG;
    }

    *pszAssemblyName = SysAllocString(m_assembly.GetString());

    if (*pszAssemblyName == NULL)
    {
        hr= E_OUTOFMEMORY;
    }

    return hr;
}
