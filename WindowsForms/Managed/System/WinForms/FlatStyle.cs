//------------------------------------------------------------------------------
// <copyright file="FlatStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    /// <include file='doc\FlatStyle.uex' path='docs/doc[@for="FlatStyle"]/*' />
    /// <devdoc>
    ///    <para>Specifies the style of control to display.</para>
    /// </devdoc>
    public enum FlatStyle {
        /// <include file='doc\FlatStyle.uex' path='docs/doc[@for="FlatStyle.Flat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The control appears flat.
        ///    </para>
        /// </devdoc>
        Flat,
        /// <include file='doc\FlatStyle.uex' path='docs/doc[@for="FlatStyle.Popup"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A control appears flat until the mouse pointer
        ///       moves over
        ///       it, at which point it appears three-dimensional.
        ///    </para>
        /// </devdoc>
        Popup,
        /// <include file='doc\FlatStyle.uex' path='docs/doc[@for="FlatStyle.Standard"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The control appears three-dimensional.
        ///    </para>
        /// </devdoc>
        Standard,
        /// <include file='doc\FlatStyle.uex' path='docs/doc[@for="FlatStyle.System"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The control appears three-dimensional.
        ///    </para>
        /// </devdoc>
        System,
    }
}

