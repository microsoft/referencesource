//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess 
{    
    using System;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    
    internal static class NativeMethods 
    {
        public const int MAX_COMPUTERNAME_LENGTH = 31;
        public const int WM_POWERBROADCAST = 0x0218;
        public const int NO_ERROR = 0;
        public const int BROADCAST_QUERY_DENY = 0x424D5144;
        public const int PBT_APMBATTERYLOW = 0x0009;
        public const int PBT_APMOEMEVENT = 0x000B;
        public const int PBT_APMPOWERSTATUSCHANGE = 0x000A;
        public const int PBT_APMQUERYSUSPEND = 0x0000;
        public const int PBT_APMQUERYSUSPENDFAILED = 0x0002;
        public const int PBT_APMRESUMEAUTOMATIC = 0x0012;                                                               
        public const int PBT_APMRESUMECRITICAL = 0x0006;
        public const int PBT_APMRESUMESUSPEND = 0x0007;
        public const int PBT_APMSUSPEND = 0x0004;
                                                                       
        public const int ERROR_MORE_DATA = 234;
        public const int ERROR_INSUFFICIENT_BUFFER = 122;
        public const int ERROR_EXCEPTION_IN_SERVICE = 1064;
        public const int MB_OK = 0x00000000;
        public const int MB_OKCANCEL = 0x00000001;
        public const int MB_ABORTRETRYIGNORE = 0x00000002;
        public const int MB_YESNOCANCEL = 0x00000003;
        public const int MB_YESNO = 0x00000004;
        public const int MB_RETRYCANCEL = 0x00000005;
        public const int MB_ICONHAND = 0x00000010;
        public const int MB_ICONQUESTION = 0x00000020;
        public const int MB_ICONEXCLAMATION = 0x00000030;
        public const int MB_ICONASTERISK = 0x00000040;
        public const int MB_USERICON = 0x00000080;
        public const int MB_ICONWARNING = 0x00000030;
        public const int MB_ICONERROR = 0x00000010;
        public const int MB_ICONINFORMATION = 0x00000040;
        public const int MB_DEFBUTTON1 = 0x00000000;
        public const int MB_DEFBUTTON2 = 0x00000100;
        public const int MB_DEFBUTTON3 = 0x00000200;
        public const int MB_DEFBUTTON4 = 0x00000300;
        public const int MB_APPLMODAL = 0x00000000;
        public const int MB_SYSTEMMODAL = 0x00001000;
        public const int MB_TASKMODAL = 0x00002000;
        public const int MB_HELP = 0x00004000;
        public const int MB_NOFOCUS = 0x00008000;
        public const int MB_SETFOREGROUND = 0x00010000;
        public const int MB_DEFAULT_DESKTOP_ONLY = 0x00020000;
        public const int MB_TOPMOST = 0x00040000;
        public const int MB_RIGHT = 0x00080000;
        public const int MB_RTLREADING = 0x00100000;
        public const int MB_SERVICE_NOTIFICATION = 0x00200000;
        // MB_SERVICE_NOTIFICATION = 0x00040000;
        public const int MB_SERVICE_NOTIFICATION_NT3X = 0x00040000;
        public const int MB_TYPEMASK = 0x0000000F;
        public const int MB_ICONMASK = 0x000000F0;
        public const int MB_DEFMASK = 0x00000F00;
        public const int MB_MODEMASK = 0x00003000;
        public const int MB_MISCMASK = 0x0000C000;
                                                
        public const int STANDARD_RIGHTS_DELETE = (0x00010000);
        public const int STANDARD_RIGHTS_REQUIRED = 0x000F0000;
        public const int SERVICE_NO_CHANGE = unchecked((int)0xffffffff);

        public const int ACCESS_TYPE_CHANGE_CONFIG = 0x0002;
        public const int ACCESS_TYPE_ENUMERATE_DEPENDENTS = 0x0008;
        public const int ACCESS_TYPE_INTERROGATE = 0x0080;
        public const int ACCESS_TYPE_PAUSE_CONTINUE = 0x0040;
        public const int ACCESS_TYPE_QUERY_CONFIG = 0x0001;
        public const int ACCESS_TYPE_QUERY_STATUS = 0x0004;
        public const int ACCESS_TYPE_START = 0x0010;
        public const int ACCESS_TYPE_STOP = 0x0020;
        public const int ACCESS_TYPE_USER_DEFINED_CONTROL = 0x0100;
        public const int ACCESS_TYPE_ALL = STANDARD_RIGHTS_REQUIRED |
                                                  ACCESS_TYPE_QUERY_CONFIG |
                                                  ACCESS_TYPE_CHANGE_CONFIG |
                                                  ACCESS_TYPE_QUERY_STATUS |
                                                  ACCESS_TYPE_ENUMERATE_DEPENDENTS |
                                                  ACCESS_TYPE_START |
                                                  ACCESS_TYPE_STOP |
                                                  ACCESS_TYPE_PAUSE_CONTINUE |
                                                  ACCESS_TYPE_INTERROGATE |
                                                  ACCESS_TYPE_USER_DEFINED_CONTROL;
        
        public const int ACCEPT_NETBINDCHANGE = 0x00000010;
        public const int ACCEPT_PAUSE_CONTINUE = 0x00000002;
        public const int ACCEPT_PARAMCHANGE = 0x00000008;
        public const int ACCEPT_POWEREVENT = 0x00000040;
        public const int ACCEPT_SHUTDOWN = 0x00000004;
        public const int ACCEPT_STOP = 0x00000001;
        public const int ACCEPT_SESSIONCHANGE = 0x00000080;
        
        public const int CONTROL_CONTINUE = 0x00000003;
        public const int CONTROL_INTERROGATE = 0x00000004;
        public const int CONTROL_NETBINDADD = 0x00000007;
        public const int CONTROL_NETBINDDISABLE = 0x0000000A;
        public const int CONTROL_NETBINDENABLE = 0x00000009;
        public const int CONTROL_NETBINDREMOVE = 0x00000008;
        public const int CONTROL_PARAMCHANGE = 0x00000006;
        public const int CONTROL_PAUSE = 0x00000002;
        public const int CONTROL_POWEREVENT = 0x0000000D;
        public const int CONTROL_SHUTDOWN = 0x00000005;
        public const int CONTROL_STOP = 0x00000001;
        public const int CONTROL_DEVICEEVENT = 0x0000000B;
        public const int CONTROL_SESSIONCHANGE = 0x0000000E;

        public const int SERVICE_CONFIG_DESCRIPTION = 0x00000001;
        public const int SERVICE_CONFIG_FAILURE_ACTIONS = 0x00000002;
        public const int SERVICE_CONFIG_DELAYED_AUTO_START_INFO = 0x00000003;

        [System.Runtime.InteropServices.ComVisible(false)]
        public enum StructFormat 
        {
            Ansi = 1,
            Unicode = 2,
            Auto = 3,
        }
        
// disable csharp compiler warning #0414: field assigned unused value
#pragma warning disable 0414
        public static readonly string DATABASE_ACTIVE = "ServicesActive";
        public static readonly string DATABASE_FAILED = "ServicesFailed";
#pragma warning restore 0414

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class ENUM_SERVICE_STATUS
        {
            public string serviceName = null;
            public string displayName = null;
            public int serviceType = 0;
            public int currentState = 0;
            public int controlsAccepted = 0;
            public int win32ExitCode = 0;
            public int serviceSpecificExitCode = 0;
            public int checkPoint = 0;
            public int waitHint = 0;
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class ENUM_SERVICE_STATUS_PROCESS
        {
            public string serviceName = null;
            public string displayName = null;
            public int serviceType = 0;
            public int currentState = 0;
            public int controlsAccepted = 0;
            public int win32ExitCode = 0;
            public int serviceSpecificExitCode = 0;
            public int checkPoint = 0;
            public int waitHint = 0;
            public int processID = 0;
            public int serviceFlags = 0;
        }

        public const int ERROR_CONTROL_CRITICAL = 0x00000003;
        public const int ERROR_CONTROL_IGNORE = 0x00000000;
        public const int ERROR_CONTROL_NORMAL = 0x00000001;
        public const int ERROR_CONTROL_SEVERE = 0x00000002;                                 

        public const int SC_MANAGER_CONNECT = 0x0001;
        public const int SC_MANAGER_CREATE_SERVICE = 0x0002;
        public const int SC_MANAGER_ENUMERATE_SERVICE = 0x0004;
        public const int SC_MANAGER_LOCK = 0x0008;
        public const int SC_MANAGER_MODIFY_BOOT_CONFIG = 0x0020;
        public const int SC_MANAGER_QUERY_LOCK_STATUS = 0x0010;
        public const int SC_MANAGER_ALL   = STANDARD_RIGHTS_REQUIRED |
                                                   SC_MANAGER_CONNECT |
                                                   SC_MANAGER_CREATE_SERVICE |
                                                   SC_MANAGER_ENUMERATE_SERVICE |
                                                   SC_MANAGER_LOCK |
                                                   SC_MANAGER_QUERY_LOCK_STATUS |
                                                   SC_MANAGER_MODIFY_BOOT_CONFIG;
        public const int SC_ENUM_PROCESS_INFO = 0;

        [StructLayout(LayoutKind.Sequential)]
        public struct SERVICE_STATUS
        {
            public int serviceType;
            public int currentState;
            public int controlsAccepted;
            public int win32ExitCode;
            public int serviceSpecificExitCode;
            public int checkPoint;
            public int waitHint;

            /*
            public SERVICE_STATUS() 
            {
                this.serviceType = SERVICE_TYPE_WIN32_OWN_PROCESS;
                this.currentState = STATE_START_PENDING;
                this.controlsAccepted = 0;
                this.win32ExitCode = 0;
                this.serviceSpecificExitCode = 0;
                this.checkPoint = 0;
                this.waitHint = 0;
            }
            */
        }

        [StructLayout(LayoutKind.Sequential)]
        public unsafe class QUERY_SERVICE_CONFIG 
        {
            public int dwServiceType;
            public int dwStartType;
            public int dwErrorControl;
            public char *lpBinaryPathName;
            public char *lpLoadOrderGroup;
            public int dwTagId;
            public char *lpDependencies;
            public char *lpServiceStartName;
            public char *lpDisplayName;
        }
        
        public const int SERVICE_QUERY_CONFIG = 0x0001;
        public const int SERVICE_CHANGE_CONFIG = 0x0002;
        public const int SERVICE_QUERY_STATUS = 0x0004;
        public const int SERVICE_ENUMERATE_DEPENDENTS = 0x0008;
        public const int SERVICE_START = 0x0010;
        public const int SERVICE_STOP = 0x0020;
        public const int SERVICE_PAUSE_CONTINUE = 0x0040;
        public const int SERVICE_INTERROGATE = 0x0080;
        public const int SERVICE_USER_DEFINED_CONTROL = 0x0100;

        public const int SERVICE_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED |
                                                     SERVICE_QUERY_CONFIG |
                                                     SERVICE_CHANGE_CONFIG |
                                                     SERVICE_QUERY_STATUS |
                                                     SERVICE_ENUMERATE_DEPENDENTS |
                                                     SERVICE_START |
                                                     SERVICE_STOP |
                                                     SERVICE_PAUSE_CONTINUE |
                                                     SERVICE_INTERROGATE |
                                                     SERVICE_USER_DEFINED_CONTROL;


        public const int SERVICE_TYPE_ADAPTER = 0x00000004;
        public const int SERVICE_TYPE_FILE_SYSTEM_DRIVER = 0x00000002;
        public const int SERVICE_TYPE_INTERACTIVE_PROCESS = 0x00000100;
        public const int SERVICE_TYPE_KERNEL_DRIVER = 0x00000001;
        public const int SERVICE_TYPE_RECOGNIZER_DRIVER = 0x00000008;
        public const int SERVICE_TYPE_WIN32_OWN_PROCESS = 0x00000010;
        public const int SERVICE_TYPE_WIN32_SHARE_PROCESS = 0x00000020;
        public const int SERVICE_TYPE_WIN32 = SERVICE_TYPE_WIN32_OWN_PROCESS |
                                                     SERVICE_TYPE_WIN32_SHARE_PROCESS;
        public const int SERVICE_TYPE_DRIVER = SERVICE_TYPE_KERNEL_DRIVER |
                                                      SERVICE_TYPE_FILE_SYSTEM_DRIVER |
                                                      SERVICE_TYPE_RECOGNIZER_DRIVER;
        public const int SERVICE_TYPE_ALL = SERVICE_TYPE_WIN32  |
                                                   SERVICE_TYPE_ADAPTER |
                                                   SERVICE_TYPE_DRIVER  |
                                                   SERVICE_TYPE_INTERACTIVE_PROCESS;        
       
        [StructLayout(LayoutKind.Sequential)]
        public class SERVICE_TABLE_ENTRY
        {
            public IntPtr name;
            public Delegate callback;
        }
             
        public const int START_TYPE_AUTO = 0x00000002;
        public const int START_TYPE_BOOT = 0x00000000;
        public const int START_TYPE_DEMAND = 0x00000003;
        public const int START_TYPE_DISABLED = 0x00000004;
        public const int START_TYPE_SYSTEM = 0x00000001;
       
        public const int SERVICE_ACTIVE = 1;
        public const int SERVICE_INACTIVE = 2;
        public const int SERVICE_STATE_ALL = SERVICE_ACTIVE | SERVICE_INACTIVE;
       
        public const int STATE_CONTINUE_PENDING = 0x00000005;
        public const int STATE_PAUSED = 0x00000007;
        public const int STATE_PAUSE_PENDING = 0x00000006;
        public const int STATE_RUNNING = 0x00000004;
        public const int STATE_START_PENDING = 0x00000002;
        public const int STATE_STOPPED = 0x00000001;
        public const int STATE_STOP_PENDING = 0x00000003;
       
        public const int STATUS_ACTIVE = 0x00000001;
        public const int STATUS_INACTIVE = 0x00000002;
        public const int STATUS_ALL = STATUS_ACTIVE | STATUS_INACTIVE;

        public const int POLICY_VIEW_LOCAL_INFORMATION = 0x00000001;
        public const int POLICY_VIEW_AUDIT_INFORMATION = 0x00000002;
        public const int POLICY_GET_PRIVATE_INFORMATION = 0x00000004;
        public const int POLICY_TRUST_ADMIN = 0x00000008;
        public const int POLICY_CREATE_ACCOUNT = 0x00000010;
        public const int POLICY_CREATE_SECRET = 0x00000020;
        public const int POLICY_CREATE_PRIVILEGE = 0x00000040;
        public const int POLICY_SET_DEFAULT_QUOTA_LIMITS = 0x00000080;
        public const int POLICY_SET_AUDIT_REQUIREMENTS = 0x00000100;
        public const int POLICY_AUDIT_LOG_ADMIN = 0x00000200;
        public const int POLICY_SERVER_ADMIN = 0x00000400;
        public const int POLICY_LOOKUP_NAMES = 0x00000800;

        public const int POLICY_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED |
                                                        POLICY_VIEW_LOCAL_INFORMATION    |
                                                        POLICY_VIEW_AUDIT_INFORMATION    |
                                                        POLICY_GET_PRIVATE_INFORMATION   |
                                                        POLICY_TRUST_ADMIN |
                                                        POLICY_CREATE_ACCOUNT |
                                                        POLICY_CREATE_SECRET |
                                                        POLICY_CREATE_PRIVILEGE |
                                                        POLICY_SET_DEFAULT_QUOTA_LIMITS |
                                                        POLICY_SET_AUDIT_REQUIREMENTS |
                                                        POLICY_AUDIT_LOG_ADMIN |
                                                        POLICY_SERVER_ADMIN;
        
        public const int STATUS_OBJECT_NAME_NOT_FOUND = -1073741772; //0xC0000034;

        public const int WTS_CONSOLE_CONNECT        = 0x1;
        public const int WTS_CONSOLE_DISCONNECT     = 0x2;
        public const int WTS_REMOTE_CONNECT         = 0x3;
        public const int WTS_REMOTE_DISCONNECT      = 0x4;
        public const int WTS_SESSION_LOGON          = 0x5;
        public const int WTS_SESSION_LOGOFF         = 0x6;
        public const int WTS_SESSION_LOCK           = 0x7;
        public const int WTS_SESSION_UNLOCK         = 0x8;
        public const int WTS_SESSION_REMOTE_CONTROL = 0x9;


        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class LSA_UNICODE_STRING 
        {
            public short length;
            public short maximumLength;
            public string buffer;
        }
         
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class LSA_UNICODE_STRING_withPointer 
        {
            public short length = 0;
            public short maximumLength = 0;
            public IntPtr pwstr = (IntPtr)0;
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class LSA_OBJECT_ATTRIBUTES 
        {
            public int length = 0;
            public IntPtr rootDirectory = (IntPtr) 0;
            public IntPtr pointerLsaString = (IntPtr) 0;
            public int attributes = 0;
            public IntPtr pointerSecurityDescriptor = (IntPtr) 0;
            public IntPtr pointerSecurityQualityOfService = (IntPtr) 0;
        }

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public struct SERVICE_DESCRIPTION 
        {
            public IntPtr description;
        };


        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct SERVICE_DELAYED_AUTOSTART_INFO {
            public bool fDelayedAutostart;
        };

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public unsafe struct SERVICE_FAILURE_ACTIONS 
        {
            public uint     dwResetPeriod;
            public IntPtr   rebootMsg;
            public IntPtr   command;
            public uint     numActions;
            public SC_ACTION* actions;
        } 

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public struct SC_ACTION 
        {
            public int  type;
            public uint delay;
        }

#if NOTIMPLEMENTED
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class DEV_BROADCAST_HANDLE
        {
            uint        dbch_size;
            uint        dbch_devicetype;
            uint        dbch_reserved;
            IntPtr      dbch_handle;     // file handle used in call to RegisterDeviceNotification
            IntPtr      dbch_hdevnotify; // returned from RegisterDeviceNotification
            //
            // The following 3 fields are only valid if wParam is DBT_CUSTOMEVENT.
            //
            IntPtr      dbch_eventguid;
            LONG        dbch_nameoffset; // offset (bytes) of variable-length string buffer (-1 if none)
            BYTE        dbch_data[1];    // variable-sized buffer, potentially containing binary and/or text data
        }
#endif

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class WTSSESSION_NOTIFICATION 
        {
            public int size;
            public int sessionId;
        }

        public delegate void ServiceMainCallback(int argCount, IntPtr argPointer);
        
        public delegate void ServiceControlCallback(int control);
        
        public delegate int ServiceControlCallbackEx(int control, int eventType, IntPtr eventData, IntPtr eventContext);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static IntPtr OpenService(IntPtr databaseHandle, string serviceName, int access);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static IntPtr RegisterServiceCtrlHandler(string serviceName, Delegate callback);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static IntPtr RegisterServiceCtrlHandlerEx(string serviceName, Delegate callback, IntPtr userData);

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public unsafe extern static bool SetServiceStatus(IntPtr serviceStatusHandle, SERVICE_STATUS *status);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool StartServiceCtrlDispatcher(IntPtr entry);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static IntPtr CreateService(IntPtr databaseHandle, string serviceName, string displayName,  int access,  int serviceType,
            int startType, int errorControl, string binaryPath, string loadOrderGroup, IntPtr pTagId, string dependencies,
            string servicesStartName, string password);       
            
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool DeleteService(IntPtr serviceHandle);               
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=false)]
        public static extern int LsaOpenPolicy(NativeMethods.LSA_UNICODE_STRING systemName, IntPtr pointerObjectAttributes,
                                                            int desiredAccess,  out IntPtr pointerPolicyHandle);

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=false)]
        public static extern int LsaAddAccountRights(IntPtr policyHandle, byte[] accountSid, NativeMethods.LSA_UNICODE_STRING userRights,
                                                            int countOfRights);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=false)]
        public static extern int LsaRemoveAccountRights(IntPtr policyHandle, byte[] accountSid, bool allRights, NativeMethods.LSA_UNICODE_STRING userRights, int countOfRights);

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=false)]
        public static extern int LsaEnumerateAccountRights(IntPtr policyHandle, byte[] accountSid, out IntPtr pLsaUnicodeStringUserRights, out int RightsCount);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool LookupAccountName(string systemName, string accountName,
            byte[] sid, int[] sidLen, char[] refDomainName, int[] domNameLen,
            [In, Out]int[] sidNameUse);                           

        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool GetComputerName(StringBuilder lpBuffer, ref int nSize);

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool ChangeServiceConfig2(IntPtr serviceHandle, uint infoLevel, ref SERVICE_DESCRIPTION serviceDesc);

        [DllImport(ExternDll.Advapi32, CharSet = System.Runtime.InteropServices.CharSet.Unicode, SetLastError = true)]
        public static extern bool ChangeServiceConfig2(IntPtr serviceHandle, uint infoLevel, ref SERVICE_DELAYED_AUTOSTART_INFO serviceDesc);

       
    }
}    

