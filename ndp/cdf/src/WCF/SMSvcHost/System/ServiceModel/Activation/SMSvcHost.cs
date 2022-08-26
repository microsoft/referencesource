//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.IO;
    using System.Runtime.Diagnostics;
    using System.ServiceProcess;
    using System.Security.Principal;
    using Microsoft.Win32;
    using System.Threading;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel;
    using System.ComponentModel;

    static class SMSvcHost
    {
        static TcpPortSharing netTcpPortSharing;
        static TcpActivation netTcpActivator;
        static NamedPipeActivation netPipeActivator;
        static MsmqActivation netMsmqActivator;
        static string listenerAdapterNativeLibrary;

        internal static bool IsTcpPortSharingPaused { get { return netTcpPortSharing.IsPaused; } }
        internal static bool IsTcpActivationPaused { get { return netTcpActivator.IsPaused; } }
        internal static bool IsNamedPipeActivationPaused { get { return netPipeActivator.IsPaused; } }

        internal static bool IsWebhostSupported
        {
            get
            {
                bool isWebhostSupported = false;
                using (RegistryKey localMachine = Registry.LocalMachine)
                {
                    using (RegistryKey versionKey = localMachine.OpenSubKey(@"Software\Microsoft\InetSTP"))
                    {
                        if (versionKey != null)
                        {
                            object majorVersion = versionKey.GetValue("MajorVersion");
                            if (majorVersion != null && majorVersion.GetType().Equals(typeof(int)))
                            {
                                if ((int)majorVersion >= 7)
                                {
                                    isWebhostSupported = File.Exists(ListenerAdapterNativeLibrary);
                                }
                            }
                        }
                    }
                }

                return isWebhostSupported;
            }
        }

        internal static string ListenerAdapterNativeLibrary
        {
            get
            {
                if (listenerAdapterNativeLibrary == null)
                {
                    listenerAdapterNativeLibrary = Environment.SystemDirectory + "\\inetsrv\\wbhstipm.dll";
                }
                return listenerAdapterNativeLibrary;
            }
        }

        static void Main(string[] args)
        {
            const string SeCreateGlobalPrivilege = "SeCreateGlobalPrivilege";
            if (!OSEnvironmentHelper.IsVistaOrGreater)
            {
                try
                {
                    Utility.KeepOnlyPrivilegeInProcess(SeCreateGlobalPrivilege);
                }
                catch (Win32Exception exception)
                {
                    ListenerTraceUtility.EventLog.LogEvent(TraceEventType.Error,
                        (ushort)EventLogCategory.SharingService,
                        (uint)EventLogEventId.ServiceStartFailed,
                        false,
                        exception.ToString());

                    throw;
                }
            }

            // Hook up with unhandled exceptions.
            AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;

            List<ServiceBase> services = new List<ServiceBase>();
            netTcpPortSharing = new TcpPortSharing();
            services.Add(netTcpPortSharing);

            // We always add the services that share the same process to the service table so
            // that we don't have to stop the existing services when installing new services.
            // NOTE: Do not add code that really depends on WAS and MSMQ to the constructors
            // of these services.
            if (OSEnvironmentHelper.IsVistaOrGreater)
            {
                MainIis7(services);
            }
            ServiceBase.Run(services.ToArray());
        }

        static void MainIis7(List<ServiceBase> services)
        {
            netTcpActivator = new TcpActivation();
            services.Add(netTcpActivator);
            netPipeActivator = new NamedPipeActivation();
            services.Add(netPipeActivator);
            netMsmqActivator = new MsmqActivation();
            services.Add(netMsmqActivator);
        }

        static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Exception exception = e.ExceptionObject as Exception;
            ListenerTraceUtility.EventLog.LogEvent(TraceEventType.Error,
                (ushort)EventLogCategory.SharingService,
                (uint)EventLogEventId.SharingUnhandledException,
                false,
                exception == null ? string.Empty : exception.ToString());
        }
    }
}
