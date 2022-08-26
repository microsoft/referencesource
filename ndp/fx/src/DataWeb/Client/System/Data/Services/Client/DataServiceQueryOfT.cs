//---------------------------------------------------------------------
// <copyright file="DataServiceQueryOfT.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// query object
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Linq.Expressions;
#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif
    using System.Reflection;
    using System.Collections;

    /// <summary>
    /// query object
    /// </summary>
    /// <typeparam name="TElement">type of object to materialize</typeparam>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Justification = "required for this feature")]
    public class DataServiceQuery<TElement> : DataServiceQuery, IQueryable<TElement>
    {
        #region Private fields.

        /// <summary>Linq Expression</summary>
        private readonly Expression queryExpression;

        /// <summary>Linq Query Provider</summary>
        private readonly DataServiceQueryProvider queryProvider;

        /// <summary>Uri, Projection, Version for translated query</summary>
        private QueryComponents queryComponents;

        #endregion Private fields.

        /// <summary>
        /// query object
        /// </summary>
        /// <param name="expression">expression for query</param>
        /// <param name="provider">query provider for query</param>
        private DataServiceQuery(Expression expression, DataServiceQueryProvider provider)
        {
            Debug.Assert(null != provider.Context, "null context");
            Debug.Assert(expression != null, "null expression");
            Debug.Assert(provider is DataServiceQueryProvider, "Currently only support Web Query Provider");

            this.queryExpression = expression;
            this.queryProvider = provider;
        }

        #region IQueryable implementation
        /// <summary>
        /// Element Type
        /// </summary>
        public override Type ElementType
        {
            get { return typeof(TElement); }
        }

        /// <summary>
        /// Linq Expression
        /// </summary>
        public override Expression Expression
        {
            get { return this.queryExpression; }
        }

        /// <summary>
        /// Linq Query Provider
        /// </summary>
        public override IQueryProvider Provider
        {
            get { return this.queryProvider; }
        }

        #endregion

        /// <summary>
        /// gets the URI for a the query
        /// </summary>
        public override Uri RequestUri
        {
            get
            {
                return this.Translate().Uri;
            }
        }

        /// <summary>The ProjectionPlan for the request (if precompiled in a previous page).</summary>
        internal override ProjectionPlan Plan
        {
            get { return null; }
        }

        /// <summary>
        /// gets the UriTranslateResult for a the query
        /// </summary>
        internal override QueryComponents QueryComponents
        {
            get
            {
                return this.Translate();
            }
        }

        /// <summary>
        /// Begins an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        public new IAsyncResult BeginExecute(AsyncCallback callback, object state)
        {
            return base.BeginExecute(this, this.queryProvider.Context, callback, state);
        }

        /// <summary>
        /// Ends an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="asyncResult">The pending request for a response. </param>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        public new IEnumerable<TElement> EndExecute(IAsyncResult asyncResult)
        {
            return DataServiceRequest.EndExecute<TElement>(this, this.queryProvider.Context, asyncResult);
        }

#if !ASTORIA_LIGHT // Synchronous methods not available
        /// <summary>
        /// Returns an IEnumerable from an Internet resource. 
        /// </summary>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        public new IEnumerable<TElement> Execute()
        {
            return this.Execute<TElement>(this.queryProvider.Context, this.Translate());
        }
#endif

        /// <summary>
        /// Sets Expand option
        /// </summary>
        /// <param name="path">Path to expand</param>
        /// <returns>New DataServiceQuery with expand option</returns>
        public DataServiceQuery<TElement> Expand(string path)
        {
            Util.CheckArgumentNull(path, "path");
            Util.CheckArgumentNotEmpty(path, "path");

            MethodInfo mi = typeof(DataServiceQuery<TElement>).GetMethod("Expand");
            return (DataServiceQuery<TElement>)this.Provider.CreateQuery<TElement>(
                Expression.Call(
                    Expression.Convert(this.Expression, typeof(DataServiceQuery<TElement>.DataServiceOrderedQuery)),
                    mi,
                    new Expression[] { Expression.Constant(path) }));
        }

        /// <summary>
        /// Turn the inline counting option on for the query
        /// </summary>
        /// <returns>New DataServiceQuery with count option</returns>
        public DataServiceQuery<TElement> IncludeTotalCount()
        {
            MethodInfo mi = typeof(DataServiceQuery<TElement>).GetMethod("IncludeTotalCount");
            
            return (DataServiceQuery<TElement>)this.Provider.CreateQuery<TElement>(
                Expression.Call(
                    Expression.Convert(this.Expression, typeof(DataServiceQuery<TElement>.DataServiceOrderedQuery)),
                    mi));
        }

        /// <summary>
        /// Sets user option
        /// </summary>
        /// <param name="name">name of value</param>
        /// <param name="value">value of option</param>
        /// <returns>New DataServiceQuery with expand option</returns>
        public DataServiceQuery<TElement> AddQueryOption(string name, object value)
        {
            Util.CheckArgumentNull(name, "name");
            Util.CheckArgumentNull(value, "value");
            MethodInfo mi = typeof(DataServiceQuery<TElement>).GetMethod("AddQueryOption");
            return (DataServiceQuery<TElement>)this.Provider.CreateQuery<TElement>(
                Expression.Call(
                    Expression.Convert(this.Expression, typeof(DataServiceQuery<TElement>.DataServiceOrderedQuery)),
                    mi,
                    new Expression[] { Expression.Constant(name), Expression.Constant(value, typeof(object)) }));
        }

        /// <summary>
        /// get an enumerator materializes the objects the Uri request
        /// </summary>
        /// <returns>an enumerator</returns>
#if !ASTORIA_LIGHT // Synchronous methods not available
        public IEnumerator<TElement> GetEnumerator()
        {
            return this.Execute().GetEnumerator();            
        }
#else
        IEnumerator<TElement> IEnumerable<TElement>.GetEnumerator()
        {
            throw Error.NotSupported(Strings.DataServiceQuery_EnumerationNotSupportedInSL);
        }
#endif

        /// <summary>
        /// gets the URI for the query
        /// </summary>
        /// <returns>a string with the URI</returns>
        public override string ToString()
        {
            try
            {
                return base.ToString();
            }
            catch (NotSupportedException e)
            {
                return Strings.ALinq_TranslationError(e.Message);
            }
        }

        /// <summary>
        /// get an enumerator materializes the objects the Uri request
        /// </summary>
        /// <returns>an enumerator</returns>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
#if !ASTORIA_LIGHT // Synchronous methods not available
            return this.GetEnumerator();
#else
            throw Error.NotSupported();
#endif
        }

