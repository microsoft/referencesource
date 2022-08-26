//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Low level bound tree methods.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// Biltree allocation size table
//
const USHORT ILTree::ILNode::s_uBiltreeAllocationSize[] =
{
#define SX_NL(en,ok,st)         ((const USHORT)sizeof(ILTree::##st )),
#define SX_NL_EX(en,ok,st, st2) ((const USHORT)sizeof(ILTree::##st )),
#include "BoundTreeTable.h"
#undef  SX_NL
#undef  SX_NL_EX
};

//-------------------------------------------------------------------------------------------------
//
// Biltree allocation size table for user defined ops.
// If SX_NL is used, the default BILTREE node is used.
//
const USHORT ILTree::ILNode::s_uUserDefinedOperatorBiltreeAllocationSize[] =
{
#define SX_NL(en,ok,st)         ((const USHORT)sizeof(ILTree::##st )),
#define SX_NL_EX(en,ok,st, st2) ((const USHORT)sizeof(ILTree::##st2 )),
#include "BoundTreeTable.h"
#undef  SX_NL
#undef  SX_NL_EX
};

//-------------------------------------------------------------------------------------------------
//
// Biltree EXPROPERKIND (SXK_xxxx) table
//
const unsigned short ILTree::ILNode::s_sxkindTable[] =
{
#define SX_NL(en,ok,st)         ok,
#define SX_NL_EX(en,ok,st,st2)  ok,
#include "BoundTreeTable.h"
#undef  SX_NL
#undef  SX_NL_EX
};

#if DEBUG

//-------------------------------------------------------------------------------------------------
//
// Biltree sxOpsName table (names of BILOP nodes)
//
const char * ILTree::ILNode::s_szBilopNames[] =
{
#define SX_NL(en,ok,st)         #en,
#define SX_NL_EX(en,ok,st, st2) #en,
#include "BoundTreeTable.h"
#undef  SX_NL
#undef  SX_NL_EX
};

//-------------------------------------------------------------------------------------------------
//
// Biltree which substruct table
//
const char * ILTree::ILNode::s_szBiltreeWhichSubStruct[] =
{
#define SX_NL(en,ok,st)         #st,
#define SX_NL_EX(en,ok,st, st2) #st2,
#include "BoundTreeTable.h"
#undef  SX_NL
#undef  SX_NL_EX
};

#endif // DEBUG



ILTree::Statement *ILTree::Statement::GetFirstStatementInGroup()
{
    ILTree::Statement *stmt = this;
    while (stmt->Parent != NULL && stmt->Parent->GroupId == GroupId)
    {
        stmt = stmt->Parent;
    }

    while (stmt->Prev != NULL && stmt->Prev->GroupId == GroupId)
    {
        stmt = stmt->Prev;
    }

    return stmt;
}

