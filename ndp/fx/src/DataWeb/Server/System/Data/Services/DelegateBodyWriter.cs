//---------------------------------------------------------------------
// <copyright file="DelegateBodyWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a BodyWriter that invokes a callback on writing.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Diagnostics;
    using System.IO;
    using System.ServiceModel.Channels;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>Use this class to handle writing body contents using a callback.</summary>
    internal class DelegateBodyWriter : BodyWriter
    {
        #region Fields.

        /// <summary>Service to dispose data source from once the response is written.</summary>
        private readonly IDataService service;

        /// <summary>Callback.</summary>
        private readonly Action<Stream> writerAction;

        #endregion Fields.

        /// <summary>Initializes a new <see cref="DelegateBodyWriter"/> instance.</summary>
        /// <param name="writer">Callback for writing.</param>
        /// <param name="service">Service to dispose data source from once the response is written.</param>
        internal DelegateBodyWriter(Action<Stream> writer, IDataService service)
            : base(false)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(writer != null, "writer != null");
            this.writerAction = writer;
            this.service = service;
        }

        /// <summary>Called when the message body is written to an XML file.</summary>
        /// <param name="writer">
        /// An <see cref="XmlDictionaryWriter"/> that is used to write this 
        /// message body to an XML file.
        /// </param>
        protected override void OnWriteBodyContents(XmlDictionaryWriter writer)
        {
            Debug.Assert(writer != null, "writer != null");

            try
            {
                writer.WriteStartElement(XmlConstants.WcfBinaryElementName);
                using (XmlWriterStream stream = new XmlWriterStream(writer))
                {
                    this.writerAction(stream);
                }

                writer.WriteEndElement();
            }
            finally
            {
                if (this.service != null)
                {
                    this.service.DisposeDataSource();
                    HttpContextServiceHost host = this.service.OperationContext.Host.HttpContextServiceHost;
                    if (host != null)
                    {
                        if (host.ErrorFound)
                        {
                            var ctx = System.ServiceModel.OperationContext.Current;
                            if (ctx != null)
                            {
                                ctx.Channel.Abort();
                            }
                        }
                    }
                }
            }
        }

        #region Inner types.

        /// <summary>Use this class to write to an <see cref="XmlDictionaryWriter"/>.</summary>
        internal class XmlWriterStream : Stream
        {
            /// <summary>Target writer.</summary>
            private XmlDictionaryWriter innerWriter;

            /// <summary>Initializes a new <see cref="XmlWriterStream"/> instance.</summary>
            /// <param name="xmlWriter">Target writer.</param>
            internal XmlWriterStream(XmlDictionaryWriter xmlWriter)
            {
                Debug.Assert(xmlWriter != null, "xmlWriter != null");
                this.innerWriter = xmlWriter;
            }

            /// <summary>Gets a value indicating whether the current stream supports reading.</summary>
            public override bool CanRead
            {
                get { return false; }
            }

            /// <summary>Gets a value indicating whether the current stream supports seeking.</summary>
            public override bool CanSeek
            {
                get { return false; }
            }

            /// <summary>Gets a value indicating whether the current stream supports writing.</summary>
            public override bool CanWrite
            {
                get { return true; }
            }

            /// <summary>Gets the length in bytes of the stream.</summary>
            public override long Length
            {
                get { throw Error.NotSupported(); }
            }

            /// <summary>Gets or sets the position within the current stream.</summary>
            public override long Position
            {
                get { throw Error.NotSupported(); }
                set { throw Error.NotSupported(); }
            }

            /// <summary>
            /// Clears all buffers for this stream and causes any buffered 
            /// data to be written to the underlying device.
            /// </summary>
            public override void Flush()
            {
                this.innerWriter.Flush();
            }

            /// <summary>
            /// Reads a sequence of bytes from the current stream and 
            /// advances the position within the stream by the number of bytes read.
            /// </summary>
            /// <param name="buffer">
            /// An array of bytes. When this method returns, the buffer contains 
            /// the specified byte array with the values between <paramref name="offset"/> 
            /// and (<paramref name="offset"/> + <paramref name="count"/> - 1) replaced 
            /// by the bytes read from the current source.
            /// </param>
            /// <param name="offset">
            /// The zero-based byte offset in <paramref name="buffer"/> at which to 
            /// begin storing the data read from the current stream.
            /// </param>
            /// <param name="count">
            /// The maximum number of bytes to be read from the current stream.
            /// </param>
            /// <returns>The total number of bytes read into the buffer.</returns>
            public override int Read(byte[] buffer, int offset, int count)
            {
                throw Error.NotSupported();
            }

            /// <summary>Sets the position within the current stream.</summary>
            /// <param name="offset">
            /// A byte offset relative to the <paramref name="origin"/> parameter.
            /// </param>
            /// <param name="origin">
            /// A value of type <see cref="SeekOrigin"/> indicating the reference 
            /// point used to obtain the new position.
            /// </param>
            /// <returns>The new position within the current stream.</returns>
            public override long Seek(long offset, SeekOrigin origin)
            {
                throw Error.NotSupported();
            }

            /// <summary>Sets the length of the current stream.</summary>
            /// <param name="value">New value for length.</param>
            public override void SetLength(long value)
            {
                throw Error.NotSupported();
            }

            /// <summary>
            /// Writes a sequence of bytes to the current stream and advances 
            /// the current position within this stream by the number of 
            /// bytes written. 
            /// </summary>
            /// <param name="buffer">
            /// An array of bytes. This method copies <paramref name="count"/> 
            /// bytes from <paramref name="buffer"/> to the current stream.
            /// </param>
            /// <param name="offset">
            /// The zero-based byte offset in buffer at which to begin copying 
            /// bytes to the current stream.
            /// </param>
            /// <param name="count">
            /// The number of bytes to be written to the current stream.
            /// </param>
            public override void Write(byte[] buffer, int offset, int count)
            {
                this.innerWriter.WriteBase64(buffer, offset, count);
            }
        }

        #endregion Inner types.
    }
}
