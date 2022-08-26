//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     Interface to abstract away the difference between a DataGridCell and a DataGridColumnHeader
    /// </summary>
    internal interface IProvideDataGridColumn
    {
        DataGridColumn Column
        {
            get;
        }
    }
}