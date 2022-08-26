//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;
using System.Windows;

namespace System.Windows.Controls
{
    public class DataGridRowEventArgs : EventArgs
    {
        public DataGridRowEventArgs(DataGridRow row)
        {
            Row = row;
        }

        public DataGridRow Row 
        { 
            get; private set; 
        }
    }
}
