#ifndef __GDIEXPORTER_NATIVE_H__
#define __GDIEXPORTER_NATIVE_H__

using namespace System::Runtime;
using namespace System::Security;

ref class CNativeMethods abstract
{
internal:
    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission, returns an unmanaged handle
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "CreateDC",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    GdiSafeDCHandle^
    CreateDC(
        [MarshalAs(UnmanagedType::LPWStr)] const String^ pDriver, 
        [MarshalAs(UnmanagedType::LPWStr)] const String^ pDevice, 
        [MarshalAs(UnmanagedType::LPWStr)] const String^ pPort, 
        [MarshalAs(UnmanagedType::LPArray)] array<Byte>^ devmode
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SaveDC",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SaveDC(
        GdiSafeDCHandle^ hdc   // handle to DC
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "RestoreDC",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    RestoreDC(
        GdiSafeDCHandle^ hdc, // handle to DC
        int nSavedDC        // restore state
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission, returns an unmanaged handle
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "ResetDCW",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    HGDIOBJ                        // Output is ignored
    ResetDCW(
        GdiSafeDCHandle^ hdc,      // handle to DC
        [MarshalAs(UnmanagedType::LPArray)] array<Byte>^ devmode  // DEVMODE
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetDCOrgEx",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    GetDCOrgEx(
        GdiSafeHandle^ hdc, // handle to a DC
        [Out] POINT* pPoint     // translation origin
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetStockObject",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    GdiSafeHandle^
    GetStockObject(
        int fnObject   // stock object type
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SelectObject",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    HGDIOBJ                     // Output is ignored
    SelectObject(
        GdiSafeDCHandle^ hdc,     // handle to DC
        GdiSafeHandle^ hgdiobj  // handle to object
        );

    
    [StructLayout(LayoutKind::Sequential, CharSet=CharSet::Unicode)]
    ref class  GdiDocInfoW
    { 
    public:
        Int32	cbSize;

        [MarshalAs(UnmanagedType::LPWStr)]
        String^ DocName;

        [MarshalAs(UnmanagedType::LPWStr)]
        String^ Output;

        [MarshalAs(UnmanagedType::LPWStr)]
        String^ DataType;

        [MarshalAs(UnmanagedType::U4)]
        Int32   Types;
    };

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "StartDocW",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    StartDocW(
        GdiSafeDCHandle^ hdc,       // handle to DC
        const GdiDocInfoW^ docinfo  // contains file names
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "EndDoc",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    EndDoc(
        GdiSafeDCHandle^ hdc   // handle to DC
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "StartPage",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    StartPage(
        GdiSafeDCHandle^ hDC   // handle to DC
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "EndPage",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    EndPage(
        GdiSafeHandle^ hdc   // handle to DC
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "BeginPath",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    BeginPath(
        GdiSafeDCHandle^ hdc   // handle to DC
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "EndPath",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    EndPath(
        GdiSafeDCHandle^ hdc   // handle to DC
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "FillPath",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    FillPath(
        GdiSafeDCHandle^ hdc   // handle to DC
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "Polygon",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    Polygon(
        GdiSafeDCHandle^ hdc,     // handle to DC
        const PointI* pPoints,    // polygon vertices
        int nCount              // count of polygon vertices
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "Polyline",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    Polyline(
        GdiSafeDCHandle^ hdc,           // handle to device context
        const PointI* ppt,              // array of endpoints
        int cPoints                     // number of points in array
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "PolyPolygon",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    PolyPolygon(
        GdiSafeDCHandle^ hdc,                           // handle to DC
        const PointI* pPoints,                          // array of vertices
        const unsigned int* pPolyCounts,                // array of count of vertices
        int nCount                                      // count of polygons
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "PolyPolyline",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    PolyPolyline(
        GdiSafeHandle^ hdc,                                 // handle to device context
        const PointI* ppt,                                  // array of points
        const unsigned int* pdwPolyPoints,                  // array of values
        DWORD cCount                                        // number of entries in values array
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "PolyBezier",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    PolyBezier(
        GdiSafeDCHandle^ hdc,           // handle to device context
        const PointI* ppt,              // endpoints and control points
        DWORD cPoints                   // count of endpoints and control points
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetStretchBltMode",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SetStretchBltMode(
        GdiSafeDCHandle^ hdc,   // handle to DC
        int iStretchMode        // bitmap stretching mode
        );

    
    /// <SecurityNote>
    ///     Critical   : Elevates to unmanagedcode permissione
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetPolyFillMode",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SetPolyFillMode(
        GdiSafeDCHandle^ hdc, // handle to device context
        int iPolyFillMode   // polygon fill mode
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetDeviceCaps",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    GetDeviceCaps(
        GdiSafeHandle^ hdc, // handle to DC
        int nIndex          // index of capability
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetObjectType",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    DWORD
    GetObjectType(
        GdiSafeHandle^ obj // handle to GDI object
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetGraphicsMode",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SetGraphicsMode(
        GdiSafeDCHandle^ hdc,   // handle to device context
        int iMode               // graphics mode
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "PolyDraw",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    PolyDraw(
        GdiSafeDCHandle^ hdc,               // handle to device context
        const PointI* ppt,                  // array of points
        const Byte* pbTypes,                // line and curve identifiers
        int cCount                          // count of points
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SelectClipPath",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    SelectClipPath(
        GdiSafeHandle^ hdc, // handle to DC
        int iMode           // clipping mode
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "ExtEscape",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    ExtEscape(
        GdiSafeDCHandle^ hdc,                       // handle to DC
        int nEscape,                                // escape function
        int cbInput,                                // size of input structure
        const void* lpszInData,                     // input structure
        int cbOutput,                               // size of output structure
        void* lpszOutData                           // output structure
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetMiterLimit",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    SetMiterLimit(
        GdiSafeDCHandle^ hdc,           // handle to DC
        FLOAT eNewLimit,                // new miter limit
        [Out] FLOAT* peOldLimit         // previous miter limit
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetTextColor",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    COLORREF
    SetTextColor(
        GdiSafeHandle^ hdc, // handle to DC
        COLORREF crColor    // text color
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetTextAlign",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    UINT
    SetTextAlign(
        GdiSafeDCHandle^ hdc,   // handle to DC
        UINT fMode              // text-alignment option
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetBkMode",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SetBackMode(
        GdiSafeDCHandle^ hdc,   // handle to DC
        int iBkMode             // background mode
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SetWorldTransform",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    SetWorldTransform(
        GdiSafeHandle^ hdc,                 // handle to device context
        const XFORM* pXform                 // transformation data
        );
    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetWorldTransform",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    GetWorldTransform(
        GdiSafeDCHandle^ hdc,           // handle to device context
        [Out] XFORM* pXform             // transformation
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "ModifyWorldTransform",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    ModifyWorldTransform(
        GdiSafeDCHandle^ hdc,    // handle to device context
        CONST XFORM *lpXform,  // transformation data
        DWORD iMode            // modification mode
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "CreateSolidBrush",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    GdiSafeHandle^
    CreateSolidBrush(
        COLORREF crColor   // brush color value
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "ExtCreatePen",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    GdiSafeHandle^
    ExtCreatePen(
        DWORD dwPenStyle,                   // pen style
        DWORD dwWidth,                      // pen width
        const LOGBRUSH* plb,                // brush attributes
        DWORD dwStyleCount,                 // length of custom style array
        array<DWORD>^ pStyle                 // custom style array
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "CreateFontIndirectEx",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    GdiSafeHandle^
    CreateFontIndirectEx(
        const ENUMLOGFONTEXDV* penumlfex   // characteristiccs
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "SelectClipRgn",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SelectClipRgn(
        GdiSafeDCHandle^ hdc,       // handle to DC
        HRGN hrgn                   // handle to region
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "ExtTextOut",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    ExtTextOut(
        GdiSafeDCHandle^ hdc,                       // handle to DC
        int X,                                      // x-coordinate of reference point
        int Y,                                      // y-coordinate of reference point
        UINT fuOptions,                             // text-output options
        const RECT* prc,                            // optional dimensions
        const unsigned short* pString,              // string
        UINT cbCount,                               // number of characters in string
        const int* pDx                              // array of spacing values
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetTextMetrics",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    GetTextMetrics(
        GdiSafeDCHandle^ hdc,         // handle to DC
        [Out] TEXTMETRIC* ptm         // text metrics
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetOutlineTextMetrics",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    UINT
    GetOutlineTextMetrics(
        GdiSafeDCHandle^ hdc,         // handle to DC
        int cb,
        array<Byte>^ ptm  // text metrics
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GetTextFace",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    GetTextFace(
        GdiSafeDCHandle^ hdc,         // handle to DC
        int count,
        [Out] System::Text::StringBuilder^ faceName
        );

  
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "GdiComment",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    GdiComment(
        GdiSafeDCHandle^ hdc,           // handle to DC
        UINT nSize,                     // buffersize
        const Byte* pData               // data
        );


    [StructLayout(LayoutKind::Sequential)]
    value class  TriVertex
    {
    private:
        // TRIVERTEX color channel limits from MSDN
        static const unsigned short MinChannel = 0x0000;
        static const unsigned short MaxChannel = 0xFF00;

    public:
        __int32			m_x;
        __int32			m_y;
        unsigned short	m_Red;
        unsigned short	m_Green;
        unsigned short	m_Blue;
        unsigned short	m_Alpha;

        // fills a TRIVERTEX structure, transforming the point using alignTransform
        void Fill(Matrix % alignTransform, double x, double y, Color % color)
        {
            // untransform the point
            Point pt = alignTransform.Transform(Point(x, y));
            
            m_x = (int) Math::Round(pt.X);
            m_y = (int) Math::Round(pt.Y);

            m_Red   = color.R << 8;
            m_Green = color.G << 8;
            m_Blue  = color.B << 8;
            m_Alpha = MaxChannel;
        }
    };


    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "Msimg32.dll",
            EntryPoint = "GradientFill",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    GradientFill(
        GdiSafeDCHandle^ hdc,                                             // handle to DC
        [MarshalAs(UnmanagedType::LPArray)] array<TriVertex> ^vertex,     // array of vertices
        ULONG dwNumVertex,                                                // number of vertices
        [MarshalAs(UnmanagedType::LPArray)] array<unsigned long> ^mesh,   // array of gradients
        ULONG dwNumMesh,                                                  // size of gradient array
        ULONG dwMode                                                      // gradient fill mode
        );

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "StretchDIBits",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    StretchDIBits(
        GdiSafeDCHandle^ hdc,                       // handle to DC
        int XDest,                                  // x-coord of destination upper-left corner
        int YDest,                                  // y-coord of destination upper-left corner
        int nDestWidth,                             // width of destination rectangle
        int nDestHeight,                            // height of destination rectangle
        int XSrc,                                   // x-coord of source upper-left corner
        int YSrc,                                   // y-coord of source upper-left corner
        int nSrcWidth,                              // width of source rectangle
        int nSrcHeight,                             // height of source rectangle
        const void* pBits,                          // bitmap bits
        const BITMAPINFO* lpBitsInfo,               // bitmap data
        UINT iUsage,                                // usage options
        DWORD dwRop                                 // raster operation code
        );

    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "PatBlt",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    PatBlt(
        GdiSafeDCHandle ^ handle,
        int x,
        int y,
        int width,
        int height,
        DWORD rop3
        );

/*    
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "AddFontResourceExW",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    AddFontResourceEx(
        [MarshalAs(UnmanagedType::LPWStr)] const String^ fileName, // font file name
        DWORD fl,                                                  // font charateristics
        const Byte* pdv                                            // reserved
        );
*/
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "RemoveFontResourceExW",
            CharSet = CharSet::Unicode,
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    BOOL
    RemoveFontResourceEx(
        [MarshalAs(UnmanagedType::LPWStr)] String^ const fileName, // font file name
        DWORD fl,                                                  // font charateristics
        const Byte* pdv                                            // reserved
        );

   
    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
            "gdi32.dll",
            EntryPoint = "AddFontMemResourceEx",
            SetLastError = true,
            CallingConvention = CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    GdiFontResourceSafeHandle^
    AddFontMemResourceEx(
        [MarshalAs(UnmanagedType::LPArray, SizeParamIndex = 1)] array<Byte>^ font,   // font resources
        DWORD cbFont,                       // number of bytes in font resource
        const Byte* pdv,                    // Reserved, must be 0
        [Out] DWORD* pcFonts                // number of fonts installed
        );


    const static int CSIDL_FONTS = 0x0014; // shlobj.h

    /// <SecurityNote>
    ///     Critical: Elevates to unmanagedcode permission
    /// </SecurityNote>
    [DllImport(
        "shell32.dll",
        EntryPoint="SHGetSpecialFolderPathW",
        CharSet=CharSet::Unicode,
        CallingConvention=CallingConvention::Winapi)]
    [SecurityCritical]
    [SuppressUnmanagedCodeSecurity]
    static
    int
    SHGetSpecialFolderPathW(
        IntPtr hwndOwner,
        StringBuilder^ lpszPath,
        int nFolder,
        int fCreate
        );
};

#endif
