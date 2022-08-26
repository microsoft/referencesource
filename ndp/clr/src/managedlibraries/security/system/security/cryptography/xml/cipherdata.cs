// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// CipherData.cs
//
// This object implements the CipherData element.
// 
// 04/01/2001
// 

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class CipherData {
        private XmlElement m_cachedXml = null;
        private CipherReference m_cipherReference = null;
        private byte[] m_cipherValue = null;

        public CipherData () {}

        public CipherData (byte[] cipherValue) {
            this.CipherValue = cipherValue;
        }

        public CipherData (CipherReference cipherReference) {
            this.CipherReference = cipherReference;
        }

        private bool CacheValid {
            get { 
                return (m_cachedXml != null);
            }
        }

        public CipherReference CipherReference {
            get { return m_cipherReference; }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");
                if (this.CipherValue != null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CipherValueElementRequired"));

                m_cipherReference = value;
                m_cachedXml = null;
            }
        }

        public byte[] CipherValue {
            get { return m_cipherValue; }
            set { 
                if (value == null)
                    throw new ArgumentNullException("value");
                if (this.CipherReference != null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CipherValueElementRequired"));

                m_cipherValue = (byte[]) value.Clone(); 
                m_cachedXml = null;
            }
        }

        public XmlElement GetXml () {
            if (CacheValid) return m_cachedXml;

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the CipherData element
            XmlElement cipherDataElement = (XmlElement)document.CreateElement("CipherData", EncryptedXml.XmlEncNamespaceUrl);
            if (CipherValue != null) {
                XmlElement cipherValueElement = document.CreateElement("CipherValue", EncryptedXml.XmlEncNamespaceUrl);
                cipherValueElement.AppendChild(document.CreateTextNode(Convert.ToBase64String(CipherValue)));
                cipherDataElement.AppendChild(cipherValueElement);
            } else {
                // No CipherValue specified, see if there is a CipherReference
                if (CipherReference == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CipherValueElementRequired"));
                cipherDataElement.AppendChild(CipherReference.GetXml(document));
            }
            return cipherDataElement;
        }

        public void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);

            XmlNode cipherValueNode = value.SelectSingleNode("enc:CipherValue", nsm);
            XmlNode cipherReferenceNode = value.SelectSingleNode("enc:CipherReference", nsm);
            if (cipherValueNode != null) {
                if (cipherReferenceNode != null) 
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CipherValueElementRequired"));
                m_cipherValue = Convert.FromBase64String(Utils.DiscardWhiteSpaces(cipherValueNode.InnerText));
            } else if (cipherReferenceNode != null) {
                m_cipherReference = new CipherReference();
                m_cipherReference.LoadXml((XmlElement) cipherReferenceNode);
            } else {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CipherValueElementRequired"));
            }

            // Save away the cached value
            m_cachedXml = value;
        }
    }
}
