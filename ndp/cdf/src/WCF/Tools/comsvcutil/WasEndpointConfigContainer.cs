//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.Diagnostics;
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
    using System.ServiceModel.Description;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;
    using System.Web.Configuration;

    class WasEndpointConfigContainer : EndpointConfigContainer
    {
        bool closed;
        AtomicFile configFile;
        SvcFileManager svcFileManager;
        string webDirectoryName;
        string webDirectoryPath;
        string webServerName;
        RuntimeVersions runtimeVersion;
        bool modified = false;

        WasEndpointConfigContainer(string webServerName, string webDirectoryName, string webDirectoryPath, RuntimeVersions runtimeVersion)
        {
            if (string.IsNullOrEmpty(webDirectoryPath) || !Directory.Exists(webDirectoryPath))
            {
                // We will not tolerate a webDir that does not exist
                throw Tool.CreateException(SR.GetString(SR.WebDirectoryPathNotFound, webDirectoryName, webDirectoryPath), null);
            }

            this.webDirectoryName = webDirectoryName;
            this.webDirectoryPath = webDirectoryPath;
            this.webServerName = webServerName;
            this.closed = false;
            this.configFile = new AtomicFile(this.webDirectoryPath + "\\web.config");
            this.svcFileManager = new SvcFileManager(this.webDirectoryPath);
            this.runtimeVersion = runtimeVersion;
        }

        public override bool WasModified { get { return this.modified; } set { this.modified = value; } }

        public override void AbortChanges()
        {
            this.closed = true;
            this.configFile.Abort();
            this.svcFileManager.Abort();
        }

        internal AtomicFile ConfigFile { get { return this.configFile; } }


        public override void Add(IList<EndpointConfig> endpointConfigs)
        {
            ThrowIfClosed();

            Configuration config = GetConfiguration(false); // not read only
            Debug.Assert(config != null, "config != null");

            bool anyAdded = false;
            const string systemWebSection = "system.web";

            foreach (EndpointConfig endpointConfig in endpointConfigs)
            {
                Guid appid;
                //verify that we someone did not delete and reinstall the app from underneath us
                if (this.svcFileManager.ResolveClsid(endpointConfig.Clsid, out appid))
                {
                    if (endpointConfig.Appid != appid)
                    {
                        ToolConsole.WriteError(SR.GetString(SR.AppIDsDontMatch), "");
                        return;
                    }
                }
            }

            SystemWebSectionGroup swsg = (SystemWebSectionGroup)config.GetSectionGroup(systemWebSection);
            CompilationSection compilationSection = swsg.Compilation;
            if (string.IsNullOrEmpty(compilationSection.TargetFramework) && RuntimeVersions.V40 == this.runtimeVersion)
            {
                anyAdded = true;
                compilationSection.TargetFramework = ".NETFramework, Version=v4.0";
            }

            foreach (EndpointConfig endpointConfig in endpointConfigs)
            {
                bool added = this.BaseAddEndpointConfig(config, endpointConfig);
                if (added)
                {
                    this.svcFileManager.Add(endpointConfig.Appid, endpointConfig.Clsid);
                    anyAdded = true;

                    // the metadata exchange endpoint is not displayed as a regular endpoint
                    if (endpointConfig.Iid == typeof(IMetadataExchange).GUID)
                    {
                        ToolConsole.WriteLine(SR.GetString(SR.MexEndpointAdded));
                        continue;
                    }

                    if (!Tool.Options.ShowGuids)
                    {
                        ToolConsole.WriteLine(SR.GetString(SR.InterfaceAdded, endpointConfig.ComponentProgID, endpointConfig.InterfaceName));
                    }
                    else
                    {
                        ToolConsole.WriteLine(SR.GetString(SR.InterfaceAdded, endpointConfig.Clsid, endpointConfig.Iid));
                    }
                }
                else
                {
                    // the metadata exchange endpoint is not displayed as a regular endpoint
                    if (endpointConfig.Iid == typeof(IMetadataExchange).GUID)
                    {
                        if (!Tool.Options.ShowGuids)
                            ToolConsole.WriteWarning(SR.GetString(SR.MexEndpointAlreadyExposed, endpointConfig.ComponentProgID));
                        else
                            ToolConsole.WriteWarning(SR.GetString(SR.MexEndpointAlreadyExposed, endpointConfig.Clsid));
                    }
                    else
                    {
                        if (!Tool.Options.ShowGuids)
                            ToolConsole.WriteWarning(SR.GetString(SR.InterfaceAlreadyExposed, endpointConfig.ComponentProgID, endpointConfig.InterfaceName));
                        else
                            ToolConsole.WriteWarning(SR.GetString(SR.InterfaceAlreadyExposed, endpointConfig.Clsid, endpointConfig.Iid));
                    }
                }
            }

            if (anyAdded)
            {
                WasModified = true;
                config.Save();
            }
        }
        void EnsureWSProfileBindingAdded(Configuration config)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            if (!sg.Bindings.WSHttpBinding.Bindings.ContainsKey(this.DefaultBindingName))
            {
                WSHttpBindingElement bindingConfig = new WSHttpBindingElement(this.DefaultBindingName);
                bindingConfig.ReliableSession.Enabled = true;
                sg.Bindings.WSHttpBinding.Bindings.Add(bindingConfig);
            }
            if (!sg.Bindings.WSHttpBinding.Bindings.ContainsKey(this.DefaultTransactionalBindingName))
            {
                WSHttpBindingElement bindingConfig = new WSHttpBindingElement(this.DefaultTransactionalBindingName);
                bindingConfig.ReliableSession.Enabled = true;
                bindingConfig.TransactionFlow = true;
                sg.Bindings.WSHttpBinding.Bindings.Add(bindingConfig);

            }
        }

        protected override void AddBinding(Configuration config)
        {
            EnsureWSProfileBindingAdded(config);
        }


        const string defaultBindingType = "wsHttpBinding";
        const string defaultTransactionBindingType = "wsHttpBinding";
        const string defaultMexBindingType = "mexHttpBinding";
        const string defaultBindingName = "comNonTransactionalBinding";
        const string defaultTransactionalBindingName = "comTransactionalBinding";

        public override string DefaultBindingType { get { return defaultBindingType; } }
        public override string DefaultBindingName { get { return defaultBindingName; } }
        public override string DefaultTransactionalBindingType { get { return defaultTransactionBindingType; } }
        public override string DefaultTransactionalBindingName { get { return defaultTransactionalBindingName; } }
        public override string DefaultMexBindingType { get { return defaultMexBindingType; } }
        public override string DefaultMexBindingName { get { return null; } }

        void EnsureBindingRemoved(Configuration config)
        {

            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            if (sg.Bindings.WSHttpBinding.Bindings.ContainsKey(this.DefaultBindingName))
            {
                WSHttpBindingElement element = sg.Bindings.WSHttpBinding.Bindings[this.DefaultBindingName];
                sg.Bindings.WSHttpBinding.Bindings.Remove(element);
            }
            if (sg.Bindings.WSHttpBinding.Bindings.ContainsKey(this.DefaultTransactionalBindingName))
            {
                WSHttpBindingElement element = sg.Bindings.WSHttpBinding.Bindings[this.DefaultTransactionalBindingName];
                sg.Bindings.WSHttpBinding.Bindings.Remove(element);
            }
        }
        protected override void RemoveBinding(Configuration config)
        {
            EnsureBindingRemoved(config);
        }
        public override void CommitChanges()
        {
            this.configFile.Commit();
            this.svcFileManager.Commit();
        }

        public override string DefaultEndpointAddress(Guid appId, Guid clsid, Guid iid)
        {
            ComAdminAppInfo adminAppInfo = ComAdminWrapper.GetAppInfo(appId.ToString("B"));
            if (null == adminAppInfo)
            {
                throw Tool.CreateException(SR.GetString(SR.CannotFindAppInfo, appId.ToString("B")), null);
            }

            ComAdminClassInfo adminClassInfo = adminAppInfo.FindClass(clsid.ToString("B"));
            if (null == adminClassInfo)
            {
                throw Tool.CreateException(SR.GetString(SR.CannotFindClassInfo, clsid.ToString("B")), null);
            }

            ComAdminInterfaceInfo adminInterfaceInfo = adminClassInfo.FindInterface(iid.ToString("B"));
            if (null == adminInterfaceInfo)
            {
                throw Tool.CreateException(SR.GetString(SR.CannotFindInterfaceInfo, iid.ToString("B")), null);
            }

            string uri = Uri.EscapeUriString(adminInterfaceInfo.Name);

            if (Uri.IsWellFormedUriString(uri, UriKind.RelativeOrAbsolute))
                return uri;

            return iid.ToString().ToUpperInvariant();
        }

        public override List<string> GetBaseAddresses(EndpointConfig config)
        {
            return new List<string>();
        }

        public override string BaseServiceAddress(Guid appId, Guid clsid, Guid iid)
        {
            return String.Empty;
        }

        public override string DefaultMexAddress(Guid appId, Guid clsid)
        {
            return EndpointConfig.MexEndpointSuffix;
        }

        public override Configuration GetConfiguration(bool readOnly)
        {
            string fileName = this.configFile.GetCurrentFileName(readOnly);
            if (string.IsNullOrEmpty(fileName))
            {
                return null;
            }

            return GetConfigurationFromFile(fileName);
        }

        public override List<EndpointConfig> GetEndpointConfigs()
        {
            ThrowIfClosed();

            Configuration config = GetConfiguration(true); // readonly
            if (config == null)
            {
                // null config means there is no config to read, return an empty list
                return new List<EndpointConfig>();
            }

            Dictionary<string, List<EndpointConfig>> endpointConfigs = BaseGetEndpointsFromConfiguration(config);
            List<EndpointConfig> list = new List<EndpointConfig>();

            // now, fix up the appid for all the endpoints
            foreach (List<EndpointConfig> endpoints in endpointConfigs.Values)
            {
                foreach (EndpointConfig endpoint in endpoints)
                {
                    Guid appid;

                    if (this.svcFileManager.ResolveClsid(endpoint.Clsid, out appid))
                    {
                        endpoint.Appid = appid;
                        list.Add(endpoint);
                    }
                    else
                    {
                        string appName = endpoint.ApplicationName;
                        string componentID = endpoint.ComponentProgID;
                        string interfaceName = endpoint.InterfaceName;

                        if (!Tool.Options.ShowGuids && !string.IsNullOrEmpty(appName) && !string.IsNullOrEmpty(componentID) && !string.IsNullOrEmpty(interfaceName))
                            ToolConsole.WriteWarning(SR.GetString(SR.EndpointNotFoundInSvcFile, appName, componentID, interfaceName, endpoint.BindingName, endpoint.Address));
                        else
                            ToolConsole.WriteWarning(SR.GetString(SR.EndpointNotFoundInSvcFile, endpoint.Appid.ToString("B"), endpoint.Clsid.ToString("B"), endpoint.Iid.ToString("B"), endpoint.BindingName, endpoint.Address));
                    }
                }
            }

            return list;
        }

        int NumEndpointsForClsid(Configuration config, Guid clsid, Guid appId)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ServiceElementCollection serviceColl = sg.Services.Services;

            foreach (ServiceElement se in serviceColl)
            {

                string[] serviceParams = se.Name.Split(',');
                if (serviceParams.Length != 2)
                {
                    continue;
                }

                Guid serviceAppId;
                Guid serviceClsid;

                try
                {
                    serviceAppId = new Guid(serviceParams[0]);
                    serviceClsid = new Guid(serviceParams[1]);
                }
                catch (FormatException)
                {
                    // Only Guid serviceTypes are of interest to us - those are the ones our listener picks up
                    continue;
                }

                if (serviceClsid == clsid && serviceAppId == appId)
                {
                    return se.Endpoints.Count;
                }
            }

            return 0;
        }

        public override void PrepareChanges()
        {
            this.closed = true;
            bool workDone = this.configFile.HasBeenModified() && WasModified;


            this.configFile.Prepare();

            if (workDone)
            {
                ToolConsole.WriteLine(SR.GetString((this.configFile.OriginalFileExists ? SR.FileUpdated : SR.FileCreated), configFile.OriginalFileName));
            }

            this.svcFileManager.Prepare();
        }

        public override void Remove(IList<EndpointConfig> endpointConfigs)
        {
            ThrowIfClosed();

            Configuration config = GetConfiguration(false); // not read only
            Debug.Assert(config != null, "config != null");

            bool anyRemoved = false;

            foreach (EndpointConfig endpointConfig in endpointConfigs)
            {
                bool removed = this.BaseRemoveEndpointConfig(config, endpointConfig);
                if (removed)
                {
                    anyRemoved = true;

                    if (NumEndpointsForClsid(config, endpointConfig.Clsid, endpointConfig.Appid) == 0)
                    {
                        // We can only Remove the SVC file when there are no more endpoints for the Clsid
                        this.svcFileManager.Remove(endpointConfig.Appid, endpointConfig.Clsid);
                    }
                    if (!Tool.Options.ShowGuids)
                        ToolConsole.WriteLine(SR.GetString(SR.InterfaceRemoved, endpointConfig.ComponentProgID, endpointConfig.InterfaceName));
                    else
                        ToolConsole.WriteLine(SR.GetString(SR.InterfaceRemoved, endpointConfig.Clsid, endpointConfig.Iid));
                }
                else if (!endpointConfig.IsMexEndpoint)
                {
                    if (!Tool.Options.ShowGuids)
                        ToolConsole.WriteWarning(SR.GetString(SR.InterfaceNotExposed, endpointConfig.ComponentProgID, endpointConfig.InterfaceName));
                    else
                        ToolConsole.WriteWarning(SR.GetString(SR.InterfaceNotExposed, endpointConfig.Clsid, endpointConfig.Iid));
                }
            }

            if (anyRemoved)
            {
                WasModified = true;
                config.Save();
            }
        }

        void ThrowIfClosed()
        {
            if (this.closed)
            {
                Debug.Assert(false, "attempting operation after container is closed");
                throw new InvalidOperationException();
            }
        }

        public static string DefaultWebServer
        {
            get { return WasAdminWrapper.DefaultWebServer; }
        }

        public static List<WasEndpointConfigContainer> Get(string applicationIdOrName)
        {
            List<WasEndpointConfigContainer> containers = new List<WasEndpointConfigContainer>();
            string[] webServers = WasAdminWrapper.GetWebServerNames();
            if (webServers != null)
            {
                foreach (string webServer in webServers)
                {
                    List<WasEndpointConfigContainer> cont = Get(webServer, applicationIdOrName);
                    containers.AddRange(cont);
                }
            }
            return containers;
        }

        public static List<WasEndpointConfigContainer> Get(string webServer, string applicationIdOrName)
        {
            List<WasEndpointConfigContainer> containers = new List<WasEndpointConfigContainer>();
            string[] webDirectories = WasAdminWrapper.GetWebDirectoryNames(webServer);
            if (webDirectories != null)
            {
                foreach (string webDirectory in webDirectories)
                {
                    WasEndpointConfigContainer container = Get(webServer, webDirectory, applicationIdOrName);
                    if (container != null)
                    {
                        containers.Add(container);
                    }
                }
            }
            return containers;
        }

        public static WasEndpointConfigContainer Get(string webServer, string webDirectory, string applicationIdOrName)
        {
            string webDirectoryPath = null;
            RuntimeVersions runtimeVersion = RuntimeVersions.V40;

            if (!string.IsNullOrEmpty(applicationIdOrName))
            {
                ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(applicationIdOrName);
                runtimeVersion = appInfo.RuntimeVersion;
            }
            if (WasAdminWrapper.GetWebDirectoryPath(webServer, webDirectory, out webDirectoryPath))
            {
                try
                {
                    return new WasEndpointConfigContainer(webServer, webDirectory, webDirectoryPath, runtimeVersion);
                }
                catch (Exception ex)
                {
                    if (ex is NullReferenceException || ex is SEHException)
                    {
                        throw ex;
                    }
                    ToolConsole.WriteWarning(SR.GetString(SR.FailedToLoadConfigForWebDirectoryOnWebSite, webDirectory, webServer));

                }
                return null;
            }
            else
            {
                return null;
            }
        }
    }
}
