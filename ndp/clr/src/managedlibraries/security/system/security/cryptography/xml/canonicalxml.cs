// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// CanonicalXml.cs
//

namespace System.Security.Cryptography.Xml {
    using System.Xml;
    using System.IO;
    using System.Text;
    using System.Collections;

    internal class CanonicalXml {
        private CanonicalXmlDocument m_c14nDoc;
        private C14NAncestralNamespaceContextManager m_ancMgr;

        // private static String defaultXPathWithoutComments = "(//. | //@* | //namespace::*)[not(self::comment())]";
        // private static String defaultXPathWithoutComments = "(//. | //@* | //namespace::*)";
        // private static String defaultXPathWithComments = "(//. | //@* | //namespace::*)";
        // private static String defaultXPathWithComments = "(//. | //@* | //namespace::*)";

        internal CanonicalXml(Stream inputStream, bool includeComments, XmlResolver resolver, string strBaseUri) {
            if (inputStream == null)
                throw new ArgumentNullException("inputStream");

            m_c14nDoc = new CanonicalXmlDocument(true, includeComments);
            m_c14nDoc.XmlResolver = resolver;
            m_c14nDoc.Load(Utils.PreProcessStreamInput(inputStream, resolver, strBaseUri));
            m_ancMgr = new C14NAncestralNamespaceContextManager();
        }

        internal CanonicalXml(XmlDocument document, XmlResolver resolver) : this(document, resolver, false) {}
        internal CanonicalXml(XmlDocument document, XmlResolver resolver, bool includeComments) {
            if (document == null)
                throw new ArgumentNullException("document");

            m_c14nDoc = new CanonicalXmlDocument(true, includeComments);
            m_c14nDoc.XmlResolver = resolver;
            m_c14nDoc.Load(new XmlNodeReader(document));
            m_ancMgr = new C14NAncestralNamespaceContextManager();
        }

        internal CanonicalXml(XmlNodeList nodeList, XmlResolver resolver, bool includeComments) {
            if (nodeList == null)
                throw new ArgumentNullException("nodeList");

            XmlDocument doc = Utils.GetOwnerDocument(nodeList);
            if (doc == null)
                throw new ArgumentException("nodeList");

            m_c14nDoc = new CanonicalXmlDocument(false, includeComments);
            m_c14nDoc.XmlResolver = resolver;
            m_c14nDoc.Load(new XmlNodeReader(doc));
            m_ancMgr = new C14NAncestralNamespaceContextManager();

            MarkInclusionStateForNodes(nodeList, doc, m_c14nDoc);
        }

        static void MarkNodeAsIncluded(XmlNode node) {
            if (node is ICanonicalizableNode)
               ((ICanonicalizableNode) node).IsInNodeSet = true;
        }

        private static void MarkInclusionStateForNodes(XmlNodeList nodeList, XmlDocument inputRoot, XmlDocument root) {
            CanonicalXmlNodeList elementList = new CanonicalXmlNodeList();
            CanonicalXmlNodeList elementListCanonical = new CanonicalXmlNodeList();
            elementList.Add(inputRoot);
            elementListCanonical.Add(root);
            int index = 0;

            do {
                XmlNode currentNode = (XmlNode) elementList[index];
                XmlNode currentNodeCanonical = (XmlNode) elementListCanonical[index];
                XmlNodeList childNodes = currentNode.ChildNodes;
                XmlNodeList childNodesCanonical = currentNodeCanonical.ChildNodes;
                for (int i = 0; i < childNodes.Count; i++) {
                    elementList.Add(childNodes[i]);
                    elementListCanonical.Add(childNodesCanonical[i]);

                    if (Utils.NodeInList(childNodes[i], nodeList)) {
                        MarkNodeAsIncluded(childNodesCanonical[i]);
                    }

                    XmlAttributeCollection attribNodes = childNodes[i].Attributes;
                    if (attribNodes != null) {
                        for (int j = 0; j < attribNodes.Count; j++) {
                            if (Utils.NodeInList(attribNodes[j], nodeList)) {
                                MarkNodeAsIncluded(childNodesCanonical[i].Attributes.Item(j));
                            }
                        }
                    }
                }
                index++;
            } while (index < elementList.Count);
        }

