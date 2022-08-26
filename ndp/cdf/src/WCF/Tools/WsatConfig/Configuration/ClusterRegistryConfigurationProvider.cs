//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------
//
// Implement the ConfigurationProvider base class for the cluster registry
//
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.AccessControl;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.Text;

    using Microsoft.Win32;
    
    class ClusterRegistryConfigurationProvider : ConfigurationProvider
    {
        SafeHKey hKey;
        RegistryExceptionHelper registryExceptionHelper = null;
        string registryKey;

        [SecurityCritical]
        internal ClusterRegistryConfigurationProvider(SafeHResource hResource, string key)
        {            
            SafeHKey rootKey = SafeNativeMethods.GetClusterResourceKey(hResource,
                                                                RegistryRights.ReadKey |
                                                                RegistryRights.EnumerateSubKeys |
                                                                RegistryRights.QueryValues);
            if (rootKey.IsInvalid)
            {
                int lastError = Marshal.GetLastWin32Error();
                throw new WsatAdminException(WsatAdminErrorCode.REGISTRY_ACCESS, SR.GetString(SR.CannotOpenClusterRegistry, lastError));
            }

            if (string.IsNullOrEmpty(key))
            {
                hKey = rootKey;
            }
            else
            {
                using (rootKey)
                {
                    int disposition;
                    int ret = SafeNativeMethods.ClusterRegCreateKey(rootKey,
                                                                    key,
                                                                    SafeNativeMethods.REG_OPTION_NON_VOLATILE,
                                                                    RegistryRights.FullControl,
                                                                    IntPtr.Zero,
                                                                    out this.hKey,
                                                                    out disposition);
                    if (ret != SafeNativeMethods.ERROR_SUCCESS)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.REGISTRY_ACCESS, SR.GetString(SR.CannotOpenClusterRegistry, ret));
                    }
                }
            }

            registryExceptionHelper = new RegistryExceptionHelper(key);
            this.registryKey = key;
            RegistryExceptionHelper.EnsureEndsWithSlash(ref this.registryKey);
        }

        ClusterRegistryConfigurationProvider(SafeHKey key, string registryKey)
        {
            this.hKey = key;

            this.registryExceptionHelper = new RegistryExceptionHelper(registryKey);
            this.registryKey = registryKey;
            RegistryExceptionHelper.EnsureEndsWithSlash(ref registryKey);
        }

        public override void Dispose()
        {
            this.hKey = null;
            GC.SuppressFinalize(this);
        }

        internal override ConfigurationProvider OpenKey(string subKey)
        {
            SafeHKey hSubKey;
            int disposition;
            int ret = SafeNativeMethods.ClusterRegCreateKey(hKey,
                                              subKey,
                                              SafeNativeMethods.REG_OPTION_NON_VOLATILE,
                                              RegistryRights.FullControl,
                                              IntPtr.Zero,
                                              out hSubKey,
                                              out disposition);
            if (ret != SafeNativeMethods.ERROR_SUCCESS)
            {
                throw new WsatAdminException(WsatAdminErrorCode.REGISTRY_ACCESS, SR.GetString(SR.CannotOpenClusterRegistry, ret));
            }

            return new ClusterRegistryConfigurationProvider(hSubKey, registryKey + subKey);
        }

        byte[] QueryValue(string name, RegistryValueKind valueType)
        {
            if (this.hKey == null || this.hKey.IsInvalid)
            {
                return null;
            }

            RegistryValueKind type;
            uint cb = 0;
            int ret = SafeNativeMethods.ClusterRegQueryValue(this.hKey,
                                                             name,
                                                             out type,
                                                             null,
                                                             ref cb);
            Utilities.Log("ClusterRegQueryValue [" + name + "], returned [" + ret + "]");
            if (ret == SafeNativeMethods.ERROR_SUCCESS || ret == SafeNativeMethods.ERROR_MORE_DATA)
            {
                if (valueType != type)
                {
                    return null;
                }

                byte[] buffer = new byte[cb];
                ret = SafeNativeMethods.ClusterRegQueryValue(this.hKey,
                                                             name,
                                                             out type,
                                                             buffer,
                                                             ref cb);
                if (ret == SafeNativeMethods.ERROR_SUCCESS)
                {
                    if (valueType != type)
                    {
                        return null;
                    }

                    return buffer;
                }
            }

            return null;
        }

        void WriteValue(string name, byte[] value, RegistryValueKind valueType)
        {
            if (this.hKey != null && !this.hKey.IsInvalid)
            {
                int ret = SafeNativeMethods.ClusterRegSetValue(this.hKey,
                                                                name,
                                                                valueType,
                                                                value,
                                                                (uint)value.Length);
                if (ret == SafeNativeMethods.ERROR_SUCCESS)
                {
                    return;
                }
            }

            throw registryExceptionHelper.CreateRegistryWriteException(name, null);
        }

        internal override uint ReadUInt32(string name, uint defaultValue)
        {
            byte[] buffer = QueryValue(name, RegistryValueKind.DWord);
            if (buffer == null)
            {
                return defaultValue;
            }

            return (uint)BitConverter.ToUInt32(buffer, 0);
        }

        internal override void WriteUInt32(string name, uint value)
        {
            byte[] buffer = BitConverter.GetBytes(value);
            WriteValue(name, buffer, RegistryValueKind.DWord);
        }

