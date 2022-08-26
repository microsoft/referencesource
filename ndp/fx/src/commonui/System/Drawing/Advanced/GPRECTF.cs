//------------------------------------------------------------------------------
// <copyright file="GPRECTF.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Internal {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal struct GPRECTF {
        internal float X;
        internal float Y;
        internal float Width;
        internal float Height;
        
        internal GPRECTF(float x, float y, float width, float height) {
            X = x;
            Y = y;
            Width = width;
            Height = height;
        }

        internal GPRECTF(RectangleF rect) {
            X = rect.X;
            Y = rect.Y;
            Width = rect.Width;
            Height = rect.Height;
        }

        internal SizeF SizeF {
            get {
                return new SizeF(Width, Height);
            }
        }

        internal RectangleF ToRectangleF() {
            return new RectangleF(X, Y, Width, Height);
        }
    }

}
