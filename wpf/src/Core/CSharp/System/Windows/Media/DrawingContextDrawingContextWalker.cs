//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// Description: DrawingContextDrawingContextWalker is a DrawingContextWalker
//              that forwards all of it's calls to a DrawingContext.
//
// History:  
//
//  2004/11/23 : timothyc - Created it.
//  2005/06/20 : timothyc - Moved to codegen
//
//---------------------------------------------------------------------------

using MS.Internal;
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;

namespace System.Windows.Media
{
    /// <summary>
    /// DrawingContextDrawingContextWalker is a DrawingContextWalker
    /// that forwards all of it's calls to a DrawingContext.    
    /// </summary>
    internal partial class DrawingContextDrawingContextWalker: DrawingContextWalker
    {
        /// <summary>
        /// Constructor for DrawingContextDrawingContextWalker - this constructor accepts the 
        /// DrawingContext it should forward calls to.
        /// </summary>
        /// <param name="drawingContext"> DrawingContext - the DrawingContext to forward calls to </param>
        public DrawingContextDrawingContextWalker(DrawingContext drawingContext)
        {
            Debug.Assert (drawingContext != null);
            
            _drawingContext = drawingContext;
        } 

        // DrawingContext to forward calls to
        private DrawingContext _drawingContext;
    }
}

