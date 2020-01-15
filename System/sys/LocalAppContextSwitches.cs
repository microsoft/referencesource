// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Runtime.CompilerServices;

namespace System
{
    internal static class LocalAppContextSwitches
    {

        #region System quirks
        private static int _memberDescriptorEqualsReturnsFalseIfEquivalent;
        internal const string MemberDescriptorEqualsReturnsFalseIfEquivalentName = @"Switch.System.MemberDescriptorEqualsReturnsFalseIfEquivalent";

        public static bool MemberDescriptorEqualsReturnsFalseIfEquivalent
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(MemberDescriptorEqualsReturnsFalseIfEquivalentName, ref _memberDescriptorEqualsReturnsFalseIfEquivalent);
            }
        }

        private static int _dontEnableStrictRFC3986ReservedCharacterSets;
        internal const string DontEnableStrictRFC3986ReservedCharacterSetsName = @"Switch.System.Uri.DontEnableStrictRFC3986ReservedCharacterSets";

        public static bool DontEnableStrictRFC3986ReservedCharacterSets
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontEnableStrictRFC3986ReservedCharacterSetsName, ref _dontEnableStrictRFC3986ReservedCharacterSets);
            }
        }

        private static int _dontKeepUnicodeBidiFormattingCharacters;
        internal const string DontKeepUnicodeBidiFormattingCharactersName = @"Switch.System.Uri.DontKeepUnicodeBidiFormattingCharacters";

        public static bool DontKeepUnicodeBidiFormattingCharacters
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontKeepUnicodeBidiFormattingCharactersName, ref _dontKeepUnicodeBidiFormattingCharacters);
            }
        }

        private static int _disableTempFileCollectionDirectoryFeature;
        internal const string DisableTempFileCollectionDirectoryFeatureName = @"Switch.System.DisableTempFileCollectionDirectoryFeature";

        public static bool DisableTempFileCollectionDirectoryFeature
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DisableTempFileCollectionDirectoryFeatureName, ref _disableTempFileCollectionDirectoryFeature);
            }
        }
        #endregion

        private static int _disableEventLogRegistryKeysFiltering;
        private const string DisableEventLogRegistryKeysFilteringName = @"Switch.System.Diagnostics.EventLog.DisableEventLogRegistryKeysFiltering";

        public static bool DisableEventLogRegistryKeysFiltering
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DisableEventLogRegistryKeysFilteringName, ref _disableEventLogRegistryKeysFiltering);
            }
        }

        #region System.Net quirks
        private static int _dontEnableSchUseStrongCrypto;
        internal const string DontEnableSchUseStrongCryptoName = @"Switch.System.Net.DontEnableSchUseStrongCrypto";

        public static bool DontEnableSchUseStrongCrypto
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontEnableSchUseStrongCryptoName, ref _dontEnableSchUseStrongCrypto);
            }
        }

        private static int _allocateOverlappedOnDemand;
        internal const string AllocateOverlappedOnDemandName = @"Switch.System.Net.WebSockets.HttpListenerAsyncEventArgs.AllocateOverlappedOnDemand";

        public static bool AllocateOverlappedOnDemand
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(AllocateOverlappedOnDemandName, ref _allocateOverlappedOnDemand);
            }
        }

        private static int _dontEnableSchSendAuxRecord;
        internal const string DontEnableSchSendAuxRecordName = @"Switch.System.Net.DontEnableSchSendAuxRecord";

        public static bool DontEnableSchSendAuxRecord
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontEnableSchSendAuxRecordName, ref _dontEnableSchSendAuxRecord);
            }
        }
        
        private static int _dontEnableSystemSystemDefaultTlsVersions;
        internal const string DontEnableSystemDefaultTlsVersionsName = @"Switch.System.Net.DontEnableSystemDefaultTlsVersions";

        public static bool DontEnableSystemDefaultTlsVersions
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontEnableSystemDefaultTlsVersionsName, ref _dontEnableSystemSystemDefaultTlsVersions);
            }
        }

        private static int _dontEnableTlsAlerts;
        internal const string DontEnableTlsAlertsName = @"Switch.System.Net.DontEnableTlsAlerts";

        public static bool DontEnableTlsAlerts
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontEnableTlsAlertsName, ref _dontEnableTlsAlerts);
            }
        }

        private static int _dontCheckCertificateEKUs;
        internal const string DontCheckCertificateEKUsName = @"Switch.System.Net.DontCheckCertificateEKUs";

        public static bool DontCheckCertificateEKUs
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontCheckCertificateEKUsName, ref _dontCheckCertificateEKUs);
            }
        }

        private static int _dontCheckCertificateRevocation;
        internal const string DontCheckCertificateRevocationName = @"System.Net.Security.SslStream.AuthenticateAsClient.DontCheckCertificateRevocation";

        public static bool DontCheckCertificateRevocation
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DontCheckCertificateRevocationName, ref _dontCheckCertificateRevocation);
            }
        }
        #endregion

        private static int _doNotCatchSerialStreamThreadExceptions;
        internal const string DoNotCatchSerialStreamThreadExceptionsName = @"Switch.System.IO.Ports.DoNotCatchSerialStreamThreadExceptions";

        public static bool DoNotCatchSerialStreamThreadExceptions
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DoNotCatchSerialStreamThreadExceptionsName, ref _doNotCatchSerialStreamThreadExceptions);
            }
        }

        private static int _doNotValidateX509KeyStorageFlags;
        internal const string DoNotValidateX509KeyStorageFlagsName = @"Switch.System.Security.Cryptography.X509Cerificates.X509Certificate2Collection.DoNotValidateX509KeyStorageFlags";

        public static bool DoNotValidateX509KeyStorageFlags
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DoNotValidateX509KeyStorageFlagsName, ref _doNotValidateX509KeyStorageFlags);
            }
        }

        private static int _doNotUseNativeZipLibraryForDecompression;
        internal const string DoNotUseNativeZipLibraryForDecompressionName = @"Switch.System.IO.Compression.DoNotUseNativeZipLibraryForDecompression";

        public static bool DoNotUseNativeZipLibraryForDecompression
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(DoNotUseNativeZipLibraryForDecompressionName, ref _doNotUseNativeZipLibraryForDecompression);
            }
        }

        private static int _useLegacyTimeoutCheck;
        internal const string UseLegacyTimeoutCheckName = @"Switch.System.Text.RegularExpressions.UseLegacyTimeoutCheck";

        public static bool UseLegacyTimeoutCheck
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(UseLegacyTimeoutCheckName, ref _useLegacyTimeoutCheck);
            }
        }
    }
}
