//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//-------------------------------------------------------------------------------------------------

#pragma once

namespace ILTree
{
    class ILNode;
}

class SourceFile;
class BCSYM;
enum CompilationState;

namespace ParseTree
{
    struct PunctuatorLocation;
};

struct Token;

#if IDE 

//Update location and offset for errors,tokens, and symbols
// Make sure you initialize all members, since this doesn't inherit from ZeroInt.
struct EditCache
{
    long iStartPos;
    long iReplacePos;
    long oAdjust;
    bool IsReplace;
};

class EditInfoLists;

#endif IDE

struct LocationPoint
{
    long Line;
    long Column;

    LocationPoint() : Line(0), Column(0) {}
    LocationPoint(long l, long c) : Line(l), Column(c) {}

    inline 
    int Compare( _In_ const LocationPoint& other) const
    {
        return LocationPoint::Compare(*this, other);
    }

    inline 
    bool IsBefore( _In_ const LocationPoint& other) const
    {
        return Compare(*this, other) < 0;
    }

    inline
    bool IsAfter( _In_ const LocationPoint& other) const
    {
        return Compare(*this, other) > 0;
    }

    inline static
    int Compare( _In_ const LocationPoint& left, _In_ const LocationPoint& right)
    {
        if ( left.Line == right.Line )
        {
            return left.Column - right.Column;
        }

        return left.Line - right.Line;
    }

    inline static
    int Compare( 
        _In_ long leftLine,
        _In_ long leftColumn,
        _In_ long rightLine,
        _In_ long rightColumn)
    {
        if ( leftLine == rightLine )
        {
            return leftColumn - rightColumn;
        }

        return leftLine - rightLine;
    }

    static 
    LocationPoint FromStartPoint( const TextSpan& span )
    {
        return LocationPoint(span.iStartLine, span.iStartIndex);
    }

    static 
    LocationPoint FromEndPoint( const TextSpan& span )
    {
        return LocationPoint(span.iEndLine, span.iEndIndex);
    }

    TextSpan ConvertToTextSpan() const
    {
        TextSpan sp;
        sp.iStartLine = sp.iEndLine = Line;
        sp.iStartIndex = sp.iEndIndex = Column;
        return sp;
    }
};

// The actual value the debugger uses in the pdb is 0xFEEFEE
// But this line value will get incremented before the actual
// store into the pdb, so we use 0xfeefed
const int HIDDEN_LINE_INDICATOR = 0xfeefed;

//============================================================================
// A generic location.  Stores the location of a range of text.
//============================================================================

struct Location
{
    long m_lBegLine;
    long m_lEndLine;
    long m_lBegColumn;
    long m_lEndColumn;

    LocationPoint GetStartPoint() const
    {
        return LocationPoint(m_lBegLine, m_lBegColumn);
    }

    LocationPoint GetEndPoint() const
    {
        return LocationPoint(m_lEndLine, m_lEndColumn);
    }

    //============================================================================
    // Get the information for the last line of this span
    //============================================================================
    Location GetLastLine() const
    {
        if ( IsSingleLine() )
        {
            return *this;
        }
        else
        {
            return Create(m_lEndLine, 0, m_lEndLine, m_lEndColumn);
        }
    }

    //============================================================================
    // The extended current location is considered to include one extra
    // column at the end position.
    //============================================================================
    Location GetExtended() const
    {
        Location locExtended = *this;
        locExtended.m_lEndColumn++;
        return locExtended;
    }

    //============================================================================
    // Invalidate the current location
    //============================================================================
    inline
    void Invalidate()
    {
        memset(this, 0, sizeof(*this));
    }

    bool IsAnyPartNegative() const
    {
        return m_lBegColumn < 0 
            || m_lBegLine < 0
            || m_lEndColumn < 0
            || m_lEndLine < 0;
    }

    //============================================================================
    // Is the current location invalid?
    //
    // Returns:
    //      -true: if the current location is invalid;
    //      -false: if otherwise
    //============================================================================
    inline
    bool IsInvalid() const
    {
        return (m_lBegLine == 0 && m_lEndLine == 0 && m_lBegColumn == 0 && m_lEndColumn == 0);
    }

    //============================================================================
    // Is the current location valid?
    //
    // Returns:
    //      -true: if the current location is valid;
    //      -false: if otherwise
    //============================================================================
    inline
    bool IsValid() const
    {
        return !IsInvalid();
    }

