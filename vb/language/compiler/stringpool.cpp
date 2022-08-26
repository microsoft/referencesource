//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The string pool manager is to hash and store strings. It also provides
//  appropriate APIs to do string comparision in case sensitive and insensitive
//  way.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// The size of the hash table.  Should be a power of 2. Must be >= 1024
#define BaseSpellingHashTableSize 4096
#define BaseStrInfoHashTableSize  4096

// Maximum size for the hash tables to grow to.  Also a power of 2.
// Must be >= the base hash table sizes above
#define MaxSpellingHashTableSize 32768
#define MaxStrInfoHashTableSize  32768

// Allowed average bucket size before growing the tables,
// preferably a power of 2.
#define IdealBucketSize 4

//============================================================================
// Initialize the string pool.
//============================================================================

StringPool::StringPool() :
    m_nraStrings(NORLSLOC, m_heap)
{
    // Allocate the stringinfo hash table.
    m_ulStrInfoHashTableSize = BaseStrInfoHashTableSize;
    m_ulStrInfoHashTableMask = m_ulStrInfoHashTableSize - 1;
    m_strinfoHashTable = VBAllocator::AllocateArray<STRING_INFO*>(BaseStrInfoHashTableSize);

    // Allocate the spelling hash table.
    m_ulSpellingHashTableSize = BaseSpellingHashTableSize;
    m_ulSpellingHashTableMask = m_ulSpellingHashTableSize - 1;
    m_spellingHashTable = VBAllocator::AllocateArray<Casing*>(BaseSpellingHashTableSize);

    // calculate thresholds.
    m_ulStrInfoThreshold = IdealBucketSize * BaseStrInfoHashTableSize;
    m_ulSpellingThreshold = IdealBucketSize * BaseSpellingHashTableSize;

    // Port Note: The below code was moved from Compiler::Compiler(3 params) 
    // inside Compiler.cpp around line 1324
    //
    // Initialize the string constant table.
    //
    {
        const WCHAR *rgConstantStrings[] =
        {
            #define DEFINE_STRING_CONSTANT(name, string) L##string,
            #include "StringConstants.h"
            #undef DEFINE_STRING_CONSTANT
        };

        unsigned iString;

        for (iString = 0; iString < STRING_CONST_MAX; iString++)
        {
            m_rgStringConstantTable[iString] = AddString(rgConstantStrings[iString]);
        }
    }

    // Hash the string representations of all the tokens.
    for (unsigned kwdNo = 0;
        kwdNo < tkCount;
        kwdNo++)
    {
        kwdDsc kdsc = g_tkKwdDescs[kwdNo];
        WCHAR *wTkName = (WCHAR *)g_tkKwdNames[kwdNo];
        WCHAR WideTokenName[18];
        unsigned Character = 0;

        // Ignore this entry if it's not a 'real' token.
        switch (kwdNo)
        {
            case tkNone:
            case tkID:
            case tkEOF:
            case tkEOL:
            case tkSyntaxError:
            case tkIntCon:
            case tkStrCon:
            case tkFltCon:
            case tkDecCon:
            case tkCharCon:
            case tkDateCon:

            // Xml specific tokens
            case tkXmlWhiteSpace:
            case tkXmlNCName:
            case tkXmlAttributeData:
            case tkXmlReference:
            case tkXmlCharData:
            case tkXmlCData:
            case tkXmlCommentData:
            case tkXmlPIData:
            case tkXmlColon:
            case tkXmlBeginPI:  // same as tkNullable
                continue;
        }

        // Hash the string representation of the token.
#pragma prefast(suppress: 26010, "We are only looping over arrays created by tables")
        m_pstrTokenToString[kdsc.kdValue] = AddToken(wTkName, kdsc.kdValue);

        // Now hash the wide version of the token. Note that we just throw away the string.
        // The point of this is so that if you enter a keyword using wide characters, then
        // it will be accepted by the compiler.
        VSASSERT(wcslen(wTkName) < DIM(WideTokenName), "Buffer too small or token too big!");

        size_t cchTokenLength = wcslen(wTkName);

        if (cchTokenLength + 1 < _countof(WideTokenName))
        {
            for (Character = 0; Character < cchTokenLength; Character++)
            {
                WideTokenName[Character] = MAKEFULLWIDTH(wTkName[Character]);
            }
        }

        WideTokenName[Character] = L'\0';
        AddToken(WideTokenName, kdsc.kdValue);
    }
}

//============================================================================
// Destructs the strpool manager. It frees hash table and all the memory
// allocated used by the string pool  manager.
//============================================================================

StringPool::~StringPool()
{
    VBFree(m_strinfoHashTable);
    VBFree(m_spellingHashTable);
}

//============================================================================
// Wrapper on the string comparison functions.  Use this if case-sensitivity
// is based on a flag.
//============================================================================

bool StringPool::IsEqual(
    _In_opt_z_ const STRING * pstr1,
    _In_opt_z_ const STRING * pstr2,
    bool isCaseSensitive)
{
    return isCaseSensitive 
        ? IsEqualCaseSensitive(pstr1, pstr2)
        : IsEqual(pstr1, pstr2);
}

