// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInProcess
**
** Purpose:  
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Runtime.Remoting;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Threading;
using System.AddIn.Contract;
using System.AddIn.Pipeline;
using System.AddIn;

using Microsoft.Win32;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{   
    [Serializable]
    public enum Platform
    {
        Host = 0,
        AnyCpu = 1,
        X86 = 2,
        X64 = 3,
        ARM = 4
    }
    
    public sealed class AddInProcess 
    {
        private bool _keepAlive = false;
        private volatile Process _process = null;
        private Guid _guid;
        private Platform _platform;
        private String _pathToAddInProcess;
        
        private readonly Object _processLock = new Object();

        // Win32Error NativeErrorCode values
        const int ERROR_FILE_NOT_FOUND = 2;
        const int ERROR_ACCESS_DENIED = 5;

        // Time to wait for the external process to start up and report for duty
        // Default to 10 seconds
        private TimeSpan _startupTimeout = new TimeSpan(0, 0, 10);
        
        public TimeSpan StartupTimeout
        {
            get
            {
                return _startupTimeout;
            }
            set
            {
                if (value.TotalSeconds < 0) throw new ArgumentOutOfRangeException("value");

                lock(_processLock)
                {
                    if(_process == null)
                    {
                        _startupTimeout = value;
                    }
                    else
                    {
                        throw new InvalidOperationException(Res.ProcessAlreadyRunning);
                    }
                }
            }
        }
        
        public Platform Platform
        {
            get
            {
                return _platform;
            }
        }

        // This is used to represent in-process situations
        private static AddInProcess s_currentProcess = new AddInProcess(true);
        
        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: RemotingHelper.InitializeClientChannel():System.Void" Ring="1" />
        // </SecurityKernel>
        [System.Security.SecurityCritical]
        [PermissionSet(SecurityAction.Demand, Name="FullTrust")]  
        public AddInProcess() : this(Platform.Host)
        {
        }
        
        // Overloaded constructor to customize the addin process platform architecture and CLR version.
        [System.Security.SecurityCritical]
        [PermissionSet(SecurityAction.Demand, Name="FullTrust")]  
        public AddInProcess(Platform platform)
        {
            _platform = platform;
            
            // Process the arguments early so we can throw an exception if they were invalid.
            String folder = RuntimeEnvironment.GetRuntimeDirectory();
            String exeName = GetProcessName(platform);
            
            _pathToAddInProcess = Path.Combine(folder, exeName);
            if(!File.Exists(_pathToAddInProcess))
            {
                throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, Res.MissingAddInProcessExecutable, _pathToAddInProcess));
            }
            
            // Eagerly call this.  Any MBRO objects created before this initialization
            // will not be usable.
            RemotingHelper.InitializeClientChannel();
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "internalOnly", Justification = "Reviewed")]
        internal AddInProcess(bool internalOnly)
        {
            // don't initialize remoting in this version.  Therefore we don't need to be Full Trust.
        }

        // We should keep processId and Guid in sync.  That is, either
        // both are null or neither are null.
        public int ProcessId
        {
            // do the same LinkDemand that Process.GetCurrentProcess().Id demands
            [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
            get
            {
                lock(_processLock)
                {
                    if (this == s_currentProcess)
                        return Process.GetCurrentProcess().Id;

                    if (_process == null)
                        return -1;
                        
                    return _process.Id;
                }
            }
        }

        internal static AddInProcess Current
        {
            get { return s_currentProcess; }
        }

        internal Guid Guid
        {
            get
            {
                if (IsCurrentProcess)
                    return Guid.Empty;

                Start();
                return _guid;
            }
        }

        // Flag
        public bool IsCurrentProcess
        {
            get { return this == s_currentProcess; }
        }

        public bool KeepAlive
        {
            get { return _keepAlive; }
            set { _keepAlive = value; }
        }

        public event EventHandler<System.ComponentModel.CancelEventArgs> ShuttingDown;

        // Message from the OOP AddInServer indicating there are no addins currently running.
        internal void SendShuttingDown(CancelEventArgs args)
        {
            if (KeepAlive)
            {
                args.Cancel = true;
                return;
            }
            ShutDownUnlessCancelled(args);
        }

        // This helper method may be called indirectly from user code via Shutdown()
        // or by a finalizer thread cleaning up the remote process.
        private void ShutDownUnlessCancelled(CancelEventArgs args)
        {
            if (ShuttingDown != null)
                ShuttingDown(this, args);

            if (args.Cancel)
                return;

            try
            {
                lock (_processLock) {
                    // Give addins a chance to clean up by running finalizers
                    // We'll get a remoting exception trying to read the response from this though.
                    AddInServer server = GetAddInServer();

                    _process = null;
                    _guid = Guid.Empty;
                    
                    // Warning - if this was called from the finalizer thread of AddInProcess,
                    // nothing after this next call will execute, even if it's in a finally block,
                    // due to the calling process being torn down.
                    server.ExitProcess();
                }
            }
            catch (RemotingException) {}
            catch (System.Runtime.Serialization.SerializationException) {}
        }

        internal AddInServer GetAddInServer()
        {
            return RemotingHelper.GetAddInServer(Guid.ToString());
        }

        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: CreateAddInProcess():Process" Ring="1" />
        // <ReferencesCritical Name="Method: GetAddInServer():AddInServer" Ring="2" />
        // </SecurityKernel>
        [System.Security.SecurityCritical]
        public bool Start()
        {
            if (this == s_currentProcess)
                throw new InvalidOperationException(Res.OperationNotValidOnCurrentProcess);

            if (_process == null)
            {
                lock (_processLock) {
                    if (_process == null)
                    {
                        _process = CreateAddInProcess();

                        // register for SendShuttingDown callbacks
                        AddInServer addInServer = GetAddInServer();
                        addInServer.Initialize(new EventWorker(this));
                    }
                }

                return true;
            }

            return false;
        }

        // returns true if it was successfully shut down by this method, false otherwise
        public bool Shutdown()
        {
            if (this == s_currentProcess)
                throw new InvalidOperationException(Res.OperationNotValidOnCurrentProcess);

            if (_process == null)
                return false;

            CancelEventArgs args = new CancelEventArgs();
            ShutDownUnlessCancelled(args);

            if (args.Cancel)
            {
                return false;
            }

            return true;
        }
        
        [System.Security.SecurityCritical]
        private static String GetProcessName(Platform platform)
        {
            String exeName;
            
            switch(platform)
            {
                case Platform.Host:
                    exeName = CurrentlyRunning32Bit() ? "AddInProcess32.exe" : "AddInProcess.exe";
                    break;
                case Platform.ARM:
                case Platform.X86:
                    exeName = "AddInProcess32.exe";
                    break;
                case Platform.X64:
                    if(!CurrentlyRunning32Bit() || CurrentlyRunningWow64()) // verify we are on a 64bit os
                    {
                        exeName = "AddInProcess.exe";
                    }
                    else
                    {
                        throw new InvalidOperationException(Res.Invalid64bitPlatformOn32bitOS);
                    }
                    break;
                case Platform.AnyCpu:
                    exeName = "AddInProcess.exe";
                    break;
                default:
                    throw new ArgumentOutOfRangeException("platform");
            }
            
            return exeName;
        }
        
        private static bool CurrentlyRunning32Bit()
        {
            return System.IntPtr.Size == 4;
        }
        
        // Finds out whether we are in a WOW64 process (i.e. a process is 32bit and the OS is 64bit).
        // Returns false if we are in a 64bit process or on a 32bit OS.
        [System.Security.SecurityCritical]
        private static bool CurrentlyRunningWow64()
        {
            bool isWow = false;
            
            try
            {
                if(!NativeMethods.IsWow64Process(System.Diagnostics.Process.GetCurrentProcess().Handle, ref isWow))
                {
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error()); // call failed
                }
            }
            catch(EntryPointNotFoundException)
            {
                return false; // Call doesn't exist in the current OS, so it means we're not in a wow process.
            }
            
            return isWow; // Return whatever IsWow64Process() returned through the out argument.
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="Process..ctor()" />
        // <SatisfiesLinkDemand Name="Process.GetCurrentProcess():System.Diagnostics.Process" />
        // <SatisfiesLinkDemand Name="Process.get_Id():System.Int32" />
        // <SatisfiesLinkDemand Name="Process.get_StartInfo():System.Diagnostics.ProcessStartInfo" />
        // <SatisfiesLinkDemand Name="ProcessStartInfo.set_FileName(System.String):System.Void" />
        // <SatisfiesLinkDemand Name="ProcessStartInfo.set_CreateNoWindow(System.Boolean):System.Void" />
        // <SatisfiesLinkDemand Name="ProcessStartInfo.set_Arguments(System.String):System.Void" />
        // <SatisfiesLinkDemand Name="ProcessStartInfo.set_UseShellExecute(System.Boolean):System.Void" />
        // <SatisfiesLinkDemand Name="EventWaitHandle..ctor(System.Boolean,System.Threading.EventResetMode,System.String)" />
        // <SatisfiesLinkDemand Name="Process.Start():System.Boolean" />
        // <ReferencesCritical Name="Method: SafeNativeMethods.GetClrInstallationDirectory():System.String" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Reviewed")]
        [System.Security.SecurityCritical]
        private Process CreateAddInProcess()
        {
            Process addInProcess = new Process();
            Guid guid = Guid.NewGuid();
            String args = String.Format(CultureInfo.InvariantCulture, "/guid:{0} /pid:{1}", guid, Process.GetCurrentProcess().Id);

            addInProcess.StartInfo.CreateNoWindow = true;
            addInProcess.StartInfo.UseShellExecute = false;
            addInProcess.StartInfo.Arguments = args;
            addInProcess.StartInfo.FileName = _pathToAddInProcess;

#if _DEBUG
            String debuggerPath = Environment.GetEnvironmentVariable("COMPLUS_AddInProcessDebugger");
            String debuggerArgs = Environment.GetEnvironmentVariable("COMPLUS_AddInProcessDebuggerArgs");
            if(!String.IsNullOrEmpty(debuggerPath))
            {
                addInProcess.StartInfo.Arguments = "";
                
                if(!String.IsNullOrEmpty(debuggerArgs))
                {
                    addInProcess.StartInfo.Arguments = debuggerArgs + " ";
                }
                addInProcess.StartInfo.Arguments += _pathToAddInProcess + " " + args;
                addInProcess.StartInfo.FileName = debuggerPath;
            }
#endif

            // wait until it's ready
            EventWaitHandle readyEvent = new EventWaitHandle(false, EventResetMode.ManualReset, "AddInProcess:" + guid);
            
            addInProcess.Start();

            bool success = readyEvent.WaitOne(_startupTimeout, false);
            readyEvent.Close();

            if (!success) {
                // Here's an effort to avoid leaving a half-baked process around if possible.
                try {
                    addInProcess.Kill();
                }
                catch (Exception) {}
                throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, Res.CouldNotCreateAddInProcess, _startupTimeout.ToString()));
            }

            _guid = guid;

            return addInProcess;
        }
    }
}
