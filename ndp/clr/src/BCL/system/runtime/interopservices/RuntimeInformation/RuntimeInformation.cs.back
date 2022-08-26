// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Reflection;
using System.Diagnostics;
using System.Security;

namespace System.Runtime.InteropServices
{
    public static partial class RuntimeInformation
    {
        private const string FrameworkName = ".NET Framework";
        private static string s_frameworkDescription;
        private static string s_osDescription = null;
        private static object s_osLock = new object();
        private static object s_processLock = new object();
        private static Architecture? s_osArch = null;
        private static Architecture? s_processArch = null;

        public static string FrameworkDescription
        {
            get
            {
                if (s_frameworkDescription == null)
                {
                    AssemblyFileVersionAttribute attr = (AssemblyFileVersionAttribute)(typeof(object).GetTypeInfo().Assembly.GetCustomAttribute(typeof(AssemblyFileVersionAttribute)));
                    BCLDebug.Assert(attr != null, "AssemblyFileVersionAttribute not found!");
                    s_frameworkDescription = $"{FrameworkName} {attr.Version}";
                }

                return s_frameworkDescription;
            }
        }

        public static bool IsOSPlatform(OSPlatform osPlatform)
        {
            return OSPlatform.Windows == osPlatform;
        }

        public static string OSDescription
        {
            [SecuritySafeCritical]
            get
            {
                if (null == s_osDescription)
                {
                    s_osDescription = RtlGetVersion();
                }

                return s_osDescription;
            }
        }

        public static Architecture OSArchitecture
        {
            [SecuritySafeCritical]
            get
            {
                lock (s_osLock)
                {
                    if (null == s_osArch)
                    {
                        Microsoft.Win32.Win32Native.SYSTEM_INFO sysInfo;
                        Microsoft.Win32.Win32Native.GetNativeSystemInfo(out sysInfo);

                        s_osArch = GetArchitecture(sysInfo.wProcessorArchitecture);
                    }
                }

                BCLDebug.Assert(s_osArch != null, "s_osArch is null");
                return s_osArch.Value;
            }
        }

        public static Architecture ProcessArchitecture
        {
            [SecuritySafeCritical]
            get
            {
                lock (s_processLock)
                {
                    if (null == s_processArch)
                    {
                        Microsoft.Win32.Win32Native.SYSTEM_INFO sysInfo = default(Microsoft.Win32.Win32Native.SYSTEM_INFO);
                        Microsoft.Win32.Win32Native.GetSystemInfo(ref sysInfo);

                        s_processArch = GetArchitecture(sysInfo.wProcessorArchitecture);
                    }
                }

                BCLDebug.Assert(s_processArch != null, "s_processArch is null");
                return s_processArch.Value;
            }
        }

        private static Architecture GetArchitecture(ushort wProcessorArchitecture)
        {
            Architecture arch = Architecture.X86;

            switch(wProcessorArchitecture)
            {
                case (ushort)Microsoft.Win32.Win32Native.ProcessorArchitecture.Processor_Architecture_ARM64:
                    arch = Architecture.Arm64;
                    break;
                case (ushort)Microsoft.Win32.Win32Native.ProcessorArchitecture.Processor_Architecture_ARM:
                    arch = Architecture.Arm;
                    break;
                case (ushort)Microsoft.Win32.Win32Native.ProcessorArchitecture.Processor_Architecture_AMD64:
                    arch = Architecture.X64;
                    break;
                case (ushort)Microsoft.Win32.Win32Native.ProcessorArchitecture.Processor_Architecture_INTEL:
                    arch = Architecture.X86;
                    break;
            }

            return arch;
        }

        [SecuritySafeCritical]
        private static string RtlGetVersion()
        {
            Microsoft.Win32.Win32Native.RTL_OSVERSIONINFOEX osvi = new Microsoft.Win32.Win32Native.RTL_OSVERSIONINFOEX();
            osvi.dwOSVersionInfoSize = (uint)Marshal.SizeOf(osvi);
            const string version = "Microsoft Windows";
            if (Microsoft.Win32.Win32Native.RtlGetVersion(out osvi) == 0)
            {
                return string.Format("{0} {1}.{2}.{3} {4}",
                    version, osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion);
            }
            else
            {
                return version;
            }
        }
    }
}