bool StringPool::IsEqual(
    _In_opt_z_ const STRING * pstr1,
    _In_opt_z_ const STRING * pstr2,
    _In_ StringComparisonEnum e)
{
    switch ( e )
    {
    case StringComparison::CaseInsensitive:
        return IsEqual(pstr1, pstr2);
    case StringComparison::CaseSensitive:
        return IsEqualCaseSensitive(pstr1, pstr2);
    default:
        ThrowIfFalse(false);
        return 0;
    }
}


//============================================================================
// Add a string to the hash table and returns a string.
//============================================================================

STRING * StringPool::AddToken(
    _In_z_ const WCHAR * pwchar,
    unsigned token)
{
    STRING *pstr;
    STRING_INFO *pstrinfo;


    // Get the string.
    pstr = AddString(pwchar);

    // Get the info.
    pstrinfo = Pstrinfo(pstr);

    VSASSERT(pstrinfo->m_MatchingToken == 0, "Token already assigned.");

    pstrinfo->m_MatchingToken = token;

    return pstr;
}

//============================================================================
// Add a new string, which is the concatenation of three strings.
// If an input string is NULL, it is ignored.
// Asserts if all three are NULL.
//============================================================================

STRING * StringPool::ConcatStrings(
    _In_z_ const WCHAR * wsz1,
    _In_z_ const WCHAR * wsz2,
    _In_opt_z_ const WCHAR * wsz3,
    _In_opt_z_ const WCHAR * wsz4,
    _In_opt_z_ const WCHAR * wsz5,
    _In_opt_z_ const WCHAR * wsz6)
{
    StringBuffer buffer;

    VSASSERT(wsz1 != NULL || wsz2 != NULL, "somebody's gotta be there");

    if (wsz1 != NULL)
    {
        buffer.AppendString(wsz1);
    }

    if (wsz2 != NULL)
    {
        buffer.AppendString(wsz2);
    }

    if (wsz3 != NULL)
    {
        buffer.AppendString(wsz3);
    }

    if (wsz4 != NULL)
    {
        buffer.AppendString(wsz4);
    }

    if (wsz5 != NULL)
    {
        buffer.AppendString(wsz5);
    }

    if (wsz6 != NULL)
    {
        buffer.AppendString(wsz6);
    }

    if (buffer.GetStringLength() == 0)
    {
        return NULL;
    }

    return AddString(&buffer);
}

STRING * StringPool::ConcatStringsArray(
    unsigned count,
    _In_count_(count)const WCHAR * * wsz,
    _In_opt_z_ const WCHAR * wszSeparator)
{
    StringBuffer buffer;

    VSASSERT(wsz != NULL, "somebody's gotta be there");

    if (wsz[0] != NULL)
    {
        buffer.AppendString(wsz[0]);
    }

    for (unsigned i = 1; i < count; i++)
    {
        if (wszSeparator)
        {
            buffer.AppendString(wszSeparator);
        }

        buffer.AppendString(wsz[i]);
    }

    return AddString(&buffer);
}


//============================================================================
// Doubles the size of the string info hash table.  Returns false if
// the table cannot be expanded, either because it cannot allocate
// memory, or because the table is already at max size.
//============================================================================

bool StringPool::ExpandStrInfoTable()
{
    // fail if not allowed to expand table further
    if (m_ulStrInfoHashTableSize == MaxStrInfoHashTableSize)
    {
        m_ulStrInfoThreshold = 0xFFFFFFFF;      // make sure we're never called again.
        return false;
    }

    // move up StrInfo threshold.
    m_ulStrInfoThreshold = m_ulStrInfoThreshold << 1;

    STRING_INFO** strinfoNewHashTable = NULL;

    // Reallocate the stringinfo hash table with new size
    strinfoNewHashTable =
        (STRING_INFO**)VBRealloc(m_strinfoHashTable,
                                 2 * m_ulStrInfoHashTableSize * sizeof(STRING_INFO*));

    // fail if unable to expand table further
    if (strinfoNewHashTable == NULL)
    {
        return false;
    }

    m_strinfoHashTable = strinfoNewHashTable;

    // zerofill newly allocated memory
    memset(m_strinfoHashTable + m_ulStrInfoHashTableSize,
           0,
           m_ulStrInfoHashTableSize  * sizeof(STRING_INFO *));

    // take old table and fill into new table, starting at bottom
    unsigned istrinfoTablePos = m_ulStrInfoHashTableSize;
    unsigned index;
    STRING_INFO *pstrinfoPrev, *pstrinfoCur, *pstrinfoNext;

    unsigned ulPrevHashTableDoublings = (m_ulStrInfoHashTableSize / 1024) - 1;
    m_ulStrInfoHashTableSize *= 2;
    m_ulStrInfoHashTableMask = m_ulStrInfoHashTableSize - 1;

    do
    {
        // go though one bucket of the table and place items elsewhere
        istrinfoTablePos--;
        pstrinfoPrev = NULL;
        pstrinfoCur = m_strinfoHashTable[istrinfoTablePos];
        while (pstrinfoCur != NULL)
        {

            pstrinfoNext = pstrinfoCur->m_pstrinfoNext;
            index = ((pstrinfoCur->m_ulSysHash << 10)+(istrinfoTablePos & 1023))
                    & (m_ulStrInfoHashTableSize-1);

            // move it if necessary
            if (index != istrinfoTablePos)
            {
                // remove it from the current bucket
                if (pstrinfoPrev == NULL)
                {
                    m_strinfoHashTable[istrinfoTablePos] = pstrinfoNext;
                }
                else
                {
                    pstrinfoPrev->m_pstrinfoNext = pstrinfoNext;
                }

                // place it in a new bucket
                pstrinfoCur->m_pstrinfoNext = m_strinfoHashTable[index];
                m_strinfoHashTable[index] = pstrinfoCur;

            }
            else
            {
                pstrinfoPrev = pstrinfoCur;
            }

            pstrinfoCur = pstrinfoNext;
        }

    } while (istrinfoTablePos > 0);

    return true;
}

