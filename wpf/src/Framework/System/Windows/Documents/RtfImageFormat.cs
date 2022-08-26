//---------------------------------------------------------------------------
// 
// File: RtfImageFormat.cs
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// Description: RtfImageFormat which indicates the image format type.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    /// <summary>
    /// Rtf image format enumeration indicates whether an image is a bitmap,
    /// png, jpeg, gif, tif, dib or windows metafile etc.
    /// </summary>
    internal enum RtfImageFormat
    {
        Unknown,
        Bmp,
        Dib,
        Emf,
        Exif,
        Gif,
        Jpeg,
        Png,
        Tif,
        Wmf
    }
}
