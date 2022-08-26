// uniprop.h
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// June 1, 1998 [paulde]
//
//-----------------------------------------------------------------
#ifdef _MSC_VER
#pragma once
#endif

#ifndef __UNIPROP_H__
#define __UNIPROP_H__

BYTE    WINAPI  UProp    (WCHAR ch);
void    WINAPI  GetProps ( _In_count_(cch) PCWSTR pchBuffer, int cch, _Out_ _Post_count_(cch) BYTE * pProps);

int     WINAPI  LastNonWhiteChar  ( _In_count_(cch) PCWSTR pch, int cch);
int     WINAPI  FirstNonWhiteChar ( _In_count_(cch) PCWSTR pch, int cch);
BOOL    WINAPI  IsWhitespace   (WCHAR ch);
BOOL    WINAPI  IsTab          (WCHAR ch);
BOOL    WINAPI  IsBlank        (WCHAR ch);
BOOL    WINAPI  IsBlanks       ( _In_count_(cch) PCWSTR pch, int cch);

WORD    WINAPI  UScript        (WCHAR ch);

BOOL    WINAPI  IsBidi         (WCHAR ch); // Arabic and Hebrew
BOOL    WINAPI  IsHangul       (WCHAR ch); // Korean
BOOL    WINAPI  IsKatakana     (WCHAR ch); // Japanese phonetic
BOOL    WINAPI  IsHiragana     (WCHAR ch); // Japanese phonetic
BOOL    WINAPI  IsIdeograph    (WCHAR ch); // Kanji, aka Han, aka Hanja
BOOL    WINAPI  IsExtender     (WCHAR ch); // 'Extender' from Unicode identifier rules

BYTE    WINAPI  CombiningClass (WCHAR ch);

BOOL    WINAPI  IsWholeWord ( __in_ecount(cchText+1) PCWSTR pchText, int cchText,  __in_ecount(cchItem+1) PCWSTR pchItem, int cchItem, BOOL fHanWordBreak = TRUE);
BOOL    WINAPI  IsWordBreak ( __in_z PCWSTR pchText, __in_ecount(1) PCWSTR pchPoint, BOOL fHanWordBreak = FALSE);

BOOL    WINAPI  IsLineBreak      (WCHAR ch);

//----------------------------------------------------------------------------------------------
#define WL_WORDONLY               0x00000001  // simple word break only (default is kinsoku rule)
#define WL_SPILLWHITESPACE        0x00000002  // spill trailing whitespace past pPoint
#define WL_SPILLGRAPHICROW        0x00000004  // spill graphical row (like comment header above)
PCWSTR  WINAPI  WrapLine ( __in_ecount(cch) PCWSTR pText, int cch, __in_ecount(1) PCWSTR pPoint, DWORD dwFlags);
// non-const version
inline PWSTR WINAPI WrapLine ( __in_ecount(cch) PWSTR pText, int cch, __in_ecount(1) PWSTR pPoint, DWORD dwFlags)
{
    return const_cast<PWSTR>(WrapLine (const_cast<PCWSTR>(pText), cch, const_cast<PCWSTR>(pPoint), dwFlags));
}

