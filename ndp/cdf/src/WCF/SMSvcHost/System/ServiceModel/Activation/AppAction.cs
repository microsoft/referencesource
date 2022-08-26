//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime;

    enum AppActionType
    {
        // An App is deleted
        Deleted,

        // Binding, or AppPool, or RequestsBlocked is changed
        SettingsChanged
    }

    class AppAction
    {
        AppActionType actionType;
        string path;
        string appPoolId;
        Nullable<bool> requestsBlocked;
        string[] bindings;

        AppAction(AppActionType actionType)
        {
            this.actionType = actionType;
        }

        public static AppAction CreateDeletedAction()
        {
            return new AppAction(AppActionType.Deleted);
        }

        public static AppAction CreateBindingsChangedAction(string[] bindings)
        {
            AppAction action = new AppAction(AppActionType.SettingsChanged);
            action.bindings = bindings;
            return action;
        }

        public static AppAction CreateAppPoolChangedAction(string appPoolId)
        {
            AppAction action = new AppAction(AppActionType.SettingsChanged);
            action.appPoolId = appPoolId;
            return action;
        }

        public AppActionType ActionType
        {
            get
            {
                return this.actionType;
            }
        }

        public string Path
        {
            get
            {
                return this.path;
            }
        }

        public string AppPoolId
        {
            get
            {
                return this.appPoolId;
            }
        }

        public string[] Bindings
        {
            get
            {
                return this.bindings;
            }
        }

        public Nullable<bool> RequestsBlocked
        {
            get
            {
                return this.requestsBlocked;
            }
        }

        public void MergeFromCreatedAction(string path, int siteId, string appPoolId, bool requestsBlocked, string[] bindings)
        {
            Fx.Assert(this.ActionType == AppActionType.Deleted, "We should get ApplicationCreated notification only when the App is to be deleted.");

            // Delete + Created = SettingsChanged
            this.actionType = AppActionType.SettingsChanged;
            SetSettings(path, appPoolId, requestsBlocked, bindings);

            // SiteId is ignored because the siteId can't be changed for the same appKey.
        }

        public void MergeFromDeletedAction()
        {
            Fx.Assert(this.ActionType == AppActionType.SettingsChanged,
                "We should not get two consecutive ApplicationDeleted notifications.");

            this.actionType = AppActionType.Deleted;
        }

        public void MergeFromBindingChangedAction(string[] bindings)
        {
            Fx.Assert(this.ActionType == AppActionType.SettingsChanged,
                "We should not get two consecutive ApplicationDeleted notifications.");

            this.bindings = bindings;
        }

        public void MergeFromAppPoolChangedAction(string appPoolId)
        {
            Fx.Assert(this.ActionType == AppActionType.SettingsChanged,
                "We should not get two consecutive ApplicationDeleted notifications.");

            this.appPoolId = appPoolId;
        }

        public void MergeFromRequestsBlockedAction(bool requestsBlocked)
        {
            Fx.Assert(this.ActionType == AppActionType.SettingsChanged,
                "We should not get two consecutive ApplicationDeleted notifications.");

            this.requestsBlocked = requestsBlocked;
        }

        void SetSettings(string path, string appPoolId, bool requestsBlocked, string[] bindings)
        {
            this.path = path;
            this.appPoolId = appPoolId;
            this.requestsBlocked = requestsBlocked;
            this.bindings = bindings;
        }
    }
}
