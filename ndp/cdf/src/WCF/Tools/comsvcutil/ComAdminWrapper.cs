//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Text;
    using System.Threading;
    using System.ServiceModel;
    using System.EnterpriseServices;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;
    using Microsoft.Win32;
    using System.Security.Permissions;

    static internal partial class HR
    {
        public static readonly int COMADMIN_E_OBJECT_DOES_NOT_EXIST = unchecked((int)0x80110809);

    }

    enum RuntimeVersions
    {
        V20,
        V40
    }

    static internal class ComAdminWrapper
    {
        static readonly string ListenerApplicationName = SR.GetString(SR.WebServiceAppName);
        static Assembly ListenerAssembly = typeof(Message).Assembly;
        static string ListenerComponentDescription = SR.GetString(SR.ListenerCompDescription);
        const string ListenerWSUName = "ServiceModelInitializer";
        internal const string Wcf30RegistryKey = @"SOFTWARE\Microsoft\NET Framework Setup\NDP\v3.0\Setup\Windows Communication Foundation";
        internal const string Runtime30InstallPathName = "RuntimeInstallPath";

        const string fileName = @"ServiceMonikerSupport.dll";

        static bool FindApplication(string appidOrName, out ICatalogObject targetAppObj, out ICatalogCollection appColl)
        {
            targetAppObj = null;
            appColl = null;

            bool found = false;
            ICatalog2 catalog = GetCatalog();
            string partitionId = null;

            partitionId = GetPartitionIdForApplication(catalog, appidOrName, true);
            if (!string.IsNullOrEmpty(partitionId))
            {
                SetCurrentPartition(catalog, partitionId);
            }

            appColl = (ICatalogCollection)(catalog.GetCollection(CollectionName.Applications));
            appColl.Populate();

            for (int i = 0; i < appColl.Count(); i++)
            {
                ICatalogObject appObj = (ICatalogObject)(appColl.Item(i));
                string id = ((string)appObj.Key()).ToLowerInvariant();
                string name = ((string)appObj.Name()).ToLowerInvariant();
                appidOrName = appidOrName.ToLowerInvariant();

                if (!found)
                {
                    if ((appidOrName == id) || (appidOrName == name))
                    {
                        found = true;
                        targetAppObj = appObj;
                    }
                }
                else
                {
                    if ((appidOrName == id) || (appidOrName == name))
                    {
                        throw Tool.CreateException(SR.GetString(SR.AmbiguousApplicationName, appidOrName), null);
                    }
                }
            }

            return found;
        }

        static bool FindListener(Guid appid, out Guid clsid, out string progid)
        {
            clsid = Guid.Empty;
            progid = null;

            ICatalogObject appObj = null;
            ICatalogCollection appColl = null;
            if (!FindApplication(appid.ToString("B"), out appObj, out appColl))
            {
                throw Tool.CreateException(SR.GetString(SR.ApplicationNotFound, appid.ToString("B")), null);
            }

            ICatalogCollection comps = (ICatalogCollection)appColl.GetCollection(CollectionName.Components, appObj.Key());
            comps.Populate();
            for (int i = 0; i < comps.Count(); i++)
            {
                ICatalogObject compObj = (ICatalogObject)comps.Item(i);
                if (IsListenerComponent(compObj))
                {
                    clsid = new Guid((string)compObj.Key());
                    progid = (string)compObj.Name();
                    return true;
                }
            }

            return false;
        }


        static bool SetComponentProperty(string appIdOrName, string compIdOrName, string property, object value)
        {
            ICatalogObject appObj = null;
            ICatalogCollection appColl = null;

            if (!FindApplication(appIdOrName, out appObj, out appColl))
            {
                throw Tool.CreateException(SR.GetString(SR.ApplicationNotFound, appIdOrName), null);
            }

            ICatalogCollection comps = (ICatalogCollection)appColl.GetCollection(CollectionName.Components, appObj.Key());
            comps.Populate();

            compIdOrName = compIdOrName.ToLowerInvariant(); //make compName lowercase

            for (int i = 0; i < comps.Count(); i++)
            {
                ICatalogObject compObj = (ICatalogObject)comps.Item(i);
                string name = ((string)compObj.Name()).ToLowerInvariant(); //make name lowercase
                string id = ((string)compObj.Key()).ToLowerInvariant(); //make key lowercase

                if (name == compIdOrName || id == compIdOrName)
                {
                    compObj.SetValue(property, value);
                    comps.SaveChanges();
                    return true;
                }
            }

            return false;
        }


        public static ComAdminAppInfo GetAppInfo(string appidOrName)
        {
            ICatalogObject appObj = null;
            ICatalogCollection appColl = null;
            if (!FindApplication(appidOrName, out appObj, out appColl))
            {
                return null;
            }

            ComAdminAppInfo appInfo = null;
            try
            {
                appInfo = new ComAdminAppInfo(appObj, appColl);
            }
            catch (COMException ex)
            {
                ToolConsole.WriteWarning(SR.GetString(SR.FailedToFetchApplicationInformationFromCatalog, appidOrName, ex.ErrorCode, ex.Message));
            }
            return appInfo;
        }


        public static Guid[] GetApplicationIds()
        {
            ICatalog2 catalog = GetCatalog();
            List<Guid> appIds = new List<Guid>();

            ICatalogCollection partitions = (ICatalogCollection)(catalog.GetCollection(CollectionName.Partitions));
            partitions.Populate();

            for (int i = 0; i < partitions.Count(); i++)
            {
                ICatalogObject partition = (ICatalogObject)(partitions.Item(i));
                ICatalogCollection applications = (ICatalogCollection)(partitions.GetCollection(CollectionName.Applications, partition.Key()));
                applications.Populate();

                for (int j = 0; j < applications.Count(); j++)
                {
                    ICatalogObject obj = (ICatalogObject)(applications.Item(j));
                    appIds.Add(new Guid((string)obj.Key()));
                }
            }
            return appIds.ToArray();
        }

        static ICatalog2 GetCatalog()
        {
            return (ICatalog2)(new xCatalog());
        }

        public static string GetPartitionIdForApplication(Guid appId)
        {
            ICatalog2 catalog = GetCatalog();

            return GetPartitionIdForApplication(catalog, appId.ToString("B"), true);
        }

        public static string GetGlobalPartitionID()
        {
            ICatalog2 catalog = GetCatalog();
            return catalog.GlobalPartitionID();
        }

        static string GetPartitionIdForApplication(ICatalog2 catalog, string appId, bool notThrow)
        {
            string partitionId = null;
            try
            {
                partitionId = catalog.GetPartitionID(appId);
            }
            catch (COMException e)
            {
                if (!notThrow)
                    throw Tool.CreateException(SR.GetString(SR.CouldNotGetPartition), e);
                else if (e.ErrorCode == HR.COMADMIN_E_OBJECT_DOES_NOT_EXIST)
                    ToolConsole.WriteWarning(SR.GetString(SR.ApplicationNotFound, appId));
                else
                    ToolConsole.WriteWarning(SR.GetString(SR.CouldnotGetPartitionForApplication, appId, e.ErrorCode, e.Message));
                return null;
            }
            return partitionId;
        }


        public static bool GetApplicationBitness(ICatalog2 catalog, string partitionID, string applicationID)
        {
            ICatalogCollection partitions = (ICatalogCollection)(catalog.GetCollection(CollectionName.Partitions));
            ICatalogCollection applications = (ICatalogCollection)(partitions.GetCollection(CollectionName.Applications, partitionID));
            applications.Populate();
            ICatalogCollection components = (ICatalogCollection)applications.GetCollection(CollectionName.Components, applicationID);
            try
            {
                components.Populate();
            }
            catch (Exception ex)
            {
                if (ex is NullReferenceException || ex is SEHException)
                {
                    throw ex;
                }

                throw Tool.CreateException(SR.GetString(SR.FailedToDetermineTheBitnessOfApplication, applicationID), ex);
            }
            ICatalogObject component = (ICatalogObject)(components.Item(0));
            return IsBitness64bit(component);

        }

        public static string GetAppropriateBitnessModuleModulePath(bool is64bit, RuntimeVersions runtimeVersion)
        {
            if (RuntimeVersions.V40 == runtimeVersion)
            {
                // Retrieve the regkey with Read permission. Note that on Windows 8 and later, only Trusted installer has write access to this key.
                using (RegistryHandle regKey = RegistryHandle.GetCorrectBitnessHKLMSubkey(is64bit, ServiceModelInstallStrings.WinFXRegistryKey, false))
                {
                    return (regKey.GetStringValue(ServiceModelInstallStrings.RuntimeInstallPathName).TrimEnd('\0') + "\\" + fileName);
                }
            }
            else
            {
                // Try to find the 3.0 version
                RegistryHandle regkey = null;
                try
                {
                    if (SafeNativeMethods.ERROR_SUCCESS == RegistryHandle.TryGetCorrectBitnessHKLMSubkey(is64bit, Wcf30RegistryKey, out regkey))
                    {
                        return (regkey.GetStringValue(Runtime30InstallPathName).TrimEnd('\0') + "\\" + fileName);
                    }
                    else
                    {
                        //We don't want to automatically roll forward to 4.0, so we throw an exception if we can't find the 3.0 reg key
                        throw Tool.CreateException(SR.GetString(SR.FailedToGetRegistryKey, Wcf30RegistryKey, "3.0"), null);
                    }
                }
                finally
                {
                    if (regkey != null)
                    {
                        regkey.Dispose();
                    }
                }
            }
        }

        public static void CreateTypeLib(String fileName, Guid clsid)
        {
            try
            {
                ICreateTypeLib typelib = SafeNativeMethods.CreateTypeLib(fileName);
                typelib.SetGuid(Guid.NewGuid());
                typelib.SetName(ListenerWSUName);
                typelib.SetDocString(ListenerWSUName);
                ICreateTypeInfo typeInfo = typelib.CreateTypeInfo(ListenerWSUName, System.Runtime.InteropServices.ComTypes.TYPEKIND.TKIND_COCLASS);
                typeInfo.SetGuid(clsid);
                typeInfo.SetDocString(ListenerWSUName);
                ICreateTypeInfo2 typeInfo2 = (ICreateTypeInfo2)typeInfo;
                typeInfo2.SetName(ListenerWSUName + "Component");
                typeInfo2.SetTypeFlags(2);
                typeInfo.LayOut();
                typelib.SaveAllChanges();
            }
            catch (Exception ex)
            {
                if (ex is NullReferenceException || ex is SEHException)
                {
                    throw ex;
                }

                throw Tool.CreateException(SR.GetString(SR.FailedToCreateTypeLibrary), ex);
            }
        }
        public static void CreateRegistryKey(bool is64bit, Guid clsid, string module)
        {
            using (RegistryHandle regKey = RegistryHandle.GetBitnessHKCR(is64bit))
            {
                using (RegistryHandle clsidKey = regKey.CreateSubKey(@"clsid\" + clsid.ToString("B")))
                {
                    clsidKey.SetValue("", ListenerWSUName);
                    using (RegistryHandle inprocServer32Key = clsidKey.CreateSubKey("InprocServer32"))
                    {
                        inprocServer32Key.SetValue("", module);
                        inprocServer32Key.SetValue("ThreadingModel", "Both");
                    }
                    using (RegistryHandle progID = clsidKey.CreateSubKey("ProgID"))
                    {
                        progID.SetValue("", ListenerWSUName);
                    }
                }
            }
        }

        public static bool IsApplicationWow(Guid appid)
        {
            if (IntPtr.Size == 8)
            {
                string application = appid.ToString("B");
                string partitionId = null;


                ICatalog2 catalog = GetCatalog();
                partitionId = GetPartitionIdForApplication(catalog, application, false);

                // Search for the listener in this partition..
                bool is64bit = GetApplicationBitness(catalog, partitionId, application);

                return !is64bit;
            }
            else
                return false;
        }

        public static void InstallListener(Guid appid, string path, RuntimeVersions runtimeVersion)
        {
            string application = appid.ToString("B");
            string partitionId = null;


            ICatalog2 catalog = GetCatalog();
            partitionId = GetPartitionIdForApplication(catalog, application, false);

            // Search for the listener in this partition..
            bool is64bit = GetApplicationBitness(catalog, partitionId, application);
            Guid clsidVal = Guid.NewGuid();
            string clsid = clsidVal.ToString("B");
            string tlb = Path.Combine(path, application + "." + clsid + ".tlb");
            try
            {
                // No other listener in this partition, we're the first - install using RegistrationHelper
                AtomicFile.SafeDeleteFile(tlb);
                string modulePath = GetAppropriateBitnessModuleModulePath(is64bit, runtimeVersion);
                if (string.IsNullOrEmpty(modulePath))
                    throw Tool.CreateException(SR.GetString(SR.CannotFindServiceInitializerModuleInRegistry), null);
                CreateTypeLib(tlb, clsidVal);
                CreateRegistryKey(is64bit, clsidVal, modulePath);
                catalog.InstallComponent(application, modulePath, tlb, null);
                MarkComponentAsPrivate(catalog, partitionId, application, ListenerWSUName);
                if (!SetComponentProperty(application, ListenerWSUName, PropertyName.Description, ListenerComponentDescription))
                    ToolConsole.WriteWarning(SR.GetString(SR.CannotSetComponentDescription, clsid, appid.ToString("B")));
                if (!SetComponentProperty(application, ListenerWSUName, PropertyName.InitializesServerApplication, "1"))
                    ToolConsole.WriteWarning(SR.GetString(SR.CannotSetComponentInitializerProperty, ListenerWSUName, appid.ToString("B")));
                if (!SetComponentProperty(application, ListenerWSUName, PropertyName.ComponentAccessChecksEnabled, "0"))
                    ToolConsole.WriteWarning(SR.GetString(SR.CannotDisableAccessChecksOnInitializer, ListenerWSUName, appid.ToString("B")));
            }
            catch (Exception ex)
            {
                if (ex is NullReferenceException || ex is SEHException)
                {
                    throw ex;
                }

                throw Tool.CreateException(SR.GetString(SR.CouldNotInstallListener), ex);
            }
            finally
            {
                AtomicFile.SafeDeleteFile(tlb);

            }
        }
        static void MarkComponentAsPrivate(ICatalog2 catalog, string partitionID, string applicationID, string progid)
        {
            ICatalogCollection partitions = (ICatalogCollection)(catalog.GetCollection(CollectionName.Partitions));
            ICatalogCollection applications = (ICatalogCollection)(partitions.GetCollection(CollectionName.Applications, partitionID));
            applications.Populate();
            ICatalogCollection components = (ICatalogCollection)applications.GetCollection(CollectionName.Components, applicationID);
            try
            {
                components.Populate();
                for (int j = 0; j < components.Count(); j++)
                {
                    ICatalogObject component = (ICatalogObject)(components.Item(j));
                    if ((string)component.Name() == progid)
                    {
                        component.SetValue(PropertyName.IsPrivateComponent, true);
                        components.SaveChanges();
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                if (ex is NullReferenceException || ex is SEHException)
                {
                    throw ex;
                }

                ToolConsole.WriteWarning(SR.GetString(SR.FailedToMarkListenerComponentAsPrivateForApplication, progid, applicationID));
            }

        }

        static bool IsBitness64bit(ICatalogObject component)
        {
            int bitness = (int)component.GetValue(PropertyName.Bitness);
            if (bitness == 1)
            {
                return false;
            }
            else
            {
                return true;
            }
        }


        internal static bool IsListenerComponent(ICatalogObject compObj)
        {
            string compName = (string)compObj.Name();
            if (compName.ToUpperInvariant() == ListenerWSUName.ToUpperInvariant())
                return true;
            return false;

        }

        static void RemoveClsidFromRegistry(bool is64bit, string clsid)
        {
            RegistryHandle regKey = RegistryHandle.GetBitnessHKCR(is64bit);
            string baseKey = "Clsid\\" + clsid;
            regKey.DeleteKey(baseKey + "\\InprocServer32");
            regKey.DeleteKey(baseKey + "\\ProgID");
            regKey.DeleteKey(baseKey);
        }
        // returns true if deleted
        static bool RemoveComponent(ICatalog2 catalog, string partitionId, string applicationId, string progid)
        {
            int deleteIndex = -1;
            ICatalogCollection partitions = (ICatalogCollection)catalog.GetCollection(CollectionName.Partitions);
            partitions.Populate();
            ICatalogCollection applications = (ICatalogCollection)(partitions.GetCollection(CollectionName.Applications, partitionId));
            applications.Populate();
            ICatalogCollection components = (ICatalogCollection)(applications.GetCollection(CollectionName.Components, applicationId));
            try
            {
                components.Populate();
                bool is64bit = false;
                string clsid = null;
                for (int i = 0; i < components.Count(); i++)
                {
                    ICatalogObject comp = (ICatalogObject)components.Item(i);
                    if (((string)comp.Name()).ToLowerInvariant() == progid.ToLowerInvariant())
                    {
                        clsid = ((string)comp.Key()).ToLowerInvariant();
                        is64bit = IsBitness64bit(comp);
                        deleteIndex = i;
                        break;
                    }
                }
                if (deleteIndex == -1)
                {
                    return false;
                }

                components.Remove(deleteIndex);
                components.SaveChanges();
                RemoveClsidFromRegistry(is64bit, clsid);

            }
            catch (Exception e)
            {
                if (e is NullReferenceException || e is SEHException)
                {
                    throw e;
                }
                ToolConsole.WriteWarning(SR.GetString(SR.FailedToRemoveListenerComponentFromApplication, applicationId, progid));
            }
            return true;
        }


        public static bool RemoveListener(Guid appid)
        {
            string application = appid.ToString("B");
            string partitionId = null;
            Guid listenerClsid;
            string listenerProgid;

            ICatalog2 catalog = GetCatalog();
            partitionId = GetPartitionIdForApplication(catalog, application, false);
            bool is64bit = GetApplicationBitness(catalog, partitionId, application);
            if (FindListener(appid, out listenerClsid, out listenerProgid))
                return RemoveComponent(catalog, partitionId, application, listenerProgid);
            return false;

        }



        public static bool ResolveApplicationId(string appidOrName, out Guid appId)
        {
            ICatalogObject appObj = null;
            ICatalogCollection appColl = null;
            appId = Guid.Empty;
            if (!FindApplication(appidOrName, out appObj, out appColl))
            {
                return false;
            }

            appId = new Guid((string)appObj.Key());
            return true;
        }

        static void SetCurrentPartition(ICatalog2 catalog, string partitionId)
        {
            try
            {
                catalog.CurrentPartition(partitionId);
            }
            catch (Exception e)
            {
                if (e is NullReferenceException || e is SEHException)
                {
                    throw e;
                }
                throw Tool.CreateException(SR.GetString(SR.CouldNotSetPartition), e);
            }
        }

        public static void SetAppDir(string appidOrName, string path)
        {
            ICatalogObject appObj = null;
            ICatalogCollection appColl = null;
            if (!FindApplication(appidOrName, out appObj, out appColl))
            {
                throw Tool.CreateException(SR.GetString(SR.ApplicationNotFound, appidOrName), null);
            }

            appObj.SetValue(PropertyName.ApplicationDirectory, path);
            appColl.SaveChanges();
        }
    }

    class ComAdminAppInfo
    {
        string appdir;
        Guid appid;
        string appname;
        bool serverActivated;
        bool systemApplication;
        bool processPooled;
        bool automaticRecycling;
        bool listenerExists;
        List<ComAdminClassInfo> classes;
        RuntimeVersions runtimeVersion;

        static Guid CLSID_CLRMetaHost = new Guid("{9280188d-0e8e-4867-b30c-7fa83884e8de}");
        static Guid CLSID_ServiceInitializer = new Guid("{59856830-3ECB-4D29-9CFE-DDD0F74B96A2}");
        static List<Version> CLRVersions = new List<Version>(2) { new Version("2.0"), new Version("4.0") };

        public string ApplicationDirectory { get { return this.appdir; } }
        public Guid ID { get { return this.appid; } }
        public string Name { get { return this.appname; } }
        public List<ComAdminClassInfo> Classes { get { return this.classes; } }
        public bool IsServerActivated { get { return this.serverActivated; } }
        public bool IsSystemApplication { get { return this.systemApplication; } }
        public bool IsProcessPooled { get { return this.processPooled; } }
        public bool IsAutomaticRecycling { get { return this.automaticRecycling; } }
        public bool ListenerExists { get { return this.listenerExists; } }
        public RuntimeVersions RuntimeVersion { get { return this.runtimeVersion; } }

        public ComAdminAppInfo(ICatalogObject appObj, ICatalogCollection appColl)
        {
            this.appid = new Guid((string)appObj.Key());
            this.appname = (string)appObj.Name();
            this.appdir = (string)appObj.GetValue(PropertyName.ApplicationDirectory);
            // Note that casting to long would throw an InvalidCastException
            this.serverActivated = ((int)appObj.GetValue(PropertyName.Activation)) == 1;
            this.systemApplication = (bool)appObj.GetValue(PropertyName.IsSystem);
            this.processPooled = ((int)appObj.GetValue(PropertyName.ConcurrentApps)) > 1;
            this.automaticRecycling = (((int)appObj.GetValue(PropertyName.RecycleActivationLimit) > 0) ||
                ((int)appObj.GetValue(PropertyName.RecycleCallLimit) > 0) ||
                ((int)appObj.GetValue(PropertyName.RecycleLifetimeLimit) > 0) ||
                ((int)appObj.GetValue(PropertyName.RecycleMemoryLimit) > 0));

            this.BuildClasses(appObj, appColl);
        }

        bool TryGetVersionFromString(StringBuilder versionStr, out Version version)
        {
            string versionString;
            bool isSuccessful = false;
            version = null;

            if (versionStr[0] == 'v' || versionStr[0] == 'V')
            {
                int strLen = versionStr.Length - 1;
                versionString = versionStr.ToString(1, strLen);
            }
            else
            {
                versionString = versionStr.ToString();
            }

            try
            {
                version = new Version(versionString);
                isSuccessful = true;
            }
            catch (ArgumentException)
            {
            }
            catch (FormatException)
            {
            }
            catch (OverflowException)
            {
            }

            return isSuccessful;
        }

        bool IsCLRVersionInstalled(Version clrVersion)
        {
            object pCLRMetaHost;
            bool isRuntimeVersionDetermined = false;

            if (SafeNativeMethods.ERROR_SUCCESS == SafeNativeMethods.CLRCreateInstance(CLSID_CLRMetaHost, typeof(IClrMetaHost).GUID, out pCLRMetaHost))
            {
                IClrMetaHost metaHost = (IClrMetaHost)pCLRMetaHost;
                IEnumUnknown enumUnknownPtr;

                enumUnknownPtr = metaHost.EnumerateInstalledRuntimes();

                object[] pUnk = new object[1];
                int pceltFetched;

                while (SafeNativeMethods.ERROR_SUCCESS == enumUnknownPtr.Next(1, pUnk, out pceltFetched) && !isRuntimeVersionDetermined)
                {
                    int bufferSize = 256;
                    StringBuilder builder = new StringBuilder(256);

                    IClrRuntimeInfo runtimeInfo = (IClrRuntimeInfo)pUnk[0];
                    runtimeInfo.GetVersionString(builder, ref bufferSize);
                    Version installedClrVersion;

                    if (TryGetVersionFromString(builder, out installedClrVersion))
                    {
                        if (clrVersion.Major == installedClrVersion.Major && clrVersion.Minor == installedClrVersion.Minor)
                        {
                            isRuntimeVersionDetermined = true;
                        }
                    }
                }
            }

            return isRuntimeVersionDetermined;
        }

        bool ValidateCLRVersion(Version clrVersion)
        {
            foreach (Version releasedCLRVersion in CLRVersions)
            {
                if (clrVersion.Major == releasedCLRVersion.Major && clrVersion.Minor == releasedCLRVersion.Minor)
                {
                    return true;
                }
            }
            return false;
        }

        void BuildClasses(ICatalogObject appObj, ICatalogCollection appColl)
        {
            int versionStrSize = 256;
            StringBuilder version = new StringBuilder(256);

            bool isFrameworkVersionSet = false;
            bool isRuntimeVersionSet = false;
            bool isRuntimeVersionInstalled = true;

            int length = 0;
            Version appClrVersion = null;
            this.classes = new List<ComAdminClassInfo>();

            ICatalogCollection comps = (ICatalogCollection)appColl.GetCollection(CollectionName.Components, appObj.Key());
            comps.Populate();

            for (int i = 0; i < comps.Count(); i++)
            {
                ICatalogObject comp = (ICatalogObject)comps.Item(i);
                ComAdminClassInfo classInfo = new ComAdminClassInfo(comp, comps);
                isFrameworkVersionSet = false;

                if (!isRuntimeVersionSet)
                {
                    isFrameworkVersionSet = (SafeNativeMethods.ERROR_SUCCESS == SafeNativeMethods.GetRequestedRuntimeVersionForCLSID(classInfo.Clsid, version, versionStrSize, ref length, 0));
                    if (isFrameworkVersionSet && TryGetVersionFromString(version, out appClrVersion))
                    {
                        if (IsCLRVersionInstalled(appClrVersion))
                        {
                            isRuntimeVersionSet = true;
                        }
                        else if (ValidateCLRVersion(appClrVersion))
                        {
                            // We've found an valid CLR version in the app but that runtime version is not installed
                            isRuntimeVersionSet = true;
                            isRuntimeVersionInstalled = false;
                        }
                    }
                }

                if (ComAdminWrapper.IsListenerComponent(comp))
                {
                    this.listenerExists = true;
                }
                else
                {
                    this.classes.Add(classInfo);
                }
            }

            //Parse the version number we get
            // If the version is V4.0* we are going to register the 4.0 version of ServiceMonikerSupport.dll
            // Anything else we are going to register the 3.0 version of ServiceMonikerSupport.dll
            if (isRuntimeVersionSet && isRuntimeVersionInstalled)
            {
                if (appClrVersion.Major == 4 && appClrVersion.Minor == 0)
                {
                    this.runtimeVersion = RuntimeVersions.V40;
                }
                else if (appClrVersion.Major == 2 && appClrVersion.Minor == 0)
                {
                    this.runtimeVersion = RuntimeVersions.V20;
                }
                else
                {
                    // It is non of the CLR version this tool recognize
                    throw Tool.CreateException(SR.GetString(SR.FailedToGetRuntime, appClrVersion.ToString()), null);
                }
            }
            else if (!isRuntimeVersionInstalled)
            {
                // When we can't find the matching runtime for the user application, throw an application exception
                throw Tool.CreateException(SR.GetString(SR.FailedToGetRuntime, appClrVersion.ToString()), null);
            }
            else
            {
                this.runtimeVersion = RuntimeVersions.V40;
            }
        }

        public ComAdminClassInfo FindClass(string classNameOrGuid)
        {
            ComAdminClassInfo resolvedClassInfo = null;

            classNameOrGuid = classNameOrGuid.ToLowerInvariant();

            foreach (ComAdminClassInfo classInfo in this.classes)
            {
                if ((classInfo.Clsid.ToString("B").ToLowerInvariant() == classNameOrGuid)
                    || classInfo.Name.ToLowerInvariant() == classNameOrGuid)
                {
                    if (resolvedClassInfo == null)
                    {
                        resolvedClassInfo = classInfo;
                    }
                    else
                    {
                        throw Tool.CreateException(SR.GetString(SR.AmbiguousComponentName, classNameOrGuid), null);
                    }
                }
            }

            return resolvedClassInfo;
        }
    }


    class ComAdminClassInfo
    {
        Guid clsid;
        List<ComAdminInterfaceInfo> interfaces;
        bool isPrivate;
        string progid;
        TransactionOption transactionOption;

        public Guid Clsid { get { return this.clsid; } }
        public bool IsPrivate { get { return this.isPrivate; } }
        public string Name { get { return this.progid; } }
        public List<ComAdminInterfaceInfo> Interfaces { get { return this.interfaces; } }
        public bool SupportsTransactionFlow
        {
            get
            {
                if ((transactionOption == TransactionOption.Required) ||
                     (transactionOption == TransactionOption.Supported))
                    return true;
                else
                    return false;
            }
        }


        public ComAdminClassInfo(ICatalogObject compObj, ICatalogCollection compColl)
        {
            this.clsid = new Guid((string)compObj.Key());
            this.progid = (string)compObj.Name();
            this.isPrivate = (bool)compObj.GetValue(PropertyName.IsPrivateComponent);
            this.transactionOption = (TransactionOption)compObj.GetValue(PropertyName.TransactionOption);
            this.BuildInterfaces(compObj, compColl);
        }

        void BuildInterfaces(ICatalogObject compObj, ICatalogCollection compColl)
        {
            this.interfaces = new List<ComAdminInterfaceInfo>();

            ICatalogCollection interfaceColl = (ICatalogCollection)compColl.GetCollection(CollectionName.InterfacesForComponent, compObj.Key());
            interfaceColl.Populate();
            for (int i = 0; i < interfaceColl.Count(); i++)
            {
                ICatalogObject itf = (ICatalogObject)interfaceColl.Item(i);
                Guid interfaceID = new Guid((string)itf.Key());
                ComAdminInterfaceInfo interfaceInfo = new ComAdminInterfaceInfo(interfaceID, (string)itf.Name());
                this.interfaces.Add(interfaceInfo);

            }

        }

        public ComAdminInterfaceInfo FindInterface(string interfaceNameOrGuid)
        {
            ComAdminInterfaceInfo resolvedInterfaceInfo = null;
            interfaceNameOrGuid = interfaceNameOrGuid.ToLowerInvariant();

            foreach (ComAdminInterfaceInfo interfaceInfo in this.interfaces)
            {
                if ((interfaceInfo.Iid.ToString("B").ToLowerInvariant() == interfaceNameOrGuid.ToLowerInvariant())
                    || interfaceInfo.Name.ToLowerInvariant() == interfaceNameOrGuid)
                {
                    if (resolvedInterfaceInfo == null)
                    {
                        resolvedInterfaceInfo = interfaceInfo;
                    }
                    else
                    {
                        throw Tool.CreateException(SR.GetString(SR.AmbiguousInterfaceName, interfaceNameOrGuid), null);
                    }
                }
            }

            return resolvedInterfaceInfo;
        }
    }

    class ComAdminInterfaceInfo
    {
        Guid iid;
        string name;

        public Guid Iid { get { return this.iid; } }
        public string Name { get { return this.name; } }

        public ComAdminInterfaceInfo(Guid iid, string name)
        {
            this.iid = iid;
            this.name = name;
        }
    }
}
