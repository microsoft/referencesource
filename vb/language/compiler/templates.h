//****************************************************************************
//
// Misc templates.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Information Contained Herein Is Proprietary and Confidential.
//
//****************************************************************************

#pragma once




#ifdef _WIN64
    // make it multiple of 8  for IA64
    #define _cbWordSize 8
#else
    #define _cbWordSize 4
#endif //Win64

#ifdef _WIN64
    // make it multiple of 8  for IA64
    #define _GetAlignedSize(cbSize) ((cbSize + 7) & ~7)
#else
    #define _GetAlignedSize(cbSize) ((cbSize + 3) & ~3)
#endif //Win64


class NorlsAllocator;
class Compiler;

//============================================================================
// Defines a unary predicate that always returns true.
//============================================================================
template <class T>
class AlwaysTrue
{
public:
    bool operator()(const T& item)
    {
        return true;
    }
};

//============================================================================
// Initializes a class with zeros.  Do not use this on a class that
// contains vtables.
// The current C++ compiler, the vft pointer is out of the memory afected by memset().
// More, the poiter is initialized after memset.
// For now it is ok to use virtual methods, but the issue can escaladate if V++ compiler
// changes
//============================================================================

template <class TYPE>
class ZeroInit
{
public:
    ZeroInit()
    {
        memset(this, 0, sizeof(TYPE));
    }
};

//============================================================================
// In FV_DEADBEEF builds only, Deadbeef overwrites the contents of the inheriting class
// with an identifiable bit-pattern (0xDEADBEEF) when the base class's DTOR
// is invoked.  This should help us quickly detect uses of stale objects.
//
// To have a class MyClass destroy its data members after its DTOR has been invoked,
// just declare it like this:
//
//      class MyClass :
//          public baseclass1,
//          public baseclass2
//      #ifdef FV_DEADBEEF
//          , public Deadbeef<MyClass> // Must be last base class!
//      #endif
//      {
//          [...body of MyClass goes here...]
//      }
// Note that the Deadbeef class *must* be the *last* base class of MyClass!  If
// it isn't, ~Deadbeef will overwrite the data members of the base classes
// that follow it, and all ---- will break loose.  You have been warned!
//============================================================================

// Enable Deadbeef only in debug builds.
#if DEBUG
#define FV_DEADBEEF 1
#else
#define FV_DEADBEEF 0
#endif

#if FV_DEADBEEF
template <class TYPE>
class Deadbeef
{
public:
    ~Deadbeef()
    {
        // Stamp the bitpattern "DEADBEEF" all over this class's data members.
        static const BYTE rgbSig[4] = { 0xEF, 0xBE, 0xAD, 0xDE };
        int irgbSig;        // Index into DEADBEEF sig

        // Cast to base type -- will do necessary pointer subtraction
        TYPE *pType = (TYPE*)this;

        // Calculate beginning and end of this class's data members
        BYTE *pbObj = ((BYTE*)pType) + offsetof(TYPE, Deadbeef<TYPE>::m_deadbeef);
        BYTE *pbObjLim = ((BYTE*)pType) + sizeof(TYPE);

        for (irgbSig = 0; pbObj < pbObjLim; pbObj++, irgbSig++)
        {
            if (irgbSig > 3) irgbSig = 0;
            *pbObj = rgbSig[irgbSig];
        }
    }

    //
    // The Deadbeef class is always the last base class of any class that
    // derives from it.  We can then use &m_deadbeef to determine where the
    // data members of the derived class start.  You can't just use
    // sizeof(TYPE) and start stomping, as that will destroy the data
    // members of any base classes, even though their DTORs have
    // yet to run.  When the base DTORs do run, kablooey!
    //
    // In other word, m_deadbeef forms a boundary between the derived class's
    // data members and the base classes' data members.  We can them stomp
    // on everything in from &m_deadbeef to the end of the derived class
    // without corrupting the base classes.
    //
    int m_deadbeef;
};
#endif  // FV_DEADBEEF

//============================================================================
// Defines a generic hash table that can hold anything using anything
// as a key.
//============================================================================

#pragma warning(disable : 4200)

template <int SIZE>
class HashTable
{
private:

    struct HashBase
    {
        size_t m_cbKeySize;
        HashBase *m_pNext;
        BYTE m_values[0];
    };

    public:
        NEW_CTOR_SAFE()

        HashTable() : m_pnra(NULL)
        {
            Clear();
        }

        HashTable(_In_ NorlsAllocator *pnra) : m_pnra(pnra)
        {
            Clear();
        }

        void Init(NorlsAllocator *pnra)
        {
            m_pnra = pnra;
            Clear();
        }

        void Clear()
        {
            m_pLastFound = NULL;
            memset(m_rghash, 0, sizeof(m_rghash));
            m_numEntries = 0;
        }

        // 




        template <class VALUETYPE>
            void *AddWithSize(_In_ void *pkey, size_t cbSize, _In_ VALUETYPE &value) // Microsoft: now allows duplicates, i.e. doesn't assert any longer if you try
        {
            cbSize = _GetAlignedSize(cbSize);

            IfFalseThrow(sizeof(HashBase) + cbSize >= cbSize && sizeof(HashBase) + cbSize + sizeof(VALUETYPE) >= sizeof(VALUETYPE));
            HashBase *phash = (HashBase *)m_pnra->Alloc(sizeof(HashBase) + cbSize + sizeof(VALUETYPE));
            unsigned i = Bucket(pkey, cbSize);

            memcpy(phash->m_values, pkey, cbSize);
            memcpy(phash->m_values + cbSize, &value, sizeof(value));

            phash->m_cbKeySize = cbSize;

#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            phash->m_pNext = m_rghash[i];

            m_rghash[i] = phash;
#pragma prefast(pop)

            ++m_numEntries;

            return (void *)(phash->m_values + phash->m_cbKeySize);
        }

        // 








        void *FindWithSize(_In_ void *pkey, size_t cbSize)
        {
            cbSize = _GetAlignedSize(cbSize);

#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *phash = m_rghash[Bucket(pkey, cbSize)];
#pragma prefast(pop)

            while (phash)
            {
                if (phash->m_cbKeySize == cbSize && !memcmp(phash->m_values, pkey, cbSize))
                {
                    break;
                }

                phash = phash->m_pNext;
            }
            if ( phash )
            {
                m_pLastFound = phash;
                return (void *)(phash->m_values + phash->m_cbKeySize);
            }
            return NULL;
        }

        template <class KEYTYPE, class VALUETYPE>
            void *Add(_In_ KEYTYPE &key, _In_ VALUETYPE &value)
        {
            return AddWithSize(&key, sizeof(KEYTYPE), value);
        }

        template <class KEYTYPE>
            void *Find(_In_ KEYTYPE &key)
        {
            return FindWithSize(&key, sizeof(KEYTYPE));
        }

        void *FindNext()
        {
            if ( m_pLastFound )
            {
                HashBase * pReturnMe = m_pLastFound->m_pNext;
                m_pLastFound = pReturnMe;
                return pReturnMe ? (void *)(pReturnMe->m_values + pReturnMe->m_cbKeySize) : NULL;
            }
            return NULL;
        }

        // NOTE: This function returns the next sibling in the bucket. IT MIGHT HAVE A
        //       DIFFERENT KEY than the value you started looking for with FindWithSize()
        //       Of course, this ok since within the bucket the values are not sorted. Be
        //       sure you check ALL siblings in the bucket to find the particular
        //       key you are searching for.
        template <class KEYTYPE>
            void *FindNextKey(KEYTYPE **ppKey)
        {
            if ( m_pLastFound )
            {
                HashBase * pReturnMe = m_pLastFound->m_pNext;
                m_pLastFound = pReturnMe;

                if ( pReturnMe )
                {
                    if( ppKey )
                        *ppKey = (KEYTYPE *)pReturnMe->m_values;

                    return( (void *)(pReturnMe->m_values + pReturnMe->m_cbKeySize) );
                }
                return NULL;
            }
            return NULL;
        }

