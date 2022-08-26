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

    sealed class SafeCertificateStore : SafeHandleZeroOrMinusOneIsInvalid
    {
        [SecurityCritical]
        SafeCertificateStore()
            : base(false)
        {
            return;
        }

        public override bool IsInvalid
        {
            get
            {
                return handle == IntPtr.Zero;
            }
        }

        protected override bool ReleaseHandle()
        {
#pragma warning suppress 56523
            return SafeNativeMethods.CertCloseStore(handle, 0);
        }
    }
}
