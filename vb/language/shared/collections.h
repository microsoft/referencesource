//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

template <class T>
class IAppendable;

template <class T>
class ISet;

template <class T>
class IInsertableAtBothEnds;

template <class T>
class IStackOrQueue;

template <class T>
class ICollectionRoot
{
public:
    virtual ~ICollectionRoot()
    {
    }

    virtual bool IsAppendable()
    {
        return false;
    }

    virtual bool IsSet()
    {
        return false;
    }

    virtual bool IsInsertableAtBothEnds()
    {
        return false;
    }

    virtual bool IsStack()
    {
        return false;
    }

    virtual bool IsQueue()
    {
        return false;
    }

    virtual IAppendable<T> * PAppendable()
    {
        Assume(false, L"Invalid cast!");
        return NULL;
    }

    virtual ISet<T> * PSet()
    {
        Assume(false, L"Invalid cast!");
        return NULL;
    }

    virtual IInsertableAtBothEnds<T> * PInsertableAtBothEnds()
    {
        Assume(false, L"Invalid cast!");
        return NULL;
    }

    virtual IStackOrQueue<T> * PStackOrQueue()
    {
        Assume(false, L"Invalid cast!");
        return NULL;
    }
};

template <class T>
class IAppendable :
    public ICollectionRoot<T>
{
public:
    virtual void Add(const T & value) = 0;
    virtual unsigned long Count() const = 0;

    bool IsAppendable()
    {
        return true;
    }

    virtual IAppendable<T> * PAppendable()
    {
        return this;
    }
};

template <class T>
class ISet :
    public IAppendable<T>
{
public:
    virtual ~ISet()
    {
    }
    virtual bool Contains(const T & value) const = 0;
    virtual bool Remove(const T & value) = 0;
    virtual void Clear() = 0;
    virtual DestroyableIterator<T> * GetIterator() = 0;

    virtual bool IsSet()
    {
        return true;
    }

    virtual ISet<T> * PSet()
    {
        return this;
    }
};

template <class T>
class IInsertableAtBothEnds:
    public IAppendable<T>
{
public:
    virtual ~IInsertableAtBothEnds()
    {
    }

    virtual void InsertBegining(const T & value) = 0;
    virtual void InsertEnd(const T & value) = 0;
    virtual void Add(const T & value)
    {
        InsertEnd(value);
    }

    virtual bool IsInsertableAtBothEnds()
    {
        return true;
    }

    virtual IInsertableAtBothEnds<T> * PInsertableAtBothEnds()
    {
        return this;
    }
protected:
};

template <class T>
class IStackOrQueue :
    public IAppendable<T>
{
public:
    virtual void Add(const T & item)
    {
        PushOrEnqueue(item);
    }

    virtual void PushOrEnqueue(const T & item) = 0;
    virtual T PopOrDequeue() = 0;
    virtual const T & Peek() const = 0;
    virtual void Clear() = 0;
    virtual unsigned long Count() const = 0;

    virtual IStackOrQueue<T> * PStackOrQueue()
    {
        return this;
    }
protected:
};

template<class T>
class IReadOnlyList
{
public:
    virtual unsigned long Count() const = 0;
    virtual const T & operator[](const unsigned long Index) const = 0;
};

template <class T, class A>
class List;

template <class T, class A>
class ListNodeIterator;

//====================================================================
// A node in a singly linked list
//====================================================================
template <class T, class A>
class ListNode
{
    friend class List<T,A>;
    friend class ListNodeIterator<T,A>;
public:
    ListNode();
    const T & Data() const;
    T & Data();
    ListNode<T,A> * Next();
    const ListNode<T,A> * Next() const;
private:
    ListNode<T,A> * m_pNext;
    T m_data;
};

//============================================================
// The list's node iterator
//============================================================
template <class T, class A>
class ListNodeIterator : public virtual IIterator<ListNode<T, A>* >
{
public:
    ~ListNodeIterator();
    ListNodeIterator(List<T, A> * pList);
    ListNodeIterator(const ListNodeIterator<T, A> & src);
    virtual void Remove();
    virtual bool MoveNext();
    virtual ListNode<T,A> * Current();
private:
    bool m_removeFlag;
    List<T,A> * m_pList;
    ListNode<T,A> ** m_ppCur;
};

//============================================================
// The list's node iterator for constant lists
//============================================================
template <class T, class A>
class ListNodeConstIterator : public ConstIteratorWrapper<ListNode<T,A>*,List<T,A>,ListNodeIterator<T,A> >
{
public:
    ListNodeConstIterator(const List<T,A> *list)
        : ConstIteratorWrapper(list)
    {
    }
    ListNodeConstIterator(const ListNodeConstIterator<T,A> &src)
        : ConstIteratorWrapper(src)
    {
    }
};

//============================================================
// The list's node iterator
//============================================================
template <class T, class A=VBAllocWrapper>
class ListValueIterator : public virtual IIterator<T>
{
public:
    ~ListValueIterator();
    ListValueIterator(List<T,A> * pList);
    ListValueIterator(const ListValueIterator<T, A> & src);
    virtual void Remove();
    virtual bool MoveNext();
    virtual T Current();
private:
    ListNodeIterator<T,A> m_wrapped;
};


//============================================================
// The list's value iterator for constant lists
//============================================================
template <class T, class A>
class ListValueConstIterator :
    public ConstIteratorWrapper<T, List<T, A>, ListValueIterator<T,A> >
{
public:
    ListValueConstIterator(const List<T,A> * list) :
        ConstIteratorWrapper(list)
    {
    }

    ListValueConstIterator(const ListValueConstIterator<T, A> & src) :
        ConstIteratorWrapper(src)
    {
    }
};


//====================================================================
// A singly linked list.
//====================================================================
template <class T, class A=VBAllocWrapper>
class List :
    public IInsertableAtBothEnds<T>
{
    friend class ListNodeIterator<T,A>;
public:

    List(const A & alloc=A());
    ~List();

    //IInsertableAtBothEnds methods
    void InsertBegining(const T & data)
    {
        AddFirst(data);
    }

    void InsertEnd(const T & data)
    {
        AddLast(data);
    }

    ListValueConstIterator<T,A> GetConstIterator() const
    {
        return ListValueConstIterator<T,A>(this);
    }

    ListValueIterator<T,A> GetIterator()
    {
        return ListValueIterator<T,A>(this);
    }

    ListNode<T,A> * AddFirst(const T & data);
    ListNode<T,A> * AddLast(const T & data);
    ListNode<T,A> * AddAfter(ListNode<T,A> *  pNode, const T& data);
    ListNode<T,A> * AddAfter(const T& after, const T& data);
    ListNode<T,A> * Find(const T& data) const;
    void InefficentRemove(ListNode<T,A> * pNode);
    void InefficentRemove(const T & data);
    unsigned long Count() const;
    ListNode<T,A> *GetFirst();
    const ListNode<T,A> *GetFirst() const;
    void Clear();
    bool Contains(const T& value) const;
private:
    // Do not autogenerate
    List(const List<T,A>&);
    List<T,A>& operator=(const List<T,A>&);

    ListNode<T,A> * m_pFakeHead;
    ListNode<T,A> * m_pFakeTail;
    A m_alloc;
    unsigned long m_Count;
};

//====================================================================
//
//====================================================================
template <class K, class V>
class KeyValuePair
{
public:
    KeyValuePair();
    KeyValuePair(const K & key, const V & value);
    K & Key();
    V & Value();
    const K & Key() const;
    const V & Value() const;
private:
    V m_value;
    K m_key;
};

template <class K,class V,class A, class H>
class DynamicHashTable;

template <class K>
class DefaultHashFunction
{
public:
    unsigned long GetHashCode(const K & value) const;
    bool AreEqual(const K & val1, const K & val2) const;
};