        template <class KEYTYPE, class VALUETYPE>
            bool Remove(_In_ KEYTYPE &key, _In_ VALUETYPE &value)
        {
            return RemoveWithSize(&key, sizeof(KEYTYPE), value);
        }

        // Remove an entry from the table - doesn't delete anything; just gets the guy out of the hash table.
        bool RemoveWithSize(
            _In_ void *key, // the key for this item to speed the search
            size_t cbSize,
            _In_ void *value, // because we allow duplicates, we send the instance also so we delete the right one
            size_t cbSizeValue)
        {
            cbSize = _GetAlignedSize(cbSize);

            // Find the first entry in the table that has the same key
            unsigned bucket = Bucket((void*)key, cbSize);
            HashBase *pLastHash = NULL;

#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *pDelMe = m_rghash[ bucket ];
#pragma prefast(pop)
            while ( pDelMe )
            { // keys match?
                if (pDelMe->m_cbKeySize == cbSize && !memcmp(pDelMe->m_values, key, cbSize))
                {   // since there can be duplicates by this name, make sure we have the right guy before removing it...
                    if ( !memcmp( pDelMe->m_values + pDelMe->m_cbKeySize, (void *)value, cbSizeValue))
                    { // if we have a match then time to quick looking around for one
                        break;
                    }
                }
                pLastHash = pDelMe;
                pDelMe = pDelMe->m_pNext;
            }
            if ( !pDelMe )
                return FALSE; // coudn't find it

            // remove it by jumping the entry pointers around the removed entry
            if ( pLastHash ) // we are removing a guy that isn't at the head of the list
            {
                pLastHash->m_pNext = pDelMe->m_pNext;
            }
            else // removing a guy at the head of the list
            {
#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
                m_rghash[ bucket ] = pDelMe->m_pNext;
#pragma prefast(pop)
            }

            // If we are deleting the node that is currently pointed to by m_pLastFound
            // we reset the traversal pointers so that we start form the begining next time
            if (pDelMe == m_pLastFound)
            {
                ResetHashIterator();
            }

            --m_numEntries;
            return TRUE; // removed succesfully
        }

        template <class KEYTYPE, class VALUETYPE>
            bool RemoveWithSize(
            _In_ KEYTYPE *key, // the key for this item to speed the search
            unsigned cbSize,
            _In_ VALUETYPE &value // because we allow duplicates, we send the instance also so we delete the right one
            )
        {
            return RemoveWithSize(key, cbSize, &value, sizeof(value));
        }

        // return the number of elements stored in the hash
        inline unsigned NumEntries()
        {
            return m_numEntries;
        }

        // Reset the iterator to the first bucket in the table
        void ResetHashIterator( void )
        {
            m_pLastIteration = NULL;
            m_LastBucket = 0;
        }

        // Iterate through the hash table.  Different from FindNext() in that
        // it iterates through ALL the elements in the hash table.  FindNext()
        // just gets the next guy in the same bucket.
        void * IterateHash( void )
        {
            if ( m_pLastIteration )
            {
                m_pLastIteration = m_pLastIteration->m_pNext;
            }

            while (!m_pLastIteration)
            {
                if (m_LastBucket >= SIZE)
                {
                    break;
                }
                m_pLastIteration = m_rghash[ m_LastBucket++ ];
            }

            if (m_pLastIteration)
            {
                return (void *)(m_pLastIteration->m_values + m_pLastIteration->m_cbKeySize);
            }
            return NULL;
        }

        //clone a hash table
        void IterateHashKeyAndValue( void ** ppKey, size_t *pcbSize, void ** ppValue )
        {
            *ppKey = NULL;
            *pcbSize = 0;
            *ppValue = NULL;
            if ( m_pLastIteration )
            {
                m_pLastIteration = m_pLastIteration->m_pNext;
            }

            while (!m_pLastIteration)
            {
                if (m_LastBucket >= SIZE)
                {
                    break;
                }
                m_pLastIteration = m_rghash[ m_LastBucket++ ];
            }

            if (m_pLastIteration)
            {
                *ppKey = (void *)m_pLastIteration->m_values;
                *pcbSize = m_pLastIteration->m_cbKeySize;
                *ppValue = (void *)(m_pLastIteration->m_values + m_pLastIteration->m_cbKeySize);
            }
        }
private:

    // This hash method was stolen from our string pool and works
    // really good for strings.  We need to verify that it's ok
    // for binary ---- as well.
    //
    unsigned Bucket(_In_ void *pkey, size_t cbSize)
    {
        VSASSERT(cbSize % _cbWordSize == 0, "Must be sizeof void * thing.");
		
//	 COMPILE_ASSERT(cbSize % _cbWordSize == 0);

        unsigned long hash = 0;
        unsigned long l;
        unsigned long *pl = (unsigned long *)pkey;

        size_t cl = cbSize / _cbWordSize;

        for (; 0 < cl; -- cl, ++ pl)
        {
            l = *pl;

            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
        }

        return hash % SIZE;
    }

    unsigned m_LastBucket; // the last bucket examined for the hash iterator
    unsigned m_numEntries; // number of elements in the hash table
    NorlsAllocator *m_pnra;
    HashBase *m_pLastFound; // for use with FindNext()
    HashBase *m_pLastIteration; // for use with IterateHash()
    HashBase *m_rghash[SIZE];
};

// Some integrigty check for nodes + a flag for empty or full node
#if DEBUG
#define HashBaseEmpty 0xabababa0
#define HashBaseFull 0xabababa1
#endif

//============================================================================
// This is similar to the HashTable class defined above with the exception
// that it only works for a fixed size keys and values. This allows us to do
// nifty deletions from the hash table by keeping a linked list of deleted
// nodes and then reusing them later.
//============================================================================

template <int SIZE, class KEY_TYPE, class VALUE_TYPE>
class FixedSizeHashTable
{
private:
    struct HashBase
    {
#if DEBUG
        unsigned m_flags;       // Some debug flags
#endif

        HashBase *m_pNext;      // Linked list stuff
        KEY_TYPE m_key;         // We store the key itself
        VALUE_TYPE m_value;     // And the value as well
    };

    struct HashBucket
    {
        HashBase *m_pFirstBase; // Points at the head of the list of nodes
        HashBase *m_pLastBase;  // Points at the tail of the list of nodes
    };

    public:

#if DEBUG
        void DumpHashTableStats()
        {
            VALUE_TYPE *pElement = NULL;
            int NumberOfUsedBuckets = 0;
            int MaxBucketSize = 0;

            for (unsigned i = 0; i < SIZE; ++i)
            {
                pElement = GetFirstInBucket(i);

                if (pElement)
                {
                    ++NumberOfUsedBuckets;
                }

                int j = 0;

                while (pElement)
                {
                    ++j;
                    pElement = FindNext();
                }

                if (j > MaxBucketSize)
                {
                    MaxBucketSize = j;
                }
            }

            DebPrintf("\nNumber of elements       = %d", m_numEntries);
            DebPrintf("\nNumber of buckets        = %d", SIZE);
            DebPrintf("\nNumber of used buckets   = %d", NumberOfUsedBuckets);
            DebPrintf("\nHashTable utilization    = %.1lf%%", ((float)NumberOfUsedBuckets / SIZE) * 100);
            DebPrintf("\nMaximum linked list size = %d", MaxBucketSize);
            DebPrintf("\nAve. linked list size    = %.1lf\n", NumberOfUsedBuckets > 0 ? (m_numEntries / (double)NumberOfUsedBuckets) : 0);
        }
#endif

        FixedSizeHashTable() : m_pnra(NULL)
        {
            TemplateUtil::CompileAssertSizeIsPointerMultiple<KEY_TYPE>();
            Clear();
        }

