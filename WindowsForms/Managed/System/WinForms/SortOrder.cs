//------------------------------------------------------------------------------
// <copyright file="SortOrder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;



    /// <include file='doc\SortOrder.uex' path='docs/doc[@for="SortOrder"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how items in
    ///       a list are sorted.
    ///    </para>
    /// </devdoc>
    public enum SortOrder {

        /// <include file='doc\SortOrder.uex' path='docs/doc[@for="SortOrder.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The items are
        ///       not sorted.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\SortOrder.uex' path='docs/doc[@for="SortOrder.Ascending"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The items
        ///       are sorted in ascending order.
        ///    </para>
        /// </devdoc>
        Ascending = 1,

        /// <include file='doc\SortOrder.uex' path='docs/doc[@for="SortOrder.Descending"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The items are
        ///       sorted in descending order.
        ///    </para>
        /// </devdoc>
        Descending = 2,

    }
}
