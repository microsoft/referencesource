// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInActivator
**
** Purpose: Allows you to create an instance of an add-in,
**     in another AppDomain or process.
**
===========================================================*/
using System;
using System.AddIn.Contract;
using System.AddIn.MiniReflection;
using System.AddIn.Pipeline;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Globalization;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.Remoting;
using System.Security;
using System.Security.Permissions;
using System.Security.Policy;
using System.Text;
using System.Threading;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;

namespace System.AddIn.Hosting
{
    internal static class AddInActivator
    {
        internal static T Activate<T>(AddInToken token, AddInSecurityLevel level)
        {
            if (token == null)
                throw new ArgumentNullException("token");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            // the name for the addin makes a good name for the appdomain
            return Activate<T>(token, level, token.Name);
        }

        // Creates a new AppDomain with the given security level & the specified friendly name.
        // Then creates an instance of the add-in within that appdomain, handing back the host
        // add-in view.  We'll also have to create an AddInControllerImpl to track & unload this add-in.
        internal static T Activate<T>(AddInToken token, AddInSecurityLevel level, String appDomainName)
        {
            if (token == null)
                throw new ArgumentNullException("token");
            if (appDomainName == null)
                throw new ArgumentNullException("appDomainName");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
        
            PermissionSet permissionSet = GetPermissionSetForLevel(level);
            return Activate<T>(token, permissionSet, appDomainName);
        }

        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: GetNamedPermissionSet(String):PermissionSet" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        [System.Security.SecuritySafeCritical]
        internal static PermissionSet GetPermissionSetForLevel(AddInSecurityLevel level)
        {
            if (AddInSecurityLevel.Internet > level || level > AddInSecurityLevel.Host)
                throw new ArgumentOutOfRangeException("level");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            if (level == AddInSecurityLevel.Host)
            {
                return AppDomain.CurrentDomain.PermissionSet;
            } 
            else
            {
                // Get a permission set for all non-GAC'ed code within the AD.
                Evidence sandboxEvidence = new Evidence();

                if (level == AddInSecurityLevel.Internet)
                {
                    sandboxEvidence.AddHostEvidence(new Zone(SecurityZone.Internet));
                }
                else if (level == AddInSecurityLevel.Intranet)
                {
                    sandboxEvidence.AddHostEvidence(new Zone(SecurityZone.Intranet));
                }
                else if (level == AddInSecurityLevel.FullTrust)
                {
                    sandboxEvidence.AddHostEvidence(new Zone(SecurityZone.MyComputer));
                }
                else if (level != AddInSecurityLevel.Internet)
                {
                    throw new ArgumentOutOfRangeException("level");
                }


                return SecurityManager.GetStandardSandbox(sandboxEvidence);
            }
        }

        // Assert full trust because we are seeing a full-trust demand here when there is
        // an AppDomainManager configured that has an internal constructor
        [System.Security.SecurityCritical]
        [PermissionSet(SecurityAction.Assert, Unrestricted=true)]
        private static AppDomain CreateDomain(AddInToken token, PermissionSet permissionSet, String appDomainName)
        {
            AppDomainSetup setup = new AppDomainSetup();
            setup.ApplicationBase = Path.GetDirectoryName(token._addin.Location);
            setup.ConfigurationFile = token._addin.Location + ".config";
            Assembly sysAddIn = typeof(AddInActivator).Assembly;
            // 
            AppDomain domain = AppDomain.CreateDomain(appDomainName,
                AppDomain.CurrentDomain.Evidence, setup, permissionSet,
                CreateStrongName(sysAddIn));  // Grant full trust to System.AddIn.dll
            // Ensure we load System.AddIn.dll in this new AD.
            domain.Load(sysAddIn.FullName);
            return domain;
        }

        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static T Activate<T>(AddInToken token, PermissionSet permissionSet, String appDomainName)
        {
            // Make a copy of the permission set to prevent the permissions from being modified after we demand
            permissionSet = permissionSet.Copy();

            //
            // Breaking security fix: (B#499362): Making a copy isn't sufficient protection if the
            // permission object comes from an untrusted source as the permission object itself
            // can interfere with the copy process. We simply can't safely pass an untrusted permission
            // down to CreateDomain(), so if there any untrusted permissions in the set, demand full trust before
            // allowing the operation to proceed.
            //
            if (!permissionSet.IsUnrestricted())
            {
                foreach (Object permission in permissionSet)
                {
                    Assembly a = permission.GetType().Assembly;
                    if (!a.GlobalAssemblyCache)
                    {
                        new PermissionSet(PermissionState.Unrestricted).Demand(); 
                        break;
                    }
                }
            }

            // Don't let them create an appdomain that elevates privileges
            permissionSet.Demand();

            AppDomain domain = null;
            try
            {
                domain = CreateDomain(token, permissionSet, appDomainName);
        
                AddInEnvironment environment = new AddInEnvironment(domain, true);
                AddInControllerImpl controller = new AddInControllerImpl(environment, true, token);
                return ActivateInAppDomain<T>(token, domain, controller, true);
            }
            catch
            {
                // Don't leak the domain.
                if (domain != null)
                {
                    try {
                        Utils.UnloadAppDomain(domain);
                    }
                    catch (AppDomainUnloadedException){}
                }
                throw;
            }
        }

