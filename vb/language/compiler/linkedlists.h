//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Contains template implementations of linked lists. Also contains
//  definition of a simple byte buffer.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// @@ Linked lists
// Simple parameterized singly and doubly link lists and iterators.
// Templates are used for safe type checking.
//
// Usage:
//
//  For singly linked lists, your data structure must inherit from CSingleLink:
//
//  struct CFoo : CSingleLink<CFoo>
//  {
//    ...
//  }
//
//  Then you can create singly lists of CFoo and its iterators as follows:
//
//  CFoo * pfoo;
//
//  CSingleList<CFoo>      foolist;
//  foolist.InsertFirst(pfoo);
//  foolist.InsertLast(pfoo);
//  ....
//
//  CSingleListIter<CFoo>  iter(&foolist);
//  while (pfoo = iter.Next())
//  {
//    ...
//  }
//
//
//  For doubly linked lists, your data structure must inherit from CDoubleLink:
//
//  struct CFoo : CDoubleLink<CFoo>
//  {
//    ...
//  }
//
//  Then you can create doubly lists of CFoo and its iterators as follows:
//
//  CFoo * pfoo;
//
//  CDoubleList<CFoo>      foolist;
//  foolist.InsertFirst(pfoo);
//  foolist.InsertLast(pfoo);
//  ....
//
//  CDoubleListFowardIter<CFoo>  f_iter(&foolist);
//  while (pfoo = f_iter.Next())
//  {
//    ...
//  }
//
//  CDoubleListBackIter<CFoo>  b_iter(&foolist);
//  while (pfoo = f_iter.Prev())
//  {
//    ...
//  }
//
//

//---------------------------------------------------------------------------
// CSingleLink class template
//---------------------------------------------------------------------------

template<class T> class CSingleList;
template<class T> class CSingleFunnel;
template<class T> class CSingleListIter;
template<class T> class CLinkRangeIter;

template<class T>
class CSingleLink
{
    friend class CSingleLink<T>;
    friend class CSingleFunnel<T>;
    friend class CSingleList<T>;
    friend class CSingleListIter<T>;
    friend class CLinkRangeIter<T>;
public:
    CSingleLink()
        : m_next(NULL)
    {
    }

    virtual ~CSingleLink()
    {
        VSASSERT(NULL == m_next, "This CSingleLink is still linked to something!");
    }

    T * Next() const
    {
        return m_next;
    }

    T ** NextPointer()
    {
        return &m_next;
    }

    void Insert(T * InsertNode)
    {
        // Make sure the next node isn't already set
        VSASSERT(InsertNode->m_next == NULL, "Already have a next");
        // 


        InsertNode->m_next = m_next;
        m_next = InsertNode;
    }

private:
    T * m_next;
};


//---------------------------------------------------------------------------
// CSingleListNode class template
//---------------------------------------------------------------------------
template<typename T>
class CSingleListNode : public CSingleLink<CSingleListNode<T> >
{
    T *m_value;

public:

    CSingleListNode(T *value)
        : m_value(value)
    {

    }

    virtual ~CSingleListNode()
    {
        // Nothing to destroy
    }

    T *GetValue() const
    {
        return m_value;
    }

private:

    // Do not auto generate
    CSingleListNode();
    CSingleListNode(const CSingleListNode&);
    CSingleListNode &operator=(const CSingleListNode&);
};

//---------------------------------------------------------------------------
// CSingleList class template
//---------------------------------------------------------------------------

template<class T>
class CSingleList
{
public:
    CSingleList()
        : m_first(NULL)
        , m_last(NULL)
        , m_ulCount(0)
        , m_fOwner(false)
    {
    }

    CSingleList(bool isOwner)
        : m_first(NULL)
        , m_last(NULL)
        , m_ulCount(0)
        , m_fOwner(isOwner)
    {
    }

    virtual ~CSingleList()
    {
        Reset();
    }

