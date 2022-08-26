// unimisc.h
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
// June 1, 1998 [paulde]
//

#ifdef _MSC_VER
#pragma once
#endif

#ifndef __UNIMISC_H__
#define __UNIMISC_H__

#include "commacro.h"   // COM macros moved to their own header

//---------------------------------------------------------------
// IN_RANGE - Test if v is in the range r1 to r2, inclusive
//
//#define IN_RANGE(v, r1, r2) ((r1) <= (v) && (v) <= (r2))
//
// This impl is slightly smaller with no branches
//
#define IN_RANGE(v, r1, r2) ((unsigned)((v) - (r1)) <= (unsigned)((r2)-(r1)))

template<class T> inline BOOL InRange(const T& v, const T &r1, const T &r2)
{
  //return ((r1 <= v) && (v <= r2));
  return (unsigned)(v - r1) <= (unsigned)(r2 - r1);
}

//---------------------------------------------------------------
// SignOf - get sign of a number
//
// Returns:
//      -1 = n is negative
//       0 = n is zero
//       1 = n is positive
//
template<class T> inline int SignOf(T n)
{
    return ((int)n < 0) ? -1 : n > 0;
}

//---------------------------------------------------------------
// Swap - exchange two values
//
template<class T> inline void Swap(T &a, T &b)
{
    T x = a;
    a = b;
    b = x;
}

//---------------------------------------------------------------
// PtrSwap - exchange two values through pointers
//
template<class T> inline void PtrSwap(T * a, T * b)
{
    T x = *a;
    *a = *b;
    *b = x;
}

//---------------------------------------------------------------
// IsAnsiText - test if some data appears to be valid ANSI text
//
// NOTE: returns FALSE for most Unicode text.
//
// Anything >= 32 is valid
//
// The OK text characters < 32 are 7,8,9,10,12,13,27  (bell, backspace, 
// tab, linefeed, formfeed, carriage-return, esc).
//
// We form a bitset from the char value and see if it intersects 
// the valid set 0x08003780.
//
inline BOOL IsAnsiText (DWORD cb, const BYTE * pb)
{
    while (cb--)
    {
        BYTE b = *pb++;
        if ((b < 32) && !((1UL << b) & 0x08003780))
            return FALSE;
    }
    return TRUE;
}

//---------------------------------------------------------------
// FillXXXX - larger-than-byte fills. Returns a pointer 1 element past the end.
// v for 'value', not 'void'
//
inline WCHAR *      FillWCHAR       (__out_ecount(cel) WCHAR      * pv, int cel, WCHAR    v) { while (cel-- > 0) *pv++ = v; return pv; }
inline WORD *       FillWord        (__out_ecount(cel) WORD       * pv, int cel, WORD     v) { while (cel-- > 0) *pv++ = v; return pv; }
inline DWORD *      FillDWORD       (__out_ecount(cel) DWORD      * pv, int cel, DWORD    v) { while (cel-- > 0) *pv++ = v; return pv; }
inline ULONG *      FillULONG       (__out_ecount(cel) ULONG      * pv, int cel, ULONG    v) { while (cel-- > 0) *pv++ = v; return pv; }
inline int *        FillInt         (__out_ecount(cel) int        * pv, int cel, int      v) { while (cel-- > 0) *pv++ = v; return pv; }
inline UINT *       FillUINT        (__out_ecount(cel) UINT       * pv, int cel, UINT     v) { while (cel-- > 0) *pv++ = v; return pv; }
inline UINT_PTR *   FillUINT_PTR    (__out_ecount(cel) UINT_PTR   * pv, int cel, UINT_PTR v) { while (cel-- > 0) *pv++ = v; return pv; }
#define FillWORD FillWord
#define FillINT  FillInt

#ifndef DELETEPTR
#define DELETEPTR(p)  \
{                     \
    if (p)            \
    {                 \
        delete p;     \
        p = NULL;     \
    }                 \
}
#endif

#ifndef DELETEARR
#define DELETEARR(p)  \
{                     \
    if (p)            \
    {                 \
        delete [] p;  \
        p = NULL;     \
    }                 \
}
#endif

