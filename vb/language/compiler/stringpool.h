//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The string pool.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class StringPool;
struct STRING_INFO;
struct Casing;
class BCSYM_Hash;
class BCSYM_Namespace;
class BCSYM_NamespaceRing;
class BCSYM_NamedRoot;

// All strings and characters in this codebase are UNICODE.
// The STRING typedef is used for all strings that appear in the string pool.
// Use BSTRs if you need to store dynamically allocated strings.
// The WCHAR typedef for fixed sized strings and general character manipulation.
typedef WCHAR STRING;

unsigned long ComputeHashValue(
    _In_ void * pv,
    unsigned cbSize);

#define SZ_SIGNATURE  "STRING"

/*
** Describes a specific case of a string.
*/

// Don't give a warning on the zero-sized array within the structure.
#pragma warning( disable : 4200 )

struct Casing
{

    //
    // Links.
    //

    STRING_INFO *m_pstrinfo;   // pointer back to the shared string info
    Casing *m_pspellingNext; // the next spelling in the spelling hash table

    //
    // Per-spelling information.
    //

    union
    {
        struct
        {
            unsigned m_cchLength:16;
            unsigned m_ulSpellingHash:16;
        };

        // So we can quickly compare with both of the values above.
        unsigned m_ulCompare;
    };

    //
    // The string itself.
    //

#if DEBUG
    char m_pstrDebSignature[sizeof(SZ_SIGNATURE)];
#endif // DEBUG

    STRING m_str[0];       // the string
};

#pragma warning( default : 4200 )

/*
** Information shared by all spellings of a string.
*/

// hungarian name : strinfo
struct STRING_INFO
{

    //
    // The following lists are needed to build the hash table.
    //

    STRING_INFO *m_pstrinfoNext;      // next string info in the hash table

    //
    // Extra information we need about the string.
    //

    union
    {
        struct
        {
            unsigned m_cchLength:16;
            unsigned m_ulSysHash:16;      // relavent bits in the system hash value
        };

        unsigned m_ulCompare;       // for fast comparisons a la Casing.
    };

    unsigned m_ulLocalHash:16;    // hash value to be used within this process
    unsigned m_MatchingToken:16;  // the token that this string represents, if any.

    // Name lookup is a potentially expensive operation in compilation,
    // especially looking up names in namespaces. The name lookup logic
    // (in NameSemantics.cpp) uses three tricks to avoid unnecessary
    // searches through namespaces:
    //
    //     1) The VB names-declared-in-modules-are-visible-at-namespace-level
    //        rule is not applied unless a name has a declaration in at least
    //        one module.
    //     2) Looking a name up in a namespace is skipped if either
    //          a) The name has no declarations in any namespaces, or
    //          b) The name is declared in exactly one namespace, and
    //             that namespace is not the one in question.
    //     3) If a name is looked up in (or starting in) a given scope
    //        using a given bindspace, and the name is ultimately found
    //        in a namespace, the combination of the starting scope, the
    //        bindspace, and the resulting declaration is cached so that
    //        a subsequent lookup of the same name starting in the same
    //        scope and the same bindspace can bypass the lookup mechanism.
    //
    // The effectiveness of these tricks is such that they reduce the number
    // of hash table lookups in namespaces by over 90% in compiling the VB
    // runtime, and reduce the number of hash table lookups for the
    // names-in-modules rule by even more.
    //
    // All of these tricks depend on storing information in STRING_INFO nodes.

    // This flag indicates that the name might be declared in at least one
    // Module, and that name lookup must apply the special Module name lookup
    // rules for this name.

    bool m_DeclaredInModule;

    // This flag indicates that the name is declared in at least one namespace.
    // If it is declared in exactly one namespace, then the UniqueNamespace field
    // gives it. Otherwise, the UniqueNamespace field is NULL. This is used
    // to skip looking up names in namespaces where it can be predetermined that
    // a name cannot be declared in a particular namespace. (This relies on
    // names usually being declared in no more than one namespace.)