    // Ownership determines whether the destructor of CSingleList deletes
    // every link in the list.
    void SetOwnership(bool fOwner)
    {
        m_fOwner = fOwner;
    }

    bool GetOwnership() const
    {
        return( m_fOwner );
    }

    T * GetFirst() const
    {
        return m_first;
    }

    T * GetLast() const
    {
        return m_last;
    }

    T ** GetFirstPointer() const
    {
        return &m_first;
    }

    unsigned long NumberOfEntries() const
    {
        return m_ulCount;
    }

    // Moves the contents of the passed-in list into this list.
    void MoveContents(CSingleList<T> &List)
    {
        m_first = List.m_first;
        m_last = List.m_last;
        m_ulCount = List.m_ulCount;
        m_fOwner = List.m_fOwner;

        List.m_first = NULL;
        List.m_last = NULL;
        List.m_ulCount = 0;
    }

    // Prepend a new element pointed to by 'pLink'. pLink must not be NULL
    // and must not be linked to something else already.
    // If CSingleList has ownership, pLink is assumed to be freed only by
    // CSingleList.
    void InsertFirst(T * pLink)
    {
        VSASSERT(pLink != NULL, "Invalid parameter");
        VSASSERT(pLink->m_next == NULL,
                  "The link to be inserted is already linked to something else");

        pLink->m_next = m_first;
        m_first = pLink;
        if (m_ulCount == 0)
        {
                m_last = pLink;
        }
        m_ulCount += 1;
    }

    // Append a new element pointed to by 'pLink'. pLink must not be NULL
    // and must not be linked to something else already.
    // If CSingleList has ownership, pLink is assumed to be freed only by
    // CSingleList.
    void InsertLast(_In_ T * pLink)
    {
        VSASSERT(pLink != NULL, "Invalid parameter");
        VSASSERT(pLink->m_next == NULL,
                  "The link to be inserted is already linked to something else");

        if (m_ulCount == 0)
        {
            m_first = pLink;
        }
        else
        {
            m_last->m_next = pLink;
        }
        m_last = pLink;
        m_ulCount += 1;
    }

    //. Insert pNewLink after pExistingLink
    void Insert(
        T * pNewLink,
        T * pExistingLink)
    {
        VSASSERT(pExistingLink != NULL, "Invalid parameter");
        VSASSERT(pNewLink != NULL, "Invalid parameter");
        VSASSERT(pNewLink->m_next == NULL,
            "The link to be inserted is already linked to something else");

        pExistingLink->Insert(pNewLink);
        if (pExistingLink == m_last)
            m_last = pNewLink;

        m_ulCount++;
    }

    // Equivalent to calling RemoveRange, where start node = end node
    void Remove(T * pLink)
    {
        RemoveRange(pLink, pLink);
    }

    // Unlink the element pointed to by 'pStartLink' from the list, and continue unlinking subsequent elements
    // through pEndLink, inclusive.
    // pStartLink and pEndLink must not be NULL, and must be present in the list.
    // Elements will NOT be freed by CSingleList regardless of whether CSingleList has ownership or not.
    void RemoveRange(
        T * pStartLink,
        T * pEndLink)
    {
        VSASSERT(pStartLink != NULL && pEndLink != NULL, "Invalid parameter");
        VSASSERT(m_ulCount > 0, "There is no element in the list to unlink!");

        T * pPrevious = NULL;
        T * pNext = m_first;
        T * pCurrent;

        // Scan for start of range
        if (pStartLink != m_first)
        {
            for (pPrevious = m_first; pPrevious->m_next != pStartLink; pPrevious = pPrevious->m_next)
            {
                VSASSERT(pPrevious->m_next != NULL, "pStartLink was not found in the list");
            }
            pNext = pPrevious->m_next;
        }

        // Start removing elements until pEndLink is discovered
        do
        {
            VSASSERT(pNext != NULL, "pEndLink was not found in the list");
            pCurrent = pNext;
            pNext = pNext->m_next;
            pCurrent->m_next = NULL;
            m_ulCount--;
        }
        while (pCurrent != pEndLink);

        // Now fix up element before pStartLink
        if (pPrevious != NULL)
        {
            pPrevious->m_next = pNext;
        }
        else
        {
            m_first = pNext;
        }

        // And fix last element pointer
        if (m_last == pEndLink)
        {
            m_last = pPrevious;
        }
    }

