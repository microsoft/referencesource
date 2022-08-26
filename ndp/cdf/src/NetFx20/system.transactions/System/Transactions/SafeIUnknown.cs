//------------------------------------------------------------------------------
// <copyright file="SafeIUnknown.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Transactions
{

    using System;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Runtime.ConstrainedExecution;
    using System.Security.Permissions;

    // Keep an interface pointer that will not be used in a SafeHandle derived so
    // that it will be properly released.
    internal sealed class SafeIUnknown : SafeHandle 
    {
        // This constructor is called by ComInterop        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal SafeIUnknown() : base(IntPtr.Zero, true)
        {
        }

        // This constructor is called by ComInterop        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal SafeIUnknown(IntPtr unknown) : base(IntPtr.Zero, true)
        {
            RuntimeHelpers.PrepareConstrainedRegions();
            try { } 
            finally 
            {
                base.handle = unknown;
            }
        }
        
        public override bool IsInvalid 
        {
            get 
            {
                return (IsClosed || (IntPtr.Zero == base.handle));
            }
        }

        override protected bool ReleaseHandle() 
        {
            // NOTE: The SafeHandle class guarantees this will be called exactly once.
            IntPtr ptr = base.handle;
            base.handle = IntPtr.Zero;
            if (IntPtr.Zero != ptr) 
            {
                Marshal.Release(ptr);
            }
            return true;
        }
    }
}

