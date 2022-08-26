//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The VB compiler scanner.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if IDE
typedef int (STDAPICALLTYPE *PFN_GetCalendarInfoA)(LCID, CALID, CALTYPE, LPTSTR, int, LPDWORD);
const char szCalendarInfo[] = "GetCalendarInfoA";
const char szKernel32[] = "kernel32.dll";
const char szCalKey[] = "Control Panel\\International\\Calendars\\TwoDigitYearMax";
const char szCalVal[] = "1";

#define CAL_ITWODIGITYEARMAX          0x00000030
#define LOCALE_RETURN_NUMBER          0x20000000   // return number instead of string

DWORD GetDateWindow()
{
    static PFN_GetCalendarInfoA s_pfnGetCalendarInfo = NULL;
    static BOOL s_fFirstTime = true;
    static DWORD s_iTwoYearMax = 2029; // default 1930-2029
    HKEY hKeyValue;

    if (!s_fFirstTime)
    {
        return s_iTwoYearMax;
    }

    // Doesn't matter if two threads come through at the same
    // time, they'll both resolve to the same address
    HANDLE hKernel = NULL;
    hKernel = GetModuleHandleA(szKernel32);
    VSASSERT(hKernel != NULL, "where is it?");
    InterlockedExchange((LONG*)&s_pfnGetCalendarInfo, (LONG)GetProcAddress((HMODULE)hKernel, szCalendarInfo));
    s_fFirstTime = FALSE;

    if (s_pfnGetCalendarInfo != NULL)
    {
        DWORD dwTwoYearMax = 0;
        (*s_pfnGetCalendarInfo)(LCID_US_ENGLISH, CAL_GREGORIAN,
                            CAL_ITWODIGITYEARMAX | LOCALE_RETURN_NUMBER,
                            NULL, 0, &dwTwoYearMax);
        if (dwTwoYearMax < 10000)
            s_iTwoYearMax = (short)dwTwoYearMax;
    }
    else if (RegOpenKeyExA(HKEY_CURRENT_USER, szCalKey, 0, KEY_QUERY_VALUE, &hKeyValue) == NOERROR)
    {
      CHAR szKey[10 + 1]; // Not going to be more than 4 digits for at least 8000 years, so I think we're
                      // safe. (hey, did you ever hear the one about the Cobol programmer who had himself
                      // frozen...)
      DWORD cb = _countof(szKey) - 1; //Leave one extra space for a null termination.
      DWORD dwType;

      // API not there, bummer. Look up in registry
      #pragma prefast(suppress: 22116, "cb is related that size of szKey, in fact it is one less so Safe here.");
      RegQueryValueExA(hKeyValue, szCalVal, NULL, &dwType, (LPBYTE)szKey, &cb);
      RegCloseKey(hKeyValue);

      if (dwType == REG_SZ)
      {
          szKey[_countof(szKey) - 1] = 0; //Prevent non-zero teminated string from entering.
          DWORD dwTwoYearMax = atol(szKey);
          if (dwTwoYearMax < 10000)
            s_iTwoYearMax = (short)dwTwoYearMax;
      }
    }

    return s_iTwoYearMax;
}
#endif

tokens IdentifierAsKeyword(
    _In_ Token *T,
    Compiler *Compiler)
{
    return IdentifierAsKeyword(T, Compiler->GetStringPool());
}

tokens IdentifierAsKeyword(
    _In_ Token *T,
    _In_ StringPool* pStringPool)
{
    VSASSERT(T->m_TokenType == tkID || T->IsKeyword(), "Expected identifier is not.");

    // Dev 10 393046
    // An identifier with a type character should not be a keyword...
    // Dev 10 470738
    // unless it's "Mid$". This is the only keyword in the VB language spec that can have a $ after it.
    // (it's optional).

    tokens r = (tokens)pStringPool->TokenOfString(T->m_Id.m_Spelling);

    if (T->m_Id.m_TypeCharacter==chType_String && r==tkMID && !T->m_Id.m_IsBracketed)
    {
        return r;
    }
    else if (T->m_Id.m_TypeCharacter != chType_NONE || T->m_Id.m_IsBracketed)
    {
        return tkID;
    }
    else
    {
        return r;
    }
}

tokens TokenAsKeyword(
    _In_ Token *T,
    Compiler *Compiler)
{
    return TokenAsKeyword(T, Compiler->GetStringPool());
}

tokens TokenAsKeyword(
    _In_ Token *T,
    _In_ StringPool* pStringPool)
{
    return T->m_TokenType == tkID ? IdentifierAsKeyword(T, pStringPool) : T->m_TokenType;
}

//
// Constructor/destructor
//

Scanner::Scanner
(
    Compiler *pCompiler,
    _In_opt_count_(InputStreamLength) const WCHAR *InputStream,
    size_t InputStreamLength,
    long InitialAbsOffset,
    long InitialLineNumber,
    long InitialColumnNumber,
    MethodDeclKind InitialMethodDeclKind
)
: m_Storage(NORLSLOC)
, m_pStringPool(pCompiler->GetStringPool())
, m_PendingStates(NULL)
, m_AllowXmlFeatures(true)
{
    Init(InputStream, InputStreamLength, InitialAbsOffset,InitialLineNumber,InitialColumnNumber,InitialMethodDeclKind);
}

Scanner::Scanner
(
    _In_ StringPool* pStringPool,
    _In_opt_count_(InputStreamLength) const WCHAR *InputStream,
    size_t InputStreamLength,
    long InitialAbsOffset,
    long InitialLineNumber,
    long InitialColumnNumber,
    MethodDeclKind InitialMethodDeclKind
)
: m_Storage(NORLSLOC)
, m_pStringPool(pStringPool)
, m_PendingStates(NULL)
, m_AllowXmlFeatures(true)
{
    Init(InputStream, InputStreamLength, InitialAbsOffset,InitialLineNumber,InitialColumnNumber,InitialMethodDeclKind);
}

// Use this ctor to scan a buffer from an arbitrary start point in the buffer to the end of the buffer.
// Also useful when you don't know the length of the text buffer (it sometimes isn't null-terminated)
// but when you do have a pointer to the end of the text buffer.  We can use that to manage the chunk
// of text you want to scan.
Scanner::Scanner(
    _In_ StringPool *pStringPool,
    _In_ const WCHAR *pInputStream, // start of the text you want the scanner to operate on
    _In_ const WCHAR *pEndOfStream, // marks the end of the text you want the scanner to operate on
    long InitialAbsOffset, // relative to where InputStream is in terms of the overall buffer you are scanning.  
    long InitialLineNumber, // "  Note that this is critical for #Const symbols produced by this scanner to be viable
    long InitialColumnNumber,
    MethodDeclKind InitialMethodDeclKind)
: m_Storage(NORLSLOC)
, m_pStringPool(pStringPool)
, m_PendingStates(NULL)
, m_AllowXmlFeatures(true)
{
    VSASSERT( pEndOfStream >= pInputStream, "Buffer for scanner is 'infinite'" );
    Init(pInputStream, pEndOfStream - pInputStream, InitialAbsOffset, InitialLineNumber, InitialColumnNumber,InitialMethodDeclKind);
}

void
Scanner::Init( 
    _In_opt_count_(InputStreamLength) const WCHAR *InputStream,
    size_t InputStreamLength,
    long InitialAbsOffset, 
    long InitialLineNumber, 
    long InitialColumnNumber,
    MethodDeclKind InitialMethodDeclKind)
{
    VSASSERT(InitialAbsOffset >= 0, "InitialAbsOffset must be positive");
    m_CharacterPositionOfCurrentClusterStart = InitialAbsOffset;

    if (!InputStream)
    {
        // Shouldn't be passing a NULL input stream unless the length is 0
        AssertIfFalse(InputStreamLength == 0);

        InputStream = L"";
        InputStreamLength = 0;
    }

    m_InputStream = InputStream;
    m_InputStreamEnd = InputStream + InputStreamLength;
    m_InputStreamPosition = InputStream;
    m_LineNumber = InitialLineNumber;
    m_LineStart = InputStream - InitialColumnNumber;
    m_FirstInUseToken = NULL;
    m_MethodDeclKind = InitialMethodDeclKind;

    Token *First = new(m_Storage) Token;
    Token *Second = new(m_Storage) Token;

    First->m_Next = Second;
    First->m_Prev = Second;
    Second->m_Prev = First;
    Second->m_Next = First;

    m_FirstFreeToken = First;

    m_State.Init(VB);
}

Scanner::~Scanner
(
)
{
    delete m_PendingStates;
}

//
// Some inline character utilities
//

inline bool
IsDoubleQuote
(
    WCHAR c
)
{
    // Besides the half width and full width ", we also check for Unicode
    // LEFT DOUBLE QUOTATION MARK and RIGHT DOUBLE QUOTATION MARK because
    // IME editors paste them in. This isn't really technically correct
    // because we ignore the left-ness or right-ness, but see VS 170991
    return c == '\"' || c == MAKEFULLWIDTH('\"') || c == 0x201C || c == 0x201D;
}

inline bool
IsFirstIdentifierCharacter
(
    WCHAR c
)
{
    BYTE CharacterProperties = UProp(c);

    return IsPropAlpha(CharacterProperties) ||
           IsPropLetterDigit(CharacterProperties) ||
           IsPropConnectorPunctuation(CharacterProperties);
}

const bool IsIDChar[128] =
{
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false,
//                                                          0      1
    false, false, false, false, false, false, false, false, true,  true,
//  2      3      4      5      6      7      8      9
    true,  true,  true,  true,  true,  true,  true,  true,  false, false,
//                                     A      B      C      D      E
    false, false, false, false, false, true,  true,  true,  true,  true,
//  F      G      H      I      J      K      L      M      N      O
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  P      Q      R      S      T      U      V      W      X      Y
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  Z                                  _             a      b      c
    true,  false, false, false, false, true,  false, true,  true,  true,
//  d      e      f      g      h      i      j      k      l      m
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  n      o      p      q      r      s      t      u      v      w
    true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  x      y      z
    true,  true,  true,  false, false, false, false, false
};

inline bool
IsWideIdentifierCharacter
(
    WCHAR c
)
{
    BYTE CharacterProperties = UProp(c);

    return IsPropAlphaNumeric(CharacterProperties) ||
           IsPropLetterDigit(CharacterProperties) ||
           IsPropConnectorPunctuation(CharacterProperties) ||
           IsPropCombining(CharacterProperties) ||
           IsPropOtherFormat(CharacterProperties);
}

inline bool
IsIdentifierCharacter
(
    WCHAR c
)
{
    if (c < 128)
    {
        return IsIDChar[c];
    }

    return IsWideIdentifierCharacter(c);
}

inline bool
BeginsExponent
(
    WCHAR c
)
{
    return c == 'E' || c =='e' ||
            c == MAKEFULLWIDTH('E') || c == MAKEFULLWIDTH('e');
}

inline bool
BeginsBaseLiteral
(
    WCHAR c
)
{
    return  c == 'H' || c == 'O' ||
            c == 'h' || c == 'o' ||
            c == MAKEFULLWIDTH('H') || c == MAKEFULLWIDTH('O') ||
            c == MAKEFULLWIDTH('h') || c == MAKEFULLWIDTH('o');
}

inline bool
IsLetterC
(
    WCHAR CharToTest
)
{
    return
        CharToTest == 'c' ||
        CharToTest == 'C' ||
        CharToTest == MAKEFULLWIDTH('c') ||
        CharToTest == MAKEFULLWIDTH('C');
}


//
// Useful static routines, used outside Scanner
//

bool Scanner::IsIdentifier
(
    _In_z_ WCHAR *Spelling
)
{

    WCHAR *Spellin----ginal = Spelling;

#pragma prefast(push)
#pragma prefast(disable: 26018, "The buffer accesses are correctly bounded")
    if (IsFirstIdentifierCharacter(*Spelling))
    {
        if (!IsConnectorPunctuation(*Spelling) ||
            IsIdentifierCharacter(*(Spelling + 1)))
        {
            Spelling++;

            // Find the end of the string.
            while (IsIdentifierCharacter(*Spelling))
            {
                Spelling++;
            }
        }
    }

    if ((Spelling - Spellin----ginal) > MaxIdentifierLength)
    {
        return false;
    }

    return *Spelling == WIDE('\0');
#pragma prefast(pop)
}

#if 0   // Not used
bool Scanner::IsDigit
(
    WCHAR c
)
{
    return IsLiteralDigit(c);
}
#endif

bool Scanner::IsValidIdentiferOrBracketedIdentifier(_In_z_ WCHAR *Spelling)
{

    WCHAR *Spellin----ginal = Spelling;

    if (*Spelling == '[' || *Spelling == MAKEFULLWIDTH('['))
    {
        Spelling++;

        if (*Spelling == ']' || *Spelling == MAKEFULLWIDTH(']'))
        {
            return false;
        }

        if (IsFirstIdentifierCharacter(*Spelling))
        {
            if (!IsConnectorPunctuation(*Spelling) ||
                IsIdentifierCharacter(*(Spelling + 1)))
            {
                Spelling++;

                // Find the end of the string.
#pragma prefast(suppress: 26018, "IsIdentifierCharacter correctly evaluates the Null terminating char")
                while (IsIdentifierCharacter(*Spelling))
                {
                    Spelling++;
                }
            }
        }

        if ((Spelling - Spellin----ginal) > MaxIdentifierLength)
        {
            return false;
        }

#pragma prefast(suppress: 26018, "The condition doesn't overflow")
        return (*Spelling == ']' || *Spelling == MAKEFULLWIDTH(']')) && Spelling[1] == WIDE('\0');
    }
    else
    {
        if (IsIdentifier(Spelling))
        {
            return true;
        }
    }

    // If we reach here it's Ill formed/reserved/etc
    return false;

}

bool
Scanner::IsWhitespaceCharacter
(
    WCHAR c
)
{
    return IsBlank(c);
}

bool
Scanner::IsPeriodCharacter
(
    WCHAR c
)
{
    return c == '.' || c == MAKEFULLWIDTH('.');
}

bool
Scanner::IsLeftParenCharacter
(
    WCHAR c
)
{
    return c == '(' || c == MAKEFULLWIDTH('(');
}

bool
Scanner::IsRightParenCharacter
(
    WCHAR c
)
{
    return c == ')' || c == MAKEFULLWIDTH(')');
}

bool
Scanner::IsLeftBraceCharacter
(
    WCHAR c
)
{
    return c == '{' || c == MAKEFULLWIDTH('{');
}

bool
Scanner::IsEqualsCharacter
(
    WCHAR c
)
{
    return c == '=' || c == MAKEFULLWIDTH('=');
}

bool
Scanner::IsGreatThanCharacter
(
    WCHAR c
)
{
    return c == '>' || c == MAKEFULLWIDTH('>');
}

bool
Scanner::IsLessThanCharacter
(
    WCHAR c
)
{
    return c == '<' || c == MAKEFULLWIDTH('<');
}

bool
Scanner::IsCommaCharacter
(
    WCHAR c
)
{
    return c == ',' || c == MAKEFULLWIDTH(',');
}

bool
Scanner::IsNewlineCharacter
(
    WCHAR c
)
{
    return IsLineBreak(c);
}

bool
Scanner::IsDateSeparatorCharacter
(
    WCHAR c
)
{
    return c == '/' || c == MAKEFULLWIDTH('/') ||
           c == '-' || c == MAKEFULLWIDTH('-');
}

bool
Scanner::IsSingleQuote
(
    WCHAR c
)
{
    // Besides the half width and full width ', we also check for Unicode
    // LEFT SINGLE QUOTATION MARK and RIGHT SINGLE QUOTATION MARK because
    // IME editors paste them in. This isn't really technically correct
    // because we ignore the left-ness or right-ness, but see VS 170991
    return c == '\'' || c == MAKEFULLWIDTH('\'') || c == 0x2018 || c == 0x2019;
}

//
// Token creation
//

Token *
Scanner::NextFreeToken
(
)
{
    if (m_FirstFreeToken->m_Next == m_FirstInUseToken) {

        Token *New = new(m_Storage) Token;

        m_FirstFreeToken->m_Next = New;
        New->m_Prev = m_FirstFreeToken;

        New->m_Next = m_FirstInUseToken;
        m_FirstInUseToken->m_Prev = New;
    }

    /* We used to be clever and only clear the token in the case that weren't growing the token ring.
         Because in that case we knew that we were reusing a token that was already in the ring.  But 
         the problem is that this assumed that there would always be a newly init'd token between the
         first free token and the first in use token when we needed to grow the token ring.
         The problem with this is that we broke this invariant in a couple places.  For instance,
         the different flavors of AbandonToken() change either m_FirstFreeToken or m_FirstInUseToken
         to an arbitrary token.  This means that we no longer necessairly have a blank token
         between m_FirstFreeToken and m_FirstInUseToken so we would end up setting m_FirstFreeToken
         to a token that had whatever was there previously.  This became a problem when we
         started relying on line continuation information in the token and we started returning random garbage.
         
         So even though it is true that while initially growing the token ring we are always getting a clean token
         (because we New it just before using it) it isn't true that every call to this function can trust that we are
         in that situation.  And rather than patch up everywhere we broke the invariant that makes this function work,
         I've fixed this issue here by always clearing out the token before we return it. */

    // Stash our location in the ring so we can restore it after we clear the token out
    Token *Prev = m_FirstFreeToken->m_Prev;
    Token *Next = m_FirstFreeToken->m_Next;

    // Re-initing beginning with the m_DummyFirstField in order to avoid writing over the vtable if ever virtuals are
    // introduced in struct Token.
    memset(((BYTE *)m_FirstFreeToken) + FIELD_OFFSET(struct Token, m_DummyFirstField),
                0, sizeof(Token) - FIELD_OFFSET(struct Token, m_DummyFirstField));

    // restore our location in the ring
    m_FirstFreeToken->m_Prev = Prev;
    m_FirstFreeToken->m_Next = Next;

    Token *Result = m_FirstFreeToken;
    VSASSERT(Result->m_EOL.m_isFollowedByBlankLine == false, "We should never have leftover EOL info in the token we are returning");

    m_FirstFreeToken = m_FirstFreeToken->m_Next;

    // If this is the first time we are getting a new token for this line, then this should be the first token in the ring.
    if (m_FirstInUseToken == NULL)
    {
        m_FirstInUseToken = Result;
    }

    return Result;
}

