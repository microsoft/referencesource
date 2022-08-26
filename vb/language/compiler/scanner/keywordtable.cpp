//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

const kwdDsc g_tkKwdDescs[tkKwdCount] =
{
    #define KEYWORD(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                       { name, prec, rsvd, new7to8kwd, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr },
    #define KWD_MIN(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                       { name, prec, rsvd, new7to8kwd, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr },
    #define KWD_MAX(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                       { name, prec, rsvd, new7to8kwd, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr },
    #define KWD_QUO(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                       { name, prec, rsvd, new7to8kwd, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr },

    #include "keywords.h"

    #undef  KEYWORD
    #undef  KWD_MIN
    #undef  KWD_MAX
    #undef  KWD_QUO


};

/*****************************************************************************/

const WCHAR *g_tkKwdNames[tkKwdCount] =
{
    #define KEYWORD(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { WIDE(#str) },
    #define KWD_MIN(str, name, rsvd, new7to8kwd, prec,isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { WIDE(#str) },
    #define KWD_MAX(str, name, rsvd, new7to8kwd, prec,isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { WIDE(#str) },
    #define KWD_QUO(str, name, rsvd, new7to8kwd, prec,isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { WIDE(str) },

    #include "keywords.h"

    #undef  KEYWORD
    #undef  KWD_MIN
    #undef  KWD_MAX
    #undef  KWD_QUO
};

/*****************************************************************************/

const long g_tkKwdNameLengths[tkKwdCount] =
{
    #define KEYWORD(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { sizeof( WIDE( #str ) ) / sizeof( WCHAR ) - 1 },
    #define KWD_MIN(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { sizeof( WIDE( #str ) ) / sizeof( WCHAR ) - 1 },
    #define KWD_MAX(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { sizeof( WIDE( #str ) ) / sizeof( WCHAR ) - 1 },
    #define KWD_QUO(str, name, rsvd, new7to8kwd, prec, isQueryKwd, atEnd, atStart, isQueryClause, isLCUpperBound, canFollowExpr) \
                  { sizeof( WIDE( str ) ) / sizeof( WCHAR ) - 1 },

    #include "keywords.h"

    #undef  KEYWORD
    #undef  KWD_MIN
    #undef  KWD_MAX
    #undef  KWD_QUO
};

#if IDE 

STRING *TokenToString(tokens tok)
{
    return GetCompilerPackage()->TokenToString(tok);
}

#endif IDE 

tokens TokenOfString(_In_z_ STRING *pstr)
{
    return Compiler::TokenOfString(pstr);
}