    // Just throw away info about the List
    void Clear()
    {
       m_ulCount = 0;
        m_first   = NULL;
        m_last    = NULL;
    }

    // Throw away everything. If CSingleList has ownership, it will delete all
    // elements. Otherwise, it will unlink all elements from each other so
    // that each of them can be used again as new.

    void Reset()
    {
        if (m_fOwner)
        {
            DeleteAll();
        }
        else
        {
            T * pCurrent;

            while (m_first != NULL)
            {
                    pCurrent = m_first;
                    m_first = m_first->m_next;

                    pCurrent->m_next = NULL;
            }
            m_ulCount = 0;
            m_first   = NULL;
            m_last    = NULL;
        }
    }

    // Delete all elements in the list regardless of whether the list has
    // ownership or not.
    void DeleteAll()
    {
        T * pCurrent;

        while (m_first != NULL)
        {
            pCurrent = m_first;
            m_first = m_first->m_next;

            pCurrent->m_next = NULL;
            delete pCurrent;
        }
        m_last = NULL;
        m_ulCount = 0;
    }

    // Splice the passed-in list into the current list.
    void Splice(_Inout_ CSingleList<T> *psl)
    {
        VSASSERT(m_fOwner == psl->m_fOwner, "Different ownership");

        if (psl->m_first)
        {
            if (!m_last)
            {
                *this = *psl;
            }
            else
            {
                m_last->m_next = psl->m_first;
                m_last = psl->m_last;
                m_ulCount += psl->m_ulCount;
            }

            psl->m_first = psl->m_last = NULL;
            psl->m_ulCount = 0;
        }
    }

    bool IsEmpty() const
    {
        return NULL == m_first;
    }

protected:
    T *             m_first;
    T *             m_last;
    unsigned long   m_ulCount;
    bool            m_fOwner;

    friend class CSingleListIter<T>;
};


template<class T>
class CSingleFunnel : public CSingleList<T>
{
public:

    void Peel(T * pLink)
    {
        VSASSERT(pLink != NULL, "Invalid parameter");
        VSASSERT(m_ulCount > 0, "There is no element in the list to peel!");

        if (pLink == m_first) {
            m_first = pLink->m_next;
            // pLink->m_next = NULL;
            m_ulCount -= 1;
            if (m_last == pLink)
            {
                m_last = NULL;
            }
            return;
        } else {
            for (T * pPrevious = m_first; pPrevious->m_next != NULL; pPrevious = pPrevious->m_next)
            {
                if (pPrevious->m_next == pLink)
                {
                    pPrevious->m_next = pLink->m_next;
                    // pLink->m_next = NULL;

                    m_ulCount -= 1;
                    if (m_last == pLink)
                    {
                        m_last = pPrevious;
                    }
                    return;
                }
            }
        }
        VSASSERT(false, "No such element found in the list to peel!");
    }
};


//---------------------------------------------------------------------------
// CSingleListIter class template
//---------------------------------------------------------------------------

template<class T>
class CSingleListIter
{
public:
    CSingleListIter()
    {
    }

    CSingleListIter(CSingleList<T> * pList)
    {
        Init(pList);
    }

    void Init(CSingleList<T> * pList)
    {
        m_pList = pList;
        Reset();
    }

    T * Next()
    {
        T * pLink = m_pCurLink;

        // We could put VSASSERT(m_pCurLink != NULL, "No more element in list!");
        // here, but many clients who use the old version probably wants to see
        // a null pointer returned in this case.
        if (m_pCurLink != NULL)
        {
            m_pCurLink = m_pCurLink->m_next;
        }
        return pLink;
    }

