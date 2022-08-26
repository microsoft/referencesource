// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// CryptographicAttributeObject.cs
//
// 07/10/2003
//

namespace System.Security.Cryptography {
    using System.Collections;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.Pkcs;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CryptographicAttributeObject {
        private Oid m_oid = null;
        private AsnEncodedDataCollection m_values = null;

        //
        // Constructors.
        //

        private CryptographicAttributeObject () {}

        [SecurityCritical]
        internal CryptographicAttributeObject (IntPtr pAttribute) : this((CAPI.CRYPT_ATTRIBUTE) Marshal.PtrToStructure(pAttribute, typeof(CAPI.CRYPT_ATTRIBUTE))) {}

        [SecurityCritical]
        internal CryptographicAttributeObject(CAPI.CRYPT_ATTRIBUTE cryptAttribute) :
            this(new Oid(cryptAttribute.pszObjId), PkcsUtils.GetAsnEncodedDataCollection(cryptAttribute)) {
        }

        [SecurityCritical]
        internal CryptographicAttributeObject (CAPI.CRYPT_ATTRIBUTE_TYPE_VALUE cryptAttribute) :
            this(new Oid(cryptAttribute.pszObjId), PkcsUtils.GetAsnEncodedDataCollection(cryptAttribute)) {
        }

        internal CryptographicAttributeObject (AsnEncodedData asnEncodedData) : this(asnEncodedData.Oid, new AsnEncodedDataCollection(asnEncodedData)) {}

        public CryptographicAttributeObject (Oid oid) : this(oid, new AsnEncodedDataCollection()) {}

        public CryptographicAttributeObject (Oid oid, AsnEncodedDataCollection values) {
            m_oid = new Oid(oid);
            if (values == null)
                m_values = new AsnEncodedDataCollection();
            else {
                foreach (AsnEncodedData asn in values) {
                    if (0 != String.Compare(asn.Oid.Value, oid.Value, StringComparison.Ordinal))
                        throw new InvalidOperationException(SecurityResources.GetResourceString("InvalidOperation_DuplicateItemNotAllowed"));
                }
                m_values = values;
            }
        }

        //
        // Public properties.
        //

        public Oid Oid {
            get {
                return new Oid(m_oid);
            }
        }

        public AsnEncodedDataCollection Values {
            get {
                return m_values;
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CryptographicAttributeObjectCollection : ICollection {
        private ArrayList m_list;

        public CryptographicAttributeObjectCollection() {
            m_list = new ArrayList();
        }

        [SecurityCritical]
        private CryptographicAttributeObjectCollection (IntPtr pCryptAttributes) : this((CAPI.CRYPT_ATTRIBUTES) Marshal.PtrToStructure(pCryptAttributes, typeof(CAPI.CRYPT_ATTRIBUTES))) {}

        [SecurityCritical]
        internal CryptographicAttributeObjectCollection (SafeLocalAllocHandle pCryptAttributes) : this(pCryptAttributes.DangerousGetHandle()) {}

        [SecurityCritical]
        internal CryptographicAttributeObjectCollection (CAPI.CRYPT_ATTRIBUTES cryptAttributes) {
            m_list = new ArrayList((int)cryptAttributes.cAttr);
            for (uint index = 0; index < cryptAttributes.cAttr; index++) {
                IntPtr pCryptAttribute = new IntPtr((long)cryptAttributes.rgAttr + (index * Marshal.SizeOf(typeof(CAPI.CRYPT_ATTRIBUTE))));
                m_list.Add(new CryptographicAttributeObject(pCryptAttribute));
            }
        }

        public CryptographicAttributeObjectCollection (CryptographicAttributeObject attribute) {
            m_list = new ArrayList();
            m_list.Add(attribute);
        }

        // Perform a deep copy of an existing collection
        private CryptographicAttributeObjectCollection(CryptographicAttributeObjectCollection other) {
            m_list = new ArrayList(other.m_list.Count);
            foreach (CryptographicAttributeObject attribute in other.m_list) {
                CryptographicAttributeObject attributeCopy = new CryptographicAttributeObject(attribute.Oid);
                foreach (AsnEncodedData encodedData in attribute.Values) {
                    attributeCopy.Values.Add(new AsnEncodedData(encodedData.Oid, encodedData.RawData));
                }
                m_list.Add(attributeCopy);
            }
        }

        public int Add (AsnEncodedData asnEncodedData) {
            if (asnEncodedData == null)
                throw new ArgumentNullException("asnEncodedData");

            return Add(new CryptographicAttributeObject(asnEncodedData));
        }

        public int Add (CryptographicAttributeObject attribute) {
            if (attribute == null)
                throw new ArgumentNullException("attribute");

            //
            // Merge with exisitng attribute, if already existed, else add as new.
            //

            string szOid1 = null;
            if (attribute.Oid != null)
                szOid1 = attribute.Oid.Value;

            for (int index = 0; index < m_list.Count; index++) {
                CryptographicAttributeObject existing = (CryptographicAttributeObject) m_list[index];

                // To prevent caller to add the existing item into the collection again
                // Otherwise the merge will be an infinite loop
                if ((Object) existing.Values == (Object) attribute.Values)
                    throw new InvalidOperationException(SecurityResources.GetResourceString("InvalidOperation_DuplicateItemNotAllowed"));

                // Merge either:
                // 1. both OIDs are null, or
                // 2. both not null and OIDs match.
                string szOid2 = null;
                if (existing.Oid != null)
                    szOid2 = existing.Oid.Value;

                if (szOid1 == null && szOid2 == null) {
                    foreach (AsnEncodedData asnEncodedData in attribute.Values) {
                        existing.Values.Add(asnEncodedData);
                    }
                    return index;
                }

                if ((szOid1 != null && szOid2 != null) && (String.Compare(szOid1, szOid2, StringComparison.OrdinalIgnoreCase) == 0)) {
                    //
                    // Only allow one signing time, per RFC.
                    //

                    if (String.Compare(szOid1, CAPI.szOID_RSA_signingTime, StringComparison.OrdinalIgnoreCase) == 0)
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Pkcs9_MultipleSigningTimeNotAllowed"));

                    foreach (AsnEncodedData asnEncodedData in attribute.Values) {
                        existing.Values.Add(asnEncodedData);
                    }
                    return index;
                }
            }

            return m_list.Add(attribute);
        }

        public void Remove (CryptographicAttributeObject attribute) {
            if (attribute == null)
                throw new ArgumentNullException("attribute");

            m_list.Remove(attribute);
        }

        public CryptographicAttributeObject this[int index] {
            get {
                return (CryptographicAttributeObject) m_list[index];
            }
        }

        public int Count {
            get {
                return m_list.Count;
            }
        }

        public CryptographicAttributeObjectEnumerator GetEnumerator() {
            return new CryptographicAttributeObjectEnumerator(this);
        }

        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return new CryptographicAttributeObjectEnumerator(this);
        }

        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            if (array == null)
                throw new ArgumentNullException("array");
            if (array.Rank != 1)
                throw new ArgumentException(SecurityResources.GetResourceString("Arg_RankMultiDimNotSupported"));
            if (index < 0 || index >= array.Length)
                throw new ArgumentOutOfRangeException("index", SecurityResources.GetResourceString("ArgumentOutOfRange_Index"));
            if (index + this.Count > array.Length)
                throw new ArgumentException(SecurityResources.GetResourceString("Argument_InvalidOffLen"));

            for (int i=0; i < this.Count; i++) {
                array.SetValue(this[i], index);
                index++;
            }
        }

        public void CopyTo(CryptographicAttributeObject[] array, int index) {
            ((ICollection)this).CopyTo(array, index);
        }

        public bool IsSynchronized {
            get {
                return false;
            }
        }

        public Object SyncRoot {
            get {
                return this;
            }
        }

        internal CryptographicAttributeObjectCollection DeepCopy() {
            return new CryptographicAttributeObjectCollection(this);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CryptographicAttributeObjectEnumerator : IEnumerator {
        private CryptographicAttributeObjectCollection m_attributes;
        private int m_current;

        private CryptographicAttributeObjectEnumerator() {}

        internal CryptographicAttributeObjectEnumerator(CryptographicAttributeObjectCollection attributes) {
            m_attributes = attributes;
            m_current = -1;
        }

        public CryptographicAttributeObject Current {
            get {
                return m_attributes[m_current];
            }
        }

        /// <internalonly/>
        Object IEnumerator.Current {
            get {
                return (Object) m_attributes[m_current];
            }
        }

        public bool MoveNext() {
            if (m_current == ((int) m_attributes.Count - 1))
                return false;
            m_current++;
            return true;
        }

        public void Reset() {
            m_current = -1;
        }
    }
}
