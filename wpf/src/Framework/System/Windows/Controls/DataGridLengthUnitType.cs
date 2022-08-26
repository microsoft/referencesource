//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Globalization;
using System.Windows;

namespace System.Windows.Controls
{
    /// <summary>
    ///     Used to indicate the type of value that DataGridLength is holding.
    /// </summary>
    public enum DataGridLengthUnitType 
    {
        // Keep in sync with DataGridLengthConverter.UnitStrings

        /// <summary>
        ///     The value indicates that content should be calculated based on the 
        ///     unconstrained sizes of all cells and header in a column.
        /// </summary>
        Auto,

        /// <summary>
        ///     The value is expressed in pixels.
        /// </summary>
        Pixel,

        /// <summary>
        ///     The value indicates that content should be be calculated based on the
        ///     unconstrained sizes of all cells in a column.
        /// </summary>
        SizeToCells,

        /// <summary>
        ///     The value indicates that content should be calculated based on the
        ///     unconstrained size of the column header.
        /// </summary>
        SizeToHeader,

        /// <summary>
        ///     The value is expressed as a weighted proportion of available space.
        /// </summary>
        Star,
    }
}