Token *
Scanner::MakeToken
(
    tokens TokenType,
    size_t TokenWidth
)
{
    Token *Result = NextFreeToken();

    Result->m_TokenType = TokenType;
    Result->m_State = m_State;
    Result->m_StartLine = m_LineNumber;
    Result->m_StartColumn = (UINT32)(m_InputStreamPosition - m_LineStart);
    Result->m_StartCharacterPosition =
        (UINT32)(m_CharacterPositionOfCurrentClusterStart +
            (m_InputStreamPosition - m_InputStream));
    Result->m_Width = (long)TokenWidth;

    // Change a tkID token with spelling of "Custom" to tkCUSTOM token when it immediately precedes a tkEVENT
    if (Result->m_TokenType == tkEVENT &&
        Result->m_Prev != NULL &&
        TokenAsKeyword(Result->m_Prev, m_pStringPool) == tkCUSTOM)
    {
        Result->m_Prev->m_TokenType = tkCUSTOM;
    }

    // Change a tkID token "Async" to a tkASYNC token when it immediately
    // precedes a tkSUB or tkFUNCTION or similar. Likewise "Iterator" to tkITERATOR
    if (Result->m_Prev != NULL && 
        (Result->m_TokenType == tkSUB ||
         Result->m_TokenType == tkFUNCTION ||
         Result->m_TokenType == tkPROPERTY ||
         Result->m_TokenType == tkGET ||
         Result->m_TokenType == tkSET ||
         Result->m_TokenType == tkOPERATOR ||
         Result->m_TokenType == tkITERATOR ||
         Result->m_TokenType == tkASYNC ||
         Result->m_TokenType == tkPUBLIC ||
         Result->m_TokenType == tkPROTECTED ||
         Result->m_TokenType == tkFRIEND ||
         Result->m_TokenType == tkPRIVATE ||
         Result->m_TokenType == tkSHADOWS ||
         Result->m_TokenType == tkOVERLOADS ||
         Result->m_TokenType == tkSHARED ||
         Result->m_TokenType == tkMUSTOVERRIDE ||
         Result->m_TokenType == tkOVERRIDABLE ||
         Result->m_TokenType == tkNOTOVERRIDABLE ||
         Result->m_TokenType == tkOVERRIDES ||
         Result->m_TokenType == tkPARTIAL ||
         Result->m_TokenType == tkREADONLY ||
         Result->m_TokenType == tkWRITEONLY ||
         Result->m_TokenType == tkEVENT ||
         Result->m_TokenType == tkDELEGATE))
    {    
        if (Result->m_Prev->m_TokenType == tkID)
        {
            tokens tkPrev = TokenAsKeyword(Result->m_Prev, m_pStringPool);
            if (tkPrev == tkASYNC || tkPrev == tkITERATOR)
            {
                Result->m_Prev->m_TokenType = tkPrev;
                m_SeenAsyncOrIteratorInCurrentLine.Push(tkPrev == tkASYNC ? AsyncKind : IteratorKind);
            }
        }
        else if (Result->m_TokenType == tkFUNCTION || Result->m_TokenType == tkSUB)
        {
            if (Result->m_Prev->m_TokenType == tkEND)
            {   
                // Bug Dev11 135219
                // Consider the following case,
                // ...
                // End Function + Await (Async Funtion ...)
                // 
                // If there is an End before Function, we need to 
                // reset m_SeenAsyncOrInteratorInCurrentLine to 
                // NoneKind, otherwise m_SeenAsyncOrIteratorInCurrent
                // sets to NormalKind, which intreprets Await as an identifier.
                if (!m_SeenAsyncOrIteratorInCurrentLine.Empty())
                {
                    m_SeenAsyncOrIteratorInCurrentLine.Pop();
                }
            }
            else
            {
                m_SeenAsyncOrIteratorInCurrentLine.Push(NormalKind);
            }   
                
        }   
    }

    if (Result->m_Prev != NULL && 
        (Result->m_TokenType == tkID ||
        Result->m_TokenType == tkNOTHING ||
        Result->m_TokenType == tkLBrace  ||
        Result->m_TokenType == tkIntCon  ||
        Result->m_TokenType == tkStrCon  ||
        Result->m_TokenType == tkFltCon  ||
        Result->m_TokenType == tkDecCon  ||
        Result->m_TokenType == tkCharCon ||
        Result->m_TokenType == tkDateCon))
    {
        tokens tkPrev = TokenAsKeyword(Result->m_Prev, m_pStringPool);
        if (tkAWAIT == tkPrev || tkYIELD == tkPrev)
        {
            Result->m_Prev->m_TokenType = tkPrev;
        }
    }

    // The "Iterator Async" or "Async Iterator" case is more subtle,
    // since neither is a keyword. It is special-cased in ScanIdentifier,
    // one of our callers. That's because we don't want to mess with
    // turning async/iterator from tkID into contextual keywords
    // unless we're absolutely sure it's safe.

    return Result;
}

Token *
Scanner::MakeXmlNameToken
(
 tokens TokenType,
 size_t TokenWidth
)
{
    VSASSERT(TokenType == tkXmlNCName, "Wrong token type passed");

    // Protect the string pool from an attempt to insert a string
    // longer than it can handle.

    if (TokenWidth > MaxIdentifierLength)
    {
        Token *Result = MakeToken(tkSyntaxError, TokenWidth);
        Result->m_Error.m_errid = ERRID_IdTooLong;
        TokenWidth = MaxIdentifierLength;
    }

    STRING *IdString;

    IdString = m_pStringPool->AddStringWithLen(m_InputStreamPosition,
                                            TokenWidth);

    if (m_State.IsXmlState())
    {
        if (!ValidateXmlName(IdString, Location::GetHiddenLocation(), NULL))
            m_State.m_IsXmlError = true;
    }

    return MakeXmlNameToken(TokenType, IdString);
}


Token *
Scanner::MakeXmlNameToken
(
 tokens TokenType,
 _In_z_ STRING *IdString
)
{
    Token *Result = MakeToken(TokenType, StringPool::StringLength(IdString));

    Result->m_Id.m_IsBracketed = false;
    Result->m_Id.m_Spelling = IdString;
    Result->m_Id.m_TypeCharacter = chType_NONE;

    return Result;
}

Token *
Scanner::MakeXmlCharToken
(
 tokens TokenType,
 size_t TokenWidth,
 bool IsAllWhitespace
)
{
    VSASSERT(TokenType == tkXmlWhiteSpace ||
             TokenType == tkXmlAttributeData ||
             TokenType == tkXmlCharData ||
             TokenType == tkXmlCData ||
             TokenType == tkXmlPIData ||
             TokenType == tkXmlCommentData, "Wrong token type passed");

    Token *Result = NULL;

    if (TokenWidth > 0)
    {
        if (IsAllWhitespace)
        {
            Result = MakeXmlWhiteSpaceToken(TokenWidth);
        }
        else
        {
            Result = MakeToken(TokenType, TokenWidth);

            Result->m_XmlCharData.m_Value = m_InputStreamPosition;
            Result->m_XmlCharData.m_Length = TokenWidth;
            Result->m_XmlCharData.m_IsWhiteSpace = false;
            Result->m_XmlCharData.m_IsNewLine = false;
            Result->m_XmlCharData.m_Type = Token::XmlCharData;
        }
    }

    return Result;
}


Token *
Scanner::MakeXmlReferenceToken
(
 size_t TokenWidth,
 const WCHAR *Value,
 Token::XmlCharTypes Type
)
{
    Token *Result = MakeToken(tkXmlReference, TokenWidth);

    size_t Length = wcslen(Value);
    Result->m_XmlCharData.m_Length = Length;
    WCHAR *Buffer = new(m_Storage) WCHAR[Length + 1];
    wcscpy_s(Buffer, Length+1, Value);
    Result->m_XmlCharData.m_Value = Buffer;
    Result->m_XmlCharData.m_Type = Type;
    Result->m_XmlCharData.m_IsWhiteSpace = isXmlWhitespace(Value, Length);
    Result->m_XmlCharData.m_IsNewLine = false;
   return Result;
}


Token *
Scanner::MakeXmlWhiteSpaceToken
(
 size_t TokenWidth,
 bool IsNewLine
)
{
    if (TokenWidth > 0)
    {
        Token *Result = MakeToken(tkXmlWhiteSpace, TokenWidth);

        if (!IsNewLine)
        {
            Result->m_XmlCharData.m_Value = m_InputStreamPosition;
            Result->m_XmlCharData.m_Length = TokenWidth;
        }
        else
        {
            Result->m_XmlCharData.m_Value = L"\n";
            Result->m_XmlCharData.m_Length = 1;
        }

        Result->m_XmlCharData.m_Type = Token::XmlCharData;
        Result->m_XmlCharData.m_IsNewLine = IsNewLine;
        Result->m_XmlCharData.m_IsWhiteSpace = true;

        return Result;
    }

    return NULL;
}

Token *
Scanner::MakeStringLiteralToken
(
    _In_ const WCHAR *Termination,
    unsigned EscapedQuoteCount,
    unsigned ClosingQuoteCount
)
{
    Token *Result =
        MakeToken(
            tkStrCon,
            (UINT32)(Termination + ClosingQuoteCount - m_InputStreamPosition));
    Result->m_StringLiteral.m_Length = (UINT32)(Termination - (m_InputStreamPosition + 1)) - EscapedQuoteCount;

    if (EscapedQuoteCount > 0) {

        WCHAR *There = new(m_Storage) WCHAR[Result->m_StringLiteral.m_Length];
        Result->m_StringLiteral.m_Value = There;

        for (const WCHAR *Here = m_InputStreamPosition + 1;
             Here < Termination;
             Here++, There++) {

            WCHAR c = *Here;

            if (IsDoubleQuote(c))
            {
                *There = '\"';
                Here++;
            }
            else
            {
                *There = c;
            }
        }
    }
    else {

        Result->m_StringLiteral.m_Value = m_InputStreamPosition + 1;
    }

    return Result;
}


//
// Methods to eat through characters, but produce no tokens
//

// Accept a CR/LF pair or either in isolation as a newline.
void
Scanner::EatNewline
(
    WCHAR StartCharacter
)
{
    if (IsLineBreak(StartCharacter)) {
        ReadNextChar();

        if (StartCharacter == UCH_CR &&
            NotAtEndOfInput() &&
            PeekNextChar() == UCH_LF)
            ReadNextChar();
    }

    m_LineNumber++;
    m_LineStart = m_InputStreamPosition;
}

void // Eat tabs and spaces (including unicode tabs/space)
Scanner::EatWhitespace
(
)
{
    // Microsoft: consider: this function advances the stream position
    // by at least one - so it assumes we are on whitespace.
    // We need an assert here like: VSASSERT( IsBlank( m_InputStreamPosition ))
    // The lack of which just bit me
    // Better yet would be to not advance the stream position by one before
    // running the loop.  Would have to check callers before making that change
    // though.

    const WCHAR *Here = m_InputStreamPosition + 1;

    while (Here < m_InputStreamEnd && IsBlank(*Here)) {

        Here++;
    }

    m_InputStreamPosition = Here;
}

void
Scanner::EatLineContinuation
(
)
{
    m_InputStreamPosition++; // get off the '_'

    /*  Dev10 #621395
        There is some trickiness regarding what to do with the EOL that follows the '_'
        The way explicit line continuation generally works is that the scanner will
        eat the EOL following '_' so that the parser sees an uninterrupted stream of 
        tokens representing the line.  Thus the '_' glues two lines together by removing
        the EOL at the scanner level so the parser won't see the line break.
        But with implicit line continuation the parser has code to eat EOL's whenever
        implicit line continuation is allowed after a token.  But if we hide the EOL that
        follows the '_' when the next line is blank, the parser can't tell that we had
        two EOLs in a row and will happily eat the one EOL it sees (the EOL right after
        the '_' will have been eaten by the scanner) 
        So what we'll do is no longer eat the EOL following the '_' when the line following
        is blank--so that the parser can see the situation we are in. Explicit line continuation
        isn't legal anyway in this situation. */

    if (NotAtEndOfInput())
    {
        if (IsBlank(PeekNextChar()))
            EatWhitespace();

        if (NotAtEndOfInput())
        {

            // We are on the EOL following '_'  Only eat it this EOL if the next line isn't blank
            if ( IsLineBreak( *m_InputStreamPosition ))
            {
                { // Introduce block scope so backup values will restore when we need them to
                    // Backup up the scanner state while we peek forward
                    BackupValue<long> BackupLineNumber(&m_LineNumber);
                    BackupValue<const WCHAR *> BackupLineStart( &m_LineStart );
                    BackupValue<const WCHAR *> BackupInputStreamPosition( &m_InputStreamPosition );

                    // Look ahead to see if the next line is blank
                    EatNewline( *m_InputStreamPosition );
                    if ( m_InputStreamPosition < m_InputStreamEnd )
                    {
                        if ( IsBlank( *m_InputStreamPosition )) // Bug 660280 - make sure we are on whitespace first
                        {
                            EatWhitespace();
                        }

                        if ( m_InputStreamPosition < m_InputStreamEnd )
                        {
                            // If we hit a line break at this point, this means the entire line was blank.
                            if ( IsLineBreak( *m_InputStreamPosition ))
                            {
                                // return without eating the newline or setting explicit continuation state
                                // backup values set above will restore as we return
                                return; 
                            }
                        }
                    }
                }
    
                // backup values will restore if we make it out of the bock context above
                EatNewline( *m_InputStreamPosition );
            }
        }
    }

    // Save the last token type on the line before the line continuation.
    // This is used by the colorizer to properly colorize xml literals.
    // Note that when the LastToken is tkEOL, that means we had an explicit
    // line continuation on its own line, e.g.
    // dim x = 1 +
    // _
    // 1
    // In that case we don't want to set tkEOL as the last token, since it
    // wasn't, and leave the last token in the state untouched as that already
    // contains the last token on the line (the plus in this case) that we care
    // about.  See Dev10_535602

    Token *LastToken = GetLastToken();
    if (LastToken != NULL && LastToken->m_TokenType != tkEOL ) 
    {
        // If there is a previous token on this line save it
        // else just leave the current value in the m_State.
        // This propagates the last token type in the case there are
        // multiple line continuations and the lines are blank.

        m_State.SetTokenTypeBeforeLineContinuation(LastToken);
    }

    m_State.m_ExplicitLineContinuation = true;
}

void
Scanner::EatThroughLogicalNewline
(
)
{
    while (NotAtEndOfInput())
    {
        WCHAR c = PeekNextChar();

        if (IsLineBreak(c))
        {
            EatNewline(c);
            return;
        }

        if ((c == '_' || c == MAKEFULLWIDTH('_')) && IsLineContinuation())
        {
            EatLineContinuation();
            continue;
        }

        m_InputStreamPosition++;
    }
}

//
// Scan routines, which consume characters and produce tokens
//

void
Scanner::ScanBracketedIdentifier
(
)
{
    const WCHAR *IdStart = m_InputStreamPosition + 1;
    const WCHAR *Here = IdStart;

    bool InvalidIdentifier = false;

    if (!IsFirstIdentifierCharacter(*Here) ||
        (IsConnectorPunctuation(*Here) && !IsIdentifierCharacter(*(Here + 1))))
    {
        InvalidIdentifier = true;
    }

    while (Here < m_InputStreamEnd) {
        WCHAR Next = *Here;

        if (Next == ']' || Next == MAKEFULLWIDTH(']'))
        {
            size_t IdStringLength = (size_t)(Here - IdStart);

            if (IdStringLength > 0 && !InvalidIdentifier)
            {
                // Protect the string pool from an attempt to insert a string
                // longer than it can handle.

                if (IdStringLength > MaxIdentifierLength)
                {
                    Token *Result = MakeToken(tkSyntaxError, IdStringLength + 2);
                    Result->m_Error.m_errid = ERRID_IdTooLong;
                    IdStringLength = MaxIdentifierLength;
                }

                STRING *IdString;

                IdString = m_pStringPool->AddStringWithLen(IdStart,
                                                        IdStringLength);

                Token *Result = MakeToken(tkID, IdStringLength + 2);

                Result->m_Id.m_IsBracketed = true;
                Result->m_Id.m_Spelling = IdString;
                Result->m_Id.m_TypeCharacter = chType_NONE;
            }
            else
            {
                // The sequence "[]" does not define a valid identifier.

                m_InputStreamPosition += 1;
                Token *Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition));
                Result->m_Error.m_errid = ERRID_ExpectedIdentifier;
            }

            m_InputStreamPosition = Here + 1;
            return;
        }
        else if (IsLineBreak(Next)) {
            break;
        }
        else if (!IsIdentifierCharacter(Next))
        {
            InvalidIdentifier = true;
            break;
        }

        Here++;
    }

    UINT32 len = (UINT32)(Here - m_InputStreamPosition);
    if (len >1)
    {
        Token *Result = MakeToken(tkSyntaxError, len );
        Result->m_Error.m_errid = ERRID_MissingEndBrack;
    }
    else
    {
        m_InputStreamPosition += 1;
        Token *Result = MakeToken(tkSyntaxError, 0);
        Result->m_Error.m_errid = ERRID_ExpectedIdentifier;
    }

    m_InputStreamPosition = Here;
}

void Scanner::ScanComment
(
    _In_ const WCHAR *Here,
    bool fRem
)
{
    bool IsDocXMLComment = false;

    // XMLDoc comments have the extra two ticks, but they are not part of the comment text.
    if (!fRem && IsSingleQuote(*Here) && (Here + 1 < m_InputStreamEnd) && IsSingleQuote(*(Here + 1)))
    {
        // Make sure that a sequence of four quotes is not counted as an xml doc comment token
        IsDocXMLComment = (Here + 2 >= m_InputStreamEnd) || !IsSingleQuote(*(Here + 2));
    }

    if (IsDocXMLComment)
    {
        Here += 2;
    }

    const WCHAR *CommentStart = Here;

    while (Here < m_InputStreamEnd) {

        WCHAR Next = *Here;

        if (IsLineBreak(Next))
            break;

        Here++;
    }

    Token *Result = MakeToken(IsDocXMLComment ? tkXMLDocComment : tkREM, (UINT32)(Here - m_InputStreamPosition));

    Result->m_Comment.m_Length = (UINT32)(Here - CommentStart);
    Result->m_Comment.m_Spelling = CommentStart;
    Result->m_Comment.m_IsRem = fRem;
    m_InputStreamPosition = Here;
}


static void CheckQueryContext(_In_ Token * InAsOrEq, _In_ StringPool* pStringPool, ScannerState &m_State)
{
    AssertIfFalse(InAsOrEq &&
                           (InAsOrEq->m_TokenType == tkIN || InAsOrEq->m_TokenType == tkAS || InAsOrEq->m_TokenType == tkEQ));

    // {AGGREGATE | FROM } <id>[?] {In | As | = }
    Token * current = InAsOrEq->m_Prev;

    // Skip '?'
    if(current && tkNullable == current->m_TokenType)
    {
        current = current->m_Prev;
    }

    if(current && current->m_Prev &&
         (tkID == current->m_TokenType || current->IsKeyword()) &&
                current->m_Prev->m_TokenType == tkID)
    {
        tokens keyWord = IdentifierAsKeyword(current->m_Prev, pStringPool);

        if(keyWord == tkAGGREGATE || keyWord == tkFROM)
        {
            // Remember that we're now in a simple query
            m_State.m_IsSimpleQuery = true;
            current->m_Prev->m_State.m_IsSimpleQuery = true;
        }
    }
}