    bool IsDone() const
    {
        return (m_pCurLink == NULL);
    }

    void Reset()
    {
        if (m_pList)
        {
            m_pCurLink = m_pList->m_first;
        }
        else
        {
            m_pCurLink = NULL;
        }
    }

private:
    T               * m_pCurLink;
    CSingleList<T>  * m_pList;
};


//---------------------------------------------------------------------------
// CLinkRangeIter class template
//---------------------------------------------------------------------------

template<class T>
class CLinkRangeIter
{
public:
    CLinkRangeIter()
    {
    }

    CLinkRangeIter(
        T * pStartLink,
        T * pEndLink)
    {
        Init(pStartLink, pEndLink);
    }

    void Init(
        T * pStartLink,
        T * pEndLink)
    {
        m_pStartLink = pStartLink;
        m_pEndLink = pEndLink;
        Reset();
    }

    T * Next()
    {
        T * pLink = m_pCurLink;

        // We could put VSASSERT(m_pCurLink != NULL, "No more element in list!");
        // here, but many clients who use the old version probably wants to see
        // a null pointer returned in this case.
        if (m_pCurLink != NULL)
        {
            if (m_pCurLink == m_pEndLink)
            {
                m_pCurLink = NULL;
            }
            else
            {
                m_pCurLink = m_pCurLink->m_next;
            }
        }
        return pLink;
    }

    bool IsDone() const
    {
        return (m_pCurLink == NULL);
    }

    void Reset()
    {
        m_pCurLink = m_pStartLink;
    }

private:
    T * m_pStartLink;
    T * m_pEndLink;
    T * m_pCurLink;
};


//---------------------------------------------------------------------------
// CDoubleLink class template
//---------------------------------------------------------------------------

template<class T> class CDoubleLink;
template<class T> class CDoubleList;
template<class T> class CDoubleListForwardIter;
template<class T> class CDoubleListBackIter;

template<class T>
class CDoubleLink
{
public:
    CDoubleLink()
        : m_next(NULL)
        , m_prev(NULL)
    {
    }

    virtual ~CDoubleLink()
    {
        VSASSERT(m_next == NULL && m_prev == NULL,
                  "This element is still linked to something!");
    }

    T * Next()
    {
        return m_next;
    }

    T * Prev()
    {
        return m_prev;
    }

private:
    T * m_prev;
    T * m_next;

    friend class CDoubleList<T>;
    friend class CDoubleListForwardIter<T>;
    friend class CDoubleListBackIter<T>;
};

//---------------------------------------------------------------------------
// CDoubleListNode class template
//---------------------------------------------------------------------------
template<typename T>
class CDoubleListNode : public CDoubleLink<CDoubleListNode<T> >
{
    T *m_value;

public:

    CDoubleListNode(T *value)
        : m_value(value)
    {

    }

    virtual ~CDoubleListNode()
    {
        // Nothing to destroy
    }

    T *GetValue() const
    {
        return m_value;
    }

private:

    // Do not auto generate
    CDoubleListNode();
    CDoubleListNode(const CDoubleListNode&);
    CDoubleListNode &operator=(const CDoubleListNode&);
};


//---------------------------------------------------------------------------
// CDoubleList class template
//---------------------------------------------------------------------------

template<class T>
class CDoubleList
{
public:
    CDoubleList()
        : m_first(NULL)
        , m_last(NULL)
        , m_ulCount(0)
        , m_fOwner(false)
    {
    }

    CDoubleList(bool isOwner)
        : m_first(NULL)
        , m_last(NULL)
        , m_ulCount(0)
        , m_fOwner(isOwner)
    {
    }

    ~CDoubleList()
    {
        Reset();
    }

    void SetOwnership(bool fOwner)
    {
        m_fOwner = fOwner;
    }

    T * GetFirst() const

    {
        return m_first;
    }

    T * GetLast() const

    {
        return m_last;
    }

