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
    using System.Transactions;

    class ComplusEndpointConfigContainer : EndpointConfigContainer
    {
        ComAdminAppInfo appInfo;
        bool closed;        // if this container has not yet been asked to commit or abort, operations are still allowed
        string appDir;
        bool mustGenerateAppDir;
        AtomicFile manifestFile;
        AtomicFile configFile;
        bool listenerComponentExists;
        bool hasServices;
        TransactionScope scope;
        bool modified = false;

        ComplusEndpointConfigContainer(ComAdminAppInfo appInfo)
        {
            this.appInfo = appInfo;
            this.scope = null;
            if (appInfo.ApplicationDirectory != null && appInfo.ApplicationDirectory.Length > 0)
            {
                this.appDir = appInfo.ApplicationDirectory;
                this.mustGenerateAppDir = false;

                if (!Directory.Exists(this.appDir))
                {
                    // We will not tolerate misconfigured COM+ apps (i.e. we wont create the dir for you, which is consistent with what COM+ does
                    throw Tool.CreateException(SR.GetString(SR.ApplicationDirectoryDoesNotExist, appDir), null);
                }
            }
            else
            {
                this.appDir = GeneratedAppDirectoryName();

                if (!Directory.Exists(this.appDir))
                    this.mustGenerateAppDir = true;
            }

            this.configFile = new AtomicFile(Path.Combine(this.appDir, "application.config"));
            this.manifestFile = new AtomicFile(Path.Combine(this.appDir, "application.manifest"));

            this.listenerComponentExists = appInfo.ListenerExists;
            this.hasServices = listenerComponentExists;   // notice how we initialize this

        }

        public override bool WasModified { get { return this.modified; } set { this.modified = value; } }

        internal AtomicFile ConfigFile { get { return this.configFile; } }

        internal bool ListenerComponentExists { get { return this.listenerComponentExists; } }

        public override void AbortChanges()
        {
            this.closed = true;
            this.manifestFile.Abort();
            this.configFile.Abort();

            if (this.mustGenerateAppDir)
            {
                // Delete the directory if it exists
                if (Directory.Exists(this.appDir))
                {
                    Directory.Delete(this.appDir);
                }
            }
            if (scope != null)
            {
                try
                {
                    Transaction.Current.Rollback();
                    scope.Complete();
                    scope.Dispose();
                }
                catch (Exception ex)
                {
                    if (ex is NullReferenceException || ex is SEHException)
                    {
                        throw;
                    }
                    ToolConsole.WriteWarning(SR.GetString(SR.FailedToAbortTransactionWithError, ex.Message));

                }
            }


        }

        public override void Add(IList<EndpointConfig> endpointConfigs)
        {
            ThrowIfClosed();

            Configuration config = GetConfiguration(false); // not read only
            Debug.Assert(config != null, "config != null");

            bool anyAdded = false;

            foreach (EndpointConfig endpointConfig in endpointConfigs)
            {
                Debug.Assert(endpointConfig.Appid == this.appInfo.ID, "can't add endpoint for a different application");

                bool added = this.BaseAddEndpointConfig(config, endpointConfig);

                if (added)
                {
                    anyAdded = true;

                    // the metadata exchange endpoint is not displayed as a regular endpoint
                    if (endpointConfig.Iid == typeof(IMetadataExchange).GUID)
                    {
                        ToolConsole.WriteLine(SR.GetString(SR.MexEndpointAdded));
                        continue;
                    }

                    if (!Tool.Options.ShowGuids)
                    {
                        ToolConsole.WriteLine(SR.GetString(SR.InterfaceAdded,
                                                       endpointConfig.ComponentProgID,
                                                       endpointConfig.InterfaceName));
                    }
                    else
                    {
                        ToolConsole.WriteLine(SR.GetString(SR.InterfaceAdded,
                                                       endpointConfig.Clsid,
                                                       endpointConfig.Iid));
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
                            ToolConsole.WriteWarning(SR.GetString(SR.InterfaceAlreadyExposed,
                                                              endpointConfig.ComponentProgID,
                                                              endpointConfig.InterfaceName));
                        else
                            ToolConsole.WriteWarning(SR.GetString(SR.InterfaceAlreadyExposed,
                                                              endpointConfig.Clsid,
                                                              endpointConfig.Iid));
                    }

                }
            }

            if (anyAdded)
            {
                WasModified = true;

                config.Save();
            }

            this.hasServices = true;
        }

        const string defaultBindingType = "netNamedPipeBinding";
        const string defaultTransactionBindingType = "netNamedPipeBinding";
        const string defaultMexBindingType = "mexNamedPipeBinding";
        const string defaultBindingName = "comNonTransactionalBinding";
        const string defaultTransactionalBindingName = "comTransactionalBinding";

        public override string DefaultBindingType { get { return defaultBindingType; } }
        public override string DefaultBindingName { get { return defaultBindingName; } }
        public override string DefaultTransactionalBindingType { get { return defaultTransactionBindingType; } }
        public override string DefaultTransactionalBindingName { get { return defaultTransactionalBindingName; } }
        public override string DefaultMexBindingType { get { return defaultMexBindingType; } }
        public override string DefaultMexBindingName { get { return null; } }

        void EnsureNetProfileNamedPipeBindingElementBinding(Configuration config)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            if (!sg.Bindings.NetNamedPipeBinding.Bindings.ContainsKey(this.DefaultBindingName))
            {

                NetNamedPipeBindingElement bindingConfig;
                bindingConfig = new NetNamedPipeBindingElement(this.DefaultBindingName);
                sg.Bindings.NetNamedPipeBinding.Bindings.Add(bindingConfig);

            }
            if (!sg.Bindings.NetNamedPipeBinding.Bindings.ContainsKey(this.DefaultTransactionalBindingName))
            {
                NetNamedPipeBindingElement bindingConfig;
                bindingConfig = new NetNamedPipeBindingElement(this.DefaultTransactionalBindingName);
                bindingConfig.TransactionFlow = true;
                sg.Bindings.NetNamedPipeBinding.Bindings.Add(bindingConfig);

            }
        }


        protected override void AddBinding(Configuration config)
        {
            EnsureNetProfileNamedPipeBindingElementBinding(config);
        }

        void EnsureBindingRemoved(Configuration config)
        {

            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            if (sg.Bindings.NetNamedPipeBinding.Bindings.ContainsKey(this.DefaultBindingName))
            {
                NetNamedPipeBindingElement element = sg.Bindings.NetNamedPipeBinding.Bindings[this.DefaultBindingName];
                sg.Bindings.NetNamedPipeBinding.Bindings.Remove(element);
            }
            if (sg.Bindings.NetNamedPipeBinding.Bindings.ContainsKey(this.DefaultTransactionalBindingName))
            {
                NetNamedPipeBindingElement element = sg.Bindings.NetNamedPipeBinding.Bindings[this.DefaultTransactionalBindingName];
                sg.Bindings.NetNamedPipeBinding.Bindings.Remove(element);
            }
        }

        protected override void RemoveBinding(Configuration config)
        {
            EnsureBindingRemoved(config);
        }

        public override void CommitChanges()
        {
            this.manifestFile.Commit();
            this.configFile.Commit();
            if (scope != null)
            {
                try
                {
                    scope.Complete();
                    scope.Dispose();
                }
                catch (Exception ex)
                {
                    if (ex is NullReferenceException || ex is SEHException)
                    {
                        throw;
                    }
                    Tool.CreateException(SR.GetString(SR.FailedToCommitChangesToCatalog), ex);
                }

            }
        }


        public override void PrepareChanges()
        {
            this.closed = true;
            bool workDone = this.configFile.HasBeenModified() && WasModified;
            TransactionOptions opts = new TransactionOptions();
            opts.Timeout = TimeSpan.FromMinutes(5);
            opts.IsolationLevel = IsolationLevel.Serializable;
            scope = new TransactionScope(TransactionScopeOption.Required, opts, EnterpriseServicesInteropOption.Full);

            if (workDone)
            {
                // if appDir doesnt exist, we must create it before we prepare the config files below
                if (this.mustGenerateAppDir)
                {
                    // create it 
                    Directory.CreateDirectory(this.appDir);

                    ToolConsole.WriteLine(SR.GetString(SR.DirectoryCreated, this.appDir));
                }
                // set the COM+ app property
                ComAdminWrapper.SetAppDir(this.appInfo.ID.ToString("B"), this.appDir);

            }

            this.configFile.Prepare();

            if (workDone)
            {
                ToolConsole.WriteLine(SR.GetString((this.configFile.OriginalFileExists ? SR.FileUpdated : SR.FileCreated), configFile.OriginalFileName));
            }


            if (workDone && !this.manifestFile.CurrentExists() && this.hasServices)
            {
                string fileName = this.manifestFile.GetCurrentFileName(false);  // for update
                CreateManifestFile(this.manifestFile.GetCurrentFileName(false));
                ToolConsole.WriteLine(SR.GetString(SR.FileCreated, this.manifestFile.OriginalFileName));
            }

            this.manifestFile.Prepare();

            if (workDone)
            {
                // Now, install the Listener if it isnt already there
                if (this.hasServices && !this.listenerComponentExists)
                {
                    ComAdminWrapper.InstallListener(this.appInfo.ID, this.appDir, this.appInfo.RuntimeVersion);
                }
                else if (!this.hasServices && this.listenerComponentExists)
                {
                    ComAdminWrapper.RemoveListener(this.appInfo.ID);
                }

                if (this.appInfo.IsServerActivated)
                {
                    ToolConsole.WriteWarning(SR.GetString(SR.ShouldRestartApp, this.appInfo.Name));
                }
            }
        }

        void CreateManifestFile(string fileName)
        {
            using (StreamWriter sw = File.CreateText(fileName))
            {
                sw.WriteLine("<assembly manifestVersion=\"1.0\" xmlns=\"urn:schemas-microsoft-com:asm.v1\"><assemblyIdentity name=\"" + this.appInfo.ID.ToString("B") + "\" version=\"1.0.0.0\" type=\"win32\"/></assembly>");
            }
        }

        //returns the properly formatted partiton id iff the app is not in the default partition
        string GetPartitionId(Guid appId)
        {
            string partitionId = null;

            partitionId = ComAdminWrapper.GetPartitionIdForApplication(appId);

            if ((!String.IsNullOrEmpty(partitionId)) && partitionId != ComAdminWrapper.GetGlobalPartitionID())
            {
                //convert guid to representation without {}
                Guid partitionGuid = new Guid(partitionId);
                partitionId = partitionGuid.ToString();
                partitionId = partitionId + "/";
            }
            else
                partitionId = "";

            return partitionId;
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

        public override string DefaultMexAddress(Guid appId, Guid clsid)
        {
            return EndpointConfig.MexEndpointSuffix;

        }

        public override string BaseServiceAddress(Guid appId, Guid clsid, Guid iid)
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

            string uri = Uri.EscapeUriString("net.pipe://localhost/" + adminAppInfo.Name + "/" + GetPartitionId(appId) + adminClassInfo.Name);

            if (Uri.IsWellFormedUriString(uri, UriKind.Absolute))
                return uri;

            return "net.pipe://localhost/" + (appId.ToString() + "/" + clsid.ToString());
        }

        // Just like COM+ Import, we create the following directory "%ProgramFiles%\ComPlus Applications\{appid}"
        string GeneratedAppDirectoryName()
        {
            string programFiles;
            if (ComAdminWrapper.IsApplicationWow(appInfo.ID))
                programFiles = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
            else
                programFiles = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
            return programFiles + "\\ComPlus Applications\\" + this.appInfo.ID.ToString("B") + "\\";
        }

        public static List<ComplusEndpointConfigContainer> Get()
        {
            List<ComplusEndpointConfigContainer> containers = new List<ComplusEndpointConfigContainer>();
            Guid[] ids = ComAdminWrapper.GetApplicationIds();
            foreach (Guid id in ids)
            {
                ComplusEndpointConfigContainer container = ComplusEndpointConfigContainer.Get(id.ToString("B"));
                if (container != null)
                    containers.Add(container);
            }
            return containers;
        }

        public static ComplusEndpointConfigContainer Get(string appIdOrName)
        {
            return Get(appIdOrName, false);
        }

        public static ComplusEndpointConfigContainer Get(string appIdOrName, bool rethrow)
        {
            ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(appIdOrName);
            if (appInfo == null)
            {
                return null;
            }
            try
            {
                ComplusEndpointConfigContainer container = new ComplusEndpointConfigContainer(appInfo);
                return container;
            }
            catch (Exception ex)
            {
                if (ex is NullReferenceException || ex is SEHException)
                {
                    throw;
                }

                if (rethrow)
                    throw;
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.FailedToLoadConfigForApplicationIgnoring, appInfo.Name, ex.Message));
            }
            return null;
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

        public override List<string> GetBaseAddresses(EndpointConfig config)
        {
            List<string> ret = new List<string>();

            Configuration svcconfig = GetConfiguration(true);

            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(svcconfig);
            ServiceElementCollection serviceColl = sg.Services.Services;


            ServiceElement serviceElement = null;

            // Find serviceElement
            foreach (ServiceElement el in serviceColl)
            {
                if (config.MatchServiceType(el.Name))
                {
                    serviceElement = el;
                    break;
                }
            }

            if (null == serviceElement)
                return ret;

            foreach (BaseAddressElement element in serviceElement.Host.BaseAddresses)
                ret.Add(element.BaseAddress);

            return ret;
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
                    endpoint.Appid = this.appInfo.ID;
                    endpoint.Container = this;
                    list.Add(endpoint);
                }
            }

            return list;
        }

        // we override the default implementation since this container represents a single app anyway
        public override List<EndpointConfig> GetEndpointConfigs(Guid appid)
        {
            ThrowIfClosed();

            if (appid == this.appInfo.ID)
            {
                return this.GetEndpointConfigs(); // all our endpoints are for that appid
            }
            else
            {
                return new List<EndpointConfig>();
            }
        }

        public override void Remove(IList<EndpointConfig> endpointConfigs)
        {
            ThrowIfClosed();

            Configuration config = GetConfiguration(false); // not read only
            Debug.Assert(config != null, "config != null");

            bool anyRemoved = false;

            foreach (EndpointConfig endpointConfig in endpointConfigs)
            {
                Debug.Assert(endpointConfig.Appid == this.appInfo.ID, "can't remove endpoint for a different application");

                bool removed = this.BaseRemoveEndpointConfig(config, endpointConfig);
                if (removed)
                {
                    anyRemoved = true;
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

            this.hasServices = ServiceModelSectionGroup.GetSectionGroup(config).Services.Services.Count > 0;
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
    }
}
