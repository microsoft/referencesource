//---------------------------------------------------------------------
// <copyright file="DataServiceStreamResponse.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// This class represents the response as a binary stream of data 
// and its metadata.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif

    /// <summary>
    /// Class which represents a stream response from the service.
    /// </summary>
    public sealed class DataServiceStreamResponse : IDisposable
    {
        /// <summary>The underlying web response</summary>
        private HttpWebResponse response;

        /// <summary>Lazy initialized cached response headers.</summary>
        private Dictionary<string, string> headers;

        /// <summary>
        /// Constructor for the response. This method is internal since we don't want users to create instances
        /// of this class.
        /// </summary>
        /// <param name="response">The web response to wrap.</param>
        internal DataServiceStreamResponse(HttpWebResponse response)
        {
            Debug.Assert(response != null, "Can't create a stream response object from a null response.");
            this.response = response;
        }

        /// <summary>
        /// Returns the content type of the response stream (ex. image/png).
        /// If the Content-Type header was not present in the response this property
        /// will return null.
        /// </summary>
        public string ContentType
        {
            get 
            {
                this.CheckDisposed();
                return this.response.Headers[XmlConstants.HttpContentType];
            }
        }

        /// <summary>
        /// Returns the content disposition of the response stream.
        /// If the Content-Disposition header was not present in the response this property
        /// will return null.
        /// </summary>
        public string ContentDisposition
        {
            get 
            {
                this.CheckDisposed();
                return this.response.Headers[XmlConstants.HttpContentDisposition];
            }
        }

        /// <summary>
        /// Returns a dictionary containing all the response headers returned from the retrieve request
        /// to obtain the stream.
        /// </summary>
        public Dictionary<string, string> Headers
        {
            get 
            {
                this.CheckDisposed();
                if (this.headers == null)
                {
                    this.headers = WebUtil.WrapResponseHeaders(this.response);
                }

                return this.headers;
            }
        }

        /// <summary>
        /// Returns the stream obtained from the data service. When reading from this stream
        /// the operations may throw if a network error occurs. This stream is read-only.
        /// 
        /// Caller must call Dispose/Close on either the returned stream or on the response
        /// object itself. Otherwise the network connection will be left open and the caller
        /// might run out of available connections.
        /// </summary>
        public Stream Stream
        {
            get
            {
                this.CheckDisposed();
                return this.response.GetResponseStream();
            }
        }

        #region IDisposable Members

        /// <summary>
        /// Disposes all resources held by this class. Namely the network stream.
        /// </summary>
        public void Dispose()
        {
            Util.Dispose(ref this.response);
        }

        #endregion

        /// <summary>Checks if the object has already been disposed. If so it throws the ObjectDisposedException.</summary>
        /// <exception cref="ObjectDisposedException">If the object has already been disposed.</exception>
        private void CheckDisposed()
        {
            if (this.response == null)
            {
                Error.ThrowObjectDisposed(this.GetType());
            }
        }
    }
}
