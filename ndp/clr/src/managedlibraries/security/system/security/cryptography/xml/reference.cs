// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// Reference.cs
// 
// 21 Microsoft 2000
//

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.IO;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Xml;
    using System.Globalization;
    using System.Runtime.Versioning;

    [Serializable]
    internal enum ReferenceTargetType {
        Stream,
        XmlElement,
        UriReference
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class Reference {
        private string m_id;
        private string m_uri;
        private string m_type;
        private TransformChain m_transformChain;
        private string m_digestMethod;
        private byte[] m_digestValue;
        private HashAlgorithm m_hashAlgorithm;
        private Object m_refTarget;
        private ReferenceTargetType m_refTargetType;
        private XmlElement m_cachedXml;
        private SignedXml m_signedXml = null;
        internal CanonicalXmlNodeList m_namespaces = null;

        //
        // public constructors
        //

        public Reference () {
            m_transformChain = new TransformChain();
            m_refTarget = null;
            m_refTargetType = ReferenceTargetType.UriReference;
            m_cachedXml = null;
            m_digestMethod = SignedXml.XmlDsigDigestDefault;
        }

        public Reference (Stream stream) {
            m_transformChain = new TransformChain();
            m_refTarget = stream;
            m_refTargetType = ReferenceTargetType.Stream;
            m_cachedXml = null;
            m_digestMethod = SignedXml.XmlDsigDigestDefault;
        }

        public Reference (string uri) {
            m_transformChain = new TransformChain();
            m_refTarget = uri;
            m_uri = uri;
            m_refTargetType = ReferenceTargetType.UriReference;
            m_cachedXml = null;
            m_digestMethod = SignedXml.XmlDsigDigestDefault;
        }

        internal Reference (XmlElement element) {
            m_transformChain = new TransformChain();
            m_refTarget = element;
            m_refTargetType = ReferenceTargetType.XmlElement;
            m_cachedXml = null;
            m_digestMethod = SignedXml.XmlDsigDigestDefault;
        }

        //
        // public properties
        //

        public string Id {
            get { return m_id; }
            set { m_id = value; }
        }

        public string Uri {
            get { return m_uri; }
            set {
                m_uri = value;
                m_cachedXml = null;
            }
        }

        public string Type {
            get { return m_type; }
            set {
                m_type = value;
                m_cachedXml = null;
            }
        }

        public string DigestMethod {
            get { return m_digestMethod; }
            set {
                m_digestMethod = value;
                m_cachedXml = null;
            }
        }

        public byte[] DigestValue {
            get { return m_digestValue; }
            set {
                m_digestValue = value;
                m_cachedXml = null;
            }
        }

        public TransformChain TransformChain {
            get {
                if (m_transformChain == null)
                    m_transformChain = new TransformChain();
                return m_transformChain; 
            }
            [ComVisible(false)]
            set {
                m_transformChain = value;
                m_cachedXml = null;
            }
        }

        internal bool CacheValid {
            get {
                return (m_cachedXml != null);
            }
        }

        internal SignedXml SignedXml {
            get { return m_signedXml; }
            set { m_signedXml = value; }
        }

        internal ReferenceTargetType ReferenceTargetType {
            get {
                return m_refTargetType;
            }
        }

        //
        // public methods
        //

        public XmlElement GetXml() {
            if (CacheValid) return(m_cachedXml);

            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            return GetXml(document);
        }

        internal XmlElement GetXml (XmlDocument document) {
            // Create the Reference
            XmlElement referenceElement = document.CreateElement("Reference", SignedXml.XmlDsigNamespaceUrl);

            if (!String.IsNullOrEmpty(m_id))
                referenceElement.SetAttribute("Id", m_id);

            if (m_uri != null)
                referenceElement.SetAttribute("URI", m_uri);

            if (!String.IsNullOrEmpty(m_type))
                referenceElement.SetAttribute("Type", m_type);

            // Add the transforms to the Reference
            if (this.TransformChain.Count != 0)
                referenceElement.AppendChild(this.TransformChain.GetXml(document, SignedXml.XmlDsigNamespaceUrl));

            // Add the DigestMethod
            if (String.IsNullOrEmpty(m_digestMethod))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_DigestMethodRequired"));

            XmlElement digestMethodElement = document.CreateElement("DigestMethod", SignedXml.XmlDsigNamespaceUrl);
            digestMethodElement.SetAttribute("Algorithm",m_digestMethod);
            referenceElement.AppendChild(digestMethodElement);

            if (DigestValue == null) {
                if (m_hashAlgorithm.Hash == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_DigestValueRequired"));
                DigestValue = m_hashAlgorithm.Hash;
            }

            XmlElement digestValueElement = document.CreateElement("DigestValue", SignedXml.XmlDsigNamespaceUrl);
            digestValueElement.AppendChild(document.CreateTextNode(Convert.ToBase64String(m_digestValue)));
            referenceElement.AppendChild(digestValueElement);

            return referenceElement;
        }

        public void LoadXml(XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            m_id = Utils.GetAttribute(value, "Id", SignedXml.XmlDsigNamespaceUrl);
            m_uri = Utils.GetAttribute(value, "URI", SignedXml.XmlDsigNamespaceUrl);
            m_type = Utils.GetAttribute(value, "Type", SignedXml.XmlDsigNamespaceUrl);
            if (!Utils.VerifyAttributes(value, new string[] { "Id", "URI", "Type" }))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            // Transforms
            bool hasTransforms = false;
            this.TransformChain = new TransformChain();
            XmlNodeList transformsNodes = value.SelectNodes("ds:Transforms", nsm);
            if (transformsNodes != null && transformsNodes.Count != 0) {
                if (!Utils.GetAllowAdditionalSignatureNodes() && transformsNodes.Count > 1) { 
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/Transforms");
                }
                hasTransforms = true;
                XmlElement transformsElement = transformsNodes[0] as XmlElement;
                if (!Utils.VerifyAttributes(transformsElement, (string[])null)) {
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/Transforms");
                }

                XmlNodeList transformNodes = transformsElement.SelectNodes("ds:Transform", nsm);
                if (transformNodes != null) {
                    if (!Utils.GetAllowAdditionalSignatureNodes() && (transformNodes.Count != transformsElement.SelectNodes("*").Count)) { 
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/Transforms");
                    }
                    if (transformNodes.Count > Utils.GetMaxTransformsPerReference()) { 
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/Transforms");
                    }
                    foreach (XmlNode transformNode in transformNodes) {
                        XmlElement transformElement = transformNode as XmlElement;
                        string algorithm = Utils.GetAttribute(transformElement, "Algorithm", SignedXml.XmlDsigNamespaceUrl);
                        if ((algorithm == null && !Utils.GetSkipSignatureAttributeEnforcement()) || !Utils.VerifyAttributes(transformElement, "Algorithm")) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                        }
                        Transform transform = Utils.CreateFromName<Transform>(algorithm);
                        if (transform == null) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                        }
                        AddTransform(transform);
                        // let the transform read the children of the transformElement for data
                        transform.LoadInnerXml(transformElement.ChildNodes);
                        // Hack! this is done to get around the lack of here() function support in XPath
                        if (transform is XmlDsigEnvelopedSignatureTransform) {
                            // Walk back to the Signature tag. Find the nearest signature ancestor
                            // Signature-->SignedInfo-->Reference-->Transforms-->Transform
                            XmlNode signatureTag = transformElement.SelectSingleNode("ancestor::ds:Signature[1]", nsm);
                            XmlNodeList signatureList = transformElement.SelectNodes("//ds:Signature", nsm);
                            if (signatureList != null) {
                                int position = 0;
                                foreach(XmlNode node in signatureList) {
                                    position++;
                                    if (node == signatureTag) {
                                        ((XmlDsigEnvelopedSignatureTransform)transform).SignaturePosition = position; 
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // DigestMethod
            XmlNodeList digestMethodNodes = value.SelectNodes("ds:DigestMethod", nsm);
            if (digestMethodNodes == null || digestMethodNodes.Count == 0 || (!Utils.GetAllowAdditionalSignatureNodes() && digestMethodNodes.Count > 1))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/DigestMethod");
            XmlElement digestMethodElement = digestMethodNodes[0] as XmlElement;
            m_digestMethod = Utils.GetAttribute(digestMethodElement, "Algorithm", SignedXml.XmlDsigNamespaceUrl);
            if ((m_digestMethod == null && !Utils.GetSkipSignatureAttributeEnforcement()) || !Utils.VerifyAttributes(digestMethodElement, "Algorithm"))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/DigestMethod");
 
            // DigestValue
            XmlNodeList digestValueNodes = value.SelectNodes("ds:DigestValue", nsm);
            if (digestValueNodes == null || digestValueNodes.Count == 0 || (!Utils.GetAllowAdditionalSignatureNodes() && digestValueNodes.Count > 1))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/DigestValue");
            XmlElement digestValueElement = digestValueNodes[0] as XmlElement;
            m_digestValue = Convert.FromBase64String(Utils.DiscardWhiteSpaces(digestValueElement.InnerText));
            if (!Utils.VerifyAttributes(digestValueElement, (string[])null))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference/DigestValue");

            // Verify that there aren't any extra nodes that aren't allowed
            int expectedChildNodeCount = hasTransforms ? 3 : 2;
            if (!Utils.GetAllowAdditionalSignatureNodes() && (value.SelectNodes("*").Count != expectedChildNodeCount))
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Reference");

            // cache the Xml
            m_cachedXml = value;
        }

        public void AddTransform(Transform transform) {
            if (transform == null)
                throw new ArgumentNullException("transform");

            transform.Reference = this;
            this.TransformChain.Add(transform);
        }

        internal void UpdateHashValue(XmlDocument document, CanonicalXmlNodeList refList) {
            DigestValue = CalculateHashValue(document, refList);
        }

        // What we want to do is pump the input throug the TransformChain and then 
        // hash the output of the chain document is the document context for resolving relative references
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        internal byte[] CalculateHashValue(XmlDocument document, CanonicalXmlNodeList refList) {
            // refList is a list of elements that might be targets of references
            // Now's the time to create our hashing algorithm
            m_hashAlgorithm = Utils.CreateFromName<HashAlgorithm>(m_digestMethod);
            if (m_hashAlgorithm == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_CreateHashAlgorithmFailed"));

            // Let's go get the target.
            string baseUri = (document == null ? System.Environment.CurrentDirectory + "\\" : document.BaseURI);
            Stream hashInputStream = null;
            WebRequest request = null;
            WebResponse response = null;
            Stream inputStream = null;
            XmlResolver resolver = null;
            byte[] hashval = null;

            try {
                switch (m_refTargetType) {
                case ReferenceTargetType.Stream:
                    // This is the easiest case. We already have a stream, so just pump it through the TransformChain
                    resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                    hashInputStream = this.TransformChain.TransformToOctetStream((Stream) m_refTarget, resolver, baseUri);
                    break;
                case ReferenceTargetType.UriReference:
                    // Second-easiest case -- dereference the URI & pump through the TransformChain
                    // handle the special cases where the URI is null (meaning whole doc)
                    // or the URI is just a fragment (meaning a reference to an embedded Object)
                    if (m_uri == null) {
                        // We need to create a DocumentNavigator out of the XmlElement
                        resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                        // In the case of a Uri-less reference, we will simply pass null to the transform chain.
                        // The first transform in the chain is expected to know how to retrieve the data to hash.
                        hashInputStream = this.TransformChain.TransformToOctetStream((Stream) null, resolver, baseUri);
                    } else if (m_uri.Length == 0) {
                        // This is the self-referential case. First, check that we have a document context.
                        // The Enveloped Signature does not discard comments as per spec; those will be omitted during the transform chain process
                       if (document == null) 
                            throw new CryptographicException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Cryptography_Xml_SelfReferenceRequiresContext"), m_uri));

                        // Normalize the containing document
                        resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                        XmlDocument docWithNoComments = Utils.DiscardComments(Utils.PreProcessDocumentInput(document, resolver, baseUri));
                        hashInputStream = this.TransformChain.TransformToOctetStream(docWithNoComments, resolver, baseUri);
                    } else if (m_uri[0] == '#') {
                        // If we get here, then we are constructing a Reference to an embedded DataObject
                        // referenced by an Id = attribute. Go find the relevant object
                        bool discardComments = true;
                        string idref = Utils.GetIdFromLocalUri(m_uri, out discardComments);
                        if (idref == "xpointer(/)") {
                            // This is a self referencial case
                            if (document == null) 
                                throw new CryptographicException(String.Format(CultureInfo.CurrentCulture, SecurityResources.GetResourceString("Cryptography_Xml_SelfReferenceRequiresContext"),m_uri));

                            // We should not discard comments here!!!
                            resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                            hashInputStream = this.TransformChain.TransformToOctetStream(Utils.PreProcessDocumentInput(document, resolver, baseUri), resolver, baseUri);
                            break;
                        }

                        XmlElement elem = this.SignedXml.GetIdElement(document, idref);
                        if (elem != null)
                            m_namespaces = Utils.GetPropagatedAttributes(elem.ParentNode as XmlElement);

                        if (elem == null) {
                            // Go throw the referenced items passed in
                            if (refList != null) {
                                foreach (XmlNode node in refList) {
                                    XmlElement tempElem = node as XmlElement;
                                    if ((tempElem != null) && (Utils.HasAttribute(tempElem, "Id", SignedXml.XmlDsigNamespaceUrl)) 
                                        && (Utils.GetAttribute(tempElem, "Id", SignedXml.XmlDsigNamespaceUrl).Equals(idref))) {
                                        elem = tempElem;
                                        if (this.m_signedXml.m_context != null)
                                            m_namespaces = Utils.GetPropagatedAttributes(this.m_signedXml.m_context);
                                        break;
                                    }
                                }
                            }
                        }

                        if (elem == null)
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidReference"));

                        XmlDocument normDocument = Utils.PreProcessElementInput(elem, resolver, baseUri);
                        // Add the propagated attributes
                        Utils.AddNamespaces(normDocument.DocumentElement, m_namespaces);

                        resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                        if (discardComments) {
                            // We should discard comments before going into the transform chain
                            XmlDocument docWithNoComments = Utils.DiscardComments(normDocument);
                            hashInputStream = this.TransformChain.TransformToOctetStream(docWithNoComments, resolver, baseUri);
                        } else {
                            // This is an XPointer reference, do not discard comments!!!
                            hashInputStream = this.TransformChain.TransformToOctetStream(normDocument, resolver, baseUri);
                        }
                    } else if (Utils.AllowDetachedSignature()) {
                        // WebRequest always expects an Absolute Uri, so try to resolve if we were passed a relative Uri.
                        System.Uri uri = new System.Uri(m_uri, UriKind.RelativeOrAbsolute);
                        if (!uri.IsAbsoluteUri) {
                            uri = new Uri(new Uri(baseUri), uri);
                        }
                        request = WebRequest.Create(uri);
                        if (request == null) goto default;
                        response = request.GetResponse();
                        if (response == null) goto default;
                        inputStream = response.GetResponseStream();
                        if (inputStream == null) goto default;
                        resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                        hashInputStream = this.TransformChain.TransformToOctetStream(inputStream, resolver, m_uri);
                    }
                    else {
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UriNotResolved"), m_uri);
                    }
                    break;
                case ReferenceTargetType.XmlElement:
                    // We need to create a DocumentNavigator out of the XmlElement
                    resolver = (this.SignedXml.ResolverSet ? this.SignedXml.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), baseUri));
                    hashInputStream = this.TransformChain.TransformToOctetStream(Utils.PreProcessElementInput((XmlElement) m_refTarget, resolver, baseUri), resolver, baseUri);
                    break;
                default:
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UriNotResolved"), m_uri);
                }

                // Compute the new hash value
                hashInputStream = SignedXmlDebugLog.LogReferenceData(this, hashInputStream);
                hashval = m_hashAlgorithm.ComputeHash(hashInputStream);
            }
            finally {
                if (hashInputStream != null)
                    hashInputStream.Close();
                if (response != null)
                    response.Close();
                if (inputStream != null)
                    inputStream.Close();
            }

            return hashval;
        }
    }
}
