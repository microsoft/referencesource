// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInController
**
** Purpose: Allows you to shut down an add-in, which may unload
**     an AppDomain or kill an out-of-process add-in.  
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Reflection;
using System.Runtime.ConstrainedExecution;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Lifetime;
using System.Runtime.Serialization;
using System.Security.Permissions;
using System.Security;
using System.Diagnostics;
using System.AddIn.Contract;
using System.AddIn.Pipeline;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    // The lifetime of an add-in is controlled by remoting's leases, and users
    // who do essentially ref counting by obtaining a lease.  No one has solved
    // the distributed (cross-machine) garbage collector problem yet, but leases
    // appear to be the best approximation we currently have.  
    internal sealed class AddInControllerImpl
    {
        // Maps host add-in views (precisely, instances of host adapters typed
        // as their base type) to the associated add-in controllers.
        private static HAVControllerPair _havList;
        private static readonly Object _havLock = new Object();

        //private AppDomain _appDomain;
        private bool _unloadDomainOnExit;
        private AddInToken _token;

        private AddInEnvironment _addInEnvironment;

        private static int _addInCountSinceLastPrune;
        private const int AddInCountSinceLastPruneLimit = 25;

        // Note that keeping this reference to the transparentProxy 
        // prevents it from being GC'd in the same sweep as the HAV.  Indeed, it cannot
        // be GC'd until we remove its HAVController pair.  This is good,
        // since it allows us to call into the TransparentProxy from the finalize method
        // of LifetimeTokenHandle
        internal IContract _contract;  // Contract or a Transparent Proxy to the contract 
        private ActivationWorker _activationWorker;
        private WeakReference _havReference;

        internal AddInControllerImpl(AddInEnvironment environment, bool unloadDomainOnExit, AddInToken token)
        {
            System.Diagnostics.Contracts.Contract.Requires(environment != null);
            System.Diagnostics.Contracts.Contract.Requires(token != null);

            _unloadDomainOnExit = unloadDomainOnExit;
            _token = token;

            _addInEnvironment = environment;
        }

        // Takes a host add-in view (HAV) and maps that to an add-in controller.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        internal static AddInController GetAddInController(Object addIn)
        {
            if (addIn == null)
                throw new ArgumentNullException("addIn");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            AddInControllerImpl controllerImpl = FindController(addIn, false);

            //return new wrapper
            if (controllerImpl != null)
            {
                // Try and increase the ref count on the addin.  If we fail, perhaps
                // because the user already called Dispose() on the HVA, that's OK.  Still allow
                // them to use the AddInController to examine the AddInToken 
                ContractHandle handle = null;
                try
                {
                     handle = new ContractHandle(controllerImpl._contract);
                }
                catch (Exception) {}
                return new AddInController(controllerImpl, addIn, handle);
            }

            throw new ArgumentException(Res.ControllerNotFound);
        }

        // Find the controller given the HAV (addIn), optionally also removing it.
        // This code also removes any stale HAVControllerPairs that it finds, as
        // may happen when a HAV is garbage collected.
        private static AddInControllerImpl FindController(Object addIn, bool remove)
        {
            System.Diagnostics.Contracts.Contract.Requires(addIn != null);

            lock (_havLock)
            {
                HAVControllerPair current = _havList;
                HAVControllerPair last = null;
                while(current != null)
                {
                    Object o = current._HAV.Target;
                    if (o == null)
                    {
                        // this one has been GC'd.  Clean up the WR
                        if (last == null)
                        {
                            _havList = current._next;
                            continue;
                        }
                        else
                        {
                            last._next = current._next;
                            current = current._next;
                            continue;
                        }
                    }
                    else
                    {
                        if (addIn.Equals(o))
                        {
                            AddInControllerImpl value = current._controller;
                            if (remove)
                            {
                                if (last == null)
                                    _havList = current._next;
                                else
                                    last._next = current._next;
                            }
                            return value;
                        }
                    }

                    last = current;
                    current = current._next;
                }
            }
            return null;
        }

        // Requires the HAV and the transparent proxy to the add-in adapter, 
        // which implements System.AddIn.Contract.IContract.
        internal void AssociateWithHostAddinView(Object hostAddinView, IContract contract)
        {
            System.Diagnostics.Contracts.Contract.Requires(hostAddinView != null);
            _contract = contract;

            // add weak reference on the HAV to our list
            _havReference = new WeakReference(hostAddinView);
            lock (_havLock)
            {
                HAVControllerPair havRef = new HAVControllerPair(hostAddinView, this);
                havRef._next = _havList;
                _havList = havRef;

                // clean up the list every so often
                _addInCountSinceLastPrune++;
                if (_addInCountSinceLastPrune == AddInCountSinceLastPruneLimit)
                {
                    // searching for a non-exiting addin will have the desired effect
                    // of pruning the list of any stale references.
                    FindController(new Object(), false);
                    _addInCountSinceLastPrune = 0;
                }
            }
        }

        internal ActivationWorker ActivationWorker {
            set {
                System.Diagnostics.Contracts.Contract.Requires(value != null);
                _activationWorker = value;
            }
        }

        //<SecurityKernel Critical="True" Ring="0">
        //<ReferencesCritical Name="Method: ActivationWorker.CreateAddInAdapter(System.Object,System.Reflection.Assembly):System.AddIn.Contract.IContract" Ring="1" />
        //<Asserts Name="Imperative: System.Security.PermissionSet" />
        //</SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed"),
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.Reflection.Assembly.LoadFrom", Justification = "LoadFrom was designed for addins")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal IContract GetContract()
        {
            if (_contract != null)
                return _contract;

            // in direct connect, the contract has not been created.  Create it now.
            Object hav = _havReference.Target;
            if (hav == null)
                throw new InvalidOperationException(Res.AddInNoLongerAvailable);

            // Assert permission to the contracts, AddInSideAdapters, AddInViews and specific Addin directories only.
            PermissionSet permissionSet = new PermissionSet(PermissionState.None);
            permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                Path.Combine(_token.PipelineRootDirectory, AddInStore.ContractsDirName)));
            permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery,
                Path.Combine(_token.PipelineRootDirectory, AddInStore.AddInAdaptersDirName)));
            permissionSet.Assert();

            Assembly.LoadFrom(_token._contract.Location);
            Assembly addinAdapterAssembly = Assembly.LoadFrom(_token._addinAdapter.Location);
            CodeAccessPermission.RevertAssert();

            // Create the AddIn adapter for the addin
            ActivationWorker worker = new ActivationWorker(_token);
            _contract = worker.CreateAddInAdapter(hav, addinAdapterAssembly);
            return _contract;
        }

        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: ActivationWorker.Dispose():System.Void" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", MessageId = "System.GC.Collect", Justification = "Recommended by GC team")]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        public void Shutdown()
        {
            // Disables usage of the add-in, by breaking the pipeline.
            // Also, if we own the appdomain, we unload it.
            lock (this)  // Ensure multiple threads racing on Shutdown don't collide.
            {
                AddInEnvironment environment = _addInEnvironment;
                if (environment != null) {
                    try {
                        if (_contract != null) {
                            
                            Object hav = _havReference.Target;
                            IDisposable disposableHAV = hav as IDisposable;
                            if (disposableHAV != null)
                            {
                                try
                                {
                                    disposableHAV.Dispose();
                                }
                                catch (AppDomainUnloadedException e)
                                {
                                    Log(e.ToString());
                                }
                                catch (RemotingException re)
                                {
                                    Log(re.ToString());
                                }
                                catch (SerializationException se)
                                {
                                    Log(se.ToString());
                                }
                            }

                            IDisposable disposableContract = _contract as IDisposable;
                            if (disposableContract != null)
                            {
                                try
                                {
                                    disposableContract.Dispose();
                                }
                                catch (AppDomainUnloadedException e)
                                {
                                    Log(e.ToString());
                                }
                                catch (RemotingException re)
                                {
                                    Log(re.ToString());
                                }
                                catch (SerializationException se)
                                {
                                    Log(se.ToString());
                                }
                            }
                            _contract = null;
                        }

                        if (_activationWorker != null) {
                            // Unhook an assembly resolve event in the target appdomain.
                            // However, if one of the adapters implemented IDisposable and cleaned
                            // up the appropriate lifetime tokens, this appdomain may be unloading
                            // already (we launch another thread to do this, so we are guaranteed
                            // to have a benign race condition).  We should catch an 
                            // AppDomainUnloadedException here.
                            try
                            {
                                _activationWorker.Dispose();
                            }
                            catch (AppDomainUnloadedException) { }
                            catch (RemotingException) { }
                            catch (SerializationException) { }

                            _activationWorker = null;
                        }
                    }
                    finally {
                        if (_unloadDomainOnExit) {
                            // AppDomain.Unload will block until we have finalized all 
                            // objects within the appdomain.  Also, this may already
                            // have been unloaded.
                            try {
                                environment.UnloadAppDomain();
                            }
                            catch (AppDomainUnloadedException) { }
                            catch (RemotingException) { }

                            // Using the transparent proxy will now cause exceptions, 
                            // as managed threads are not allowed to enter this appdomain.
                        }
                    }
                    _addInEnvironment = null;

                    // eagerly remove from list
                    lock (_havLock)
                    {
                        Object addin = _havReference.Target;
                        if (addin != null)
                            FindController(addin, true);
                    }

                    // The perf team recommends doing a GC after a large amount of memory has
                    // been dereferenced.  We wait for the finalizers to complete first
                    // becase some references in the addin are not released until finalization.
                    // Also, if an addin is buggy and causes the finalizer thread to hang, 
                    // waiting here makes it fail deterministically. 
                    System.GC.WaitForPendingFinalizers();
                    System.GC.Collect();

                } // end if domain != null
                else {
                    throw new InvalidOperationException(Res.AppDomainNull);
                } 
            }
        }

        // This will not be usable for OOP scenarios.  
        internal AppDomain AppDomain {
            get {
                if (_addInEnvironment == null)
                    throw new ObjectDisposedException("appdomain");
                return _addInEnvironment.AppDomain;
            }
        }

        internal AddInToken Token {
            get {
                return _token;
            }
        }

        internal AddInEnvironment AddInEnvironment
        {
            get {
                return _addInEnvironment;
            }
        }

        private static void Log(String message)
        {
            Debugger.Log(0, "AddInController", message);
        }

        internal sealed class HAVControllerPair
        {
            internal WeakReference _HAV;
            internal AddInControllerImpl _controller;
            internal HAVControllerPair _next;

            public HAVControllerPair(Object hav, AddInControllerImpl controller)
            {
                _HAV = new WeakReference(hav);
                _controller = controller;
            }
        }
    }
}
