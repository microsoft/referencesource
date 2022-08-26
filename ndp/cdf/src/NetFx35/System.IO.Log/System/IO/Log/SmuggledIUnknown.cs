//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Services;
    
    internal sealed class SmuggledIUnknown : SafeHandle
    {
        internal SmuggledIUnknown() : base(IntPtr.Zero, true)
        {
        }

        internal SmuggledIUnknown(object unknown) : this()
        {
            RuntimeHelpers.PrepareConstrainedRegions();
            try
            {
            }
            finally
            {
                base.handle = Marshal.GetIUnknownForObject(unknown);
            }
        }

        public override bool IsInvalid
        {
            get
            {
                return (IsClosed || (IntPtr.Zero == base.handle));
            }
        }

        public object Smuggle()
        {
            object comObject;
            RuntimeHelpers.PrepareConstrainedRegions();
            try
            {
            }
            finally
            {
                comObject = EnterpriseServicesHelper.WrapIUnknownWithComObject(base.handle);
                GC.KeepAlive(this);
            }
            return comObject;
        }

        override protected bool ReleaseHandle()
        {
            if (base.handle != IntPtr.Zero)
            {
                Marshal.Release(base.handle);
                base.handle = IntPtr.Zero;
            }

            return true;
        }
    }    
}
