//---------------------------------------------------------------------
// <copyright file="DictionaryContent.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a syndication content that holds name/value pairs.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System.Collections.Generic;
    using System.Diagnostics;
    using System.ServiceModel.Syndication;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>
    /// Use this class to hold the name/value pairs of content that will be written 
    /// to the content element of a syndication item.
    /// </summary>
    internal class DictionaryContent : SyndicationContent
    {
        #region Private fields.

        /// <summary>Content for a property value: one of a string, dictionary or null.</summary>
        private List<object> valueContents;

        /// <summary>Names for property values.</summary>
        private List<string> valueNames;

        /// <summary>Declared type names for property values.</summary>
        private List<string> valueTypes;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new DictionaryContent instance.</summary>
        public DictionaryContent()
        {
            this.valueContents = new List<object>();
            this.valueTypes = new List<string>();
            this.valueNames = new List<string>();
        }

        /// <summary>Initializes a new DictionaryContent instance.</summary>
        /// <param name='capacity'>Initial capacity for entries.</param>
        public DictionaryContent(int capacity)
        {
            this.valueContents = new List<object>(capacity);
            this.valueTypes = new List<string>(capacity);
            this.valueNames = new List<string>(capacity);
        }

        /// <summary>
        /// Initializes a new DictionaryContent instance by copying values from 
        /// the specified one.
        /// </summary>
        /// <param name="other">Dictionary to copy content from.</param>
        /// <remarks>This produces a shallow copy only.</remarks>
        private DictionaryContent(DictionaryContent other)
        {
            Debug.Assert(other != null, "other != null");
            this.valueContents = other.valueContents;
            this.valueTypes = other.valueTypes;
            this.valueNames = other.valueNames;
        }

        #endregion Constructors.

        #region Properties.

        /// <summary>The MIME type of this content.</summary>
        public override string Type
        {
            get { return XmlConstants.MimeApplicationXml; }
        }

        /// <summary>True if there is no properties to serialize out.</summary>
        public bool IsEmpty
        {
            get { return this.valueNames.Count == 0; }
        }

        #endregion Properties.

        #region Methods.

        /// <summary>Creates a shallow copy of this content.</summary>
        /// <returns>A shallow copy of this content.</returns>
        public override SyndicationContent Clone()
        {
            return new DictionaryContent(this);
        }

        /// <summary>Adds the specified property.</summary>
        /// <param name="name">Property name.</param>
        /// <param name="expectedTypeName">Expected type name for value.</param>
        /// <param name="value">Property value in text form.</param>
        internal void Add(string name, string expectedTypeName, object value)
        {
            Debug.Assert(value != null, "value != null -- otherwise AddNull should have been called.");
            Debug.Assert(
                value is DictionaryContent || value is string,
                "value is DictionaryContent || value is string -- only sub-dictionaries and formatted strings are allowed.");
            Debug.Assert(!this.valueNames.Contains(name), "!this.valueNames.Contains(name) -- otherwise repeated property set.");
            this.valueNames.Add(name);
            this.valueTypes.Add(expectedTypeName);
            this.valueContents.Add(value);
            Debug.Assert(this.valueNames.Count == this.valueTypes.Count, "this.valueNames.Count == this.valueTypes.Count");
            Debug.Assert(this.valueNames.Count == this.valueContents.Count, "this.valueNames.Count == this.valueContents.Count");
        }

        /// <summary>Adds a property with a null value.</summary>
        /// <param name="expectedTypeName">Type for the property.</param>
        /// <param name="name">Property name.</param>
        internal void AddNull(string expectedTypeName, string name)
        {
            Debug.Assert(!this.valueNames.Contains(name), "!this.valueNames.Contains(name) -- otherwise repeated property set.");
            this.valueNames.Add(name);
            this.valueTypes.Add(expectedTypeName);
            this.valueContents.Add(null);
            Debug.Assert(this.valueNames.Count == this.valueTypes.Count, "this.valueNames.Count == this.valueTypes.Count");
            Debug.Assert(this.valueNames.Count == this.valueContents.Count, "this.valueNames.Count == this.valueContents.Count");
        }

        /// <summary>
        /// Gets a XmlReader to the property contents.
        /// </summary>
        /// <returns>XmlReader for the property contents.</returns>
        internal XmlReader GetPropertyContentsReader()
        {
            Debug.Assert(!this.IsEmpty, "!this.IsEmpty -- calling with 0 properties is disallowed.");

            System.IO.MemoryStream stream = new System.IO.MemoryStream();
            using (XmlWriter writer = XmlWriter.Create(stream))
            {
                writer.WriteStartElement(XmlConstants.DataWebMetadataNamespacePrefix, XmlConstants.AtomPropertiesElementName, XmlConstants.DataWebMetadataNamespace);
                writer.WriteAttributeString(XmlConstants.XmlnsNamespacePrefix, XmlConstants.DataWebNamespacePrefix, null, XmlConstants.DataWebNamespace);
                this.WritePropertyContentsTo(writer);
                writer.WriteEndElement();
                writer.Flush();
                stream.Position = 0;
                return XmlReader.Create(stream);
            }
        }

        /// <summary>
        /// Looks up the dictionary content value for the property bag for complex property 
        /// </summary>
        /// <param name="propertyName">Name of property to lookup</param>
        /// <param name="found">Was content found</param>
        /// <returns>The property bag or null if the property never existed</returns>
        internal DictionaryContent Lookup(string propertyName, out bool found)
        {
            int index = this.valueNames.IndexOf(propertyName);
            if (index >= 0)
            {
                found = true;
                object value = this.valueContents[index];
                if (value != null)
                {
                    Debug.Assert(value is DictionaryContent, "Must only look for complex properties, primitives must not be found");
                    return value as DictionaryContent;
                }
                else
                {
                    // It might have been the case that the complex property was itself null
                    return null;
                }
            }
            else
            {
                found = false;
                return null;
            }
        }

        /// <summary>Writes the contents of this SyndicationContent object to the specified XmlWriter.</summary>
        /// <param name='writer'>The XmlWriter to write to.</param>
        protected override void WriteContentsTo(XmlWriter writer)
        {
            if (0 < this.valueNames.Count)
            {
                writer.WriteStartElement(XmlConstants.AtomPropertiesElementName, XmlConstants.DataWebMetadataNamespace);
                this.WritePropertyContentsTo(writer);
                writer.WriteEndElement();
            }
        }

        /// <summary>Writes the contents of this SyndicationContent object to the specified XmlWriter.</summary>
        /// <param name='writer'>The XmlWriter to write to.</param>
        private void WritePropertyContentsTo(XmlWriter writer)
        {
            for (int i = 0; i < this.valueNames.Count; i++)
            {
                string propertyName = this.valueNames[i];
                string propertyTypeName = this.valueTypes[i];
                object propertyValue = this.valueContents[i];
                if (propertyValue == null)
                {
                    PlainXmlSerializer.WriteNullValue(writer, propertyName, propertyTypeName);
                }
                else if (propertyValue is DictionaryContent)
                {
                    PlainXmlSerializer.WriteStartElementWithType(writer, propertyName, propertyTypeName);
                    ((DictionaryContent)propertyValue).WritePropertyContentsTo(writer);
                    writer.WriteEndElement();
                }
                else
                {
                    string propertyText = (string)propertyValue;
                    PlainXmlSerializer.WriteTextValue(writer, propertyName, propertyTypeName, propertyText);
                }
            }
        }

        #endregion Methods.
    }
}