void
Scanner::ScanIdentifier
(
)
{
    const WCHAR *IdStart = m_InputStreamPosition;
    const WCHAR *Here = IdStart + 1;

    VSASSERT(IsFirstIdentifierCharacter(*IdStart), "Identifier scanning is off to a bad start.");

    if (IsConnectorPunctuation(*IdStart) && !IsIdentifierCharacter(*Here))
    {
        Token *Result = MakeToken(tkSyntaxError, 1);
        Result->m_Error.m_errid = ERRID_ExpectedIdentifier;
        Result->m_Error.m_IsTokenForInvalidLineContinuationChar = true;
        m_InputStreamPosition++;
        return;
    }

    // The C++ compiler refuses to inline IsIdentifierCharacter, so the
    // < 128 test is inline here. (This loop gets a *lot* of traffic.)

    while (Here < m_InputStreamEnd &&
           (*Here < 128 ? IsIDChar[*Here] : IsWideIdentifierCharacter(*Here)))
    {
        Here++;
    }

    size_t IdStringLength = (size_t)(Here - IdStart);

    // Protect the string pool from an attempt to insert a string
    // longer than it can handle.

    if (IdStringLength > MaxIdentifierLength)
    {
        Token *Result = MakeToken(tkSyntaxError, IdStringLength);
        Result->m_Error.m_errid = ERRID_IdTooLong;

        IdStringLength = MaxIdentifierLength;
    }

    STRING *IdString;

    IdString =
        m_pStringPool->AddStringWithLen(
            IdStart,
            IdStringLength);

    tokens IdAsKeyword = (tokens)StringPool::TokenOfString(IdString);

    typeChars TypeCharacter = chType_NONE;

    if (IdAsKeyword == tkREM) {

        // Comment.

        ScanComment(Here, true);
        return;
    }

    else if (Here < m_InputStreamEnd) {

        // Check for a type character.
        // Reject appending type characters to reserved words.

        WCHAR Next = *Here;

FullWidthRepeat:
        switch (Next) {

            case '!':

                // If the ! is followed by an identifier it is a dictionary lookup operator,
                // not a type character.

                if (NotCloseToEndOfInput(1) &&
                    (IsFirstIdentifierCharacter(*(Here + 1)) ||
                     *(Here + 1) == '[' || *(Here + 1) == MAKEFULLWIDTH('[') ||
                     *(Here + 1) == ']' || *(Here + 1) == MAKEFULLWIDTH(']')))
                {

                    break;
                }

                TypeCharacter = chType_sR4;
                Here++;
                break;

            case '#':

                TypeCharacter = chType_sR8;
                Here++;
                break;

            case '$':

                TypeCharacter = chType_String;
                Here++;
                break;

            case '%':

                TypeCharacter = chType_sI4;
                Here++;
                break;

            case '&':

                TypeCharacter = chType_sI8;
                Here++;
                break;

            case '@':

                TypeCharacter = chType_sDecimal;
                Here++;
                break;

            default:
                if (ISFULLWIDTH(Next))
                {
                    Next = MAKEHALFWIDTH(Next);
                    goto FullWidthRepeat;
                }
                break;
        }
    }

    tokens IdKeyword = (
        IdAsKeyword != 0 &&
        TokenIsReserved(IdAsKeyword) &&
        TypeCharacter == chType_NONE
        ) ? IdAsKeyword : tkID;

    Token *Result =
        MakeToken(
        IdKeyword,
        (UINT32)(Here - IdStart));

    Result->m_Id.m_IsBracketed = false;
    Result->m_Id.m_Spelling = IdString;
    Result->m_Id.m_TypeCharacter = TypeCharacter;

    // Normally, "Async/Iterator" before a keyword is established inside MakeToken().
    // But in the case of "Async/Iterator before Async/Iterator", we have to do it here
    // using the IdAsKeyword.
    if (IdAsKeyword == tkASYNC || IdAsKeyword == tkITERATOR)
    {
        Token *Prev = GetPrevToken(Result);
        if (Prev != NULL && Prev->m_TokenType == tkID)
        {
            tokens tkPrev = TokenAsKeyword(Prev, m_pStringPool);
            if (tkPrev == tkASYNC || tkPrev == tkITERATOR)
            {
                Prev->m_TokenType = tkPrev;
            }
        }
    }

    MethodDeclKind SeenAsyncOrIteratorInCurrentLine = m_SeenAsyncOrIteratorInCurrentLine.Empty() ? NoneKind : m_SeenAsyncOrIteratorInCurrentLine.Top();
    if (IdAsKeyword == tkYIELD && m_FirstTokenOfLine->m_TokenType != tkSharp)
    {
        if (SeenAsyncOrIteratorInCurrentLine == IteratorKind ||
            (m_MethodDeclKind == IteratorKind && SeenAsyncOrIteratorInCurrentLine == NoneKind))
        {
            Result->m_TokenType = TokenAsKeyword(Result, m_pStringPool);
        }
    }
    else if (IdAsKeyword == tkAWAIT && m_FirstTokenOfLine->m_TokenType != tkSharp)
    {
        if (SeenAsyncOrIteratorInCurrentLine == AsyncKind ||
            (m_MethodDeclKind == AsyncKind && SeenAsyncOrIteratorInCurrentLine == NoneKind))
        {
            Result->m_TokenType = TokenAsKeyword(Result, m_pStringPool);
        }
    }
    

    if (IdKeyword == tkFUNCTION || IdKeyword == tkSUB || IdKeyword == tkOPERATOR)
    {
        // Remember that this line is a function declaration because Xml literals are highly restricted in this case.
        Token *Prev = GetPrevToken(Result);   // Dev10_486780 FunctionDecl tells us that we are scanning Sub foo()  To think we are on a function
        m_State.m_LineType = Prev == NULL ||  //  declaration when we hit END SUB is wrong...  So we now check against tkEND.  
                           (Prev->m_TokenType != tkDot && Prev->m_TokenType != tkBang && Prev->m_TokenType != tkEND) ?
                           FunctionDecl : NotClassified;

        // For Lambda's this will be Reset when parsing the left parenthesis in FunctionDecl context where previous token is tkFunction.
    }
    else if (IdKeyword == tkIMPORTS)
    {
        // Remember that this is an Imports declaration
        Token *Prev = GetPrevToken(Result);
        m_State.m_LineType = Prev == NULL || (Prev->m_TokenType != tkDot && Prev->m_TokenType != tkBang) ?
            ImportStatement : NotClassified;
    }
    else if (IdKeyword == tkIN || IdKeyword == tkAS || IdKeyword == tkEQ)
    {
        CheckQueryContext(Result, m_pStringPool, m_State);
    }
    else if (IdKeyword == tkCASE)
    {
        // Remember that this is a case statement.  Xml can not appear in a case statement unless it is inside of parentheses
        Token *Prev = GetPrevToken(Result);
        m_State.m_LineType = Prev == NULL || (Prev->m_TokenType != tkDot && Prev->m_TokenType != tkBang) ?
            CaseStatement : NotClassified;
    }

    m_InputStreamPosition = Here;
}

void
Scanner::ScanXmlAttributeIdentifier
(
)
{
    VSASSERT(PeekNextChar() == '@' ||  MAKEHALFWIDTH(PeekNextChar()) == '@', "Xml attribute identifier needs to start with the '@' symbol");

    MakeToken(tkAt, 1);
    m_InputStreamPosition++;

    if (NotAtEndOfInput() && PeekNextChar() == '<') {
        // Scan bracketed qualified name
        ScanBracketedXmlQualifiedName();
    }
    else {
        // Scan first part of qualified name
        ScanXmlAttributeNCName();

        if (NotAtEndOfInput() && PeekNextChar() == ':')
        {
            // Scan second part of qualified name
            MakeToken(tkXmlColon, 1);
            m_InputStreamPosition++;
            ScanXmlAttributeNCName();
        }
    }
}

void
Scanner::ScanXmlAttributeNCName
(
)
{
    const WCHAR * Start = m_InputStreamPosition;
    Token * PreviousToken;
    size_t IdStringLength;

    // Attribute name is restricted to VB identifier characters
    if (IsFirstIdentifierCharacter(*m_InputStreamPosition))
        ScanIdentifier();

    IdStringLength = m_InputStreamPosition - Start;

    if (IdStringLength <= 0)
    {
        // Zero-length identifier is not valid
        Token *Result = MakeToken(tkSyntaxError, 0);
        Result->m_Error.m_errid = ERRID_ExpectedXmlName;
    }

    // Set TokenType to XmlNCName rather than ID
    PreviousToken = GetLastToken();
    if (PreviousToken->m_TokenType == tkID || PreviousToken->IsKeyword())
    {
        PreviousToken->m_TokenType = tkXmlNCName;
    }
}

// Scans Xml qualified name with angle brackets, i.e. <foo:bar>
void
Scanner::ScanBracketedXmlQualifiedName
(
)
{
    VSASSERT(PeekNextChar() == '<', "Bracketed Xml qualified name needs to start with the '<' symbol");

    MakeToken(tkLT, 1);
    m_InputStreamPosition++;

    ScanXmlQName();

    // Do not allow whitespace between end of QName and '>' token -- make zero-length token and let parser detect problem
    if (NotAtEndOfInput())
    {
        WCHAR c = PeekNextChar();
        if (c == '>')
        {
            // Bug 39579: Must disambiguate 'e.<foo> >' case from shift right operator
            MakeToken(tkGT, 1);
            m_InputStreamPosition++;
        }
        else if (IsWhitespaceCharacter(PeekNextChar()))
        {
            MakeToken(tkXmlWhiteSpace, 0);
        }
    }
}

void
Scanner::ScanGetXmlNamespace
(
)
{
    // Treat GetXmlNamespace as if it were a single VB token (i.e. no whitespace or line continuations allowed)
    if (NotAtEndOfInput() && PeekNextChar() == '(') {
        MakeToken(tkLParen, 1);
        m_InputStreamPosition++;

        ScanXmlNCName();
    }

    // Do not allow whitespace after any of the tokens -- make zero-length token and let parser detect problem
    if (NotAtEndOfInput() && IsWhitespaceCharacter(PeekNextChar()))
    {
        MakeToken(tkXmlWhiteSpace, 0);
    }
}

void
Scanner::ScanStringLiteral
(
)
{
    const WCHAR *literal_start = m_InputStreamPosition + 1;
    const WCHAR *Here = literal_start;
    WCHAR c;

    // Check for a Char literal, which can be of the form:
    // """"c or "<anycharacter-except-">"c

    if (NotCloseToEndOfInput(3) && IsDoubleQuote(PeekAheadChar(2)))
    {
        if (IsDoubleQuote(PeekAheadChar(1)))
        {
            if (IsDoubleQuote(PeekAheadChar(3)) &&
                NotCloseToEndOfInput(4) &&
                IsLetterC(PeekAheadChar(4)))
            {
                // Double-quote Char literal: """"c

                Token *Result = MakeToken(tkCharCon, 5);
                Result->m_CharLiteral.m_Value = '\"';
                m_InputStreamPosition += 5;

                return;
            }
        }
        else if (IsLetterC(PeekAheadChar(3)))
        {
            // Char literal.

            Token *Result = MakeToken(tkCharCon, 4);
            Result->m_CharLiteral.m_Value = PeekAheadChar(1);
            m_InputStreamPosition += 4;

            return;
        }
    }

    if (NotCloseToEndOfInput(2) &&
        IsDoubleQuote(PeekAheadChar(1)) &&
        IsLetterC(PeekAheadChar(2)))
    {
        // Error. ""c is not a legal char constant
        Token *Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition + 1));
        Result->m_Error.m_errid = ERRID_IllegalCharConstant;
        m_InputStreamPosition = Here;
    }

    unsigned escaped_quote_count = 0;

    while (Here < m_InputStreamEnd) {

        c = *Here;

        if (IsDoubleQuote(c)) {

            c = (Here + 1 < m_InputStreamEnd) ? *(Here + 1) : '\0';

            if (IsDoubleQuote(c)) {

                // An escaped double quote

                escaped_quote_count++;
                Here++;
            }

            else {

                // The end of the literal
                if (Here + 1 < m_InputStreamEnd &&
                    IsLetterC(*(Here + 1)))
                {
                    // Error. "aad"c is not a legal char constant

                    // +2 to include the 'c' in the token span
                    Token *Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition + 2));
                    Result->m_Error.m_errid = ERRID_IllegalCharConstant;
                    m_InputStreamPosition = Here + 2;
                }
                else
                {
                    MakeStringLiteralToken(Here, escaped_quote_count, 1);
                    m_InputStreamPosition = Here + 1;
                }
                return;
            }
        }

        else if (IsLineBreak(c)) {
            break;
        }

        Here++;
    }
#if IDE 
    Token *Result = MakeStringLiteralToken(Here, escaped_quote_count, 0);
    //VS#305044, don't eat a line break
    //DDB#104407, make sure we don't dereference past the end of the file!
    //If a file has an unterminated string *without* a CRLF, this will crash if we do *Here.
    m_InputStreamPosition = Here;
    if (NotAtEndOfInput() && Here && *Here && !IsLineBreak(*Here))
    {
        m_InputStreamPosition += 1;
    }

    Result->m_StringLiteral.m_HasUnReportedUnterminatedStringError = true;
#else
    // The literal does not have an explicit termination.
    Token *Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition));
    Result->m_Error.m_errid = ERRID_UnterminatedStringLiteral;
    m_InputStreamPosition = Here;
#endif
}

bool
Scanner::ScanIntLiteral
(
    __int64 *IntegerValue,
    _Inout_ const WCHAR **CurrentPosition
)
{
    __int64 IntegralValue;
    const WCHAR *Here = *CurrentPosition;

    if (Here == m_InputStreamEnd)
        return false;

    if (!IsLiteralDigit(*Here))
        return false;

    IntegralValue = IntegralLiteralCharacterValue(*Here);
    Here++;

    while (Here < m_InputStreamEnd && IsLiteralDigit(*Here))
    {
        unsigned __int64 LastIntegralValue = IntegralValue;
        IntegralValue *= 10;

        if (LastIntegralValue > 0x7fffffff &&
            IntegralValue / 10 != LastIntegralValue)
        {
            return false;
        }

        IntegralValue += IntegralLiteralCharacterValue(*Here);
        Here++;
    }

    if (IntegralValue < 0)
    {
        return false;
    }

    *CurrentPosition = Here;
    *IntegerValue = IntegralValue;
    return true;
}

bool
Scanner::ScanDateLiteral
(
)
{
    const WCHAR *LiteralStart = m_InputStreamPosition;
    const WCHAR *Here = LiteralStart + 1;
    __int64 FirstValue;
    __int64 YearValue, MonthValue, DayValue, HourValue, MinuteValue, SecondValue;
    __int64 DateTimeValue;
    bool HaveDateValue = false;
    bool HaveYearValue = false;
    bool HaveTimeValue = false;
    bool HaveMinuteValue = false;
    bool HaveSecondValue = false;
    bool HaveAM = false;
    bool HavePM = false;
    bool DateIsInvalid = false;
    bool YearIsTwoDigits = false;
    int *DaysToMonth = NULL;

    // Unfortunately, we can't fall back on OLE Automation's date parsing because
    // they don't have the same range as the URT's DateTime class

    // First, eat any whitespace
    while (Here < m_InputStreamEnd && IsBlank(*Here))
    {
        Here++;
    }

    // The first thing has to be an integer, although it's not clear what it is yet
    if (!ScanIntLiteral(&FirstValue, &Here))
    {
        return false;
    }

    // If we see a /, then it's a date
    if (Here < m_InputStreamEnd && IsDateSeparatorCharacter(*Here))
    {
        const WCHAR *FirstDateSeparator = Here;

        // We've got a date
        HaveDateValue = true;
        Here++;
        MonthValue = FirstValue;

        // We have to have a day value
        if (!ScanIntLiteral(&DayValue, &Here))
        {
            goto baddate;
        }

        // Do we have a year value?
        if (Here < m_InputStreamEnd && IsDateSeparatorCharacter(*Here))
        {
            // Check to see they used a consistent separator
            if (*Here != *FirstDateSeparator)
            {
                goto baddate;
            }

            // Yes.
            HaveYearValue = true;
            Here++;

            const WCHAR *YearStart = Here;

            if (!ScanIntLiteral(&YearValue, &Here))
            {
                goto baddate;
            }

            if ((Here - YearStart) == 2)
            {
                YearIsTwoDigits = true;
            }
        }

        while (Here < m_InputStreamEnd && IsBlank(*Here))
        {
            Here++;
        }
    }

    // If we haven't seen a date, assume it's a time value
    if (!HaveDateValue)
    {
        HaveTimeValue = true;
        HourValue = FirstValue;
    }
    else
    {
        // We did see a date. See if we see a time value...
        if (ScanIntLiteral(&HourValue, &Here))
        {
            // Yup.
            HaveTimeValue = true;
        }
    }

    if (HaveTimeValue)
    {
        // Do we see a :?
        if (Here < m_InputStreamEnd && (*Here == ':' || *Here == MAKEFULLWIDTH(':')))
        {
            Here++;

            // Now let's get the minute value
            if (!ScanIntLiteral(&MinuteValue, &Here))
            {
                goto baddate;
            }

            HaveMinuteValue = true;

            // Do we have a second value?
            if (Here < m_InputStreamEnd && (*Here == ':' || *Here == MAKEFULLWIDTH(':')))
            {
                // Yes.
                HaveSecondValue = true;
                Here++;

                if (!ScanIntLiteral(&SecondValue, &Here))
                {
                    goto baddate;
                }
            }
        }

        while (Here < m_InputStreamEnd && IsBlank(*Here))
        {
            Here++;
        }

        // Check AM/PM
        if (Here < m_InputStreamEnd)
        {
            if (*Here == 'A' || *Here == MAKEFULLWIDTH('A') ||
                *Here == 'a' || *Here == MAKEFULLWIDTH('a'))
            {
                HaveAM = true;
                Here++;
            }
            else if (*Here == 'P' || *Here == MAKEFULLWIDTH('P') ||
                     *Here == 'p' || *Here == MAKEFULLWIDTH('p'))
            {
                HavePM = true;
                Here++;
            }

            if (Here < m_InputStreamEnd && (HaveAM || HavePM))
            {
                if(*Here == 'M' || *Here == MAKEFULLWIDTH('M') ||
                   *Here == 'm' || *Here == MAKEFULLWIDTH('m'))
                {
                    Here++;
                    while (Here < m_InputStreamEnd && IsBlank(*Here))
                    {
                        Here++;
                    }
                }
                else
                {
                    goto baddate;
                }
            }
        }

        // If there's no minute/second value and no AM/PM, it's invalid
        if (!HaveMinuteValue && !HaveAM && !HavePM)
        {
            goto baddate;
        }

    }

    if (Here == m_InputStreamEnd || !(*Here == '#' || *Here == MAKEFULLWIDTH('#')))
    {
        // Oooh, so close. But no cigar
        goto baddate;
    }

    Here++;

    // OK, now we've got all the values, let's see if we've got a valid date

    if (HaveDateValue)
    {
        if (MonthValue < 1 || MonthValue > 12)
            DateIsInvalid = true;

        // We'll check Days in a moment...
        if (!HaveYearValue)
        {
#if !IDE
            DateIsInvalid = true;
            YearValue = 1;
#else
            SYSTEMTIME LocalTime;

            GetLocalTime(&LocalTime);
            YearValue = LocalTime.wYear;
#endif
        }

        // Check if not a leap year
        if (!((YearValue % 4 == 0) && (!(YearValue % 100 == 0) || (YearValue % 400 == 0))))
        {
            DaysToMonth = DaysToMonth365;
        }
        else
        {
            DaysToMonth = DaysToMonth366;
        }

        if (DayValue < 1 ||
            (!DateIsInvalid && DayValue > DaysToMonth[MonthValue] - DaysToMonth[MonthValue - 1]))
            DateIsInvalid = true;

        if (YearIsTwoDigits)
        {
#if !IDE
            DateIsInvalid = true;
#else
            DWORD iTwoYearMax = GetDateWindow();

            if (YearValue <= (iTwoYearMax % 100))
                YearValue += iTwoYearMax - (iTwoYearMax % 100);
            else
                YearValue += (iTwoYearMax - 99) - ((iTwoYearMax - 99) % 100);
#endif
        }

        if (YearValue < 1 || YearValue > 9999)
            DateIsInvalid = true;
    }
    else
    {
        MonthValue = 1;
        DayValue = 1;
        YearValue = 1;
        DaysToMonth = DaysToMonth365;
    }

    if (HaveTimeValue)
    {
        if (HaveAM || HavePM)
        {
            // 12-hour value
            if (HourValue < 1 || HourValue > 12)
                DateIsInvalid = true;

            if (HaveAM)
            {
                HourValue = HourValue % 12;
            }
            else if (HavePM)
            {
                HourValue = HourValue + 12;

                if (HourValue == 24)
                    HourValue = 12;
            }
        }
        else
        {
            if (HourValue < 0 || HourValue > 23)
                DateIsInvalid = true;
        }

        if (HaveMinuteValue)
        {
            if (MinuteValue < 0 || MinuteValue > 59)
                DateIsInvalid = true;
        }
        else
        {
            MinuteValue = 0;
        }

        if (HaveSecondValue)
        {
            if (SecondValue < 0 || SecondValue > 59)
                DateIsInvalid = true;
        }
        else
        {
            SecondValue = 0;
        }
    }
    else
    {
        HourValue = 0;
        MinuteValue = 0;
        SecondValue = 0;
    }

    // Ok, we've got a valid value. Now make into an i8.
    if (!DateIsInvalid)
    {
        YearValue--;
        DateTimeValue = (YearValue * 365 + YearValue / 4 - YearValue / 100 + YearValue / 400 + DaysToMonth[MonthValue - 1] + DayValue - 1) * 864000000000;
        DateTimeValue += (HourValue * 3600 + MinuteValue * 60 + SecondValue) * 10000000;

        Token *Result = MakeToken(tkDateCon, (UINT32)(Here - LiteralStart));
        Result->m_DateLiteral.m_Value = DateTimeValue;
    }
    else
    {
        Token *Result = MakeToken(tkSyntaxError, (UINT32)(Here - LiteralStart));
        Result->m_Error.m_errid = ERRID_InvalidDate;
    }

    m_InputStreamPosition = Here;
    return true;

baddate:
    // If we can find a closing #, then assume it's a malformed date,
    // otherwise, it's not a date
    while (Here < m_InputStreamEnd && *Here != '#' && *Here != MAKEFULLWIDTH('#') && !IsLineBreak(*Here))
    {
        Here++;
    }

    if (Here == m_InputStreamEnd || IsLineBreak(*Here))
    {
        // No closing #
        return false;
    }
    else
    {
        Here++;

        Token *Result = MakeToken(tkSyntaxError, (UINT32)(Here - LiteralStart));
        Result->m_Error.m_errid = ERRID_InvalidDate;
        m_InputStreamPosition = Here;
        return true;
    }

}

