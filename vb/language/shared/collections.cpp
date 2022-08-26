//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

unsigned long GetHashCode(
    const void * pKey,
    unsigned long size)
{
    Assume(size % WORD_BYTE_SIZE == 0,L"Must be 4 byte thing.");

    unsigned long hash = 0;
    unsigned long l;
    const unsigned long *pl = (const unsigned long *)pKey;

    size_t cl = size  / WORD_BYTE_SIZE;

    for (; 0 < cl; -- cl, ++ pl)
    {
        l = *pl;

        hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
    }

    return hash;
}

// This code is stolen from the string pool
unsigned long CaseInsensitiveUnicodeHashFunction::GetHashCode(const wchar_t * pString) const
{
    size_t len = wcslen(pString);
    size_t cl = len >> 1;
    size_t r = (len) & 1;
    const unsigned long * pl = reinterpret_cast<const unsigned long *>(pString);

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
        l = SleazyLowerCasePair((*(unsigned short *)pl) | 0x00200000);

        hash = _lrotl(hash, 2) + ((l >> 9) + l) * 0x10004001;     // 966, 2, 10, 0
    }

#undef SleazyLowerCasePair

    return hash;
}

bool CaseInsensitiveUnicodeHashFunction::AreEqual(
    const wchar_t * val1,
    const wchar_t * val2) const
{
    return( ::_wcsicmp( val1, val2 ) == 0 );
}

NegatedBitVector::NegatedBitVector(IReadonlyBitVector * pWrapped) :
    m_pWrapped(pWrapped)
{
    ThrowIfNull(pWrapped);
}

bool NegatedBitVector::BitValue(unsigned long index) const
{
    ThrowIfNull(m_pWrapped);
    return ! m_pWrapped->BitValue(index);
}

unsigned long NegatedBitVector::BitCount() const
{
    ThrowIfNull(m_pWrapped);
    return m_pWrapped->BitCount();
}

unsigned long NegatedBitVector::SetCount() const
{
    ThrowIfNull(m_pWrapped);
    return m_pWrapped->BitCount() - m_pWrapped->SetCount();
}
