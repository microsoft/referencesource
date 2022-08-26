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
    // Wrapper on AddInControllerImpl so that a ref count is kept on the Contract
    // keeping the addin alive while an AddInController object is alive in the host.
    public sealed class AddInController
    {
        private AddInControllerImpl _impl;

        // keep the addin alive while the controller is alive
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification="Needed to prevent remoting failures")]
        private Object _hostViewOfAddIn;

        // Prevent the addin domain from being torn down while there is a controller still alive when, for example,
        // Dispose is called on the HVA and the hostAdapter releases its LifetimeToken.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1823:AvoidUnusedPrivateFields", Justification="Needed for finalizer behavior")]
        private ContractHandle _contractHandle;

        internal AddInController(AddInControllerImpl impl, Object hostViewOfAddIn, ContractHandle contractHandle)
        {
            System.Diagnostics.Contracts.Contract.Requires(impl != null);

            _impl = impl;

            _hostViewOfAddIn = hostViewOfAddIn;

            _contractHandle = contractHandle;
        }

        // Takes a host add-in view (HAV) and maps that to an add-in controller.
        public static AddInController GetAddInController(Object addIn)
        {
            return AddInControllerImpl.GetAddInController(addIn);
        }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        public void Shutdown()
        {
            _impl.Shutdown();
        }

        // This will not be usable for OOP scenarios.  
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="AppDomain")]
        public AppDomain AppDomain {
            get { return _impl.AppDomain; }
        }

        public AddInToken Token {
            get { return _impl.Token; }
        }

        public AddInEnvironment AddInEnvironment
        {
            get { return _impl.AddInEnvironment; }
        }

        internal AddInControllerImpl AddInControllerImpl
        {
            get { return _impl; }
        }
    }
}

