// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

using System;
using System.Diagnostics;
using System.Reflection;
using System.Threading;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

namespace System.Security.Cryptography.X509Certificates
{
    internal static class X509CertificateExtensions
    {
        public static RSA GetRSAPublicKey(this X509Certificate2 certificate)
        {
            return CngLightup.GetRSAPublicKey(certificate);
        }

        public static RSA GetRSAPrivateKey(this X509Certificate2 certificate)
        {
            return CngLightup.GetRSAPrivateKey(certificate);
        }

        public static DSA GetDSAPublicKey(this X509Certificate2 certificate)
        {
            return CngLightup.GetDSAPublicKey(certificate);
        }

        public static DSA GetDSAPrivateKey(this X509Certificate2 certificate)
        {
            return CngLightup.GetDSAPrivateKey(certificate);
        }

        // The ECDsa type lives in System.Core so we can't reference it directly. Using AsymmetricAlgorithm instead.
        public static AsymmetricAlgorithm GetECDsaPublicKey(this X509Certificate2 certificate)
        {
            return s_getEcdsaPublicKey.Value(certificate);
        }

        // The ECDsa type lives in System.Core so we can't reference it directly. Using AsymmetricAlgorithm instead.
        public static AsymmetricAlgorithm GetECDsaPrivateKey(this X509Certificate2 certificate)
        {
            return s_getEcdsaPrivateKey.Value(certificate);
        }

        public static AsymmetricAlgorithm GetAnyPublicKey(this X509Certificate2 c)
        {
            AsymmetricAlgorithm a;

            a = c.GetRSAPublicKey();
            if (a != null)
                return a;

            a = c.GetDSAPublicKey();
            if (a != null)
                return a;

            a = c.GetECDsaPublicKey();
            if (a != null)
                return a;

            throw new NotSupportedException(SecurityResources.GetResourceString("NotSupported_KeyAlgorithm"));
        }

        private static Lazy<Func<X509Certificate2, AsymmetricAlgorithm>> s_getEcdsaPublicKey = CreateLazyInvoker<AsymmetricAlgorithm>("ECDsa", isPublic: true);
        private static Lazy<Func<X509Certificate2, AsymmetricAlgorithm>> s_getEcdsaPrivateKey = CreateLazyInvoker<AsymmetricAlgorithm>("ECDsa", isPublic: false);

        private static Lazy<Func<X509Certificate2, T>> CreateLazyInvoker<T>(string algorithmName, bool isPublic) where T : AsymmetricAlgorithm
        {
            Func<Func<X509Certificate2, T>> factory =
                delegate ()
                {
                    // Load System.Core.dll and load the appropriate extension class 
                    // (one of
                    //    System.Security.Cryptography.X509Certificates.RSACertificateExtensions
                    //    System.Security.Cryptography.X509Certificates.DSACertificateExtensions
                    //    System.Security.Cryptography.X509Certificates.ECDsaCertificateExtensions
                    // )
                    string assemblyQualifiedTypeName = "System.Security.Cryptography.X509Certificates." + algorithmName + "CertificateExtensions" + ", " + AssemblyRef.SystemCore;
                    Type type = Type.GetType(assemblyQualifiedTypeName, throwOnError: false, ignoreCase: false);

                    // In the event that this DLL gets patched to 4.5.x-4.6.0 the ECDsaCertificateExtensions type
                    // will not be found. There's no fall-back for ECDSA, so return null.
                    //
                    // Other algorithms are taken care of by the CngLightup helper.
                    if (type == null)
                    {
                        return (x509) => null;
                    }

                    // Now, find the api we want to call:
                    //   
                    // (one of
                    //     GetRSAPublicKey(this X509Certificate2 c)
                    //     GetRSAPrivateKey(this X509Certificate2 c)
                    //     GetDSAPublicKey(this X509Certificate2 c)
                    //     GetDSAPrivateKey(this X509Certificate2 c)
                    //     GetECDsaPublicKey(this X509Certificate2 c)
                    //     GetECDsaPrivateKey(this X509Certificate2 c)
                    // )
                    string methodName = "Get" + algorithmName + (isPublic ? "Public" : "Private") + "Key";
                    MethodInfo api = type.GetMethod(methodName, BindingFlags.Public | BindingFlags.Static, null, new Type[] { typeof(X509Certificate2) }, null);
                    Debug.Assert(api != null, "Method '" + methodName + "(X509Certificate2 c)' not found on type '" + type + "'");

                    Func<X509Certificate2, T> apiInvoker = (Func<X509Certificate2, T>)api.CreateDelegate(typeof(Func<X509Certificate2, T>));
                    return apiInvoker;
                };

            return new Lazy<Func<X509Certificate2, T>>(factory, LazyThreadSafetyMode.PublicationOnly);
        }
    }
}

