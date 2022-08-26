// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// <OWNER>Microsoft</OWNER>
// 

//
// DataObject.cs
//
// 21 Microsoft 2000
// 

namespace System.Security.Cryptography.Xml
{
    using System;
    using System.IO;
    using System.Xml;

    [System.Security.Permissions.HostProtection(MayLeakOnAbort = true)]
    public class DataObject {
        private string m_id;
        private string m_mimeType;
        private string m_encoding;
        private CanonicalXmlNodeList m_elData;
        private XmlElement m_cachedXml;

        //
        // public constructors
        //

        public DataObject () {
            m_cachedXml = null;
            m_elData = new CanonicalXmlNodeList();
        }

        public DataObject (string id, string mimeType, string encoding, XmlElement data) {
            if (data == null) 
                throw new ArgumentNullException("data");

            m_id = id;
            m_mimeType = mimeType;
            m_encoding = encoding;
            m_elData = new CanonicalXmlNodeList();
            m_elData.Add(data);
            m_cachedXml = null;
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

        public string MimeType {
            get { return m_mimeType; }
            set {
                m_mimeType = value; 
                m_cachedXml = null;
            }
        }

        public string Encoding {
            get { return m_encoding; }
            set {
                m_encoding = value;
                m_cachedXml = null;
            }
        }

        public XmlNodeList Data {
            get { return m_elData; }
            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                // Reset the node list
                m_elData = new CanonicalXmlNodeList();
                foreach (XmlNode node in value) {
                    m_elData.Add(node);
                }
                m_cachedXml = null;
            }
        }

        private bool CacheValid {
            get { 
                return(m_cachedXml != null);
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
            XmlElement objectElement = document.CreateElement("Object", SignedXml.XmlDsigNamespaceUrl);

            if (!String.IsNullOrEmpty(m_id))
                objectElement.SetAttribute("Id", m_id);
            if (!String.IsNullOrEmpty(m_mimeType))
                objectElement.SetAttribute("MimeType", m_mimeType);
            if (!String.IsNullOrEmpty(m_encoding))
                objectElement.SetAttribute("Encoding", m_encoding);

            if (m_elData != null) {
                foreach (XmlNode node in m_elData) {
                    objectElement.AppendChild(document.ImportNode(node, true));
                }
            }

            return objectElement;
        }

        public void LoadXml (XmlElement value) {
            if (value == null)
                throw new ArgumentNullException("value");

            m_id = Utils.GetAttribute(value, "Id", SignedXml.XmlDsigNamespaceUrl);
            m_mimeType = Utils.GetAttribute(value, "MimeType", SignedXml.XmlDsigNamespaceUrl);
            m_encoding = Utils.GetAttribute(value, "Encoding", SignedXml.XmlDsigNamespaceUrl);

            foreach (XmlNode node in value.ChildNodes) {
                m_elData.Add(node);
            }

            // Save away the cached value
            m_cachedXml = value;
        }
    }
}