        internal static T Activate<T>(AddInToken token, AppDomain target)
        {
            if (token == null)
                throw new ArgumentNullException("token");
            if (target == null)
                throw new ArgumentNullException("target");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            AddInEnvironment environment = new AddInEnvironment(target);
            AddInControllerImpl controller = new AddInControllerImpl(environment, false, token);
            return ActivateInAppDomain<T>(token, target, controller, false);
        }

        internal static T Activate<T>(AddInToken token, PermissionSet permissionSet)
        {
            if (token == null)
                throw new ArgumentNullException("token");
            if (permissionSet == null)
                throw new ArgumentNullException("permissionSet");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            return Activate<T>(token, permissionSet, token.Name);
        }

        // OOP Activation
        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: RemotingHelper.InitializeClientChannel():System.Void" Ring="1" />
        // <ReferencesCritical Name="Method: AddInProcess.GetAddInServer():System.AddIn.Hosting.AddInServer" Ring="1" />
        // <ReferencesCritical Name="Method: AddInServer.CreateDomain(System.AddIn.Hosting.AddInToken,System.Security.PermissionSet):System.AddIn.Hosting.AddInServerWorker" Ring="1" />
        // <ReferencesCritical Name="Method: ActivateOutOfProcess(AddInToken, AddInEnvironment, Boolean):T" Ring="2" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        internal static T Activate<T>(AddInToken token, AddInProcess process, PermissionSet permissionSet)
        {
            if (token == null)
                throw new ArgumentNullException("token");
            if (permissionSet == null)
                throw new ArgumentNullException("permissionSet");
            if (process == null)
                throw new ArgumentNullException("process");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            // check that they have ExecutionPermission.  Otherwise OOP remoting fails 
            // by shutting down the pipe, leaving the user scratching his head.
            if (!permissionSet.IsUnrestricted())
            {
                SecurityPermission p = (SecurityPermission)permissionSet.GetPermission(typeof(SecurityPermission));
                SecurityPermissionFlag requiredFlags = SecurityPermissionFlag.Execution;  
                if (p == null || (p.Flags & requiredFlags) != requiredFlags)
                    throw new ArgumentException(Res.NeedSecurityFlags);
            }

            RemotingHelper.InitializeClientChannel();

            AddInServer addInServer = process.GetAddInServer();

            AddInServerWorker addInServerWorker = addInServer.CreateDomain(token, permissionSet);
            AddInEnvironment fullEnvironment = new AddInEnvironment(process, addInServerWorker);

            return ActivateOutOfProcess<T>(token, fullEnvironment, true);
        }

        internal static T Activate<T>(AddInToken token, AddInProcess process, AddInSecurityLevel level)
        {
            PermissionSet permissionSet = GetPermissionSetForLevel(level);
            return Activate<T>(token, process, permissionSet);
        }

        // Activation in an existing appdomain, either in-process or out-of-process
        internal static T Activate<T>(AddInToken token, AddInEnvironment environment)
        {
            if (environment == null)
                throw new ArgumentNullException("environment");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            if (environment.Process.IsCurrentProcess)
            {
                AddInControllerImpl controller = new AddInControllerImpl(environment, false, token); 
                return ActivateInAppDomain<T>(token, environment.AppDomain, controller, false);
            }
            else
            {
                return ActivateOutOfProcess<T>(token, environment, false);
            }
        }