//============================================================================
// Doubles the size of the spelling hash table.  Returns false if
// the table cannot be expanded, either because it cannot allocate
// memory, or because the table is already at max size.
//============================================================================

bool StringPool::ExpandSpellingTable()
{
    // fail if not allowed to expand table further
    if (m_ulSpellingHashTableSize == MaxSpellingHashTableSize)
    {
        m_ulSpellingThreshold = 0xFFFFFFFF;     // make sure we're never called again.
        return false;
    }

    // move up spelling threshold.
    m_ulSpellingThreshold = m_ulSpellingThreshold << 1;

    Casing** spellingNewHashTable = NULL;

    // Reallocate the stringinfo hash table with new size
    spellingNewHashTable =
        (Casing**)VBRealloc(  m_spellingHashTable,
                              2 * m_ulSpellingHashTableSize * sizeof(Casing*));

    // fail if unable to expand table further
    if (spellingNewHashTable == NULL)
    {
        return false;
    }

    m_spellingHashTable = spellingNewHashTable;

    // zerofill newly allocated memory
    memset( m_spellingHashTable + m_ulSpellingHashTableSize,
            0,
            (size_t)(m_ulSpellingHashTableSize * sizeof(Casing*)));

    // take old table and fill into new table, starting at bottom
    unsigned ispellingTablePos = m_ulSpellingHashTableSize;
    unsigned index;
    Casing *pspellingPrev, *pspellingCur, *pspellingNext;

    unsigned ulPrevHashTableDoublings = (m_ulSpellingHashTableSize / 1024) - 1;
    m_ulSpellingHashTableSize *= 2;
    m_ulSpellingHashTableMask = m_ulSpellingHashTableSize - 1;

    do
    {
        // go though one bucket of the table and place items elsewhere
        ispellingTablePos--;
        pspellingPrev = NULL;
        pspellingCur = m_spellingHashTable[ispellingTablePos];
        while (pspellingCur != NULL)
        {

            pspellingNext = pspellingCur->m_pspellingNext;
            index = ((pspellingCur->m_ulSpellingHash << 10)+(ispellingTablePos & 1023))
                    & (m_ulSpellingHashTableSize-1);

            // move it if necessary
            if (index != ispellingTablePos)
            {
                // remove it from the current bucket
                if (pspellingPrev == NULL)
                {
                    m_spellingHashTable[ispellingTablePos] = pspellingNext;
                }
                else
                {
                    pspellingPrev->m_pspellingNext = pspellingNext;
                }

                // place it in a new bucket
                pspellingCur->m_pspellingNext = m_spellingHashTable[index];
                m_spellingHashTable[index] = pspellingCur;

            }
            else
            {
                pspellingPrev = pspellingCur;
            }

            pspellingCur = pspellingNext;
        }
    } while (ispellingTablePos > 0);

    return true;
}


#if 0

//============================================================================
// Dump the hash tables.
//============================================================================