void
Scanner::ScanNumericLiteral
(
)
{
    const WCHAR *LiteralStart = m_InputStreamPosition;
    const WCHAR *IntegerLiteralStart;
    const WCHAR *Here = LiteralStart;

    Token::LiteralBase Base = Token::Decimal;
    bool IsFloat = false;

    Token *Result;

    // First read a leading base specifier, if present, followed by a sequence of zero
    // or more digits.

    if (*Here == '&' || *Here == MAKEFULLWIDTH('&'))
    {
        Here++;

        WCHAR c = (Here < m_InputStreamEnd) ? *Here : 0;

FullWidthRepeat:
        switch (c)
        {
            case 'H':
            case 'h':
                Here++;
                IntegerLiteralStart = Here;
                Base = Token::Hexadecimal;
                while (Here < m_InputStreamEnd)
                {
                    c = *Here;
                    if(!IsHexDigit(c))
                        break;

                    Here++;
                }

                break;

            case 'O':
            case 'o':
                if (c == 'o' || c == 'O' ||
                     c == MAKEFULLWIDTH('o') || c == MAKEFULLWIDTH('O'))
                {
                    Here++;
                }
                IntegerLiteralStart = Here;
                Base = Token::Octal;
                while (Here < m_InputStreamEnd)
                {
                    c = *Here;

                    if ((c < '0' || c > '7') &&
                         (c < MAKEFULLWIDTH('0') || c > MAKEFULLWIDTH('7')))
                        break;

                    Here++;
                }

                break;

            default:
                if (ISFULLWIDTH(c))
                {
                    c = MAKEHALFWIDTH(c);
                    goto FullWidthRepeat;
                }

                VSFAIL("We should never get here, the caller should have caught this!");

                Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition));
                Result->m_Error.m_errid = ERRID_Syntax;

                m_InputStreamPosition = Here;
                return;
        }
    }

    else
    {
        IntegerLiteralStart = Here;

        while (Here < m_InputStreamEnd)
        {
            WCHAR c = *Here;

            if (!IsLiteralDigit(c))
                break;

            Here++;
        }
    }

    const WCHAR *IntegerLiteralEnd = Here;

    // Unless there was an explicit base specifier (which indicates an integer literal),
    // read the rest of a float literal.

    if (Base == Token::Decimal && Here < m_InputStreamEnd)
    {
        // First read a '.' followed by a sequence of one or more digits.

        if ((*Here == '.' || *Here == MAKEFULLWIDTH('.')) &&
            (Here + 1) < m_InputStreamEnd &&
            IsLiteralDigit(*(Here + 1)))
        {
            Here++;

            while (Here < m_InputStreamEnd)
            {
                WCHAR c = *Here;

                if (!IsLiteralDigit(c))
                    break;

                Here++;
            }

            IsFloat = true;
        }

        // Read an exponent symbol followed by an optional sign and a sequence of
        // one or more digits.

        if (Here < m_InputStreamEnd && BeginsExponent(*Here))
        {
            Here++;

            if (Here < m_InputStreamEnd)
            {
                WCHAR c = *Here;

                if (c == '+' || c == '-' ||
                     c == MAKEFULLWIDTH('+') || c == MAKEFULLWIDTH('-'))
                {
                    Here++;
                }
            }

            WCHAR c = (Here < m_InputStreamEnd) ? *Here : 0;

            if (IsLiteralDigit(c))
            {
                Here++;

                while (Here < m_InputStreamEnd)
                {
                    c = *Here;

                    if (!IsLiteralDigit(c))
                        break;

                    Here++;
                }
            }

            else
            {
                Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition));

                Result->m_Error.m_errid = ERRID_InvalidLiteralExponent;

                m_InputStreamPosition = Here;
                return;
            }

            IsFloat = true;
        }
    }

    // Read a trailing type character.

    typeChars TypeCharacter = chType_NONE;

    if (Here < m_InputStreamEnd)
    {

        WCHAR c = *Here;

FullWidthRepeat2:
        switch (c)
        {
            case '!':

                if (Base == Token::Decimal) {
                    TypeCharacter = chType_sR4;
                    IsFloat = true;
                }
                break;

            case 'F':
            case 'f':

                if (Base == Token::Decimal) {
                    TypeCharacter = chType_R4;
                    IsFloat = true;
                }
                break;

            case '#':

                if (Base == Token::Decimal) {
                    TypeCharacter = chType_sR8;
                    IsFloat = true;
                }
                break;

            case 'R':
            case 'r':

                if (Base == Token::Decimal) {
                    TypeCharacter = chType_R8;
                    IsFloat = true;
                }
                break;

            case 'S':
            case 's':

                if (!IsFloat)
                    TypeCharacter = chType_I2;
                break;

            case '%':

                if (!IsFloat)
                    TypeCharacter = chType_sI4;
                break;

            case 'I':
            case 'i':

                if (!IsFloat)
                    TypeCharacter = chType_I4;
                break;

            case '&':

                if (!IsFloat)
                    TypeCharacter = chType_sI8;
                break;

            case 'L':
            case 'l':

                if (!IsFloat)
                    TypeCharacter = chType_I8;
                break;

            case '@':

                if (Base == Token::Decimal)
                    TypeCharacter = chType_sDecimal;
                break;

            case 'D':
            case 'd':

                if (Base == Token::Decimal)
                    TypeCharacter = chType_Decimal;

                if (Here < m_InputStreamEnd &&
                    (IsLiteralDigit(*(Here + 1)) ||
                     *(Here + 1) == '+' ||
                     *(Here + 1) == '-' ||
                     *(Here + 1) == MAKEFULLWIDTH('+') ||
                     *(Here + 1) == MAKEFULLWIDTH('-')))
                {
                    m_InputStreamPosition = Here;

                    Result = MakeToken(tkSyntaxError, 1);

                    Result->m_Error.m_errid = ERRID_ObsoleteExponent;

                    return;
                }
                break;

            case 'U':
            case 'u':

                if (!IsFloat && Here < m_InputStreamEnd)
                {
                    WCHAR NextChar = *(Here + 1);

                    if (NextChar == 'S' || NextChar == MAKEFULLWIDTH('S') ||
                        NextChar == 's' || NextChar == MAKEFULLWIDTH('s'))
                    {
                        TypeCharacter = chType_U2;
                    }
                    else if (NextChar == 'I' || NextChar == MAKEFULLWIDTH('I') ||
                             NextChar == 'i' || NextChar == MAKEFULLWIDTH('i'))
                    {
                        TypeCharacter = chType_U4;
                    }
                    else if (NextChar == 'L' || NextChar == MAKEFULLWIDTH('L') ||
                             NextChar == 'l' || NextChar == MAKEFULLWIDTH('l'))
                    {
                        TypeCharacter = chType_U8;
                    }
                }
                break;

            default:
                if (ISFULLWIDTH(c))
                {
                    c = MAKEHALFWIDTH(c);
                    goto FullWidthRepeat2;
                }
                break;
        }
    }

    // Copy the text of the literal so that it can be null terminated. Boo.

    WCHAR LiteralScratch[128];
    WCHAR *LiteralSpelling = LiteralScratch;
    unsigned LiteralLength = (unsigned)(Here - LiteralStart);
    unsigned CurrentCharacter;

    if (LiteralLength >= 128)
        LiteralSpelling = new(m_Storage) WCHAR[LiteralLength + 1];

    for (CurrentCharacter = 0; CurrentCharacter < LiteralLength; CurrentCharacter++)
    {
        LiteralSpelling[CurrentCharacter] = MAKEHALFWIDTH(LiteralStart[CurrentCharacter]);
    }

    LiteralSpelling[LiteralLength] = 0;

    // The type character has not been advanced over, so that it won't have been
    // copied to the literal spelling.
    if (TypeCharacter != chType_NONE)
    {
        Here++;
    }

    // Advance it again for the special type characters of size two.
    if (TypeCharacter == chType_U2 || TypeCharacter == chType_U4 || TypeCharacter == chType_U8)
    {
        Here++;
    }

    // Produce a value for the literal.

    unsigned __int64 IntegralValue = 0;
    double FloatingValue;
    DECIMAL DecimalValue;
    Vtypes LiteralType;
    bool Overflows = false;

    if (TypeCharacter == chType_Decimal ||
        TypeCharacter == chType_sDecimal)
    {

        if (FAILED(VarDecFromStr(
                    LiteralSpelling,
                    LCID_US_ENGLISH,
                    LOCALE_NOUSEROVERRIDE,
                    &DecimalValue)))
        {
            Overflows = true;
        }

        else
            LiteralType = t_decimal;
    }

    else
    {
        if (!IsFloat)
        {
            if (IntegerLiteralStart == IntegerLiteralEnd)
            {
                Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition));

                Result->m_Error.m_errid = ERRID_Syntax;

                m_InputStreamPosition = Here;
                return;

            }
            else
            {
                LiteralType = t_i8;

                IntegralValue = IntegralLiteralCharacterValue(*IntegerLiteralStart);

                if (Base == Token::Decimal)
                {
                    for (const WCHAR *LiteralCharacter = IntegerLiteralStart + 1;
                         LiteralCharacter < IntegerLiteralEnd;
                         LiteralCharacter++)
                    {
                        int NextCharacterValue = IntegralLiteralCharacterValue(*LiteralCharacter);

                        if (IntegralValue > 1844674407370955161ui64 ||
                            (IntegralValue == 1844674407370955161ui64 && NextCharacterValue > 5))
                        {
                            Overflows = true;
                        }

                        IntegralValue = (IntegralValue * 10) + NextCharacterValue;
                    }

                    if (TypeCharacter != chType_U8 && (__int64)IntegralValue < 0)
                    {
                        Overflows = true;
                    }
                }
                else
                {
                    int Shift =
                        (Base == Token::Hexadecimal ? 4 : 3);
                    unsigned __int64 OverflowMask =
                        (Base == Token::Hexadecimal ? 0xf000000000000000ui64 : 0xe000000000000000ui64);

                    for (const WCHAR *LiteralCharacter = IntegerLiteralStart + 1;
                         LiteralCharacter < IntegerLiteralEnd;
                         LiteralCharacter++)
                    {
                        if (IntegralValue & OverflowMask)
                        {
                            Overflows = true;
                        }

                        IntegralValue = (IntegralValue << Shift) + IntegralLiteralCharacterValue(*LiteralCharacter);
                    }
                }

                if (TypeCharacter == chType_NONE)
                {
                }
                else if (TypeCharacter == chType_I4 || TypeCharacter == chType_sI4)
                {
                    if ((Base == Token::Decimal && IntegralValue > 0x7fffffff) ||
                        IntegralValue > 0xffffffff)
                    {
                        Overflows = true;
                    }

                    IntegralValue = (__int32)IntegralValue;
                }
                else if (TypeCharacter == chType_U4)
                {
                    if (IntegralValue > 0xffffffff)
                    {
                        Overflows = true;
                    }

                    IntegralValue = (unsigned __int32)IntegralValue;
                }
                else if (TypeCharacter == chType_I2)
                {
                    if ((Base == Token::Decimal && IntegralValue > 0x7fff) ||
                        IntegralValue > 0xffff)
                    {
                        Overflows = true;
                    }

                    IntegralValue = (__int16)IntegralValue;
                }
                else if (TypeCharacter == chType_U2)
                {
                    if (IntegralValue > 0xffff)
                    {
                        Overflows = true;
                    }

                    IntegralValue = (unsigned __int16)IntegralValue;
                }
                else
                {
                    VSASSERT(TypeCharacter == chType_I8 || TypeCharacter == chType_sI8 || TypeCharacter == chType_U8,
                              "Integral literal value computation is lost.");
                }
            }
        }

        if (IsFloat)
        {
            LiteralType = t_double;

            // Attempt to convert to double.

            if (FAILED(VarR8FromStr(
                            LiteralSpelling,
                            LCID_US_ENGLISH,
                            LOCALE_NOUSEROVERRIDE,
                            &FloatingValue)))
            {
                Overflows = true;
            }
            else
            {
                // The value correctly converts to double. Attempt to convert to single
                // only if the requested type is single.

                if (TypeCharacter == chType_R4 || TypeCharacter == chType_sR4)
                {
                    float f;

                    if (VarR4FromR8(FloatingValue, &f) != NOERROR)
                    {
                        Overflows = true;
                    }
                }
                else
                {
                }
            }
        }
    }

    if (Overflows)
    {
        Result = MakeToken(tkSyntaxError, (UINT32)(Here - m_InputStreamPosition));

        Result->m_Error.m_errid = ERRID_Overflow;
    }

    else
    {

        switch (LiteralType)
        {
            case t_i8:

                Result = MakeToken(tkIntCon, (UINT32)(Here - m_InputStreamPosition));

                Result->m_IntLiteral.m_Base = Base;
                Result->m_IntLiteral.m_Value = (__int64)IntegralValue;
                Result->m_IntLiteral.m_TypeCharacter = TypeCharacter;

                break;

            case t_ui8:

                Result = MakeToken(tkIntCon, (UINT32)(Here - m_InputStreamPosition));

                Result->m_IntLiteral.m_Base = Base;
                Result->m_IntLiteral.m_Value = (unsigned __int64)IntegralValue;
                Result->m_IntLiteral.m_TypeCharacter = TypeCharacter;

                break;

            case t_double:

                Result = MakeToken(tkFltCon, (UINT32)(Here - m_InputStreamPosition));

                Result->m_FloatLiteral.m_Value = FloatingValue;
                Result->m_FloatLiteral.m_TypeCharacter = TypeCharacter;

                break;

            case t_decimal:

                Result = MakeToken(tkDecCon, (UINT32)(Here - m_InputStreamPosition));

                Result->m_DecimalLiteral.m_Value = DecimalValue;
                Result->m_DecimalLiteral.m_TypeCharacter = TypeCharacter;

                break;
        }
    }

    m_InputStreamPosition = Here;
}

void
Scanner::ScanLeftAngleBracket
(
)
{
    const WCHAR *Here = m_InputStreamPosition + 1;

    // Allow whitespace between the characters of a two-character token.
    while (Here < m_InputStreamEnd && IsBlank(*Here)) {
        Here++;
    }

    if (Here < m_InputStreamEnd)
    {
        WCHAR c = *Here;

        if (c == '=' || c == MAKEFULLWIDTH('=')) {

            Here++;
            MakeToken(tkLE, (UINT32)(Here - m_InputStreamPosition));
            m_InputStreamPosition = Here;

            return;
        }

        else if (c == '>' || c == MAKEFULLWIDTH('>')) {

            Here++;
            MakeToken(tkNE, (UINT32)(Here - m_InputStreamPosition));
            m_InputStreamPosition = Here;

            return;
        }

        else if (c == '<' || c == MAKEFULLWIDTH('<')) {

            Here++;
            c = *Here;
            if (c != '%' && c != MAKEFULLWIDTH('%'))
            {
                TryFollowingEquals(tkShiftLeft, tkShiftLeftEQ, Here);
                return;
            }
        }
    }

    MakeToken(tkLT, 1);
    m_InputStreamPosition++;
}

void
Scanner::ScanRightAngleBracket
(
)
{
    const WCHAR *Here = m_InputStreamPosition + 1;

    // Allow whitespace between the characters of a two-character token.
    while (Here < m_InputStreamEnd && IsBlank(*Here)) {
        Here++;
    }

    if (Here < m_InputStreamEnd)
    {
        WCHAR c = *Here;

        if (c == '=' || c == MAKEFULLWIDTH('=')) {

            Here++;
            MakeToken(tkGE, (UINT32)(Here - m_InputStreamPosition));
            m_InputStreamPosition = Here;

            return;
        }
        else if (c == '>' || c == MAKEFULLWIDTH('>')) {

            Here++;
            TryFollowingEquals(tkShiftRight, tkShiftRightEQ, Here);

            return;
        }

    }

    MakeToken(tkGT, 1);
    m_InputStreamPosition++;
}

//
// Some useful scanning routines
//

void
Scanner::TryFollowingEquals
(
    tokens DefaultToken,
    tokens AlternativeToken,
    _In_ const WCHAR *Start
)
{
    const WCHAR *Here = Start;

    // Allow whitespace between the characters of a two-character token.
    while (Here < m_InputStreamEnd && IsBlank(*Here)) {
        Here++;
    }

    if (Here < m_InputStreamEnd &&
        (*Here == '=' || *Here == MAKEFULLWIDTH('='))) {

        Here++;
        MakeToken(AlternativeToken, (UINT32)(Here - m_InputStreamPosition));
        m_InputStreamPosition = Here;
    }

    else {
        MakeToken(DefaultToken, (UINT32)(Start - m_InputStreamPosition));
        m_InputStreamPosition = Start;
    }
}

void
Scanner::TryFollowingEquals
(
    tokens OneCharToken,
    tokens AlternativeToken
)
{
    TryFollowingEquals(
        OneCharToken,
        AlternativeToken,
        m_InputStreamPosition + 1);
}

void
Scanner::TryTwoFollowingAlternatives
(
    tokens OneCharToken,
    WCHAR AlternativeChar1,
    tokens AlternativeToken1,
    WCHAR AlternativeChar2,
    tokens AlternativeToken2
)
{
    const WCHAR *Here = m_InputStreamPosition + 1;

    // Allow whitespace between the characters of a two-character token.
    while (Here < m_InputStreamEnd && IsBlank(*Here)) {
        Here++;
    }

    if (Here < m_InputStreamEnd) {

        WCHAR c = *Here;

        if (c == AlternativeChar1 || c == MAKEFULLWIDTH(AlternativeChar1)) {

            Here++;
            MakeToken(AlternativeToken1, (UINT32)(Here - m_InputStreamPosition));
            m_InputStreamPosition = Here;

            return;
        }

        else if (c == AlternativeChar2 || c == MAKEFULLWIDTH(AlternativeChar2)) {

            Here++;
            MakeToken(AlternativeToken2, (UINT32)(Here - m_InputStreamPosition));
            m_InputStreamPosition = Here;

            return;
        }
    }

    MakeToken(OneCharToken, 1);
    m_InputStreamPosition++;
}

bool
Scanner::IsLineContinuation
(
)
{
    const WCHAR *Here = m_InputStreamPosition + 1;

    if ((m_InputStreamPosition != m_InputStream) && IsBlank(*(m_InputStreamPosition - 1)))
    {
        if (Here < m_InputStreamEnd)
        {
            // Skip whitespace.
            while (Here < m_InputStreamEnd && IsBlank(*Here))
                Here++;

            if (IsLineBreak(*Here))
                return true;
        }
        else if (Here == m_InputStreamEnd)
        {
            // When called by the colorizer there may not be a new line char after the '_'.
            return true;
        }
    }

    return false;
}

