//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Input
{
    /// <summary>
    ///     Represents various actions that occur with touch devices.
    /// </summary>
    public enum TouchAction
    {
        /// <summary>
        ///     The act of putting a finger onto the screen.
        /// </summary>
        Down,

        /// <summary>
        ///     The act of dragging a finger across the screen.
        /// </summary>
        Move,

        /// <summary>
        ///     The act of lifting a finger off of the screen.
        /// </summary>
        Up,
    }
}
