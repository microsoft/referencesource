using System;
using System.Diagnostics.Tracing;
using MS.Internal;
using MS.Internal.Telemetry;

#if WINDOWS_BASE
using MS.Internal.Telemetry.WindowsBase;
#elif PRESENTATION_CORE
using MS.Internal.Telemetry.PresentationCore;
#elif PRESENTATIONFRAMEWORK
using MS.Internal.Telemetry.PresentationFramework;
#else
#error Attempt to use Tracelogger in an unexpected assembly
#error To use the Tracelogger in this assembly, update TraceLoggingProvider to support it first.
#endif

namespace MS.Internal.Telemetry
{
    /// <summary>
    /// Trace logger for NetFx version details
    /// </summary>
    internal static class NetFxVersionTraceLogger
    {
        /// <summary>
        /// Logs target framework version and release version details
        /// </summary>
        internal static void LogVersionDetails()
        {
            EventSource logger = TraceLoggingProvider.GetProvider();
            VersionInfo versionInfo = new VersionInfo();

            versionInfo.TargetFrameworkVersion = NetfxVersionHelper.GetTargetFrameworkVersion();
            versionInfo.NetfxReleaseVersion = NetfxVersionHelper.GetNetFXReleaseVersion();

            logger?.Write(NetFxVersion, TelemetryEventSource.MeasuresOptions(), versionInfo);
        }

        /// <summary>
        /// Event name for logging target framework version and release version
        /// </summary>
        private static readonly string NetFxVersion = "NetFxVersion";

        /// <summary>
        /// To store the target and release version details
        /// </summary>
        [EventData]
        private struct VersionInfo
        {

            // It has to be public as event source iterates only the public instance properties
            public string TargetFrameworkVersion
            {
                get
                {
                    return _targetFrameworkVersion;
                }
                set
                {
                    _targetFrameworkVersion = value;
                }
            }

            // It has to be public as event source iterates only the public instance properties
            public int NetfxReleaseVersion
            {
                get
                {
                    return _netfxReleaseVersion;
                }
                set
                {
                    _netfxReleaseVersion = value;
                }
            }

            private string _targetFrameworkVersion;
            private int _netfxReleaseVersion;
        }
    }
}
