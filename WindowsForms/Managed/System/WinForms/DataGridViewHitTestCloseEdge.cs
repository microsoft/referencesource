//------------------------------------------------------------------------------
// <copyright file="DataGridViewHitTestCloseEdge.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;
    using System.ComponentModel;

    /// <include file='doc\DataGridViewHitTestCloseEdge.uex' path='docs/doc[@for="DataGridViewHitTestTypeCloseEdge"]/*' />
    internal enum DataGridViewHitTestTypeCloseEdge 
    {
        /// <include file='doc\DataGridViewHitTestCloseEdge.uex' path='docs/doc[@for="DataGridViewHitTestTypeCloseEdge.None"]/*' />
        None   = 0,

        /// <include file='doc\DataGridViewHitTestCloseEdge.uex' path='docs/doc[@for="DataGridViewHitTestTypeCloseEdge.Left"]/*' />
        Left   = 1,

        /// <include file='doc\DataGridViewHitTestCloseEdge.uex' path='docs/doc[@for="DataGridViewHitTestTypeCloseEdge.Right"]/*' />
        Right  = 2,

        /// <include file='doc\DataGridViewHitTestCloseEdge.uex' path='docs/doc[@for="DataGridViewHitTestTypeCloseEdge.Top"]/*' />
        Top    = 3,

        /// <include file='doc\DataGridViewHitTestCloseEdge.uex' path='docs/doc[@for="DataGridViewHitTestTypeCloseEdge.Bottom"]/*' />
        Bottom = 4
    }
}
