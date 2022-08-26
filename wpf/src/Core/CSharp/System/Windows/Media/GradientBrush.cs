//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
// File: GradientBrush.cs
//
// Description: This file contains the implementation of GradientBrush.
//              The GradientBrush is an abstract class of Brushes which describes
//              a way to fill a region by a gradient.  Derived classes describe different
//              ways of interpreting gradient stops.
//
// History:
//  04/30/2003 : Microsoft - Created it.
//
//---------------------------------------------------------------------------
    
using System;
using System.Diagnostics;
using System.ComponentModel;
using System.Windows;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using MS.Internal;
using System.Runtime.InteropServices;
using System.Windows.Media.Composition;

namespace System.Windows.Media 
{
    /// <summary>
    /// GradientBrush
    /// The GradientBrush is an abstract class of Brushes which describes
    /// a way to fill a region by a gradient.  Derived classes describe different
    /// ways of interpreting gradient stops.
    /// </summary>
    [ContentProperty("GradientStops")]
    public abstract partial class GradientBrush : Brush
    {
        #region Constructors
        
        /// <summary>
        /// Protected constructor for GradientBrush
        /// </summary>
        protected GradientBrush()
        {
        }

        /// <summary>
        /// Protected constructor for GradientBrush
        /// Sets all the values of the GradientStopCollection, all other values are left as default.
        /// </summary>
        protected GradientBrush(GradientStopCollection gradientStopCollection) 
        {
            GradientStops = gradientStopCollection;
        }

        #endregion Constructors
    }
}
