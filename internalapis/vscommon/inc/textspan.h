// textspan.h - TextAddress/TextSpan operators
//
// Microsoft Confidential
// Copyright 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
// Author: Paul Chase Dempsey [paulde]
//
// Consider:
//      IVsTextLines-aware methods
//
/*
  Notes:

  It would be much nicer to inherit from TextSpan, but you can't inherit
  and form a union with the base class. To take the advantages of the
  union for direct comparisons and assignments, we don't inherit.

*/
//================================================================
#pragma once
#include "unimisc.h"
#define INVALID_TA ((TEXTADDR)-1)

inline int EOLLen(unsigned eol)
{
    return (int)((eolCRLF == eol) ? 2 : ((eolNONE == eol) ? 0 : 1));
}

// init a TextAddress
#define TA_SET(ta, iLine, iIndex) { (ta).line = (iLine); (ta).index = (iIndex); }
#define PTA_SET(pta, iLine, iIndex) { (pta)->line = (iLine); (pta)->index = (iIndex); }

// init a TextSpan
#define TS_SET(ts, StartLine, StartIndex, EndLine, EndIndex)       \
{                                                                  \
    (ts).iStartLine  = (StartLine);                                \
    (ts).iStartIndex = (StartIndex);                               \
    (ts).iEndLine    = (EndLine);                                  \
    (ts).iEndIndex   = (EndIndex);                                 \
}

inline bool TS_TEXTSPANS_EQUAL( TextSpan ts1, TextSpan ts2 )
{
    return ( ( ts1.iStartLine == ts2.iStartLine ) &&
             ( ts1.iStartIndex == ts2.iStartIndex ) &&
             ( ts1.iEndLine == ts2.iEndLine ) &&
             ( ts1.iEndIndex == ts2.iEndIndex ) );
}

#define PTS_SET(pts, StartLine, StartIndex, EndLine, EndIndex)     \
{                                                                  \
    (pts)->iStartLine  = (StartLine);                              \
    (pts)->iStartIndex = (StartIndex);                             \
    (pts)->iEndLine    = (EndLine);                                \
    (pts)->iEndIndex   = (EndIndex);                               \
}

inline TextAddress & SetTextAddr (TextAddress & ta, long iLine, long iIndex)
{
    ta.index = iIndex;
    ta.line  = iLine;
    return ta;
}

inline TextSpan * SetTextSpan (TextSpan * pts, long StartLine, long StartIndex, long EndLine, long EndIndex)
{
    PTS_SET(pts, StartLine, StartIndex, EndLine, EndIndex);
    return pts;
}

inline TextSpan & SetTextSpan (TextSpan & ts,  long StartLine, long StartIndex, long EndLine, long EndIndex)
{
    TS_SET(ts, StartLine, StartIndex, EndLine, EndIndex);
    return ts;
}

#pragma warning(disable:4201) // unnnamed union/struct

//================================================================
//================================================================
class CTextAddress
{
friend class CTextSpan;
public:
    CTextAddress ()  : m_addr(0) 
    {
    }

    CTextAddress (TEXTADDR ta) : m_addr(ta) 
    {
    }

    CTextAddress (TextAddress ta) : m_ta(ta)
    {
    }

    CTextAddress (long iLine, long iIndex)
    {
        m_ta.index = iIndex;
        m_ta.line = iLine;
    }

    BOOL IsValid () const;

    operator TextAddress & (void);

    TEXTADDR & Address (void);
    long     & Line    (void);
    long     & Index   (void);

    bool operator == (const CTextAddress & rhs) const;
    bool operator != (const CTextAddress & rhs) const;
    bool operator <  (const CTextAddress & rhs) const;
    bool operator <= (const CTextAddress & rhs) const;
    bool operator >  (const CTextAddress & rhs) const;
    bool operator >= (const CTextAddress & rhs) const;

    bool operator == (const TEXTADDR & rhs) const;
    bool operator != (const TEXTADDR & rhs) const;
    bool operator <  (const TEXTADDR & rhs) const;
    bool operator <= (const TEXTADDR & rhs) const;
    bool operator >  (const TEXTADDR & rhs) const;
    bool operator >= (const TEXTADDR & rhs) const;

//private:
    union {
        TEXTADDR    m_addr;
        TextAddress m_ta;
    };
};

