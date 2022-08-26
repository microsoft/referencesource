//------------------------------------------------------------------------------
// <copyright file="IWin32window.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;

    /// <include file='doc\IWin32window.uex' path='docs/doc[@for="IWin32Window"]/*' />
    /// <devdoc>
    ///    <para>Provides an interface to expose Win32 HWND handles.</para>
    /// </devdoc>
    [System.Runtime.InteropServices.Guid("458AB8A2-A1EA-4d7b-8EBE-DEE5D3D9442C"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    [System.Runtime.InteropServices.ComVisible(true)]
    public interface IWin32Window {
    
        /// <include file='doc\IWin32window.uex' path='docs/doc[@for="IWin32Window.Handle"]/*' />
        /// <devdoc>
        ///    <para>Gets the handle to the window represented by the implementor.</para>
        /// </devdoc>
        IntPtr Handle { get; }
    }
}
