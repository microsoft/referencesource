//------------------------------------------------------------------------------
// <copyright file="DockingBehavior.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    /// <include file='doc\DockingBehavior.uex' path='docs/doc[@for="DockingBehavior"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies whether any characters in the
    ///       current selection have the style or attribute.
    ///
    ///    </para>
    /// </devdoc>
    public enum DockingBehavior {
        /// <include file='doc\DockingBehavior.uex' path='docs/doc[@for="DockingBehavior.Never"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Some but not all characters.
        ///    </para>
        /// </devdoc>
        Never     = 0,

        /// <include file='doc\DockingBehavior.uex' path='docs/doc[@for="DockingBehavior.Ask"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No characters.
        ///    </para>
        /// </devdoc>
        Ask      = 1,

        /// <include file='doc\DockingBehavior.uex' path='docs/doc[@for="DockingBehavior.AutoDock"]/*' />
        /// <devdoc>
        ///    <para>
        ///       All characters.
        ///    </para>
        /// </devdoc>
        AutoDock = 2
    }
}
