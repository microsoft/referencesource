// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInEnvironment
**
** Purpose: Abstraction representing an AppDomain, Process and Machine
**
===========================================================*/
using System;
using System.AddIn.Contract;
using System.Runtime.Remoting;
using System.Security;
using System.Security.Permissions;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{

    public sealed class AddInEnvironment
    {
        private AddInProcess _process;

        // for in process we have an appdomain. 
        private AppDomain _appDomain;
        // for out-of-process we have this.
        private AddInServerWorker _addInServerWorker;

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="appDomain")]
        public AddInEnvironment(AppDomain appDomain)
        {
            if (appDomain == null)
                throw new ArgumentNullException("appDomain");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            if (appDomain != AppDomain.CurrentDomain && !Utils.HasFullTrust()) {
                throw new SecurityException(Res.PartialTrustCannotActivate); 
            }

            _appDomain = appDomain;
            _process = AddInProcess.Current;
        }

        // This version is used when we have just created a new appdomain for this addin.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="appDomain")]
        internal AddInEnvironment(AppDomain appDomain, bool skipDomainCheck)
        {
            if (appDomain == null)
                throw new ArgumentNullException("appDomain");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            _appDomain = appDomain;
            _process = AddInProcess.Current;
        }

        internal AddInEnvironment(AddInProcess process, AddInServerWorker worker)
        {
            _addInServerWorker = worker;
            _process = process;
        }

        public AddInProcess Process
        {
            get { return _process; }
        }

        internal AppDomain AppDomain
        {
            get { return _appDomain; }
        } 

        internal AddInServerWorker AddInServerWorker
        {
            get { return _addInServerWorker; }
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        // <ReferencesCritical Name="Method: AddInServerWorker.UnloadAppDomain():System.Void" Ring="1" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal void UnloadAppDomain()
        {
            if (Process.IsCurrentProcess)
            {
                SecurityPermission permission = new SecurityPermission(SecurityPermissionFlag.ControlAppDomain);
                permission.Assert();
                
                AppDomain.Unload(AppDomain);
                CodeAccessPermission.RevertAssert();
            }
            else
            {
                try
                {
                    _addInServerWorker.UnloadAppDomain();
                }
                catch (AppDomainUnloadedException) { }
                catch (RemotingException) { }
            }
        }
    }
}

