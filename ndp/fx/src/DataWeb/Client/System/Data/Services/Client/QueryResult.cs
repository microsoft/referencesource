//---------------------------------------------------------------------
// <copyright file="QueryResult.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// query object
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif

    /// <summary>
    /// Wrapper HttpWebRequest &amp; HttWebResponse
    /// </summary>
    internal class QueryResult : BaseAsyncResult
    {
        /// <summary>Originating service request</summary>
        internal readonly DataServiceRequest ServiceRequest;

        /// <summary>Originating WebRequest</summary>
        internal readonly HttpWebRequest Request;

        /// <summary>reusuable async copy buffer</summary>
        private static byte[] reusableAsyncCopyBuffer;

        /// <summary>content to write to request stream</summary>
        private MemoryStream requestStreamContent;

        /// <summary>active web request stream</summary>
        private Stream requestStream;

        /// <summary>web response, closed when completed</summary>
        private HttpWebResponse httpWebResponse;

        /// <summary>async response stream, closed when completed</summary>
        private Stream asyncResponseStream;

        /// <summary>buffer when copying async stream to response stream cache</summary>
        private byte[] asyncStreamCopyBuffer;

        /// <summary>response stream, returned to other parts of the system</summary>
        /// <remarks>with async, the asyncResponseStream is copied into this stream</remarks>
        private Stream responseStream;

        /// <summary>copy of HttpWebResponse.ContentType</summary>
        private string contentType;

        /// <summary>copy of HttpWebResponse.ContentLength</summary>
        private long contentLength;

        /// <summary>copy of HttpWebResponse.StatusCode</summary>
        private HttpStatusCode statusCode;

        /// <summary>
        /// does this own the response stream or does the container of this QueryAsyncResult?
        /// </summary>
        private bool responseStreamOwner;

        /// <summary>
        /// if the BeginRead has been called with asyncStreamCopyBuffer, but EndRead has not.
        /// do not return the buffer to general pool if any question of it being in use.
        /// </summary>
        private bool usingBuffer;

#if StreamContainsBuffer
        /// <summary>does the responseStream contain the asyncStreamCopyBuffer?</summary>
        /// <remarks>optimize reading in to the async buffer without copying into separate response stream</remarks>
        private bool responseStreamIsCopyBuffer;
#endif

        /// <summary>constructor</summary>
        /// <param name="source">source object of async request</param>
        /// <param name="method">async method name on source object</param>
        /// <param name="serviceRequest">Originating serviceRequest</param>
        /// <param name="request">Originating WebRequest</param>
        /// <param name="callback">user callback</param>
        /// <param name="state">user state</param>
        internal QueryResult(object source, string method, DataServiceRequest serviceRequest, HttpWebRequest request, AsyncCallback callback, object state)
            : base(source, method, callback, state)
        {
            Debug.Assert(null != request, "null request");
            this.ServiceRequest = serviceRequest;
            this.Request = request;
            this.Abortable = request;
        }

        #region HttpResponse wrapper - ContentLength, ContentType, StatusCode

        /// <summary>HttpWebResponse.ContentLength</summary>
        internal long ContentLength
        {
            get { return this.contentLength; }
        }

        /// <summary>HttpWebResponse.ContentType</summary>
        internal string ContentType
        {
            get { return this.contentType; }
        }

        /// <summary>HttpWebResponse.StatusCode</summary>
        internal HttpStatusCode StatusCode
        {
            get { return this.statusCode; }
        }

        #endregion

        /// <summary>
        /// Ends the asynchronous query request.
        /// </summary>
        /// <typeparam name="TElement">Element type of the result.</typeparam>
        /// <param name="source">Source object of async request.</param>
        /// <param name="asyncResult">The asyncResult being ended.</param>
        /// <returns>Data service response.</returns>
        internal static QueryResult EndExecute<TElement>(object source, IAsyncResult asyncResult)
        {
            QueryResult response = null;

            try
            {
                response = BaseAsyncResult.EndExecute<QueryResult>(source, "Execute", asyncResult);
            }
            catch (InvalidOperationException ex)
            {
                response = asyncResult as QueryResult;
                Debug.Assert(response != null, "response != null, BaseAsyncResult.EndExecute() would have thrown a different exception otherwise.");

                QueryOperationResponse operationResponse = response.GetResponse<TElement>(MaterializeAtom.EmptyResults);
                if (operationResponse != null)
                {
                    operationResponse.Error = ex;
                    throw new DataServiceQueryException(Strings.DataServiceException_GeneralError, ex, operationResponse);
                }

                throw;
            }

            return response;
        }

        /// <summary>wrapper for HttpWebResponse.GetResponseStream</summary>
        /// <returns>stream</returns>
        internal Stream GetResponseStream()
        {
            return this.responseStream;
        }

        /// <summary>start the asynchronous request</summary>
        internal void BeginExecute()
        {
            try
            {
                IAsyncResult asyncResult;
#if false
                if ((null != requestContent) && (0 < requestContent.Length))
                {
                    requestContent.Position = 0;
                    this.requestStreamContent = requestContent;
                    this.Request.ContentLength = requestContent.Length;
                    asyncResult = this.Request.BeginGetRequestStream(QueryAsyncResult.AsyncEndGetRequestStream, this);
                }
                else
#endif
                {
                    asyncResult = BaseAsyncResult.InvokeAsync(this.Request.BeginGetResponse, QueryResult.AsyncEndGetResponse, this);
                }

                this.CompletedSynchronously &= asyncResult.CompletedSynchronously;
            }
            catch (Exception e)
            {
                this.HandleFailure(e);
                throw;
            }
            finally
            {
                this.HandleCompleted();
            }

            Debug.Assert(!this.CompletedSynchronously || this.IsCompleted, "if CompletedSynchronously then MUST IsCompleted");
        }

#if !ASTORIA_LIGHT
        /// <summary>Synchronous web request</summary>
        internal void Execute()
        {
            try
            {
#if false
                if ((null != requestContent) && (0 < requestContent.Length))
                {
                    using (System.IO.Stream stream = Util.NullCheck(this.Request.GetRequestStream(), InternalError.InvalidGetRequestStream))
                    {
                        byte[] buffer = requestContent.GetBuffer();
                        int bufferOffset = checked((int)requestContent.Position);
                        int bufferLength = checked((int)requestContent.Length) - bufferOffset;

                        // the following is useful in the debugging Immediate Window
                        // string x = System.Text.Encoding.UTF8.GetString(buffer, bufferOffset, bufferLength);
                        stream.Write(buffer, bufferOffset, bufferLength);
                    }
                }
#endif

                HttpWebResponse response = null;
                try
                {
                    response = (HttpWebResponse)this.Request.GetResponse();
                }
                catch (WebException ex)
                {
                    response = (HttpWebResponse)ex.Response;
                    if (null == response)
                    {
                        throw;
                    }
                }

                this.SetHttpWebResponse(Util.NullCheck(response, InternalError.InvalidGetResponse));

                if (HttpStatusCode.NoContent != this.StatusCode)
                {
                    using (Stream stream = this.httpWebResponse.GetResponseStream())
                    {
                        if (null != stream)
                        {
                            Stream copy = this.GetAsyncResponseStreamCopy();
                            this.responseStream = copy;

                            Byte[] buffer = this.GetAsyncResponseStreamCopyBuffer();

                            long copied = WebUtil.CopyStream(stream, copy, ref buffer);
                            if (this.responseStreamOwner)
                            {
                                if (0 == copied)
                                {
                                    this.responseStream = null;
                                }
                                else if (copy.Position < copy.Length)
                                {   // In Silverlight, generally 3 bytes less than advertised by ContentLength are read
                                    ((MemoryStream)copy).SetLength(copy.Position);
                                }
                            }

                            this.PutAsyncResponseStreamCopyBuffer(buffer);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                this.HandleFailure(e);
                throw;
            }
            finally
            {
                this.SetCompleted();
                this.CompletedRequest();
            }

            if (null != this.Failure)
            {
                throw this.Failure;
            }
        }
#endif

        /// <summary>
        /// Returns the response for the request.
        /// </summary>
        /// <param name="results">materialized results for the request.</param>
        /// <typeparam name="TElement">element type of the results.</typeparam>
        /// <returns>returns the instance of QueryOperationResponse containing the response.</returns>
        internal QueryOperationResponse<TElement> GetResponse<TElement>(MaterializeAtom results)
        {
            if (this.httpWebResponse != null)
            {
                Dictionary<string, string> headers = WebUtil.WrapResponseHeaders(this.httpWebResponse);
                QueryOperationResponse<TElement> response = new QueryOperationResponse<TElement>(headers, this.ServiceRequest, results);
                response.StatusCode = (int)this.httpWebResponse.StatusCode;
                return response;
            }

            return null;
        }

        /// <summary>
        /// Returns the response for the request.
        /// </summary>
        /// <param name="results">materialized results for the request.</param>
        /// <param name="elementType">element type of the results.</param>
        /// <returns>returns the instance of QueryOperationResponse containing the response.</returns>
        internal QueryOperationResponse GetResponseWithType(MaterializeAtom results, Type elementType)
        {
            if (this.httpWebResponse != null)
            {
                Dictionary<string, string> headers = WebUtil.WrapResponseHeaders(this.httpWebResponse);
                QueryOperationResponse response = QueryOperationResponse.GetInstance(elementType, headers, this.ServiceRequest, results);
                response.StatusCode = (int)this.httpWebResponse.StatusCode;
                return response;
            }

            return null;
        }

        /// <summary>
        /// Create materializer on top of response stream
        /// </summary>
        /// <param name="context">The data service context</param>
        /// <param name="plan">Precompiled projection plan (possibly null).</param>
        /// <returns>A materializer instance ready to deserialize ther result</returns>
        internal MaterializeAtom GetMaterializer(DataServiceContext context, ProjectionPlan plan)
        {
            Debug.Assert(this.IsCompletedInternally, "request hasn't completed yet");

            MaterializeAtom materializer;
            if (HttpStatusCode.NoContent != this.StatusCode)
            {
                materializer = DataServiceRequest.Materialize(context, this.ServiceRequest.QueryComponents, plan, this.ContentType, this.GetResponseStream());
            }
            else
            {
                materializer = MaterializeAtom.EmptyResults;
            }

            return materializer;
        }
        
        /// <summary>
        /// Processes the result for successfull request and produces the actual result of the request.
        /// </summary>
        /// <typeparam name="TElement">Element type of the result.</typeparam>
        /// <param name="context">The data service context.</param>
        /// <param name="plan">The plan to use for the projection, if available in precompiled form.</param>
        /// <returns>A instance of QueryResponseResult created on top of of the request.</returns>
        internal QueryOperationResponse<TElement> ProcessResult<TElement>(DataServiceContext context, ProjectionPlan plan)
        {
            MaterializeAtom materializeAtom = DataServiceRequest.Materialize(context, this.ServiceRequest.QueryComponents, plan, this.ContentType, this.GetResponseStream());
            return this.GetResponse<TElement>(materializeAtom);
        }
        
        /// <summary>cleanup work to do once the request has completed</summary>
        protected override void CompletedRequest()
        {
            Util.Dispose(ref this.asyncResponseStream);
            Util.Dispose(ref this.requestStream);
            Util.Dispose(ref this.requestStreamContent);

            byte[] buffer = this.asyncStreamCopyBuffer;
            this.asyncStreamCopyBuffer = null;
#if StreamContainsBuffer
            if (!this.responseStreamIsCopyBuffer)
#endif
            if ((null != buffer) && !this.usingBuffer)
            {
                this.PutAsyncResponseStreamCopyBuffer(buffer);
            }

            if (this.responseStreamOwner)
            {
                if (null != this.responseStream)
                {
                    this.responseStream.Position = 0;
                }
            }

            Debug.Assert(null != this.httpWebResponse || null != this.Failure, "should have response or exception");
            if (null != this.httpWebResponse)
            {
                // we've cached off what we need, headers still accessible after close
                this.httpWebResponse.Close();

                Exception ex = DataServiceContext.HandleResponse(this.StatusCode, this.httpWebResponse.Headers[XmlConstants.HttpDataServiceVersion], this.GetResponseStream, false);
                if (null != ex)
                {
                    this.HandleFailure(ex);
                }
            }
        }

        /// <summary>get stream which of copy buffer (via response stream) will be copied into</summary>
        /// <returns>writtable stream, happens before GetAsyncResponseStreamCopyBuffer</returns>
        protected virtual Stream GetAsyncResponseStreamCopy()
        {
            this.responseStreamOwner = true;

            long length = this.contentLength;
            if ((0 < length) && (length <= Int32.MaxValue))
            {
                Debug.Assert(null == this.asyncStreamCopyBuffer, "not expecting buffer");

#if StreamContainsBuffer
                byte[] buffer = new byte[(int)length];
                if (length < UInt16.MaxValue)
                {   // larger than this appears to cause trouble, specifically tested with 2619442
                    responseStreamIsCopyBuffer = true;
                    this.asyncStreamCopyBuffer = buffer;
                }
                return new MemoryStream(buffer, 0, buffer.Length, true, true);
#else
                // SQLBU 644097 - if more content is returned than specified we want the memory
                // stream to be expandable which doesn't happen if you preallocate a buffer
                return new MemoryStream((int)length);
#endif
            }

            return new MemoryStream();
        }

        /// <summary>get buffer which response stream will be copied into</summary>
        /// <returns>writtable stream</returns>
        protected virtual byte[] GetAsyncResponseStreamCopyBuffer()
        {   // consider having a cache of these buffers since they will be pinned
            Debug.Assert(null == this.asyncStreamCopyBuffer, "non-null this.asyncStreamCopyBuffer");
            return System.Threading.Interlocked.Exchange(ref reusableAsyncCopyBuffer, null) ?? new byte[8000];
        }

        /// <summary>returning a buffer after being done with it</summary>
        /// <param name="buffer">buffer to return</param>
        protected virtual void PutAsyncResponseStreamCopyBuffer(byte[] buffer)
        {
            reusableAsyncCopyBuffer = buffer;
        }

        /// <summary>set the http web response</summary>
        /// <param name="response">response object</param>
        protected virtual void SetHttpWebResponse(HttpWebResponse response)
        {
            this.httpWebResponse = response;
            this.statusCode = response.StatusCode;
            this.contentLength = response.ContentLength;
            this.contentType = response.ContentType;
        }

#if false
        /// <summary>handle request.BeginGetRequestStream with request.EndGetRquestStream and then write out request stream</summary>
        /// <param name="asyncResult">async result</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
        private static void AsyncEndGetRequestStream(IAsyncResult asyncResult)
        {
            QueryAsyncResult state = asyncResult.AsyncState as QueryAsyncResult;
            try
            {
                int step = CompleteCheck(state, InternalError.InvalidEndGetRequestCompleted);
                state.CompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginGetRequestStream

                HttpWebRequest httpWebRequest = Util.NullCheck(state.Request, InternalError.InvalidEndGetRequestStreamRequest);

                Stream stream = Util.NullCheck(httpWebRequest.EndGetRequestStream(asyncResult), InternalError.InvalidEndGetRequestStreamStream);
                state.requestStream = stream;

                MemoryStream memoryStream = Util.NullCheck(state.requestStreamContent, InternalError.InvalidEndGetRequestStreamContent);
                byte[] buffer = memoryStream.GetBuffer();
                int bufferOffset = checked((int)memoryStream.Position);
                int bufferLength = checked((int)memoryStream.Length) - bufferOffset;
                if ((null == buffer) || (0 == bufferLength))
                {
                    Error.ThrowInternalError(InternalError.InvalidEndGetRequestStreamContentLength);
                }

                // the following is useful in the debugging Immediate Window
                // string x = System.Text.Encoding.UTF8.GetString(buffer, bufferOffset, bufferLength);
                asyncResult = stream.BeginWrite(buffer, bufferOffset, bufferLength, QueryAsyncResult.AsyncEndWrite, state);

                bool reallyCompletedSynchronously = asyncResult.CompletedSynchronously && (step < state.asyncCompleteStep);
                state.CompletedSynchronously &= reallyCompletedSynchronously; // BeginWrite
            }
            catch (Exception e)
            {
                if (state.HandleFailure(e))
                {
                    throw;
                }
            }
            finally
            {
                state.HandleCompleted();
            }
        }

        /// <summary>handle reqestStream.BeginWrite with requestStream.EndWrite then BeginGetResponse</summary>
        /// <param name="asyncResult">async result</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
        private static void AsyncEndWrite(IAsyncResult asyncResult)
        {
            QueryAsyncResult state = asyncResult.AsyncState as QueryAsyncResult;
            try
            {
                int step = CompleteCheck(state, InternalError.InvalidEndWriteCompleted);
                state.CompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginWrite

                HttpWebRequest httpWebRequest = Util.NullCheck(state.Request, InternalError.InvalidEndWriteRequest);

                Stream stream = Util.NullCheck(state.requestStream, InternalError.InvalidEndWriteStream);
                stream.EndWrite(asyncResult);

                state.requestStream = null;
                stream.Dispose();

                stream = state.requestStreamContent;
                if (null != stream)
                {
                    state.requestStreamContent = null;
                    stream.Dispose();
                }

                asyncResult = httpWebRequest.BeginGetResponse(QueryAsyncResult.AsyncEndGetResponse, state);

                bool reallyCompletedSynchronously = asyncResult.CompletedSynchronously && (step < state.asyncCompleteStep);
                state.CompletedSynchronously &= reallyCompletedSynchronously; // BeginGetResponse
            }
            catch (Exception e)
            {
                if (state.HandleFailure(e))
                {
                    throw;
                }
            }
            finally
            {
                state.HandleCompleted();
            }
        }
#endif

        /// <summary>handle request.BeginGetResponse with request.EndGetResponse and then copy response stream</summary>
        /// <param name="asyncResult">async result</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
        private static void AsyncEndGetResponse(IAsyncResult asyncResult)
        {
            Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
            QueryResult state = asyncResult.AsyncState as QueryResult;
            try
            {
                CompleteCheck(state, InternalError.InvalidEndGetResponseCompleted);
                state.CompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginGetResponse

                HttpWebRequest httpWebRequest = Util.NullCheck(state.Request, InternalError.InvalidEndGetResponseRequest);

                // the httpWebResponse is kept for batching, discarded by non-batch
                HttpWebResponse response = null;
                try
                {
                    response = (HttpWebResponse)httpWebRequest.EndGetResponse(asyncResult);
                }
                catch (WebException e)
                {
                    response = (HttpWebResponse)e.Response;
                    if (null == response)
                    {
                        throw;
                    }
                }

                state.SetHttpWebResponse(Util.NullCheck(response, InternalError.InvalidEndGetResponseResponse));
                Debug.Assert(null == state.asyncResponseStream, "non-null asyncResponseStream");

                Stream stream = null;
                if (HttpStatusCode.NoContent != response.StatusCode)
                {
                    stream = response.GetResponseStream();
                    state.asyncResponseStream = stream;
                }

                if ((null != stream) && stream.CanRead)
                {
                    if (null == state.responseStream)
                    {   // this is the stream we copy the reponse to
                        state.responseStream = Util.NullCheck(state.GetAsyncResponseStreamCopy(), InternalError.InvalidAsyncResponseStreamCopy);
                    }

                    if (null == state.asyncStreamCopyBuffer)
                    {   // this is the buffer we read into and copy out of
                        state.asyncStreamCopyBuffer = Util.NullCheck(state.GetAsyncResponseStreamCopyBuffer(), InternalError.InvalidAsyncResponseStreamCopyBuffer);
                    }

                    // Make async calls to read the response stream
                    QueryResult.ReadResponseStream(state);
                }
                else
                {
                    state.SetCompleted();
                }
            }
            catch (Exception e)
            {
                if (state.HandleFailure(e))
                {
                    throw;
                }
            }
            finally
            {
                state.HandleCompleted();
            }
        }

        /// <summary>
        /// Make async calls to read the response stream.
        /// </summary>
        /// <param name="queryResult">query result to populate</param>
        private static void ReadResponseStream(QueryResult queryResult)
        {
            IAsyncResult asyncResult;

            byte[] buffer = queryResult.asyncStreamCopyBuffer;
            Stream stream = queryResult.asyncResponseStream;
            do
            {
                int bufferOffset, bufferLength;
#if StreamContainsBuffer
                if (state.responseStreamIsCopyBuffer)
                {   // we may have asked for, but not received the entire stream
                    bufferOffset = checked((int)state.responseStream.Position);
                    bufferLength = buffer.Length - bufferOffset;
                }
                else
#endif
                {
                    bufferOffset = 0;
                    bufferLength = buffer.Length;
                }

                queryResult.usingBuffer = true;
                asyncResult = BaseAsyncResult.InvokeAsync(stream.BeginRead, buffer, bufferOffset, bufferLength, QueryResult.AsyncEndRead, queryResult);
                queryResult.CompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginRead
            }
            while (asyncResult.CompletedSynchronously && !queryResult.IsCompletedInternally && stream.CanRead);

            Debug.Assert(!queryResult.CompletedSynchronously || queryResult.IsCompletedInternally, "AsyncEndGetResponse !IsCompleted");
        }

        /// <summary>handle responseStream.BeginRead with responseStream.EndRead</summary>
        /// <param name="asyncResult">async result</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "required for this feature")]
        private static void AsyncEndRead(IAsyncResult asyncResult)
        {
            Debug.Assert(asyncResult != null && asyncResult.IsCompleted, "asyncResult.IsCompleted");
            QueryResult state = asyncResult.AsyncState as QueryResult;
            int count = 0;
            try
            {
                CompleteCheck(state, InternalError.InvalidEndReadCompleted);
                state.CompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginRead

                Stream stream = Util.NullCheck(state.asyncResponseStream, InternalError.InvalidEndReadStream);
                Stream outputResponse = Util.NullCheck(state.responseStream, InternalError.InvalidEndReadCopy);
                byte[] buffer = Util.NullCheck(state.asyncStreamCopyBuffer, InternalError.InvalidEndReadBuffer);

                count = stream.EndRead(asyncResult);
                state.usingBuffer = false;
                if (0 < count)
                {
#if StreamContainsBuffer
                    if (state.responseStreamIsCopyBuffer)
                    {   // we may have asked for, but not received the entire stream
                        outputResponse.Position = outputResponse.Position + count;
                    }
                    else
#endif
                    {
                        outputResponse.Write(buffer, 0, count);
                    }
                }

                if (0 < count && 0 < buffer.Length && stream.CanRead)
                {
                    if (!asyncResult.CompletedSynchronously)
                    {
                        // if CompletedSynchronously then caller will call and we reduce risk of stack overflow
                        QueryResult.ReadResponseStream(state);
                    }
                }
                else
                {
#if StreamContainsBuffer
                    Debug.Assert(!state.responseStreamIsCopyBuffer || (outputResponse.Position == outputResponse.Length), "didn't read expected count");
#endif
                    // Debug.Assert(state.ContentLength < 0 || outputResponse.Length == state.ContentLength, "didn't read expected ContentLength");
                    if (outputResponse.Position < outputResponse.Length)
                    {
                        // In Silverlight, generally 3 bytes less than advertised by ContentLength are read
                        ((MemoryStream)outputResponse).SetLength(outputResponse.Position);
                    }

                    state.SetCompleted();
                }
            }
            catch (Exception e)
            {
                if (state.HandleFailure(e))
                {
                    throw;
                }
            }
            finally
            {
                state.HandleCompleted();
            }
        }

        /// <summary>verify non-null and not completed</summary>
        /// <param name="pereq">async result</param>
        /// <param name="errorcode">error code if null or completed</param>
        private static void CompleteCheck(QueryResult pereq, InternalError errorcode)
        {
            if ((null == pereq) || (pereq.IsCompletedInternally && !pereq.IsAborted))
            {
                // if aborting, let the request throw its abort code
                Error.ThrowInternalError(errorcode);
            }
        }
    }
}