void StringPool::DumpStringTableMemoryUse()
{
    unsigned iBucket, cBucketsFull = 0, cListMax = 0, cList;
    Casing *pspelling;

    for (iBucket = 0; iBucket < m_ulSpellingHashTableSize; iBucket++)
    {

        pspelling = m_spellingHashTable[iBucket];

        if (pspelling != NULL)
        {
            cBucketsFull++;
            cList = 0;

            for (; pspelling; pspelling = pspelling->m_pspellingNext)
            {
                cList++;
            }

            if (cList > cListMax)
            {
                cListMax = cList;
            }
        }
    }

    //
    // The string pool.
    //

    DebPrintf("\nThe string pool table contains:\n\n");

    DebPrintf("  %ld names with a total of %ld different spellings\n", m_ulNameCount, m_ulSpellingCount);
    DebPrintf("  %ld buckets, of which %ld are used\n", m_ulSpellingHashTableSize, cBucketsFull);
    DebPrintf("  Average used bucket length of %ld nodes, with a maximum length of %ld\n", m_ulSpellingCount / cBucketsFull, cListMax);
    DebPrintf("  Current table size is %ld entries\n", m_ulSpellingHashTableSize);

    DebPrintf("\nString pool stats so far:\n\n");

//    DebPrintf("  %ld calls to the string pool\n", m_cCalls);
//    DebPrintf("  %ld probes into the spelling table, %ld of which required a string compare (%ld failed)\n", m_cSpellingProbes, m_cDeepSpellingProbes, m_cFailedDeepSpellingProbes);
//    DebPrintf("  %ld lookups did not match in the spelling table\n", m_cStringAttempts);
//    DebPrintf("  %ld probes into the string table, %ld of which required a string compare\n", m_cStringProbes, m_cDeepStringProbes);
    DebPrintf("  Current table size is %ld entries\n", m_ulStrInfoHashTableSize);

    DebPrintf("\nMemory Usage:\n\n");

//    DebPrintf("  %ld bytes used by strings and first spellings\n", m_cStringMemory);
//    DebPrintf("  %ld bytes used by additional spellings\n", m_cAddtlSpellingMemory);
#if DEBUG
//    DebPrintf("  %ld bytes overhead in strinfo and spelling structures\n", m_cNonStringMemory-m_cDebugOnlyMemory);
//    DebPrintf("  %ld extra bytes being used becuase this is a debug build\n", m_cDebugOnlyMemory);
#else
    DebPrintf("  %ld bytes overhead in strinfo and spelling structures\n", m_cNonStringMemory);
#endif
    DebPrintf("  %ld bytes overhead in hash tables\n", (m_ulSpellingHashTableSize+m_ulStrInfoHashTableSize)*sizeof(VOID*));

    DebPrintf("\nSpelling table:\n\n");

    for (iBucket = 0; iBucket < m_ulSpellingHashTableSize; iBucket++)
    {

        pspelling = m_spellingHashTable[iBucket];

        if (pspelling != NULL)
        {
            DebPrintf("  Bucket #%ld:\n\n", iBucket);

            for (; pspelling; pspelling = pspelling->m_pspellingNext)
            {
                DebPrintf(L"      %s\n", pspelling->m_str);
            }

            DebPrintf("\n");
        }
    }

    DebPrintf("\nString table:\n\n");

    STRING_INFO *pstrinfo;

    for (iBucket = 0; iBucket < m_ulStrInfoHashTableSize; iBucket++)
    {

        pstrinfo = m_strinfoHashTable[iBucket];

        if (pstrinfo)
        {

            DebPrintf("  Bucket #%ld:\n\n", iBucket);

            for (; pstrinfo; pstrinfo = pstrinfo->m_pstrinfoNext)
            {
                DebPrintf(L"      %s\n", pstrinfo->m_spelling.m_str);
            }

            DebPrintf("\n");
        }
    }
}
#endif 0
#include "atlsafe.h"

SAFEARRAY * StringPool::GetStringPoolData()
{
    CComSafeArray<BSTR> saRetval;
    
    unsigned iBucket;
    Casing *pspelling;
#define TEMPBUFSIZE   2048* sizeof(WCHAR)      
    WCHAR szBuffer[TEMPBUFSIZE];
    const WCHAR* szFmt =  L"%c,%6d,%08x,%08x,%7d,%4d,%3d,";

    CompilerIdeLock lock(m_CriticalSection);

    for (iBucket = 0; iBucket < m_ulSpellingHashTableSize; iBucket++)
    {

        pspelling = m_spellingHashTable[iBucket];

        for (; pspelling; pspelling = pspelling->m_pspellingNext)
        {
            //"C" for Casing
            // make them fixed length beginning and 
            // put the actual string at the end after the length to accomodate strings with embedded CRs
            VSASSERT(pspelling->m_cchLength < 1000,"string too long");
            StringCchPrintfW(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]),szFmt,
                    'C',
                    iBucket,
                    pspelling->m_ulSpellingHash,
                    pspelling->m_pstrinfo,
                    0, // align with m_ulLocalHash below
                    0, // align with m_MatchingToken below
                    pspelling->m_cchLength
                    );
            CComBSTR bstrTemp(szBuffer);
            bstrTemp.Append(pspelling->m_str, pspelling->m_cchLength);
            saRetval.Add(bstrTemp);
        }
    }

    STRING_INFO *pstrinfo;

    for (iBucket = 0; iBucket < m_ulStrInfoHashTableSize; iBucket++)
    {

        pstrinfo = m_strinfoHashTable[iBucket];

        for (; pstrinfo; pstrinfo = pstrinfo->m_pstrinfoNext)
        {
            VSASSERT(pstrinfo->m_spelling.m_cchLength < 1000,"string too long for STRING_INFO");
            //"S" for StringInfo
            StringCchPrintfW(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), szFmt, 
                    'S',
                    iBucket,
                    pstrinfo->m_ulSysHash,
                    pstrinfo,  // align with m_pstrinfo above
                    pstrinfo->m_ulLocalHash,
                    pstrinfo->m_MatchingToken,
                    pstrinfo->m_spelling.m_cchLength
                    );
            CComBSTR bstrTemp(szBuffer);
            bstrTemp.Append(pstrinfo->m_spelling.m_str, pstrinfo->m_spelling.m_cchLength);
            saRetval.Add(bstrTemp);
        }

    }
    
    return saRetval.Detach(); 
}

//============================================================================
// These are called a lot, and so need speed optimization.
//============================================================================

//#pragma optimize("atw", on)

