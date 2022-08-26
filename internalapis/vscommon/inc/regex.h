// regex.h - Regular expression parser/engine
//--------------------------------------------------------------------------
// Copyright (c) 1988 - 1999, Microsoft Corp. All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//--------------------------------------------------------------------------
#pragma once

//#define SUPPORT_WILDCARD    // implement wildcard operator ?
//#define SUPPORT_CARET       // implement caret operator \c
//#define SUPPORT_WORDBREAK   // implement wordbreak operator %
#define SUPPORT_PROPS       // implement Unicode properties

#include "textfind2.h"

enum __RE_COMPILEFLAGS {
    REC_REGEX           = 0x00000001,       // Full VS regex syntax
    REC_WILDCARD        = 0x00000002,       // VB Wildcard matching
    REC_MATCHCASE       = 0x00000010,       // Match exact case (case-sensitive)
    REC_WHOLEWORD       = 0x00000020        // Match whole words
};
typedef DWORD RE_COMPILEFLAGS;

enum __RE_FINDFLAGS {
    REF_LINESTART       = 0x00000001,       // Buffer starts at line boundary
    REF_LINEEND         = 0x00000002,       // Buffer ends at line boundary
    REF_TRYMATCH        = 0x00010000,       // Match only at the start position (otherwise seeks)
    REF_REVERSE         = 0x00020000        // Scan buffer in reverse
};
typedef DWORD RE_FINDFLAGS;

enum __RE_MATCHINFO {
    REMI_FOUND          = 0x00000001,       // Found!
    REMI_LINESTART      = 0x00000002,       // Matched start of line (^)
    REMI_LINEEND        = 0x00000004,       // Matched end of line ($)
    REMI_TAGS           = 0x00000008,       // Tagged results available
    REMI_ENDOFBUFFER    = 0x00000010,       // Scanned to end of buffer
    REMI_CANCELLEDFIND  = 0x00000020        // User cancelled find
};
typedef DWORD RE_MATCHINFO;

typedef struct __RE_TEXTRANGE
{
    LONG    iStart;
    LONG    iLength;
} RE_TEXTRANGE, * PRE_TEXTRANGE;

//----------------------------------------------------------------
// Regex HRESULTS
//
// This must be kept in sync with VS textfind.idl.
//
#ifndef __RE_E_DEFINED__ // Don't collide with textfind.h if it's being used
#define __RE_E_DEFINED__
//                     Failure  |  Facility            | Code
#define RE_ERROR(x) (0x80000000 | (FACILITY_ITF << 16) | (x))

enum RE_ERRORS {
  RE_E_INTERNALERROR          = RE_ERROR(0x0001), // internal error (e.g. undefined opcode)
  RE_E_SYNTAXERROR            = RE_ERROR(0x0002), // syntax error in expression    
  RE_E_STACKOVERFLOW          = RE_ERROR(0x0003), // evaluation stack overflow
  RE_E_MISSINGARG             = RE_ERROR(0x0004), // missing argument in syntax
  RE_E_POWERARGOUTOFRANGE     = RE_ERROR(0x0005), // ^n power closure argument out of range
  RE_E_ESCAPEMISSINGARG       = RE_ERROR(0x0006), // \ or \x or \u missing valid argument
  RE_E_SPECIALUNKNOWN         = RE_ERROR(0x0007), // :x unknown x
  RE_E_TAGOUTOFRANGE          = RE_ERROR(0x0008), // \n n out of range
  RE_E_SETMISSINGCLOSE        = RE_ERROR(0x0009), // [] missing ]
  RE_E_TAGMISSINGCLOSE        = RE_ERROR(0x000a), // {} missing }
  RE_E_TOOMANYTAGS            = RE_ERROR(0x000b), // {} too many tagged expressions
  RE_E_EMPTYSET               = RE_ERROR(0x000c), // [] Empty set 
  RE_E_GROUPMISSINGCLOSE      = RE_ERROR(0x000d), // () missing ) 
  RE_E_REPLACETEXT            = RE_ERROR(0x000e), // Unable to create replacement text
};

#undef RE_ERROR
#endif // __RE_E_DEFINED__