    unsigned long NumberOfEntries() const
    {
        return m_ulCount;
    }

    bool IsEmpty() const
    {
        return m_ulCount == 0;
    }

    // Prepend a new element pointed to by 'pLink'. pLink must not be NULL
    // and must not be linked to something else already.
    // If CDoubleList has ownership, pLink is assumed to be freed only by
    // CDoubleList.
    void InsertFirst(T * pLink)
    {
        VSASSERT(pLink != NULL, "Invalid parameter");
        VSASSERT(pLink->m_next == NULL && pLink->m_prev == NULL,
                  "The link to be inserted is already linked to something else");

        pLink->m_next = m_first;
        pLink->m_prev = NULL;
        if (m_ulCount == 0)
        {
            m_last = pLink;
        }
        else
        {
            m_first->m_prev = pLink;
        }
        m_first = pLink;
        m_ulCount += 1;
    }

    // Append a new element pointed to by 'pLink'. pLink must not be NULL
    // and must not be linked to something else already.
    // If CDoubleList has ownership, pLink is assumed to be freed only by
    // CDoubleList.
    void InsertLast(T * pLink)
    {
        VSASSERT(pLink != NULL, "Invalid parameter");
        VSASSERT(pLink->m_next == NULL && pLink->m_prev == NULL,
                  "The link to be inserted is already linked to something else");

        pLink->m_prev = m_last;
        pLink->m_next = NULL;
        if (m_ulCount == 0)
        {
            m_first = pLink;
        }
        else
        {
            m_last->m_next = pLink;
        }
        m_last = pLink;
        m_ulCount += 1;
    }

    // Insert a new element pointed to by 'pLinkInsert' after the element
    // pointed to by 'pLinkAfter'. Both pointers must not be NULL.
    // pLinkAfter is assumed to be in the list, and pLinkInsert must not be
    // linked to anything else already.
    // If CDoubleList has ownership, pLinkInsert is assumed to be freed only
    // by CDoubleList.
    void InsertAfter(
        T * pLinkInsert,
        T * pLinkAfter)
    {
        VSASSERT(pLinkInsert != NULL, "Invalid parameter");
        VSASSERT(pLinkAfter != NULL, "Invalid parameter");
        VSASSERT(pLinkInsert->m_next == NULL && pLinkInsert->m_prev == NULL,
                  "The link to be inserted is already linked to something else");

#if DEBUG
        // Check whether pLinkAfter is in this list
        T * pTemp = m_first;
        while (pTemp != NULL && pTemp != pLinkAfter)
        {
            pTemp = pTemp->m_next;
        }
        VSASSERT(pTemp == pLinkAfter,
                  "pLinkAfter is not found in this list!");
#endif  // DEBUG

        if (pLinkAfter->m_next != NULL)
        {
            pLinkAfter->m_next->m_prev = pLinkInsert;
        }
        pLinkInsert->m_next = pLinkAfter->m_next;
        pLinkInsert->m_prev = pLinkAfter;
        pLinkAfter->m_next = pLinkInsert;
        m_ulCount += 1;

        if (pLinkAfter == m_last)
        {
            m_last = pLinkInsert;
        }
    }

    // Unlink the element pointed by pLink from the list. pLink must not
    // be NULL. pLink will not be deleted by CDoubleList regardless of
    // whether CDoubleList has ownership or not.
    void Remove(_In_ T * pLink)
    {
        VSASSERT(pLink != NULL, "Invalid parameter");

        if (m_ulCount == 0)
        {
            // This is the suspected cause of bug VSW 584964
            VbThrow(E_FAIL);
        }

#if DEBUG
    {
        T * pTest;

        for (pTest = m_first; pTest && pTest != pLink; pTest = pTest->m_next)
            ;

        VSASSERT(pTest, "Link is not in the list. This is the suspected cause of bug VSW 584964 ");
    }
#endif DEBUG

        //NULL case is handled by the last two if's
        if (pLink->m_next != NULL)
        {
            pLink->m_next->m_prev = pLink->m_prev;
        }
        if (pLink->m_prev != NULL)
        {
            pLink->m_prev->m_next = pLink->m_next;
        }
        if (pLink == m_first)
        {
            m_first = pLink->m_next;
        }
        if (pLink == m_last)
        {
            m_last = pLink->m_prev;
        }
        pLink->m_next = NULL;
        pLink->m_prev = NULL;

        m_ulCount -= 1;
    }