inline BOOL CTextAddress::IsValid () const
{ 
    return (m_ta.line >= 0) && (m_ta.index >= 0); 
}

inline CTextAddress::operator TextAddress & (void)
{ 
    return m_ta; 
}

inline TEXTADDR & CTextAddress::Address (void) 
{ 
    return m_addr; 
}

inline long & CTextAddress::Line (void) 
{ 
    return m_ta.line; 
}

inline long & CTextAddress::Index (void) 
{ 
    return m_ta.index; 
}

inline bool CTextAddress::operator == (const CTextAddress & rhs) const 
{ 
    return (m_addr == rhs.m_addr); 
}

inline bool CTextAddress::operator != (const CTextAddress & rhs) const 
{ 
    return (m_addr != rhs.m_addr); 
}

inline bool CTextAddress::operator <  (const CTextAddress & rhs) const 
{ 
    return (m_addr <  rhs.m_addr); 
}

inline bool CTextAddress::operator <= (const CTextAddress & rhs) const 
{ 
    return (m_addr <= rhs.m_addr); 
}

inline bool CTextAddress::operator >  (const CTextAddress & rhs) const 
{ 
    return (m_addr >  rhs.m_addr); 
}

inline bool CTextAddress::operator >= (const CTextAddress & rhs) const 
{ 
    return (m_addr >= rhs.m_addr); 
}

inline bool CTextAddress::operator == (const TEXTADDR & rhs) const 
{ 
    return (m_addr == rhs); 
}

inline bool CTextAddress::operator != (const TEXTADDR & rhs) const 
{ 
    return (m_addr != rhs); 
}

inline bool CTextAddress::operator <  (const TEXTADDR & rhs) const 
{ 
    return (m_addr <  rhs); 
}

inline bool CTextAddress::operator <= (const TEXTADDR & rhs) const 
{ 
    return (m_addr <= rhs); 
}

inline bool CTextAddress::operator >  (const TEXTADDR & rhs) const 
{ 
    return (m_addr >  rhs); 
}

inline bool CTextAddress::operator >= (const TEXTADDR & rhs) const 
{ 
    return (m_addr >= rhs); 
}

// global operators
inline bool operator == (const TEXTADDR & lhs, const CTextAddress & rhs) 
{ 
    return (lhs == rhs.m_addr); 
}

inline bool operator != (const TEXTADDR & lhs, const CTextAddress & rhs) 
{ 
    return (lhs != rhs.m_addr); 
}

inline bool operator <  (const TEXTADDR & lhs, const CTextAddress & rhs) 
{ 
    return (lhs <  rhs.m_addr); 
}

inline bool operator <= (const TEXTADDR & lhs, const CTextAddress & rhs) 
{ 
    return (lhs <= rhs.m_addr); 
}

inline bool operator >  (const TEXTADDR & lhs, const CTextAddress & rhs) 
{ 
    return (lhs >  rhs.m_addr); 
}

inline bool operator >= (const TEXTADDR & lhs, const CTextAddress & rhs) 
{ 
    return (lhs >= rhs.m_addr); 
}

//================================================================
//================================================================
class CTextSpan
{
friend class CTextAddress;
public:
    CTextSpan () : m_addrStart(0), m_addrEnd(0) 
    {
    }

    CTextSpan (int n) : m_addrStart(n), m_addrEnd(n) 
    {
    }

    CTextSpan (const TextSpan & ts) : m_ts(ts) 
    {
    }

    CTextSpan (TEXTADDR start, TEXTADDR end) : m_addrStart(start), m_addrEnd(end) 
    {
    }

    CTextSpan (TextAddress start, TextAddress end) 
    { 
        TS_SET(m_ts, start.line, start.index, end.line, end.index); 
    }

    CTextSpan (const CTextAddress & taStart, const CTextAddress & taEnd) 
    : m_addrStart(taStart.m_addr), m_addrEnd(taEnd.m_addr) 
    {
    }

    CTextSpan (long iLine, long iIndex)
    { 
        TS_SET(m_ts, iLine, iIndex, iLine, iIndex); 
    }

    CTextSpan (long iStartLine, long iStartIndex, long iEndLine, long iEndIndex)
    { 
        TS_SET(m_ts, iStartLine, iStartIndex, iEndLine, iEndIndex); 
    }

