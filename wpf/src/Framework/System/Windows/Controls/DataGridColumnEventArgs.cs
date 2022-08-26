//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Controls
{
    /// <summary>
    ///     EventArgs used for events related to DataGridColumn.
    /// </summary>
    public class DataGridColumnEventArgs : EventArgs
    {
        /// <summary>
        ///     Instantiates a new instance of this class.
        /// </summary>
        public DataGridColumnEventArgs(DataGridColumn column)
        {
            _column = column;
        }

        /// <summary>
        ///     DataGridColumn that the DataGridColumnEventArgs refers to
        /// </summary>
        public DataGridColumn Column
        {
            get { return _column; }
        }

        private DataGridColumn _column;
    }
}
