// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// EncryptedReference.cs
//
// This object implements the EncryptedReference element.
// 
// 04/01/2002
// 

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public abstract class EncryptedReference {
        private string m_uri;
        private string m_referenceType;
        private TransformChain m_transformChain;
        internal XmlElement m_cachedXml = null;

        protected EncryptedReference () : this (String.Empty, new TransformChain()) {
        }

        protected EncryptedReference (string uri) : this (uri, new TransformChain()) {
        }

        protected EncryptedReference (string uri, TransformChain transformChain) {
            this.TransformChain = transformChain;
            this.Uri = uri;
            m_cachedXml = null;
        }

        public string Uri {
            get { return m_uri; }
            set { 
                if (value == null)
                    throw new ArgumentNullException(SecurityResources.GetResourceString("Cryptography_Xml_UriRequired"));
                m_uri = value;
                m_cachedXml = null;
            }
        }

        public TransformChain TransformChain {
            get { 
                if (m_transformChain == null)
                    m_transformChain = new TransformChain();
                return m_transformChain; 
            }
            set {
                m_transformChain = value;
                m_cachedXml = null;
            }
        }

        public void AddTransform (Transform transform) {
            this.TransformChain.Add(transform);
        }

        protected string ReferenceType {
            get { return m_referenceType; }
            set {
                m_referenceType = value;
                m_cachedXml = null;
            }
        }

        internal protected bool CacheValid {
            get {
                return (m_cachedXml != null);
            }
        }

        public virtual XmlElement GetXml () {
            if (CacheValid) return m_cachedXml;

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            if (ReferenceType == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_ReferenceTypeRequired"));

            // Create the Reference
            XmlElement referenceElement = document.CreateElement(ReferenceType, EncryptedXml.XmlEncNamespaceUrl);
            if (!String.IsNullOrEmpty(m_uri))
                referenceElement.SetAttribute("URI", m_uri);

            // Add the transforms to the CipherReference
            if (this.TransformChain.Count > 0)
                referenceElement.AppendChild(this.TransformChain.GetXml(document, SignedXml.XmlDsigNamespaceUrl));

            return referenceElement;
        }

        public virtual void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            this.ReferenceType = value.LocalName;
            this.Uri = Utils.GetAttribute(value, "URI", EncryptedXml.XmlEncNamespaceUrl);

            // Transforms
            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            XmlNode transformsNode = value.SelectSingleNode("ds:Transforms", nsm);
            if (transformsNode != null)
                this.TransformChain.LoadXml(transformsNode as XmlElement);

            // cache the Xml
            m_cachedXml = value;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CipherReference : EncryptedReference {
        private byte[] m_cipherValue;

        public CipherReference () : base () {
            ReferenceType = "CipherReference";
        }

        public CipherReference (string uri) : base(uri) {
            ReferenceType = "CipherReference";
        }

        public CipherReference (string uri, TransformChain transformChain) : base(uri, transformChain) {
            ReferenceType = "CipherReference";
        }

        // This method is used to cache results from resolved cipher references.
        internal byte[] CipherValue {
            get {
                if (!CacheValid)
                    return null;
                return m_cipherValue;
            }
            set {
                m_cipherValue = value;
            }
        }

        public override XmlElement GetXml () {
            if (CacheValid) return m_cachedXml;

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        new internal XmlElement GetXml (XmlDocument document) {
            if (ReferenceType == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_ReferenceTypeRequired"));

            // Create the Reference
            XmlElement referenceElement = document.CreateElement(ReferenceType, EncryptedXml.XmlEncNamespaceUrl);
            if (!String.IsNullOrEmpty(this.Uri))
                referenceElement.SetAttribute("URI", this.Uri);

            // Add the transforms to the CipherReference
            if (this.TransformChain.Count > 0) 
                referenceElement.AppendChild(this.TransformChain.GetXml(document, EncryptedXml.XmlEncNamespaceUrl));

            return referenceElement;
        }

        public override void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            this.ReferenceType = value.LocalName;
            string uri = Utils.GetAttribute(value, "URI", EncryptedXml.XmlEncNamespaceUrl);
            if (!Utils.GetSkipSignatureAttributeEnforcement() && uri == null) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UriRequired"));
            }
            this.Uri = uri;

            // Transforms
            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);
            XmlNode transformsNode = value.SelectSingleNode("enc:Transforms", nsm);
            if (transformsNode != null)
                this.TransformChain.LoadXml(transformsNode as XmlElement);

            // cache the Xml
            m_cachedXml = value;
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class DataReference : EncryptedReference {
        public DataReference () : base () {
            ReferenceType = "DataReference";
        }

        public DataReference (string uri) : base(uri) {
            ReferenceType = "DataReference";
        }

        public DataReference (string uri, TransformChain transformChain) : base(uri, transformChain) {
            ReferenceType = "DataReference";
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class KeyReference : EncryptedReference {
        public KeyReference () : base () {
            ReferenceType = "KeyReference";
        }

        public KeyReference (string uri) : base(uri) {
            ReferenceType = "KeyReference";
        }

        public KeyReference (string uri, TransformChain transformChain) : base(uri, transformChain) {
            ReferenceType = "KeyReference";
        }
    }
}