#if !ASTORIA_LIGHT
        /// Synchronous methods not available
        /// <summary>
        /// Returns an IEnumerable from an Internet resource. 
        /// </summary>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        internal override IEnumerable ExecuteInternal()
        {
            return this.Execute();
        }
#endif

        /// <summary>
        /// Begins an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        internal override IAsyncResult BeginExecuteInternal(AsyncCallback callback, object state)
        {
            return this.BeginExecute(callback, state);
        }

        /// <summary>
        /// Ends an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="asyncResult">The pending request for a response. </param>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        internal override IEnumerable EndExecuteInternal(IAsyncResult asyncResult)
        {
            return this.EndExecute(asyncResult);
        }

        /// <summary>
        /// gets the query components for the query after translating
        /// </summary>
        /// <returns>QueryComponents for query</returns>
        private QueryComponents Translate()
        {
            if (this.queryComponents == null)
            {
                this.queryComponents = this.queryProvider.Translate(this.queryExpression);
            }

            return this.queryComponents;
        }

        /// <summary>
        /// Ordered DataServiceQuery which implements IOrderedQueryable.
        /// </summary>
        internal class DataServiceOrderedQuery : DataServiceQuery<TElement>, IOrderedQueryable<TElement>, IOrderedQueryable
        {
            /// <summary>
            /// constructor
            /// </summary>
            /// <param name="expression">expression for query</param>
            /// <param name="provider">query provider for query</param>
            internal DataServiceOrderedQuery(Expression expression, DataServiceQueryProvider provider)
                : base(expression, provider)
            {
            }
        }
    }
}
