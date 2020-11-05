//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration.Install {
    using System;    
    using System.Runtime.InteropServices;    
    using System.ComponentModel;

    // not public!
    internal static class NativeMethods {
        [DllImport(ExternDll.Msi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int MsiCreateRecord(int cParams);
    
        [DllImport(ExternDll.Msi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int MsiRecordSetInteger(int hRecord, int iField, int iValue);
    
        [DllImport(ExternDll.Msi, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern int MsiRecordSetStringW(int hRecord, int iField, string szValue);

        [DllImport(ExternDll.Msi, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int MsiProcessMessage(int hInstall, int messageType, int hRecord); 
        
        public const int INSTALLMESSAGE_ERROR = 0x01000000;
    }
}  