bool
Scanner::XmlCanFollow
(
    _In_opt_ Token *Prev
)
{
    // </ is always Xml.  Case line starts with < and its a closing tag for fragments
    if (NotCloseToEndOfInput(1) && PeekAheadChar(1) == '/')
    {
        return true;
    }

    tokens PrevTokenType;

    if (Prev)
    {
        PrevTokenType = Prev->m_TokenType;
        if ( PrevTokenType == tkEOL )
        {
            /* dev10_427763 (and probably others) 
                When we are on an EOL, we should look to see if we encountered an implicit line contination.
                And if we did, grab what the previous token was so we can determine whether XML can continue
                the implicit line continuation or not  */
            if ( Prev->m_State.IsVBImplicitLineContinuationState())
            {
                PrevTokenType = m_State.TokenTypeBeforeLineContinuation();
            }
        }
    }
    else
    {
        PrevTokenType = m_State.IsVBState() ? m_State.TokenTypeBeforeLineContinuation() : tkNone;
    }

    if (m_State.m_LineType == FunctionDecl)
    {
        // Handle case of optional parameter in function/sub
        if (PrevTokenType == tkEQ)
            return true;

        return false;
    }
    else if (m_State.m_LineType == CaseStatement)
    {
        // Xml can not appear in a case statement unless it is inside of parentheses due to
        //  Case >=9, <20
        if (PrevTokenType == tkLParen)
            return true;

        return false;
    }
    else if (PrevTokenType == tkRParen &&
             (Prev && Prev->m_State.m_IsRParensOfLambdaParameter ||
              !Prev && m_State.m_IsRParensOfLambdaParameter))
    {
        return true;
    }

    switch (PrevTokenType)
    {
    // It never makes sense for xml to follow IN, ON, AS keywords
    // User can always escape by using parentheses.
    case tkIN:
    case tkON:
    case tkAS:
    // These are tokens whuch can be the last token of a term
    case tkID:
    case tkGLOBAL:
    case tkMYBASE:
    case tkMYCLASS:
    case tkME:
    case tkRParen:
    case tkTRUE:
    case tkFALSE:
    case tkNOTHING:
    case tkSHORT:
    case tkUSHORT:
    case tkINTEGER:
    case tkUINTEGER:
    case tkLONG:
    case tkULONG:
    case tkDECIMAL:
    case tkSINGLE:
    case tkDOUBLE:
    case tkSBYTE:
    case tkBYTE:
    case tkBOOLEAN:
    case tkCHAR:
    case tkDATE:
    case tkSTRING:
    case tkVARIANT:
    case tkOBJECT:
    case tkDateCon:
    case tkCharCon:
    case tkDecCon:
    case tkFltCon:
    case tkStrCon:
    case tkIntCon:
    case tkRBrace:
        //Case of new line
    case tkNone:
    case tkEOL:
        // Case of sequence of code attributes for example  <CLSCompliant(True)> : <HideModuleName()>
    case tkColon:
    case tkGT:
        // Case of Xml element member identifier for example e.<foo>
    case tkXmlNCName:
        // Case of Xml attribute member identifier for example e.@foo
	case tkDot:
    case tkXmlEndComment:
    case tkXmlEndPI:
    case tkXmlEndCData:
    case tkFUNCTION: // if we land here m_LineType isn't FunctionDecl so we are on the end function and XML can't follow directly after
    case tkNullable:
        // case of new ID? or EXPR? for any identifier ID or expression EXPR.
    case tkSyntaxError:
        // if the token before < was an error then not enough information to decide.
        return false;
    default:
        // todo - Microsoft - This fails for line continuation case.
        // Need a way to remember dot from prior lines.
        // This catches . ID when ID is a keyword.
        // Fix - When last token on line is keyword save tkID instead of the tkKeyword.
        if (Token::IsKeyword(PrevTokenType))
        {
            if (Prev && Prev->m_Prev->m_TokenType == tkDot)
            {
                return false;
            }
        }

    }

    return true;
}


//
// Called when current character is '(' but before the tkLParen is created
// This is used to detect a VB call on the next line inside of partial Xml.
//
// NL WS* ID WS* (
// Example  Console.WriteLine (
//

Token*
Scanner::CheckXmlForCallOrCodeAttribute()
{
    Token *t = GetLastToken();

    t = SkipXmlWhiteSpace(t);

    if (t == NULL || t->m_TokenType != tkXmlNCName)
    {
        return NULL;
    }

    t = GetPrevToken(t);

    if (t && t->m_TokenType == tkLT)
    {
        // Code attribute case

        // If "<tkNCName (" is found then back up to the tkLT and switch back to VB
        // scanning. A code attribute has mistakenly been scanned as an Xml literal.

        if (!m_State.m_IsDocument && m_State.OpenXmlElements() == 0)
        {
            return t;
        }
    }
    else
    {
        // Call case

        t = SkipXmlWhiteSpace(t);

        if (!t ||  t->m_TokenType == tkXmlWhiteSpace &&
            t->m_XmlCharData.m_IsNewLine)
        {
            return t ? t : m_FirstTokenOfLine;
        }
    }

    return NULL;
}

//
// Called after a tkXmlName is scanned in the element state
// This is used to detect a VB statement on the next line inside of partial Xml.
//
// NL WS* KW WS* ID | KW
// Example  Dim x
// or
// NL WS* # KW
// Example #END
// 

Token*
Scanner::CheckXmlForStatement()
{
    Token *t = GetLastToken();
    Token *id = t;

    t = GetPrevToken(t);

    if (t && t->m_TokenType == tkXmlWhiteSpace)
    {
        t = SkipXmlWhiteSpace(t);
    }

    if (!t)
        return NULL;

    if (t->m_TokenType == tkXmlNCName)
    {
        tokens type = TokenOfString(t->m_Id.m_Spelling);
        if (type == tkNone || !Token::IsKeyword(type))
        {
            return NULL;
        }
    }
    else if (t->m_TokenType == tkSharp)
    {

        // The following if branch is added to fix bug 729007
        // This fix assumes that if m_Spelling is null then id 
        // must not be a keyword.
        
        AssertIfFalse( id->m_Id.m_Spelling != NULL || !id->IsKeyword() );
        if (id->m_Id.m_Spelling == NULL)
        {
            return NULL;
        }
        tokens type = TokenOfString(id->m_Id.m_Spelling);
        if (type == tkNone || !Token::IsKeyword(type))
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }

    t = GetPrevToken(t);

    if (t && t->m_TokenType == tkXmlWhiteSpace)
    {
        t = SkipXmlWhiteSpace(t);
    }

    if (!t)
    {
        return m_FirstTokenOfLine;
    }

    if (t->m_TokenType == tkXmlWhiteSpace &&
        t->m_XmlCharData.m_IsNewLine)
    {
        return t;
    }

    return NULL;
}


Token *
Scanner::SkipXmlWhiteSpace(_In_opt_ Token *t)
{
    while (t && t->m_TokenType == tkXmlWhiteSpace && !t->m_XmlCharData.m_IsNewLine)
    {
        t = GetPrevToken(t);
    }

    return t;
}

bool
Scanner::PrecededByNonWhitespace
(
)
{
    return
        m_InputStreamPosition > m_LineStart &&
        !IsBlank(*(m_InputStreamPosition - 1));
}

//
// Stream reading functions
//

// Copy a string from the text into the buffer.
void
Scanner::CopyString
(
    unsigned Start,
    unsigned End,
    _Out_capcount_((End - Start)) WCHAR *Buffer
)
{
    VSASSERT((m_CharacterPositionOfCurrentClusterStart >= 0) && (static_cast<unsigned long>(m_CharacterPositionOfCurrentClusterStart) <= Start),
        "Parameter Start must exceed positive m_CharacterPositionOfCurrentClusterStart");

    // Copy it in.
    memcpy(Buffer, m_InputStream + (Start - m_CharacterPositionOfCurrentClusterStart), (End - Start) * sizeof(WCHAR));
}

// SkipToNextConditionalLine advances through the input stream until it finds a (logical)
// line that has a '#' character as its first non-whitespace, non-continuation character.
// SkipToNextConditionalLine ignores implicit line continuation.

void
Scanner::SkipToNextConditionalLine
(
)
{
    while (NotAtEndOfInput())
    {
        WCHAR c = PeekNextChar();

        switch (c)
        {
            case '\r':
            case '\n':

                EatNewline(c);
                continue;

            case ' ':
            case '\t':

                EatWhitespace();
                continue;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':
            case '\'':

                EatThroughLogicalNewline();
                continue;

            case '#':

                return;

            case '_':

                if (IsLineContinuation())
                {
                    EatLineContinuation();
                    continue;
                }

                EatThroughLogicalNewline();
                continue;

            default:

                if (IsBlank(c)) {

                    EatWhitespace();
                    continue;
                }

                else if (IsLineBreak(c))
                {
                    EatNewline(c);
                    continue;
                }

                EatThroughLogicalNewline();
                continue;
        }
    }
}

static void
VerifyLineNotTooLong
(
    Token *FirstTokenOfLine,
    _Inout_ Token *LastTokenOfLine
)
{
    // Most of the compiler uses 16-bit numbers to keep track of column numbers, which
    // limits lines to 65535 characters.
    const long MaxLineLength = 65535;

    if (LastTokenOfLine->m_StartColumn + LastTokenOfLine->m_Width > MaxLineLength)
    {
        // The line is too long. Find the token that crosses the boundary, make it a
        // syntax error, and patch the location info for all tokens that follow it to
        // make them fall within the allowable range.

        Token *OverlimitToken = LastTokenOfLine;
        while (OverlimitToken != FirstTokenOfLine &&
               OverlimitToken->m_Prev->m_StartColumn +
                   OverlimitToken->m_Prev->m_Width > MaxLineLength)
        {
            // This isn't the first one over the line.
            // Patch the location info and move on.

            if (OverlimitToken->m_StartColumn >= MaxLineLength)
            {
                OverlimitToken->m_StartColumn = MaxLineLength - 1;
            }
            OverlimitToken->m_Width = MaxLineLength - OverlimitToken->m_StartColumn;

            OverlimitToken = OverlimitToken->m_Prev;
        }

        if (OverlimitToken->m_TokenType != tkEOF && OverlimitToken->m_TokenType != tkEOL)
        {
            //We can't change tkEOF, tkEOL since they are used to mark the end of the input
            // see Dev10_669804 and Dev10_772259
            OverlimitToken->m_TokenType =  tkSyntaxError;
       
            OverlimitToken->m_Error.m_errid = ERRID_LineTooLong;
        }

        // Make the location info of the error token the point where the line
        // becomes too long.

        OverlimitToken->m_StartColumn = MaxLineLength - 1;
        OverlimitToken->m_Width = MaxLineLength - OverlimitToken->m_StartColumn;
    }
}

unsigned // returns the offset from the current index into the text where whitespace ends
Scanner::FindEndOfWhitespace
(
    unsigned peekPos // [in] Start looking for whitespace at this offset from the current index into the text
)
{
    // ****************************************************
    // Microsoft: !!!! MANAGED COMPILER PORT !!!!
    // Aleksey added +1 to peekPos in the managed compiler.
    // I have fixed NotCloseToEndOfInput() so that should be
    // reverted in the managed compiler when porting changes
    // *****************************************************
    while ( NotCloseToEndOfInput(peekPos) && 
            IsWhitespaceCharacter(PeekAheadChar(peekPos))
    )
    {
        ++peekPos;
    }    

    return peekPos;
}


void 
Scanner::CheckForAnotherBlankLine
(
    Token * pToken
)
{
    unsigned peekPos = FindEndOfWhitespace(0); // find the offset where we hit whitespace
    
    // ****************************************************
    // Microsoft: !!!! MANAGED COMPILER PORT !!!!
    // Aleksey added +1 to peekPos in the managed compiler.
    // I have fixed NotCloseToEndOfInput() so that should be
    // reverted in the managed compiler when porting changes
    // *****************************************************
    if (! NotCloseToEndOfInput(peekPos) || IsNewlineCharacter(PeekAheadChar(peekPos)))
    {
        pToken->m_EOL.m_isFollowedByBlankLine = true;
    }
}

/*****************************************************************************************
;LineAllowsImplicitLineContinuation

Some lines end with a token that implies a line continuation.  To imply line continuation 
means that you don't have to have an explicit '_' at the end of the line in order for you 
to continue the line on the next line.  For instance: Sub Foo( <- the lparen allows for 
implicit line continuation so that the args can be placed on the next line.
This function determines whether the line ends in a token that allows for implicit line
continuation.

It assumes that we get called while sitting on the EOL at the end of the line.
******************************************************************************************/
bool // TRUE-the line ends with a token that implies line continuation / FALSE - it doesn't
Scanner::LineAllowsImplicitLineContinuation()
{
    VSASSERT( IsLineBreak(*m_InputStreamPosition), 
              "LineAllowsImplicitLineContinuation() assumes that we get called while sitting on the end of the line");

    bool IsImplicitLineContinuation = false;
    if ( IsLineBreak(*m_InputStreamPosition))
    {
        Token *lastToken = GetLastToken(); // last token that we scanned

        if ( lastToken && lastToken->CanContinueAtEnd(m_pStringPool))
        {
            IsImplicitLineContinuation = true;
        }
        else
        {
            // make sure our scanner state reflects the fact that we aren't in implied line continuation mode
            m_State.EndVBContinuationState(); 
            IsImplicitLineContinuation = false;
        }
    }
    return IsImplicitLineContinuation;
}


Token *
Scanner::ScanVBLine
(
)
{

    
    while (NotAtEndOfInput()) {

        WCHAR c = PeekNextChar();

FullWidthRepeat:
        switch (c) {

            case '\r':
            case '\n':
            {
                /* Reset state to to VB. This clears all flags such m_ExplicitLineContinuation and m_IsFunctionDecl.
                   But don't reset the scanner state if we are in the middle of processing XML because we may have something like this:
                   dim x = <xml><%=      <-- here we are processing an XML tag that isn't complete yet
                      1+2  <-- this will scan in VB mode.  When we hit the EOL we don't want to lose the fact that we are processing the <%= tag
                        %></xml>  Dev10_429857 */
                bool ScannerIsProcessingXML = PendingStates() && m_PendingStates->m_Stack.Top().IsXmlState();
                bool ImplicitLineContinuation = LineAllowsImplicitLineContinuation();
                if ( !ScannerIsProcessingXML)
                {
                    if ( ImplicitLineContinuation )
                    {
                        m_State.Init( VBImplicitLineContinuation ); // clear out everything except the things we want to propogate for line continuation
                    }
                    else
                    {
                        m_State.Init(VB);

                        m_SeenAsyncOrIteratorInCurrentLine.Reset();
                        // 


                    }
                }
                
                if ( ImplicitLineContinuation )
                {
                    Token *pLastToken = GetLastToken();
                    m_State.BeginVBContinuationState( pLastToken ? pLastToken->m_TokenType : tkNone );
                }
              
                Token *EOLToken = MakeToken(tkEOL, 0);
                EatNewline(c); 
                if ( NotAtEndOfInput())
                {
                    CheckForAnotherBlankLine(EOLToken); 
                }
                
                /* Don't return if we are in the middle of parsing an XML expression.  The parser will deal with the EOL we generated above but we
                   need to keep scanning so we don't lose all our XML scanner state.  Dev10_429857 */
                if ( !ScannerIsProcessingXML )
                {
                    return m_FirstTokenOfLine;
                }

                EOLToken->m_EOL.m_NextLineAlreadyScanned = true; // we are going to keep scanning. 
                continue;
            }
            case ' ':
            case '\t':

                EatWhitespace();
                continue;

            case '\'':

                ScanComment(m_InputStreamPosition + 1, false);
                continue;

            case '@':

                ScanXmlAttributeIdentifier();
                continue;

            case '(':
            {
                unsigned lambdaNesting = m_State.LambdaParameterNesting();

                // For Lambda's we use the function keyword.
                // this is used by Xml to indicate we are inside a function and disables the Xml Literals.
                if (lambdaNesting > 0)
                {
                    if (!m_State.SetLambdaParameterNesting(lambdaNesting + 1))
                    {
                        Token *Result = MakeToken(tkSyntaxError, 1);
                        Result->m_Error.m_errid = ERRID_TooDeepNestingOfParensInLambdaParam;
                        m_InputStreamPosition++;
                        continue;
                    }
                }
                else if (m_State.m_LineType == FunctionDecl && GetLastToken() && ((GetLastToken()->m_TokenType == tkFUNCTION) || (GetLastToken()->m_TokenType == tkSUB)))
                {
                    m_State.SetLambdaParameterNesting(1);
                }

                MakeToken(tkLParen, 1);
                m_InputStreamPosition++;
                
                continue;
            }
            case ')':
                {
                unsigned lambdaNesting = m_State.LambdaParameterNesting();
                if (lambdaNesting > 0)
                {
                    if (lambdaNesting == 1)
                    {
                        m_State.m_LineType = NotClassified;
                        m_State.m_IsRParensOfLambdaParameter = true; // This value will be embedded in the state of the tkRParen created below
                    }
                    m_State.SetLambdaParameterNesting(lambdaNesting - 1);
                }
                else
                {
                    m_State.m_IsRParensOfLambdaParameter = false; // This value will be embedded in the state of the tkRParen created below
                    m_State.SetLambdaParameterNesting(0);
                }

                MakeToken(tkRParen, 1);
                m_InputStreamPosition++;
                continue;
                }
            case '{':

               MakeToken(tkLBrace, 1);
               m_InputStreamPosition++;
               continue;

            case '}':

               MakeToken(tkRBrace, 1);
               m_InputStreamPosition++;
               continue;

            case ',':

                MakeToken(tkComma, 1);
                m_InputStreamPosition++;
                continue;

            case '#':

                if (!ScanDateLiteral()) {

                    Token *pSharpToken = MakeToken(tkSharp, 1);
                    m_InputStreamPosition++;
                }

                continue;

            case '&':

                if (NotCloseToEndOfInput(1) &&
                    BeginsBaseLiteral(PeekAheadChar(1))) {

                    ScanNumericLiteral();
                }
                else {

                    TryFollowingEquals(tkConcat, tkConcatEQ);
                }

                continue;

            case '=':

                {
                    Token *Result = MakeToken(tkEQ, 1);
                    m_InputStreamPosition++;

                    CheckQueryContext(Result, m_pStringPool, m_State);
                }

                continue;

            case '<':

                // If the previous token in this line is a '.' then parse this as XML instead of as an attribute
                if (GetLastTokenType() == tkDot ||
                    // Dev10_535610 - XML can follow '.' but make sure what lies before is actually '.' (which may be followed by implicit line continuation)
                    // Note: XMLCanFollow() isn't the place for this test since it will reset the XML parsing mode and
                    // we want to continue on in our current state, which is VB implicit line continuation mode.
                    (m_State.IsVBImplicitLineContinuationState() &&
                     m_State.TokenTypeBeforeLineContinuation() == tkDot &&
                     GetLastTokenType() == tkEOL ))
                {
                    ScanBracketedXmlQualifiedName();
                }
                else
				{
                    if (m_State.m_LineType == ImportStatement)
                    {
                        // Imports <xmlns:p="ns">
                        m_State.BeginXmlState(XmlElement);
                        MakeToken(tkLT, 1);
                        m_InputStreamPosition++;
                        return m_FirstTokenOfLine;
                    }
					else if (m_AllowXmlFeatures && XmlCanFollow(GetLastToken()))
                    {
                        if (ISFULLWIDTH(PeekNextChar()))
                        {
                            // Check that c is really an ascii LT and not a FULLWIDTH LT (0xFF1C)
                            Token *Result = MakeToken(tkSyntaxError, 1);

                            Result->m_Error.m_errid = ERRID_FullWidthAsXmlDelimiter;
                            m_InputStreamPosition++;
                        }

                        m_State.BeginXmlState(XmlMarkup);
                        MakeToken(tkLT, 1);
                        m_InputStreamPosition++;
                        return m_FirstTokenOfLine;
                    }
                    else
                    {
                        ScanLeftAngleBracket();
                    }
                }
                continue;

            case '>':

                ScanRightAngleBracket();
                continue;

            case ':':

                TryFollowingEquals(tkColon, tkColEq);
                continue;

            case '+':

                TryFollowingEquals(tkPlus, tkPlusEQ);
                continue;

            case '-':

                TryFollowingEquals(tkMinus, tkMinusEQ);
                continue;

            case '*':

                TryFollowingEquals(tkMult, tkMultEQ);
                continue;

            case '/':

                TryFollowingEquals(tkDiv, tkDivEQ);
                continue;

            case '\\':

                TryFollowingEquals(tkIDiv, tkIDivEQ);
                continue;

            case '^':

                TryFollowingEquals(tkPwr, tkPwrEQ);
                continue;

            case '!':

                MakeToken(tkBang, 1);
                m_InputStreamPosition++;
                continue;

            case '.':

                if (NotCloseToEndOfInput(1) &&
                    IsLiteralDigit(PeekAheadChar(1)))
                {
                    ScanNumericLiteral();
                }
                else
                {
                    MakeToken(tkDot, 1);
                    m_InputStreamPosition++; // move off the dot
                } // handle the '.' for the non-numeric case
                continue;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':

                ScanNumericLiteral();
                continue;

            case '\"':

                ScanStringLiteral();
                continue;

            // Keywords identified as occurring frequently enough to warrant special
            // handling are As, End, and If.

            case 'A':

                if (NotCloseToEndOfInput(2) &&
                    PeekAheadChar(1) == 's' &&
                    PeekAheadChar(2) == ' ')
                {
                    Token *Result = MakeToken(tkAS, 2);

                    Result->m_Id.m_IsBracketed = false;
                    Result->m_Id.m_Spelling = STRING_CONST2(m_pStringPool, AsToken);
                    Result->m_Id.m_TypeCharacter = chType_NONE;

                    AdvanceChars(3);

                    CheckQueryContext(Result, m_pStringPool, m_State);
                }

                else
                    ScanIdentifier();
                continue;

            case 'E':

                if (NotCloseToEndOfInput(3) &&
                    PeekAheadChar(1) == 'n' &&
                    PeekAheadChar(2) == 'd' &&
                    PeekAheadChar(3) == ' ')
                {
                    Token *Result = MakeToken(tkEND, 3);

                    Result->m_Id.m_IsBracketed = false;
                    Result->m_Id.m_Spelling = STRING_CONST2(m_pStringPool, EndToken);
                    Result->m_Id.m_TypeCharacter = chType_NONE;

                    AdvanceChars(4);
                }

                else
                    ScanIdentifier();
                continue;

            case 'I':

                if (NotCloseToEndOfInput(2) &&
                    PeekAheadChar(1) == 'f' &&
                    PeekAheadChar(2) == ' ')
                {
                    Token *Result = MakeToken(tkIF, 2);

                    Result->m_Id.m_IsBracketed = false;
                    Result->m_Id.m_Spelling = STRING_CONST2(m_pStringPool, IfToken);
                    Result->m_Id.m_TypeCharacter = chType_NONE;

                    AdvanceChars(3);
                }

                else
                    ScanIdentifier();
                continue;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
            case 'B':
            case 'C':
            case 'D':
            case 'F':
            case 'G':
            case 'H':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':

                ScanIdentifier();

                // Scan GetXmlNamespace prefix using Xml rules
                if (GetLastTokenType() == tkGETXMLNAMESPACE)
                {
                    ScanGetXmlNamespace();
                }
                continue;

            case '_':
            {
                if (IsLineContinuation())
                {
                    EatLineContinuation();
                    continue;
                }

                ScanIdentifier();
                continue;
            }

            case '[':

                ScanBracketedIdentifier();
                continue;

            case '?':
                MakeToken(tkNullable, 1);
                m_InputStreamPosition++;
                continue;

            default:

                if (IsFirstIdentifierCharacter(c)) {

                    ScanIdentifier();
                    continue;
                }

                else if (IsBlank(c)) {

                    EatWhitespace();
                    continue;
                }

                else if (IsLineBreak(c))
                {
                    Token *LastTokenOfLine = MakeToken(tkEOL, 0);
                    LastTokenOfLine->m_EOL.m_NextLineAlreadyScanned = false;
                    
                    EatNewline(c);
                    CheckForAnotherBlankLine(LastTokenOfLine);
                    
                    return m_FirstTokenOfLine;
                }

                else if (IsDoubleQuote(c))
                {
                    ScanStringLiteral();
                    continue;
                }

                else if (IsSingleQuote(c))
                {
                    ScanComment(m_InputStreamPosition + 1, false);
                    continue;
                }

                else if (ISFULLWIDTH(c))
                {
                    c = MAKEHALFWIDTH(c);
                    goto FullWidthRepeat;
                }
                else {
                    if (c == '%')
                    {
                        // %> is only valid when parsing VB inside of an xml literal.
                        // It marks the end of a VB statement or expression.

                        if (NotCloseToEndOfInput(1) && PeekAheadChar(1) == '>')
                        {
                            if (PendingStates())
                                PopState();

                            Token *LastTokenOfLine = MakeToken(tkXmlEndEmbedded, 2);
                            m_InputStreamPosition += 2;

                            if (m_State.IsXmlState())
                                return m_FirstTokenOfLine;

                            continue;
                        }
                    }
                    // Bogus character.
                    long Width = 1;

                    // Don't break up surrogate pairs
                    if (NotCloseToEndOfInput(1) &&
                        IsHighSurrogate(c) &&
                        IsLowSurrogate(PeekAheadChar(1)))
                    {
                        Width = 2;
                    }

                    Token *Result = MakeToken(tkSyntaxError, Width);

                    Result->m_Error.m_errid = ERRID_IllegalChar;

                    AdvanceChars(Width);
                    continue;
                }
        }
    }

    return m_FirstTokenOfLine;
}

