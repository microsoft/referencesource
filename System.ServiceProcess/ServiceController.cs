//------------------------------------------------------------------------------
// <copyright file="ServiceController.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTCAST = System.Int32;
using INTPTR_INTPTRCAST = System.IntPtr;
           
namespace System.ServiceProcess 
{
    using System.Text;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Threading;
    using System.Globalization;
    using System.Security;
    using System.ServiceProcess.Design;
    using System.Security.Permissions;

    /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController"]/*' />
    /// <devdoc>
    /// This class represents an NT service. It allows you to connect to a running or stopped service
    /// and manipulate it or get information about it.
    /// </devdoc>
    [
    Designer("System.ServiceProcess.Design.ServiceControllerDesigner, " + AssemblyRef.SystemDesign),
    ServiceProcessDescription(Res.ServiceControllerDesc)
    ]
    public class ServiceController : Component 
    {
        private string machineName = ".";
        private string name = "";
        private string displayName = "";
        private string eitherName = "";
        private int commandsAccepted;
        private ServiceControllerStatus status;
        private IntPtr serviceManagerHandle;
        private bool statusGenerated;
        private bool controlGranted;
        private bool browseGranted;
        private ServiceController[] dependentServices;
        private ServiceController[] servicesDependedOn;
        private int type; 
        private bool disposed;       
        private bool startTypeInitialized;
        private ServiceStartMode startType;
        
        private const int DISPLAYNAMEBUFFERSIZE = 256;
        private static readonly int UnknownEnvironment = 0;
        private static readonly int NtEnvironment = 1;
        private static readonly int NonNtEnvironment = 2;
        private static int environment = UnknownEnvironment;

        private static Object s_InternalSyncObject;
        private static Object InternalSyncObject {
            get {
                if (s_InternalSyncObject == null) {
                    Object o = new Object();
                    Interlocked.CompareExchange(ref s_InternalSyncObject, o, null);
                }
                return s_InternalSyncObject;
            }
        }
        
        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceController"]/*' />
        /// <devdoc>
        ///     Creates a ServiceController object.
        /// </devdoc>
        public ServiceController() 
        {                                    
            this.type = NativeMethods.SERVICE_TYPE_ALL;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceController1"]/*' />
        /// <devdoc>
        ///     Creates a ServiceController object, based on
        ///     service name.
        /// </devdoc>
        public ServiceController(string name) : this(name, ".") 
        {
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceController2"]/*' />
        /// <devdoc>
        ///     Creates a ServiceController object, based on
        ///     machine and service name.
        /// </devdoc>
        public ServiceController(string name, string machineName) 
        {                
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.BadMachineName, machineName));

            if (name == null || name.Length == 0)
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "name", name));                

