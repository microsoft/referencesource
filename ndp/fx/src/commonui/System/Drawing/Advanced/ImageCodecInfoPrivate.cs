//------------------------------------------------------------------------------
// <copyright file="ImageCodecInfoPrivate.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Imaging {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.Drawing;

    // sdkinc\imaging.h
    [StructLayout(LayoutKind.Sequential, Pack=8)]
    internal class ImageCodecInfoPrivate {
        [MarshalAs(UnmanagedType.Struct)]
        public Guid Clsid;
        [MarshalAs(UnmanagedType.Struct)]
        public Guid FormatID;

        public IntPtr CodecName = IntPtr.Zero;
        public IntPtr DllName = IntPtr.Zero;
        public IntPtr FormatDescription = IntPtr.Zero;
        public IntPtr FilenameExtension = IntPtr.Zero;
        public IntPtr MimeType = IntPtr.Zero;

        public int Flags;
        public int Version;
        public int SigCount;
        public int SigSize;

        public IntPtr SigPattern = IntPtr.Zero;
        public IntPtr SigMask = IntPtr.Zero;
    }
}
