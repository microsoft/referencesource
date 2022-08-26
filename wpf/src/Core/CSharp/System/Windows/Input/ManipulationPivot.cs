//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;

namespace System.Windows.Input
{
    /// <summary>
    ///     Data regarding a pivot associated with a manipulation.
    /// </summary>
    public class ManipulationPivot
    {
        /// <summary>
        ///     Initializes a new instance of this object.
        /// </summary>
        public ManipulationPivot()
        {
        }

        /// <summary>
        ///     Initializes a new instance of this object.
        /// </summary>
        public ManipulationPivot(Point center, double radius)
        {
            Center = center;
            Radius = radius;
        }

        /// <summary>
        ///     Location of a pivot.
        /// </summary>
        public Point Center
        {
            get;
            set;
        }

        /// <summary>
        ///     The area that is considered "close" to the pivot.
        ///     Movement within this area will dampen the effect of rotation.
        /// </summary>
        public double Radius
        {
            get;
            set;
        }
    }
}