#ifndef DELETEGDIOBJ
#define DELETEGDIOBJ(hobj)             \
{                                      \
    if (hobj)                          \
    {                                  \
        ::DeleteObject((HGDIOBJ)hobj); \
        hobj = NULL;                   \
    }                                  \
}
#endif

// Free BSTR and set to NULL
#ifndef SYSFREE
#define SYSFREE(bstr)         \
{                             \
    if (bstr)                 \
    {                         \
        SysFreeString(bstr);  \
        bstr = NULL;          \
    }                         \
}
#endif

#ifndef VSFREE
#define VSFREE(pv)   \
{                    \
    if (pv)          \
    {                \
        VSFree(pv);  \
        pv = NULL;   \
    }                \
}
#endif

#ifndef VSHEAPFREE
#define VSHEAPFREE(heap, pv)    \
{                               \
    if (pv)                     \
    {                           \
        VSHeapFree(heap, pv);   \
        pv = NULL;              \
    }                           \
}
#endif

#define VSALLOCTYPE(type, cel)        ( (cel > UINT_MAX / sizeof(type)) ? NULL : (type *)VSAlloc((cel)*sizeof(type)) )
#define VSALLOCTYPEZERO(type, cel)    (type *)VSAllocZero((cel)*sizeof(type))
#define VSREALLOCTYPE(type, ptr, cel) (type *)VSRealloc((ptr), (cel)*sizeof(type))
#define VSALLOCSTR(cch)               VSALLOCTYPE(WCHAR, (cch))

//---------------------------------------------------------------
// write optional return value
#ifndef SETRETVAL
#define SETRETVAL(pv, v) { if (pv) (*(pv)) = (v); }
#endif

//---------------------------------------------------------------
class CBufImpl
{
private:
    BYTE * m_pData;
    int    m_cb;
    
    HRESULT _SetByteSize (int cb);

public:
    CBufImpl() : m_pData(NULL), m_cb(0) {}
    ~CBufImpl()  { Clear(); }

    void    Clear             ();
    HRESULT SetByteSize       (int cb);
    HRESULT SetByteSizeShrink (int cb);
    int     GetByteSize       () { return m_cb; }
    BYTE *  ByteData          () { return m_pData; }
};

inline HRESULT CBufImpl::SetByteSize (int cb)
{
    if (cb <= m_cb)
        return S_OK;
    return _SetByteSize(cb);
}

//---------------------------------------------------------------
template <class T> class CMinimalArray : public CBufImpl
{
public:
    HRESULT SetSize       (int cel) { return SetByteSize(cel*sizeof(T)); }
    HRESULT SetSizeShrink (int cel) { return SetByteSizeShrink(cel*sizeof(T)); }
    int     Size    ()        { return GetByteSize()/sizeof(T); }
    operator T*     ()        { return (T*)ByteData(); }
    T*      GetData ()        { return (T*)ByteData(); }
};

typedef UNALIGNED PWSTR   UPWSTR;
typedef UNALIGNED PSTR    UPSTR;
typedef UNALIGNED int *   UPINT;
typedef UNALIGNED long *  UPLONG;
typedef UNALIGNED ULONG * UPULONG;
typedef UNALIGNED DWORD * UPDWORD;

//---------------------------------------------------------------
// CMinimalStream version 2
// 
// This version of CMinimalStream is more efficient in the case 
// where you can grow to a large size. It never reallocates during 
// string building. Instead, it chains blocks and coalesces to a 
// new block of memory only when you ask for a contiguous block
// (Gimme* or *Data).
// 

class CMSBlock; // CMinimalStream helper class

// Callback for CMinimalStream::Walk.
// Return S_FALSE or fail to stop the walk.
typedef HRESULT (CALLBACK * MSWALKFN) (UINT_PTR pUser, BYTE * pb, int cb); 

class CMinimalStream
{
public:
    enum { cbGrowByDefault = 512 };

    CMinimalStream() : m_cbGrowBy(cbGrowByDefault), m_cb(0), m_pos(0), m_pData(NULL), m_pCur(NULL) {}
    ~CMinimalStream();

    // Set byte size of memory blocks. Pass 0 to restore default
    void    GrowBy      (int cb) { m_cbGrowBy = ((cb > 0) ? cb : cbGrowByDefault); }
    void    Reset       (void); // releases all memory