    //============================================================================
    // Is the current location hidden?
    //
    // Returns:
    //      -true: if the start line of the current location matches
    //             the hidden line indicator.
    //      -false: if otherwise
    //
    // Note:
    //      Synthetic codegen for WithEvents varialbes could require multiple closures
    //      since that code is hidden, the closure names might clash. m_lBegColumn is used
    //      to distinguish these closures. Ensure that this method does not take a dependecy
    //      on m_lBegColumn.
    //============================================================================
    inline
    bool IsHidden() const
    {
        return (m_lBegLine == HIDDEN_LINE_INDICATOR);
    }

    inline
    bool IsSingleLine() const
    {
        return m_lBegLine == m_lEndLine;
    }

    //============================================================================
    // Does this location cross the specified line
    //============================================================================
    bool ContainsLine( _In_ long lineNumber ) const
    {
        return lineNumber >= m_lBegLine && lineNumber <= m_lEndLine;
    }

    //============================================================================
    // Is the current location between the two given locations,
    // excluding their ends?
    //
    // The function will take care of ordering the locations if necessary.
    // The given locations should not overlap each other.
    //
    // Returns:
    //      -true: if the start point of the current location is
    //             greater than the end point of the first location
    //             and the end point of the current location is
    //             less than the start point of the second location;
    //      -false: if otherwise, or the given locations overlap each other.
    //============================================================================
    inline
    bool IsBetweenExclusive(
        const Location * ploc1,
        const Location * ploc2) const
    {
        if (ploc1->EndsBefore(ploc2))
        {
            return
                // Compare the current start point with the end point of the first location
                CompareStartWithEndPoint(this, ploc1) > 0 &&
                // Compare the last location's start point with the end point of the current location
                CompareStartWithEndPoint(ploc2, this) > 0;
        }
        else if (ploc1->StartsAfter(ploc2))
        {
            return
                // Compare the current start point with the end point of the first location
                CompareStartWithEndPoint(this, ploc2) > 0 &&
                // Compare the last location's start point with the end point of the current location
                CompareStartWithEndPoint(ploc1, this) > 0;
        }

        return false;
    }

    //============================================================================
    // Is the current location between the two given locations,
    // including their ends?
    //
    //The function will take care of ordering the locations if necessary.
    // The given locations should not overlap each other.
    //
    // Returns:
    //      -true: if the start point of the current location is
    //             greater than or equal to the end point of the first
    //             location and the end point of the current location is
    //             less than or equal to the start point of the second
    //             location;
    //      -false: if otherwise, or the given locations overlap each other.
    //============================================================================
    inline
    bool IsBetweenInclusive(
        const Location * ploc1,
        const Location * ploc2) const
    {
        if (ploc1->EndsOnOrBefore(ploc2))
        {
            return
                // Compare the current start point with the end point of the first location
                CompareStartWithEndPoint(this, ploc1) >= 0 &&
                // Compare the last location's start point with the end point of the current location
                CompareStartWithEndPoint(ploc2, this) >= 0;
        }
        else if (ploc1->StartsOnOrAfter(ploc2))
        {
            return
                // Compare the current start point with the end point of the first location
                CompareStartWithEndPoint(this, ploc2) >= 0 &&
                // Compare the last location's start point with the end point of the current location
                CompareStartWithEndPoint(ploc1, this) >= 0;
        }

        return false;
    }

    //============================================================================
    // Is the current location equal to the given location?
    //
    // Returns:
    //      -true: if the current location is equal to the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool IsEqual(const Location * ploc) const
    {
        return Compare(this, ploc) == 0;
    }

    //============================================================================
    // Does the current location overlap the given location,
    // excluding its ends?
    //
    // Returns:
    //      -true: if the start point of the given location is
    //             less than the end point of the current location
    //             and the start point of the current location is
    //             less than the end point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool IsOverlappedExclusive(const Location * ploc) const
    {
        return
            CompareStartWithEndPoint(ploc, this) < 0 &&
            CompareStartWithEndPoint(this, ploc) < 0;
    }

    //============================================================================
    // Is the current location between the two given locations,
    // including their ends?
    //
    // Returns:
    //      -true: if the start point of the given location is
    //             less than or equal the end point of the current
    //             location and the start point of the current location
    //             is less than or equal to the end point of the given
    //             location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool IsOverlappedInclusive(const Location * ploc) const
    {
        return
            CompareStartWithEndPoint(ploc, this) <= 0 &&
            CompareStartWithEndPoint(this, ploc) <= 0;
    }

