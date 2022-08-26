//------------------------------------------------------------------------------
// <copyright file="GPRECT.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Internal {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct GPRECT {
        internal int X;
        internal int Y;
        internal int Width;
        internal int Height;

        internal GPRECT(int x, int y, int width, int height) {
            X = x;
            Y = y;
            Width = width;
            Height = height;
        }

        internal GPRECT(Rectangle rect) {
            X = rect.X;
            Y = rect.Y;
            Width = rect.Width;
            Height = rect.Height;
        }

        internal Rectangle ToRectangle() {
            return new Rectangle(X, Y, Width, Height);
        }
    }

}