    // Pre-size first block if none
    HRESULT EnsureInitialBlock (int cb = cbGrowByDefault);
    int     Recalc      (int * ppos = NULL); // calculate total data size and optionally, virtual position

    HRESULT Add         (const BYTE * pbData, int cb);
    HRESULT AddStringA  (PCSTR  psz, int cch = -1);
    HRESULT AddStringW  (PCWSTR psz, int cch = -1);
    HRESULT AddFillW    (WCHAR  ch,  int cch);
    HRESULT AddCharA    (char   ch) { return Add((const BYTE *)&ch, 1); }
    HRESULT AddCharW    (WCHAR  ch) { return AddStringW(&ch, 1); }

    int     Length      (void) { return m_pos; } // length in bytes
    BYTE *  End         (void);

    int     WLength     (void) { return m_pos/sizeof(WCHAR); } // length in WCHARs
    UPWSTR  WEnd        (void) { return (UPWSTR)End(); }

    // These let you treat the stream as a growable array.
    // If you need the stream 0-terminated, call Terminate() before *Data().
    //
    BYTE *   Data       (void);
    UPWSTR   WData      (void) { return (UPWSTR )Data(); }
    UPSTR    AData      (void) { return (UPSTR  )Data(); }
    UPINT    IData      (void) { return (UPINT  )Data(); }
    UPLONG   LData      (void) { return (UPLONG )Data(); }
    UPULONG  ULData     (void) { return (UPULONG)Data(); }
    UPDWORD  DWData     (void) { return (UPDWORD)Data(); }

    HRESULT Coalesce    (void); // coalesce everything into one contiguous block

    // 0-terminate the stream without incrementing the position.
    HRESULT Terminate    (int cbSize = sizeof(WCHAR)); 

    // 0-terminate the stream and increment the position.
    HRESULT AddTerminate (int cbSize = sizeof(WCHAR));

    // Walk visits the internal blocks in order until fnCallback fails, 
    // returns S_FALSE, or there are no more blocks. Returns S_OK if empty, 
    // otherwise the last value returned by fnCallback. fnCallback is passed 
    // pUser at each call.
    // You can use this to write a stream to a file without forcing a coalesce,
    // then Reset() to clear it.
    HRESULT Walk (MSWALKFN fnCallback, UINT_PTR pUser);

    // Gimme's hand out the accumulated buffer trimmed to size and, if Gimme*Str, 
    // null-terminated. The caller then owns the memory and is responsible for 
    // freeing it appropriately.
    //
    // The stream is always reset to accumulate a new buffer.
    // Get the size _before_ calling a Gimme* if you need it, because Gimme's always reset. 
    //
    PSTR    GimmeAStr   (void); // Free with VSFree
    PWSTR   GimmeWStr   (void); // Free with VSFree
    BSTR    GimmeBSTR   (void); // Free with SysFreeString
    BYTE *  GimmeData   (void); // Free with VSFree (not 0 - terminated)
    HGLOBAL GimmeHandle (UINT uFlags); // Free with GlobalFree, uFlags = GlobalAlloc flags, (not 0 - terminated)

private:
    int         m_cbGrowBy; // block size
    int         m_cb;       // total data size
    int         m_pos;      // virtual offset of current image end
    CMSBlock *  m_pData;    // start of chain
    CMSBlock *  m_pCur;     // current end
    CMSBlock *  NewBlock (int cb);
};

//---------------------------------------------------------------
// CMinimalStream formatted string functions.
//
// Wraps the CRT sprintf format functions (not Windows format functions).
// Not members so that they won't be linked (and pull in stdio) if you don't use 'em.
//
// NOTE: Total length of formatted result is limited to 1024 chars, including NUL.
//
HRESULT __cdecl MinStreamFormatA (CMinimalStream & ms, PCSTR  fmt, ...);
HRESULT __cdecl MinStreamFormatW (CMinimalStream & ms, PCWSTR fmt, ...);

//---------------------------------------------------------------
// MinStreamWalkWriteFile
//
// Use ms.Walk(MinStreamWalkWriteFile, (UINT_PTR)filehandle); to write
// a CMinimalStream to the open file handle given in pUser.
//
HRESULT CALLBACK MinStreamWalkWriteFile (UINT_PTR pUser, BYTE * pb, int cb); 

#endif // __UNIMISC_H__
