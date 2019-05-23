//------------------------------------------------------------------------------
// <copyright file="ServiceAccount.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {
   using System;

    /// <include file='doc\ServiceAccount.uex' path='docs/doc[@for="ServiceAccount"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum ServiceAccount {
        
        /// <include file='doc\ServiceAccount.uex' path='docs/doc[@for="ServiceAccount.LocalService"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        LocalService = 0,
        /// <include file='doc\ServiceAccount.uex' path='docs/doc[@for="ServiceAccount.NetworkService"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        NetworkService = 1,
        /// <include file='doc\ServiceAccount.uex' path='docs/doc[@for="ServiceAccount.LocalSystem"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        LocalSystem = 2,
        /// <include file='doc\ServiceAccount.uex' path='docs/doc[@for="ServiceAccount.User"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        User = 3,
    }
}    
  