void
Scanner::PushState()
{
    if (!m_PendingStates)
    {
        m_PendingStates = new StateStack();
    }

    m_PendingStates->m_Stack.Push(m_State);
}


void
Scanner::PopState()
{
    VSASSERT(m_PendingStates == NULL ||!m_PendingStates->m_Stack.Empty(), "Very surprising, stack should never be empty");

    if (PendingStates())
    {
        m_State = m_PendingStates->m_Stack.Top();
        m_PendingStates->m_Stack.Pop();
        return;
    }

    // Go back to VB scanning if the stack is empty
    m_State.Init(VB);
}

ColorizerState
Scanner::GetState()
{
    if (PendingStates())
    {
        PushState();
        ColorizerState State(m_PendingStates);

        // m_PendingStates = NULL;
        return State;
    }

    ColorizerState State(m_State);
    return State;
}


ColorizerState
Scanner::GetState(_In_ Token *LastToken)
{
    StateStack *States = NULL;
	Token *T;
    for (T = m_FirstTokenOfLine;
        T != m_FirstFreeToken;
        T = T->m_Next)
    {
        if (T->m_TokenType == tkXmlBeginEmbedded)
        {
            if (States == NULL)
                States = new StateStack();

            States->m_Stack.Push(T->m_State);
        }
        else if (T->m_TokenType == tkXmlEndEmbedded)
        {
            if (States && !States->m_Stack.Empty())
                States->m_Stack.Pop();
        }

        if (T == LastToken)
        {
            break;
        }
    }

    VSASSERT(T == LastToken, "Token does not appear in this line");

    if (States && !States->m_Stack.Empty())
    {
        States->m_Stack.Push(LastToken->m_State);
        ColorizerState State(States);
        delete States;
        return State;
    }
    else
    {
        ColorizerState State(LastToken->m_State);
        delete States;
        return State;
    }
}

/*****************************************************************************************
;GetNextLine

Produces tokens for the next line of text.  Tokens are appended to the current token ring.
This variation of GetNextLine() allows the caller to tell the scanner what LineType it 
should assume when starting the scan.
This is unfortunately necessary in cases where the implicit line continuation comes *before*
a token such as ')'  The existing VBImplicitLineContinuation state doesn't work for tokens
where the line continuation occurs before the token instead of after it.  So we have to have
the parser hint to us what mode we should be scanning in.
I'm hoping we can get rid of this once we move XML parsing to the parser.  Having it in the
scanner as we do makes it necessary for the parser to give us crib notes on how to parse
sometimes.  See dev10_504604
******************************************************************************************/
Token *
Scanner::GetNextLine(
    LineType UseThisLineType // the line type we want the scanner to use when it starts scanning
)
{
    m_State.Init(VB);
    SetStateStack(NULL);
    
    m_State.m_LineType = UseThisLineType;
    m_FirstTokenOfLine = m_FirstFreeToken; // We'll add tokens to the end of the token ring
    
    if ( m_State.m_LineType != FunctionDecl )
    {
        m_AllowXmlFeatures = true; // we always allow xml features when this function is called.  The colorizer is the only one that sets
                                   // this to be false and it has a separate entry point to the scanner.  So force true.
    }

    return GetNextLineHelper();
}

Token *
Scanner::GetNextLine(_In_ ColorizerState& State, bool ResetFirstToken, bool AllowXmlFeatures)
{
    if (State.IsSimpleState())
    {
        SetStateStack(NULL);
        m_State = State.m_ScannerState;
    }
    else
    {
        SetStateStack(State.m_Stack);
        State.m_Stack = NULL;
        PopState();
    }

    VSASSERT(m_PendingStates == NULL ||!m_PendingStates->m_Stack.Empty(), "Very surprising, stack should never be empty");

    if (ResetFirstToken)
    {
        m_FirstTokenOfLine = m_FirstFreeToken;
        m_State.m_ExplicitLineContinuation = false;
    }

    m_AllowXmlFeatures = AllowXmlFeatures;

    return GetNextLineHelper();
}


/*****************************************************************************************
;GetNextLine

Produces tokens for the next line of text.  Tokens are appended to the current token ring.
There are a couple quirks here:
- you can pass in a tokenType which will change the context the scanner thinks it is in.
  Grepping the code base seems to indicate that people did this to allow XML evaulation 
  when the scanner might otherwise think it is scanning an attribute (see ParseOneExpression()
  in the parser) or that we are processing Imports (ParseOneImportsDirective() in the parser)
- another quirk is that we can be restarting the scanner in VBImplicitLineContinuation mode.
  That mode doesn't completely clear the scanner state out in order to remember what token we
  were looking at on the previous line to provide context whether XML parsing is legal or not.
  If you provide a TokenType, however, that will trump VBImplicitLineContinuation mode and
  we will scan the line solely in the context of the previous token information you give us.
******************************************************************************************/
Token *
Scanner::GetNextLine(
    tokens TokenTypeEndingPreviousLine // [in - defaults to tkNone] you can force the scanner to think this was the last token we scanned.
                                       // Providing a value besides tkNONE will override the scanner's VBImplicitLineContinuation state
                                       // which means that the line will be scanned as if it stood alone, though with the context of what
                                       // the last token on the previous line was.
)
{
    if ( TokenTypeEndingPreviousLine != tkNone || // if the TokenTypeEndingPreviousLine is passed in then we assume they don't want to continue previous context
        !m_State.IsVBImplicitLineContinuationState())
    {
        m_State.Init(VB);
        SetStateStack(NULL);
        m_State.SetTokenTypeBeforeLineContinuation(TokenTypeEndingPreviousLine);
    }
    else // The caller didn't provide context on the previous line and we are in implicit continuation mode
    {
        // If we are in continuation mode we want to remember our state so we can continue scanning with the context
        // where we left off.
        m_State.Init(VBImplicitLineContinuation);
        SetStateStack(NULL); // But we don't maintain a stack when when we are in VBImplicitContinuation mode so clear that out.
    }

    m_FirstTokenOfLine = m_FirstFreeToken; // We'll add tokens to the end of the token ring
    m_AllowXmlFeatures = true; // we always allow xml features when this function is called.  The colorizer is the only one that sets
                               // this to be false and it has a separate entry point to the scanner.  So force true.

    return GetNextLineHelper();
}

Token * 
Scanner::GetAllLines()
{
    Token * pRet = GetNextLine();

    Token * pLastToken = GetLastToken();

    while (!pLastToken->IsEndOfParse())
    {
        GetNextLine();
        pLastToken = GetLastToken();
    }

    return pRet;    
}


Token *
Scanner::GetNextLineHelper()
{
    LexicalState StartState;

    bool timeToStop = false;
    do {
        StartState = m_State.m_LexicalState;

        switch (m_State.m_LexicalState)
        {
        case VB:
        case VBImplicitLineContinuation:
            ScanVBLine();
            break;

        case XmlMarkup:
            ScanXmlMarkup();
            break;

        case XmlElement:
            ScanXmlElement();
            break;

        case XmlAttributeValue:
            ScanXmlAttributeValue();
            break;

        case XmlSingleQuoteString:
            ScanXmlString(L'\'', tkSQuote);
            break;
        case XmlDoubleQuoteString:
            ScanXmlString(L'"', tkQuote);
            break;
        case XmlSingleSmartQuoteString:
            ScanXmlString(0x2018, tkSQuote);
            break;
        case XmlDoubleSmartQuoteString:
            ScanXmlString(0x201C, tkQuote);
            break;

        case XmlContent:
            ScanXmlContent();
            break;

        case XmlComment:
            ScanXmlComment();
            break;

        case XmlCData:
            ScanXmlCData();
            break;

        case XmlPI:
            ScanXmlPI();
            break;

        case XmlPIData:
            ScanXmlPIData();
            break;

        case XmlMisc:
            ScanXmlMisc();
            break;

        default:
            VSASSERT(true, "Invalid xml scanner state");
        }

        /*  The scanner likes to finish scanning in the same mode it started in.  This is 
            helpful during state changes, i.e. if we start scanning in VB mode we probably
            don't want to return when we go to XML scanning mode because we want to capture
            the entire VB logical line.  If we start scanning in XML mode we want to stop
            when we go to VB mode because that indicates we've probably finished consuming
            the logical line.
            ImplicitLineContinuation throws us slightly in that when we go from VBImplicitLineContinuation
            mode to VB mode, it means we are done with the line.  Or vice versa.  So we check
            for those situations explicitly. There are other ways to write this boolean test
            but I think the verbose one makes it more apparent which situations we are looking for */
        if ( StartState == VB && m_State.m_LexicalState == VBImplicitLineContinuation )
        {
            timeToStop = true;
        }
        else if ( StartState == VBImplicitLineContinuation && m_State.m_LexicalState == VB )
        {
            timeToStop = true;
        }
        else if ( StartState == m_State.m_LexicalState )
        {
            timeToStop = true;
        }   

    } while (!timeToStop);

    // The scanner has reached the end of the input. If there are any tokens on the
    // current line, pretend that the line ends in a newline.

    if (!NotAtEndOfInput()) // If we are at the end of the input
    {
        /* Normally state is initialized on entry.  However, in the colorizer case, which
           lands here (EOF) after scanning each snippet of text, it is important to ensure
           that the state we are in upon finishing scanning the line does not propagate to 
           the tokens we can for the next line, since that line can come from anywhere and isn't
           necessarily sequential to the one we are scanning now.
           Therefore, when the state is VB and there isn't a line continuation, ensure that all 
           special VB state flags are cleared. */

        if ((m_State.m_LexicalState == VB ||
             m_State.m_LexicalState == VBImplicitLineContinuation ) && 
            !m_State.m_ExplicitLineContinuation)
        {
            /* Microsoft: Consider - I've introduced the new state VBImplicitLineContinuation, which
               I test for above.  The safest thing to do for now is to continue to clear the same flags
               we were clearing before.  We are at the end of file after all.  But in the colorizer case
               we get called per line.  So it's an interesting question about whether it would help the
               colorizer do a better job if it had more state, such as whether we are in a query, etc.
               I'll wait for a case where it will help us before actually doing that though so we can
               continue operating as closely to how we did before. */

            m_State.SetTokenTypeBeforeLineContinuation(tkNone);
            m_State.m_LineType = NotClassified;
            m_State.m_IsSimpleQuery = false;
            m_State.SetLambdaParameterNesting(0);
            m_State.m_IsRParensOfLambdaParameter = false;
        }


        // If we scanned something (anything) on the line then make sure the line ends in tkEOL
        bool ResetLineStart = false;

        if (m_FirstTokenOfLine != m_FirstFreeToken)
        {
            Token *last = GetLastToken();

            AssertIfNull(last);

            // Dev10 #687030: If last token is tkEOL, set m_NextLineAlreadyScanned for it.
            if ( last != NULL && last->m_TokenType == tkEOL )
            {
                AssertIfFalse( last->m_StartLine != m_LineNumber ); // About to create another tkEOL on the same line?
                
                last->m_EOL.m_NextLineAlreadyScanned = true; // Set this to true since we are tacking on the EOF for the 'next line', below
            }
            else // Dev10 #713909 The else is so we don't make two successive EOL's when there aren't two. But we do need to make sure all lines end in EOL
            {
                MakeToken(tkEOL, 0)->m_EOL.m_NextLineAlreadyScanned = true; // Set this to true since we are tacking on the EOF for the 'next line', below
                //Bug 760510: The following line is added to adjust m_LineStart, a newline is added here and it need to be "eaten".
                if (m_LineStart != m_InputStreamPosition)
                { 
                    // m_LineStart must be reset after insert tkEOF, since m_StartColumn of tkEOF depends on the current m_LineStart.
                    ResetLineStart = true;
                }
            }
        }

        // Note: you can get multiple tkEOF tokens if someone is calling GetNextLine() when
        // we have already scanned to the end of the buffer. 
        MakeToken(tkEOF, 0);

        if (ResetLineStart)
        {       
            m_LineStart = m_InputStreamPosition; 
        }        

    }

     Token *LastTokenOfLine = GetLastToken();
     if (LastTokenOfLine)
     {
        VerifyLineNotTooLong(m_FirstTokenOfLine, LastTokenOfLine);
     }

    VSASSERT(!LastTokenOfLine || LastTokenOfLine->m_TokenType == tkEOL || LastTokenOfLine->m_TokenType == tkEOF, "line not terminated correctly");

    return m_FirstTokenOfLine;
}

