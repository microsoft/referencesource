//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     An event handler used to notify of changes to the SelectedCells collection.
    /// </summary>
    /// <param name="sender">The DataGrid that owns the SelectedCells collection that changed.</param>
    /// <param name="e">Event arguments that communicate which cells were added or removed.</param>
    public delegate void SelectedCellsChangedEventHandler(object sender, SelectedCellsChangedEventArgs e);
}