//------------------------------------------------------------------------------
// <copyright file="MetafileHeaderWmf.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {

    using System.Diagnostics;

    using System.Drawing;
    using System;
    using System.Runtime.InteropServices;
     
    [StructLayout(LayoutKind.Sequential, Pack=8)]
    internal class MetafileHeaderWmf
    {
        /// SECREVIEW :  The ENHMETAHEADER structure is defined natively as a union with WmfHeader.  
        ///              Extreme care should be taken if changing the layout of the corresponding managaed 
        ///              structures to minimize the risk of buffer overruns.  The affected managed classes 
        ///              are the following: ENHMETAHEADER, MetaHeader, MetafileHeaderWmf, MetafileHeaderEmf.
        ///              See ASURT#82822 or changes in Metafile.cs@115636 for more information.
        ///  
        public MetafileType type = MetafileType.Invalid;
        public int size = Marshal.SizeOf(typeof(MetafileHeaderWmf));
        public int version;
        public EmfPlusFlags emfPlusFlags=0;
        public float dpiX;
        public float dpiY;
        public int X;
        public int Y;
        public int Width;
        public int Height;
        
        //The below datatype, WmfHeader, file is defined natively
        //as a union with EmfHeader.  Since EmfHeader is a larger
        //structure, we need to pad the struct below so that this
        //will marshal correctly.
        [MarshalAs(UnmanagedType.Struct)]
        public MetaHeader WmfHeader = new MetaHeader();
        public int dummy1;
        public int dummy2;
        public int dummy3;
        public int dummy4;
        public int dummy5;
        public int dummy6;
        public int dummy7;
        public int dummy8;
        public int dummy9;
        public int dummy10;
        public int dummy11;
        public int dummy12;
        public int dummy13;
        public int dummy14;
        public int dummy15;
        public int dummy16;

        public int EmfPlusHeaderSize;
        public int LogicalDpiX;
        public int LogicalDpiY;
    }
}