class CaseInsensitiveUnicodeHashFunction
{
public:
    unsigned long GetHashCode(const wchar_t * pString) const;
    bool AreEqual(const wchar_t * val1, const wchar_t * val2) const;
};


//====================================================================
//
//====================================================================
template <class K,class V, class A, class H = DefaultHashFunction<K> >
class HashTableIterator : public virtual IIterator<KeyValuePair<K,V> >
{
public:
    HashTableIterator(DynamicHashTable<K,V,A,H> * pHashTable);
    HashTableIterator(const HashTableIterator<K,V,A,H> & src);
    ~HashTableIterator();
    virtual bool MoveNext();
    virtual KeyValuePair<K,V> Current();
    virtual void Remove();
private:
    void GetNextBucket();
    unsigned long m_tableIndex;
    bool m_fBeforeStart;
    DynamicHashTable<K,V,A,H> * m_pHashTable;
    DestroyableIterator<ListNode<KeyValuePair<K,V>, A> *>  * m_iter;
};

//====================================================================
//
//====================================================================
template <class K, class V, class A, class H = DefaultHashFunction<K>>
class HashTableConstIterator : public ConstIteratorWrapper<KeyValuePair<K,V>, DynamicHashTable<K,V,A,H>, HashTableIterator<K,V,A,H> >
{
public:
    HashTableConstIterator(const DynamicHashTable<K,V,A,H> *pHashTable)
        :ConstIteratorWrapper(pHashTable)
    {
    }
    HashTableConstIterator(const HashTableConstIterator<K,V,A,H> &src)
        :ConstIteratorWrapper(src)
    {
    }
};

//====================================================================
//
//====================================================================
template <class K, class V, class A, class H = DefaultHashFunction<K>>
class HashTableKeyIterator : public virtual IIterator<K>
{
public:
    ~HashTableKeyIterator();
    HashTableKeyIterator(DynamicHashTable<K,V,A,H> * pHashTable);
    HashTableKeyIterator(const HashTableKeyIterator<K,V,A,H> & src);
    virtual bool MoveNext();
    virtual K Current();
    virtual void Remove();
private:
    HashTableIterator<K,V,A,H> m_iter;
};

//====================================================================
//
//====================================================================
template <class K, class V, class A, class H = DefaultHashFunction<K>>
class HashTableValueIterator : public virtual IIterator<V>
{
public:
    ~HashTableValueIterator();
    HashTableValueIterator(DynamicHashTable<K,V,A,H> * pHashTable);
    HashTableValueIterator(const HashTableValueIterator<K,V,A,H> & src);
    virtual bool MoveNext();
    virtual V Current();
    virtual void Remove();
private:
    HashTableIterator<K,V,A,H> m_iter;
};  

//====================================================================
//
//====================================================================
template <class K, class V, class A, class H = DefaultHashFunction<K>>
class HashTableValueConstIterator : public ConstIteratorWrapper<V, DynamicHashTable<K,V,A,H>, HashTableValueIterator<K,V,A,H> >
{
public:
    HashTableValueConstIterator(const DynamicHashTable<K,V,A,H> *pHashTable)
        :ConstIteratorWrapper(pHashTable)
    {
    }
    HashTableValueConstIterator(const HashTableValueConstIterator<K,V,A,H> &src)
        :ConstIteratorWrapper(src)
    {
    }
};


//====================================================================
//
//====================================================================
template <class K, class V, class A=VBAllocWrapper, class H = DefaultHashFunction<K>>
class DynamicHashTable
{
    friend class HashTableIterator<K,V,A,H>;
public:
    ~DynamicHashTable();
    DynamicHashTable(const A & alloc = A(), const H & hashFunc = H(), const unsigned long capacity = default_initial_capacity);

    bool Init(const unsigned long capacity);
    void SetValue(const K & key, const V & value);
    bool Contains(const K & key) const;
    unsigned long Count() const;
    bool GetValue(const K & key, _Out_opt_ V * pOutValue) const;
    V GetValue(const K &key) const;
    V GetValueOrDefault(const K &key, V defaultValue = V()) const;
    bool Remove(const K & key);
    void Clear();
    static const unsigned long default_initial_capacity = 16;

    HashTableKeyIterator<K,V,A,H> GetKeyIterator()
    {
        return HashTableKeyIterator<K,V,A,H>(this);
    }
    
    HashTableValueConstIterator<K,V,A,H> GetValueConstIterator() const
    {
        return HashTableValueConstIterator<K,V,A,H>(this);
    }

    HashTableValueIterator<K,V,A,H> GetValueIterator()
    {
        return HashTableValueIterator<K,V,A,H>(this);
    }

    HashTableConstIterator<K,V,A,H> GetConstIterator() const
    {
        return HashTableConstIterator<K,V,A,H>(this);
    }

    HashTableIterator<K,V,A,H> GetIterator() 
    {
        return HashTableIterator<K,V,A,H>(this);
    }

protected:
    A m_alloc;
private:
    DynamicHashTable
    (
        const A & alloc,
        const H & hashFunc,
        unsigned long capacity,
        unsigned long count,
        List<KeyValuePair<K,V>, A> ** rgListKVP
    );

    // Do not autogenerate
    DynamicHashTable(const DynamicHashTable<K,V,A,H> &);
    DynamicHashTable<K,V,A,H>& operator=(const DynamicHashTable<K,V,A,H>&);

    unsigned long Bucket(const K & value) const;
    void Resize();



    bool GetValueInternal(const K &key, _Out_ V &value) const;
    unsigned long m_capacity;
    unsigned long m_count;
    List<KeyValuePair<K,V>, A> ** m_rgListKVP;
    H m_hashFunc;
};

template <class T, class A, class H>
class HashSet;

//====================================================================
//
//====================================================================
template <class T, class A=VBAllocWrapper, class H = DefaultHashFunction<T> >
class HashSetIterator : public virtual HashTableKeyIterator<T, bool, A, H>
{
public:
    HashSetIterator(HashSet<T, A,H> * pHashSet);
};

//====================================================================
//
//====================================================================
template <class T, class A=VBAllocWrapper, class H = DefaultHashFunction<T>>
class HashSet :
    protected DynamicHashTable<T, bool, A, H>,
    public ISet<T>
{
public:
	HashSet();
    HashSet(const A & alloc, const H & hashFunc = H(), const unsigned long capacity = default_initial_capacity);
    bool Contains(const T & value) const;
    void Add(const T & value);
    bool Remove(const T & value);
    void Clear();
    DestroyableIterator<T> * GetIterator();
    unsigned long Count() const;
};

template <class T>
class LessThanComparer
{
public:
    int Compare(const T& item1, const T&item2) const;
};

template <class T, class A>
class ArrayListIterator;

template <class T, class A>
class ArrayListReverseIterator;

