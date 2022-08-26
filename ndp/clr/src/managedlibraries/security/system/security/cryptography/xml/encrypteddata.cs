// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// EncryptedData.cs
//
// This object implements the EncryptedData element.
// 
// 04/01/2002
//

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public sealed class EncryptedData : EncryptedType {
        public override void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            this.Id = Utils.GetAttribute(value, "Id", EncryptedXml.XmlEncNamespaceUrl);
            this.Type = Utils.GetAttribute(value, "Type", EncryptedXml.XmlEncNamespaceUrl);
            this.MimeType = Utils.GetAttribute(value, "MimeType", EncryptedXml.XmlEncNamespaceUrl);
            this.Encoding = Utils.GetAttribute(value, "Encoding", EncryptedXml.XmlEncNamespaceUrl);

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

            // Save away the cached value
            m_cachedXml = value;
        }

        public override XmlElement GetXml() {
            if (CacheValid) return(m_cachedXml);

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the EncryptedData element
            XmlElement encryptedDataElement = (XmlElement) document.CreateElement("EncryptedData", EncryptedXml.XmlEncNamespaceUrl);

            // Deal with attributes
            if (!String.IsNullOrEmpty(this.Id))
                encryptedDataElement.SetAttribute("Id", this.Id);
            if (!String.IsNullOrEmpty(this.Type))
                encryptedDataElement.SetAttribute("Type", this.Type);
            if (!String.IsNullOrEmpty(this.MimeType))
                encryptedDataElement.SetAttribute("MimeType", this.MimeType);
            if (!String.IsNullOrEmpty(this.Encoding))
                encryptedDataElement.SetAttribute("Encoding", this.Encoding);

            // EncryptionMethod
            if (this.EncryptionMethod != null)
                encryptedDataElement.AppendChild(this.EncryptionMethod.GetXml(document));

            // KeyInfo
            if (this.KeyInfo.Count > 0)
                encryptedDataElement.AppendChild(this.KeyInfo.GetXml(document));

            // CipherData is required.
            if (this.CipherData == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_MissingCipherData"));
            encryptedDataElement.AppendChild(this.CipherData.GetXml(document));

            // EncryptionProperties
            if (this.EncryptionProperties.Count > 0) {
                XmlElement encryptionPropertiesElement = document.CreateElement("EncryptionProperties", EncryptedXml.XmlEncNamespaceUrl);
                for (int index = 0; index < this.EncryptionProperties.Count; index++) {
                    EncryptionProperty ep = this.EncryptionProperties.Item(index);
                    encryptionPropertiesElement.AppendChild(ep.GetXml(document));
                }
                encryptedDataElement.AppendChild(encryptionPropertiesElement);
            }
            return encryptedDataElement;
        }
    }
}