    bool m_DeclaredInNamespace;

    BCSYM_NamespaceRing *m_UniqueNamespace;

    //
    // The default string.
    //

    Casing m_spelling;
};


//============================================================================
// StringPool
//
//   This class implements the string pool.  There is a single string pool
//   object shared by all the projects in one context. There is no facilty
//   for removing names, they just keep getting added and are all freed when
//   the compiler shuts down.
//
//============================================================================

DECLARE_ENUM(StringComparison)
    CaseInsensitive,
    CaseSensitive
END_ENUM(StringComparison)

class StringPool
{
private:

    // Get the spelling from the string.
    static
    Casing * Pspelling(const STRING * pstr)
    {
        Casing *pspelling = (Casing *)((BYTE *)pstr - offsetof(Casing, m_str));

        VSASSERT(pstr != NULL, "Someone passed the StringPool a NULL string!");
        VSASSERT(!strcmp(pspelling->m_pstrDebSignature, SZ_SIGNATURE), "Bad string.");

        return pspelling;
    }

    // double the size of the current hash tables if possible
    bool ExpandStrInfoTable();
    bool ExpandSpellingTable();

public:
    NEW_MUST_ZERO()

    StringPool();
    ~StringPool();

    // move between a string to its STRING_INFO.
    static
    STRING_INFO * Pstrinfo(const STRING * pstr)
    {
        return Pspelling(pstr)->m_pstrinfo;
    }

    // add a name to the hash table and return a STRING
    STRING * AddString(_In_opt_z_ const WCHAR * pwchar)
    {
        if (!pwchar)
        {
            return NULL;
        }

        return AddStringWithLen(pwchar, wcslen(pwchar));
    }

    StringPoolEntry AddStringPoolEntry( _In_opt_z_ const WCHAR *pwszData )
    {
        return AddString(pwszData);
    }

    // add a name to the hash table and return a STRING
    STRING *AddString(StringBuffer *pStringBuffer)
    {
        if (!pStringBuffer)
        {
            return NULL;
        }

        return AddStringWithLen(pStringBuffer->GetString(), pStringBuffer->GetStringLength());
    }


    STRING * 
#if HOSTED
        __stdcall
#else
        _fastcall 
#endif
            AddStringWithLen(
        _In_opt_count_(cchSize)const WCHAR * pwchar,
        size_t cchSize);

    // Lookup a string without adding it if it isn't already there.
    STRING * _fastcall LookupStringWithLen(
        _In_count_(cchSize)const WCHAR * pwchar,
        size_t cchSize,
        bool isCaseSensitive);

    // add a token to the hash table and return a STRING
    STRING * AddToken(
        _In_z_ const WCHAR * pwchar,
        unsigned tk);

    // add a new string, which is the concatenation of three strings.
    STRING * ConcatStrings(
        _In_z_ const WCHAR * wsz1,
        _In_z_ const WCHAR * wsz2,
        _In_opt_z_ const WCHAR * wsz3 = NULL,
        _In_opt_z_ const WCHAR * wsz4 = NULL,
        _In_opt_z_ const WCHAR * wsz5 = NULL,
        _In_opt_z_ const WCHAR * wsz6 = NULL);

    STRING * ConcatStringsArray(
        unsigned count,
        _In_count_(count)const WCHAR * * wsz,
        _In_opt_z_ const WCHAR * wszSeparator);

    //========================================================================
    // Well-known strings
    //========================================================================

    STRING *GetStringConstant(unsigned iString)
    {
        if (iString < _countof(m_rgStringConstantTable))
        {
            return m_rgStringConstantTable[iString];
        }

        return NULL;
    }

    STRING *TokenToString(tokens tk)
    {
        if (tk >= 0 && tk < _countof(m_pstrTokenToString))
        {
            VSASSERT(m_pstrTokenToString[tk], "this token has no string representation");
            return m_pstrTokenToString[tk];
        }

        return NULL;
    }

