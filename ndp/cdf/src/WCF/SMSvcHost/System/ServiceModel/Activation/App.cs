//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.Diagnostics;
    using System.Runtime;

    class App
    {
        string appKey;
        AppPool appPool;
        int siteId;
        IActivatedMessageQueue messageQueue;
        string path;
        bool requestBlocked;
        bool hasInvalidBinding;
        AppAction pendingAction;

        internal App(string appKey, string path, int siteId, AppPool appPool, bool requestsBlocked)
            : base()
        {
            Debug.Print("App.ctor(appKey:" + appKey + " path:" + path + " appPoolId:" + appPool.AppPoolId + ")");

            this.appKey = appKey;
            this.path = path;
            this.appPool = appPool;
            this.siteId = siteId;
            this.requestBlocked = requestsBlocked;
        }

        internal AppAction PendingAction
        {
            get
            {
                return this.pendingAction;
            }
        }

        internal void SetPendingAction(AppAction action)
        {
            if (action != null)
            {
                Fx.Assert(this.pendingAction == null, "There is already a pending action.");
            }

            this.pendingAction = action;
        }

        internal void RegisterQueue(IActivatedMessageQueue messageQueue)
        {
            if (this.messageQueue != null)
            {
                throw Fx.AssertAndThrow("a message queue was already registered");
            }
            this.messageQueue = messageQueue;
        }

        internal string AppKey { get { return appKey; } }
        internal AppPool AppPool { get { return appPool; } }
        internal int SiteId { get { return siteId; } }
        internal IActivatedMessageQueue MessageQueue { get { return messageQueue; } }
        internal string Path
        {
            get
            {
                return path;
            }

            set
            {
                this.path = value;
            }
        }

        internal void OnAppPoolChanged(AppPool newAppPool)
        {
            this.appPool = newAppPool;
        }

        internal void SetRequestBlocked(bool requestBlocked)
        {
            if (this.requestBlocked != requestBlocked)
            {
                this.requestBlocked = requestBlocked;
                OnStateChanged();
            }
        }

        internal void OnAppPoolStateChanged()
        {
            OnStateChanged();
        }

        internal void OnDeleted(bool appPoolDeleted)
        {
            messageQueue.Delete();
        }

        internal bool IsEnabled
        {
            get
            {
                return this.appPool.IsEnabled && !this.requestBlocked && !this.hasInvalidBinding;
            }
        }

        internal void OnInvalidBinding(bool hasInvalidBinding)
        {
            this.hasInvalidBinding = hasInvalidBinding;
            OnStateChanged();
        }

        void OnStateChanged()
        {
            messageQueue.SetEnabledState(this.IsEnabled);
        }
    }
}
