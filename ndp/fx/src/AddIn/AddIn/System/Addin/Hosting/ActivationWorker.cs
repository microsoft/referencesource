// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ActivationWorker
**
** Purpose: Created in the remote AppDomain, this worker is 
**     used to start up & shut down the add-in.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security;
using System.Security.Permissions;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    internal sealed class ActivationWorker : MarshalByRefObject, IDisposable
    {
        private AddInToken _pipeline;
        private ResolveEventHandler _assemblyResolver;
        private PipelineComponentType _currentComponentType;
        private bool _usingHostAppDomain;

        internal ActivationWorker(AddInToken pipeline)
        {
            System.Diagnostics.Contracts.Contract.Requires(pipeline != null);
            System.Diagnostics.Contracts.Contract.Requires(pipeline.PipelineRootDirectory != null);

            _pipeline = pipeline;
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="AppDomain.remove_AssemblyResolve(System.ResolveEventHandler):System.Void" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        [System.Security.SecuritySafeCritical]
        public void Dispose()
        {
            if (_assemblyResolver != null) {
                AppDomain.CurrentDomain.AssemblyResolve -= _assemblyResolver;
                _assemblyResolver = null;
            }
        }

        // Don't let this object time out in Remoting.  We need this object to be 
        // alive until we clean up the appdomain, and we want to unhook our assembly 
        // resolve event.
        public override Object InitializeLifetimeService()
        {
            return null;
        }

        internal bool UsingHostAppDomain 
        {
            set { _usingHostAppDomain = value; }
        }

        // This method should return System.AddIn.Contract.IContract, instead of Object.  This gives
        // Remoting half a chance at setting up the transparent proxy to contain
        // the appropriate interface method table.  Then, hopefully Reflection
        // is built to check that interface method table first.  (hopefully.)
        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="AppDomain.add_AssemblyResolve(System.ResolveEventHandler):System.Void" />
        // <Asserts Name="Imperative: System.Security.PermissionSet" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification="Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.Reflection.Assembly.LoadFrom", Justification="LoadFrom was explicitly designed for addin loading")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        [System.Security.SecuritySafeCritical]
        internal System.AddIn.Contract.IContract Activate() 
        {
            // Assert permission to the contracts, AddInSideAdapters, AddInViews and specific Addin directories only.
            PermissionSet permissionSet = new PermissionSet(PermissionState.None);
            permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                Path.Combine(_pipeline.PipelineRootDirectory, AddInStore.ContractsDirName)));
            permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                Path.Combine(_pipeline.PipelineRootDirectory, AddInStore.AddInAdaptersDirName)));
            permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                Path.Combine(_pipeline.PipelineRootDirectory, AddInStore.AddInBasesDirName)));
            permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                Path.GetDirectoryName(_pipeline._addin.Location)));
            permissionSet.Assert();

            // Let's be very deliberate about loading precisely the components we want,
            // instead of relying on an assembly resolve event.  We may still need the 
            // resolve event, but let's ensure we load the right files from the right
            // places in the right loader contexts.
            Assembly.LoadFrom(_pipeline._contract.Location);

            // only load the AddInBase in the LoadFrom context if there is no copy
            // of the assembly loaded already in the Load context.  Otherwise there would be an InvalidCastException
            // when returning it to the host in the single appdomain, non direct-connect scenario, when
            // the HVA assembly is also used as the AddInBase assembly.
            // Since the reflection call to determine whether the assembly is loaded is expensive, only
            // do it when we are in the single-appdomain scenario.
            bool alreadyPresent = false; 
            if (_usingHostAppDomain)
                 alreadyPresent = IsAssemblyLoaded(_pipeline._addinBase._assemblyName);

            if (!alreadyPresent)
                Assembly.LoadFrom(_pipeline._addinBase.Location);

            Assembly addInAssembly = Assembly.LoadFrom(_pipeline._addin.Location);
            Assembly addinAdapterAssembly = Assembly.LoadFrom(_pipeline._addinAdapter.Location);

            CodeAccessPermission.RevertAssert();

            // Create instances of all the interesting objects.
            // Either we don't need this here, or it may need to exist for the duration of 
            // the add-in's lifetime.
            //
            // The assembly resolve event will be removed when the addin 
            // controller's Shutdown method is run.
            _assemblyResolver = new ResolveEventHandler(ResolveAssembly);
            AppDomain.CurrentDomain.AssemblyResolve += _assemblyResolver;

            // Create the AddIn
            _currentComponentType = PipelineComponentType.AddIn;
            Type addinType = addInAssembly.GetType(_pipeline._addin.TypeInfo.FullName, true);
            Object addIn = addinType.GetConstructor(new Type[0]).Invoke(new Object[0]);
            System.Diagnostics.Contracts.Contract.Assert(addIn != null, "CreateInstance didn't create the add-in");

            return CreateAddInAdapter(addIn, addinAdapterAssembly);
        }


        // Return true if the assembly of the given name has already been loaded, perhaps
        // from a different path.  
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static bool IsAssemblyLoaded(String assemblyName)
        {
            // Since there is no managed API for finding the GAC path, I 
            // assert path discovery for all local files.
            FileIOPermission permission = new FileIOPermission(PermissionState.None);
            permission.AllLocalFiles = FileIOPermissionAccess.PathDiscovery;
            permission.Assert();

            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                if (assembly.FullName.Equals(assemblyName, StringComparison.OrdinalIgnoreCase)) {
                    return true;
                }

                // Warn if they have the same assembly with different versions.  If we don't do this,
                // they will get an InvalidCastException instead.
                AssemblyName name1 = new AssemblyName(assemblyName);
                AssemblyName name2 = assembly.GetName();
                if (name1.Name == name2.Name && 
                    name1.CultureInfo.Equals(name2.CultureInfo) &&
                    Utils.PublicKeyMatches(name1, name2) && 
                    name1.Version != name2.Version)
                {
                    throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, Res.IncompatibleAddInBaseAssembly, assemblyName));
                }
            }
            return false;
        }

        //<SecurityKernel Critical="True" Ring="0">
        //<Asserts Name="Imperative: System.Security.Permissions.ReflectionPermission" />
        //</SecurityKernel>
        [System.Security.SecuritySafeCritical]
        internal System.AddIn.Contract.IContract CreateAddInAdapter(Object addIn, Assembly addinAdapterAssembly)
        {
            System.Diagnostics.Contracts.Contract.Ensures(System.Diagnostics.Contracts.Contract.Result<System.AddIn.Contract.IContract>() != null);

            // Create the AddIn Adapter
            System.AddIn.Contract.IContract addInAdapter = null;
            _currentComponentType = PipelineComponentType.AddInAdapter;
            Type adapterType = addinAdapterAssembly.GetType(_pipeline._addinAdapter.TypeInfo.FullName, true);
            Type addInBaseType = Type.GetType(_pipeline._addinBase.TypeInfo.AssemblyQualifiedName, true);

            AddInActivator.InvokerDelegate myInvokerDelegate = AddInActivator.CreateConsInvoker(adapterType, addIn.GetType());
            addInAdapter = (System.AddIn.Contract.IContract)myInvokerDelegate(addIn);

            System.Diagnostics.Contracts.Contract.Assert(addInAdapter != null, "CreateInstance didn't create the add-in adapter");

            return addInAdapter;
        }
        
        // This is necessary if an add-in or an add-in adapter depends on other 
        // assemblies within the same directory.  
        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.PermissionSet" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal Assembly ResolveAssembly(Object sender, ResolveEventArgs args)
        {
            System.Diagnostics.Contracts.Contract.Assert(_pipeline != null);

            String assemblyRef = args.Name;
            //Console.WriteLine("ResolveAssembly (in add-in's AD) called for {0}", assemblyRef);

            // Two purposes here:
            // 1) Ensure that our already-loaded pipeline components get upgraded
            //    from the LoadFrom context to the default loader context.
            // 2) Any dependencies of our already-loaded pipeline components would
            //    have been loaded in the LoadFrom context, and need to also be
            //    manually upgraded to the default loader context.
            // We can do both of the above by calling LoadFrom on the appropriate
            // assemblies on disk, or by walking the list of already-loaded 
            // assemblies, looking for the ones we need to upgrade.
            // Since we'll have multiple add-ins in the same AD, it may make the
            // most sense to simply look for an already-loaded assembly instead of
            // looking in directories on disk, to avoid conflicts.

            // Check to see if this assembly was already loaded in the 
            // LoadFrom context.  If so, upgrade it.
            Assembly a = Utils.FindLoadedAssemblyRef(assemblyRef);
            if (a != null)
                return a;

            // It wasn't found in memory, so look on disk
            String rootDir = _pipeline.PipelineRootDirectory;

            List<String> dirsToLookIn = new List<String>();
            switch (_currentComponentType)
            {
                case PipelineComponentType.AddInAdapter:
                    // Look in contract directory and addin base directory.
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.ContractsDirName));
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.AddInBasesDirName));
                    break;

                case PipelineComponentType.AddIn:
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.AddInBasesDirName));
                    break;

                default:
                    System.Diagnostics.Contracts.Contract.Assert(false);
                    throw new InvalidOperationException("Fell through switch in assembly resolve event!");
            }

            // ARROWHEAD START
            // In the LoadFrom context, assemblies we depend on in the same folder are loaded automatically.
            // We don't have that behavior in Arrowhead.
            //String addinFolder = Path.GetDirectoryName(_pipeline._addin.Location);
            //dirsToLookIn.Add(addinFolder);
            // ARROWHEAD END 

            // Assert permission to read from addinBase folder (and maybe contracts folder).
            PermissionSet permissionSet = new PermissionSet(PermissionState.None);
            foreach (string dir in dirsToLookIn)
            {
                permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, dir));
            }
            permissionSet.Assert();

            return Utils.LoadAssemblyFrom(dirsToLookIn, assemblyRef);
            
            //Console.WriteLine("Couldn't resolve assembly {0} while loading a {1}", simpleName, _currentComponentType);
        }
    }
}