        FixedSizeHashTable(_In_ NorlsAllocator *pnra)
        {
            TemplateUtil::CompileAssertSizeIsPointerMultiple<KEY_TYPE>();
            Init(pnra);
        }

        void Init(_In_ NorlsAllocator *pnra)
        {
            m_pnra = pnra;
            Clear();
        }

        void Clear()
        {
            memset(m_rghash, 0, sizeof(m_rghash));
            m_numEntries = 0;
            m_LastBucket = 0;
            m_pLastFound = NULL;
            m_pLastIteration = NULL;
            m_pfreeList = NULL;
        }

        NorlsAllocator *GetAllocator()
        {
            return m_pnra;
        }

        // Adds a new node to the hash table. We copy both the value and the key so that we can find it later.
        // NOTE: This function adds the new element to the end of the list of nodes, this way, if there are
        // several nodes with the same key, the newest node to be inserted is the last one to be found.
        // Don't change this to add to the begining of the list unless you analyze how clients will be affected
        // See VS213488
        void HashAdd(const KEY_TYPE &key, _In_ VALUE_TYPE &value)
        {
            unsigned i = Bucket(&key, sizeof(KEY_TYPE));     // Get a bucket
            HashBase *phash = m_pfreeList;

            if (phash == NULL)          // Free list is empty, get some space from NRA
            {
                phash = (HashBase *)m_pnra->Alloc(sizeof(HashBase));
            }
            else                        // Get first one off the free list
            {
                VSASSERT(IsClear(phash), "A hashtable node was put on the free list, but appeares to be not free!");
                m_pfreeList = m_pfreeList->m_pNext;
            }

#if DEBUG
            phash->m_flags = HashBaseFull;
#endif

            memcpy(&phash->m_key, &key, sizeof(KEY_TYPE));          // Copy the key
            memcpy(&phash->m_value, &value, sizeof(VALUE_TYPE));    // Copy the value
            phash->m_pNext = NULL;

#pragma prefast(push)
#pragma prefast(disable: 26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            if (m_rghash[i].m_pLastBase)
            {
                // Case 1: this bucket is not empty, adjust the last element pointer
                m_rghash[i].m_pLastBase->m_pNext = phash;
                m_rghash[i].m_pLastBase = phash;
            }
            else
            {
                m_rghash[i].m_pFirstBase = m_rghash[i].m_pLastBase = phash;
            }
#pragma prefast(pop)

            ++m_numEntries;
        }

        // The way this HashFind function works is the following: If only key is given, we use this
        // to find the first item with this key. If both key and value are given, we first use the
        // key to find the right bucket, then we use the value to match the values. This allows us
        // to have duplicates in the hash table.
        // To use this function, if you know that there are duplicates in the hash table, then you can
        // just use the first parameter, but if there is a possibility of duplicates, then you will
        // need to use both parameters
        VALUE_TYPE *HashFind(const KEY_TYPE &key, _In_opt_ VALUE_TYPE *pValue = NULL)
        {
            unsigned i = Bucket(&key, sizeof(KEY_TYPE));

#pragma prefast(push)
#pragma prefast(disable: 26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *phash = m_rghash[i].m_pFirstBase;
#pragma prefast(pop)

            // Look in linked list, stop when we run out of keys or we find the right key
            for ( ; phash ; phash = phash->m_pNext)
            {
                VSASSERT(Isfull(phash), "Trying to access a node that looks damaged!");

                // We first check keys
                if (!memcmp(&phash->m_key, &key, sizeof(KEY_TYPE)))
                {
                    // Found the right key. Do we need to check values as well?
                    if (pValue)
                    {
                        if (!memcmp(&phash->m_value, pValue, sizeof(VALUE_TYPE)))
                        {
                            break;  // Key match, value match
                        }
                    }
                    else
                    {
                        break;      // Key match, no value
                    }
                }
            }

            if (phash)
            {
                m_pLastFound = phash;
                return &(phash->m_value);
            }

            return NULL;
        }

        // Gets the next guy in the linked list of nodes. Doesn't move on to the next bucket.
        // 
        // Note: In general this should be const because we have no intention of modifying the 
        // key.  
        //
        // Additionally though this solves a 32/64 bit problem.  Our bucket limitation requires
        // that the size of the key be a pointer multiple.  However many of the types we use are
        // platform independent and stuck at 4 bytes.  See mdToken,unsigned,DWORD, etc ...  It's
        // easy enough to use size_t as the key as we rarely use the KEY_TYPE for anything other
        // than lookup.  However without const the following code will not compile on 64 bit
        //
        // DynamicFixedSizeHashTable<42,size_t,Value> map;
        // unsigned key = 5;
        // map.FindNextWithKey(5);
        //
        // The reason being you cannot take a ref& of a value when the source and dest have different
        // sizes.  Normally the compiler would create a temporary and widen key to a size_t but it's 
        // illegal to take a reference to a temporary.  
        //
        // Taking a const ref to a temporary though is legal and part of the standard.  Hence switching
        // this to const will make the above code compile.  
        //
        // Also, this should just be const regardless of the above problem.  
        VALUE_TYPE *FindNextWithKey(_In_ const KEY_TYPE &key, _In_opt_ VALUE_TYPE *pValue = NULL)
        {
            if (m_pLastFound)
            {
                for (HashBase *pReturnMe = m_pLastFound->m_pNext; pReturnMe; pReturnMe = pReturnMe->m_pNext)
                {
                    // We first check keys
                    if (!memcmp(&pReturnMe->m_key, &key, sizeof(KEY_TYPE)))
                    {
                        // Found the right key. Do we need to check values as well?
                        if (pValue)
                        {
                            if (!memcmp(&pReturnMe->m_value, pValue, sizeof(VALUE_TYPE)))
                            {
                                m_pLastFound = pReturnMe;
                                return &(pReturnMe->m_value);   // Key match, value match
                            }
                        }
                        else
                        {
                            m_pLastFound = pReturnMe;
                            return &(pReturnMe->m_value);   // Key match, value match
                        }
                    }
                }
            }
            m_pLastFound = NULL;
            return NULL;
        }

        VALUE_TYPE *FindNext()
        {
            if (m_pLastFound)
            {
                HashBase *pReturnMe = m_pLastFound->m_pNext;
                m_pLastFound = pReturnMe;

                if (pReturnMe)
                {
                    return &(pReturnMe->m_value);
                }
            }

            return NULL;
        }

#if DEBUG
        // Gets the next guy in the linked list of nodes. Doesn't move on to the next bucket.
        VALUE_TYPE *GetFirstInBucket(unsigned bucket)
        {
            if (bucket >= SIZE)
            {
                return NULL;        // Bucket overflow
            }

#pragma prefast(suppress: 26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *phash = m_rghash[bucket].m_pFirstBase;

            if (phash)
            {
                m_pLastFound = phash;
                return &(phash->m_value);
            }

            return NULL;
        }
#endif DEBUG

