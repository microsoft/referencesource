//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;

    class ListenerConnectionDemuxer
    {
        ConnectionAcceptor acceptor;
        List<InitialServerConnectionReader> connectionReaders;
        bool isDisposed;

        ListenerConnectionModeCallback onConnectionModeKnown;
        ConnectionClosedCallback onConnectionClosed;
        // this is the one provided by the caller
        ConnectionHandleDuplicated connectionHandleDuplicated;
        // this is the onw we use internally
        ViaDecodedCallback onViaDecoded;
        TransportType transportType;
        TimeSpan channelInitializationTimeout;

        public ListenerConnectionDemuxer(IConnectionListener listener, 
            TransportType transportType,
            int maxAccepts, int initialMaxPendingConnections,
            TimeSpan channelInitializationTimeout,
            ConnectionHandleDuplicated connectionHandleDuplicated)
        {
            this.transportType = transportType;
            this.connectionReaders = new List<InitialServerConnectionReader>();
            this.connectionHandleDuplicated = connectionHandleDuplicated;
            this.acceptor = new ConnectionAcceptor(listener, maxAccepts, initialMaxPendingConnections, OnConnectionAvailable);
            this.channelInitializationTimeout = channelInitializationTimeout;
            this.onConnectionClosed = new ConnectionClosedCallback(OnConnectionClosed);
            this.onViaDecoded = new ViaDecodedCallback(OnViaDecoded);
        }

        object ThisLock
        {
            get { return this; }
        }

        public void Dispose()
        {
            lock (ThisLock)
            {
                if (isDisposed)
                    return;

                isDisposed = true;
            }

            for (int i = 0; i < connectionReaders.Count; i++)
            {
                connectionReaders[i].Dispose();
            }

            connectionReaders.Clear();
            acceptor.Dispose();
        }

        ListenerConnectionModeReader SetupModeReader(IConnection connection)
        {
            if (onConnectionModeKnown == null)
            {
                onConnectionModeKnown = new ListenerConnectionModeCallback(OnConnectionModeKnown);
            }

            ListenerConnectionModeReader modeReader = new ListenerConnectionModeReader(connection, onConnectionModeKnown, onConnectionClosed);
            lock (ThisLock)
            {
                if (isDisposed)
                {
                    modeReader.Dispose();
                    return null;
                }
                else
                {
                    connectionReaders.Add(modeReader);
                    return modeReader;
                }
            }
        }

        void OnConnectionAvailable(IConnection connection, Action connectionDequeuedCallback)
        {
            if (transportType == TransportType.Tcp)
            {
                ListenerPerfCounters.IncrementConnectionsAcceptedTcp();
            }
            else
            {
                ListenerPerfCounters.IncrementConnectionsAcceptedNamedPipe();
            }
            
            ListenerConnectionModeReader modeReader = SetupModeReader(connection);

            if (modeReader != null)
            {
                // StartReading() will never throw non-fatal exceptions; 
                // it propagates all exceptions into the onConnectionModeKnown callback, 
                // which is where we need our robust handling
                modeReader.StartReading(this.channelInitializationTimeout, connectionDequeuedCallback);
            }
            else
            {
                connectionDequeuedCallback();
            }
        }

        void OnConnectionModeKnown(ListenerConnectionModeReader modeReader)
        {
            lock (ThisLock)
            {
                if (isDisposed)
                {
                    return;
                }

                connectionReaders.Remove(modeReader);
            }

            try
            {
                FramingMode framingMode = modeReader.GetConnectionMode();
                switch (framingMode)
                {
                    case FramingMode.Duplex:
                        OnDuplexConnection(modeReader);
                        break;
                    case FramingMode.Singleton:
                        OnSingletonConnection(modeReader);
                        break;
                    default:
                        {
                            Exception inner = new InvalidDataException(SR.GetString(
                                SR.FramingModeNotSupported, framingMode));
                            Exception exception = new ProtocolException(inner.Message, inner);
                            FramingEncodingString.AddFaultString(exception, FramingEncodingString.UnsupportedModeFault);
                            throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(exception);
                        }
                }
            }
            catch (ProtocolException exception)
            {
                DiagnosticUtility.TraceHandledException(exception, TraceEventType.Information);

                modeReader.Dispose();
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                DiagnosticUtility.TraceHandledException(exception, TraceEventType.Error);

                // containment -- abort the errant reader
                modeReader.Dispose();
            }
        }

        void OnViaDecoded(InitialServerConnectionReader connectionReader, ListenerSessionConnection session)
        {
            try
            {
                connectionHandleDuplicated(session);
            }
            finally
            {
                session.TriggerDequeuedCallback();
            }
            lock (ThisLock)
            {
                if (isDisposed)
                {
                    return;
                }

                connectionReaders.Remove(connectionReader);
            }
        }

        void OnConnectionClosed(InitialServerConnectionReader connectionReader)
        {
            lock (ThisLock)
            {
                if (isDisposed)
                {
                    return;
                }

                connectionReaders.Remove(connectionReader);
            }
        }

        void OnSingletonConnection(ListenerConnectionModeReader modeReader)
        {
            ListenerSingletonConnectionReader singletonReader = new ListenerSingletonConnectionReader(
                modeReader.Connection, modeReader.GetConnectionDequeuedCallback(),
                transportType, modeReader.StreamPosition,
                modeReader.BufferOffset, modeReader.BufferSize, 
                onConnectionClosed, onViaDecoded);

            lock (ThisLock)
            {
                if (isDisposed)
                {
                    singletonReader.Dispose();
                    return;
                }

                connectionReaders.Add(singletonReader);
            }
            singletonReader.StartReading(modeReader.AccruedData, modeReader.GetRemainingTimeout());
        }

        void OnDuplexConnection(ListenerConnectionModeReader modeReader)
        {
            ListenerSessionConnectionReader sessionReader = new ListenerSessionConnectionReader(
                modeReader.Connection, modeReader.GetConnectionDequeuedCallback(), 
                transportType, modeReader.StreamPosition,
                modeReader.BufferOffset, modeReader.BufferSize,
                onConnectionClosed, onViaDecoded);

            lock (ThisLock)
            {
                if (isDisposed)
                {
                    sessionReader.Dispose();
                    return;
                }

                connectionReaders.Add(sessionReader);
            }
            sessionReader.StartReading(modeReader.AccruedData, modeReader.GetRemainingTimeout());
        }

        public void StartDemuxing()
        {
            acceptor.StartAccepting();
        }
    }
}
