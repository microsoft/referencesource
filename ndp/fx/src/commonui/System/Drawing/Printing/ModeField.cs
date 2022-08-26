//------------------------------------------------------------------------------
// <copyright file="ModeField.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using System.Diagnostics;
    using System;
    using System.Runtime.InteropServices;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    // Some of the fields in DEVMODE
    internal enum ModeField {
        Orientation,
        PaperSize,
        PaperLength,
        PaperWidth,
        Copies,
        DefaultSource,
        PrintQuality,
        Color,
        Duplex,
        YResolution,
        TTOption,
        Collate,
    }
}

