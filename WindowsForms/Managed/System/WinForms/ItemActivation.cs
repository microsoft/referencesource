//------------------------------------------------------------------------------
// <copyright file="ItemActivation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;



    /// <include file='doc\ItemActivation.uex' path='docs/doc[@for="ItemActivation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how the user activates items and the appearance
    ///       of items as the mouse cursor moves over them.
    ///
    ///    </para>
    /// </devdoc>
    public enum ItemActivation {

        /// <include file='doc\ItemActivation.uex' path='docs/doc[@for="ItemActivation.Standard"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Activate items with a double-click.
        ///       Items do not change appearance.
        ///
        ///    </para>
        /// </devdoc>
        Standard = 0,

        /// <include file='doc\ItemActivation.uex' path='docs/doc[@for="ItemActivation.OneClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Activate
        ///       items with a single click. The cursor changes shape and the item
        ///       text changes color.
        ///
        ///    </para>
        /// </devdoc>
        OneClick = 1,

        /// <include file='doc\ItemActivation.uex' path='docs/doc[@for="ItemActivation.TwoClick"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Activate items with a
        ///       double click. The item text changes color.
        ///
        ///    </para>
        /// </devdoc>
        TwoClick = 2,

    }
}
