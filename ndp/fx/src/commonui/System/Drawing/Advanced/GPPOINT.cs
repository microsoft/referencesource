//------------------------------------------------------------------------------
// <copyright file="GPPOINT.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Internal {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal class GPPOINT
    {
        internal int X;
        internal int Y;

    	 internal GPPOINT()
    	 {
    	 }

    	 internal GPPOINT(PointF pt)
    	 {
    		 X = (int) pt.X;
    		 Y = (int) pt.Y;
    	 }

    	 internal GPPOINT(Point pt)
    	 {
    		 X = pt.X;
    		 Y = pt.Y;
    	 }

    	 internal PointF ToPoint()
    	 {
    		 return new PointF(X, Y);
    	 }
    }

}
