//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Reflection;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using Microsoft.Win32.SafeHandles;
    using System.Resources;
    using System.Text;
    using _SYSKIND = System.Runtime.InteropServices.ComTypes.SYSKIND;
    using System.Globalization;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;
    using System.Runtime.CompilerServices;
    using System.Diagnostics.CodeAnalysis;


    internal delegate int CallbackThreadSet();
    internal delegate int CallbackThreadUnset();
    internal delegate void RuntimeLoadedCallback(IClrRuntimeInfo pRuntimeInfo, CallbackThreadSet callbackThreadSet, CallbackThreadUnset callbackThreadUnset);

    internal class RegistryHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        internal static readonly RegistryHandle HKEY_CLASSES_ROOT = new RegistryHandle(new IntPtr(unchecked((int)0x80000000)), false);
        internal static readonly RegistryHandle HKEY_CURRENT_USER = new RegistryHandle(new IntPtr(unchecked((int)0x80000001)), false);
        internal static readonly RegistryHandle HKEY_LOCAL_MACHINE = new RegistryHandle(new IntPtr(unchecked((int)0x80000002)), false);
        internal static readonly RegistryHandle HKEY_USERS = new RegistryHandle(new IntPtr(unchecked((int)0x80000003)), false);
        internal static readonly RegistryHandle HKEY_PERFORMANCE_DATA = new RegistryHandle(new IntPtr(unchecked((int)0x80000004)), false);
        internal static readonly RegistryHandle HKEY_CURRENT_CONFIG = new RegistryHandle(new IntPtr(unchecked((int)0x80000005)), false);
        internal static readonly RegistryHandle HKEY_DYN_DATA = new RegistryHandle(new IntPtr(unchecked((int)0x80000006)), false);

        static RegistryHandle GetHKCR()
        {
            RegistryHandle regHandle = null;
            int status = SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, @"Software\Classes", 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE, out regHandle);
            if (status != SafeNativeMethods.ERROR_SUCCESS || null == regHandle || regHandle.IsInvalid)
            {
                throw Tool.CreateException(SR.GetString(SR.FailedToOpenRegistryKey, ""), null);
            }
            return regHandle;

        }

        static RegistryHandle Get64bitHKCR()
        {
            RegistryHandle regHandle = null;
            int status = SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, @"Software\Classes", 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE | SafeNativeMethods.KEY_WOW64_64KEY, out regHandle);
            if (status != SafeNativeMethods.ERROR_SUCCESS || null == regHandle || regHandle.IsInvalid)
            {
                throw Tool.CreateException(SR.GetString(SR.FailedToOpenRegistryKey, ""), null);
            }
            return regHandle;
        }

        static RegistryHandle Get32bitHKCR()
        {
            RegistryHandle regHandle = null;
            int status = SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, @"Software\Classes", 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE | SafeNativeMethods.KEY_WOW64_32KEY, out regHandle);
            if (status != SafeNativeMethods.ERROR_SUCCESS || null == regHandle || regHandle.IsInvalid)
            {
                throw Tool.CreateException(SR.GetString(SR.FailedToOpenRegistryKey, ""), null);
            }
            return regHandle;
        }

        static RegistryHandle GetCorrectBitnessHive(bool is64bit)
        {
            if (is64bit && IntPtr.Size == 8) // No worries we are trying to open up a 64 bit hive just return 
                return GetHKCR();
            else if (is64bit && IntPtr.Size == 4) // we are running under wow get the 64 bit hive
                return Get64bitHKCR();
            else if (!is64bit && IntPtr.Size == 8) // we are running in 64 bit but need to open a 32 bit hive
                return Get32bitHKCR();
            else if (!is64bit && IntPtr.Size == 4)
                return GetHKCR();

            throw Tool.CreateException(SR.GetString(SR.UnableToDetermineHiveBitness), null);
        }

        public static RegistryHandle GetBitnessHKCR(bool is64bit)
        {
            return GetCorrectBitnessHive(is64bit);
        }


        public static RegistryHandle GetCorrectBitnessHKLMSubkey(bool is64bit, string key, bool isWriteRequired)
        {
            if (is64bit && IntPtr.Size == 8) // No worries we are trying to open up a 64 bit hive just return 
                return GetHKLMSubkey(key, isWriteRequired);
            else if (is64bit && IntPtr.Size == 4) // we are running under wow get the 64 bit hive
                return Get64bitHKLMSubkey(key, isWriteRequired);
            else if (!is64bit && IntPtr.Size == 8) // we are running in 64 bit but need to open a 32 bit hive
                return Get32bitHKLMSubkey(key, isWriteRequired);
            else if (!is64bit && IntPtr.Size == 4)
                return GetHKLMSubkey(key, isWriteRequired);

            throw Tool.CreateException(SR.GetString(SR.UnableToDetermineHiveBitness), null);
        }

        public static int TryGetCorrectBitnessHKLMSubkey(bool is64bit, string key, out RegistryHandle regHandle)
        {
            if (is64bit && IntPtr.Size == 8) // No worries we are trying to open up a 64 bit hive just return 
                return SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE, out regHandle);
            else if (is64bit && IntPtr.Size == 4) // we are running under wow get the 64 bit hive
                return SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE | SafeNativeMethods.KEY_WOW64_64KEY, out regHandle);
            else if (!is64bit && IntPtr.Size == 8) // we are running in 64 bit but need to open a 32 bit hive
                return SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE | SafeNativeMethods.KEY_WOW64_32KEY, out regHandle);
            else if (!is64bit && IntPtr.Size == 4)
                return SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE, out regHandle);

            throw Tool.CreateException(SR.GetString(SR.UnableToDetermineHiveBitness), null);
        }

        static RegistryHandle GetHKLMSubkey(string key, bool isWriteRequired)
        {
            RegistryHandle regHandle = null;
            int samDesired = SafeNativeMethods.KEY_READ;
            if (isWriteRequired)
            {
                samDesired = samDesired | SafeNativeMethods.KEY_WRITE;
            }
            int status = SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, samDesired, out regHandle);
            if (status != SafeNativeMethods.ERROR_SUCCESS || null == regHandle || regHandle.IsInvalid)
            {
                throw Tool.CreateException(SR.GetString(SR.FailedToOpenRegistryKey, ""), null);
            }
            return regHandle;

        }

        static RegistryHandle Get64bitHKLMSubkey(string key, bool isWriteRequired)
        {
            RegistryHandle regHandle = null;
            int samDesired = SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WOW64_64KEY;
            if (isWriteRequired)
            {
                samDesired = samDesired | SafeNativeMethods.KEY_WRITE;
            }
            int status = SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, samDesired, out regHandle);
            if (status != SafeNativeMethods.ERROR_SUCCESS || null == regHandle || regHandle.IsInvalid)
            {
                throw Tool.CreateException(SR.GetString(SR.FailedToOpenRegistryKey, ""), null);
            }
            return regHandle;
        }

        static RegistryHandle Get32bitHKLMSubkey(string key, bool isWriteRequired)
        {
            RegistryHandle regHandle = null;
            int samDesired = SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WOW64_32KEY;
            if (isWriteRequired)
            {
                samDesired = samDesired | SafeNativeMethods.KEY_WRITE;
            }
            int status = SafeNativeMethods.RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, samDesired, out regHandle);
            if (status != SafeNativeMethods.ERROR_SUCCESS || null == regHandle || regHandle.IsInvalid)
            {
                throw Tool.CreateException(SR.GetString(SR.FailedToOpenRegistryKey, ""), null);
            }
            return regHandle;
        }


        public RegistryHandle(IntPtr hKey, bool ownHandle)
            : base(ownHandle)
        {
            handle = hKey;
        }

        public RegistryHandle()
            : base(true)
        {



        }

        public bool DeleteKey(string key)
        {
            int status = SafeNativeMethods.RegDeleteKey(this, key);
            if (status == SafeNativeMethods.ERROR_SUCCESS)
                return true;
            else
                return false;
        }

        public RegistryHandle CreateSubKey(string subKey)
        {
            RegistryHandle regHandle = null;
            int disposition;
            int status = SafeNativeMethods.RegCreateKeyEx(this, subKey, 0, null, 0, SafeNativeMethods.KEY_READ | SafeNativeMethods.KEY_WRITE, IntPtr.Zero, out regHandle, out disposition);
            if (status != SafeNativeMethods.ERROR_SUCCESS || regHandle == null || regHandle.IsInvalid)
                Tool.CreateException(SR.GetString(SR.FailedToCreateSubKey, subKey), null);
            return regHandle;
        }

        public void SetValue(string valName, string value)
        {
            int status = SafeNativeMethods.RegSetValueEx(this, valName, 0, SafeNativeMethods.REG_SZ, value, (value.Length * 2) + 2);
            if (status != SafeNativeMethods.ERROR_SUCCESS)
                Tool.CreateException(SR.GetString(SR.FailedToSetValue, valName), null);
        }

        public string GetStringValue(string valName)
        {
            int type = 0;
            int datasize = 0;
            int ret = SafeNativeMethods.RegQueryValueEx(this, valName, null, ref type, (byte[])null, ref datasize);
            if (ret == SafeNativeMethods.ERROR_SUCCESS)
                if (type == SafeNativeMethods.REG_SZ)
                {
                    byte[] blob = new byte[datasize];
                    ret = SafeNativeMethods.RegQueryValueEx(this, valName, null, ref type, (byte[])blob, ref datasize);
                    UnicodeEncoding unicode = new UnicodeEncoding();
                    return unicode.GetString(blob);
                }
            return null;
        }

        protected override bool ReleaseHandle()
        {
            if (SafeNativeMethods.RegCloseKey(handle) == SafeNativeMethods.ERROR_SUCCESS)
                return true;
            else
                return false;
        }
    }

    internal static class SafeNativeMethods
    {


        internal const int ERROR_MORE_DATA = 0xEA;
        internal const int ERROR_SUCCESS = 0;
        internal const int READ_CONTROL = 0x00020000;
        internal const int SYNCHRONIZE = 0x00100000;
        internal const int STANDARD_RIGHTS_READ = READ_CONTROL;
        internal const int STANDARD_RIGHTS_WRITE = READ_CONTROL;
        internal const int KEY_QUERY_VALUE = 0x0001;
        internal const int KEY_SET_VALUE = 0x0002;
        internal const int KEY_CREATE_SUB_KEY = 0x0004;
        internal const int KEY_ENUMERATE_SUB_KEYS = 0x0008;
        internal const int KEY_NOTIFY = 0x0010;
        internal const int KEY_CREATE_LINK = 0x0020;
        internal const int KEY_READ = ((STANDARD_RIGHTS_READ |
                                                           KEY_QUERY_VALUE |
                                                           KEY_ENUMERATE_SUB_KEYS |
                                                           KEY_NOTIFY)
                                                          &
                                                          (~SYNCHRONIZE));

        internal const int KEY_WRITE = ((STANDARD_RIGHTS_WRITE |
                                                           KEY_SET_VALUE |
                                                           KEY_CREATE_SUB_KEY)
                                                          &
                                                          (~SYNCHRONIZE));
        internal const int REG_NONE = 0;     // No value type
        internal const int REG_SZ = 1;     // Unicode nul terminated string
        internal const int REG_EXPAND_SZ = 2;     // Unicode nul terminated string
        internal const int KEY_WOW64_32KEY = (0x0200);
        internal const int KEY_WOW64_64KEY = (0x0100);


        // (with environment variable references)
        internal const int REG_BINARY = 3;     // Free form binary
        internal const int REG_DWORD = 4;     // 32-bit number
        internal const int REG_DWORD_LITTLE_ENDIAN = 4;     // 32-bit number (same as REG_DWORD)
        internal const int REG_DWORD_BIG_ENDIAN = 5;     // 32-bit number
        internal const int REG_LINK = 6;     // Symbolic Link (unicode)
        internal const int REG_MULTI_SZ = 7;     // Multiple Unicode strings
        internal const int REG_RESOURCE_LIST = 8;     // Resource list in the resource map
        internal const int REG_FULL_RESOURCE_DESCRIPTOR = 9;   // Resource list in the hardware description
        internal const int REG_RESOURCE_REQUIREMENTS_LIST = 10;
        internal const int REG_QWORD = 11;    // 64-bit number

        internal const int HWND_BROADCAST = 0xffff;
        internal const int WM_SETTINGCHANGE = 0x001A;

        internal const String KERNEL32 = "kernel32.dll";
        internal const String USER32 = "user32.dll";
        internal const String ADVAPI32 = "advapi32.dll";
        internal const String OLE32 = "ole32.dll";
        internal const String OLEAUT32 = "oleaut32.dll";
        internal const String SHFOLDER = "shfolder.dll";
        internal const String SHIM = "mscoree.dll";
        internal const String CRYPT32 = "crypt32.dll";
        internal const String SECUR32 = "secur32.dll";
        internal const String MSCORWKS = "mscorwks.dll";

        [DllImport(OLE32, ExactSpelling = true, PreserveSig = false)]
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern object DllGetClassObject(
        [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
        [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid);



        [DllImport(ADVAPI32, CharSet = CharSet.Unicode, BestFitMapping = false)]
        internal static extern int RegOpenKeyEx(RegistryHandle hKey, String lpSubKey,
                    int ulOptions, int samDesired, out RegistryHandle hkResult);

        [DllImport(ADVAPI32, CharSet = CharSet.Unicode, BestFitMapping = false)]
        internal static extern int RegSetValueEx(RegistryHandle hKey, String lpValueName,
                    int Reserved, int dwType, String val, int cbData);

        // Note: RegCreateKeyEx won't set the last error on failure - it returns
        // an error code if it fails.
        [DllImport(ADVAPI32, CharSet = CharSet.Unicode, BestFitMapping = false)]
        internal static extern int RegCreateKeyEx(RegistryHandle hKey, String lpSubKey,
                    int Reserved, String lpClass, int dwOptions,
                    int samDesigner, IntPtr lpSecurityAttributes,
                    out RegistryHandle hkResult, out int lpdwDisposition);

        [DllImport(ADVAPI32, SetLastError = false)]
        internal static extern int RegCloseKey(IntPtr handle);

        [DllImport(ADVAPI32, CharSet = CharSet.Unicode, BestFitMapping = false)]
        internal static extern int RegQueryValueEx(RegistryHandle hKey, String lpValueName,
                    int[] lpReserved, ref int lpType, [Out] byte[] lpData,
                    ref int lpcbData);

        [DllImport(ADVAPI32, CharSet = CharSet.Unicode, BestFitMapping = false)]
        internal static extern int RegDeleteKey(RegistryHandle hKey, String lpValueName);
        [DllImport(OLEAUT32, CharSet = CharSet.Unicode, BestFitMapping = false)]
        internal static extern int CreateTypeLib2(System.Runtime.InteropServices.ComTypes.SYSKIND sysKind, string szFile, out ICreateTypeLib ppctlib);

        [DllImport(SHIM, CharSet = CharSet.Unicode)]
        internal static extern int GetRequestedRuntimeVersionForCLSID(
        [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
        [Out, MarshalAs(UnmanagedType.LPWStr, SizeParamIndex = 1)] StringBuilder pVersion,
        [In] int cchBuffer,
        ref int dwLength,
        [In] int dwResolutionFlags
        );

        [DllImport(SHIM, CharSet = CharSet.Unicode, PreserveSig = true)]
        internal static extern int CLRCreateInstance(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid rclsid,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid,
            [Out, MarshalAs(UnmanagedType.Interface)] out object pMetaHost);

        internal static ICreateTypeLib CreateTypeLib(string file)
        {
            ICreateTypeLib typelib;
            int hr = CreateTypeLib2(System.Runtime.InteropServices.ComTypes.SYSKIND.SYS_WIN32, file, out typelib);
            if (hr == 0)
                return typelib;
            else
                Marshal.ThrowExceptionForHR(hr);
            return null;
        }
    }
    [ComImport]
    [Guid("D5F569D0-593B-101A-B569-08002B2DBF7A")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IPSFactoryBuffer
    {
        void CreateProxy(IntPtr inner, [In, MarshalAs(UnmanagedType.LPStruct)] Guid riid, out object proxy, out object ppv);
        void CreateStub();
    }

    [ComImport]
    [Guid("00020406-0000-0000-C000-000000000046")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ICreateTypeLib
    {
        [return: MarshalAs(UnmanagedType.Interface)]
        ICreateTypeInfo CreateTypeInfo([In, MarshalAs(UnmanagedType.LPStr)] String szName,
                              System.Runtime.InteropServices.ComTypes.TYPEKIND tkind);
        void SetName([In, MarshalAs(UnmanagedType.LPStr)] string szName);
        void SetVersion(short wMajorVerNum, short wMinorVerNum);
        void SetGuid([In, MarshalAs(UnmanagedType.LPStruct)] Guid guid);
        void SetDocString([In, MarshalAs(UnmanagedType.LPStr)] String szDoc);
        void SetHelpFileName([In, MarshalAs(UnmanagedType.LPStr)] String szHelpFileName);
        void SetHelpContext(int dwHelpContext);
        void SetLcid(int lcid);
        void SetLibFlags(int uLibFlags);
        void SaveAllChanges();
    }

    [ComImport]
    [Guid("00020405-0000-0000-C000-000000000046")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ICreateTypeInfo
    {
        void SetGuid([In, MarshalAs(UnmanagedType.LPStruct)] Guid guid);
        void SetTypeFlags(int uTypeFlags);
        void SetDocString(string doc);
        void SetHelpContext(int dwHelpContext);
        void SetVersion(short wMajorVerNum, short wMinorVerNum);
        void AddRefTypeInfo(ITypeInfo pTInfo, IntPtr phRefType);
        void AddFuncDesc(uint index, ref System.Runtime.InteropServices.ComTypes.FUNCDESC pFuncDesc);
        void AddImplType(uint index, IntPtr hRefType);
        void SetImplTypeFlags(uint index, int implTypeFlags);
        void SetAlignment(int cbAlignment);
        void SetSchema(string pStrSchema);
        void AddVarDesc(uint index, ref System.Runtime.InteropServices.ComTypes.VARDESC pVarDesc);
        void SetFuncAndParamNames();
        void SetVarName();
        void SetTypeDescAlias();
        void DefineFuncAsDllEntry();
        void SetFuncDocString();
        void SetVarDocString(ushort index, string szDocString);
        void SetFuncHelpContext(ushort index, uint dwHelpContext);
        void SetVarHelpContext(ushort index, uint dwHelpContext);
        void SetMops(ushort index, string bstrMops);
        void SetTypeIdldesc(ref System.Runtime.InteropServices.ComTypes.IDLDESC pIdlDesc);
        void LayOut();
    }

    [ComImport]
    [Guid("0002040E-0000-0000-C000-000000000046")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface ICreateTypeInfo2
    {
        void SetGuid([In, MarshalAs(UnmanagedType.LPStruct)] Guid guid);
        void SetTypeFlags(int uTypeFlags);
        void SetDocString(string doc);
        void SetHelpContext(int dwHelpContext);
        void SetVersion(short wMajorVerNum, short wMinorVerNum);
        void AddRefTypeInfo(ITypeInfo pTInfo, IntPtr phRefType);
        void AddFuncDesc(uint index, ref System.Runtime.InteropServices.ComTypes.FUNCDESC pFuncDesc);
        void AddImplType(uint index, IntPtr hRefType);
        void SetImplTypeFlags(uint index, int implTypeFlags);
        void SetAlignment(int cbAlignment);
        void SetSchema(string pStrSchema);
        void AddVarDesc(uint index, ref System.Runtime.InteropServices.ComTypes.VARDESC pVarDesc);
        void SetFuncAndParamNames();
        void SetVarName();
        void SetTypeDescAlias();
        void DefineFuncAsDllEntry();
        void SetFuncDocString();
        void SetVarDocString(ushort index, string szDocString);
        void SetFuncHelpContext(ushort index, uint dwHelpContext);
        void SetVarHelpContext(ushort index, uint dwHelpContext);
        void SetMops(ushort index, string bstrMops);
        void SetTypeIdldesc(ref System.Runtime.InteropServices.ComTypes.IDLDESC pIdlDesc);
        void LayOut();
        void DeleteFuncDesc(ushort index);
        void DeleteFuncDescByMemId(int memid, System.Runtime.InteropServices.ComTypes.INVOKEKIND invKind);
        void DeleteVarDesc(ushort index);
        void DeleteVarDescByMemId(int memid);
        void DeleteImplType();
        void SetCustData();
        void SetFuncCustData();
        void SetParamCustData();
        void SetVarCustData();
        void SetImplTypeCustData();
        void SetHelpStringContext();
        void SetFuncHelpStringContext();
        void SetVarHelpStringContext();
        void Invalidate();
        void SetName(string szName);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("00000100-0000-0000-C000-000000000046")]
    interface IEnumUnknown
    {
        // Legitimate reflection of native API name.
        [SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords")]
        [PreserveSig]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        int Next(
            [In, MarshalAs(UnmanagedType.U4)] int elementArrayLength,
            [Out, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.IUnknown, SizeParamIndex = 0)] object[] elementArray,
            [MarshalAs(UnmanagedType.U4)] out int fetchedElementCount);

        [PreserveSig]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        int Skip(
            [In, MarshalAs(UnmanagedType.U4)] int count);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Reset();

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Clone(
            [MarshalAs(UnmanagedType.Interface)] out IEnumUnknown enumerator);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("D332DB9E-B9B3-4125-8207-A14884F53216")]
    internal interface IClrMetaHost
    {
        [return: MarshalAs(UnmanagedType.Interface)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        object GetRuntime(
            [In, MarshalAs(UnmanagedType.LPWStr)] string version,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid interfaceId);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetVersionFromFile(
            [In, MarshalAs(UnmanagedType.LPWStr)] string filePath,
            [Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder buffer,
            [In, Out, MarshalAs(UnmanagedType.U4)] ref int bufferLength);

        [return: MarshalAs(UnmanagedType.Interface)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        IEnumUnknown EnumerateInstalledRuntimes();

        [return: MarshalAs(UnmanagedType.Interface)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        IEnumUnknown EnumerateLoadedRuntimes(
            [In] IntPtr processHandle);

        // Placeholder for RequestRuntimeLoadedNotification
        [PreserveSig]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        int Reserved01(
            [In] IntPtr reserved1);

        [return: MarshalAs(UnmanagedType.Interface)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        object QueryLegacyV2RuntimeBinding(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid interfaceId);
    }


    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("BD39D1D2-BA2F-486A-89B0-B4B0CB466891")]
    internal interface IClrRuntimeInfo
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetVersionString(
            [Out, MarshalAs(UnmanagedType.LPWStr, SizeParamIndex = 1)] StringBuilder buffer,
            [In, Out, MarshalAs(UnmanagedType.U4)] ref int bufferLength);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetRuntimeDirectory(
            [Out, MarshalAs(UnmanagedType.LPWStr, SizeParamIndex = 1)] StringBuilder buffer,
            [In, Out, MarshalAs(UnmanagedType.U4)] ref int bufferLength);

        [return: MarshalAs(UnmanagedType.Bool)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        bool IsLoaded(
            [In] IntPtr processHandle);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime), LCIDConversion(3)]
        void LoadErrorString(
            [In, MarshalAs(UnmanagedType.U4)] int resourceId,
            [Out, MarshalAs(UnmanagedType.LPWStr, SizeParamIndex = 2)] StringBuilder buffer,
            [In, Out, MarshalAs(UnmanagedType.U4)] ref int bufferLength);

        //@
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        IntPtr LoadLibrary(
            [In, MarshalAs(UnmanagedType.LPWStr)] string dllName);

        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        IntPtr GetProcAddress(
            [In, MarshalAs(UnmanagedType.LPStr)] string procName);

        [return: MarshalAs(UnmanagedType.Interface)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        object GetInterface(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid coClassId,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid interfaceId);
    }
}
