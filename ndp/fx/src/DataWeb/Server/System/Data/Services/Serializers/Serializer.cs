//---------------------------------------------------------------------
// <copyright file="Serializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      An abstract base serializer for various serializers
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Text;
    using System.Data.Services.Internal;

    #endregion Namespaces.

    /// <summary>Abstract base class for all serializers.</summary>
    internal abstract class Serializer : IExceptionWriter
    {
        #region Private fields.

        /// <summary>Maximum recursion limit on serializers.</summary>
        private const int RecursionLimit = 100;

        /// <summary>
        /// These query parameters can be copied for each next page link.
        /// Don't need to copy $skiptoken, $skip and $top because they are calculated every time.
        /// </summary>
        private static readonly String[] NextPageQueryParametersToCopy = 
        { 
            XmlConstants.HttpQueryStringFilter,
            XmlConstants.HttpQueryStringExpand,
            XmlConstants.HttpQueryStringOrderBy,
            XmlConstants.HttpQueryStringInlineCount,
            XmlConstants.HttpQueryStringSelect
        };

        /// <summary>Base URI from which resources should be resolved.</summary>
        private readonly Uri absoluteServiceUri;

        /// <summary>Data provider from which metadata should be gathered.</summary>
        private readonly string httpETagHeaderValue;

        /// <summary>Data provider from which metadata should be gathered.</summary>
        private readonly IDataService service;

        /// <summary>Description for the requested results.</summary>
        private readonly RequestDescription requestDescription;

        /// <summary>Collection of complex types, used for cycle detection.</summary>
        private HashSet<object> complexTypeCollection;

        /// <summary>Resolved segment containers.</summary>
        private List<ResourceSetWrapper> segmentContainers;

        /// <summary>Segment names.</summary>
        private List<string> segmentNames;

        /// <summary>Result counts for segments.</summary>
        private List<int> segmentResultCounts;

        /// <summary>Depth of recursion.</summary>
        private int recursionDepth;
        
        /// <summary>Current skip token object for custom paging.</summary>
        private object[] currentSkipTokenForCustomPaging;

        #endregion Private fields.

        /// <summary>Initializes a new base Serializer, ready to write out a description.</summary>
        /// <param name="requestDescription">Description for the requested results.</param>
        /// <param name="absoluteServiceUri">Base URI from which resources should be resolved.</param>
        /// <param name="service">Service with configuration and provider from which metadata should be gathered.</param>
        /// <param name="httpETagHeaderValue">HTTP ETag header value.</param>
        internal Serializer(RequestDescription requestDescription, Uri absoluteServiceUri, IDataService service, string httpETagHeaderValue)
        {
            Debug.Assert(requestDescription != null, "requestDescription != null");
            Debug.Assert(absoluteServiceUri != null, "absoluteServiceUri != null");
            Debug.Assert(service != null, "service != null");

            this.requestDescription = requestDescription;
            this.absoluteServiceUri = absoluteServiceUri;
            this.service = service;
            this.httpETagHeaderValue = httpETagHeaderValue;
        }

        /// <summary>Container for the resource being serialized (possibly null).</summary>
        protected ResourceSetWrapper CurrentContainer
        {
            get
            {
                if (this.segmentContainers == null || this.segmentContainers.Count == 0)
                {
                    return this.requestDescription.LastSegmentInfo.TargetContainer;
                }
                else
                {
                    return this.segmentContainers[this.segmentContainers.Count - 1];
                }
            }
        }

        /// <summary>Is current container the root container.</summary>
        protected bool IsRootContainer
        {
            get
            {
                return (this.segmentContainers == null || this.segmentContainers.Count == 1);
            }
        }

        /// <summary>
        /// Gets the Data provider from which metadata should be gathered.
        /// </summary>
        protected DataServiceProviderWrapper Provider
        {
            [DebuggerStepThrough]
            get { return this.service.Provider; }
        }

        /// <summary>
        /// Gets the Data service from which metadata should be gathered.
        /// </summary>
        protected IDataService Service
        {
            [DebuggerStepThrough]
            get { return this.service; }
        }

        /// <summary>Gets the absolute URI to the service.</summary>
        protected Uri AbsoluteServiceUri
        {
            [DebuggerStepThrough]
            get { return this.absoluteServiceUri; }
        }

        /// <summary>
        /// Gets the RequestDescription for the request that is getting serialized.
        /// </summary>
        protected RequestDescription RequestDescription
        {
            [DebuggerStepThrough]
            get
            {
                return this.requestDescription;
            }
        }

        /// <summary>Are we using custom paging?</summary>
        protected bool IsCustomPaged
        {
            get
            {
                return this.service.PagingProvider.IsCustomPagedForSerialization;
            }
        }

        /// <summary>Serializes exception information.</summary>
        /// <param name="args">Description of exception to serialize.</param>
        public abstract void WriteException(HandleExceptionArgs args);

        /// <summary>
        /// Gets the uri given the list of resource properties. This logic must be the same across all
        /// serializers. Hence putting this in a util class
        /// </summary>
        /// <param name="resource">instance of the resource type whose properties needs to be returned</param>
        /// <param name="provider">Provider from which resource was obtained.</param>
        /// <param name="container">Container for the resource.</param>
        /// <param name="absoluteServiceUri">Base URI from which resources should be resolved.</param>
        /// <returns>uri for the given resource</returns>
        internal static Uri GetUri(object resource, DataServiceProviderWrapper provider, ResourceSetWrapper container, Uri absoluteServiceUri)
        {
            Debug.Assert(container != null, "container != null");
            string objectKey = GetObjectKey(resource, provider, container.Name);
            return RequestUriProcessor.AppendEscapedSegment(absoluteServiceUri, objectKey);
        }

        /// <summary>
        /// Appends the given entry to the given uri
        /// </summary>
        /// <param name="currentUri">uri to which the entry needs to be appended</param>
        /// <param name="entry">entry which gets appended to the given uri</param>
        /// <returns>new uri with the entry appended to the given uri</returns>
        internal static Uri AppendEntryToUri(Uri currentUri, string entry)
        {
            return RequestUriProcessor.AppendUnescapedSegment(currentUri, entry);
        }

        /// <summary>
        /// Handles the complete serialization for the specified <see cref="RequestDescription"/>.
        /// </summary>
        /// <param name="queryResults">Query results to enumerate.</param>
        /// <param name="hasMoved">Whether <paramref name="queryResults"/> was succesfully advanced to the first element.</param>
        /// <remarks>
        /// <paramref name="queryResults"/> should correspond to the RequestQuery of the 
        /// RequestDescription object passed while constructing this serializer
        /// We allow the results to be passed in
        /// to let the query be executed earlier than at result-writing time, which
        /// helps detect data and query errors where they can be better handled.
        /// </remarks>
        internal void WriteRequest(IEnumerator queryResults, bool hasMoved)
        {
            Debug.Assert(this.requestDescription.RequestEnumerable != null, "this.requestDescription.RequestEnumerable != null");
            Debug.Assert(queryResults != null, "queryResults != null");
            IExpandedResult expanded = queryResults as IExpandedResult;

            if (this.requestDescription.LinkUri)
            {
                bool needPop = this.PushSegmentForRoot();
                if (this.requestDescription.IsSingleResult)
                {
                    this.WriteLink(queryResults.Current);
                    if (queryResults.MoveNext())
                    {
                        throw new InvalidOperationException(Strings.SingleResourceExpected);
                    }
                }
                else
                {
                    this.WriteLinkCollection(queryResults, hasMoved);
                }

                this.PopSegmentName(needPop);
            }
            else if (this.requestDescription.IsSingleResult)
            {
                Debug.Assert(hasMoved == true, "hasMoved == true");
                this.WriteTopLevelElement(expanded, queryResults.Current);
                if (queryResults.MoveNext())
                {
                    throw new InvalidOperationException(Strings.SingleResourceExpected);
                }
            }
            else
            {
                this.WriteTopLevelElements(expanded, queryResults, hasMoved);
            }

            this.Flush();
        }

        /// <summary>Gets the expandable value for the specified object.</summary>
        /// <param name="provider">underlying data source instance.</param>
        /// <param name="expanded">Expanded properties for the result, possibly null.</param>
        /// <param name="customObject">Object with value to retrieve.</param>
        /// <param name="property">Property for which value will be retrieved.</param>
        /// <returns>The property value.</returns>
        protected static object GetExpandedProperty(DataServiceProviderWrapper provider, IExpandedResult expanded, object customObject, ResourceProperty property)
        {
            Debug.Assert(property != null, "property != null");
            if (expanded == null)
            {
                return WebUtil.GetPropertyValue(provider, customObject, null, property, null);
            }
            else
            {
                // We may end up projecting null as a value of ResourceSetReference property. This can in theory break
                //   the serializers as they expect a non-null (possibly empty) IEnumerable instead. But note that
                //   if we project null into the expanded property, we also project null into the ExpandedElement property
                //   and thus the serializers should recognize this value as null and don't try to expand its properties.
                Debug.Assert(
                    expanded.ExpandedElement != null, 
                    "We should not be accessing expanded properties on null resource.");
                return expanded.GetExpandedPropertyValue(property.Name);
            }
        }

        /// <summary>Gets the expanded element for the specified expanded result.</summary>
        /// <param name="expanded">The expanded result to process.</param>
        /// <returns>The expanded element.</returns>
        protected static object GetExpandedElement(IExpandedResult expanded)
        {
            Debug.Assert(expanded != null, "expanded != null");
            return expanded.ExpandedElement;
        }

        /// <summary>Flushes the writer to the underlying stream.</summary>
        protected abstract void Flush();

        /// <summary>Writes a single top-level element.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="element">Element to write, possibly null.</param>
        protected abstract void WriteTopLevelElement(IExpandedResult expanded, object element);

        /// <summary>Writes multiple top-level elements, possibly none.</summary>
        /// <param name="expanded">Expanded properties for the result.</param>
        /// <param name="elements">Result elements.</param>
        /// <param name="hasMoved">Whether <paramref name="elements"/> was succesfully advanced to the first element.</param>
        protected abstract void WriteTopLevelElements(IExpandedResult expanded, IEnumerator elements, bool hasMoved);

        /// <summary>
        /// Write out the entry count
        /// </summary>
        protected abstract void WriteRowCount();

        /// <summary>
        /// Write out the uri for the given element
        /// </summary>
        /// <param name="element">element whose uri needs to be written out.</param>
        protected abstract void WriteLink(object element);

        /// <summary>
        /// Write out the uri for the given elements
        /// </summary>
        /// <param name="elements">elements whose uri need to be writtne out</param>
        /// <param name="hasMoved">the current state of the enumerator.</param>
        protected abstract void WriteLinkCollection(IEnumerator elements, bool hasMoved);

        /// <summary>
        /// Adds the given object instance to complex type collection
        /// </summary>
        /// <param name="complexTypeInstance">instance to be added</param>
        /// <returns>true, if it got added successfully</returns>
        protected bool AddToComplexTypeCollection(object complexTypeInstance)
        {
            if (this.complexTypeCollection == null)
            {
                this.complexTypeCollection = new HashSet<object>(ReferenceEqualityComparer<object>.Instance);
            }

            return this.complexTypeCollection.Add(complexTypeInstance);
        }

        /// <summary>
        /// Gets the skip token object contained in the expanded result for standard paging.
        /// </summary>
        /// <param name="expanded">Current expanded result.</param>
        /// <returns>Skip token object if any.</returns>
        protected IExpandedResult GetSkipToken(IExpandedResult expanded)
        {
            if (expanded != null && !this.IsCustomPaged && !this.RequestDescription.IsRequestForEnumServiceOperation)
            {
                return expanded.GetExpandedPropertyValue(XmlConstants.HttpQueryStringSkipToken) as IExpandedResult;
            }
            
            return null;
        }

        /// <summary>
        /// Obtains the URI for the link for next page in string format
        /// </summary>
        /// <param name="lastObject">Last object serialized to be used for generating $skiptoken</param>
        /// <param name="skipTokenExpandedResult">The <see cref="IExpandedResult"/> of the $skiptoken property of object corresponding to last serialized object</param>
        /// <param name="absoluteUri">Absolute response URI</param>
        /// <returns>URI for the link for next page</returns>
        protected String GetNextLinkUri(object lastObject, IExpandedResult skipTokenExpandedResult, Uri absoluteUri)
        {
            UriBuilder builder = new UriBuilder(absoluteUri);
            SkipTokenBuilder skipTokenBuilder = null;

            if (this.IsRootContainer)
            {
                if (!this.IsCustomPaged)
                {
                    if (skipTokenExpandedResult != null)
                    {
                        skipTokenBuilder = new SkipTokenBuilderFromExpandedResult(skipTokenExpandedResult, this.RequestDescription.SkipTokenExpressionCount);
                    }       
                    else
                    {
                        Debug.Assert(this.RequestDescription.SkipTokenProperties != null, "Must have skip token properties collection");
                        Debug.Assert(this.RequestDescription.SkipTokenProperties.Count > 0, "Must have some valid ordered properties in the skip token properties collection");
                        skipTokenBuilder = new SkipTokenBuilderFromProperties(lastObject, this.Provider, this.RequestDescription.SkipTokenProperties);
                    }
                }
                else
                {
                    Debug.Assert(this.currentSkipTokenForCustomPaging != null, "Must have obtained the skip token for custom paging.");
                    skipTokenBuilder = new SkipTokenBuilderFromCustomPaging(this.currentSkipTokenForCustomPaging);
                }

                builder.Query = this.GetNextPageQueryParametersForRootContainer().Append(skipTokenBuilder.GetSkipToken()).ToString();
            }
            else
            {
                if (!this.IsCustomPaged)
                {
                    // Internal results
                    skipTokenBuilder = new SkipTokenBuilderFromProperties(lastObject, this.Provider, this.CurrentContainer.ResourceType.KeyProperties);
                }
                else
                {
                    Debug.Assert(this.currentSkipTokenForCustomPaging != null, "Must have obtained the skip token for custom paging.");
                    skipTokenBuilder = new SkipTokenBuilderFromCustomPaging(this.currentSkipTokenForCustomPaging);
                }

                builder.Query = this.GetNextPageQueryParametersForExpandedContainer().Append(skipTokenBuilder.GetSkipToken()).ToString();
            }
            
            return builder.Uri.AbsoluteUri;
        }
        
        /// <summary>Is next page link needs to be appended to the feed</summary>
        /// <param name="enumerator">Current result enumerator.</param>
        /// <returns>true if the feed must have a next page link</returns>
        protected bool NeedNextPageLink(IEnumerator enumerator)
        {
            // For open types, current container could be null
            if (this.CurrentContainer != null && !this.RequestDescription.IsRequestForEnumServiceOperation)
            {
                if (this.IsCustomPaged)
                {
                    this.currentSkipTokenForCustomPaging = 
                        this.service.PagingProvider.PagingProviderInterface.GetContinuationToken(BasicExpandProvider.ExpandedEnumerator.UnwrapEnumerator(enumerator));
                    Debug.Assert(
                            this.RequestDescription.ResponseVersion != RequestDescription.DataServiceDefaultResponseVersion,
                            "If custom paging is enabled, our response should be 2.0 and beyond.");
                            
                    return this.currentSkipTokenForCustomPaging != null && this.currentSkipTokenForCustomPaging.Length > 0;
                }
                else
                {
                    int pageSize = this.CurrentContainer.PageSize;

                    if (pageSize != 0 && this.RequestDescription.ResponseVersion != RequestDescription.DataServiceDefaultResponseVersion)
                    {
                        // For the root segment, if the $top parameter value is less than or equal to page size then we
                        // don't need to send the next page link.
                        if (this.segmentResultCounts.Count == 1)
                        {
                            int? topQueryParameter = this.GetTopQueryParameter();
                            
                            if (topQueryParameter.HasValue)
                            {
                                Debug.Assert(topQueryParameter.Value >= this.segmentResultCounts[this.segmentResultCounts.Count - 1], "$top must be the upper limits of the number of results returned.");
                                if (topQueryParameter.Value <= pageSize)
                                {
                                    return false;
                                }
                            }
                        }

                        return this.segmentResultCounts[this.segmentResultCounts.Count - 1] == pageSize;
                    }
                }
            }

            return false;
        }

        /// <summary>Pushes a segment for the root of the tree being written out.</summary>
        /// <param name="propertyName">Name of open property.</param>
        /// <param name="propertyResourceType">Resulved type of open property.</param>
        /// <remarks>Calls to this method should be balanced with calls to PopSegmentName.</remarks>
        /// <returns>true if segment was pushed, false otherwise</returns>
        protected bool PushSegmentForOpenProperty(string propertyName, ResourceType propertyResourceType)
        {
            Debug.Assert(propertyName != null, "propertyName != null");
            Debug.Assert(propertyResourceType != null, "propertyResourceType != null");
            ResourceSetWrapper container = null;
            if (propertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
            }

            return this.PushSegment(propertyName, container);
        }

        /// <summary>Increments the result count for the current segment, throws if exceeds the limit.</summary>
        protected void IncrementSegmentResultCount()
        {
            if (this.segmentResultCounts != null)
            {
                Debug.Assert(this.segmentResultCounts.Count > 0, "this.segmentResultCounts.Count > 0 -- otherwise we didn't PushSegmentForRoot");
                int max = this.service.Configuration.MaxResultsPerCollection;
                
                if (!this.IsCustomPaged)
                {
                    // For Open types, current container could be null, even though MaxResultsPerCollection has been set
                    // set we need to check for container before we try to assume page size, also open types do not have
                    // page sizes so we can safely ignore this check here
                    if (this.CurrentContainer != null && this.CurrentContainer.PageSize != 0)
                    {
                        Debug.Assert(max == Int32.MaxValue, "Either page size or max result count can be set, but not both");
                        max = this.CurrentContainer.PageSize;
                    }
                }
                
                if (max != Int32.MaxValue)
                {
                    int count = this.segmentResultCounts[this.segmentResultCounts.Count - 1];
                    checked
                    { 
                        count++;
                    }
                    
                    if (count > max)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.Serializer_ResultsExceedMax(max));
                    }

                    this.segmentResultCounts[this.segmentResultCounts.Count - 1] = count;
                }
            }
        }

        /// <summary>Pushes a segment from the stack of names being written.</summary>
        /// <param name='property'>Property to push.</param>
        /// <remarks>Calls to this method should be balanced with calls to PopSegmentName.</remarks>
        /// <returns>true if a segment was pushed, false otherwise</returns>
        protected bool PushSegmentForProperty(ResourceProperty property)
        {
            Debug.Assert(property != null, "property != null");
            ResourceSetWrapper current = null;
            if ((property.Kind & (ResourcePropertyKind.ResourceReference | ResourcePropertyKind.ResourceSetReference)) != 0)
            {
                current = this.CurrentContainer;
                if (current != null)
                {
                    current = this.service.Provider.GetContainer(current, current.ResourceType, property);
                }
            }

            return this.PushSegment(property.Name, current);
        }

        /// <summary>Pushes a segment for the root of the tree being written out.</summary>
        /// <remarks>Calls to this method should be balanced with calls to PopSegmentName.</remarks>
        /// <returns>true if the segment was pushed, false otherwise</returns>
        protected bool PushSegmentForRoot()
        {
            return this.PushSegment(this.RequestDescription.ContainerName, this.CurrentContainer);
        }

        /// <summary>Pops a segment name from the stack of names being written.</summary>
        /// <param name="needPop">Is a pop required. Only true if last push was successful</param>
        /// <remarks>Calls to this method should be balanced with previous calls to PushSegmentName.</remarks>
        protected void PopSegmentName(bool needPop)
        {
            if (this.segmentNames != null && needPop)
            {
                Debug.Assert(this.segmentNames.Count > 0, "this.segmentNames.Count > 0");
                this.segmentNames.RemoveAt(this.segmentNames.Count - 1);
                this.segmentContainers.RemoveAt(this.segmentContainers.Count - 1);
                this.segmentResultCounts.RemoveAt(this.segmentResultCounts.Count - 1);
                Debug.Assert(
                    this.segmentContainers.Count == this.segmentNames.Count,
                    "this.segmentContainers.Count == this.segmentNames.Count -- should always be one-to-one");
                Debug.Assert(
                    this.segmentContainers.Count == this.segmentResultCounts.Count,
                    "this.segmentContainers.Count == this.segmentResultCounts.Count -- should always be one-to-one");
            }
        }

        /// <summary>Marks the fact that a recursive method was entered, and checks that the depth is allowed.</summary>
        protected void RecurseEnter()
        {
            WebUtil.RecurseEnter(RecursionLimit, ref this.recursionDepth);
        }

        /// <summary>Marks the fact that a recursive method is leaving.</summary>
        protected void RecurseLeave()
        {
            WebUtil.RecurseLeave(ref this.recursionDepth);
        }

        /// <summary>Returns a clone of the segment names and containers trackd at this moment.</summary>
        /// <returns>A clone of the segment names and containers tracked at this moment; possibly null.</returns>
        protected object SaveSegmentNames()
        {
            if (this.segmentNames == null)
            {
                return null;
            }
            else
            {
                return new object[3]
                {
                    new List<string>(this.segmentNames),
                    new List<ResourceSetWrapper>(this.segmentContainers),
                    new List<int>(this.segmentResultCounts)
                };
            }
        }

        /// <summary>Restores the segment names saved through <see cref="SaveSegmentNames" />.</summary>
        /// <param name='savedSegmentNames'>Value returned from a previous call to <see cref="SaveSegmentNames" />.</param>
        protected void RestoreSegmentNames(object savedSegmentNames)
        {
            object[] savedLists = (object[])savedSegmentNames;
            if (savedLists == null)
            {
                this.segmentNames = null;
                this.segmentContainers = null;
                this.segmentResultCounts = null;
            }
            else
            {
                this.segmentNames = (List<string>)savedLists[0];
                this.segmentContainers = (List<ResourceSetWrapper>)savedLists[1];
                this.segmentResultCounts = (List<int>)savedLists[2];
            }
        }

        /// <summary>
        /// Remove the given object instance from the complex type collection
        /// </summary>
        /// <param name="complexTypeInstance">instance to be removed</param>
        protected void RemoveFromComplexTypeCollection(object complexTypeInstance)
        {
            Debug.Assert(this.complexTypeCollection != null, "this.complexTypeCollection != null");
            Debug.Assert(this.complexTypeCollection.Contains(complexTypeInstance), "this.complexTypeCollection.Contains(complexTypeInstance)");

            this.complexTypeCollection.Remove(complexTypeInstance);
        }

        /// <summary>Checks whether the property with the specified name should be expanded in-line.</summary>
        /// <param name='name'>Name of property to consider for expansion.</param>
        /// <returns>true if the segment should be expanded; false otherwise.</returns>
        protected bool ShouldExpandSegment(string name)
        {
            Debug.Assert(name != null, "name != null");

            if (this.segmentNames == null)
            {
                return false;
            }

            if (this.requestDescription.RootProjectionNode != null)
            {
                if (this.requestDescription.RootProjectionNode.UseExpandPathsForSerialization &&
                    this.requestDescription.RootProjectionNode.ExpandPaths != null)
                {
                    // We need to use the old ExpandPaths to determine which segments to expand
                    //   since the IExpandProvider might have modified this collection.
                    for (int i = 0; i < this.requestDescription.RootProjectionNode.ExpandPaths.Count; i++)
                    {
                        List<ExpandSegment> expandPath = this.requestDescription.RootProjectionNode.ExpandPaths[i];
                        if (expandPath.Count < this.segmentNames.Count)
                        {
                            continue;
                        }

                        // We start off at '1' for segment names because the first one is the
                        // "this" in the query (/Customers?$expand=Orders doesn't include "Customers").
                        bool matchFound = true;
                        for (int j = 1; j < this.segmentNames.Count; j++)
                        {
                            if (expandPath[j - 1].Name != this.segmentNames[j])
                            {
                                matchFound = false;
                                break;
                            }
                        }

                        if (matchFound && expandPath[this.segmentNames.Count - 1].Name == name)
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    // We can use the new tree of expanded nodes to determine the expansions.
                    // So find the expanded node on which we are now.
                    ExpandedProjectionNode expandedNode = this.GetCurrentExpandedProjectionNode();
                    if (expandedNode != null)
                    {
                        // And then if that node contains a child node of the specified name
                        //   and that child is also an expanded node, we should expand it.
                        ProjectionNode lastNode = expandedNode.FindNode(name);
                        if (lastNode != null && lastNode is ExpandedProjectionNode)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        /// <summary>Returns a list of projection segments defined for the current segment.</summary>
        /// <returns>List of <see cref="ProjectionNode"/> describing projections for the current segment.
        /// If this method returns null it means no projections are to be applied and the entire resource
        /// for the current segment should be serialized. If it returns non-null only the properties described
        /// by the returned projection segments should be serialized.</returns>
        protected IEnumerable<ProjectionNode> GetProjections()
        {
            ExpandedProjectionNode expandedProjectionNode = this.GetCurrentExpandedProjectionNode();
            if (expandedProjectionNode == null || expandedProjectionNode.ProjectAllProperties)
            {
                return null;
            }
            else
            {
                return expandedProjectionNode.Nodes;
            }
        }

        /// <summary>
        /// Returns the ETag value from the host response header
        /// </summary>
        /// <param name="resource">resource whose etag value gets to be returned</param>
        /// <returns>returns the etag value for the given resource</returns>
        protected string GetETagValue(object resource)
        {
            // this.httpETagHeaderValue is the etag value which got computed for writing the etag in the response
            // headers. The etag response header only gets written out in certain scenarios, whereas we always 
            // write etag in the response payload, if the type has etag properties. So just checking here is the
            // etag has already been computed, and if yes, returning that, otherwise computing the etag.
            if (!String.IsNullOrEmpty(this.httpETagHeaderValue))
            {
                return this.httpETagHeaderValue;
            }
            else
            {
                Debug.Assert(this.CurrentContainer != null, "this.CurrentContainer != null");
                return WebUtil.GetETagValue(this.service, resource, this.CurrentContainer);
            }
        }

        /// <summary>Returns the key for the given resource.</summary>
        /// <param name="resource">Resource for which key value needs to be returned.</param>
        /// <param name="provider">Specific provider from which resource was obtained.</param>
        /// <param name="containerName">name of the entity container that the resource belongs to</param>
        /// <returns>Key for the given resource, with values encoded for use in a URI.</returns>
        private static string GetObjectKey(object resource, DataServiceProviderWrapper provider, string containerName)
        {
            Debug.Assert(resource != null, "resource != null");
            Debug.Assert(provider != null, "provider != null");
            Debug.Assert(!String.IsNullOrEmpty(containerName), "container name must be specified");

            StringBuilder resultBuilder = new StringBuilder();
            resultBuilder.Append(containerName);
            resultBuilder.Append('(');
            ResourceType resourceType = WebUtil.GetNonPrimitiveResourceType(provider, resource);
            Debug.Assert(resourceType != null, "resourceType != null");
            IList<ResourceProperty> keyProperties = resourceType.KeyProperties;
            Debug.Assert(keyProperties.Count != 0, "every resource type must have a key");
            for (int i = 0; i < keyProperties.Count; i++)
            {
                ResourceProperty property = keyProperties[i];
                Debug.Assert(property.IsOfKind(ResourcePropertyKind.Key), "must be key property");

                object keyValue = WebUtil.GetPropertyValue(provider, resource, resourceType, property, null);
                if (keyValue == null)
                {
                    // null keys not supported.
                    throw new InvalidOperationException(Strings.Serializer_NullKeysAreNotSupported(property.Name));
                }

                if (i == 0)
                {
                    if (keyProperties.Count != 1)
                    {
                        resultBuilder.Append(property.Name);
                        resultBuilder.Append('=');
                    }
                }
                else
                {
                    resultBuilder.Append(',');
                    resultBuilder.Append(property.Name);
                    resultBuilder.Append('=');
                }

                string keyValueText;
                if (!System.Data.Services.Parsing.WebConvert.TryKeyPrimitiveToString(keyValue, out keyValueText))
                {
                    throw new InvalidOperationException(Strings.Serializer_CannotConvertValue(keyValue));
                }

                Debug.Assert(keyValueText != null, "keyValueText != null - otherwise TryKeyPrimitiveToString returned true and null value");
                resultBuilder.Append(keyValueText);
            }

            resultBuilder.Append(')');
            return resultBuilder.ToString();
        }

        /// <summary>Helper method to append a path to the $expand or $select path list.</summary>
        /// <param name="pathsBuilder">The <see cref="StringBuilder"/> to which to append the path.</param>
        /// <param name="parentPathSegments">The segments of the path up to the last segment.</param>
        /// <param name="lastPathSegment">The last segment of the path.</param>
        private static void AppendProjectionOrExpansionPath(StringBuilder pathsBuilder, List<string> parentPathSegments, string lastPathSegment)
        {
            if (pathsBuilder.Length != 0)
            {
                pathsBuilder.Append(',');
            }

            foreach (string parentPathSegment in parentPathSegments)
            {
                pathsBuilder.Append(parentPathSegment).Append('/');
            }

            pathsBuilder.Append(lastPathSegment);
        }

        /// <summary>Finds the <see cref="ExpandedProjectionNode"/> node which describes the current segment.</summary>
        /// <returns>The <see cref="ExpandedProjectionNode"/> which describes the current segment, or null
        /// if no such node is available.</returns>
        private ExpandedProjectionNode GetCurrentExpandedProjectionNode()
        {
            ExpandedProjectionNode expandedProjectionNode = this.RequestDescription.RootProjectionNode;
            if (expandedProjectionNode == null)
            {
                return null;
            }

            if (this.segmentNames != null)
            {
                // We start off at '1' for segment names because the first one is the
                // "this" in the query and projection segments don't have that (the root is the "this")
                for (int i = 1; i < this.segmentNames.Count; i++)
                {
                    ProjectionNode projectionNode = expandedProjectionNode.FindNode(this.segmentNames[i]);
                    if (projectionNode == null)
                    {
                        // If we don't have a projection node, report everything (complex types for example).
                        return null;
                    }

                    Debug.Assert(
                        projectionNode is ExpandedProjectionNode,
                        "We have a pushed segment on the serialization stack which is not backed by expanded node in the query definition.");
                    expandedProjectionNode = (ExpandedProjectionNode)projectionNode;
                }
            }

            return expandedProjectionNode;
        }

        /// <summary>
        /// Builds the string corresponding to query parameters for top level results to be put in next page link.
        /// </summary>
        /// <returns>StringBuilder which has the query parameters in the URI query parameter format.</returns>
        private StringBuilder GetNextPageQueryParametersForRootContainer()
        {
            StringBuilder queryParametersBuilder = new StringBuilder();

            // Handle service operation parameters
            if (this.RequestDescription.SegmentInfos[0].TargetSource == RequestTargetSource.ServiceOperation)
            {
                foreach (var parameter in this.RequestDescription.SegmentInfos[0].Operation.Parameters)
                {
                    if (queryParametersBuilder.Length > 0)
                    {
                        queryParametersBuilder.Append('&');
                    }

                    queryParametersBuilder.Append(parameter.Name).Append('=');
                    string escapedQueryStringItem = DataStringEscapeBuilder.EscapeDataString(this.service.OperationContext.Host.GetQueryStringItem(parameter.Name));
                    queryParametersBuilder.Append(escapedQueryStringItem);
                }
            }

            foreach (String queryParameter in Serializer.NextPageQueryParametersToCopy)
            {
                String value = this.service.OperationContext.Host.GetQueryStringItem(queryParameter);
                if (!String.IsNullOrEmpty(value))
                {
                    if (queryParametersBuilder.Length > 0)
                    {
                        queryParametersBuilder.Append('&');
                    }

                    queryParametersBuilder.Append(queryParameter).Append('=').Append(DataStringEscapeBuilder.EscapeDataString(value));
                }
            }

            int? topQueryParameter = this.GetTopQueryParameter();
            if (topQueryParameter.HasValue)
            {
                int remainingResults = topQueryParameter.Value;
                
                // We don't touch the top count in case of custom paging.
                if (!this.IsCustomPaged)
                {
                    remainingResults = topQueryParameter.Value - this.CurrentContainer.PageSize;
                }

                if (remainingResults > 0)
                {
                    if (queryParametersBuilder.Length > 0)
                    {
                        queryParametersBuilder.Append('&');
                    }

                    queryParametersBuilder.Append(XmlConstants.HttpQueryStringTop).Append('=').Append(remainingResults);
                }
            }

            if (queryParametersBuilder.Length > 0)
            {
                queryParametersBuilder.Append('&');
            }

            return queryParametersBuilder;
        }

        /// <summary>Recursive method which builds the $expand and $select paths for the specified node.</summary>
        /// <param name="parentPathSegments">List of path segments which lead up to this node. 
        /// So for example if the specified node is Orders/OrderDetails the list will contains two strings
        /// "Orders" and "OrderDetails".</param>
        /// <param name="projectionPaths">The result to which the projection paths are appended as a comma separated list.</param>
        /// <param name="expansionPaths">The result to which the expansion paths are appended as a comma separated list.</param>
        /// <param name="expandedNode">The node to inspect.</param>
        /// <param name="foundProjections">Out parameter which is set to true if there were some explicit projections on the inspected node.</param>
        /// <param name="foundExpansions">Our parameter which is set to true if there were some expansions on the inspected node.</param>
        private void BuildProjectionAndExpansionPathsForNode(
            List<string> parentPathSegments, 
            StringBuilder projectionPaths, 
            StringBuilder expansionPaths, 
            ExpandedProjectionNode expandedNode,
            out bool foundProjections,
            out bool foundExpansions)
        {
            foundProjections = false;
            foundExpansions = false;

            bool foundExpansionChild = false;
            bool foundProjectionChild = false;
            List<ExpandedProjectionNode> expandedChildrenNeededToBeProjected = new List<ExpandedProjectionNode>();
            foreach (ProjectionNode childNode in expandedNode.Nodes)
            {
                ExpandedProjectionNode expandedChildNode = childNode as ExpandedProjectionNode;
                if (expandedChildNode == null)
                {
                    // Explicitely project the property mentioned in this node
                    AppendProjectionOrExpansionPath(projectionPaths, parentPathSegments, childNode.PropertyName);
                    foundProjections = true;
                }
                else
                {
                    foundExpansions = true;

                    parentPathSegments.Add(expandedChildNode.PropertyName);
                    this.BuildProjectionAndExpansionPathsForNode(
                        parentPathSegments,
                        projectionPaths,
                        expansionPaths,
                        expandedChildNode,
                        out foundProjectionChild,
                        out foundExpansionChild);
                    parentPathSegments.RemoveAt(parentPathSegments.Count - 1);

                    // Add projection paths for this node if all its properties should be projected
                    if (expandedChildNode.ProjectAllProperties)
                    {
                        if (foundProjectionChild)
                        {
                            // There were some projections in our children, but this node requires all properties -> project *
                            AppendProjectionOrExpansionPath(projectionPaths, parentPathSegments, childNode.PropertyName + "/*");
                        }
                        else
                        {
                            // There were no projections underneath this node, so we need to "project" this node
                            // we just don't know yet if we need to project this one explicitly or if some parent will do it for us implicitly.
                            expandedChildrenNeededToBeProjected.Add(expandedChildNode);
                        }
                    }

                    foundProjections |= foundProjectionChild;

                    if (!foundExpansionChild)
                    {
                        // If there were no expansions in children, we need to add this node to expansion list
                        AppendProjectionOrExpansionPath(expansionPaths, parentPathSegments, childNode.PropertyName);
                    }
                }
            }

            if (!expandedNode.ProjectAllProperties || foundProjections)
            {
                // If we already projected some properties explicitely or this node does not want to project all properties 
                // and we have some expanded children which were not projected yet
                // we need to project those explicitely (as the other projections disable the "include all" for this node 
                // or we don't really want the "include all" anyway)
                foreach (ExpandedProjectionNode childToProject in expandedChildrenNeededToBeProjected)
                {
                    AppendProjectionOrExpansionPath(projectionPaths, parentPathSegments, childToProject.PropertyName);

                    // And since we're adding an explicit projection, mark us as using explicit projections
                    foundProjections = true;
                }
            }
        }

        /// <summary>
        /// Builds the string corresponding to query parameters for top level results to be put in next page link.
        /// </summary>
        /// <returns>StringBuilder which has the query parameters in the URI query parameter format.</returns>
        private StringBuilder GetNextPageQueryParametersForExpandedContainer()
        {
            StringBuilder queryParametersBuilder = new StringBuilder();

            ExpandedProjectionNode expandedProjectionNode = this.GetCurrentExpandedProjectionNode();
            if (expandedProjectionNode != null)
            {
                // Build a string containing all the $expand and $select for the current node and children
                List<string> pathSegments = new List<string>();
                StringBuilder projectionPaths = new StringBuilder();
                StringBuilder expansionPaths = new StringBuilder();
                bool foundExpansions = false;
                bool foundProjections = false;
                this.BuildProjectionAndExpansionPathsForNode(
                    pathSegments,
                    projectionPaths,
                    expansionPaths,
                    expandedProjectionNode,
                    out foundProjections,
                    out foundExpansions);

                // In most cases the root level of the query is projected by default
                // The only exception is if there were projections in some children
                // and we need all the properties of the root -> then project *
                if (foundProjections && expandedProjectionNode.ProjectAllProperties)
                {
                    AppendProjectionOrExpansionPath(projectionPaths, pathSegments, "*");
                }

                if (projectionPaths.Length > 0)
                {
                    if (queryParametersBuilder.Length > 0)
                    {
                        queryParametersBuilder.Append('&');
                    }

                    queryParametersBuilder.Append("$select=").Append(projectionPaths.ToString());
                }

                if (expansionPaths.Length > 0)
                {
                    if (queryParametersBuilder.Length > 0)
                    {
                        queryParametersBuilder.Append('&');
                    }

                    queryParametersBuilder.Append("$expand=").Append(expansionPaths.ToString());
                }
            }

            if (queryParametersBuilder.Length > 0)
            {
                queryParametersBuilder.Append('&');
            }

            return queryParametersBuilder;
        }

        /// <summary>Pushes a segment from the stack of names being written.</summary>
        /// <param name="name">Name of property to push.</param>
        /// <param name="container">Container to push (possibly null).</param>
        /// <remarks>Calls to this method should be balanced with calls to PopSegmentName.</remarks>
        /// <returns>true if a segment was push, false otherwise</returns>
        private bool PushSegment(string name, ResourceSetWrapper container)
        {
            if (this.service.Configuration.MaxResultsPerCollection != Int32.MaxValue ||
                (container != null && container.PageSize != 0) || // Complex types have null container , paging should force a push
                (this.requestDescription.RootProjectionNode != null &&
                 this.requestDescription.RootProjectionNode.ExpansionsSpecified))
            {
                if (this.segmentNames == null)
                {
                    this.segmentNames = new List<string>();
                    this.segmentContainers = new List<ResourceSetWrapper>();
                    this.segmentResultCounts = new List<int>();
                }

                Debug.Assert(
                    this.segmentContainers.Count == this.segmentNames.Count,
                    "this.segmentContainers.Count == this.segmentNames.Count -- should always be one-to-one");
                Debug.Assert(
                    this.segmentContainers.Count == this.segmentResultCounts.Count,
                    "this.segmentContainers.Count == this.segmentResultCounts.Count -- should always be one-to-one");
                this.segmentNames.Add(name);
                this.segmentContainers.Add(container);
                this.segmentResultCounts.Add(0);
                Debug.Assert(
                    this.segmentContainers.Count == this.segmentNames.Count,
                    "this.segmentContainers.Count == this.segmentNames.Count -- should always be one-to-one");
                Debug.Assert(
                    this.segmentContainers.Count == this.segmentResultCounts.Count,
                    "this.segmentContainers.Count == this.segmentResultCounts.Count -- should always be one-to-one");
                return true;
            }
            
            return false;
        }

        /// <summary>
        /// Obtains the $top query parameter value.
        /// </summary>
        /// <returns>Integer value for $top or null otherwise.</returns>
        private int? GetTopQueryParameter()
        {
            String topQueryItem = this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringTop);
            if (!String.IsNullOrEmpty(topQueryItem))
            {
                return Int32.Parse(topQueryItem, System.Globalization.CultureInfo.InvariantCulture);
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Builds the $skiptoken=[value,value] representation for appending to the next page link URI.
        /// </summary>
        private abstract class SkipTokenBuilder
        {
            /// <summary>Skip token string representation.</summary>
            private StringBuilder skipToken;

            /// <summary>Constructor.</summary>
            public SkipTokenBuilder()
            {
                this.skipToken = new StringBuilder();
                this.skipToken.Append(XmlConstants.HttpQueryStringSkipToken).Append('=');
            }

            /// <summary>Returns the string representation for $skiptoken query parameter.</summary>
            /// <returns>String representation for $skiptoken query parameter.</returns>
            public StringBuilder GetSkipToken()
            {
                object[] skipTokenProperties = this.GetSkipTokenProperties();

                bool first = true;
                for (int i = 0; i < skipTokenProperties.Length; i++)
                {
                    object value = skipTokenProperties[i];
                    string stringValue;

                    if (value == null)
                    {
                        stringValue = Parsing.ExpressionConstants.KeywordNull;
                    }
                    else
                    if (!System.Data.Services.Parsing.WebConvert.TryKeyPrimitiveToString(value, out stringValue))
                    {
                        throw new InvalidOperationException(Strings.Serializer_CannotConvertValue(value));
                    }

                    if (!first)
                    {
                        this.skipToken.Append(',');
                    }

                    this.skipToken.Append(stringValue);
                    first = false;
                }

                return this.skipToken;
            }

            /// <summary>Derived classes override this to provide the collection of values for skip token.</summary>
            /// <returns>Array of primitive values that comprise the skip token.</returns>
            protected abstract object[] GetSkipTokenProperties();
        }

        /// <summary>Obtains the skip token from IExpandedResult values.</summary>
        private class SkipTokenBuilderFromExpandedResult : SkipTokenBuilder
        {
            /// <summary>IExpandedResult to lookup for skip token values.</summary>
            private IExpandedResult skipTokenExpandedResult;
            
            /// <summary>Number of values in skip token.</summary>
            private int skipTokenExpressionCount;

            /// <summary>Constructor.</summary>
            /// <param name="skipTokenExpandedResult">IExpandedResult to lookup for skip token values.</param>
            /// <param name="skipTokenExpressionCount">Number of values in skip token.</param>
            public SkipTokenBuilderFromExpandedResult(IExpandedResult skipTokenExpandedResult, int skipTokenExpressionCount)
                : base()
            {
                this.skipTokenExpandedResult = skipTokenExpandedResult;
                this.skipTokenExpressionCount = skipTokenExpressionCount;
            }

            /// <summary>Obtains skip token values by looking up IExpandedResult.</summary>
            /// <returns>Array of primitive values that comprise the skip token.</returns>
            protected override object[] GetSkipTokenProperties()
            {
                object[] values = new object[this.skipTokenExpressionCount];

                for (int i = 0; i < this.skipTokenExpressionCount; i++)
                {
                    String keyName = XmlConstants.SkipTokenPropertyPrefix + i.ToString(System.Globalization.CultureInfo.InvariantCulture);
                    object value = this.skipTokenExpandedResult.GetExpandedPropertyValue(keyName);
                    if (value == DBNull.Value)
                    {
                        value = null;
                    }

                    values[i] = value;
                }

                return values;
            }
        }

        /// <summary>Obtains the skip token by reading properties directly from an object.</summary>
        private class SkipTokenBuilderFromProperties : SkipTokenBuilder
        {
            /// <summary>Object to read skip token values from.</summary>
            private object element;
            
            /// <summary>Collection of properties that comprise the skip token.</summary>
            private ICollection<ResourceProperty> properties;
            
            /// <summary>Current provider.</summary>
            private DataServiceProviderWrapper provider;

            /// <summary>Constructor.</summary>
            /// <param name="element">Object to read skip token values from.</param>
            /// <param name="provider">Current provider.</param>
            /// <param name="properties">Collection of properties that comprise the skip token.</param>
            public SkipTokenBuilderFromProperties(object element, DataServiceProviderWrapper provider, ICollection<ResourceProperty> properties)
                : base()
            {
                this.element = element;
                this.provider = provider;
                this.properties = properties;
            }

            /// <summary>Obtains skip token values by reading properties directly from the last object.</summary>
            /// <returns>Array of primitive values that comprise the skip token.</returns>
            protected override object[] GetSkipTokenProperties()
            {
                object[] values = new object[this.properties.Count];

                int propertyIndex = 0;
                foreach (ResourceProperty property in this.properties)
                {
                    object value = WebUtil.GetPropertyValue(this.provider, this.element, null, property, null);
                    if (value == DBNull.Value)
                    {
                        value = null;
                    }

                    values[propertyIndex++] = value;
                }

                return values;
            }
        }

        /// <summary>Provides the skip token obtained from the custom paging provider.</summary>
        private class SkipTokenBuilderFromCustomPaging : SkipTokenBuilder
        {
            /// <summary>Skip token obtained from custom paging provider.</summary>
            private object[] lastTokenValue;

            /// <summary>Constructor.</summary>
            /// <param name="lastTokenValue">Skip token obtained from custom paging provider.</param>
            public SkipTokenBuilderFromCustomPaging(object[] lastTokenValue)
                : base()
            {
                this.lastTokenValue = lastTokenValue;
            }

            /// <summary>Provides the skip token values that were obtained from custom paging provider.</summary>
            /// <returns>Array of primitive values that comprise the skip token.</returns>
            protected override object[] GetSkipTokenProperties()
            {
                return this.lastTokenValue;
            }
        }        
    }
}
