//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Runtime;
    using System.ServiceModel.Channels;
    using System.Threading;

    class ActivatedMessageQueue : MessageQueue, IActivatedMessageQueue
    {
        const int ThrottlingMaxSkewInMilliseconds = 5000;
        static TimeSpan FailureThrottlingTimeout = TimeSpan.FromSeconds(15);

        App app;
        bool enabled;
        ListenerAdapter listenerAdapter;
        static int listenerChannelIdCounter;
        static Hashtable listenerChannelIds = new Hashtable();
        ListenerChannelContext listenerChannelContext;
        QueueState queueState;
        object syncRoot = new object();

        // Used for failure throttling.
        int listenerChannelFailCount;
        IOThreadTimer throttlingTimer;

        internal ActivatedMessageQueue(ListenerAdapter listenerAdapter, App app)
            : base()
        {
            Debug.Print("ActivatedMessageQueue.ctor(listenerAdapter:" + listenerAdapter + " appKey:" + app.AppKey + " appPoolId:" + app.AppPool.AppPoolId + ")");
            this.listenerAdapter = listenerAdapter;
            this.app = app;
            this.queueState = QueueState.PendingOpen;

            CreateListenerChannelContext();
        }

        void CreateListenerChannelContext()
        {
            listenerChannelContext = new ListenerChannelContext(this.app.AppKey,
                Interlocked.Increment(ref listenerChannelIdCounter), Guid.NewGuid());

            listenerChannelIds[listenerChannelContext.ListenerChannelId] = this;
        }

        public App App { get { return app; } }
        public ListenerChannelContext ListenerChannelContext { get { return listenerChannelContext; } }
        public void Delete()
        {
            SetEnabledState(false);
            Close();
        }

        internal static ActivatedMessageQueue Find(int listenerChannelId) { return listenerChannelIds[listenerChannelId] as ActivatedMessageQueue; }
        object ThisLock { get { return syncRoot; } }
        protected override bool CanShare { get { return true; } }

        internal override bool CanDispatch
        {
            get
            {
                return
                    base.CanDispatch &&
                    enabled &&
                    queueState != QueueState.Faulted &&
                    listenerAdapter.CanDispatch &&
                       (TransportType == TransportType.Tcp && !SMSvcHost.IsTcpActivationPaused
                        || TransportType == TransportType.NamedPipe && !SMSvcHost.IsNamedPipeActivationPaused) &&
                    app.AppPool.Enabled;
            }
        }

        // Return true if it's faulted.
        bool OnListenerChannelFailed()
        {
            lock (ThisLock)
            {
                // Increment the count.
                listenerChannelFailCount++;

                if (listenerChannelFailCount <= 6)
                {
                    return false;
                }

                listenerChannelFailCount = 0;
            }

            FaultMessageQueueOnFailure();
            return true;
        }

        void FaultMessageQueueOnFailure()
        {
            lock (ThisLock)
            {
                this.queueState = QueueState.Faulted;

                // Drop pending messages.
                this.DropPendingMessages(true);

                // Throttling
                if (throttlingTimer == null)
                {
                    throttlingTimer = new IOThreadTimer(new Action<object>(ThrottlingCallback),
                        this, true, ThrottlingMaxSkewInMilliseconds);
                }

                throttlingTimer.Set(FailureThrottlingTimeout);
            }
        }

        void ThrottlingCallback(object state)
        {
            lock (ThisLock)
            {
                this.queueState = QueueState.PendingOpen;
                listenerChannelFailCount = 0;
            }
        }

        public void LaunchQueueInstance()
        {
            lock (ThisLock)
            {
                if (this.queueState == QueueState.Faulted)
                {
                    return;
                }
                else if (this.queueState == QueueState.OpenedPendingConnect)
                {
                    // We treat this as error case.
                    if (this.OnListenerChannelFailed())
                    {
                        return;
                    }
                }
                
                this.queueState = QueueState.PendingOpen;
            }

            if (this.PendingCount > 0)
            {
                EnsureListenerChannelInstanceOpened();
            }
        }

        internal static ListenerExceptionStatus Register(int listenerChannelId, Guid token, WorkerProcess worker)
        {
            Debug.Print("ActivatedMessageQueue.Register() listenerChannelId: " + listenerChannelId + " token: " + token + " worker: " + worker.ProcessId);
            
            ActivatedMessageQueue thisPtr = null;
            lock (listenerChannelIds)
            {
                thisPtr = Find(listenerChannelId);
                if (thisPtr == null)
                {
                    // this is an error.
                    return ListenerExceptionStatus.InvalidArgument;
                }

                if (!token.Equals(thisPtr.listenerChannelContext.Token))
                {
                    return ListenerExceptionStatus.InvalidArgument;
                }
            }

            thisPtr.OnListenerChannelConnected();
            thisPtr.OnNewWorkerAvailable(worker);
            return ListenerExceptionStatus.Success;
        }

        void OnListenerChannelConnected()
        {
            lock (ThisLock)
            {
                // Clear the failure count.
                this.listenerChannelFailCount = 0;
                this.queueState = QueueState.Connected;
            }
        }

        public void SetEnabledState(bool enabled)
        {
            if (this.enabled != enabled)
            {
                this.enabled = enabled;

                if (enabled)
                {
                    IncrementRegistrationsActiveCounters();
                }
                else
                {
                    DecrementRegistrationsActiveCounters();
                    DropPendingMessages(true);
                }
            }
        }

        protected override void OnSessionEnqueued()
        {
            // Make sure that the ListenerChannelInstance is opened for new requests.
            EnsureListenerChannelInstanceOpened();
        }

        protected override void OnRegisterCompleted()
        {
            this.queueState = QueueState.PendingOpen;
        }

        protected override void OnUnregisterCompleted()
        {
            this.queueState = QueueState.PendingOpen;
        }

        void EnsureListenerChannelInstanceOpened()
        {
            lock (ThisLock)
            {
                if (this.queueState != QueueState.PendingOpen)
                {
                    return;
                }

                this.queueState = QueueState.OpenedPendingConnect;
            }

            if (!listenerAdapter.OpenListenerChannelInstance(this))
            {
                FaultMessageQueueOnFailure();
            }
        }

        bool IActivatedMessageQueue.HasStartedQueueInstances
        {
            get
            {
                return this.queueState == QueueState.Connected;
            }
        }

        void IActivatedMessageQueue.OnQueueInstancesStopped()
        {
            lock (ThisLock)
            {
                this.queueState = QueueState.PendingOpen;
            }
        }

        protected override void OnUnregisterLastWorker()
        {
        }

        ListenerExceptionStatus IActivatedMessageQueue.Register(BaseUriWithWildcard url)
        {
            return base.Register(url);
        }

        void IActivatedMessageQueue.UnregisterAll()
        {
            base.UnregisterAll();
        }

        enum QueueState
        {
            Faulted,
            PendingOpen,
            OpenedPendingConnect,
            Connected
        }
    }
}

