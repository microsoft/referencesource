//----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------------------
namespace System.ServiceModel.Activation.Diagnostics
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime;    
    using System.Runtime.Diagnostics;    
    using System.ServiceModel.Diagnostics;
   
    static class ListenerTraceUtility
    {
        // This dictionary must always be kept synchronized with the list of trace codes in ListenerTraceCode.cs
        // Make sure to update the size of the dictionary as well when you add a new trace code to it.
        static Dictionary<int, string> traceCodes = new Dictionary<int, string>(39)
        {
            { ListenerTraceCode.MessageQueueClosed, "MessageQueueClosed" },
            { ListenerTraceCode.MessageQueueDuplicatedPipe, "MessageQueueDuplicatedPipe" },
            { ListenerTraceCode.MessageQueueDuplicatedPipeError, "MessageQueueDuplicatedPipeError" },
            { ListenerTraceCode.MessageQueueDuplicatedSocket, "MessageQueueDuplicatedSocket" },
            { ListenerTraceCode.MessageQueueDuplicatedSocketError, "MessageQueueDuplicatedSocketError" },
            { ListenerTraceCode.MessageQueueUnregisterSucceeded, "MessageQueueUnregisterSucceeded" },
            { ListenerTraceCode.MessageQueueRegisterCalled, "MessageQueueRegisterCalled" },
            { ListenerTraceCode.MessageQueueRegisterSucceeded, "MessageQueueRegisterSucceeded" },
            { ListenerTraceCode.MessageQueueRegisterFailed, "MessageQueueRegisterFailed" },
            { ListenerTraceCode.ServiceContinue, "ServiceContinue" },
            { ListenerTraceCode.ServicePause, "ServicePause" },
            { ListenerTraceCode.ServiceShutdown, "ServiceShutdown" },
            { ListenerTraceCode.ServiceShutdownError, "ServiceShutdownError" },
            { ListenerTraceCode.ServiceStart, "ServiceStart" },
            { ListenerTraceCode.ServiceStartPipeError, "ServiceStartPipeError" },
            { ListenerTraceCode.ServiceStop, "ServiceStop" },

            // PortSharingtrace codes (TraceCode.PortSharing)
            { ListenerTraceCode.ReadNetTcpConfig, "ReadNetTcpConfig" },
            { ListenerTraceCode.ReadNetPipeConfig, "ReadNetPipeConfig" },
            { ListenerTraceCode.RoutingTableCannotListen, "RoutingTableCannotListen" },
            { ListenerTraceCode.RoutingTableLookup, "RoutingTableLookup" },
            { ListenerTraceCode.RoutingTableNamespaceConflict, "RoutingTableNamespaceConflict" },
            { ListenerTraceCode.RoutingTablePathTooLong, "RoutingTablePathTooLong" },
            { ListenerTraceCode.RoutingTableRegisterSuccess, "RoutingTableRegisterSuccess" },
            { ListenerTraceCode.RoutingTableUnsupportedProtocol, "RoutingTableUnsupportedProtocol" },
            { ListenerTraceCode.SharedManagerServiceEndpointNotExist, "SharedManagerServiceEndpointNotExist" },
            { ListenerTraceCode.TransportListenerListening, "TransportListenerListening" },
            { ListenerTraceCode.TransportListenerListenRequest, "TransportListenerListenRequest" },
            { ListenerTraceCode.TransportListenerSessionsReceived, "TransportListenerSessionsReceived" },
            { ListenerTraceCode.TransportListenerStop, "TransportListenerStop" },
            { ListenerTraceCode.WasCloseAllListenerChannelInstances, "WasCloseAllListenerChannelInstances" },
            { ListenerTraceCode.WasWebHostAPIFailed, "WasWebHostAPIFailed" },
            { ListenerTraceCode.WasConnected, "WasConnected" },

            //shared
            { ListenerTraceCode.PortSharingClosed, "PortSharingClosed" },
            { ListenerTraceCode.PortSharingDuplicatedPipe, "PortSharingDuplicatedPipe" },
            { ListenerTraceCode.PortSharingDupHandleGranted, "PortSharingDupHandleGranted" },
            { ListenerTraceCode.PortSharingDuplicatedSocket, "PortSharingDuplicatedSocket" },
            { ListenerTraceCode.PortSharingListening, "PortSharingListening" },
            
            { ListenerTraceCode.PerformanceCountersFailedDuringUpdate, "PerformanceCountersFailedDuringUpdate" },           
        };

        internal static void TraceEvent(TraceEventType severity, int traceCode, string traceDescription, object source)
        {
            TraceEvent(severity, traceCode, traceDescription, null, source, null);
        }

        internal static void TraceEvent(TraceEventType severity, int traceCode, string traceDescription, object source, Exception exception)
        {
            TraceEvent(severity, traceCode, traceDescription, null, source, exception);
        }

        internal static void TraceEvent(TraceEventType severity, int traceCode, string traceDescription, TraceRecord record, object source, Exception exception)
        {
            if (DiagnosticUtility.ShouldTrace(severity))
            {
                Fx.Assert(traceCodes.ContainsKey(traceCode), 
                    string.Format(CultureInfo.InvariantCulture, "Unsupported trace code: Please add trace code 0x{0} to the dictionary ListenerTraceUtility.traceCodes in {1}", 
                    traceCode.ToString("X", CultureInfo.InvariantCulture), typeof(ListenerTraceUtility)));
                string msdnTraceCode = LegacyDiagnosticTrace.GenerateMsdnTraceCode("SMSvcHost", traceCodes[traceCode]);
                DiagnosticUtility.DiagnosticTrace.TraceEvent(severity, traceCode, msdnTraceCode, traceDescription, record, exception, source);
            }
        }

        internal static string CreateSourceString(object source)
        {
            return source.GetType().ToString() + "/" + source.GetHashCode().ToString(CultureInfo.CurrentCulture);
        }

//        // NOTE: We need special EventLog in this class so that we can log event even if we can't initialize due to
//        // config errors.
        internal static System.Runtime.Diagnostics.EventLogger EventLog
        {
            // One doesn't hold onto the EventLogger for a long period of time.
            // Just long enough to log an event. 
#pragma warning disable 618
            get
            {
                return DiagnosticUtility.EventLog;
            }
#pragma warning restore 618
        }
    }
}