    //============================================================================
    // Is the given location contained within the current location,
    // considering the start and end locations exclusive?
    //
    // Returns:
    //      -true: if the start point of the given location is
    //             greater than the start point of the current
    //             location and the end point of the given location
    //             is less than the end point of the current location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool ContainsExclusive(const Location * ploc) const
    {
        return
            // Compare the start points
            CompareStartPoints(this, ploc) < 0 &&
            // Compare the end points
            CompareEndPoints(this, ploc) > 0;
    }

    //============================================================================
    // Is the given location contained within the current location,
    // considering the start location inclusive and the end location
    // inclusive?
    //
    // Returns:
    //      -true: if the start point of the current  location is
    //             less than or equal to the start point of the given
    //             location and the end point of the current location
    //             is greater than the end point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool ContainsInclusive(const Location * ploc) const
    {
        if ( !ploc )
        {
            return ComparePointers(this, ploc);
        }

        return ContainsInclusive(*ploc);
    }

    inline
    bool ContainsInclusive(const Location& loc ) const
    {
        return 
            // Compare the start points 
            CompareStartPoints(*this, loc) <= 0 &&
            // Compare the end points
            CompareEndPoints(*this, loc) >= 0;
    }

    //============================================================================
    // Does the location contain this particular point
    //============================================================================
    inline
    bool Contains(const LocationPoint& point) const
    {
        return ContainsInclusive(Location::Create(point, point));
    }

    //============================================================================
    // Does the current location start after the given location?
    //
    // Returns:
    //      -true: if the start point of the current location is
    //             greater than the end point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool StartsAfter(const Location * ploc) const
    {
        // Compare the current start point with the given end point.
        return CompareStartWithEndPoint(this, ploc) > 0;
    }

    //============================================================================
    // Does the current location start on or after the given location?
    //
    // Returns:
    //      -true: if the start point of the current location is
    //             greater than or equal to the end point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool StartsOnOrAfter(const Location * ploc) const
    {
        // Compare the current start point with the given end point.
        return CompareStartWithEndPoint(this, ploc) >= 0;
    }

    //============================================================================
    // Does the current location start before the given location?
    //
    // Returns:
    //      -true: if the start point of the current location is
    //             less than the start point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool StartsBefore(const Location * ploc) const
    {
        // Compare the start points
        return CompareStartPoints(this, ploc) < 0;
    }

    inline
    bool StartsBefore( _In_ const Location& loc) const
    {
        return CompareStartPoints(*this, loc) < 0;
    }

    //============================================================================
    // Does the current location start on or before the given location?
    //
    // Returns:
    //      -true: if the start point of the current location is
    //             less than or equal to the start point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool StartsOnOrBefore(const Location * ploc) const
     {
         // Compare the start points
         return CompareStartPoints(this, ploc) <= 0;
     }
   
    inline
    bool StartsOnOrBefore(_In_ const Location& loc) const
     {
         return CompareStartPoints(*this, loc) <= 0;
     }

    //============================================================================
    // Does the current location end after the given location?
    //
    // Returns:
    //      -true: if the end point of the current location is
    //             greater than the end point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool EndsAfter(const Location * ploc) const
    {
        // Compare the end points.
        return CompareEndPoints(this, ploc) > 0;
    }

    //============================================================================
    // Does the current location end on or after the given location?
    //
    // Returns:
    //      -true: if the end point of the current location is
    //             greater than the end point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool EndsOnOrAfter(const Location * ploc) const
    {
        // Compare the end points.
        return CompareEndPoints(this, ploc) >= 0;
    }

    //============================================================================
    // Does the current location end before the given location?
    //
    // Returns:
    //      -true: if the end point of the current location is
    //             less than the start point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool EndsBefore(const Location * ploc) const
    {
        // Compare the given start point with the current end point.
        return CompareStartWithEndPoint(ploc, this) > 0;
    }

    //============================================================================
    // Does the current location end on or before the given location?
    //
    // Returns:
    //      -true: if the end point of the current location is
    //             less than or equal the to start point of the given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool EndsOnOrBefore(const Location * ploc) const
    {
        // Compare the given start point with the current end point.
        return CompareStartWithEndPoint(ploc, this) >= 0;
    }

