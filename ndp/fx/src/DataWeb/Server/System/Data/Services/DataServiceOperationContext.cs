//---------------------------------------------------------------------
// <copyright file="DataServiceOperationContext.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Class holding all the context about the current request being
//      processed.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces

    using System;
    using System.Diagnostics;
    using System.Net;

    #endregion Namespaces

    /// <summary>
    /// Class holding all the context about the current operation being processed.
    /// </summary>
    public sealed class DataServiceOperationContext
    {
        #region Private Fields

        /// <summary>
        /// Host interface for the current operation.
        /// </summary>
        private readonly IDataServiceHost hostInterface;

        /// <summary>
        /// Host wrapper for the current operation. The wrapper class caches the request header values and validates the data from the host interface.
        /// </summary>
        private DataServiceHostWrapper hostWrapper;

        /// <summary>
        /// True if the current operation is part of a batch request.
        /// </summary>
        private bool? isBatchRequest;

        #endregion Private Fields

        #region Constructor

        /// <summary>
        /// Constructs a new instance of DataServiceOperationContext object
        /// </summary>
        /// <param name="host">Host instance for the current operation context.</param>
        internal DataServiceOperationContext(IDataServiceHost host)
        {
            Debug.Assert(host != null, "host != null");
            this.hostInterface = host;
        }

        /// <summary>
        /// Constructs a new instance of DataServiceOperationContext object
        /// </summary>
        /// <param name="isBatchRequest">True if the current operation is part of a batch request.</param>
        /// <param name="host">Host instance for the current operation context.</param>
        internal DataServiceOperationContext(bool isBatchRequest, IDataServiceHost2 host)
            : this(host)
        {
            this.isBatchRequest = isBatchRequest;
        }

        #endregion Constructor

        #region Public Properties

        /// <summary>
        /// True if the current operation is part of a batch request.
        /// </summary>
        public bool IsBatchRequest
        {
            get
            {
                if (!this.isBatchRequest.HasValue)
                {
                    string[] segments = RequestUriProcessor.EnumerateSegments(this.AbsoluteRequestUri, this.AbsoluteServiceUri);
                    if (segments.Length > 0 && segments[0] == XmlConstants.UriBatchSegment)
                    {
                        this.isBatchRequest = true;
                    }
                    else
                    {
                        this.isBatchRequest = false;
                    }
                }

                return this.isBatchRequest.Value;
            }
        }

        /// <summary>
        /// The HTTP request method (GET, POST, etc.)
        /// </summary>
        public string RequestMethod
        {
            get { return this.hostWrapper.RequestHttpMethod; }
        }

        /// <summary>
        /// Request Uri for the current operation.
        /// </summary>
        public Uri AbsoluteRequestUri
        {
            get { return this.hostWrapper.AbsoluteRequestUri; }
        }

        /// <summary>
        /// Base service Uri for the request.
        /// </summary>
        public Uri AbsoluteServiceUri
        {
            get { return this.hostWrapper.AbsoluteServiceUri; }
        }

        /// <summary>
        /// Request headers for the current operation.
        /// </summary>
        public WebHeaderCollection RequestHeaders
        {
            get { return this.hostWrapper.RequestHeaders; }
        }

        /// <summary>
        /// Response headers for the current operation.
        /// </summary>
        public WebHeaderCollection ResponseHeaders
        {
            get { return this.hostWrapper.ResponseHeaders; }
        }

        /// <summary>
        /// Gets and sets the response status code
        /// </summary>
        public int ResponseStatusCode
        {
            get { return this.hostWrapper.ResponseStatusCode; }
            set { this.hostWrapper.ResponseStatusCode = value; }
        }

        #endregion Public Properties

        #region Internal Properties

        /// <summary>
        /// Host instance for the current operation.
        /// </summary>
        internal DataServiceHostWrapper Host
        {
            get
            {
                Debug.Assert(this.hostWrapper != null, "Must call InitializeAndCacheHeaders() before calling the Host property.");
                return this.hostWrapper;
            }
        }

        #endregion Internal Properties

        #region Internal Methods

        /// <summary>
        /// Creates a new instance of the host wrapper to cache the request headers and to validate the data from the host interface.
        /// </summary>
        internal void InitializeAndCacheHeaders()
        {
            Debug.Assert(this.hostInterface != null, "this.hostInterface != null");
            this.hostWrapper = new DataServiceHostWrapper(this.hostInterface);
        }

        #endregion Internal Methods
    }
}