//====================================================================
//
//====================================================================
template <class T, class A=VBAllocWrapper>
class ArrayList :
    public IAppendable<T>
{
    friend class ArrayListIterator<T, A>;
    friend class ArrayListReverseIterator<T,A>;

public:
	ArrayList();
    ArrayList(const A & m_alloc, const unsigned long capacity = default_initial_capacity);
    ArrayList(const ArrayList & src);
    ~ArrayList();
    void Add(const T & item);
    bool Remove(const unsigned long index);
    unsigned long Count() const;
    const T & operator[](const unsigned long index) const;
    T & operator[](const unsigned long);
    void Reserve(const unsigned long index);
    void Clear();
    unsigned long Capacity();
    void Fill(unsigned long count, const T& value);

    ArrayListIterator<T,A> GetIterator()
    {
        return ArrayListIterator<T,A>(this);
    }

    template <class Comparer>
    void Sort(Comparer & comp)
    {
        MakeHeap(comp);

        for (unsigned long i = m_count; i > 1; i-=1)
        {
            Swap(i-1, 0);
            MaintainHeap(comp, 0, i-1);
        }
    }

    template <class Comparer>
    bool BinarySearch(const T & item, unsigned long * pIndex, Comparer & comp)
    {
        unsigned long start = 0;
        unsigned long end = m_count;

        while (start < end)
        {
            unsigned long middle = ((end - start) / 2) + start;
            int compResult = comp.Compare(item, m_pData[middle]);
            if (compResult == 0)
            {
                if (pIndex)
                {
                    *pIndex = middle;
                }
                return true;
            }
            else if (compResult < 0)
            {
                end = middle;
            }
            else
            {
                start = middle + 1;
            }
        }

        return false;
    }

    template <class Comparer>
    bool RemoveLinear(const T & item, Comparer & comp)
    {
        unsigned long index = -1;
        if (LinearSearch(item, &index, comp))
        {
            Remove(index);
            return true;
        }

        return false;
    }

    template <class Comparer>
    bool LinearSearch(const T & item, _Out_opt_ unsigned long * pIndex, Comparer & comp)
    {
        for (unsigned long i = 0; i < m_count; ++i)
        {
            if (comp.Compare(item, m_pData[i]) == 0)
            {
                if (pIndex)
                {
                    *pIndex = i;
                }
                return true;
            }
        }
        return false;
    }

private:
    template <class Comparer>
    void MakeHeap(Comparer & comp)
    {
        for (unsigned long i = m_count / 2; i > 0; i -= 1)
        {
            MaintainHeap(comp, i - 1, m_count);
        }
    }

    template <class Comparer>
    void MaintainHeap(Comparer & comp, unsigned long index, unsigned long count)
    {

        unsigned long left = LeftChild(index);
        unsigned long right = RightChild(index);
        unsigned long most = index;

        do
        {
            index = most;
            most = Most (comp, index, left, right, count);
            if (index != most)
            {
                Swap(index, most);
                left= LeftChild(most);
                right = RightChild(most);
            }
        }
        while (most != index);
    }

    template <class Comparer>
    unsigned long Most(Comparer & comp, unsigned long index, unsigned long left, unsigned long right, unsigned long count)
    {
        if (index >= count)
        {
            return 0;
        }
        else
        {
            return More(comp, index, More(comp, left, right, count), count);
        }
    }

    template <class Comparer>
    unsigned long More(Comparer & comp, unsigned long i1, unsigned long i2, unsigned long count)
    {
        if (i1 >= count)
        {
            return i2;
        }
        else if (i2 >= count)
        {
            return i1;
        }
        else if (comp.Compare(m_pData[i1], m_pData[i2]) > 0)
        {
            return i1;
        }
        else
        {
            return i2;
        }
    }


    unsigned long LeftChild(unsigned long index);
    unsigned long RightChild(unsigned long index);
    void Swap(unsigned long i1, unsigned long i2);

    static const unsigned long default_initial_capacity = 16;
    T dummy_t_value;

    T* m_pData;
    unsigned long m_count;
    unsigned long m_capacity;
    A m_alloc;
};

//====================================================================
//
//====================================================================
template <class T, class A=VBAllocWrapper>
class ArrayListIterator : public virtual IIterator<T>
{
public:
    ArrayListIterator(ArrayList<T, A>* pArrayList);
    ArrayListIterator(const ArrayListIterator<T, A> & src);
    virtual ~ArrayListIterator();
    virtual bool MoveNext();
    virtual T Current();
    virtual void Remove();
private:
    ArrayList<T, A> * m_pArrayList;
    unsigned long m_index;
    bool m_fBeforeStart;
};

//====================================================================
//
//====================================================================
template <class T, class A>
class ArrayListReverseIterator : public virtual IIterator<T>
{
public:
    ArrayListReverseIterator(ArrayList<T,A> *pArrayList);
    ArrayListReverseIterator(const ArrayListReverseIterator<T,A> &src);
    virtual ~ArrayListReverseIterator() { }
    virtual bool MoveNext();
    virtual T Current();
    virtual void Remove();
private:
    ArrayList<T,A> *m_pArrayList;
    unsigned long m_index;
    bool m_beforeStart;
    bool m_fRemove;
};

template <class T, class A>
class ArrayListReverseConstIterator : public ConstIteratorWrapper<T,ArrayList<T,A>, ArrayListReverseIterator<T,A> >
{
public:
    ArrayListReverseConstIterator(const ArrayList<T,A> *pArrayList)
        : ConstIteratorWrapper(pArrayList)
    {
    }
    ArrayListReverseConstIterator(const ArrayListReverseConstIterator<T,A> &src)
        : ConstIteratorWrapper(src)
    {
    }
};

//====================================================================
//
//====================================================================
template <class T, class A=VBAllocWrapper>
class Filo :
    public IStackOrQueue<T>
{
public:

    Filo(const A& alloc=A())
        : m_list(alloc)
    {
    }

    ~Filo() { }

    ArrayListReverseConstIterator<T,A> GetConstIterator() const
    {
        return ArrayListReverseConstIterator<T,A>(&m_list);
    }

    ArrayListReverseIterator<T,A> GetIterator()
    {
        return ArrayListReverseIterator<T,A>(&m_list);
    }

    void PushOrEnqueue(const T &item) 
    { 
        m_list.Add(item); 
    }

    T PopOrDequeue()
    {
        Assume(m_list.Count() > 0, L"Filo is empty");
        T ret = m_list[m_list.Count() - 1];
        m_list.Remove(m_list.Count()-1);
        return ret;
    }

    T &Peek()
    {
        Assume(m_list.Count() > 0, L"Filo is empty");
        return m_list[m_list.Count()-1];
    }

    const T& Peek() const
    {
        Assume(m_list.Count() > 0, L"Filo is empty");
        return m_list[m_list.Count()-1];
    }

    void Clear() { m_list.Clear(); }
    unsigned long Count() const { return m_list.Count(); }
    virtual bool IsStack() { return true; }
private:
    // Do not auto generate

    ArrayList<T,A> m_list;
};

class NegatedBitVector;

class IReadonlyBitVector
{
public:
    virtual ~IReadonlyBitVector() {}
    virtual bool BitValue(unsigned long index) const = 0 ;
    virtual unsigned long BitCount() const = 0;
    virtual unsigned long SetCount() const = 0;
};

class IBitVector :
    public virtual IReadonlyBitVector
{
public:
    virtual ~IBitVector() {}
    virtual void AddBit(bool value) = 0;
    virtual void SetBit(unsigned long index, bool value = true) = 0;
    virtual void ClearAll() = 0;
};

template <class A=VBAllocWrapper>
class BitVector :
    public virtual IBitVector
{
public:
    BitVector(const A & alloc = A(), unsigned long numberOfBits = BITS_PER_ENTRY);
    virtual ~BitVector() {}
    void AddBit(bool value);
    void SetBit(unsigned long index, bool value = true);
    bool BitValue(unsigned long index) const;
    void ClearAll();
    unsigned long BitCount() const;
    unsigned long SetCount() const;
private:
    static inline unsigned long Map(bool value);
    inline unsigned long GetTotalBitCapacity();
    static unsigned long ComputeArraySize(unsigned long index);

    static const unsigned long BITS_PER_ENTRY = sizeof(unsigned long) * 8;
    ArrayList<unsigned long, A> m_ArrayList;
    unsigned long m_setCount;
    unsigned long m_bitCount;
};

class NegatedBitVector :
    public virtual IReadonlyBitVector
{
public:
    NegatedBitVector(IReadonlyBitVector * pWrapped);
    bool BitValue(unsigned long index) const;
    unsigned long BitCount() const;
    unsigned long SetCount() const;
private:
    IReadonlyBitVector * m_pWrapped;
};

