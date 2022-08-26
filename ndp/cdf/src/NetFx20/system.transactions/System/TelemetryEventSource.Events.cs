// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace System
{
    using System.Diagnostics.Tracing;

    internal sealed partial class TelemetryEventSource
    {
        /// <summary>
        /// Transactions telemetry provider name.
        /// </summary>
        private const string TxProviderName = "Microsoft.DOTNET.System.Transactions";
        
        /// <summary>  
        /// Constructs a new instance of the TelemetryEventSource class with the  
        /// specified name. Sets the EtwSelfDescribingEventFormat option and joins the  
        /// MicrosoftTelemetry group.  
        /// </summary>  
        internal TelemetryEventSource() :
            this(TxProviderName)
        { 
        }

        /// <summary>
        /// Event fired if an InternalTransaction is used.
        /// </summary>
        internal void InternalTransaction() 
        {
            WriteUsageEvent();
        }  
    }
}