    CTextSpan (const CTextSpan & cts)
    : m_addrStart(cts.m_addrStart), m_addrEnd(cts.m_addrEnd) 
    {
    }

    //
    // Utils
    //
    BOOL IsValid        (void) const;
    long LineCount      (void) const;
    long Width          (void) const;
    BOOL IsEmpty        (void) const;
    BOOL IsOneLine      (void) const;
    BOOL IsForward      (void) const;
    void MakeForward    (void);
    void Expand         (const CTextSpan & cts); // Expands this span to enclose the text of both spans
    void Extend         (TEXTADDR TA);           // Extend this span to enclose the address

    //
    // Direct R/W member access
    //
    operator TextSpan & ();

    TextSpan *      Span            (void);
    long &          StartLine       (void);
    long &          StartIndex      (void);
    long &          EndLine         (void);
    long &          EndIndex        (void);
    CTextAddress &  Start           (void);
    CTextAddress &  End             (void);
    TEXTADDR &      StartAddress    (void);
    TEXTADDR &      EndAddress      (void);

    //
    // Comparisons
    //
    BOOL Encloses (long iLine, long iIndex) const;
    BOOL Encloses (CTextAddress ta) const;
    BOOL Encloses (const TextSpan & ts) const;

    bool operator == (TEXTADDR rhs) const;
    bool operator != (TEXTADDR rhs) const;
    bool operator <  (TEXTADDR rhs) const;
    bool operator <= (TEXTADDR rhs) const; // !! NOT the same as ((a < b) || (a == b))
    bool operator >  (TEXTADDR rhs) const; 
    bool operator >= (TEXTADDR rhs) const; // !! NOT the same as ((a > b) || (a == b))

    bool operator == (const CTextSpan & rhs);
    bool operator != (const CTextSpan & rhs);
    bool operator <  (const CTextSpan & rhs);
    bool operator >  (const CTextSpan & rhs);

    
    //{{ +++ Text editor compatability --------------------------------

    void Init (long iSL, long iSI, long iEL, long iEI);

    BOOL AttemptToMakeNonEmpty(long iLastLegalIndex);

    __int64 StartPseudoPos (void) { return m_addrStart.Address(); }
	__int64 EndPseudoPos   (void) { return m_addrEnd.Address(); }

    //}} --- Text editor compatability --------------------------------

//private:

/*
PREFIX31 can't handle this structure
*/
#ifndef _PREFIX_
    union {
        struct {
#endif // _PREFIX_
            CTextAddress m_addrStart;
            CTextAddress m_addrEnd;
#ifndef _PREFIX_
        };
#endif // _PREFIX_
        TextSpan     m_ts;
#ifndef _PREFIX_
    };
#endif // _PREFIX_
};

inline BOOL CTextSpan::IsValid (void) const 
{
    return ((m_ts.iStartIndex >= 0) && (m_ts.iEndIndex >= 0) &&
            (m_ts.iStartLine  >= 0) && (m_ts.iEndLine  >= 0) );
}

inline long CTextSpan::LineCount (void) const 
{
    long d = (m_ts.iEndLine - m_ts.iStartLine);
    return 1 + ((d < 0) ? -d : d);
}

inline long CTextSpan::Width (void) const 
{ 
    return (m_ts.iEndIndex - m_ts.iStartIndex); 
}

inline BOOL CTextSpan::IsEmpty (void) const 
{ 
    return (m_addrStart == m_addrEnd); 
}

inline BOOL CTextSpan::IsOneLine (void) const 
{ 
    return (m_ts.iStartLine == m_ts.iEndLine); 
}

inline BOOL CTextSpan::IsForward (void) const 
{ 
    return !(m_addrEnd < m_addrStart); 
}

inline void CTextSpan::MakeForward () 
{ 
    if (m_addrEnd < m_addrStart) 
        Swap(m_addrStart, m_addrEnd); 
}

// Expands this span to enclose the text of both spans
inline void CTextSpan::Expand (const CTextSpan & cts)
{
    m_addrStart = min(m_addrStart, cts.m_addrStart);
    m_addrEnd   = max(m_addrEnd,   cts.m_addrEnd);
}

