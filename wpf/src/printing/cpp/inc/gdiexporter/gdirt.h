#ifndef __GDIEXPORTER_RT_H__
#define __GDIEXPORTER_RT_H__

/**************************************************************************
*c
* Copyright (c) 2002-2004 Microsoft Corporation
*
* Module Name:
*
*   Print Render Target (RT)
*
* Created:
*
*   6/10/2002 fyuan
*      Created it.
*
**************************************************************************/

COLORREF ToCOLORREF(SolidColorBrush ^ pBrush);

bool     PenSupported(Pen ^ pPen, Matrix * pMatrix, unsigned dpi);

/*
*
* CGDIRenderTarget
*
* Internal class that serves as bridge between Avalon drawing primitive and
* legacy GDI32. Intended for printing to legacy drivers.
*
* Usage:
*   Construct with a PrintQueue object. Call StartDoc/StartPage, etc. functions.
*   Call DrawGeometry, etc. to draw.
*
* Notes:
*   Implements the ILegacyDevice interface, which is a lightweight interface
*   similar to DrawingContext, but with less functions.
*
*/

ref class CGDIRenderTarget : public CGDIDevice, public ILegacyDevice
{
// Memeber variables
private:
    bool m_startPage;       // StartPage called

public:
    /// <SecurityNote>
    /// Critical    - Calls native method to start printing a document
    ///             - It gives away the job id, considered critical in Partial Trust
    ///             - Changes critical GDI device state
    /// </SecurityNote>
    [SecurityCritical]
    virtual int StartDocument(String^ printerName, String ^ jobName, String^ filename, array<Byte>^ devmode);

    /// <SecurityNote>
    /// Critical    - Calls native method to start printing a document
    ///             - Changes critical GDI device state
    /// </SecurityNote>
    [SecurityCritical]
    virtual void StartDocumentWithoutCreatingDC(String^ priterName, String ^ jobName, String^ filename);

    /// <SecurityNote>
    /// Critical    - Calls native method to stop printing a document
    ///             - Changes critical GDI device state
    /// </SecurityNote>
    [SecurityCritical]
    virtual void EndDocument();

    /// <SecurityNote>
    /// Critical    - Calls native method to create critical GDI device context
    /// </SecurityNote>
    [SecurityCritical]
    virtual void CreateDeviceContext(String ^ printerName, String^ jobName, array<Byte>^ devmode);

    /// <SecurityNote>
    /// Critical    - Calls native method to delete critical GDI device context
    /// </SecurityNote>
    [SecurityCritical]
    virtual void DeleteDeviceContext();

    ///<SecurityNote>
    /// Critical    - Calls critical ExtEscape to get the destination file path 
    ///                 when printing to the Microsoft XPS Document Writer
    ///             - Exposes critical local file path
    ///</SecurityNote>
    [SecurityCritical] 
    virtual String^ ExtEscGetName();

    ///<SecurityNote>
    /// Critical    - Calls critical ExtEscape to put GDI device in bypass mode
    ///             - Changes critical GDI device state
    ///</SecurityNote>
    [SecurityCritical]
    virtual bool    ExtEscMXDWPassThru();

    /// <SecurityNote>
    /// Critical    - Calls native method to start drawing a page to a GDI device
    ///             - Calls critical GetDeviceCaps
    ///             - Changes critical device state
    /// </SecurityNote>
    [SecurityCritical]
    virtual void StartPage(array<Byte>^ devmode, int rasterizationDPI);

    /// <SecurityNote>
    /// Critical    - Calls native method to stop drawing a page to a GDI device
    ///             - Changes critical device state
    /// </SecurityNote>
    [SecurityCritical]
    virtual void EndPage();

    /// <SecurityNote>
    /// Critical    - Calls native method to change critical GDI device state
    /// </SecurityNote>
    [SecurityCritical]
    virtual void PopClip();

    virtual void PopTransform();

    virtual void PushClip(Geometry^ clipGeometry);

    virtual void PushTransform(System::Windows::Media::Matrix transform);

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    virtual void DrawGeometry(Brush^ brush, Pen^ pen, Brush^ strokeBrush, Geometry^ geometry);

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    virtual void DrawImage(BitmapSource^ source, array<Byte>^ buffer, Rect rect);

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    virtual void DrawGlyphRun(Brush ^ pBrush, GlyphRun^ glyphRun);

    /// <SecurityNote>
    /// Critical    - Call native method to send GDI comment to device
    /// </SecurityNote>
    [SecurityCritical]
    virtual void Comment(String ^ comment);

protected:
    // current state data
    System::Collections::Stack^ m_state;

    int                         m_clipLevel;
    Matrix                      m_transform;
    Matrix                      m_DeviceTransform;
    int                         m_nWidth;
    int                         m_nHeight;

    // Fix for bug 985195: We try each charset when creating style-simulated font
    // in effort to force GDI to create unstyled font with style simulations. Here
    // we cache the charsets that work in creating unstyled, style-simulated font.
    //
    // Hash from FontSimulatedStyleKey^ -> BYTE charset
    ref class FontSimulatedStyleKey sealed
    {
    public:
        FontSimulatedStyleKey(String^ faceName, LONG lfWeight, BYTE lfItalic)
        {
            Debug::Assert(faceName != nullptr);

            m_faceName = faceName;
            m_lfWeight = lfWeight;
            m_lfItalic = lfItalic;
        }

    private:
        String^ m_faceName;
        LONG m_lfWeight;
        BYTE m_lfItalic;

    public:
        virtual int GetHashCode() override sealed
        {
            return m_faceName->GetHashCode() ^ m_lfWeight.GetHashCode() ^ m_lfItalic.GetHashCode();
        }

        virtual bool Equals(Object^ other) override sealed
        {
            FontSimulatedStyleKey^ o = dynamic_cast<FontSimulatedStyleKey^>(other);

            if (o == nullptr)
            {
                return false;
            }
            else
            {
                return m_faceName == o->m_faceName &&
                    m_lfWeight == o->m_lfWeight &&
                    m_lfItalic == o->m_lfItalic;
            }
        }
    };

    System::Collections::Hashtable^ m_cachedUnstyledFontCharsets;

    // Throws an exception for an HRESULT if it's a failure.
    // Special case: Throws PrintingCanceledException for ERROR_CANCELLED/ERROR_PRINT_CANCELLED.

    /// <SecurityNote>
    /// Critical    - Instantiates critical type PrintSystemException
    /// TreatAsSafe - PrintSystemException's ctor is safe
    /// </SecurityNote>
    [SecuritySafeCritical]
    void ThrowOnFailure(HRESULT hr);
    
    /// <SecurityNote>
    /// Critical    - Calls native methods to get top left corner of paper
    /// TreatAsSafe - Does not expose critical data
    ///             - Does not expose change critical device state
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT Initialize();

    /// <SecurityNote>
    /// Critical    - Calls StretchBlt
    /// TreatAsSafe - Information is still kept with the same object
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT DrawBitmap(
        BitmapSource ^pImage, 
        array<Byte>^ buffer,
        Rect rectDest
        );

    HRESULT GetBrushScale(
        Brush ^ pFillBrush,
        double % ScaleX,
        double % ScaleY
        );

    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// TreatAsSafe - Data sent to GDI is converted from managed object
    /// </SecurityNote>
    [SecuritySafeCritical]
    void PushClipProxy(GeometryProxy% geometry);

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT StrokePath(
        IN GeometryProxy% geometry,
        IN Pen ^pPen,
        IN Brush ^pStrokeBrush
        );

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT FillPath(
        IN GeometryProxy% geometry,
        IN Brush ^pFillBrush
        );

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    // Fills geometry with ImageBrush if possible.
    HRESULT FillImage(
        IN GeometryProxy% geometry,
        IN ImageBrush^ brush
        );

    HRESULT RasterizeBrush(
        CGDIBitmap    % bmpdata,
        Int32Rect       renderBounds,     // render bounds in device space, rounded
        Int32Rect       bounds,           // geometry bounds in device space, rounded
        Rect            geometryBounds,   // geometry bounds in local space
        Brush         ^ pFillBrush,
        bool            vertical,
        bool            horizontal,
        double          ScaleX,
        double          ScaleY
        );

    /// <SecurityNote>
    /// Critical    - Calls StretchBlt
    /// TreatAsSafe - Information is still kept with the same object
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT RasterizeShape(
        GeometryProxy% geometry,
        Int32Rect % pBounds,
        Brush ^ pFillBrush
        );

    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// TreatAsSafe - Data sent to GDI is converted from managed object
    /// </SecurityNote>
    [SecuritySafeCritical]
    BOOL SetTextWorldTransform(
        XFORM % OriginalTransform
        );

    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// TreatAsSafe - Data sent to GDI is converted from managed object, return value is managed using SafeHandle
    /// </SecurityNote>
    [SecuritySafeCritical]
    GdiSafeHandle^ CreateFont(
        GlyphRun ^pGlyphRun,
        double fontSize,        // in MIL units (96.0 dpi)
        double scaleY,
        [Out] Boolean% isPrivateFont
        );
    
    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT RenderGlyphRun(
        GlyphRun ^pGlyphRun,
        Point translate,
        Point scale,
        Boolean isPrivateFont
        );

    /// <SecurityNote>
    /// Critical    - Calls native method to draw to a GDI device
    /// </SecurityNote>
    [SecurityCritical]
    HRESULT RenderTextThroughGDI(
        GlyphRun ^pGlyphRun,
        Brush ^pBrush
        );

    /// <summary>
    /// Creates a font and caches it, or retrieves an existing cached font.
    /// Returns nullptr on failure.
    /// </summary>
    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ CreateFontCached(interior_ptr<ENUMLOGFONTEXDV> logfontdv);

    /// <summary>
    /// Attempts to create a font with simulated styles. It will loop through
    /// available charsets to try to force GDI to create simulated style font.
    /// See definition for details.
    /// </summary>
    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ CreateSimulatedStyleFont(interior_ptr<ENUMLOGFONTEXDV> logfontdv, StyleSimulations styleSimulations);

    /// <summary>
    /// Creates an unstyled (normal weight, non italics) version of a font.
    /// Returns nullptr on failure.
    /// </summary>
    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// </SecurityNote>
    [SecurityCritical]
    GdiSafeHandle^ CreateUnstyledFont(interior_ptr<ENUMLOGFONTEXDV> logfontdv);

    /// <summary>
    /// Gets the face name for a font, ex: "Arial", "Times New Roman".
    /// Returns nullptr on failure.
    /// </summary>
    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// </SecurityNote>
    [SecurityCritical]
    String^ GetFontFace(GdiSafeHandle^ font);

    /// <summary>
    /// Gets the font style, ex: "Regular", "Bold".
    /// Returns nullptr on failure.
    /// </summary>
    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// </SecurityNote>
    [SecurityCritical]
    String^ GetFontStyle(GdiSafeHandle^ font);

    /// <summary>
    /// Checks if a font has particular face and style names.
    /// </summary>
    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// </SecurityNote>
    [SecurityCritical]
    bool CheckFontFaceAndStyle(GdiSafeHandle^ font, String^ fontFace, String^ fontStyle);

    /// <SecurityNote>
    /// Critical    - 1 - It calls GDI function
    ///				  2 - Asserts Registry Permisions in order to obtain codec info
    /// TreatAsSafe - 1 -Data sent to GDI is converted from managed object
    ///				  2 - Codec Info used internaly and not returned.
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT DrawBitmap_PassThrough(
        BitmapSource     ^ pIBitmap,
        Int32Rect        % rcDstBounds,    // pixels
        INT                nImageWidth,     // pixels
        INT                nImageHeight     // pixels
        );

    /// <SecurityNote>
    /// Critical    - It calls GDI function
    /// TreateAsSafe - Data is converted from managed data, GDI is guarded againt it
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT FillLinearGradient(
        IN GeometryProxy% geometry,
        IN Brush^ brush
        );
        
private:
    /// sets some members of the ENUMLOGFONTEXDV structure to vaues computed from index
    /// Returns false if no members could be set based on index
    /// Used to generate a series of ENUMLOGFONTEXDV structures
    [SecurityCritical]
    bool SetLOGFONT(
        interior_ptr<ENUMLOGFONTEXDV> logfontdv,
        int index
        );
};

#endif
