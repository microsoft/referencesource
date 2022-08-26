//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Security.Principal;
    using System.Diagnostics;
    using System.ServiceModel;

    // NOTE: This class is not thread-safe. The caller should perform synchronization.
    class AppManager
    {
        Dictionary<string, App> apps;
        Dictionary<string, AppPool> pools;

        public AppManager()
        {
            this.apps = new Dictionary<string, App>();
            this.pools = new Dictionary<string, AppPool>();
        }

        public int AppsCount
        {
            get
            {
                return this.apps.Count;
            }
        }

        public Dictionary<string, App> Apps
        {
            get
            {
                return this.apps;
            }
        }

        public Dictionary<string, AppPool> AppPools
        {
            get
            {
                return this.pools;
            }
        }

        public void CreateAppPool(string appPoolId, SecurityIdentifier sid)
        {
            AppPool appPool = new AppPool(appPoolId, false, sid);
            this.pools.Add(appPoolId, appPool);
        }

        public App CreateApp(string appKey, string path, int siteId, string appPoolId, bool requestsBlocked)
        {
            AppPool appPool = this.AppPools[appPoolId];
            App app = new App(appKey, path, siteId, appPool, requestsBlocked);
            this.apps.Add(appKey, app);
            appPool.AddApp(app);

            return app;
        }

        public void DeleteAppPool(string appPoolId)
        {
            AppPool pool;
            if (this.pools.TryGetValue(appPoolId, out pool))
            {
                if (pool != null)
                {
                    foreach (App app in pool.SnapshotApps())
                    {
                        DeleteApp(app, true);
                    }

                    pools.Remove(appPoolId);
                    pool.OnDeleted();
                }
            }
        }

        public void DeleteApp(App app, bool appPoolDeleted)
        {
            app.AppPool.RemoveApp(app);
            apps.Remove(app.AppKey);
            app.OnDeleted(appPoolDeleted);
        }

        public void Clear()
        {
            foreach (App app in apps.Values)
            {
                app.OnDeleted(false);
            }

            apps.Clear();
            pools.Clear();
        }
    }
}

