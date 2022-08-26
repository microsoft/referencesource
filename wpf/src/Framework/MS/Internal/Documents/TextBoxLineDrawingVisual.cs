//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// File: TextBoxLineDrawingVisual 
//
// Description: Extension of DrawingVisual for state that TextBoxView needs.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace System.Windows.Controls
{
    /// <summary>
    /// Extension of DrawingVisual for state that TextBoxView needs.
    /// </summary>
    internal class TextBoxLineDrawingVisual : DrawingVisual
    {
        /// <summary>
        /// Whether this line visual should be removed from the visual tree on Arrange.
        /// </summary>
        internal bool DiscardOnArrange { get; set; }
    }
}
