//------------------------------------------------------------------------------
// <copyright file="DataGridViewCellStyleChangedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    internal class DataGridViewCellStyleChangedEventArgs : EventArgs
    {
        private bool changeAffectsPreferredSize;

        internal DataGridViewCellStyleChangedEventArgs()
        {
        }

        internal bool ChangeAffectsPreferredSize
        {
            get
            {
                return this.changeAffectsPreferredSize;
            }
            set
            {
                this.changeAffectsPreferredSize = value;
            }
        }
    }
}