BOOL    WINAPI  IsAlpha                     (WCHAR ch);
BOOL    WINAPI  IsDigit                     (WCHAR ch);
BOOL    WINAPI  IsAlphaNumeric              (WCHAR ch);
BOOL    WINAPI  IsUppercaseLetter           (WCHAR ch);
BOOL    WINAPI  IsLowercaseLetter           (WCHAR ch);
BOOL    WINAPI  IsTitlecaseLetter           (WCHAR ch);
BOOL    WINAPI  IsModifierLetter            (WCHAR ch);
BOOL    WINAPI  IsOtherLetter               (WCHAR ch);
BOOL    WINAPI  IsDecimalDigit              (WCHAR ch);
BOOL    WINAPI  IsLetterDigit               (WCHAR ch);
BOOL    WINAPI  IsOtherDigit                (WCHAR ch);
BOOL    WINAPI  IsPunctuation               (WCHAR ch);
BOOL    WINAPI  IsPairPunctuation           (WCHAR ch);
BOOL    WINAPI  IsOpenPunctuation           (WCHAR ch);
BOOL    WINAPI  IsClosePunctuation          (WCHAR ch);
BOOL    WINAPI  IsInitialQuotePunctuation   (WCHAR ch);
BOOL    WINAPI  IsFinalQuotePunctuation     (WCHAR ch);
BOOL    WINAPI  IsDashPunctuation           (WCHAR ch);
BOOL    WINAPI  IsConnectorPunctuation      (WCHAR ch);
BOOL    WINAPI  IsOtherPunctuation          (WCHAR ch);
BOOL    WINAPI  IsSpaceSeparator            (WCHAR ch);
BOOL    WINAPI  IsLineSeparator             (WCHAR ch); // 0x2028
BOOL    WINAPI  IsParagraphSeparator        (WCHAR ch); // 0x2029
BOOL    WINAPI  IsCombining                 (WCHAR ch);
BOOL    WINAPI  IsNonSpacingMark            (WCHAR ch);
BOOL    WINAPI  IsCombiningMark             (WCHAR ch);
BOOL    WINAPI  IsEnclosingMark             (WCHAR ch);
BOOL    WINAPI  IsMathSymbol                (WCHAR ch);
BOOL    WINAPI  IsSymbol                    (WCHAR ch);
BOOL    WINAPI  IsCurrencySymbol            (WCHAR ch);
BOOL    WINAPI  IsModifierSymbol            (WCHAR ch);
BOOL    WINAPI  IsOtherSymbol               (WCHAR ch);
BOOL    WINAPI  IsOtherControl              (WCHAR ch);
BOOL    WINAPI  IsOtherFormat               (WCHAR ch);
BOOL    WINAPI  IsOtherSurrogate            (WCHAR ch);
BOOL    WINAPI  IsOtherPrivateUse           (WCHAR ch);
BOOL    WINAPI  IsOtherNotAssigned          (WCHAR ch); // currently no entries in Uni w/ this prop
BOOL    WINAPI  IsNotAssigned               (WCHAR ch);

BOOL    WINAPI  IsPropAlpha                     (BYTE prop);
BOOL    WINAPI  IsPropDigit                     (BYTE prop);
BOOL    WINAPI  IsPropAlphaNumeric              (BYTE prop);
BOOL    WINAPI  IsPropUppercaseLetter           (BYTE prop);
BOOL    WINAPI  IsPropLowercaseLetter           (BYTE prop);
BOOL    WINAPI  IsPropTitlecaseLetter           (BYTE prop);
BOOL    WINAPI  IsPropModifierLetter            (BYTE prop);
BOOL    WINAPI  IsPropOtherLetter               (BYTE prop);
BOOL    WINAPI  IsPropDecimalDigit              (BYTE prop);
BOOL    WINAPI  IsPropLetterDigit               (BYTE prop);
BOOL    WINAPI  IsPropOtherDigit                (BYTE prop);
BOOL    WINAPI  IsPropPunctuation               (BYTE prop);
BOOL    WINAPI  IsPropPairPunctuation           (BYTE prop);
BOOL    WINAPI  IsPropOpenPunctuation           (BYTE prop);
BOOL    WINAPI  IsPropClosePunctuation          (BYTE prop);
BOOL    WINAPI  IsPropInitialQuotePunctuation   (BYTE prop);
BOOL    WINAPI  IsPropFinalQuotePunctuation     (BYTE prop);
BOOL    WINAPI  IsPropDashPunctuation           (BYTE prop);
BOOL    WINAPI  IsPropConnectorPunctuation      (BYTE prop);
BOOL    WINAPI  IsPropOtherPunctuation          (BYTE prop);
BOOL    WINAPI  IsPropSpaceSeparator            (BYTE prop);
BOOL    WINAPI  IsPropLineSeparator             (BYTE prop); // 0x2028
BOOL    WINAPI  IsPropParagraphSeparator        (BYTE prop); // 0x2029
BOOL    WINAPI  IsPropCombining                 (BYTE prop);
BOOL    WINAPI  IsPropNonSpacingMark            (BYTE prop);
BOOL    WINAPI  IsPropCombiningMark             (BYTE prop);
BOOL    WINAPI  IsPropEnclosingMark             (BYTE prop);
BOOL    WINAPI  IsPropSymbol                    (BYTE prop);
BOOL    WINAPI  IsPropMathSymbol                (BYTE prop);
BOOL    WINAPI  IsPropCurrencySymbol            (BYTE prop);
BOOL    WINAPI  IsPropModifierSymbol            (BYTE prop);
BOOL    WINAPI  IsPropOtherSymbol               (BYTE prop);
BOOL    WINAPI  IsPropOtherControl              (BYTE prop);
BOOL    WINAPI  IsPropOtherFormat               (BYTE prop);
BOOL    WINAPI  IsPropOtherSurrogate            (BYTE prop);
BOOL    WINAPI  IsPropOtherPrivateUse           (BYTE prop);
BOOL    WINAPI  IsPropOtherNotAssigned          (BYTE prop); // currently no entries in Uni w/ this prop
BOOL    WINAPI  IsPropNotAssigned               (BYTE prop);

