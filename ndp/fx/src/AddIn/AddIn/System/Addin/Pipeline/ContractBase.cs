// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ContractBase 
**
** Purpose: Provides default implementations of IContract members 
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.AddIn.Contract;
using System.AddIn;
using System.Runtime.Remoting.Lifetime;
using System.Security.Permissions;
using System.Security;
using System.Diagnostics.Contracts;
using System.Threading;

namespace System.AddIn.Pipeline
{
    /// <summary>
    /// Provides default implementations of IContract members
    /// </summary>
    public class ContractBase : MarshalByRefObject, IContract, ISponsor
    {
        private List<int> m_lifetimeTokens = new List<int>();
        private List<String> m_contractIdentifiers;
        private readonly Object m_contractIdentifiersLock = new Object();
        private Random m_random;
        // flag to indicate that the references that were taken have all been removed;
        private bool m_zeroReferencesLeft = false;
        private const int MaxAppDomainUnloadWaits  = 15;  // must be <=30, and 15 corresponds to ~16 sec
        private int m_tokenOfAppdomainOwner;

        public virtual bool RemoteEquals(IContract contract)
        {
            return this.Equals(contract);
        }

        public virtual String RemoteToString()
        {
            return this.ToString();
        }

        public virtual int GetRemoteHashCode()
        {
            return this.GetHashCode();
        }

        //
        // return 'this' if we implement the contract, null otherwise
        //
        public virtual IContract QueryContract(String contractIdentifier)
        {
            if (contractIdentifier == null)
                throw new ArgumentNullException("contractIdentifier");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            if (m_contractIdentifiers == null)
            {
                lock (m_contractIdentifiersLock)
                {
                    if (m_contractIdentifiers == null)
                    {
                        Type t = this.GetType();
                        Type[] interfaces = t.GetInterfaces();
                        List<String> identifiers = new List<String>(interfaces.Length);
                        Type typeOfIContract = typeof(IContract);

                        foreach (Type iface in interfaces)
                        {
                            if (typeOfIContract.IsAssignableFrom(iface))
                            {
                                identifiers.Add(iface.AssemblyQualifiedName);
                            }
                        }

                        identifiers.Sort();

                        System.Threading.Thread.MemoryBarrier();
                        m_contractIdentifiers = identifiers;
                    }
                }
            }

            int i = m_contractIdentifiers.BinarySearch(contractIdentifier);
            return i >= 0 ? this : null;
        }

