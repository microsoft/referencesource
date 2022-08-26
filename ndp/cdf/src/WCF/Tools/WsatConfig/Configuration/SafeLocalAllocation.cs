//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using System.Runtime.ConstrainedExecution;
    using Microsoft.Win32.SafeHandles;

    sealed class SafeLocalAllocation : SafeHandleZeroOrMinusOneIsInvalid
    {
        [SecurityCritical]
        internal SafeLocalAllocation(int size)
            : base(true)
        {
#pragma warning suppress 56523
            IntPtr pointer = SafeNativeMethods.LocalAlloc(SafeNativeMethods.LMEM_ZEROINIT, size);
            
            SetHandle(pointer);
        }

#if WSAT_UI
        [SecurityCritical]
        internal SafeLocalAllocation(object source)
            : base(true)
        {
#pragma warning suppress 56523
            IntPtr pointer = SafeNativeMethods.LocalAlloc(SafeNativeMethods.LMEM_ZEROINIT, Marshal.SizeOf(source));
            
            SetHandle(pointer);
            Marshal.StructureToPtr(
                source,
                handle,
                false);
        }
#endif

        internal void Copy(byte[] source, int startIndex, int count)
        {
            System.Runtime.InteropServices.Marshal.Copy(
                    source,
                    startIndex,
                    handle,
                    count);
        }

        protected override bool ReleaseHandle()
        {
#pragma warning suppress 56523
            IntPtr r = SafeNativeMethods.LocalFree(handle);
            return r == IntPtr.Zero;
        }
    }
}
