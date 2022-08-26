//---------------------------------------------------------------------
// <copyright file="GetReadStreamResult.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Implementation of IAsyncResult for GetReadStream operation.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Diagnostics;
#if !ASTORIA_LIGHT
    using System.Net;
#else // Data.Services http stack
    using System.Data.Services.Http;
#endif

    /// <summary>
    /// Class which implements the <see cref="IAsyncResult"/> for the GetReadStream operation.
    /// Note that this effectively behaves as a simple wrapper around the IAsyncResult returned
    /// by the underlying HttpWebRequest, although it's implemented fully on our own to get the same
    /// behavior as other IAsyncResult objects returned by the client library.
    /// </summary>
    internal class GetReadStreamResult : BaseAsyncResult
    {
        /// <summary>The web request this class wraps (effectively)</summary>
        private readonly HttpWebRequest request;

        /// <summary>The web response once we get it.</summary>
        private HttpWebResponse response;

        /// <summary>
        /// Constructs a new async result object
        /// </summary>
        /// <param name="source">The source of the operation.</param>
        /// <param name="method">Name of the method which is invoked asynchronously.</param>
        /// <param name="request">The <see cref="HttpWebRequest"/> object which is wrapped by this async result.</param>
        /// <param name="callback">User specified callback for the async operation.</param>
        /// <param name="state">User state for the async callback.</param>
        internal GetReadStreamResult(
            object source, 
            string method, 
            HttpWebRequest request, 
            AsyncCallback callback, 
            object state)
            : base(source, method, callback, state)
        {
            Debug.Assert(request != null, "Null request can't be wrapped to a result.");
            this.request = request;
            this.Abortable = request;
        }

        /// <summary>
        /// Begins the async request
        /// </summary>
        internal void Begin()
        {
            try
            {
                IAsyncResult asyncResult;
                asyncResult = BaseAsyncResult.InvokeAsync(this.request.BeginGetResponse, GetReadStreamResult.AsyncEndGetResponse, this);

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

        /// <summary>
        /// Ends the request and creates the response object.
        /// </summary>
        /// <returns>The response object for this request.</returns>
        internal DataServiceStreamResponse End()
        {
            if (this.response != null)
            {
                DataServiceStreamResponse streamResponse = new DataServiceStreamResponse(this.response);
                return streamResponse;
            }
            else 
            {
                return null;
            }
        }

#if !ASTORIA_LIGHT
        /// <summary>
        /// Executes the request synchronously.
        /// </summary>
        /// <returns>The response object for this request.</returns>
        internal DataServiceStreamResponse Execute()
        {
            try
            {
                System.Net.HttpWebResponse webresponse = null;
                try
                {
                    webresponse = (System.Net.HttpWebResponse)this.request.GetResponse();
                }
                catch (System.Net.WebException ex)
                {
                    webresponse = (System.Net.HttpWebResponse)ex.Response;
                    if (null == webresponse)
                    {
                        throw;
                    }
                }

                this.SetHttpWebResponse(webresponse);
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

            return this.End();
        }
#endif

        /// <summary>invoked for derived classes to cleanup before callback is invoked</summary>
        protected override void CompletedRequest()
        {
            Debug.Assert(null != this.response || null != this.Failure, "should have response or exception");
            if (null != this.response)
            {
                // Can't use DataServiceContext.HandleResponse as this request didn't necessarily go to our server
                //   the MR could have been served by arbitrary server.
                InvalidOperationException failure = null;
                if (!WebUtil.SuccessStatusCode(this.response.StatusCode))
                {
                    failure = DataServiceContext.GetResponseText(this.response.GetResponseStream, this.response.StatusCode);
                }

                if (failure != null)
                {
                    // we've cached off what we need, headers still accessible after close
                    this.response.Close();
                    this.HandleFailure(failure);
                }
            }
        }

        /// <summary>
        /// Async callback registered with the underlying HttpWebRequest object.
        /// </summary>
        /// <param name="asyncResult">The async result associated with the HttpWebRequest operation.</param>
        private static void AsyncEndGetResponse(IAsyncResult asyncResult)
        {
            GetReadStreamResult state = asyncResult.AsyncState as GetReadStreamResult;
            Debug.Assert(state != null, "Async callback got called for different request.");

            try
            {
                state.CompletedSynchronously &= asyncResult.CompletedSynchronously; // BeginGetResponse
                HttpWebRequest request = Util.NullCheck(state.request, InternalError.InvalidEndGetResponseRequest);

                HttpWebResponse webresponse = null;
                try
                {
                    webresponse = (HttpWebResponse)request.EndGetResponse(asyncResult);
                }
                catch (WebException e)
                {
                    webresponse = (HttpWebResponse)e.Response;
                    if (null == webresponse)
                    {
                        throw;
                    }
                }

                state.SetHttpWebResponse(webresponse);
                state.SetCompleted();
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
        /// Sets the response when received from the underlying HttpWebRequest
        /// </summary>
        /// <param name="webResponse">The response recieved.</param>
        private void SetHttpWebResponse(HttpWebResponse webResponse)
        {
            Debug.Assert(webResponse != null, "Can't set a null response.");
            this.response = webResponse;
        }
    }
}
