#pragma once

class CLRTypeName
{
private:
    friend class TypeNameBuilder;

private:
    class TypeNameParser
    {
        TypeNameParser(LPCWSTR szTypeName, CLRTypeName* pTypeName, DWORD* pError)
        {
            if (szTypeName == NULL)
            {
                szTypeName = L"";
            }

            m_currentToken = TypeNameEmpty;
            m_nextToken = TypeNameEmpty;

            *pError = (DWORD)-1;
            m_pTypeName = pTypeName;
            m_sszTypeName = szTypeName;
            m_currentItr = m_itr = m_sszTypeName;

            if (!START())
            {
                *pError = (DWORD)(m_currentItr - m_sszTypeName) - 1;
            }
        }

    private:
        friend class CLRTypeName;

    private:
        typedef enum {
            //
            // TOKENS
            //
            TypeNameEmpty               = 0x8000,
            TypeNameIdentifier          = 0x0001,
            TypeNamePostIdentifier      = 0x0002,
            TypeNameOpenSqBracket       = 0x0004,
            TypeNameCloseSqBracket      = 0x0008,
            TypeNameComma               = 0x0010,
            TypeNamePlus                = 0x0020,
            TypeNameAstrix              = 0x0040,
            TypeNameAmperstand          = 0x0080,
            TypeNameBackSlash           = 0x0100,
            TypeNameEnd                 = 0x4000,

            //
            // 1 TOKEN LOOK AHEAD
            //
            TypeNameNAME                = TypeNameIdentifier,
            TypeNameNESTNAME            = TypeNameIdentifier,
            TypeNameASSEMSPEC           = TypeNameIdentifier,
            TypeNameGENPARAM            = TypeNameOpenSqBracket | TypeNameEmpty,
            TypeNameFULLNAME            = TypeNameNAME,
            TypeNameAQN                 = TypeNameFULLNAME | TypeNameEnd,
            TypeNameASSEMBLYSPEC        = TypeNameIdentifier,
            TypeNameGENARG              = TypeNameOpenSqBracket | TypeNameFULLNAME,
            TypeNameGENARGS             = TypeNameGENARG,
            TypeNameEAQN                = TypeNameIdentifier,
            TypeNameEASSEMSPEC          = TypeNameIdentifier,
            TypeNameARRAY               = TypeNameOpenSqBracket,
            TypeNameQUALIFIER           = TypeNameAmperstand | TypeNameAstrix | TypeNameARRAY | TypeNameEmpty,
            TypeNameRANK                = TypeNameComma | TypeNameEmpty,
        } TypeNameTokens;

        typedef enum {
            TypeNameNone                = 0x00,
            TypeNameId                  = 0x01,
            TypeNameFusionName          = 0x02,
            TypeNameEmbeddedFusionName  = 0x03,
        } TypeNameIdentifiers;

    //
    // LEXIFIER
    //
    private:
        TypeNameTokens LexAToken();
        BOOL GetIdentifier(StringBuffer* sszId, TypeNameIdentifiers identiferType);

        void NextToken()
        {
            m_currentToken = m_nextToken;
            m_currentItr = m_itr;
            m_nextToken = LexAToken();
        }

        BOOL NextTokenIs(TypeNameTokens token)
        {
            return !!(m_nextToken & token);
        }

        BOOL TokenIs(TypeNameTokens token)
        {
            return !!(m_currentToken & token);
        }

        BOOL TokenIs(int token)
        {
            return TokenIs((TypeNameTokens)token);
        }

    //
    // PRODUCTIONS
    //
    private:
        BOOL START();

        BOOL AQN();
        // /* empty */
        // FULLNAME ',' ASSEMSPEC
        // FULLNAME

        BOOL ASSEMSPEC();
        // fusionName

        BOOL FULLNAME();
        // NAME GENPARAMS QUALIFIER

        BOOL GENPARAMS();
        // *empty*
        // '[' GENARGS ']'