        // The way this HashRemove function works is the following: If only key is given, we use this
        // to find the first item with this key and wackit. If both key and value are given, we first
        // use the key to find the right bucket, then we use the value to match the values. This allows
        // us to have duplicates in the hash table.
        // To use this function, if you know that there are duplicates in the hash table, then you can
        // just use the first parameter, but if there is a possibility of duplicates, then you will
        // need to use both parameters
        VALUE_TYPE *HashRemove(const KEY_TYPE &key, _In_opt_ VALUE_TYPE *pValue = NULL)
        {
            unsigned i = Bucket(&key, sizeof(KEY_TYPE));
#pragma prefast(push)
#pragma prefast(disable: 26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *phash     = m_rghash[i].m_pFirstBase;
            HashBase *phashPrev = m_rghash[i].m_pFirstBase;
#pragma prefast(pop)

            // Look in linked list, stop when we run out of keys or we find the right key
            for ( ; phash ; phash = phash->m_pNext)
            {
                VSASSERT(Isfull(phash), "Trying to access a node that looks damaged!");

                // We first check keys
                if (!memcmp(&phash->m_key, &key, sizeof(KEY_TYPE)))
                {
                    // Found the right key. Do we need to check values as well?
                    if (pValue)
                    {
                        if (!memcmp(&phash->m_value, pValue, sizeof(VALUE_TYPE)))
                        {
                            break;  // Key match, value match
                        }
                    }
                    else
                    {
                        break;      // Key match, no value
                    }
                }

                phashPrev = phash;
            }

            if (phash)
            {
                if (phashPrev == phash)
                {
                    // phash is the first node in the list
#pragma prefast(push)
#pragma prefast(disable: 26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
                    if (m_rghash[i].m_pFirstBase == m_rghash[i].m_pLastBase)
                    {
                        // There is only one node in this bucket
                        VSASSERT(!phash->m_pNext, "Node should be the only one in the list");
                        m_rghash[i].m_pLastBase = NULL;
                    }

                    m_rghash[i].m_pFirstBase = phash->m_pNext;
                }
                else
                {
                    // phash is not the first node in the list
                    if (phash == m_rghash[i].m_pLastBase)
                    {
                        // phash is the last one in the list
                        m_rghash[i].m_pLastBase = phashPrev;
                    }

                    phashPrev->m_pNext = phash->m_pNext;
                }
#pragma prefast(pop)

                // If we are deleting the node that is currently pointed to by m_pLastFound
                // we reset the traversal pointers so that we start form the begining next time
                if (phash == m_pLastFound)
                {
                    ResetHashIterator();
                }

                phash->m_pNext = m_pfreeList;
                m_pfreeList = phash;

                --m_numEntries;

#if DEBUG
                phash->m_flags = HashBaseEmpty;
#endif

                return &(phash->m_value);   // found
            }

            return NULL;        // wasn't found
        }

        // Reset the iterator to the first bucket in the table
        void ResetHashIterator()
        {
            m_pLastIteration = NULL;
            m_LastBucket = 0;
        }

        // Iterate through the hash table.  Different from FindNext() in that
        // it iterates through ALL the elements in the hash table.  FindNext()
        // just gets the next guy in the same bucket.
        VALUE_TYPE *IterateHash()
        {
            if (m_pLastIteration)
            {
                m_pLastIteration = m_pLastIteration->m_pNext;
            }

            while (!m_pLastIteration)
            {
                if (m_LastBucket >= SIZE)
                {
                    break;
                }

                m_pLastIteration = m_rghash[m_LastBucket++].m_pFirstBase;
            }

            if (m_pLastIteration)
            {
                return &(m_pLastIteration->m_value);
            }

            return NULL;
        }

        //clone a hash table
        void IterateHashKeyAndValue(_Out_ KEY_TYPE ** ppKey,_Out_ VALUE_TYPE **ppValue)
        {
            *ppKey = NULL;
            *ppValue = NULL;
            if (m_pLastIteration)
            {
                m_pLastIteration = m_pLastIteration->m_pNext;
            }

            while (!m_pLastIteration)
            {
                if (m_LastBucket >= SIZE)
                {
                    break;
                }

                m_pLastIteration = m_rghash[m_LastBucket++].m_pFirstBase;
            }

            if (m_pLastIteration)
            {
                *ppKey = &m_pLastIteration->m_key;
                *ppValue = &m_pLastIteration->m_value;
            }
        }

        // return the number of elements stored in the hash
        inline unsigned NumEntries()
        {
            return m_numEntries;
        }

private:

#if DEBUG
    inline bool IsClear(HashBase *phash)
    {
        return ((phash->m_flags == 0x0) || (phash->m_flags == HashBaseEmpty));
    }


    inline bool Isfull(HashBase *phash)
    {
        return (phash->m_flags == HashBaseFull);
    }
#endif

    // This hash method was stolen from our string pool and works
    // really good for strings.  We need to verify that it's ok
    // for binary ---- as well.
    //
    unsigned Bucket(const void *pkey, unsigned cbSize)
    {
//        VSASSERT(cbSize % _cbWordSize == 0, "Must be sizeof void * thing.");
//	 COMPILE_ASSERT(cbSize % _cbWordSize == 0);

        unsigned long hash = 0;
        unsigned long l;
        unsigned long *pl = (unsigned long *)pkey;

        unsigned long cl = cbSize / _cbWordSize;

        for (; 0 < cl; -- cl, ++ pl)
        {
            l = *pl;

            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;
        }

        return hash % SIZE;
    }

    unsigned m_LastBucket;      // the last bucket examined for the hash iterator
    HashBase *m_pLastFound;     // for use with FindNext()
    HashBase *m_pLastIteration; // for use with IterateHash()

    unsigned m_numEntries;      // number of elements in the hash table
    NorlsAllocator *m_pnra;     // NRA to use for memory
    HashBucket m_rghash[SIZE];  // Array of buckets to nodes
    HashBase  *m_pfreeList;     // linked list of free nodes to recycle
};

//============================================================================
// This is the same as FixedSizeHashTable except that the hashtable size can be determined at
// construction time rather than fixed at declaration time.
//============================================================================

template <class KEY_TYPE, class VALUE_TYPE>
class DynamicFixedSizeHashTable
{
private:
    struct HashBase
    {
#if DEBUG
        unsigned m_flags;       // Some debug flags
#endif

        HashBase *m_pNext;      // Linked list stuff
        KEY_TYPE m_key;         // We store the key itself
        VALUE_TYPE m_value;     // And the value as well
    };

    struct HashBucket
    {
        HashBase *m_pFirstBase; // Points at the head of the list of nodes
        HashBase *m_pLastBase;  // Points at the tail of the list of nodes
    };

    public:

#if DEBUG
        void DumpHashTableStats()
        {
            VALUE_TYPE *pElement = NULL;
            int NumberOfUsedBuckets = 0;
            int MaxBucketSize = 0;

            for (unsigned i = 0; i < m_Size; ++i)
            {
                pElement = GetFirstInBucket(i);

                if (pElement)
                {
                    ++NumberOfUsedBuckets;
                }

                int j = 0;

                while (pElement)
                {
                    ++j;
                    pElement = FindNext();
                }

                if (j > MaxBucketSize)
                {
                    MaxBucketSize = j;
                }
            }

            DebPrintf("\nNumber of elements       = %d", m_numEntries);
            DebPrintf("\nNumber of buckets        = %d", m_Size);
            DebPrintf("\nNumber of used buckets   = %d", NumberOfUsedBuckets);
            DebPrintf("\nHashTable utilization    = %.1lf%%", ((float)NumberOfUsedBuckets / m_Size) * 100);
            DebPrintf("\nMaximum linked list size = %d", MaxBucketSize);
            DebPrintf("\nAve. linked list size    = %.1lf\n", NumberOfUsedBuckets > 0 ? (m_numEntries / (double)NumberOfUsedBuckets) : 0);
        }
#endif

        DynamicFixedSizeHashTable()
        {
            // Bucket algorithm requires word multiple size key
            TemplateUtil::CompileAssertSizeIsPointerMultiple<KEY_TYPE>();
            m_Size = 0;
            Clear();
        }

        DynamicFixedSizeHashTable(NorlsAllocator *pnra, unsigned Size)
        {
            // Bucket algorithm requires word multiple size key
            TemplateUtil::CompileAssertSizeIsPointerMultiple<KEY_TYPE>();
            Init(pnra, Size);
        }

        void Init(_Inout_ NorlsAllocator *pnra, unsigned Size)
        {
            IfFalseThrow(VBMath::TryMultiply(Size, sizeof(HashBucket)));
            IfFalseThrow(Size <= (size_t)-1 / sizeof(HashBucket));

            VSASSERT(m_rghash == NULL, "Can't double initialize!");
            m_pnra = pnra;
            m_Size = Size;
            Clear();

            m_rghash = (HashBucket *)pnra->Alloc(Size * sizeof(HashBucket));
        }

