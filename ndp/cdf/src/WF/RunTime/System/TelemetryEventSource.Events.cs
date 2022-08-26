// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace System
{
    using System.Diagnostics.Tracing;

    internal sealed partial class TelemetryEventSource
    {
        /// <summary>
        /// Workflow V1 telemetry provider name.
        /// </summary>
        private const string WfV1ProviderName = "Microsoft.DOTNET.WF.V1";
        
        /// <summary>  
        /// Constructs a new instance of the TelemetryEventSource class with the  
        /// specified name. Sets the EtwSelfDescribingEventFormat option and joins the  
        /// MicrosoftTelemetry group.  
        /// </summary>  
        internal TelemetryEventSource() :
            this(WfV1ProviderName)
        { 
        }

        /// <summary>
        /// Event fired if WFV1 runtime is used.
        /// </summary>
        internal void V1Runtime() 
        {
            WriteUsageEvent();
        }  
    }
}
