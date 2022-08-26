//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.Security.Principal;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.Threading;

    class ListenerAdapter : ListenerAdapterBase
    {
        const string SiteRootPath = "/";
        IActivationService activationService;
        bool canDispatch;

        // Double-checked locking pattern requires volatile for read/write synchronization
        volatile bool isOpen;
        AutoResetEvent wasConnected;
        AutoResetEvent cleanupComplete;
        AppManager appManager;
        ManualResetEvent initCompleted;
        Action<object> closeAllListenerChannelInstancesCallback;
        Action<object> launchQueueInstanceCallback;

        // CSDMain 190118
        // We introduce this int to indicate the current closing status
        int closingProcessStatus;
        const int ClosingProcessUnBlocked = 0;
        const int ClosingProcessBlocked = 1;
        const int ClosingProcessUnBlockedByEventOnConfigManagerDisconnected = 2;

        internal ListenerAdapter(IActivationService activationService)
            : base(activationService.ProtocolName)
        {
            this.activationService = activationService;
            appManager = new AppManager();
        }

        object ThisLock { get { return this; } }
        internal bool CanDispatch { get { return canDispatch; } }


        internal override void Open()
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::Open()");
            if (!isOpen)
            {
                lock (ThisLock)
                {
                    if (!isOpen)
                    {
                        initCompleted = new ManualResetEvent(false);
                        base.Open();
                        initCompleted.WaitOne();
                        initCompleted.Close();
                        initCompleted = null;
                        isOpen = true;
                    }
                }
            }
        }

        internal new void Close()
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::Close()");
            if (isOpen)
            {
                lock (ThisLock)
                {
                    if (isOpen)
                    {
                        isOpen = false;

                        // When calling Cleanup() in the g----ful case, we must wait for all
                        // OnApplicationPoolAllQueueInstancesStopped callbacks to fire before we can stop
                        cleanupComplete = new AutoResetEvent(false);

                        if (!Cleanup(true))
                        {
                            // CSDMain 190118
                            // When the Cleanup(true) returns false, it means we are requesting WAS to close all the existing worker
                            // process and give us a callback. So the current thread is blocked here.
                            // In two cases the thread can be unblocked:
                            // 1. onApplicationPoolAllQueueInstancesStopped function called. That means WAS finished its work and send
                            // us the notification. This is the normal case.
                            // 2. onConfigManagerDisconnected function called. That means there's something wrong with WAS and we should
                            // not wait for the onApplicationPoolAllQueueInstancesStopped call anymore.
                            Interlocked.Exchange(ref closingProcessStatus, ClosingProcessBlocked);
                            cleanupComplete.WaitOne(ListenerConstants.ServiceStopTimeout, false);
                            Interlocked.Exchange(ref closingProcessStatus, ClosingProcessUnBlocked);
                        }

                        // base.Close causes WebhostUnregisterProtocol to be called.
                        base.Close();
                    }
                }
            }
        }

        bool Cleanup(bool closeInstances)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::Cleanup()");
            canDispatch = false;
            bool completeSelf = true;

            if (closeInstances)
            {
                List<App> existingApps = new List<App>();
                List<App> removeApps = new List<App>();
                List<App> delayRemoveApps = new List<App>();
                lock (appManager)
                {
                    if (appManager.AppsCount != 0)
                    {
                        // cleanup for activation service stop: tell WAS about it
                        existingApps.AddRange(appManager.Apps.Values);
                        foreach (App app in existingApps)
                        {
                            if (app.MessageQueue.HasStartedQueueInstances)
                            {
                                delayRemoveApps.Add(app);
                            }
                            else
                            {
                                removeApps.Add(app);
                            }
                        }

                        existingApps.Clear();
                    }
                }

                if (removeApps.Count != 0)
                {
                    foreach (App app in removeApps)
                    {
                        RemoveApp(app);
                    }
                }

                if (delayRemoveApps.Count != 0)
                {
                    foreach (App app in delayRemoveApps)
                    {
                        if (app.PendingAction != null)
                        {
                            app.PendingAction.MergeFromDeletedAction();
                        }
                        else
                        {
                            // Create a new action
                            app.SetPendingAction(AppAction.CreateDeletedAction());
                            CloseAllListenerChannelInstances(app);
                        }
                    }

                    completeSelf = false;
                }
            }
            else
            {
                lock (appManager)
                {
                    appManager.Clear();
                }
            }

            return completeSelf;
        }

        void CloseAllListenerChannelInstances(App app)
        {
            int hresult = CloseAllListenerChannelInstances(app.AppPool.AppPoolId, app.MessageQueue.ListenerChannelContext.ListenerChannelId);
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::CloseAllListenerChannelInstances(" + app.AppKey + ") returned: " + hresult);
            if (hresult == 0)
            {
                if (DiagnosticUtility.ShouldTraceInformation)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.WasCloseAllListenerChannelInstances, SR.GetString(SR.TraceCodeWasCloseAllListenerChannelInstances), this);
                }
                if (TD.WasCloseAllListenerChannelInstancesCompletedIsEnabled())
                {
                    //WasWebHostAPI
                    TD.WasCloseAllListenerChannelInstancesCompleted(this.EventTraceActivity);
                }
            }
            else
            {
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.WasWebHostAPIFailed, SR.GetString(SR.TraceCodeWasWebHostAPIFailed),
                        new StringTraceRecord("HRESULT", SR.GetString(SR.TraceCodeWasWebHostAPIFailed,
                        "WebhostCloseAllListenerChannelInstances", hresult.ToString(CultureInfo.CurrentCulture))), this, null);
                }
                if (TD.WasCloseAllListenerChannelInstancesFailedIsEnabled())
                {
                    //WasWebHostAPIFailed
                    TD.WasCloseAllListenerChannelInstancesFailed(this.EventTraceActivity, hresult.ToString(CultureInfo.CurrentCulture));
                }
            }
        }

        protected override void OnApplicationAppPoolChanged(string appKey, string appPoolId)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationAppPoolChanged(" + appKey + ", " + appPoolId + ")");

            try
            {
                App app = null;
                lock (appManager)
                {
                    // The app might have been removed due to the service shutdown.
                    if (!appManager.Apps.TryGetValue(appKey, out app))
                    {
                        return;
                    }
                }

                if (app.PendingAction != null)
                {
                    app.PendingAction.MergeFromAppPoolChangedAction(appPoolId);
                }
                else
                {
                    if (app.MessageQueue.HasStartedQueueInstances)
                    {
                        // Create a new action
                        app.SetPendingAction(AppAction.CreateAppPoolChangedAction(appPoolId));
                        ScheduleClosingListenerChannelInstances(app);
                    }
                    else
                    {
                        CompleteAppPoolChange(app, appPoolId);
                    }
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        internal void CompleteAppPoolChange(App app, string appPoolId)
        {
            lock (appManager)
            {
                AppPool appPool;
                if (!appManager.AppPools.TryGetValue(appPoolId, out appPool))
                {
                    // The AppPool has been removed
                    return;
                }

                if (appPool != app.AppPool)
                {
                    app.AppPool.RemoveApp(app);
                    appPool.AddApp(app);
                    app.OnAppPoolChanged(appPool);
                }
            }
        }

        protected override void OnApplicationDeleted(string appKey)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationDeleted(" + appKey + ")");

            try
            {
                App app = null;
                lock (appManager)
                {
                    // CSDMain 190118
                    // In some cases WAS will send us duplicated notification for the deletion of a same appKey
                    if (!appManager.Apps.ContainsKey(appKey))
                    {
                        return;
                    }
                    app = appManager.Apps[appKey];
                }

                if (app.PendingAction != null)
                {
                    app.PendingAction.MergeFromDeletedAction();
                }
                else
                {
                    if (app.MessageQueue.HasStartedQueueInstances)
                    {
                        // Creae a new action
                        app.SetPendingAction(AppAction.CreateDeletedAction());
                        ScheduleClosingListenerChannelInstances(app);
                    }
                    else
                    {
                        CompleteDeleteApp(app);
                    }
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        internal void CompleteDeleteApp(App app)
        {
            RemoveApp(app);
        }

        void CompleteAppSettingsChanged(App app)
        {
            if (app.PendingAction.AppPoolId != null)
            {
                CompleteAppPoolChange(app, app.PendingAction.AppPoolId);
            }

            if (app.PendingAction.Path != null)
            {
                app.Path = app.PendingAction.Path;
            }

            if (app.PendingAction.Bindings != null)
            {
                RegisterNewBindings(app, app.PendingAction.Bindings);
            }

            if (app.PendingAction.RequestsBlocked.HasValue)
            {
                app.SetRequestBlocked(app.PendingAction.RequestsBlocked.Value);
            }
        }

        void RemoveApp(App app)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::RemoveApp(" + app.AppKey + ")");

            lock (appManager)
            {
                // The App might have been deleted when the AppPool is deleted.
                if (appManager.Apps.ContainsKey(app.AppKey))
                {
                    appManager.DeleteApp(app, false);
                }
            }
        }

        protected override void OnApplicationBindingsChanged(string appKey, IntPtr bindingsMultiSz, int numberOfBindings)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationBindingsChanged(" + appKey + ")");
            string[] bindings = null;
            try
            {
                bindings = base.ParseBindings(bindingsMultiSz, numberOfBindings);
            }
            catch (ArgumentException exception)
            {
                DiagnosticUtility.TraceHandledException(exception, TraceEventType.Error);

                // Ignore the binding change if WAS provides wrong bindings.
                return;
            }

            App app = null;
            lock (appManager)
            {
                // The app might have been removed due to the service shutdown.
                if (!appManager.Apps.TryGetValue(appKey, out app))
                {
                    return;
                }
            }

            try
            {
                if (app.PendingAction != null)
                {
                    app.PendingAction.MergeFromBindingChangedAction(bindings);
                }
                else
                {
                    if (app.MessageQueue.HasStartedQueueInstances)
                    {
                        app.SetPendingAction(AppAction.CreateBindingsChangedAction(bindings));
                        ScheduleClosingListenerChannelInstances(app);
                    }
                    else
                    {
                        RegisterNewBindings(app, bindings);
                    }
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        internal void RegisterNewBindings(App app, string[] bindings)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::RegisterNewBindings(" + app.AppKey + ")");
            // we could be smart-er and leave the bindings that have not changed alone

            app.MessageQueue.UnregisterAll();

            bool success = RegisterBindings(app.MessageQueue, app.SiteId, bindings, app.Path);
            app.OnInvalidBinding(!success);
        }

        protected override void OnApplicationCreated(string appKey, string path, int siteId, string appPoolId, IntPtr bindingsMultiSz, int numberOfBindings, bool requestsBlocked)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationCreated(" + appKey + ", " + path + ", " + siteId + ", " + appPoolId + ", " + requestsBlocked + ")");
            string[] bindings = null;
            try
            {
                bindings = base.ParseBindings(bindingsMultiSz, numberOfBindings);
            }
            catch (ArgumentException exception)
            {
                DiagnosticUtility.TraceHandledException(exception, TraceEventType.Error);

                // Ignore the app if WAS provides wrong bindings.
                return;
            }

            try
            {
                bool found = true;
                App app = null;

                lock (appManager)
                {
                    if (!appManager.Apps.TryGetValue(appKey, out app))
                    {
                        found = false;
                        app = appManager.CreateApp(appKey, path, siteId, appPoolId, requestsBlocked);
                    }
                }

                if (found)
                {
                    Fx.Assert(app.PendingAction != null, "The app should be waiting for AllLCStopped notification.");
                    app.PendingAction.MergeFromCreatedAction(path, siteId, appPoolId, requestsBlocked, bindings);
                }
                else
                {
                    CompleteAppCreation(app, bindings);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        void CompleteAppCreation(App app, string[] bindings)
        {
            IActivatedMessageQueue queue = activationService.CreateQueue(this, app);
            app.RegisterQueue(queue);

            bool success = RegisterBindings(app.MessageQueue, app.SiteId, bindings, app.Path);
            app.OnInvalidBinding(!success);
        }

        bool RegisterBindings(IActivatedMessageQueue queue, int siteId, string[] bindings, string path)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::RegisterBindings() bindings#: " + bindings.Length);
            BaseUriWithWildcard[] baseAddresses = new BaseUriWithWildcard[bindings.Length];
            // first make sure all the bindings are valid for this protocol
            for (int i = 0; i < bindings.Length; i++)
            {
                string binding = bindings[i];
                int index = binding.IndexOf(':');
                string protocol = binding.Substring(0, index);
                if (string.Compare(this.ProtocolName, protocol, StringComparison.OrdinalIgnoreCase) != 0)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidOperationException(
                        SR.GetString(SR.LAProtocolMismatch, protocol, path, this.ProtocolName)));
                }

                binding = binding.Substring(index + 1);
                try
                {
                    baseAddresses[i] = BaseUriWithWildcard.CreateHostedUri(ProtocolName, binding, path);
                    Debug.Print("ListenerAdapter[" + ProtocolName + "]::RegisterBindings() CreateUrlFromBinding(binding: " + binding + " path: " + path + ") returned baseAddress: " + baseAddresses[i]);
                }
                catch (UriFormatException exception)
                {
                    Debug.Print("ListenerAdapter[" + ProtocolName + "]::RegisterBindings() CreateUrlFromBinding(binding: " + binding + " path: " + path + ") failed with UriFormatException: " + exception.Message);
                    DiagnosticUtility.TraceHandledException(exception, TraceEventType.Error);

                    // We only log the event for the site root.
                    if (string.Compare(path, SiteRootPath, StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        DiagnosticUtility.EventLog.LogEvent(TraceEventType.Error,
                            (ushort)EventLogCategory.ListenerAdapter,
                            (uint)EventLogEventId.BindingError,
                            protocol,
                            binding,
                            siteId.ToString(NumberFormatInfo.CurrentInfo),
                            bindings[i],
                            ListenerTraceUtility.CreateSourceString(this),
                            exception.ToString());
                    }

                    return false;
                }
            }

            // now make sure all the bindings can be listened on or roll back
            for (int i = 0; i < bindings.Length; i++)
            {
                ListenerExceptionStatus status = ListenerExceptionStatus.FailedToListen;
                Exception exception = null;
                try
                {
                    status = queue.Register(baseAddresses[i]);
                    Debug.Print("ListenerAdapter[" + ProtocolName + "]::RegisterBindings() registering baseAddress: " + baseAddresses[i] + " with queue returned: " + status);
                }
                catch (Exception ex)
                {
                    if (Fx.IsFatal(ex))
                    {
                        throw;
                    }

                    DiagnosticUtility.TraceHandledException(exception, TraceEventType.Error);

                    exception = ex;
                }

                if (status != ListenerExceptionStatus.Success)
                {
                    // We only log the event for the site root.
                    if (string.Compare(path, SiteRootPath, StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        DiagnosticUtility.EventLog.LogEvent(TraceEventType.Error,
                            (ushort)EventLogCategory.ListenerAdapter,
                            (uint)EventLogEventId.LAFailedToListenForApp,
                            activationService.ActivationServiceName,
                            ProtocolName,
                            siteId.ToString(NumberFormatInfo.CurrentInfo),
                            baseAddresses[i].ToString(),
                            status.ToString(),
                            exception == null ? string.Empty : exception.ToString());
                    }

                    queue.UnregisterAll();
                    return false;
                }
            }

            return true;
        }

        protected override void OnApplicationPoolAllQueueInstancesStopped(string appPoolId, int queueId)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationPoolAllQueueInstancesStopped(" + appPoolId + ", " + queueId + ")");

            if (!isOpen)
            {
                if (closingProcessStatus != ClosingProcessBlocked)
                {
                    // OnConfigManagerDisconnected was already called. We should just return.
                    return;
                }
            }

            try
            {
                AppPool appPool = null;
                lock (appManager)
                {
                    if (!appManager.AppPools.TryGetValue(appPoolId, out appPool))
                    {
                        // Ignore this notification if we received OnApplicationPoolDeleted.                
                        return;
                    }
                }

                IActivatedMessageQueue queue = activationService.FindQueue(queueId);
                if (queue == null)
                {
                    // This is the belated notification. Ignore it.
                    return;
                }

                Fx.Assert(queue.App.AppPool.AppPoolId == appPoolId, "OnApplicationPoolAllQueueInstancesStopped: unexpected pool id");
                queue.OnQueueInstancesStopped();

                App app = queue.App;

                try
                {
                    if (app.PendingAction.ActionType == AppActionType.Deleted)
                    {
                        CompleteDeleteApp(app);
                        SignalCleanupForNoApps();
                    }
                    else if (app.PendingAction.ActionType == AppActionType.SettingsChanged)
                    {
                        CompleteAppSettingsChanged(app);
                    }
                }
                finally
                {
                    // Reset the action
                    app.SetPendingAction(null);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnApplicationPoolCanLaunchQueueInstance(string appPoolId, int queueId)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationPoolCanLaunchQueueInstance(" + appPoolId + ", " + queueId + ")");

            try
            {
                IActivatedMessageQueue queue = activationService.FindQueue(queueId);
                if (queue != null)
                {
                    if (queue.App.AppPool.AppPoolId != appPoolId)
                    {
                        throw Fx.AssertAndThrow("OnApplicationPoolCanLaunchQueueInstance: unexpected pool id");
                    }

                    ScheduleLaunchingQueueInstance(queue);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnApplicationPoolCreated(string appPoolId, SecurityIdentifier sid)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationPoolCreated(" + appPoolId + ", " + sid + ")");

            try
            {
                lock (appManager)
                {
                    appManager.CreateAppPool(appPoolId, sid);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnApplicationPoolDeleted(string appPoolId)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationPoolDeleted(" + appPoolId + ")");

            try
            {
                lock (appManager)
                {
                    appManager.DeleteAppPool(appPoolId);
                }

                SignalCleanupForNoApps();
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnApplicationPoolIdentityChanged(string appPoolId, SecurityIdentifier sid)
        {
            try
            {
                Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationPoolIdentityChanged(" + appPoolId + ", " + sid + ")");

                AppPool appPool = null;
                lock (appManager)
                {
                    if (!appManager.AppPools.TryGetValue(appPoolId, out appPool))
                    {
                        return;
                    }

                    appPool.SetIdentity(sid);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnApplicationPoolStateChanged(string appPoolId, bool isEnabled)
        {
            try
            {
                Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationPoolStateChanged(" + appPoolId + ", " + isEnabled + ")");
                AppPool appPool = null;
                lock (appManager)
                {
                    if (!appManager.AppPools.TryGetValue(appPoolId, out appPool))
                    {
                        return;
                    }

                    appPool.SetEnabledState(isEnabled);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnApplicationRequestsBlockedChanged(string appKey, bool requestsBlocked)
        {
            try
            {
                Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnApplicationRequestsBlockedChanged(" + appKey + ", " + requestsBlocked + ")");
                App app = null;

                lock (appManager)
                {
                    if (!appManager.Apps.TryGetValue(appKey, out app))
                    {
                        return;
                    }
                }

                if (app.PendingAction != null)
                {
                    app.PendingAction.MergeFromRequestsBlockedAction(requestsBlocked);
                }
                else
                {
                    app.SetRequestBlocked(requestsBlocked);
                }
            }
            catch (Exception exception)
            {
                HandleUnknownError(exception);
            }
        }

        protected override void OnConfigManagerConnected()
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnConfigManagerConnected()");

            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.WasConnected, SR.GetString(SR.TraceCodeWasConnected), this);
            }
            if (TD.WasConnectedIsEnabled())
            {
                TD.WasConnected(this.EventTraceActivity);
            }

            if (wasConnected != null)
            {
                wasConnected.Set();
            }
        }

        protected override void OnConfigManagerDisconnected(int hresult)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnConfigManagerDisconnected(" + hresult + ") isOpen: " + isOpen);
            if (!isOpen)
            {
                int currentClosingProcessStatus = Interlocked.CompareExchange(ref closingProcessStatus, 
                                                                    ClosingProcessUnBlockedByEventOnConfigManagerDisconnected,
                                                                    ClosingProcessBlocked);
                if (currentClosingProcessStatus == ClosingProcessBlocked)
                {
                    // CSDMain 190118
                    // According to WAS team, if we receive this call before the call "OnApplicationPoolAllQueueInstancesStopped",
                    // then we will not receive the call "OnApplicationPoolAllQueueInstancesStopped" ever. So in this case we should
                    // do cleanup on our side directly.
                    Cleanup(false);
                    SignalCleanupForNoApps();
                }
                return;
            }

            if (TD.WasDisconnectedIsEnabled())
            {
                TD.WasDisconnected(this.EventTraceActivity);
            }

            DiagnosticUtility.EventLog.LogEvent(TraceEventType.Warning,
                (ushort)EventLogCategory.ListenerAdapter,
                (uint)EventLogEventId.WasDisconnected,
                hresult.ToString(CultureInfo.InvariantCulture));

            // WAS has crashed.
            Cleanup(false);
            wasConnected = new AutoResetEvent(false);
            ThreadPool.UnsafeRegisterWaitForSingleObject(wasConnected, Fx.ThunkCallback(new WaitOrTimerCallback(WasConnected)), null, ListenerConstants.WasConnectTimeout, true);
        }

        void WasConnected(object state, bool timedOut)
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::WasConnected() timedOut: " + timedOut);
            if (timedOut)
            {
                // WAS didn't connect within the timeout give up waiting and stop
                DiagnosticUtility.EventLog.LogEvent(TraceEventType.Warning,
                    (ushort)EventLogCategory.ListenerAdapter,
                    (uint)EventLogEventId.WasConnectionTimedout);
                
                if (TD.WasConnectionTimedoutIsEnabled())
                {
                    TD.WasConnectionTimedout(this.EventTraceActivity);
                }

                activationService.StopService();

            }
            wasConnected.Close();
            wasConnected = null;
        }

        protected override void OnConfigManagerInitializationCompleted()
        {
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::OnConfigManagerInitializationCompleted()");
            canDispatch = true;

            ManualResetEvent initCompleted = this.initCompleted;
            if (initCompleted != null)
            {
                initCompleted.Set();
            }
        }

        internal bool OpenListenerChannelInstance(IActivatedMessageQueue queue)
        {
            byte[] queueBlob = queue.ListenerChannelContext.Dehydrate();
            Debug.Print("ListenerAdapter[" + ProtocolName + "]::ListenerAdapter.OpenListenerChannelInstance(appPoolId:" + queue.App.AppPool.AppPoolId + " appKey:" + queue.ListenerChannelContext.AppKey + " queueId:" + queue.ListenerChannelContext.ListenerChannelId + ")");
            int hresult = OpenListenerChannelInstance(queue.App.AppPool.AppPoolId, queue.ListenerChannelContext.ListenerChannelId, queueBlob);
            if (hresult != 0)
            {
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.WasWebHostAPIFailed, SR.GetString(SR.TraceCodeWasWebHostAPIFailed),
                        new StringTraceRecord("HRESULT", SR.GetString(SR.TraceCodeWasWebHostAPIFailed,
                        "WebhostOpenListenerChannelInstance", hresult.ToString(CultureInfo.CurrentCulture))), this, null);
                }
                if (TD.OpenListenerChannelInstanceFailedIsEnabled())
                {
                    TD.OpenListenerChannelInstanceFailed(this.EventTraceActivity, hresult.ToString(CultureInfo.CurrentCulture));
                }

                return false;
            }

            return true;
        }

        void ScheduleClosingListenerChannelInstances(App app)
        {
            if (closeAllListenerChannelInstancesCallback == null)
            {
                closeAllListenerChannelInstancesCallback = new Action<object>(OnCloseAllListenerChannelInstances);
            }

            ActionItem.Schedule(closeAllListenerChannelInstancesCallback, app);
        }

        void OnCloseAllListenerChannelInstances(object state)
        {
            App app = state as App;
            Fx.Assert(app != null, "OnCloseAllListenerChannelInstances: app is null");

            CloseAllListenerChannelInstances(app);
        }

        void ScheduleLaunchingQueueInstance(IActivatedMessageQueue queue)
        {
            if (launchQueueInstanceCallback == null)
            {
                launchQueueInstanceCallback = new Action<object>(OnLaunchQueueInstance);
            }

            ActionItem.Schedule(launchQueueInstanceCallback, queue);
        }

        void OnLaunchQueueInstance(object state)
        {
            IActivatedMessageQueue queue = state as IActivatedMessageQueue;
            Fx.Assert(queue != null, "OnLaunchQueueInstance: queue is null");

            queue.LaunchQueueInstance();
        }

        void HandleUnknownError(Exception exception)
        {
            DiagnosticUtility.EventLog.LogEvent(TraceEventType.Error,
                (ushort)EventLogCategory.ListenerAdapter,
                (uint)EventLogEventId.UnknownListenerAdapterError,
                this.ProtocolName,
                exception.ToString());

            if (TD.FailFastExceptionIsEnabled())
            {
                TD.FailFastException(this.EventTraceActivity, exception);
            }
            // We cannot handle this exception and thus have to terminate the process.
            DiagnosticUtility.InvokeFinalHandler(exception);
        }

        void SignalCleanupForNoApps()
        {
            if (this.cleanupComplete != null)
            {
                lock (appManager)
                {
                    if (appManager.AppsCount == 0)
                    {
                        // on g----ful cleanup we need to unblock Close() when no app is left running
                        this.appManager.Clear();
                        this.cleanupComplete.Set();
                    }
                }
            }
        }
    }
}