        void Clear()
        {
            if (m_rghash)
            {
                memset(m_rghash, 0, m_Size * sizeof(HashBucket));
                m_rghash = 0;
            }
            m_numEntries = 0;
            m_LastBucket = 0;
            m_pLastFound = NULL;
            m_pLastIteration = NULL;
            m_pfreeList = NULL;
        }

        NorlsAllocator *GetAllocator()
        {
            return m_pnra;
        }

        // Adds a new node to the hash table. We copy both the value and the key so that we can find it later.
        // NOTE: This function adds the new element to the end of the list of nodes, this way, if there are
        // several nodes with the same key, the newest node to be inserted is the last one to be found.
        // Don't change this to add to the begining of the list unless you analyze how clients will be affected
        // See VS213488
        void HashAdd(_In_ KEY_TYPE &key, _In_ VALUE_TYPE &value)
        {
            unsigned i = Bucket(&key, sizeof(KEY_TYPE));     // Get a bucket
            HashBase *phash = m_pfreeList;

            if (phash == NULL)          // Free list is empty, get some space from NRA
            {
                phash = (HashBase *)m_pnra->Alloc(sizeof(HashBase));
            }
            else                        // Get first one off the free list
            {
                VSASSERT(IsClear(phash), "A hashtable node was put on the free list, but appeares to be not free!");
                m_pfreeList = m_pfreeList->m_pNext;
            }

#if DEBUG
            phash->m_flags = HashBaseFull;
#endif

            memcpy(&phash->m_key, &key, sizeof(KEY_TYPE));          // Copy the key
            memcpy(&phash->m_value, &value, sizeof(VALUE_TYPE));    // Copy the value
            phash->m_pNext = NULL;

#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            if (m_rghash[i].m_pLastBase)
            {
                // Case 1: this bucket is not empty, adjust the last element pointer
                m_rghash[i].m_pLastBase->m_pNext = phash;
                m_rghash[i].m_pLastBase = phash;
            }
            else
            {
                m_rghash[i].m_pFirstBase = m_rghash[i].m_pLastBase = phash;
            }
#pragma prefast(pop)

            ++m_numEntries;
        }

        // The way this HashFind function works is the following: If only key is given, we use this
        // to find the first item with this key. If both key and value are given, we first use the
        // key to find the right bucket, then we use the value to match the values. This allows us
        // to have duplicates in the hash table.
        // To use this function, if you know that there are duplicates in the hash table, then you can
        // just use the first parameter, but if there is a possibility of duplicates, then you will
        // need to use both parameters
        VALUE_TYPE *HashFind(_In_ const KEY_TYPE &key, _In_opt_ VALUE_TYPE *pValue = NULL)
        {
            unsigned i = Bucket(&key, sizeof(KEY_TYPE));

#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *phash = m_rghash[i].m_pFirstBase;
#pragma prefast(pop)

            // Look in linked list, stop when we run out of keys or we find the right key
            for ( ; phash ; phash = phash->m_pNext)
            {
                VSASSERT(Isfull(phash), "Trying to access a node that looks damaged!");

                // We first check keys
                if (!memcmp(&phash->m_key, &key, sizeof(KEY_TYPE)))
                {
                    // Found the right key. Do we need to check values as well?
                    if (pValue)
                    {
                        if (!memcmp(&phash->m_value, pValue, sizeof(VALUE_TYPE)))
                        {
                            break;  // Key match, value match
                        }
                    }
                    else
                    {
                        break;      // Key match, no value
                    }
                }
            }

            if (phash)
            {
                m_pLastFound = phash;
                return &(phash->m_value);
            }

            return NULL;
        }

        // Gets the next guy in the linked list of nodes. Doesn't move on to the next bucket.
        VALUE_TYPE *FindNext()
        {
            if (m_pLastFound)
            {
                HashBase *pReturnMe = m_pLastFound->m_pNext;
                m_pLastFound = pReturnMe;

                if (pReturnMe)
                {
                    return &(pReturnMe->m_value);
                }
            }

            return NULL;
        }

#if DEBUG
        // Gets the next guy in the linked list of nodes. Doesn't move on to the next bucket.
        VALUE_TYPE *GetFirstInBucket(unsigned bucket)
        {
            if (bucket >= m_Size)
            {
                return NULL;        // Bucket overflow
            }

            HashBase *phash = m_rghash[bucket].m_pFirstBase;

            if (phash)
            {
                m_pLastFound = phash;
                return &(phash->m_value);
            }

            return NULL;
        }
#endif DEBUG

        // The way this HashRemove function works is the following: If only key is given, we use this
        // to find the first item with this key and wackit. If both key and value are given, we first
        // use the key to find the right bucket, then we use the value to match the values. This allows
        // us to have duplicates in the hash table.
        // To use this function, if you know that there are duplicates in the hash table, then you can
        // just use the first parameter, but if there is a possibility of duplicates, then you will
        // need to use both parameters
        VALUE_TYPE *HashRemove(KEY_TYPE &key, VALUE_TYPE *pValue = NULL)
        {
            unsigned i = Bucket(&key, sizeof(KEY_TYPE));

#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
            HashBase *phash     = m_rghash[i].m_pFirstBase;
            HashBase *phashPrev = m_rghash[i].m_pFirstBase;
#pragma prefast(pop)

            // Look in linked list, stop when we run out of keys or we find the right key
            for ( ; phash ; phash = phash->m_pNext)
            {
                VSASSERT(Isfull(phash), "Trying to access a node that looks damaged!");

                // We first check keys
                if (!memcmp(&phash->m_key, &key, sizeof(KEY_TYPE)))
                {
                    // Found the right key. Do we need to check values as well?
                    if (pValue)
                    {
                        if (!memcmp(&phash->m_value, pValue, sizeof(VALUE_TYPE)))
                        {
                            break;  // Key match, value match
                        }
                    }
                    else
                    {
                        break;      // Key match, no value
                    }
                }

                phashPrev = phash;
            }

            if (phash)
            {
#pragma prefast(push)
#pragma prefast(disable:26010, "The call to 'Bucket' returns an index that is properly bounded by the array size")
                if (phashPrev == phash)
                {
                    // phash is the first node in the list
                    if (m_rghash[i].m_pFirstBase == m_rghash[i].m_pLastBase)
                    {
                        // There is only one node in this bucket
                        VSASSERT(!phash->m_pNext, "Node should be the only one in the list");
                        m_rghash[i].m_pLastBase = NULL;
                    }

                    m_rghash[i].m_pFirstBase = phash->m_pNext;
                }
                else
                {
                    // phash is not the first node in the list
                    if (phash == m_rghash[i].m_pLastBase)
                    {
                        // phash is the last one in the list
                        m_rghash[i].m_pLastBase = phashPrev;
                    }

                    phashPrev->m_pNext = phash->m_pNext;
                }
#pragma prefast(pop)

                // If we are deleting the node that is currently pointed to by m_pLastFound
                // we reset the traversal pointers so that we start form the begining next time
                if (phash == m_pLastFound)
                {
                    ResetHashIterator();
                }

                phash->m_pNext = m_pfreeList;
                m_pfreeList = phash;

                --m_numEntries;

#if DEBUG
                phash->m_flags = HashBaseEmpty;
#endif

                return &(phash->m_value);   // found
            }

            return NULL;        // wasn't found
        }

        // Reset the iterator to the first bucket in the table
        void ResetHashIterator()
        {
            m_pLastIteration = NULL;
            m_LastBucket = 0;
        }

