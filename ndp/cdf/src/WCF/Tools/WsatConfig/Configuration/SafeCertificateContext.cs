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
    using System.Security.Cryptography.X509Certificates;
    
    internal sealed class SafeCertificateContext : SafeHandleZeroOrMinusOneIsInvalid
    {
        [SecurityCritical]
        internal SafeCertificateContext() : base(false) { }

        public override bool IsInvalid
        {
            get
            {
                return handle == IntPtr.Zero;
            }
        }

        protected override bool ReleaseHandle()
        {
            // Returns a Win32 error code, 0 for success
            int r = SafeNativeMethods.CertFreeCertificateContext(handle);
            return r == 0;
        }

        internal X509Certificate2 GetNewX509Certificate()
        {
            return new X509Certificate2(handle);
        }
    }
}