            this.machineName = machineName;
            this.eitherName = name;
            this.type = NativeMethods.SERVICE_TYPE_ALL;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceController3"]/*' />
        /// <devdoc>
        /// Used by the GetServices and GetDevices methods. Avoids duplicating work by the static
        /// methods and our own GenerateInfo().
        /// </devdoc>
        /// <internalonly/>
        internal ServiceController(string machineName, NativeMethods.ENUM_SERVICE_STATUS status) 
        {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.BadMachineName, machineName));

            this.machineName = machineName;
            this.name = status.serviceName;
            this.displayName = status.displayName;
            this.commandsAccepted = status.controlsAccepted;
            this.status = (ServiceControllerStatus)status.currentState;
            this.type = status.serviceType;
            this.statusGenerated = true;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceController4"]/*' />
        /// <devdoc>
        /// Used by the GetServicesInGroup method.
        /// </devdoc>
        /// <internalonly/>
        internal ServiceController(string machineName, NativeMethods.ENUM_SERVICE_STATUS_PROCESS status) 
        {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.BadMachineName, machineName));

            this.machineName = machineName;
            this.name = status.serviceName;
            this.displayName = status.displayName;
            this.commandsAccepted = status.controlsAccepted;
            this.status = (ServiceControllerStatus)status.currentState;
            this.type = status.serviceType;
            this.statusGenerated = true;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.CanPauseAndContinue"]/*' />
        /// <devdoc>
        ///     Tells if the service referenced by this object can be paused.
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPCanPauseAndContinue)
        ]
        public bool CanPauseAndContinue 
        {
            get 
            {                
                GenerateStatus();
                return(this.commandsAccepted & NativeMethods.ACCEPT_PAUSE_CONTINUE) != 0;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.CanShutdown"]/*' />
        /// <devdoc>
        ///     Tells if the service is notified when system shutdown occurs.
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPCanShutdown)
            ]
        public bool CanShutdown 
        {
            get 
            {                
                GenerateStatus();
                return(this.commandsAccepted & NativeMethods.ACCEPT_SHUTDOWN) != 0;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.CanStop"]/*' />
        /// <devdoc>
        ///     Tells if the service referenced by this object can be stopped.
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPCanStop)
        ]
        public bool CanStop 
        {
            get 
            {                
                GenerateStatus();
                return(this.commandsAccepted & NativeMethods.ACCEPT_STOP) != 0;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.DisplayName"]/*' />
        /// <devdoc>
        /// The descriptive name shown for this service in the Service applet.
        /// </devdoc>
        [
            ReadOnly(true), 
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPDisplayName),        
        ]
        public string DisplayName 
        {
            get 
            {
                if (displayName.Length == 0 && (eitherName.Length > 0 || name.Length > 0))
                    GenerateNames();
                return this.displayName;
            }
            set 
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (string.Compare(value, displayName, StringComparison.OrdinalIgnoreCase) == 0) 
                {
                    // they're just changing the casing. No need to close.
                    displayName = value;
                    return;
                }

                Close();
                displayName = value;
                name = "";
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.DependentServices"]/*' />
        /// <devdoc>
        /// The set of services that depend on this service. These are the services that will be stopped if
        /// this service is stopped.
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPDependentServices)
        ]
        public ServiceController[] DependentServices 
        {
            get 
            {
                if (!browseGranted) 
                {
                    ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                            ServiceControllerPermissionAccess.Browse, machineName, ServiceName);
                    permission.Demand();                                                                                            
                    browseGranted = true;                        
                }                    
                
                if (dependentServices == null) 
                {
                    IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_ENUMERATE_DEPENDENTS);
                    try 
                    {
                        // figure out how big a buffer we need to get the info
                        int bytesNeeded = 0;
                        int numEnumerated = 0;
                        bool result = UnsafeNativeMethods.EnumDependentServices(serviceHandle, NativeMethods.SERVICE_STATE_ALL, (IntPtr)0, 0, 
                            ref bytesNeeded, ref numEnumerated);
                        if (result) 
                        {
                            dependentServices = new ServiceController[0];
                            return dependentServices;
                        }
                        if (Marshal.GetLastWin32Error() != NativeMethods.ERROR_MORE_DATA) 
                            throw CreateSafeWin32Exception();
                                
                        // allocate the buffer
                        IntPtr enumBuffer = Marshal.AllocHGlobal((IntPtr)bytesNeeded);
    
                        try 
                        {
                            // get all the info
                            result = UnsafeNativeMethods.EnumDependentServices(serviceHandle, NativeMethods.SERVICE_STATE_ALL, enumBuffer, bytesNeeded,
                                ref bytesNeeded, ref numEnumerated);
                            if (!result) 
                                throw CreateSafeWin32Exception();                    
    
                            // for each of the entries in the buffer, create a new ServiceController object.
                            dependentServices = new ServiceController[numEnumerated];
                            for (int i = 0; i < numEnumerated; i++) 
                            {
                                NativeMethods.ENUM_SERVICE_STATUS status = new NativeMethods.ENUM_SERVICE_STATUS();
                                IntPtr structPtr = (IntPtr)((long)enumBuffer + (i * Marshal.SizeOf(typeof(NativeMethods.ENUM_SERVICE_STATUS))));
                                Marshal.PtrToStructure(structPtr, status);                                
                                dependentServices[i] = new ServiceController(MachineName, status);
                            }
                        }
                        finally 
                        {
                            Marshal.FreeHGlobal(enumBuffer);
                        }
                    }
                    finally 
                    {                    
                        SafeNativeMethods.CloseServiceHandle(serviceHandle);
                    }
                }
                
                return dependentServices;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.MachineName"]/*' />
        /// <devdoc>
        /// The name of the machine on which this service resides.
        /// </devdoc>
        [
            Browsable(false), 
            ServiceProcessDescription(Res.SPMachineName), 
            DefaultValue("."), 
            SettingsBindable(true)
        ]
        public string MachineName 
        {
            get 
            {
                return this.machineName;
            }
            set 
            {
                if (!SyntaxCheck.CheckMachineName(value))
                    throw new ArgumentException(Res.GetString(Res.BadMachineName, value));

                if (string.Compare(machineName, value, StringComparison.OrdinalIgnoreCase) == 0) 
                {
                    // no need to close, because the most they're changing is the
                    // casing.
                    machineName = value;
                    return;
                }

                Close();
                machineName = value;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceName"]/*' />
        /// <devdoc>
        ///     Returns the short name of the service referenced by this
        ///     object.
        /// </devdoc>
        [
            ReadOnly(true), 
            ServiceProcessDescription(Res.SPServiceName), 
            DefaultValue(""), 
            TypeConverter(typeof(ServiceNameConverter)), 
            SettingsBindable(true)
        ]
        public string ServiceName 
        {
            get 
            {
                if (name.Length == 0 && (eitherName.Length > 0 || displayName.Length > 0))
                    GenerateNames();
                return this.name;
            }
            set 
            {
                if (value == null)
                    throw new ArgumentNullException("value");
                
                if (string.Compare(value, name, StringComparison.OrdinalIgnoreCase) == 0) 
                {
                    // they might be changing the casing, but the service we refer to
                    // is the same. No need to close.
                    name = value;
                    return;
                }

                if (!ValidServiceName(value)) 
                    throw new ArgumentException(Res.GetString(Res.ServiceName, value, ServiceBase.MaxNameLength.ToString(CultureInfo.CurrentCulture)));

                Close();
                name = value;
                displayName = "";
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServicesDependedOn"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPServicesDependedOn)
        ]
        public unsafe ServiceController[] ServicesDependedOn 
        {
            get 
            {
                if (!browseGranted) 
                {
                    ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                            ServiceControllerPermissionAccess.Browse, machineName, ServiceName);
                    permission.Demand();                                                                                            
                    browseGranted = true;                        
                }                    
            
                if (servicesDependedOn != null)
                    return servicesDependedOn;

                IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_QUERY_CONFIG);
                try 
                {

                    int bytesNeeded = 0;
                    bool success = UnsafeNativeMethods.QueryServiceConfig(serviceHandle, (IntPtr)0, 0, out bytesNeeded);
                    if (success) 
                    {
                        servicesDependedOn = new ServiceController[0];
                        return servicesDependedOn;
                    }
                    if (Marshal.GetLastWin32Error() != NativeMethods.ERROR_INSUFFICIENT_BUFFER) 
                        throw CreateSafeWin32Exception();
                        
                    // get the info
                    IntPtr bufPtr = Marshal.AllocHGlobal((IntPtr)bytesNeeded);
                    try 
                    {
                        success = UnsafeNativeMethods.QueryServiceConfig(serviceHandle, bufPtr, bytesNeeded, out bytesNeeded);
                        if (!success) 
                            throw CreateSafeWin32Exception();                

                        NativeMethods.QUERY_SERVICE_CONFIG config = new NativeMethods.QUERY_SERVICE_CONFIG();
                        Marshal.PtrToStructure(bufPtr, config);
                        char *dependencyChar = config.lpDependencies;
                        Hashtable dependencyHash = new Hashtable();
                        
                        if (dependencyChar != null) 
                        {
                            // lpDependencies points to the start of multiple null-terminated strings. The list is
                            // double-null terminated.                            
                            StringBuilder dependencyName = new StringBuilder();
                            while (*dependencyChar != '\0') 
                            {
                                dependencyName.Append(*dependencyChar);
                                dependencyChar++;
                                if (*dependencyChar == '\0') 
                                {
                                    string dependencyNameStr = dependencyName.ToString();
                                    dependencyName = new StringBuilder();
                                    dependencyChar++;
                                    if (dependencyNameStr.StartsWith("+", StringComparison.Ordinal)) 
                                    {
                                        // this entry is actually a service load group
                                        NativeMethods.ENUM_SERVICE_STATUS_PROCESS[] loadGroup = GetServicesInGroup(machineName, dependencyNameStr.Substring(1));
                                        foreach (NativeMethods.ENUM_SERVICE_STATUS_PROCESS groupMember in loadGroup) 
                                        {
                                            if (!dependencyHash.Contains(groupMember.serviceName))
                                                dependencyHash.Add(groupMember.serviceName, new ServiceController(MachineName, groupMember));    
                                        }
                                    } else 
                                    {
                                        if (!dependencyHash.Contains(dependencyNameStr))
                                            dependencyHash.Add(dependencyNameStr, new ServiceController(dependencyNameStr, MachineName));
                                    }
                                }
                            }                                
                        }

                        servicesDependedOn = new ServiceController[dependencyHash.Count];
                        dependencyHash.Values.CopyTo(servicesDependedOn, 0);

                        return servicesDependedOn;
                    }
                    finally 
                    {
                        Marshal.FreeHGlobal(bufPtr);
                    }
                }
                finally 
                {
                    SafeNativeMethods.CloseServiceHandle(serviceHandle);
                }
            }
        }

        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            Browsable(false)
        ]
        public SafeHandle ServiceHandle { 
            get {
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
                
                return new SafeServiceHandle(GetServiceHandle(NativeMethods.SERVICE_ALL_ACCESS), true);
            }
        }


        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Status"]/*' />
        /// <devdoc>
        ///     Gets the status of the service referenced by this
        ///     object, e.g., Running, Stopped, etc.
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPStatus)
        ]
        public ServiceControllerStatus Status 
        {
            get 
            {
                GenerateStatus();
                return this.status;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ServiceType"]/*' />
        /// <devdoc>
        ///     Gets the type of service that this object references.
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPServiceType)
        ]
        public ServiceType ServiceType 
        {
            get 
            {
                GenerateStatus();
                return (ServiceType) this.type;
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.StartType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            ServiceProcessDescription(Res.SPStartType)
        ]
        public unsafe ServiceStartMode StartType
        {
            get
            {
                if (!browseGranted)
                {
                    ServiceControllerPermission permission = new ServiceControllerPermission(ServiceControllerPermissionAccess.Browse, machineName, ServiceName);
                    permission.Demand();
                    browseGranted = true;
                }

                if (startTypeInitialized)
                    return startType;

                IntPtr serviceHandle = IntPtr.Zero;
                try
                {
                    serviceHandle = GetServiceHandle(NativeMethods.SERVICE_QUERY_CONFIG);

                    int bytesNeeded = 0;
                    bool success = UnsafeNativeMethods.QueryServiceConfig(serviceHandle, (IntPtr)0, 0, out bytesNeeded);

                    if (Marshal.GetLastWin32Error() != NativeMethods.ERROR_INSUFFICIENT_BUFFER)
                        throw CreateSafeWin32Exception();

                    // get the info
                    IntPtr bufPtr = IntPtr.Zero;
                    try
                    {
                        bufPtr = Marshal.AllocHGlobal((IntPtr)bytesNeeded);
                        success = UnsafeNativeMethods.QueryServiceConfig(serviceHandle, bufPtr, bytesNeeded, out bytesNeeded);
                        if (!success)
                            throw CreateSafeWin32Exception();

                        NativeMethods.QUERY_SERVICE_CONFIG config = new NativeMethods.QUERY_SERVICE_CONFIG();
                        Marshal.PtrToStructure(bufPtr, config);

                        startType = (ServiceStartMode)config.dwStartType;
                        startTypeInitialized = true;
                    }
                    finally
                    {
                        if (bufPtr != IntPtr.Zero)
                            Marshal.FreeHGlobal(bufPtr);
                    }
                }
                finally
                {
                    if (serviceHandle != IntPtr.Zero)
                        SafeNativeMethods.CloseServiceHandle(serviceHandle);
                }

                return startType;
            }
        }
        
        private static void CheckEnvironment() 
        {
            if (environment == UnknownEnvironment) 
            { 
                lock (InternalSyncObject) 
                {
                    if (environment == UnknownEnvironment) 
                    {
                        if (Environment.OSVersion.Platform == PlatformID.Win32NT)
                            environment = NtEnvironment; 
                        else                    
                            environment = NonNtEnvironment;
                    }                
                }
            }                                                  
            
            if (environment == NonNtEnvironment)
                throw new PlatformNotSupportedException(Res.GetString(Res.CantControlOnWin9x));                                            
        }
        

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Close"]/*' />
        /// <devdoc>
        ///     Disconnects this object from the service and frees any allocated
        ///     resources.
        /// </devdoc>
        public void Close() 
        {
            if (this.serviceManagerHandle != (IntPtr)0)
                SafeNativeMethods.CloseServiceHandle(this.serviceManagerHandle);

            this.serviceManagerHandle = (IntPtr)0;
            this.statusGenerated = false;
            this.startTypeInitialized = false;
            this.type = NativeMethods.SERVICE_TYPE_ALL;
            this.browseGranted = false;
            this.controlGranted = false;
        }
        
        private static Win32Exception CreateSafeWin32Exception() 
        {
            Win32Exception newException = null;
            //SECREVIEW: Need to assert SecurtiyPermission, otherwise Win32Exception
            //                         will not be able to get the error message. At this point the right
            //                         permissions have already been demanded.
            SecurityPermission securityPermission = new SecurityPermission(PermissionState.Unrestricted);
            securityPermission.Assert();                            
            try 
            {                
                newException = new Win32Exception();               
            }
            finally 
            {
                SecurityPermission.RevertAssert();
            }                       
                        
            return newException;        
        }


        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Dispose1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) 
        {
            // safe to call while finalizing or disposing
            //
            Close();
            this.disposed = true;
            base.Dispose(disposing);
        }
        
        private unsafe void GenerateStatus() 
        {
            if (!statusGenerated) 
            {
                if (!browseGranted) 
                {
                    ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                            ServiceControllerPermissionAccess.Browse,  machineName, ServiceName);
                    permission.Demand();                                                                                            
                    browseGranted = true;                        
                }                    
            
                IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_QUERY_STATUS);
                try 
                {
                    NativeMethods.SERVICE_STATUS svcStatus = new NativeMethods.SERVICE_STATUS();
                    bool success = UnsafeNativeMethods.QueryServiceStatus(serviceHandle, &svcStatus);
                    if (!success) 
                        throw CreateSafeWin32Exception();            

                    commandsAccepted = svcStatus.controlsAccepted;
                    status = (ServiceControllerStatus) svcStatus.currentState;
                    type = svcStatus.serviceType;
                    statusGenerated = true;
                }
                finally 
                {
                    SafeNativeMethods.CloseServiceHandle(serviceHandle);
                }
            }
        }

        private void GenerateNames() 
        {
            if (machineName.Length == 0)
                throw new ArgumentException(Res.GetString(Res.NoMachineName));

            GetDataBaseHandleWithConnectAccess();
            if (name.Length == 0) 
            {
                // figure out the service name based on the information we have. If we don't have ServiceName,
                // we must either have DisplayName or the constructor parameter (eitherName).
                string userGivenName = eitherName;
                if (userGivenName.Length == 0)
                    userGivenName = displayName;
                if (userGivenName.Length == 0)
                    throw new InvalidOperationException(Res.GetString(Res.NoGivenName));                                

                int bufLen = DISPLAYNAMEBUFFERSIZE;
                StringBuilder buf = new StringBuilder(bufLen);
                bool success = SafeNativeMethods.GetServiceKeyName(serviceManagerHandle, userGivenName, buf, ref bufLen);
                if (success) 
                {
                    name = buf.ToString();
                    displayName = userGivenName;
                    eitherName = "";
                }
                else 
                {
                    success = SafeNativeMethods.GetServiceDisplayName(serviceManagerHandle, userGivenName, buf, ref bufLen);
                    if (!success && bufLen >= DISPLAYNAMEBUFFERSIZE) 
                    {
                        // DISPLAYNAMEBUFFERSIZE is total number of chars in buffer, buffLen is
                        // required chars for name, minus null terminator.  If we're here,
                        // we need a bigger buffer.
                        buf = new StringBuilder(++bufLen);
                        success = SafeNativeMethods.GetServiceDisplayName(serviceManagerHandle, userGivenName, buf, ref bufLen);
                    }
                    if (success) 
                    {
                        name = userGivenName;
                        displayName = buf.ToString();
                        eitherName = "";
                    }
                    else 
                    {
                        Exception inner = CreateSafeWin32Exception();                                        
                        throw new InvalidOperationException(Res.GetString(Res.NoService, userGivenName, machineName), inner);
                    }                        
                }
            }

            if (displayName.Length == 0) 
            {
                // by this point we know we have ServiceName, so just figure DisplayName out from that.
                                
                int bufLen = DISPLAYNAMEBUFFERSIZE;
                StringBuilder buf = new StringBuilder(bufLen);
                bool success = SafeNativeMethods.GetServiceDisplayName(serviceManagerHandle, name, buf, ref bufLen);
                if (!success && bufLen >= DISPLAYNAMEBUFFERSIZE) 
                {
                    // DISPLAYNAMEBUFFERSIZE is total number of chars in buffer, buffLen is
                    // required chars for name, minus null terminator.  If we're here,
                    // we need a bigger buffer.
                    buf = new StringBuilder(++bufLen);
                    success = SafeNativeMethods.GetServiceDisplayName(serviceManagerHandle, name, buf, ref bufLen);
                }
                if (!success) 
                {
                    Exception inner = CreateSafeWin32Exception();                                        
                    throw new InvalidOperationException(Res.GetString(Res.NoDisplayName, this.name,this.machineName), inner);
                }                    
                displayName = buf.ToString();
            }
        }

        private static IntPtr GetDataBaseHandleWithAccess(string machineName, int serviceControlManaqerAccess) {
            //Need to check the environment before trying to access service database
            CheckEnvironment();
            
            IntPtr databaseHandle = IntPtr.Zero;
            if (machineName.Equals(".") || machineName.Length == 0) {
                databaseHandle = SafeNativeMethods.OpenSCManager(null, null, serviceControlManaqerAccess);
            }
            else {
                databaseHandle = SafeNativeMethods.OpenSCManager(machineName, null, serviceControlManaqerAccess);
            }

            if (databaseHandle == (IntPtr)0) {
                Exception inner = CreateSafeWin32Exception();                                        
                throw new InvalidOperationException(Res.GetString(Res.OpenSC, machineName), inner);
            }            
            
            return databaseHandle;
        }
        
        private void GetDataBaseHandleWithConnectAccess() {
            if (this.disposed) {
                throw new ObjectDisposedException(GetType().Name);
            }
            
            // get a handle to SCM with connect access and store it in serviceManagerHandle field.
            if (this.serviceManagerHandle == (IntPtr)0) {
                this.serviceManagerHandle = GetDataBaseHandleWithAccess(this.MachineName, NativeMethods.SC_MANAGER_CONNECT);
            }
        }

        private static IntPtr GetDataBaseHandleWithEnumerateAccess(string machineName) {
            return GetDataBaseHandleWithAccess(machineName, NativeMethods.SC_MANAGER_ENUMERATE_SERVICE);
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetDevices"]/*' />
        /// <devdoc>
        ///     Gets all the device-driver services on the local machine.
        /// </devdoc>
        public static ServiceController[] GetDevices() 
        {
            return GetDevices(".");
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetDevices1"]/*' />
        /// <devdoc>
        ///     Gets all the device-driver services in the machine specified.
        /// </devdoc>
        public static ServiceController[] GetDevices(string machineName) 
        {
            return GetServicesOfType(machineName, NativeMethods.SERVICE_TYPE_DRIVER);
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetServiceHandle"]/*' />
        /// <devdoc>
        /// Opens a handle for the current service. The handle must be closed with
        /// a call to NativeMethods.CloseServiceHandle().
        /// </devdoc>
        private IntPtr GetServiceHandle(int desiredAccess) 
        {            
            GetDataBaseHandleWithConnectAccess();

            IntPtr serviceHandle = UnsafeNativeMethods.OpenService(serviceManagerHandle, ServiceName, desiredAccess);
            if (serviceHandle == (IntPtr)0) 
            {
                Exception inner = CreateSafeWin32Exception();                                                                     
                throw new InvalidOperationException(Res.GetString(Res.OpenService, ServiceName, MachineName), inner);
            }

            return serviceHandle;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetServices"]/*' />
        /// <devdoc>
        ///     Gets the services (not including device-driver services) on the local machine.
        /// </devdoc>
        public static ServiceController[] GetServices() 
        {
            return GetServices(".");
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetServices1"]/*' />
        /// <devdoc>
        ///     Gets the services (not including device-driver services) on the machine specified.
        /// </devdoc>
        public static ServiceController[] GetServices(string machineName) 
        {
            return GetServicesOfType(machineName, NativeMethods.SERVICE_TYPE_WIN32);
        }

        
        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetServicesInGroup"]/*' />
        /// <devdoc>
        /// Helper function for DependentServices.
        /// </devdoc>
        private static NativeMethods.ENUM_SERVICE_STATUS_PROCESS[] GetServicesInGroup(string machineName, string group) 
        {
            IntPtr databaseHandle = (IntPtr)0;

            // Allocate memory
            //
            IntPtr memory = (IntPtr)0;
            int bytesNeeded;
            int servicesReturned;
            int resumeHandle = 0;

            NativeMethods.ENUM_SERVICE_STATUS_PROCESS[] services;

            try 
            {
                databaseHandle = GetDataBaseHandleWithEnumerateAccess(machineName);
                UnsafeNativeMethods.EnumServicesStatusEx(databaseHandle, NativeMethods.SC_ENUM_PROCESS_INFO, NativeMethods.SERVICE_TYPE_WIN32, NativeMethods.STATUS_ALL,
                                                 (IntPtr)0, 0, out bytesNeeded, out servicesReturned, ref resumeHandle, group);

                memory = Marshal.AllocHGlobal((IntPtr)bytesNeeded);

                //
                // Get the set of services
                //
                UnsafeNativeMethods.EnumServicesStatusEx(databaseHandle, NativeMethods.SC_ENUM_PROCESS_INFO, NativeMethods.SERVICE_TYPE_WIN32, NativeMethods.STATUS_ALL,
                                                 memory, bytesNeeded, out bytesNeeded, out servicesReturned, ref resumeHandle, group);
    
                int count = servicesReturned;
    
                //
                // Go through the block of memory it returned to us
                //
                services = new NativeMethods.ENUM_SERVICE_STATUS_PROCESS[count];
                for (int i = 0; i < count; i++) 
                {
                    IntPtr structPtr = (IntPtr)((long)memory + (i * Marshal.SizeOf(typeof(NativeMethods.ENUM_SERVICE_STATUS_PROCESS))));
                    NativeMethods.ENUM_SERVICE_STATUS_PROCESS status = new NativeMethods.ENUM_SERVICE_STATUS_PROCESS();
                    Marshal.PtrToStructure(structPtr, status);   
                    services[i] = status;
                }
            }
            finally 
            {
                //
                // Free the memory
                //
                Marshal.FreeHGlobal(memory);

                if (databaseHandle != (IntPtr)0) {
                    SafeNativeMethods.CloseServiceHandle(databaseHandle);
                }
            }

            return services;
        }
        

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.GetServicesOfType"]/*' />
        /// <devdoc>
        /// Helper function for GetDevices and GetServices.
        /// </devdoc>
        private static ServiceController[] GetServicesOfType(string machineName, int serviceType) 
        {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(Res.GetString(Res.BadMachineName, machineName));
                    
            ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                            ServiceControllerPermissionAccess.Browse, machineName, "*");
            permission.Demand();                                                                                            
            
            
            //Need to check the environment before trying to access service database
            CheckEnvironment();
            
            //
            // Open the services database
            //
            IntPtr databaseHandle = (IntPtr)0;

            //
            // Allocate memory
            //
            IntPtr memory = (IntPtr)0;
            int bytesNeeded;
            int servicesReturned;
            int resumeHandle = 0;

            ServiceController[] services;

            try 
            {
                databaseHandle = GetDataBaseHandleWithEnumerateAccess(machineName);
                UnsafeNativeMethods.EnumServicesStatus(databaseHandle, serviceType , NativeMethods.STATUS_ALL,
                                                 (IntPtr)0, 0, out bytesNeeded, out servicesReturned, ref resumeHandle);

                memory = Marshal.AllocHGlobal((IntPtr)bytesNeeded);

    
                //
                // Get the set of services
                //
                UnsafeNativeMethods.EnumServicesStatus(databaseHandle, serviceType, NativeMethods.STATUS_ALL,
                                                 memory, bytesNeeded, out bytesNeeded, out servicesReturned, ref resumeHandle);
    
                int count = servicesReturned;
    
                //
                // Go through the block of memory it returned to us
                //
                services = new ServiceController[count];
                for (int i = 0; i < count; i++) 
                {
                    IntPtr structPtr = (IntPtr)((long)memory + (i * Marshal.SizeOf(typeof(NativeMethods.ENUM_SERVICE_STATUS))));
                    NativeMethods.ENUM_SERVICE_STATUS status = new NativeMethods.ENUM_SERVICE_STATUS();
                    Marshal.PtrToStructure(structPtr, status);    
                    services[i] = new ServiceController(machineName, status);
                }
            }
            finally 
            {
                //
                // Free the memory
                //
                Marshal.FreeHGlobal(memory);
                
                if (databaseHandle != (IntPtr)0) {
                    SafeNativeMethods.CloseServiceHandle(databaseHandle);
                }                
            }


            return services;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Pause"]/*' />
        /// <devdoc>
        ///     Suspends a service's operation.
        /// </devdoc>
        public unsafe void Pause() 
        {
            if (!controlGranted) 
            {
                ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                        ServiceControllerPermissionAccess.Control,  machineName, ServiceName);
                permission.Demand();                                                                                            
                controlGranted = true;                        
            }                    
                    
            IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_PAUSE_CONTINUE);
            try 
            {
                NativeMethods.SERVICE_STATUS status = new NativeMethods.SERVICE_STATUS();
                bool result = UnsafeNativeMethods.ControlService(serviceHandle,  NativeMethods.CONTROL_PAUSE, &status);
                if (!result) 
                {
                    Exception inner = CreateSafeWin32Exception();                                           
                    throw new InvalidOperationException(Res.GetString(Res.PauseService, ServiceName, MachineName), inner);
                }
            }
            finally 
            {
                SafeNativeMethods.CloseServiceHandle(serviceHandle);
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Continue"]/*' />
        /// <devdoc>
        ///     Continues a service after it has been paused.
        /// </devdoc>
        public unsafe void Continue() 
        {
            if (!controlGranted) 
            {
                ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                        ServiceControllerPermissionAccess.Control,  machineName, ServiceName);
                permission.Demand();                                                                                            
                controlGranted = true;                        
            }                    
        
            IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_PAUSE_CONTINUE);
            try 
            {
                NativeMethods.SERVICE_STATUS status = new NativeMethods.SERVICE_STATUS();
                bool result = UnsafeNativeMethods.ControlService(serviceHandle,  NativeMethods.CONTROL_CONTINUE, &status);
                if (!result) 
                {
                    Exception inner = CreateSafeWin32Exception();                                           
                    throw new InvalidOperationException(Res.GetString(Res.ResumeService, ServiceName, MachineName), inner);
                }
            }
            finally 
            {
                SafeNativeMethods.CloseServiceHandle(serviceHandle);
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.ExecuteCommand"]/*' />
        /// <devdoc>
        ///     Executes a custom command on a Service
        /// </devdoc>
        public unsafe void ExecuteCommand(int command) 
        {
            if (!controlGranted) 
            {
                ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                        ServiceControllerPermissionAccess.Control,  machineName, ServiceName);
                permission.Demand();                                                                                            
                controlGranted = true;                        
            }                    
            
            IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_USER_DEFINED_CONTROL);
            try 
            {
                NativeMethods.SERVICE_STATUS status = new NativeMethods.SERVICE_STATUS();
                bool result = UnsafeNativeMethods.ControlService(serviceHandle,  command, &status);
                if (!result) 
                {
                    Exception inner = CreateSafeWin32Exception();                                           
                    throw new InvalidOperationException(Res.GetString(Res.ControlService, ServiceName, MachineName), inner);
                }
            }
            finally 
            {
                SafeNativeMethods.CloseServiceHandle(serviceHandle);
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Refresh"]/*' />
        /// <devdoc>
        /// Refreshes all property values.
        /// </devdoc>
        public void Refresh() 
        {
            statusGenerated = false;
            startTypeInitialized = false;
            dependentServices = null;
            servicesDependedOn = null;
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Start"]/*' />
        /// <devdoc>
        /// Starts the service.
        /// </devdoc>
        public void Start() 
        {
            Start(new string[0]);
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Start1"]/*' />
        /// <devdoc>
        ///     Starts a service in the machine specified.
        /// </devdoc>
        public void Start(string[] args) 
        {
            if (args == null)
                throw new ArgumentNullException("args");
            
            if (!controlGranted) 
            {
                ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                        ServiceControllerPermissionAccess.Control,  machineName, ServiceName);
                permission.Demand();                                                                                            
                controlGranted = true;                        
            }                    
            
            IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_START);

            try 
            {
                IntPtr[] argPtrs = new IntPtr[args.Length];
                int i = 0;
                try 
                {
                    for (i = 0; i < args.Length; i++) {
                        if (args[i] == null)
                            throw new ArgumentNullException(Res.GetString(Res.ArgsCantBeNull), "args");
                        
                        argPtrs[i] = Marshal.StringToHGlobalUni(args[i]);
                    }
                }
                catch
                {
                    for (int j = 0; j < i; j++)
                        Marshal.FreeHGlobal(argPtrs[i]);
                    throw;
                }

                GCHandle argPtrsHandle = new GCHandle();
                try 
                {
                    argPtrsHandle = GCHandle.Alloc(argPtrs, GCHandleType.Pinned);
                    bool result = UnsafeNativeMethods.StartService(serviceHandle, args.Length, (INTPTR_INTPTRCAST)argPtrsHandle.AddrOfPinnedObject());
                    if (!result) 
                    {
                        Exception inner = CreateSafeWin32Exception();                                        
                        throw new InvalidOperationException(Res.GetString(Res.CannotStart, ServiceName, MachineName), inner);
                    }                        
                }
                finally 
                {
                    for (i = 0; i < args.Length; i++)
                        Marshal.FreeHGlobal(argPtrs[i]);
                    if (argPtrsHandle.IsAllocated)
                        argPtrsHandle.Free();
                }
            }
            finally 
            {
                SafeNativeMethods.CloseServiceHandle(serviceHandle);
            }
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.Stop"]/*' />
        /// <devdoc>
        ///     Stops the service. If any other services depend on this one for operation,
        ///     they will be stopped first. The DependentServices property lists this set
        ///     of services.
        /// </devdoc>
        public unsafe void Stop() 
        {
            if (!controlGranted) 
            {
                ServiceControllerPermission permission = new ServiceControllerPermission(
                                                                                        ServiceControllerPermissionAccess.Control,  machineName, ServiceName);
                permission.Demand();                                                                                            
                controlGranted = true;                        
            }                                                
        
            IntPtr serviceHandle = GetServiceHandle(NativeMethods.SERVICE_STOP);

            try 
            {
                // Before stopping this service, stop all the dependent services that are running.
                // (It's OK not to cache the result of getting the DependentServices property because it caches on its own.)
                for (int i = 0; i < DependentServices.Length; i++) 
                {
                    ServiceController currentDependent = DependentServices[i];
                    currentDependent.Refresh();
                    if (currentDependent.Status != ServiceControllerStatus.Stopped) 
                    {
                        currentDependent.Stop();
                        currentDependent.WaitForStatus(ServiceControllerStatus.Stopped, new TimeSpan(0, 0, 30));
                    }
                }

                NativeMethods.SERVICE_STATUS status = new NativeMethods.SERVICE_STATUS();
                bool result = UnsafeNativeMethods.ControlService(serviceHandle,  NativeMethods.CONTROL_STOP, &status);
                if (!result) 
                {
                    Exception inner = CreateSafeWin32Exception();                                        
                    throw new InvalidOperationException(Res.GetString(Res.StopService, ServiceName, MachineName), inner);
                }
            }
            finally 
            {
                SafeNativeMethods.CloseServiceHandle(serviceHandle);
            }
        }

        
        internal static bool ValidServiceName(string serviceName) 
        {
            if (serviceName == null)
                return false;

            //not too long
            // PS 104425 -- check for empty name as well.
            if (serviceName.Length > ServiceBase.MaxNameLength || serviceName.Length == 0)
                return false;
            
            //no slashes or backslash allowed
            foreach (char c in serviceName.ToCharArray()) 
            {                
                if ((c == '\\') || (c == '/'))
                    return false;
            }
                                
            return true;              
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.WaitForStatus"]/*' />
        /// <devdoc>
        /// Waits infinitely until the service has reached the given status.
        /// </devdoc>
        public void WaitForStatus(ServiceControllerStatus desiredStatus) 
        {
            WaitForStatus(desiredStatus, TimeSpan.MaxValue);
        }

        /// <include file='doc\ServiceController.uex' path='docs/doc[@for="ServiceController.WaitForStatus1"]/*' />
        /// <devdoc>
        /// Waits until the service has reached the given status or until the specified time
        /// has expired
        /// </devdoc>
        public void WaitForStatus(ServiceControllerStatus desiredStatus, TimeSpan timeout) 
        {
            if (!Enum.IsDefined(typeof(ServiceControllerStatus), desiredStatus)) 
                    throw new InvalidEnumArgumentException("desiredStatus", (int)desiredStatus, typeof(ServiceControllerStatus));
                    
            DateTime start = DateTime.UtcNow;
            Refresh();
            while (Status != desiredStatus) 
            {
                if (DateTime.UtcNow - start > timeout) 
                    throw new System.ServiceProcess.TimeoutException(Res.GetString(Res.Timeout));
                                    
                Thread.Sleep(250);
                Refresh();
            }
        }
    }
}