        // Iterate through the hash table.  Different from FindNext() in that
        // it iterates through ALL the elements in the hash table.  FindNext()
        // just gets the next guy in the same bucket.
        VALUE_TYPE *IterateHash()
        {
            if (m_pLastIteration)
            {
                m_pLastIteration = m_pLastIteration->m_pNext;
            }

            while (!m_pLastIteration)
            {
                if (m_LastBucket >= m_Size)
                {
                    break;
                }

                m_pLastIteration = m_rghash[m_LastBucket++].m_pFirstBase;
            }

            if (m_pLastIteration)
            {
                return &(m_pLastIteration->m_value);
            }

            return NULL;
        }

        //clone a hash table
        void IterateHashKeyAndValue(KEY_TYPE ** ppKey,VALUE_TYPE **ppValue)
        {
            *ppKey = NULL;
            *ppValue = NULL;
            if (m_pLastIteration)
            {
                m_pLastIteration = m_pLastIteration->m_pNext;
            }

            while (!m_pLastIteration)
            {
                if (m_LastBucket >= m_Size)
                {
                    break;
                }

                m_pLastIteration = m_rghash[m_LastBucket++].m_pFirstBase;
            }

            if (m_pLastIteration)
            {
                *ppKey = &m_pLastIteration->m_key;
                *ppValue = &m_pLastIteration->m_value;
            }
        }

        // return the number of elements stored in the hash
        inline unsigned NumEntries()
        {
            return m_numEntries;
        }

private:

#if DEBUG
    inline bool IsClear(HashBase *phash)
    {
        return ((phash->m_flags == 0x0) || (phash->m_flags == HashBaseEmpty));
    }


    inline bool Isfull(HashBase *phash)
    {
        return (phash->m_flags == HashBaseFull);
    }
#endif

    // This hash method was stolen from our string pool and works
    // really good for strings.  We need to verify that it's ok
    // for binary ---- as well.
    //
    unsigned Bucket(_In_ const void *pkey, unsigned cbSize)
    {
        VSASSERT(cbSize % _cbWordSize == 0, "Must be sizeof void * thing.");
//	 COMPILE_ASSERT(cbSize % _cbWordSize == 0);

        unsigned long hash = 0;
        unsigned long l;
        unsigned long *pl = (unsigned long *)pkey;

        unsigned long cl = cbSize / _cbWordSize;

        for (; 0 < cl; -- cl, ++ pl)
        {
            l = *pl;

            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;
        }

        return hash % m_Size;
    }

    unsigned m_Size; // Number of buckets
    unsigned m_LastBucket;      // the last bucket examined for the hash iterator
    HashBase *m_pLastFound;     // for use with FindNext()
    HashBase *m_pLastIteration; // for use with IterateHash()

    unsigned m_numEntries;      // number of elements in the hash table
    NorlsAllocator *m_pnra;     // NRA to use for memory
    HashBucket *m_rghash;   // Array of buckets to nodes
    HashBase  *m_pfreeList;     // linked list of free nodes to recycle
};

#pragma warning(default : 4200)

template<class TYPE>
class StackState
{
public:

    StackState(Stack<TYPE> *m_pStack) :
        m_pStack(m_pStack),
        m_ulCount(m_pStack->Count())
    {
    }

    ~StackState()
    {
        while (m_pStack->Count() > m_ulCount)
        {
            m_pStack->Pop();
        }
    }

private:

    Stack<TYPE> *m_pStack;
    ULONG m_ulCount;
};


//============================================================================
// Helper function to fill an array with a specific value.
//============================================================================

template<class FILLARRAY, class TYPE>
inline void Fill
(
    FILLARRAY pFirst,
    FILLARRAY pLast,
    const TYPE &Value)
{
    for (; pFirst != pLast; ++pFirst)
    {
        *pFirst = Value;
    }
}

//============================================================================
// Simple search tree implementation.  Does not support multiple entries
// with the same key.
//============================================================================

template<class KEY> class ExistanceTreeIterator;

template<class KEY>
class ExistanceTree
{
    friend class ExistanceTreeIterator<KEY>;

    struct TreeNode
    {
        KEY m_Key;

        TreeNode *m_pLeft;
        TreeNode *m_pRight;
    };

    public:

        ExistanceTree()
        {
        }

        ExistanceTree(NorlsAllocator *pnra)
        {
            Init(pnra);
        }

        void Init(NorlsAllocator *pnra)
        {
            m_pnra = pnra;

            Clear();
        }

        void Clear()
        {
            m_pHead = NULL;

#if DEBUG
            ClearTreeStats();
#endif
        }

        // Returns true if the existance tree is empty or false if it is not.
        bool IsEmpty()
        {
            return m_pHead == NULL;
        }

        // Add a node to the tree if it doesn't already exist.
        // Returns true if the node already exists, false otherwise.
        bool Add(KEY key)
        {
            Check();

            TreeNode *pNode, **ppNode = &m_pHead;
            int iCompare;

            // Walk the tree until we hit our leaf.
            while (*ppNode != NULL)
            {
                iCompare = (int)(key - (*ppNode)->m_Key);

#if DEBUG
                m_ulInsertionDepth++; // Total depth of all combined insertions to obtain the average.
#endif

                if (iCompare == 0)
                {
#if DEBUG
                    m_ulInsertionMisses++; // Number of misses
#endif
                    return true;
                }
                else if (iCompare < 0)
                {
                    ppNode = &(*ppNode)->m_pLeft;
                }
                else
                {
                    ppNode = &(*ppNode)->m_pRight;
                }
            }

            // Create a new node.
#if DEBUG
            //
            pNode = (TreeNode *)m_pnra->Alloc(sizeof(TreeNode) + sizeof(size_t));

            *(size_t *)pNode = 'SNKR';
            pNode = (TreeNode *)(((size_t *)pNode) + 1);

            m_ulInsertionHits++; // Number of hits

#else !DEBUG

            pNode = (TreeNode *)m_pnra->Alloc(sizeof(TreeNode));

#endif !DEBUG

            pNode->m_Key = key;
            pNode->m_pLeft = pNode->m_pRight = NULL;

            *ppNode = pNode;

            Check();

            return false;
        }

        bool Exist(KEY key)
        {
            Check();

            TreeNode *pNode = m_pHead;
            int iCompare;

            // Walk the tree until we hit our leaf.
            while (pNode != NULL)
            {
                iCompare = (int)(key - pNode->m_Key);

#if DEBUG
                m_ulSearchDepth++; // Total depth of all combined searches to obtain the average.
#endif

                if (iCompare == 0)
                {
#if DEBUG
                    m_ulSearchHits++; // Number of hits
#endif
                    return true;
                }
                else if (iCompare < 0)
                {
                    pNode = pNode->m_pLeft;
                }
                else
                {
                    pNode = pNode->m_pRight;
                }
            }

#if DEBUG
            m_ulSearchMisses++; // Number of misses
#endif

            return false;
        }

#if DEBUG && ID_TEST

        void Check()
        {
            CheckHelper(m_pHead);
        }

        void ClearTreeStats()
        {
            m_ulLeaves = 0;
            m_ulMaxDepth = 0;
            m_ulTotalDepth = 0;
            m_ulSearchDepth = 0;
            m_ulSearchHits = 0;
            m_ulSearchMisses = 0;
            m_ulInsertionDepth = 0;
            m_ulInsertionHits = 0;
            m_ulInsertionMisses = 0;
        }

