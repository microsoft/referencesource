//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Management;
    using System.Threading;
    using System.IO;
    using System.Runtime.InteropServices;

    class RemoteHelper
    {
        // the name of the remote machine on which the commands are executed
        string machineName;

        // the class that represnets the WMI Win32_Process
        ManagementClass processClass;
        ManagementScope managementScope;

        const int autoEventTimeout = 90000;      // 90 secs
        const int delayInAutoEventTimeout = 400; // 400 milliseconds

        internal RemoteHelper(string machineName)
        {
            this.machineName = machineName;

            // establish connection
            ConnectionOptions co = new ConnectionOptions();
            co.Authentication = AuthenticationLevel.PacketPrivacy;
            co.Impersonation = ImpersonationLevel.Impersonate;

            // define the management scope
            managementScope = new ManagementScope("\\\\" + machineName + "\\root\\cimv2", co);
            // define the path used
            ManagementPath path = new ManagementPath("Win32_Process");
            ObjectGetOptions options = new ObjectGetOptions(new ManagementNamedValueCollection(), TimeSpan.FromSeconds(15), false);

            // get the object for the defined path in the defined scope
            // this object will be the object on which the InvokeMethod will be called
            processClass = new ManagementClass(managementScope, path, options);
        }

        const string MethodCreate = "Create";
        const string QueryProcessExitEvent = "SELECT * FROM Win32_ProcessStopTrace";
        static class InputParameters
        {
            public const string CommandLine = "CommandLine";
        }

        static class OutputParameters
        {
            public const string ProcessId = "ProcessID";
            public const string ReturnValue = "ReturnValue";
        }

        internal void ExecuteWsatProcess(string arguments)
        {
            ProcessStopTraceHandler handler;
            Utilities.Log("ExecuteWsatProcess(" + arguments + ")");

            try
            {
                ManagementBaseObject inParams = processClass.GetMethodParameters(MethodCreate);
                inParams[InputParameters.CommandLine] = GetDeploymentPath() + " " + arguments;

                WqlEventQuery wqlEventQuery = new WqlEventQuery(QueryProcessExitEvent);
                ManagementEventWatcher watcher = new ManagementEventWatcher(managementScope, wqlEventQuery);

                handler = new ProcessStopTraceHandler();
                watcher.EventArrived += new EventArrivedEventHandler(handler.Arrived);
                watcher.Start();

                ManagementBaseObject outParams = processClass.InvokeMethod(
                    MethodCreate,
                    inParams,
                    null);

                if (outParams.Properties[OutputParameters.ProcessId].Value == null ||
                    outParams.Properties[OutputParameters.ReturnValue].Value == null)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.REMOTE_MISSING_WSAT, SR.GetString(SR.ErrorRemoteWSATMissing));

                }

                // the process ID when executing a remote command
                uint processID = (uint)outParams.Properties[OutputParameters.ProcessId].Value;

                uint result = (uint)outParams.Properties[OutputParameters.ReturnValue].Value;

                if (result != 0)
                {
                    throw new WsatAdminException(WsatAdminErrorCode.REMOTE_MISSING_WSAT, SR.GetString(SR.ErrorRemoteWSATMissing));
                }

                handler.ProcessId = processID;

                int totalDelay = 0;
                while (!handler.IsArrived && totalDelay < autoEventTimeout)
                {
                    totalDelay += delayInAutoEventTimeout;
                    System.Threading.Thread.Sleep(delayInAutoEventTimeout);
                }
                watcher.Stop();
            }
            catch (WsatAdminException)
            {
                throw;
            }
#pragma warning suppress 56500
            catch (Exception e)
            {
                if (Utilities.IsCriticalException(e))
                {
                    throw;
                }

                throw new WsatAdminException(WsatAdminErrorCode.REMOTE_EXECUTION_ATTEMPT_ERROR,
                                                SR.GetString(SR.ErrorAttemptRemoteExecution, e.Message));
            }

            if (handler.IsArrived && handler.ReturnCode != 0)
            {
                throw new WsatAdminException((WsatAdminErrorCode)handler.ReturnCode,
                                                SR.GetString(SR.ErrorRemoteExecution, handler.ReturnCode));
            }
            else if (!handler.IsArrived)
            {
                throw new WsatAdminException(WsatAdminErrorCode.REMOTE_TIMEOUT, SR.GetString(SR.ErrorRemoteTimeout));
            }

            Utilities.Log("ExecuteWSATProcess successfully quitted.");
        }

        internal string GetDeploymentPath()
        {
            string path = null;

            try
            {
                //We first check if the 4.0 install path was available.
                RegistryConfigurationProvider reg = new RegistryConfigurationProvider(Microsoft.Win32.RegistryHive.LocalMachine,
                    WsatKeys.WcfSetupKey40,
                    machineName);

                path = reg.ReadString(WsatKeys.RuntimeInstallPath, null);

                if (string.IsNullOrEmpty(path))
                {

             //If path under 4.0 doesnt exit, check under 3.0
                 RegistryConfigurationProvider reg2 = new RegistryConfigurationProvider(Microsoft.Win32.RegistryHive.LocalMachine,
                        WsatKeys.WcfSetupKey,
                machineName);

                 path = reg2.ReadString(WsatKeys.RuntimeInstallPath, null);
                 if (string.IsNullOrEmpty(path))
                 {
                    throw new WsatAdminException(WsatAdminErrorCode.CANNOT_GET_REMOTE_INSTALL_PATH,
                                                                    SR.GetString(SR.ErrorCannotGetRemoteInstallPath));
        
                 }

                }

                path = Path.Combine(path, "WsatConfig.exe");
            }
            catch (WsatAdminException e)
            {
                throw new WsatAdminException(WsatAdminErrorCode.CANNOT_GET_REMOTE_INSTALL_PATH,
                                                                SR.GetString(SR.ErrorCannotGetRemoteInstallPath), e);
            }
            catch (ArgumentException e) // if the path in the remote registry is invalid...
            {
                throw new WsatAdminException(WsatAdminErrorCode.CANNOT_GET_REMOTE_INSTALL_PATH,
                                                                SR.GetString(SR.ErrorCannotGetRemoteInstallPath), e);
            }
            return path;
        }
    }

    class ProcessStopTraceHandler
    {
        bool isArrived = false;
        uint processID = 0;
        uint returnCode;

        Dictionary<uint, uint> processesExited = new Dictionary<uint, uint>();

        public uint ProcessId
        {
            get { return processID; }
            set
            {
                processID = value;
                if (value > 0)
                {
                    lock (processesExited)
                    {
                        uint exitCode = 0;
                        if (processesExited.TryGetValue(value, out exitCode))
                        {
                            isArrived = true;
                            returnCode = exitCode;
                        }
                    }
                }
            }
        }

        //Handles the event when it arrives
        internal void Arrived(object sender, EventArrivedEventArgs e)
        {
            uint procID = (uint)e.NewEvent.Properties["ProcessID"].Value;
            uint exitCode = (uint)e.NewEvent.Properties["ExitStatus"].Value;

            if (ProcessId == 0)
            {
                lock (processesExited)
                {
                    processesExited[procID] = exitCode;
                }
            }
            else if (procID == this.ProcessId)
            {
                this.returnCode = exitCode;
                isArrived = true;
            }
        }

        //Used to determine whether the event has arrived or not.
        internal bool IsArrived
        {
            get
            {
                return isArrived;
            }
        }

        internal uint ReturnCode
        {
            get
            {
                return returnCode;
            }
        }
    }
}