        BOOL GENARGS();
        // GENARG
        // GENARG ',' GENARGS

        BOOL GENARG();
        // '[' EAQN ']'
        // FULLNAME

        BOOL EAQN();
        // FULLNAME ',' EASSEMSPEC
        // FULLNAME

        BOOL EASSEMSPEC();
        // embededFusionName

        BOOL QUALIFIER();
        // *empty*
        // '&'
        // *' QUALIFIER
        // ARRAY QUALIFIER

        BOOL ARRAY();
        // '[' RANK ']'
        // '[' '*' ']'

        BOOL RANK(DWORD* pdwRank);
        // *empty*
        // ',' RANK

        BOOL NAME();
        // id
        // id '+' NESTNAME

        BOOL NESTNAME();
        // id
        // id '+' NESTNAME

    public:
        void MakeRotorHappy() { }

    private:
        CLRTypeName* m_pTypeName;
        LPCWSTR m_sszTypeName;
        LPCWSTR m_itr;
        LPCWSTR m_currentItr;
        TypeNameTokens m_currentToken;
        TypeNameTokens m_nextToken;
    };
    friend class CLRTypeName::TypeNameParser;

public:
    virtual ULONG AddRef();
    virtual ULONG Release();

    HRESULT GetNameCount(DWORD* pCount);
    HRESULT GetNames(DWORD count, BSTR* rgbszNames, DWORD* pFetched);
    HRESULT GetTypeArgumentCount(DWORD* pCount);
    HRESULT GetTypeArguments(DWORD count, CLRTypeName** rgpArguments, DWORD* pFetched);
    HRESULT GetModifierLength(DWORD* pCount);
    HRESULT GetModifiers(DWORD count, DWORD* rgModifiers, DWORD* pFetched);
    HRESULT GetAssemblyName(BSTR* rgbszAssemblyNames);

    static bool IsTypeNameReservedChar(WCHAR ch)
    {
        switch (ch)
        {
        case L',':
        case L'[':
        case L']':
        case L'&':
        case L'*':
        case L'+':
        case L'\\':
            return true;

        default:
            return false;
        }
    }

    static HRESULT ParseTypeName(LPCWSTR szTypeName, DWORD* pError, CLRTypeName** ppTypeName);

public:
    CLRTypeName() :
        m_bIsGenericArgument(FALSE),
        m_count(0)
    {
    }

    CLRTypeName(LPCWSTR szTypeName, DWORD* pError) :
        m_bIsGenericArgument(FALSE),
        m_count(0)
    {
        TypeNameParser parser(szTypeName, this, pError);
        parser.MakeRotorHappy();
    }

    ~CLRTypeName();

    StringBuffer* GetAssembly()
    {
        return &m_assembly;
    }

private:
    CLRTypeName* AddGenericArgument();

    StringBuffer* AddName()
    {
        StringBuffer* pStringBuffer = new StringBuffer();
        m_names.AddElement(pStringBuffer);
        return pStringBuffer;
    }

    void SetByRef()
    {
        m_signature.AddElement(ELEMENT_TYPE_BYREF);
    }

    void SetPointer()
    {
        m_signature.AddElement(ELEMENT_TYPE_PTR);
    }

    void SetSzArray()
    {
        m_signature.AddElement(ELEMENT_TYPE_SZARRAY);
    }

    void SetArray(DWORD rank)
    {
        m_signature.AddElement(ELEMENT_TYPE_ARRAY);
        m_signature.AddElement(rank);
    }

    static INT32 GetCharacterInfoHelper(WCHAR c, INT32 CharInfoType);
    static BOOL NativeIsWhiteSpace(WCHAR c);

private:
    BOOL m_bIsGenericArgument;
    DWORD m_count;
    DynamicArray<DWORD> m_signature;
    DynamicArray<CLRTypeName*> m_genericArguments;
    DynamicArray<StringBuffer*> m_names;
    StringBuffer m_assembly;
};