// limits for ^n power closure
// Although 1 is redundant, we allow it to avoid generating overly pedantic errors.
// (exp)^1 matches the same thing as (exp).
#define MIN_POWER   1
#define MAX_POWER   256

//----------------------------------------------------------------
// RE_ScanChar - Scan a char, possibly expressed in the RE literal char syntax
//
PCWSTR WINAPI RE_ScanChar (_In_z_ PCWSTR pSrc, _Deref_opt_out_z_ PWSTR * ppDst);

//----------------------------------------------------------------
// RE_Free - Free storage (ctx, stack)
//
void WINAPI RE_Free (void * pv);

//----------------------------------------------------------------
// RE_Compile - compile a pattern
//
HRESULT WINAPI RE_Compile (
    /*[in]*/    RE_COMPILEFLAGS  flags,        // RE_COMPILEFLAGS options
    /*[in]*/    _In_z_ PCWSTR pszPattern,      // Text Pattern
    /*[out]*/   void **          ppvCtx        // Context for RE_Search - Must be RE_Free()'d.
    );

//----------------------------------------------------------------
// RE_Find - Search text using compiled pattern
//
// You must free the compiled pattern and stack using RE_Free.
// ppvStack should initially point at NULL.
//
HRESULT WINAPI RE_Find (
    /*[in]*/        RE_FINDFLAGS    flags,         // Search options
    /*[in]*/        PCWSTR          pchText,       // Text to search
    /*[in]*/        LONG            cchText,       // Length (-1 == NULL-terminated)
    /*[in]*/        PCWSTR          pchStart,      // Point within Text to start matching.
                                                   // if NULL, starts at the edge of the buffer
    /*[in,out]*/    void *          pvCtx,         // Previously compiled pattern
    /*[in,out]*/    void **         ppvStack,      // Evaluation stack (initially NULL, must be RE_Free()'d.)

    /*[out]*/       LONG *          pcchMatch,     // Length of match
    /*[out]*/       PCWSTR *        ppchMatch,     // Matched text in pchText
    /*[out,retval]*/RE_MATCHINFO *  pMatchInfo,    // RE_MATCHINFO - Info about match
    /*[in]*/        IVsFindCancelDialog * pFindCancelDialog = NULL // Ptr to IVsFindCancelDialog, defaults to NULL
    );

//----------------------------------------------------------------
// RE_TagCount - Get count of tagged items in match
//
HRESULT WINAPI RE_TagCount (
    /*[in] */ void * pvCtx, 
    /*[out]*/ LONG * pcTags
    );

//----------------------------------------------------------------
// RE_GetTags - Get array of text ranges for tagged items in match
//
// The first element is always the same as the match returned by RE_Find.
//
HRESULT WINAPI RE_GetTags (
    /*[in]*/        void            * pvCtx,    // Previously matched pattern
    /*[in]*/        LONG              cTags,    // Count of elements in prgTags
    /*[in,out]*/    PRE_TEXTRANGE     prgTags   // Array of ranges
    );

//----------------------------------------------------------------
// RE_*Translate - evaluate RE replace syntax
//----------------------------------------------------------------

enum __RE_TRANSLATEFLAGS
{
    // Use empty replacements for unmatched or out-of-range tag refs.
    // If not set, then unmatched or out-of-range tag refs generate an error.
    RETF_EMPTYTAGS          = 0x00000001    
};
typedef DWORD RE_TRANSLATEFLAGS;

//----------------------------------------------------------------
// This base class and Translate functions are used when the ranges 
// of a tagged match are translated to another coordinate space and
// text image. The VS environment uses these.
//
class __declspec(novtable) RE_TranslateHelper
{
public:

  // Return length of tagged item iSegment, from 0-9
  // Return -1 if iSegment is out of range.
  virtual LONG  SegmentLength (LONG iSegment); 

  // Copy the text of the tagged item iSegment into pDst.
  // Return a pointer to the next character after the end of the copied text.
  virtual PWSTR CopySegment   (LONG iSegment, __out_ecount(cchDst) PWSTR pDst, DWORD cchDst);

  virtual ~RE_TranslateHelper(){};
};

