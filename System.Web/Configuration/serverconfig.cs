//------------------------------------------------------------------------------
// <copyright file="ServerConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Web.Configuration {

    using System.Configuration;
    using System.Collections;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using System.Web.Util;
    using System.Web.Hosting;
    using System.Web.Caching;
    using System.Web.Compilation;
    using Microsoft.Win32;

    //
    // Abstracts differences between config retreived from IIS 6 metabase
    // and config retreived from new IIS7 configuration system.
    //
    static internal class ServerConfig {
        
        static object s_expressConfigsLock = new object();

        // used in the default domain only, by the ClientBuildManager
        static Dictionary<string, ExpressServerConfig> s_expressConfigs;

        static string s_iisExpressVersion;

        // used in non-default domains only, by the ClientBuildManager
        internal static string IISExpressVersion {
            get {
                return s_iisExpressVersion;
            }
            set {
                if (Thread.GetDomain().IsDefaultAppDomain() || (s_iisExpressVersion != null && s_iisExpressVersion != value ))
                    throw new InvalidOperationException();
                s_iisExpressVersion = value;
            }
        }

        internal static bool UseMetabase {
            get {                
                // Devdiv #463596 - The current version of .net framework is only for win7 and above,
                // so we should not use metabase at all
                return false;
            }
        }

        static internal IServerConfig GetInstance() {
            // IIS 7 bits on <= IIS 6: use the metabase
            if (UseMetabase) {
                return MetabaseServerConfig.GetInstance();
            }
            if (IISExpressVersion == null) {
                return ProcessHostServerConfig.GetInstance();
            }
            return ExpressServerConfig.GetInstance(IISExpressVersion);
        }

        static internal IServerConfig GetDefaultDomainInstance(string version) {
            if (version == null) {
                return GetInstance();
            }
            ExpressServerConfig expressConfig = null;
            lock (s_expressConfigsLock) {
                if (s_expressConfigs == null) {
                    if (!Thread.GetDomain().IsDefaultAppDomain()) {
                        throw new InvalidOperationException();
                    }
                    s_expressConfigs = new Dictionary<string, ExpressServerConfig>(3);
                }
                if (!s_expressConfigs.TryGetValue(version, out expressConfig)) {
                    expressConfig = new ExpressServerConfig(version);
                    s_expressConfigs[version] = expressConfig;
                }
            }
            return expressConfig;
        }

        //
        // Return true in cases where web server configuration should be used
        // to resolve paths.
        //
        static int s_useServerConfig = -1;
        static internal bool UseServerConfig {
            get {
                if (s_useServerConfig == -1) {
                    int useServerConfig = 0;
                    // Must use web server config if there is no hosting environment
                    if (!HostingEnvironment.IsHosted) {
                        useServerConfig = 1;
                    }
                    // Hosting environment is the web server
                    else if (HostingEnvironment.ApplicationHostInternal is ISAPIApplicationHost) {
                        useServerConfig = 1;
                    }
                    // Hosting environment is the web server
                    else if (HostingEnvironment.IsUnderIISProcess && !BuildManagerHost.InClientBuildManager) {
                        useServerConfig = 1;
                    }
                    Interlocked.CompareExchange(ref s_useServerConfig, useServerConfig, -1);
                }

                return s_useServerConfig == 1;
            }
        }

    }
}