    //============================================================================
    // Is the given location contained within the extended current location,
    // considering the start and end locations inclusive?
    // The extended current location is considered to include one extra
    // column at the end position.
    //
    // WARNING: This function is mainly used by intellisense to determine
    // if the caret position is located within the textspan of an element
    // in the parsetree, or in a position immediately following the element.
    // Avoid using this function unless that's the intended behavior.
    //
    // Returns:
    //      -true: if the start point of the given location is
    //             greater than or equal the start point of the extended
    //             location and the end point of the given location
    //             is less than or equal to the end point of the extended
    //             location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool ContainsExtended(const Location * ploc) const
    {
        return GetExtended().ContainsInclusive(ploc);
    }

    inline 
    bool ContainsExtended(const Location& loc) const
    {
        return GetExtended().ContainsInclusive(loc);
    }

    //============================================================================
    // Does the current location start after the given extended location?
    // The extended given location is considered to include one extra
    // column at the end position.
    //
    // WARNING: This function is mainly used by intellisense to determine
    // if the caret position is after the position immediately following the
    // textspan of an element in the parsetree.
    // Avoid using this function unless that's the intended behavior.
    //
    // Returns:
    //      -true: if the start point of the current location is greater than
    //             the end point of the extended given location;
    //      -false: if otherwise
    //============================================================================
    inline
    bool StartsAfterExtended(const Location * ploc)
    {
        Location locExtended = ploc->GetExtended();
        return StartsAfter(&locExtended);
    }

    //============================================================================
    // Compares the two given location and determines their relative
    // order. The function first compares the start points. If they are
    // equal it then compares the end points.
    //
    // Returns: 0 if the two locations are the same.
    //              <0 if this location is less than loc
    //              >0 if this location is greater than loc.
    //============================================================================
    inline static
    int Compare(
        const Location * ploc1,
        const Location * ploc2)
    {
        if ( !(ploc1 && ploc2) )
        {
            return ComparePointers(ploc1, ploc2);
        }

        return Compare(*ploc1, *ploc2);
    }

    inline static
    int Compare(
        _In_ const Location& loc1,
        _In_ const Location& loc2)
    {
        int iComp = CompareStartPoints(loc1, loc2);
        if ( 0 == iComp )
        {
            iComp = CompareEndPoints(loc1, loc2);
        }

        return iComp;
    }

    //============================================================================
    // Compares the start points of the two given location and determines
    // their relative order.
    //
    // Returns: 0 if the start points of the locations are the same.
    //              <0 if location 1's start point is less than location 2's start point.
    //              >0 if location 1's start point is greater than location 2's start point.
    //============================================================================
    inline static
    int CompareStartPoints(
        _In_opt_ const Location * ploc1,
        _In_opt_ const Location * ploc2)
    {
        if ( ploc1 && ploc2 )
        {
            return CompareStartPoints(*ploc1, *ploc2);
        }

        return ComparePointers(ploc1, ploc2);
    }

    inline static
    int CompareStartPoints(
        _In_ const Location& loc1,
        _In_ const Location& loc2)
    {       
        return LocationPoint::Compare(loc1.GetStartPoint(), loc2.GetStartPoint());
    }

    //============================================================================
    // Compares the end  points of the two given location and determines
    // their relative order.
    //
    // Returns: 0 if the end points of the locations are the same.
    //              <0 if location 1's end point is less than location 2's end point.
    //              >0 if location 1's end point is greater than location 2's end point.
    //============================================================================
    inline static
    int CompareEndPoints(
        _In_ const Location * ploc1,
        _In_ const Location * ploc2)
    {
        return CompareEndPoints(*ploc1, *ploc2);
    }

    inline static
    int CompareEndPoints(
        _In_ const Location& loc1,
        _In_ const Location& loc2)
    {
        return LocationPoint::Compare(loc1.GetEndPoint(), loc2.GetEndPoint());
    }

    //============================================================================
    // Compares the start points of the frist given location with the end point
    // of the second location and determines their relative order.
    //
    // Returns: 0 if the first location's start point is the same as
    //                 the second location's end point.
    //              <0 if the first location's start point is less than
    //                 the second location's end point.
    //              >0 if the first location's start point is greater than
    //                 the second location's end point.
    //============================================================================
    inline static
    int CompareStartWithEndPoint(
        const Location * plocstart,
        const Location * plocend)
    {
        if (plocstart->m_lBegLine == plocend->m_lEndLine)
        {
            return plocstart->m_lBegColumn - plocend->m_lEndColumn;
        }

        return plocstart->m_lBegLine - plocend->m_lEndLine;
    }


