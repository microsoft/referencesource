//---------------------------------------------------------------------
// <copyright file="DataServiceResponse.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// data service response to ExecuteBatch &amp; SaveChanges
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Collections.Generic;

    /// <summary>
    /// Data service response to ExecuteBatch &amp; SaveChanges
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1010", Justification = "required for this feature")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710", Justification = "required for this feature")]
    public sealed class DataServiceResponse : IEnumerable<OperationResponse>
    {
        /// <summary>Http headers of the response.</summary>
        private Dictionary<string, string> headers;

        /// <summary>Http status code of the response.</summary>
        private int statusCode;

        /// <summary>responses</summary>
        private IEnumerable<OperationResponse> response;

        /// <summary>true if this is a batch response, otherwise false.</summary>
        private bool batchResponse;

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="headers">HTTP headers</param>
        /// <param name="statusCode">HTTP status code</param>
        /// <param name="response">list of responses</param>
        /// <param name="batchResponse">true if this represents a batch response, otherwise false.</param>
        internal DataServiceResponse(Dictionary<string, string> headers, int statusCode, IEnumerable<OperationResponse> response, bool batchResponse)
        {
            this.headers = headers ?? new Dictionary<string, string>(EqualityComparer<string>.Default);
            this.statusCode = statusCode;
            this.batchResponse = batchResponse;
            this.response = response;
        }

        /// <summary>Http headers of the response.</summary>
        public IDictionary<string, string> BatchHeaders
        {
            get { return this.headers; }
        }

        /// <summary>Http status code of the response.</summary>
        public int BatchStatusCode
        {
            get { return this.statusCode; }
        }

        /// <summary> Returns true if this is a batch response. Otherwise returns false.</summary>
        public bool IsBatchResponse
        {
            get { return this.batchResponse; }
        }

        /// <summary>Responses of a batch query operation.</summary>
        /// <returns>The sequence of respones to operation</returns>
        public IEnumerator<OperationResponse> GetEnumerator()
        {
            return this.response.GetEnumerator();
        }

        /// <summary>Get an enumerator for the OperationResponse.</summary>
        /// <returns>an enumerator</returns>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }
}
