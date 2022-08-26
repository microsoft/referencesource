//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Text;

namespace System.Windows.Controls
{
    /// <summary>
    /// Event args for sorting event on datagrid
    /// </summary>
    public class DataGridSortingEventArgs : DataGridColumnEventArgs
    {
        public DataGridSortingEventArgs(DataGridColumn column)
            : base(column)
        {
        }

        /// <summary>
        /// To indicate that the sort has been handled
        /// </summary>
        public bool Handled
        {
            get
            {
                return _handled;
            }

            set
            {
                _handled = value;
            }
        }

        private bool _handled = false;
    }
}