    void SetStart(_In_ const Location * pNewStartLocation);
    
    void SetStart(
        long iLine,
        long iColumn);

    void SetStart( _In_ const LocationPoint& point)
    {
        m_lBegLine = point.Line;
        m_lBegColumn = point.Column;
    }

    void SetEnd(_In_ const Location * pNewEndLocation);

    void SetEnd(
        long iLine,
        long iColumn);

    void SetEnd( _In_ const LocationPoint& point)
    {
        m_lEndLine = point.Line;
        m_lEndColumn = point.Column;
    }

    bool Expand(const Location * pLocation);

    void SetLocation(ILTree::ILNode * ptree);

    void SetLocationToToken( _In_opt_ const Token * pToken );

    void SetLocationToTokenStart( _In_ const Token *pToken );

    void SetLocationToLine(long iLine);

    void SetLocation(_In_ const Location * pNewLocation);

    void SetLocation(
        long iLine,
        long iColumn);

    void SetLocation(
        long BegLine,
        long BegColumn,
        long EndLine,
        long EndColumn)
    {
        m_lBegLine = BegLine;
        m_lBegColumn = BegColumn;
        m_lEndLine = EndLine;
        m_lEndColumn = EndColumn;
    }

    void SetLocation(
        long BegLine,
        _In_opt_ const ParseTree::PunctuatorLocation * pPunctuator,
        long Width);

    void SetLocationToHidden();

    void OffSet(const Location * pOffsetLoc)
    {
        m_lBegLine += pOffsetLoc->m_lBegLine;
        m_lBegColumn += pOffsetLoc->m_lBegColumn;
        m_lEndLine += pOffsetLoc->m_lBegLine;
        m_lEndColumn += pOffsetLoc->m_lBegColumn;
    }

    static Location Create(
        _In_ long begLine,
        _In_ long begColumn,
        _In_ long endLine,
        _In_ long endColumn)
    {
        Location loc;
        loc.SetLocation(begLine, begColumn, endLine, endColumn);
        return loc;
    }

    static Location Create( _In_ const LocationPoint& point)
    {
        return Create(point, point);
    }

    static Location Create(
        _In_ const LocationPoint& start,
        _In_ const LocationPoint& end)
    {
        return Create(start.Line, start.Column, end.Line, end.Column);
    }

    static Location Create( _In_ const TextSpan& span)
    {
        return Location::Create(
            span.iStartLine, 
            span.iStartIndex,
            span.iEndLine,
            span.iEndIndex);
    }


    static
    Location GetHiddenLocation();

    static
    Location GetInvalidLocation();

#if DEBUG
    void DebCheck()
    {
        VSASSERT(m_lBegLine <= m_lEndLine, "Bad line.");
        VSASSERT(m_lBegLine != m_lEndLine || m_lBegColumn <= m_lEndColumn, "Bad column.");
    }
#else !DEBUG
    void DebCheck() {}
#endif !DEBUG

private:
    inline static
    int ComparePointers( _In_opt_ const Location *ploc1, _In_opt_ const Location *ploc2)
    {
        if (!ploc1 && ! ploc2)
        {
            return 0;
        }
        else if (! ploc1 && ploc2)
        {
            return 1;
        }
        else 
        {
            return -1;
        }
    }

public:
    inline
    bool operator<(const Location& other) const
    {
        return Location::Compare(*this,other) < 0;
    }


};

//============================================================================
// The location of a block of code in the file or buffer.
//============================================================================
enum LOCATIONTYPE
{
    TrackedLoc,
    CodeBlockLoc,
    ErrorLoc,
    OutOfProcBuildErrorLoc,
};

struct CodeBlockLocation : public Location
{
    long m_oBegin;
    long m_oEnd;
    char  m_locType:4;
    // these 2 were moved here from TrackedLocation to use available bits
    bool fIsInSourceFileHashTable:1;      // Is this Location stored in the SourceFile's location based hashtable?
    bool fIsChanged:1;                    // Has this location been changed?

    void Invalidate()
    {
        memset(this, 0, sizeof(*this));
    }

    const bool IsInvalid()
    {
        return (m_lBegLine == 0 &&
                m_lEndLine == 0 &&
                m_lBegColumn == 0 &&
                m_lEndColumn == 0 &&
                m_oBegin == 0 &&
                m_oEnd == 0);
    }

