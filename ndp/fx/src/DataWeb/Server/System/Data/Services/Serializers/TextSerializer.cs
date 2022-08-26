//---------------------------------------------------------------------
// <copyright file="TextSerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for text content.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>
    /// Provides support for serializing responses in text format.
    /// </summary>
    internal struct TextSerializer : IExceptionWriter
    {
        /// <summary>Writer to which output is sent.</summary>
        private readonly TextWriter writer;

        /// <summary>Initializes a new <see cref="TextSerializer"/> for the specified stream.</summary>
        /// <param name="output">Stream to which output should be sent.</param>
        /// <param name="encoding">Encoding to be used to write the result.</param>
        internal TextSerializer(Stream output, Encoding encoding)
        {
            Debug.Assert(output != null, "output != null");
            Debug.Assert(encoding != null, "encoding != null");

            this.writer = new StreamWriter(output, encoding);
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public void WriteException(HandleExceptionArgs args)
        {
            XmlWriter xmlWriter = XmlWriter.Create(this.writer);
            ErrorHandler.SerializeXmlError(args, xmlWriter);
            this.writer.Flush();
        }

        /// <summary>Handles the complete serialization for the specified content.</summary>
        /// <param name="content">Single Content to write..</param>
        /// <remarks><paramref name="content"/> should be a byte array.</remarks>
        internal void WriteRequest(object content)
        {
            Debug.Assert(content != null, "content != null");

            string contentAsText;
            if (!System.Data.Services.Parsing.WebConvert.TryXmlPrimitiveToString(content, out contentAsText))
            {
                throw new InvalidOperationException(Strings.Serializer_CannotConvertValue(content));
            }

            Debug.Assert(contentAsText != null, "contentAsText != null");

            this.writer.Write(contentAsText);
            this.writer.Flush();
        }
    }
}
