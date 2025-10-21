//------------------------------------------------------------------------------
// <copyright file="ItemDragEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\ItemDragEventHandler.uex' path='docs/doc[@for="ItemDragEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the <see cref='System.Windows.Forms.ListView.OnItemDrag'/>
    ///       
    ///       event of a
    ///    <see cref='System.Windows.Forms.ListView'/> 
    ///    .
    ///    
    /// </para>
    /// </devdoc>
    public delegate void ItemDragEventHandler(object sender, ItemDragEventArgs e);
}
