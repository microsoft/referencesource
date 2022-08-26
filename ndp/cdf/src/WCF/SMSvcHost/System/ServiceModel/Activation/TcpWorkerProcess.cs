//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Diagnostics;
    using System.Net.Sockets;
    using System.Runtime;
    using System.ServiceModel.Activation.Diagnostics;

    class TcpWorkerProcess : WorkerProcess
    {
        protected override DuplicateContext DuplicateConnection(ListenerSessionConnection session)
        {
            SocketInformation dupedSocket = default(SocketInformation);
            try
            {
                dupedSocket = (SocketInformation)session.Connection.DuplicateAndClose(this.ProcessId);
            }
#pragma warning suppress 56500 // covered by FxCOP
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                // this normally happens if:
                // A) we don't have rights to duplicate handles to the WorkerProcess NativeErrorCode == 10022
                // B) we fail to duplicate handle because the WorkerProcess is exiting/exited NativeErrorCode == 10024
                // - in the self hosted case: report error to the client
                // - in the web hosted case: roundrobin to the next available WorkerProcess (if this WorkerProcess is down?)
#if DEBUG
                if (exception is SocketException)
                {
                    Debug.Print("TcpWorkerProcess.DuplicateConnection() failed duplicating socket for processId: " + this.ProcessId + " errorCode:" + ((SocketException)exception).NativeErrorCode + " exception:" + exception.Message);
                }
#endif
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.MessageQueueDuplicatedSocketError, SR.GetString(SR.TraceCodeMessageQueueDuplicatedSocketError), this, exception);
                }
                if (TD.MessageQueueDuplicatedSocketErrorIsEnabled())
                {                    
                    TD.MessageQueueDuplicatedSocketError(session.EventTraceActivity);
                }


                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ServiceActivationException(SR.GetString(SR.MessageQueueDuplicatedSocketError), exception));
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueDuplicatedSocket, SR.GetString(SR.TraceCodeMessageQueueDuplicatedSocket), this);
            }
            if (TD.MessageQueueDuplicatedSocketCompleteIsEnabled())
            {
                TD.MessageQueueDuplicatedSocketComplete(session.EventTraceActivity);
            }

            return new TcpDuplicateContext(dupedSocket, session.Via, session.Data);
        }

        protected override void OnDispatchSuccess()
        {
            ListenerPerfCounters.IncrementConnectionsDispatchedTcp();
        }

        protected override TransportType TransportType
        {
            get
            {
                return TransportType.Tcp;
            }
        }
    }
}
