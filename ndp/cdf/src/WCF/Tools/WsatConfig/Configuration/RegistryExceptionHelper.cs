//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using Microsoft.Win32;
    using System;

    class RegistryExceptionHelper
    {
        string registryKey;

        public RegistryExceptionHelper(string registryKey)
        {
            this.registryKey = registryKey;
            EnsureEndsWithSlash(ref this.registryKey);
        }

        public RegistryExceptionHelper(string machineName, RegistryHive registryHive, string registryKeyRelativeToHive)
            : this(RegistryExceptionHelper.GetRegistryKeyBase(machineName, registryHive) + registryKeyRelativeToHive)
        {
        }

        public static void EnsureEndsWithSlash(ref string str)
        {
            if (!string.IsNullOrEmpty(str))
            {
                if (!str.EndsWith("\\", StringComparison.OrdinalIgnoreCase))
                {
                    str += '\\';
                }
            }
        }

        static string GetRegistryKeyBase(string machineName, RegistryHive registryHive)
        {
            string registryBase = Utilities.IsLocalMachineName(machineName) ? string.Empty : SR.GetString(SR.RemoteRegistryFormat, machineName);
            switch (registryHive)
            {
                case RegistryHive.ClassesRoot:
                    registryBase += Registry.ClassesRoot.Name;
                    break;
                case RegistryHive.CurrentUser:
                    registryBase += Registry.CurrentUser.Name;
                    break;
                case RegistryHive.LocalMachine:
                    registryBase += Registry.LocalMachine.Name;
                    break;
                default:
                    // We do not support other values here
                    System.Diagnostics.Debug.Assert(false, "registryHive is not supported");
                    break;
            }
            RegistryExceptionHelper.EnsureEndsWithSlash(ref registryBase);
            return registryBase;
        }

        public WsatAdminException CreateRegistryAccessException(int errorCode)
        {
            return CreateRegistryAccessException(unchecked((uint)errorCode));
        }

        public WsatAdminException CreateRegistryAccessException(uint errorCode)
        {
            return new WsatAdminException(WsatAdminErrorCode.REGISTRY_ACCESS, SR.GetString(SR.ErrorRegistryAccess, registryKey, errorCode));
        }

        public WsatAdminException CreateRegistryAccessException(Exception innerException)
        {
            return DoCreateRegistryAccessException(registryKey, innerException);
        }

        public WsatAdminException CreateRegistryAccessException(string subRegistryKey, Exception innerException)
        {
            return DoCreateRegistryAccessException(registryKey + subRegistryKey, innerException);
        }

        static WsatAdminException DoCreateRegistryAccessException(string regKey, Exception innerException)
        {
            if (innerException == null)
            {
                return new WsatAdminException(WsatAdminErrorCode.REGISTRY_ACCESS, SR.GetString(SR.ErrorRegistryAccessNoErrorCode, regKey));
            }
            return new WsatAdminException(WsatAdminErrorCode.REGISTRY_ACCESS, SR.GetString(SR.ErrorRegistryAccessNoErrorCode, regKey), innerException);
        }

        public WsatAdminException CreateRegistryWriteException(Exception innerException)
        {
            return DoCreateRegistryWriteException(registryKey, innerException);
        }

        public WsatAdminException CreateRegistryWriteException(string subRegistryKey, Exception innerException)
        {
            return DoCreateRegistryWriteException(registryKey + subRegistryKey, innerException);
        }

        static WsatAdminException DoCreateRegistryWriteException(string regKey, Exception innerException)
        {
            if (innerException == null)
            {
                return new WsatAdminException(WsatAdminErrorCode.REGISTRY_WRITE, SR.GetString(SR.ErrorRegistryWrite, regKey));
            }
            return new WsatAdminException(WsatAdminErrorCode.REGISTRY_WRITE, SR.GetString(SR.ErrorRegistryWrite, regKey), innerException);
        }
    }
}
