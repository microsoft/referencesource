// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInServer
**
** Purpose: Created in the remote process, this worker is 
**     used to start up & shut down the add-in.
**
===========================================================*/
using System;
using System.AddIn;
using System.AddIn.Contract;
using System.AddIn.Hosting;
using System.AddIn.Pipeline;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Reflection;
using System.Runtime.Remoting;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{ 
    [SuppressMessage("Microsoft.Performance","CA1812:AvoidUninstantiatedInternalClasses", Justification="Instantiation is via remoting")]
    internal sealed class AddInServer : MarshalByRefObject
    {
        private int _addInAppDomains = 0;
        private volatile bool _startedExitProcess = false;

        // This serves as a MBRO that can send the ShuttingDown event back to the client
        private EventWorker _eventWorker;

        public void Initialize(EventWorker eventWorker)
        {
            _eventWorker = eventWorker;
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <ReferencesCritical Name="Method: AddInActivator.CreateStrongName(System.Reflection.Assembly):System.Security.Policy.StrongName" Ring="1" />
        // <Asserts Name="Imperative: System.Security.PermissionSet" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        public AddInServerWorker CreateDomain(AddInToken token, PermissionSet permissionSet)
        {
            AppDomain domain;
            AppDomainSetup setup = new AppDomainSetup();
            setup.ApplicationBase = Path.GetDirectoryName(token._addin.Location);
            setup.ConfigurationFile = token._addin.Location + ".config";

            Assembly sysCore = typeof(AddInActivator).Assembly;
            domain = AppDomain.CreateDomain(token.Name,  
                AppDomain.CurrentDomain.Evidence, setup, permissionSet,
                AddInActivator.CreateStrongName(sysCore));  // Grant full trust to System.Core.dll

            // Ensure we load System.Core.dll in this new AD.
            domain.Load(sysCore.FullName);

            ObjectHandle workerHandle = Activator.CreateInstance(domain, sysCore.FullName, typeof(AddInServerWorker).FullName);
            AddInServerWorker server = (AddInServerWorker)workerHandle.Unwrap();

            server.AddInServer = this;

            Interlocked.Increment(ref _addInAppDomains);

            return server;
        }

        public void ExitProcess()
        {
            // set flag so that we don't call back to host from finalizers
            _startedExitProcess = true;
            // This is called in the full-trust appdomain, so no assert is needed.
            Environment.Exit(0);
        }

        public void AddInDomainFinalized()
        {
            long val = Interlocked.Decrement(ref _addInAppDomains);
            if (!_startedExitProcess && val == 0)
            {
                try
                {
                    // the host will call ExitProcess here if it is not cancelled 
                    _eventWorker.SendShutdownMessage();
                }
                catch (RemotingException)
                {
                    // the application likely has shut down itself.
                    // The main thread will shut down this app
                }
            }
        }

        // Don't let this object time out in Remoting.  
        public override Object InitializeLifetimeService()
        {
            return null;
        }
    }

    [SuppressMessage("Microsoft.Performance","CA1812:AvoidUninstantiatedInternalClasses", Justification="Instantiation is via remoting")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
    internal sealed class AddInServerWorker : MarshalByRefObject 
    {
        AddInServer _addInServer;

        [SuppressMessage("Microsoft.Security","CA2128:SecurityTransparentCodeShouldNotAssert", Justification="SafeCritical code can Assert")]
        [System.Security.SecuritySafeCritical]
        public AddInServerWorker()
        {
            PermissionSet permissionSet = new PermissionSet(PermissionState.None);
            permissionSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.ControlPrincipal));
            permissionSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode));
            permissionSet.AddPermission(new ReflectionPermission(PermissionState.Unrestricted));
            permissionSet.Assert();

            // without this call to initialize the client channel, this object cannot be remoted
            RemotingHelper.InitializeClientChannel();
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="AppDomain.SetData(System.String,System.Object):System.Void" />
        // <Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1822:MarkMembersAsStatic", Justification="Needs to be instance for remoting"),
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        public void SetAppDomainOwner(IContract contract)
        {
            new SecurityPermission(SecurityPermissionFlag.ControlAppDomain).Assert();
            AppDomain.CurrentDomain.SetData(ContractHandle.s_appDomainOwner, contract);
        }

        public AddInServer AddInServer
        {
            set { _addInServer = value; }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1822:MarkMembersAsStatic", Justification="Needs to be instance for remoting")]
        public IContract Activate(AddInToken pipeline, out ActivationWorker worker)
        {
            worker = new ActivationWorker(pipeline);

            IContract contract = worker.Activate();

            return contract;
        }

        // Don't let this object time out in Remoting.  
        public override Object InitializeLifetimeService()
        {
            return null;
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1822:MarkMembersAsStatic", Justification="Needs to be instance for remoting")]
        [System.Security.SecurityCritical]
        public void UnloadAppDomain()
        {
            SecurityPermission permission = new SecurityPermission(SecurityPermissionFlag.ControlAppDomain);
            permission.Assert();
                
            AppDomain.Unload(AppDomain.CurrentDomain);
            // thread will be aborted
        }

        //This appdomain must be going away
        // <SecurityKernel Critical="True" Ring="6">
        // <ReferencesCritical Name="Method: AddInServer.AddInDomainFinalized():System.Void" Ring="6" />
        // </SecurityKernel>
        [System.Security.SecurityCritical]
        ~AddInServerWorker()
        {
            if (_addInServer != null)
            {
                _addInServer.AddInDomainFinalized();
            }
        }
    }

    internal sealed class EventWorker : MarshalByRefObject
    {
        AddInProcess _process;

        public EventWorker(AddInProcess process)
        {
            _process = process;
        }

        public bool SendShutdownMessage()
        {
            CancelEventArgs args = new CancelEventArgs();
            _process.SendShuttingDown(args);
            return args.Cancel;
        }
    }
}

