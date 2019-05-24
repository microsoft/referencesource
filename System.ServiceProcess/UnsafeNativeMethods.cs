//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {    
    using System;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    
    [
    ComVisible(false), 
    SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal static class UnsafeNativeMethods {
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public unsafe extern static bool ControlService(IntPtr serviceHandle, int control, NativeMethods.SERVICE_STATUS *pStatus);
                       
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern unsafe bool QueryServiceStatus(IntPtr serviceHandle, NativeMethods.SERVICE_STATUS *pStatus);
                
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool EnumServicesStatus(IntPtr databaseHandle, int serviceType, int serviceState,
             IntPtr status, int size, out int bytesNeeded, out int servicesReturned, ref int resumeHandle);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
         public extern static bool EnumServicesStatusEx(IntPtr databaseHandle, int infolevel, int serviceType, int serviceState,
             IntPtr status, int size, out int bytesNeeded, out int servicesReturned, ref int resumeHandle, string group);                

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static IntPtr OpenService(IntPtr databaseHandle, string serviceName, int access);
         
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool StartService(IntPtr serviceHandle, int argNum, IntPtr argPtrs);
                                                
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool EnumDependentServices(IntPtr serviceHandle, int serviceState, IntPtr bufferOfENUM_SERVICE_STATUS,
            int bufSize, ref int bytesNeeded, ref int numEnumerated);
            
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool QueryServiceConfig(IntPtr serviceHandle, IntPtr query_service_config_ptr, int bufferSize, out int bytesNeeded);                                                
                    
    }                                                                                                                                                                          
}    

