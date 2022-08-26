// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       AppSettings.cs
//
//  Summary:    AppSettings container for .Net Remoting.
//
//==========================================================================

using System;
using System.Configuration;

namespace System.Runtime.Remoting.Configuration
{
    internal static class AppSettings
    {
        // All supported appSettings keys, default values, and fwlinks where appropriate
        internal static readonly string AllowTransparentProxyMessageKeyName = "microsoft:Remoting:AllowTransparentProxyMessage";
        internal static readonly bool AllowTransparentProxyMessageDefaultValue = false;
        internal static readonly string AllowTransparentProxyMessageFwLink = "http://go.microsoft.com/fwlink/?LinkId=390633";

        internal static readonly string AllowUnsanitizedWSDLUrlsKeyName = "microsoft:Remoting:AllowUnsanitizedWSDLUrls";
        internal static readonly bool AllowUnsanitizedWSDLUrlsDefaultValue = false;

        // All appSettings must be initialized to their default value in case appSettings section is not present.
        private static bool allowTransparentProxyMessageValue = AllowTransparentProxyMessageDefaultValue;
        private static bool allowUnsanitizedWSDLUrlsValue = AllowUnsanitizedWSDLUrlsDefaultValue;

        private static volatile bool settingsInitialized = false;
        private static object appSettingsLock = new object();

        internal static bool AllowUnsanitizedWSDLUrls
        {
            get
            {
                EnsureSettingsLoaded();
                return allowUnsanitizedWSDLUrlsValue;
            }
        }

        internal static bool AllowTransparentProxyMessage
        {
            get
            {
                EnsureSettingsLoaded();
                return allowTransparentProxyMessageValue;
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
                        try
                        {
                            AppSettingsReader reader = new AppSettingsReader();
                            object value = null;
                            if (TryGetValue(reader, AllowTransparentProxyMessageKeyName, typeof(bool), out value))
                            {
                                allowTransparentProxyMessageValue = (bool)value;
                            }
                            else
                            {
                                allowTransparentProxyMessageValue = AllowTransparentProxyMessageDefaultValue;
                            }

                            if (TryGetValue(reader, AllowUnsanitizedWSDLUrlsKeyName, typeof(bool), out value))
                            {
                                allowUnsanitizedWSDLUrlsValue = (bool)value;
                            }
                            else
                            {
                                allowUnsanitizedWSDLUrlsValue = AllowUnsanitizedWSDLUrlsDefaultValue;
                            }
                        }
                        catch
                        {
                            // AppSettingsReader.ctor will throw if no appSettings section
                        }
                        finally
                        {
                            settingsInitialized = true;
                        }
                    }
                }
            }
        }
        
        // Helper method to provide a TryGetValue for AppSettingsReader.
        // AppSettingReader is used to avoid taking a direct dependendency on System.Configuration.dll.
        // In .Net 1.1, AppSettingsReader uses System.Configuration.ConfigurationSettings.AppSettings.
        // Later versions of .Net use System.Configuration.ConfigurationManager.AppSettings.
        // No version of AppSettingsReader provides a TryGetValue.
        private static bool TryGetValue(AppSettingsReader appSettingsReader, string key, Type type, out object value)
        {
            try
            {
                // GetValue throws if arguments are null, key is not present or Convert.ChangeType cannot convert to 'type'
                value = appSettingsReader.GetValue(key, type);
                return true;
            }
            catch
            {
                value = null;
                return false;
            }
        }
    }
} // namespace System.Runtime.Remoting.Configuration
