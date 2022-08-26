//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.ConstrainedExecution;
    using System.Runtime.Versioning;
    using System.Security;
    
    internal sealed class SafeMarshalContext : SafeHandle
    {
        public SafeMarshalContext() : base(IntPtr.Zero, true)
        {
        }

        public override bool IsInvalid
        {
            get
            {
                return IsClosed || this.handle == IntPtr.Zero;
            }
        }

        [DllImport("clfsw32.dll"),
         SuppressUnmanagedCodeSecurity,
         ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [ResourceExposure(ResourceScope.None)]
        private static extern bool DeleteLogMarshallingArea(IntPtr ptr);
        
        override protected bool ReleaseHandle()
        {
            return DeleteLogMarshallingArea(this.handle);
        }
    }
}
