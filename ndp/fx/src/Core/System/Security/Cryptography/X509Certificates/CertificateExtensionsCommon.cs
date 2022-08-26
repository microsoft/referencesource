// Copyright (c) Microsoft Corporation.  All rights reserved.

using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace System.Security.Cryptography.X509Certificates
{
    internal static class CertificateExtensionsCommon
    {
        [SecurityCritical]
        internal static X509Certificate2 CopyWithPersistedCngKey(X509Certificate2 publicCert, CngKey cngKey)
        {
            if (string.IsNullOrEmpty(cngKey.KeyName))
            {
                return null;
            }

            X509Certificate2 publicPrivate = new X509Certificate2(publicCert.RawData);
            CngProvider provider = cngKey.Provider;
            string keyName = cngKey.KeyName;
            bool machineKey = cngKey.IsMachineKey;

            // CAPI RSA_SIGN keys won't open correctly under CNG without the key number being specified, so
            // check to see if we can figure out what key number it needs to re-open.
            int keySpec = GuessKeySpec(provider, keyName, machineKey, cngKey.AlgorithmGroup);

            var keyProvInfo = new X509Native.CRYPT_KEY_PROV_INFO();
            keyProvInfo.pwszContainerName = cngKey.KeyName;
            keyProvInfo.pwszProvName = cngKey.Provider.Provider;
            keyProvInfo.dwFlags = machineKey ? X509Native.CRYPT_MACHINE_KEYSET : 0;
            keyProvInfo.dwKeySpec = keySpec;

            using (SafeCertContextHandle certificateContext = X509Native.GetCertificateContext(publicPrivate))
            {
                if (!X509Native.SetCertificateKeyProvInfo(certificateContext, ref keyProvInfo))
                {
                    int hr = Marshal.GetLastWin32Error();
                    publicPrivate.Dispose();
                    throw new CryptographicException(hr);
                }
            }

            return publicPrivate;
        }

        [SecurityCritical]
        internal static X509Certificate2 CopyWithPersistedCapiKey(X509Certificate2 publicCert, CspKeyContainerInfo keyContainerInfo)
        {
            if (string.IsNullOrEmpty(keyContainerInfo.KeyContainerName))
            {
                return null;
            }

            X509Certificate2 publicPrivate = new X509Certificate2(publicCert.RawData);

            var keyProvInfo = new X509Native.CRYPT_KEY_PROV_INFO();
            keyProvInfo.pwszContainerName = keyContainerInfo.KeyContainerName;
            keyProvInfo.pwszProvName = keyContainerInfo.ProviderName;
            keyProvInfo.dwProvType = keyContainerInfo.ProviderType;
            keyProvInfo.dwKeySpec = (int)keyContainerInfo.KeyNumber;
            keyProvInfo.dwFlags = keyContainerInfo.MachineKeyStore ? X509Native.CRYPT_MACHINE_KEYSET : 0;

            using (SafeCertContextHandle certificateContext = X509Native.GetCertificateContext(publicPrivate))
            {
                if (!X509Native.SetCertificateKeyProvInfo(certificateContext, ref keyProvInfo))
                {
                    int hr = Marshal.GetLastWin32Error();
                    publicPrivate.Dispose();
                    throw new CryptographicException(hr);
                }
            }

            return publicPrivate;
        }

        [SecurityCritical]
        internal static X509Certificate2 CopyWithEphemeralCngKey(X509Certificate2 publicCert, CngKey cngKey)
        {
            Debug.Assert(string.IsNullOrEmpty(cngKey.KeyName));

            X509Certificate2 publicPrivate = new X509Certificate2(publicCert.RawData);
            SafeNCryptKeyHandle handle = cngKey.Handle;

            using (SafeCertContextHandle certificateContext = X509Native.GetCertificateContext(publicPrivate))
            {
                if (!X509Native.SetCertificateNCryptKeyHandle(certificateContext, handle))
                {
                    int hr = Marshal.GetLastWin32Error();
                    publicPrivate.Dispose();
                    throw new CryptographicException(hr);
                }
            }

            // The value was transferred to the certificate.
            handle.SetHandleAsInvalid();
            return publicPrivate;
        }

        private static int GuessKeySpec(
            CngProvider provider,
            string keyName,
            bool machineKey,
            CngAlgorithmGroup algorithmGroup)
        {
            if (provider == CngProvider.MicrosoftSoftwareKeyStorageProvider ||
                provider == CngProvider.MicrosoftSmartCardKeyStorageProvider)
            {
                // Well-known CNG providers, keySpec is 0.
                return 0;
            }

            try
            {
                CngKeyOpenOptions options = machineKey ? CngKeyOpenOptions.MachineKey : CngKeyOpenOptions.None;

                using (CngKey.Open(keyName, provider, options))
                {
                    // It opened with keySpec 0, so use keySpec 0.
                    return 0;
                }
            }
            catch (CryptographicException)
            {
                // While NTE_BAD_KEYSET is what we generally expect here for RSA, on Windows 7
                // PROV_DSS produces NTE_BAD_PROV_TYPE, and PROV_DSS_DH produces NTE_NO_KEY.
                //
                // So we'll just try the CAPI fallback for any error code, and see what happens.

                CspParameters cspParameters = new CspParameters
                {
                    ProviderName = provider.Provider,
                    KeyContainerName = keyName,
                    Flags = CspProviderFlags.UseExistingKey,
                    KeyNumber = (int)KeyNumber.Signature,
                };

                if (machineKey)
                {
                    cspParameters.Flags |= CspProviderFlags.UseMachineKeyStore;
                }

                int keySpec;

                if (TryGuessKeySpec(cspParameters, algorithmGroup, out keySpec))
                {
                    return keySpec;
                }

                throw;
            }
        }

        private static bool TryGuessKeySpec(
            CspParameters cspParameters,
            CngAlgorithmGroup algorithmGroup,
            out int keySpec)
        {
            if (algorithmGroup == CngAlgorithmGroup.Rsa)
            {
                return TryGuessRsaKeySpec(cspParameters, out keySpec);
            }

            if (algorithmGroup == CngAlgorithmGroup.Dsa)
            {
                return TryGuessDsaKeySpec(cspParameters, out keySpec);
            }

            keySpec = 0;
            return false;
        }

        private static bool TryGuessRsaKeySpec(CspParameters cspParameters, out int keySpec)
        {
            // Try the AT_SIGNATURE spot in each of the 4 RSA provider type values,
            // ideally one of them will work.
            const int PROV_RSA_FULL = 1;
            const int PROV_RSA_SIG = 2;
            const int PROV_RSA_SCHANNEL = 12;
            const int PROV_RSA_AES = 24;

            // These are ordered in terms of perceived likeliness, given that the key
            // is AT_SIGNATURE.
            int[] provTypes =
            {
                PROV_RSA_FULL,
                PROV_RSA_AES,
                PROV_RSA_SCHANNEL,

                // Nothing should be PROV_RSA_SIG, but if everything else has failed,
                // just try this last thing.
                PROV_RSA_SIG,
            };

            foreach (int provType in provTypes)
            {
                cspParameters.ProviderType = provType;

                try
                {
                    using (new RSACryptoServiceProvider(cspParameters))
                    {
                        {
                            keySpec = cspParameters.KeyNumber;
                            return true;
                        }
                    }
                }
                catch (CryptographicException)
                {
                }
            }

            Debug.Fail("RSA key did not open with KeyNumber 0 or AT_SIGNATURE");
            keySpec = 0;
            return false;
        }

        private static bool TryGuessDsaKeySpec(CspParameters cspParameters, out int keySpec)
        {
            const int PROV_DSS = 3;
            const int PROV_DSS_DH = 13;

            int[] provTypes =
            {
                PROV_DSS_DH,
                PROV_DSS,
            };

            foreach (int provType in provTypes)
            {
                cspParameters.ProviderType = provType;

                try
                {
                    using (new DSACryptoServiceProvider(cspParameters))
                    {
                        {
                            keySpec = cspParameters.KeyNumber;
                            return true;
                        }
                    }
                }
                catch (CryptographicException)
                {
                }
            }

            Debug.Fail("DSA key did not open with KeyNumber 0 or AT_SIGNATURE");
            keySpec = 0;
            return false;
        }
    }
}
