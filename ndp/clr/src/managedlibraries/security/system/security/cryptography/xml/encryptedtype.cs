// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// EncryptedType.cs
//
// This object implements the abstract type from which EncryptedData and EncrytpedKey derive.
// 
// 04/01/2002
//

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public abstract class EncryptedType {
        private string m_id;
        private string m_type;
        private string m_mimeType;
        private string m_encoding;
        private EncryptionMethod m_encryptionMethod;
        private CipherData m_cipherData;
        private EncryptionPropertyCollection m_props;
        private KeyInfo m_keyInfo;
        internal XmlElement m_cachedXml = null;

        internal bool CacheValid {
            get {
                return (m_cachedXml != null);
            }
        }

        public virtual string Id {
            get { return m_id; }
            set {
                m_id = value; 
                m_cachedXml = null;
            }
        }

        public virtual string Type {
            get { return m_type; }
            set { 
                m_type = value; 
                m_cachedXml = null;
            }
        }

        public virtual string MimeType {
            get { return m_mimeType; }
            set {
                m_mimeType = value; 
                m_cachedXml = null;
            }
        }

        public virtual string Encoding {
            get { return m_encoding; }
            set { 
                m_encoding = value; 
                m_cachedXml = null;
            }
        }

        public KeyInfo KeyInfo {
            get { 
                if (m_keyInfo == null)
                    m_keyInfo = new KeyInfo();
                return m_keyInfo; 
            }
            set { m_keyInfo = value; }
        }

        public virtual EncryptionMethod EncryptionMethod {
            get { return m_encryptionMethod; }
            set {
                m_encryptionMethod = value; 
                m_cachedXml = null;
            }
        }

        public virtual EncryptionPropertyCollection EncryptionProperties {
            get { 
                if (m_props == null)
                    m_props = new EncryptionPropertyCollection();
                return m_props; 
            }
        }

        public void AddProperty(EncryptionProperty ep) {
            this.EncryptionProperties.Add(ep);
        }

        public virtual CipherData CipherData {
            get {
                if (m_cipherData == null)
                    m_cipherData = new CipherData();

                return m_cipherData;
            }
            set { 
                if (value == null) 
                    throw new ArgumentNullException("value");

                m_cipherData = value;
                m_cachedXml = null;
            }
        }

        public abstract void LoadXml (XmlElement value);
        public abstract XmlElement GetXml();
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class EncryptionMethod {
        private XmlElement m_cachedXml = null;
        private int m_keySize = 0;
        private string m_algorithm;

        public EncryptionMethod () {
            m_cachedXml = null;
        }

        public EncryptionMethod (string algorithm) {
            m_algorithm = algorithm;
            m_cachedXml = null;
        }

        private bool CacheValid {
            get {
                return (m_cachedXml != null);
            }
        }

        public int KeySize {
            get { return m_keySize; }
            set {
                if (value <= 0) 
                    throw new ArgumentOutOfRangeException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidKeySize"));
                m_keySize = value;
                m_cachedXml = null;
            }
        }

        public string KeyAlgorithm {
            get { return m_algorithm; }
            set { 
                m_algorithm = value;
                m_cachedXml = null;
            }
        }

        public XmlElement GetXml() {
            if (CacheValid) return(m_cachedXml);

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the EncryptionMethod element
            XmlElement encryptionMethodElement = (XmlElement) document.CreateElement("EncryptionMethod", EncryptedXml.XmlEncNamespaceUrl);
            if (!String.IsNullOrEmpty(m_algorithm))
                encryptionMethodElement.SetAttribute("Algorithm", m_algorithm);
            if (m_keySize > 0) {
                // Construct a KeySize element
                XmlElement keySizeElement = document.CreateElement("KeySize", EncryptedXml.XmlEncNamespaceUrl);
                keySizeElement.AppendChild(document.CreateTextNode(m_keySize.ToString(null, null)));
                encryptionMethodElement.AppendChild(keySizeElement);
            }
            return encryptionMethodElement;
        }

        public void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);

            XmlElement encryptionMethodElement = value;
            m_algorithm = Utils.GetAttribute(encryptionMethodElement, "Algorithm", EncryptedXml.XmlEncNamespaceUrl);

            XmlNode keySizeNode = value.SelectSingleNode("enc:KeySize", nsm);
            if (keySizeNode != null) {
                KeySize = Convert.ToInt32(Utils.DiscardWhiteSpaces(keySizeNode.InnerText), null);
            }

            // Save away the cached value
            m_cachedXml = value;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class EncryptionProperty {
        private string m_target;
        private string m_id;
        private XmlElement m_elemProp; 
        private XmlElement m_cachedXml = null;

        // We are being lax here as per the spec
        public EncryptionProperty() {}

        public EncryptionProperty(XmlElement elementProperty) {
            if (elementProperty == null)
                throw new ArgumentNullException("elementProperty");
            if (elementProperty.LocalName != "EncryptionProperty" || elementProperty.NamespaceURI != EncryptedXml.XmlEncNamespaceUrl) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidEncryptionProperty"));

            m_elemProp = elementProperty;
            m_cachedXml = null;
        }

        public string Id {
            get { return m_id; }
        }

        public string Target {
            get { return m_target; }
        }

        public XmlElement PropertyElement {
            get { return m_elemProp; }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                if (value.LocalName != "EncryptionProperty" || value.NamespaceURI != EncryptedXml.XmlEncNamespaceUrl) 
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidEncryptionProperty"));

                m_elemProp = value;
                m_cachedXml = null;
            }
        }

        private bool CacheValid {
            get {
                return (m_cachedXml != null);
            }
        }

        public XmlElement GetXml() {
            if (CacheValid) return m_cachedXml;

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            return document.ImportNode(m_elemProp, true) as XmlElement;
        }

        public void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");    
            if (value.LocalName != "EncryptionProperty" || value.NamespaceURI != EncryptedXml.XmlEncNamespaceUrl) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidEncryptionProperty"));

            // cache the Xml
            m_cachedXml = value;
            m_id = Utils.GetAttribute(value, "Id", EncryptedXml.XmlEncNamespaceUrl);
            m_target = Utils.GetAttribute(value, "Target", EncryptedXml.XmlEncNamespaceUrl);
            m_elemProp = value;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class EncryptionPropertyCollection : IList {
        private ArrayList m_props;

        public EncryptionPropertyCollection() {
            m_props = new ArrayList();
        }

        public IEnumerator GetEnumerator() {
            return m_props.GetEnumerator();
        }

        public int Count {
            get { return m_props.Count; }
        }

        /// <internalonly/>
        int IList.Add(Object value) {
            if (!(value is EncryptionProperty)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            return m_props.Add(value);
        }

        public int Add(EncryptionProperty value) {
            return m_props.Add(value);
        }

        public void Clear() {
            m_props.Clear();
        }

        /// <internalonly/>
        bool IList.Contains(Object value) {
            if (!(value is EncryptionProperty)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            return m_props.Contains(value);
        }

        public bool Contains(EncryptionProperty value) {
            return m_props.Contains(value);
        }

        /// <internalonly/>
        int IList.IndexOf(Object value) {
            if (!(value is EncryptionProperty)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            return m_props.IndexOf(value);
        }

        public int IndexOf(EncryptionProperty value) {
            return m_props.IndexOf(value);
        }

        /// <internalonly/>
        void IList.Insert(int index, Object value) {
            if (!(value is EncryptionProperty)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            m_props.Insert(index, value);
        }

        public void Insert(int index, EncryptionProperty value) {
            m_props.Insert(index, value);
        }

        /// <internalonly/>
        void IList.Remove(Object value) {
            if (!(value is EncryptionProperty)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            m_props.Remove(value);
        }

        public void Remove(EncryptionProperty value) {
            m_props.Remove(value);
        }

        public void RemoveAt(int index) {
            m_props.RemoveAt(index);
        }

        public Boolean IsFixedSize {
            get { return m_props.IsFixedSize; }
        }

        public Boolean IsReadOnly {
            get { return m_props.IsReadOnly; }
        }

        public EncryptionProperty Item(int index) {
            return (EncryptionProperty) m_props[index];
        }

        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public EncryptionProperty this[int index] {
            get {
                return (EncryptionProperty) ((IList) this)[index];
            }
            set {
                ((IList) this)[index] = value;
            }
        }

        /// <internalonly/>
        Object IList.this[int index] {
            get { return m_props[index]; }
            set { 
                if (!(value is EncryptionProperty)) 
                    throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

                m_props[index] = value;
            }
        }

        /// <internalonly/>
        public void CopyTo(Array array, int index) {
            m_props.CopyTo(array, index);
        }

        public void CopyTo(EncryptionProperty[] array, int index) {
            m_props.CopyTo(array, index);
        }

        public Object SyncRoot {
            get { return m_props.SyncRoot; }
        }

        public bool IsSynchronized {
            get { return m_props.IsSynchronized; }
        }
    }
}
