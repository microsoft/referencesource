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

    sealed class SafeRegistryKey : SafeHandleZeroOrMinusOneIsInvalid
    {
        [SecurityCritical]
        internal SafeRegistryKey() : base(false) { }

        [SecurityCritical]
        internal SafeRegistryKey(IntPtr preexistingHandle, bool ownsHandle)
            : base(ownsHandle)
        {
            SetHandle(preexistingHandle);
        }
        
        protected override bool ReleaseHandle()
        {
            // Returns a Win32 error code, 0 for success
            int r = SafeNativeMethods.RegCloseKey(handle);
            return r == 0;
        }
    }
}