        void DumpTreeStats()
        {
            m_ulLeaves = 0;
            m_ulMaxDepth = 0;
            m_ulTotalDepth = 0;

            ComputeStats(m_pHead, 1);

            DebPrintf("Existance Tree Statistics: \n");
            DebPrintf("    Number of nodes: %lu\n", m_ulInsertionHits);
            DebPrintf("    Number of leaves: %lu\n", m_ulLeaves);
            DebPrintf("    ---------------\n");

            DebPrintf("    Maximum tree depth: %lu\n", m_ulMaxDepth);
            DebPrintf("    Average tree depth: %lf\n", m_ulInsertionHits ? (double)(m_ulTotalDepth / m_ulInsertionHits) : (double) 0.0);
            DebPrintf("    ---------------\n");

            DebPrintf("    Search hits: %lu\n", m_ulSearchHits);
            DebPrintf("    Search misses: %lu\n", m_ulSearchMisses);
            DebPrintf("    Average Search: %lf\n", (m_ulSearchHits || m_ulSearchMisses) ? ((double) m_ulSearchDepth / (m_ulSearchHits + m_ulSearchMisses)) : (double) 0.0);
            DebPrintf("    ---------------\n");

            DebPrintf("    Insertion hits: %lu \n", m_ulInsertionHits);
            DebPrintf("    Insertion misses: %lu \n", m_ulInsertionMisses);
            DebPrintf("    Average insertion: %lf\n", (m_ulInsertionHits || m_ulInsertionMisses) ?  ((double) m_ulInsertionDepth / (m_ulInsertionHits + m_ulInsertionMisses)) : (double) 0.0);
            DebPrintf("    ---------------\n");
        }

#else !(DEBUG && ID_TEST)

        void Check() {}

#endif !DEBUG

private:

#if DEBUG

    void CheckHelper(TreeNode *pNode)
    {
        if (!pNode)
        {
            return;
        }

        CheckHelper(pNode->m_pLeft);

        VSASSERT(*((size_t *)pNode - 1) == 'SNKR', "Bad tree node.");

        CheckHelper(pNode->m_pRight);
    }

    void ComputeStats(TreeNode *pNode, unsigned long ulDepth)
    {
        if (pNode != NULL)
        {
            if (ulDepth > m_ulMaxDepth)
            {
                m_ulMaxDepth = ulDepth;
            }

            m_ulTotalDepth += ulDepth;

            if (pNode->m_pLeft == NULL && pNode->m_pRight == NULL)
            {
                m_ulLeaves++;
            }

            ComputeStats(pNode->m_pLeft, ulDepth + 1);
            ComputeStats(pNode->m_pRight, ulDepth + 1);
        }
    }

#endif DEBUG

    NorlsAllocator *m_pnra;
    TreeNode *m_pHead;

#if DEBUG
    unsigned long m_ulLeaves;
    unsigned long m_ulMaxDepth;
    unsigned long m_ulTotalDepth;
    unsigned long m_ulSearchDepth;
    unsigned long m_ulSearchHits;
    unsigned long m_ulSearchMisses;
    unsigned long m_ulInsertionDepth;
    unsigned long m_ulInsertionHits;
    unsigned long m_ulInsertionMisses;
#endif
};


template<class KEY>
class ExistanceTreeIterator
{
public:
    typedef typename ExistanceTree<KEY>::TreeNode TreeNode;

    ExistanceTreeIterator(ExistanceTree<KEY> *pTree)
        :m_stack(pTree->m_pnra->GetCompiler()), m_tree(pTree)
    {
        Reset();
    }

    void Reset()
    {
        m_stack.Reset();

        if ( m_tree->m_pHead )
        {
            m_stack.Push(m_tree->m_pHead);
        }
    }

    KEY *GetNext()
    {
        if ( m_stack.Empty())
        {
            return NULL;
        }

        TreeNode *top = m_stack.Top();
        m_stack.Pop();

        AssertIfNull(top);
        if ( top->m_pLeft )
        {
            m_stack.Push(top->m_pLeft);
        }
        if ( top->m_pRight)
        {
            m_stack.Push(top->m_pRight);
        }

        return &top->m_Key;
    }

private:
    ExistanceTreeIterator();
    ExistanceTreeIterator(const ExistanceTreeIterator&);
    ExistanceTreeIterator &operator=(const ExistanceTreeIterator&);

    ExistanceTree<KEY> *m_tree;
    Stack<TreeNode*> m_stack;
};

//============================================================================
// This template declars an object that can change the cursor to whatever you
// want it to be, then changes it back to the original settings when the variable
// declared of this type goes out of scope.
//
// ChangeCursor<IDC_WAITCURSOR> waitCursor;
// This Will change the cursor to IDC_WAITCURSOR in the scope of the declaration,
// and will change it back when waitCursor goes out of scope.
//============================================================================

template<LPWSTR TChangeType>
class ChangeCursor
{
public:
     ChangeCursor() {m_hPrevCursor = ::SetCursor(::LoadCursor(NULL, TChangeType));}
    ~ChangeCursor() {::SetCursor(m_hPrevCursor);}

protected:
    HCURSOR m_hPrevCursor;
};

//============================================================================
// Templated comparison to compare two values (binary)
//============================================================================
template
<
    class Type
>
int
BinaryCompare
(
    const Type *pType1,
    const Type *pType2
)
{
    VSASSERT(pType1 && pType2, "Invalid input into BinaryCompare!");
    return CompareValues(*pType1, *pType2);
}

//============================================================================
// Templated comparison to compare two values (binary)
//============================================================================
template
<
    class Type
>
int
BinaryValueCompare
(
    Type type1,
    Type type2
)
{
    return CompareValues(type1, type2);
}

//============================================================================
// Templated binary search function
// The out parameter plMatchIndex receives
//   - The index of the matching element if BSearchEx returns true
//   - The insertion point for the key if BSearchEx returns false
//============================================================================
template
<
    class KeyType,
    class ElementType
>
bool
BSearchEx
(
    const KeyType *pKey,                                            // [in]  Key used to search the array, can be NULL
    const ElementType *pArray,                                      // [in]  Array to search, can be NULL if lCount is 0
    long lCount,                                                    // [in]  Number of elements in the array
    int (*pfnCompare) (const KeyType *, const ElementType *),       // [in]  Comparison function, cannot be NULL
    _Out_opt_ long *plMatchIndex = NULL                                       // [out] Receives the index of the matching element or the insertion point, can be NULL
)
{
    VSASSERT(pArray || lCount == 0, "Argument pArray cannot be NULL unless lCount is 0.");
    VSASSERT(pfnCompare, "Argument pfnCompare cannot be NULL!");

    bool FoundMatch = false;
    long lMin = 0;
    long lMax = lCount - 1;
    long lMid;
    long lMatchIndex = 0;

    // Run the binary search
    while (lMin <= lMax)
    {
        lMid = (lMax + lMin) / 2;

        int iCompare = pfnCompare(pKey, &pArray[lMid]);

        if (iCompare == 0)
        {
            lMatchIndex = lMid;
            FoundMatch = true;
            break;
        }
        else if (iCompare < 0)
        {
            lMax = lMid - 1;
        }
        else
        {
            lMin = lMid + 1;
        }
    }

    // Store the insertion point if the search failed
    if (!FoundMatch)
    {
        lMatchIndex = lMin;
    }

    if (plMatchIndex)
    {
        *plMatchIndex = lMatchIndex;
    }

    return FoundMatch;
}


//============================================================================
// Templated binary search function
//
// The template types are slightly different from BSearchEx, which makes
// it easier to search arrays of pointer types (array of BCSYM_Proc *,
// searching with BCSYM_Proc *)
//
// The out parameter plMatchIndex receives
//   - The index of the matching element if BSearchEx returns true
//   - The insertion point for the key if BSearchEx returns false
//============================================================================
template
<
    class KeyType,
    class ElementType
