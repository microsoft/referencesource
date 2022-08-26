//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     Enum to specify the scroll orientation of cells in selective scroll grid
    /// </summary>
    public enum SelectiveScrollingOrientation
    {
        /// <summary>
        /// The cell will not be allowed to get
        /// sctolled in any direction
        /// </summary>
        None = 0,

        /// <summary>
        /// The cell will be allowed to
        /// get scrolled only in horizontal direction
        /// </summary>
        Horizontal = 1,

        /// <summary>
        /// The cell will be allowed to
        /// get scrolled only in vertical directions
        /// </summary>
        Vertical = 2,

        /// <summary>
        /// The cell will be allowed to get
        /// scrolled in all directions
        /// </summary>
        Both = 3
    }
}
