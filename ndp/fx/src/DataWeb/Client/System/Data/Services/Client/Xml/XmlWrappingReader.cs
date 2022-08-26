//---------------------------------------------------------------------
// <copyright file="XmlWrappingReader.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class used to wrap XmlReader instances.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client.Xml
{
    #region Namespaces.

    using System.Xml;
    using System.Xml.Schema;
    using System.Collections.Generic;
    using System.Diagnostics;

    #endregion Namespaces.

    /// <summary>Use this class to wrap an existing <see cref="XmlReader"/>.</summary>
    internal class XmlWrappingReader : XmlReader, IXmlLineInfo
    {
        #region Private fields.

        /// <summary>Wrapper reader.</summary>
        private XmlReader reader;

        /// <summary>Line information of the wrapper reader (possibly null).</summary>
        private IXmlLineInfo readerAsIXmlLineInfo;

        /// <summary>stack to keep track of the base uri.</summary>
        private Stack<XmlBaseState> xmlBaseStack;

        /// <summary>while creating a nested reader, we need to use the base uri from the previous reader. This field is used for that purpose.</summary>
        private string previousReaderBaseUri;

        #endregion Private fields.

        /// <summary>Initializes a new <see cref="XmlWrappingReader"/> instance.</summary>
        /// <param name="baseReader">Reader to wrap.</param>
        internal XmlWrappingReader(XmlReader baseReader)
        {
            this.Reader = baseReader;
        }

        #region Properties.

        /// <summary>Gets the number of attributes on the current node.</summary>
        public override int AttributeCount
        {
            get
            {
                return this.reader.AttributeCount;
            }
        }

        /// <summary>Gets the base URI of the current node.</summary>
        public override string BaseURI
        {
            get
            {
                if (this.xmlBaseStack != null && this.xmlBaseStack.Count > 0)
                {
                    return this.xmlBaseStack.Peek().BaseUri.AbsoluteUri;
                }
                else if (!String.IsNullOrEmpty(this.previousReaderBaseUri))
                {
                    return this.previousReaderBaseUri;
                }

                return this.reader.BaseURI;
            }
        }

        /// <summary>Gets a value indicating whether this reader can parse and resolve entities.</summary>
        public override bool CanResolveEntity
        {
            get
            {
                return this.reader.CanResolveEntity;
            }
        }

        /// <summary>Gets the depth of the current node in the XML document.</summary>
        public override int Depth
        {
            get
            {
                return this.reader.Depth;
            }
        }

        // NOTE: there is no support for wrapping the DtdSchemaInfo property.

        /// <summary>Gets a value indicating whether the reader is positioned at the end of the stream.</summary>
        public override bool EOF
        {
            get
            {
                return this.reader.EOF;
            }
        }

        /// <summary>Gets a value indicating whether the current node has any attributes.</summary>
        public override bool HasAttributes
        {
            get
            {
                return this.reader.HasAttributes;
            }
        }

        /// <summary>Gets a value indicating whether the current node can have a <see cref="Value"/>.</summary>
        public override bool HasValue
        {
            get
            {
                return this.reader.HasValue;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current node is an attribute that was generated from the default value 
        /// defined in the DTD or schema.
        /// </summary>
        public override bool IsDefault
        {
            get
            {
                return this.reader.IsDefault;
            }
        }

        /// <summary>Gets a value indicating whether the current node is an empty element.</summary>
        public override bool IsEmptyElement
        {
            get
            {
                return this.reader.IsEmptyElement;
            }
        }

        /// <summary>Gets the current line number.</summary>
        public virtual int LineNumber
        {
            get
            {
                if (this.readerAsIXmlLineInfo != null)
                {
                    return this.readerAsIXmlLineInfo.LineNumber;
                }

                return 0;
            }
        }

        /// <summary>Gets the current line position.</summary>
        public virtual int LinePosition
        {
            get
            {
                if (this.readerAsIXmlLineInfo != null)
                {
                    return this.readerAsIXmlLineInfo.LinePosition;
                }

                return 0;
            }
        }

        /// <summary>Gets the local name of the current node.</summary>
        public override string LocalName
        {
            get
            {
                return this.reader.LocalName;
            }
        }

        /// <summary>Gets the qualified name of the current node.</summary>
        public override string Name
        {
            get
            {
                return this.reader.Name;
            }
        }

        /// <summary>Gets the namespace URI (as defined in the W3C Namespace specification) of the node on which the reader is positioned.</summary>
        public override string NamespaceURI
        {
            get
            {
                return this.reader.NamespaceURI;
            }
        }

        /// <summary>Gets the XmlNameTable associated with this implementation.</summary>
        public override XmlNameTable NameTable
        {
            get
            {
                return this.reader.NameTable;
            }
        }

        /// <summary>Gets the type of the current node.</summary>
        public override XmlNodeType NodeType
        {
            get
            {
                return this.reader.NodeType;
            }
        }

        /// <summary>Gets the namespace prefix associated with the current node.</summary>
        public override string Prefix
        {
            get
            {
                return this.reader.Prefix;
            }
        }

#if !ASTORIA_LIGHT

        /// <summary>Gets the quotation mark character used to enclose the value of an attribute node.</summary>
        public override char QuoteChar
        {
            get
            {
                return this.reader.QuoteChar;
            }
        }

#endif

        /// <summary>Gets the state of the reader.</summary>
        public override ReadState ReadState
        {
            get
            {
                return this.reader.ReadState;
            }
        }

#if !ASTORIA_LIGHT

        /// <summary>Gets the schema information that has been assigned to the current node as a result of schema validation.</summary>
        public override IXmlSchemaInfo SchemaInfo
        {
            get
            {
                return this.reader.SchemaInfo;
            }
        }
#endif

        /// <summary>Gets the XmlReaderSettings object used to create this XmlReader instance.</summary>
        public override XmlReaderSettings Settings
        {
            get
            {
                return this.reader.Settings;
            }
        }

        /// <summary>Gets the text value of the current node.</summary>
        public override string Value
        {
            get
            {
                return this.reader.Value;
            }
        }

        /// <summary>Gets the Common Language Runtime (CLR) type for the current node.</summary>
        public override Type ValueType
        {
            get
            {
                return this.reader.ValueType;
            }
        }

        /// <summary>Gets the current xml:lang scope.</summary>
        public override string XmlLang
        {
            get
            {
                return this.reader.XmlLang;
            }
        }

        /// <summary>Gets the current xml:space scope.</summary>
        public override XmlSpace XmlSpace
        {
            get
            {
                return this.reader.XmlSpace;
            }
        }

        /// <summary>Wrapped reader.</summary>
        protected XmlReader Reader
        {
            get
            {
                return this.reader;
            }

            set
            {
                this.reader = value;
                this.readerAsIXmlLineInfo = value as IXmlLineInfo;
            }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Changes the ReadState to Closed.</summary>
        public override void Close()
        {
            this.reader.Close();
        }

        /// <summary>Gets the value of the attribute with the specified index.</summary>
        /// <param name="i">The index of the attribute. The index is zero-based. (The first attribute has index 0.)</param>
        /// <returns>The value of the specified attribute. This method does not move the reader.</returns>
        public override string GetAttribute(int i)
        {
            return this.reader.GetAttribute(i);
        }

        /// <summary>Gets the value of the attribute with the specified name.</summary>
        /// <param name="name">The qualified name of the attribute.</param>
        /// <returns>
        /// The value of the specified attribute. If the attribute is not found or the value is String.Empty, 
        /// a null reference is returned.
        /// </returns>
        public override string GetAttribute(string name)
        {
            return this.reader.GetAttribute(name);
        }

        /// <summary>Gets the value of the attribute with the specified index.</summary>
        /// <param name="name">The local name of the attribute.</param>
        /// <param name="namespaceURI">The namespace URI of the attribute.</param>
        /// <returns>
        /// The value of the specified attribute. If the attribute is not found or the value is String.Empty, 
        /// a null reference is returned.
        /// </returns>
        public override string GetAttribute(string name, string namespaceURI)
        {
            return this.reader.GetAttribute(name, namespaceURI);
        }

        /// <summary>Gets a value indicating whether the class can return line information.</summary>
        /// <returns>true if LineNumber and LinePosition can be provided; otherwise, false.</returns>
        public virtual bool HasLineInfo()
        {
            return ((this.readerAsIXmlLineInfo != null) && this.readerAsIXmlLineInfo.HasLineInfo());
        }

        /// <summary>Resolves a namespace prefix in the current element's scope.</summary>
        /// <param name="prefix">
        /// The prefix whose namespace URI you want to resolve. To match the default namespace, pass an empty string.
        /// </param>
        /// <returns>The namespace URI to which the prefix maps or a null reference.</returns>
        public override string LookupNamespace(string prefix)
        {
            return this.reader.LookupNamespace(prefix);
        }

        /// <summary>Moves to the attribute with the specified index.</summary>
        /// <param name="i">The index of the attribute.</param>
        public override void MoveToAttribute(int i)
        {
            this.reader.MoveToAttribute(i);
        }

        /// <summary>Moves to the attribute with the specified name.</summary>
        /// <param name="name">The qualified name of the attribute.</param>
        /// <returns>
        /// true if the attribute is found; otherwise, false. If false, the reader's position does not change.
        /// </returns>
        public override bool MoveToAttribute(string name)
        {
            return this.reader.MoveToAttribute(name);
        }

        /// <summary>Moves to the attribute with the specified name.</summary>
        /// <param name="name">The local name of the attribute.</param>
        /// <param name="ns">The namespace URI of the attribute.</param>
        /// <returns>
        /// true if the attribute is found; otherwise, false. If false, the reader's position does not change.
        /// </returns>
        public override bool MoveToAttribute(string name, string ns)
        {
            return this.reader.MoveToAttribute(name, ns);
        }

        /// <summary>Moves to the element that contains the current attribute node.</summary>
        /// <returns>
        /// true if the reader is positioned on an attribute (the reader moves to the element that owns the attribute); 
        /// false if the reader is not positioned on an attribute (the position of the reader does not change).
        /// </returns>
        public override bool MoveToElement()
        {
            return this.reader.MoveToElement();
        }

        /// <summary>Moves to the first attribute.</summary>
        /// <returns>
        /// true if an attribute exists (the reader moves to the first attribute); otherwise, false (the position of 
        /// the reader does not change).
        /// </returns>
        public override bool MoveToFirstAttribute()
        {
            return this.reader.MoveToFirstAttribute();
        }

        /// <summary>Moves to the next attribute.</summary>
        /// <returns>true if there is a next attribute; false if there are no more attributes.</returns>
        public override bool MoveToNextAttribute()
        {
            return this.reader.MoveToNextAttribute();
        }

        /// <summary>Reads the next node from the stream.</summary>
        /// <returns>true if the next node was read successfully; false if there are no more nodes to read.</returns>
        public override bool Read()
        {
            if (this.reader.NodeType == XmlNodeType.EndElement)
            {
                this.PopXmlBase();
            }
            else
            {
                // If the xmlreader is located at the attributes, IsEmptyElement will always return false.
                // Hence we need to call MoveToElement first, before checking for IsEmptyElement
                this.reader.MoveToElement();
                if (this.reader.IsEmptyElement)
                {
                    this.PopXmlBase();
                }
            }

            bool result = this.reader.Read();
            if (result) 
            {
                if (this.reader.NodeType == XmlNodeType.Element &&
                    this.reader.HasAttributes) 
                {
                    string baseAttribute = this.reader.GetAttribute(XmlConstants.XmlBaseAttributeNameWithPrefix);
                    if (String.IsNullOrEmpty(baseAttribute))
                    {
                        // If there is no xml base attribute specified, don't do anything
                        return result;
                    }

                    Uri newBaseUri = null;
                    newBaseUri = Util.CreateUri(baseAttribute, UriKind.RelativeOrAbsolute);

                    // Initialize the stack first time you see the xml base uri
                    if (this.xmlBaseStack == null)
                    {
                        this.xmlBaseStack = new Stack<XmlBaseState>();
                    }

                    if (this.xmlBaseStack.Count > 0)
                    {
                        // If there is a xml:base attribute already specified, then the new xml base
                        // value must be relative to the previous xml base.
                        // For more information, look into section 3 of the following RFC
                        // http://www.w3.org/TR/2001/REC-xmlbase-20010627/
                        newBaseUri = Util.CreateUri(this.xmlBaseStack.Peek().BaseUri, newBaseUri);
                    }

                    // Push current state and allocate new one
                    this.xmlBaseStack.Push(new XmlBaseState(newBaseUri, this.reader.Depth));
                }
            }

            return result;
        }

        /// <summary>Parses the attribute value into one or more Text, EntityReference, or EndEntity nodes.</summary>
        /// <returns>true if there are nodes to return.</returns>
        public override bool ReadAttributeValue()
        {
            return this.reader.ReadAttributeValue();
        }

        /// <summary>Resolves the entity reference for EntityReference nodes.</summary>
        public override void ResolveEntity()
        {
            this.reader.ResolveEntity();
        }

        /// <summary>Skips the children of the current node.</summary>
        public override void Skip()
        {
            this.reader.Skip();
        }

        /// <summary>
        /// Creates a new instance of XmlWrappingReader instance which wraps the <paramref name="newReader"/>
        /// </summary>
        /// <param name="currentBaseUri">current base uri.</param>
        /// <param name="newReader">xml reader which needs to be wrapped.</param>
        /// <returns>a new instance of XmlWrappingReader.</returns>
        internal static XmlWrappingReader CreateReader(string currentBaseUri, XmlReader newReader)
        {
            Debug.Assert(!(newReader is XmlWrappingReader), "The new reader must not be a xmlWrappingReader");
            XmlWrappingReader reader = new XmlWrappingReader(newReader);
            reader.previousReaderBaseUri = currentBaseUri;
            return reader;
        }

        /// <summary>
        /// Releases the unmanaged resources used by the XmlReader and optionally releases the managed resources.
        /// </summary>
        /// <param name="disposing">
        /// true to release both managed and unmanaged resources; false to release only unmanaged resources.
        /// </param>
        protected override void Dispose(bool disposing)
        {
            if (this.reader != null)
            {
                ((IDisposable)this.reader).Dispose();
            }

            // XmlReader.Dispose(bool) does nothing, but for completeness w/ FxCop
            base.Dispose(disposing);
        }

        /// <summary>
        /// Pops the xml base from the top of the stack, if required.
        /// </summary>
        private void PopXmlBase()
        {
            if (this.xmlBaseStack != null && this.xmlBaseStack.Count > 0 && this.reader.Depth == this.xmlBaseStack.Peek().Depth)
            {
                this.xmlBaseStack.Pop();
            }
        }

        #endregion Methods.

        #region Private Class

        /// <summary>
        /// Private class to maintain the state information for keeping track of base uri's.
        /// </summary>
        private class XmlBaseState
        {
            /// <summary>
            /// Creates a new instance of the XmlBaseState class.
            /// </summary>
            /// <param name="baseUri">base uri for the given element.</param>
            /// <param name="depth">depth of the element.</param>
            internal XmlBaseState(Uri baseUri, int depth)
            {
                this.BaseUri = baseUri;
                this.Depth = depth;
            }

            /// <summary>base uri as specified in the xmml:base attribute of the given element.</summary>
            public Uri BaseUri
            {
                get;
                private set;
            }

            /// <summary>depth of the element.</summary>
            public int Depth
            {
                get;
                private set;
            }
        }

        #endregion // Private Class
    }
}
