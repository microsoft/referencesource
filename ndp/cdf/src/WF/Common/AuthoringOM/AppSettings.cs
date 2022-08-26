// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace System.Workflow.ComponentModel
{
    using System;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Workflow.Interop;

    internal static class AppSettings
    {
        private const int DefaultXOMLMaximumNestedObjectDepth = 300;

        private static volatile bool settingsInitialized = false;
        private static object appSettingsLock = new object();

        // false [default] to check for x:Code usage in disabled activities.
        // true to NOT check for x:Code usage in disabled activities (old behavior).
        // <add key="microsoft:WorkflowComponentModel:XOMLAllowXCodeInDisabledActivities" value="true"/>
        private static bool allowXCode;

        // The maximum depth of nested objects allowed during XOML serialization/deserialization
        // Default value is DefaultXOMLMaximumNestedObjectDepth.
        // <add key="microsoft:WorkflowComponentModel:XOMLMaximumNestedObjectDepth" value="100"/>
        private static int xomlMaximumNestedObjectDepth;

        // false [default] to verify ActivityBind or DependencyObject types in ActivitySurrogateSelector
        // true to NOT verify the types in the ActivitySurrogateSelector
        // <add key="microsoft:WorkflowComponentModel:DisableActivitySurrogateSelectorTypeCheck" value="true"/>
        private static bool disableActivitySurrogateSelectorTypeCheck;

        // false [default] to include "default" UNauthorized types for XOML Serializer the verification
        // true to NOT include the "default" UNauthorized types
        // <add key="microsoft:WorkflowComponentModel:DisableXOMLSerializerDefaultUnauthorizedTypes" value="true"/>
        private static bool disableXOMLSerializerDefaultUnauthorizedTypes;

        // false [default] to perform authorized type checking in XOML Serializer
        // true to disable type checking in XOML Serializer
        // <add key="microsoft:WorkflowComponentModel:DisableXOMLSerializerTypeChecking" value="true"/>
        private static bool disableXOMLSerializerTypeChecking;

        internal static bool AllowXCode
        {
            get
            {
                EnsureSettingsLoaded();
                return allowXCode;
            }
        }

        internal static int XOMLMaximumNestedObjectDepth
        {
            get
            {
                EnsureSettingsLoaded();
                return xomlMaximumNestedObjectDepth;
            }
        }

        internal static bool DisableActivitySurrogateSelectorTypeCheck
        {
            get
            {
                // If DynamicCodePolicy is enabled, we are on a "device guard" system, so
                // don't allow the ActivitySurrogateSelectorTypeCheck to be disabled. Don't
                // even bother to check for the opt-out AppSetting.
                if (NativeMethods.IsDynamicCodePolicyEnabled())
                {
                    return false;
                }

                EnsureSettingsLoaded();
                return disableActivitySurrogateSelectorTypeCheck;
            }
        }

        internal static bool DisableXOMLSerializerDefaultUnauthorizedTypes
        {
            get
            {
                // If DynamicCodePolicy is enabled, we are on a "device guard" system, so
                // don't even bother to check for the opt-out AppSetting.
                if (NativeMethods.IsDynamicCodePolicyEnabled())
                {
                    return false;
                }

                EnsureSettingsLoaded();
                return disableXOMLSerializerDefaultUnauthorizedTypes;
            }
        }

        internal static bool DisableXOMLSerializerTypeChecking
        {
            get
            {
                // If DynamicCodePolicy is enabled, we are on a "device guard" system, so
                // don't even bother to check for the opt-out AppSetting.
                if (NativeMethods.IsDynamicCodePolicyEnabled())
                {
                    return false;
                }

                EnsureSettingsLoaded();
                return disableXOMLSerializerTypeChecking;
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
                            if (settings == null || !bool.TryParse(settings["microsoft:WorkflowComponentModel:XOMLAllowXCodeInDisabledActivities"], out allowXCode))
                            {
                                allowXCode = false;
                            }

                            if (settings == null || !int.TryParse(settings["microsoft:WorkflowComponentModel:XOMLMaximumNestedObjectDepth"], out xomlMaximumNestedObjectDepth))
                            {
                                xomlMaximumNestedObjectDepth = DefaultXOMLMaximumNestedObjectDepth;
                            }

                            if (settings == null || !bool.TryParse(settings["microsoft:WorkflowComponentModel:DisableActivitySurrogateSelectorTypeCheck"], out disableActivitySurrogateSelectorTypeCheck))
                            {
                                disableActivitySurrogateSelectorTypeCheck = false;
                            }

                            if (settings == null || !bool.TryParse(settings["microsoft:WorkflowComponentModel:DisableXOMLSerializerDefaultUnauthorizedTypes"], out disableXOMLSerializerDefaultUnauthorizedTypes))
                            {
                                disableXOMLSerializerDefaultUnauthorizedTypes = false;
                            }

                            if (settings == null || !bool.TryParse(settings["microsoft:WorkflowComponentModel:DisableXOMLSerializerTypeChecking"], out disableXOMLSerializerTypeChecking))
                            {
                                disableXOMLSerializerTypeChecking = false;
                            }

                            settingsInitialized = true;
                        }
                    }
                }
            }
        }
    }
}