        public int AcquireLifetimeToken()
        {
            if (m_zeroReferencesLeft)
            {
                throw new InvalidOperationException(Res.TokenCountZero);
            }

            int next;

            lock (m_lifetimeTokens)
            {
                // Lazily initialize m_random for perf.
                if (m_random == null)
                {
                    m_random = new Random();
                }
                
                next = m_random.Next();
                while (m_lifetimeTokens.Contains(next))
                {
                    next = m_random.Next();
                }
                m_lifetimeTokens.Add(next);

                if (m_lifetimeTokens.Count == 1)
                {
                    // Register as a sponsor the first time a token is aquired.
                    // need to do this here instead of in InitializeLifetimeService because of a bug in remoting
                    // involving security.
                    RegisterAsSponsor();

                    // Increment ref count on appdomain owner if needed
                    IContract owner = ContractHandle.AppDomainOwner(AppDomain.CurrentDomain);
                    if (owner != null && owner != this)
                    {
                        m_tokenOfAppdomainOwner = owner.AcquireLifetimeToken();
                    }
                }
            }

            return next;
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        // <ReferencesCritical Name="Method: AppDomainUnload():Void" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        public void RevokeLifetimeToken(int token)
        {

            lock (m_lifetimeTokens)
            {
                if (!m_lifetimeTokens.Remove(token))
                    throw new InvalidOperationException(Res.LifetimeTokenNotFound);

                if (m_lifetimeTokens.Count == 0)
                {
                    m_zeroReferencesLeft = true;
                    // hook to allow subclasses to clean up
                    OnFinalRevoke();

                    IContract owner = ContractHandle.AppDomainOwner(AppDomain.CurrentDomain);
                    if (owner != null)
                    {
                        if (owner == this)
                        {
                            // Create a separate thread to unload this appdomain, because we 
                            // cannot shut down an appdomain from the Finalizer thread (though there
                            // is a bug that allows us to do so after the first appdomain has been
                            // unloaded, but let's not rely on that).  
                            // We can consider using an threadpool thread to do this, but that would add
                            // test burden.

                            SecurityPermission permission = new SecurityPermission(SecurityPermissionFlag.ControlThread);
                            permission.Assert();

                            System.Threading.ThreadStart threadDelegate = new System.Threading.ThreadStart(AppDomainUnload);
                            System.Threading.Thread unloaderThread = new System.Threading.Thread(threadDelegate);
                            unloaderThread.Start();
                        }
                        else
                            owner.RevokeLifetimeToken(m_tokenOfAppdomainOwner);
                    }
                }
            }
        }

        // To be overridden if needed.
        // This is similar to the Dispose pattern, but it is not public.  
        protected virtual void OnFinalRevoke()
        {
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        // </SecurityKernel>
        [System.Security.SecurityCritical]
        private void AppDomainUnload()
        {
            // assert is needed for the Internet named permission set.
            SecurityPermission permission = new SecurityPermission(SecurityPermissionFlag.ControlAppDomain);
            permission.Assert();

            // Sadly, AppDomains can refuse to unload under certain circumstances - mainly when threads get stuck in native code.
            // If that happens, we will retry with increasing intervals.
            // Other exceptions can also happen, in which case, we will trace the error message and quiety leave, otherwise the process might be torn down
            // and this is never what we want as the user has no way to catch anything thrown by this thread.
            try
            {
                for (int wait = 0; wait < MaxAppDomainUnloadWaits; wait++)
                {
                    try
                    {
                        AppDomain.Unload(AppDomain.CurrentDomain);
                        // if we get here successfully, we have unloaded the AppDomain, and thus life is good
                        return;
                    }
                    catch (CannotUnloadAppDomainException)
                    {
                        // Double the interval and wait
                        Thread.Sleep(1 << wait);
                    }
                }

                // We end up here if all retries failed. We try once more, and if that throws - so be it.
                // The outer handler will eat it and trace it out
                Thread.Sleep(1 << MaxAppDomainUnloadWaits);
                AppDomain.Unload(AppDomain.CurrentDomain);
            }
            // We are unloading our own AppDomain so one of these two will typically happen. They actually indicate success, and we don't need to report these
            catch (AppDomainUnloadedException) { }
            catch (ThreadAbortException) { }
            // If we got here, this means that either we are done trying to unload or something unexpected happened during the unload. We trace that out
            catch (Exception ex)
            {
                Trace.WriteLine(String.Format(System.Globalization.CultureInfo.CurrentCulture, Res.FailedToUnloadAppDomain, AppDomain.CurrentDomain.FriendlyName, ex.Message));
            }
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="ILease.get_InitialLeaseTime():System.TimeSpan" />
        // </SecurityKernel>
        [System.Security.SecurityCritical]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Justification="SecurityRules.Level1 assemblies cannot contain Critical code, so they need LinkDemands to protect overridden Critical members from SecurityRules.Level2 assemblies")]
        public TimeSpan Renewal(ILease lease)
        {
            if (lease == null)
                throw new ArgumentNullException("lease");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            lock (m_lifetimeTokens)
            {
                return m_lifetimeTokens.Count > 0 ? lease.InitialLeaseTime : TimeSpan.Zero;
            }
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="MarshalByRefObject.GetLifetimeService():System.Object" />
        // <SatisfiesLinkDemand Name="ILease.Register(System.Runtime.Remoting.Lifetime.ISponsor):System.Void" />
        // <Asserts Name="Imperative: System.Security.Permissions.SecurityPermission" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private void RegisterAsSponsor()
        {
            ILease baseLease = (ILease)GetLifetimeService();

            if (baseLease != null)
            {
                // Assert permission to register as a sponsor for this lease.  This is needed for the Internet permission level.
                SecurityPermission permission = new SecurityPermission(SecurityPermissionFlag.RemotingConfiguration);
                permission.Assert();
                try
                {
                    // register for sponsorship
                    baseLease.Register(this);
                }
                finally
                {
                    CodeAccessPermission.RevertAssert();
                }
            }
            // baseLease == null when we are not in a separate appdomain
        }
    }
}

