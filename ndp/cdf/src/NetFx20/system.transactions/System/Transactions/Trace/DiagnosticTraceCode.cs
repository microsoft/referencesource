//-----------------------------------------------------------------------------
// <copyright file="DiagnosticTraceCode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions.Diagnostics
{
    static class DiagnosticTraceCode
    {
        const string Prefix = "http://msdn.microsoft.com/TraceCodes/System/ActivityTracing/2004/07/";
        const string DiagnosticsFeature = "Diagnostics/";
        const string ReliabilityFeature = "Reliability/";
        internal const string ActivityIdSet = Prefix + DiagnosticsFeature + "ActivityId/Set";
        internal const string ActivityName = Prefix + DiagnosticsFeature + "ActivityId/Name";
        internal const string AppDomainUnload = Prefix + DiagnosticsFeature + "AppDomainUnload";
        internal const string NewActivityIdIssued = Prefix + DiagnosticsFeature + "ActivityId/IssuedNew";
        internal const string UnhandledException = Prefix + ReliabilityFeature + "Exception/Unhandled";
    }
}