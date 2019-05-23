//------------------------------------------------------------------------------
// <copyright file="ServiceControllerPermissionAccess.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {

    /// <include file='doc\ServiceControllerPermissionAccess.uex' path='docs/doc[@for="ServiceControllerPermissionAccess"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]         
    public enum ServiceControllerPermissionAccess {
        /// <include file='doc\ServiceControllerPermissionAccess.uex' path='docs/doc[@for="ServiceControllerPermissionAccess.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\ServiceControllerPermissionAccess.uex' path='docs/doc[@for="ServiceControllerPermissionAccess.Browse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Browse = 1 << 1,
        /// <include file='doc\ServiceControllerPermissionAccess.uex' path='docs/doc[@for="ServiceControllerPermissionAccess.Control"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Control = 1 << 2 | Browse,
    }    
}  
  
