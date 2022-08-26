//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.Collections.Generic;
    using System.Security.Principal;
    using System.Configuration;
    using System.Diagnostics;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Activation.Configuration;

    static class ListenerConfig
    {
        static object syncRoot = new object();

        // Double-checked locking pattern requires volatile for read/write synchronization
        static volatile NetTcpSectionData tcpData = null;

        // Double-checked locking pattern requires volatile for read/write synchronization
        static volatile NetPipeSectionData pipeData = null;
        // Double-checked locking pattern requires volatile for read/write synchronization
        static volatile bool diagnosticsSectionInited = false;
        static bool perfCountersEnabled = false;

        static object SyncRoot
        {
            get
            {
                return syncRoot;
            }
        }

        static void EnsureInitializedForDiagnostics()
        {
            if (!diagnosticsSectionInited)
            {
                lock (SyncRoot)
                {
                    if (!diagnosticsSectionInited)
                    {
                        DiagnosticSection diag = DiagnosticSection.GetSection();
                        perfCountersEnabled = diag.PerformanceCountersEnabled;
                        diagnosticsSectionInited = true;
                    }
                }
            }
        }

        public static void EnsureInitializedForNetTcp()
        {
            EnsureInitializedForDiagnostics();

            if (tcpData == null)
            {
                lock (SyncRoot)
                {
                    if (tcpData == null)
                    {
                        tcpData = new NetTcpSectionData();
                    }
                }
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ReadNetTcpConfig,
                    SR.GetString(SR.TraceCodeReadNetTcpConfig, tcpData.ListenBacklog, tcpData.MaxPendingConnections,
                    tcpData.MaxPendingAccepts, tcpData.ReceiveTimeout, tcpData.TeredoEnabled), null);
            }
        }

        public static void EnsureInitializedForNetPipe()
        {
            EnsureInitializedForDiagnostics();

            if (pipeData == null)
            {
                lock (SyncRoot)
                {
                    if (pipeData == null)
                    {
                        pipeData = new NetPipeSectionData();
                    }
                }
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ReadNetPipeConfig,
                    SR.GetString(SR.TraceCodeReadNetPipeConfig, pipeData.MaxPendingConnections, pipeData.MaxPendingAccepts, pipeData.ReceiveTimeout), null);
            }
        }

        public static NetTcpSectionData NetTcp
        {
            get
            {
                EnsureInitializedForNetTcp();
                return tcpData;
            }
        }

        public static NetPipeSectionData NetPipe 
        {
            get
            {
                EnsureInitializedForNetPipe();
                return pipeData;
            }
        }

        public static bool PerformanceCountersEnabled
        {
            get
            {
                EnsureInitializedForDiagnostics();
                return perfCountersEnabled;
            }
        }

        public static List<SecurityIdentifier> GetAllowAccounts(TransportType transportType)
        {
            if (transportType == TransportType.Tcp)
            {
                if (NetTcp.AllowAccounts.Count == 0)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException(
                        SR.GetString(SR.ConfigSectionHasZeroAllowAccounts, "NetTcpSection")));
                }

                return NetTcp.AllowAccounts;
            }
            else if (transportType == TransportType.NamedPipe)
            {
                if (NetPipe.AllowAccounts.Count == 0)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException(
                        SR.GetString(SR.ConfigSectionHasZeroAllowAccounts, "NetPipeSection")));
                }

                return NetPipe.AllowAccounts;
            }

            return null;
        }
    }
}