//      Name                           Code    Property Symbol in Unicode Character Database
#define LetterUppercase                0x01 // Lu
#define LetterLowercase                0x02 // Ll
#define LetterTitlecase                0x03 // Lt
#define LetterModifier                 0x04 // Lm
#define LetterOther                    0x05 // Lo
#define DigitDecimal                   0x06 // Nd
#define DigitLetter                    0x07 // Nl
#define DigitOther                     0x08 // No
#define PunctuationOpen                0x09 // Ps
#define PunctuationClose               0x0a // Pe
#define PunctuationInitialQuote        0x0b // Pi
#define PunctuationFinalQuote          0x0c // Pf
#define PunctuationDash                0x0d // Pd
#define PunctuationConnector           0x0e // Pc
#define PunctuationOther               0x0f // Po
#define SeparatorSpace                 0x10 // Zs
#define SeparatorLine                  0x11 // Zl
#define SeparatorParagraph             0x12 // Zp
#define MarkNonSpacing                 0x13 // Mn
#define MarkCombining                  0x14 // Mc
#define MarkEnclosing                  0x15 // Me
#define SymbolMath                     0x16 // Sm
#define SymbolCurrency                 0x17 // Sc
#define SymbolModifier                 0x18 // Sk
#define SymbolOther                    0x19 // So
#define OtherControl                   0x1a // Cc  Control characters
#define OtherFormat                    0x1b // Cf  CSS (BiDi) Controls
#define OtherSurrogate                 0x1c // Cs
#define OtherPrivateUse                0x1d // Co
#define OtherNotAssigned               0x1e // Cn // currently no entries in Uni w/ this prop

#define NotAssigned                    0xff // (No entry in Uni database)

// enum - not flags
#define SCRIPT_NONE          0x0000 // punctuation, dingbat, symbol, math, ...
#define SCRIPT_LATIN         0x0001
#define SCRIPT_GREEK         0x0002
#define SCRIPT_CYRILLIC      0x0003
#define SCRIPT_ARMENIAN      0x0004
#define SCRIPT_HEBREW        0x0005
#define SCRIPT_ARABIC        0x0006
#define SCRIPT_DEVANAGARI    0x0007
#define SCRIPT_BENGALI       0x0008
#define SCRIPT_GURMUKHI      0x0009
#define SCRIPT_GUJARATI      0x000a
#define SCRIPT_ORIYA         0x000b
#define SCRIPT_TAMIL         0x000c
#define SCRIPT_TELUGU        0x000d
#define SCRIPT_KANNADA       0x000e
#define SCRIPT_MALAYALAM     0x000f
#define SCRIPT_THAI          0x0010
#define SCRIPT_LAO           0x0011
#define SCRIPT_TIBETAN       0x0012
#define SCRIPT_GEORGIAN      0x0013
#define SCRIPT_CJK           0x0014 // Chinese, Japanese, Korean (Han, Katakana, Hiragana, Hangul)

