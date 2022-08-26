// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace System.ServiceModel
{
    using System;
    using System.Diagnostics.Tracing;
    using System.Runtime;
    using System.Security;
    using System.ServiceModel.Description;

    [EventData(Name = "ServiceKPI")]
    struct ServiceKPI
    {
        [EventField(Tags = TelemetryEventSource.HashPiiField)]
        public string ServiceId { get; set; }

        public string HostType { get; set; }

        public string EndpointsV2 { get; set; }

        public string Version { get; set; }
    }

    /// <summary>
    /// Registers provider for TraceLogging
    /// </summary>
    internal class TelemetryTraceLogging
    {
        /// <summary>
        /// Event name for logging how WCF service is hosted and associated .NET Framework version, 
        /// </summary>
        private static readonly string wcfHostTypeWithVersions = "WCFServiceKPI";

        /// <summary>
        /// WCF telemetry provider name.
        /// </summary>
        private static readonly string wcfproviderName = "Microsoft.DOTNET.WCF.ServiceModel";

        /// <summary>
        /// WCFTelemetryEventSource instance used to do all telemetry data logging
        /// </summary>
        private static EventSource logger;

        static TelemetryTraceLogging()
        {
            logger = new TelemetryEventSource(wcfproviderName);
        }

        /// <summary>
        /// Logs WCF service Host Type
        /// Logs target framework version and release version details
        /// </summary>
        /// <param name="description">ServiceDescription instance of the ServiceHost.</param> 
        public static void LogSeriveKPIData(ServiceDescription description) 
        {
            // this try-catch is important since we don't want to interrupt the program if telemetry logging throw exception unless it's fatal.
            try
            {
                TelemetryHelper telemetryHelper = new TelemetryHelper();

                if (TelemetryTraceLogging.logger != null && TelemetryTraceLogging.logger.IsEnabled())
                {
                    TelemetryTraceLogging.logger.Write(
                                    wcfHostTypeWithVersions,
                                    TelemetryEventSource.MeasuresOptions(),
                                    new ServiceKPI
                                    {
                                        ServiceId = telemetryHelper.GetServiceId(description),
                                        HostType = telemetryHelper.GetHostType(),
                                        EndpointsV2 = telemetryHelper.GetEndpoints(description),
                                        Version = telemetryHelper.GetAssemblyVersion()
                                    });
                }
            }
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }

                // If not fatal, just eat the exception since we don't want telemetry tracing to cause runtime failure.
            }
        }
    }
}
