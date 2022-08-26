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
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Diagnostics;
    using System.ServiceProcess;
    using System.Threading;
    using MQException = System.Messaging.MessageQueueException;
    using MQMessage = System.Messaging.Message;
    using MQMessageQueue = System.Messaging.MessageQueue;

    class MsmqActivation : ServiceBase
    {
        BindingsManager bindings;

        ActivationService integrationActivationService;
        ListenerAdapter integrationListenerAdapter;

        ActivationService transportActivationService;
        ListenerAdapter transportListenerAdapter;

        public MsmqActivation()
        {
            ServiceName = ListenerConstants.MsmqActivationServiceName;
            CanHandlePowerEvent = false;
            AutoLog = false;
            CanStop = true;
            CanPauseAndContinue = true;
            CanShutdown = true;

            this.bindings = new BindingsManager();

            this.integrationActivationService = new ActivationService(this, MsmqUri.FormatNameAddressTranslator.Scheme);
            this.transportActivationService = new ActivationService(this, MsmqUri.NetMsmqAddressTranslator.Scheme);
        }

        protected override void OnStart(string[] args)
        {
            try
            {
                if (DiagnosticUtility.ShouldTraceInformation)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceStart, SR.GetString(SR.TraceCodeServiceStart), this);
                }

#if DEBUG
                if (DebuggableService.DelayStart(ServiceName))
                {
                    (new Thread(new ThreadStart(Start))).Start();
                    return;
                }
#endif

                Start();
            }
            catch (Exception exception)
            {
                // Log the error to eventlog.
                ListenerTraceUtility.EventLog.LogEvent(TraceEventType.Error,
                    (ushort)System.Runtime.Diagnostics.EventLogCategory.ListenerAdapter,
                    (uint)System.Runtime.Diagnostics.EventLogEventId.ServiceStartFailed,
                    false,
                    exception.ToString());

                throw;
            }
        }

        protected override void OnStop()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceStop, SR.GetString(SR.TraceCodeServiceStop), this);
            }

            Shutdown();
        }

        protected override void OnContinue()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceContinue, SR.GetString(SR.TraceCodeServiceContinue), this);
            }

            this.integrationActivationService.Paused = false;
            this.transportActivationService.Paused = false;
        }

        protected override void OnPause()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServicePause, SR.GetString(SR.TraceCodeServicePause), this);
            }

            this.integrationActivationService.Paused = true;
            this.transportActivationService.Paused = true;
        }

        protected override void OnShutdown()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceShutdown, SR.GetString(SR.TraceCodeServiceShutdown), this);
            }

            Shutdown();
            Stop();
        }

        void Start()
        {
#if DEBUG
            DebuggableService.WaitForDebugger(ServiceName);
#endif
            if (!SMSvcHost.IsWebhostSupported)
            {
                const int ERROR_NOT_SUPPORTED = 50;
                this.ExitCode = ERROR_NOT_SUPPORTED;
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new NotSupportedException(SR.GetString(SR.ServiceRequiresWas)));
            }

            this.integrationListenerAdapter = new ListenerAdapter(this.integrationActivationService);
            this.transportListenerAdapter = new ListenerAdapter(this.transportActivationService);

            this.integrationListenerAdapter.Open();
            this.transportListenerAdapter.Open();
        }

        void Shutdown()
        {
            this.integrationListenerAdapter.Close();
            this.transportListenerAdapter.Close();
            this.bindings.Close();
        }

        class BindingsManager
        {
            Dictionary<string, MsmqBindingMonitor> bindingMonitors;
            object thisLock = new object();

            public BindingsManager()
            {
                this.bindingMonitors = new Dictionary<string, MsmqBindingMonitor>(StringComparer.OrdinalIgnoreCase);
            }

            public void RegisterBindingFilterIfNecessary(string host, MsmqBindingFilter filter)
            {
                lock (this.thisLock)
                {
                    MsmqBindingMonitor bindingMonitor;
                    if (!this.bindingMonitors.TryGetValue(host, out bindingMonitor))
                    {
                        bindingMonitor = new MsmqBindingMonitor(host);
                        bindingMonitor.Open();
                        this.bindingMonitors.Add(host, bindingMonitor);
                    }

                    // register the new filter if it doesn't already exist:
                    if (!bindingMonitor.ContainsFilter(filter))
                    {
                        bindingMonitor.AddFilter(filter);
                    }
                }
            }

            public void UnregisterBindingFilter(MsmqBindingFilter filter)
            {
                lock (this.thisLock)
                {
                    foreach (MsmqBindingMonitor monitor in this.bindingMonitors.Values)
                    {
                        monitor.RemoveFilter(filter);
                    }
                }
            }

            public void Close()
            {
                lock (this.thisLock)
                {
                    foreach (MsmqBindingMonitor monitor in this.bindingMonitors.Values)
                    {
                        monitor.Close();
                    }

                    this.bindingMonitors.Clear();
                }
            }
        }

        class ActivationService : IActivationService
        {
            Dictionary<int, QueueMonitorGroup> groups;
            string protocol;
            BindingsManager bindings;
            object thisLock = new object();
            ServiceBase service;
            bool paused;

            public ActivationService(MsmqActivation service, string protocol)
            {
                this.protocol = protocol;
                this.bindings = service.bindings;
                this.service = service;
                this.paused = false;

                this.groups = new Dictionary<int, QueueMonitorGroup>();
            }

            public bool Paused
            {
                get { return this.paused; }
                set
                {
                    lock (this.thisLock)
                    {
                        if (this.paused != value)
                        {
                            this.paused = value;
                            if (!this.paused)
                            {
                                foreach (QueueMonitorGroup group in this.groups.Values)
                                {
                                    group.Start();
                                }
                            }
                        }
                    }
                }
            }

            public BindingsManager Bindings
            {
                get { return this.bindings; }
            }

            public string ActivationServiceName
            {
                get
                {
                    return this.service.ServiceName;
                }
            }

            public string ProtocolName
            {
                get { return this.protocol; }
            }

            public IActivatedMessageQueue CreateQueue(ListenerAdapter la, App app)
            {
                QueueMonitorGroup qmg = new QueueMonitorGroup(this, la, app);
                lock (this.thisLock)
                {
                    this.groups[qmg.ListenerChannelContext.ListenerChannelId] = qmg;
                }
                return qmg;
            }

            public IActivatedMessageQueue FindQueue(int queueId)
            {
                lock (this.thisLock)
                {
                    QueueMonitorGroup group;
                    this.groups.TryGetValue(queueId, out group);
                    return group;
                }
            }

            public void StopService()
            {
                this.service.Stop();
            }

            public void QueueMonitorGroupClosed(QueueMonitorGroup qmg)
            {
                lock (this.thisLock)
                {
                    this.groups.Remove(qmg.ListenerChannelContext.ListenerChannelId);
                }
            }
        }

        class QueueMonitorGroup : IActivatedMessageQueue
        {
            static int queueIdCounter = 0;
            static readonly TimeSpan RetryMonitorInterval = TimeSpan.FromMinutes(5);

            ActivationService activationService;
            App app;
            ActivationBindingFilter filter;
            ListenerAdapter listenerAdapter;
            int startQueueInstanceCount;
            ListenerChannelContext listenerChannelContext;
            List<QueueMonitor> monitors = new List<QueueMonitor>();
            List<QueueMonitor> failedMonitors = new List<QueueMonitor>();
            bool enabled;
            int pendingNotificationCount;
            IOThreadTimer retryTimer;
            bool retryScheduled = false;
            bool hasStartedQueueInstances;
            object thisLock = new object();

            public QueueMonitorGroup(ActivationService activationService, ListenerAdapter la, App app)
            {
                this.activationService = activationService;
                this.listenerAdapter = la;
                this.app = app;
                this.startQueueInstanceCount = 1;
                this.listenerChannelContext = new ListenerChannelContext(app.AppKey,
                    Interlocked.Increment(ref queueIdCounter), Guid.Empty);

                this.pendingNotificationCount = 0;
                this.filter = new ActivationBindingFilter(this, app.Path);
                this.retryTimer = new IOThreadTimer(new Action<object>(OnRetryTimer), null, false);
            }

            public bool CanDispatch
            {
                get { return this.enabled && !this.activationService.Paused; }
            }

            public App App
            {
                get { return this.app; }
            }

            public ListenerChannelContext ListenerChannelContext
            {
                get { return this.listenerChannelContext; }
            }

            bool IActivatedMessageQueue.HasStartedQueueInstances
            {
                get { return this.hasStartedQueueInstances; }
            }

            void IActivatedMessageQueue.OnQueueInstancesStopped()
            {
                this.hasStartedQueueInstances = false;
            }

            public void Delete()
            {
                this.retryTimer.Cancel();
                this.activationService.QueueMonitorGroupClosed(this);
                UnregisterAll();
            }

            public void LaunchQueueInstance()
            {
                bool startInstance = false;

                lock (this.thisLock)
                {
                    if (this.pendingNotificationCount > 0)
                    {
                        this.pendingNotificationCount--;
                        startInstance = true;
                    }
                    else
                    {
                        // start monitoring for new messages...
                        startQueueInstanceCount++;

                        // Make sure that everyone is peeking:
                        foreach (QueueMonitor monitor in this.monitors)
                        {
                            monitor.Start();
                        }
                    }
                }

                if (startInstance)
                {
                    if (this.listenerAdapter.OpenListenerChannelInstance(this))
                    {
                        this.hasStartedQueueInstances = true;
                    }
                }
            }

            public ListenerExceptionStatus Register(BaseUriWithWildcard url)
            {
                this.activationService.Bindings.RegisterBindingFilterIfNecessary(url.BaseAddress.Host, this.filter);
                return ListenerExceptionStatus.Success;
            }

            public void Start()
            {
                lock (this.thisLock)
                {
                    if (this.CanDispatch)
                    {
                        // Ensure that we're started...
                        foreach (QueueMonitor monitor in this.monitors)
                        {
                            monitor.Start();
                        }
                    }
                }
            }

            public void SetEnabledState(bool enabled)
            {
                lock (this.thisLock)
                {
                    if (this.enabled != enabled)
                    {
                        this.enabled = enabled;
                        Start();
                    }
                }
            }

            public void UnregisterAll()
            {
                lock (this.thisLock)
                {
                    foreach (QueueMonitor monitor in this.monitors)
                    {
                        if (monitor != null)
                        {
                            monitor.Dispose();
                        }
                    }

                    this.monitors.Clear();
                }

                this.activationService.Bindings.UnregisterBindingFilter(this.filter);
            }

            public bool NotifyMessageAvailable()
            {
                bool startInstance = false;
                bool shouldContinue = false;

                lock (this.thisLock)
                {
                    if (!this.CanDispatch)
                    {
                        this.pendingNotificationCount++;
                    }
                    else if (this.startQueueInstanceCount == 0)
                    {
                        this.pendingNotificationCount++;
                    }
                    else
                    {
                        this.startQueueInstanceCount--;
                        startInstance = true;
                        shouldContinue = this.startQueueInstanceCount > 0;
                    }
                }

                if (startInstance)
                {
                    MsmqDiagnostics.StartingApplication(this.app.Path);
                    this.listenerAdapter.OpenListenerChannelInstance(this);
                    this.hasStartedQueueInstances = true;
                }
                return shouldContinue;
            }

            public void ScheduleRetry(QueueMonitor monitor)
            {
                lock (this.thisLock)
                {
                    this.failedMonitors.Add(monitor);

                    if (!this.retryScheduled)
                    {
                        this.retryTimer.Set(RetryMonitorInterval);
                        this.retryScheduled = true;
                    }
                }
            }

            object AddQueueToGroup(Uri queue)
            {
                QueueMonitor monitor = null;
                lock (this.thisLock)
                {
                    monitor = new QueueMonitor(queue, this);
                    this.monitors.Add(monitor);
                    if (this.enabled)
                    {
                        monitor.Start();
                    }
                }

                return monitor;
            }

            void OnRetryTimer(object state)
            {
                lock (this.thisLock)
                {
                    if (this.enabled)
                    {
                        foreach (QueueMonitor monitor in this.failedMonitors)
                        {
                            // Only start it if we still own it...
                            if (this.monitors.Contains(monitor))
                            {
                                monitor.Start();
                            }
                        }
                    }
                    this.failedMonitors.Clear();
                }
            }

            void RemoveQueueFromGroup(object state)
            {
                QueueMonitor monitor = (QueueMonitor)state;
                lock (this.thisLock)
                {
                    this.monitors.Remove(monitor);
                    monitor.Dispose();
                }
            }

            // Note that we inherit from the transport binding filter here - that's not 
            // a big deal, because we never need these uris to create services.
            class ActivationBindingFilter : MsmqBindingFilter
            {
                QueueMonitorGroup group;

                public ActivationBindingFilter(QueueMonitorGroup group, string path)
                    : base(path, MsmqUri.NetMsmqAddressTranslator)
                {
                    this.group = group;
                }

                public override object MatchFound(string host, string name, bool isPrivate)
                {
                    MsmqDiagnostics.MatchedApplicationFound(host, name, isPrivate, this.CanonicalPrefix);
                    return this.group.AddQueueToGroup(CreateServiceUri(host, name, isPrivate));
                }

                public override void MatchLost(string host, string name, bool isPrivate, object callbackState)
                {
                    this.group.RemoveQueueFromGroup(callbackState);
                }
            }
        }

        class QueueMonitor : IDisposable
        {
            static readonly TimeSpan InfiniteTimeout = TimeSpan.FromMilliseconds(UInt32.MaxValue);
            object thisLock = new object();
            int disposed;
            QueueMonitorGroup group;
            bool peeking;
            string queueName;
            MQMessageQueue queue;

            public QueueMonitor(Uri uri, QueueMonitorGroup group)
            {
                // The defaults don't really matter here - we don't use
                // the buffer manager.
                this.group = group;
                this.queueName = MsmqFormatName.ToSystemMessagingQueueName(MsmqUri.UriToFormatNameByScheme(uri));
                this.peeking = false;
                Debug.Print("opening queue: " + this.queueName);
            }

            public void Start()
            {
                lock (this.thisLock)
                {
                    try
                    {
                        if (this.queue == null)
                        {
                            this.queue = new MQMessageQueue(this.queueName);
                            this.queue.MessageReadPropertyFilter.ClearAll();
                            this.queue.MessageReadPropertyFilter.LookupId = true;
                        }

                        if (!this.peeking)
                        {
                            this.peeking = true;
                            this.queue.BeginPeek(InfiniteTimeout, null, Fx.ThunkCallback(new AsyncCallback(OnPeekCompleted)));
                        }
                    }
                    catch (MQException)
                    {
                        this.group.ScheduleRetry(this);
                    }
                }
            }

            public void Dispose()
            {
                if (Interlocked.Exchange(ref disposed, 1) == 0)
                {
                    if (this.queue != null)
                    {
                        this.queue.Dispose();
                    }
                }
            }

            void OnPeekCompleted(IAsyncResult result)
            {
                bool shouldContinue = true;
                try
                {
                    MQMessage message = this.queue.EndPeek(result);

                    Debug.Print("MsmqActivation.QueueMonitor.OnPeekCompleted: message available");
                    shouldContinue = this.group.NotifyMessageAvailable();
                }
                catch (MQException ex)
                {
                    MsmqDiagnostics.CannotPeekOnQueue(this.queue.FormatName, ex);
                    this.group.ScheduleRetry(this);
                    return;
                }
                catch (Exception ex)
                {
                    DiagnosticUtility.TraceHandledException(ex, TraceEventType.Error);

                    if (!Fx.IsFatal(ex))
                    {
                        this.group.ScheduleRetry(this);
                    }

                    throw;
                }

                lock (this.thisLock)
                {
                    if ((this.disposed == 0) && shouldContinue)
                    {
                        this.queue.BeginPeek(InfiniteTimeout, null, Fx.ThunkCallback(new AsyncCallback(OnPeekCompleted)));
                    }
                    else
                    {
                        this.peeking = false;
                    }
                }
            }
        }
    }
}

