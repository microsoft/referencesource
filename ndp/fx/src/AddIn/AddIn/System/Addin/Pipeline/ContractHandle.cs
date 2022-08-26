// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ContractHandle
**
** Purpose: 
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.AddIn.Contract;
using System.AddIn;
using System.Runtime.Remoting.Lifetime;
using System.Runtime.Remoting;
using System.Runtime.Serialization;
using System.Diagnostics.Contracts;

namespace System.AddIn.Pipeline
{
    /// <summary>
    /// </summary>
    public class ContractHandle : IDisposable
    {
        private IContract m_contract;
        private int? m_token;
        internal const string s_appDomainOwner = "System.AddIn_Owner_Contract";

        public ContractHandle(IContract contract)
        {
            if (contract == null)
                throw new ArgumentNullException("contract");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            m_contract = contract;
            m_token = m_contract.AcquireLifetimeToken();
        }

        public IContract Contract
        {
            get { return m_contract; }
        }

        ~ContractHandle()
        {
            Dispose(false);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // managed cleanup would go here
            }
            if (m_token != null)
            {
                try
                {
                    // See comments in AddInController for why its OK to call into the
                    // transparentProxy from this objects Finalize member.
                    m_contract.RevokeLifetimeToken((int)m_token);
                }
                catch (AppDomainUnloadedException) { }
                catch (RemotingException) { }
                catch (SerializationException) {}
                finally
                {
                    m_token = null;
                    m_contract = null;
                }
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this); 
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="AppDomain")]
        public static bool ContractOwnsAppDomain(IContract contract, AppDomain domain)
        {
            if (domain == null)
                throw new ArgumentNullException("domain");
            if (contract == null)
                throw new ArgumentNullException("contract");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            return domain.GetData(s_appDomainOwner) == contract;
        }
        
		[System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming","CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId="AppDomain")]
        public static IContract AppDomainOwner(AppDomain domain)
        {
            if (domain == null)
                throw new ArgumentNullException("domain");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            return (IContract)domain.GetData(s_appDomainOwner);
        }
    }
}

