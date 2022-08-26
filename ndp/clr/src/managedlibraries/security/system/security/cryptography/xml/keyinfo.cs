// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// KeyInfo.cs
// 
// 21 Microsoft 2000
// 

namespace System.Security.Cryptography.Xml 
{
    using System;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class KeyInfo : IEnumerable {
        private string m_id = null;
        private ArrayList m_KeyInfoClauses;

        //
        // public constructors
        //

        public KeyInfo() {
            m_KeyInfoClauses = new ArrayList();
        }

        //
        // public properties
        //

        public String Id {
            get { return m_id; }
            set { m_id = value; }
        }

        public XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal XmlElement GetXml (XmlDocument xmlDocument) {
            // Create the KeyInfo element itself
            XmlElement keyInfoElement = xmlDocument.CreateElement("KeyInfo", SignedXml.XmlDsigNamespaceUrl);
            if (!String.IsNullOrEmpty(m_id)) {
                keyInfoElement.SetAttribute("Id", m_id);
            }

            // Add all the clauses that go underneath it
            for (int i = 0; i < m_KeyInfoClauses.Count; ++i) {
                XmlElement xmlElement = ((KeyInfoClause) m_KeyInfoClauses[i]).GetXml(xmlDocument);
                if (xmlElement != null) {
                    keyInfoElement.AppendChild(xmlElement);
                }
            }
            return keyInfoElement;
        }

        public void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlElement keyInfoElement = value;
            m_id = Utils.GetAttribute(keyInfoElement, "Id", SignedXml.XmlDsigNamespaceUrl);
            if (!Utils.VerifyAttributes(keyInfoElement, "Id"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "KeyInfo");

            XmlNode child = keyInfoElement.FirstChild;
            while (child != null) {
                XmlElement elem = child as XmlElement;
                if (elem != null) {
                    // Create the right type of KeyInfoClause; we use a combination of the namespace and tag name (local name)
                    String kicString = elem.NamespaceURI + " " + elem.LocalName;
                    // Special-case handling for KeyValue -- we have to go one level deeper
                    if (kicString == "http://www.w3.org/2000/09/xmldsig# KeyValue") {
                        if (!Utils.VerifyAttributes(elem, (string[])null)) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "KeyInfo/KeyValue");
                        }

                        XmlNodeList nodeList2 = elem.ChildNodes;
                        foreach (XmlNode node2 in nodeList2) {
                            XmlElement elem2 = node2 as XmlElement;
                            if (elem2 != null) {
                                kicString += "/" + elem2.LocalName;
                                break;
                            }
                        }
                    }
                    KeyInfoClause keyInfoClause = Utils.CreateFromName<KeyInfoClause>(kicString);
                    // if we don't know what kind of KeyInfoClause we're looking at, use a generic KeyInfoNode:
                    if (keyInfoClause == null)
                        keyInfoClause = new KeyInfoNode();

                    // Ask the create clause to fill itself with the corresponding XML
                    keyInfoClause.LoadXml(elem);
                    // Add it to our list of KeyInfoClauses
                    AddClause(keyInfoClause);
                }
                child = child.NextSibling;
            }
        }

        public Int32 Count {
            get { return m_KeyInfoClauses.Count; }
        }

        //
        // public constructors
        //

        public void AddClause(KeyInfoClause clause) {
            m_KeyInfoClauses.Add(clause);
        }

        public IEnumerator GetEnumerator() {
            return m_KeyInfoClauses.GetEnumerator();
        }

        public IEnumerator GetEnumerator(Type requestedObjectType) {
            ArrayList requestedList = new ArrayList();

            Object tempObj;
            IEnumerator tempEnum = m_KeyInfoClauses.GetEnumerator();

            while(tempEnum.MoveNext()) {
                tempObj = tempEnum.Current;
                if (requestedObjectType.Equals(tempObj.GetType()))
                    requestedList.Add(tempObj);
            }

            return requestedList.GetEnumerator();
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public abstract class KeyInfoClause {
        //
        // protected constructors
        //

        protected KeyInfoClause () {}

        //
        // public methods
        //

        public abstract XmlElement GetXml();
        internal virtual XmlElement GetXml (XmlDocument xmlDocument) {
            XmlElement keyInfo = GetXml();
            return (XmlElement) xmlDocument.ImportNode(keyInfo, true);
        }

        public abstract void LoadXml(XmlElement element);
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class KeyInfoName : KeyInfoClause {
        private string m_keyName;

        //
        // public constructors
        //

        public KeyInfoName () : this (null) {}

        public KeyInfoName (string keyName) {
            this.Value = keyName;
        }

        //
        // public properties
        //

        public String Value {
            get { return m_keyName; }
            set { m_keyName = value; }
        }

        //
        // public methods
        //

        public override XmlElement GetXml() {
            XmlDocument xmlDocument =  new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            XmlElement nameElement = xmlDocument.CreateElement("KeyName", SignedXml.XmlDsigNamespaceUrl);
            nameElement.AppendChild(xmlDocument.CreateTextNode(m_keyName));
            return nameElement;
        }

        public override void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");
            XmlElement nameElement = value;
            m_keyName = nameElement.InnerText.Trim();
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class DSAKeyValue : KeyInfoClause {
        private DSA m_key;

        //
        // public constructors
        //

        public DSAKeyValue () {
            m_key = DSA.Create();
        }

        public DSAKeyValue (DSA key) {
            m_key = key;
        }

        //
        // public properties
        //

        public DSA Key {
            get { return m_key; }
            set { m_key = value; }
        }

        //
        // public methods
        //

        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            DSAParameters dsaParams = m_key.ExportParameters(false);

            XmlElement keyValueElement = xmlDocument.CreateElement("KeyValue", SignedXml.XmlDsigNamespaceUrl);
            XmlElement dsaKeyValueElement = xmlDocument.CreateElement("DSAKeyValue", SignedXml.XmlDsigNamespaceUrl);

            XmlElement pElement = xmlDocument.CreateElement("P", SignedXml.XmlDsigNamespaceUrl);
            pElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(dsaParams.P)));
            dsaKeyValueElement.AppendChild(pElement);

            XmlElement qElement = xmlDocument.CreateElement("Q", SignedXml.XmlDsigNamespaceUrl);
            qElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(dsaParams.Q)));
            dsaKeyValueElement.AppendChild(qElement);

            XmlElement gElement = xmlDocument.CreateElement("G", SignedXml.XmlDsigNamespaceUrl);
            gElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(dsaParams.G)));
            dsaKeyValueElement.AppendChild(gElement);

            XmlElement yElement = xmlDocument.CreateElement("Y", SignedXml.XmlDsigNamespaceUrl);
            yElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(dsaParams.Y)));
            dsaKeyValueElement.AppendChild(yElement);

            // Add optional components if present
            if (dsaParams.J != null) {
                XmlElement jElement = xmlDocument.CreateElement("J", SignedXml.XmlDsigNamespaceUrl);
                jElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(dsaParams.J)));
                dsaKeyValueElement.AppendChild(jElement);
            }

            if (dsaParams.Seed != null) {  // note we assume counter is correct if Seed is present
                XmlElement seedElement = xmlDocument.CreateElement("Seed", SignedXml.XmlDsigNamespaceUrl);
                seedElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(dsaParams.Seed)));
                dsaKeyValueElement.AppendChild(seedElement);

                XmlElement counterElement = xmlDocument.CreateElement("PgenCounter", SignedXml.XmlDsigNamespaceUrl);
                counterElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(Utils.ConvertIntToByteArray(dsaParams.Counter))));
                dsaKeyValueElement.AppendChild(counterElement);
            }

            keyValueElement.AppendChild(dsaKeyValueElement);

            return keyValueElement;
        }

        public override void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            // Get the XML string 
            m_key.FromXmlString(value.OuterXml);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class RSAKeyValue : KeyInfoClause {
        private RSA m_key;

        //
        // public constructors
        //

        public RSAKeyValue () {
            m_key = RSA.Create();
        }

        public RSAKeyValue (RSA key) {
            m_key = key;
        }

        //
        // public properties
        //

        public RSA Key {
            get { return m_key; }
            set { m_key = value; }
        }

        //
        // public methods
        //

        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            RSAParameters rsaParams = m_key.ExportParameters(false);

            XmlElement keyValueElement = xmlDocument.CreateElement("KeyValue", SignedXml.XmlDsigNamespaceUrl);
            XmlElement rsaKeyValueElement = xmlDocument.CreateElement("RSAKeyValue", SignedXml.XmlDsigNamespaceUrl);

            XmlElement modulusElement = xmlDocument.CreateElement("Modulus", SignedXml.XmlDsigNamespaceUrl);
            modulusElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(rsaParams.Modulus)));
            rsaKeyValueElement.AppendChild(modulusElement);

            XmlElement exponentElement = xmlDocument.CreateElement("Exponent", SignedXml.XmlDsigNamespaceUrl);
            exponentElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(rsaParams.Exponent)));
            rsaKeyValueElement.AppendChild(exponentElement);

            keyValueElement.AppendChild(rsaKeyValueElement);

            return keyValueElement;
        }

        public override void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            // Get the XML string 
            m_key.FromXmlString(value.OuterXml);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class KeyInfoRetrievalMethod : KeyInfoClause {
        private String m_uri;
        private String m_type;

        //
        // public constructors
        //

        public KeyInfoRetrievalMethod () {}

        public KeyInfoRetrievalMethod (string strUri) {
            m_uri = strUri;
        }

        public KeyInfoRetrievalMethod (String strUri, String typeName) {
            m_uri = strUri;
            m_type = typeName;
        }

        //
        // public properties
        //

        public String Uri {
            get { return m_uri; }
            set { m_uri = value; }
        }

        [ComVisible(false)]
        public String Type {
            get { return m_type; }
            set { m_type = value; }
        }

        public override XmlElement GetXml() {
            XmlDocument xmlDocument =  new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            // Create the actual element
            XmlElement retrievalMethodElement = xmlDocument.CreateElement("RetrievalMethod",SignedXml.XmlDsigNamespaceUrl);

            if (!String.IsNullOrEmpty(m_uri))
                retrievalMethodElement.SetAttribute("URI", m_uri);
            if (!String.IsNullOrEmpty(m_type))
                retrievalMethodElement.SetAttribute("Type", m_type);

            return retrievalMethodElement;
        }

        public override void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlElement retrievalMethodElement = value;
            m_uri = Utils.GetAttribute(value, "URI", SignedXml.XmlDsigNamespaceUrl);
            m_type = Utils.GetAttribute(value, "Type", SignedXml.XmlDsigNamespaceUrl);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class KeyInfoEncryptedKey : KeyInfoClause {
        private EncryptedKey m_encryptedKey;

        public KeyInfoEncryptedKey () {}
        
        public KeyInfoEncryptedKey (EncryptedKey encryptedKey) {
            m_encryptedKey = encryptedKey;
        }

        public EncryptedKey EncryptedKey {
            get { return m_encryptedKey; }
            set { m_encryptedKey = value; }
        }

        public override XmlElement GetXml() {
            if (m_encryptedKey == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "KeyInfoEncryptedKey");
            return m_encryptedKey.GetXml();
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            if (m_encryptedKey == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "KeyInfoEncryptedKey");
            return m_encryptedKey.GetXml(xmlDocument);
        }

        public override void LoadXml(XmlElement value) {
            m_encryptedKey = new EncryptedKey();
            m_encryptedKey.LoadXml(value);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public struct X509IssuerSerial {

        private string issuerName;
        private string serialNumber;
        
        internal X509IssuerSerial (string issuerName, string serialNumber) {
            if (issuerName == null || issuerName.Length == 0)
                throw new ArgumentException(SecurityResources.GetResourceString("Arg_EmptyOrNullString"), "issuerName");
            if (serialNumber == null || serialNumber.Length == 0)
                throw new ArgumentException(SecurityResources.GetResourceString("Arg_EmptyOrNullString"), "serialNumber");
            this.issuerName = issuerName;
            this.serialNumber = serialNumber;
        }

        
        public string IssuerName {
            get {
                return issuerName;
            }
            set {
                issuerName = value;
            }
        }
        
        public string SerialNumber {
            get {
                return serialNumber;            
            }
            set {
                serialNumber = value;
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class KeyInfoX509Data : KeyInfoClause {
        // An array of certificates representing the certificate chain 
        private ArrayList m_certificates = null;
        // An array of issuer serial structs
        private ArrayList m_issuerSerials = null;
        // An array of SKIs
        private ArrayList m_subjectKeyIds = null;
        // An array of subject names
        private ArrayList m_subjectNames = null;
        // A raw byte data representing a certificate revocation list
        private byte[] m_CRL = null;

        //
        // public constructors
        //

        public KeyInfoX509Data () {}

        public KeyInfoX509Data (byte[] rgbCert) {
            X509Certificate2 certificate = new X509Certificate2(rgbCert);
            AddCertificate(certificate);
        }

        public KeyInfoX509Data (X509Certificate cert) {
            AddCertificate(cert);
        }

        [SecuritySafeCritical]
        public KeyInfoX509Data (X509Certificate cert, X509IncludeOption includeOption) {
            if (cert == null)
                throw new ArgumentNullException("cert");

            X509Certificate2 certificate = new X509Certificate2(cert);
            X509ChainElementCollection elements = null;
            X509Chain chain = null;
            switch (includeOption) {
            case X509IncludeOption.ExcludeRoot:
                // Build the certificate chain
                chain = new X509Chain();
                chain.Build(certificate);

                // Can't honor the option if we only have a partial chain.
                if ((chain.ChainStatus.Length > 0) && 
                    ((chain.ChainStatus[0].Status & X509ChainStatusFlags.PartialChain) == X509ChainStatusFlags.PartialChain))
                    throw new CryptographicException(CAPI.CERT_E_CHAINING);

                elements = (X509ChainElementCollection) chain.ChainElements;
                for (int index = 0; index < (X509Utils.IsSelfSigned(chain) ? 1 : elements.Count - 1); index++) {
                    AddCertificate(elements[index].Certificate);
                }
                break;
            case X509IncludeOption.EndCertOnly:
                AddCertificate(certificate);
                break;
            case X509IncludeOption.WholeChain:
                // Build the certificate chain
                chain = new X509Chain();
                chain.Build(certificate);

                // Can't honor the option if we only have a partial chain.
                if ((chain.ChainStatus.Length > 0) && 
                    ((chain.ChainStatus[0].Status & X509ChainStatusFlags.PartialChain) == X509ChainStatusFlags.PartialChain))
                    throw new CryptographicException(CAPI.CERT_E_CHAINING);

                elements = (X509ChainElementCollection) chain.ChainElements;
                foreach (X509ChainElement element in elements) {
                    AddCertificate(element.Certificate);
                }
                break;
            }
        }

        //
        // public properties
        //

        public ArrayList Certificates {
            get { return m_certificates; }
        }

        public void AddCertificate (X509Certificate certificate) {
            if (certificate == null)
                throw new ArgumentNullException("certificate");

            if (m_certificates == null)
                m_certificates = new ArrayList();

            X509Certificate2 x509 = new X509Certificate2(certificate);
            m_certificates.Add(x509);
        }

        public ArrayList SubjectKeyIds {
            get { return m_subjectKeyIds; }
        }

        public void AddSubjectKeyId(byte[] subjectKeyId) {
            if (m_subjectKeyIds == null)
                m_subjectKeyIds = new ArrayList();
            m_subjectKeyIds.Add(subjectKeyId);
        }

        [ComVisible(false)]
        public void AddSubjectKeyId(string subjectKeyId) {
            if (m_subjectKeyIds == null)
                m_subjectKeyIds = new ArrayList();
            m_subjectKeyIds.Add(X509Utils.DecodeHexString(subjectKeyId));
        }

        public ArrayList SubjectNames {
            get { return m_subjectNames; }
        }

        public void AddSubjectName(string subjectName) {
            if (m_subjectNames == null)
                m_subjectNames = new ArrayList();
            m_subjectNames.Add(subjectName);
        }

        public ArrayList IssuerSerials {
            get { return m_issuerSerials; }
        }

        public void AddIssuerSerial(string issuerName, string serialNumber) {
            BigInt h = new BigInt();
            h.FromHexadecimal(serialNumber);
            if (m_issuerSerials == null)
                m_issuerSerials = new ArrayList();
            m_issuerSerials.Add(new X509IssuerSerial(issuerName, h.ToDecimal()));
        }

        // When we load an X509Data from Xml, we know the serial number is in decimal representation.
        internal void InternalAddIssuerSerial(string issuerName, string serialNumber) {
            if (m_issuerSerials == null)
                m_issuerSerials = new ArrayList();
            m_issuerSerials.Add(new X509IssuerSerial(issuerName, serialNumber));
        }

        public byte[] CRL {
            get { return m_CRL; }
            set { m_CRL = value; }
        }

        //
        // private methods
        //

        private void Clear() {
            m_CRL = null;
            if (m_subjectKeyIds != null) m_subjectKeyIds.Clear();
            if (m_subjectNames != null) m_subjectNames.Clear();
            if (m_issuerSerials != null) m_issuerSerials.Clear();
            if (m_certificates != null) m_certificates.Clear();
        }

        //
        // public methods
        //

        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            XmlElement x509DataElement = xmlDocument.CreateElement("X509Data", SignedXml.XmlDsigNamespaceUrl);

            if (m_issuerSerials != null) {
                foreach(X509IssuerSerial issuerSerial in m_issuerSerials) {
                    XmlElement issuerSerialElement = xmlDocument.CreateElement("X509IssuerSerial", SignedXml.XmlDsigNamespaceUrl);
                    XmlElement issuerNameElement = xmlDocument.CreateElement("X509IssuerName", SignedXml.XmlDsigNamespaceUrl);
                    issuerNameElement.AppendChild(xmlDocument.CreateTextNode(issuerSerial.IssuerName));
                    issuerSerialElement.AppendChild(issuerNameElement);
                    XmlElement serialNumberElement = xmlDocument.CreateElement("X509SerialNumber", SignedXml.XmlDsigNamespaceUrl);
                    serialNumberElement.AppendChild(xmlDocument.CreateTextNode(issuerSerial.SerialNumber));
                    issuerSerialElement.AppendChild(serialNumberElement);
                    x509DataElement.AppendChild(issuerSerialElement);
                }
            }

            if (m_subjectKeyIds != null) {
                foreach(byte[] subjectKeyId in m_subjectKeyIds) {
                    XmlElement subjectKeyIdElement = xmlDocument.CreateElement("X509SKI", SignedXml.XmlDsigNamespaceUrl);
                    subjectKeyIdElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(subjectKeyId)));
                    x509DataElement.AppendChild(subjectKeyIdElement);
                }
            }

            if (m_subjectNames != null) {
                foreach(string subjectName in m_subjectNames) {
                    XmlElement subjectNameElement = xmlDocument.CreateElement("X509SubjectName", SignedXml.XmlDsigNamespaceUrl);
                    subjectNameElement.AppendChild(xmlDocument.CreateTextNode(subjectName));
                    x509DataElement.AppendChild(subjectNameElement);
                }
            }

            if (m_certificates != null) {
                foreach(X509Certificate certificate in m_certificates) {
                    XmlElement x509Element = xmlDocument.CreateElement("X509Certificate", SignedXml.XmlDsigNamespaceUrl);
                    x509Element.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(certificate.GetRawCertData())));
                    x509DataElement.AppendChild(x509Element);
                }
            }

            if (m_CRL != null) {
                XmlElement crlElement = xmlDocument.CreateElement("X509CRL", SignedXml.XmlDsigNamespaceUrl);
                crlElement.AppendChild(xmlDocument.CreateTextNode(Convert.ToBase64String(m_CRL)));
                x509DataElement.AppendChild(crlElement);
            }

            return x509DataElement;
        }

        public override void LoadXml(XmlElement element) {
            if (element == null)
                throw new ArgumentNullException("element");

            XmlNamespaceManager nsm = new XmlNamespaceManager(element.OwnerDocument.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            XmlNodeList x509IssuerSerialNodes = element.SelectNodes("ds:X509IssuerSerial", nsm);
            XmlNodeList x509SKINodes = element.SelectNodes("ds:X509SKI", nsm);
            XmlNodeList x509SubjectNameNodes = element.SelectNodes("ds:X509SubjectName", nsm);
            XmlNodeList x509CertificateNodes = element.SelectNodes("ds:X509Certificate", nsm);
            XmlNodeList x509CRLNodes = element.SelectNodes("ds:X509CRL", nsm);

            if ((x509CRLNodes.Count == 0 && x509IssuerSerialNodes.Count == 0 && x509SKINodes.Count == 0
                    && x509SubjectNameNodes.Count == 0 && x509CertificateNodes.Count == 0)) // Bad X509Data tag, or Empty tag
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "X509Data");

            // Flush anything in the lists
            Clear();

            if (x509CRLNodes.Count != 0)
                m_CRL = Convert.FromBase64String(Utils.DiscardWhiteSpaces(x509CRLNodes.Item(0).InnerText));

            foreach (XmlNode issuerSerialNode in x509IssuerSerialNodes) {
                XmlNode x509IssuerNameNode = issuerSerialNode.SelectSingleNode("ds:X509IssuerName", nsm);
                XmlNode x509SerialNumberNode = issuerSerialNode.SelectSingleNode("ds:X509SerialNumber", nsm);
                if (x509IssuerNameNode == null || x509SerialNumberNode == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "IssuerSerial");
                InternalAddIssuerSerial(x509IssuerNameNode.InnerText.Trim(), x509SerialNumberNode.InnerText.Trim());
            }

            foreach (XmlNode node in x509SKINodes) {
                AddSubjectKeyId(Convert.FromBase64String(Utils.DiscardWhiteSpaces(node.InnerText)));
            }

            foreach (XmlNode node in x509SubjectNameNodes) {
                AddSubjectName(node.InnerText.Trim());
            }

            foreach (XmlNode node in x509CertificateNodes) {
                AddCertificate(new X509Certificate2(Convert.FromBase64String(Utils.DiscardWhiteSpaces(node.InnerText))));
            }
        }
    }

    // This is for generic, unknown nodes
    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class KeyInfoNode : KeyInfoClause {
        private XmlElement m_node;

        //
        // public constructors
        //

        public KeyInfoNode() {}

        public KeyInfoNode(XmlElement node) {
            m_node = node;
        }

        //
        // public properties
        //

        public XmlElement Value {
            get { return m_node; }
            set { m_node = value; }
        }

        //
        // public methods
        //

        public override XmlElement GetXml() {
            XmlDocument xmlDocument = new XmlDocument();
            xmlDocument.PreserveWhitespace = true;
            return GetXml(xmlDocument);
        }

        internal override XmlElement GetXml (XmlDocument xmlDocument) {
            return xmlDocument.ImportNode(m_node, true) as XmlElement;
        }

        public override void LoadXml(XmlElement value) {
            m_node = value; 
        }
    }
}
