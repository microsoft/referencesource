#ifndef __GDIEXPORTER_GDIPATH_H__
#define __GDIEXPORTER_GDIPATH_H__

ref class CGDIPath
{
private:
    CGDIPath(GeometryProxy% geometry, Matrix matrix, bool ForFill, Pen^ pen);

public:
    static CGDIPath^ CreateFillPath(GeometryProxy% geometry, Matrix matrix)
    {
        return gcnew CGDIPath(geometry, matrix, true, nullptr);
    }

    static CGDIPath^ CreateStrokePath(GeometryProxy% geometry, Matrix matrix, Pen^ pen)
    {
        return gcnew CGDIPath(geometry, matrix, false, pen);
    }

    /// <SecurityNote>
    /// Critical    - Calls critical CGDIDevice methods to fill a path
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT Fill(CGDIDevice ^ dc, GdiSafeHandle^ brush);

    /// <SecurityNote>
    /// Critical    - Calls critical CGDIDevice methods to draw the outline of a path
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT Draw(CGDIDevice ^ dc, GdiSafeHandle^ pen);
    
    bool IsValid(void)
    {
        return m_IsValid;
    }

    bool HasCurve()
    {
        return m_HasCurve;
    }

    int GetResolutionScale()
    {
        return m_ResolutionScale;
    }

/*  void GetBounds(Int32Rect * pRect)
    {
        pRect->X      = m_DeviceBounds.X;
		pRect->Y      = m_DeviceBounds.Y;
		pRect->Width  = m_DeviceBounds.Width;
		pRect->Height = m_DeviceBounds.Height;
    }
*/

    /// <SecurityNote>
    /// Critical    - Calls critical CGDIDevice methods to clip based on a path
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT SelectClip(CGDIDevice ^ dc, int mode);

    double MaxCos(void);

protected:
    void GetDeviceBounds(array<PointI>^ p, INT count);

    void   ProcessCurve(int count, bool ForFill);
    void ProcessPolygon(int count, bool ForFill, int figureCount);

private:

    bool                    m_IsValid;
    bool                    m_HasCurve;

    Int32Rect               m_DeviceBounds;
    int                     m_ResolutionScale;  // Fix bug 1534923: See GdiGeometryConverter.ResolutionScale.

    array<Byte>^            m_Types;
    array<PointI>^          m_Points;
    
    array<unsigned int> ^   m_PolyCounts;

    int                     m_NumPoints;
    int                     m_NumPolygons;
    int                     m_PathFillMode;

    unsigned long           m_Flags;
};

/**************************************************************************\
*
* Class Description:
*    Break PolyPolygon with disjointing groups into multiple PolyPolygons
*    to avoid O(N^2) algorithm in HP PCL 5/6 printer firmware implementation
*
* Created:
*    9/28/2001 fyuan
*
\**************************************************************************/

ref class CPolyPolygon
{
protected:
    int                     m_cPolygons;
    
    array<PointI>^          m_rgptVertex;
    int                     m_offsetP;

    array<unsigned int> ^   m_rgcPoly;
    int                     m_offsetC;
    
    PointI                  m_topleft;
    PointI                  m_bottomright;

    void Divide(array<CPolyPolygon ^> ^pPolygons, int cGroup);

    bool DisJoint(CPolyPolygon ^poly2);

    static bool DisJoint(array<CPolyPolygon ^> ^ pPolygons, int cGroup);

    void GetBounds(void);

public:

    void Set(array<PointI>^       rgptVertex, int offsetP,
             array<unsigned int>^ rgcPoly,    int offsetC, 
             int cPolygons);

    /// <SecurityNote>
    /// Critical    - Calls critical CGDIDevice methods to draw a set polygon
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT Draw(CGDIDevice ^ dc);
};

#endif
