//---------------------------------------------------------------------------
//
// <copyright file="SafeProcessHandle.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//              
// History:  
//  10/04/2003 : Microsoft    Created
//---------------------------------------------------------------------------

// PRESHARP: In order to avoid generating warnings about unkown message numbers and unknown pragmas.
#pragma warning disable 1634, 1691

using System;
using System.Security;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Windows.Automation;
using Microsoft.Win32.SafeHandles;
using MS.Win32;

namespace MS.Internal.Automation
{
    internal sealed class SafeProcessHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        // This constructor is used by the P/Invoke marshaling layer
        // to allocate a SafeHandle instance.  P/Invoke then does the
        // appropriate method call, storing the handle in this class.
        private SafeProcessHandle() : base(true) {}

        internal SafeProcessHandle(NativeMethods.HWND hwnd) : base(true)
        {
            int processId;

            // Get process id...
            // GetWindowThreadProcessId does use SetLastError().  So a call to GetLastError() would be meanless.
            // Disabling the PreSharp warning.
#pragma warning suppress 6523
            if (SafeNativeMethods.GetWindowThreadProcessId(hwnd, out processId) == 0)
            {
                throw new ElementNotAvailableException();
            }

            SetHandle(Misc.OpenProcess(UnsafeNativeMethods.PROCESS_QUERY_INFORMATION | UnsafeNativeMethods.PROCESS_VM_READ, false, processId, hwnd));
        }

        // 



        protected override bool ReleaseHandle()
        {
            return Misc.CloseHandle(handle);
        }
    }
}
