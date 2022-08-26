#ifndef __GDIEXPORTER_DEVICE_H__
#define __GDIEXPORTER_DEVICE_H__

ref class CGDIPath;

typedef enum
{
    CAP_WorldTransform = 0x0004, // Support world transform
    CAP_PolyPolygon    = 0x0008, // Support Polypolygon

    CAP_JPGPassthrough = 0x0080, // Support JPG passthrough
    CAP_PNGPassthrough = 0x0100, // Support PNG passthrough
    
    CAP_GradientRect   = 0x1000,
    CAP_CharacterStream = 0x2000,  // Is a text only device
};

ref class CachedGDIObject
{
protected:
    array<Byte>   ^ m_RawData;

    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle ^ m_handle;
    
public:
    
    property GdiSafeHandle^ Handle
    {
        /// <SecurityNote>
        ///     Critical : Exposes critical handle
        /// </SecurityNote>
        [SecurityCritical]
        GdiSafeHandle^ get() { return m_handle; }
    }

    /// <SecurityNote>
    ///     Critical : Unmanaged pointer
    /// </SecurityNote>
    [SecurityCritical]
    CachedGDIObject(const interior_ptr<Byte> pData, int size, GdiSafeHandle ^ handle)
    {
        m_RawData = gcnew array<Byte>(size);

        for (int i = 0; i < size; i ++)
        {
            m_RawData[i] = pData[i];
        }

        m_handle = handle;
    }

    /// <SecurityNote>
    ///     Critical : Unmanaged pointer
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ Match(const interior_ptr<Byte> pData, int size)
    {
        if (size != m_RawData->Length)
        {
            return nullptr;
        }

        for (int i = 0; i < size; i ++)
        {
            if (m_RawData[i] != pData[i])
            {
                return nullptr;
            }
        }

        return m_handle;
    }
};