        internal byte[] GetBytes() {
            StringBuilder sb = new StringBuilder();
            m_c14nDoc.Write(sb, DocPosition.BeforeRootElement, m_ancMgr);
            UTF8Encoding utf8 = new UTF8Encoding(false);
            return utf8.GetBytes(sb.ToString());
        }

        internal byte[] GetDigestedBytes(HashAlgorithm hash) {
            m_c14nDoc.WriteHash(hash, DocPosition.BeforeRootElement, m_ancMgr);
            hash.TransformFinalBlock(new byte[0], 0, 0);
            byte[] res = (byte[]) hash.Hash.Clone();
            // reinitialize the hash so it is still usable after the call
            hash.Initialize();
            return res;
        }
    }

    // the stack of currently active NamespaceFrame contexts. this
    // object also maintains the inclusive prefix list in a tokenized form.
    internal class C14NAncestralNamespaceContextManager : AncestralNamespaceContextManager {
        internal C14NAncestralNamespaceContextManager () {}

        private void GetNamespaceToRender(string nsPrefix, SortedList attrListToRender, SortedList nsListToRender, Hashtable nsLocallyDeclared) {
            foreach (object a in nsListToRender.GetKeyList()) {
                if (Utils.HasNamespacePrefix((XmlAttribute) a, nsPrefix))
                    return;
            }
            foreach (object a in attrListToRender.GetKeyList()) {
                if (((XmlAttribute) a).LocalName.Equals(nsPrefix))
                    return;
            }

            int rDepth;
            XmlAttribute local = (XmlAttribute) nsLocallyDeclared[nsPrefix];
            XmlAttribute rAncestral = GetNearestRenderedNamespaceWithMatchingPrefix(nsPrefix, out rDepth);
            if(local != null) {
                if(Utils.IsNonRedundantNamespaceDecl(local, rAncestral)) {
                    nsLocallyDeclared.Remove(nsPrefix);
                    if (Utils.IsXmlNamespaceNode(local))
                        attrListToRender.Add(local, null);
                    else
                        nsListToRender.Add(local, null);
                }
            } else {
                int uDepth;
                XmlAttribute uAncestral = GetNearestUnrenderedNamespaceWithMatchingPrefix(nsPrefix, out uDepth);
                if (uAncestral != null && uDepth > rDepth && Utils.IsNonRedundantNamespaceDecl(uAncestral, rAncestral)) {
                    if(Utils.IsXmlNamespaceNode(uAncestral))
                        attrListToRender.Add(uAncestral, null);
                    else
                        nsListToRender.Add(uAncestral,null);
                }
            }
        }

        internal override void GetNamespacesToRender (XmlElement element, SortedList attrListToRender, SortedList nsListToRender, Hashtable nsLocallyDeclared) {
            XmlAttribute attrib = null;
            object[] attrs = new object[nsLocallyDeclared.Count];
            nsLocallyDeclared.Values.CopyTo(attrs,0);
            foreach(Object a in attrs) {
                attrib = (XmlAttribute) a;
                int rDepth;
                XmlAttribute rAncestral = GetNearestRenderedNamespaceWithMatchingPrefix(Utils.GetNamespacePrefix(attrib), out rDepth);                
                if(Utils.IsNonRedundantNamespaceDecl(attrib, rAncestral)) {
                    nsLocallyDeclared.Remove(Utils.GetNamespacePrefix(attrib));
                    if (Utils.IsXmlNamespaceNode(attrib))
                        attrListToRender.Add(attrib, null);
                    else
                        nsListToRender.Add(attrib, null);
                }
            }

            for (int i = m_ancestorStack.Count - 1; i >= 0; i--) {
                foreach (Object a in GetScopeAt(i).GetUnrendered().Values) {
                    attrib = (XmlAttribute) a;
                    if (attrib != null)
                        GetNamespaceToRender(Utils.GetNamespacePrefix(attrib), attrListToRender, nsListToRender, nsLocallyDeclared);
                }
            }
        }

        internal override void TrackNamespaceNode(XmlAttribute attr, SortedList nsListToRender, Hashtable nsLocallyDeclared) {
            nsLocallyDeclared.Add(Utils.GetNamespacePrefix(attr), attr);
        }

        internal override void TrackXmlNamespaceNode(XmlAttribute attr, SortedList nsListToRender, SortedList attrListToRender, Hashtable nsLocallyDeclared) {
            nsLocallyDeclared.Add(Utils.GetNamespacePrefix(attr), attr);
        }
    }
}
