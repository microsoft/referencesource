//------------------------------------------------------------------------------
// <copyright file="MetafileHeaderEmf.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal class MetafileHeaderEmf
    {
        /// SECREVIEW :  The ENHMETAHEADER structure is defined natively as a union with WmfHeader.  
        ///              Extreme care should be taken if changing the layout of the corresponding managaed 
        ///              structures to minimize the risk of buffer overruns.  The affected managed classes 
        ///              are the following: ENHMETAHEADER, MetaHeader, MetafileHeaderWmf, MetafileHeaderEmf.
        ///              See ASURT#82822 or changes in Metafile.cs@115636 for more information.
        ///  
         public MetafileType type = MetafileType.Invalid;
         public int size;
         public int version;
         public EmfPlusFlags emfPlusFlags = 0;
         public float dpiX;
         public float dpiY;
         public int X;
         public int Y;
         public int Width;
         public int Height;
         public SafeNativeMethods.ENHMETAHEADER EmfHeader;
         public int EmfPlusHeaderSize;
         public int LogicalDpiX;
         public int LogicalDpiY;
    }
}