HRESULT WINAPI RE_THTranslateSize   (RE_TranslateHelper *pth, RE_TRANSLATEFLAGS flags, PCWSTR pszPattern, LONG *pcch);
HRESULT WINAPI RE_THTranslateBuffer (RE_TranslateHelper *pth, RE_TRANSLATEFLAGS flags, _In_opt_z_ PCWSTR pszPattern, _Out_z_cap_(cchResult) PWSTR pszResult, DWORD cchResult);
HRESULT WINAPI RE_THTranslate       (RE_TranslateHelper *pth, RE_TRANSLATEFLAGS flags, PCWSTR pszPattern, BSTR * pbstrResult);

//----------------------------------------------------------------
// RE_Translate - calculate the size required to evaluate the pattern.
//
// RETURNS: length of replacement pattern, or -1 if error
//
HRESULT WINAPI RE_TranslateSize (void *pvCtx, RE_TRANSLATEFLAGS flags, PCWSTR pszPattern, LONG *pcch);

//----------------------------------------------------------------
// RE_Translate - Generate translated text for replacement syntax.
//
HRESULT WINAPI RE_TranslateBuffer (void *pvCtx, RE_TRANSLATEFLAGS flags, _In_opt_z_ PCWSTR pszPattern, _Out_z_cap_(cchResult) PWSTR pszResult, DWORD cchResult);
HRESULT WINAPI RE_Translate       (void *pvCtx, RE_TRANSLATEFLAGS flags, PCWSTR pszPattern, BSTR * pbstrResult);

#ifdef SUPPORT_CARET
//----------------------------------------------------------------
// RE_CaretPos - Get caret position (\c) from matched RE (if any) 
//
HRESULT WINAPI RE_GetCaretPos (void *pvCtx, PCWSTR * ppchCaret);
#endif

//----------------------------------------------------------------
// RE_Status - Get HRESULT of previous compilation
//
HRESULT WINAPI RE_Status (void *pvCtx, HRESULT * phr);

enum __RE_FEATURES 
{
    REF_Any                 = 0x00000001,   // .
    REF_Closure             = 0x00000002,   // *
    REF_Closure1            = 0x00000004,   // +
    REF_MinClosure          = 0x00000008,   // @
    REF_MinClosure1         = 0x00000010,   // #
    REF_AnchorStartOfLine   = 0x00000020,   // ^
    REF_AnchorEndOfLine     = 0x00000040,   // $
    REF_OR                  = 0x00000080,   // |
    REF_Group               = 0x00000100,   // (expr)
    REF_Tag                 = 0x00000200,   // {expr}
    REF_PreviousTag         = 0x00000400,   // \#
    REF_Power               = 0x00000800,   // (expr)^n
    REF_Set                 = 0x00001000,   // [...]
    REF_NotSet              = 0x00002000,   // [^...]
    REF_Fail                = 0x00004000,   // ~(expr)
    REF_LineBreak           = 0x00008000,   // \n
    REF_Escape              = 0x00010000,   // \\, \t, etc.
    REF_Hex                 = 0x00020000,   // \u####, \x####
    REF_WordStart           = 0x00040000,   // <
    REF_WordEnd             = 0x00080000,   // >
    REF_WordBreak           = 0x00100000,   // % NOT IMPLEMENTED
    REF_Shorthand           = 0x00200000,   // :x
    REF_Property            = 0x00400000,   // :Xx
    REF_Wildcard            = 0x00800000,   // ? NOT IMPLEMENTED
    REF_Caret               = 0x01000000    // \c NOT IMPLEMENTED
};
typedef DWORD RE_FEATURES;

HRESULT WINAPI RE_AnalyseFeatures (PCWSTR pszPattern, RE_FEATURES * pREF);

// Replace features are only REF_Escape REF_Hex REF_Tag
HRESULT WINAPI RE_AnalyseReplaceFeatures (PCWSTR pszPattern, RE_FEATURES * pREF);

#if 0
enum __WC_FEATURES
{
    WCF_Any      = 0x00000001,   // ?
    WCF_Some     = 0x00000002,   // *
    WCF_Digit    = 0x00000004,   // #
    WCF_Set      = 0x00000008,   // [...]
    WCF_NotSet   = 0x00000010    // [!...]
};
typedef DWORD WC_FEATURES;
#endif
