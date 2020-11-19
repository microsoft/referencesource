//------------------------------------------------------------------------------
// <copyright file="UninstallAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {

    using System.Diagnostics;

    /// <include file='doc\UninstallAction.uex' path='docs/doc[@for="UninstallAction"]/*' />
    /// <devdoc>
    ///    <para> An enum of what an installer can do at uninstall time.
    ///       It can either remove the resource it created, or do nothing (leaving the resource as installed).</para>
    /// </devdoc>
    public enum UninstallAction {
        /// <include file='doc\UninstallAction.uex' path='docs/doc[@for="UninstallAction.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Remove,
        /// <include file='doc\UninstallAction.uex' path='docs/doc[@for="UninstallAction.NoAction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NoAction
    }

}
