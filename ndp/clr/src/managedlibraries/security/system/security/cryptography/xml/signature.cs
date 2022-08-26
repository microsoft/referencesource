// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// Signature.cs
// 
// 21 Microsoft 2000
// 

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class Signature {
        private string m_id;
        private SignedInfo m_signedInfo;
        private byte[] m_signatureValue;
        private string m_signatureValueId;
        private KeyInfo m_keyInfo;
        private IList m_embeddedObjects;
        private CanonicalXmlNodeList m_referencedItems;
        private SignedXml m_signedXml = null;

        internal SignedXml SignedXml {
            get { return m_signedXml; }
            set { m_signedXml = value; }
        }

        //
        // public constructors
        //

        public Signature() {
            m_embeddedObjects = new ArrayList();
            m_referencedItems = new CanonicalXmlNodeList();
        }

        //
        // public properties
        //

        public string Id {
            get { return m_id; }
            set { m_id = value; }
        }

        public SignedInfo SignedInfo {
            get { return m_signedInfo; }
            set { 
                m_signedInfo = value;
                if (this.SignedXml != null && m_signedInfo != null)
                    m_signedInfo.SignedXml = this.SignedXml;
            }
        }

        public byte[] SignatureValue {
            get { return m_signatureValue; }
            set { m_signatureValue = value; }
        }

        public KeyInfo KeyInfo {
            get {
                if (m_keyInfo == null)
                    m_keyInfo = new KeyInfo();
                return m_keyInfo;
            }
            set { m_keyInfo = value; }
        }

        public IList ObjectList {
            get { return m_embeddedObjects; }
            set { m_embeddedObjects = value; }
        }

        internal CanonicalXmlNodeList ReferencedItems {
            get { return m_referencedItems; }
        }

        //
        // public methods
        //

        public XmlElement GetXml() {
            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the Signature
            XmlElement signatureElement = (XmlElement)document.CreateElement("Signature", SignedXml.XmlDsigNamespaceUrl);
            if (!String.IsNullOrEmpty(m_id))
                signatureElement.SetAttribute("Id", m_id);

            // Add the SignedInfo
            if (m_signedInfo == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignedInfoRequired"));

            signatureElement.AppendChild(m_signedInfo.GetXml(document));

            // Add the SignatureValue
            if (m_signatureValue == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureValueRequired"));

            XmlElement signatureValueElement = document.CreateElement("SignatureValue", SignedXml.XmlDsigNamespaceUrl);
            signatureValueElement.AppendChild(document.CreateTextNode(Convert.ToBase64String(m_signatureValue)));
            if (!String.IsNullOrEmpty(m_signatureValueId))
                signatureValueElement.SetAttribute("Id", m_signatureValueId);
            signatureElement.AppendChild(signatureValueElement);

            // Add the KeyInfo
            if (this.KeyInfo.Count > 0)
                signatureElement.AppendChild(this.KeyInfo.GetXml(document));

            // Add the Objects
            foreach (Object obj in m_embeddedObjects) {
                DataObject dataObj = obj as DataObject;
                if (dataObj != null) {
                    signatureElement.AppendChild(dataObj.GetXml(document));
                }
            }

            return signatureElement;
        }

        public void LoadXml(XmlElement value) {
             // Make sure we don't get passed null
            if (value == null)
                throw new ArgumentNullException("value");

            // Signature
            XmlElement signatureElement = value;
            if (!signatureElement.LocalName.Equals("Signature"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Signature");

            // Attributes
            m_id = Utils.GetAttribute(signatureElement, "Id", SignedXml.XmlDsigNamespaceUrl);
            if (!Utils.VerifyAttributes(signatureElement, "Id"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Signature");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            int expectedChildNodes = 0;

            // SignedInfo
            XmlNodeList signedInfoNodes = signatureElement.SelectNodes("ds:SignedInfo", nsm);
            if (signedInfoNodes == null || signedInfoNodes.Count == 0 || (!Utils.GetAllowAdditionalSignatureNodes() && signedInfoNodes.Count > 1))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"SignedInfo");
            XmlElement signedInfoElement = signedInfoNodes[0] as XmlElement;
            expectedChildNodes += signedInfoNodes.Count;

            this.SignedInfo = new SignedInfo();
            this.SignedInfo.LoadXml(signedInfoElement);

            // SignatureValue
            XmlNodeList signatureValueNodes = signatureElement.SelectNodes("ds:SignatureValue", nsm);
            if (signatureValueNodes == null || signatureValueNodes.Count == 0 || (!Utils.GetAllowAdditionalSignatureNodes() && signatureValueNodes.Count > 1))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"),"SignatureValue");
            XmlElement signatureValueElement = signatureValueNodes[0] as XmlElement;
            expectedChildNodes += signatureValueNodes.Count;
            m_signatureValue = Convert.FromBase64String(Utils.DiscardWhiteSpaces(signatureValueElement.InnerText));
            m_signatureValueId = Utils.GetAttribute(signatureValueElement, "Id", SignedXml.XmlDsigNamespaceUrl);
            if (!Utils.VerifyAttributes(signatureValueElement, "Id"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignatureValue");

            // KeyInfo - optional single element
            XmlNodeList keyInfoNodes = signatureElement.SelectNodes("ds:KeyInfo", nsm);
            m_keyInfo = new KeyInfo();
            if (keyInfoNodes != null) {
                if (!Utils.GetAllowAdditionalSignatureNodes() && keyInfoNodes.Count > 1) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "KeyInfo");
                }
                foreach(XmlNode node in keyInfoNodes) {
                    XmlElement keyInfoElement = node as XmlElement;
                    if (keyInfoElement != null)
                        m_keyInfo.LoadXml(keyInfoElement);
                }
                expectedChildNodes += keyInfoNodes.Count;
            }

            // Object - zero or more elements allowed
            XmlNodeList objectNodes = signatureElement.SelectNodes("ds:Object", nsm);
            m_embeddedObjects.Clear();
            if (objectNodes != null) {
                foreach(XmlNode node in objectNodes) {
                    XmlElement objectElement = node as XmlElement;
                    if (objectElement != null) {
                        DataObject dataObj = new DataObject();
                        dataObj.LoadXml(objectElement);
                        m_embeddedObjects.Add(dataObj);
                    }
                }
                expectedChildNodes += objectNodes.Count;
            }

            // Select all elements that have Id attributes
            XmlNodeList nodeList = signatureElement.SelectNodes("//*[@Id]", nsm);
            if (nodeList != null) {
                foreach (XmlNode node in nodeList) {
                    m_referencedItems.Add(node);
                }
            }

            // Verify that there aren't any extra nodes that aren't allowed
            if (!Utils.GetAllowAdditionalSignatureNodes() && (signatureElement.SelectNodes("*").Count != expectedChildNodes)) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Signature");
            }
        }

        public void AddObject(DataObject dataObject) {
            m_embeddedObjects.Add(dataObject);
        }
    }
}

