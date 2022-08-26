//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Helper functions for Red Black Trees and Interval Trees.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// Structure to store a line/column location.
struct LineColumn
{
    long line;
    long column;
};

// Template specialization to use Location as a key to the tree.
struct LocationOperations :
    SimpleKeyOperationsT<Location>
{
    inline
    int compare(
        const Location * pLoc1,
        const Location * pLoc2)
    {
        return Location::Compare(pLoc1, pLoc2);
    }

    inline
    bool intersect(
        const Location * pLoc1,
        const Location * pLoc2)
    {
        return pLoc1->IsOverlappedInclusive(pLoc2);
    }
};

// Template specialization to use LineColumn as a key to the tree.
struct LineColumnOperations :
    SimpleKeyOperationsT<LineColumn>
{
    inline static
    int compare(
        const LineColumn * pPoint1,
        const LineColumn * pPoint2)
    {
        if (pPoint1->line == pPoint2->line)
        {
            return pPoint1->column - pPoint2->column;
        }

        return pPoint1->line - pPoint2->line;
    }
};

// Template specialization to perform operations between Location and LineColumn
// structures.
struct LocationToLineColumnOperations
{
   inline static
   int compareHigh(
       const Location * pInterval,
       const LineColumn * pPoint)
   {
        if (pInterval->m_lEndLine == pPoint->line)
        {
            return pInterval->m_lEndColumn - pPoint->column;
        }

        return pInterval->m_lEndLine - pPoint->line;
   }

   inline static
   int compareLow(
       const Location * pInterval,
       const LineColumn * pPoint)
   {
        if (pInterval->m_lBegLine == pPoint->line)
        {
            return pInterval->m_lBegColumn - pPoint->column;
        }

        return pInterval->m_lBegLine - pPoint->line;
   }

   inline static
   void copyHigh(
       const Location * PIntervalSrc,
       LineColumn * pPointDest)
   {
        pPointDest->line = PIntervalSrc->m_lEndLine;
        pPointDest->column = PIntervalSrc->m_lEndColumn;
   }
};

typedef  STRING *STRPTR;

struct STRINGOperations :
    public SimpleKeyOperationsT<STRPTR>
{
    int compare(
        _In_z_ const STRPTR * pKey1,
        _In_z_ const STRPTR * pKey2)
    {
        // Perform case insensitive comparison
        return CompareValues(
            StringPool::Pstrinfo(* pKey1),
            StringPool::Pstrinfo(* pKey2));
    }
};

typedef RedBlackTreeT
<
    STRPTR,
    STRINGOperations
> STRINGTree;

typedef RedBlackTreeT
<
    BCSYM_NamedRoot *
> NamedRootTree;

typedef RedBlackTreeT
<
    tokens
> TokenTypeTree;
