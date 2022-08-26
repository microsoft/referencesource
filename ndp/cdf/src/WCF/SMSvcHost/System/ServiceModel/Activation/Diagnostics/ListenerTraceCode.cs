//----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------------------
namespace System.ServiceModel.Activation.Diagnostics
{
    using System;
    using System.Diagnostics;
    using System.ServiceModel.Diagnostics;
    using System.Runtime.Diagnostics;    
    using System.Collections.Generic;    

    // Whenever a trace code is added to this class, it must also be added to the dictionary "traceCodes" in ListenerTraceUtility.cs
    static class ListenerTraceCode
    {
        public const int Activation = 0X90000;        
        public const int MessageQueueClosed = ListenerTraceCode.Activation | 0X0008;
        public const int MessageQueueDuplicatedPipe = ListenerTraceCode.Activation | 0X0009;
        public const int MessageQueueDuplicatedPipeError = ListenerTraceCode.Activation | 0X000A;
        public const int MessageQueueDuplicatedSocket = ListenerTraceCode.Activation | 0X000B;
        public const int MessageQueueDuplicatedSocketError = ListenerTraceCode.Activation | 0X000C;
        public const int MessageQueueUnregisterSucceeded = ListenerTraceCode.Activation | 0X000D;
        public const int MessageQueueRegisterCalled = ListenerTraceCode.Activation | 0X000E;
        public const int MessageQueueRegisterSucceeded = ListenerTraceCode.Activation | 0X000F;
        public const int MessageQueueRegisterFailed = ListenerTraceCode.Activation | 0X0020;
        public const int ServiceContinue = ListenerTraceCode.Activation | 0X0021;
        public const int ServicePause = ListenerTraceCode.Activation | 0X0022;
        public const int ServiceShutdown = ListenerTraceCode.Activation | 0X0023;
        public const int ServiceShutdownError = ListenerTraceCode.Activation | 0X0024;
        public const int ServiceStart = ListenerTraceCode.Activation | 0X0025;
        public const int ServiceStartPipeError = ListenerTraceCode.Activation | 0X0026;
        public const int ServiceStop = ListenerTraceCode.Activation | 0X0027;
        
        public const int ReadNetTcpConfig = ListenerTraceCode.PortSharing | 0X0006;
        public const int ReadNetPipeConfig = ListenerTraceCode.PortSharing | 0X0007;
        public const int RoutingTableCannotListen = ListenerTraceCode.PortSharing | 0X0008;
        public const int RoutingTableLookup = ListenerTraceCode.PortSharing | 0X0009;
        public const int RoutingTableNamespaceConflict = ListenerTraceCode.PortSharing | 0X000A;
        public const int RoutingTablePathTooLong = ListenerTraceCode.PortSharing | 0X000B;
        public const int RoutingTableRegisterSuccess = ListenerTraceCode.PortSharing | 0X000C;
        public const int RoutingTableUnsupportedProtocol = ListenerTraceCode.PortSharing | 0X000D;
        public const int SharedManagerServiceEndpointNotExist = ListenerTraceCode.PortSharing | 0X000E;
        public const int TransportListenerListening = ListenerTraceCode.PortSharing | 0X000F;
        public const int TransportListenerListenRequest = ListenerTraceCode.PortSharing | 0X0010;
        public const int TransportListenerSessionsReceived = ListenerTraceCode.PortSharing | 0X0011;
        public const int TransportListenerStop = ListenerTraceCode.PortSharing | 0X0012;
        public const int WasCloseAllListenerChannelInstances = ListenerTraceCode.PortSharing | 0X0013;
        public const int WasWebHostAPIFailed = ListenerTraceCode.PortSharing | 0X0014;
        public const int WasConnected = ListenerTraceCode.PortSharing | 0X0015;

        // Shared with trace codes defined in System.ServiceModel.Diagnostics
        public const int PortSharing = TraceCode.PortSharing;
        public const int PortSharingClosed = TraceCode.PortSharingClosed;
        public const int PortSharingDuplicatedPipe = TraceCode.PortSharingDuplicatedPipe;
        public const int PortSharingDupHandleGranted = TraceCode.PortSharingDupHandleGranted;
        public const int PortSharingDuplicatedSocket = TraceCode.PortSharingDuplicatedSocket;
        public const int PortSharingListening = TraceCode.PortSharingListening;

        public const int PerformanceCountersFailedDuringUpdate = TraceCode.PerformanceCountersFailedDuringUpdate;       
    }
}
