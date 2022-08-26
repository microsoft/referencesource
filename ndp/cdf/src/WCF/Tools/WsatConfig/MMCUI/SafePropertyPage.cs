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

    public sealed class SafePropertyPage : SafeHandleZeroOrMinusOneIsInvalid
    {
        [SecurityCritical]
        internal SafePropertyPage(PropSheetPage psp, bool ownsHandle)
            : base(ownsHandle)
        {
            SetHandle(SafeNativeMethods.CreatePropertySheetPage(ref psp));
        }

        protected override bool ReleaseHandle()
        {
            return SafeNativeMethods.DestroyPropertySheetPage(handle);
        }
    }
}