>
bool
BSearchEx2
(
    KeyType Key,                                                    // [in]  Key used to search the array, can be NULL
    const ElementType *pArray,                                      // [in]  Array to search, can be NULL if lCount is 0
    long lCount,                                                    // [in]  Number of elements in the array
    int (*pfnCompare) (KeyType, ElementType),                       // [in]  Comparison function, cannot be NULL
    _Out_opt_ long *plMatchIndex = NULL                                       // [out] Receives the index of the matching element or the insertion point, can be NULL
)
{
    VSASSERT(pArray || lCount == 0, "Argument pArray cannot be NULL unless lCount is 0.");
    VSASSERT(pfnCompare, "Argument pfnCompare cannot be NULL!");

    bool FoundMatch = false;
    long lMin = 0;
    long lMax = lCount - 1;
    long lMid;
    long lMatchIndex = 0;

    // Run the binary search
    while (lMin <= lMax)
    {
        lMid = (lMax + lMin) / 2;

        int iCompare = pfnCompare(Key, pArray[lMid]);

        if (iCompare == 0)
        {
            lMatchIndex = lMid;
            FoundMatch = true;
            break;
        }
        else if (iCompare < 0)
        {
            lMax = lMid - 1;
        }
        else
        {
            lMin = lMid + 1;
        }
    }

    // Store the insertion point if the search failed
    if (!FoundMatch)
    {
        lMatchIndex = lMin;
    }

    if (plMatchIndex)
    {
        *plMatchIndex = lMatchIndex;
    }

    return FoundMatch;
}


//============================================================================
// Insert the element into the array maintaining a sorted order
//============================================================================
template
<
    class Type
>
void
InsertSortedNoDuplicate
(
    Type *pElement,                                      // [in]  Element to insert, cannot be NULL
    DynamicArray<Type> *pdaArray,                        // [in]  Array of sorted elements, cannot be NULL
    int (*pfnCompare) (const Type *, const Type *)                       // [in]  Comparison function, cannot be NULL
)
{
    VSASSERT(pElement, "Argument pElement cannot be NULL!");
    VSASSERT(pdaArray, "Argument pdaArray cannot be NULL!");
    VSASSERT(pfnCompare, "Argument pfnCompare cannot be NULL!");

    long lInsertionPoint = 0;
    if (!BSearchEx<Type, Type>(pElement,
                               pdaArray->Array(),
                               pdaArray->Count(),
                               pfnCompare,
                               &lInsertionPoint))
    {
        pdaArray->Insert(lInsertionPoint) = *pElement;
    }
}

//============================================================================
// Insert the element into the array maintaining a sorted order
//============================================================================
template
<
    class Type
>
void
InsertSortedNoDuplicate2
(
    Type pElement,                                     // [in]  Element to insert, cannot be NULL
    _Inout_ DynamicArray<Type> *pdaArray,                      // [in]  Array of sorted elements, cannot be NULL
    int (*pfnCompare) (Type, Type)     // [in]  Comparison function, cannot be NULL
)
{
    // Microsoft: The element index can be 0.
    // VSASSERT(pElement, "Argument pElement cannot be NULL!");
    VSASSERT(pdaArray, "Argument pdaArray cannot be NULL!");
    VSASSERT(pfnCompare, "Argument pfnCompare cannot be NULL!");

    long lInsertionPoint = 0;
    if (!BSearchEx2<Type, Type>(pElement,
                                pdaArray->Array(),
                                pdaArray->Count(),
                                pfnCompare,
                                &lInsertionPoint))
    {
        pdaArray->Insert(lInsertionPoint) = pElement;
    }
}

//============================================================================
// Remove an element from the given array
//============================================================================
template
<
    class Type
>
void
Delete2
(
    Type pElement,                                     // [in]  Element to delete, cannot be NULL
    DynamicArray<Type> *pdaArray,                      // [in]  Array of sorted elements, cannot be NULL
    int (*pfnCompare) (Type, Type)                     // [in]  Comparison function, cannot be NULL
)
{
    VSASSERT(pElement, "Argument pElement cannot be NULL!");
    VSASSERT(pdaArray, "Argument pdaArray cannot be NULL!");
    VSASSERT(pfnCompare, "Argument pfnCompare cannot be NULL!");

    long lIndex = 0;
    if (BSearchEx2<Type, Type>(pElement,
                               pdaArray->Array(),
                               pdaArray->Count(),
                               pfnCompare,
                               &lIndex))
    {
        pdaArray->Remove(lIndex);
    }
}

//============================================================================
// Find the first matching element
//============================================================================
template
<
    class KeyType,
    class ElementType
>
bool
FindFirstMatch
(
    KeyType *pElement,                                              // [in]  Element to insert, cannot be NULL
    const ElementType *pArray,                                      // [in]  Array to search, can be NULL if lCount is 0
    long lCount,                                                    // [in]  Number of elements in the array
    int (*pfnCompare) (const KeyType *, const ElementType *),       // [in]  Comparison function, cannot be NULL
    long *plMatchIndex = NULL                                       // [out] Receives the index of the matching element or the insertion point, can be NULL
)
{
    VSASSERT(pElement, "Argument pElement cannot be NULL!");
    VSASSERT(pArray, "Argument pArray cannot be NULL!");
    VSASSERT(pfnCompare, "Argument pfnCompare cannot be NULL!");

    bool fMatch = false;
    long lIndex = 0;
    long lFirstIndex = 0;
    long lCurrentCount = lCount;

    while (BSearchEx(pElement,
                     pArray,
                     lCurrentCount,
                     pfnCompare,
                     &lIndex))
    {
        lFirstIndex = lIndex;
        lCurrentCount = lFirstIndex;
        fMatch = true;
    }

    if (plMatchIndex)
    {
        *plMatchIndex = lFirstIndex;
    }

    return fMatch;
}

#undef GET_ALIGNED_SIZE

template <class ConstructorArg, class SavedValue>
class BackupState
{
public:
    BackupState(const ConstructorArg & constructorArg, const SavedValue & savedValue) :
        m_constructorArg(constructorArg),
        m_savedValue(savedValue),
        m_fRestored(false)
    {
    }

    void Restore()
    {
        if (! m_fRestored)
        {
            DoRestore();
            m_fRestored = true;
        }
    }

    const SavedValue & OldValue()
    {
        return m_savedValue;
    }

    void ReInit()
    {
        VSASSERT( m_fRestored, "No need to reinitialize if restore has not been called");
        m_fRestored = false;
    }

protected:
    virtual void DoRestore() = 0;

    SavedValue m_savedValue;
    ConstructorArg m_constructorArg;
private:
    bool m_fRestored;
};

template <class T>
class BackupValue :
    public BackupState<T *, T>
{
public:
    BackupValue(_In_opt_ T * pMemory)
        : BackupState(pMemory, pMemory ? *pMemory : T())
    {
    }

    BackupValue(_In_opt_ T  *pMemory, T value)
        : BackupState(pMemory, value)
    {
    }

    ~BackupValue()
    {
        Restore();
    }

    void Init(_In_ T* pMemory)
    {
        VSASSERT(!m_constructorArg, "Double Init");
        VSASSERT(pMemory, "NULL Memory location to backup");
        m_constructorArg = pMemory;
        m_savedValue = *pMemory;
    }

protected:
    void DoRestore()
    {
        if (m_constructorArg)
        {
            *m_constructorArg = m_savedValue;
        }
    }

protected:
};

template <class T, class U, void (T::*setter)(U arg) >
class BackupProperty :
    public BackupState<T *, U>
{
public:
    BackupProperty(T * obj, U value) 
        : BackupState(obj, value)
    {
    }

    ~BackupProperty()
    {
        Restore();
    }

protected:
    void DoRestore()
    {
        if (m_constructorArg)
        {
            (m_constructorArg->*setter)(m_savedValue);
        }
    }

protected:
};

template <class T>
class OutputParameter
{
public:
    OutputParameter(T * pMemory) :
        m_pMemory(pMemory)
    {
    }

    OutputParameter & operator = (const T & value)
    {
        if (m_pMemory)
        {
            *m_pMemory = value;
        }
        return *this;
    }

    operator T *()
    {
        return m_pMemory;
    }

    T & operator *()
    {
        return * m_pMemory;
    }
private:
    T * m_pMemory;
};
