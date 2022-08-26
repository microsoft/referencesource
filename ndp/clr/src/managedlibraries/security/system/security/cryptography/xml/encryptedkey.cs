// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// EncryptedKey.cs
//
// This object implements the EncryptedKey element.
// 
// 04/01/2002
// 

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class EncryptedKey : EncryptedType {
        private string m_recipient;
        private string m_carriedKeyName;
        private ReferenceList m_referenceList;

        public EncryptedKey () {}

        public string Recipient {
            get {
                // an unspecified value for an XmlAttribute is String.Empty
                if (m_recipient == null)
                    m_recipient = String.Empty;
                return m_recipient;
            }
            set {
                m_recipient = value;
                m_cachedXml = null;
            }
        }

       public string CarriedKeyName {
            get { return m_carriedKeyName; }
            set { 
                m_carriedKeyName = value; 
                m_cachedXml = null;
            }
        }

        public ReferenceList ReferenceList {
            get {
                if (m_referenceList == null)
                    m_referenceList = new ReferenceList();
                return m_referenceList;
            }
        }

        public void AddReference (DataReference dataReference) {
            ReferenceList.Add(dataReference);
        }

        public void AddReference (KeyReference keyReference) {
            ReferenceList.Add(keyReference);
        }

        public override void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);
            nsm.AddNamespace("ds",SignedXml.XmlDsigNamespaceUrl);

            this.Id = Utils.GetAttribute(value, "Id", EncryptedXml.XmlEncNamespaceUrl);
            this.Type = Utils.GetAttribute(value, "Type", EncryptedXml.XmlEncNamespaceUrl);
            this.MimeType = Utils.GetAttribute(value, "MimeType", EncryptedXml.XmlEncNamespaceUrl);
            this.Encoding = Utils.GetAttribute(value, "Encoding", EncryptedXml.XmlEncNamespaceUrl);
            this.Recipient = Utils.GetAttribute(value, "Recipient", EncryptedXml.XmlEncNamespaceUrl);

            XmlNode encryptionMethodNode = value.SelectSingleNode("enc:EncryptionMethod", nsm);

            // EncryptionMethod
            this.EncryptionMethod = new EncryptionMethod();
            if (encryptionMethodNode != null)
                this.EncryptionMethod.LoadXml(encryptionMethodNode as XmlElement);

            // Key Info
            this.KeyInfo = new KeyInfo();
            XmlNode keyInfoNode = value.SelectSingleNode("ds:KeyInfo", nsm);
            if (keyInfoNode != null)
                this.KeyInfo.LoadXml(keyInfoNode as XmlElement);

            // CipherData
            XmlNode cipherDataNode = value.SelectSingleNode("enc:CipherData", nsm);
            if (cipherDataNode == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_MissingCipherData"));

            this.CipherData = new CipherData();
            this.CipherData.LoadXml(cipherDataNode as XmlElement);

            // EncryptionProperties
            XmlNode encryptionPropertiesNode = value.SelectSingleNode("enc:EncryptionProperties", nsm);
            if (encryptionPropertiesNode != null) {
                // Select the EncryptionProperty elements inside the EncryptionProperties element
                XmlNodeList encryptionPropertyNodes = encryptionPropertiesNode.SelectNodes("enc:EncryptionProperty", nsm);
                if (encryptionPropertyNodes != null) {
                    foreach (XmlNode node in encryptionPropertyNodes) {
                        EncryptionProperty ep = new EncryptionProperty();
                        ep.LoadXml(node as XmlElement);
                        this.EncryptionProperties.Add(ep);
                    }
                }
            }

            // CarriedKeyName
            XmlNode carriedKeyNameNode = value.SelectSingleNode("enc:CarriedKeyName", nsm);
            if (carriedKeyNameNode != null) {
                this.CarriedKeyName = carriedKeyNameNode.InnerText;
            }

            // ReferenceList
            XmlNode referenceListNode = value.SelectSingleNode("enc:ReferenceList", nsm);
            if (referenceListNode != null) {
                // Select the DataReference elements inside the ReferenceList element
                XmlNodeList dataReferenceNodes = referenceListNode.SelectNodes("enc:DataReference", nsm);
                if (dataReferenceNodes != null) {
                    foreach (XmlNode node in dataReferenceNodes) {
                        DataReference dr = new DataReference();
                        dr.LoadXml(node as XmlElement);
                        this.ReferenceList.Add(dr);
                    }
                }
                // Select the KeyReference elements inside the ReferenceList element
                XmlNodeList keyReferenceNodes = referenceListNode.SelectNodes("enc:KeyReference", nsm);
                if (keyReferenceNodes != null) {
                    foreach (XmlNode node in keyReferenceNodes) {
                        KeyReference kr = new KeyReference();
                        kr.LoadXml(node as XmlElement);
                        this.ReferenceList.Add(kr);
                    }
                }
            }

            // Save away the cached value
            m_cachedXml = value;
        }

        public override XmlElement GetXml () {
            if (CacheValid) return m_cachedXml;

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the EncryptedKey element
            XmlElement encryptedKeyElement = (XmlElement) document.CreateElement("EncryptedKey", EncryptedXml.XmlEncNamespaceUrl);

            // Deal with attributes
            if (!String.IsNullOrEmpty(this.Id))
                encryptedKeyElement.SetAttribute("Id", this.Id);
            if (!String.IsNullOrEmpty(this.Type))
                encryptedKeyElement.SetAttribute("Type", this.Type);
            if (!String.IsNullOrEmpty(this.MimeType))
                encryptedKeyElement.SetAttribute("MimeType", this.MimeType);
            if (!String.IsNullOrEmpty(this.Encoding))
                encryptedKeyElement.SetAttribute("Encoding", this.Encoding);
            if (!String.IsNullOrEmpty(this.Recipient))
                encryptedKeyElement.SetAttribute("Recipient", this.Recipient);

            // EncryptionMethod
            if (this.EncryptionMethod != null)
                encryptedKeyElement.AppendChild(this.EncryptionMethod.GetXml(document));

            // KeyInfo
            if (this.KeyInfo.Count > 0)
                encryptedKeyElement.AppendChild(this.KeyInfo.GetXml(document));

            // CipherData
            if (this.CipherData == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_MissingCipherData"));
            encryptedKeyElement.AppendChild(this.CipherData.GetXml(document));

            // EncryptionProperties
            if (this.EncryptionProperties.Count > 0) {
                XmlElement encryptionPropertiesElement = document.CreateElement("EncryptionProperties", EncryptedXml.XmlEncNamespaceUrl);
                for (int index = 0; index < this.EncryptionProperties.Count; index++) {
                    EncryptionProperty ep = this.EncryptionProperties.Item(index);
                    encryptionPropertiesElement.AppendChild(ep.GetXml(document));
                }
                encryptedKeyElement.AppendChild(encryptionPropertiesElement);
            }

            // ReferenceList
            if (this.ReferenceList.Count > 0) {
                XmlElement referenceListElement = document.CreateElement("ReferenceList", EncryptedXml.XmlEncNamespaceUrl);
                for (int index = 0; index < this.ReferenceList.Count; index++) {
                    referenceListElement.AppendChild(this.ReferenceList[index].GetXml(document));
                }
                encryptedKeyElement.AppendChild(referenceListElement);
            }

            // CarriedKeyName
            if (this.CarriedKeyName != null) {
                XmlElement carriedKeyNameElement = (XmlElement) document.CreateElement("CarriedKeyName", EncryptedXml.XmlEncNamespaceUrl);
                XmlText carriedKeyNameText = document.CreateTextNode(this.CarriedKeyName);
                carriedKeyNameElement.AppendChild(carriedKeyNameText);                
                encryptedKeyElement.AppendChild(carriedKeyNameElement);
            }

            return encryptedKeyElement;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class ReferenceList : IList {
        private ArrayList m_references;

        public ReferenceList() {
            m_references = new ArrayList();
        }

        public IEnumerator GetEnumerator() {
            return m_references.GetEnumerator();
        }

        public int Count {
            get { return m_references.Count; }
        }

        public int Add(Object value) {
            if (value == null)
                throw new ArgumentNullException("value");

            if (!(value is DataReference) && !(value is KeyReference)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            return m_references.Add(value);
        }

        public void Clear() {
            m_references.Clear();
        }

        public bool Contains(Object value) {
            return m_references.Contains(value);
        }

        public int IndexOf(Object value) {
            return m_references.IndexOf(value);
        }

        public void Insert(int index, Object value) {
            if (value == null)
                throw new ArgumentNullException("value");

            if (!(value is DataReference) && !(value is KeyReference)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

            m_references.Insert(index, value);
        }

        public void Remove(Object value) {
            m_references.Remove(value);
        }

        public void RemoveAt(int index) {
            m_references.RemoveAt(index);
        }

        public EncryptedReference Item(int index) {
            return (EncryptedReference) m_references[index];
        }

        [System.Runtime.CompilerServices.IndexerName ("ItemOf")]
        public EncryptedReference this[int index] {
            get {
                return this.Item(index);
            }
            set {
                ((IList) this)[index] = value;
            }
        }

        /// <internalonly/>
        Object IList.this[int index] {
            get { return m_references[index]; }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (!(value is DataReference) && !(value is KeyReference)) 
                    throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");

                m_references[index] = value;
            }
        }

        public void CopyTo(Array array, int index) {
            m_references.CopyTo(array, index);
        }

        Boolean IList.IsFixedSize {
            get { return m_references.IsFixedSize; }
        }

        Boolean IList.IsReadOnly {
            get { return m_references.IsReadOnly; }
        }

        public Object SyncRoot {
            get { return m_references.SyncRoot; }
        }

        public bool IsSynchronized {
            get { return m_references.IsSynchronized; }
        }
    }
}