inline
int dmemcmp(
    const unsigned * pul1,
    const unsigned * pul2,
    size_t N,
    size_t r)
{

#ifdef _WIN64

    // both pul1 and pul2 may not be byte aligned.
    if (((size_t)pul1 & 0x3) || ((size_t)pul2 & 0x03)) //If not 4 byte aligned??
    {
        N = N << 1; //multiply by 2
        N += r;
        unsigned short *temppul1 = (unsigned short *) pul1;
        unsigned short *temppul2 = (unsigned short *) pul2;
        while (N && (*temppul1++ == *temppul2++)) N--;
        if (0 != N) return 1;
        return 0;
    }
    else // 4 byte aligned
    {
        for (; 0 < N; ++pul1, ++pul2, --N)
        {
            if (*pul1 != *pul2)
            {
                return 1;
            }
        }
    }
#else

    // do the long comparisons.
    for (; 0 < N; ++pul1, ++pul2, --N)
    {
        if (*pul1 != *pul2)
        {
            return 1;
        }
    }

#endif //_Win64

    if (r)
    {
        if (*((unsigned short *)pul1) != *((unsigned short *)pul2))
        {
            return 1;
        }
    }

    return 0;
}

inline void dmemcpy
(
    _Out_ unsigned *pul1,
    const unsigned *pul2,
    size_t N,
    size_t r
)
{
#ifdef _WIN64

    if (((size_t)pul1 & 0x3) || ((size_t)pul2 & 0x03)) //If not 4 byte aligned??
    {
        N = N <<1;
        N += r;
        unsigned short *temppul1 = (unsigned short *) pul1;
        unsigned short *temppul2 = (unsigned short *) pul2;
        while (N--)
            *temppul1++ = *temppul2++;
        return;
    }
    else
    {
        for (; 0 < N; ++pul1, ++pul2, --N)
        *pul1 = *pul2;
    }

#else

    for (; 0 < N; ++pul1, ++pul2, --N)
        *pul1 = *pul2;
#endif

    if (r)
        *((unsigned short *)pul1) = *((unsigned short *)pul2);
}

//#pragma optimize("atw", default)

//============================================================================
// Computes the hash value of a binary string.  This is used occasionally
// from outside the string put but not from within.
//============================================================================

unsigned long StringPool::ComputeHashValue(
    _In_ void * pv,
    size_t cbSize)
{
    BYTE *pb = (BYTE *)pv;
    BYTE *pbEnd = pb + cbSize;

    unsigned long hash = 0;

    while (pb != pbEnd)
    {
        BYTE b = *pb++;

        //
        // The following values are based off loading, but not compiling,
        // Pretty Printer.  The columns are:
        //
        //   buckets used (out of 1024)
        //   average hash list length
        //   maximum hash list length
        //   # of failed string compares
        //

//      hash = _lrotl(hash, 2) + g_rgulHash[b];                   // 932, 3, 21, 0
//      hash = _lrotl(hash, 2) + b;                               // 916, 3, 22, 1478
//      hash = _lrotl(hash, 2) + g_rgulHash[(b + hash) & 0xFF];   // 615, 4, 18, 0
//      hash = _lrotl(hash, 2) + b * 0x10204081;                  // 967, 2, 9, 0
        hash = _lrotl(hash, 1) + b * 0x10204081;                  // 968, 2, 9, 0
//      hash = _lrotl(hash, 2) + b * 0x00204081;                  // 954, 3, 9, 0
    }

    return hash;
}

//============================================================================
// Computes the hash value of a wchar string. You can choose whether you
// want the case sensitive or case insensitive hash value computed. The
// case insensitive hash value is computed using the Unicode 1-1 lowercase
// mappings (i.e. it is not locale sensitive). This is a ---- of a lot better
// than calling LHashValOfNameSysW which does horrific WideCharToMultiByte
// conversions.
//
// ***WARNING***
//
// The locking structure of StringPool takes a hard depenedency on the fact
// that this function is static / pure.  If this semantic ever changes we need
// to re-assess the locking structure because this method can and will execute
// in parallel
//============================================================================

