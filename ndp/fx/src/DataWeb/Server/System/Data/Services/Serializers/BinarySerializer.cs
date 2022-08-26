//---------------------------------------------------------------------
// <copyright file="BinarySerializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a serializer for binary content.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System.Diagnostics;
    using System.IO;
    using System.Xml;

    /// <summary>Provides support for serializing responses in binary format.</summary>
    /// <remarks>
    /// The file histroy should show a BinaryExceptionTextWriter which is no longer used.
    /// </remarks>
    internal struct BinarySerializer : IExceptionWriter
    {
        /// <summary>Stream to which output is sent.</summary>
        private readonly Stream outputStream;

        /// <summary>Initializes a new <see cref="BinarySerializer"/> for the specified stream.</summary>
        /// <param name="output">Stream to which output should be sent.</param>
        internal BinarySerializer(Stream output)
        {
            Debug.Assert(output != null, "output != null");
            this.outputStream = output;
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public void WriteException(HandleExceptionArgs args)
        {
            Debug.Assert(args != null, "args != null");
            XmlWriter xmlWriter = XmlWriter.Create(this.outputStream);
            ErrorHandler.SerializeXmlError(args, xmlWriter);
            xmlWriter.Flush();
        }

        /// <summary>Handles the complete serialization for the specified content.</summary>
        /// <param name="content">Single Content to write..</param>
        /// <remarks><paramref name="content"/> should be a byte array.</remarks>
        internal void WriteRequest(object content)
        {
            Debug.Assert(content != null, "content != null");

            // The metadata layer should only accept byte arrays as binary-serialized values.
            byte[] bytes;
            if (content is byte[])
            {
                bytes = (byte[])content;
            }
            else
            {
                bytes = (byte[])((System.Data.Linq.Binary)content).ToArray();
            }

            this.outputStream.Write(bytes, 0, bytes.Length);
        }

        /// <summary>Handles the complete serialization for the specified stream.</summary>
        /// <param name="inputStream">Input stream to write out.</param>
        /// <param name="bufferSize">Buffer size to use during copying.</param>
        internal void WriteRequest(Stream inputStream, int bufferSize)
        {
            Debug.Assert(inputStream != null, "stream != null");
            WebUtil.CopyStream(inputStream, this.outputStream, bufferSize);
        }
    }
}