    // Case insensitive compare.  Handles NULL parameters.
    static
    bool IsEqual(
        const STRING * pstr1,
        const STRING * pstr2)
    {
        if (pstr1 == pstr2)
        {
            return true;
        }

        if (!pstr1 || !pstr2)
        {
            return false;
        }

        return Pstrinfo(pstr1) == Pstrinfo(pstr2);
    }

    // Case sensitive comparision
    static
    bool IsEqualCaseSensitive(
        _In_opt_z_ const STRING * pstr1,
        _In_opt_z_ const STRING * pstr2)
    {
        return pstr1 == pstr2;
    }

    // Compare, with sensitivity switched on a flag.
    static
    bool IsEqual(
        _In_opt_z_ const STRING * pstr1,
        _In_opt_z_ const STRING * pstr2,
        bool isCaseSensitive);

    static
    bool IsEqual(
        _In_opt_z_ const STRING * pstr1,
        _In_opt_z_ const STRING * pstr2,
        _In_ StringComparisonEnum e);

    static int Compare(
        _In_opt_z_ const STRING *pstr1,
        _In_opt_z_ const STRING *pstr2 )
    {
        if ( 0 == (Pstrinfo(pstr1) - Pstrinfo(pstr2)))
        {
            return 0;
        }

        return CompareNoCase(pstr1, pstr2);
    }

    static int CompareCaseSensitive(
        _In_z_ const STRING* pstr1,
        _In_z_ const STRING * pstr2 )
    {
        if ( 0 == (pstr1 - pstr2))
        {
            return 0;
        }

        return CompareCase( pstr1, pstr2);
    }

    static int Compare(
        _In_opt_z_ const STRING* pstr1,
        _In_opt_z_ const STRING * pstr2,
        _In_ bool isCaseSensitive)
    {
        return isCaseSensitive 
            ? CompareCaseSensitive(pstr1, pstr2)
            : Compare(pstr1, pstr2);
    }

    static int Compare(
        _In_opt_z_ const STRING* pstr1,
        _In_opt_z_ const STRING * pstr2,
        _In_ StringComparisonEnum comp)
    {
        switch ( comp )
        {
        case StringComparison::CaseInsensitive:
            return Compare(pstr1, pstr2);
        case StringComparison::CaseSensitive:
            return CompareCaseSensitive(pstr1, pstr2);
        default:
            ThrowIfFalse(false);
            return 0;
        }
    }

    // Get the local hash value of a string.
    static
    ULONG HashValOfString(_In_opt_z_ const STRING * pstr)
    {
        return pstr ? Pstrinfo(pstr)->m_ulLocalHash : 0;
    }

    // Get the token associated with a string.
    static
    unsigned TokenOfString(_In_z_ const STRING * pstr)
    {
        return Pstrinfo(pstr)->m_MatchingToken;
    }

    // The length of the spelling.
    static
    size_t StringLength(_In_z_ const STRING * pstr)
    {
        return Pspelling(pstr)->m_cchLength;
    }

    // Is the string null or empty.
    static
    bool IsNullOrEmpty(_In_z_ const STRING * pstr)
    {
        return pstr == NULL ? true : StringPool::StringLength(pstr) == 0;
    }

    // Get the hash value of binary data.
    static
    unsigned long ComputeHashValue(
        _In_ void * pv,
        size_t cbSize);

    // Test if the string might have a declaration in a Module.
    static
    bool DeclaredInModule(_In_z_ STRING * pstr)
    {
        return Pstrinfo(pstr)->m_DeclaredInModule;
    }

    static
    void SetDeclaredInModule(_In_z_ STRING * pstr)
    {
        Pstrinfo(pstr)->m_DeclaredInModule = true;
    }