inline
unsigned long _fastcall StringPool::ComputeStringHashValue(
    _In_ unsigned long * pl,
    size_t cl,
    size_t r,
    bool CaseSensitive)
{
    // We know that we can always treat this string as an array of
    // longs.  If the string is of odd length, we'll include the
    // terminating NUL in our calculations.  If it's even, we'll
    // skip it.
    //
    unsigned long hash = 0;
    unsigned long l =0 ;

#ifdef _WIN64
    bool IsNotAligned = (size_t) pl & 0x3;
#endif

    // Factor the 'if' out of the loop and repeat the loop inside.
    // The perf gain on this is worth the code maintentance headache.
    // Doing a conditional on CaseSensitive inside this loop was
    // hurting us.
    if (CaseSensitive)
    {
        for (; 0 < cl; -- cl, ++ pl)
        {
#ifdef _WIN64
            // Alignment problem.
            if (IsNotAligned)
            {
                unsigned short * pl1 = (unsigned short *)pl;
                l = *pl1;
                pl1++;
                l |= *pl1 << 16;
            }
            else
                l = *pl;
            // End of alignment modification.
#else
            l = *pl;
#endif

            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
        }

        if (r)
        {
            // simulate the null termination in the case it's not there.
            l = (unsigned)(*((unsigned short *)pl));
            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
        }
    }
    else    // !CaseSensitive
    {
        //
        // In most cases, we don't *really* need to call LowerCase in
        // this loop.  All we need to do is guarantee that the hash we
        // return is the same giiven either uppercase or lowercase input.
        // Given that 'A' == 0x41 and 'a' == 0x61, all we really
        // need to do is bit-or each character with 0x20.
        // Note that this changes the meaning of non-alpha chars,
        // but it doesn't matter!  We're just computing a hash!
        // To be even sleazier, we no longer have to do
        // this as separate operations on two separate WCHARS --
        // we can just modify both characters at once by or-ing
        // with 0x00200020.
        //
        // Note that we only apply this hack for ANSI characters.
        // Anything >=128 gets passed to LowerCase as before.
        // This is encapsulated in the SleazyLowerCasePair macro,
        // which takes a DWORD that contains two packed WCHARS.
        //
    // Port SP1 CL 2922610 to VS10
#define SleazyLowerCasePair(dwWchPair)         \
        (((dwWchPair) & 0xff80ff80) == 0 ?    \
            ((dwWchPair) | 0x00200020) :      \
            ((LowerCase((WCHAR)((dwWchPair) >> 16)) << 16) | LowerCase((WCHAR)((dwWchPair) & 0xffff))));

        // *************************************************
        // *************************************************
        // This loop should be *identical* to the above loop
        // except for uses of SleazyLowerCasePair.
        // *************************************************
        // *************************************************
        for (; 0 < cl; -- cl, ++ pl)
        {
#ifdef _WIN64
            // Alignment problem.
            if (IsNotAligned)
            {
                unsigned short * pl1 = (unsigned short *)pl;
                l = *pl1;
                pl1++;
                l |= *pl1 << 16;
                l = SleazyLowerCasePair(l);
            }
            else
                l = SleazyLowerCasePair(*pl);
            // End of alignment modification.
#else
            l = SleazyLowerCasePair(*pl);
#endif

            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
        }

        if (r)
        {
            // Simulate the null termination in the case it's not there.

            //Microsoft - DevDivBugs #
            l = SleazyLowerCasePair((*(unsigned short *)pl) | 0x00200000);

            hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
        }

#undef SleazyLowerCasePair

    }

    return hash;
}

//============================================================================
// Lookup a string without adding it if it isn't already there.
//============================================================================

STRING * StringPool::LookupStringWithLen(
    _In_count_(cchSize)const WCHAR * pwchar,
    size_t cchSize,
    bool isCaseSensitive)
{
    size_t clSizeLong = cchSize >> 1;
    size_t cchAfterLong = (cchSize) & 1;

    unsigned ulCompare, index, ulHash;
    Casing *pspelling;
    STRING_INFO *pstrinfo;
    STRING *pstr = NULL;

    //
    // Check the spelling hash table to see if we can do this via a fast
    // lookup.
    //

    ulHash = ComputeStringHashValue((unsigned long *)pwchar, clSizeLong, cchAfterLong, true);

    // Grab the lock after ComputeStringHashValue.  Profiles show that this method in particular 
    // is very expensive and accounts to up to 2/3 of the entire AddStringWithLength call.  It is 
    // a static method which does not touch any shared variables so it is safe to execute outside
    // the lock.  Putting it outside the lock allows for the most expensive part of the function
    // to execute in parallel
    // Lock after ComputeStringHashValue because 2/3 of the time is spent 
    CompilerIdeLock lock(m_CriticalSection);

    index = ulHash & (m_ulSpellingHashTableMask);
    ulCompare = GetCompareValue(cchSize, GetSignificantSpellingHashValue(ulHash));

    for (pspelling = m_spellingHashTable[index];
         pspelling;
         pspelling = pspelling->m_pspellingNext)
    {

#if FV_TRACK_MEMORY
        m_cSpellingProbes++;
#endif  // DEBUG

        // If this string doesn't even match case insensitively,
        // find something else.
        //
        if (pspelling->m_ulCompare != ulCompare)
        {
            continue;
        }

        // If we want a case sensitive match, check further to see if
        // we got it.

#if FV_TRACK_MEMORY
        m_cDeepSpellingProbes++;
#endif // DEBUG

        if (!dmemcmp((const unsigned *)pspelling->m_str, (const unsigned *)pwchar, clSizeLong, cchAfterLong))
        {
            pstr = pspelling->m_str;
            goto Done;
        }

#if FV_TRACK_MEMORY
        m_cFailedDeepSpellingProbes++;
#endif // DEBUG
    } // end for

    // We failed to find an exact case match, so now
    // try to find a case-insensitive match.
    if (!isCaseSensitive)
    {
        // Compute the hash value for the stringinfo
        ulHash = ComputeStringHashValue((unsigned long *)pwchar, clSizeLong, cchAfterLong, false);
        index = ulHash & (m_ulStrInfoHashTableMask);
        ulCompare = GetCompareValue(cchSize, GetSignificantSpellingHashValue(ulHash));

#if FV_TRACK_MEMORY
        m_cStringAttempts++;
#endif // DEBUG

        for (pstrinfo = m_strinfoHashTable[index];
            pstrinfo;
            pstrinfo = pstrinfo->m_pstrinfoNext)
        {

#if FV_TRACK_MEMORY
            m_cStringProbes++;
#endif // DEBUG

            if (pstrinfo->m_ulCompare != ulCompare)
            {
                continue;
            }

#if FV_TRACK_MEMORY
            m_cDeepStringProbes++;
#endif // DEBUG

            // compare the strings. Note that we use a local-insensitive compare here,
            // which is correct both from the standpoint of normal comparison, but also
            // from the standpoint of case sensitivity (i.e. we use the standard Unicode
            // 1-1 case mappings rather than dealing with local sensitive casing)
            if (!CompareNoCaseN(pwchar, pstrinfo->m_spelling.m_str, (int)cchSize))
            {
                pstr = pstrinfo->m_spelling.m_str;
                goto Done;
            }
        }
    } // end if

Done:

    return pstr;
}

