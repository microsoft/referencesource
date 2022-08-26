// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace System.Workflow.Runtime
{
    using System;
    using System.Collections.Specialized;
    using System.Configuration;

    internal static class AppSettings
    {
        private static volatile bool settingsInitialized = false;
        private static object appSettingsLock = new object();

        // false [default] to use P/Invoke of Windows routines for MD5 hashing.
        // true to use the MD5CryptoServiceProvider, which requires that FIPS be disabled.
        // <add key="microsoft:WorkflowRuntime:FIPSRequired" value="true"/>
        private static bool fipsRequired;

        internal static bool FIPSRequired
        {
            get
            {
                EnsureSettingsLoaded();
                return fipsRequired;
            }
        }

        private static void EnsureSettingsLoaded()
        {
            if (!settingsInitialized)
            {
                lock (appSettingsLock)
                {
                    if (!settingsInitialized)
                    {
                        NameValueCollection settings = null;

                        try
                        {
                            settings = ConfigurationManager.AppSettings;
                        }
                        finally
                        {
                            if (settings == null || !bool.TryParse(settings["microsoft:WorkflowRuntime:FIPSRequired"], out fipsRequired))
                            {
                                fipsRequired = false;
                            }

                            settingsInitialized = true;
                        }
                    }
                }
            }
        }
    }
}
