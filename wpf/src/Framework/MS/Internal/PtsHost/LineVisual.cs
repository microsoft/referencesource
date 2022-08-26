//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: LineVisaul.cs
//
// Description: Visual representing line of text. 
//
// History:  
//  05/20/2003 : Microsoft - created.
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Media;

namespace MS.Internal.PtsHost
{
    // ----------------------------------------------------------------------
    // Visual representing line of text.
    // ----------------------------------------------------------------------
    internal sealed class LineVisual : DrawingVisual
    {
        // ------------------------------------------------------------------
        // Open drawing context.
        // ------------------------------------------------------------------
        internal DrawingContext Open()
        {
            return RenderOpen();
        }
        
        internal double WidthIncludingTrailingWhitespace;
    }
}
