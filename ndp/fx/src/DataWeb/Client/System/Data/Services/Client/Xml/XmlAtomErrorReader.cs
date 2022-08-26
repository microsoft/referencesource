//---------------------------------------------------------------------
// <copyright file="XmlAtomErrorReader.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a wrapping XmlReader that can detect in-line errors.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Client.Xml
{
    #region Namespaces.

    using System.Diagnostics;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>Use this class to wrap an existing <see cref="XmlReader"/>.</summary>
    [DebuggerDisplay("XmlAtomErrorReader {NodeType} {Name} {Value}")]
    internal class XmlAtomErrorReader : XmlWrappingReader
    {
        /// <summary>Initializes a new <see cref="XmlAtomErrorReader"/> instance.</summary>
        /// <param name="baseReader">Reader to wrap.</param>
        internal XmlAtomErrorReader(XmlReader baseReader) : base(baseReader)
        {
            Debug.Assert(baseReader != null, "baseReader != null");
            this.Reader = baseReader;
        }

        #region Methods.

        /// <summary>Reads the next node from the stream.</summary>
        /// <returns>true if the next node was read successfully; false if there are no more nodes to read.</returns>
        public override bool Read()
        {
            bool result = base.Read();

            if (this.NodeType == XmlNodeType.Element &&
                Util.AreSame(this.Reader, XmlConstants.XmlErrorElementName, XmlConstants.DataWebMetadataNamespace))
            {
                string message = ReadErrorMessage(this.Reader);

                // In case of instream errors, the status code should be 500 (which is the default)
                throw new DataServiceClientException(Strings.Deserialize_ServerException(message));
            }

            return result;
        }

        /// <summary>Reads an element string from the specified <paramref name="reader"/>.</summary>
        /// <param name="reader">Reader to get value from.</param>
        /// <param name="checkNullAttribute">Whether a null attribute marker should be checked on the element.</param>
        /// <returns>The text value within the element, possibly null.</returns>
        /// <remarks>
        /// Simple values only are expected - mixed content will throw an error.
        /// Interspersed comments are ignored.
        /// </remarks>
        internal static string ReadElementString(XmlReader reader, bool checkNullAttribute)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(XmlNodeType.Element == reader.NodeType, "not positioned on Element");

            string result = null;
            bool empty = checkNullAttribute && !Util.DoesNullAttributeSayTrue(reader);

            if (reader.IsEmptyElement)
            {
                return (empty ? String.Empty : null);
            }

            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.EndElement:
                        return result ?? (empty ? String.Empty : null);
                    case XmlNodeType.CDATA:
                    case XmlNodeType.Text:
                    case XmlNodeType.SignificantWhitespace:
                        if (null != result)
                        {
                            throw Error.InvalidOperation(Strings.Deserialize_MixedTextWithComment);
                        }

                        result = reader.Value;
                        break;
                    case XmlNodeType.Comment:
                    case XmlNodeType.Whitespace:
                        break;
                    case XmlNodeType.Element:
                    default:
                        throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
                }
            }

            // xml ended before EndElement?
            throw Error.InvalidOperation(Strings.Deserialize_ExpectingSimpleValue);
        }

        /// <summary>With the reader positioned on an 'error' element, reads the text of the 'message' child.</summary>
        /// <param name="reader"><see cref="XmlReader"/> from which to read a WCF Data Service inline error message.</param>
        /// <returns>The text of the 'message' child element, empty if not found.</returns>
        private static string ReadErrorMessage(XmlReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            Debug.Assert(reader.NodeType == XmlNodeType.Element, "reader.NodeType == XmlNodeType.Element");
            Debug.Assert(reader.LocalName == XmlConstants.XmlErrorElementName, "reader.LocalName == XmlConstants.XmlErrorElementName");

            int depth = 1;
            while (depth > 0 && reader.Read())
            {
                if (reader.NodeType == XmlNodeType.Element)
                {
                    if (!reader.IsEmptyElement)
                    {
                        depth++;
                    }

                    if (depth == 2 &&
                        Util.AreSame(reader, XmlConstants.XmlErrorMessageElementName, XmlConstants.DataWebMetadataNamespace))
                    {
                        return ReadElementString(reader, false);
                    }
                }
                else if (reader.NodeType == XmlNodeType.EndElement)
                {
                    depth--;
                }
            }

            return String.Empty;
        }

        #endregion Methods.
    }
}
