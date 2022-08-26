//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     Used to specify action to take out of edit mode.
    /// </summary>
    public enum DataGridEditAction
    {
        /// <summary>
        ///     Cancel the changes.
        /// </summary>
        Cancel,

        /// <summary>
        ///     Commit edited value.
        /// </summary>
        Commit
    }
}