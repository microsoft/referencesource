//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.Collections.Generic;
    using System.Runtime;
    using System.Security.Principal;

    class AppPool
    {
        string appPoolId;
        List<App> apps;
        bool enabled;
        SecurityIdentifier securityIdentifier;

        internal AppPool(string appPoolId, bool enabled, SecurityIdentifier securityIdentifier)
        {
            this.apps = new List<App>();
            this.appPoolId = appPoolId;
            this.enabled = enabled;
            this.securityIdentifier = securityIdentifier;
        }

        internal string AppPoolId { get { return appPoolId; } }
        internal bool Enabled { get { return enabled; } }

        internal void AddApp(App app)
        {
            lock (this.apps)
            {
                this.apps.Add(app);
            }
        }

        internal void RemoveApp(App app)
        {
            lock (this.apps)
            {
                this.apps.Remove(app);
            }
        }

        internal IEnumerable<App> SnapshotApps()
        {
            lock (this.apps)
            {
                return new List<App>(this.apps);
            }
        }

        internal void OnDeleted()
        {
            // We should have removed all apps.
            Fx.Assert(apps.Count == 0, "");
            this.enabled = false;
        }

        internal void SetEnabledState(bool enabled)
        {
            if (this.enabled != enabled)
            {
                this.enabled = enabled;

                foreach (App app in apps)
                {
                    app.OnAppPoolStateChanged();
                }
            }
        }

        internal bool IsEnabled
        {
            get
            {
                return this.enabled;
            }
        }

        internal void SetIdentity(SecurityIdentifier securityIdentifier)
        {
            this.securityIdentifier = securityIdentifier;
        }
    }

}

