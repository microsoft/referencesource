// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// C14NUtil.cs
//

namespace System.Security.Cryptography.Xml {
    using System;
    using System.Xml;
    using System.IO;
    using System.Text;
    using System.Collections;

    // the current rendering position in document
    internal enum DocPosition {
        BeforeRootElement,
        InRootElement,
        AfterRootElement
    }

    // the interface to be implemented by all subclasses of XmlNode
    // that have to provide node subsetting and canonicalization features.
    internal interface ICanonicalizableNode {
        bool IsInNodeSet {
            get;
            set;
        }

        void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc);
        void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc);
    }

    // the central dispatcher for canonicalization writes. not all node classes
    // implement ICanonicalizableNode; so a manual dispatch is sometimes necessary.
    internal class CanonicalizationDispatcher {
        private CanonicalizationDispatcher() {}

        public static void Write(XmlNode node, StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (node is ICanonicalizableNode) {
                ((ICanonicalizableNode) node).Write(strBuilder, docPos, anc);
            } else {
                WriteGenericNode(node, strBuilder, docPos, anc);
            }
        }

        public static void WriteGenericNode(XmlNode node, StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (node == null)
                throw new ArgumentNullException("node");

            XmlNodeList childNodes = node.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                Write(childNode, strBuilder, docPos, anc);
            }
        }

        public static void WriteHash(XmlNode node, HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (node is ICanonicalizableNode) {
                ((ICanonicalizableNode) node).WriteHash(hash, docPos, anc);
            } else {
                WriteHashGenericNode(node, hash, docPos, anc);
            }
        }

        public static void WriteHashGenericNode(XmlNode node, HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (node == null)
                throw new ArgumentNullException("node");

            XmlNodeList childNodes = node.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                WriteHash(childNode, hash, docPos, anc);
            }
        }
    }

    // all input types eventually lead to the creation of an XmlDocument document
    // of this type. it maintains the node subset state and performs output rendering during canonicalization
    internal class CanonicalXmlDocument : XmlDocument, ICanonicalizableNode {
        bool m_defaultNodeSetInclusionState;
        bool m_includeComments;
        bool m_isInNodeSet;

        public CanonicalXmlDocument(bool defaultNodeSetInclusionState, bool includeComments) : base() {
            this.PreserveWhitespace = true;
            m_includeComments = includeComments;
            m_isInNodeSet = m_defaultNodeSetInclusionState = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            docPos = DocPosition.BeforeRootElement;
            foreach (XmlNode childNode in this.ChildNodes) {
                if (childNode.NodeType == XmlNodeType.Element) {
                    CanonicalizationDispatcher.Write(childNode, strBuilder, DocPosition.InRootElement, anc);
                    docPos = DocPosition.AfterRootElement;
                } else {
                    CanonicalizationDispatcher.Write(childNode, strBuilder, docPos, anc);
                }
            }
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            docPos = DocPosition.BeforeRootElement;
            foreach (XmlNode childNode in this.ChildNodes) {
                if (childNode.NodeType == XmlNodeType.Element) {
                    CanonicalizationDispatcher.WriteHash(childNode, hash, DocPosition.InRootElement, anc);
                    docPos = DocPosition.AfterRootElement;
                } else {
                    CanonicalizationDispatcher.WriteHash(childNode, hash, docPos, anc);
                }
            }
        }

        public override XmlElement CreateElement(string prefix, string localName, string namespaceURI) {
            return new CanonicalXmlElement(prefix, localName, namespaceURI, this, m_defaultNodeSetInclusionState);
        }

        public override XmlAttribute CreateAttribute(string prefix, string localName, string namespaceURI) {
            return new CanonicalXmlAttribute(prefix, localName, namespaceURI, this, m_defaultNodeSetInclusionState);
        }

        protected override XmlAttribute CreateDefaultAttribute(string prefix, string localName, string namespaceURI) {
            return new CanonicalXmlAttribute(prefix, localName, namespaceURI, this, m_defaultNodeSetInclusionState);
        }

        public override XmlText CreateTextNode(string text) {
            return new CanonicalXmlText(text, this, m_defaultNodeSetInclusionState);
        }

        public override XmlWhitespace CreateWhitespace(string prefix) {
            return new CanonicalXmlWhitespace(prefix, this, m_defaultNodeSetInclusionState);
        }

        public override XmlSignificantWhitespace CreateSignificantWhitespace(string text) {
            return new CanonicalXmlSignificantWhitespace(text, this, m_defaultNodeSetInclusionState);
        }

        public override XmlProcessingInstruction CreateProcessingInstruction(string target, string data) {
            return new CanonicalXmlProcessingInstruction(target, data, this, m_defaultNodeSetInclusionState);
        }

        public override XmlComment CreateComment(string data) {
            return new CanonicalXmlComment(data, this, m_defaultNodeSetInclusionState, m_includeComments);
        }

        public override XmlEntityReference CreateEntityReference(string name) {
            return new CanonicalXmlEntityReference(name, this, m_defaultNodeSetInclusionState);
        }

        public override XmlCDataSection CreateCDataSection(string data) {
            return new CanonicalXmlCDataSection(data, this, m_defaultNodeSetInclusionState);
        }
    }

    // the class that provides node subset state and canonicalization function to XmlElement
    internal class CanonicalXmlElement : XmlElement, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlElement(string prefix, string localName, string namespaceURI, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(prefix, localName, namespaceURI, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            Hashtable nsLocallyDeclared = new Hashtable();
            SortedList nsListToRender = new SortedList(new NamespaceSortOrder());
            SortedList attrListToRender = new SortedList(new AttributeSortOrder());

            XmlAttributeCollection attrList = this.Attributes;
            if (attrList != null) {
                foreach (XmlAttribute attr in attrList) {
                    if (((CanonicalXmlAttribute) attr).IsInNodeSet || Utils.IsNamespaceNode(attr) || Utils.IsXmlNamespaceNode(attr)) {
                        if (Utils.IsNamespaceNode(attr)) {
                            anc.TrackNamespaceNode(attr, nsListToRender, nsLocallyDeclared);
                        }
                        else if (Utils.IsXmlNamespaceNode(attr)) {
                            anc.TrackXmlNamespaceNode(attr, nsListToRender, attrListToRender, nsLocallyDeclared);
                        } 
                        else if (IsInNodeSet) {
                            attrListToRender.Add(attr, null);
                        }
                    }
                }
            }

            if (!Utils.IsCommittedNamespace(this, this.Prefix, this.NamespaceURI)) {
                string name = ((this.Prefix.Length > 0) ? "xmlns" + ":" + this.Prefix : "xmlns");
                XmlAttribute nsattrib = (XmlAttribute) this.OwnerDocument.CreateAttribute(name);
                nsattrib.Value = this.NamespaceURI;
                anc.TrackNamespaceNode(nsattrib, nsListToRender, nsLocallyDeclared);
            }

            if (IsInNodeSet) {
                anc.GetNamespacesToRender(this, attrListToRender, nsListToRender, nsLocallyDeclared);

                strBuilder.Append("<" + this.Name);
                foreach (object attr in nsListToRender.GetKeyList()) {
                    (attr as CanonicalXmlAttribute).Write(strBuilder, docPos, anc);
                }
                foreach (object attr in attrListToRender.GetKeyList()) {
                    (attr as CanonicalXmlAttribute).Write(strBuilder, docPos, anc);
                }
                strBuilder.Append(">");
            }

            anc.EnterElementContext();
            anc.LoadUnrenderedNamespaces(nsLocallyDeclared);
            anc.LoadRenderedNamespaces(nsListToRender);

            XmlNodeList childNodes = this.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                CanonicalizationDispatcher.Write(childNode, strBuilder, docPos, anc);
            }

            anc.ExitElementContext();

            if (IsInNodeSet) {
                strBuilder.Append("</" + this.Name + ">");
            }
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            Hashtable nsLocallyDeclared = new Hashtable();
            SortedList nsListToRender = new SortedList(new NamespaceSortOrder());
            SortedList attrListToRender = new SortedList(new AttributeSortOrder());
            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] rgbData;

            XmlAttributeCollection attrList = this.Attributes;
            if (attrList != null) {
                foreach (XmlAttribute attr in attrList) {
                    if (((CanonicalXmlAttribute) attr).IsInNodeSet || Utils.IsNamespaceNode(attr) || Utils.IsXmlNamespaceNode(attr)) {
                        if (Utils.IsNamespaceNode(attr)) {
                            anc.TrackNamespaceNode(attr, nsListToRender, nsLocallyDeclared);
                        }
                        else if (Utils.IsXmlNamespaceNode(attr)) {
                            anc.TrackXmlNamespaceNode(attr, nsListToRender, attrListToRender, nsLocallyDeclared);
                        }
                        else if(IsInNodeSet) {
                            attrListToRender.Add(attr, null);
                        }
                    }
                }
            }

            if (!Utils.IsCommittedNamespace(this, this.Prefix, this.NamespaceURI)) {
                string name = ((this.Prefix.Length > 0) ? "xmlns" + ":" + this.Prefix : "xmlns");
                XmlAttribute nsattrib = (XmlAttribute) this.OwnerDocument.CreateAttribute(name);
                nsattrib.Value = this.NamespaceURI;
                anc.TrackNamespaceNode(nsattrib, nsListToRender, nsLocallyDeclared);
            }

            if (IsInNodeSet) {
                anc.GetNamespacesToRender(this, attrListToRender, nsListToRender, nsLocallyDeclared);
                rgbData = utf8.GetBytes("<" + this.Name);
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
                foreach (object attr in nsListToRender.GetKeyList()) {
                    (attr as CanonicalXmlAttribute).WriteHash(hash, docPos, anc);
                }
                foreach (object attr in attrListToRender.GetKeyList()) {
                    (attr as CanonicalXmlAttribute).WriteHash(hash, docPos, anc);
                }
                rgbData = utf8.GetBytes(">");
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }

            anc.EnterElementContext();
            anc.LoadUnrenderedNamespaces(nsLocallyDeclared);
            anc.LoadRenderedNamespaces(nsListToRender);

            XmlNodeList childNodes = this.ChildNodes;
            foreach (XmlNode childNode in childNodes) {
                CanonicalizationDispatcher.WriteHash(childNode, hash, docPos, anc);
            }

            anc.ExitElementContext();

            if (IsInNodeSet) {
                rgbData = utf8.GetBytes("</" + this.Name + ">");
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }

    // the class that provides node subset state and canonicalization function to XmlAttribute
    internal class CanonicalXmlAttribute : XmlAttribute, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlAttribute(string prefix, string localName, string namespaceURI, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(prefix, localName, namespaceURI, doc) {
            IsInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            strBuilder.Append(" " + this.Name + "=\"");
            strBuilder.Append(Utils.EscapeAttributeValue(this.Value));
            strBuilder.Append("\"");
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] rgbData = utf8.GetBytes(" " + this.Name + "=\"");
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            rgbData = utf8.GetBytes(Utils.EscapeAttributeValue(this.Value));
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            rgbData = utf8.GetBytes("\"");
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
        }
    }

    // the class that provides node subset state and canonicalization function to XmlText
    internal class CanonicalXmlText : XmlText, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlText(string strData, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(strData, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet)
                strBuilder.Append(Utils.EscapeTextData(this.Value));
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet) {
                UTF8Encoding utf8 = new UTF8Encoding(false);
                byte[] rgbData = utf8.GetBytes(Utils.EscapeTextData(this.Value));
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }

    // the class that provides node subset state and canonicalization function to XmlWhitespace
    internal class CanonicalXmlWhitespace : XmlWhitespace, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlWhitespace(string strData, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(strData, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet && docPos == DocPosition.InRootElement)
                strBuilder.Append(Utils.EscapeWhitespaceData(this.Value));
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet && docPos == DocPosition.InRootElement) {
                UTF8Encoding utf8 = new UTF8Encoding(false);
                byte[] rgbData = utf8.GetBytes(Utils.EscapeWhitespaceData(this.Value));
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }

    // the class that provides node subset state and canonicalization function to XmlSignificantWhitespace
    internal class CanonicalXmlSignificantWhitespace : XmlSignificantWhitespace, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlSignificantWhitespace(string strData, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(strData, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet && docPos == DocPosition.InRootElement)
                strBuilder.Append(Utils.EscapeWhitespaceData(this.Value));
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet && docPos == DocPosition.InRootElement) {
                UTF8Encoding utf8 = new UTF8Encoding(false);
                byte[] rgbData = utf8.GetBytes(Utils.EscapeWhitespaceData(this.Value));
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }

    // the class that provides node subset state and canonicalization function to XmlComment
    internal class CanonicalXmlComment : XmlComment, ICanonicalizableNode {
        private bool m_isInNodeSet;
        private bool m_includeComments;

        public CanonicalXmlComment(string comment, XmlDocument doc, bool defaultNodeSetInclusionState, bool includeComments)
            : base(comment, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
            m_includeComments = includeComments;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public bool IncludeComments {
            get { return m_includeComments; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (!IsInNodeSet || !IncludeComments)
                return;

            if (docPos == DocPosition.AfterRootElement)
                strBuilder.Append((char) 10);
            strBuilder.Append("<!--");
            strBuilder.Append(this.Value);
            strBuilder.Append("-->");
            if (docPos == DocPosition.BeforeRootElement)
                strBuilder.Append((char) 10);
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (!IsInNodeSet || !IncludeComments)
                return;

            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] rgbData = utf8.GetBytes("(char) 10");
            if (docPos == DocPosition.AfterRootElement)
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            rgbData = utf8.GetBytes("<!--");
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            rgbData = utf8.GetBytes(this.Value);
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            rgbData = utf8.GetBytes("-->");
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            if (docPos == DocPosition.BeforeRootElement) {
                rgbData = utf8.GetBytes("(char) 10");
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }



    // the class that provides node subset state and canonicalization function to XmlProcessingInstruction
    internal class CanonicalXmlProcessingInstruction : XmlProcessingInstruction, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlProcessingInstruction(string target, string data, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(target, data, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (!IsInNodeSet)
                return;

            if (docPos == DocPosition.AfterRootElement)
                strBuilder.Append((char) 10);
            strBuilder.Append("<?");
            strBuilder.Append(this.Name);
            if ((this.Value != null) && (this.Value.Length > 0))
                strBuilder.Append(" " + this.Value);
            strBuilder.Append("?>");
            if (docPos == DocPosition.BeforeRootElement)
                strBuilder.Append((char) 10);
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (!IsInNodeSet)
                return;

            UTF8Encoding utf8 = new UTF8Encoding(false);
            byte[] rgbData;
            if (docPos == DocPosition.AfterRootElement) {
                rgbData = utf8.GetBytes("(char) 10");
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
            rgbData = utf8.GetBytes("<?");
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            rgbData = utf8.GetBytes((this.Name));
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            if ((this.Value != null) && (this.Value.Length > 0)) {
                rgbData = utf8.GetBytes(" " + this.Value);
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
            rgbData = utf8.GetBytes("?>");
            hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            if (docPos == DocPosition.BeforeRootElement) {
                rgbData = utf8.GetBytes("(char) 10");
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }



    // the class that provides node subset state and canonicalization function to XmlEntityReference
    internal class CanonicalXmlEntityReference : XmlEntityReference, ICanonicalizableNode {
        private bool m_isInNodeSet;

        public CanonicalXmlEntityReference(string name, XmlDocument doc, bool defaultNodeSetInclusionState)
            : base(name, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet)
                CanonicalizationDispatcher.WriteGenericNode(this, strBuilder, docPos, anc);
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet)
                CanonicalizationDispatcher.WriteHashGenericNode(this, hash, docPos, anc);
        }
    }

    // the class that provides node subset state and canonicalization function to XmlCDataSection
    internal class CanonicalXmlCDataSection: XmlCDataSection, ICanonicalizableNode {
        private bool m_isInNodeSet;
        public CanonicalXmlCDataSection(string data, XmlDocument doc, bool defaultNodeSetInclusionState) : base(data, doc) {
            m_isInNodeSet = defaultNodeSetInclusionState;
        }

        public bool IsInNodeSet {
            get { return m_isInNodeSet; }
            set { m_isInNodeSet = value; }
        }

        public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet)
                strBuilder.Append(Utils.EscapeCData(this.Data));
        }

        public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc) {
            if (IsInNodeSet) {
                UTF8Encoding utf8 = new UTF8Encoding(false);
                byte[] rgbData = utf8.GetBytes(Utils.EscapeCData(this.Data));
                hash.TransformBlock(rgbData, 0, rgbData.Length, rgbData, 0);
            }
        }
    }

    internal class CanonicalXmlNodeList : XmlNodeList, IList {
        private ArrayList m_nodeArray;

        internal CanonicalXmlNodeList() {
            m_nodeArray = new ArrayList();
        }

        public override XmlNode Item(int index) {
            return (XmlNode) m_nodeArray[index];
        }

        public override IEnumerator GetEnumerator() {
            return m_nodeArray.GetEnumerator();
        }

        public override int Count {
            get { return m_nodeArray.Count; }
        }

        // IList methods
        public int Add(Object value) {
            if (!(value is XmlNode))
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "node");
            return m_nodeArray.Add(value);
        }

        public void Clear() {
            m_nodeArray.Clear();
        }

        public bool Contains(Object value) {
            return m_nodeArray.Contains(value);
        }

        public int IndexOf(Object value) {
            return m_nodeArray.IndexOf(value);
        }

        public void Insert(int index, Object value) {
            if (!(value is XmlNode)) 
                throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");
            m_nodeArray.Insert(index,value);
        }

        public void Remove(Object value) {
            m_nodeArray.Remove(value);
        }

        public void RemoveAt(int index) {
            m_nodeArray.RemoveAt(index);
        }

        public Boolean IsFixedSize {
            get { return m_nodeArray.IsFixedSize; }
        }

        public Boolean IsReadOnly {
            get { return m_nodeArray.IsReadOnly; }
        }

        Object IList.this[int index] {
            get { return m_nodeArray[index]; }
            set { 
                if (!(value is XmlNode)) 
                    throw new ArgumentException(SecurityResources.GetResourceString("Cryptography_Xml_IncorrectObjectType"), "value");
                m_nodeArray[index] = value;
            }
        }

        public void CopyTo(Array array, int index) {
            m_nodeArray.CopyTo(array, index);
        }

        public Object SyncRoot {
            get { return m_nodeArray.SyncRoot; }
        }

        public bool IsSynchronized {
            get { return m_nodeArray.IsSynchronized; }
        }
    }

    // This class does lexicographic sorting by NamespaceURI first and then by LocalName.
    internal class AttributeSortOrder : IComparer {
        internal AttributeSortOrder() {}

        public int Compare(Object a, Object b) {
            XmlNode nodeA = a as XmlNode;
            XmlNode nodeB = b as XmlNode;
            if ((a == null) || (b == null))
                throw new ArgumentException();
            int namespaceCompare = String.CompareOrdinal(nodeA.NamespaceURI, nodeB.NamespaceURI);
            if (namespaceCompare != 0) return namespaceCompare;
            return String.CompareOrdinal(nodeA.LocalName, nodeB.LocalName);
        }
    }

    internal class NamespaceSortOrder : IComparer {
        internal NamespaceSortOrder() {}

        public int Compare(Object a, Object b) {
            XmlNode nodeA = a as XmlNode;
            XmlNode nodeB = b as XmlNode;
            if ((a == null) || (b == null))
                throw new ArgumentException();
            bool nodeAdefault = Utils.IsDefaultNamespaceNode(nodeA);
            bool nodeBdefault = Utils.IsDefaultNamespaceNode(nodeB);
            if (nodeAdefault && nodeBdefault) return 0;
            if (nodeAdefault) return -1;
            if (nodeBdefault) return 1;
            return String.CompareOrdinal(nodeA.LocalName, nodeB.LocalName);
        }
    }

    // the namespaces context corresponding to one XmlElement. the rendered list contains the namespace nodes that are actually
    // rendered to the canonicalized output. the unrendered list contains the namespace nodes that are in the node set and have
    // the XmlElement as the owner, but are not rendered.
    internal class NamespaceFrame {
        private Hashtable m_rendered = new Hashtable();
        private Hashtable m_unrendered = new Hashtable();

        internal NamespaceFrame() {}

        internal void AddRendered(XmlAttribute attr) {
            m_rendered.Add(Utils.GetNamespacePrefix(attr), attr);
        }

        internal XmlAttribute GetRendered(string nsPrefix) {
            return (XmlAttribute) m_rendered[nsPrefix];
        }

        internal void AddUnrendered(XmlAttribute attr) {
            m_unrendered.Add(Utils.GetNamespacePrefix(attr), attr);
        }

        internal XmlAttribute GetUnrendered(string nsPrefix) {
            return (XmlAttribute) m_unrendered[nsPrefix];
        }

        internal Hashtable GetUnrendered() {
            return m_unrendered;
        }
    }

    abstract internal class AncestralNamespaceContextManager {
        internal ArrayList m_ancestorStack = new ArrayList();

        internal NamespaceFrame GetScopeAt(int i) {
            return (NamespaceFrame) m_ancestorStack[i];
        }

        internal NamespaceFrame GetCurrentScope() {
            return GetScopeAt(m_ancestorStack.Count - 1);
        }

        protected XmlAttribute GetNearestRenderedNamespaceWithMatchingPrefix(string nsPrefix, out int depth) {
            XmlAttribute attr = null;
            depth = -1;
            for (int i = m_ancestorStack.Count - 1; i >= 0; i--) {
                if ((attr = GetScopeAt(i).GetRendered(nsPrefix)) != null) {
                    depth = i;
                    return attr;
                }
            }
            return null;
        }

        protected XmlAttribute GetNearestUnrenderedNamespaceWithMatchingPrefix(string nsPrefix, out int depth) {
            XmlAttribute attr = null;
            depth = -1;
            for (int i = m_ancestorStack.Count - 1; i >= 0; i--) {
                if ((attr = GetScopeAt(i).GetUnrendered(nsPrefix)) != null) {
                    depth = i;
                    return attr;
                }
            }
            return null;
        }

        internal void EnterElementContext() {
            m_ancestorStack.Add(new NamespaceFrame());
        }

        internal void ExitElementContext() {
            m_ancestorStack.RemoveAt(m_ancestorStack.Count - 1);
        }

        internal abstract void TrackNamespaceNode(XmlAttribute attr, SortedList nsListToRender, Hashtable nsLocallyDeclared);
        internal abstract void TrackXmlNamespaceNode(XmlAttribute attr, SortedList nsListToRender, SortedList attrListToRender, Hashtable nsLocallyDeclared);
        internal abstract void GetNamespacesToRender (XmlElement element, SortedList attrListToRender, SortedList nsListToRender, Hashtable nsLocallyDeclared);

        internal void LoadUnrenderedNamespaces(Hashtable nsLocallyDeclared) {
            object[] attrs = new object[nsLocallyDeclared.Count];
            nsLocallyDeclared.Values.CopyTo(attrs, 0);
            foreach (object attr in attrs) {
                AddUnrendered((XmlAttribute) attr);
            }
        }

        internal void LoadRenderedNamespaces(SortedList nsRenderedList) {
            foreach (object attr in nsRenderedList.GetKeyList()) {
                AddRendered((XmlAttribute) attr);
            }
        }

        internal void AddRendered(XmlAttribute attr) {
            GetCurrentScope().AddRendered(attr);
        }

        internal void AddUnrendered(XmlAttribute attr) {
            GetCurrentScope().AddUnrendered(attr);
        }
    }
}
