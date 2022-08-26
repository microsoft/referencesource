//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using Microsoft.Win32;
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Cryptography;

    static class CertificateManager
    {
        static string certificateStore = @"Software\Microsoft\SystemCertificates\My";

        // Throw if cannot open a valid store handle
        [SecurityCritical]
        internal static SafeCertificateStore GetCertificateStorePointer(string machineName)
        {
            SafeCertificateStore storeHandle;

            RegistryExceptionHelper registryExceptionHelper = new RegistryExceptionHelper(machineName, RegistryHive.LocalMachine, certificateStore);

            if (Utilities.IsLocalMachineName(machineName))
            {
                SafeRegistryKey hive = new SafeRegistryKey(new IntPtr((int)Microsoft.Win32.RegistryHive.LocalMachine), false);
                SafeRegistryKey regKey = null;

                try
                {
                    int ret = SafeNativeMethods.RegOpenKeyEx(
                        hive,
                        certificateStore,
                        0,
                        SafeNativeMethods.KEY_READ,
                        out regKey);
                    if (ret != SafeNativeMethods.ERROR_SUCCESS)
                    {
                        throw registryExceptionHelper.CreateRegistryAccessException(ret);
                    }

                    storeHandle = SafeNativeMethods.CertOpenStore_ptr(
                            SafeNativeMethods.CERT_STORE_PROV_REG,
                            0,
                            0,
                            SafeNativeMethods.CERT_STORE_OPEN_EXISTING_FLAG |
                            SafeNativeMethods.CERT_STORE_READONLY_FLAG,
                            regKey);
                    if (storeHandle.IsInvalid)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.CERT_STORE_ACCESS, SR.GetString(SR.ErrorAccessCertStore, Marshal.GetLastWin32Error()));
                    }
                    return storeHandle;
                }
                finally
                {
                    if (regKey != null)
                    {
                        regKey.Close();
                    }
                    hive.Close();
                }
            }
#if WSAT_UI
            else
            {
                SafeRegistryKey remoteBase = null;
                SafeRegistryKey finalKey = null;

                try
                {
                    int ret = SafeNativeMethods.RegConnectRegistry(
                        machineName,
                        new SafeRegistryKey(new IntPtr((int)Microsoft.Win32.RegistryHive.LocalMachine), false),
                        out remoteBase);
                    if (ret != SafeNativeMethods.ERROR_SUCCESS)
                    {
                        throw registryExceptionHelper.CreateRegistryAccessException(ret);
                    }

                    ret = SafeNativeMethods.RegOpenKeyEx(
                        remoteBase,
                        certificateStore,
                        0,
                        SafeNativeMethods.KEY_READ,
                        out finalKey);
                    if (ret != SafeNativeMethods.ERROR_SUCCESS)
                    {
                        throw registryExceptionHelper.CreateRegistryAccessException(ret);
                    }

                    storeHandle = SafeNativeMethods.CertOpenStore_ptr(
                                                    SafeNativeMethods.CERT_STORE_PROV_REG,
                                                    0, 0,
                                                    SafeNativeMethods.CERT_REGISTRY_STORE_REMOTE_FLAG |
                                                    SafeNativeMethods.CERT_STORE_READONLY_FLAG |
                                                    SafeNativeMethods.CERT_STORE_OPEN_EXISTING_FLAG,
                                                    finalKey);
                    if (storeHandle.IsInvalid)
                    {
                        throw new WsatAdminException(WsatAdminErrorCode.CERT_STORE_ACCESS, SR.GetString(SR.ErrorAccessCertStore, Marshal.GetLastWin32Error()));
                    }
                    return storeHandle;                    
                }
                finally
                {
                    if (remoteBase != null)
                    {
                        remoteBase.Close();
                    }
                    if (finalKey != null)
                    {
                        finalKey.Close();
                    }
                }
            }
#else
            else
            {
                throw new WsatAdminException(WsatAdminErrorCode.CERT_STORE_ACCESS, SR.GetString(SR.ErrorAccessCertStore, 0));
            }
#endif
        }