    const bool IsValid()
    {
        return !IsInvalid();
    }

#if IDE 
    void InvalidateOffsets()
    {
        m_oBegin = m_oEnd = 0;
    }

    const bool IsValidOffsets()
    {
        // It is valid to have the start offset being = 0.
        return (m_oBegin >= 0 && m_oEnd > 0 && m_oEnd >= m_oBegin);
    }

    //============================================================================
    // Does this location inside the edit that is passed in?
    //============================================================================
    bool IsThisLocationWithinEdit(EditCache SingleEdit)
    {
        long Start = min(SingleEdit.iStartPos, SingleEdit.iStartPos + SingleEdit.oAdjust);
        long End   = max(SingleEdit.iStartPos, SingleEdit.iStartPos + SingleEdit.oAdjust);

        return (m_oBegin >= Start && m_oEnd <= End);
    }

    //============================================================================
    // Moves this location to the new Replace offset. This can only be used if we
    // are inside a replace edit.
    //============================================================================
    void MoveLocation(EditCache SingleEdit)
    {
        VSASSERT(SingleEdit.IsReplace, L"MoveLocation() should only be called for a replace case");

        long Start = min(SingleEdit.iStartPos, SingleEdit.iStartPos + SingleEdit.oAdjust);
        long End   = max(SingleEdit.iStartPos, SingleEdit.iStartPos + SingleEdit.oAdjust);

        long LocSpan = m_oEnd - m_oBegin;
        long OffsetFromBegin  = m_oBegin - Start;

        m_oBegin = SingleEdit.iReplacePos + OffsetFromBegin;
        m_oEnd   = m_oBegin + LocSpan;

        VSASSERT(m_oBegin >= 0, L"Invalid Begin offset");
        VSASSERT(m_oEnd >= m_oBegin, L"Invalid End offset");
    }

    //============================================================================
    // Updates the start Line and column info for this location.
    //============================================================================
    void UpdateStartLineAndColumn(VBTextBuffer * pVBTextBuffer)
    {
        VSASSERT(pVBTextBuffer, L"Bad pVBTextBuffer");

        long Line     = 0;
        long Column   = 0;

        if (SUCCEEDED(pVBTextBuffer->GetLineIndexOfPosition(m_oBegin, &Line, &Column)))
        {
            m_lBegLine   = Line;
            m_lBegColumn = Column;
        }
#if DEBUG
        else
        {
            DebugLineData(pVBTextBuffer);
        }
#endif DEBUG
    }

    //============================================================================
    // Updates the end Line and column info for this location.
    //============================================================================
    void UpdateEndLineAndColumn(VBTextBuffer * pVBTextBuffer)
    {
        VSASSERT(pVBTextBuffer, L"Bad pVBTextBuffer");

        long Line     = 0;
        long Column   = 0;

        if (SUCCEEDED(pVBTextBuffer->GetLineIndexOfPosition(m_oEnd, &Line, &Column)))
        {
            m_lEndLine   = Line;
            m_lEndColumn = Column;
        }
#if DEBUG
        else
        {
            DebugLineData(pVBTextBuffer);
        }
#endif DEBUG
    }


#if DEBUG
    //============================================================================
    //============================================================================
    void DebugLineData(VBTextBuffer * pVBTextBuffer)
    {
        long NumberOfLines  = 0;
        long LastLineLength = 0;
        long LastIndex = 0;
        TextLineData ld;

        // VSW#155359
        // From the change for VSW#51592, error locations stick to their last location until
        // the recompile removes it.  The location may not exist in the buffer and consumers
        // of the error location will clip the location if necessary.
        if (m_locType != ErrorLoc && SUCCEEDED(pVBTextBuffer->GetLineCount(&NumberOfLines)))
        {
            if (SUCCEEDED(pVBTextBuffer->GetLengthOfLine(NumberOfLines - 1, &LastLineLength)))
            {
                if (LastLineLength > 0 && SUCCEEDED(pVBTextBuffer->GetPositionOfLineIndex(NumberOfLines - 1, LastLineLength - 1, &LastIndex)))
                {
                    if (SUCCEEDED(pVBTextBuffer->GetLineDataEx(gldeDefault, NumberOfLines - 1, 0, 0, &ld, NULL)))
                    {
                        VSFAIL(L"Invalid end offset");
                        pVBTextBuffer->ReleaseLineDataEx(&ld);
                    }
                }
            }
        }
    }
#endif DEBUG
#endif IDE
};