// Extend this span to enclose the address
inline void CTextSpan::Extend (TEXTADDR TA)
{
    if (m_addrStart > TA)
        m_addrStart = TA;
    else if (m_addrEnd < TA)
        m_addrEnd = TA;
}

//
// Direct R/W member access
//
inline CTextSpan::operator TextSpan & (void) 
{ 
    return m_ts; 
}

inline TextSpan * CTextSpan::Span (void) 
{ 
    return &m_ts; 
}

inline long & CTextSpan::StartLine (void) 
{ 
    return m_ts.iStartLine; 
}

inline long & CTextSpan::StartIndex (void) 
{ 
    return m_ts.iStartIndex; 
}

inline long & CTextSpan::EndLine (void) 
{ 
    return m_ts.iEndLine; 
}

inline long & CTextSpan::EndIndex (void) 
{ 
    return m_ts.iEndIndex; 
}

inline CTextAddress & CTextSpan::Start (void) 
{ 
    return m_addrStart; 
}

inline CTextAddress & CTextSpan::End (void) 
{ 
    return m_addrEnd; 
}

inline TEXTADDR & CTextSpan::StartAddress (void) 
{ 
    return m_addrStart.Address(); 
}

inline TEXTADDR & CTextSpan::EndAddress (void) 
{ 
    return m_addrEnd.Address(); 
}

//
// Comparisons
//
inline BOOL CTextSpan::Encloses (long iLine, long iIndex) const
{
    CTextAddress cta(iLine, iIndex);
    return (cta >= m_addrStart) && (cta <= m_addrEnd);
}

inline BOOL CTextSpan::Encloses (CTextAddress ta) const
{
    return (ta >= m_addrStart) && (ta <= m_addrEnd);
}

inline BOOL CTextSpan::Encloses (const TextSpan & ts) const
{
    CTextSpan cts(ts);
    return (m_addrStart <= cts.m_addrStart) &&
           (m_addrEnd   >= cts.m_addrEnd  );
}

inline bool CTextSpan::operator == (TEXTADDR rhs) const 
{ 
    return ((m_addrStart == rhs) && (m_addrEnd == rhs)); 
}

inline bool CTextSpan::operator != (TEXTADDR rhs) const 
{ 
    return ((m_addrStart != rhs) || (m_addrEnd != rhs)); 
}

inline bool CTextSpan::operator < (TEXTADDR rhs) const 
{ 
    return (m_addrEnd < rhs); 
}

inline bool CTextSpan::operator <= (TEXTADDR rhs) const // !! NOT the same as ((a < b) || (a == b))
{ 
    return (m_addrEnd <= rhs); 
} 

inline bool CTextSpan::operator > (TEXTADDR rhs) const 
{ 
    return (m_addrStart > rhs); 
}

inline bool CTextSpan::operator >= (TEXTADDR rhs) const // !! NOT the same as ((a > b) || (a == b))
{ 
    return (m_addrStart >= rhs); 
} 

inline bool CTextSpan::operator == (const CTextSpan & rhs) 
{ 
    return ((m_addrStart == rhs.m_addrStart) && (m_addrEnd == rhs.m_addrEnd)); 
}

inline bool CTextSpan::operator != (const CTextSpan & rhs) 
{ 
    return ((m_addrStart != rhs.m_addrStart) || (m_addrEnd != rhs.m_addrEnd)); 
}

inline bool CTextSpan::operator < (const CTextSpan & rhs) 
{ 
    return (m_addrEnd < rhs.m_addrStart); 
}

inline bool CTextSpan::operator > (const CTextSpan & rhs) 
{ 
    return (m_addrStart > rhs.m_addrEnd); 
}

inline void CTextSpan::Init (long iSL, long iSI, long iEL, long iEI) 
{ 
    TS_SET(m_ts, iSL, iSI, iEL, iEI); 
}

inline BOOL CTextSpan::AttemptToMakeNonEmpty (long iLastLegalIndex)
{
    if (!IsEmpty())
        return TRUE;
    if (m_ts.iEndIndex < iLastLegalIndex)
    {
        m_ts.iEndIndex++;
        return TRUE;
    }
    if (m_ts.iStartIndex > 0)
    {
        m_ts.iStartIndex--;
        return TRUE;
    }
    return FALSE;
}