    // These methods implement a mechanism for determining whether looking
    // up a name in a particular namespace has any chance to succeed.
    // (This is used to short-circuit unnecessary work in name lookup.)

    static
    void AddDeclarationInNamespace(
        _In_z_ STRING * pstr,
        _In_ BCSYM_Namespace * pNamespace);

    static
    bool CanBeDeclaredInNamespace(
        _In_z_ STRING * pstr,
        _In_ BCSYM_Namespace * pNamespace);


    SAFEARRAY * GetStringPoolData();
private:

    // Get the significant portion of the spelling hash value.  Ignore the
    // lower 10 bits because they're encoded in the hash table.  Ignore
    // the top 6 bits because we'll probably never have a hash table
    // that big.  Just return the remaining 16 bits.
    //
    static
    unsigned GetSignificantSpellingHashValue(unsigned ulSpellingHash)
    {
        return(ulSpellingHash >> 10) & 0xFFFF;
    }

    // Convert a hash value and a length into a compare value.
    static
    unsigned GetCompareValue(
        size_t cchLength,
        unsigned ulSpellingHash)
    {
        return(ulSpellingHash << 16) | (unsigned)cchLength;
    }

    // Get the hash value of a WCHAR string.
    static
    unsigned long _fastcall ComputeStringHashValue(
        _In_ unsigned long * wsz,
        size_t cl,
        size_t r,
        bool CaseSensitive);

    //
    // private data members
    //

    STRING_INFO **m_strinfoHashTable;     // string hash table
    Casing **m_spellingHashTable;       // spelling hash table

    unsigned m_ulStrInfoHashTableSize;    // current size of the strinfo hash table
    unsigned m_ulSpellingHashTableSize;   // current size of the spelling hash table

    unsigned m_ulStrInfoHashTableMask;    // size of hash table minus 1.
    unsigned m_ulSpellingHashTableMask;

    unsigned m_ulStrInfoThreshold;        // thresholds for resizing hash table.
    unsigned m_ulSpellingThreshold;

    unsigned m_ulNameCount;               // number of names in the table
    unsigned m_ulSpellingCount;           // number of spellings in the table

#if FV_TRACK_MEMORY

    // stats
    unsigned m_cCalls;
    unsigned m_cSpellingProbes;
    unsigned m_cDeepSpellingProbes;
    unsigned m_cFailedDeepSpellingProbes;

    unsigned m_cStringAttempts;
    unsigned m_cStringProbes;
    unsigned m_cDeepStringProbes;

    size_t m_cStringMemory;           // memory used by first spelling of a string
    size_t m_cAddtlSpellingMemory;    // memory used by additional spellings
    size_t m_cNonStringMemory;        // memory used for misc. overhead (included in above) not incl. tables
#if DEBUG
    size_t m_cDebugOnlyMemory;        // memory only used in the debug version (included in above)
#endif
#endif

    PageHeap m_heap;                    // allocate all strings from this heap to avoid fragmentation
                                        // across pages allocated for other purposes.  It makes it 
                                        // nearly impossible to free arenas when there are StringPool
                                        // entries fragmented across each of them
    
    NorlsAllocator m_nraStrings;        // memory allocator

    // String constant table.
    STRING *m_rgStringConstantTable[STRING_CONST_MAX];

    // Keyword tables.
    STRING *m_pstrTokenToString[tkCount];

    // This critical section is used to serialize access to the underlying shared
    // memory of the StringPool.  In the IDE, the StringPool can and will be accessed 
    // from multiple threads and we need to protect the resources which get updated 
    // and accessed in this context.  In particular this lock is designed to 
    // protect access to 
    //
    //   - m_strinfoHashTable
    //   - m_spellingHashTable
    //   - m_ul*
    //
    // All other member variables, while potentially accessed in multiple threads, are
    // initialized to their final state within the StringPool constructor and hence
    // are safe to read from multiple threads
    CompilerIdeCriticalSection m_CriticalSection;
};