/*        internal override ushort ReadUInt16(string name, ushort defaultValue)
        {
            uint regValue = ReadUInt32(name, defaultValue);

            return (regValue <= (uint)ushort.MaxValue) ? (ushort)regValue : defaultValue;
        }*/

        internal override string ReadString(string value, string defaultValue)
        {
            byte[] buffer = QueryValue(value, RegistryValueKind.String);
            if (buffer == null || buffer.Length < 2)
            {
                return defaultValue;
            }

            return Encoding.Unicode.GetString(buffer, 0, buffer.Length - 2);
        }

        byte[] GetByteArrayFromString(string s)
        {
#pragma warning suppress 56507
            if (s == null)
            {
                s = string.Empty;
            }
            byte[] temp = Encoding.Unicode.GetBytes(s);
            byte[] buffer = new byte[temp.Length + 2];
            Array.Copy(temp, buffer, temp.Length);
            buffer[temp.Length] = 0;
            buffer[temp.Length + 1] = 0;
            return buffer;
        }

        internal override void WriteString(string name, string value)
        {
            byte[] buffer = GetByteArrayFromString(value);
            WriteValue(name, buffer, RegistryValueKind.String);
        }

        internal override string[] ReadMultiString(string value, string[] defaultValue)
        {
            byte[] buffer = QueryValue(value, RegistryValueKind.MultiString);
            if (buffer == null)
            {
                return defaultValue;
            }

            List<string> list = new List<string>(5);
            string item;
            int index = 0;
            while ((item = GetStringFromMultiSz(value, buffer, ref index)) != null)
            {
                list.Add(item);
            }

            return list.ToArray();        
        }

        internal override void WriteMultiString(string name, string[] value)
        {
            int len = 0;
            foreach (string s in value)
            {
                if (!string.IsNullOrEmpty(s))
                {
                    len += s.Length;
                }
            }

            byte[] buffer = new byte[(len + value.Length + 1) * 2];
            int index = 0;
            foreach (string s in value)
            {
                // this alrady contains the null terminator
                byte[] temp = GetByteArrayFromString(s);
                Array.Copy(temp, 0, buffer, index, temp.Length);
                index += temp.Length;
            }
            // The very last Unicode NULL
            buffer[index++] = 0;
            buffer[index++] = 0;

            WriteValue(name, buffer, RegistryValueKind.MultiString);
        }


        string GetStringFromMultiSz(string value, byte[] buffer, ref int index)
        {
            // E.g. abc\0def\0\0
            // This loop terminates when it finds a null terminating character
            // The index will be left pointing at the first null terminator it finds
            int start = index;
            for (;
                 index < buffer.Length - 1 && BitConverter.ToChar(buffer, index) != 0;
                 index += 2);

            // If we found no valid characters, we're done
            if (start == index)
            {
                return null;
            }

            // Skip past the null terminator at which we stopped
            index += 2;

            // Convert the characters we found to a string
            return Encoding.Unicode.GetString(buffer, start, index - start - 2);
        }

        internal override void AdjustRegKeyPermission()
        {
            const uint maxSecurityDescriptorSize = 40960;
            byte[] binarySecurityDescriptor;
            CommonSecurityDescriptor securityDescriptor = null;
            int ret;
            uint dwSize = 256;

            do
            {
                binarySecurityDescriptor = new byte[dwSize];
                ret = SafeNativeMethods.ClusterRegGetKeySecurity(hKey, SecurityInfos.DiscretionaryAcl, binarySecurityDescriptor, ref dwSize);
                if (ret == SafeNativeMethods.ERROR_SUCCESS)
                {
                    break;
                }
                else if (ret == SafeNativeMethods.ERROR_INSUFFICIENT_BUFFER)
                {
                    dwSize *= 2;
                }
                else
                {
                    throw registryExceptionHelper.CreateRegistryWriteException(null);
                }
            } while (dwSize <= maxSecurityDescriptorSize);

            if (dwSize > maxSecurityDescriptorSize)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(null);
            }
                        
            try
            {
                securityDescriptor = new CommonSecurityDescriptor(false, false, binarySecurityDescriptor, 0);
                DiscretionaryAcl dacl = securityDescriptor.DiscretionaryAcl;
                if (dacl.Count == 1)
                {
                    CommonAce ace = dacl[0] as CommonAce;
                    if (ace != null && ace.AceType == AceType.AccessAllowed && ace.SecurityIdentifier.IsWellKnown(WellKnownSidType.WorldSid))
                    {
                        // This is the Allowed for everyone full access ACE that's automatically added by
                        // CommonSecurityDescriptor ctor; we should remove it
                        dacl.Purge(new SecurityIdentifier(WellKnownSidType.WorldSid, null));
                    }
                }

                // Add Read access for Authenticated Users account and Network Service account
                dacl.AddAccess(AccessControlType.Allow, new SecurityIdentifier(WellKnownSidType.AuthenticatedUserSid, null),
                    unchecked((int)0x80000000), InheritanceFlags.None, PropagationFlags.None);
                dacl.AddAccess(AccessControlType.Allow, new SecurityIdentifier(WellKnownSidType.NetworkServiceSid, null),
                    unchecked((int)0x80000000), InheritanceFlags.None, PropagationFlags.None);
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
                throw registryExceptionHelper.CreateRegistryWriteException(e);
            }

            int dsNewSecDescSize = securityDescriptor.BinaryLength;
            byte[] newBinarySecurityDescriptor = new byte[dsNewSecDescSize];
            securityDescriptor.GetBinaryForm(newBinarySecurityDescriptor, 0);

            ret = SafeNativeMethods.ClusterRegSetKeySecurity(hKey, SecurityInfos.DiscretionaryAcl, newBinarySecurityDescriptor);
            if (ret != SafeNativeMethods.ERROR_SUCCESS)
            {
                throw registryExceptionHelper.CreateRegistryWriteException(null);
            }
        }
    }
}