// Unicode 3.0 added scripts
#define SCRIPT_BRAILLE              0x0015
#define SCRIPT_SYRIAC               0x0016
#define SCRIPT_THAANA               0x0017
#define SCRIPT_SINHALA              0x0018
#define SCRIPT_MYANMAR              0x0019
#define SCRIPT_ETHIOPIC             0x001a
#define SCRIPT_CHEROKEE             0x001b
#define SCRIPT_CANADIAN_ABORIGINAL  0x001c
#define SCRIPT_OGHAM                0x001d
#define SCRIPT_RUNIC                0x001e
#define SCRIPT_KHMER                0x001f
#define SCRIPT_MONGOLIAN            0x0020
#define SCRIPT_YI                   0x0021


//-----------------------------------------------------------------
//
// PCWSTR WINAPI WrapLine (PCWSTR pText, int cch, PCWSTR pPoint, DWORD dwFlags);
//
// Word-wrap a line, applying Kinsoku rule.
//
// Input: 
//  pText     Line to break.
//  cch       Count of characters in the line (Does NOT take -1 for NULL-terminated mode).
//  pPoint    Candidate point in pText where we calculate the break from.
//  dwFlags   Line-break options.
//      WL_WORDONLY           simple word break only (default is kinsoku rule)
//      WL_SPILLWHITESPACE    contiguous trailing whitespace extends past pPoint
//      WL_SPILLGRAPHICROW    allow graphical row (e.g. row of dashes) to extend past Point
//
// Returns: The best place to start the next line.
//          The returned break point can be one past the candidate point.
//          pPoint is returned if no suitable break point is found.
//

//================================================================
//====  Implementation  ==========================================
//================================================================

//-----------------------------------------------------------------
// We include decimal digits and LOW LINE ('_') in the
// definition of a 'word'.
//
inline BOOL IsWordChar(WCHAR ch, BYTE prop)
{
    return IsPropAlphaNumeric(prop) || ch == L'_';
}

#if 1 // new prop table for Unicode 3.0
// defined in uGenTab.cpp
extern BYTE WINAPI LookupGen (WCHAR ch);
inline BYTE WINAPI UProp(WCHAR ch) { return LookupGen(ch); }
#else // old prop table
extern const BYTE * rgpbUGenProps[];

inline BYTE WINAPI UProp(WCHAR ch)
{
	const BYTE *p = rgpbUGenProps[ch>>8];
	if ((INT_PTR)p > 0xff)
		return p[ch & 0xff];
	return (BYTE)p;
}
#endif

#undef __IN_RANGE__
#define __IN_RANGE__(v, r1, r2) ((unsigned)((v) - (r1)) <= (unsigned)((r2)-(r1)))

////////////////////////////////////////////////////////////////////////////
//
// Implement BOOL Is...(WCHAR ch) functions
//
#define IS_FT_CHAR 1
#include "uniprop.inc"
#undef IS_FT_CHAR
//
// Implement BOOL IsProp...(BYTE prop) functions
//
#define IS_FT_PROP 1
#include "uniprop.inc"
#undef IS_FT_PROP

//--------------------------------------------------------------------------

inline BOOL WINAPI IsHangul(WCHAR ch)
{
    char f = 0;
    if      (ch <  0x1100) {}
    else if (ch <= 0x11FF) f = 1;  // (0x1100 - 0x11FF) Hangul Combining Jamo
    else if (ch <  0x3130) {}
    else if (ch <= 0x318f) f = 1;  // (0x3130 - 0x318f) Hangul Compatability Jamo
    else if (ch <  0xac00) {}
    else if (ch <= 0xd7a3) f = 1;  // (0xac00 - 0xd7a3) Hangul Syllables
    else if (ch <  0xffa0) {}
    else if (ch <= 0xffdf) f = 1;  // (0xffa0 - 0xffdf) Halfwidth Hangul Compatability Jamo
    return (BOOL)f;
}

inline BOOL WINAPI IsKatakana(WCHAR ch)
{
    return (ch >= 0x3031) &&
        (
        __IN_RANGE__(ch, 0x3099, 0x30fe) || 
        __IN_RANGE__(ch, 0xff66, 0xff9f) 
		);
}

inline BOOL WINAPI IsHiragana(WCHAR ch)
{
    return (ch >= 0x3031) &&
        (
        __IN_RANGE__(ch, 0x3041, 0x309e) ||
        (0x30fc == ch) || (0xff70 == ch) // fullwidth and halfwidth hira/kata prolonged sound mark
		);
}

