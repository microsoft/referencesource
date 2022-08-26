//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.IO;
    using System.Text;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    static class Utilities
    {
        static Nullable<bool> isHttpApiLibAvailable;
        static string localizedMyComputerString;

        internal static void Log(string s)
        {
            /*using (StreamWriter sw = new StreamWriter("c:\\wsat.log", true))
            {
                sw.WriteLine(s);
                sw.Flush();
                sw.Close();
            }*/
        }
        internal static bool SafeCompare(string s1, string s2)
        {
            return string.Compare(s1, s2, StringComparison.OrdinalIgnoreCase) == 0;
        }

        internal static int OSMajor
        {
            get { return System.Environment.OSVersion.Version.Major; }
        }

        internal static int GetOSMajor(string machineName)
        {
            if (IsLocalMachineName(machineName))
            {
                return OSMajor;
            }

            int result = 0;

            try
            {
                IntPtr workstationPtr;
                const int WorkstationInformationLevel = 100;
                int hr = SafeNativeMethods.NetWkstaGetInfo(
                        @"\\" + machineName,
                        WorkstationInformationLevel,
                        out workstationPtr);

                if (hr == SafeNativeMethods.NERR_Success && workstationPtr != IntPtr.Zero)
                {
                    try
                    {
                        SafeNativeMethods.WKSTA_INFO_100 workstationInfo = (SafeNativeMethods.WKSTA_INFO_100)Marshal.PtrToStructure(workstationPtr, typeof(SafeNativeMethods.WKSTA_INFO_100));
                        result = workstationInfo.ver_major;
                    }
                    finally
                    {
#pragma warning suppress 56523
                        hr = SafeNativeMethods.NetApiBufferFree(workstationPtr);
                    }
                    workstationPtr = IntPtr.Zero;
                }
            }
#pragma warning suppress 56500
            catch (Exception ex)
            {
                if (Utilities.IsCriticalException(ex))
                {
                    throw;
                }
            }

            return result;
        }

#if WSAT_CMDLINE
        internal static string GetEnabledStatusString(bool enabled)
        {
            return enabled ? SR.GetString(SR.ConfigStatusEnabled) : SR.GetString(SR.ConfigStatusDiabled);
        }
#endif

        // We do not parse ActivityTracing in this function
        internal static bool ParseSourceLevel(string levelString, out SourceLevels level)
        {
            level = SourceLevels.Off;

            if (Utilities.SafeCompare(levelString, CommandLineOption.TracingOff))
            {
                level = SourceLevels.Off;
            }
            else if (Utilities.SafeCompare(levelString, CommandLineOption.TracingError))
            {
                level = SourceLevels.Error;
            }
            else if (Utilities.SafeCompare(levelString, CommandLineOption.TracingCritical))
            {
                level = SourceLevels.Critical;
            }
            else if (Utilities.SafeCompare(levelString, CommandLineOption.TracingWarning))
            {
                level = SourceLevels.Warning;
            }
            else if (Utilities.SafeCompare(levelString, CommandLineOption.TracingInformation))
            {
                level = SourceLevels.Information;
            }
            else if (Utilities.SafeCompare(levelString, CommandLineOption.TracingVerbose))
            {
                level = SourceLevels.Verbose;
            }
            else if (Utilities.SafeCompare(levelString, CommandLineOption.TracingAll))
            {
                level = SourceLevels.All;
            }
            else
            {
                return false;
            }

            return true;
        }

        internal static string LocalHostName
        {
            get
            {
                string hostName;
                try
                {
                    hostName = System.Net.Dns.GetHostName();
                }
                catch (System.Net.Sockets.SocketException)
                {
                    hostName = null;
                }
                return hostName;
            }
        }
        internal static bool IsLocalMachineName(string machineName)
        {
            const string comResDllName = "ComRes.dll";
            const string englishMyComputer = "My Computer";
            const int IDS_MYCOMPUTER = 1824;    // resource id for My Computer in ComRes.dll

            if (localizedMyComputerString == null)
            {
                // default value
                localizedMyComputerString = englishMyComputer;

#pragma warning suppress 56523
                IntPtr comResDll = SafeNativeMethods.LoadLibrary(comResDllName);
                if (comResDll != IntPtr.Zero)
                {
                    StringBuilder sb;
                    int len, size = 16;
                    do
                    {
                        sb = new StringBuilder(size);
#pragma warning suppress 56523
                        len = SafeNativeMethods.LoadString(comResDll, IDS_MYCOMPUTER, sb, size);
                        if (len > 0 && len < size)
                        {
                            localizedMyComputerString = sb.ToString();
                            break;
                        }
                        size *= 2;
                    } while (len > 0);
#pragma warning suppress 56523
                    SafeNativeMethods.FreeLibrary(comResDll);
                }
            }

            return (string.IsNullOrEmpty(machineName) || SafeCompare(machineName, "localhost") ||
                    SafeCompare(machineName, localizedMyComputerString) || SafeCompare(machineName, LocalHostName));
        }

        // code from System\ServiceModel\Install\InstallUtil.cs
        internal static bool IsHttpApiLibAvailable
        {
            get
            {
                if (!isHttpApiLibAvailable.HasValue)
                {
                    isHttpApiLibAvailable = IsHttpApiLibAvailableHelper();
                }

                return (bool)isHttpApiLibAvailable.Value;
            }
        }

        static bool IsHttpApiLibAvailableHelper()
        {
            bool retVal = false;
            int retCode = SafeNativeMethods.NoError;

            try
            {
                retCode = SafeNativeMethods.HttpInitialize(new HttpApiVersion(1, 0), SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                SafeNativeMethods.HttpTerminate(SafeNativeMethods.HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                if (retCode != SafeNativeMethods.NoError)
                {
                    retVal = false;
                }
                else
                {
                    retVal = true;
                }
            }
            catch (System.IO.FileNotFoundException)
            {
                retVal = false;
            }

            return retVal;
        }

        internal static bool IsCriticalException(Exception e)
        {
            return (e is System.AccessViolationException) || (e is System.StackOverflowException) ||
                (e is System.OutOfMemoryException) || (e is System.InvalidOperationException) ||
                (e is System.Threading.ThreadAbortException);
        }
    }
}
