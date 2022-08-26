//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Diagnostics;
    using System.Threading;

    class MessageQueue
    {
        static Action<object> dispatchToNewWorkerCallback = new Action<object>(DispatchToNewWorkerCallback);
        static Action<object> dispatchSessionCallback = new Action<object>(DispatchSessionCallback);

        static Dictionary<BaseUriWithWildcard, MessageQueue> registry = new Dictionary<BaseUriWithWildcard, MessageQueue>();
        static List<MessageQueue> instances = new List<MessageQueue>();
        AsyncCallback dispatchSessionCompletedCallback;
        List<BaseUriWithWildcard> paths;

        // we use a queue of session-messages for dispatching
        // we use it to park messages that can't be dispatched and need to be pended
        // we use a queue of WorkerProcess instances to find free ones that can be dispatched to
        Queue<ListenerSessionConnection> sessionMessages;
        Queue<WorkerProcess> sessionWorkers;
        int maxQueueSize;

        TransportType transportType;
        EventTraceActivity eventTraceActivity;

        // each MessageQueue has a list of WorkerProcess instances.
        // each WorkerProcess is associated to a single MessageQueue.
        // Self-Hosted: 1 WorkerProcess in the list at all times, always the same WorkerProcess (unless we DCR it). 1st WorkerProcess creates the MessageQueue, last WorkerProcess deletes the MessageQueue.
        // Web-Hosted: 0-n WorkerProcess in the list. MessageQueue created/delete by WAS explicitly or implicitly by WAS going away.
        List<WorkerProcess> workers;

        internal MessageQueue()
        {
            transportType = TransportType.Unsupported;
            paths = new List<BaseUriWithWildcard>();
            workers = new List<WorkerProcess>();
            sessionWorkers = new Queue<WorkerProcess>();
            sessionMessages = new Queue<ListenerSessionConnection>();
            dispatchSessionCompletedCallback = Fx.ThunkCallback(new AsyncCallback(DispatchSessionCompletedCallback));

            lock (instances)
            {
                instances.Add(this);
            }
        }

#if DEBUG
        internal List<WorkerProcess> SnapshotWorkers()
        {
            lock (this.workers)
            {
                return new List<WorkerProcess>(workers);
            }
        }
#endif
        internal virtual bool CanDispatch
        {
            get
            {
                return TransportType != TransportType.Tcp ||
                    !SMSvcHost.IsTcpPortSharingPaused;
            }
        }

        internal TransportType TransportType
        {
            get
            {
                return transportType;
            }
        }

        object SessionLock
        {
            get
            {
                return sessionWorkers;
            }
        }

        internal static void CloseAll(TransportType transportType)
        {
            MessageQueue[] instancesCopy;
            lock (instances)
            {
                instancesCopy = instances.ToArray();
                instances.Clear();
            }
            foreach (MessageQueue messageQueue in instancesCopy)
            {
                if (messageQueue.TransportType == transportType)
                {
                    messageQueue.CloseCore();
                }
            }
        }

        protected int PendingCount
        {
            get
            {
                lock (SessionLock)
                {
                    return sessionMessages.Count;
                }
            }
        }

        EventTraceActivity EventTraceActivity
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

        protected void Close()
        {
            Debug.Print("MessageQueue.Close()");
            // this is only called when all the workers are done
            // with I/O (they could be in the process of closing)
            lock (instances)
            {
                instances.Remove(this);
            }
            CloseCore();

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueClosed, SR.GetString(SR.TraceCodeMessageQueueClosed), this);
            }
        }

        protected void DropPendingMessages(bool sendFault)
        {
            lock (SessionLock)
            {
                foreach (ListenerSessionConnection sessionMessage in sessionMessages.ToArray())
                {
                    if (sessionMessage != null)
                    {
                        if (sendFault)
                        {
                            TransportListener.SendFault(sessionMessage.Connection, FramingEncodingString.EndpointUnavailableFault);
                        }
                        else
                        {
                            sessionMessage.Connection.Abort();
                        }
                    }

                }
                sessionMessages.Clear();
            }
        }

        void CloseCore()
        {
            Debug.Print("MessageQueue.CloseCore()");
            UnregisterAll();
            DropPendingMessages(false);
            lock (registry)
            {
                foreach (WorkerProcess worker in workers.ToArray())
                {
                    worker.Close();
                }
                workers.Clear();
            }

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueClosed, SR.GetString(SR.TraceCodeMessageQueueClosed), this);
            }
        }

        internal void EnqueueSessionAndDispatch(ListenerSessionConnection session)
        {
            lock (SessionLock)
            {
                if (!CanDispatch)
                {
                    TransportListener.SendFault(session.Connection, FramingEncodingString.EndpointUnavailableFault);
                    OnDispatchFailure(transportType);
                    return;
                }
                else if (sessionMessages.Count >= maxQueueSize)
                {
                    // Abort the connection when the queue is full.
                    if (TD.PendingSessionQueueFullIsEnabled())
                    {
                        TD.PendingSessionQueueFull(session.EventTraceActivity,
                            (session.Via != null) ? session.Via.ToString() : string.Empty,
                            sessionMessages.Count);
                    }
                    session.Connection.Abort();
                    OnDispatchFailure(transportType);
                    return;
                }
                else
                {
                    sessionMessages.Enqueue(session);
                    if (TD.PendingSessionQueueRatioIsEnabled())
                    {
                        TD.PendingSessionQueueRatio(sessionMessages.Count, maxQueueSize);
                    }
                }
            }

            OnSessionEnqueued();
            DispatchSession();
        }

        void EnqueueWorkerAndDispatch(WorkerProcess worker, bool canDispatchOnThisThread)
        {
            lock (SessionLock)
            {
                sessionWorkers.Enqueue(worker);
            }

            if (canDispatchOnThisThread)
            {
                DispatchSession();
            }
            else
            {
                ActionItem.Schedule(dispatchSessionCallback, this);
            }
        }

        static void DispatchSessionCallback(object state)
        {
            MessageQueue thisPtr = (MessageQueue)state;
            thisPtr.DispatchSession();
        }

        void DispatchSession()
        {
            for (;;)
            {
                ListenerSessionConnection session = null;
                lock (SessionLock)
                {
                    if (sessionMessages.Count > 0)
                    {
                        WorkerProcess worker = null;
                        while (sessionWorkers.Count > 0)
                        {
                            worker = sessionWorkers.Dequeue();
                            if (worker.IsRegistered)
                            {
                                break;
                            }
                            worker = null;
                        }

                        if (worker == null)
                        {
                            // There is no more active worker. So break the loop.
                            break;
                        }

                        // For better performance, we may want to check whether the message has been timed out in the future.
                        session = sessionMessages.Dequeue();
                        session.WorkerProcess = worker;
                    }
                }

                if (session == null)
                {
                    // There is mo more message left. So break the loop.
                    break;
                }

                StartDispatchSession(session);
            }
        }

        void StartDispatchSession(ListenerSessionConnection session)
        {
            if (TD.DispatchSessionStartIsEnabled())
            {
                TD.DispatchSessionStart(session.EventTraceActivity);
            }

            IAsyncResult dispatchAsyncResult = null;
            try
            {
                dispatchAsyncResult = session.WorkerProcess.BeginDispatchSession(session, dispatchSessionCompletedCallback, session);
            }
            catch (Exception exception)
            {
                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                DiagnosticUtility.TraceHandledException(exception, TraceEventType.Warning);

                if (session.WorkerProcess.IsRegistered)
                {
                    // Add the worker back to the queue.
                    EnqueueWorkerAndDispatch(session.WorkerProcess, false);
                }
            }

            if (dispatchAsyncResult != null && dispatchAsyncResult.CompletedSynchronously)
            {
                CompleteDispatchSession(dispatchAsyncResult);
            }
        }

        void DispatchSessionCompletedCallback(IAsyncResult result)
        {
            if (result.CompletedSynchronously)
            {
                return;
            }

            CompleteDispatchSession(result);
        }

        void CompleteDispatchSession(IAsyncResult result)
        {
            ListenerSessionConnection session = (ListenerSessionConnection)result.AsyncState;
            Fx.Assert(session.WorkerProcess != null, "The WorkerProcess should be set on the message.");

            bool success = session.WorkerProcess.EndDispatchSession(result);
            TraceDispatchCompleted(success, session);

            if (!success)
            {
                OnConnectionDispatchFailed(session.Connection);
            }

            EnqueueWorkerAndDispatch(session.WorkerProcess, !result.CompletedSynchronously);
        }

        void TraceDispatchCompleted(bool success, ListenerSessionConnection session)
        {
            if (success)
            {
                if (TD.DispatchSessionSuccessIsEnabled())
                {
                    TD.DispatchSessionSuccess(session.EventTraceActivity);
                }
            }
            else
            {
                if (TD.DispatchSessionFailedIsEnabled())
                {
                    TD.DispatchSessionFailed(session.EventTraceActivity);
                }
            }
        }

        protected virtual bool CanShare
        {
            get { return false; }
        }

        internal static void OnDispatchFailure(TransportType transportType)
        {
            if (transportType == TransportType.Tcp)
            {
                ListenerPerfCounters.IncrementDispatchFailuresTcp();
            }
            else if (transportType == TransportType.NamedPipe)
            {
                ListenerPerfCounters.IncrementDispatchFailuresNamedPipe();
            }
        }

        bool OnConnectionDispatchFailed(IConnection connection)
        {
            TransportListener.SendFault(connection, FramingEncodingString.ConnectionDispatchFailedFault);
            return false;
        }

        protected void OnNewWorkerAvailable(WorkerProcess worker)
        {
            lock (this.workers)
            {
                worker.Queue = this;
                workers.Add(worker);

                // offload draining the IO queues to this new worker on a different thread
                ActionItem.Schedule(dispatchToNewWorkerCallback, worker);
            }
        }

        static void DispatchToNewWorkerCallback(object state)
        {
            WorkerProcess worker = state as WorkerProcess;
            worker.Queue.EnqueueWorkerAndDispatch(worker, true);
        }

        public ListenerExceptionStatus Register(BaseUriWithWildcard path)
        {
            if (path.BaseAddress.Scheme == Uri.UriSchemeNetTcp)
            {
                if (transportType == TransportType.NamedPipe)
                {
                    return ListenerExceptionStatus.ProtocolUnsupported;
                }

                maxQueueSize = ListenerConfig.NetTcp.MaxPendingConnections;
                transportType = TransportType.Tcp;
            }
            else if (path.BaseAddress.Scheme == Uri.UriSchemeNetPipe)
            {
                if (transportType == TransportType.Tcp)
                {
                    return ListenerExceptionStatus.ProtocolUnsupported;
                }

                maxQueueSize = ListenerConfig.NetPipe.MaxPendingConnections;
                transportType = TransportType.NamedPipe;
            }
            else
            {
                return ListenerExceptionStatus.ProtocolUnsupported;
            }

            ListenerExceptionStatus status = RoutingTable.Start(this, path);
            if (status == ListenerExceptionStatus.Success)
            {
                paths.Add(path);
                IncrementUrisRegisteredCounters();
                OnRegisterCompleted();
            }

            return status;
        }

        internal static ListenerExceptionStatus Register(BaseUriWithWildcard path, WorkerProcess worker)
        {
            MessageQueue queue = null;
            lock (registry)
            {
                if (registry.TryGetValue(path, out queue))
                {
                    if (!queue.CanShare)
                    {
                        return ListenerExceptionStatus.ConflictingRegistration;
                    }
                }
                else
                {
                    queue = new MessageQueue();
                    ListenerExceptionStatus status = ListenerExceptionStatus.FailedToListen;

                    try
                    {
                        status = queue.Register(path);
                    }
                    catch (Exception exception)
                    {
                        if (Fx.IsFatal(exception))
                        {
                            throw;
                        }

                        if (DiagnosticUtility.ShouldTraceError)
                        {
                            ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.RoutingTableCannotListen, SR.GetString(SR.TraceCodeRoutingTableCannotListen), new StringTraceRecord("Path", path.ToString()), null, exception);
                        }
                    }

                    if (status != ListenerExceptionStatus.Success)
                    {
                        // not setting the worker.queue is not a problem, since we can't use this WorkerProcess
                        return status;
                    }

                    registry.Add(path, queue);
                }
            }

            queue.OnNewWorkerAvailable(worker);
            return ListenerExceptionStatus.Success;
        }

        protected virtual void OnSessionEnqueued() { }

        public void UnregisterAll()
        {
            while (paths.Count > 0)
            {
                Unregister(paths[0]);
            }
        }

        void Unregister(BaseUriWithWildcard path)
        {
            Fx.Assert(paths.Contains(path), "Unregister: unregistering an unregistered path");

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.MessageQueueUnregisterSucceeded, SR.GetString(SR.TraceCodeMessageQueueUnregisterSucceeded), new StringTraceRecord("Path", path.ToString()), this, null);
            }

            if (TD.MessageQueueUnregisterSucceededIsEnabled())
            {
                TD.MessageQueueUnregisterSucceeded(this.EventTraceActivity, path.ToString());
            }

            RoutingTable.Stop(this, path);
            IncrementUrisUnregisteredCounters();
            OnUnregisterCompleted();

            registry.Remove(path);
            paths.Remove(path);
        }

        protected virtual void OnUnregisterLastWorker()
        {
            Debug.Print("MessageQueue.OnUnregisterLastWorker() calling Close()");
            Close();
        }

        internal virtual void Unregister(WorkerProcess worker)
        {
            Debug.Print("MessageQueue.Unregister() worker: " + worker.ProcessId);
            lock (registry)
            {
                Fx.Assert(object.Equals(this, worker.Queue), "MessageQueue.Unregister() cannot unregister a worker registered with a queue different than this.");

                workers.Remove(worker);
                Debug.Print("MessageQueue.Unregister() left with workers: " + workers.Count);
                if (workers.Count == 0)
                {
                    OnUnregisterLastWorker();
                }
            }
        }

        protected virtual void OnRegisterCompleted()
        {
            IncrementRegistrationsActiveCounters();
        }

        protected virtual void OnUnregisterCompleted()
        {
            DecrementRegistrationsActiveCounters();
        }

        protected void IncrementRegistrationsActiveCounters()
        {
            if (this.TransportType == TransportType.Tcp)
            {
                ListenerPerfCounters.IncrementRegistrationsActiveTcp();
            }
            else
            {
                ListenerPerfCounters.IncrementRegistrationsActiveNamedPipe();
            }
        }

        protected void DecrementRegistrationsActiveCounters()
        {
            if (this.TransportType == TransportType.Tcp)
            {
                ListenerPerfCounters.DecrementRegistrationsActiveTcp();
            }
            else
            {
                ListenerPerfCounters.DecrementRegistrationsActiveNamedPipe();
            }
        }

        void IncrementUrisUnregisteredCounters()
        {
            if (this.TransportType == TransportType.Tcp)
            {
                ListenerPerfCounters.IncrementUrisUnregisteredTcp();
            }
            else
            {
                ListenerPerfCounters.IncrementUrisUnregisteredNamedPipe();
            }
        }

        void IncrementUrisRegisteredCounters()
        {
            if (this.TransportType == TransportType.Tcp)
            {
                ListenerPerfCounters.IncrementUrisRegisteredTcp();
            }
            else
            {
                ListenerPerfCounters.IncrementUrisRegisteredNamedPipe();
            }
        }
    }
}