/// <Summary>
///     Thin wrapper over an HDC
/// </Summary>
ref class CGDIDevice
{
protected:
    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeDCHandle ^ m_hDC;

    unsigned long m_Caps;

    unsigned m_nDpiX;
    unsigned m_nDpiY;
    double   m_RasterizationDPI;

    array<Byte>^ m_lastDevmode;

    array<CachedGDIObject ^> ^ m_Cache;
    int                             m_CacheFirst;
    
    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^                  m_lastFont;

    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^                  m_lastPen;
    
    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^                  m_lastBrush;
    
    COLORREF                        m_lastTextColor;
    int                             m_lastPolyFillMode;
    unsigned                        m_lastTextAlign;
    float                           m_lastMiterLimit;

    /// <SecurityNote>
    ///     Critical : It contains list of fonts installed on the system and their paths
    /// </SecurityNote>
    /// <Remarks>
    /// Hash table mapping from font name string to FontInfo. An entry here does not imply
    /// the font is installed and usable; see FontInfo for more information.
    /// </Remarks>
    [SecurityCritical]
    static Hashtable              ^ s_InstalledFonts;   // For local EMF spooling, we can't unstall fonts until print job finishes
                                                        // So for the moment, we will leak the fonts until applications closes
                                                        // In long term, we need a way to wait for job completion
    static Object                 ^ s_lockObject = gcnew Object();            // Synchronization
    static ArrayList              ^ s_oldPrivateFonts = gcnew ArrayList();    // Fonts to be deleted after 10 minutes, upon new print job  
    
public:

    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle                 ^ m_nullPen;

    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle                 ^ m_nullBrush;

    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle                 ^ m_whiteBrush;

    /// <SecurityNote>
    ///     Critical : Field for critical type
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle                 ^ m_blackBrush;

    static property ArrayList^ OldPrivateFonts
    {
        ArrayList^ get()
        {
            return s_oldPrivateFonts;
        }
    }

    // Constructor
    /// <SecurityNote>
    /// Critical    - It calls native methods to obtains stock GDI objects
    /// TreatAsSafe - Does not set critical GDI state
    ///             - Does not expose critical data
    /// </SecurityNote>
    [SecuritySafeCritical]
    CGDIDevice();

    /// <SecurityNote>
    /// Critical    - Calls critical SafeHandle::Close to dispose DC handle
    /// </SecurityNote>
    [SecurityCritical]
    void Release();

    /// <SecurityCritical>
    /// Critical : References security critical type 'GdiSafeHandle'
    /// </SecurityCritical>
    [SecurityCritical]
    void ResetStates();
    
    // Initialization
    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT InitializeDevice();

    // Query

    unsigned long GetCaps(void)
    {
        return m_Caps;
    }

    unsigned GetDpiX(void)
    {
        return m_nDpiX;
    }

    unsigned GetDpiY(void)
    {
        return m_nDpiY;
    }

    /// <SecurityNote>
    /// Critical    - Calls native method to obtain the origin of the DC's coordinate system
    /// TreatAsSafe - Inputs are safe
    ///             - Critical data is not exposed
    ///             - Critical device state is not changed
    /// </SecurityNote>
    [SecuritySafeCritical]
    BOOL GetDCOrgEx(POINT & pOrigin);

    /// <SecurityNote>
    /// Critical    - Calls native method to change critical device state
    /// </SecurityNote>
    [SecurityCritical]
    void SelectObject(GdiSafeHandle^ hObj, int type);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT SetupForIncreasedResolution(int resolutionMultiplier, XFORM& oldTransform);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT CleanupForIncreasedResolution(int resolutionMultiplier, const XFORM& oldTransform);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT SetPolyFillMode(int polyfillmode);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    BOOL SelectClipPath(int iMode);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT SetMiterLimit(FLOAT eNewLimit);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT SetTextColor(COLORREF color);

    // Drawing primitives

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT Polygon (array<PointI>^ pPoints, int offset, int nCount);

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT Polyline(array<PointI>^ pPoints, int offset, int nCount);    
    
    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT PolyPolygon(array<PointI>^ pPoints,     int offsetP, 
                     array<unsigned int> ^   pPolyCounts, int offsetC, 
                     int nCount);

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT PolyPolyline(array<PointI>^ pPoints, array<unsigned int>^ pPolyCounts, int nCount);
    
    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT BeginPath(void);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT EndPath(void);

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT FillPath(void);

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT StretchDIBits(
            int XDest,                    // x-coord of destination upper-left corner
            int YDest,                    // y-coord of destination upper-left corner
            int nDestWidth,               // width of destination rectangle
            int nDestHeight,              // height of destination rectangle
            int XSrc,                     // x-coord of source upper-left corner
            int YSrc,                     // y-coord of source upper-left corner
            int nSrcWidth,                // width of source rectangle
            int nSrcHeight,               // height of source rectangle
            interior_ptr<Byte> pBits,  // bitmap bits
            interior_ptr<BITMAPINFO> pBitsInfo         // bitmap data
            );

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT FillRect(int x, int y, int width, int height, GdiSafeHandle^ hBrush);

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT PolyBezier(array<PointI>^ pPoints, int nCount);

    /// <SecurityNote>
    /// Critical    - Calls native methods to draw to GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT DrawMixedPath(array<PointI>^ pPoints, array<Byte> ^ pTypes, int nCount);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT HrEndDoc(void);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT HrStartPage(array<Byte>^ pDevmode);
    
    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT HrEndPage(void);

    /// <SecurityNote>
    /// Critical    - Calls native methods to set critical GDI state
    /// </SecurityNote>
    [SecurityCritical, SecurityTreatAsSafe]
    HRESULT SetTextAlign(
        unsigned     textAlign // text-alignment option
    );

    /// <SecurityNote>
    /// Critical    - Calls native method to see if a GDI escape is supported
    /// </SecurityNote>
    [SecurityCritical]
    BOOL EscapeSupported(DWORD function)
    {
        return CNativeMethods::ExtEscape(m_hDC, QUERYESCSUPPORT, sizeof(DWORD), (void*)(LPCSTR)& function, 0, NULL) != 0;
    }

    /// <SecurityNote>
    ///     Critical : Calls critical method to retrieve cached GDI handle
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ CacheMatch(const interior_ptr<Byte> pData, int size);

    /// <SecurityNote>
    ///     Critical : Calls critical method to cache GDI handle
    /// </SecurityNote>
    [SecurityCritical]
    void CacheObject(const interior_ptr<Byte> pData, int size, GdiSafeHandle^ handle);

    /// <SecurityNote>
    /// Critical    - Calls native method to obtain create GDI pen from WPF pen
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle ^ ConvertPen(
        Pen           ^ pen,
        Brush         ^ pStrokeBrush,
        Matrix          matrix,
        CGDIPath      ^ pPath,
        int             dpi
        );

    /// <SecurityNote>
    /// Critical    - Calls native method to obtain create GDI brush from WPF brush
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ ConvertBrush(Brush ^brush);
    
    /// <SecurityNote>
    /// Critical    - Calls native method to obtain create GDI brush from WPF brush
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ ConvertBrush(COLORREF color);

    /// <SecurityNote>
    ///     Critical   : It calls critical function BuildFontList, uses font path, may install and uninstall fonts
    /// </SecurityNote>
    
    /// <Remarks>
    /// Checks if font is installed, and performs necessary installs and uninstalls to make font usable by GDI.
    /// </Remarks>
    /// <Returns>Returns nullptr if unable to retrieve font directory name or install the font.
    /// Otherwise, (new) font family name will be returned.
    /// If nullptr is returned, caller should fallback to filling text geometry.</Returns>
    [SecurityCritical]
    String^ CheckFont(GlyphTypeface^ typeface, String ^ name, [Out] Boolean% isPrivateFont);

    /// <SecurityNote>
    ///     Critical   : Uninstalls fonts via critical FontInfo::Uninstall
    /// </SecurityNote>
    /// <Remarks>
    /// Uninstalls only private fonts, i.e. fonts that we manually install during glyph printing.
    /// </Remarks>
    [SecurityCritical]
    void UninstallFonts();

    property
    bool
    HasDC
    {
        /// <SecurityNote>
        ///     Critical    : Uses critcal SafeHandle type
        ///     TreatAsSafe : Does not expose critical state or change critical state
        /// </SecurityNote>
        [SecuritySafeCritical]
        bool get();
    }
};

#endif
