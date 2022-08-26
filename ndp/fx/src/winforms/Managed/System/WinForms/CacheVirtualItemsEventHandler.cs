//------------------------------------------------------------------------------
// <copyright file="CacheVirtualItemsEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;

    /// <include file='doc\CacheVirtualItemsEventHandler.uex' path='docs/doc[@for="CacheVirtualItemsEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the CacheVirtualItems event of a ListView.
    ///    </para>
    /// </devdoc>
    public delegate void CacheVirtualItemsEventHandler(object sender, CacheVirtualItemsEventArgs e);
}
