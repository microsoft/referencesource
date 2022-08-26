// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// SignedInfo.cs
// 
// 21 Microsoft 2000
//

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Xml;
    using System.Globalization;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class SignedInfo : ICollection {
        private string m_id;
        private string m_canonicalizationMethod;
        private string m_signatureMethod;
        private string m_signatureLength;
        private ArrayList m_references;
        private XmlElement m_cachedXml = null;
        private SignedXml m_signedXml = null;
        private Transform m_canonicalizationMethodTransform = null;

        internal SignedXml SignedXml {
            get { return m_signedXml; }
            set { m_signedXml = value; }
        }

        public SignedInfo() {
            m_references = new ArrayList();
        }

        public IEnumerator GetEnumerator() {
            throw new NotSupportedException();
        } 

        public void CopyTo(Array array, int index) {
            throw new NotSupportedException();
        }

        public Int32 Count {
            get { throw new NotSupportedException(); }
        }

        public Boolean IsReadOnly {
            get { throw new NotSupportedException(); }
        }

        public Boolean IsSynchronized {
            get { throw new NotSupportedException(); }
        }

        public object SyncRoot {
            get { throw new NotSupportedException(); }
        }

        //
        // public properties
        //

        public string Id {
            get { return m_id; }
            set {
                m_id = value; 
                m_cachedXml = null;
            }
        }

        public string CanonicalizationMethod {
            get {
                // Default the canonicalization method to C14N
                if (m_canonicalizationMethod == null)
                    return SignedXml.XmlDsigC14NTransformUrl;
                return m_canonicalizationMethod; 
            }
            set {
                m_canonicalizationMethod = value;
                m_cachedXml = null;
            }
        }

        [ComVisible(false)]
        public Transform CanonicalizationMethodObject {
            get {
                if (m_canonicalizationMethodTransform == null) {
                    m_canonicalizationMethodTransform = Utils.CreateFromName<Transform>(this.CanonicalizationMethod);
                    if (m_canonicalizationMethodTransform == null)
                        throw new CryptographicException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Cryptography_Xml_CreateTransformFailed"), this.CanonicalizationMethod));
                    m_canonicalizationMethodTransform.SignedXml = this.SignedXml;
                    m_canonicalizationMethodTransform.Reference = null;
                }
                return m_canonicalizationMethodTransform;
            }
        }

        public string SignatureMethod {
            get { return m_signatureMethod; }
            set {
                m_signatureMethod = value;
                m_cachedXml = null;
            }
        }

        public string SignatureLength {
            get { return m_signatureLength; }
            set {
                m_signatureLength = value;
                m_cachedXml = null;
            }
        }

        public ArrayList References {
            get { return m_references; }
        }

        internal bool CacheValid {
            get {
                if (m_cachedXml == null) return false;
                // now check all the references
                foreach (Reference reference in References) {
                    if (!reference.CacheValid) return false;
                }
                return true;
            }
        }

        //
        // public methods
        //

        public XmlElement GetXml() {
            if (CacheValid) return m_cachedXml;

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the root element
            XmlElement signedInfoElement = document.CreateElement("SignedInfo", SignedXml.XmlDsigNamespaceUrl);
            if (!String.IsNullOrEmpty(m_id))
                signedInfoElement.SetAttribute("Id", m_id);

            // Add the canonicalization method, defaults to SignedXml.XmlDsigNamespaceUrl
            XmlElement canonicalizationMethodElement = this.CanonicalizationMethodObject.GetXml(document, "CanonicalizationMethod");
            signedInfoElement.AppendChild(canonicalizationMethodElement);

            // Add the signature method
            if (String.IsNullOrEmpty(m_signatureMethod))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_SignatureMethodRequired"));

            XmlElement signatureMethodElement = document.CreateElement("SignatureMethod", SignedXml.XmlDsigNamespaceUrl);
            signatureMethodElement.SetAttribute("Algorithm", m_signatureMethod);
            // Add HMACOutputLength tag if we have one
            if (m_signatureLength != null) {
                XmlElement hmacLengthElement = document.CreateElement(null, "HMACOutputLength", SignedXml.XmlDsigNamespaceUrl);
                XmlText outputLength = document.CreateTextNode(m_signatureLength);
                hmacLengthElement.AppendChild(outputLength);
                signatureMethodElement.AppendChild(hmacLengthElement);
            }

            signedInfoElement.AppendChild(signatureMethodElement);

            // Add the references
            if (m_references.Count == 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_ReferenceElementRequired"));

            for (int i = 0; i < m_references.Count; ++i) {
                Reference reference = (Reference)m_references[i];
                signedInfoElement.AppendChild(reference.GetXml(document));
            }

            return signedInfoElement;
        }

        public void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            // SignedInfo
            XmlElement signedInfoElement = value;
            if (!signedInfoElement.LocalName.Equals("SignedInfo"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);
            int expectedChildNodes = 0;

            // Id attribute -- optional
            m_id = Utils.GetAttribute(signedInfoElement, "Id", SignedXml.XmlDsigNamespaceUrl);
            if (!Utils.VerifyAttributes(signedInfoElement, "Id"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo");

            // CanonicalizationMethod -- must be present
            XmlNodeList canonicalizationMethodNodes = signedInfoElement.SelectNodes("ds:CanonicalizationMethod", nsm);
            if (canonicalizationMethodNodes == null || canonicalizationMethodNodes.Count == 0 || (!Utils.GetAllowAdditionalSignatureNodes() && canonicalizationMethodNodes.Count > 1))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo/CanonicalizationMethod");
            XmlElement canonicalizationMethodElement = canonicalizationMethodNodes.Item(0) as XmlElement;
            expectedChildNodes += canonicalizationMethodNodes.Count;
            m_canonicalizationMethod = Utils.GetAttribute(canonicalizationMethodElement, "Algorithm", SignedXml.XmlDsigNamespaceUrl);
            if ((m_canonicalizationMethod == null && !Utils.GetSkipSignatureAttributeEnforcement()) || !Utils.VerifyAttributes(canonicalizationMethodElement, "Algorithm"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo/CanonicalizationMethod");
            m_canonicalizationMethodTransform = null;
            if (canonicalizationMethodElement.ChildNodes.Count > 0)
                this.CanonicalizationMethodObject.LoadInnerXml(canonicalizationMethodElement.ChildNodes);

            // SignatureMethod -- must be present
            XmlNodeList signatureMethodNodes = signedInfoElement.SelectNodes("ds:SignatureMethod", nsm);
            if (signatureMethodNodes == null || signatureMethodNodes.Count == 0 || (!Utils.GetAllowAdditionalSignatureNodes() && signatureMethodNodes.Count > 1))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo/SignatureMethod");
            XmlElement signatureMethodElement = signatureMethodNodes.Item(0) as XmlElement;
            expectedChildNodes += signatureMethodNodes.Count;
            m_signatureMethod = Utils.GetAttribute(signatureMethodElement, "Algorithm", SignedXml.XmlDsigNamespaceUrl);
            if ((m_signatureMethod == null && !Utils.GetSkipSignatureAttributeEnforcement()) || !Utils.VerifyAttributes(signatureMethodElement, "Algorithm"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo/SignatureMethod");

            // Now get the output length if we are using a MAC algorithm
            XmlElement signatureLengthElement = signatureMethodElement.SelectSingleNode("ds:HMACOutputLength", nsm) as XmlElement;
            if (signatureLengthElement != null) 
                m_signatureLength = signatureLengthElement.InnerXml;

            // flush out any reference that was there
            m_references.Clear();

            // Reference - 0 or more
            XmlNodeList referenceNodes = signedInfoElement.SelectNodes("ds:Reference", nsm);
            if (referenceNodes != null) {
                if (referenceNodes.Count > Utils.GetMaxReferencesPerSignedInfo()) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo/Reference");
                }
                foreach(XmlNode node in referenceNodes) {
                    XmlElement referenceElement = node as XmlElement;
                    Reference reference = new Reference();
                    AddReference(reference);
                    reference.LoadXml(referenceElement);
                }
                expectedChildNodes += referenceNodes.Count;
            }

            // Verify that there aren't any extra nodes that aren't allowed
            if (!Utils.GetAllowAdditionalSignatureNodes() && (signedInfoElement.SelectNodes("*").Count != expectedChildNodes)) {
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "SignedInfo");
            }

            // Save away the cached value
            m_cachedXml = signedInfoElement;
        }

        public void AddReference (Reference reference) {
            if (reference == null)
                throw new ArgumentNullException("reference");

            reference.SignedXml = this.SignedXml;
            m_references.Add(reference);
        }
    }
}
