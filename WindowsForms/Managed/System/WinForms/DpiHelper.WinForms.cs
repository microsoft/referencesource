//------------------------------------------------------------------------------
// <copyright file="DpiHelper.Microsoft.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System.Configuration;

    /// <summary>
    /// Part of DpiHelper class, with methods specific to Microsoft assembly
    /// Also handles configuration switches that control states of various Microsoft features
    /// </summary>
    internal static partial class DpiHelper
    {        
        /// <summary>
        /// Sets DPI awareness for the process by reading configuration value from app.config file
        /// </summary>
        /// <returns>true/false - If process DPI awareness is successfully set, returns true. Otherwise false.</returns>
        public static bool SetWinformsApplicationDpiAwareness()
        {
            NativeMethods.PROCESS_DPI_AWARENESS dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_DPI_UNINITIALIZED;
            Version currentOSVersion = Environment.OSVersion.Version;
            if (!IsDpiAwarenessValueSet() || Environment.OSVersion.Platform != System.PlatformID.Win32NT)
            {
                return false;
            }

            string dpiAwareness = (dpiAwarenessValue ?? string.Empty).ToLowerInvariant();
            if (currentOSVersion.CompareTo(ConfigurationOptions.RS2Version) >= 0)
            {
                int rs2AndAboveDpiFlag;
                switch (dpiAwareness)
                {
                    case "true":
                    case "system":
                        rs2AndAboveDpiFlag = NativeMethods.DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
                        break;
                    case "true/pm":
                    case "permonitor":
                        rs2AndAboveDpiFlag = NativeMethods.DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
                        break;
                    case "permonitorv2":
                        rs2AndAboveDpiFlag = NativeMethods.DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
                        break;
                    case "false":
                    default:
                        rs2AndAboveDpiFlag = NativeMethods.DPI_AWARENESS_CONTEXT_UNAWARE;
                        break;
                }
                if (!SafeNativeMethods.SetProcessDpiAwarenessContext(rs2AndAboveDpiFlag))
                {
                    return false;
                }
            }
            // For operating systems windows 8.1 to Windows 10 redstone 1 version.
            else if (currentOSVersion.CompareTo(new Version(6, 3, 0, 0)) >= 0 && currentOSVersion.CompareTo(ConfigurationOptions.RS2Version) < 0)
            {
                switch (dpiAwareness)
                {
                    case "false":
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_DPI_UNAWARE;
                        break;
                    case "true":
                    case "system":
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_SYSTEM_DPI_AWARE;
                        break;
                    case "true/pm":
                    case "permonitor":
                    case "permonitorv2":
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_PER_MONITOR_DPI_AWARE;
                        break;
                    default:
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_DPI_UNINITIALIZED;
                        break;
                }

                if (SafeNativeMethods.SetProcessDpiAwareness(dpiFlag) != NativeMethods.S_OK)
                {
                    return false;
                }

            }
            // For operating systems windows 7 to windows 8
            else if (currentOSVersion.CompareTo(new Version(6, 1, 0, 0)) >= 0 && currentOSVersion.CompareTo(new Version(6, 3, 0, 0)) < 0)
            {
                switch (dpiAwareness)
                {
                    case "false":
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_DPI_UNAWARE;
                        break;
                    case "true":
                    case "system":
                    case "true/pm":
                    case "permonitor":
                    case "permonitorv2":
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_SYSTEM_DPI_AWARE;
                        break;
                    default:
                        dpiFlag = NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_DPI_UNINITIALIZED;
                        break;
                }

                if (dpiFlag == NativeMethods.PROCESS_DPI_AWARENESS.PROCESS_SYSTEM_DPI_AWARE)
                {
                    if (!SafeNativeMethods.SetProcessDPIAware())
                    {
                        return false;
                    }
                }
            }
            // For windows vista and below, this is not a supported feature.
            else
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Returns a new Padding with the input's
        /// dimensions converted from logical units to device units.
        /// </summary>
        /// <param name="logicalPadding">Padding in logical units</param>
        /// <returns>Padding in device units</returns>
        public static Padding LogicalToDeviceUnits(Padding logicalPadding, int deviceDpi = 0)
        {
            return new Padding(LogicalToDeviceUnits(logicalPadding.Left, deviceDpi),
                               LogicalToDeviceUnits(logicalPadding.Top, deviceDpi),
                               LogicalToDeviceUnits(logicalPadding.Right, deviceDpi),
                               LogicalToDeviceUnits(logicalPadding.Bottom, deviceDpi));
        }
    }
}
