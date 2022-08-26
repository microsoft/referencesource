// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
 
using System.ComponentModel; 
using System.Runtime.InteropServices;
 
namespace System.Security
{
    [EditorBrowsable(EditorBrowsableState.Never)]
    public static class SecureStringMarshal
    {
		[SecuritySafeCritical]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static IntPtr SecureStringToCoTaskMemAnsi(SecureString s) => Marshal.SecureStringToCoTaskMemAnsi(s);
		[SecuritySafeCritical]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static IntPtr SecureStringToGlobalAllocAnsi(SecureString s) => Marshal.SecureStringToGlobalAllocAnsi(s);
		[SecuritySafeCritical]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static IntPtr SecureStringToCoTaskMemUnicode(SecureString s) => Marshal.SecureStringToCoTaskMemUnicode(s);
		[SecuritySafeCritical]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static IntPtr SecureStringToGlobalAllocUnicode(SecureString s) => Marshal.SecureStringToGlobalAllocUnicode(s);
    }
}
