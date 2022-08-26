// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// Transform.cs
//

// This file contains the classes necessary to represent the Transform processing model used in 
// XMLDSIG. The basic idea is as follows. A Reference object contains within it a TransformChain, which
// is an ordered set of XMLDSIG transforms (represented by <Transform>...</Transform> clauses in the XML).
// A transform in XMLDSIG operates on an input of either an octet stream or a node set and produces
// either an octet stream or a node set. Conversion between the two types is performed by parsing (octet stream->
// node set) or C14N (node set->octet stream). We generalize this slightly to allow a transform to define an array of
// input and output types (because I believe in the future there will be perf gains by being smarter about what goes in & comes out)
// Each XMLDSIG transform is represented by a subclass of the abstract Transform class. We need to use CryptoConfig to
// associate Transform classes with URLs for transform extensibility, but that's a future concern for this code.
// Once the Transform chain is constructed, call TransformToOctetStream to convert some sort of input type to an octet
// stream. (We only bother implementing that much now since every use of transform chains in XmlDsig ultimately yields something to hash).

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.Collections;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Policy;
    using System.Text;
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl;

    // This class represents an ordered chain of transforms

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class TransformChain {
        private ArrayList m_transforms;

        public TransformChain () {
            m_transforms = new ArrayList();
        }

        public void Add (Transform transform) {
            if (transform != null) 
                m_transforms.Add(transform);
        }

        public IEnumerator GetEnumerator() {
            return m_transforms.GetEnumerator();
        }

        public int Count {
            get { return m_transforms.Count; }
        }

        public Transform this[int index] {
            get {
                if (index >= m_transforms.Count)
                    throw new ArgumentException( SecurityResources.GetResourceString("ArgumentOutOfRange_Index"), "index");
                return (Transform) m_transforms[index];
            }
        }

        // The goal behind this method is to pump the input stream through the transforms and get back something that
        // can be hashed
        internal Stream TransformToOctetStream(Object inputObject, Type inputType, XmlResolver resolver, string baseUri) {
            Object currentInput = inputObject;
            foreach (Transform transform in m_transforms) {
                if (currentInput == null || transform.AcceptsType(currentInput.GetType())) {
                    //in this case, no translation necessary, pump it through
                    transform.Resolver = resolver;
                    transform.BaseURI = baseUri;
                    transform.LoadInput(currentInput);
                    currentInput = transform.GetOutput();
                } else {
                    // We need translation 
                    // For now, we just know about Stream->{XmlNodeList,XmlDocument} and {XmlNodeList,XmlDocument}->Stream
                    if (currentInput is Stream) {
                        if (transform.AcceptsType(typeof(XmlDocument))) {
                            Stream currentInputStream = currentInput as Stream;
                            XmlDocument doc = new XmlDocument();
                            doc.PreserveWhitespace = true;
                            XmlReader valReader = Utils.PreProcessStreamInput(currentInputStream, resolver, baseUri);
                            doc.Load(valReader);
                            transform.LoadInput(doc);
                            currentInputStream.Close();
                            currentInput = transform.GetOutput();
                            continue;
                        } else {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                        }
                    } 
                    if (currentInput is XmlNodeList) {
                        if (transform.AcceptsType(typeof(Stream))) {
                            CanonicalXml c14n = new CanonicalXml((XmlNodeList) currentInput, resolver, false);
                            MemoryStream ms = new MemoryStream(c14n.GetBytes());
                            transform.LoadInput(ms);
                            currentInput = transform.GetOutput();
                            ms.Close();
                            continue;
                        } else {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                        }
                    }
                    if (currentInput is XmlDocument) {
                        if (transform.AcceptsType(typeof(Stream))) {
                            CanonicalXml c14n = new CanonicalXml((XmlDocument) currentInput, resolver);
                            MemoryStream ms = new MemoryStream(c14n.GetBytes());
                            transform.LoadInput(ms);
                            currentInput = transform.GetOutput();
                            ms.Close();
                            continue;
                        } else {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                        }
                    }
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
                }
            }

            // Final processing, either we already have a stream or have to canonicalize
            if (currentInput is Stream) {
                return currentInput as Stream;
            }
            if (currentInput is XmlNodeList) {
                CanonicalXml c14n = new CanonicalXml((XmlNodeList) currentInput, resolver, false);
                MemoryStream ms = new MemoryStream(c14n.GetBytes());
                return ms;
            }
            if (currentInput is XmlDocument) {
                CanonicalXml c14n = new CanonicalXml((XmlDocument) currentInput, resolver);
                MemoryStream ms = new MemoryStream(c14n.GetBytes());
                return ms;
            }
            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"));
        }

        internal Stream TransformToOctetStream(Stream input, XmlResolver resolver, string baseUri) {
            return TransformToOctetStream(input, typeof(Stream), resolver, baseUri);
        }

        internal Stream TransformToOctetStream(XmlDocument document, XmlResolver resolver, string baseUri) {
            return TransformToOctetStream(document, typeof(XmlDocument), resolver, baseUri);
        }

        internal XmlElement GetXml (XmlDocument document, string ns) {
            XmlElement transformsElement = document.CreateElement("Transforms", ns);
            foreach (Transform transform in m_transforms) {
                if (transform != null) {
                    // Construct the individual transform element
                    XmlElement transformElement = transform.GetXml(document);
                    if (transformElement != null)
                        transformsElement.AppendChild(transformElement);
                }
            }
            return transformsElement;
        }

        internal void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            XmlNamespaceManager nsm = new XmlNamespaceManager(value.OwnerDocument.NameTable);
            nsm.AddNamespace("ds", SignedXml.XmlDsigNamespaceUrl);

            XmlNodeList transformNodes = value.SelectNodes("ds:Transform", nsm);
            if (transformNodes.Count == 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_InvalidElement"), "Transforms");

            m_transforms.Clear();
            for (int i = 0; i < transformNodes.Count; ++i) {
                XmlElement transformElement = (XmlElement) transformNodes.Item(i);
                string algorithm = Utils.GetAttribute(transformElement, "Algorithm", SignedXml.XmlDsigNamespaceUrl);
                Transform transform = Utils.CreateFromName<Transform>(algorithm);
                if (transform == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                // let the transform read the children of the transformElement for data
                transform.LoadInnerXml(transformElement.ChildNodes);
                m_transforms.Add(transform);
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public abstract class Transform {
        private string m_algorithm;
        private string m_baseUri = null;
        internal XmlResolver m_xmlResolver = null;
        private bool m_bResolverSet = false;
        private SignedXml m_signedXml = null;
        private Reference m_reference = null;
        private Hashtable m_propagatedNamespaces = null;
        private XmlElement m_context = null;

        internal string BaseURI {
            get { return m_baseUri; }
            set { m_baseUri = value; }
        }

        internal SignedXml SignedXml {
            get { return m_signedXml; }
            set { m_signedXml = value; }
        }

        internal Reference Reference {
            get { return m_reference; }
            set { m_reference = value; }
        }

        //
        // protected constructors
        //

        protected Transform() {}

        //
        // public properties
        //

        public string Algorithm {
            get { return m_algorithm; }
            set { m_algorithm = value; }
        }

        [ComVisible(false)]
        public XmlResolver Resolver {
            // This property only has a setter. The rationale for this is that we don't have a good value
            // to return when it has not been explicitely set, as we are using XmlSecureResolver by default
            set { 
                m_xmlResolver = value; 
                m_bResolverSet = true;
            }

            internal get {
                return m_xmlResolver;
            }
        }

        internal bool ResolverSet {
            get { return m_bResolverSet; }
        }

        public abstract Type[] InputTypes {
            get;
        }

        public abstract Type[] OutputTypes {
            get;
        }

        internal bool AcceptsType(Type inputType) {
            if (InputTypes != null) {
                for (int i=0; i<InputTypes.Length; i++) {
                    if (inputType == InputTypes[i] || inputType.IsSubclassOf(InputTypes[i]))
                        return true;
                }
            }
            return false;
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
            return GetXml (document, "Transform");
        }

        internal XmlElement GetXml (XmlDocument document, string name) {
            XmlElement transformElement = document.CreateElement(name, SignedXml.XmlDsigNamespaceUrl);
            if (!String.IsNullOrEmpty(this.Algorithm))
                transformElement.SetAttribute("Algorithm", this.Algorithm);
            XmlNodeList children = this.GetInnerXml();
            if (children != null) {
                foreach (XmlNode node in children) {
                    transformElement.AppendChild(document.ImportNode(node, true));
                }
            }
            return transformElement;
        }

        public abstract void LoadInnerXml(XmlNodeList nodeList);

        protected abstract XmlNodeList GetInnerXml();

        public abstract void LoadInput(Object obj);

        public abstract Object GetOutput();

        public abstract Object GetOutput(Type type);

        [ComVisible(false)]
        public virtual byte[] GetDigestedOutput(HashAlgorithm hash) {
            return hash.ComputeHash((Stream) GetOutput(typeof(Stream)));
        }

        [ComVisible(false)]
        public XmlElement Context {
            get {
                if (m_context != null)
                    return m_context;

                Reference reference = this.Reference;
                SignedXml signedXml = (reference == null ? this.SignedXml : reference.SignedXml);
                if (signedXml == null)
                    return null;

                return signedXml.m_context;
            }
            set {
                m_context = value;
            }
        }

        [ComVisible(false)]
        public Hashtable PropagatedNamespaces {
            get {
                if (m_propagatedNamespaces != null)
                    return m_propagatedNamespaces;

                Reference reference = this.Reference;
                SignedXml signedXml = (reference == null ? this.SignedXml : reference.SignedXml);

                // If the reference is not a Uri reference with a DataObject target, return an empty hashtable.
                if (reference != null && 
                    ((reference.ReferenceTargetType != ReferenceTargetType.UriReference) ||
                     (reference.Uri == null || reference.Uri.Length == 0 || reference.Uri[0] != '#'))) {
                    m_propagatedNamespaces = new Hashtable(0);
                    return m_propagatedNamespaces;
                }

                CanonicalXmlNodeList namespaces = null;
                if (reference != null)
                    namespaces = reference.m_namespaces;
                else if (signedXml.m_context != null)
                    namespaces = Utils.GetPropagatedAttributes(signedXml.m_context);

                // if no namespaces have been propagated, return an empty hashtable.
                if (namespaces == null) {
                    m_propagatedNamespaces = new Hashtable(0);
                    return m_propagatedNamespaces;
                }

                m_propagatedNamespaces = new Hashtable(namespaces.Count);
                foreach (XmlNode attrib in namespaces) {
                    string key = ((attrib.Prefix.Length > 0) ? attrib.Prefix + ":" + attrib.LocalName : attrib.LocalName);
                    if (!m_propagatedNamespaces.Contains(key))
                        m_propagatedNamespaces.Add(key, attrib.Value);
                }
                return m_propagatedNamespaces;
            }
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigC14NTransform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlDocument), typeof(XmlNodeList) };
        private Type[] _outputTypes = { typeof(Stream) };
        private CanonicalXml _cXml;
        private bool _includeComments = false;

        public XmlDsigC14NTransform() {
            Algorithm = SignedXml.XmlDsigC14NTransformUrl;
        }

        public XmlDsigC14NTransform(bool includeComments) {
            _includeComments = includeComments;
            Algorithm = (includeComments ? SignedXml.XmlDsigC14NWithCommentsTransformUrl : SignedXml.XmlDsigC14NTransformUrl);
        }

        public override Type[] InputTypes {
            get { return _inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return _outputTypes; }
        }

        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (!Utils.GetAllowAdditionalSignatureNodes() && nodeList != null && nodeList.Count > 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
        }

        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        public override void LoadInput(Object obj) {
            XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
            if (obj is Stream) {
                _cXml = new CanonicalXml((Stream) obj, _includeComments, resolver, this.BaseURI);
                return;
            }
            if (obj is XmlDocument) {
                _cXml = new CanonicalXml((XmlDocument) obj, resolver, _includeComments);
                return;
            }
            if (obj is XmlNodeList) {
                _cXml = new CanonicalXml((XmlNodeList) obj, resolver, _includeComments);
            } 
	     else {
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "obj");            
	     }
        }

        public override Object GetOutput() {
            return new MemoryStream(_cXml.GetBytes());
        }

        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream))) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
            return new MemoryStream(_cXml.GetBytes());
        }

        [ComVisible(false)]
        public override byte[] GetDigestedOutput(HashAlgorithm hash) {
            return _cXml.GetDigestedBytes(hash);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigC14NWithCommentsTransform : XmlDsigC14NTransform {
        public XmlDsigC14NWithCommentsTransform() 
            : base(true) {
            Algorithm = SignedXml.XmlDsigC14NWithCommentsTransformUrl;
        }
    }

    // <ds:Transform Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#">
    //     <ec:InclusiveNamespaces PrefixList="dsig soap #default" xmlns:ec="http://www.w3.org/2001/10/xml-exc-c14n#"/>
    // </ds:Transform>

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigExcC14NTransform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlDocument), typeof(XmlNodeList) };
        private Type[] _outputTypes = { typeof(Stream) };
        private bool _includeComments = false;
        private string _inclusiveNamespacesPrefixList;
        private ExcCanonicalXml _excCanonicalXml;

        public XmlDsigExcC14NTransform() : this(false, null) {}

        public XmlDsigExcC14NTransform(bool includeComments) : this(includeComments, null) {}

        public XmlDsigExcC14NTransform(string inclusiveNamespacesPrefixList) : this(false, inclusiveNamespacesPrefixList) {}

        public XmlDsigExcC14NTransform(bool includeComments, string inclusiveNamespacesPrefixList) {
            _includeComments = includeComments;
            _inclusiveNamespacesPrefixList = inclusiveNamespacesPrefixList;
            Algorithm = (includeComments ? SignedXml.XmlDsigExcC14NWithCommentsTransformUrl : SignedXml.XmlDsigExcC14NTransformUrl);
        }

        public string InclusiveNamespacesPrefixList {
            get { return _inclusiveNamespacesPrefixList; }
            set { _inclusiveNamespacesPrefixList = value; }
        }

        public override Type[] InputTypes {
            get { return _inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return _outputTypes; }
        }

        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (nodeList != null) {
                foreach (XmlNode n in nodeList) {
                    XmlElement e = n as XmlElement;
                    if (e != null) {
                        if (e.LocalName.Equals("InclusiveNamespaces") 
                        && e.NamespaceURI.Equals(SignedXml.XmlDsigExcC14NTransformUrl) &&
                        Utils.HasAttribute(e, "PrefixList", SignedXml.XmlDsigNamespaceUrl)) {
                            if (!Utils.VerifyAttributes(e, "PrefixList")) {
                                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                            }
                            this.InclusiveNamespacesPrefixList = Utils.GetAttribute(e, "PrefixList", SignedXml.XmlDsigNamespaceUrl);
                            return;
                        }
                        else if (!Utils.GetAllowAdditionalSignatureNodes()) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                        }
                    }
                }
            }
        }

        public override void LoadInput(Object obj) {
            XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
            if (obj is Stream) {
                _excCanonicalXml = new ExcCanonicalXml((Stream) obj, _includeComments, _inclusiveNamespacesPrefixList, resolver, this.BaseURI);
            }
            else if (obj is XmlDocument) {
                _excCanonicalXml = new ExcCanonicalXml((XmlDocument) obj, _includeComments, _inclusiveNamespacesPrefixList, resolver);
            }
            else if (obj is XmlNodeList) {
                _excCanonicalXml = new ExcCanonicalXml((XmlNodeList) obj, _includeComments, _inclusiveNamespacesPrefixList, resolver);
            } else
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "obj");
        }

        protected override XmlNodeList GetInnerXml() {
            if (InclusiveNamespacesPrefixList == null)
                return null;
            XmlDocument document = new XmlDocument();
            XmlElement element = document.CreateElement("Transform", SignedXml.XmlDsigNamespaceUrl);
            if (!String.IsNullOrEmpty(this.Algorithm))
                element.SetAttribute("Algorithm", this.Algorithm);
            XmlElement prefixListElement = document.CreateElement("InclusiveNamespaces", SignedXml.XmlDsigExcC14NTransformUrl);
            prefixListElement.SetAttribute("PrefixList", InclusiveNamespacesPrefixList);
            element.AppendChild(prefixListElement);
            return element.ChildNodes;
        }

        public override Object GetOutput() {
            return new MemoryStream(_excCanonicalXml.GetBytes());
        }

        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
            return new MemoryStream(_excCanonicalXml.GetBytes());
        }

        public override byte[] GetDigestedOutput(HashAlgorithm hash) {
            return _excCanonicalXml.GetDigestedBytes(hash);
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigExcC14NWithCommentsTransform : XmlDsigExcC14NTransform {
        public XmlDsigExcC14NWithCommentsTransform() : base(true) {
            Algorithm = SignedXml.XmlDsigExcC14NWithCommentsTransformUrl;
        }

        public XmlDsigExcC14NWithCommentsTransform(string inclusiveNamespacesPrefixList) : base(true, inclusiveNamespacesPrefixList) {
            Algorithm = SignedXml.XmlDsigExcC14NWithCommentsTransformUrl;
        }
    }

    // A class representing conversion from Base64 using CryptoStream
    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigBase64Transform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlNodeList), typeof(XmlDocument) };
        private Type[] _outputTypes = { typeof(Stream) };
        private CryptoStream _cs = null;

        public XmlDsigBase64Transform() {
            Algorithm = SignedXml.XmlDsigBase64TransformUrl;
        }

        public override Type[] InputTypes {
            get { return _inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return _outputTypes; }
        }

        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (!Utils.GetAllowAdditionalSignatureNodes() && nodeList != null && nodeList.Count > 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
        }

        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
                return;
            }
            if (obj is XmlNodeList) {
                LoadXmlNodeListInput((XmlNodeList) obj);
                return;
            }
            if (obj is XmlDocument) {
                LoadXmlNodeListInput(((XmlDocument) obj).SelectNodes("//."));
                return;
            }
        }

        private void LoadStreamInput(Stream inputStream) {
            if (inputStream == null) throw new ArgumentException("obj");
            MemoryStream ms = new MemoryStream();
            byte[] buffer = new byte[1024];
            int bytesRead;
            do {
                bytesRead = inputStream.Read(buffer,0,1024);
                if (bytesRead > 0) {
                    int i = 0;
                    int j = 0;
                    while ((j < bytesRead) && (!Char.IsWhiteSpace((char) buffer[j]))) j++;
                    i = j; j++;
                    while (j < bytesRead) {
                        if (!Char.IsWhiteSpace((char) buffer[j])) {
                            buffer[i] = buffer[j];
                            i++;
                        }
                        j++;
                    }
                    ms.Write(buffer,0,i);
                }
            } while (bytesRead > 0);
            ms.Position = 0;
            _cs = new CryptoStream(ms, new FromBase64Transform(), CryptoStreamMode.Read);
        }

        private void LoadXmlNodeListInput(XmlNodeList nodeList) {
            StringBuilder sb = new StringBuilder();
            foreach (XmlNode node in nodeList) {
                XmlNode result = node.SelectSingleNode("self::text()");
                if (result != null)
                    sb.Append(result.OuterXml);
            }
            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] buffer = utf8.GetBytes(sb.ToString());
            int i = 0;
            int j = 0;
            while ((j <buffer.Length) && (!Char.IsWhiteSpace((char) buffer[j]))) j++;
            i = j; j++;
            while (j < buffer.Length) {
                if (!Char.IsWhiteSpace((char) buffer[j])) {
                    buffer[i] = buffer[j];
                    i++;
                }
                j++;
            }
            MemoryStream ms = new MemoryStream(buffer, 0, i);
            _cs = new CryptoStream(ms, new FromBase64Transform(), CryptoStreamMode.Read);
        }

        public override Object GetOutput() {
            return _cs;
        }

        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
            return _cs;
        }
    }

    // A class representing DSIG XPath Transforms

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigXPathTransform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlNodeList), typeof(XmlDocument) };
        private Type[] _outputTypes = { typeof(XmlNodeList) };
        private string _xpathexpr;
        private XmlDocument _document;
        private XmlNamespaceManager _nsm;

        public XmlDsigXPathTransform() {
            Algorithm = SignedXml.XmlDsigXPathTransformUrl;
        }

        public override Type[] InputTypes {
            get { return _inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return _outputTypes; }
        }

        public override void LoadInnerXml(XmlNodeList nodeList) {
            // XPath transform is specified by text child of first XPath child
            if (nodeList == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));

            foreach (XmlNode node in nodeList) {
                string prefix = null;
                string namespaceURI = null;
                XmlElement elem = node as XmlElement;
                if (elem != null) {
                    if (elem.LocalName == "XPath") {
                        _xpathexpr = elem.InnerXml.Trim(null);
                        XmlNodeReader nr = new XmlNodeReader(elem);
                        XmlNameTable nt = nr.NameTable;
                        _nsm = new XmlNamespaceManager(nt);
                        if (!Utils.VerifyAttributes(elem, (string)null)) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                        }
                        // Look for a namespace in the attributes
                        foreach (XmlAttribute attrib in elem.Attributes) {
                            if (attrib.Prefix == "xmlns") {
                                prefix = attrib.LocalName;
                                namespaceURI = attrib.Value;
                                if (prefix == null) {
                                    prefix = elem.Prefix;
                                    namespaceURI = elem.NamespaceURI;
                                }
                                _nsm.AddNamespace(prefix, namespaceURI);
                            }
                        }
                        break;
                    }
                    else if (!Utils.GetAllowAdditionalSignatureNodes()) {
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                    }
                }
            }

            if (_xpathexpr == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
        }

        protected override XmlNodeList GetInnerXml() {
            XmlDocument document = new XmlDocument();
            XmlElement element = document.CreateElement(null, "XPath", SignedXml.XmlDsigNamespaceUrl);

            if (_nsm != null) {
                // Add each of the namespaces as attributes of the element
                foreach (string prefix in _nsm) {
                    switch (prefix) {
                        // Ignore the xml namespaces
                        case "xml":
                        case "xmlns":
                            break;

                        // Other namespaces
                        default:
                            // Ignore the default namespace
                            if (prefix != null && prefix.Length > 0)
                                element.SetAttribute("xmlns:" + prefix, _nsm.LookupNamespace(prefix));
                            break;
                    }
                }
            }
            // Add the XPath as the inner xml of the element
            element.InnerXml = _xpathexpr;
            document.AppendChild(element);
            return document.ChildNodes;
        }

        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
            } else if (obj is XmlNodeList) {
                LoadXmlNodeListInput((XmlNodeList) obj);
            } else if (obj is XmlDocument) {
                LoadXmlDocumentInput((XmlDocument) obj);
            }
        }

        private void LoadStreamInput(Stream stream) {
            XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
            XmlReader valReader = Utils.PreProcessStreamInput(stream, resolver, this.BaseURI);
            _document = new XmlDocument();
            _document.PreserveWhitespace = true;
            _document.Load(valReader);
        }

        private void LoadXmlNodeListInput(XmlNodeList nodeList) {
            // Use C14N to get a document
            XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
            CanonicalXml c14n = new CanonicalXml((XmlNodeList) nodeList, resolver, true);
            using (MemoryStream ms = new MemoryStream(c14n.GetBytes())) {
                LoadStreamInput(ms);
            }
        }

        private void LoadXmlDocumentInput(XmlDocument doc) {
            _document = doc;
        }

        public override Object GetOutput() {
            CanonicalXmlNodeList resultNodeList = new CanonicalXmlNodeList();
            if (!String.IsNullOrEmpty(_xpathexpr)) {
                XPathNavigator navigator = _document.CreateNavigator();
                XPathNodeIterator it = navigator.Select("//. | //@*");

                XPathExpression xpathExpr = navigator.Compile("boolean(" + _xpathexpr + ")");
                xpathExpr.SetContext(_nsm);

                while (it.MoveNext()) {
                    XmlNode node = ((IHasXmlNode) it.Current).GetNode();

                    bool include = (bool) it.Current.Evaluate(xpathExpr);
                    if (include == true)
                        resultNodeList.Add(node);
                }

                // keep namespaces
                it = navigator.Select("//namespace::*");
                while (it.MoveNext()) {
                    XmlNode node = ((IHasXmlNode) it.Current).GetNode();
                    resultNodeList.Add(node);
                }
            }

            return resultNodeList;
        }

        public override Object GetOutput(Type type) {
            if (type != typeof(XmlNodeList) && !type.IsSubclassOf(typeof(XmlNodeList))) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
            return (XmlNodeList) GetOutput();
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigXsltTransform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlDocument), typeof(XmlNodeList) };
        private Type[] _outputTypes = { typeof(Stream) };
        private XmlNodeList _xslNodes;
        private string _xslFragment;
        private Stream _inputStream;
        private bool _includeComments = false;

        public XmlDsigXsltTransform() {
            Algorithm = SignedXml.XmlDsigXsltTransformUrl;
        }

        public XmlDsigXsltTransform(bool includeComments) {
            _includeComments = includeComments;
            Algorithm = SignedXml.XmlDsigXsltTransformUrl;
        }

        public override Type[] InputTypes {
            get {
                return _inputTypes;
            }
        }

        public override Type[] OutputTypes {
            get {
                return _outputTypes;
            }
        }

        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (nodeList == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            // check that the XSLT element is well formed
            XmlElement firstDataElement = null;
            int count = 0;
            foreach (XmlNode node in nodeList) {
                // ignore white spaces, but make sure only one child element is present
                if (node is XmlWhitespace) continue;
                if (node is XmlElement) {
                    if (count != 0) 
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                    firstDataElement = node as XmlElement;
                    count++;
                    continue;
                }
                // Only allow white spaces
                count++;
            }
            if (count != 1 || firstDataElement == null) 
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            _xslNodes = nodeList;
            _xslFragment = firstDataElement.OuterXml.Trim(null);
        }

        protected override XmlNodeList GetInnerXml() {
            return _xslNodes;
        }

        public override void LoadInput(Object obj) {
            if (_inputStream != null)
                _inputStream.Close();
            _inputStream = new MemoryStream();
            if (obj is Stream) {
                _inputStream = (Stream) obj;
            }
            else if (obj is XmlNodeList) {
                CanonicalXml xmlDoc = new CanonicalXml((XmlNodeList) obj, null, _includeComments);
                byte[] buffer = xmlDoc.GetBytes();
                if (buffer == null) return;
                _inputStream.Write(buffer, 0, buffer.Length);
                _inputStream.Flush();
                _inputStream.Position = 0;
            }
            else if (obj is XmlDocument) {
                CanonicalXml xmlDoc = new CanonicalXml((XmlDocument) obj, null, _includeComments);
                byte[] buffer = xmlDoc.GetBytes();
                if (buffer == null) return;
                _inputStream.Write(buffer, 0, buffer.Length);
                _inputStream.Flush();
                _inputStream.Position = 0;
            }
        }

        public override Object GetOutput() {
            //  XSL transforms expose many powerful features by default:
            //  1- we need to pass a null evidence to prevent script execution.
            //  2- XPathDocument will expand entities, we don't want this, so set the resolver to null
            //  3- We don't want the document function feature of XslTransforms.

            // load the XSL Transform
            XslCompiledTransform xslt = new XslCompiledTransform();
            XmlReaderSettings settings = new XmlReaderSettings();
            settings.XmlResolver = null;
            settings.MaxCharactersFromEntities = Utils.GetMaxCharactersFromEntities();
            settings.MaxCharactersInDocument = Utils.GetMaxCharactersInDocument();
            using (StringReader sr = new StringReader(_xslFragment)) {
                XmlReader readerXsl = XmlReader.Create(sr, settings, (string)null);
                xslt.Load(readerXsl, XsltSettings.Default, null);

                // Now load the input stream, XmlDocument can be used but is less efficient
                XmlReader reader = XmlReader.Create(_inputStream, settings, this.BaseURI);
                XPathDocument inputData = new XPathDocument(reader, XmlSpace.Preserve);

                // Create an XmlTextWriter
                MemoryStream ms = new MemoryStream();
                XmlWriter writer = new XmlTextWriter(ms, null);

                // Transform the data and send the output to the memory stream
                xslt.Transform(inputData, null, writer);
                ms.Position = 0;
                return ms;
            }
        }

        public override Object GetOutput(Type type) {
            if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
            return (Stream) GetOutput();
        }
    }


    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDsigEnvelopedSignatureTransform : Transform {
        private Type[] _inputTypes = { typeof(Stream), typeof(XmlNodeList), typeof(XmlDocument) };
        private Type[] _outputTypes = { typeof(XmlNodeList), typeof(XmlDocument) };
        private XmlNodeList _inputNodeList;
        private bool _includeComments = false;
        private XmlNamespaceManager _nsm = null;
        private XmlDocument _containingDocument = null;
        private int _signaturePosition = 0;

        internal int SignaturePosition {
            set { _signaturePosition = value; }
        }

        public XmlDsigEnvelopedSignatureTransform() {
            Algorithm = SignedXml.XmlDsigEnvelopedSignatureTransformUrl;
        }

        /// <internalonly/>
        public XmlDsigEnvelopedSignatureTransform(bool includeComments) {
            _includeComments = includeComments;
            Algorithm = SignedXml.XmlDsigEnvelopedSignatureTransformUrl;
        }

        public override Type[] InputTypes {
            get { return _inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return _outputTypes; }
        }

        // An enveloped signature has no inner XML elements
        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (!Utils.GetAllowAdditionalSignatureNodes() && nodeList != null && nodeList.Count > 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
        }

        // An enveloped signature has no inner XML elements
        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
                return;
            }
            if (obj is XmlNodeList) {
                LoadXmlNodeListInput((XmlNodeList) obj);
                return;
            }
            if (obj is XmlDocument) {
                LoadXmlDocumentInput((XmlDocument) obj);
                return;
            }
        }

        private void LoadStreamInput(Stream stream) {
            XmlDocument doc = new XmlDocument();
            doc.PreserveWhitespace = true;
            XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
            XmlReader xmlReader = Utils.PreProcessStreamInput(stream, resolver, this.BaseURI);
            doc.Load(xmlReader);
            _containingDocument = doc;
            if (_containingDocument == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_EnvelopedSignatureRequiresContext"));
            _nsm = new XmlNamespaceManager(_containingDocument.NameTable);
            _nsm.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);
        }

        private void LoadXmlNodeListInput(XmlNodeList nodeList) {
            // Empty node list is not acceptable
            if (nodeList == null) 
                throw new ArgumentNullException("nodeList");
            _containingDocument = Utils.GetOwnerDocument(nodeList);
            if (_containingDocument == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_EnvelopedSignatureRequiresContext"));

            _nsm = new XmlNamespaceManager(_containingDocument.NameTable);
            _nsm.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);
            _inputNodeList = nodeList;
        }

        private void LoadXmlDocumentInput(XmlDocument doc) {
            if (doc == null)
                throw new ArgumentNullException("doc");
            _containingDocument = doc;
            _nsm = new XmlNamespaceManager(_containingDocument.NameTable);
            _nsm.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);   
        }

        public override Object GetOutput() {
            if (_containingDocument == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_EnvelopedSignatureRequiresContext"));

            // If we have received an XmlNodeList as input
            if (_inputNodeList != null) {
                // If the position has not been set, then we don't want to remove any signature tags
                if (_signaturePosition == 0) return _inputNodeList;
                XmlNodeList signatureList = _containingDocument.SelectNodes("//dsig:Signature", _nsm);
                if (signatureList == null) return _inputNodeList;

                CanonicalXmlNodeList resultNodeList = new CanonicalXmlNodeList();
                foreach (XmlNode node in _inputNodeList) {
                    if (node == null)  continue;
                    // keep namespaces
                    if (Utils.IsXmlNamespaceNode(node) || Utils.IsNamespaceNode(node)) {
                        resultNodeList.Add(node);
                    } else {
                        // SelectSingleNode throws an exception for xmldecl PI for example, so we will just ignore those exceptions
                        try {
                            // Find the nearest signature ancestor tag 
                            XmlNode result = node.SelectSingleNode("ancestor-or-self::dsig:Signature[1]", _nsm);
                            int position = 0;
                            foreach (XmlNode node1 in signatureList) {
                                position++;
                                if (node1 == result) break;
                            } 
                            if (result == null || (result != null && position != _signaturePosition)) {
                                resultNodeList.Add(node);
                            }
                        }
                        catch {}
                    }
                }
                return resultNodeList;
            }
            // Else we have received either a stream or a document as input
            else {
                XmlNodeList signatureList = _containingDocument.SelectNodes("//dsig:Signature", _nsm);
                if (signatureList == null) return _containingDocument;
                if (signatureList.Count < _signaturePosition || _signaturePosition <= 0) return _containingDocument;

                // Remove the signature node with all its children nodes
                signatureList[_signaturePosition - 1].ParentNode.RemoveChild(signatureList[_signaturePosition - 1]);
                return _containingDocument;
            }
        }

        public override Object GetOutput(Type type) {
            if (type == typeof(XmlNodeList) || type.IsSubclassOf(typeof(XmlNodeList))) {
                if (_inputNodeList == null) {
                    _inputNodeList = Utils.AllDescendantNodes(_containingDocument, true);
                }
                return (XmlNodeList) GetOutput();
            } else if (type == typeof(XmlDocument) || type.IsSubclassOf(typeof(XmlDocument))) {
                if (_inputNodeList != null) throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
                return (XmlDocument) GetOutput();
            } else {
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");            
            }
        }
    }

    [Serializable]
    internal enum TransformInputType {
        XmlDocument = 1,
        XmlStream   = 2,
        XmlNodeSet  = 3
    }

    // XML Decryption Transform is used to specify the order of XML Digital Signature 
    // and XML Encryption when performed on the same document.

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlDecryptionTransform : Transform {
        private Type[] m_inputTypes = { typeof(Stream), typeof(XmlDocument) };
        private Type[] m_outputTypes = { typeof(XmlDocument) };
        private XmlNodeList m_encryptedDataList = null;
        private ArrayList m_arrayListUri = null; // this ArrayList object represents the Uri's to be excluded
        private EncryptedXml m_exml = null; // defines the XML encryption processing rules
        private XmlDocument m_containingDocument = null;
        private XmlNamespaceManager m_nsm = null;
        private const string XmlDecryptionTransformNamespaceUrl = "http://www.w3.org/2002/07/decrypt#";

        public XmlDecryptionTransform() {
            Algorithm = SignedXml.XmlDecryptionTransformUrl;
        }

        private ArrayList ExceptUris {
            get {
                if (m_arrayListUri == null)
                    m_arrayListUri = new ArrayList();
                return m_arrayListUri;
            }
        }

        protected virtual bool IsTargetElement (XmlElement inputElement, string idValue) {
            if (inputElement == null)
                return false;
            if (inputElement.GetAttribute("Id") == idValue || inputElement.GetAttribute("id") == idValue ||
                inputElement.GetAttribute("ID") == idValue)
                return true;

            return false;
        }

        public EncryptedXml EncryptedXml {
            get {
                if (m_exml != null)
                    return m_exml;

                Reference reference = this.Reference;
                SignedXml signedXml = (reference == null ? this.SignedXml : reference.SignedXml);
                if (signedXml == null || signedXml.EncryptedXml == null)
                    m_exml = new EncryptedXml(m_containingDocument); // default processing rules
                else
                    m_exml = signedXml.EncryptedXml;

                return m_exml;
            }
            set { m_exml = value; }
        }

        public override Type[] InputTypes {
            get { return m_inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return m_outputTypes; }
        }

        public void AddExceptUri (string uri) {
            if (uri == null)
                throw new ArgumentNullException("uri");
            ExceptUris.Add(uri);
        }

        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (nodeList == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
            ExceptUris.Clear();
            foreach (XmlNode node in nodeList) {
                XmlElement elem = node as XmlElement;
                if (elem != null) {
                    if (elem.LocalName == "Except" && elem.NamespaceURI == XmlDecryptionTransformNamespaceUrl) {
                        // the Uri is required
                        string uri = Utils.GetAttribute(elem, "URI", XmlDecryptionTransformNamespaceUrl);
                        if (uri == null || uri.Length == 0 || uri[0] != '#') 
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UriRequired"));
                        if (!Utils.VerifyAttributes(elem, "URI")) {
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                        }
                        string idref = Utils.ExtractIdFromLocalUri(uri);
                        ExceptUris.Add(idref);
                    }
                    else if (!Utils.GetAllowAdditionalSignatureNodes()) {
                        throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
                    }
                }
            }
        }

        protected override XmlNodeList GetInnerXml() {
            if (ExceptUris.Count == 0)
                return null;
            XmlDocument document = new XmlDocument();
            XmlElement element = document.CreateElement("Transform", SignedXml.XmlDsigNamespaceUrl);
            if (!String.IsNullOrEmpty(this.Algorithm))
                element.SetAttribute("Algorithm", this.Algorithm);
            foreach (string uri in ExceptUris) {
                XmlElement exceptUriElement = document.CreateElement("Except", XmlDecryptionTransformNamespaceUrl);
                exceptUriElement.SetAttribute("URI", uri);
                element.AppendChild(exceptUriElement);
            }
            return element.ChildNodes;
        }

        public override void LoadInput(Object obj) {
            if (obj is Stream) {
                LoadStreamInput((Stream) obj);
            } else if (obj is XmlDocument) {
                LoadXmlDocumentInput((XmlDocument) obj);
            }
        }

        private void LoadStreamInput(Stream stream) {
            XmlDocument document = new XmlDocument();
            document.PreserveWhitespace = true;
            XmlResolver resolver = (this.ResolverSet ? this.m_xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), this.BaseURI));
            XmlReader xmlReader = Utils.PreProcessStreamInput(stream, resolver, this.BaseURI);
            document.Load(xmlReader);
            m_containingDocument = document;
            m_nsm = new XmlNamespaceManager(m_containingDocument.NameTable);
            m_nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);
            // select all EncryptedData elements
            m_encryptedDataList = document.SelectNodes("//enc:EncryptedData", m_nsm);
        }

        private void LoadXmlDocumentInput(XmlDocument document) {
            if (document == null)
                throw new ArgumentNullException("document");
            m_containingDocument = document;
            m_nsm = new XmlNamespaceManager(document.NameTable);
            m_nsm.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);
            // select all EncryptedData elements
            m_encryptedDataList = document.SelectNodes("//enc:EncryptedData", m_nsm);
        }

        // Replace the encrytped XML element with the decrypted data for signature verification
        private void ReplaceEncryptedData(XmlElement encryptedDataElement, byte[] decrypted) {
            XmlNode parent = encryptedDataElement.ParentNode;
            if (parent.NodeType == XmlNodeType.Document) {
                // We're replacing the root element.  In order to correctly reflect the semantics of the
                // decryption transform, we need to replace the entire document with the decrypted data. 
                // However, EncryptedXml.ReplaceData will preserve other top-level elements such as the XML
                // entity declaration and top level comments.  So, in this case we must do the replacement
                // ourselves.
                parent.InnerXml = EncryptedXml.Encoding.GetString(decrypted);
            }
            else {
                // We're replacing a node in the middle of the document - EncryptedXml knows how to handle
                // this case in conformance with the transform's requirements, so we'll just defer to it.
                EncryptedXml.ReplaceData(encryptedDataElement, decrypted);
            }
        }

        private bool ProcessEncryptedDataItem (XmlElement encryptedDataElement) {
            // first see whether we want to ignore this one
            if (ExceptUris.Count > 0) {
                for (int index = 0; index < ExceptUris.Count; index++) {
                    if (IsTargetElement(encryptedDataElement, (string) ExceptUris[index]))
                        return false;
                }
            }
            EncryptedData ed = new EncryptedData();
            ed.LoadXml(encryptedDataElement);
            SymmetricAlgorithm symAlg = this.EncryptedXml.GetDecryptionKey(ed, null);
            if (symAlg == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_MissingDecryptionKey"));
            byte[] decrypted = EncryptedXml.DecryptData(ed, symAlg);

            ReplaceEncryptedData(encryptedDataElement, decrypted);
            return true;
        }

        private void ProcessElementRecursively (XmlNodeList encryptedDatas) {
            if (encryptedDatas == null || encryptedDatas.Count == 0)
                return;
            Queue encryptedDatasQueue = new Queue();
            foreach (XmlNode value in encryptedDatas) {
                encryptedDatasQueue.Enqueue(value);
            }
            XmlNode node = encryptedDatasQueue.Dequeue() as XmlNode;
            while (node != null) {
                XmlElement encryptedDataElement = node as XmlElement;
                if (encryptedDataElement != null && encryptedDataElement.LocalName == "EncryptedData" &&
                    encryptedDataElement.NamespaceURI == EncryptedXml.XmlEncNamespaceUrl) {
                    XmlNode sibling = encryptedDataElement.NextSibling;
                    XmlNode parent = encryptedDataElement.ParentNode;
                    if (ProcessEncryptedDataItem(encryptedDataElement)) {
                        // find the new decrypted element.
                        XmlNode child = parent.FirstChild;
                        while (child != null && child.NextSibling != sibling)
                            child = child.NextSibling;
                        if (child != null) {
                            XmlNodeList nodes = child.SelectNodes("//enc:EncryptedData", m_nsm);
                            if (nodes.Count > 0) {
                                foreach (XmlNode value in nodes) {
                                    encryptedDatasQueue.Enqueue(value);
                                }
                            }
                        }
                    }
                }
                if (encryptedDatasQueue.Count == 0)
                    break;
                node = encryptedDatasQueue.Dequeue() as XmlNode;
            }
        }

        public override Object GetOutput() {
            // decrypt the encrypted sections
            if (m_encryptedDataList != null)
                ProcessElementRecursively(m_encryptedDataList);
            // propagate namespaces
            Utils.AddNamespaces(m_containingDocument.DocumentElement, this.PropagatedNamespaces);
            return m_containingDocument;
        }

        public override Object GetOutput(Type type) {
            if (type == typeof(XmlDocument))
                return (XmlDocument) GetOutput();
            else
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");
        }
    }

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class XmlLicenseTransform : Transform {
        private Type[]              inputTypes       = { typeof(XmlDocument) };
        private Type[]              outputTypes      = { typeof(XmlDocument) };
        private XmlNamespaceManager namespaceManager = null;
        private XmlDocument         license          = null;
        private IRelDecryptor       relDecryptor     = null;
        private const string        ElementIssuer    = "issuer";
        private const string        NamespaceUriCore = "urn:mpeg:mpeg21:2003:01-REL-R-NS";

        public XmlLicenseTransform() {
            Algorithm = SignedXml.XmlLicenseTransformUrl;
        }

        public override Type[] InputTypes {
            get { return inputTypes; }
        }

        public override Type[] OutputTypes {
            get { return outputTypes; }
        }

        public IRelDecryptor Decryptor {
            get { return relDecryptor; }
            set { relDecryptor = value; }
        }

        private void DecryptEncryptedGrants(XmlNodeList encryptedGrantList, IRelDecryptor decryptor) {
            XmlElement       encryptionMethod    = null;
            XmlElement       keyInfo             = null;
            XmlElement       cipherData          = null;
            EncryptionMethod encryptionMethodObj = null;
            KeyInfo          keyInfoObj          = null;
            CipherData       cipherDataObj       = null;

            for (int i = 0, count = encryptedGrantList.Count; i < count; i++) {
                encryptionMethod = encryptedGrantList[i].SelectSingleNode("//r:encryptedGrant/enc:EncryptionMethod", namespaceManager) as XmlElement;
                keyInfo          = encryptedGrantList[i].SelectSingleNode("//r:encryptedGrant/dsig:KeyInfo", namespaceManager) as XmlElement;
                cipherData       = encryptedGrantList[i].SelectSingleNode("//r:encryptedGrant/enc:CipherData", namespaceManager) as XmlElement;
                if ((encryptionMethod != null) &&
                    (keyInfo != null) &&
                    (cipherData != null)) {
                    encryptionMethodObj = new EncryptionMethod();
                    keyInfoObj          = new KeyInfo();
                    cipherDataObj       = new CipherData();

                    encryptionMethodObj.LoadXml(encryptionMethod);
                    keyInfoObj.LoadXml(keyInfo);
                    cipherDataObj.LoadXml(cipherData);

                    MemoryStream toDecrypt        = null;
                    Stream       decryptedContent = null;
                    StreamReader streamReader     = null;

                    try {
                        toDecrypt = new MemoryStream(cipherDataObj.CipherValue);
                        decryptedContent = relDecryptor.Decrypt(encryptionMethodObj,
                                                                keyInfoObj, toDecrypt);

                        if ((decryptedContent == null) || (decryptedContent.Length == 0))
                            throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_XrmlUnableToDecryptGrant"));

                        streamReader = new StreamReader(decryptedContent);
                        string clearContent = streamReader.ReadToEnd();

                        encryptedGrantList[i].ParentNode.InnerXml = clearContent;
                    }
                    finally {
                        if (toDecrypt != null)
                            toDecrypt.Close();

                        if (decryptedContent != null)
                            decryptedContent.Close();

                        if (streamReader != null)
                            streamReader.Close();
                    }

                    encryptionMethodObj = null;
                    keyInfoObj          = null;
                    cipherDataObj       = null;
                }

                encryptionMethod = null;
                keyInfo          = null;
                cipherData       = null;
            }
        }

        // License transform has no inner XML elements
        protected override XmlNodeList GetInnerXml() {
            return null;
        }

        public override object GetOutput() {
            return license;
        }

        public override object GetOutput(Type type) {
            if ((type != typeof(XmlDocument)) || (!type.IsSubclassOf(typeof(XmlDocument))))
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_TransformIncorrectInputType"), "type");

            return GetOutput();
        }

        // License transform has no inner XML elements
        public override void LoadInnerXml(XmlNodeList nodeList) {
            if (!Utils.GetAllowAdditionalSignatureNodes() && nodeList != null && nodeList.Count > 0)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_UnknownTransform"));
        }

        [SuppressMessage("Microsoft.Security.Xml", "CA3058:DoNotUseSetInnerXml", Justification="Operates on inputs which were already parsed by XmlDocument with valid settings and already would have produced errors (DTD or external resolution)")]
        public override void LoadInput (object obj) {
            // Check if the Context property is set before this transform is invoked.
            if (Context == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_XrmlMissingContext"));

            license = new XmlDocument();
            license.PreserveWhitespace = true;
            namespaceManager = new XmlNamespaceManager(license.NameTable);
            namespaceManager.AddNamespace("dsig", SignedXml.XmlDsigNamespaceUrl);
            namespaceManager.AddNamespace("enc", EncryptedXml.XmlEncNamespaceUrl);
            namespaceManager.AddNamespace("r", NamespaceUriCore);

            XmlElement currentIssuerContext  = null;
            XmlElement currentLicenseContext = null;
            XmlNode    signatureNode         = null;

            // Get the nearest issuer node
            currentIssuerContext = Context.SelectSingleNode("ancestor-or-self::r:issuer[1]", namespaceManager) as XmlElement;
            if (currentIssuerContext == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_XrmlMissingIssuer"));

            signatureNode = currentIssuerContext.SelectSingleNode("descendant-or-self::dsig:Signature[1]", namespaceManager) as XmlElement;
            if (signatureNode != null)
                signatureNode.ParentNode.RemoveChild(signatureNode);

            // Get the nearest license node
            currentLicenseContext = currentIssuerContext.SelectSingleNode("ancestor-or-self::r:license[1]", namespaceManager) as XmlElement;
            if (currentLicenseContext == null)
                throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_XrmlMissingLicence"));

            XmlNodeList issuerList = currentLicenseContext.SelectNodes("descendant-or-self::r:license[1]/r:issuer", namespaceManager);

            // Remove all issuer nodes except current
            for (int i = 0, count = issuerList.Count; i < count; i++) {
                if (issuerList[i] == currentIssuerContext)
                    continue;

                if ((issuerList[i].LocalName == ElementIssuer) && 
                    (issuerList[i].NamespaceURI == NamespaceUriCore))
                    issuerList[i].ParentNode.RemoveChild(issuerList[i]);
            }

            XmlNodeList encryptedGrantList = currentLicenseContext.SelectNodes("/r:license/r:grant/r:encryptedGrant", namespaceManager);

            if (encryptedGrantList.Count > 0) {
                if (relDecryptor == null)
                    throw new CryptographicException(SecurityResources.GetResourceString("Cryptography_Xml_XrmlMissingIRelDecryptor"));

                DecryptEncryptedGrants(encryptedGrantList, relDecryptor);
            }

            license.InnerXml = currentLicenseContext.OuterXml;
        }
    }

    public interface IRelDecryptor {
        Stream Decrypt(EncryptionMethod encryptionMethod, KeyInfo keyInfo, Stream toDecrypt);
    }
}