//============================================================================
// Add a string to the hash table and returns a string.
//============================================================================

STRING * 
#if HOSTED
    __stdcall
#else
    _fastcall 
#endif
        StringPool::AddStringWithLen(
    _In_opt_count_(cchSize)const WCHAR * pwchar,
    size_t cchSize)
{
    // NULL?
    if (!pwchar)
    {
        return NULL;
    }

    size_t clSizeLong = cchSize >> 1;
    size_t cchAfterLong = (cchSize) & 1;

#if FV_TRACK_MEMORY
    m_cCalls++;
#endif // DEBUG

    unsigned ulCompare, ulSpCompare, index, indexSp, ulSpHash, ulHash;
    Casing *pspelling;
    STRING_INFO *pstrinfo;

    //
    // Check the spelling hash table to see if we can do this via a fast
    // lookup.
    //

    ulSpHash = ComputeStringHashValue((unsigned long *)pwchar, clSizeLong, cchAfterLong, true);

    // Grab the lock after ComputeStringHashValue.  Profiles show that this method in particular 
    // is very expensive and accounts to up to 2/3 of the entire AddStringWithLength call.  It is 
    // a static method which does not touch any shared variables so it is safe to execute outside
    // the lock.  Putting it outside the lock allows for the most expensive part of the function
    // to execute in parallel
    CompilerIdeLock lock(m_CriticalSection);

    indexSp = ulSpHash & (m_ulSpellingHashTableMask);
    ulSpCompare = GetCompareValue(cchSize, GetSignificantSpellingHashValue(ulSpHash));

    for (pspelling = m_spellingHashTable[indexSp];
         pspelling;
         pspelling = pspelling->m_pspellingNext)
    {

#if FV_TRACK_MEMORY
        m_cSpellingProbes++;
#endif // DEBUG

        if (pspelling->m_ulCompare != ulSpCompare)
        {
            continue;
        }

#if FV_TRACK_MEMORY
        m_cDeepSpellingProbes++;
#endif // DEBUG

        if (!dmemcmp((const unsigned *)pspelling->m_str, (const unsigned *)pwchar, clSizeLong, cchAfterLong))
        {
            return pspelling->m_str;
        }

#if FV_TRACK_MEMORY
        m_cFailedDeepSpellingProbes++;
#endif // DEBUG
    }

    //
    // Spelling does not exist.  Before we add one, see if there is
    // a stringinfo with the same case-insensitive spelling as
    // the one we're adding.
    //

    // Compute the hash value for the stringinfo
    ulHash = ComputeStringHashValue((unsigned long *)pwchar, clSizeLong, cchAfterLong, false);
    index = ulHash & (m_ulStrInfoHashTableMask);
    ulCompare = GetCompareValue(cchSize, GetSignificantSpellingHashValue(ulHash));

#if FV_TRACK_MEMORY
    m_cStringAttempts++;
#endif // DEBUG

    for (pstrinfo = m_strinfoHashTable[index];
        pstrinfo;
        pstrinfo = pstrinfo->m_pstrinfoNext)
    {

#if FV_TRACK_MEMORY
        m_cStringProbes++;
#endif // DEBUG

        if (pstrinfo->m_ulCompare != ulCompare)
        {
            continue;
        }

#if FV_TRACK_MEMORY
        m_cDeepStringProbes++;
#endif // DEBUG

        // compare the strings.
        if (!CompareNoCaseN(pwchar, pstrinfo->m_spelling.m_str, (int)cchSize))
        {
            break;
        }
    }

    //
    // Go ahead an add the new spelling.
    //

    // If we matched a string, add a new spelling.  We know that there is no existing
    // spelling for this or we would have matched in the first lookup above.
    //

    size_t cbSize;
    if (pstrinfo)
    {
        // Allocate the new spelling, aligning it on a 4-byte boundary.
        // Note: For 64 bit it is possible to allocate slightly more than necessary because the structure
        // end by an [0] array. For now, it doesn’t seem to be worth to  change.
        IfFalseThrow(cchSize + 1 >= 1);
        cbSize = VBMath::Add(VBMath::Multiply((cchSize + 1), sizeof(WCHAR)), sizeof(Casing));
        pspelling = (Casing *)m_nraStrings.AllocNonZero(cbSize);

#if FV_TRACK_MEMORY
        m_cAddtlSpellingMemory += ((cchSize + 1) * sizeof(WCHAR) + sizeof(Casing));
        m_cNonStringMemory += sizeof(Casing);
#if DEBUG
        m_cDebugOnlyMemory += sizeof(SZ_SIGNATURE);
#endif
#endif
    }

    // Otherwise, create a new stringinfo.
    else
    {
        // Alloc the memory, aligning it on a 4-byte boundary.
        IfFalseThrow(cchSize + 1 >= 1);
        cbSize = VBMath::Add(VBMath::Multiply((cchSize + 1), sizeof(WCHAR)), sizeof(STRING_INFO));
        pstrinfo = (STRING_INFO *)m_nraStrings.AllocNonZero(cbSize);

#if FV_TRACK_MEMORY
        m_cStringMemory += (cchSize + 1) * sizeof(WCHAR) + sizeof(STRING_INFO);
        m_cNonStringMemory += sizeof(STRING_INFO);
#if DEBUG
        m_cDebugOnlyMemory += sizeof(SZ_SIGNATURE);
#endif
#endif

        // Set it up.
        pstrinfo->m_ulCompare = ulCompare;
        pstrinfo->m_ulLocalHash = m_ulNameCount++;

        pstrinfo->m_UniqueNamespace = NULL;
        pstrinfo->m_MatchingToken =
        pstrinfo->m_DeclaredInModule = 
        pstrinfo->m_DeclaredInNamespace = 0;

        pstrinfo->m_pstrinfoNext = m_strinfoHashTable[index];
        m_strinfoHashTable[index] = pstrinfo;

        // Fix up the spelling.
        pspelling = &pstrinfo->m_spelling;
    }

    // fix up the back pointer.
    pspelling->m_pstrinfo = pstrinfo;

#if DEBUG
    // put the signature.
    strcpy_s(pspelling->m_pstrDebSignature, _countof(pspelling->m_pstrDebSignature), SZ_SIGNATURE);

#endif // DEBUG

    // copy the string
    dmemcpy((unsigned *)pspelling->m_str, (const unsigned *)pwchar, clSizeLong, cchAfterLong);
    pspelling->m_str[cchSize] = 0;

#if DEBUG && _X86_ 
    if ((ulSpCompare & 0xffff) != cchSize) // it's a union: pspelling->m_ulCompare 
    {
        // can't use VSASSERT: the background thread will continue
        _asm int 3
    }
#endif DEBUG
    // Save the extra info.
    pspelling->m_ulCompare = ulSpCompare;


    VSASSERT(pspelling->m_cchLength == cchSize, "Overflow.");
    VSASSERT(pspelling->m_ulCompare == ulSpCompare, "Overflow.");

    pspelling->m_pspellingNext = m_spellingHashTable[indexSp];
    m_spellingHashTable[indexSp] = pspelling;

    // update the counter
    m_ulSpellingCount++;

    // expand the tables if necessary
    if (m_ulSpellingCount > m_ulSpellingThreshold)
    {
        ExpandSpellingTable();

        // now check if we have to grow the strinfo table, too.  the only time we add
        // a strinfo we also add a spelling.
        //
        if (m_ulNameCount > m_ulStrInfoThreshold)
        {
            ExpandStrInfoTable();
        }
    }

    // return the string.
    return pspelling->m_str;
}

