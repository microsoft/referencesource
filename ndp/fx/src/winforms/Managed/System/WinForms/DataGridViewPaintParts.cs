//------------------------------------------------------------------------------
// <copyright file="DataGridViewPaintParts.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.DataGridViewPaintParts"]/*' />
    [Flags]
    public enum DataGridViewPaintParts
    {
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.None"]/*' />
        None = 0x00,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.All"]/*' />
        All = 0x7F,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.Background"]/*' />
        Background = 0x01,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.Border"]/*' />
        Border = 0x02,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.ContentBackground"]/*' />
        ContentBackground = 0x04,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.ContentForeground"]/*' />
        ContentForeground = 0x08,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.ErrorIcon"]/*' />
        ErrorIcon = 0x10,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.Focus"]/*' />
        Focus = 0x20,
        /// <include file='doc\DataGridViewPaintParts.uex' path='docs/doc[@for="DataGridViewPaintParts.SelectedBackground"]/*' />
        SelectionBackground = 0x40
    }
}
