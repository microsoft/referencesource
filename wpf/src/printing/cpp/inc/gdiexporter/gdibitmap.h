#ifndef __GDIEXPORTER_BITMAP_H__
#define __GDIEXPORTER_BITMAP_H__

inline int GetDIBStride(int width, int bitcount)
{
    return (width * bitcount + 31) / 32 * 4;
}

ref class PaletteSorter;

value class CGDIBitmap
{
private:
    // m_Buffer[m_Offset] points to the top scanline.
    // This implies that if (m_Stride > 0), then it's a top-down buffer, otherwise
    // it's a bottom-up buffer.
    int                m_Width;
    int                m_Height;
    int                m_Stride;
    PixelFormat        m_PixelFormat;

    int                m_Offset;
    BitmapSource     ^ m_pBitmap;
    array<BYTE>      ^ m_Bi;


    /// <SecurityNote>
    /// Critical - Data is retrieved using CriticalCopyPixels
    /// </SecurityNote>
    [SecurityCritical]
    array<BYTE> ^ m_Buffer;
    
    /// <SecurityNote>
    /// Critical - m_Buffer is passed to PaletteSorter for color reduction
    /// </SecurityNote>
    [SecurityCritical]
    PaletteSorter    ^ m_pSorter;

public: 

    /// <SecurityNote>
    /// Critical    - It calls PresentationCore internal function CriticalCopyPixels
    /// TreatAsSafe - Image is copied to m_Buffer, which is marked as SecurityCritical
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT Load(BitmapSource ^ pBitmap, array<Byte>^ buffer, PixelFormat LoadFormat);

    System::Collections::Generic::IList<Color> ^ GetColorTable();

//  HRESULT CopyCropImage();
    
    /// <SecurityNote>
    /// Critical    - It touches m_Buffer
    /// TreatAsSafe - Information is still kept with the same object
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT ColorReduction();

    /// <SecurityNote>
    /// Critical    - It touches m_Buffer
    /// TreatAsSafe - Information is still kept with the same object
    /// </SecurityNote>
    [SecuritySafeCritical]
    void SetBits(interior_ptr<BITMAPINFO> bmi);

    /// <SecurityNote>
    /// Critical    - It sends critical information in m_Buffer out
    /// TreatAsSafe - Info is sent to printer driver, which is considered trusted
    /// </SecurityNote>
    [SecuritySafeCritical]
    HRESULT StretchBlt(CGDIDevice ^ pDevice, const Int32Rect & dst, bool flipHoriz, bool flipVert);
    
    /// <SecurityNote>
    /// Critical    - Uses m_Buffer
    /// TreatAsSafe - No information leaking out
    /// </SecurityNote>
    [SecuritySafeCritical]
    bool IsValid(void)
    {
        return m_Buffer != nullptr;
    }

    /// <SecurityNote>
    ///  Critical: Calls critical SetQuad
    /// </SecurityNote>
    [SecurityCritical]
    void SetupPalette(interior_ptr<BITMAPINFO> bmi, int bitCount);
};

// Pushes transform, then rasterizes rectangle with specified bounds and brush
// to bitmap.
BitmapSource ^ CreateBitmapAndFillWithBrush(
    int width,              // rasterization bitmap size
    int height,
    Brush ^ brush,
    Rect bounds,            // geometry bounds in local space
    Transform^ transform,   // DrawingContext transform
    PixelFormat pixelFormat
    );

#endif