// Wrapper that checks whether two strings are case-insensatively equal.
bool IsEqual(
    _In_opt_z_ STRING * pstr1,
    _In_opt_z_ STRING * pstr2)
{
    return StringPool::IsEqual(pstr1, pstr2);
}

// Wrapper that checks whether two strings are case-sensatively equal.
bool IsEqualCaseSensitive(
    _In_opt_z_ STRING * pstr1,
    _In_opt_z_ STRING * pstr2)
{
    return StringPool::IsEqualCaseSensitive(pstr1, pstr2);
}

// Wrapper that checks whether two strings are case-insensatively equal.
bool IsEqual(
    _In_opt_z_ STRING * pstr1,
    _In_opt_z_ STRING * pstr2,
    bool IsCaseSensitive)
{
    return IsCaseSensitive ? StringPool::IsEqualCaseSensitive(pstr1, pstr2) : StringPool::IsEqual(pstr1, pstr2);
}

// Wrapper the gets the length of a string.
size_t StringLength(_In_z_ STRING * pstr)
{
    return StringPool::StringLength(pstr);
}

// Return a hash value.
unsigned long ComputeHashValue(
    _In_ void * pv,
    unsigned cbSize)
{
    return StringPool::ComputeHashValue(pv, cbSize);
}

void StringPool::AddDeclarationInNamespace(
    _In_z_ STRING * pstr,
    _In_ BCSYM_Namespace * pNamespace)
{
    STRING_INFO *Info = Pstrinfo(pstr);

    if (Info->m_DeclaredInNamespace)
    {
        if (pNamespace->GetNamespaceRing() != Info->m_UniqueNamespace)
        {
            // The name is declared in more than one namespace.
            Info->m_UniqueNamespace = NULL;
        }
    }

    else
    {
        Info->m_DeclaredInNamespace = true;

        if (!Info->m_DeclaredInModule)
        {
            Info->m_UniqueNamespace = pNamespace->GetNamespaceRing();
        }
    }
}

bool StringPool::CanBeDeclaredInNamespace(
    _In_z_ STRING * pstr,
    _In_ BCSYM_Namespace * pNamespace)
{
    STRING_INFO *Info = Pstrinfo(pstr);

    // A name can be assumed undeclared in a namespace if the name is
    // not declared in any modules, and the name is declared in no
    // namespaces or in only one and that one is not this one.

    return
        (Info->m_DeclaredInNamespace &&
         (Info->m_UniqueNamespace == NULL ||
          Info->m_UniqueNamespace == pNamespace->GetNamespaceRing())) ||
        Info->m_DeclaredInModule;
}
