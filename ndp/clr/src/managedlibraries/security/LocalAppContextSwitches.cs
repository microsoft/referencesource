// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System
{
    using System;
    using System.Runtime.CompilerServices;

    internal static class LocalAppContextSwitches
    {
        private static int _xmlUseInsecureHashAlgorithms;
        internal static readonly string SwitchXmlUseInsecureHashAlgorithms = "Switch.System.Security.Cryptography.Xml.UseInsecureHashAlgorithms";
        public static bool XmlUseInsecureHashAlgorithms
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(SwitchXmlUseInsecureHashAlgorithms, ref _xmlUseInsecureHashAlgorithms);
            }
        }

        private static int _signedXmlUseLegacyCertificatePrivateKey;
        internal static readonly string SwitchSignedXmlUseLegacyCertificatePrivateKey = "Switch.System.Security.Cryptography.Xml.SignedXmlUseLegacyCertificatePrivateKey";
        public static bool SignedXmlUseLegacyCertificatePrivateKey
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(SwitchSignedXmlUseLegacyCertificatePrivateKey, ref _signedXmlUseLegacyCertificatePrivateKey);
            }
        }

        private static int _cmsUseInsecureHashAlgorithms;
        internal static readonly string SwitchCmsUseInsecureHashAlgorithms = "Switch.System.Security.Cryptography.Pkcs.UseInsecureHashAlgorithms";
        public static bool CmsUseInsecureHashAlgorithms
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(SwitchCmsUseInsecureHashAlgorithms, ref _cmsUseInsecureHashAlgorithms);
            }
        }

        private static int _envelopedCmsUseLegacyDefaultAlgorithm;
        internal static readonly string SwitchEnvelopedCmsUseLegacyDefaultAlgorithm = "Switch.System.Security.Cryptography.Pkcs.EnvelopedCmsUseLegacyDefaultAlgorithm";
        public static bool EnvelopedCmsUseLegacyDefaultAlgorithm
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return LocalAppContext.GetCachedSwitchValue(SwitchEnvelopedCmsUseLegacyDefaultAlgorithm, ref _envelopedCmsUseLegacyDefaultAlgorithm);
            }
        }
    }
}