inline BOOL WINAPI IsIdeograph(WCHAR ch)
{
  char f = 0;
  if      (ch <  0x4e00) { if (ch == 0x3005) f = 1; }
  else if (ch <= 0x9fa5) f = 1;
  else if (ch <  0xf900) {}
  else if (ch <= 0xfa2d) f = 1;
  return (BOOL)f;
}

// TRUE if ch is a bidirectional character (e.g. Hebrew or Arabic)
// 
// NOTE [micleh]: There is a very similar function called IsBidiChar() in 
// env\msenv\textmgr\paint.h which is implemented slightly differently.  If you
// suspect a bug in this function, take a look at IsBidiChar() to see if it
// handles the situation differently.  Ideally, they should both be the same, 
// but I'm not sure which is (more) correct.
//
// Determining the directionality of a character does not seem to be very
// straightforward, and thus, this funciton is at best an approximation.  See
// the following documents for details:
//
// UAX #9: The Bidirectional Algorithm: http://www.unicode.org/reports/tr9/
// Unicode Character Database: http://www.unicode.org/ucd/
inline BOOL WINAPI IsBidi(WCHAR ch)
{
  char f = 0;
  if      (ch <  0x0590) {}
  else if (ch <= 0x06FF) f = 1;
  else if (ch <  0xFB00) {}
  else if (ch <= 0xFDFF) f = 1;
  else if (ch <  0xFE70) {}
  else if (ch <= 0xFEFC) f = 1;
  return (BOOL)f;
}

inline BOOL WINAPI IsNonBreaking(WCHAR ch)
{
    BOOL fIsNotBreakingCharacter = (
        (ch == 0x00a0) || // NO-BREAK SPACE
        (ch == 0x2011) || // NON-BREAKING HYPHEN
        (ch == 0x202F) || // NARROW NO-BREAK SPACE
        (ch == 0x30FC) || // Japanese NO-BREAK character a bit like a long hyphen
        (ch == 0xfeff)    // ZERO WIDTH NO-BREAK SPACE (byte order mark)
        );

    return fIsNotBreakingCharacter;
}

inline BOOL WINAPI IsBlank(WCHAR ch)
{
    return (
        (0x0020 == ch) || (0x0009 == ch) ||
        (0x00a0 == ch) || (0x3000 == ch) ||
        __IN_RANGE__(ch, 0x2000, 0x200b)
        );
}

//
// 0x0009  [HORIZONTAL TABULATION]
// 0x000a  [LINE FEED]
// 0x000b  [VERTICAL TABULATION]
// 0x000c  [FORM FEED]
// 0x000d  [CARRIAGE RETURN]
// 0x0020  SPACE
// 0x00a0  NO-BREAK SPACE
// 0x1361  ETHIOPIC WORDSPACE   $



















inline BOOL WINAPI IsWhitespace (WCHAR ch)
{
  char f = 0;
  if (0x0020 == ch) f = 1;
  else if (ch < 0x00a0)
  {
    if ((ch > 0x000D) || (ch < 0x0009))
    {
    }
    else
      f = 1;
  }
  else if (ch <= 0x303F)
  {
    if (ch <= 0x200b)
    {
      if      (0x00a0 == ch) f = 1;
//      else if (0x1361 == ch) f = 1;     // ETHIOPIC WORDSPACE
//      else if (0x1680 == ch) f = 1;     // 0x1680  OGHAM SPACE MARK
      else if (ch >= 0x2000) f = 1;
    }
    else
    {
      if      (0x2028 == ch) f = 1;
      else if (0x2029 == ch) f = 1;
      else if (0x202F == ch) f = 1;
      else if (0x3000 == ch) f = 1;
      else if (0x303F == ch) f = 1;
    }
  }
  return (BOOL)f;
}
inline BOOL WINAPI IsTab (WCHAR ch)
{
  switch (ch)
  {
  case 0x0009: 
  case 0x000b: 
      return true;
  }
  return false;
}

#undef __IN_RANGE__

#endif // __UNIPROP_H__
