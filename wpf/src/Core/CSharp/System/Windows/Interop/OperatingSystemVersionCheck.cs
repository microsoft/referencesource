//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Interop
{
    /// <summary>
    ///     Performs a simple check of the current operating system version.
    ///     Use this class to determine whether version specific features should
    ///     be enabled.
    ///     
    /// DevDiv:1158540
    /// This is an older version check class that is susceptible to AppCompat version lies.
    /// For a more complete (and updated) class, please see OSVersionHelper.cs
    /// </summary>
    internal static class OperatingSystemVersionCheck
    {
        internal static bool IsVersionOrLater(OperatingSystemVersion version)
        {
            // 
            int major;
            int minor;
            PlatformID platform = PlatformID.Win32NT;
            switch (version)
            {
                case OperatingSystemVersion.Windows8:
                    major = 6;
                    minor = 2;
                    break;
                
                case OperatingSystemVersion.Windows7:
                    major = 6;
                    minor = 1;
                    break;

                case OperatingSystemVersion.WindowsVista:
                    major = 6;
                    minor = 0;
                    break;

                case OperatingSystemVersion.WindowsXPSP2:
                default:
                    major = 5;
                    minor = 1;
                    break;
            }

            OperatingSystem os = Environment.OSVersion;
            return (os.Platform == platform) && 
                (((os.Version.Major == major) && (os.Version.Minor >= minor)) || (os.Version.Major > major));
        }
    }
}
