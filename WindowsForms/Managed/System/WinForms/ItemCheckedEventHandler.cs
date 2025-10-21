//------------------------------------------------------------------------------
// <copyright file="ItemCheckedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\ItemCheckedEventHandler.uex' path='docs/doc[@for="ItemCheckedEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will
    ///       handle the <see langword='ItemCheck'/> event of a
    ///    <see cref='System.Windows.Forms.CheckedListBox'/> or 
    ///    <see cref='System.Windows.Forms.ListView'/>.
    ///       
    ///    </para>
    /// </devdoc>
    public delegate void ItemCheckedEventHandler(object sender, ItemCheckedEventArgs e);
}
