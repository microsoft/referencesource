//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

namespace System.Windows.Controls
{
    public enum DataGridRowDetailsVisibilityMode
    {
        Collapsed,              // Show no details by default. Developer must toggle visibility
        Visible,                // Show the details section for all rows
        VisibleWhenSelected     // Show the details section only for the selected row(s)
    }
}
