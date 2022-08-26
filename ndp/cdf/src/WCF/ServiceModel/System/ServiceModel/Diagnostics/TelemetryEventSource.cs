// <copyright>
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

//--------------------------------------------------------------------------
// This code is modified based on the OSG tracelogging Partner code template 
//--------------------------------------------------------------------------

namespace System.ServiceModel
{
    using System.Diagnostics.Tracing;

    /// <summary>  
    /// <para>  
    /// An Asimov-enabled EventSource. This inherits from EventSource, and is  
    /// exactly the same as EventSource except that it forces Asimov-compatible  
    /// construction (it always enables EtwSelfDescribingEventFormat and joins the  
    /// MicrosoftPartnerTelemetry group). It also provides several Asimov-specific  
    /// constants.  
    /// </para>  
    /// <para>  
    /// Note that this class DOES NOT automatically add any keywords to your events.  
    /// Even when using this class, events will be ignored by UTC unless they include  
    /// one of the telemetry keywords. Each event that you want to send to UTC must  
    /// have one (and only one) of the following keywords set in  
    /// eventSourceOptions.Keywords: TelemetryKeyword, MeasuresKeyword, or  
    /// CriticalDataKeyword.  
    /// </para>  
    /// </summary>  
    internal sealed class TelemetryEventSource
        : EventSource
    {
        /// <summary>  
        /// Keyword 0x0000100000000000 is reserved for future definition by UTC. Do  
        /// not use keyword 0x0000100000000000 for telemetry-enabled ETW events.  
        /// </summary>  
        public const EventKeywords Reserved44Keyword = (EventKeywords)0x0000100000000000;

        /// <summary>  
        /// Add TelemetryKeyword to eventSourceOptions.Keywords to indicate that  
        /// an event is for general-purpose telemetry.  
        /// This keyword should not be combined with MeasuresKeyword or  
        /// CriticalDataKeyword.  
        /// </summary>  
        public const EventKeywords TelemetryKeyword = (EventKeywords)0x0000200000000000;

        /// <summary>  
        /// Add MeasuresKeyword to eventSourceOptions.Keywords to indicate that  
        /// an event is for understanding measures and reporting scenarios.  
        /// This keyword should not be combined with TelemetryKeyword or  
        /// CriticalDataKeyword.  
        /// </summary>  
        public const EventKeywords MeasuresKeyword = (EventKeywords)0x0000400000000000;

        /// <summary>  
        /// Add CriticalDataKeyword to eventSourceOptions.Keywords to indicate that  
        /// an event powers user experiences or is critical to business intelligence.  
        /// This keyword should not be combined with TelemetryKeyword or  
        /// MeasuresKeyword.  
        /// </summary>  
        public const EventKeywords CriticalDataKeyword = (EventKeywords)0x0000800000000000;

        /// <summary>  
        /// Add CoreData to eventSourceOptions.Tags to indicate that an event  
        /// contains high priority "core data". (Core data is defined by the telemetry  
        /// team. If you think your data is "core data", please work with the telemetry  
        /// team to add your event to the "core data" list before adding this flag to  
        /// your event.)  
        /// </summary>  
        public const EventTags CoreData = (EventTags)0x00080000;

        /// <summary>  
        /// Add InjectXToken to eventSourceOptions.Tags to indicate that an XBOX  
        /// identity token should be injected into the event before the event is  
        /// uploaded.  
        /// </summary>  
        public const EventTags InjectXToken = (EventTags)0x00100000;

        /// <summary>  
        /// Add RealtimeLatency to eventSourceOptions.Tags to indicate that an event  
        /// should be transmitted in real time (via any available connection).  
        /// </summary>  
        public const EventTags RealtimeLatency = (EventTags)0x0200000;

        /// <summary>  
        /// Add NormalLatency to eventSourceOptions.Tags to indicate that an event  
        /// should be transmitted via the preferred connection based on device policy.  
        /// </summary>  
        public const EventTags NormalLatency = (EventTags)0x0400000;

        /// <summary>  
        /// Add CriticalPersistence to eventSourceOptions.Tags to indicate that an  
        /// event should be deleted last when low on spool space.  
        /// </summary>  
        public const EventTags CriticalPersistence = (EventTags)0x0800000;

        /// <summary>  
        /// Add NormalPersistence to eventSourceOptions.Tags to indicate that an event  
        /// should be deleted first when low on spool space.  
        /// </summary>  
        public const EventTags NormalPersistence = (EventTags)0x1000000;

        /// <summary>  
        /// Add DropPii to eventSourceOptions.Tags to indicate that an event contains  
        /// PII and should be anonymized by the telemetry client. If this tag is  
        /// present, PartA fields that might allow identification or cross-event  
        /// correlation will be removed from the event.  
        /// </summary>  
        public const EventTags DropPii = (EventTags)0x02000000;

        /// <summary>  
        /// Add HashPii to eventSourceOptions.Tags to indicate that an event contains  
        /// PII and should be anonymized by the telemetry client. If this tag is  
        /// present, PartA fields that might allow identification or cross-event  
        /// correlation will be hashed (obfuscated).  
        /// </summary>  
        public const EventTags HashPii = (EventTags)0x04000000;

        /// <summary>  
        /// Add MarkPii to eventSourceOptions.Tags to indicate that an event contains  
        /// PII but may be uploaded as-is. If this tag is present, the event will be  
        /// marked so that it will only appear on the Asimov private stream.  
        /// </summary>  
        public const EventTags MarkPii = (EventTags)0x08000000;

        /// <summary>  
        /// Add DropPiiField to eventFieldAttribute.Tags to indicate that a field  
        /// contains PII and should be dropped by the telemetry client.  
        /// </summary>  
        public const EventFieldTags DropPiiField = (EventFieldTags)0x04000000;

        /// <summary>  
        /// Add HashPiiField to eventFieldAttribute.Tags to indicate that a field  
        /// contains PII and should be hashed (obfuscated) prior to uploading.  
        /// </summary>  
        public const EventFieldTags HashPiiField = (EventFieldTags)0x08000000;

        /// <summary>
        /// Microsoft OSG Telemetry Group GUID
        /// </summary>
        private static readonly string[] telemetryTraits = { "ETW_GROUP", "{4f50731a-89cf-4782-b3e0-dce8c90476ba}" };

        /// <summary>  
        /// Constructs a new instance of the WCFTelemetryEventSource class with the  
        /// specified name. Sets the EtwSelfDescribingEventFormat option and joins the  
        /// MicrosoftTelemetry group.  
        /// </summary>  
        /// <param name="eventSourceName">The name of the event source.</param>  
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Shared class with tiny helper methods - not all constructors/methods are used by all consumers")]
        public TelemetryEventSource(string eventSourceName)
            : base(eventSourceName, EventSourceSettings.EtwSelfDescribingEventFormat, telemetryTraits)
        {
        }

        public static EventSourceOptions MeasuresOptions()
        {
            EventSourceOptions eventSourceOptions = new EventSourceOptions() { Keywords = MeasuresKeyword };

            return eventSourceOptions;
        }
    }
}