template <class T, class A>
class QueueIterator;


//====================================================================
//
//====================================================================
template <class T, class A=VBAllocWrapper>
class Queue :
    public virtual IStackOrQueue<T>
{
    friend class QueueIterator<T, A>;
public:
    Queue(const A & alloc = A());

    virtual void PushOrEnqueue(const T & item) ;
    virtual T PopOrDequeue() ;
    virtual const T & Peek() const;
    virtual void Clear();
    virtual unsigned long Count() const;

    virtual bool IsQueue()
    {
        return true;
    }

private:
    List<T, A> m_list;
};

template <class T, class A>
class QueueIterator :
    public virtual IConstIterator<T>
{
public:
    QueueIterator(const Queue<T, A> * pQueue)  :
        m_iterator(pQueue ? &pQueue->m_list : NULL)
    {
    }

    virtual bool MoveNext() { return m_iterator.MoveNext(); }
    virtual T Current() { return m_iterator.Current(); }
private:
    ListValueConstIterator<T, A> m_iterator;
};

template <class T>
class ChainIterator :
    public IConstIterator<T>
{
public:
    ChainIterator(IConstIterator<T>* firstIterator, IConstIterator<T>* secondIterator);

    virtual bool MoveNext();
    virtual T Current();

private:
    IConstIterator<T>* m_pFirstIterator;
    IConstIterator<T>* m_pSecondIterator;
};
//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T, A>::ListNode() : m_pNext(NULL), m_data()
{
}

//====================================================================
//
//====================================================================
template <class T, class A>
const T & ListNode<T,A>::Data() const
{
    return m_data;
}

