// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// Pkcs9Attribute.cs
//

namespace System.Security.Cryptography.Pkcs {
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class Pkcs9AttributeObject : AsnEncodedData {
        //
        // Constructors.
        //

        internal Pkcs9AttributeObject (Oid oid) {
            base.Oid = oid;
        }

        public Pkcs9AttributeObject () : base () {}

        public Pkcs9AttributeObject (string oid, byte[] encodedData) : this(new AsnEncodedData(oid, encodedData)) {}

        public Pkcs9AttributeObject (Oid oid, byte[] encodedData) : this(new AsnEncodedData(oid, encodedData)) {}

        public Pkcs9AttributeObject (AsnEncodedData asnEncodedData) : base (asnEncodedData) {
            if (asnEncodedData.Oid == null)
                throw new ArgumentNullException("asnEncodedData.Oid");
            string szOid = base.Oid.Value;
            if (szOid == null)
                throw new ArgumentNullException("oid.Value");
            if (szOid.Length == 0)
                throw new ArgumentException(SecurityResources.GetResourceString("Arg_EmptyOrNullString"), "oid.Value");
        }

        //
        // Public properties.
        //

        public new Oid Oid {
            get {
                return base.Oid;
            }
        }

        public override void CopyFrom (AsnEncodedData asnEncodedData) {
            if (asnEncodedData == null)
                throw new ArgumentNullException("asnEncodedData");
            Pkcs9AttributeObject att = asnEncodedData as Pkcs9AttributeObject;
            if (att == null)
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Pkcs9_AttributeMismatch"));
            base.CopyFrom(asnEncodedData);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class Pkcs9SigningTime : Pkcs9AttributeObject {
        private DateTime m_signingTime;
        private bool m_decoded = false;

        //
        // Constructors.
        //

        public Pkcs9SigningTime() : this(DateTime.Now) {}

        public Pkcs9SigningTime(DateTime signingTime) : base(CAPI.szOID_RSA_signingTime, Encode(signingTime)) {
            m_signingTime = signingTime;
            m_decoded = true;
        }

        public Pkcs9SigningTime(byte[] encodedSigningTime) : base(CAPI.szOID_RSA_signingTime, encodedSigningTime) {}

        //
        // Public properties.
        //

        public DateTime SigningTime {
            get {
                if (!m_decoded && (null != RawData))
                    Decode();
                return m_signingTime;
            }
        }

        public override void CopyFrom (AsnEncodedData asnEncodedData) {
            base.CopyFrom(asnEncodedData);
            m_decoded = false;
        }

        //
        // Private methods.
        //

        [System.Security.SecuritySafeCritical]
        private void Decode() {
            uint cbDecoded = 0;
            SafeLocalAllocHandle pbDecoded = null;

            if (!CAPI.DecodeObject(new IntPtr(CAPI.PKCS_UTC_TIME),
                                   RawData,
                                   out pbDecoded,
                                   out cbDecoded)) {
                throw new CryptographicException(Marshal.GetLastWin32Error());
            }

            long signingTime = Marshal.ReadInt64(pbDecoded.DangerousGetHandle());
            pbDecoded.Dispose();

            m_signingTime = DateTime.FromFileTimeUtc(signingTime);
            m_decoded = true;
        }

        [System.Security.SecuritySafeCritical]
        private static byte[] Encode (DateTime signingTime) {
            long ft = signingTime.ToFileTimeUtc();
            SafeLocalAllocHandle pbSigningTime = CAPI.LocalAlloc(CAPI.LPTR, new IntPtr(Marshal.SizeOf(typeof(Int64))));
            Marshal.WriteInt64(pbSigningTime.DangerousGetHandle(), ft);

            byte[] encodedSigningTime = new byte[0];
            if (!CAPI.EncodeObject(CAPI.szOID_RSA_signingTime, pbSigningTime.DangerousGetHandle(), out encodedSigningTime))
                throw new CryptographicException(Marshal.GetLastWin32Error());

            pbSigningTime.Dispose();

            return encodedSigningTime;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class Pkcs9DocumentName : Pkcs9AttributeObject {
        private string m_documentName = null;
        private bool m_decoded = false;

        //
        // Constructors.
        //

        public Pkcs9DocumentName () :
            base(new Oid(CAPI.szOID_CAPICOM_documentName)) {
            // CAPI doesn't have an OID mapping for szOID_CAPICOM_documentName, so we cannot use the faster
            // FromOidValue factory
        }

        public Pkcs9DocumentName (string documentName) : 
            base(CAPI.szOID_CAPICOM_documentName, Encode(documentName)) {
            m_documentName = documentName;
            m_decoded = true;
        }

        public Pkcs9DocumentName (byte[] encodedDocumentName) : 
            base(CAPI.szOID_CAPICOM_documentName, encodedDocumentName) {}

        //
        // Public methods.
        //

        public string DocumentName {
            get {
                if (!m_decoded && (null != RawData))
                    Decode();
                return m_documentName;
            }
        }

        public override void CopyFrom (AsnEncodedData asnEncodedData) {
            base.CopyFrom(asnEncodedData);
            m_decoded = false;
        }

        //
        // Private methods.
        //

        private void Decode() {
            m_documentName = PkcsUtils.DecodeOctetString(RawData);
            m_decoded = true;
        }

        private static byte[] Encode (string documentName) {
            if (String.IsNullOrEmpty(documentName))
                throw new ArgumentNullException("documentName");
            return PkcsUtils.EncodeOctetString(documentName);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class Pkcs9DocumentDescription : Pkcs9AttributeObject {
        private string m_documentDescription = null;
        private bool m_decoded = false;

        //
        // Constructors.
        //

        public Pkcs9DocumentDescription () :
            base (new Oid(CAPI.szOID_CAPICOM_documentDescription)) {
            // CAPI doesn't have an OID mapping for szOID_CAPICOM_documentDescription, so we cannot use the faster
            // FromOidValue factory
        }

        public Pkcs9DocumentDescription(string documentDescription) : 
            base(CAPI.szOID_CAPICOM_documentDescription, Encode(documentDescription)) {
            m_documentDescription = documentDescription;
            m_decoded = true;
        }

        public Pkcs9DocumentDescription(byte[] encodedDocumentDescription) : 
            base(CAPI.szOID_CAPICOM_documentDescription, encodedDocumentDescription) {}

        //
        // Public methods.
        //

        public string DocumentDescription {
            get {
                if (!m_decoded && (null != RawData))
                    Decode();
                return m_documentDescription;
            }
        }

        public override void CopyFrom (AsnEncodedData asnEncodedData) {
            base.CopyFrom(asnEncodedData);
            m_decoded = false;
        }

        //
        // Private methods.
        //

        private void Decode () {
            m_documentDescription = PkcsUtils.DecodeOctetString(RawData);
            m_decoded = true;
        }

        private static byte[] Encode (string documentDescription) {
            if (String.IsNullOrEmpty(documentDescription))
                throw new ArgumentNullException("documentDescription");
            return PkcsUtils.EncodeOctetString(documentDescription);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class Pkcs9ContentType : Pkcs9AttributeObject {
        private Oid m_contentType = null;
        private bool m_decoded = false;

        //
        // Constructors.
        //

        internal Pkcs9ContentType (byte[] encodedContentType) :
            base(Oid.FromOidValue(CAPI.szOID_RSA_contentType, OidGroup.ExtensionOrAttribute), encodedContentType) {
        }

        public Pkcs9ContentType() :
            base(Oid.FromOidValue(CAPI.szOID_RSA_contentType, OidGroup.ExtensionOrAttribute)) {
        }

        //
        // Public properties.
        //

        public Oid ContentType {
            get {
                if (!m_decoded && (null != RawData))
                    Decode();
                return m_contentType;
            }
        }

        public override void CopyFrom (AsnEncodedData asnEncodedData) {
            base.CopyFrom(asnEncodedData);
            m_decoded = false;
        }

        //
        // Private methods.
        //

        private void Decode () {
            if ((RawData.Length < 2) || ((uint) RawData[1] != (uint) (RawData.Length - 2)))
                throw new CryptographicException(CAPI.CRYPT_E_BAD_ENCODE);

            if (RawData[0] != CAPI.ASN_TAG_OBJID)
                throw new CryptographicException(CAPI.CRYPT_E_ASN1_BADTAG);

            m_contentType = new Oid(PkcsUtils.DecodeObjectIdentifier(RawData, 2));
            m_decoded = true;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class Pkcs9MessageDigest : Pkcs9AttributeObject {
        private byte[] m_messageDigest = null;
        private bool m_decoded = false;

        //
        // Constructors.
        //

        internal Pkcs9MessageDigest (byte[] encodedMessageDigest) :
            base(Oid.FromOidValue(CAPI.szOID_RSA_messageDigest, OidGroup.ExtensionOrAttribute), encodedMessageDigest) {
        }
        
        public Pkcs9MessageDigest () :
            base(Oid.FromOidValue(CAPI.szOID_RSA_messageDigest, OidGroup.ExtensionOrAttribute)) {
        }

        //
        // Public properties.
        //

        public byte[] MessageDigest {
            get {
                if (!m_decoded && (null != RawData))
                    Decode();
                return m_messageDigest;
            }
        }

        public override void CopyFrom (AsnEncodedData asnEncodedData) {
            base.CopyFrom(asnEncodedData);
            m_decoded = false;
        }

        //
        // Private methods.
        //

        private void Decode () {
            m_messageDigest = PkcsUtils.DecodeOctetBytes(RawData);
            m_decoded = true;
        }
    }
}
