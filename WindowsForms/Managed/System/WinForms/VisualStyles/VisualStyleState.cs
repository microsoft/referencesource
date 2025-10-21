//------------------------------------------------------------------------------
// <copyright file="VisualStyleState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.MSInternal", "CA905:SystemAndMicrosoftNamespacesRequireApproval", Scope="namespace", Target="System.Windows.Forms.VisualStyles")]

namespace System.Windows.Forms.VisualStyles {

    /// <include file='doc\VisualStyleState.uex' path='docs/doc[@for="VisualStyleState"]/*' />
    /// <devdoc>
    ///    <para>
    ///     Determines whether visual styles are enabled.
    ///    </para>
    /// </devdoc>

    public enum VisualStyleState {
        /// <include file='doc\VisualStyleState.uex' path='docs/doc[@for="VisualStyleState.NoneEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///  Visual styles are not enabled.
        ///    </para>
        /// </devdoc>
        NoneEnabled = 0,

        /// <include file='doc\VisualStyleState.uex' path='docs/doc[@for="VisualStyleState.ClientAreaEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        /// Visual styles enabled only for client area.
        ///    </para>
        /// </devdoc>
        ClientAreaEnabled = NativeMethods.STAP_ALLOW_CONTROLS,

        /// <include file='doc\VisualStyleState.uex' path='docs/doc[@for="VisualStyleState.NonClientAreaEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        /// Visual styles enabled only for non-client area.
        ///    </para>
        /// </devdoc>
        NonClientAreaEnabled = NativeMethods.STAP_ALLOW_NONCLIENT,

        /// <include file='doc\VisualStyleState.uex' path='docs/doc[@for="VisualStyleState.ClientAndNonClientAreasEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        /// Visual styles enabled only for client and non-client areas. 
        ///    </para>
        /// </devdoc>
       ClientAndNonClientAreasEnabled = NativeMethods.STAP_ALLOW_NONCLIENT | NativeMethods.STAP_ALLOW_CONTROLS
    }
}
