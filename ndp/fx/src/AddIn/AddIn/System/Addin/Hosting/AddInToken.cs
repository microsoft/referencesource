// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInToken
**
** Purpose: Represents a valid combination of add-ins and 
**          associated classes, like host adaptors, etc.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Permissions;
using System.Security;
using System.Text;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;
namespace System.AddIn.Hosting
{
    [Serializable]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Justification="Need approval for API change")]
    public sealed class AddInToken : IEnumerable<QualificationDataItem>
    {
        internal readonly TypeInfo[] _hostAddinViews;
        internal readonly HostAdapter _hostAdapter;
        internal readonly ContractComponent _contract;
        internal readonly AddInAdapter _addinAdapter;
        internal readonly AddInBase _addinBase;
        internal readonly AddIn _addin;
        private String _pipelineRootDir; 
        private String _addInRootDir;
        private TypeInfo _resolvedHostAddinView;

        // Two representations of the qualification data.  Anders says both may be needed.
        private IDictionary<AddInSegmentType, IDictionary<String, String>> _qualificationData;
        private List<QualificationDataItem> _qualificationDataItems;

        private static volatile bool _enableDirectConnect;

        public static bool EnableDirectConnect
        {
            get { return _enableDirectConnect; }
            set { _enableDirectConnect = value; }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures", Justification = "This read only collection is easy to use.")]
        public IDictionary<AddInSegmentType, IDictionary<String, String>> QualificationData
        {
            get
            {
                if (_qualificationData == null)
                {
                    Dictionary<AddInSegmentType, IDictionary<String, String>> dictionary =
                        new Dictionary<AddInSegmentType, IDictionary<String, String>>();

                    dictionary[AddInSegmentType.HostViewOfAddIn] = PipelineComponent.s_emptyDictionary;
                    dictionary[AddInSegmentType.HostSideAdapter] = _hostAdapter.QualificationData;

                    dictionary[AddInSegmentType.Contract] = _contract.QualificationData;
                    dictionary[AddInSegmentType.AddInSideAdapter] = _addinAdapter.QualificationData;
                    dictionary[AddInSegmentType.AddInView] = _addinBase.QualificationData;

                    dictionary[AddInSegmentType.AddIn] = _addin.QualificationData;

                    _qualificationData = new ReadOnlyDictionary<AddInSegmentType, IDictionary<String, String>>(dictionary);
                }
                return _qualificationData;
            }
        }

        // Needed for IEnumerable interface
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() 
        { 
            return GetEnumerator(); 
        }

        // Useful for LINQ queries
        public IEnumerator<QualificationDataItem> GetEnumerator()
        {
            if (_qualificationDataItems == null)
            {
                _qualificationDataItems = new List<QualificationDataItem>();
                for (AddInSegmentType t = AddInSegmentType.HostSideAdapter; t <= AddInSegmentType.AddIn; t++)
                {
                    IDictionary<String, String> pairs = QualificationData[t];
                    foreach (KeyValuePair<String, String> pair in pairs)
                    {
                        QualificationDataItem item = new QualificationDataItem(t, pair.Key, pair.Value);
                        _qualificationDataItems.Add(item);
                    }
                }
            }
            return _qualificationDataItems.GetEnumerator();
        }

        internal bool HasQualificationDataOnPipeline
        {
            get {
                for (AddInSegmentType t = AddInSegmentType.HostSideAdapter; t <= AddInSegmentType.AddInView; t++)
                {
                    IDictionary<String, String> pairs = QualificationData[t];
                    if (pairs.Count != 0)
                        return true;
                }
                return false;
            }
        }

        // The FullName of the addin type, not the "name" specified in the attribute.
        // This may be used to uniquely identify the addin.
        public String AddInFullName
        {
            get { return _addin.FullName; }
        }

        internal AddInToken(HostAdapter hostAdapter, ContractComponent contract,
            AddInAdapter addinAdapter, AddInBase addinBase, AddIn addin)
        {
            System.Diagnostics.Contracts.Contract.Requires(hostAdapter != null);
            System.Diagnostics.Contracts.Contract.Requires(contract != null);
            System.Diagnostics.Contracts.Contract.Requires(addinAdapter != null);
            System.Diagnostics.Contracts.Contract.Requires(addinBase != null);
            System.Diagnostics.Contracts.Contract.Requires(addin != null);

            _hostAddinViews = hostAdapter.HostAddinViews;
            _hostAdapter = hostAdapter;
            _contract = contract;
            _addinAdapter = addinAdapter;
            _addinBase = addinBase;
            _addin = addin;

            // _pipelineRootDir must be filled in after deserialization.
        }

        internal TypeInfo ResolvedHostAddInView
        {
            /* 
            [Pure]
                get { return _resolvedHostAddinView; }
            */

            set
            {
                System.Diagnostics.Contracts.Contract.Requires(value != null);
                System.Diagnostics.Contracts.Contract.Assert(AddInStore.Contains(_hostAddinViews, value));

                _resolvedHostAddinView = value;                
            }
        }

        internal String PipelineRootDirectory {
            [Pure]
            get { return _pipelineRootDir; }

            set {
                System.Diagnostics.Contracts.Contract.Requires(value != null);
                _pipelineRootDir = value;
                // Update the paths for each add-in model component.
                _hostAdapter.SetRootDirectory(_pipelineRootDir);
                _contract.SetRootDirectory(_pipelineRootDir);
                _addinAdapter.SetRootDirectory(_pipelineRootDir);
                _addinBase.SetRootDirectory(_pipelineRootDir);
            }
        }

        internal String AddInRootDirectory {
            set {
                System.Diagnostics.Contracts.Contract.Requires(value != null);

                _addInRootDir = value;
                _addin.SetRootDirectory(_addInRootDir);
            }
        }

        public String Name
        {
            get { return _addin.AddInName; }
        }

        public String Publisher
        {
            get { return _addin.Publisher; }
        }

        public String Version
        {
            get { return _addin.Version; }
        }

        public String Description
        {
            get { return _addin.Description; }
        }

        public override String ToString()
        {
            return _addin.AddInName;
        }

        public AssemblyName AssemblyName
        {
            get {
                return _addin.AssemblyName;
            }
        }

        internal TypeInfo[] HostAddinViews {
            get { return _hostAddinViews; }
        }

        internal String HostViewId
        {
            get { return _resolvedHostAddinView.FullName + " " + _addin.RelativeLocation + " " + _addin.TypeInfo.FullName; }
        }

        // base identity on the addin itself.  This way, if a host
        // asks for addins for multiple HAV's, he can easily identify
        // addins that match for more than one HAV.
        public override bool Equals(object obj)
        {
            AddInToken thatToken = obj as AddInToken;
            if (thatToken != null)
            {
                return this.Equals(thatToken);
            }
            return false;
        }

        bool Equals(AddInToken addInToken)
        {
            // perf optimization.  Check pointers.
            if (Object.ReferenceEquals(this, addInToken))
                return true;
            
            if (_hostAdapter.TypeInfo.AssemblyQualifiedName  == addInToken._hostAdapter.TypeInfo.AssemblyQualifiedName &&
                _contract.TypeInfo.AssemblyQualifiedName     == addInToken._contract.TypeInfo.AssemblyQualifiedName &&
                _addinAdapter.TypeInfo.AssemblyQualifiedName == addInToken._addinAdapter.TypeInfo.AssemblyQualifiedName &&
                _addinBase.TypeInfo.AssemblyQualifiedName    == addInToken._addinBase.TypeInfo.AssemblyQualifiedName &&
                _addin.TypeInfo.AssemblyQualifiedName        == addInToken._addin.TypeInfo.AssemblyQualifiedName)
                return true;

            return false;
        }

        public override int GetHashCode()
        {
            // the TypeInfos base their hash codes on their AssemblyQualifiedNames, so we can use those
            return unchecked(_hostAdapter.TypeInfo.GetHashCode() + _contract.TypeInfo.GetHashCode() +
                    _addinAdapter.TypeInfo.GetHashCode() + _addinBase.TypeInfo.GetHashCode() +
                    _addin.TypeInfo.GetHashCode());
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Factory Method")]
        public T Activate<T>(AddInSecurityLevel trustLevel)
        {
            return AddInActivator.Activate<T>(this, trustLevel);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Factory Method")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="appDomain")]
        public T Activate<T>(AddInSecurityLevel trustLevel, String appDomainName)
        {
            return AddInActivator.Activate<T>(this, trustLevel, appDomainName);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Factory Method")]
        public T Activate<T>(AppDomain target)
        {
            if (target != AppDomain.CurrentDomain && !Utils.HasFullTrust())
            {
                throw new SecurityException(Res.PartialTrustCannotActivate); 
            }

            return AddInActivator.Activate<T>(this, target);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter")]
        public T Activate<T>(PermissionSet permissions)
        {
            if (permissions == null)
                throw new ArgumentNullException("permissions");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            return AddInActivator.Activate<T>(this, permissions);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter")]
        [PermissionSet(SecurityAction.Demand, Name="FullTrust")]
        [SecuritySafeCritical]
        public T Activate<T>(AddInProcess process, PermissionSet permissionSet)
        {
            return AddInActivator.Activate<T>(this, process, permissionSet);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification="Factory Method")]
        [PermissionSet(SecurityAction.Demand, Name="FullTrust")]  
        [SecuritySafeCritical]
        public T Activate<T>(AddInProcess process, AddInSecurityLevel level)
        {
            return AddInActivator.Activate<T>(this, process, level);
        }

        // no full-trust demand here because the environment may be local
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification="Factory method")]
        public T Activate<T>(AddInEnvironment environment)
        {   
            return AddInActivator.Activate<T>(this, environment);
        }

        // The loader may load assemblies from the wrong place in certain cases.
        // We need to warn people.
        // 1. Contracts is not in AddInAdapter directory
        // 2. AddInView is not in AddInAdapter directory
        // 3. AddInView is not in AddIn directory
        // 4. Contracts is not in HostSideAdapter directory
        //
        // At discovery time, rootDir is passed in.
        // At activation time, it is not needed.
        internal bool HasDuplicatedAssemblies(String rootDir, Collection<String> warnings)
        {
            PipelineComponent[] componentsAndDependents = new PipelineComponent[] {
                _contract,      _addinAdapter,
                _addinBase,     _addinAdapter,
                _addinBase,     _addin,
                _contract,      _hostAdapter};

            bool duplicates = false;
            for(int i = 0; i < componentsAndDependents.Length; i+=2)
            {
                if (ComponentInWrongLocation(componentsAndDependents[i], componentsAndDependents[i+1], rootDir, warnings))
                    duplicates = true;
            }
            return duplicates;
        }

        private bool ComponentInWrongLocation(PipelineComponent component, PipelineComponent dependentComponent, 
                String rootDir, Collection<String> warnings)
        {
            System.Diagnostics.Contracts.Contract.Requires(rootDir != null || PipelineRootDirectory != null);
            string dependentPath = rootDir == null ? dependentComponent.Location : Path.Combine(rootDir, dependentComponent.Location);

            String fileName = Path.GetFileName(component.Location);
            String location = Path.GetDirectoryName(dependentPath);

            if (File.Exists(Path.Combine(location, fileName)))
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.ComponentInWrongLocation, fileName, location));
                return true;
            }
            return false;
        }
    }

}

