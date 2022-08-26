//---------------------------------------------------------------------
// <copyright file="ProcessRequestArgs.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class to provide information about the request
//      that is going to be processed.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;

    /// <summary>Use this class to look at the request uri and doing some custom validation.</summary>
    public sealed class ProcessRequestArgs
    {
        #region Private fields.

        /// <summary>The uri for this request.</summary>
        private readonly Uri requestUri;

        /// <summary>True if this request is a operation specified within a batch request, otherwise false.</summary>
        private readonly bool isBatchOperation;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initalizes a new <see cref="ProcessRequestArgs"/> instance.</summary>
        /// <param name="requestUri">The uri for this request.</param>
        /// <param name="isBatchOperation">True if this request is a operation specified within a batch request, otherwise false.</param>
        /// <param name="operationContext">Context about the current operation being processed.</param>
        internal ProcessRequestArgs(Uri requestUri, bool isBatchOperation, DataServiceOperationContext operationContext)
        {
            System.Diagnostics.Debug.Assert(requestUri != null, "requestUri != null");

            this.requestUri = requestUri;
            this.isBatchOperation = isBatchOperation;
            this.OperationContext = operationContext;
        }

        #endregion Constructors.

        #region Public Properties.

        /// <summary>The uri for this request that is about to get processed.</summary>
        public Uri RequestUri
        {
            get { return this.requestUri; }
        }

        /// <summary>Returns true if this request is a operation specified within a batch request, otherwise returns false.</summary>
        public bool IsBatchOperation
        {
            get { return this.isBatchOperation; }
        }

        /// <summary>Context about the current operation being processed.</summary>
        public DataServiceOperationContext OperationContext
        {
            get;
            private set;
        }

        #endregion Public Properties.
    }
}
