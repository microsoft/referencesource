//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

extern BYTE g_anCharProps[];
extern BYTE * g_apCharTables[];

//-------------------------------------------------------------------------------------------------
//
// Use XMLCharacter when testing for XML character properties.
// Use UniCharacter when testing for Unicode character properties.
//
class XmlCharacter
{
public:
    // XML character properties (used by XML, XQL parsers).
    // These are derived from enumerations in the XML 1.0 spec
    enum
    {
        FWHITESPACE    = 1,     // Whitespace chars -- Section 2.3 [3]
        FLETTER        = 2,     // Letters -- Appendix B [84]
        FSTARTNAME     = 4,     // Starting name characters -- Section 2.3 [5]
        FNAME          = 8,     // Name characters -- Section 2.3 [4]
        FCHARDATA      = 16,    // Character data characters -- Section 2.2 [2]
        FCHARDATAUTF16 = 32,    // Character data characters -- Section 2.2 [2], encoded with UTF-16
        FSTARTNAMENS   = 64,    // Starting name characters from XMLNS 1.0 spec
        FNAMENS        = 128,   // Name characters from XMLNS 1.0 spec
    };

    static int isWhitespace(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FWHITESPACE;
    }

    static int isLetter(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FLETTER;
    }

    static int isAsciiDigit(WCHAR ch)
    {
        // Return true if ch is an Arabic (0..9) digit
        return (ch <= 0x39 && ch >= 0x30);
    }

    static int isHexDigit(WCHAR ch)
    {
        return (ch >= 0x30 && ch <= 0x39) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
    }

    static int isNameChar(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FNAME;
    }

    static int isStartNameChar(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FSTARTNAME;
    }

    static int isNameNsChar(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FNAMENS;
    }

    static int isStartNameNsChar(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FSTARTNAMENS;
    }

    static int isCharData(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FCHARDATA;
    }

    static int isCharDataUtf16(WCHAR ch)
    {
        return g_apCharTables[ch >> 8][ch & 0xff] & FCHARDATAUTF16;
    }

    static __forceinline bool isValidUtf16(WCHAR wh)
    {
        return (short)wh >= 0x20
            || (short)wh <= (short)0xFFFD
            || XmlCharacter::isCharDataUtf16(wh);
    }

    // [13] PubidChar ::=  #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] Section 2.3 of spec
    static bool isPubidChar(WCHAR ch)
    {
        return (ch >= 0x20 && ch <= 0x5A && ch != 0x22 && ch != 0x26 && ch != 0x3C && ch != 0x3E)
            || (ch >= 0x61 && ch <= 0x7A) || ch == 0x0A || ch == 0x0D || ch == 0x5F;
    }
};

// From unicharacter.hxx

// A WCHAR is really a UTF16 character.  It takes two to form a surrogate
// character.  Define a UTF32 character, to be used when a surrogate pair
// needs to be treated as a single scalar Unicode value.

typedef WCHAR UTF16;
typedef DWORD UTF32;

class UniCharacter
{
    public: static bool isHighSurrogate(TCHAR ch)
    {
        return (ch >= 0xD800 && ch <= 0xDBFF);
    }
    public: static bool isLowSurrogate(TCHAR ch)
    {
        return (ch >= 0xDC00 && ch <= 0xDFFF);
    }
    public: static UTF32 UniCharacter::toUTF32(_In_ const TCHAR * pch, _Out_ int * pcUTF16)
    {
        VSASSERT(pch && pcUTF16, "Invalid args");

        if (!isHighSurrogate(*pch))
        {
            *pcUTF16 = 1;
            return (UTF32) *pch;
        }
        VSASSERT(isLowSurrogate(pch[1]), "Invalid surrogate pair");
        *pcUTF16 = 2;
        return ((UTF32) *pch - 0xd800) * 0x400 + ((UTF32) pch[1] - 0xdc00) + 0x10000;
    }
};

// End from unicharacter.hxx

bool isValidLanguageID(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen);

bool isValidPublicID(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen);

bool isValidSystemID(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen);

bool isValidEncName(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen);

// Only recognizes XML whitespace chars (no intl space chars);
bool isXmlWhitespace(_In_count_(ulLen)const WCHAR * pwcText, size_t ulLen);

// resolve built-in entities.
WCHAR BuiltinEntity(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen);

HRESULT HexToUTF16(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen, _Out_z_cap_(cchOut) WCHAR * pwcOut, unsigned cchOut);
HRESULT DecToUTF16(_In_count_(ulLen) const WCHAR * pwcText, size_t ulLen, _Out_z_cap_(cchOut) WCHAR * pwcOut, unsigned cchOut);

HRESULT UnicodeToUTF16(ULONG ulCode, _Out_z_cap_(cchOut) WCHAR * pwcUtf16, unsigned cchOut);

#ifdef UNIX
    extern const unsigned char s_ByteOrderMarkTrident[sizeof(WCHAR)];
    extern const unsigned char s_ByteOrderMark[sizeof(WCHAR)];
    extern const unsigned char s_ByteOrderMarkUCS2[2];  // This is 2 for a reason!
#else
    extern const unsigned char s_ByteOrderMark[2];
    #define s_ByteOrderMarkTrident s_ByteOrderMark
    #define s_ByteOrderMarkUCS2    s_ByteOrderMark
#endif
