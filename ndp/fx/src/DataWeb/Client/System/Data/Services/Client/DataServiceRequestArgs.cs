//---------------------------------------------------------------------
// <copyright file="DataServiceRequestArgs.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// This class represents additional metadata to be applied to a request
// sent from the client to a data service.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Collections.Generic;

    /// <summary>
    /// This class represent additional metadata to be applied to a request
    /// sent from the client to a data service.
    /// </summary>
    public class DataServiceRequestArgs
    {
        /// <summary>
        /// The headers collection for the request.
        /// This is just a storage, no validation is done on this.
        /// </summary>
        private readonly Dictionary<string, string> headers;

        /// <summary>
        /// Constructs a new DataServiceRequestArgs instance
        /// </summary>
        public DataServiceRequestArgs()
        {
            this.headers = new Dictionary<string, string>(EqualityComparer<string>.Default);
        }

        /// <summary>
        /// Sets the mime type (ex. image/png) to be used when retrieving the stream.
        /// Note that no validation is done on the contents of this property.
        /// It is the responsibility of the user to format it correctly to be used
        /// as the value of an HTTP Accept header.
        /// </summary>
        public string AcceptContentType
        {
            get { return this.GetHeaderValue(XmlConstants.HttpRequestAccept); }
            set { this.SetHeaderValue(XmlConstants.HttpRequestAccept, value); }
        }

        /// <summary>
        /// Sets the Content-Type header to be used when sending the stream to the server.
        /// Note that no validation is done on the contents of this property.
        /// It is the responsibility of the user to format it correctly to be used
        /// as the value of an HTTP Content-Type header.
        /// </summary>
        public string ContentType
        {
            get { return this.GetHeaderValue(XmlConstants.HttpContentType); }
            set { this.SetHeaderValue(XmlConstants.HttpContentType, value);  }
        }

        /// <summary>
        /// Sets the Slug header to be used when sending the stream to the server.
        /// Note that no validation is done on the contents of this property.
        /// It is the responsibility of the user to format it correctly to be used
        /// as the value of an HTTP Slug header.
        /// </summary>
        public string Slug
        {
            get { return this.GetHeaderValue(XmlConstants.HttpSlug); }
            set { this.SetHeaderValue(XmlConstants.HttpSlug, value); }
        }

        /// <summary>
        /// Dictionary containing all the request headers to be used when retrieving the stream.
        /// The user should take care so as to not alter an HTTP header which will change
        /// the meaning of the request.
        /// No validation is performed on the header names or values.
        /// This class will not attempt to fix up any of the headers specified and
        /// will try to use them "as is".
        /// </summary>
        public Dictionary<string, string> Headers
        {
            get { return this.headers; }
        }

        /// <summary>
        /// Helper to return a value of the header.
        /// </summary>
        /// <param name="header">The name of the header to get.</param>
        /// <returns>The value of the header or null if the header is not set.</returns>
        private string GetHeaderValue(string header)
        {
            string value;
            if (!this.headers.TryGetValue(header, out value))
            {
                return null;
            }

            return value;
        }

        /// <summary>
        /// Helper to set a value of the header
        /// </summary>
        /// <param name="header">The name of the header to set.</param>
        /// <param name="value">The value to set for the header. If this is null the header will be removed.</param>
        private void SetHeaderValue(string header, string value)
        {
            if (value == null)
            {
                if (this.headers.ContainsKey(header))
                {
                    this.headers.Remove(header);
                }
            }
            else
            {
                this.headers[header] = value;
            }
        }
    }
}
