//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.ConstrainedExecution;
    using System.Runtime.Versioning;
    using System.Security;

    internal sealed class CoTaskMemHandle : SafeHandle
    {
        public CoTaskMemHandle() : base(IntPtr.Zero, true)
        {
        }

        public override bool IsInvalid
        {
            get
            {
                return IsClosed || this.handle == IntPtr.Zero;
            }
        }
        
        [DllImport("ole32.dll"),
        SuppressUnmanagedCodeSecurity,
        ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        [ResourceExposure(ResourceScope.None)]     
        private static extern void CoTaskMemFree(IntPtr ptr);
        override protected bool ReleaseHandle()
        {
            CoTaskMemFree(this.handle);
            return true;
        }
    }    
}
