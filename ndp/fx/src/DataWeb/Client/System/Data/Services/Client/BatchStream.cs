//---------------------------------------------------------------------
// <copyright file="BatchStream.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// break a batch stream into its multiple parts
// text reading parts grabbed from System.IO.StreamReader
// </summary>
//---------------------------------------------------------------------

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;

#if ASTORIA_CLIENT
#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif
#endif

    /// <summary>
    /// materialize objects from an application/atom+xml stream
    /// </summary>
    internal class BatchStream : Stream
    {
        /// <summary>Default buffer size, should be larger than buffer size of StreamReader</summary> 
        private const int DefaultBufferSize = 8000;

        /// <summary>Is this a batch resquest or batch response</summary>
        private readonly bool batchRequest;

        /// <summary>Buffered bytes from the stream.</summary> 
        private readonly byte[] byteBuffer;

        /// <summary>Underlying stream being buffered.</summary> 
        private Stream reader;

        /// <summary>Number of valid bytes in the byteBuffer.</summary> 
        private int byteLength;

        /// <summary>Position in the byteBuffer.</summary> 
        private int bytePosition;

        /// <summary>Discovered encoding of underlying stream.</summary>
        private Encoding batchEncoding;

        /// <summary>check preamble.</summary>
        private bool checkPreamble;

        /// <summary>batch boundary.</summary>
        private string batchBoundary;

        /// <summary>batch length</summary>
        private int batchLength;

        /// <summary>running total byte count</summary>
        private int totalCount;

        /// <summary>changeset boundary.</summary>
        private string changesetBoundary;

        /// <summary>Discovered encoding of underlying neseted stream.</summary>
        private Encoding changesetEncoding;

        /// <summary>content headers</summary>
        private Dictionary<string, string> contentHeaders;

        /// <summary>content stream</summary>
        private Stream contentStream;

        /// <summary>stream dispose delayed until the contentStream is disposed</summary>
        private bool disposeWithContentStreamDispose;

#if ASTORIA_SERVER
        /// <summary>content uri</summary>
        private string contentUri;
#else
        /// <summary>status code of the response.</summary>
        private string statusCode;
#endif

        /// <summary>batch state</summary>
        private BatchStreamState batchState;

#if DEBUG && !ASTORIA_LIGHT
        /// <summary>everything batch reads to help debugging</summary>
        private MemoryStream writer = new MemoryStream();
#else
#pragma warning disable 649
        /// <summary>everything batch reads to help debugging</summary>
        private MemoryStream writer;
#pragma warning restore 649
#endif

        /// <summary>Wrap a stream for batching.</summary>
        /// <param name="stream">underlying stream</param>
        /// <param name="boundary">batch boundary</param>
        /// <param name="batchEncoding">encoding of batch</param>
        /// <param name="requestStream">is request stream or response stream</param>
        internal BatchStream(Stream stream, string boundary, Encoding batchEncoding, bool requestStream)
        {
            Debug.Assert(null != stream, "null stream");

            this.reader = stream;
            this.byteBuffer = new byte[DefaultBufferSize];
            this.batchBoundary = VerifyBoundary(boundary);
            this.batchState = BatchStreamState.StartBatch;
            this.batchEncoding = batchEncoding;
            this.checkPreamble = (null != batchEncoding);
            this.batchRequest = requestStream;
        }

        #region batch properties ContentHeaders, ContentStream, Encoding, Sate
        /// <summary>content headers</summary>
        public Dictionary<string, string> ContentHeaders
        {
            get { return this.contentHeaders; }
        }

#if ASTORIA_SERVER
        /// <summary>Content URI.</summary>
        public string ContentUri
        {
            get { return this.contentUri; }
        }
#endif

        /// <summary>encoding</summary>
        public Encoding Encoding
        {
            get { return this.changesetEncoding ?? this.batchEncoding; }
        }

        /// <summary>batch state</summary>
        public BatchStreamState State
        {
            get { return this.batchState; }
        }
        #endregion

        #region Stream properties
        /// <summary>Delegate to underlying stream</summary>
        public override bool CanRead
        {
            get { return (null != this.reader && this.reader.CanRead); }
        }

        /// <summary>False</summary>
        public override bool CanSeek
        {
            get { return false; }
        }

        /// <summary>False</summary>
        public override bool CanWrite
        {
            get { return false; }
        }

        /// <summary>Not supported.</summary>
        public override long Length
        {
            get { throw Error.NotSupported(); }
        }

        /// <summary>Not supported.</summary>
        public override long Position
        {
            get { throw Error.NotSupported(); }
            set { throw Error.NotSupported(); }
        }
        #endregion

        #region Stream methods
        /// <summary>Does nothing.</summary>
        public override void Flush()
        {
            this.reader.Flush();
        }

        /// <summary>Not supported.</summary>
        /// <param name="buffer">The parameter is not used.</param>
        /// <param name="offset">The parameter is not used.</param>
        /// <param name="count">The parameter is not used.</param>
        /// <param name="callback">The parameter is not used.</param>
        /// <param name="state">The parameter is not used.</param>
        /// <returns>nothing</returns>
        public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
        {
            throw Error.NotSupported();
        }

        /// <summary>Not supported.</summary>
        /// <param name="buffer">The parameter is not used.</param>
        /// <param name="offset">The parameter is not used.</param>
        /// <param name="count">The parameter is not used.</param>
        /// <returns>nothing</returns>
        public override int Read(byte[] buffer, int offset, int count)
        {
            throw Error.NotSupported();
        }

        /// <summary>
        /// Forward seek in buffered underlying stream.
        /// </summary>
        /// <param name="offset">non-negative bytes to forward seek</param>
        /// <param name="origin">must be Current</param>
        /// <returns>underlying stream forward seek result.</returns>
        public override long Seek(long offset, SeekOrigin origin)
        {
            this.AssertOpen();

            if (offset < 0)
            {
                throw Error.ArgumentOutOfRange("offset");
            }

            if (SeekOrigin.Current != origin)
            {
                throw Error.ArgumentOutOfRange("origin");
            }

            if (Int32.MaxValue == offset)
            {   // special case - read to end of delimiter
                byte[] buffer = new byte[256]; // should be at least 70 for minimum boundary length
                while (0 < this.ReadDelimiter(buffer, 0, buffer.Length))
                {
                    /* ignore data */
                }
            }
            else if (0 < offset)
            {   // underlying stream may not support seek, so just move forward the buffered bytes
                do
                {
                    int count = Math.Min(checked((int)offset), Math.Min(this.byteLength, this.batchLength));
                    this.totalCount += count;
                    this.bytePosition += count;
                    this.byteLength -= count;
                    this.batchLength -= count;
                    offset -= count;

                    // underlying stream doesn't support Seek, so we just need to fill our buffer.
                }
                while ((0 < offset) && (this.batchLength != 0) && this.ReadBuffer());
            }

            Debug.Assert(0 <= this.byteLength, "negative byteLength");
            Debug.Assert(0 <= this.batchLength, "negative batchLength");
            return 0;
        }

        /// <summary>Not supported.</summary>
        /// <param name="value">The parameter is not used.</param>
        public override void SetLength(long value)
        {
            throw Error.NotSupported();
        }

        /// <summary>Not supported.</summary>
        /// <param name="buffer">The parameter is not used.</param>
        /// <param name="offset">The parameter is not used.</param>
        /// <param name="count">The parameter is not used.</param>
        public override void Write(byte[] buffer, int offset, int count)
        {
            throw Error.NotSupported();
        }
        #endregion

        /// <summary>
        /// Get the boundary string and encoding if the content type is multipart/mixed.
        /// </summary>
        /// <param name="contentType">content type specified in the request.</param>
        /// <param name="boundary">returns the boundary string specified in the content type.</param>
        /// <param name="encoding">returns the encoding specified in the content type.</param>
        /// <returns>true if multipart/mixed with boundary</returns>
        /// <exception cref="InvalidOperationException">if multipart/mixed without boundary</exception>
        internal static bool GetBoundaryAndEncodingFromMultipartMixedContentType(string contentType, out string boundary, out Encoding encoding)
        {
            boundary = null;
            encoding = null;

            string mime;
            KeyValuePair<string, string>[] parameters = HttpProcessUtility.ReadContentType(contentType, out mime, out encoding);

            if (String.Equals(XmlConstants.MimeMultiPartMixed, mime, StringComparison.OrdinalIgnoreCase))
            {
                if (null != parameters)
                {
                    foreach (KeyValuePair<string, string> parameter in parameters)
                    {
                        if (String.Equals(parameter.Key, XmlConstants.HttpMultipartBoundary, StringComparison.OrdinalIgnoreCase))
                        {
                            if (boundary != null)
                            {   // detect multiple boundary parameters
                                boundary = null;
                                break;
                            }

                            boundary = parameter.Value;
                        }
                    }
                }

                // if an invalid boundary string is specified or no boundary string is specified
                if (String.IsNullOrEmpty(boundary))
                {   // however, empty string is considered a valid boundary
                    throw Error.BatchStreamMissingBoundary();
                }
            }

            return (null != boundary);
        }

#if !ASTORIA_CLIENT
        /// <summary>
        /// Checks whether the given stream instance is a internal batch stream class or not.
        /// </summary>
        /// <param name="stream">stream instance.</param>
        /// <returns>returns true if the given stream instance is a internal batch stream class. Otherwise returns false.</returns>
        internal static bool IsBatchStream(Stream stream)
        {
            return (stream is StreamWithDelimiter || stream is StreamWithLength);
        }

        /// <summary>
        /// Validates that there is no extreaneous data in the stream beyond the end of batch
        /// </summary>
        /// <returns>Exception if there is remaining data after the end boundary of the batch. Otherwise null.</returns>
        internal Exception ValidateNoDataBeyondEndOfBatch()
        {
            if (this.reader.ReadByte() >= 0)
            {
                return Error.BatchStreamMoreDataAfterEndOfBatch();
            }

            return null;
        }
#endif

#if ASTORIA_CLIENT
        /// <summary>Gets the version from content-headers if available.</summary>
        /// <returns>The value for the DataServiceVersion header.</returns>
        internal string GetResponseVersion()
        {
            string result;
            this.ContentHeaders.TryGetValue(XmlConstants.HttpDataServiceVersion, out result);
            return result;
        }

        /// <summary>Get and parse status code from content-headers</summary>
        /// <returns>status code</returns>
        internal HttpStatusCode GetStatusCode()
        {
            return (HttpStatusCode)(null != this.statusCode ? Int32.Parse(this.statusCode, CultureInfo.InvariantCulture) : 500);
        }
#endif

        /// <summary>start a multipart content section with a specific boundary</summary>
        /// <returns>true if this is content to process</returns>
        /// <remarks>
        /// 5.1.2
        /// an improperly truncated "multipart" entity may not have
        /// any terminating boundary marker.
        /// MIME implementations are required to recognize outer level
        /// boundary markers at ANY level of inner nesting.
        /// 5.1.3
        /// The "mixed" subtype of "multipart" is intended for use when the body
        /// parts are independent and need to be bundled in a particular order.
        /// Any "multipart" subtypes that an implementation does not recognize
        /// must be treated as being of subtype "mixed".
        /// </remarks>
        internal bool MoveNext()
        {
            #region dispose previous content stream
            if (null == this.reader || this.disposeWithContentStreamDispose)
            {
                return false;
            }

            if (null != this.contentStream)
            {
                this.contentStream.Dispose();
            }

            Debug.Assert(0 <= this.byteLength, "negative byteLength");
            Debug.Assert(0 <= this.batchLength, "negative batchLength");
            #endregion

            #region initialize start state to EndBatch or EndChangeSet
            switch (this.batchState)
            {
                case BatchStreamState.EndBatch:
                    // already finished
                    Debug.Assert(null == this.batchBoundary, "non-null batch boundary");
                    Debug.Assert(null == this.changesetBoundary, "non-null changesetBoundary boundary");
                    throw Error.BatchStreamInvalidBatchFormat();

                case BatchStreamState.Get:
                case BatchStreamState.GetResponse:
                    // Since there is no content specified for Get operations,
                    // after the operation is performed, we need to clear out the headers and uri information
                    // specified for the Get operation
                    this.ClearPreviousOperationInformation();
                    goto case BatchStreamState.StartBatch;

                case BatchStreamState.StartBatch:
                case BatchStreamState.EndChangeSet:
                    Debug.Assert(null != this.batchBoundary, "null batch boundary");
                    Debug.Assert(null == this.changesetBoundary, "non-null changeset boundary");
                    this.batchState = BatchStreamState.EndBatch;
                    this.batchLength = Int32.MaxValue;
                    break;

                case BatchStreamState.BeginChangeSet:
                    Debug.Assert(null != this.batchBoundary, "null batch boundary");
                    Debug.Assert(null != this.contentHeaders, "null contentHeaders");
                    Debug.Assert(null != this.changesetBoundary, "null changeset boundary");
                    this.contentHeaders = null;
                    this.changesetEncoding = null;
                    this.batchState = BatchStreamState.EndChangeSet;
                    break;

                case BatchStreamState.ChangeResponse:
                case BatchStreamState.Delete:
                    Debug.Assert(null != this.changesetBoundary, "null changeset boundary");
                    this.ClearPreviousOperationInformation();
                    this.batchState = BatchStreamState.EndChangeSet;
                    break;

                case BatchStreamState.Post:
                case BatchStreamState.Put:
                case BatchStreamState.Merge:
                    // Since there is no content specified for DELETE operations or PUT response
                    // after the operation is performed, we need to clear out the headers and uri information
                    // specified for the DELETE operation
                    Debug.Assert(null != this.changesetBoundary, "null changeset boundary");
                    this.batchState = BatchStreamState.EndChangeSet;
                    break;

                default:
                    Debug.Assert(false, "unknown state");
                    throw Error.BatchStreamInvalidBatchFormat();
            }

            Debug.Assert(null == this.contentHeaders, "non-null content headers");
            Debug.Assert(null == this.contentStream, "non-null content stream");
#if ASTORIA_SERVER
            Debug.Assert(null == this.contentUri, "non-null content uri");
#endif
#if ASTORIA_CLIENT
            Debug.Assert(null == this.statusCode, "non-null statusCode");
#endif

            Debug.Assert(
                this.batchState == BatchStreamState.EndBatch ||
                this.batchState == BatchStreamState.EndChangeSet,
                "unexpected state at start");
            #endregion

            #region read --delimiter
            string delimiter = this.ReadLine();
            if (String.IsNullOrEmpty(delimiter))
            {   // was the \r\n not included in the previous section's content-length?
                delimiter = this.ReadLine();
            }

            if (String.IsNullOrEmpty(delimiter))
            {
                throw Error.BatchStreamInvalidBatchFormat();
            }

            if (delimiter.EndsWith("--", StringComparison.Ordinal))
            {
                delimiter = delimiter.Substring(0, delimiter.Length - 2);

                if ((null != this.changesetBoundary) && (delimiter == this.changesetBoundary))
                {
                    Debug.Assert(this.batchState == BatchStreamState.EndChangeSet, "bad changeset boundary state");

                    this.changesetBoundary = null;
                    return true;
                }
                else if (delimiter == this.batchBoundary)
                {
                    if (BatchStreamState.EndChangeSet == this.batchState)
                    {   // we should technically recover, but we are not going to.
                        throw Error.BatchStreamMissingEndChangesetDelimiter();
                    }

                    this.changesetBoundary = null;
                    this.batchBoundary = null;
                    if (this.byteLength != 0)
                    {
                        throw Error.BatchStreamMoreDataAfterEndOfBatch();
                    }

                    return false;
                }
                else
                {
                    throw Error.BatchStreamInvalidDelimiter(delimiter);
                }
            }
            else if ((null != this.changesetBoundary) && (delimiter == this.changesetBoundary))
            {
                Debug.Assert(this.batchState == BatchStreamState.EndChangeSet, "bad changeset boundary state");
            }
            else if (delimiter == this.batchBoundary)
            {
                if (this.batchState != BatchStreamState.EndBatch)
                {
                    if (this.batchState == BatchStreamState.EndChangeSet)
                    {   // we should technically recover, but we are not going to.
                        throw Error.BatchStreamMissingEndChangesetDelimiter();
                    }
                    else
                    {
                        throw Error.BatchStreamInvalidBatchFormat();
                    }
                }
            }
            else
            {   // unknown delimiter
                throw Error.BatchStreamInvalidDelimiter(delimiter);
            }

            #endregion

            #region read header with values in this form (([^:]*:.*)\r\n)*\r\n
            this.ReadContentHeaders();
            #endregion

            #region should start changeset?
            string contentType;
            bool readHttpHeaders = false;
            if (this.contentHeaders.TryGetValue(XmlConstants.HttpContentType, out contentType))
            {
                if (String.Equals(contentType, XmlConstants.MimeApplicationHttp, StringComparison.OrdinalIgnoreCase))
                {
                    // We don't allow custom headers at the start of changeset or get batch request.
                    // One can always specify custom headers along with the other http headers that
                    // follows these headers
                    if (this.contentHeaders.Count != 2)
                    {
                        throw Error.BatchStreamInvalidNumberOfHeadersAtOperationStart(
                            XmlConstants.HttpContentType,
                            XmlConstants.HttpContentTransferEncoding);
                    }

                    string transferEncoding;
                    if (!this.contentHeaders.TryGetValue(XmlConstants.HttpContentTransferEncoding, out transferEncoding) ||
                        XmlConstants.BatchRequestContentTransferEncoding != transferEncoding)
                    {
                        throw Error.BatchStreamMissingOrInvalidContentEncodingHeader(
                            XmlConstants.HttpContentTransferEncoding,
                            XmlConstants.BatchRequestContentTransferEncoding);
                    }

                    readHttpHeaders = true;
                }
                else if (BatchStreamState.EndBatch == this.batchState)
                {
                    string boundary;
                    Encoding encoding;
                    if (GetBoundaryAndEncodingFromMultipartMixedContentType(contentType, out boundary, out encoding))
                    {
                        this.changesetBoundary = VerifyBoundary(boundary);
                        this.changesetEncoding = encoding;
                        this.batchState = BatchStreamState.BeginChangeSet;
                    }
                    else
                    {
                        throw Error.BatchStreamInvalidContentTypeSpecified(
                            XmlConstants.HttpContentType,
                            contentType,
                            XmlConstants.MimeApplicationHttp,
                            XmlConstants.MimeMultiPartMixed);
                    }

                    // We don't allow custom headers at the start of batch operation.
                    // One can always specify custom headers along with the other http headers that
                    // are present in the changeset.
                    if (this.contentHeaders.Count > 2 ||
                        (this.contentHeaders.Count == 2 && !this.contentHeaders.ContainsKey(XmlConstants.HttpContentLength)))
                    {
                        throw Error.BatchStreamInvalidNumberOfHeadersAtChangeSetStart(XmlConstants.HttpContentType, XmlConstants.HttpContentLength);
                    }
                }
                else
                {
                    throw Error.BatchStreamInvalidContentTypeSpecified(
                        XmlConstants.HttpContentType,
                        contentType,
                        XmlConstants.MimeApplicationHttp,
                        XmlConstants.MimeMultiPartMixed);
                }
            }
            else
            {
                throw Error.BatchStreamMissingContentTypeHeader(XmlConstants.HttpContentType);
            }
            #endregion

            #region what is the operation and uri?
            if (readHttpHeaders)
            {
                this.ReadHttpHeaders();

                // read the content type to clear the value
                // of the content type
                this.contentHeaders.TryGetValue(XmlConstants.HttpContentType, out contentType);
            }
            #endregion

            //// stream is now positioned on content 
            //// or its on the start of the actual headers

            #region does content have a fixed length?
            string text = null;
            int length = -1;
            if (this.contentHeaders.TryGetValue(XmlConstants.HttpContentLength, out text))
            {
                length = Int32.Parse(text, CultureInfo.InvariantCulture);
                if (length < 0)
                {
                    throw Error.BatchStreamInvalidContentLengthSpecified(text);
                }

                if (this.batchState == BatchStreamState.BeginChangeSet)
                {
                    this.batchLength = length;
                }
                else if (length != 0)
                {
                    Debug.Assert(
                        this.batchState == BatchStreamState.Delete ||
                        this.batchState == BatchStreamState.Get ||
                        this.batchState == BatchStreamState.Post ||
                        this.batchState == BatchStreamState.Put ||
                        this.batchState == BatchStreamState.Merge,
                        "unexpected contentlength location");
                    this.contentStream = new StreamWithLength(this, length);
                }
            }
            else
            {
                if (this.batchState == BatchStreamState.EndBatch)
                {
                    this.batchLength = Int32.MaxValue;
                }

                if (this.batchState != BatchStreamState.BeginChangeSet)
                {
                    this.contentStream = new StreamWithDelimiter(this);
                }
            }

            #endregion

            Debug.Assert(
                this.batchState == BatchStreamState.BeginChangeSet ||
                (this.batchRequest && (this.batchState == BatchStreamState.Delete ||
                                       this.batchState == BatchStreamState.Get ||
                                       this.batchState == BatchStreamState.Post ||
                                       this.batchState == BatchStreamState.Put ||
                                       this.batchState == BatchStreamState.Merge)) ||
                (!this.batchRequest && (this.batchState == BatchStreamState.GetResponse ||
                                        this.batchState == BatchStreamState.ChangeResponse)),
                "unexpected state at return");

            #region enforce if contentStream is expected, caller needs to enforce if contentStream is not expected
            if (null == this.contentStream)
            {
                switch (this.batchState)
                {
                    case BatchStreamState.BeginChangeSet:
                    case BatchStreamState.Delete:
                    case BatchStreamState.Get:
                    case BatchStreamState.ChangeResponse:   // example DELETE /Customers(1)
                    case BatchStreamState.GetResponse:      // example GET /Customers(1)/BestFriend
                        break;

                    case BatchStreamState.Post:
                    case BatchStreamState.Put:
                    case BatchStreamState.Merge:
                    default:
                        // we expect a content stream
                        throw Error.BatchStreamContentExpected(this.batchState);
                }
            }
            #endregion

            #region enforce if contentType not is expected, caller needs to enforce if contentType is expected
            if (!String.IsNullOrEmpty(contentType))
            {
                switch (this.batchState)
                {
                    case BatchStreamState.BeginChangeSet:
                    case BatchStreamState.Post:
                    case BatchStreamState.Put:
                    case BatchStreamState.Merge:
                    case BatchStreamState.GetResponse:
                    case BatchStreamState.ChangeResponse:
                        // we do allow content-type to be defined
                        break;

                    case BatchStreamState.Get:              // request does not expect content-type
                    case BatchStreamState.Delete:           // request does not expect content-type
                    default:
                        // we do NOT expect content-type to be defined
                        throw Error.BatchStreamContentUnexpected(this.batchState);
                }
            }
            #endregion

            return true;
        }

        /// <summary>Method to get content stream instead of property so it can be passed as function</summary>
        /// <returns>ContentStream</returns>
        internal Stream GetContentStream()
        {
            return this.contentStream;
        }

        /// <summary>Dispose underlying stream</summary>
        /// <param name="disposing">true if active dispose, false if finalizer</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (null != this.contentStream)
                {   // delay disposing of the reader until content stream is disposed
                    this.disposeWithContentStreamDispose = true;
                }
                else
                {
                    this.byteLength = 0;
                    if (null != this.reader)
                    {
                        this.reader.Dispose();
                        this.reader = null;
                    }

                    this.contentHeaders = null;
                    if (null != this.contentStream)
                    {
                        this.contentStream.Dispose();
                    }

                    if (null != this.writer)
                    {
                        this.writer.Dispose();
                    }
                }
            }

            // Stream.Dispose(bool) does nothing, but for completeness w/ FxCop
            base.Dispose(disposing);
        }

        /// <summary>
        /// Validates the method name and returns the state based on the method name
        /// </summary>
        /// <param name="methodName">method name to be validated</param>
        /// <returns>state based on the method name</returns>
        private static BatchStreamState GetStateBasedOnHttpMethodName(string methodName)
        {
            if (XmlConstants.HttpMethodGet.Equals(methodName, StringComparison.Ordinal))
            {
                return BatchStreamState.Get;
            }
            else if (XmlConstants.HttpMethodDelete.Equals(methodName, StringComparison.Ordinal))
            {
                return BatchStreamState.Delete;
            }
            else if (XmlConstants.HttpMethodPost.Equals(methodName, StringComparison.Ordinal))
            {
                return BatchStreamState.Post;
            }
            else if (XmlConstants.HttpMethodPut.Equals(methodName, StringComparison.Ordinal))
            {
                return BatchStreamState.Put;
            }
            else if (XmlConstants.HttpMethodMerge.Equals(methodName, StringComparison.Ordinal))
            {
                return BatchStreamState.Merge;
            }
            else
            {
                throw Error.BatchStreamInvalidHttpMethodName(methodName);
            }
        }

        /// <summary>
        /// verify boundary delimiter if valid
        /// </summary>
        /// <param name="boundary">boundary to test</param>
        /// <returns>"--" + boundary</returns>
        private static string VerifyBoundary(string boundary)
        {
            if ((null == boundary) || (70 < boundary.Length))
            {
                throw Error.BatchStreamInvalidDelimiter(boundary);
            }

            foreach (char c in boundary)
            {
                if ((127 < (int)c) || Char.IsWhiteSpace(c) || Char.IsControl(c))
                {   // must be 7-bit, non-whitespace (including newline char), non-control character
                    throw Error.BatchStreamInvalidDelimiter(boundary);
                }
            }

            return "--" + boundary;
        }

        /// <summary>
        /// Clears the headers, contentUri and stream of the previous operation
        /// </summary>
        private void ClearPreviousOperationInformation()
        {
            this.contentHeaders = null;
            this.contentStream = null;
#if ASTORIA_SERVER
            this.contentUri = null;
#endif
#if ASTORIA_CLIENT
            this.statusCode = null;
#endif
        }

        /// <summary>appends bytes from byteBuffer to buffer</summary>
        /// <param name="buffer">buffer to append to, grows as necessary</param>
        /// <param name="count">count of bytes to append</param>
        private void Append(ref byte[] buffer, int count)
        {
            int oldSize = (null != buffer) ? buffer.Length : 0;

            byte[] tmp = new byte[oldSize + count];
            if (0 < oldSize)
            {
                Buffer.BlockCopy(buffer, 0, tmp, 0, oldSize);
            }

            Buffer.BlockCopy(this.byteBuffer, this.bytePosition, tmp, oldSize, count);
            buffer = tmp;

            this.totalCount += count;
            this.bytePosition += count;
            this.byteLength -= count;
            this.batchLength -= count;

            Debug.Assert(0 <= this.byteLength, "negative byteLength");
            Debug.Assert(0 <= this.batchLength, "negative batchLength");
        }

        /// <summary>verify reader is open</summary>
        /// <exception cref="ObjectDisposedException">if reader is used after dispose</exception>
        private void AssertOpen()
        {
            if (null == this.reader)
            {
                Error.ThrowObjectDisposed(this.GetType());
            }
        }

        /// <summary>Fill the buffer from the underlying stream.</summary>
        /// <returns>true if any data was read.</returns>
        private bool ReadBuffer()
        {
            this.AssertOpen();

            if (0 == this.byteLength)
            {
                this.bytePosition = 0;
                this.byteLength = this.reader.Read(this.byteBuffer, this.bytePosition, this.byteBuffer.Length);
                if (null != this.writer)
                {
                    this.writer.Write(this.byteBuffer, this.bytePosition, this.byteLength);
                }

                if (null == this.batchEncoding)
                {
                    this.batchEncoding = this.DetectEncoding();
                }
                else if (null != this.changesetEncoding)
                {
                    this.changesetEncoding = this.DetectEncoding();
                }
                else if (this.checkPreamble)
                {
                    bool match = true;
                    byte[] preamble = this.batchEncoding.GetPreamble();
                    if (preamble.Length <= this.byteLength)
                    {
                        for (int i = 0; i < preamble.Length; ++i)
                        {
                            if (preamble[i] != this.byteBuffer[i])
                            {
                                match = false;
                                break;
                            }
                        }

                        if (match)
                        {
                            this.byteLength -= preamble.Length;
                            this.bytePosition += preamble.Length;
                        }
                    }

                    this.checkPreamble = false;
                }

                return (0 < this.byteLength);
            }

            return true;
        }

        /// <summary>
        /// Reads a line. A line is defined as a sequence of characters followed by
        /// a carriage return ('\r'), a line feed ('\n'), or a carriage return
        /// immediately followed by a line feed. The resulting string does not
        /// contain the terminating carriage return and/or line feed. The returned
        /// value is null if the end of the input stream has been reached.
        /// </summary>
        /// <returns>line from the buffered stream</returns>
        private String ReadLine()
        {
            if ((0 == this.batchLength) || !this.ReadBuffer())
            {
                return null;
            }

            byte[] buffer = null;
            do
            {
                Debug.Assert(0 < this.byteLength, "out of bytes");
                Debug.Assert(this.bytePosition + this.byteLength <= this.byteBuffer.Length, "byte tracking out of range");
                int i = this.bytePosition;
                int end = i + Math.Min(this.byteLength, this.batchLength);
                do
                {
                    char ch = (char)this.byteBuffer[i];

                    // Note the following common line feed chars:
                    // \n - UNIX   \r\n - DOS   \r - Mac
                    if (('\r' == ch) || ('\n' == ch))
                    {
                        string s;

                        i -= this.bytePosition;
                        if (null != buffer)
                        {
                            this.Append(ref buffer, i);
                            s = this.Encoding.GetString(buffer, 0, buffer.Length);
                        }
                        else
                        {
                            s = this.Encoding.GetString(this.byteBuffer, this.bytePosition, i);

                            this.totalCount += i;
                            this.bytePosition += i;
                            this.byteLength -= i;
                            this.batchLength -= i;
                        }

                        this.totalCount++;
                        this.bytePosition++;
                        this.byteLength--;
                        this.batchLength--;
                        if (('\r' == ch) && ((0 < this.byteLength) || this.ReadBuffer()) && (0 < this.batchLength))
                        {
                            ch = (char)this.byteBuffer[this.bytePosition];
                            if ('\n' == ch)
                            {
                                this.totalCount++;
                                this.bytePosition++;
                                this.byteLength--;
                                this.batchLength--;
                            }
                        }

                        Debug.Assert(0 <= this.byteLength, "negative byteLength");
                        Debug.Assert(0 <= this.batchLength, "negative batchLength");
                        return s;
                    }

                    i++;
                }
                while (i < end);

                i -= this.bytePosition;
                this.Append(ref buffer, i);
            }
            while (this.ReadBuffer() && (0 < this.batchLength));

            Debug.Assert(0 <= this.byteLength, "negative byteLength");
            Debug.Assert(0 <= this.batchLength, "negative batchLength");
            return this.Encoding.GetString(buffer, 0, buffer.Length);
        }

        /// <summary>Detect the encoding based data from the stream.</summary>
        /// <returns>discovered encoding</returns>
        private Encoding DetectEncoding()
        {
            if (this.byteLength < 2)
            {
#if !ASTORIA_LIGHT  // ASCII not available
                return Encoding.ASCII;
#else
                return HttpProcessUtility.FallbackEncoding;
#endif
            }
            else if (this.byteBuffer[0] == 0xFE && this.byteBuffer[1] == 0xFF)
            {   // Big Endian Unicode
                this.bytePosition = 2;
                this.byteLength -= 2;
                return new UnicodeEncoding(true, true);
            }
            else if (this.byteBuffer[0] == 0xFF && this.byteBuffer[1] == 0xFE)
            {   // Little Endian Unicode, or possibly little endian UTF32
                if (this.byteLength >= 4 &&
                    this.byteBuffer[2] == 0 &&
                    this.byteBuffer[3] == 0)
                {
#if !ASTORIA_LIGHT  // Little Endian UTF32 not available
                    this.bytePosition = 4;
                    this.byteLength -= 4;
                    return new UTF32Encoding(false, true);
#else
                    throw Error.NotSupported();
#endif
                }
                else
                {
                    this.bytePosition = 2;
                    this.byteLength -= 2;
                    return new UnicodeEncoding(false, true);
                }
            }
            else if (this.byteLength >= 3 &&
                     this.byteBuffer[0] == 0xEF &&
                     this.byteBuffer[1] == 0xBB &&
                     this.byteBuffer[2] == 0xBF)
            {   // UTF-8
                this.bytePosition = 3;
                this.byteLength -= 3;
                return Encoding.UTF8;
            }
            else if (this.byteLength >= 4 &&
                     this.byteBuffer[0] == 0 &&
                     this.byteBuffer[1] == 0 &&
                     this.byteBuffer[2] == 0xFE &&
                     this.byteBuffer[3] == 0xFF)
            {   // Big Endian UTF32
#if !ASTORIA_LIGHT  // Big Endian UTF32 not available
                this.bytePosition = 4;
                this.byteLength -= 4;
                return new UTF32Encoding(true, true);
#else
                throw Error.NotSupported();
#endif
            }
            else
            {
#if !ASTORIA_LIGHT  // ASCII not available
                return Encoding.ASCII;
#else
                return HttpProcessUtility.FallbackEncoding;
#endif
            }
        }

        /// <summary>
        /// read from BatchStream buffer into user buffer, stopping when a boundary delimiter is found
        /// </summary>
        /// <param name="buffer">place to copy bytes read from underlying stream</param>
        /// <param name="offset">offset in buffer to start writing</param>
        /// <param name="count">count of bytes to read from buffered underlying stream</param>
        /// <returns>count of bytes actualy copied into buffer</returns>
        private int ReadDelimiter(byte[] buffer, int offset, int count)
        {
            Debug.Assert(null != buffer, "null != buffer");
            Debug.Assert(0 <= offset, "0 <= offset");
            Debug.Assert(0 <= count, "0 <= count");
            Debug.Assert(offset + count <= buffer.Length, "offset + count <= buffer.Length");
            int copied = 0;

            // which boundary are we looking for
            string boundary = null;
            string boundary1 = this.batchBoundary;
            string boundary2 = this.changesetBoundary;

            while ((0 < count) && (0 < this.batchLength) && this.ReadBuffer())
            {
                // if a boundary spanned to actually buffer reads, we shifted and restart boundary match
                // how many bytes have we matched in the boundary
                int boundaryIndex = 0;
                int boundary1Index = 0;
                int boundary2Index = 0;

                // how many bytes can we search for
                int size = Math.Min(Math.Min(count, this.byteLength), this.batchLength) + this.bytePosition;

                byte[] data = this.byteBuffer;
                for (int i = this.bytePosition; i < size; ++i)
                {
                    byte value = data[i];
                    buffer[offset++] = value; // copy value to caller's buffer

                    if ((char)value == boundary1[boundary1Index])
                    {
                        if (boundary1.Length == ++boundary1Index)
                        {   // found full match
                            size = (1 + i) - boundary1Index;
                            offset -= boundary1Index;
                            Debug.Assert(this.bytePosition <= size, "negative size");
                            break;
                        }
                    }
                    else
                    {
                        boundary1Index = 0;
                    }

                    if ((null != boundary2) && ((char)value == boundary2[boundary2Index]))
                    {
                        if (boundary2.Length == ++boundary2Index)
                        {   // found full match
                            size = (1 + i) - boundary2Index;
                            offset -= boundary2Index;
                            Debug.Assert(this.bytePosition <= size, "negative size");
                            break;
                        }
                    }
                    else
                    {
                        boundary2Index = 0;
                    }
                }

                // Here the size is going to be the amount of data just read before hitting the boundary
                // Basically it indicates the payload size for the operation.
                size -= this.bytePosition;
                Debug.Assert(0 <= size, "negative size");

                if (boundary1Index < boundary2Index)
                {
                    boundaryIndex = boundary2Index;
                    boundary = boundary2;
                }
                else
                {
                    Debug.Assert(null != boundary1, "batch boundary shouldn't be null");
                    boundaryIndex = boundary1Index;
                    boundary = boundary1;
                }

                if (size == this.batchLength)
                {   // outer batch stream has reached its limit - there will be no more data for this delimiter
                    // partial match at EOF is not a match
                    boundaryIndex = 0;
                }

                // boundaryIndex either represents either
                // full match
                // partial match and we just need more data in the buffer to continue
                // partial match in the requested count buffer (count maybe < boundary.Length)
                if ((0 < boundaryIndex) && (boundary.Length != boundaryIndex))
                {   // partial boundary in stream - but hit the end (compress and continue)
                    if ((size + copied == boundaryIndex) && (boundaryIndex < this.byteLength))
                    {
                        // The issue here is that someone is asking us to read a chunk of data which is 
                        // smaller than the boundary length, and we cannot return partial boundary content
                        // requested smaller amount than we buffered - have partial match - is it real?
                        // the count caller is requesting is too small without look ahead
                        throw Error.BatchStreamInternalBufferRequestTooSmall();
                    }
                    else
                    {   // we need more data before we can determine if match
                        size -= boundaryIndex;
                        offset -= boundaryIndex;
                    }
                }

                this.totalCount += size;
                this.bytePosition += size;
                this.byteLength -= size;
                this.batchLength -= size;

                count -= size;
                copied += size;

                // Each boundary should begin in a new line.  We should remove the "\r\n" right before the boundary.
                // Having the extra "\r\n" corrupts the data if the content before the delimiter is binary.
                if (boundaryIndex > 0 && copied >= 2 && buffer[copied - 2] == '\r' && buffer[copied - 1] == '\n')
                {
                    copied -= 2;
                }

                if (boundary.Length == boundaryIndex)
                {
                    break;
                }
                else if (0 < boundaryIndex)
                {
                    if (boundaryIndex == this.byteLength)
                    {   // we need more data from underlying stream
                        if (0 < this.bytePosition)
                        {   // compress the buffer
                            Buffer.BlockCopy(data, this.bytePosition, data, 0, this.byteLength);
                            this.bytePosition = 0;
                        }

                        int tmp = this.reader.Read(this.byteBuffer, this.byteLength, this.byteBuffer.Length - this.byteLength);
                        if (null != this.writer)
                        {
                            this.writer.Write(this.byteBuffer, this.byteLength, tmp);
                        }

                        if (0 == tmp)
                        {   // partial boundary is at EOF
                            this.totalCount += boundaryIndex;
                            this.bytePosition += boundaryIndex;
                            this.byteLength -= boundaryIndex;
                            this.batchLength -= boundaryIndex;

                            offset += boundaryIndex;
                            count -= boundaryIndex;
                            copied += boundaryIndex;
                            break;
                        }

                        // partial boundary not at EOF, restart the boundary match
                        this.byteLength += tmp;
                    }
                    else
                    {   // return smaller than requested buffer to user
                        break;
                    }
                }
            }

            return copied;
        }

        /// <summary>Read from internal buffer or use unbuffered read from underlying stream.</summary>
        /// <param name="buffer">place to copy bytes read from underlying stream</param>
        /// <param name="offset">offset in buffer to start writing</param>
        /// <param name="count">count of bytes to read from buffered underlying stream</param>
        /// <returns>count of bytes actualy copied into buffer</returns>
        private int ReadLength(byte[] buffer, int offset, int count)
        {
            Debug.Assert(null != buffer, "null != buffer");
            Debug.Assert(0 <= offset, "0 <= offset");
            Debug.Assert(0 <= count, "0 <= count");
            Debug.Assert(offset + count <= buffer.Length, "offset + count <= buffer.Length");
            int copied = 0;

            if (0 < this.byteLength)
            {   // initial read drains from our buffer
                int size = Math.Min(Math.Min(count, this.byteLength), this.batchLength);
                Buffer.BlockCopy(this.byteBuffer, this.bytePosition, buffer, offset, size);
                this.totalCount += size;
                this.bytePosition += size;
                this.byteLength -= size;
                this.batchLength -= size;

                offset += size;
                count -= size;
                copied = size;
            }

            if (0 < count && this.batchLength > 0)
            {   // read remainder directly from stream
                int size = this.reader.Read(buffer, offset, Math.Min(count, this.batchLength));
                if (null != this.writer)
                {
                    this.writer.Write(buffer, offset, size);
                }

                this.totalCount += size;
                this.batchLength -= size;
                copied += size;
            }

            Debug.Assert(0 <= this.byteLength, "negative byteLength");
            Debug.Assert(0 <= this.batchLength, "negative batchLength");
            return copied;
        }

        /// <summary>
        /// Read the content headers
        /// </summary>
        private void ReadContentHeaders()
        {
            // Read the content headers u
            this.contentHeaders = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            while (true)
            {
                string line = this.ReadLine();
                if (0 < line.Length)
                {
                    int colon = line.IndexOf(':');
                    if (colon <= 0)
                    {   // expecting "name: value"
                        throw Error.BatchStreamInvalidHeaderValueSpecified(line);
                    }

                    string name = line.Substring(0, colon).Trim();
                    string value = line.Substring(colon + 1).Trim();
                    this.contentHeaders.Add(name, value);
                }
                else
                {
                    break;
                }
            }
        }

        /// <summary>
        /// Validate that the first header is the http method name, followed by url followed by http version
        /// E.g. POST /Customers HTTP/1.1
        /// </summary>
        private void ReadHttpHeaders()
        {
            // read the header line
            string line = this.ReadLine();

            // Batch Request: POST /Customers HTTP/1.1
            // Since the uri can contain spaces, the only way to read the request url, is to
            // check for first space character and last space character and anything between
            // them.

            // Batch Response: HTTP/1.1 200 Ok
            // Since the http status code strings have spaces in them, we cannot use the same
            // logic. We need to check for the second space and anything after that is the error
            // message.
            int index1 = line.IndexOf(' ');
            if ((index1 <= 0) || ((line.Length - 3) <= index1))
            {
                // only 1 segment or empty first segment or not enough left for 2nd and 3rd segments
                throw Error.BatchStreamInvalidMethodHeaderSpecified(line);
            }

            int index2 = (this.batchRequest ? line.LastIndexOf(' ') : line.IndexOf(' ', index1 + 1));
            if ((index2 < 0) || (index2 - index1 - 1 <= 0) || ((line.Length - 1) <= index2))
            {
                // only 2 segments or empty 2nd or 3rd segments
                throw Error.BatchStreamInvalidMethodHeaderSpecified(line);
            }

            string segment1 = line.Substring(0, index1);                        // Request - Http method,  Response - Http version
            string segment2 = line.Substring(index1 + 1, index2 - index1 - 1);  // Request - Request uri,  Response - Http status code
            string segment3 = line.Substring(index2 + 1);                       // Request - Http version, Response - Http status description

            #region validate HttpVersion
            string httpVersion = this.batchRequest ? segment3 : segment1;
            if (httpVersion != XmlConstants.HttpVersionInBatching)
            {
                throw Error.BatchStreamInvalidHttpVersionSpecified(httpVersion, XmlConstants.HttpVersionInBatching);
            }
            #endregion

            // read the actual http headers now
            this.ReadContentHeaders();

            BatchStreamState state;
            if (this.batchRequest)
            {
                state = GetStateBasedOnHttpMethodName(segment1);
#if ASTORIA_SERVER
                this.contentUri = segment2;
#endif
            }
            else
            {
                // caller must use content-id to correlate response to action
                state = (BatchStreamState.EndBatch == this.batchState) ? BatchStreamState.GetResponse : BatchStreamState.ChangeResponse;
#if ASTORIA_CLIENT
                this.statusCode = segment2;
#endif
            }

            #region validate state change
            Debug.Assert(
                BatchStreamState.EndBatch == this.batchState ||
                BatchStreamState.EndChangeSet == this.batchState,
                "unexpected BatchStreamState");

            if (this.batchState == BatchStreamState.EndBatch)
            {
                if ((this.batchRequest && (state == BatchStreamState.Get)) ||
                    (!this.batchRequest && (state == BatchStreamState.GetResponse)))
                {
                    this.batchState = state;
                }
                else
                {
                    throw Error.BatchStreamOnlyGETOperationsCanBeSpecifiedInBatch();
                }
            }
            else if (this.batchState == BatchStreamState.EndChangeSet)
            {
                if ((this.batchRequest && ((BatchStreamState.Post == state) || (BatchStreamState.Put == state) || (BatchStreamState.Delete == state) || (BatchStreamState.Merge == state))) ||
                    (!this.batchRequest && (state == BatchStreamState.ChangeResponse)))
                {
                    this.batchState = state;
                }
                else
                {
                    // setting the batch state to POST so that in the next round, we can have the correct
                    // state to start with.
                    this.batchState = BatchStreamState.Post;

                    // bad http method verb for changeset
                    throw Error.BatchStreamGetMethodNotSupportInChangeset();
                }
            }
            else
            {   // bad state for operation to exist
                throw Error.BatchStreamInvalidOperationHeaderSpecified();
            }
            #endregion
        }

        /// <summary>
        /// sub stream of BatchStream that reads up to a boundary delimiter
        /// </summary>
        private sealed class StreamWithDelimiter : StreamWithLength
        {
            /// <summary>
            /// constructor
            /// </summary>
            /// <param name="stream">underlying stream</param>
            internal StreamWithDelimiter(BatchStream stream)
                : base(stream, Int32.MaxValue)
            {
            }

            /// <summary>read bytes from stream</summary>
            /// <param name="buffer">buffer to store bytes being read</param>
            /// <param name="offset">offset in buffer to start storing bytes</param>
            /// <param name="count">count of bytes to read</param>
            /// <returns>count of bytes actualy read into the buffer</returns>
            public override int Read(byte[] buffer, int offset, int count)
            {
                if (null == this.Target)
                {
                    Error.ThrowObjectDisposed(this.GetType());
                }

                int result = this.Target.ReadDelimiter(buffer, offset, count);
                return result;
            }
        }

        /// <summary>
        /// sub stream of BatchStream that reads a specific length from underlying stream
        /// </summary>
        /// <remarks>
        /// Allows users of stream to call Dispose multiple times
        /// without affecting the BatchStream
        /// </remarks>
        private class StreamWithLength : Stream
        {
            /// <summary>Underlying batch stream</summary>
            private BatchStream target;

            /// <summary>Max remaining byte length to read from underlying stream</summary>
            private int length;

            /// <summary>
            /// constructor
            /// </summary>
            /// <param name="stream">underlying stream</param>
            /// <param name="contentLength">max byte length to read</param>
            internal StreamWithLength(BatchStream stream, int contentLength)
            {
                Debug.Assert(null != stream, "null != stream");
                Debug.Assert(0 < contentLength, "0 < contentLength");
                this.target = stream;
                this.length = contentLength;
            }

            /// <summary>Delegate to underlying stream</summary>
            public override bool CanRead
            {
                get { return (null != this.target && this.target.CanRead); }
            }

            /// <summary>False</summary>
            public override bool CanSeek
            {
                get { return false; }
            }

            /// <summary>False</summary>
            public override bool CanWrite
            {
                get { return false; }
            }

            /// <summary>Not supported.</summary>
            public override long Length
            {
                get { throw Error.NotSupported(); }
            }

            /// <summary>Not supported.</summary>
            public override long Position
            {
                get { throw Error.NotSupported(); }
                set { throw Error.NotSupported(); }
            }

            /// <summary>Underlying batch stream</summary>
            internal BatchStream Target
            {
                get { return this.target; }
            }

            /// <summary>Does nothing.</summary>
            public override void Flush()
            {
            }

#if DEBUG && !ASTORIA_LIGHT // Synchronous methods not available
            /// <summary>Not supported.</summary>
            /// <param name="buffer">ignored</param>
            /// <param name="offset">ignored</param>
            /// <param name="count">ignored</param>
            /// <param name="callback">ignored</param>
            /// <param name="state">ignored</param>
            /// <returns>nothing</returns>
            public override IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
            {
                throw Error.NotSupported();
            }
#endif

            /// <summary>read bytes from stream</summary>
            /// <param name="buffer">buffer to store bytes being read</param>
            /// <param name="offset">offset in buffer to start storing bytes</param>
            /// <param name="count">count of bytes to read</param>
            /// <returns>count of bytes actualy read into the buffer</returns>
            public override int Read(byte[] buffer, int offset, int count)
            {
                if (null == this.target)
                {
                    Error.ThrowObjectDisposed(this.GetType());
                }

                int result = this.target.ReadLength(buffer, offset, Math.Min(count, this.length));
                this.length -= result;
                Debug.Assert(0 <= this.length, "Read beyond expected length");
                return result;
            }

            /// <summary>Not supported.</summary>
            /// <param name="offset">The parameter is not used.</param>
            /// <param name="origin">The parameter is not used.</param>
            /// <returns>nothing</returns>
            public override long Seek(long offset, SeekOrigin origin)
            {
                throw Error.NotSupported();
            }

            /// <summary>Not supported.</summary>
            /// <param name="value">ignored</param>
            public override void SetLength(long value)
            {
                throw Error.NotSupported();
            }

            /// <summary>Not supported.</summary>
            /// <param name="buffer">The parameter is not used.</param>
            /// <param name="offset">The parameter is not used.</param>
            /// <param name="count">The parameter is not used.</param>
            public override void Write(byte[] buffer, int offset, int count)
            {
                throw Error.NotSupported();
            }

            /// <summary>Dispose of this nested stream.</summary>
            /// <param name="disposing">true if active dispose</param>
            protected override void Dispose(bool disposing)
            {
                base.Dispose(disposing);

                if (disposing && (null != this.target))
                {
                    if (this.target.disposeWithContentStreamDispose)
                    {
                        this.target.contentStream = null;
                        this.target.Dispose();
                    }
                    else if (0 < this.length)
                    {
                        if (null != this.target.reader)
                        {
                            this.target.Seek(this.length, SeekOrigin.Current);
                        }

                        this.length = 0;
                    }

                    this.target.ClearPreviousOperationInformation();
                }

                this.target = null;
            }
        }
    }
}