//====================================================================
//
//====================================================================
template <class T, class A>
T & ListNode<T, A>::Data()
{
    return m_data;
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T,A> * ListNode<T,A>::Next()
{
    return m_pNext;
}

//====================================================================
//
//====================================================================
template <class T, class A>
const ListNode<T,A> * ListNode<T,A>::Next() const
{
    return m_pNext;
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNodeIterator<T,A>::~ListNodeIterator()
{
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNodeIterator<T,A>::ListNodeIterator(List<T,A> * pList) :
    m_pList(pList),
    m_ppCur(pList ? &pList->m_pFakeHead : NULL),
    m_removeFlag(false)
{
    Assume(pList,L"An attempt was made to construct a ListNodIterator<T,A> with a null list pointer");
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNodeIterator<T,A>::ListNodeIterator(const ListNodeIterator<T,A> & src) :
    m_removeFlag(src.m_removeFlag),
    m_pList(src.m_pList),
    m_ppCur(src.m_ppCur)
{
}

//====================================================================
//
//====================================================================
template <class T, class A>
void ListNodeIterator<T,A>::Remove()
{
    Assume(!m_removeFlag,L"Remove was called more than once without an intervening call to MoveNext()!");

    if (!m_removeFlag)
    {
        Assume((*m_ppCur != m_pList->m_pFakeHead) && (*m_ppCur != m_pList->m_pFakeTail),L"Remove was called on an iterator located at an invalid position. Perhaps a call to MoveNext() is missing or the iterator has been exausted.");
        if ((*m_ppCur != m_pList->m_pFakeHead) && (*m_ppCur != m_pList->m_pFakeTail))
        {
            ListNode<T,A> * pTmp = *m_ppCur;
            *m_ppCur = pTmp->m_pNext;
            m_removeFlag = true;
            m_pList->m_alloc.DeAllocate(pTmp);
            --(m_pList->m_Count);
        }
    }
}

//====================================================================
//
//====================================================================
template <class T, class A>
bool ListNodeIterator<T, A>::MoveNext()
{
    if (m_ppCur)
    {
        if (m_removeFlag)
        {
            m_removeFlag = false;
        }
        else if (*m_ppCur != m_pList->m_pFakeTail)
        {
            m_ppCur = &((*m_ppCur)->m_pNext);
        }
    }
    return m_ppCur && (*m_ppCur != (m_pList->m_pFakeTail));
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T, A> * ListNodeIterator<T,A>::Current()
{
    Assume(! m_removeFlag,L"A call to Current() was made after a call to Remove() without an intervening call to MoveNext()");
    Assume(((*m_ppCur) != m_pList->m_pFakeHead) && ((*m_ppCur) != m_pList->m_pFakeTail),L"An attempt was made to extract a value from an interator that is in an invalid state. Perhaps a call to MoveNext() is missing or the iterator has been exausted.");

    if (! m_removeFlag  && ((*m_ppCur) != m_pList->m_pFakeHead) && ((*m_ppCur) != m_pList->m_pFakeTail))
    {
        return (*m_ppCur);
    }
    else
    {
        return NULL;
    }
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListValueIterator<T,A>::~ListValueIterator()
{
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListValueIterator<T,A>::ListValueIterator(List<T,A> * pList) : m_wrapped(pList)
{
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListValueIterator<T,A>::ListValueIterator(const ListValueIterator<T, A> & src) : m_wrapped(src.m_wrapped)
{
}

//====================================================================
//
//====================================================================
template <class T, class A>
void ListValueIterator<T,A>::Remove()
{
    m_wrapped.Remove();
}

//====================================================================
//
//====================================================================
template <class T, class A>
bool ListValueIterator<T,A>::MoveNext()
{
    return m_wrapped.MoveNext();
}

//====================================================================
//
//====================================================================
template <class T, class A>
T ListValueIterator<T,A>::Current()
{
    return m_wrapped.Current()->Data();
}

//====================================================================
//
//====================================================================
template <class T, class A>
List<T,A>::~List()
{
    ListNodeIterator<T,A> iter(this);
    while (iter.MoveNext())
    {
        iter.Remove();
    }
    m_alloc.DeAllocate(m_pFakeHead);
    m_alloc.DeAllocate(m_pFakeTail);
    m_pFakeHead = NULL;
    m_pFakeTail = NULL;
}

//====================================================================
//
//====================================================================
template <class T, class A>
List<T,A>::List(const A & alloc) : m_alloc(alloc), m_pFakeHead(NULL), m_pFakeTail(NULL), m_Count(0)
{
    m_pFakeHead = m_alloc.Allocate<ListNode<T,A> >();
    Assume(m_pFakeHead,L"Allocation failed in List<T,A>::List(Compiler *)");
    m_pFakeTail = m_alloc.Allocate<ListNode<T,A> >();
    Assume(m_pFakeTail,L"Allocation failed in List<T,A>::List(Compiler *)");
    m_pFakeHead->m_pNext = m_pFakeTail;
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T, A> * List<T,A>::GetFirst()
{
    if (m_pFakeHead->m_pNext != m_pFakeTail)
    {
        return m_pFakeHead->m_pNext;
    }
    else
    {
        return NULL;
    }
}

template <class T, class A>
const ListNode<T, A> * List<T,A>::GetFirst() const
{
    if (m_pFakeHead->m_pNext != m_pFakeTail)
    {
        return m_pFakeHead->m_pNext;
    }
    else
    {
        return NULL;
    }
}

//====================================================================
//
//====================================================================
template <class T, class A>
void List<T,A>::Clear()
{
    ListNodeIterator<T,A> iter(this);

    while (iter.MoveNext())
    {
        iter.Remove();
    }
}

//====================================================================
//
//====================================================================
template <class T, class A>
bool List<T,A>::Contains(const T& value) const
{
    ListNodeConstIterator<T,A> it(this);
    while ( it.MoveNext() )
    {
        if ( it.Current()->Data() == value )
        {
            return true;
        }
    }

    return false;
}


//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T,A> * List<T,A>::AddFirst(const T & data)
{
    ListNode<T,A> * pNewNode = m_alloc.Allocate<ListNode<T,A> >();

    Assume(pNewNode,L"Allocation failed in List<T,A>::AddFirst");
    if (pNewNode)
    {
        pNewNode->m_pNext = m_pFakeHead->m_pNext;
        pNewNode->m_data = data;
        m_pFakeHead->m_pNext = pNewNode;
        ++m_Count;
    }
    return pNewNode;
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T,A> * List<T,A>::AddLast(const T &data)
{
    ListNode<T,A> * pNewNode = m_alloc.Allocate<ListNode<T,A> >();
    Assume(pNewNode,L"Allocation failed in List<T,A>::AddLast");
    ListNode<T,A> * ret = NULL;
    if (pNewNode)
    {
        ret = m_pFakeTail;
        m_pFakeTail->m_pNext = pNewNode;
        m_pFakeTail->Data() = data;
        m_pFakeTail = pNewNode;
        ++m_Count;
    }
    return ret;

}

//====================================================================
//
//====================================================================
template <class T,class A>
ListNode<T,A> * List<T,A>::AddAfter(ListNode<T,A> * pNode, const T & data)
{
    Assume(pNode,L"List<T,A>::AddAfter was called with a null pNode value");
    if (pNode)
    {
        ListNode<T,A> * pNewNode = m_alloc.Allocate<ListNode<T,A> >();
        Assume(pNewNode,L"Allocation failed in List<T,A>::AddAfter");
        if (pNewNode)
        {
            pNewNode->Data() = data;
            pNewNode->m_pNext = pNode->m_pNext;
            pNode->m_pNext = pNewNode;
            ++m_Count;
        }
        return pNewNode;
    }
    else
    return NULL;
}


//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T,A> * List<T,A>::AddAfter(const T& after, const T &data)
{
    ListNode<T,A> *node = this->Find(after);
    Assume(node, L"List<T,A>::AddAfter could not locate the node after which to add");
    return AddAfter(node, data);
}

//====================================================================
//
//====================================================================
template <class T, class A>
ListNode<T,A> * List<T,A>::Find(const T& data) const
{
    ListNodeConstIterator<T,A> iter(this);
    while ( iter.MoveNext() )
    {
        if ( iter.Current()->Data() == data )
        {
            return iter.Current();
        }
    }

    return NULL;
}

//====================================================================
//
//====================================================================
template <class T, class A>
void List<T,A>::InefficentRemove(ListNode<T,A> * pNode)
{
    Assume(pNode,L"List<T,A>::Remove was called with a null node");

    ListNodeIterator<T,A> iter(this);
    while (iter.MoveNext())
    {
        if (iter.Current() == pNode)
        {
            iter.Remove();
            break;
        }

    }
}

//====================================================================
//
//====================================================================
template <class T, class A>
void List<T,A>::InefficentRemove(const T & data)
{
    ListValueIterator<T,A> iter(this);
    while (iter.MoveNext() )
    {
        if ( iter.Current() == data )
        {
            iter.Remove();
            break;
        }
    }
}

//====================================================================
//
//====================================================================
template <class T, class A>
unsigned long List<T,A>::Count() const
{
    return m_Count;
}

//====================================================================
//
//====================================================================
template <class K, class V>
KeyValuePair<K,V>::KeyValuePair() : m_key(), m_value()
{
}

//====================================================================
//
//====================================================================
template <class K, class V>
KeyValuePair<K,V>::KeyValuePair(const K & key, const V & value) :
    m_key(key),
    m_value(value)
{
}

//====================================================================
//
//====================================================================
template <class K, class V>
K & KeyValuePair<K,V>::Key()
{
    return m_key;
}

//====================================================================
//
//====================================================================
template <class K, class V>
const K & KeyValuePair<K,V>::Key() const
{
    return m_key;
}

//====================================================================
//
//====================================================================
template <class K, class V>
V & KeyValuePair<K,V>::Value()
{
    return m_value;
}

//====================================================================
//
//====================================================================
template <class K, class V>
const V & KeyValuePair<K,V>::Value() const
{
    return m_value;
}

//====================================================================
//
//====================================================================
template <class K,class V,class A,class H>
HashTableIterator<K,V,A,H>::HashTableIterator(DynamicHashTable<K,V,A,H> * pHashTable) :
    m_tableIndex(0),
    m_fBeforeStart(true),
    m_pHashTable(pHashTable),
    m_iter(NULL)
{
    Assume(pHashTable,L"An attempt was made to construct an iterator with a null hash table.");
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableIterator<K,V,A,H>::HashTableIterator(const HashTableIterator<K,V,A,H> & src) :
    m_fBeforeStart(src.m_fBeforeStart),
    m_tableIndex(src.m_tableIndex),
    m_pHashTable(src.m_pHashTable),
    m_iter(src.m_iter)
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableIterator<K,V,A,H>::~HashTableIterator()
{
    if (m_iter)
    {
        m_iter->Destroy();
        m_iter = NULL;
    }
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool HashTableIterator<K,V,A,H>::MoveNext()
{
    while (true)
    {
        if (m_iter)
        {
            if (m_iter->MoveNext())
            {
                return true;
            }
            else
            {
                m_iter->Destroy();
                m_iter = NULL;
            }
        }
        GetNextBucket();
        if (! m_iter)
            return false;
    }
}

template <class K,class V,class A, class H>
void HashTableIterator<K,V,A,H>::Remove()
{
    if (m_iter)
    {
        m_iter->Remove();
        m_pHashTable->m_count--;
    }
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
KeyValuePair<K,V> HashTableIterator<K,V,A,H>::Current()
{
    Assume(m_iter,L"An attempt was made to call current on an interator in an invalid state. Perhaps a call is missing to MoveNext() or the sequence has been exausted.");
    if (m_iter)
    {
        return m_iter->Current()->Data();
    }
    else
        return KeyValuePair<K,V>();
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
void HashTableIterator<K,V,A,H>::GetNextBucket()
{
    if (m_fBeforeStart)
    {
        m_fBeforeStart = false;
    }
    else
    {
        ++m_tableIndex;
    }

    while (m_pHashTable && m_tableIndex < m_pHashTable->m_capacity && ! m_iter)
    {
        if (m_pHashTable->m_rgListKVP[m_tableIndex])
        {
            m_iter = m_pHashTable->m_alloc.Allocate<AllocDestroyableIterator<ListNode<KeyValuePair<K,V>, A> *, ListNodeIterator<KeyValuePair<K,V>, A>, A> >
            (
                m_pHashTable->m_alloc,
                m_pHashTable->m_rgListKVP[m_tableIndex]
            );
            Assume(m_iter,L"Allocation failed in HashTableIterator<K,V,A,H>::GetNextBucket()");
        }
        else
            ++m_tableIndex;
    }
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableKeyIterator<K,V,A,H>::~HashTableKeyIterator()
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableKeyIterator<K,V,A,H>::HashTableKeyIterator(DynamicHashTable<K,V,A,H> * pHashTable) :
    m_iter(HashTableIterator<K,V,A,H>(pHashTable))
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableKeyIterator<K,V,A,H>::HashTableKeyIterator(const HashTableKeyIterator<K,V,A,H> & src) :
    m_iter(src.m_iter)
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool HashTableKeyIterator<K,V,A,H>::MoveNext()
{
    return m_iter.MoveNext();
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
K HashTableKeyIterator<K,V,A,H>::Current()
{
    return m_iter.Current().Key();
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
void HashTableKeyIterator<K,V,A,H>::Remove()
{
    return m_iter.Remove();
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableValueIterator<K,V,A,H>::~HashTableValueIterator()
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableValueIterator<K,V,A,H>::HashTableValueIterator(DynamicHashTable<K,V,A,H> * pHashTable) :
    m_iter(HashTableIterator<K,V,A,H>(pHashTable))
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
HashTableValueIterator<K,V,A,H>::HashTableValueIterator(const HashTableValueIterator<K,V,A,H> & src) :
    m_iter(src.m_iter)
{
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool HashTableValueIterator<K,V,A,H>::MoveNext()
{
    return m_iter.MoveNext();
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
V HashTableValueIterator<K,V,A,H>::Current()
{
    return m_iter.Current().Value();
}

template <class K,class V,class A, class H>
void HashTableValueIterator<K,V,A,H>::Remove()
{
    m_iter.Remove();
}

template <class T, class A>
ArrayListIterator<T, A>::ArrayListIterator(ArrayList<T,A>* pList) :
    m_pArrayList(pList),
    m_index(0),
    m_fBeforeStart(true)
{
    Assume(pList,L"An attempt was made to construct an iterator with a null array list.");
}

template <class T, class A>
ArrayListIterator<T, A>::ArrayListIterator(const ArrayListIterator<T, A> & src) :
    m_pArrayList(src.m_pArrayList),
    m_index(src.m_index),
    m_fBeforeStart(src.m_fBeforeStart)
{
}

template <class T, class A>
ArrayListIterator<T, A>::~ArrayListIterator()
{
}

template <class T, class A>
bool ArrayListIterator<T, A>::MoveNext()
{
    if (m_fBeforeStart)
    {
        m_fBeforeStart = false;
    }
    else
    {
        m_index += 1;
    }

    if (m_index < m_pArrayList->m_count)
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

template <class T, class A>
T ArrayListIterator<T, A>::Current()
{
    Assume (m_index < m_pArrayList->m_count, L"Attempt to access invalid array list node. Did you call MoveNext, or is the iterator exhausted?");
    if (m_index < m_pArrayList->m_count)
    {
        return (*m_pArrayList)[ m_index ];
    }
    else
    {
        return T();
    }
}

template <class T, class A>
void ArrayListIterator<T, A>::Remove()
{
    Assume(false, L"This method isn't actually implemented");
}

template <class T, class A>
ArrayListReverseIterator<T,A>::ArrayListReverseIterator(ArrayList<T,A> *pArrayList) :
    m_pArrayList(pArrayList),
    m_index(pArrayList ? pArrayList->Count() : 0 ),
    m_beforeStart(false),
    m_fRemove(false)
{
    Assume(pArrayList, L"Passed an invalid array list");
}

template <class T, class A>
ArrayListReverseIterator<T,A>::ArrayListReverseIterator(const ArrayListReverseIterator<T,A> &src)
:   m_pArrayList(src.m_pArrayList),
    m_index(src.m_index),
    m_beforeStart(src.m_beforeStart),
    m_fRemove(src.m_fRemove)
{

}

template <class T, class A>
bool ArrayListReverseIterator<T,A>::MoveNext()
{
    if ( m_beforeStart )
    {
        return false;
    }

    if ( m_index )
    {
        --m_index;
        m_fRemove = false;
        return true;
    }
    else
    {
        m_beforeStart = true;
        return false;
    }
}

template <class T, class A>
T ArrayListReverseIterator<T,A>::Current()
{
    Assume(!m_beforeStart && m_index < m_pArrayList->Count() && !m_fRemove, L"Current called in an invalid state");
    if ( m_beforeStart || m_index >= m_pArrayList->Count() || m_fRemove)
    {
        return T();
    }

    return (*m_pArrayList)[m_index];
}

template <class T, class A>
void ArrayListReverseIterator<T,A>::Remove()
{
    Assume(!m_fRemove, L"Remove called twice");
    Assume(m_index < m_pArrayList->Count(), L"Remove called in an invalid state");

    m_fRemove = true;
    m_pArrayList->Remove(m_index);
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
DynamicHashTable<K,V,A,H>::~DynamicHashTable()
{
    Clear();

    // Free the individual lists
    if ( m_rgListKVP )
    {
        for ( unsigned long i = 0;  i < m_capacity; ++i )
        {
            if ( m_rgListKVP[i] )
            {
                m_alloc.DeAllocate(m_rgListKVP[i]);
            }
        }

        m_alloc.DeAllocateArray(m_rgListKVP);
    }

    m_rgListKVP = NULL;
    m_capacity = 0;
    m_count = 0;
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
DynamicHashTable<K,V,A,H>::DynamicHashTable
(
    const A & alloc,
    const H & hashFunc,
    unsigned long capacity,
    unsigned long count,
    List<KeyValuePair<K,V>, A> ** rgListKVP
) :
    m_capacity(capacity),
    m_count(count),
    m_rgListKVP(rgListKVP),
    m_hashFunc(hashFunc),
    m_alloc(alloc)
{
    TemplateUtil::CompileAssertSizeIsPointerMultiple<K>();
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
DynamicHashTable<K,V,A,H>::DynamicHashTable
(
    const A & alloc,
    const H & hashFunc,
    const unsigned long capacity
) :
    m_capacity(0),
    m_count(0),
    m_rgListKVP(NULL),
    m_hashFunc(hashFunc),
    m_alloc(alloc)
{
    TemplateUtil::CompileAssertSizeIsPointerMultiple<K>();
    Init(capacity);
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool DynamicHashTable<K,V,A,H>::Init(unsigned long capacity)
{
    m_count = 0;
    Assume(capacity, L"An attempt was made to initialize a DynamicHashtable<K,V,A,H> with capacity zero.");
    m_rgListKVP = m_alloc.AllocateArray<List<KeyValuePair<K,V>, A > * >(capacity);
    Assume(m_rgListKVP,L"Allocation failed in DynamicHashTable<K,V,A,H>::Init.");
    if (m_rgListKVP)
    {
        m_capacity = capacity;
    }
    return m_rgListKVP != NULL;
}


//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
void DynamicHashTable<K,V,A,H>::SetValue(const K &key, const V &value)
{
    Assume(m_rgListKVP,L"SetValue was called on a DynamicHashTable<K,V,A,H> that is in an invalid state. The underlying array should never be null.");
    if (m_rgListKVP)
    {
        const unsigned long idx = Bucket(key);
        List<KeyValuePair<K,V >, A> * pBucket = m_rgListKVP[idx];
        if (pBucket)
        {
            ListNodeIterator<KeyValuePair<K,V>, A > iter(pBucket);

            while (iter.MoveNext())
            {
                if (iter.Current()->Data().Key() == key)
                {
                    iter.Current()->Data().Value() = value;
                    return;
                }
            }
            if (m_count < m_capacity)
            {
                pBucket->AddFirst(KeyValuePair<K,V>(key, value));
                ++m_count;
            }
            else
            {
                Resize();
                SetValue(key, value);
            }
        }
        else
        {
            if (m_count < m_capacity)
            {
                m_rgListKVP[idx] = pBucket = m_alloc.Allocate<List<KeyValuePair<K,V>, A > >(m_alloc);
                Assume(pBucket,L"Allocation failed in DynamicHashTable<K,V,A,H>::SetValue");
                if (pBucket)
                {
                    pBucket->AddFirst(KeyValuePair<K,V>(key, value));
                    ++m_count;
                }
            }
            else
            {
                Resize();
                SetValue(key, value);
            }
        }
    }
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool DynamicHashTable<K,V,A,H>::Contains(const K &key) const
{
    return GetValue(key, NULL);
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
unsigned long DynamicHashTable<K,V,A,H>::Count() const
{
    return m_count;
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool DynamicHashTable<K,V,A,H>::GetValue(const K &key, _Out_opt_ V *pOutValue) const
{
    V temp;
    if ( GetValueInternal(key, temp) )
    {
        if ( pOutValue )
        {
            *pOutValue = temp;
        }
        return true;
    }
    return false;
}

//====================================================================
// Operates the same as bool GetValue(const K, V*) except this will
// throw if the key is not present in the dictionary
//====================================================================
template <class K,class V,class A, class H>
V DynamicHashTable<K,V,A,H>::GetValue(const K &key) const
{
    V temp;
    if ( !GetValueInternal(key, temp) )
    {
        Assume(false, L"Key not present in the DynamicHashTable<K,V,A,H>");
    }

    return temp;
}

//====================================================================
// 
// Gets the value from the hash table.  If no value for the key is found
// then it will return the default value as specified
//
//====================================================================
template <class K,class V,class A, class H>
V 
DynamicHashTable<K,V,A,H>::GetValueOrDefault(const K &key, V defaultValue) const
{
    V value;
    if ( GetValue(key, &value) )
    {
        return value;
    }

    return defaultValue;
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool DynamicHashTable<K,V,A,H>::GetValueInternal(const K & key, _Out_ V & outValue) const
{
    Assume(m_rgListKVP,L"GetValue was called on a DynamicHashTable<K,V,A,H> that is in an invalid state. The underlying array should never be null.");
    bool ret = false;
    if (m_rgListKVP)
    {
        unsigned long idx = Bucket(key);
        List<KeyValuePair<K,V>, A > * pBucket = m_rgListKVP[idx];
        if (pBucket)
        {
            ListNodeIterator<KeyValuePair<K, V>, A > iter(pBucket);
            while (iter.MoveNext())
            {
                if (m_hashFunc.AreEqual(iter.Current()->Data().Key(), key))
                {
                    ret = true;
                    outValue = iter.Current()->Data().Value();
                    break;
                }
            }
        }
    }
    return ret;
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
bool DynamicHashTable<K,V,A,H>::Remove(const K & key)
{
    Assume(m_rgListKVP,L"Remove was called on a DynamicHashTable<K,V,A,H> that is in an invalid state. The underlying array should never be null.");
    bool ret = false;
    if (m_rgListKVP)
    {
        unsigned long idx = Bucket(key);
        List<KeyValuePair<K,V>, A > * pBucket = m_rgListKVP[idx];
        if (pBucket)
        {
            ListNodeIterator<KeyValuePair<K,V>, A > iter(pBucket);
            while (iter.MoveNext())
            {
                if (iter.Current()->Data().Key() == key)
                {
                    ret = true;
                    iter.Remove();
                    --m_count;
                    break;
                }
            }
        }
    }
    return ret;
}

template <class K,class V,class A, class H>
void DynamicHashTable<K,V,A,H>::Clear()
{
    if (m_rgListKVP)
    {
        HashTableIterator<K,V,A,H> it(this);
        while ( it.MoveNext() )
        {
            it.Remove();
        }
        Assume(0 == m_count, L"Clearing a HashTable did not reset the count to 0");
    }
}

//====================================================================
//
//====================================================================
template <class K,class V,class A, class H>
void DynamicHashTable<K,V,A,H>::Resize()
{
    unsigned long oldCapacity = m_capacity;
    unsigned long oldCount = m_count;
    List<KeyValuePair<K,V>, A > ** oldArray = m_rgListKVP;
    if (Init(oldCapacity * 2))
    {
        DynamicHashTable<K,V,A,H> oldData(m_alloc, m_hashFunc, oldCapacity, oldCount, oldArray);
        HashTableIterator<K,V,A,H> iter(&oldData);
        KeyValuePair<K,V> current;
        while (iter.MoveNext())
        {
            current = iter.Current();
            SetValue(current.Key(), current.Value());
        }
        Assume(m_count == oldCount,L"Count mismatch detected when resizing a dynamic hash table");

        //the destructor for oldData will cause it to destroy the old table
    }

}

template <class K,class V,class A, class H>
unsigned long DynamicHashTable<K,V,A,H>::Bucket(const K & value) const
{
    return m_hashFunc.GetHashCode(value) % m_capacity;
}

//====================================================================
//
//====================================================================
template <class T, class A, class H>
HashSetIterator<T,A,H>::HashSetIterator(HashSet<T,A,H> * pHashSet) : HashTableKeyIterator<T,bool,A, H>((DynamicHashTable<T, bool,A,H>*)pHashSet)
{
}

//====================================================================
//
//====================================================================
template <class T, class A, class H>
HashSet<T,A,H>::HashSet
(
    const A & alloc,
    const H & hashFunc,
    const unsigned long capacity
) :
    DynamicHashTable <T, bool, A, H>(alloc, hashFunc, capacity)
{
}

//====================================================================
//
//====================================================================
template <class T, class A, class H>
HashSet<T,A,H>::HashSet() :
    DynamicHashTable<T,bool, A, H>(A())
{
}

//====================================================================
//
//====================================================================
template <class T, class A, class H>
bool HashSet<T,A,H>::Contains(const T &value) const
{
    return DynamicHashTable<T, bool, A, H>::Contains(value);
}

//====================================================================
//
//====================================================================
template <class T, class A, class H>
void HashSet<T,A,H>::Add(const T & value)
{
    DynamicHashTable<T, bool, A, H>::SetValue(value, true);
}

//====================================================================
//
//====================================================================
template <class T, class A, class H>
bool HashSet<T,A,H>::Remove(const T &value)
{
    return DynamicHashTable<T,bool, A, H>::Remove(value);
}

template <class T, class A, class H>
void HashSet<T,A,H>::Clear()
{
    HashSetIterator<T,A,H> iter(this);

    while (iter.MoveNext())
    {
        iter.Remove();
    }
}

template <class T, class A, class H>
DestroyableIterator<T> * HashSet<T, A, H>::GetIterator()
{
    return m_alloc.Allocate<AllocDestroyableIterator<T, HashSetIterator<T, A, H>, A> >(m_alloc, this);
}

template <class T, class A, class H>
unsigned long HashSet<T,A,H>::Count() const
{
    return DynamicHashTable<T,bool, A, H>::Count();
}

//====================================================================
// This hash method was stolen from our string pool and works
// really good for strings.  We need to verify that it's ok
// for binary ---- as well.
//====================================================================
unsigned long GetHashCode(const void * pKey, unsigned long size);

template <class K>
unsigned long DefaultHashFunction<K>::GetHashCode(const K & value) const
{
    return ::GetHashCode(&value, sizeof(K));
}

template <class K>
bool DefaultHashFunction<K>::AreEqual(const K & val1, const K & val2) const
{
    return (val1 == val2);
}

template <class T>
int LessThanComparer<T>::Compare(const T& item1, const T&item2) const
{
    if (item1 < item2)
    {
        return -1;
    }
    else if (item2 < item1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

template <class T, class A>
ArrayList<T,A>::ArrayList
(
    const A & alloc,
    const unsigned long capacity
) :
    m_alloc(alloc),
    m_capacity(0),
    m_pData(NULL),
    m_count(0)
{
    Reserve(capacity);
}

template <class T, class A>
ArrayList<T,A>::ArrayList
() :
    m_alloc(),
    m_capacity(0),
    m_pData(NULL),
    m_count(0)
{
    Reserve(default_initial_capacity);
}


template <class T, class A>
ArrayList<T,A>::ArrayList(const ArrayList & src) :
    m_alloc(src.m_alloc),
    m_capacity(0),
    m_pData(NULL),
    m_count(0)
{
    Reserve(src.Count());
    for (unsigned long i = 0; i < src.Count(); ++i)
    {
        Add(src[i]);
    }
}

template <class T, class A>
ArrayList<T,A>::~ArrayList()
{
    if (m_pData)
    {
        m_alloc.DeAllocateArray<T>(m_pData);
        m_pData = NULL;
        m_capacity = 0;
        m_count = 0;
    }
}

template <class T, class A>
void ArrayList<T,A>::Add(const T & item)
{
    if (m_count == m_capacity)
    {
        Reserve((m_capacity==0 ? default_initial_capacity : m_capacity) << 1);
    }
    if (m_count < m_capacity)
    {
        m_pData[m_count] = item;
        ++m_count;
    }
    else
    {
        VSFAIL("ArrayList<T,A>::Add was unable to add an item due to allocation failure");
    }
}

template <class T, class A>
void ArrayList<T,A>::Fill(unsigned long count, const T& value)
{
    for ( unsigned long i = 0; i < count; ++i )
    {
        Add(value);
    }
}

template <class T, class A>
bool ArrayList<T,A>::Remove(const unsigned long index)
{
    if (index < m_count)
    {
        for (unsigned long i = index; i < m_count - 1; ++i)
        {
            m_pData[i] = m_pData[i+1];
        }
        --m_count;
        return true;
    }
    else
    {
        return false;
    }
}

template <class T, class A>
unsigned long ArrayList<T,A>::Count() const
{
    return m_count;
}

template <class T, class A>
const T & ArrayList<T,A>::operator[](const unsigned long index) const
{
    return (*const_cast<ArrayList<T,A> * >(this))[index];
}

template <class T, class A>
T & ArrayList<T,A>::operator[](const unsigned long index)
{
    Assume(index < m_count,L"The index is out of range. A dummy value will be returned.");

    if (index < m_count)
    {
        return m_pData[index];
    }
    else
    {
        return dummy_t_value;
    }
}

template <class T, class A>
unsigned long ArrayList<T,A>::LeftChild(const unsigned long index)
{
    return ((index + 1) *2) - 1;
}

template <class T, class A>
unsigned long ArrayList<T,A>::RightChild(const unsigned long index)
{
    return (index + 1) *2;
}


template <class T, class A>
void ArrayList<T,A>::Reserve(unsigned long count)
{
    if (m_capacity < count)
    {
        T * pNewData = m_alloc.AllocateArray<T>(count);
        Assume(pNewData,L"An allocation failure occured in ArrayList<T,A>::Reserve");

        if (pNewData)
        {
            for (unsigned long i = 0; i < m_count; ++i)
            {
                pNewData[i] = m_pData[i];
            }
            m_capacity = count;
            if (m_pData)
            {
                m_alloc.DeAllocateArray(m_pData);
            }
            m_pData = pNewData;

        }
    }
}

template <class T, class A>
void ArrayList<T,A>::Clear()
{
    m_count = 0;
}


template <class T, class A>
unsigned long ArrayList<T,A>::Capacity()
{
    return m_capacity;
}

template <class T, class A>
void ArrayList<T,A>::Swap(unsigned long i1, unsigned long i2)
{
    T temp = m_pData[i1];
    m_pData[i1] = m_pData[i2];
    m_pData[i2] = temp;
}


template <class A>
BitVector<A>::BitVector(const A & alloc, unsigned long numberOfBits) :
    m_ArrayList(alloc, ComputeArraySize(numberOfBits)),
    m_bitCount(0),
    m_setCount(0)
{
}

template <class A>
void BitVector<A>::AddBit(bool value)
{
    unsigned long totalCapacity = GetTotalBitCapacity();

    if (!totalCapacity || !(totalCapacity % m_ArrayList.Count()))
    {
        m_ArrayList.Add(0);
    }

    m_ArrayList[m_ArrayList.Count() - 1] |= (Map(value) << (m_bitCount % BITS_PER_ENTRY));
    ++m_bitCount;
    if (value)
    {
        ++m_setCount;
    }
}

template <class A>
void BitVector<A>::SetBit(unsigned long index, bool value)
{
    if (index >= m_bitCount)
    {
        unsigned long neededArraySize = ComputeArraySize(index + 1);

        while (neededArraySize > m_ArrayList.Count())
        {
            m_ArrayList.Add(0);
        }

        m_bitCount = (index +1);
    }

    if (BitValue(index) && !value)
    {
        --m_setCount;
    }
    else if (!BitValue(index) && value)
    {
        ++m_setCount;
    }

    unsigned long tmp = m_ArrayList[index / BITS_PER_ENTRY] & ~(1 << (index % BITS_PER_ENTRY));
    m_ArrayList[index / BITS_PER_ENTRY] = tmp | (Map(value) << (index % BITS_PER_ENTRY));

}

template <class A>
bool BitVector<A>::BitValue(unsigned long index) const
{
    if (index >= m_bitCount)
    {
        return false;
    }
    else
    {
        return m_ArrayList[index / BITS_PER_ENTRY] & (1 << (index % BITS_PER_ENTRY));
    }
}

template <class A>
void BitVector<A>::ClearAll()
{
    for (unsigned long i = 0; i < m_ArrayList.Count(); ++i)
    {
        m_ArrayList[i] = 0;
    }
    m_setCount = 0;
}

template <class A>
unsigned long BitVector<A>::BitCount() const
{
    return m_bitCount;
}

template <class A>
unsigned long BitVector<A>::SetCount() const
{
    return m_setCount;
}

template <class A>
inline unsigned long BitVector<A>::Map(bool value)
{
    return value ? 1 : 0;
}

template <class A>
inline unsigned long BitVector<A>::GetTotalBitCapacity()
{
    return m_ArrayList.Count() * BITS_PER_ENTRY;
}

template <class A>
unsigned long BitVector<A>::ComputeArraySize(unsigned long index)
{
    unsigned long ret = index / BITS_PER_ENTRY;

    if (index % BITS_PER_ENTRY)
    {
        ++ret;
    }
    return ret;
}

template <class T, class A>
Queue<T, A>::Queue(const A & alloc) :
    m_list(alloc)
{

}

template <class T, class A>
void Queue<T, A>::PushOrEnqueue(const T & item)
{
    m_list.AddLast(item);
}

template <class T, class A>
T Queue<T, A>::PopOrDequeue()
{
    ListNode<T,A>  * pNode = m_list.GetFirst();

    T value = pNode->Data();

    //pNode is the first item in the list, so it's ok to
    //call InefficentRemove (i.e. it won't be inefficent)
    m_list.InefficentRemove(pNode);

    return value;
}

template <class T, class A>
const T & Queue<T, A>::Peek() const
{
    Assume(m_list.Count() > 0, L"Queue is empty");
    return m_list.GetFirst()->Data();
}

template <class T, class A>
void Queue<T, A>::Clear()
{
    m_list.Clear();
}

template <class T, class A>
unsigned long Queue<T, A>::Count() const
{
    return m_list.Count();
}

//====================================================================
// ChainIterator
//====================================================================


template <class T>
ChainIterator<T>::ChainIterator(IConstIterator<T>* firstIterator, IConstIterator<T>* secondIterator) :
    IConstIterator(),
    m_pFirstIterator(firstIterator),
    m_pSecondIterator(secondIterator)
{
}

template <class T>
bool ChainIterator<T>::MoveNext()
{
    if (m_pFirstIterator)
    {
        if (m_pFirstIterator->MoveNext())
        {
            return true;
        }
        else
        {
            m_pFirstIterator = NULL;
            // Fall through to the second iterator in this case.
        }
    }
    if (m_pSecondIterator)
    {
        if (m_pSecondIterator->MoveNext())
        {
            return true;
        }
        else
        {
            m_pSecondIterator = NULL;
        }
    }

    return false;
}

template <class T>
T ChainIterator<T>::Current()
{
    if (m_pFirstIterator)
    {
        return m_pFirstIterator->Current();
    }
    else if (m_pSecondIterator)
    {
        return m_pSecondIterator->Current();
    }
    else
    {
        return NULL;
    }
}