void
Scanner::ScanXmlMarkup()
{

    //  tkXmlNCName
    //  ?
    //  !--
    //  ![[CDATA[
    //  %
    //  %=
    //  (
    //  >
    //  tkXmlWhitespace

    Token *T;
    LexicalState NextLexicalState;

    m_State.m_IsEndElement = false;

    // todo: Microsoft - Ensure that state is set before creating the token.

    while (NotAtEndOfInput()) {

       // Bug #568994 fix assert as we can be called after processing xml or an xml expression hole (VBImplicitLinecontiuation)
       VSASSERT(m_State.m_LexicalState == XmlMarkup || m_State.m_LexicalState == VBImplicitLineContinuation, "Wrong scanner state");

       WCHAR c = PeekNextChar();

        switch (c) {
            // Whitespace
            //  S    ::=    (#x20 | #x9 | #xD | #xA)+
            case '\r':
            case '\n':
                if (m_State.OpenXmlElements() == 0)
                {
                    // New line is never valid in the markup state so terminate xml scanning mode
                    m_State.EndXmlState();
                    MakeToken(tkXmlAbort, 0);
                    return;
                }

                // Mark as new line.  In Xml mode new lines do not terminate the line.
                MakeXmlWhiteSpaceToken(1, true);
                EatNewline(c);
                break;

            case ' ':
            case '\t':
                ScanXmlWhiteSpace();
                break;

            case '!':
                if (NotCloseToEndOfInput(1))
                {
                    switch (PeekAheadChar(1))
                    {
                    case '-':
                        if (NotCloseToEndOfInput(2) && PeekAheadChar(2) == '-')
                        {
                            MakeToken(tkXmlBeginComment, 3);
                            AdvanceChars(3);
                            m_State.m_LexicalState = XmlComment;
                            return;
                        }
                        goto ScanQName;

                    case '[':
                        if (NotCloseToEndOfInput(7) &&
                            PeekAheadChar(2) == 'C' &&
                            PeekAheadChar(3) == 'D' &&
                            PeekAheadChar(4) == 'A' &&
                            PeekAheadChar(5) == 'T' &&
                            PeekAheadChar(6) == 'A' &&
                            PeekAheadChar(7) == '[')
                        {
                            MakeToken(tkXmlBeginCData, 8);
                            AdvanceChars(8);
                            m_State.m_LexicalState = XmlCData;
                            return;
                        }
                        goto ScanQName;


                    default:
                        MakeToken(tkBang, 1);
                        AdvanceChars(1);
                        m_State.m_InDTD = true;
                        continue;
                    }
                }
                goto ScanQName;

            case '?':
                MakeToken(tkXmlBeginPI, 1);
                m_InputStreamPosition++;

                ScanXmlQName();

                T = GetLastToken();

                if (T &&
                    T->m_TokenType == tkXmlNCName &&
                    T->m_Id.m_Spelling == STRING_CONST2(m_pStringPool, Xml) &&
                    !m_State.m_IsDocument &&
                    m_State.OpenXmlElements() == 0
                    )
                {
                    m_State.m_LexicalState = XmlElement;
                    m_State.m_IsDocument = true;
                }
                else
                {
                    m_State.m_LexicalState = XmlPI;
                }

                T->m_State = m_State;
               return;

            case '%':
                NextLexicalState = m_State.OpenXmlElements() > 0 ? XmlContent : XmlMisc;
                ScanXmlVBExpr(NextLexicalState);

                // Dev10_502369 part II - bail when we have switched modes to VB (either regular or line continuation mode)
                if (m_State.m_LexicalState == VB || m_State.m_LexicalState == VBImplicitLineContinuation) 
                {
                    return;
                }
                goto ScanQName;

            case '(':
                MakeToken(tkLParen, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlElement;
                return;

            case '>':
                {
                    Token *GT = MakeToken(tkGT, 1);
                    if (m_InputStreamPosition[-1] == '<')
                    {
                        m_State.IncrementOpenXmlElements();
                    }
                    else if (m_State.OpenXmlElements() == 0)
                    {
                        m_InputStreamPosition++;
                        m_State.m_ScannedElement = true;
                        m_State.m_LexicalState = XmlMisc;
                        return;
                    }
                    m_InputStreamPosition++;
                    m_State.m_LexicalState = XmlContent;
                }
                return;

            case '=':
                MakeToken(tkEQ, 1);
                m_InputStreamPosition++;
                break;

            case '\'':
                MakeToken(tkSQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlSingleQuoteString;
                return;

            case '"':
                MakeToken(tkQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlDoubleQuoteString;
                return;

            case '<':
                ScanXmlLT(XmlElement);
                // Dev10 #568994 If we are in implicit continuation mode return just as if we were in VB mode since implicit line continuation
                // effectively means the same thing - we are still scanning VB statements
                if (m_State.m_LexicalState == VB || m_State.m_LexicalState == VBImplicitLineContinuation) 
                {
                    return;
                }
                continue;

            case '/':
                if (m_InputStreamPosition[-1] == '<')
                {
                    if (m_State.OpenXmlElements() > 0)
                    {
                        m_State.DecrementOpenXmlElements();
                    }
                    m_State.m_IsEndElement = true;
                }
                MakeToken(tkDiv, 1);
                m_InputStreamPosition++;
                continue;

            default:
ScanQName:
                // Per the Xml specification, FULLWIDTH characters are not the same as HALFWIDTH.
                // This differs from what is done during VB scanning.

                // Because of weak scanning of QName, this state must always handle
                //    '=' | '\'' | '"'| '/' | '>' | '<' | '(' | '?'

                ScanXmlQName();
                m_State.m_LexicalState = XmlElement;
                return;
        }
    }

    if (m_State.OpenXmlElements() == 0)
    {
        m_State.EndXmlState();
    }
}

void
Scanner::ScanXmlMisc()
{
    // Misc    ::=    Comment | PI | S

    VSASSERT(m_State.m_LexicalState == XmlMisc && m_State.OpenXmlElements() == 0, "Wrong scanner state");
    Token *tEOL = NULL;

    if (!m_State.m_IsDocument)
    {
        m_State.EndXmlState();
        return;
    }

    if (!m_State.m_ScannedElement)
    {
        m_State.m_ScannedElement = GetLastTokenType() == tkXmlEndEmbedded;
    }

    while (NotAtEndOfInput()) {

        WCHAR c = PeekNextChar();

        switch (c) {
            // Whitespace
            //  S    ::=    (#x20 | #x9 | #xD | #xA)+
        case '\r':
        case '\n':
            // Mark as new line
            if (tEOL == NULL && m_FirstTokenOfLine != m_FirstFreeToken) {

                tEOL = MakeToken(tkEOL, 0);
                tEOL->m_EOL.m_NextLineAlreadyScanned = false;
                EatNewline(c);
                CheckForAnotherBlankLine(tEOL);
            }
            else
            {
                EatNewline(c);
            }
            continue;

        case ' ':
        case '\t':
            m_InputStreamPosition++;
            continue;

        case '<':
            if (NotCloseToEndOfInput(1))
            {
                c = PeekAheadChar(1);

                if (!m_State.m_ScannedElement || c == '?' || c == '!')
                {
                    // Remove tEOL from token ring if any exists
                    if (tEOL != NULL)
                    {
                        m_FirstFreeToken = tEOL;
                    }

                    m_State.m_LexicalState = XmlMarkup;
                    MakeToken(tkLT, 1);
                    m_InputStreamPosition++;
                    return;
                }
            }

            m_State.EndXmlState();
            if (tEOL)
            {
                tEOL->m_EOL.m_NextLineAlreadyScanned = true;
            }
            else
            {
                MakeToken(tkLT, 1);
                m_InputStreamPosition++;
            }
            return;

        default:
            if (tEOL != NULL)
            {
                tEOL->m_EOL.m_NextLineAlreadyScanned = true;
            }
            m_State.EndXmlState();
            return;
        }
    }
}

void
Scanner::ScanXmlElement()
{
    // Only legal tokens
    //  QName
    //  /
    //  >
    //  =
    //  Whitespace

    Token *Prev;

    while (NotAtEndOfInput()) {

        VSASSERT(m_State.m_LexicalState == XmlElement, "Wrong scanner state");

        WCHAR c = PeekNextChar();

        switch (c) {
            // Whitespace
            //  S    ::=    (#x20 | #x9 | #xD | #xA)+
            case '\r':
            case '\n':
                // Mark as new line.  In Xml mode new lines do not terminate the line.
                MakeXmlWhiteSpaceToken(1, true);
                EatNewline(c);
                break;

            case ' ':
            case '\t':
                ScanXmlWhiteSpace();
                break;

            case '/':
                MakeToken(tkDiv, 1);
                m_InputStreamPosition++;
                break;

            case '>':
                {
                    Token *GT = MakeToken(tkGT, 1);
                    m_InputStreamPosition++;

                    if (m_State.m_LineType == ImportStatement)
                    {
                        // Xml import declaration, so no content
                        m_State.EndXmlState();
                        return;
                    }
                    else if (m_State.m_InDTD)
                    {
                        m_State.m_InDTD = false;
                    }
                    else if (m_InputStreamPosition[-2] != '/' && !m_State.m_IsEndElement)
                    {
                        m_State.IncrementOpenXmlElements();
                    }
                    else if (m_State.OpenXmlElements() == 0)
                    {
                        m_State.m_ScannedElement = true;
                        m_State.m_LexicalState = XmlMisc;
                        return;
                    }
                    m_State.m_LexicalState = XmlContent;
                }
                return;

            case '=':
                MakeToken(tkEQ, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlAttributeValue;
                return;

            case '\'':
                Prev = GetLastToken();

                // This isn't valid.  Check for a VB comment
                if (Prev)
                {
                    Prev = SkipXmlWhiteSpace(Prev);

                    if (!Prev || (Prev->m_TokenType == tkXmlWhiteSpace && Prev->m_XmlCharData.m_IsNewLine))
                    {
                        // Appears to be NL WS* '

                        m_State.EndXmlState();
                        AbandonTokens(Prev ? Prev : m_FirstTokenOfLine);
                        MakeToken(tkXmlAbort, 0);
                        return;
                    }
                }

                MakeToken(tkSQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlSingleQuoteString;
                return;

            case '"':
                MakeToken(tkQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlDoubleQuoteString;
                return;

            case '<':
                ScanXmlLT(XmlElement);
                // Dev10 #591341 If we are in implicit continuation mode return just as if we were in VB mode since implicit line continuation
                // effectively means the same thing - we are still scanning VB statements
                if (m_State.m_LexicalState == VB || m_State.m_LexicalState == VBImplicitLineContinuation )
                {
                    return;
                }
                m_State.m_LexicalState = XmlMarkup;
                return;

            case '(':
                // Check if this is possibly a code attribute or a function call
                // If it is then abort the xml scanning
                Prev = CheckXmlForCallOrCodeAttribute();
                if (Prev)
                {
                    m_State.EndXmlState();
                    if (Prev->m_TokenType == tkLT)
                    {
                        // Code attribute found
                        AbandonTokens(Prev->m_Next);
                        Prev->m_State = m_State;
                    }
                    else
                    {
                        // Function/Method call found
                        AbandonTokens(Prev);
                        MakeToken(tkXmlAbort, 0);
                    }
                    return;
                }

                MakeToken(tkLParen, 1);
                m_InputStreamPosition++;
                break;

            case ')':
                MakeToken(tkRParen, 1);
                m_InputStreamPosition++;
                break;

            case '?':
                if (NotCloseToEndOfInput(1) && PeekAheadChar(1) == '>')
                {
                    // Create token for the '?>' termination sequence
                    MakeToken(tkXmlEndPI, 2);
                    m_InputStreamPosition += 2;
                    if (m_State.OpenXmlElements() == 0)
                    {
                        m_State.m_LexicalState = XmlMisc;
                        return;
                    }
                    m_State.m_LexicalState = XmlContent;
                    return;
                }
                MakeToken(tkXmlBeginPI, 1);
                m_InputStreamPosition++;
                break;

            case ';':
                MakeToken(tkSColon, 1);
                m_InputStreamPosition++;
                break;

            case '#':
                MakeToken(tkSharp, 1);
                m_InputStreamPosition++;
                break;

            default:
                // Because of weak scanning of QName, this state must always handle
                //    '=' | '\'' | '"'| '/' | '>' | '<' | '?'

                ScanXmlQName();

                Prev = CheckXmlForStatement();
                if (Prev)
                {
                    // Abort Xml - Found Keyword space at the beginning of the line
                    AbandonTokens(Prev);
                    m_State.Init(VB);
                    MakeToken(tkXmlAbort, 0);
                    return;
                }
                break;
        }
    }
}

void
Scanner::ScanXmlAttributeValue()
{
   // [10]    AttValue    ::=    '"' ([^<&"] | Reference)* '"'
   //                         |  "'" ([^<&'] | Reference)* "'"

    Token *WS1 = NULL;

    if (m_InputStreamPosition == m_InputStream)
    {
        // Create a token for colorizer case
        WS1 = MakeToken(tkXmlWhiteSpace, 0);
    }

    while (NotAtEndOfInput()) {

        VSASSERT(m_State.m_LexicalState == XmlAttributeValue, "Wrong scanner state");

        WCHAR c = PeekNextChar();

        switch (c) {
            // Whitespace
            //  S    ::=    (#x20 | #x9 | #xD | #xA)+
            case '\r':
            case '\n':
                // Mark as new line.  In Xml mode new lines do not terminate the line.
                MakeXmlWhiteSpaceToken(1, true);
                EatNewline(c);
                if (!WS1)
                    WS1 = GetLastToken();
                break;

            case ' ':
            case '\t':
                ScanXmlWhiteSpace();
                continue;

            case '\'':
                MakeToken(tkSQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlSingleQuoteString;
                return;

            case '"':
                MakeToken(tkQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlDoubleQuoteString;
                return;

            case 0x2018: // Left single smart quote
            case 0x2019: // Right single smart quote
                MakeToken(tkSQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlSingleSmartQuoteString;
                return;

            case 0x201C: // Left double smart quote
            case 0x201D: // Right double smart quote
                MakeToken(tkQuote, 1);
                m_InputStreamPosition++;
                m_State.m_LexicalState = XmlDoubleSmartQuoteString;
                return;

            case '<':
                ScanXmlLT(XmlElement);
                if (m_State.m_LexicalState == VB || m_State.m_LexicalState == VBImplicitLineContinuation )
                    return;

                m_State.m_LexicalState = XmlMarkup;
                return;

            default:
                // ScanXmlString();
                m_State.m_LexicalState = XmlElement;
                if (WS1)
                    AbandonTokens(WS1);
                return;
        }
    }
}


void
Scanner::ScanXmlCData()
{
    // [18]    CDSect    ::=    CDStart CData CDEnd
    // [19]    CDStart    ::=    '<![CDATA['
    // [20]    CData    ::=    (Char* - (Char* ']]>' Char*))
    // [21]    CDEnd    ::=    ']]>'

    const WCHAR *Here = m_InputStreamPosition;

    while (Here < m_InputStreamEnd)
    {
        VSASSERT(m_State.m_LexicalState == XmlCData, "Wrong scanner state");

        WCHAR c = *Here;

        switch (c)
        {
        case '\r':
        case '\n':
            // If valid characters found then return them.
            MakeXmlCharToken(tkXmlCData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            Here = m_InputStreamPosition;
            break;

        case ']':
            if (Here + 2 < m_InputStreamEnd &&
                Here[1] == ']' &&
                Here[2] == '>')
            {
                MakeXmlCharToken(tkXmlCData, Here - m_InputStreamPosition);
                m_InputStreamPosition = Here;

                // Create token for ']]>' sequence
                MakeToken(tkXmlEndCData, 3);
                m_InputStreamPosition += 3;
                if (m_State.OpenXmlElements() == 0)
                {
                    m_State.m_LexicalState = XmlMisc;
                    return;
                }
                m_State.m_LexicalState = XmlContent;
                return;
            }
            goto ScanChars;

        default:
ScanChars:
            // Check characters are valid
            Here = ScanXmlDefault(Here, tkXmlCData);
            continue;
        }
    }

    // If valid characters found then return them.
    MakeXmlCharToken(tkXmlCData, Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;
}

void
Scanner::ScanXmlContent()
{
    // [14]    CharData    ::=    [^<&]* - ([^<&]* ']]>' [^<&]*)

    const WCHAR *Here = m_InputStreamPosition;
    bool IsAllWhitespace = true;

    while (Here < m_InputStreamEnd)
    {
        VSASSERT(m_State.m_LexicalState == XmlContent, "Wrong scanner state");

        WCHAR c = *Here;

        switch (c)
        {
        case '\r':
        case '\n':
            MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
            m_InputStreamPosition = Here;

            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            Here = m_InputStreamPosition;
            break;

        case ' ':
        case '\t':
            Here++;
            break;

        case '&':
            MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
            m_InputStreamPosition = Here;
            IsAllWhitespace = false;

            ScanXmlReference();
            Here = m_InputStreamPosition;
            continue;

        case '<':
            MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
            m_InputStreamPosition = Here;

            m_State.m_LexicalState = XmlMarkup;
            MakeToken(tkLT, 1);
            m_InputStreamPosition++;
            return;

        case ']':
            if (Here + 2 < m_InputStreamEnd &&
                Here[1] == ']' &&
                Here[2] == '>')
            {
                // If valid characters found then return them.
                MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
                m_InputStreamPosition = Here;
                IsAllWhitespace = false;

                // Create an invalid character data token for the illegal ']]>' sequence
                MakeToken(tkXmlEndCData, 3);
                Here += 3;
                m_InputStreamPosition = Here;
                continue;
            }
            goto ScanChars;

        case '#':
            // Even though # is valid in content, abort xml scanning if the m_State shows and error
            // and the line begins with NL WS* # WS* KW
            if (m_State.m_IsXmlError)
            {
                MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
                m_InputStreamPosition = Here;

                Token *sharp = MakeToken(tkSharp, 1);
                m_InputStreamPosition++;

                while (*m_InputStreamPosition == ' ' || *m_InputStreamPosition == '\t')
                    m_InputStreamPosition++;

                ScanXmlQName();
                
                Token *restart = CheckXmlForStatement();

                if (restart)
                {
                    // Abort Xml - Found Keyword space at the beginning of the line
                    AbandonTokens(restart);
                    m_State.Init(VB);
                    MakeToken(tkXmlAbort, 0);
                    return;
                }

                AbandonTokens(sharp);
                Here = m_InputStreamPosition;
            }
            goto ScanChars;

        case '%':
            // Even though %> is valid in pcdata.  When inside of an embedded expression
            // return this sequence separately so that the xml literal completion code can
            // easily detect the end of an embedded expression that may be temporarily hidden
            // by a new element.  i.e. <%= <a> %>

            if (PendingStates() &&
                Here + 1 < m_InputStreamEnd &&
                Here[1] == '>')
            {
                // If valid characters found then return them.
                MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
                m_InputStreamPosition = Here;
                IsAllWhitespace = false;

                // Create a special pcdata token for the possible tkEndXmlEmbedded
                MakeXmlCharToken(tkXmlCharData, 2, false);
                Here += 2;
                m_InputStreamPosition = Here;
            }
            else
            {
                IsAllWhitespace = false;
                Here++;
            }
            continue;

        default:
ScanChars:
            // Check characters are valid
            if (IsAllWhitespace)
            {
                MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
                m_InputStreamPosition = Here;
                IsAllWhitespace = false;
            }
            Here = ScanXmlDefault(Here, tkXmlCharData);
        }
    }

    MakeXmlCharToken(tkXmlCharData, Here - m_InputStreamPosition, IsAllWhitespace);
    m_InputStreamPosition = Here;
}


void
Scanner::ScanXmlPI()
{
    // Scan the white space between the PI trget and the PI data

    // [16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
    // [17]    PITarget    ::=    Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

    const WCHAR *Here = m_InputStreamPosition;

    while (Here < m_InputStreamEnd)
    {
        VSASSERT(m_State.m_LexicalState == XmlPI, "Wrong scanner state");

        WCHAR c = *Here;

        switch (c)
        {
        case '\r':
        case '\n':
            MakeXmlWhiteSpaceToken(Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            Here = m_InputStreamPosition;
            continue;

        case ' ':
        case '\t':
            Here++;
            continue;

        default:
            break;
        }

        break;
    }

    // If valid characters found then return them.
    MakeXmlWhiteSpaceToken(Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;
    m_State.m_LexicalState = XmlPIData;
}


void
Scanner::ScanXmlPIData()
{
    // Scan the PI data after the white space
    // [16]    PI    ::=    '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
    // [17]    PITarget    ::=    Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))

    const WCHAR *Here = m_InputStreamPosition;

    while (Here < m_InputStreamEnd)
    {
        VSASSERT(m_State.m_LexicalState == XmlPIData, "Wrong scanner state");

        WCHAR c = *Here;

        switch (c)
        {
        case '\r':
        case '\n':
            // If valid characters found then return them.
            MakeXmlCharToken(tkXmlPIData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            Here = m_InputStreamPosition;
            break;

        case '?':
            if (Here + 1 < m_InputStreamEnd &&
                Here[1] == '>')
            {
                // If valid characters found then return them.
                MakeXmlCharToken(tkXmlPIData, Here - m_InputStreamPosition);
                m_InputStreamPosition = Here;

                // Create token for the '?>' termination sequence
                MakeToken(tkXmlEndPI, 2);
                m_InputStreamPosition += 2;
                if (m_State.OpenXmlElements() == 0)
                {
                    m_State.m_LexicalState = XmlMisc;
                    return;
                }
                m_State.m_LexicalState = XmlContent;
                return;
            }
            goto ScanChars;

        default:
ScanChars:
            // Check characters are valid
            Here = ScanXmlDefault(Here, tkXmlPIData);
            continue;
        }
    }

    // If valid characters found then return them.
    MakeXmlCharToken(tkXmlPIData, Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;
}

void
Scanner::ScanXmlComment()
{
    // [15]    Comment    ::=    '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'

    const WCHAR *Here = m_InputStreamPosition;

    while (Here < m_InputStreamEnd)
    {
        VSASSERT(m_State.m_LexicalState == XmlComment, "Wrong scanner state");

        WCHAR c = *Here;

        switch (c)
        {
        case '\r':
        case '\n':
            // If valid characters found then return them.
            MakeXmlCharToken(tkXmlCommentData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            Here = m_InputStreamPosition;
            break;

        case '-':
            if (Here + 1 < m_InputStreamEnd &&
                Here[1] == '-')

            {
                // --> terminates an Xml comment but otherwise -- is an illegal character sequence.
                // The scanner will always returns "--" as a separate comment data string and the
                // the semantics will error if '--' is ever found.

                // Return valid characters up to the --
                MakeXmlCharToken(tkXmlCommentData, Here - m_InputStreamPosition);
                m_InputStreamPosition = Here;

                if (Here + 2 < m_InputStreamEnd)
                {
                    Here += 2;

                    // if > is not found then this is an error.  Return the -- string
                    if (*Here != '>')
                    {
                        MakeXmlCharToken(tkXmlCommentData, 2, false);
                        m_InputStreamPosition = Here;

                        // For better error recovery, allow -> to terminate the comment.
                        // This works because the -> terminates only when the invalid --
                        // is returned.

                        if (Here + 1 < m_InputStreamEnd &&
                            Here[0] == '-' &&
                            Here[1] == '>')
                        {
                            Here++;
                        }
                        else
                        {
                            continue;
                        }
                    }

                    if (*Here == '>')
                    {
                        Here++;
                        MakeToken(tkXmlEndComment, Here - m_InputStreamPosition);
                        m_InputStreamPosition = Here;
                        if (m_State.OpenXmlElements() == 0)
                        {
                            m_State.m_LexicalState = XmlMisc;
                            return;
                        }
                        m_State.m_LexicalState = XmlContent;
                        return;
                    }
                }

                MakeXmlCharToken(tkXmlCommentData, 2, false);
                Here += 2;
                m_InputStreamPosition = Here;
                continue;
            }
            goto ScanChars;

        default:
ScanChars:
            // Check characters are valid
            Here = ScanXmlDefault(Here, tkXmlCommentData);
            continue;
        }
    }
    MakeXmlCharToken(tkXmlCommentData, Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;
}


void
Scanner::ScanXmlString(const WCHAR TerminatingChar, tokens TerminatingToken)
{
    // [10]    AttValue    ::=    '"' ([^<&"] | Reference)* '"'
    //                         |  "'" ([^<&'] | Reference)* "'"

    const WCHAR *Here = m_InputStreamPosition;

    while (Here < m_InputStreamEnd)
    {
        VSASSERT(m_State.m_LexicalState == XmlSingleQuoteString || m_State.m_LexicalState == XmlDoubleQuoteString ||
            m_State.m_LexicalState == XmlSingleSmartQuoteString || m_State.m_LexicalState == XmlDoubleSmartQuoteString ||
            (m_State.m_LexicalState == XmlAttributeValue && TerminatingChar == ' '), "Wrong scanner state");

        const WCHAR c = *Here;
        const WCHAR AltTerminatingChar = (m_State.m_LexicalState == XmlSingleSmartQuoteString || m_State.m_LexicalState == XmlDoubleSmartQuoteString) ? TerminatingChar + 1 : TerminatingChar;

        if (TerminatingChar == c || AltTerminatingChar == c)
        {
            MakeXmlCharToken(tkXmlAttributeData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            if (TerminatingChar != ' ')
            {
                MakeToken(TerminatingToken, 1);
                m_InputStreamPosition++;
            }
            m_State.m_LexicalState = XmlElement;
            return;
        }

        switch(c)
        {
        case '\r':
        case '\n':
            MakeXmlCharToken(tkXmlAttributeData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            Here = m_InputStreamPosition;

            if (TerminatingChar == ' ')
            {
                m_State.m_LexicalState = XmlElement;
                return;
            }

            break;

        case '<':
            // If valid characters found then return them.
            MakeXmlCharToken(tkXmlAttributeData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            m_State.m_LexicalState = XmlMarkup;
            MakeToken(tkLT, 1);
            m_InputStreamPosition++;
            return;

        case '&':
            MakeXmlCharToken(tkXmlAttributeData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;

            ScanXmlReference();
            Here = m_InputStreamPosition;
            continue;

        case '/':
        case '>':
        case '?':
        if (TerminatingChar == ' ')
        {
            MakeXmlCharToken(tkXmlAttributeData, Here - m_InputStreamPosition);
            m_InputStreamPosition = Here;
            m_State.m_LexicalState = XmlElement;
            return;
        }

        __fallthrough;

        default:
            // Check characters are valid
            Here = ScanXmlDefault(Here, tkXmlAttributeData);
            continue;
        }
    }

    MakeXmlCharToken(tkXmlAttributeData, Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;
}

void
Scanner::ScanXmlReference()
{
    MakeToken(tkConcat, 1);
    m_InputStreamPosition++;

    if (NotAtEndOfInput())
    {
        WCHAR c = PeekNextChar();

        switch(c)
        {
        case '\r':
        case '\n':
            // Mark as new line.  In Xml mode new lines do not terminate the line.
            MakeXmlWhiteSpaceToken(1, true);
            EatNewline(c);
            return;

        case '#':
            ScanXmlCharRef();
            break;

        case 'a':
            // &amp;
            // &apos;
            if (NotCloseToEndOfInput(3) &&
                PeekAheadChar(1) == 'm' &&
                PeekAheadChar(2) == 'p' &&
                PeekAheadChar(3) == ';')
            {
                MakeXmlNameToken(tkXmlNCName, STRING_CONST2(m_pStringPool, amp));
                AdvanceChars(3);
                break;
            }
            else if (NotCloseToEndOfInput(4) &&
                PeekAheadChar(1) == 'p' &&
                PeekAheadChar(2) == 'o' &&
                PeekAheadChar(3) == 's' &&
                PeekAheadChar(4) == ';')
            {
                MakeXmlNameToken(tkXmlNCName, STRING_CONST2(m_pStringPool, apos));
                AdvanceChars(4);
                break;
            }
            goto ScanXmlName;

        case 'l':
            // &lt;
            if (NotCloseToEndOfInput(2) &&
                PeekAheadChar(1) == 't' &&
                PeekAheadChar(2) == ';')
            {
                MakeXmlNameToken(tkXmlNCName, STRING_CONST2(m_pStringPool, lt));
                AdvanceChars(2);
                break;
            }
            goto ScanXmlName;

        case 'g':
            // &gt;
            if (NotCloseToEndOfInput(2) &&
                PeekAheadChar(1) == 't' &&
                PeekAheadChar(2) == ';')
            {
                MakeXmlNameToken(tkXmlNCName, STRING_CONST2(m_pStringPool, gt));
                AdvanceChars(2);
                break;
            }
            goto ScanXmlName;

        case 'q':
            // &quot;
            if (NotCloseToEndOfInput(4) &&
                PeekAheadChar(1) == 'u' &&
                PeekAheadChar(2) == 'o' &&
                PeekAheadChar(3) == 't' &&
                PeekAheadChar(4) == ';')
            {
                MakeXmlNameToken(tkXmlNCName, STRING_CONST2(m_pStringPool, quot));
                AdvanceChars(4);
                break;
            }
            goto ScanXmlName;

        default:
ScanXmlName:
            // scan name
            ScanXmlNCName();
            break;
        }

        if (NotAtEndOfInput())
        {
            c = PeekNextChar();
            if (c == ';')
            {
                MakeToken(tkSColon, 1);
                AdvanceChars(1);
            }
        }
    }
}


void
Scanner::ScanXmlCharRef()
{
    const WCHAR *Here = m_InputStreamPosition + 1;
    const WCHAR *Start;
    WCHAR Chars[4];
    size_t Length;

    if (Here < m_InputStreamEnd)
    {
        if (*Here == 'x')
        {
            Here++;

            while (Here < m_InputStreamEnd)
            {
                if (!XmlCharacter::isHexDigit(*Here))
                {
                    Start = m_InputStreamPosition + 2;
                    Length = Here - Start;
                    if (HexToUTF16(Start, Length, Chars, _countof(Chars)) == S_OK)
                    {
                        MakeXmlReferenceToken(Length + 2, Chars, Token::XmlHexCharRef);
                    }
                    else
                    {
                        MakeToken(tkSyntaxError, Length + 2)->m_Error.m_errid = ERRID_IllegalChar;
                    }
                    m_InputStreamPosition = Here;
                    return;
                }

                Here++;
            }
        }
        else
        {
            while (Here < m_InputStreamEnd)
            {
                if (!XmlCharacter::isAsciiDigit(*Here))
                {
                    Start = m_InputStreamPosition + 1;
                    Length = Here - Start;
                    if (DecToUTF16(Start, Length, Chars, _countof(Chars)) == S_OK)
                    {
                        MakeXmlReferenceToken(Length + 1, Chars, Token::XmlDecCharRef);
                    }
                    else
                    {
                        MakeToken(tkSyntaxError, Length + 1)->m_Error.m_errid = ERRID_IllegalChar;
                    }
                    m_InputStreamPosition = Here;
                    return;
                }

                Here++;
            }
        }
    }

    m_InputStreamPosition = Here;
}

void
Scanner::ScanXmlQName()
{
    ScanXmlNCName();

    if (NotAtEndOfInput() && PeekNextChar() == ':')
    {
        MakeToken(tkXmlColon, 1);
        m_InputStreamPosition++;
        ScanXmlNCName();
    }
}

void
Scanner::ScanXmlNCName()
{
    // Scan a non qualified name per Xml Namespace 1.0
    // [4]  NCName ::=  (Letter | '_') (NCNameChar)* /*  An XML Name, minus the ":" */

    // This scanner is much looser than a pure Xml scanner.
    // Names are any character up to a separator character from
    //      ':' | ' ' | '\t' | '\n' | '\r' | '=' | '\'' | '"'| '/' | '<' | '>' | EOF
    // Each name token will be marked as to whether it contains only valid Xml name characters.

    const WCHAR *Here = m_InputStreamPosition;
    bool IsIllegalChar = false;

    while (Here < m_InputStreamEnd)
    {
        WCHAR c = *Here;

        switch (c)
        {
        case ':':
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '=':
        case '\'':
        case '"':
        case '/':
        case '>':
        case '<':
        case '(':
        case ')':
        case '?':
        case ';':
            goto CreateNCNameToken;

        default:
            // Invalid Xml name but scan as Xml name anyway

            // Check characters are valid
            if (XmlCharacter::isValidUtf16(c))
            {
                // Check for surrogates
                // Don't break up surrogate pairs

                if (!IsSurrogate(c))
                {
                    Here++;
                    continue;
                }

                if (IsHighSurrogate(c))
                {
                    if (Here + 1 < m_InputStreamEnd &&
                        IsLowSurrogate(Here[1]))
                    {
                        Here += 2;
                        continue;
                    }
                }

                // Invalid surrogate
            }
            IsIllegalChar = true;
            goto CreateNCNameToken;
        }
    }

CreateNCNameToken:
    if (Here > m_InputStreamPosition)
    {
        MakeXmlNameToken(tkXmlNCName, Here - m_InputStreamPosition);
        m_InputStreamPosition = Here;
    }

    if (IsIllegalChar)
    {
       Token *Result = MakeToken(tkSyntaxError, 1);

       Result->m_Error.m_errid = ERRID_IllegalChar;

       m_InputStreamPosition++;
    }
}


const WCHAR *
Scanner::ScanXmlDefault(const WCHAR *Here, tokens TokenType)
{
    const WCHAR c = *Here;

    // Check characters are valid
    if (XmlCharacter::isValidUtf16(c))
    {
        // Check for surrogates
        // Don't break up surrogate pairs

        if (!IsSurrogate(c))
        {
            return Here + 1;
        }

        if (IsHighSurrogate(c))
        {
            if (Here + 1 < m_InputStreamEnd &&
                IsLowSurrogate(Here[1]))
            {
                return Here + 2;
            }
        }

        // Invalid surrogate
    }

    // If valid characters found then return them.
    MakeXmlCharToken(TokenType, Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;

    // Create an invalid character data token for the illegal characters
    m_InputStreamPosition = Here;

   Token *Result = MakeToken(tkSyntaxError, 1);

   Result->m_Error.m_errid = ERRID_IllegalChar;

   return ++m_InputStreamPosition;
}


void
Scanner::ScanXmlWhiteSpace()
{
    const WCHAR *Here = m_InputStreamPosition + 1;

    while (Here < m_InputStreamEnd && (' ' == *Here || '\t' == *Here))
    {
        Here++;
    }

    MakeXmlWhiteSpaceToken(Here - m_InputStreamPosition);
    m_InputStreamPosition = Here;
}


void
Scanner::ScanXmlLT(LexicalState NewState)
{
    MakeToken(tkLT, 1);
    AdvanceChars(1);
    ScanXmlVBExpr(NewState);
}

void
Scanner::ScanXmlVBExpr(LexicalState NewState)
{
    if (NotCloseToEndOfInput(1) && PeekNextChar() == '%' && PeekAheadChar(1) == '=')
    {
        // We are in XML mode so keep track of that.  We'll be moving over to VB/VBImpliedLineContinuation mode
        // below but that's just for the purposes of handling the VB expression in the XMl hole.  We'll need to
        // return to XML mode when we are done with the VB expression.
        m_State.m_LexicalState = NewState;
        PushState(); 

        MakeToken(tkXmlBeginEmbedded, 2);        
        m_InputStreamPosition += 2;

        m_State.m_LexicalState = VB; // the expression will be scanned in VB mode.  

        //we just scanned the %=
        //now look and see if there is an EOL after the %=, and if so, create an EOL token.
        if (NotAtEndOfInput())
        {
            if (IsWhitespaceCharacter(*m_InputStreamPosition))
            {
                EatWhitespace();
            }

            if (NotAtEndOfInput())
            {
                wchar_t c = PeekNextChar(); // returns what is at m_InputStreamPosition (i.e. doesn't actually look ahead)

                if (c == '\r' || c=='\n')
                {
                    // Dev10_502369 Allow implied LC after <%=
                    // %= implies line continuation.  Since the last token we built before the EOL was tkXmlBeginEmbedded,
                    // pass that as our previous token and set the scanner state to VBImplicitLineContinuation
                    m_State.BeginVBContinuationState( tkXmlBeginEmbedded ); 
                    Token * pToken = MakeToken(tkEOL, 0);
                    pToken->m_EOL.m_NextLineAlreadyScanned = true;
                    EatNewline(c);
                    CheckForAnotherBlankLine( pToken );
                }
            }
        } // deal with trailing whitespace and EOL
    } // if we are on '%=' 
    else
    {
        // Push the current XML state we are in on the stack.  This won't reflect the NewState
        // passed in since we didn't in fact find the %= characters
        PushState(); 
    }
}

#if 0
bool
Scanner::LookAheadForKeyword()
{
    bool FoundKeyword = false;

    Scanner::LineMarker LineStart = MarkLine();

    EatWhitespace();
    if (IsFirstIdentifierCharacter(PeekNextChar())) {

        ScanIdentifier();

        Token *Current = GetLastToken();

        FoundKeyword = Token::IsKeyword(TokenOfString(Current->m_Id.m_Spelling));

        m_FirstFreeToken = Current;
    }

    ResetToLine(LineStart);

    return FoundKeyword;
}
#endif

//
// Line marking functions
//

void
Scanner::AbandonTokens
(
)
{
    m_FirstInUseToken = NULL;
}

void
Scanner::AbandonTokensBefore
(
    Token *T
)
{
    m_FirstInUseToken = T;
}


void Scanner::AbandonTokens
(
    _In_ Token *T
)
{
    m_FirstFreeToken = T;
    m_InputStreamPosition = TokenToStreamPosition(T);
    m_LineNumber = T->m_StartLine;
    m_LineStart = m_InputStreamPosition - T->m_StartColumn;

    // Since we reset m_InputStreamPosition, we need to clear out the indication
    // that we've scanned the next line since we are now starting in a new place
    // and consequently haven't necessairly scanned the line following, yet.
    Token *FindEOL = T;
    while ( FindEOL->m_TokenType != tkEOL && FindEOL->m_TokenType != tkNone)
    {
        FindEOL = FindEOL->m_Next;
        ThrowIfNull(FindEOL); // We should always have a EOL before hitting EOF
    }
    if ( FindEOL->m_TokenType == tkEOL )
    {
        FindEOL->m_EOL.m_NextLineAlreadyScanned = false;
    }
}

Token *
Scanner::GetLastToken
(
)
const
{
    if (m_FirstInUseToken != NULL)
        return m_FirstFreeToken->m_Prev;

    return NULL;
}

tokens
Scanner::GetLastTokenType
(
)
{
    if (m_FirstInUseToken != NULL)
        return m_FirstFreeToken->m_Prev->m_TokenType;

    return m_State.TokenTypeBeforeLineContinuation();
}

Scanner::LineMarker
Scanner::MarkLine
(
)
{
    LineMarker Result;

    Result.m_LineNumber = m_LineNumber;
    Result.m_LineStart = m_LineStart;
    Result.m_CharacterPositionOfCurrentClusterStart = m_CharacterPositionOfCurrentClusterStart;

    return Result;
}

void
Scanner::ResetToLine (LineMarker Marker)
{
    m_LineNumber = Marker.m_LineNumber;
    m_LineStart = Marker.m_LineStart;
    m_InputStreamPosition = Marker.m_LineStart;
    m_CharacterPositionOfCurrentClusterStart = Marker.m_CharacterPositionOfCurrentClusterStart;
}

bool
Token::Is7to8NewKeyword()
{
    return KeywordIsNew7to8(this->m_TokenType);
}

Location
Token::GetLocationExcludeTypeIdChar() const
{
    Location loc = GetLocation();

    if (m_TokenType == tkID && m_Id.m_TypeCharacter != chType_NONE)
    {
        loc.m_lEndColumn = max(
            0, 
            loc.m_lEndColumn - (long)wcslen(WszTypeChar(m_Id.m_TypeCharacter)));
    }

    return loc;
}

void
ScannerState::SetTokenTypeBeforeLineContinuation(_In_ Token *t)
{
    tokens TokenType = t->m_TokenType;
    VSASSERT(TokenType >= 0 && TokenType < tkCount, "Invalid token being set, it doesn't fit in the size allocated in m_Data which implies it is bigger than tkCount");
    m_Token = TokenType;
}


ColorizerState::~ColorizerState()
{
    if (!m_ScannerState.m_IsSimpleState)
    {
        delete m_Stack;
    }
}

void ColorizerState::Init(long State)
{
    m_Long = State;
    if (State == 0)
    {
        m_ScannerState.m_IsSimpleState = true;
        m_ScannerState.m_LexicalState = VB;
        m_ScannerState.SetTokenTypeBeforeLineContinuation(tkNone);
        m_ScannerState.SetLambdaParameterNesting(0);
    }
    else if (!m_ScannerState.m_IsSimpleState)
    {
        m_Stack = m_Stack->Clone();
    }

}

ColorizerState&
ColorizerState::Assign(const ColorizerState& State)
{
    if (this != &State)
    {
        if (!m_ScannerState.m_IsSimpleState)
            delete m_Stack;

        Init(State);
    }

    return *this;
}
