//------------------------------------------------------------------------------
// <copyright file="IManagedInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration.Install {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;

    /// <include file='doc\IManagedInstaller.uex' path='docs/doc[@for="IManagedInstaller"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [ComImport, Guid("1E233FE7-C16D-4512-8C3B-2E9988F08D38"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    public interface IManagedInstaller {

        /// <include file='doc\IManagedInstaller.uex' path='docs/doc[@for="IManagedInstaller.ManagedInstall"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
    	[return: MarshalAs(UnmanagedType.I4)]
        int ManagedInstall(
    		[In, MarshalAs(UnmanagedType.BStr)] 
            string commandLine,

            [In, MarshalAs(UnmanagedType.I4)] 
            int hInstall);  // this handle is alway 32 bits (even on a 64 bit machine)
    }
}
