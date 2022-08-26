//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.Diagnostics;
    using System.DirectoryServices;
    using System.Configuration;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Collections.Generic;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.ServiceModel;
    using System.ServiceModel.Configuration;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;
    using Microsoft.Win32;

    internal static class WasAdminWrapper
    {
        const string webService = "IIS://localhost/w3svc";
        const string defaultWebServer = "1";

        public static string DefaultWebServer
        {
            get { return webService + "/" + defaultWebServer; }
        }

        public static bool IsIISInstalled()
        {
            const string inetStp = "SOFTWARE\\Microsoft\\InetStp";
            const string majorVersion = "MajorVersion";
            const string minorVersion = "MinorVersion";
            const string components = "Components";
            const string W3SVC = "W3SVC";

            try
            {
                RegistryKey inetStpKey = Registry.LocalMachine.OpenSubKey(inetStp);
                if (inetStpKey != null)
                {
                    // Check if the IIS is installed, we don't care which version is installed
                    inetStpKey.GetValue(majorVersion);
                    inetStpKey.GetValue(minorVersion);
                    RegistryKey componentsKey = inetStpKey.OpenSubKey(components);
                    // Check if the w3svc component is installed, WCF service over HTTP requires it
                    if (componentsKey != null)
                    {
                        int w3svcValue = (int)componentsKey.GetValue(W3SVC);
                        if (w3svcValue != 0)
                        {
                            return true;
                        }
                    }
                }

            }
            catch (IOException)
            {
                return false;
            }

            return false;
        }
        public static string[] GetWebServerNames()
        {
            if (!IsIISInstalled())
                return null;
            try
            {
                List<string> webServerNames = new List<string>();

                DirectoryEntry webServiceEntry = new DirectoryEntry(webService);

                foreach (DirectoryEntry child in webServiceEntry.Children)
                {
                    if (child.SchemaClassName.ToUpperInvariant() == "IISWEBSERVER")
                    {
                        webServerNames.Add(webService + "/" + child.Name);      // Note, child.Name is a number!  the "friendly" name is actually child.Description
                    }
                }
                return webServerNames.ToArray();
            }
            catch (COMException ex)
            {
                // assume a failure here means that no web servers exist
                ToolConsole.WriteWarning(SR.GetString(SR.CannotGetWebServersIgnoringWas,
                        ex.ErrorCode, ex.Message));
                return null;

            }
        }

        public static string[] GetWebDirectoryNames(string webServer)
        {
            if (!IsIISInstalled())
                return null;

            try
            {
                List<string> webDirectoryNames = new List<string>();

                DirectoryEntry webServiceEntry = new DirectoryEntry(webServer);

                foreach (DirectoryEntry child in webServiceEntry.Children)
                {
                    if (child.SchemaClassName.ToUpperInvariant() == "IISWEBDIRECTORY" || child.SchemaClassName.ToUpperInvariant() == "IISWEBVIRTUALDIR")
                    {
                        webDirectoryNames.Add(child.Name);

                        // Must special case the "ROOT" vDir, since most actual vDirs are subchildren of the ROOT vdir of a server.
                        if (child.Name.ToUpperInvariant() == "ROOT")
                        {
                            foreach (DirectoryEntry rootChild in child.Children)
                            {
                                if (rootChild.SchemaClassName.ToUpperInvariant() == "IISWEBDIRECTORY" || rootChild.SchemaClassName.ToUpperInvariant() == "IISWEBVIRTUALDIR")
                                {
                                    webDirectoryNames.Add("ROOT" + "/" + rootChild.Name);
                                }
                            }
                        }
                    }
                }
                return webDirectoryNames.ToArray();
            }
            catch (COMException ex)
            {
                // assume a failure here means that no web directory exist
                ToolConsole.WriteWarning(SR.GetString(SR.CannotGetWebDirectoryForServer,
                        webServer, ex.ErrorCode, ex.Message));
                return null;

            }

        }

        public static bool GetWebDirectoryPath(string webServer, string webDirectory, out string webDirectoryPath)
        {
            webDirectoryPath = null;

            if (!IsIISInstalled())
                return false;

            if (!webDirectory.ToUpperInvariant().StartsWith("ROOT", StringComparison.Ordinal))
                webDirectory = "root/" + webDirectory;

            string[] webDirectories = GetWebDirectoryNames(webServer);
            if (webDirectories == null)
                return false;
            bool found = false;
            foreach (string webDirectoryName in webDirectories)
                if (webDirectoryName.ToUpperInvariant() == webDirectory.ToUpperInvariant())
                {
                    found = true;
                    break;
                }
            if (!found) return false;
            DirectoryEntry webDirectoryEntry = new DirectoryEntry(webServer + "/" + webDirectory);
            try
            {
                if (webDirectoryEntry.Properties.Contains("Path"))
                {
                    webDirectoryPath = (string)webDirectoryEntry.Properties["Path"].Value;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            catch (COMException ex)
            {
                // assume a failure here means the dir does not exist
                ToolConsole.WriteWarning(SR.GetString(SR.CannotGetWebDirectoryPathOnWebDirOfWebServIgnoring,
                        webServer, webDirectory, ex.ErrorCode, ex.Message));
                return false;
            }
        }
    }
}