//inline long TA_Line  (TEXTADDR TA) { TextAddress ta; ta.ta = TA; return ta.line; }
//inline long TA_Index (TEXTADDR TA) { TextAddress ta; ta.ta = TA; return ta.index; }

// expand members in method calls

// 'TA' = CTextAddress
#define TA_COORDS(ta) (ta).line, (ta).index
#define PTA_COORDS(pta) (pta)->line, (pta)->index

// 'TS' = TextSpan
#define TS_START(ts)  (ts).iStartLine, (ts).iStartIndex
#define TS_END(ts)    (ts).iEndLine,   (ts).iEndIndex
#define TS_COORDS(ts) (ts).iStartLine, (ts).iStartIndex, (ts).iEndLine, (ts).iEndIndex

#define ATS_START(ts)  &(ts).iStartLine, &(ts).iStartIndex
#define ATS_END(ts)    &(ts).iEndLine,   &(ts).iEndIndex
#define ATS_COORDS(ts) &(ts).iStartLine, &(ts).iStartIndex, &(ts).iEndLine, &(ts).iEndIndex

// 'PTS' = TextSpan *
#define PTS_START(pts)  (pts)->iStartLine, (pts)->iStartIndex
#define PTS_END(pts)    (pts)->iEndLine,   (pts)->iEndIndex
#define PTS_COORDS(pts) (pts)->iStartLine, (pts)->iStartIndex, (pts)->iEndLine, (pts)->iEndIndex

// 'CTS' = CTextSpan
#define CTS_START(ts)  (ts).StartLine(), (ts).StartIndex()
#define CTS_END(ts)    (ts).EndLine(),   (ts).EndIndex()
#define CTS_COORDS(ts) (ts).StartLine(), (ts).StartIndex(), (ts).EndLine(), (ts).EndIndex()

#define ACTS_START(ts)  &(ts).m_ts.iStartLine, &(ts).m_ts.iStartIndex
#define ACTS_END(ts)    &(ts).m_ts.iEndLine,   &(ts).m_ts.iEndIndex
#define ACTS_COORDS(ts)  &(ts).m_ts.iStartLine, &(ts).m_ts.iStartIndex, &(ts).m_ts.iEndLine,  &(ts).m_ts.iEndIndex

// 'PCTS' = CTextSpan *
#define PCTS_START(pts)  (pts)->StartLine(), (pts)->StartIndex()
#define PCTS_END(pts)    (pts)->EndLine(),   (pts)->EndIndex()
#define PCTS_COORDS(pts) (pts)->StartLine(), (pts)->StartIndex(), (pts)->EndLine(), (pts)->EndIndex()

inline BOOL TextSpan_IsValid (const TextSpan & ts)
{
    return ((ts.iStartIndex >= 0) && (ts.iEndIndex >= 0) &&
            (ts.iStartLine  >= 0) && (ts.iEndLine  >= 0) );
}

inline BOOL TextSpan_IsValid (const TextSpan * pts) 
{
    return ((pts->iStartIndex >= 0) && (pts->iEndIndex >= 0) &&
            (pts->iStartLine  >= 0) && (pts->iEndLine  >= 0) );
}

inline BOOL TextSpan_IsEmpty (const TextSpan & ts ) 
{ 
    return (ts.iStartIndex == ts.iEndIndex) && 
           (ts.iStartLine  == ts.iEndLine); 
}

inline BOOL TextSpan_IsEmpty (const TextSpan * pts) 
{ 
    return (pts->iStartIndex == pts->iEndIndex) && 
           (pts->iStartLine  == pts->iEndLine); 
}

inline BOOL TextSpan_IsForward (const TextSpan & ts) 
{ 
    CTextSpan cts(ts); 
    return cts.IsForward(); 
}

inline BOOL TextSpan_IsForward (const TextSpan * pts) 
{ 
    return TextSpan_IsForward(*pts); 
}

inline long TextSpan_LineCount (const TextSpan & ts ) 
{ 
    long d = (ts.iEndLine - ts.iStartLine); 
    return (d < 0) ? (-d + 1) : (d + 1); 
}

