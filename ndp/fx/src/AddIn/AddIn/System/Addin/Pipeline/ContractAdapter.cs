// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ContractAdapter 
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Globalization;
using System.AddIn.Contract;
using System.AddIn;
using System.Security;
using System.Security.Permissions;
using System.AddIn.Hosting;
using System.Reflection;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;

namespace System.AddIn.Pipeline
{
    public static class ContractAdapter
    {
        public static ContractHandle ViewToContractAdapter(Object view)
        {
            if (view == null)
                throw new ArgumentNullException("view");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            AddInController controller = AddInController.GetAddInController(view);
            if (controller != null)
            {
                return new ContractHandle(controller.AddInControllerImpl.GetContract());
            }

            return null;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Factory Method")]
        [SecuritySafeCritical]
        public static TView ContractToViewAdapter<TView>(ContractHandle contract, PipelineStoreLocation location)
        {
            if (location != PipelineStoreLocation.ApplicationBase)
                throw new ArgumentException(Res.InvalidPipelineStoreLocation, "location");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String appBase = AddInStore.GetAppBase();
            return ContractToViewAdapterImpl<TView>(contract, appBase, false);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification = "Factory Method")]
        [SecuritySafeCritical]
        public static TView ContractToViewAdapter<TView>(ContractHandle contract, string pipelineRoot)
        {
            return ContractToViewAdapterImpl<TView>(contract, pipelineRoot, true);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Justification="The message is about SafeHandles")]      
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1004:GenericMethodsShouldProvideTypeParameter", Justification="Factory Method")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity")]
        [SecurityCritical]
        private static TView ContractToViewAdapterImpl<TView>(ContractHandle contract, String pipelineRoot, bool demand)
        {
            if (contract == null)
                throw new ArgumentNullException("contract");
            if (pipelineRoot == null)
                throw new ArgumentNullException("pipelineRoot");
            if (String.IsNullOrEmpty(pipelineRoot))
                throw new ArgumentException(Res.PathCantBeEmpty);
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            if (demand)
                new FileIOPermission(FileIOPermissionAccess.Read, pipelineRoot).Demand();

            Type havType = typeof(TView);
            TypeInfo havTypeInfo = new TypeInfo(havType);

            List<PartialToken> partialTokens = AddInStore.GetPartialTokens(pipelineRoot);
            foreach (PartialToken partialToken in partialTokens)
            {
                if (AddInStore.Contains(partialToken.HostAdapter.HostAddinViews, havTypeInfo))
                {
                    partialToken.PipelineRootDirectory = pipelineRoot;

                    //Ask for something that can implement the contract in this partial token.  The result will 
                    //either be null, the addin adapter itself, or another addin adapter 
                    IContract subcontract = contract.Contract.QueryContract(partialToken._contract.TypeInfo.AssemblyQualifiedName);
                    if (subcontract != null)
                    {
                        //Instantiate the adapter and pass in the addin to its constructor
                        TView hav = AddInActivator.ActivateHostAdapter<TView>(partialToken, subcontract);
                        return hav;
                    }
                }
            }

            // Don't let the ref count go to zero too soon, before we increment it in ActivateHostAdapter
            // This is important when QueryContract returns the addIn adapter itself.  A GC at that point
            // may collect the ContractHandle and decrement the ref count to zero before we have a chance to increment it
            System.GC.KeepAlive(contract);

            // return null.  Compiler makes us return default(TView), which will be null
            return default(TView);
        }
    }
}

