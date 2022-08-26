//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Security.AccessControl;
    using System.Security;
    using System.Security.Principal;
    using System.Runtime.ConstrainedExecution;
    using System.Diagnostics;
    using System.IO;

    using Microsoft.Win32;
    using Microsoft.Win32.SafeHandles;

    static class RegistryHelper
    {
        class StringFinder
        {
            string target;
            public StringFinder(string target)
            {
                this.target = target;
            }

            public bool IsTarget(string str)
            {
                return string.CompareOrdinal(str, target) == 0;
            }
        }

        internal static void DeleteKey(string key, string subKeyToDelete)
        {
            RegistryExceptionHelper registryExceptionHelper = new RegistryExceptionHelper(string.Empty, RegistryHive.LocalMachine, key);

            try
            {
                RegistryKey regKey = Registry.LocalMachine.OpenSubKey(
                                            key,
                                            RegistryKeyPermissionCheck.ReadWriteSubTree,
                                            RegistryRights.FullControl);

                if (regKey != null)
                {
                    using (regKey)
                    {
                        string[] subKeys = regKey.GetSubKeyNames();

                        if (subKeys != null && Array.FindIndex<string>(subKeys, new StringFinder(subKeyToDelete).IsTarget) != -1)
                        {
                            regKey.DeleteSubKeyTree(subKeyToDelete);
                        }
                    }
                }
            }
            catch (SecurityException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(subKeyToDelete, e); 
            }
            catch (ObjectDisposedException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(subKeyToDelete, e); 
            }
            catch (ArgumentNullException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(subKeyToDelete, e); 
            }
            catch (ArgumentException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(subKeyToDelete, e); 
            }
            catch (UnauthorizedAccessException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(subKeyToDelete, e); 
            }
        }

        internal static void DeleteValue(string key, string name)
        {
            RegistryExceptionHelper registryExceptionHelper = new RegistryExceptionHelper(string.Empty, RegistryHive.LocalMachine, key);

            try
            {
                RegistryKey regKey = Registry.LocalMachine.OpenSubKey(
                                            key,
                                            RegistryKeyPermissionCheck.ReadWriteSubTree,
                                            RegistryRights.FullControl);
                if (regKey != null)
                {
                    using (regKey)
                    {
                        string[] valueNames = regKey.GetValueNames();
                        if (valueNames != null && Array.FindIndex<string>(valueNames, new StringFinder(name).IsTarget) != -1)
                        {
                            regKey.DeleteValue(name, false);
                        }
                    }
                }
            }
            catch (SecurityException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(name, e);
            }
            catch (ObjectDisposedException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(name, e);
            }
            catch (ArgumentNullException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(name, e);
            }
            catch (ArgumentException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(name, e);
            }
            catch (UnauthorizedAccessException e)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(name, e);
            }
        }
    }
}
