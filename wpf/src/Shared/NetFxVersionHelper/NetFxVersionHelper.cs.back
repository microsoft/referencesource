using System;
using System.IO;
using System.Runtime.Versioning;
using System.Security;
using System.Security.Permissions;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace MS.Internal
{
    /// <summary>
    /// Helper to get NetFx version details
    /// </summary>
    internal static class NetfxVersionHelper
    {
        /// <summary>
        /// Gets the release version of dotnet framework from registry
        /// </summary>
        /// <returns>returns release value if successful, 0 otherwise</returns>
        /// <SecurityNote>
        ///     Critical: This code elevates to open an HKLM registry location and reads it.
        ///     Safe: The information it exposes is safe to give out
        /// </SecurityNote>
        [SecuritySafeCritical]
        internal static int GetNetFXReleaseVersion()
        {
            int releaseVersion = 0;
            RegistryKey releaseKey;
            RegistryPermission regPerm = new RegistryPermission(RegistryPermissionAccess.Read, _frameworkRegKeyFullPath);
            try
            {
                regPerm.Assert();
                releaseKey = Registry.LocalMachine.OpenSubKey(_frameworkRegKey);
                if (releaseKey != null)
                {
                    object release = releaseKey.GetValue("Release");
                    if (release != null)
                    {
                        releaseVersion = Convert.ToInt32(release);
                    }
                }
            }
            catch(Exception e) when (e is SecurityException || e is ObjectDisposedException || e is IOException || e is UnauthorizedAccessException || e is FormatException || e is OverflowException)
            {
                // in case of any exception we want 0 to be returned
                // so do nothing
            }
            finally
            {
                RegistryPermission.RevertAssert();
            }
            return releaseVersion;
        }

        /// <summary>
        /// Gets the Netfx target framework version
        /// </summary>
        /// <returns>netfx target framework version if successful, empty string otherwise</returns>
        internal static string GetTargetFrameworkVersion()
        {
            string frameworkVersion = string.Empty;
            string targetFrameworkName = AppDomain.CurrentDomain.SetupInformation.TargetFrameworkName;
            if (!string.IsNullOrEmpty(targetFrameworkName))
            {
                try
                {
                    FrameworkName frameworkName = new FrameworkName(targetFrameworkName);
                    frameworkVersion = frameworkName.Version.ToString();
                }
                catch (Exception e) when (e is ArgumentException)
                {
                    // do nothing as we want empty string to be returned in case of exception
                }
            }
            return frameworkVersion;
        }

        /// <summary>
        /// Registry path for the netfx framework excluding HKLM
        /// </summary>
        internal static string FrameworkRegKey
        {
            get
            {
                return _frameworkRegKey;
            }
        }

        /// <summary>
        /// Registry path for the netfx framework including HKLM
        /// </summary>
        internal static string FrameworkRegKeyFullPath
        {
            get
            {
                return _frameworkRegKeyFullPath;
            }
        }

        private static readonly string _frameworkRegKey = @"SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full";
        private static readonly string _frameworkRegKeyFullPath = @"HKEY_LOCAL_MACHINE\" + _frameworkRegKey;
    }
}
