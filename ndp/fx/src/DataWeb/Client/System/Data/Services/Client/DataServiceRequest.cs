//---------------------------------------------------------------------
// <copyright file="DataServiceRequest.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// query request object
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif
    using System.Xml;

    /// <summary>non-generic placeholder for generic implementation</summary>
    public abstract class DataServiceRequest
    {
        /// <summary>internal constructor so that only our assembly can provide an implementation</summary>
        internal DataServiceRequest()
        {
        }

        /// <summary>Element Type</summary>
        public abstract Type ElementType
        {
            get;
        }

        /// <summary>Gets the URI for a the query</summary>
        public abstract Uri RequestUri
        {
            get;
        }

        /// <summary>The ProjectionPlan for the request, if precompiled in a previous page; null otherwise.</summary>
        internal abstract ProjectionPlan Plan
        {
            get;
        }

        /// <summary>The TranslateResult associated with this request</summary>
        internal abstract QueryComponents QueryComponents
        {
            get;
        }

        /// <summary>Gets the URI for a the query</summary>
        /// <returns>a string with the URI</returns>
        public override string ToString()
        {
            return this.QueryComponents.Uri.ToString();
        }   

        /// <summary>
        /// get an enumerable materializes the objects the response
        /// </summary>
        /// <param name="context">context</param>
        /// <param name="queryComponents">query components</param>
        /// <param name="plan">Projection plan (if compiled in an earlier query).</param>
        /// <param name="contentType">contentType</param>
        /// <param name="response">method to get http response stream</param>
        /// <returns>atom materializer</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2000:Dispose objects before losing scope", Justification = "Returning MaterializeAtom, caller will dispose")]
        internal static MaterializeAtom Materialize(DataServiceContext context, QueryComponents queryComponents, ProjectionPlan plan, string contentType, Stream response)
        {
            Debug.Assert(null != queryComponents, "querycomponents");

            string mime = null;
            Encoding encoding = null;
            if (!String.IsNullOrEmpty(contentType))
            {
                HttpProcessUtility.ReadContentType(contentType, out mime, out encoding);
            }

            if (String.Equals(mime, XmlConstants.MimeApplicationAtom, StringComparison.OrdinalIgnoreCase) ||
                String.Equals(mime, XmlConstants.MimeApplicationXml, StringComparison.OrdinalIgnoreCase))
            {
                if (null != response)
                {
                    XmlReader reader = XmlUtil.CreateXmlReader(response, encoding);
                    return new MaterializeAtom(context, reader, queryComponents, plan, context.MergeOption);
                }
            }

            return MaterializeAtom.EmptyResults;
        }

        /// <summary>
        /// Creates a instance of strongly typed DataServiceRequest with the given element type.
        /// </summary>
        /// <param name="elementType">element type for the DataServiceRequest.</param>
        /// <param name="requestUri">constructor parameter.</param>
        /// <returns>returns the strongly typed DataServiceRequest instance.</returns>
        internal static DataServiceRequest GetInstance(Type elementType, Uri requestUri)
        {
            Type genericType = typeof(DataServiceRequest<>).MakeGenericType(elementType);
            return (DataServiceRequest)Activator.CreateInstance(genericType, new object[] { requestUri });
        }

        /// <summary>
        /// Ends an asynchronous request to an Internet resource.
        /// </summary>
        /// <typeparam name="TElement">Element type of the result.</typeparam>
        /// <param name="source">Source object of async request.</param>
        /// <param name="context">The data service context.</param>
        /// <param name="asyncResult">The asyncResult being ended.</param>
        /// <returns>The response - result of the request.</returns>
        internal static IEnumerable<TElement> EndExecute<TElement>(object source, DataServiceContext context, IAsyncResult asyncResult)
        {
            QueryResult result = null;
            try
            {
                result = QueryResult.EndExecute<TElement>(source, asyncResult);
                return result.ProcessResult<TElement>(context, result.ServiceRequest.Plan);
            }
            catch (DataServiceQueryException ex)
            {
                Exception inEx = ex;
                while (inEx.InnerException != null)
                {
                    inEx = inEx.InnerException;
                }

                DataServiceClientException serviceEx = inEx as DataServiceClientException;
                if (context.IgnoreResourceNotFoundException && serviceEx != null && serviceEx.StatusCode == (int)HttpStatusCode.NotFound)
                {
                    QueryOperationResponse qor = new QueryOperationResponse<TElement>(new Dictionary<string, string>(ex.Response.Headers), ex.Response.Query, MaterializeAtom.EmptyResults);
                    qor.StatusCode = (int)HttpStatusCode.NotFound;
                    return (IEnumerable<TElement>)qor;
                }

                throw;
            }
        }

#if !ASTORIA_LIGHT // Synchronous methods not available
        /// <summary>
        /// execute uri and materialize result
        /// </summary>
        /// <typeparam name="TElement">element type</typeparam>
        /// <param name="context">context</param>
        /// <param name="queryComponents">query components for request to execute</param>
        /// <returns>enumerable of results</returns>
        internal QueryOperationResponse<TElement> Execute<TElement>(DataServiceContext context, QueryComponents queryComponents)
        {
            QueryResult result = null;
            try
            {
                DataServiceRequest<TElement> serviceRequest = new DataServiceRequest<TElement>(queryComponents, this.Plan);
                result = serviceRequest.CreateResult(this, context, null, null);
                result.Execute();
                return result.ProcessResult<TElement>(context, this.Plan);
            }
            catch (InvalidOperationException ex)
            {
                QueryOperationResponse operationResponse = result.GetResponse<TElement>(MaterializeAtom.EmptyResults);

                if (null != operationResponse)
                {
                    if (context.IgnoreResourceNotFoundException)
                    {
                        DataServiceClientException cex = ex as DataServiceClientException;
                        if (cex != null && cex.StatusCode == (int)HttpStatusCode.NotFound)
                        {
                            // don't throw
                            return (QueryOperationResponse<TElement>)operationResponse;
                        }
                    }

                    operationResponse.Error = ex;
                    throw new DataServiceQueryException(Strings.DataServiceException_GeneralError, ex, operationResponse);
                }

                throw;
            }
        }

        /// <summary>
        /// Synchronizely get the query set count from the server by executing the $count=value query
        /// </summary>
        /// <param name="context">The context</param>
        /// <returns>The server side count of the query set</returns>
        internal long GetQuerySetCount(DataServiceContext context)
        {
            Debug.Assert(null != context, "context is null");
            this.QueryComponents.Version = Util.DataServiceVersion2;

            QueryResult response = null;
            DataServiceRequest<long> serviceRequest = new DataServiceRequest<long>(this.QueryComponents, null);
            HttpWebRequest request = context.CreateRequest(this.QueryComponents.Uri, XmlConstants.HttpMethodGet, false, null, this.QueryComponents.Version, false);
            
            request.Accept = "text/plain";
            response = new QueryResult(this, "Execute", serviceRequest, request, null, null);

            try
            {
                response.Execute();

                if (HttpStatusCode.NoContent != response.StatusCode)
                {
                    StreamReader sr = new StreamReader(response.GetResponseStream());
                    long r = -1;
                    try
                    {
                        r = XmlConvert.ToInt64(sr.ReadToEnd());
                    }
                    finally
                    {
                        sr.Close();
                    }

                    return r;
                }
                else
                {
                    throw new DataServiceQueryException(Strings.DataServiceRequest_FailGetCount, response.Failure);
                }
            }
            catch (InvalidOperationException ex)
            {
                QueryOperationResponse operationResponse = null;
                operationResponse = response.GetResponse<long>(MaterializeAtom.EmptyResults);
                if (null != operationResponse)
                {
                    operationResponse.Error = ex;
                    throw new DataServiceQueryException(Strings.DataServiceException_GeneralError, ex, operationResponse);
                }

                throw;
            }
        }
#endif

        /// <summary>
        /// Begins an asynchronous request to an Internet resource.
        /// </summary>
        /// <param name="source">source of execute (DataServiceQuery or DataServiceContext</param>
        /// <param name="context">context</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for this request.</param>
        /// <returns>An IAsyncResult that references the asynchronous request for a response.</returns>
        internal IAsyncResult BeginExecute(object source, DataServiceContext context, AsyncCallback callback, object state)
        {
            QueryResult result = this.CreateResult(source, context, callback, state);
            result.BeginExecute();
            return result;
        }

        /// <summary>
        /// Creates the result object for the specified query parameters.
        /// </summary>
        /// <param name="source">The source object for the request.</param>
        /// <param name="context">The data service context.</param>
        /// <param name="callback">The AsyncCallback delegate.</param>
        /// <param name="state">The state object for the callback.</param>
        /// <returns>Result representing the create request. The request has not been initiated yet.</returns>
        private QueryResult CreateResult(object source, DataServiceContext context, AsyncCallback callback, object state)
        {
            Debug.Assert(null != context, "context is null");
            HttpWebRequest request = context.CreateRequest(this.QueryComponents.Uri, XmlConstants.HttpMethodGet, false, null, this.QueryComponents.Version, false);
            return new QueryResult(source, "Execute", this, request, callback, state);
        }
    }
}
