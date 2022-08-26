//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Diagnostics;
    using System.Runtime;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;    

    class NamedPipeWorkerProcess : WorkerProcess
    {
        protected override DuplicateContext DuplicateConnection(ListenerSessionConnection session)
        {
            IntPtr dupedPipe = IntPtr.Zero;
            try
            {
                dupedPipe = (IntPtr)session.Connection.DuplicateAndClose(this.ProcessId);
            }
#pragma warning suppress 56500 // covered by FxCOP
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                // this normally happens if:
                // A) we don't have rights to duplicate handles to the WorkerProcess NativeErrorCode == 87
                // B) we fail to duplicate handle because the WorkerProcess is exiting/exited NativeErrorCode == ???
                // - in the self hosted case: report error to the client
                // - in the web hosted case: roundrobin to the next available WorkerProcess (if this WorkerProcess is down?)
#if DEBUG
                if (exception is CommunicationException)
                {
                    int errorCode = ((System.IO.PipeException)exception.InnerException).ErrorCode;
                    Debug.Print("NamedPipeWorkerProcess.DuplicateConnection() failed duplicating pipe for processId: " + this.ProcessId + " errorCode:" + errorCode + " exception:" + exception.Message);
                }
#endif
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.MessageQueueDuplicatedPipe, SR.GetString(SR.TraceCodeMessageQueueDuplicatedPipe), this, exception);
                }

                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ServiceActivationException(SR.GetString(SR.MessageQueueDuplicatedPipeError), exception));
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueDuplicatedPipe, SR.GetString(SR.TraceCodeMessageQueueDuplicatedPipe), this);
            }

            return new NamedPipeDuplicateContext(dupedPipe, session.Via, session.Data);
        }

        protected override void OnDispatchSuccess()
        {
            ListenerPerfCounters.IncrementConnectionsDispatchedNamedPipe();
        }

        protected override TransportType TransportType
        {
            get
            {
                return TransportType.NamedPipe;
            }
        }
    }
}