        // helper method
        // 
        private static T ActivateOutOfProcess<T>(AddInToken token, AddInEnvironment environment, bool weOwn)
        {
            ActivationWorker worker;
            IContract contract = environment.AddInServerWorker.Activate(token, out worker);

            AddInControllerImpl controller = new AddInControllerImpl(environment, weOwn, token); 
            controller.ActivationWorker = worker;

            T hav = AdaptToHost<T>(token, contract);
            if (weOwn)
                environment.AddInServerWorker.SetAppDomainOwner(contract);

            // Add this HAV and add-in controller to our list of currently 
            // non-disposed add-ins.
            controller.AssociateWithHostAddinView(hav, contract);
            return hav;
        }


        // Create a worker class in the remote appdomain, create the add-in & 
        // add-in adapter in the remote appdomain, then create the host adapter
        // in this appdomain, passing in a transparent proxy to the user's contract
        // implementation (which is really an instance of the add-in adapter).
        // The host adapter is a subclass of the host add-in view, which is our
        // return value (T).  Also, associate the host add-in view with the add-in
        // controller.
        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="Activator.CreateInstance(System.AppDomain,System.String,System.String,System.Boolean,System.Reflection.BindingFlags,System.Reflection.Binder,System.Object[],System.Globalization.CultureInfo,System.Object[],System.Security.Policy.Evidence):System.Runtime.Remoting.ObjectHandle" />
        // <SatisfiesLinkDemand Name="AppDomain.SetData(System.String,System.Object):System.Void" />
        // <SatisfiesLinkDemand Name="AppDomain.add_AssemblyResolve(System.ResolveEventHandler):System.Void" />
        // <ReferencesCritical Name="Method: ActivationWorker.Activate():System.AddIn.Contract.IContract" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.Reflection.Assembly.LoadFrom", Justification = "LoadFrom was designed for addin loading")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static T ActivateInAppDomain<T>(AddInToken pipeline, AppDomain domain, AddInControllerImpl controller, bool weOwn)
        {
            ContractComponent contract = pipeline._contract;
            HostAdapter hostAdapter = pipeline._hostAdapter;

            bool usingHostAppDomain = domain == AppDomain.CurrentDomain; 

            //begin direct connect code
            if (AddInToken.EnableDirectConnect && !weOwn && usingHostAppDomain)
            {
                Type havType = typeof(T);
                TypeInfo havTypeInfo = new TypeInfo(havType);
        
                if (pipeline._addinBase.CanDirectConnectTo(havTypeInfo))
                {
                    // Connect directly for best performance.
                    // Assert permission to the specific Addin directory only.
                    PermissionSet permissionSet = new PermissionSet(PermissionState.None);
                    permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                        Path.GetDirectoryName(pipeline._addin.Location)));
                    permissionSet.Assert();

                    Assembly addInAssembly = Assembly.LoadFrom(pipeline._addin.Location);
                    //
                    Type addinType = addInAssembly.GetType(pipeline._addin.TypeInfo.FullName, true);
                    Object addIn = addinType.GetConstructor(new Type[0]).Invoke(new Object[0]);
                    System.Diagnostics.Contracts.Contract.Assert(addIn != null, "Bypass couldn't create the add-in");
                    
                    // remember the addin directly as the HAV.  Set the contract to null.
                    controller.AssociateWithHostAddinView(addIn, null);

                    return (T)addIn;
                }
            }
            //end direct connect code

            // Use Activator.CreateInstance instead of AppDomain.CreateInstanceAndUnwrap
            // because Activator will do the appropriate security asserts in the
            // remote appdomain.
            Type t = typeof(ActivationWorker);
            Object[] args = new Object[] { pipeline };
            ObjectHandle objHandle = Activator.CreateInstance(domain, t.Assembly.FullName, t.FullName, 
                false, BindingFlags.Instance | BindingFlags.NonPublic, null, 
                args, null, null);
            ActivationWorker activationWorker = (ActivationWorker) objHandle.Unwrap();
            activationWorker.UsingHostAppDomain = usingHostAppDomain;

            System.AddIn.Contract.IContract addInContract = null;
            try
            {
                addInContract = activationWorker.Activate();
            }
            catch (Exception ex)
            {
                CheckForDuplicateAssemblyProblems(pipeline, ex);
                throw;
            }
            
            if (weOwn)
                domain.SetData(ContractHandle.s_appDomainOwner, addInContract);

            controller.ActivationWorker = activationWorker;
            T hav = AdaptToHost<T>(pipeline, addInContract);
            controller.AssociateWithHostAddinView(hav, addInContract);

