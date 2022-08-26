// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;

namespace System
{
    internal static class LocalAppContextSwitches 
    {
        //
        //  Switch.System.Security.Cryptography.X509Certificates.RSACertificateExtensions.DontReliablyClonePrivateKey
        //
        //    4.6 introduced the GetRSAPrivateKey() extension method on the X509Certificate2 type.
        //
        //    Unlike its predecessor (X509Certificate2.PrivateKey) this method is supposed to return a freshly allocated object
        //    each time so that the caller can safely dispose and use it without interfering with other callers of this method.
        //
        //    Unfortunately, 4.6 and 4.6.1 only honor this when returning a CNG key. If the certificate's private key was
        //    a CSP key, GetRSAPrivateKey() returns the same object each time, whose lifetime is that of the X509Certificate2 object.
        //
        //    4.6.2. fixed this method so it returns a freshly allocated object each time as intended.
        //
        //    Setting this switch to true reinstates the old broken behavior.
        // 
        internal const string DontReliablyClonePrivateKeyStr = @"Switch.System.Security.Cryptography.X509Certificates.RSACertificateExtensions.DontReliablyClonePrivateKey";
        private static int _dontReliablyClonePrivateKeyName;
        public static bool DontReliablyClonePrivateKey
        {
             [MethodImpl(MethodImplOptions.AggressiveInlining)]
             get
             {
                 return LocalAppContext.GetCachedSwitchValue(DontReliablyClonePrivateKeyStr, ref _dontReliablyClonePrivateKeyName);
             }
        }

        //
        //  Switch.System.Security.Cryptography.X509Certificates.ECDsaCertificateExtensions.UseLegacyPublicKeyBehavior
        //
        //    Preceding 4.8 GetECDsaPublicKey did not correctly handle brainpool curves
        //    Setting this switch to true reinstates the old broken behavior.
        // 
        internal const string UseLegacyPublicKeyBehaviorStr = @"Switch.System.Security.Cryptography.X509Certificates.ECDsaCertificateExtensions.UseLegacyPublicKeyBehavior";
        private static int _useLegacyPublicKeyBehavior;
        public static bool UseLegacyPublicKeyBehavior
        {
             [MethodImpl(MethodImplOptions.AggressiveInlining)]
             get
             {
                 return LocalAppContext.GetCachedSwitchValue(UseLegacyPublicKeyBehaviorStr, ref _useLegacyPublicKeyBehavior);
             }
        }

        //
        //  Switch.System.Security.Cryptography.AesCryptoServiceProvider.DontCorrectlyResetDecryptor
        //
        //    Affects the behavior of AesCryptoServiceProvider.CreateDecryptor().TransformBlock() after a TransformFinalBlock()
        //    has been called, when a chained encryption mode such as CBC is being used.
        //
        //    Prior to 4.6.2., AesCryptoServiceProvider's Decryptor transform failed to reinitialize the transform as advertised after a
        //    TransformFinalBlock() call. The result is that further calls to TransformBlock() would yield incorrect values as AesCryptoServiceProvider
        //    would treat it as continuation of the prior decryption rather than the start of a new one.
        //
        //    4.6.2 and onward has fixed this behavior so that TransformFinalBlock() reinitializes the transform as advertised.
        //
        //    Setting this switch to "true" reinstates the pre-4.6.2. bugged behavior.
        //
        internal const string AesCryptoServiceProviderDontCorrectlyResetDecryptorStr = @"Switch.System.Security.Cryptography.AesCryptoServiceProvider.DontCorrectlyResetDecryptor";
        private static int _aesCryptoServiceProviderDontCorrectlyResetDecryptorName;
        public static bool AesCryptoServiceProviderDontCorrectlyResetDecryptor
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(AesCryptoServiceProviderDontCorrectlyResetDecryptorStr, ref _aesCryptoServiceProviderDontCorrectlyResetDecryptorName);
            }
        }

        //
        //  Switch.System.Security.Cryptography.SymmetricCng.AlwaysUseNCrypt
        //
        //    The returned object from CreateEncryptor or CreateDecryptor of AesCng and TripleDESCng in prior framework
        //    versions always used NCryptEncrypt and NCryptDecrypt, but NCrypt* APIs have an IPC and indirection cost
        //    that makes them orders of magnitude slower than the BCrypt* equivalents (at least, for small data sizes).
        //
        //    Now when an ephemeral key is being used we will use BCrypt* functions, and when a persisted key is being
        //    used we will use the NCrypt* functions.
        //
        //    Setting this switch to "true" returns to always using the NCrypt* functions, with any performance and side
        //    effect consequences.
        //
        internal const string SymmetricCngAlwaysUseNCryptStr = @"Switch.System.Security.Cryptography.SymmetricCng.AlwaysUseNCrypt";
        private static int _symmetricCngAlwaysUseNCryptName;
        public static bool SymmetricCngAlwaysUseNCrypt
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(SymmetricCngAlwaysUseNCryptStr, ref _symmetricCngAlwaysUseNCryptName);
            }
        }

        //
        //  Switch.System.Security.Cryptography.UseLegacyFipsThrow
        //
        //    .NET Framework 2.0-4.7.2 threw in the constructors of managed implementations of algorithms as well as
        //    the native-wrapper implementations of algorithms which were not FIPS Approved in 2005 (e.g. MD5).
        //    
        //    Now the algorithms which are not FIPS Approved are unrestricted, and the managed implementations of
        //    algorithms provided by CAPI/CNG will defer to the native-wrapper implementation.
        //
        //    Setting this switch to "true" returns to throwing in the constructor.
        //
        internal static readonly string SwitchCryptographyUseLegacyFipsThrow = "Switch.System.Security.Cryptography.UseLegacyFipsThrow";
        private static int _useLegacyFipsThrow;
        public static bool UseLegacyFipsThrow
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(SwitchCryptographyUseLegacyFipsThrow, ref _useLegacyFipsThrow);
            }
        }
    }

    internal static partial class AppContextDefaultValues
    {
        static partial void PopulateDefaultValuesPartial(string platformIdentifier, string profile, int version)
        {
            // When defining a new switch  you should add it to the last known version.
            // For instance, if you are adding a switch in .NET 4.6 (the release after 4.5.2) you should defined your switch
            // like this:
            //    if (version <= 40502) ...
            // This ensures that all previous versions of that platform (up-to 4.5.2) will get the old behavior by default
            // NOTE: When adding a default value for a switch please make sure that the default value is added to ALL of the existing platforms!
            // NOTE: When adding a new if statement for the version please ensure that ALL previous switches are enabled (ie. don't use else if)
            switch (platformIdentifier)
            {
                case ".NETCore":
                case ".NETFramework":
                {
                    if (version <= 40601)
                    {
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.AesCryptoServiceProviderDontCorrectlyResetDecryptorStr, true);
                    }

                    if (version <= 40702)
                    {
                        LocalAppContext.DefineSwitchDefault(LocalAppContextSwitches.SwitchCryptographyUseLegacyFipsThrow, true);
                    }
                    break;
                }
            }
        }
    }
}

