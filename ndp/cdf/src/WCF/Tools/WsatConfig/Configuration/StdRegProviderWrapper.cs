//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.IO;
    using System.Management;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.AccessControl;
    using System.Security.Principal;
    using Microsoft.Win32;

    class StdRegProviderWrapper : IDisposable
    {
        ManagementClass regClassInstance;
        string subKey;
        uint hiveValue;
        
        bool ensuredSubKeyExists;
        bool ensuredReadAccess;
        bool ensuredWriteAccess;

        RegistryExceptionHelper registryExceptionHelper;

        static class StdRegProvMethods
        {
            public const string GetDwordValue = "GetDWORDValue";
            public const string GetStringValue = "GetStringValue";
            public const string GetMultiStringValue = "GetMultiStringValue";
            public const string SetDwordValue = "SetDWORDValue";
            public const string SetStringValue = "SetStringValue";
            public const string SetMultiStringValue = "SetMultiStringValue";
            public const string EnumKey = "EnumKey";
            public const string CreateRegistryKey = "CreateKey";
            public const string CheckAccess = "CheckAccess";
        }

        static class InputParameters
        {
            public const string DefKey = "hDefKey";
            public const string SubKeyName = "sSubKeyName";
            public const string ValueName = "sValueName";
            public const string AccessPermission = "uRequired";

            public const string DwordValueKey = "uValue";
            public const string StringValueKey = "sValue";
        }

        static class OutputParameters
        {
            public const string IsAccessGranted = "bGranted";
            public const string SubKeyNames = "sNames";
            public const string ReturnValue = "ReturnValue";
        }

        public StdRegProviderWrapper(RegistryHive registryHive, string subKey, string machineName)
        {
            switch (registryHive)
            {
                case RegistryHive.ClassesRoot:
                    this.hiveValue = 0x80000000;
                    break;
                case RegistryHive.CurrentUser:
                    this.hiveValue = 0x80000001;
                    break;
                case RegistryHive.LocalMachine:
                    this.hiveValue = 0x80000002;
                    break;
                default:
                    // We do not support other values here
                    throw new ArgumentException("remoteHive");
            }

            registryExceptionHelper = new RegistryExceptionHelper(machineName, registryHive, subKey);

            try
            {
                ConnectionOptions co = null;

                if (Utilities.IsLocalMachineName(machineName))
                {
                    machineName = "localhost";
                }
                else
                {
                    co = new ConnectionOptions();
                    co.Authentication = AuthenticationLevel.PacketPrivacy;
                    co.Impersonation = ImpersonationLevel.Impersonate;
                }

                ManagementScope managementScope = new ManagementScope("\\\\" + machineName + "\\root\\DEFAULT", co);
                ManagementPath managementPath = new ManagementPath("StdRegProv");
                ObjectGetOptions options = new ObjectGetOptions(new ManagementNamedValueCollection(), TimeSpan.FromSeconds(15), false);
                this.regClassInstance = new ManagementClass(managementScope, managementPath, options);
                this.subKey = subKey;
            }
            catch (ManagementException e)
            {
                throw registryExceptionHelper.CreateRegistryAccessException(e);
            }
            catch (COMException e) // for RPC_S_SERVER_UNAVAILABLE sort of errors
            {
                throw registryExceptionHelper.CreateRegistryAccessException(e);
            }
        }

        StdRegProviderWrapper(uint hiveValue, string subKey, ManagementClass regClassInstance)
        {
            this.hiveValue = hiveValue;
            this.subKey = subKey;
            this.registryExceptionHelper = new RegistryExceptionHelper(subKey);
            this.regClassInstance = new ManagementClass(regClassInstance.Path, regClassInstance.Options);
        }

        internal StdRegProviderWrapper OpenKey(string subKey)
        {
            string s = this.subKey;
            RegistryExceptionHelper.EnsureEndsWithSlash(ref s);

            s += subKey;

            return new StdRegProviderWrapper(this.hiveValue, s, regClassInstance);
        }

        internal uint ReadUInt32(string name, uint defaultValue)
        {
            return (uint)DoReadData(name, InputParameters.DwordValueKey, defaultValue, StdRegProvMethods.GetDwordValue);
        }

        internal string ReadString(string name, string defaultValue)
        {
            return (string)DoReadData(name, InputParameters.StringValueKey, defaultValue, StdRegProvMethods.GetStringValue);
        }

        internal string[] ReadMultiString(string name, string[] defaultValue)
        {
            return (string[])DoReadData(name, InputParameters.StringValueKey, defaultValue, StdRegProvMethods.GetMultiStringValue);
        }

        internal void WriteUInt32(string name, uint value)
        {
            DoWriteData(name, InputParameters.DwordValueKey, value, StdRegProvMethods.SetDwordValue);
        }

        internal void WriteString(string name, string value)
        {
            DoWriteData(name, InputParameters.StringValueKey, value, StdRegProvMethods.SetStringValue);
        }

        internal void WriteMultiString(string name, string[] value)
        {
            DoWriteData(name, InputParameters.StringValueKey, value, StdRegProvMethods.SetMultiStringValue);
        }

        object DoReadData(string name, string valueKey, object defaultValue, string readMethod)
        {
            EnsureReadAccess();

            try
            {
                ManagementBaseObject inParams = regClassInstance.GetMethodParameters(readMethod);

                inParams[InputParameters.DefKey] = this.hiveValue;
                inParams[InputParameters.SubKeyName] = subKey;
                inParams[InputParameters.ValueName] = name;

                ManagementBaseObject outParams = regClassInstance.InvokeMethod(readMethod,
                                                                                inParams, null);
                uint ret = (uint)outParams[OutputParameters.ReturnValue];
                if (ret == 0) // zero means success
                {
                    return outParams[valueKey];
                }
                return defaultValue;
            }
#pragma warning suppress 56500
            catch (Exception e)
            {
                // MSDN does not have a spec of possible exceptions for the APIs used above.
                // To be safe, we should be a bit more generic in catching exceptions
                if (Utilities.IsCriticalException(e))
                {
                    throw;
                }
                throw registryExceptionHelper.CreateRegistryAccessException(name, e);
            }
        }

        void DoWriteData(string name, string valueKey, object value, string writeMethod)
        {
            EnsureSubKeyExists();
            EnsureWriteAccess();

            try
            {
                ManagementBaseObject inParams = regClassInstance.GetMethodParameters(writeMethod);

                inParams[InputParameters.DefKey] = this.hiveValue;
                inParams[InputParameters.SubKeyName] = subKey;
                inParams[InputParameters.ValueName] = name;
                inParams[valueKey] = value;

                ManagementBaseObject outParams = regClassInstance.InvokeMethod(writeMethod,
                                                                                inParams, null);
                uint ret = (uint)outParams[OutputParameters.ReturnValue];
                if (ret != 0) // zero means success
                {
                    string registryKey = this.subKey;
                    RegistryExceptionHelper.EnsureEndsWithSlash(ref registryKey);
                    registryKey += name;

                    registryExceptionHelper.CreateRegistryWriteException(registryKey, null);
                }
            }
#pragma warning suppress 56500
            catch (Exception e)
            {
                // MSDN does not have a spec of possible exceptions for the APIs used above.
                // To be safe, we should be a bit more generic in catching exceptions
                if (Utilities.IsCriticalException(e))
                {
                    throw;
                }
                throw registryExceptionHelper.CreateRegistryAccessException(name, e);
            }
        }

        void EnsureSubKeyExists()
        {
            try
            {
                if (!ensuredSubKeyExists)
                {
                    ManagementBaseObject inParams = regClassInstance.GetMethodParameters(StdRegProvMethods.CreateRegistryKey);
                    inParams[InputParameters.DefKey] = this.hiveValue;
                    inParams[InputParameters.SubKeyName] = this.subKey;

                    ManagementBaseObject outParams = regClassInstance.InvokeMethod(StdRegProvMethods.CreateRegistryKey, inParams, null);
                    uint ret = (uint)outParams[OutputParameters.ReturnValue];
                    if (ret != 0) // zero means success
                    {
                        throw registryExceptionHelper.CreateRegistryAccessException(ret);
                    }
                    ensuredSubKeyExists = true;
                }
            }
#pragma warning suppress 56500
            catch (Exception e)
            {
                // MSDN does not have a spec of possible exceptions for the APIs used above.
                // To be safe, we should be a bit more generic in catching exceptions
                if (Utilities.IsCriticalException(e))
                {
                    throw;
                }
                throw registryExceptionHelper.CreateRegistryAccessException(e);
            }
        }

        const uint ERROR_ACCESS_DENIED = 5;

        bool CheckRegistryAccess(UInt32 accessPermission, out bool isAccessGranted)
        {
            ManagementBaseObject inParams = null;

            try
            {
                inParams = regClassInstance.GetMethodParameters(StdRegProvMethods.CheckAccess);
            }
#pragma warning suppress 56500
            catch (Exception e)
            {
                // MSDN does not have a spec of possible exceptions for the APIs used above.
                // To be safe, we should be a bit more generic in catching exceptions
                if (Utilities.IsCriticalException(e))
                {
                    throw;
                }
                throw registryExceptionHelper.CreateRegistryAccessException(e);
            }

            inParams[InputParameters.DefKey] = this.hiveValue;
            inParams[InputParameters.SubKeyName] = this.subKey;
            inParams[InputParameters.AccessPermission] = accessPermission;

            ManagementBaseObject outParams = regClassInstance.InvokeMethod(StdRegProvMethods.CheckAccess, inParams, null);
            uint ret = (uint)outParams[OutputParameters.ReturnValue];
            isAccessGranted = (bool)outParams[OutputParameters.IsAccessGranted];
            return ret == 0 || ret == ERROR_ACCESS_DENIED;
        }

        const UInt32 KEY_QUERY_VALUE = 0x01;
        const UInt32 KEY_SET_VALUE = 0x02;

        void EnsureReadAccess()
        {
            if (!ensuredReadAccess)
            {
                bool accessGranted = false;
                if (CheckRegistryAccess(KEY_QUERY_VALUE, out accessGranted))
                {
                    if (!accessGranted)
                    {
                        throw registryExceptionHelper.CreateRegistryAccessException(null);
                    }
                    ensuredReadAccess = true;
                }
            }
        }

        void EnsureWriteAccess()
        {
            if (!ensuredWriteAccess)
            {
                bool accessGranted = false;
                if (CheckRegistryAccess(KEY_QUERY_VALUE | KEY_SET_VALUE, out accessGranted))
                {
                    if (!accessGranted)
                    {
                        throw registryExceptionHelper.CreateRegistryWriteException(null);
                    }
                    ensuredWriteAccess = true;
                }
            }
        }

        internal void AdjustRegKeyPermission()
        {
            try
            {
                RegistryKey regKey;
                regKey = Registry.LocalMachine.OpenSubKey(
                    WsatKeys.WsatRegKey,
                    RegistryKeyPermissionCheck.ReadWriteSubTree,
                    RegistryRights.FullControl);

                if (regKey != null)
                {
                    using (regKey)
                    {
                        // NetworkService always needs access to the WS-AT key
                        // On some platforms, it doesn't inherit this permission from the parent MSDTC key
                        RegistryAccessRule rule = new RegistryAccessRule(
                            new SecurityIdentifier(WellKnownSidType.NetworkServiceSid, null),
                            RegistryRights.ReadKey,
                            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit,
                            PropagationFlags.None,
                            AccessControlType.Allow);

                        // Ensure the authenticated users have read access to the WS-AT key
                        // there is a key under the WS-AT key named OleTxUpgradeEnabled that requires the permission
                        RegistryAccessRule rule2 = new RegistryAccessRule(
                            new SecurityIdentifier(WellKnownSidType.AuthenticatedUserSid, null),
                            RegistryRights.ReadKey,
                            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit,
                            PropagationFlags.None,
                            AccessControlType.Allow);

                        RegistrySecurity registrySecurity = regKey.GetAccessControl();
                        registrySecurity.AddAccessRule(rule);
                        registrySecurity.AddAccessRule(rule2);
                        regKey.SetAccessControl(registrySecurity);
                    }
                }
            }
            catch (SecurityException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(e);
            }
            catch (ObjectDisposedException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(e);
            }
            catch (ArgumentNullException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(e);
            }
            catch (ArgumentException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(e);
            }
            catch (UnauthorizedAccessException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(e);
            }        
        }

        public void Dispose()
        {
            if (regClassInstance != null)
            {
                regClassInstance.Dispose();
                regClassInstance = null;
            }
        }
    }
}
