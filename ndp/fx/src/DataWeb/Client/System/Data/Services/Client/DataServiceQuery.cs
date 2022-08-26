//---------------------------------------------------------------------
// <copyright file="DataServiceQuery.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// query base object
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Collections;
    using System.Linq;
    using System.Linq.Expressions;

    /// <summary>non-generic placeholder for generic implementation</summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1010", Justification = "required for this feature")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710", Justification = "required for this feature")]
    public abstract class DataServiceQuery : DataServiceRequest, IQueryable
    {
        /// <summary>internal constructor so that only our assembly can provide an implementation</summary>
        internal DataServiceQuery()
        {
        }

        /// <summary>Linq Expression</summary>
        public abstract Expression Expression
        {
            get;
        }

        /// <summary>Linq Query Provider</summary>
        public abstract IQueryProvider Provider
        {
            get;
        }

        /// <summary>Get an enumerator materializes the objects the Uri request.</summary>
        /// <returns>an enumerator</returns>
        /// <remarks>Expect derived class to override this with an explict interface implementation</remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1033", Justification = "required for this feature")]
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            throw Error.NotImplemented();
        }

#if !ASTORIA_LIGHT
        /// Synchronous methods not available
        /// <summary>
        /// Returns an IEnumerable from an Internet resource. 
        /// </summary>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        public IEnumerable Execute()
        {
            return this.ExecuteInternal();
        }
#endif

        /// <summary>
        /// Begins an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        public IAsyncResult BeginExecute(AsyncCallback callback, object state)
        {
            return this.BeginExecuteInternal(callback, state);
        }

        /// <summary>
        /// Ends an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="asyncResult">The pending request for a response. </param>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        public IEnumerable EndExecute(IAsyncResult asyncResult)
        {
            return this.EndExecuteInternal(asyncResult);
        }

#if !ASTORIA_LIGHT
        /// Synchronous methods not available
        /// <summary>
        /// Returns an IEnumerable from an Internet resource. 
        /// </summary>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        internal abstract IEnumerable ExecuteInternal();
#endif

        /// <summary>
        /// Begins an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        internal abstract IAsyncResult BeginExecuteInternal(AsyncCallback callback, object state);

        /// <summary>
        /// Ends an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="asyncResult">The pending request for a response. </param>
        /// <returns>An IEnumerable that contains the response from the Internet resource.</returns>
        internal abstract IEnumerable EndExecuteInternal(IAsyncResult asyncResult);
    }
}
