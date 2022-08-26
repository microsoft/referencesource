//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2005
//
//  File:       BlurImageEffect.cs
//------------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Media;

namespace System.Windows.Media.Effects
{

    /// <summary>
    /// BlurEffect
    /// </summary>
    public partial class BlurEffect
    {

        #region Constructors
        /// <summary>
        /// Constructor
        /// </summary>
        public BlurEffect()
        {
            
        }

        #endregion
        
        /// <summary>
        /// Takes in content bounds, and returns the bounds of the rendered
        /// output of that content after the Effect is applied.
        /// </summary>
        internal override Rect GetRenderBounds(Rect contentBounds)
        {
            Point topLeft = new Point();
            Point bottomRight = new Point();

            double radius = Radius;
            topLeft.X = contentBounds.TopLeft.X - radius;
            topLeft.Y = contentBounds.TopLeft.Y - radius;
            bottomRight.X = contentBounds.BottomRight.X + radius;
            bottomRight.Y = contentBounds.BottomRight.Y + radius;
            
            return new Rect(topLeft, bottomRight);
        }
    }
}

