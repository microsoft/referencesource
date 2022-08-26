//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     Delegate used for the InitializingNewItem event on DataGrid.
    /// </summary>
    /// <param name="sender">The DataGrid that raised the event.</param>
    /// <param name="e">The event arguments where callbacks can access the new item.</param>
    public delegate void InitializingNewItemEventHandler(object sender, InitializingNewItemEventArgs e);
}
