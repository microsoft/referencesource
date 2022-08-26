//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Manages the keyword mappings. 
//
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
// VB tokens kinds produced by the scanner.
//
typedef enum
{
#define KEYWORD(str, nam, res, new7to8kwd, prec, isQueryKwd, atEnd, atStart,  isQueryClause, isLCUpperBound, ccanFollowExpr) nam,
#define KWD_MIN(str, nam, res, new7to8kwd, prc, isQueryKwd, atEnd, atStart,  isQueryClause, isLCUpperBound, ccanFollowExpr) nam, tkKwdIdFirst = nam,
#define KWD_MAX(str, nam, res, new7to8kwd, prc, isQueryKwd, atEnd, atStart,  isQueryClause, isLCUpperBound, ccanFollowExpr) nam, tkKwdIdLast =nam,
#define KWD_QUO(str, nam, res, new7to8kwd, prc, isQueryKwd, atEnd, atStart,  isQueryClause, isLCUpperBound, ccanFollowExpr) nam,
#include "Keywords.h"
#undef  KEYWORD
#undef  KWD_MIN
#undef  KWD_MAX
#undef  KWD_QUO

    /*
        tkKwdCount  yields the number of keyword table entries
        tkKwdLast   yields the last entry in the keyword table
     */

    tkKwdCount,
    tkKwdLast = tkKwdCount-1,

    tkCount,
} tokens;

//-------------------------------------------------------------------------------------------------
//
// Precedence levels of unary and binary operators.
//
enum OperatorPrecedence
{
    PrecedenceNone = 0,
    PrecedenceXor,
    PrecedenceOr,
    PrecedenceAnd,
    PrecedenceNot,
    PrecedenceRelational,
    PrecedenceShift,
    PrecedenceConcatenate,
    PrecedenceAdd,
    PrecedenceModulus,
    PrecedenceIntegerDivide,
    PrecedenceMultiply,
    PrecedenceNegate,
    PrecedenceExponentiate,
    PrecedenceAwait
};

struct kwdDsc
{
    unsigned short  kdValue;
    unsigned        kdOperPrec    :5;
    unsigned        kdReserved    :1;
    unsigned        kdNew7to8kwd  :1;
    unsigned        kdIsQuerykwd  :1;
    unsigned        kdCanContinueAtEnd : 1;
    unsigned        kdCanContinueAtStart : 1;
    unsigned        kdIsQueryClause : 1;
    unsigned        kdIsLCUpperBound : 1;
    unsigned        kdCanFollowExpr : 1;
};

extern const kwdDsc g_tkKwdDescs[];
extern const WCHAR * g_tkKwdNames[];
extern const long g_tkKwdNameLengths[];

//
// Accessor functions to extract token information from the stringpool\
//

#if IDE 
STRING *TokenToString(tokens tok);
#endif IDE

tokens TokenOfString(_In_z_ STRING *pstr);

inline
OperatorPrecedence TokenOpPrec(tokens  tok)
{
    VSASSERT(tok < tkKwdCount, "Asking precedence of out-of-range token value.");
    VSASSERT(g_tkKwdDescs[tok].kdOperPrec != 0, "Asking precedence of non-operator.");
    return (OperatorPrecedence)g_tkKwdDescs[tok].kdOperPrec;
}

inline
bool TokenIsReserved(tokens  tok)
{
    VSASSERT(tok < tkKwdCount, "Should be Less");
    return g_tkKwdDescs[tok].kdReserved;
}

inline
bool KeywordIsNew7to8(tokens tok)
{
    VSASSERT(tok < tkKwdCount, "Should be Less");
    return g_tkKwdDescs[tok].kdNew7to8kwd;
}
