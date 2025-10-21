//------------------------------------------------------------------------------
// <copyright file="DataGridViewToolTip.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>            
//------------------------------------------------------------------------------

using System.Security;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;

namespace System.Windows.Forms
{
    public partial class DataGridView
    {
        private class DataGridViewToolTip
        {
            DataGridView dataGridView = null;
            ToolTip toolTip = null;
            private bool toolTipActivated = false;

            public DataGridViewToolTip(DataGridView dataGridView)
            {
                this.dataGridView = dataGridView;
            }

            public bool Activated
            {
                get
                {
                    return this.toolTipActivated;
                }
            }

            public ToolTip ToolTip
            {
                get
                {
                    return this.toolTip;
                }
            }

            public void Activate(bool activate)
            {
                if (this.dataGridView.DesignMode)
                {
                    return;
                }

                // Create the tool tip handle on demand.
                if (activate && this.toolTip == null)
                {
                    this.toolTip = new ToolTip();
                    this.toolTip.ShowAlways = true;
                    this.toolTip.InitialDelay = 0;
                    this.toolTip.UseFading = false;
                    this.toolTip.UseAnimation = false;
                    this.toolTip.AutoPopDelay = 0;
                }

                if (this.dataGridView.IsRestricted)
                {
                    IntSecurity.AllWindows.Assert();
                }
                try
                {
                    if (activate)
                    {
                        this.toolTip.Active = true;
                        this.toolTip.Show(this.dataGridView.ToolTipPrivate, this.dataGridView);
                    }
                    else if (this.toolTip != null)
                    {
                        this.toolTip.Hide(this.dataGridView);
                        this.toolTip.Active = false;
                    }
                }
                finally
                {
                    if (this.dataGridView.IsRestricted)
                    {
                        CodeAccessPermission.RevertAssert();
                    }
                }

                this.toolTipActivated = activate;
            }

            public void Dispose()
            {
                if (this.toolTip != null)
                {
                    this.toolTip.Dispose();
                    this.toolTip = null;
                }
            }
        }
    }
}
