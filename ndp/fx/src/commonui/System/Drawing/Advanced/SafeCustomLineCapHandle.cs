//------------------------------------------------------------------------------
// <copyright file="SafeCustomLineCapHandle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security;

    [SecurityCritical]
    internal class SafeCustomLineCapHandle : SafeHandle {

        // Create a SafeHandle, informing the base class
        // that this SafeHandle instance "owns" the handle,
        // and therefore SafeHandle should call
        // our ReleaseHandle method when the SafeHandle
        // is no longer in use.
        internal SafeCustomLineCapHandle(IntPtr h)
            : base(IntPtr.Zero, true) {
                SetHandle(h);
        }

        [SecurityCritical]
        override protected bool ReleaseHandle() {
            int status = SafeNativeMethods.Gdip.Ok;
            if (!IsInvalid)
            {
                try {
                    status = SafeNativeMethods.Gdip.GdipDeleteCustomLineCap(new HandleRef(this, handle));
                }
                catch (Exception ex) {
                    if (ClientUtils.IsSecurityOrCriticalException(ex)) {
                        throw;
                    }

                    Debug.Fail("Exception thrown during ReleaseHandle: " + ex.ToString());
                }
                finally {
                    handle = IntPtr.Zero;
                }
                Debug.Assert(status == SafeNativeMethods.Gdip.Ok, "GDI+ returned an error status: " + status.ToString(CultureInfo.InvariantCulture));
            }
            return status == SafeNativeMethods.Gdip.Ok;
        }

        public override bool IsInvalid {
            get { return handle == IntPtr.Zero; }
        }
    
        public static implicit operator IntPtr(SafeCustomLineCapHandle handle) {
            return (handle == null) ? IntPtr.Zero : handle.handle;
        }

        public static explicit operator SafeCustomLineCapHandle(IntPtr handle) {
            return new SafeCustomLineCapHandle(handle);
        }
    }
}
