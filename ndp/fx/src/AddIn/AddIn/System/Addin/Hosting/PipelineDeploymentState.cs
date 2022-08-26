// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  PipelineDeploymentState
**
** Purpose: Represents all the add-in pipeline components 
**          in a directory structure.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Text;
using System.Reflection;
using System.AddIn.MiniReflection;
using System.Diagnostics.Contracts;

namespace System.AddIn.Hosting
{
    [Serializable]
    internal sealed class PipelineDeploymentState : DeploymentState
    {
        private List<HostAdapter> _hostAdapters;
        private List<ContractComponent> _contracts;
        private List<AddInAdapter> _addinAdapters;
        private List<AddInBase> _addinBases;

        private List<PartialToken> _partialTokens;
        private List<int> _fileCounts;

        internal PipelineDeploymentState()
        {
            _hostAdapters = new List<HostAdapter>();
            _contracts = new List<ContractComponent>();
            _addinAdapters = new List<AddInAdapter>();
            _addinBases = new List<AddInBase>();

            _fileCounts = new List<int>(new int[]{0,0,0,0});
        }

        internal List<HostAdapter> HostAdapters {
            get { return _hostAdapters; }
        }

        internal List<ContractComponent> Contracts {
            get { return _contracts; }
        }

        internal List<AddInAdapter> AddInAdapters {
            get { return _addinAdapters; }
        }

        internal List<AddInBase> AddInBases {
            get { return _addinBases; }
        }

        internal List<PartialToken> PartialTokens {
            get { return _partialTokens; }
        }

        internal List<int> FileCounts {
            get { return _fileCounts; }
        }

        internal void ConnectPipeline(Collection<String> warnings)
        {
            List<PartialToken> result = new List<PartialToken>();
            // For ease of maintanence & debugging for users of the add-in 
            // model, we must make it easy to report which parts are not 
            // usable, due to missing connections with other parts of the 
            // pipeline.  

            // Real connect loop.
            foreach (HostAdapter hostAdapter in HostAdapters)
            {
                foreach (ContractComponent contract in Contracts)
                {
                    if (!hostAdapter.Constructors.Contains(contract.TypeInfo))
                        continue;

                    hostAdapter.ConnectedToNeighbors = true;

                    foreach (AddInAdapter addinAdapter in AddInAdapters)
                    {
                        if (!addinAdapter.Contracts.Contains(contract.TypeInfo))
                            continue;

                        contract.ConnectedToNeighbors = true;

                        foreach (AddInBase addinBase in AddInBases)
                        {
                            if (!addinAdapter.CanConnectTo(addinBase))
                                continue;

                            addinAdapter.ConnectedToNeighbors = true;
                            addinBase.ConnectedToNeighbors = true;
                            PartialToken partialToken = new PartialToken(hostAdapter, contract, addinAdapter, addinBase);
                            result.Add(partialToken);
                        } // foreach addinBase
                    } // foreach addinAdapter
                }  // foreach contract
            }  // foreach hostAdapter

            // Look for unconnected parts.
            int unconnectedParts = 0;
            unconnectedParts += LookForUnconnectedParts(HostAdapters, warnings);
            unconnectedParts += LookForUnconnectedParts(Contracts, warnings);
            unconnectedParts += LookForUnconnectedParts(AddInAdapters, warnings);
            unconnectedParts += LookForUnconnectedParts(AddInBases, warnings);

#if ADDIN_VERBOSE_WARNINGS
            warnings.Add(String.Format(CultureInfo.CurrentCulture, "PipelineDeploymentState::Connect: Found {0} valid pipelines.", result.Count));
#endif
            if (unconnectedParts > 0)
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.CouldntConnectNInvalidParts, unconnectedParts));

            _partialTokens = result;
        }

        private static int LookForUnconnectedParts<T>(IEnumerable<T> parts, Collection<String> warnings) where T : PipelineComponent
        {
            int numUnconnected = 0;
            foreach (PipelineComponent part in parts) {
                if (!part.ConnectedToNeighbors) {
                    warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.CouldntConnectOnePart, part.ToString()));
                    numUnconnected++;
                }
            }
            return numUnconnected;
        }
    }
}
