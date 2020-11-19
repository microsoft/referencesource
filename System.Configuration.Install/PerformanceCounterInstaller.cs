//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.Configuration.Install;
    using System.Globalization;
    
    /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PerformanceCounterInstaller : ComponentInstaller {

        private const string ServicePath = "SYSTEM\\CurrentControlSet\\Services";                
        private const string PerfShimName = "netfxperf.dll";
        private const string PerfShimFullNameSuffix = @"\netfxperf.dll";
        private string categoryName = string.Empty;
        private CounterCreationDataCollection counters = new CounterCreationDataCollection();
        private string categoryHelp = string.Empty;
        private UninstallAction uninstallAction = System.Configuration.Install.UninstallAction.Remove;
        private PerformanceCounterCategoryType categoryType = PerformanceCounterCategoryType.Unknown;

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.CategoryName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        ResDescription(Res.PCCategoryName),        
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]                                                      
        public string CategoryName {
            get {
                return categoryName;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                CheckValidCategory(value);
                
                categoryName = value;
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.CategoryHelp"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(""),
        ResDescription(Res.PCI_CategoryHelp)
        ]
        public string CategoryHelp {
            get {
                return categoryHelp;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                categoryHelp = value;
            }
        }

        [
        DefaultValue(PerformanceCounterCategoryType.Unknown),
        ResDescription(Res.PCI_IsMultiInstance),
        ComVisible(false)
        ]
        public PerformanceCounterCategoryType CategoryType {
            get {
                return categoryType;
            }
            set {
                if (value < PerformanceCounterCategoryType.Unknown || value > PerformanceCounterCategoryType.MultiInstance)
                    throw new InvalidEnumArgumentException("value", (int) value, typeof(PerformanceCounterCategoryType));
                
                categoryType = value;
            }
        }
        
        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Counters"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Content), 
        ResDescription(Res.PCI_Counters)
        ]
        public CounterCreationDataCollection Counters {
            get {
                return counters;
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.UninstallAction"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        DefaultValue(UninstallAction.Remove),
        ResDescription(Res.PCI_UninstallAction)
        ]
        public UninstallAction UninstallAction {
            get {
                return uninstallAction;
            }
            set {
                if (!Enum.IsDefined(typeof(UninstallAction), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(UninstallAction));
            
                uninstallAction = value;
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void CopyFromComponent(IComponent component) {
            if (!(component is PerformanceCounter))
                throw new ArgumentException(Res.GetString(Res.NotAPerformanceCounter));

            PerformanceCounter counter = (PerformanceCounter) component;

            if (counter.CategoryName == null || counter.CategoryName.Length == 0)
                throw new ArgumentException(Res.GetString(Res.IncompletePerformanceCounter));

            if (!CategoryName.Equals(counter.CategoryName) && !String.IsNullOrEmpty(CategoryName)) 
                throw new ArgumentException(Res.GetString(Res.NewCategory));

            PerformanceCounterType counterType = PerformanceCounterType.NumberOfItems32;
            string counterHelp = string.Empty;
            if (String.IsNullOrEmpty(CategoryName)) {
                CategoryName = counter.CategoryName;
            }
            if (Environment.OSVersion.Platform == PlatformID.Win32NT) {              
                string machineName = counter.MachineName;
                    
                if (PerformanceCounterCategory.Exists(CategoryName, machineName)) {
                    string keyPath = ServicePath + "\\" + CategoryName + "\\Performance";       
                    RegistryKey key = null;
                    try {                 
                        if (machineName == "." || String.Compare(machineName, SystemInformation.ComputerName, StringComparison.OrdinalIgnoreCase) == 0) {
                            key = Registry.LocalMachine.OpenSubKey(keyPath);                                                    
                        }                        
                        else {
                            RegistryKey baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, "\\\\" + machineName);                    
                            key =  baseKey.OpenSubKey(keyPath);                                                    
                        }   
                                    
                        if (key == null) 
                            throw new ArgumentException(Res.GetString(Res.NotCustomPerformanceCategory));
                            
                        object systemDllName = key.GetValue("Library", null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                        if (systemDllName == null || !(systemDllName is string) 
                            || (String.Compare((string)systemDllName, PerfShimName, StringComparison.OrdinalIgnoreCase) != 0)
                            && !((string)systemDllName).EndsWith(PerfShimFullNameSuffix, StringComparison.OrdinalIgnoreCase))
                            throw new ArgumentException(Res.GetString(Res.NotCustomPerformanceCategory));

                        PerformanceCounterCategory pcat = new PerformanceCounterCategory(CategoryName, machineName);                        
                        CategoryHelp = pcat.CategoryHelp;
                        if (pcat.CounterExists(counter.CounterName)) {                                                                                        
                            counterType = counter.CounterType;
                            counterHelp = counter.CounterHelp;
                        }

                        CategoryType = pcat.CategoryType;
                    }
                    finally {
                        if (key != null)
                            key.Close();
                    }                        
                }
            }                    

            CounterCreationData data = new CounterCreationData(counter.CounterName, counterHelp, counterType);
            Counters.Add(data);
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.DoRollback"]/*' />
        /// <devdoc>
        /// Restores the registry to how it was before install, as saved in the state dictionary.
        /// </devdoc>
        private void DoRollback(IDictionary state) {
            //we need to remove the category key OR put old data back into the key
            Context.LogMessage(Res.GetString(Res.RestoringPerformanceCounter, CategoryName));

            using (RegistryKey servicesKey = Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Services", true)) {
                RegistryKey categoryKey = null;
            
                if ((bool) state["categoryKeyExisted"]) {
                    categoryKey = servicesKey.OpenSubKey(CategoryName, true);

                    if (categoryKey == null)
                        categoryKey = servicesKey.CreateSubKey(CategoryName);
                        
                    categoryKey.DeleteSubKeyTree("Performance");
                    SerializableRegistryKey performanceKeyData = (SerializableRegistryKey) state["performanceKeyData"];
                    if (performanceKeyData != null) {
                        RegistryKey performanceKey = categoryKey.CreateSubKey("Performance");
                        performanceKeyData.CopyToRegistry(performanceKey);
                        performanceKey.Close();
                    }
                    
                    categoryKey.DeleteSubKeyTree("Linkage");
                    SerializableRegistryKey linkageKeyData = (SerializableRegistryKey) state["linkageKeyData"];
                    if (linkageKeyData != null) {
                        RegistryKey linkageKey = categoryKey.CreateSubKey("Linkage");
                        linkageKeyData.CopyToRegistry(linkageKey);
                        linkageKey.Close();
                    }
                }
                else {
                    categoryKey = servicesKey.OpenSubKey(CategoryName);
                    if (categoryKey != null) {
                        categoryKey.Close();
                        categoryKey = null;
                        servicesKey.DeleteSubKeyTree(CategoryName);
                    }
                }
                if (categoryKey != null )
                    categoryKey.Close();
            }
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Install(IDictionary stateSaver) {
            base.Install(stateSaver);

            Context.LogMessage(Res.GetString(Res.CreatingPerformanceCounter, CategoryName));

            RegistryKey categoryKey = null;
            RegistryKey subKey = null;
            RegistryKey servicesKey = Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Services", true);
            stateSaver["categoryKeyExisted"] = false;

            // save the old values of the Performance and Linkage subkeys in case we need to rollback the install
            try {
                if (servicesKey != null) {
                    
                    categoryKey = servicesKey.OpenSubKey(CategoryName, true);
                    if (categoryKey != null) {
                        stateSaver["categoryKeyExisted"] = true;

                        subKey = categoryKey.OpenSubKey("Performance");
                        if (subKey != null) {
                            stateSaver["performanceKeyData"] = new SerializableRegistryKey(subKey);
                            subKey.Close();
                            categoryKey.DeleteSubKeyTree("Performance");
                        }

                        subKey = categoryKey.OpenSubKey("Linkage");
                        if (subKey != null) {
                            stateSaver["linkageKeyData"] = new SerializableRegistryKey(subKey);
                            subKey.Close();
                            categoryKey.DeleteSubKeyTree("Linkage");
                        }
                    }
                }
            }
            finally {
                if (categoryKey != null) categoryKey.Close();
                if (servicesKey != null) servicesKey.Close();
            }

            // delete the "Performance" and "Linkage" subkeys, unregister the category,
            // and finally re-create the category from scratch.
            if (PerformanceCounterCategory.Exists(CategoryName)) {
                PerformanceCounterCategory.Delete(CategoryName);  
            }
            PerformanceCounterCategory.Create(CategoryName, CategoryHelp, categoryType, Counters);
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Rollback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Rollback(IDictionary savedState) {
            base.Rollback(savedState);

            DoRollback(savedState);
        }

        /// <include file='doc\PerformanceCounterInstaller.uex' path='docs/doc[@for="PerformanceCounterInstaller.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            base.Uninstall(savedState);

            if (UninstallAction == UninstallAction.Remove) {
                Context.LogMessage(Res.GetString(Res.RemovingPerformanceCounter, CategoryName));
                PerformanceCounterCategory.Delete(CategoryName);
            }
        }

        // CheckValidCategory and CheckValidId were copied from PerformanceCounterCategory.
        internal static void CheckValidCategory(string categoryName) {
            if (categoryName == null)
                throw new ArgumentNullException("categoryName");
        
            if (!CheckValidId(categoryName))
                throw new ArgumentException(Res.GetString(Res.PerfInvalidCategoryName, 1, 253));                                                                                
        }                                                                                         
        
        // CheckValidCategory and CheckValidId were copied from PerformanceCounterCategory.
        internal static bool CheckValidId(string id) {
            if (id.Length == 0 || id.Length > 253)
                return false;
                
            for (int index = 0; index < id.Length; ++index) {
                char current = id[index];
                
                if ((index == 0 || index == (id.Length -1)) && current == ' ')
                    return false;
                    
                if (current == '\"')
                    return false;                    
                
                if (char.IsControl(current))
                    return false;                    
            }
                        
            return true;
        }
    }
}

