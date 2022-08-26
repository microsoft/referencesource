// unichar.h - Unicode character literals
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// June 1, 1998 [paulde]
//
#ifdef _MSC_VER
#pragma once
#endif

#ifndef __UNICHAR_H__
#define __UNICHAR_H__

#include "unimisc.h"  // IN_RANGE

//      ---------                   ------------  ----------------------------------
//         ID                        Code point        Unicode character name
//      ---------                   ------------  ----------------------------------
#define  UCH_NULL                      0x0000     // NULL
#define  UCH_TAB                       0x0009     // HORIZONTAL TABULATION
#define  UCH_LF                        0x000A     // LINE FEED
#define  UCH_CR                        0x000D     // CARRAIGE RETURN
#define  UCH_SPACE                     0x0020     // SPACE
#define  UCH_SP                        0x0020     // SPACE
#define  UCH_NBSP                      0x00A0     // NO-BREAK SPACE
#define  UCH_IDEOSP                    0x3000     // IDEOGRAPHIC SPACE
#define  UCH_LS                        0x2028     // LINE SEPARATOR
#define  UCH_PS                        0x2029     // PARAGRAPH SEPARATOR
#define  UCH_NEL                       0x0085     // NEXT LINE
#define  UCH_ZWNBSP                    0xFEFF     // ZERO WIDTH NO-BREAK SPACE (byte order mark)
#define  UCH_BOM                       0xFEFF     // ZERO WIDTH NO-BREAK SPACE (byte order mark)
#define  UCH_BOMSWAP                   0xFFFE     // byte-swapped byte order mark
#define  UCH_NONCHAR                   0xFFFF     // Not a Character
#define  UCH_OBJECT                    0xFFFC     // OBJECT REPLACEMENT CHARACTER
#define  UCH_REPLACE                   0xFFFD     // REPLACEMENT CHARACTER

#define  UCH_ENQUAD                    0x2000     // EN QUAD
#define  UCH_EMQUAD                    0x2001     // EM QUAD
#define  UCH_ENSP                      0x2002     // EN SPACE
#define  UCH_EMSP                      0x2003     // EM SPACE
#define  UCH_3EMSP                     0x2004     // THREE-PER-EM SPACE
#define  UCH_4EMSP                     0x2005     // FOUR-PER-EM SPACE
#define  UCH_6EMSP                     0x2006     // SIX-PER-EM SPACE
#define  UCH_FIGSP                     0x2007     // FIGURE SPACE
#define  UCH_PUNSP                     0x2008     // PUNCTUATION SPACE
#define  UCH_THINSP                    0x2009     // THIN SPACE
#define  UCH_HAIRSP                    0x200A     // HAIR SPACE
#define  UCH_ZWSP                      0x200B     // ZERO WIDTH SPACE
#define  UCH_ZWNJ                      0x200C     // ZERO WIDTH NON-JOINER
#define  UCH_ZWJ                       0x200D     // ZERO WIDTH JOINER
#define  UCH_LTR                       0x200E     // LEFT-TO-RIGHT MARK
#define  UCH_RTL                       0x200F     // RIGHT-TO-LEFT MARK
#define  UCH_HYPHEN                    0x2010     // HYPHEN
#define  UCH_NBHYPHEN                  0x2011     // NON-BREAKING HYPHEN
#define  UCH_FIGDASH                   0x2012     // FIGURE DASH
#define  UCH_ENDASH                    0x2013     // EN DASH
#define  UCH_EMDASH                    0x2014     // EM DASH
#define  UCH_IDEOSP                    0x3000     // IDEOGRAPHIC SPACE

#define  UCH_EURO                      0x20AC     // EURO SIGN

//---------------------------------------------------------------
// Convert a FULLWIDTH LATIN character to it's Latin (ASCII) counterpart
// Returns the same character if it's not FULLWIDTH LATIN.
//
inline WCHAR FullWidthLatinToLatin (WCHAR ch)
{
    return (ch < 0xFF01 || ch > 0xFF5E) ? ch : WCHAR(L'!' + (ch - 0xFF01));
}

//---------------------------------------------------------------
// Line terminators
//
#define USZ_EOLCHARSET   L"\x000D\x000A\x2028\x2029\x0085"   // The set of Unicode line-break chars
#define USZ_CRLF         L"\x000D\x000A"
#define USZ_LF           L"\x000A"
#define USZ_CR           L"\x000D"
#define USZ_LS           L"\x2028"
#define USZ_PS           L"\x2029"
#define USZ_NEL          L"\x0085"

//---------------------------------------------------------------
// IsLineBreak - Is ch a Unicode line break character
// 
inline BOOL WINAPI IsLineBreak (WCHAR ch)
{
    return (ch <= UCH_PS) && (UCH_CR == ch || UCH_LF == ch || UCH_LS == ch || UCH_PS == ch || UCH_NEL == ch);
}