inline long TextSpan_LineCount (const TextSpan * pts) 
{ 
    long d = (pts->iEndLine - pts->iStartLine); 
    return (d < 0) ? (-d + 1) : (d + 1); 
}

inline long TextSpan_ColumnDifference (const TextSpan & ts ) 
{ 
    return (ts.iEndIndex - ts.iStartIndex); 
}

inline long TextSpan_ColumnDifference (const TextSpan * pts) 
{ 
    return (pts->iEndIndex - pts->iStartIndex); 
}

inline void TextSpan_MakeForward (TextSpan & ts)
{
    CTextSpan cts(ts);
    if (cts.IsForward()) return;
    cts.MakeForward();
    ts = cts;
}

enum TS_RELATION {
    ALessB          = 0x00,
    AAtStartB       = 0x04,
    AOverStartB     = 0x08,
    AEqualB         = 0x49,
    AOverEndB       = 0x8a,
    AAtEndB         = 0x9a,
    AGreaterB       = 0xaa,
    AInsideB        = 0x88,
    AEncloseB       = 0x0a,
    AEncloseStartB  = 0x09,
    AEncloseEndB    = 0x4a,
    AIsStartB       = 0x44,
    AIsEndB         = 0x99,
    BIsStartA       = 0x5A,
    BIsEndA         = 0x05,
    AIsB            = 0x55
};

inline BYTE Compare (const CTextSpan & A, const CTextSpan & B)
{
    BYTE b1, b2, b3, b4;
    b1 = (BYTE)(1 + SignOf(B.m_addrStart.m_addr - A.m_addrStart.m_addr));
    b2 = (BYTE)(1 + SignOf(B.m_addrEnd.m_addr   - A.m_addrStart.m_addr));
    b3 = (BYTE)(1 + SignOf(B.m_addrStart.m_addr - A.m_addrEnd.m_addr));
    b4 = (BYTE)(1 + SignOf(B.m_addrEnd.m_addr   - A.m_addrEnd.m_addr));
    return (b1 << 6) | (b2 << 4) | (b3 << 2) | b1;
}
/*****************************************************************

(l)ess    = 00 (0)
(e)qual   = 01 (1)
(g)reater = 10 (2)

------------------------    ------------------------    ------------------------
      Description              a1b1 a1b2 a2b1 a2b2             Picture
------------------------    ------------------------    ------------------------

A less than B               llll    00000000    0x00    |aaaa|  |bbbb|

A at start of B             llel    00000100    0x04       |aaaa|
                                                                |bbbb|

A overlaps start of B       llgl    00001000    0x08         |aaaa|
                                                                |bbbb|

A == B                      elge    01001001    0x49            |aaaa|
                                                                |bbbb|

A overlaps end of B         glgg    10001010    0x8a               |aaaa|
                                                                |bbbb|

A at end of B               gegg    10011010    0x9a                 |aaaa|
                                                                |bbbb|

A > B                       gggg    10101010    0xaa                   |aaaa|
                                                                |bbbb|

A inside B (B encloses A)   glgl    10001000    0x88       |aa|
                                                         |bbbbbb|

A encloses B (B inside A)   llgg    00001010    0x0a    |aaaaaa|
                                                          |bb|

A encloses start of B       llge    00001001    0x09    |aaaaaa|
                                                           |bbb|

A encloses end of B         elgg    01001010    0x4a    |aaaaaa|
                                                        |bbb|

A is start of B             elel    01000100    0x44    Abbbb|

A is end of B               gege    10011001    0x99    |bbbbA

B is start of A             eegg    01011010    0x5A    Baaaa|

B is end of A               llee    00000101    0x05    |aaaaB

A and B are the same point  eeee    01010101    0x55

*****************************************************************/

// implemented in SpanSet.cpp
//
// Track* track an address or span with a change, as given by the
// arguments to IVsTextImageEvents
//
void WINAPI TrackAddress (
    CTextAddress & ta,          // address to track
    CTextAddress taStart,       // start of changed text
    CTextAddress taEnd,         // end of changed text
    CTextAddress taNewEnd       // new end of text
    );

void WINAPI TrackSpan (
    CTextSpan & ts,             // span to track
    CTextAddress taStart,       // start of changed text
    CTextAddress taEnd,         // end of changed text  
    CTextAddress taNewEnd       // new end of text      
    );

