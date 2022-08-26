//---------------------------------------------------------------------
// <copyright file="QueryResponse.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// Response to a batched query.
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Diagnostics;

    #endregion Namespaces.

    /// <summary>
    /// Response to a batched query.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1010", Justification = "required for this feature")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710", Justification = "required for this feature")]
    public class QueryOperationResponse : OperationResponse, System.Collections.IEnumerable
    {
        #region Private fields.

        /// <summary>Original query</summary>
        private readonly DataServiceRequest query;

        /// <summary>Enumerable of objects in query</summary>
        private readonly MaterializeAtom results;

        #endregion Private fields.

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="headers">HTTP headers</param>
        /// <param name="query">original query</param>
        /// <param name="results">retrieved objects</param>
        internal QueryOperationResponse(Dictionary<string, string> headers, DataServiceRequest query, MaterializeAtom results)
            : base(headers)
        {
            this.query = query;
            this.results = results;
        }

        /// <summary>The query that executed within the batch.</summary>
        public DataServiceRequest Query
        {
            get { return this.query; }
        }

        /// <summary>
        /// The server Result Set Count value from a query, if the query has requested the value
        /// </summary>
        /// <returns>
        /// The return value can be either a zero or positive value equals to number of entities in the set on the server
        /// </returns>
        /// <exception cref="InvalidOperationException">
        /// Is thrown when count was not part of the original request, i.e., if the count tag is not found in the response stream.
        /// </exception>
        public virtual long TotalCount
        {
            get
            {
                throw new NotSupportedException();
            }
        }

        /// <summary>get a non-null enumerable of the result</summary>
        internal MaterializeAtom Results
        {
            get
            {
                if (null != this.Error)
                {
                    throw System.Data.Services.Client.Error.InvalidOperation(Strings.Context_BatchExecuteError, this.Error);
                }

                return this.results;
            }
        }

        /// <summary>Results from a query</summary>
        /// <returns>enumerator of objects in query</returns>
        public System.Collections.IEnumerator GetEnumerator()
        {
            return this.Results.GetEnumerator();
        }

        /// <summary>
        /// Gets the continuation for the top level data stream associated with this response object
        /// </summary>
        /// <returns>A continuation that points to the next page for the current data stream in the response.</returns>
        public DataServiceQueryContinuation GetContinuation()
        {
            return this.results.GetContinuation(null);
        }

        /// <summary>
        /// Gets the continuation for the next page in the collection.
        /// </summary>
        /// <param name="collection">The collection, or null, if the top level link is to be retrieved</param>
        /// <returns>A continuation that points to the next page for the collection</returns>
        public DataServiceQueryContinuation GetContinuation(IEnumerable collection)
        {
            return this.results.GetContinuation(collection);
        }

        /// <summary>
        /// Gets the continuation for the next page in the collection.
        /// </summary>
        /// <typeparam name="T">Element type for continuation results.</typeparam>
        /// <param name="collection">The collection, or null, if the top level link is to be retrieved</param>
        /// <returns>A continuation that points to the next page for the collection</returns>
        public DataServiceQueryContinuation<T> GetContinuation<T>(IEnumerable<T> collection)
        {
            return (DataServiceQueryContinuation<T>)this.results.GetContinuation(collection);
        }

        /// <summary>
        /// Creates a generic instance of the QueryOperationResponse and return it
        /// </summary>
        /// <param name="elementType">generic type for the QueryOperationResponse.</param>
        /// <param name="headers">constructor parameter1</param>
        /// <param name="query">constructor parameter2</param>
        /// <param name="results">constructor parameter3</param>
        /// <returns>returns a new strongly typed instance of QueryOperationResponse.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static QueryOperationResponse GetInstance(Type elementType, Dictionary<string, string> headers, DataServiceRequest query, MaterializeAtom results)
        {
            Type genericType = typeof(QueryOperationResponse<>).MakeGenericType(elementType);
#if !ASTORIA_LIGHT
            return (QueryOperationResponse)Activator.CreateInstance(
                genericType,
                BindingFlags.CreateInstance | BindingFlags.NonPublic | BindingFlags.Instance,
                null,
                new object[] { headers, query, results },
                System.Globalization.CultureInfo.InvariantCulture);
#else
            System.Reflection.ConstructorInfo[] info = genericType.GetConstructors(BindingFlags.NonPublic | BindingFlags.Instance);
            System.Diagnostics.Debug.Assert(1 == info.Length, "only expected 1 ctor");
            return (QueryOperationResponse)Util.ConstructorInvoke(info[0],new object[] { headers, query, results });
#endif
        }
    }
}