#include "unipriv.h"

//---------------------------------------------------------------
// LineBreakAdvance - Advance a pointer at EOL past the line ending
// 
inline PCWSTR WINAPI LineBreakAdvance (__in_ecount(2) PCWSTR pch)
{
    if(!pch)
    {
        UASSERT(false);
        return NULL;
    }
    UASSERT(IsLineBreak(*pch));
    if (UCH_CR == *pch++)
        if (UCH_LF == *pch)
            pch++;
    return pch;
}
inline PWSTR WINAPI LineBreakAdvance (__in_ecount(2) PWSTR pch) {return const_cast<PWSTR>(LineBreakAdvance (const_cast<PCWSTR>(pch)));}

//---------------------------------------------------------------
// Surrogates
// [U2] 3.7 <Surrogates>
// [U2] 5.5 <Handling Surrogate Characters>
//
// All surrogates fall in a single contiguous range.
// In well-formed text:
//   -  Surrogates come in pairs that must be treated atomically.
//   -  A high surrogate always precedes a low surrogate.
//
#define  UCH_SURROGATE_FIRST         0xD800    // First surrogate
#define  UCH_HI_SURROGATE_FIRST      0xD800    // First High Surrogate
#define  UCH_PV_HI_SURROGATE_FIRST   0xDB80    // <Private Use High Surrogate, First>
#define  UCH_PV_HI_SURROGATE_LAST    0xDBFF    // <Private Use High Surrogate, Last>
#define  UCH_HI_SURROGATE_LAST       0xDBFF    // Last High Surrogate
#define  UCH_LO_SURROGATE_FIRST      0xDC00    // <Low Surrogate, First>
#define  UCH_LO_SURROGATE_LAST       0xDFFF    // <Low Surrogate, Last>
#define  UCH_SURROGATE_LAST          0xDFFF    // Last surrogate

#define IsSurrogate(ch)     IN_RANGE(ch, UCH_SURROGATE_FIRST,    UCH_SURROGATE_LAST)
#define IsHighSurrogate(ch) IN_RANGE(ch, UCH_HI_SURROGATE_FIRST, UCH_HI_SURROGATE_LAST)
#define IsLowSurrogate(ch)  IN_RANGE(ch, UCH_LO_SURROGATE_FIRST, UCH_LO_SURROGATE_LAST)


//---------------------------------------------------------------
// Hangul Jamo
// [U2] 3.10 <Combining Jamo Behavior>
// [U2] 5.13 <Locating Text Element Boundaries>
//
// Hangul syllables are composed of
//   -  leading consonant  (Choseong)
//   -  vowel              (Jungseong)
//   -  trailing consonant (Jongseong)
//

#define UCH_HANGUL_JAMO_FIRST          0x1100
#define UCH_HANGUL_JAMO_LEAD_FIRST     0x1100
#define UCH_HANGUL_JAMO_LEAD_LAST      0x115F
#define UCH_HANGUL_JAMO_VOWEL_FIRST    0x1160
#define UCH_HANGUL_JAMO_VOWEL_LAST     0x11A2
#define UCH_HANGUL_JAMO_TRAIL_FIRST    0x11A8
#define UCH_HANGUL_JAMO_TRAIL_LAST     0x11F9
#define UCH_HANGUL_JAMO_LAST           0x11FF

#define IsHangulJamo(ch) IN_RANGE(ch, UCH_HANGUL_JAMO_FIRST, UCH_HANGUL_JAMO_LAST)

//---------------------------------------------------------------
// JamoSyllableBreak - detect syllable (char) boundary in Hangul text
//
// [U2] 3.10 <Combining Jamo Behavior> 'Syllable Boundaries' Table 3-3
//
// Given two characters, either one or both a Hangul Jamo, returns
// TRUE when there is a syllable break between them. 
//
// Key: x = non-Jamo  * = break
//      L = Leading consonant  V = Vowel  T = Trailing consonant 
//
// Rule: 
//    x*L  x*V  x*T  L*x  V*x  T*x
//    V*L
//    T*L
//    T*V
//    All other combinations are part of the same syllable.
//
inline BOOL WINAPI JamoSyllableBreak(WCHAR c1, WCHAR c2)
{
    // if this fires, you aren't using it correctly see comments above
    UASSERT((IN_RANGE(c1,0x1100,0x11FF)) || (IN_RANGE(c2,0x1100,0x11FF)));
    return 
        ((c1 <  0x1100) || (c2 >  0x11FF)) ||	                // x*L  x*V  x*T  L*x  V*x  T*x
        ((c1 >= 0x11A8) && (c2 <  0x11A8)) ||	                // T*L, T*V
        ((c1 >= 0x1160) && (c1 <= 0x11A2) && (c2 <= 0x115F))	// V*L
        ;
}

#endif // __UNICHAR_H__