            return hav;
        }


        // <SecurityKernel Critical="True" Ring="0">
        // <ReferencesCritical Name="Method: LoadContractAndHostAdapter(ContractComponent, HostAdapter, Type&, Type&):Void" Ring="1" />
        // <SatisfiesLinkDemand Name="AppDomain.add_AssemblyResolve(System.ResolveEventHandler):System.Void" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        [System.Security.SecuritySafeCritical]
        private static T AdaptToHost<T>(AddInToken pipeline, IContract addInContract)
        {
            if (addInContract == null)
                throw new ArgumentNullException("addInContract");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            ContractComponent contract = pipeline._contract;
            HostAdapter hostAdapter = pipeline._hostAdapter;

            Type hostAdapterType;
            Type contractType;
            LoadContractAndHostAdapter(contract, hostAdapter, out contractType, out hostAdapterType);

            int? temporaryToken = null;
            try
            {
                temporaryToken = addInContract.AcquireLifetimeToken();

                // This assembly resolve event exists to:
                //    1) To upgrade the contract assembly from the LoadFrom context to
                //       the default loader context
                ResolverHelper resolver = new ResolverHelper(pipeline);
                ResolveEventHandler assemblyResolver = new ResolveEventHandler(resolver.ResolveAssemblyForHostAdapter);
                AppDomain.CurrentDomain.AssemblyResolve += assemblyResolver;

                // Create the Host Adapter, which is a subclass of the Host Add-In View.
                // Pass the contract implementation to this constructor, using a 
                // transparent proxy to the real type.  Detect common errors here.
                InvokerDelegate myInvokerDelegate = CreateConsInvoker(hostAdapterType, contractType);

                T hav;
                try
                {
                    hav = (T)myInvokerDelegate(addInContract);
                }
                catch (ArgumentException e)
                {
                    CheckForLoaderContextProblems(contract, pipeline._addinAdapter, e);
                    throw;
                }
                finally
                {
                    AppDomain.CurrentDomain.AssemblyResolve -= assemblyResolver;
                }

                return hav;
            }
            finally
            {
                if (temporaryToken != null && addInContract != null)
                    addInContract.RevokeLifetimeToken((int)temporaryToken);
            }
        }

        // Delegate used to invoke the constructor of the adapter, providing a contract, and return 
        // the adapter.
        internal delegate Object InvokerDelegate(Object contract);

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.PermissionSet" />
        // </SecurityKernel>
        /// This method is used instead of the usual reflection Invoke on the constructor because
        /// we need to Assert the ability to see the internal constructor, but we don't
        /// want that Assert to carry over into the user code inside the constructor.  Reflection.Emit(LWCG)
        /// allows us to "see" the internal members first and then invoke them later in a separate action.
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal static InvokerDelegate CreateConsInvoker(Type targetType, Type argType)
        {
            ConstructorInfo havCtor = targetType.GetConstructor(
                BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance,
                null, new Type[]{argType}, null);

            Type[] methodArgs = new Type[]{typeof(Object)};
            DynamicMethod invoker = AssertAndCreateInvoker(targetType, argType, methodArgs, havCtor);

            return (InvokerDelegate)invoker.CreateDelegate(typeof(InvokerDelegate));
        }


        /// We assert full trust because that is needed in scenarios where the caller is lower
        /// trust than the callee assembly.  The demand happens when the call is jitted, but
        /// the stack is captured when the DynamicMethod is created.
        [PermissionSet(SecurityAction.Assert, Unrestricted=true)]
        [System.Security.SecurityCritical]
        private static DynamicMethod AssertAndCreateInvoker(Type targetType, Type argType,  Type[] methodArgs, ConstructorInfo havCtor)
        {
            // As a workaround to a red bits bug that leaks memory, we don't associate this
            // DM with an assembly.  Instead, we host it anonymously, and we need to assert 
            // full trust.            
            DynamicMethod invoker = new DynamicMethod(targetType.Name + "_ConstructorInvoker", // name, only usefult for debugging 
                                                      typeof(Object),  // return type
                                                      methodArgs, // parameterTypes
                                                      true); // skip visibility
            ILGenerator il = invoker.GetILGenerator();
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Castclass, argType); // need to cast Object to the specific Contract type expected by the constructor
            il.Emit(OpCodes.Newobj, havCtor); // invoke the constructor
            il.Emit(OpCodes.Ret);

            return invoker;
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.PermissionSet" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.Reflection.Assembly.LoadFrom", Justification = "LoadFrom was designed for addins")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal static void LoadContractAndHostAdapter(ContractComponent contract, HostAdapter hostAdapter,
                out Type contractType, out Type hostAdapterType)
        {
            PermissionSet assertSet = new PermissionSet(PermissionState.None);
            assertSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, hostAdapter.Location));
            assertSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, contract.Location));
            assertSet.Assert();

            // Explicitly load the host adapter & contract first, using LoadFrom 
            // to ensure they are loaded in the same loader context.
            Assembly hostAdapterAssembly = Assembly.LoadFrom(hostAdapter.Location);
            Assembly contractAssembly = Assembly.LoadFrom(contract.Location);

            hostAdapterType = hostAdapterAssembly.GetType(hostAdapter.TypeInfo.FullName, true);
            contractType = contractAssembly.GetType(contract.TypeInfo.FullName, true);
        }

        // This method is used to re-adapt a given addin to another HAV
        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="AppDomain.add_AssemblyResolve(System.ResolveEventHandler):System.Void" />
        // <SatisfiesLinkDemand Name="AppDomain.remove_AssemblyResolve(System.ResolveEventHandler):System.Void" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        [System.Security.SecuritySafeCritical]
        internal static T ActivateHostAdapter<T>(PartialToken pipeline, IContract addIn)
        {
            if (pipeline == null)
                throw new ArgumentNullException("pipeline");
            if (addIn == null)
                throw new ArgumentNullException("addIn");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            ContractComponent contract = pipeline._contract;
            HostAdapter hostAdapter = pipeline._hostAdapter;

            Type hostAdapterType; 
            Type contractType; 
            LoadContractAndHostAdapter(contract, hostAdapter, out contractType, out hostAdapterType);

            // This assembly resolve event exists to:
            //    1) To upgrade the contract assembly from the LoadFrom context to
            //       the default loader context
            ResolverHelper resolver = new ResolverHelper(contract);
            ResolveEventHandler assemblyResolver = new ResolveEventHandler(resolver.ResolveAssemblyForHostAdapter);
            AppDomain.CurrentDomain.AssemblyResolve += assemblyResolver;

            T hav;

            InvokerDelegate myInvokerDelegate = CreateConsInvoker(hostAdapterType, contractType);
            try
            {
                hav = (T)myInvokerDelegate(addIn);
            }
            catch (ArgumentException e)
            {
                CheckForLoaderContextProblems(contract, pipeline._addinAdapter, e);
                throw;
            }
            finally
            {
                AppDomain.CurrentDomain.AssemblyResolve -= assemblyResolver;
            }

            return hav;
        }


        private static void CheckForDuplicateAssemblyProblems(AddInToken pipeline, Exception inner)
        {
            Collection<String> warnings = new Collection<String>();
            bool duplicates = pipeline.HasDuplicatedAssemblies(null, warnings);
            if (duplicates)
            {
                String[] s = new String[warnings.Count];
                warnings.CopyTo(s, 0);
                throw new InvalidOperationException(String.Join("\n", s), inner);
            }
        }

        // Detects two somewhat easy to hit user bugs, where the contract assembly
        // is loaded twice in different loader contexts, or where the add-in adapter
        // has been loaded in the host's assembly.  If either of these happens, 
        // then the transparent proxy will not be castable to the interface type,
        // and the remoting code will attempt to load the addin adapter's 
        // type in the host's appdomain.  
        // We explicitly do not allow the add-in adapter's assembly to leak 
        // into this appdomain.  If the contract assembly exists in the same
        // directory as the application, or in potentially other locations, it may
        // get loaded twice in different loader contexts by the CLR V2's loader.
        // This took about a week to debug, with experts from all the affected areas.
        private static void CheckForLoaderContextProblems(ContractComponent contract, AddInAdapter addinAdapter, Exception inner)
        {
            String contractAsmName = contract.TypeInfo.AssemblyName;
            String addinAdapterAsmName = addinAdapter.TypeInfo.AssemblyName;

            List<Assembly> contracts = new List<Assembly>();
            List<Assembly> addinAdapters = new List<Assembly>();
            foreach (Assembly a in AppDomain.CurrentDomain.GetAssemblies()) {
                String aName = a.GetName().FullName;
                if (aName == contractAsmName)
                    contracts.Add(a);
                if (aName == addinAdapterAsmName)
                    addinAdapters.Add(a);
            }

            if (addinAdapters.Count > 0) {
                StringBuilder locations = new StringBuilder();
                foreach (Assembly a in addinAdapters) {
                    locations.Append(Environment.NewLine);
                    locations.Append(a.CodeBase);
                }

                Exception e = new InvalidOperationException(
                    String.Format(CultureInfo.CurrentCulture, Res.AddInAdapterLoadedInWrongAppDomain,
                        addinAdapter.TypeInfo.AssemblyName, addinAdapter.Location, locations.ToString()), inner);
                e.Data["Incorrectly loaded add-in adapters"] = addinAdapters;
                e.Data["Expected adapter location"] = addinAdapter.Location;
                if (contracts.Count > 1)
                    e.Data["Duplicate Contracts"] = contracts;
                throw e;
            }

            if (contracts.Count > 1) {
                StringBuilder locations = new StringBuilder();
                foreach (Assembly a in contracts)
                {
                    locations.Append(Environment.NewLine);
                    locations.Append(a.CodeBase);
                }

                Exception e = new InvalidOperationException(
                    String.Format(CultureInfo.CurrentCulture, Res.ContractAssemblyLoadedMultipleTimes,
                        contract.TypeInfo.AssemblyName, contract.Location, locations.ToString()), inner);
                e.Data["Incorrectly loaded contracts"] = contracts;
                e.Data["Expected contract location"] = contract.Location;
                throw e;
            }
            else {
                // If you're seeing this in a debugger on V2 of the CLR, make sure you hook the assembly resolve event.
                // Otherwise, see if there was an ArgumentException thrown from an HAV's constructor.
                System.Diagnostics.Contracts.Contract.Assert(false, "Did the AddIn Model upgrade the contract to the default loader context using an assembly resolve event?  Either that, or your HAV's constructor threw an ArgumentException");
            }
        }

        internal sealed class ResolverHelper
        {
            private ContractComponent _contract;

            internal ResolverHelper(AddInToken pipeline) : this(pipeline._contract)
            {
            }

            internal ResolverHelper(ContractComponent contract)
            {
                _contract = contract;
            }

            internal Assembly ResolveAssemblyForHostAdapter(Object sender, ResolveEventArgs args)
            {
                System.Diagnostics.Contracts.Contract.Assert(_contract != null);

                String assemblyRef = args.Name;
                //Console.WriteLine("ResolveAssemblyForHostAdapter: {0}", assemblyRef);
                
                // We load the contract assembly in the LoadFrom context, but we
                // need it in the default loader context.  In the V2 loader, we'll
                // need to explicitly return the currently loaded contract assembly
                // here, which will "upgrade" the assembly from LoadFrom to default.
                Assembly a = Utils.FindLoadedAssemblyRef(assemblyRef);
                if (a != null)
                    return a;

                // look in the contracts folder, in case there is a second contract there
                // this HostAdapter depends on.
                List<String> dirsToLookIn = new List<String>();
                string contractsDir = Path.GetDirectoryName(_contract.Location);
                dirsToLookIn.Add(contractsDir);

                return Utils.LoadAssemblyFrom(dirsToLookIn, assemblyRef);
            }
        }

        /// <summary>
        /// Create a StrongName that matches a specific assembly
        /// </summary>
        /// <exception cref="ArgumentNullException">
        /// if <paramref name="assembly"/> is null
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// if <paramref name="assembly"/> does not represent a strongly named assembly
        /// </exception>
        /// <param name="assembly">Assembly to create a StrongName for</param>
        /// <returns>A StrongName that matches the given assembly</returns>
        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.FileIOPermission" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal static StrongName CreateStrongName(Assembly assembly)
        {
            System.Diagnostics.Contracts.Contract.Requires(assembly != null);

            // Since there is no managed API for finding the GAC path, I 
            // assert path discovery for all local files.
            FileIOPermission permission = new FileIOPermission(PermissionState.None);
            permission.AllLocalFiles = FileIOPermissionAccess.PathDiscovery;
            permission.Assert();
            AssemblyName assemblyName = assembly.GetName();
            CodeAccessPermission.RevertAssert();

            // get the public key blob
            byte[] publicKey = assemblyName.GetPublicKey();
            if (publicKey == null || publicKey.Length == 0)
                throw new InvalidOperationException(Res.NoStrongName);

            StrongNamePublicKeyBlob keyBlob = new StrongNamePublicKeyBlob(publicKey);

            // and create the StrongName
            return new StrongName(keyBlob, assemblyName.Name, assemblyName.Version);
        }
    }
}
