//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Description;
    using System.Diagnostics;
    using System.Configuration;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Collections.Generic;
    using System.IO;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.ServiceModel;
    using System.ServiceModel.Configuration;
    using Microsoft.Tools.ServiceModel;
    using System.Xml;


    class EndpointConfig
    {
        Guid appid;
        Guid clsid;
        Guid iid;
        string bindingType;
        string bindingName;
        Uri address;
        bool isMexEndpoint;
        List<string> methods;
        EndpointConfigContainer container;
        public static readonly string TempURI = "http://tempuri.org/";
        public static readonly string MexEndpointSuffix = "mex";
        public Guid Appid { get { return this.appid; } set { this.appid = value; } }
        public Guid Clsid { get { return this.clsid; } }
        public Guid Iid { get { return this.iid; } }
        public string BindingType { get { return this.bindingType; } }
        public string BindingName { get { return this.bindingName; } }
        public Uri Address { get { return this.address; } }
        public bool IsMexEndpoint { get { return isMexEndpoint; } }
        public EndpointConfigContainer Container { get { return container; } set { container = value; } }

        public IList<string> Methods
        {
            get
            {
                return methods;
            }
            set
            {
                methods = (List<string>)value;
            }
        }

        public string ContractType
        {
            get
            {
                if (isMexEndpoint)
                    return ServiceMetadataBehavior.MexContractName;
                else
                    return iid.ToString("B").ToUpperInvariant();

            }
        }

        public bool MatchServiceType(string serviceType)
        {
            string[] serviceParams = serviceType.Split(',');
            if (serviceParams.Length != 2)
            {
                return false;
            }

            try
            {
                Guid guid = new Guid(serviceParams[0]);
                if (guid != Appid)
                    return false;

                guid = new Guid(serviceParams[1]);
                if (guid != Clsid)
                    return false;

                return true;
            }
            catch (FormatException)
            {
            }
            return false;
        }

        public bool MatchContract(string contract)
        {
            if (isMexEndpoint)
            {
                if (ContractType == contract)
                    return true;
                else
                    return false;
            }
            else
            {
                try
                {
                    Guid guid = new Guid(contract);
                    if (guid == Iid)
                        return true;
                }
                catch (FormatException)
                {
                }
                return false;
            }

        }
        public string ServiceType
        {
            get
            {
                return Appid.ToString("B").ToUpperInvariant() + "," + Clsid.ToString("B").ToUpperInvariant();

            }
        }

        public string ApplicationName
        {
            get
            {
                ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(appid.ToString("B"));
                if (null == appInfo)
                    return null;

                return appInfo.Name;
            }
        }

        public string ComponentProgID
        {
            get
            {
                ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(appid.ToString("B"));
                if (null == appInfo)
                    return null;

                ComAdminClassInfo classInfo = appInfo.FindClass(clsid.ToString("B"));
                if (null == classInfo)
                    return null;

                return classInfo.Name;
            }
        }

        public string InterfaceName
        {
            get
            {
                if (!isMexEndpoint)
                {
                    ComAdminAppInfo appInfo = ComAdminWrapper.GetAppInfo(appid.ToString("B"));
                    if (null == appInfo)
                        return null;

                    ComAdminClassInfo classInfo = appInfo.FindClass(clsid.ToString("B"));
                    if (null == classInfo)
                        return null;

                    ComAdminInterfaceInfo interfaceInfo = classInfo.FindInterface(iid.ToString("B"));
                    if (null == interfaceInfo)
                        return null;

                    return interfaceInfo.Name;
                }
                else
                {
                    return ContractType;
                }

            }
        }

        public EndpointConfig(Guid appid,
                              Guid clsid,
                              Guid iid,
                              string bindingType,
                              string bindingName,
                              Uri address,
                              bool isMexEndpoint, List<string> methods)
        {
            this.appid = appid;
            this.clsid = clsid;
            this.iid = iid;
            this.bindingType = bindingType;
            this.bindingName = bindingName;
            this.address = address;
            this.isMexEndpoint = isMexEndpoint;
            this.methods = methods;

            this.container = null;
        }
    }

    abstract class EndpointConfigContainer
    {

        public abstract List<EndpointConfig> GetEndpointConfigs();
        public virtual bool HasEndpointsForApplication(Guid appid)
        {
            // This is a default (but kind of inefficient) implementation of this function
            List<EndpointConfig> endpointConfigs = GetEndpointConfigs(appid);
            return (endpointConfigs.Count > 0);
        }
        public virtual List<EndpointConfig> GetEndpointConfigs(Guid appid)
        {
            // A default implementation, kind of inefficient, but subclasses can override if they want..
            List<EndpointConfig> endpointConfigs = new List<EndpointConfig>();
            foreach (EndpointConfig endpointConfig in GetEndpointConfigs())
            {
                if (endpointConfig.Appid == appid)
                {
                    endpointConfigs.Add(endpointConfig);
                }
            }

            return endpointConfigs;
        }

        public abstract void Add(IList<EndpointConfig> endpointConfigs);
        public abstract void PrepareChanges();
        public abstract void AbortChanges(); // this can be called at any point prior to Commit
        public abstract void CommitChanges();

        public abstract string DefaultEndpointAddress(Guid appId, Guid clsid, Guid iid);
        public abstract string DefaultMexAddress(Guid appId, Guid clsid);
        public abstract string DefaultBindingType { get; }
        public abstract string DefaultBindingName { get; }
        public abstract string DefaultTransactionalBindingType { get; }
        public abstract string DefaultTransactionalBindingName { get; }
        public abstract string DefaultMexBindingType { get; }
        public abstract string DefaultMexBindingName { get; }
        public abstract bool WasModified { get; set; }
        public abstract Configuration GetConfiguration(bool readOnly);
        public abstract string BaseServiceAddress(Guid appId, Guid clsid, Guid iid);
        public abstract List<string> GetBaseAddresses(EndpointConfig config);


        public abstract void Remove(IList<EndpointConfig> endpointConfigs);
        protected abstract void RemoveBinding(Configuration config);
        protected abstract void AddBinding(Configuration config);

        // returns true if added successfully, or false if didnt add due to duplicate.
        protected bool BaseAddEndpointConfig(Configuration config, EndpointConfig endpointConfig)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ServiceElementCollection serviceColl = sg.Services.Services;


            ServiceElement serviceElement = null;

            // Find serviceElement
            foreach (ServiceElement el in serviceColl)
            {
                if (endpointConfig.MatchServiceType(el.Name))
                {
                    serviceElement = el;
                    break;
                }
            }

            if (serviceElement == null)
            {
                // Didn't find one, create new element for this clsid
                serviceElement = new ServiceElement(endpointConfig.ServiceType);
                string baseServiceAddress = BaseServiceAddress(endpointConfig.Appid, endpointConfig.Clsid, endpointConfig.Iid);
                if (!String.IsNullOrEmpty(baseServiceAddress))
                {
                    BaseAddressElement bae = new BaseAddressElement();
                    bae.BaseAddress = baseServiceAddress;
                    serviceElement.Host.BaseAddresses.Add(bae);
                }
                sg.Services.Services.Add(serviceElement);
            }

            if (endpointConfig.IsMexEndpoint)
            {
                EnsureComMetaDataExchangeBehaviorAdded(config);
                serviceElement.BehaviorConfiguration = comServiceBehavior;
            }
            bool methodsAdded = false;
            if (!endpointConfig.IsMexEndpoint)
            {
                methodsAdded = AddComContractToConfig(config, endpointConfig.InterfaceName, endpointConfig.Iid.ToString("B"), endpointConfig.Methods);

            }

            // Now, check if endpoint already exists..
            foreach (ServiceEndpointElement ee in serviceElement.Endpoints)
            {
                bool listenerExists = true;
                if (this is ComplusEndpointConfigContainer)
                    listenerExists = ((ComplusEndpointConfigContainer)this).ListenerComponentExists;

                if (endpointConfig.MatchContract(ee.Contract))
                {
                    if (listenerExists)
                        return methodsAdded; // didn't add due to duplicate
                    else
                        serviceElement.Endpoints.Remove(ee);
                }
            }

            // All right, add the new endpoint now
            ServiceEndpointElement endpointElement = new ServiceEndpointElement(endpointConfig.Address, endpointConfig.ContractType);
            endpointElement.Binding = endpointConfig.BindingType;
            endpointElement.BindingConfiguration = endpointConfig.BindingName;
            serviceElement.Endpoints.Add(endpointElement);

            AddBinding(config);

            return true;
        }
        protected bool RemoveComContractMethods(Configuration config, string interfaceID, IList<string> methods)
        {
            Guid iidInterface = new Guid(interfaceID);
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ComContractElementCollection contractCollection = sg.ComContracts.ComContracts;
            foreach (ComContractElement contractElement in contractCollection)
            {
                try
                {
                    Guid contract = new Guid(contractElement.Contract);
                    if (contract == iidInterface)
                    {
                        foreach (string methodName in methods)
                        {
                            foreach (ComMethodElement methodElement in contractElement.ExposedMethods)
                            {
                                if (methodElement.ExposedMethod == methodName)
                                {
                                    contractElement.ExposedMethods.Remove(methodElement);
                                    break;
                                }
                            }
                        }
                        if (contractElement.ExposedMethods.Count == 0)
                        {
                            sg.ComContracts.ComContracts.Remove(contractElement);
                            return true;
                        }
                    }
                }
                catch (FormatException)
                {
                }
            }
            return false;
        }
        protected bool RemoveComContractIfNotUsedByAnyService(Configuration config, string interfaceID)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ServiceElementCollection serviceColl = sg.Services.Services;
            Guid iidInterface = new Guid(interfaceID);

            // Find serviceElement
            foreach (ServiceElement el in serviceColl)
            {
                // Now, check if endpoint already exists..
                foreach (ServiceEndpointElement ee in el.Endpoints)
                {
                    try
                    {
                        if (!IsMetaDataEndpoint(ee))
                        {
                            Guid Iid = new Guid(ee.Contract);
                            if (iidInterface == Iid)
                                return false;
                        }
                    }
                    catch (FormatException)
                    {

                    }
                }
            }
            ComContractElementCollection contractCollection = sg.ComContracts.ComContracts;
            foreach (ComContractElement element in contractCollection)
            {
                try
                {
                    Guid contract = new Guid(element.Contract);
                    if (contract == iidInterface)
                    {
                        contractCollection.Remove(element);
                        return true;
                    }

                }
                catch (FormatException)
                {

                }
            }
            return false;
        }
        protected bool AddComContractToConfig(Configuration config, string name, string contractType, IList<string> methods)
        {
            Guid contractIID = new Guid(contractType);
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ComContractElementCollection contractCollection = sg.ComContracts.ComContracts;
            foreach (ComContractElement comContract in contractCollection)
            {
                try
                {
                    Guid contractFound = new Guid(comContract.Contract);
                    if (contractIID == contractFound)
                    {
                        bool methodsAdded = false;
                        bool found = false;
                        foreach (string methodName in methods)
                        {
                            found = false;
                            foreach (ComMethodElement methodElement in comContract.ExposedMethods)
                            {
                                if (methodElement.ExposedMethod == methodName)
                                    found = true;
                            }
                            if (!found)
                            {
                                comContract.ExposedMethods.Add(new ComMethodElement(methodName));
                                methodsAdded = true;
                            }
                        }

                        if (comContract.PersistableTypes.Count == 0 && Tool.Options.AllowReferences && methodsAdded)
                        {
                            comContract.PersistableTypes.EmitClear = true;
                        }

                        return methodsAdded;
                    }
                }
                catch (FormatException)
                {

                }
            }
            // The contract does not exists 
            // so we are going to add it
            ComContractElement newComContract = new ComContractElement(contractIID.ToString("B").ToUpperInvariant());
            newComContract.Name = name;
            newComContract.Namespace = EndpointConfig.TempURI + contractIID.ToString().ToUpperInvariant();
            foreach (string methodName in methods)
                newComContract.ExposedMethods.Add(new ComMethodElement(methodName));

            if (newComContract.PersistableTypes.Count == 0 && Tool.Options.AllowReferences)
            {
                newComContract.PersistableTypes.EmitClear = true;
            }

            newComContract.RequiresSession = true;

            contractCollection.Add(newComContract);

            return true;

        }

        protected bool IsMetaDataEndpoint(ServiceEndpointElement ee)
        {
            if (ee.Contract == ServiceMetadataBehavior.MexContractName)
                return true;
            else
                return false;

        }
        // NOTE that all EndpointConfigs returned by this guy have Guid.Empty as the appid, caller needs to fix that up
        protected Dictionary<string, List<EndpointConfig>> BaseGetEndpointsFromConfiguration(Configuration config)
        {
            Dictionary<string, List<EndpointConfig>> endpointConfigs = new Dictionary<string, List<EndpointConfig>>();

            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ServiceElementCollection serviceColl;

            try
            {
                serviceColl = sg.Services.Services;
            }
            catch (System.Configuration.ConfigurationErrorsException e)
            {
                ToolConsole.WriteWarning(e.Message);
                ToolConsole.WriteWarning(SR.GetString(SR.ConfigFileSkipped, e.Filename));

                return endpointConfigs;
            }

            foreach (ServiceElement se in serviceColl)
            {
                string serviceType = se.Name;
                Guid clsid = Guid.Empty;
                Guid appId = Guid.Empty;

                string[] serviceParams = serviceType.Split(',');
                if (serviceParams.Length != 2)
                {
                    continue;
                }

                try
                {
                    appId = new Guid(serviceParams[0]);
                    clsid = new Guid(serviceParams[1]);

                }
                catch (FormatException)
                {
                    // Only Guid serviceTypes are of interest to us - those are the ones our listener picks up
                    continue;
                }


                List<EndpointConfig> list = null;
                if (endpointConfigs.ContainsKey(serviceType))
                {
                    list = endpointConfigs[serviceType];
                }
                else
                {
                    list = new List<EndpointConfig>();
                    endpointConfigs[serviceType] = list;
                }

                foreach (ServiceEndpointElement ee in se.Endpoints)
                {
                    EndpointConfig ec = null;
                    if (!IsMetaDataEndpoint(ee))
                    {
                        Guid contractType;
                        try
                        {
                            contractType = new Guid(ee.Contract);
                        }
                        catch (FormatException)
                        {
                            continue;
                        }
                        ec = new EndpointConfig(Guid.Empty,
                                                               clsid,
                                                               contractType,
                                                               ee.Binding,
                                                               ee.BindingConfiguration,
                                                               ee.Address,
                                                               false, 
                                                               new List<string>());
                    }
                    else
                    {
                        ec = new EndpointConfig(Guid.Empty,
                                                               clsid,
                                                               typeof(IMetadataExchange).GUID,
                                                               ee.Binding,
                                                               ee.BindingConfiguration,
                                                               ee.Address,
                                                               true, new List<string>());
                    }
                    list.Add(ec);
                }
            }
            return endpointConfigs;
        }

        protected bool RemoveAllServicesForContract(Configuration config, string interfaceID)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ServiceElementCollection serviceColl = sg.Services.Services;
            bool removed = false;
            // Iterate over every serviceElement 
            // in the services collection
            // and delete the specific interface
            ServiceElementCollection svcColl = new ServiceElementCollection();
            foreach (ServiceElement el in serviceColl)
            {
                ServiceEndpointElementCollection endpointCollection = new ServiceEndpointElementCollection();
                foreach (ServiceEndpointElement ee in el.Endpoints)
                {
                    if (interfaceID.ToUpperInvariant() == ee.Contract.ToUpperInvariant())
                    {
                        // found it !
                        removed = true;
                        endpointCollection.Add(ee);
                    }
                }
                foreach (ServiceEndpointElement elementEndpoint in endpointCollection)
                {
                    el.Endpoints.Remove(elementEndpoint);
                    if (el.Endpoints.Count == 1)
                        if (el.Endpoints[0].Contract == ServiceMetadataBehavior.MexContractName)
                            el.Endpoints.Remove(el.Endpoints[0]); // if Mex endpoint remove it.
                    if (el.Endpoints.Count == 0)
                        svcColl.Add(el);
                }

            }
            foreach (ServiceElement service in svcColl)
            {
                sg.Services.Services.Remove(service);
            }
            if (serviceColl.Count == 0)
            {
                EnsureComMetaDataExchangeBehaviorRemoved(config);
                RemoveBinding(config);
            }
            return removed;
        }

        protected bool RemoveEndpointFromServiceOnly(Configuration config, EndpointConfig endpointConfig)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            ServiceElementCollection serviceColl = sg.Services.Services;
            ServiceElement serviceElement = null;

            // Find serviceElement
            foreach (ServiceElement el in serviceColl)
            {
                if (endpointConfig.MatchServiceType(el.Name))
                {
                    serviceElement = el;
                    break;
                }
            }

            if (serviceElement == null)
            {
                // Didn't find class
                return false;
            }

            // Now, check if endpoint already exists..
            foreach (ServiceEndpointElement ee in serviceElement.Endpoints)
            {
                if (endpointConfig.MatchContract(ee.Contract) &&
                    (ee.Address == endpointConfig.Address))
                {
                    // found it !
                    serviceElement.Endpoints.Remove(ee);
                    if (!endpointConfig.IsMexEndpoint)
                        RemoveComContractIfNotUsedByAnyService(config, ee.Contract);
                    if (serviceElement.Endpoints.Count == 1)
                        if (serviceElement.Endpoints[0].Contract == ServiceMetadataBehavior.MexContractName)
                            serviceElement.Endpoints.Remove(serviceElement.Endpoints[0]); // if Mex endpoint remove it.


                    if (serviceElement.Endpoints.Count == 0)
                    {
                        serviceColl.Remove(serviceElement);
                        if (serviceColl.Count == 0)
                        {
                            EnsureComMetaDataExchangeBehaviorRemoved(config);
                            RemoveBinding(config);
                        }
                    }
                    return true;
                }
            }
            return false;
        }
        // returns true if removed successfully, or false if didnt remove due to missing.
        protected bool BaseRemoveEndpointConfig(Configuration config, EndpointConfig endpointConfig)
        {
            if ((endpointConfig.Methods != null) && (!endpointConfig.IsMexEndpoint))
            {
                bool removeContract = RemoveComContractMethods(config, endpointConfig.Iid.ToString("B"), endpointConfig.Methods);
                if (removeContract)
                    RemoveAllServicesForContract(config, endpointConfig.Iid.ToString("B"));
                return true;
            }
            else
                return RemoveEndpointFromServiceOnly(config, endpointConfig);
        }

        // helper function used by subclasses
        protected Configuration GetConfigurationFromFile(string fileName)
        {
            ExeConfigurationFileMap fileMap = new ExeConfigurationFileMap();
            Configuration machineConfig = ConfigurationManager.OpenMachineConfiguration();
            fileMap.MachineConfigFilename = machineConfig.FilePath;
            fileMap.ExeConfigFilename = fileName;

            if (!IsValidRuntime(fileName))
            {
                string runtimeVersion = Assembly.GetExecutingAssembly().ImageRuntimeVersion;
                ToolConsole.WriteError(SR.GetString(SR.InvalidRuntime, runtimeVersion), "");
                throw Tool.CreateException(SR.GetString(SR.OperationAbortedDuetoClrVersion), null);
            }

            return ConfigurationManager.OpenMappedExeConfiguration(fileMap, ConfigurationUserLevel.None);

        }


        const string comServiceBehavior = "ComServiceMexBehavior";

        protected void EnsureComMetaDataExchangeBehaviorAdded(Configuration config)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            if (!sg.Behaviors.ServiceBehaviors.ContainsKey(comServiceBehavior))
            {
                ServiceBehaviorElement behavior = new ServiceBehaviorElement(comServiceBehavior);
                sg.Behaviors.ServiceBehaviors.Add(behavior);
                ServiceMetadataPublishingElement metadataPublishing = new ServiceMetadataPublishingElement();

                if (Tool.Options.Hosting == Hosting.Complus || Tool.Options.Hosting == Hosting.NotSpecified)
                    metadataPublishing.HttpGetEnabled = false;
                else
                    metadataPublishing.HttpGetEnabled = true;
                behavior.Add(metadataPublishing);

                ServiceDebugElement serviceDebug = new ServiceDebugElement();
                serviceDebug.IncludeExceptionDetailInFaults = false;
                behavior.Add(serviceDebug);

            }

        }

        protected void EnsureComMetaDataExchangeBehaviorRemoved(Configuration config)
        {
            ServiceModelSectionGroup sg = ServiceModelSectionGroup.GetSectionGroup(config);
            if (sg.Behaviors.ServiceBehaviors.ContainsKey(comServiceBehavior))
            {
                ServiceBehaviorElement element = sg.Behaviors.ServiceBehaviors[comServiceBehavior];
                sg.Behaviors.ServiceBehaviors.Remove(element);
            }
        }
        // we can only run on the clr version that we are built against
        internal static bool IsValidVersion(string version)
        {
            if (String.IsNullOrEmpty(version))
                return false;

            return (version == Assembly.GetExecutingAssembly().ImageRuntimeVersion);
        }

        public static bool IsValidRuntime(string fileName)
        {
            // this is a negative check - we only want to fail
            // if incorrect data specifically was set

            if (String.IsNullOrEmpty(fileName))
                return true;

            // this check is needed since we might get here on temporary files that are gone by now
            if (!File.Exists(fileName))
                return true;

            XmlNode startupNode = null;
            XmlNodeList supportedRuntimes = null;
            XmlNode requiredRuntime = null;
            XmlDocument xmlDoc = new XmlDocument();

            xmlDoc.Load(fileName);
            startupNode = xmlDoc.DocumentElement.SelectSingleNode("startup");
            bool validRuntimeAvailable = true;

            if (null != startupNode)
            {
                supportedRuntimes = startupNode.SelectNodes("supportedRuntime");

                if (null != supportedRuntimes)
                {
                    if (supportedRuntimes.Count == 0)
                        validRuntimeAvailable = true;
                    else
                        validRuntimeAvailable = false;

                    foreach (XmlNode xmlNodeS in supportedRuntimes)
                    {
                        if (IsValidVersion(xmlNodeS.Attributes.GetNamedItem("version").Value))
                            validRuntimeAvailable = true;
                    }
                }

                requiredRuntime = startupNode.SelectSingleNode("requiredRuntime");

                if (null != requiredRuntime)
                {
                    string requiredVersion = requiredRuntime.Attributes.GetNamedItem("version").Value;

                    if (!IsValidVersion(requiredVersion))
                    {
                        validRuntimeAvailable = false;
                    }

                }
            }

            return validRuntimeAvailable;
        }
    }


}
