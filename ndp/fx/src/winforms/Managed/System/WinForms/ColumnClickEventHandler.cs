//------------------------------------------------------------------------------
// <copyright file="ColumnClickEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\ColumnClickEventHandler.uex' path='docs/doc[@for="ColumnClickEventHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will handle the
    ///       the <see cref='System.Windows.Forms.ListView.OnColumnClick'/>event of a <see cref='System.Windows.Forms.ListView'/>
    ///       .
    ///       
    ///    </para>
    /// </devdoc>
    public delegate void ColumnClickEventHandler(object sender, ColumnClickEventArgs e);
}
