//---------------------------------------------------------------------
// <copyright file="OperationResponse.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Operation response base class
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;

    /// <summary>Operation response base class</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1010", Justification = "required for this feature")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710", Justification = "required for this feature")]
    public abstract class OperationResponse
    {
        /// <summary>Http headers of the response.</summary>
        private Dictionary<string, string> headers;

        /// <summary>Http status code of the response.</summary>
        private int statusCode;

        /// <summary>exception to throw during get results</summary>
        private Exception innerException;

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="headers">HTTP headers</param>
        internal OperationResponse(Dictionary<string, string> headers)
        {
            Debug.Assert(null != headers, "null headers");
            this.headers = headers;
        }

        /// <summary>Http headers of the response.</summary>
        public IDictionary<string, string> Headers
        {
            get { return this.headers; }
        }

        /// <summary>Http status code of the response.</summary>
        public int StatusCode
        {
            get { return this.statusCode; }
            internal set { this.statusCode = value; }
        }

        /// <summary>Get and set the exception object if this response had a failure</summary>
        public Exception Error
        {
            get
            {
                return this.innerException;
            }

            set
            {
                Debug.Assert(null != value, "should not set null");
                this.innerException = value;
            }
        }
    }
}
