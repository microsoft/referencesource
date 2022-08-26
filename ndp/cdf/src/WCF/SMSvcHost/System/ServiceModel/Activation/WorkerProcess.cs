//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.Security.AccessControl;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.Threading;

    [ServiceBehavior(InstanceContextMode = InstanceContextMode.PerSession)]
    abstract class WorkerProcess : IConnectionRegister
    {
        int isUnregistered;
        int processId;
        MessageQueue queue;
        int queueId;
        IConnectionDuplicator connectionDuplicator;
        EventTraceActivity eventTraceActivity;

        public bool IsRegistered
        {
            get { return isUnregistered == 0; }
        }

        public MessageQueue Queue
        {
            get
            {
                return this.queue;
            }

            set
            {
                this.queue = value;
            }
        }

        public int ProcessId
        {
            get
            {
                return this.processId;
            }
        }

#if DEBUG
        public int QueueId
        {
            get
            {
                return this.queueId;
            }
        }
#endif

        internal void Close()
        {
            if (Interlocked.Increment(ref isUnregistered) == 1)
            {
                if (this.queue != null)
                {
                    this.queue.Unregister(this);
                }
            }
        }

        protected EventTraceActivity EventTraceActivity
        {
            get
            {
                if (this.eventTraceActivity == null)
                {
                    this.eventTraceActivity = EventTraceActivity.GetFromThreadOrCreate();
                }
                return this.eventTraceActivity;
            }
        }

        protected abstract DuplicateContext DuplicateConnection(ListenerSessionConnection session);
        protected abstract void OnDispatchSuccess();
        protected abstract TransportType TransportType { get; }

        internal IAsyncResult BeginDispatchSession(ListenerSessionConnection session, AsyncCallback callback, object state)
        {
            return new DispatchSessionAsyncResult(session, callback, state);
        }

        internal bool EndDispatchSession(IAsyncResult result)
        {
            try
            {
                DispatchSessionAsyncResult dispatchAsyncResult = DispatchSessionAsyncResult.End(result);
                if (dispatchAsyncResult.DuplicateSucceeded)
                {
                    OnDispatchSuccess();
                    return true;
                }
            }
            catch (Exception exception)
            {
                EventLogEventId logEventId = EventLogEventId.MessageQueueDuplicatedSocketLeak;
                if (this.TransportType == TransportType.NamedPipe)
                {
                    logEventId = EventLogEventId.MessageQueueDuplicatedPipeLeak;
                }

                Debug.Print("WorkerProcess.DispatchSession() failed sending duplicated socket to processId: " + this.ProcessId + " exception:" + exception);
                DiagnosticUtility.EventLog.LogEvent(TraceEventType.Error,
                    (ushort)EventLogCategory.SharingService,
                    (uint)logEventId,
                    this.ProcessId.ToString(NumberFormatInfo.InvariantInfo),
                    ListenerTraceUtility.CreateSourceString(this),
                    exception.ToString());

                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                Close();

                // make sure we close the connection to the SharedConnectionListener
                // so it knows we've unregistered it
                ((IChannel)connectionDuplicator).Abort();

                if (!ShouldRecoverFromProxyCall(exception))
                {
                    throw;
                }
            }

            return false;
        }

        internal IConnectionDuplicator ConnectionDuplicator
        {
            get
            {
                return this.connectionDuplicator;
            }
        }

        void WorkerProcess_Closed(object sender, EventArgs e)
        {
            Debug.Print("WorkerProcess.WorkerProcess_Closed() worker leaving: " + processId + " State: " + ((IDuplexContextChannel)sender).State);
            Close();
        }

        void WorkerProcess_Faulted(object sender, EventArgs e)
        {
            Debug.Print("WorkerProcess.WorkerProcess_Faulted() worker leaving: " + processId + " State: " + ((IDuplexContextChannel)sender).State);
            Close();
        }

        ListenerExceptionStatus IConnectionRegister.Register(Version version, int processId, BaseUriWithWildcard path, int queueId, Guid token, string eventName)
        {
            if (TD.MessageQueueRegisterStartIsEnabled())
            {
                TD.MessageQueueRegisterStart(this.EventTraceActivity);
            }

            Debug.Print("WorkerProcess.Register() version: " + version + " processId: " + processId + " path: " + path + " queueId: " + queueId + " token: " + token + " eventName: " + eventName);

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueRegisterCalled, SR.GetString(SR.TraceCodeMessageQueueRegisterCalled), new StringTraceRecord("Path", path.ToString()), this, null);
            }

            // Get the callback channel
            this.connectionDuplicator = OperationContext.Current.GetCallbackChannel<IConnectionDuplicator>();

            // Prevent this duplicate operation from timing out, faulting the pipe, and stopping any further communication with w3wp
            // we're gated by MaxPendingAccepts + MaxPendingConnection. see CSD Main bug 193390 for details
            ((IContextChannel)this.connectionDuplicator).OperationTimeout = TimeSpan.MaxValue;

            ListenerExceptionStatus status = ListenerExceptionStatus.Success;
            bool abortInstance = false;

            if (path == null || eventName == null)
            {
                status = ListenerExceptionStatus.InvalidArgument;
                abortInstance = true;
                goto FAILED;
            }

            // Vista only: validate remote process ID
            if (OSEnvironmentHelper.IsVistaOrGreater)
            {
                status = ListenerExceptionStatus.InvalidArgument;
                object property = OperationContext.Current.IncomingMessage.Properties[ConnectionMessageProperty.Name];
                Fx.Assert(property != null, "WorkerProcess.Register() ConnectionMessageProperty not found!");

                IConnection connection = property as IConnection;
                Fx.Assert(connection != null, "WorkerProcess.Register() ConnectionMessageProperty is not IConnection!");

                PipeHandle pipe = connection.GetCoreTransport() as PipeHandle;
                Fx.Assert(pipe != null, "WorkerProcess.Register() CoreTransport is not PipeHandle!");

                if (processId != pipe.GetClientPid())
                {
                    status = ListenerExceptionStatus.InvalidArgument;
                    abortInstance = true;
                    goto FAILED;
                }
            }

            // validate version
            Version ourVersion = Assembly.GetExecutingAssembly().GetName().Version;
            if (version > ourVersion)
            {
                // VERSIONING
                // in V1 we assume that we can handle earlier versions
                // this might not be true when we ship later releases.
                Debug.Print("WorkerProcess.Register() unsupported version ourVersion: " + ourVersion + " version: " + version);
                status = ListenerExceptionStatus.VersionUnsupported;
                goto FAILED;
            }

            if (queueId == 0 && path == null)
            {
                status = ListenerExceptionStatus.InvalidArgument;
                abortInstance = true;
                goto FAILED;
            }

            this.processId = processId;
            this.queueId = 0;
            if (queueId != 0)
            {
                this.queueId = queueId;
                status = ActivatedMessageQueue.Register(queueId, token, this);
            }
            else
            {
                status = MessageQueue.Register(path, this);
            }

            if (status == ListenerExceptionStatus.Success)
            {
                foreach (IChannel channel in OperationContext.Current.InstanceContext.IncomingChannels)
                {
                    channel.Faulted += new EventHandler(WorkerProcess_Faulted);
                    channel.Closed += new EventHandler(WorkerProcess_Closed);
                }

                try
                {
                    using (EventWaitHandle securityEvent = EventWaitHandle.OpenExisting(ListenerConstants.GlobalPrefix + eventName, EventWaitHandleRights.Modify))
                    {
                        securityEvent.Set();
                    }
                }
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    DiagnosticUtility.TraceHandledException(exception, TraceEventType.Error);

                    status = ListenerExceptionStatus.InvalidArgument;
                    abortInstance = true;
                }
            }

            if (status != ListenerExceptionStatus.Success)
            {
                goto FAILED;
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueRegisterSucceeded, SR.GetString(SR.TraceCodeMessageQueueRegisterSucceeded), new StringTraceRecord("Path", path.ToString()), this, null);
            }
            if (TD.MessageQueueRegisterCompletedIsEnabled())
            {
                TD.MessageQueueRegisterCompleted(this.EventTraceActivity, path.ToString());
            }
        FAILED:
            if (abortInstance)
            {
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.MessageQueueRegisterFailed, SR.GetString(SR.TraceCodeMessageQueueRegisterFailed),
                        new StringTraceRecord("Register", SR.GetString(SR.SharingRegistrationFailedAndAbort, status.ToString())), this, null);
                }
                if (TD.MessageQueueRegisterAbortIsEnabled())
                {
                    TD.MessageQueueRegisterAbort(this.EventTraceActivity, 
                        status.ToString(),
                        (path != null) ? path.ToString() : string.Empty);
                }

                AbortServiceInstance();
            }
            else if (status != ListenerExceptionStatus.Success)
            {
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.MessageQueueRegisterFailed, SR.GetString(SR.TraceCodeMessageQueueRegisterFailed),
                        new StringTraceRecord("Register", SR.GetString(SR.SharingRegistrationFailed, status.ToString())), this, null);
                }
                if (TD.MessageQueueRegisterFailedIsEnabled())
                {
                    TD.MessageQueueRegisterFailed(this.EventTraceActivity,
                        (path != null) ? path.ToString() : string.Empty, 
                        status.ToString());
                }

                InitiateClosingServiceInstance();
            }

            return status;
        }

        bool IConnectionRegister.ValidateUriRoute(Uri uri, System.Net.IPAddress address, int port)
        {
            if (this.queue == null)
            {
                AbortServiceInstance();
                return false;
            }

            MessageQueue destinationQueue = RoutingTable.Lookup(uri, address, port);
            return object.ReferenceEquals(destinationQueue, this.queue);
        }

        void IConnectionRegister.Unregister()
        {
            Debug.Print("WorkerProcess.Unregister() processId: " + processId);
            Close();
        }

        static bool ShouldRecoverFromProxyCall(Exception exception)
        {
            return (
                (exception is CommunicationException) ||
                (exception is ObjectDisposedException) ||
                (exception is TimeoutException)
                );
        }

        void AbortServiceInstance()
        {
            OperationContext.Current.InstanceContext.Abort();
        }

        void InitiateClosingServiceInstance()
        {
            InstanceContext serviceInstance = OperationContext.Current.InstanceContext;
            serviceInstance.BeginClose(ListenerConstants.RegistrationCloseTimeout,
                Fx.ThunkCallback(new AsyncCallback(CloseCallback)), serviceInstance);
        }

        static void CloseCallback(IAsyncResult asyncResult)
        {
            InstanceContext serviceInstance = asyncResult.AsyncState as InstanceContext;
            try
            {
                serviceInstance.EndClose(asyncResult);
            }
            catch (CommunicationException e)
            {
                DiagnosticUtility.TraceHandledException(e, TraceEventType.Information);
            }
            catch (TimeoutException e)
            {
                DiagnosticUtility.TraceHandledException(e, TraceEventType.Information);
            }
        }

        class DispatchSessionAsyncResult : AsyncResult
        {
            ListenerSessionConnection session;
            bool duplicateSucceeded;
            static AsyncCallback dispatchSessionCallback = Fx.ThunkCallback(new AsyncCallback(DispatchSessionCompletedCallback));

            public DispatchSessionAsyncResult(ListenerSessionConnection session, AsyncCallback callback, object state)
                : base(callback, state)
            {
                this.session = session;
                DuplicateContext duplicateContext = null;
                try
                {
                    duplicateContext = session.WorkerProcess.DuplicateConnection(session);
                }
                catch (ServiceActivationException e)
                {
                    int traceCode;
                    string traceDescription;
                    if (session.WorkerProcess is TcpWorkerProcess)
                    {
                        traceCode = ListenerTraceCode.MessageQueueDuplicatedSocketError;
                        traceDescription = SR.GetString(SR.TraceCodeMessageQueueDuplicatedSocketError);
                    }
                    else
                    {
                        traceCode = ListenerTraceCode.MessageQueueDuplicatedPipeError;
                        traceDescription = SR.GetString(SR.TraceCodeMessageQueueDuplicatedPipeError);
                    }

                    if (DiagnosticUtility.ShouldTraceError)
                    {
                        ListenerTraceUtility.TraceEvent(TraceEventType.Error, traceCode, traceDescription, this, e);
                    }
                    this.Complete(true, e);
                    return;
                }

                IAsyncResult result = this.session.WorkerProcess.ConnectionDuplicator.BeginDuplicate(duplicateContext,
                    dispatchSessionCallback, this);

                if (result.CompletedSynchronously)
                {
                    CompleteDuplicateSession(result);
                    this.Complete(true);
                }
            }

            static void DispatchSessionCompletedCallback(IAsyncResult result)
            {
                if (result.CompletedSynchronously)
                    return;

                DispatchSessionAsyncResult thisPtr = (DispatchSessionAsyncResult)result.AsyncState;

                Exception completeException = null;

                try
                {
                    thisPtr.CompleteDuplicateSession(result);
                }
#pragma warning suppress 56500 // covered by FxCOP
                catch (Exception exception)
                {
                    if (Fx.IsFatal(exception))
                    {
                        throw;
                    }

                    completeException = exception;
                }

                thisPtr.Complete(false, completeException);
            }

            void CompleteDuplicateSession(IAsyncResult result)
            {
                this.session.WorkerProcess.ConnectionDuplicator.EndDuplicate(result);

                // Successfully duplicated the session.
                duplicateSucceeded = true;
            }

            public bool DuplicateSucceeded
            {
                get
                {
                    return duplicateSucceeded;
                }
            }

            public static DispatchSessionAsyncResult End(IAsyncResult result)
            {
                return AsyncResult.End<DispatchSessionAsyncResult>(result);
            }
        }
    }
}
