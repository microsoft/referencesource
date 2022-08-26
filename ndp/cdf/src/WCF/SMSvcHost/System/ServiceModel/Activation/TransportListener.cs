//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Net;
    using System.Net.Sockets;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.Security;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Diagnostics;
    using System.Threading;

    class TransportListener
    {
        // Double-checked locking pattern requires volatile for read/write synchronization
        static volatile byte[] drainBuffer;
        static Hashtable namedPipeInstances = new Hashtable();
        static Hashtable tcpInstances = new Hashtable();

        int count;
        ListenerConnectionDemuxer demuxer;
        ListenerConnectionDemuxer demuxerV6;
        TransportType transportType;
        EventTraceActivity eventTraceActvity;

        TransportListener(IPEndPoint endPoint)
        {
            if (TD.TcpTransportListenerListeningStartIsEnabled())
            {
                TD.TcpTransportListenerListeningStart(this.EventTraceActivity, GetRemoteEndpointAddressPort(endPoint));
            }

            transportType = TransportType.Tcp;
            SocketSettings socketSettings = new SocketSettings();
            IConnectionListener connectionListener = null;
            if (endPoint.Address.Equals(IPAddress.Broadcast))
            {
                if (Socket.OSSupportsIPv4)
                {
                    connectionListener = new SocketConnectionListener(new IPEndPoint(IPAddress.Any, endPoint.Port), socketSettings, true);
                    demuxer = Go(connectionListener);
                }

                if (Socket.OSSupportsIPv6)
                {
                    connectionListener = new SocketConnectionListener(new IPEndPoint(IPAddress.IPv6Any, endPoint.Port), socketSettings, true);
                    demuxerV6 = Go(connectionListener);
                }
            }
            else
            {
                connectionListener = new SocketConnectionListener(endPoint, socketSettings, true);
                demuxer = Go(connectionListener);
            }

            if (TD.TcpTransportListenerListeningStopIsEnabled())
            {
                TD.TcpTransportListenerListeningStop(this.EventTraceActivity);
            }
        }

        TransportListener(BaseUriWithWildcard pipeUri)
        {
            if (TD.PipeTransportListenerListeningStartIsEnabled())
            {
                TD.PipeTransportListenerListeningStart(this.EventTraceActivity, (pipeUri.BaseAddress != null) ? pipeUri.BaseAddress.ToString() : string.Empty);
            }

            transportType = TransportType.NamedPipe;
            IConnectionListener connectionListener = new PipeConnectionListener(pipeUri.BaseAddress, pipeUri.HostNameComparisonMode,
                ListenerConstants.SharedConnectionBufferSize, null, false, int.MaxValue);
            demuxer = Go(connectionListener);

            if (TD.PipeTransportListenerListeningStopIsEnabled())
            {
                TD.PipeTransportListenerListeningStop(this.EventTraceActivity);
            }
        }

        EventTraceActivity EventTraceActivity
        {
            get
            {
                if (this.eventTraceActvity == null)
                {
                    this.eventTraceActvity = EventTraceActivity.GetFromThreadOrCreate();
                }
                return this.eventTraceActvity;
            }
        }

        void AddRef()
        {
            ++count;
        }

        int DelRef()
        {
            return --count;
        }

        internal ListenerConnectionDemuxer Go(IConnectionListener connectionListener)
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.TransportListenerListenRequest, SR.GetString(SR.TraceCodeTransportListenerListenRequest), this);
            }

            ConnectionHandleDuplicated onDupHandle = new ConnectionHandleDuplicated(OnDupHandle);
            ListenerConnectionDemuxer connectionDemuxer = null;

            if (transportType == TransportType.Tcp)
            {
                connectionDemuxer = new ListenerConnectionDemuxer(connectionListener,
                    transportType,
                    ListenerConfig.NetTcp.MaxPendingAccepts,
                    ListenerConfig.NetTcp.MaxPendingConnections,
                    ListenerConfig.NetTcp.ReceiveTimeout,
                    onDupHandle);
            }
            else if (transportType == TransportType.NamedPipe)
            {
                connectionDemuxer = new ListenerConnectionDemuxer(connectionListener,
                    transportType,
                    ListenerConfig.NetPipe.MaxPendingAccepts,
                    ListenerConfig.NetPipe.MaxPendingConnections,
                    ListenerConfig.NetPipe.ReceiveTimeout,
                    onDupHandle);
            }

            if (ExecutionContext.IsFlowSuppressed())
            {
                if (SecurityContext.IsFlowSuppressed())
                {
                    connectionDemuxer.StartDemuxing();
                }
                else
                {
                    using (SecurityContext.SuppressFlow())
                    {
                        connectionDemuxer.StartDemuxing();
                    }
                }
            }
            else
            {
                using (ExecutionContext.SuppressFlow())
                {
                    if (SecurityContext.IsFlowSuppressed())
                    {
                        connectionDemuxer.StartDemuxing();
                    }
                    else
                    {
                        using (SecurityContext.SuppressFlow())
                        {
                            connectionDemuxer.StartDemuxing();
                        }
                    }
                }
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.TransportListenerListening, SR.GetString(SR.TraceCodeTransportListenerListening), this);
            }

            return connectionDemuxer;
        }

        internal static void Listen(IPEndPoint endPoint)
        {
            lock (tcpInstances)
            {
                TransportListener t = tcpInstances[endPoint] as TransportListener;
                if (t != null)
                {
                    // We use the shared TransportListener that is created earlier. 
                    t.AddRef();
                }
                else
                {
                    t = new TransportListener(endPoint);
                    tcpInstances.Add(endPoint, t);
                    t.AddRef();
                }
            }
        }

        internal static void Listen(BaseUriWithWildcard pipeUri)
        {
            lock (namedPipeInstances)
            {
                if (namedPipeInstances.ContainsKey(pipeUri))
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new CommunicationException(SR.GetString(SR.PipeAddressAlreadyUsed)));
                }
                else
                {
                    TransportListener t = new TransportListener(pipeUri);
                    namedPipeInstances.Add(pipeUri, t);
                }
            }
        }

        static TransportType GetTransportTypeAndAddress(IConnection connection, out IPAddress address, out int port)
        {
            Socket socket = connection.GetCoreTransport() as Socket;
            address = null;
            port = -1;
            TransportType transportType = TransportType.NamedPipe;
            if (socket != null)
            {
                address = (socket.LocalEndPoint as IPEndPoint).Address;
                port = (socket.LocalEndPoint as IPEndPoint).Port;
                transportType = TransportType.Tcp;
            }
            return transportType;
        }

        internal void OnDupHandle(ListenerSessionConnection session)
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.TransportListenerSessionsReceived, SR.GetString(SR.TraceCodeTransportListenerSessionsReceived), this);
            }

            if (TD.TransportListenerSessionsReceivedIsEnabled())
            {
                TD.TransportListenerSessionsReceived(session.EventTraceActivity, session.Via.ToString());
            }

            IPAddress address;
            int port;
            TransportType transportType = GetTransportTypeAndAddress(session.Connection, out address, out port);
            Debug.Print("TransportListener.OnDupHandle() via: " + session.Via.ToString() + " transportType: " + transportType);
            MessageQueue messageQueue = RoutingTable.Lookup(session.Via, address, port);
            if (messageQueue != null)
            {
                messageQueue.EnqueueSessionAndDispatch(session);
            }
            else
            {
                TransportListener.SendFault(session.Connection, FramingEncodingString.EndpointNotFoundFault);
                MessageQueue.OnDispatchFailure(transportType);
            }
        }

        static object ThisStaticLock { get { return tcpInstances; } }

        internal static void SendFault(IConnection connection, string fault)
        {
            if (drainBuffer == null)
            {
                lock (ThisStaticLock)
                {
                    if (drainBuffer == null)
                    {
                        drainBuffer = new byte[1024];
                    }
                }
            }

            try
            {
                InitialServerConnectionReader.SendFault(connection, fault, drainBuffer,
                    ListenerConstants.SharedSendTimeout, ListenerConstants.SharedMaxDrainSize);
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                // We don't care the error when sending a fault.
                DiagnosticUtility.TraceHandledException(exception, TraceEventType.Warning);
            }
        }

        void Stop()
        {
            if (demuxer != null)
            {
                demuxer.Dispose();
            }

            if (demuxerV6 != null)
            {
                demuxerV6.Dispose();
            }
        }

        internal static void Stop(IPEndPoint endPoint)
        {
            lock (tcpInstances)
            {
                TransportListener t = tcpInstances[endPoint] as TransportListener;
                if (t != null)
                {
                    if (t.DelRef() == 0)
                    {
                        if (DiagnosticUtility.ShouldTraceInformation)
                        {
                            ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.TransportListenerStop, SR.GetString(SR.TraceCodeTransportListenerStop), t);
                        }

                        try
                        {
                            t.Stop();
                        }
                        finally
                        {
                            tcpInstances.Remove(endPoint);
                        }
                    }
                }
            }
        }

        internal static void Stop(BaseUriWithWildcard pipeUri)
        {
            lock (namedPipeInstances)
            {
                TransportListener t = namedPipeInstances[pipeUri] as TransportListener;
                if (t != null)
                {
                    if (DiagnosticUtility.ShouldTraceInformation)
                    {
                        ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.TransportListenerStop, SR.GetString(SR.TraceCodeTransportListenerStop), t);
                    }

                    try
                    {
                        t.Stop();
                    }
                    finally
                    {
                        namedPipeInstances.Remove(pipeUri);
                    }
                }
            }
        }

        static string GetRemoteEndpointAddressPort(Net.IPEndPoint iPEndPoint)
        {
            //We really don't want any exceptions out of TraceUtility.
            if (iPEndPoint != null)
            {
                try
                {
                    return iPEndPoint.Address.ToString() + ":" + iPEndPoint.Port;
                }
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }
                    //ignore and continue with all non-fatal exceptions.
                }
            }

            return string.Empty;
        }
    }
}