#if WSAT_CMDLINE
        // Input: "Issuer\SubjectName". Issuer and SubjectName can be wildcard character '*'
        // This one is not required to support remote machine operation so we could rely on the NDP certificate API
        [SecurityCritical]
        internal static X509Certificate2 GetCertificateFromIssuerAndSubjectName(string constraint)
        {
            if (string.IsNullOrEmpty(constraint) || constraint.Length < 3)
            {
                return null;
            }

            int separatorIndex = constraint.IndexOf('\\');
            if (separatorIndex <= 0)   // issuer can't be empty but can be '*'
            {
                return null;
            }

            string issuer, subjectName;
            issuer = constraint.Substring(0, separatorIndex);            
            subjectName = constraint.Substring(separatorIndex + 1, constraint.Length - separatorIndex - 1);
            if (Utilities.SafeCompare(subjectName, "{EMPTY}"))
            {
                subjectName = string.Empty;
            }

            X509Store store = null;
            try
            {
                store = new X509Store(StoreName.My, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadOnly);
                X509Certificate2Collection certs = store.Certificates;
                if (certs == null || certs.Count == 0)
                {
                    return null;
                }

                if (!Utilities.SafeCompare(issuer, "*"))
                {
                    certs = certs.Find(X509FindType.FindByIssuerDistinguishedName, issuer, false);
                }
                if (certs.Count > 0 && !Utilities.SafeCompare(subjectName, "*"))
                {
                    certs = certs.Find(X509FindType.FindBySubjectDistinguishedName, subjectName, false);
                }

                if (certs.Count == 1)
                {
                    return certs[0];
                }
            }
            catch (ArgumentException)
            {
                // does nothing
            }
            catch (SecurityException)
            {
                // does nothing
            }
            catch (CryptographicException)
            {
                // does nothing
            }
            finally
            {
                if (store != null)
                {
                    store.Close();
                }
            }

            return null;
        }
#endif

        [SecurityCritical]
        internal static X509Certificate2 GetCertificateFromThumbprint(string thumbprint, string machineName)
        {
            if (String.IsNullOrEmpty(thumbprint))
            {
                return null;
            }

            X509Certificate2 cert = null;
            SafeCertificateStore storeHandle = CertificateManager.GetCertificateStorePointer(machineName);
            SafeCertificateContext prev = new SafeCertificateContext();
            SafeCertificateContext current = new SafeCertificateContext();

            bool foundThumbprint = false;
            do
            {
                // the CertFindCertificateInStore function frees the SafeHandleCertificateContext
                // referenced by non-null values of "prev"
#pragma warning suppress 56523
                current = SafeNativeMethods.CertFindCertificateInStore(
                    storeHandle,
                    SafeNativeMethods.X509_ASN_ENCODING,
                    0,
                    SafeNativeMethods.CERT_FIND_ANY,
                    IntPtr.Zero,
                    prev);

                prev = current;
                if (!current.IsInvalid)
                {
                    cert = current.GetNewX509Certificate();
                    if (Utilities.SafeCompare(cert.Thumbprint, thumbprint))
                    {
                        foundThumbprint = true;
                    }
                }
            } while (!current.IsInvalid && !foundThumbprint);

            storeHandle.Close();
            prev.Close();

            if (!current.IsInvalid)
            {
                current.Close();
                return cert;
            }
            else
            {
                return null;
            }
        }

#if WSAT_CMDLINE
        internal static X509Certificate2 GetMachineIdentityCertificate()
        {
            X509Store store = null;
            try
            {
                store = new X509Store(StoreName.My, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadOnly);
                string hostName = System.Net.Dns.GetHostEntry(string.Empty).HostName;
                X509Certificate2 result = null;
                int count = 0;

                // The best way to find the Issued-to-machine certificate is to walk through each one and compare the DnsName                
                foreach (X509Certificate2 cert in store.Certificates)
                {                    
                    if (Utilities.SafeCompare(cert.GetNameInfo(X509NameType.DnsName, false), hostName))
                    {
                        try
                        {
                            WsatConfiguration.ValidateIdentityCertificateThrow(cert, false);
                            if (++count > 1)
                            {
                                break;
                            }
                            result = cert;
                        }
                        catch (WsatAdminException)
                        {
                            // Explicitly ignore
                        }
                    }
                }

                // We only use the cert if we found one and only one
                return (count == 1) ? result : null;
            }
            catch (ArgumentException)
            {
                return null;
            }
            catch (SecurityException)
            {
                return null;
            }
            catch (CryptographicException)
            {
                return null;
            }
            finally
            {
                if (store != null)
                {
                    store.Close();
                }
            }
        }
#endif
    }
}
