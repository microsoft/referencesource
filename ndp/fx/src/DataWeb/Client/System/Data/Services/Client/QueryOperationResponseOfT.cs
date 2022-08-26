//---------------------------------------------------------------------
// <copyright file="QueryOperationResponseOfT.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Response to a batched query.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System.Collections.Generic;
    using System.Linq;

    #endregion Namespaces.

    /// <summary>
    /// Response to a batched query.
    /// </summary>
    /// <typeparam name="T">The type to construct for the request results</typeparam>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710", Justification = "required for this feature")]
    public sealed class QueryOperationResponse<T> : QueryOperationResponse, IEnumerable<T>
    {
        #region Constructors.

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="headers">HTTP headers</param>
        /// <param name="query">original query</param>
        /// <param name="results">retrieved objects</param>
        internal QueryOperationResponse(Dictionary<string, string> headers, DataServiceRequest query, MaterializeAtom results)
            : base(headers, query, results)
        {
        }

        #endregion Constructors.

        #region Public properties.

        /// <summary>
        /// The server Result Set Count value from a query, if the query has requested the value
        /// </summary>
        /// <returns>
        /// The return value can be either a zero or positive value equals to number of entities in the set on the server
        /// </returns>
        /// <exception cref="InvalidOperationException">
        /// Is thrown when count was not part of the original request, i.e., if the count tag is not found in the response stream.
        /// </exception>
        public override long TotalCount
        {
            get
            {
                if (this.Results != null && !this.Results.IsEmptyResults)
                {
                    return this.Results.CountValue();
                }
                else
                {
                    throw new InvalidOperationException(Strings.MaterializeFromAtom_CountNotPresent);
                }
            }
        }

        #endregion Public properties.

        #region Public methods.

        /// <summary>
        /// Gets the continuation for the top level data stream associated with this response object
        /// </summary>
        /// <returns>A continuation that points to the next page for the current data stream in the response.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1024:UsePropertiesWhereAppropriate", Justification = "required for this feature")]
        public new DataServiceQueryContinuation<T> GetContinuation()
        {
            return (DataServiceQueryContinuation<T>)base.GetContinuation();
        }

        /// <summary>Results from a query</summary>
        /// <returns>enumerator of objects in query</returns>
        public new IEnumerator<T> GetEnumerator()
        {
            return this.Results.Cast<T>().GetEnumerator();
        }

        #endregion Public methods.
    }
}
