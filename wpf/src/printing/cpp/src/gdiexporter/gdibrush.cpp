/**************************************************************************
*
* Copyright (c) 2002 Microsoft Corporation
*
* Module Name:
*
*   CMILBrush to GDI conversion
*
* Created:
*
*   6/14/2002 fyuan
*      Created it.
*
**************************************************************************/


GdiSafeHandle^ CGDIDevice::ConvertBrush(COLORREF colorRef)
{
    if (colorRef == 0xFFFFFF) // white: RGB(255, 255, 255)
    {
        return m_whiteBrush;
    }
    else if (colorRef == 0)   // black: RGB(0, 0, 0)
    {
        return m_blackBrush;
    }
                
    GdiSafeHandle^ brush = CacheMatch((interior_ptr<Byte>) & colorRef, sizeof(colorRef));

    if (brush == nullptr)
    {
        brush = CNativeMethods::CreateSolidBrush(colorRef);

        if (brush != nullptr)
        {
            CacheObject((interior_ptr<Byte>) & colorRef, sizeof(colorRef), brush);
        }
        else
        {
            Debug::Assert(false, "CreateSolidBrush failed");
        }
    }

    return brush;
}


GdiSafeHandle^ CGDIDevice::ConvertBrush(Brush ^ brush)
{
    // WARNING: Brush types must be scaled according to CGDIPath::GetResolutionScale().
    // We don't need to do it for solid color brushes, but adding any other types will require it.

    if (brush->GetType() == SolidColorBrush::typeid)
    {
        SolidColorBrush ^ pSolid = (SolidColorBrush ^) brush;
    
        return ConvertBrush(ToCOLORREF(pSolid));
    }

    // t-bnguy: pattern brush creation removed; it acts like a brush without scaling so the output looks different
    // at different zoom levels

    return nullptr;
}


COLORREF ToCOLORREF(SolidColorBrush ^ pBrush)
{
    Color c = pBrush->Color;

    return RGB(c.R, c.G, c.B);
}