    // Throw away everything. If CDoubleList has ownership, it will delete all
    // elements. Otherwise, it will unlink all elements from each other so
    // that each of them can be used again as new.
    void Reset()
    {
        if (m_fOwner)
        {
            DeleteAll();
        }
        else
        {
            T * pTemp;
            while (m_first != NULL)
            {
                pTemp = m_first;
                m_first = m_first->m_next;

                pTemp->m_next = NULL;
                pTemp->m_prev = NULL;
            }
            m_first = NULL;
            m_last = NULL;
            m_ulCount = 0;
        }
    }

    // Delete all elements in the list regardless of whether the list has
    // ownership or not.
    void DeleteAll()
    {
        T * pTemp;

        while (m_first != NULL)
        {
            pTemp = m_first;
            m_first = m_first->m_next;

            pTemp->m_next = NULL;
            pTemp->m_prev = NULL;
            delete pTemp;
        }
        m_first = NULL;
        m_last = NULL;
        m_ulCount = 0;
    }

    void TransferFrom(CDoubleList<T> *pSourceList)
    {
        m_first = pSourceList->m_first;
        m_last = pSourceList->m_last;
        m_ulCount = pSourceList->m_ulCount;
        m_fOwner = pSourceList->m_fOwner;

        pSourceList->m_first = NULL;
        pSourceList->m_last = NULL;
        pSourceList->m_ulCount = 0;
        pSourceList->m_fOwner = false;;
    }

    IInsertableAtBothEnds<T *> * PInsertableAtBothEnds()
    {
        return (IInsertableAtBothEnds<T *> *)this;
    }
private:


protected:
    T *              m_first;
    T *              m_last;
    bool             m_fOwner;
    unsigned long    m_ulCount;

    friend class CDoubleListForwardIter<T>;
    friend class CDoubleListBackIter<T>;
};

//---------------------------------------------------------------------------
// CDoubleListForwardIter class template
//---------------------------------------------------------------------------

template<class T>
class CDoubleListForwardIter
{
public:
    CDoubleListForwardIter(CDoubleList<T> * pList)
        : m_pList(pList)
    {
        Reset();
    }

    T * Next()
    {
        T * pTemp = m_pCurLink;

        if (m_pCurLink != NULL)
        {
            m_pCurLink = m_pCurLink->m_next;
        }
        return pTemp;
    }

    void Reset()
    {
        m_pCurLink = m_pList->m_first;
    }

    bool IsDone() const
    {
        return (m_pCurLink == NULL);
    }

private:
    T               * m_pCurLink;            // current element
    CDoubleList<T>  * m_pList;
};

//---------------------------------------------------------------------------
// CDoubleListBackIter class template
//---------------------------------------------------------------------------

template<class T>
class CDoubleListBackIter
{
public:
    CDoubleListBackIter(CDoubleList<T> * pList)
        : m_pList(pList)
    {
        VSASSERT(m_pList != NULL, "CDoubleList<T> pointer is NULL!");
        Reset();
    }

    T * Prev()
    {
        T * pTemp = m_pCurLink;

        if (m_pCurLink != NULL)
        {
            m_pCurLink = m_pCurLink->m_prev;
        }
        return pTemp;
    }

    void Reset()
    {
        m_pCurLink = m_pList->m_last;
    }

    bool IsDone() const
    {
        return (m_pCurLink == NULL);
    }

private:
    T               * m_pCurLink;            // current element
    CDoubleList<T>  * m_pList;
};