//============================================================================
// A location that is updated automatically as text is being edited.  The
// location must be manually added to the line marker table.
//============================================================================

struct TrackedLocation : public CodeBlockLocation
{
    BCSYM *m_pSymbolForLocation;
};

struct TrackedCodeBlock : public TrackedLocation
{
    bool fProcBlock;
};

struct ErrorLocation : public TrackedLocation
{
    CompilationState m_state;

    // When true, RemoveInvalidErrorLocations will remove this location even
    // if the location is still valid
    bool m_fForceRemove;
};

//============================================================================
// LineMarkerTable
//============================================================================

class LineMarkerTable
#if FV_DEADBEEF
    : public Deadbeef<LineMarkerTable> // Must be last base class!
#endif
{
public:

    //========================================================================
    // Constructor
    //========================================================================
    LineMarkerTable(SourceFile * pSourceFile);

#if IDE

    //========================================================================
    // Out of proc build error locations are added and removed explicitly.
    //========================================================================

    void AddOutOfProcBuildErrorLocation(ErrorLocation* pErrorLocation);
    void RemoveInvalidOutOfProcBuildErrorLocations();
    void RemoveOutOfProcBuildErrorLocations();


    //========================================================================
    // Error locations are added and removed explicitly.
    //========================================================================

    // Add an error location to the list.
    void AddErrorLocation(
        _Out_ ErrorLocation * ploc,
        CompilationState state);

    // Remove all of the invalid locations from the error location list.
    void RemoveInvalidErrorLocations();

    //========================================================================
    // Decl locations are stored with the symbols and will be created and
    // go away at the same time.  A codeblock will be created and destroyed
    // at the same time.
    //========================================================================

    // Add a decl location to the list.
    void AddDeclLocation(
        _Out_ TrackedLocation * ploc,
        BCSYM * pSymbolForLocation);

    // Add a code block definition to the list.
    void AddCodeBlock(
        TrackedCodeBlock * pblock,
        BCSYM * pSymbolForLocation);

    // Remove all locations from the decl list.
    void RemoveAllDeclLocations();

    //========================================================================
    // All edits may affect the various offsets and line numbers we have
    // stored in here.
    //========================================================================

    // Pass in offsets and line numbers explicitly because we already know them
    // both when we call this method.
    //
    void PreNotifyOfEdit(
            _In_ VBTextBuffer * pVBTextBuffer, 
            _In_ long lStartLine);

    void PostNotifyOfEdit(
        VBTextBuffer * pVBTextBuffer,
        EditInfoLists * pEditLists,
        EditCache SingleEdit,
        bool LastEdit);

    void ClearPreNotified() 
    {
         m_fPreNotified = false; 
    }

    void CheckCodeBlockLocations();

private:

    
    template <typename T>
    struct TrackedLocations
    {
#if DEBUG
        typedef std::set<T> Type;
#else
        typedef std::vector<T> Type;
#endif
    };

    void RemoveInvalidErrorLocations(TrackedLocations<ErrorLocation*>::Type * errorLocations);
    
    template <typename T>
    void CacheTrackedLocationBufferOffsets(
        _In_ const typename TrackedLocations<T*>::Type &, 
        _In_ VBTextBuffer* pVBTextBuffer, 
        _In_ long lStartLine);

    template <typename T>
    bool UpdateTrackedOffsetsAndLocations(
        const typename TrackedLocations<T*>::Type &,
        VBTextBuffer * pVBTextBuffer,
        EditInfoLists * pEditLists,
        EditCache SingleEdit,
        bool LastEdit);

    void UpdateTrackedLocation(
        VBTextBuffer * pVBTextBuffer,
        TrackedLocation *);

    bool IsEditBeforeLocation(
        long lEditPos,
        TrackedLocation * ploc,
        bool fCompareBegin);

    bool m_fPreNotified;

    TrackedLocations<ErrorLocation *>::Type      m_outOfProcBuildErrorLocations;
    TrackedLocations<ErrorLocation *>::Type      m_errorLocations;
    TrackedLocations<TrackedLocation *>::Type    m_declLocations;
    TrackedLocations<TrackedLocation *>::Type    m_trackedCodeBlockLocations;


    SourceFile* m_pSourceFile;

    SafeCriticalSection m_lock;
#endif IDE
};
