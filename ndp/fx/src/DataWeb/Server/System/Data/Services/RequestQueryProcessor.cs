//---------------------------------------------------------------------
// <copyright file="RequestQueryProcessor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class capable of processing query arguments.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Client;
    using System.Data.Services.Internal;
    using System.Data.Services.Parsing;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Globalization;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;
    using System.Text;

    /// <summary>Use this class to process a web data service request URI.</summary>
    internal struct RequestQueryProcessor
    {
        #region Private fields.

        /// <summary>MethodInfo for Skip.</summary>
        private static readonly MethodInfo InvokeSkipMethodInfo = typeof(RequestQueryProcessor).GetMethod("InvokeSkip", BindingFlags.NonPublic | BindingFlags.Static);

        /// <summary>MethodInfo for Take.</summary>
        private static readonly MethodInfo InvokeTakeMethodInfo = typeof(RequestQueryProcessor).GetMethod("InvokeTake", BindingFlags.NonPublic | BindingFlags.Static);

        /// <summary>The generic method for CountQueryResult'[T]()</summary>
        private static readonly MethodInfo countQueryResultMethodInfo = typeof(RequestQueryProcessor).GetMethod("CountQueryResult", BindingFlags.NonPublic | BindingFlags.Static);

        /// <summary>Original description over which query composition takes place.</summary>
        private readonly RequestDescription description;

        /// <summary>Whether the $filter query option can be applied to the request.</summary>
        private readonly bool filterQueryApplicable;

        /// <summary>Service with data and configuration.</summary>
        private readonly IDataService service;

        /// <summary>Whether the $orderby, $skip, $take and $count options can be applied to the request.</summary>
        private readonly bool setQueryApplicable;

        /// <summary>Whether the top level request is a candidate for paging.</summary>
        private readonly bool pagingApplicable;

        /// <summary>Has custom paging already been applied?</summary>
        private bool appliedCustomPaging;

        /// <summary>List of paths to be expanded.</summary>
        private List<ExpandSegmentCollection> expandPaths;

        /// <summary>List of paths to be expanded, before resolving the identifiers</summary>
        private List<List<string>> expandPathsAsText;

        /// <summary>Root projection segment of the tree specifying projections and expansions for the query.</summary>
        private RootProjectionNode rootProjectionNode;

        /// <summary>Parser used for parsing ordering expressions</summary>
        private RequestQueryParser.ExpressionParser orderingParser;

        /// <summary>Collection of ordering expressions for the current query</summary>
        private OrderingInfo topLevelOrderingInfo; 

        /// <summary>Whether any order has been applied.</summary>
        private bool orderApplied;

        /// <summary>Value of $skip argument</summary>
        private int? skipCount;
        
        /// <summary>Value of $top argument</summary>
        private int? topCount;

        /// <summary>Query being composed.</summary>
        private IQueryable query;

        /// <summary>Query results to be returned.</summary>
        private IEnumerable queryResults;

        #endregion Private fields.

        /// <summary>Initializes a new <see cref="RequestQueryProcessor"/> instance.</summary>
        /// <param name="service">Service with data and configuration.</param>
        /// <param name="description">Description for request processed so far.</param>
        private RequestQueryProcessor(IDataService service, RequestDescription description)
        {
            this.service = service;
            this.description = description;
            this.orderApplied = false;
            this.skipCount = null;
            this.topCount = null;
            this.query = (IQueryable)description.RequestEnumerable;
            
            this.filterQueryApplicable =
                description.TargetKind == RequestTargetKind.Resource ||
                description.TargetKind == RequestTargetKind.OpenProperty ||
                description.TargetKind == RequestTargetKind.ComplexObject ||
                (description.CountOption == RequestQueryCountOption.ValueOnly);

            this.setQueryApplicable = (description.TargetKind == RequestTargetKind.Resource && !description.IsSingleResult) || 
                                       description.CountOption == RequestQueryCountOption.ValueOnly;

            // Server Driven Paging is not considered for the following cases: 
            // 1. Top level result is not or resource type or it is a single valued result.
            // 2. $count segment provided.
            // 3. Non-GET requests do not honor SDP.
            // 4. Only exception for Non-GET requests is if the request is coming from a Service
            //    operation that returns a set of result values of entity type.
            this.pagingApplicable = (description.TargetKind == RequestTargetKind.Resource && !description.IsSingleResult) && 
                                    (description.CountOption != RequestQueryCountOption.ValueOnly) &&
                                    !description.IsRequestForEnumServiceOperation &&
                                    (service.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.GET || description.SegmentInfos[0].TargetSource == RequestTargetSource.ServiceOperation);

            this.appliedCustomPaging = false;

            this.expandPaths = null;
            this.expandPathsAsText = null;
            this.rootProjectionNode = null;
            this.orderingParser = null;
            this.topLevelOrderingInfo = null;
            this.queryResults = null;
        }

        /// <summary>
        /// Is the top level container for the query paged i.e. we need to use StandardPaging.
        /// </summary>
        private bool IsStandardPaged
        {
            get
            {
                if (this.pagingApplicable && !this.IsCustomPaged)
                {
                    ResourceSetWrapper targetContainer = this.description.LastSegmentInfo.TargetContainer;
                    Debug.Assert(targetContainer != null, "Must have target container for non-open properties");
                    return targetContainer.PageSize != 0;
                }
                else
                {
                    // Target container is null for OpenProperties and we have decided not to 
                    // to enable paging for OpenType scenarios
                    return false;
                }
            }
        }

        /// <summary>Do we need to use CustomPaging for this service.</summary>
        private bool IsCustomPaged
        {
            get
            {
                return this.service.PagingProvider.IsCustomPagedForQuery;
            }
        }

        /// <summary>Checks that no query arguments were sent in the request.</summary>
        /// <param name="service">Service to check.</param>
        /// <param name="checkForOnlyV2QueryParameters">true if only V2 query parameters must be checked, otherwise all the query parameters will be checked.</param>
        /// <remarks>
        /// Regular processing checks argument applicability, but for
        /// service operations that return an IEnumerable this is part
        /// of the contract on service operations, rather than a structural
        /// check on the request.
        /// </remarks>
        internal static void CheckEmptyQueryArguments(IDataService service, bool checkForOnlyV2QueryParameters)
        {
            Debug.Assert(service != null, "service != null");

            DataServiceHostWrapper host = service.OperationContext.Host;
            if ((!checkForOnlyV2QueryParameters &&
                 (!String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringExpand)) ||
                  !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringFilter)) ||
                  !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringOrderBy)) ||
                  !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringSkip)) ||
                  !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringTop)))) ||
                !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringInlineCount)) ||
                !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringSelect)) ||
                !String.IsNullOrEmpty(host.GetQueryStringItem(XmlConstants.HttpQueryStringSkipToken)))
            {
                // 400: Bad Request
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QueryNoOptionsApplicable);
            }
        }

        /// <summary>Checks that no query arguments were sent in the request.</summary>
        /// <param name="service">Service to check.</param>
        /// <remarks>
        /// Regular processing checks argument applicability, but for
        /// service operations that return an IEnumerable this is part
        /// of the contract on service operations, rather than a structural
        /// check on the request.
        /// </remarks>
        internal static void CheckV2EmptyQueryArguments(IDataService service)
        {
            Debug.Assert(service != null, "service != null");
            CheckEmptyQueryArguments(service, service.Provider.IsV1Provider);
        }

        /// <summary>
        /// Composes a property navigation with the appropriate filter lamba, as appropriate.
        /// </summary>
        /// <param name="expression">Member access expression to compose.</param>
        /// <param name="filterLambda">Lambda expression used for the filter.</param>
        /// <param name="propagateNull">Whether null propagation is required on the <paramref name="expression"/>.</param>
        /// <returns>The composed expression.</returns>
        internal static Expression ComposePropertyNavigation(
            Expression expression,
            LambdaExpression filterLambda,
            bool propagateNull)
        {
            Debug.Assert(expression != null, "expression != null");
            Debug.Assert(filterLambda != null, "filterLambda != null");

            Expression fixedFilter = ParameterReplacerVisitor.Replace(
                    filterLambda.Body,
                    filterLambda.Parameters[0],
                    expression);
            
            Expression test;
            if (propagateNull)
            {
                Expression nullConstant = WebUtil.NullLiteral;
                test = Expression.AndAlso(Expression.NotEqual(expression, nullConstant), fixedFilter);
            }
            else
            {
                test = fixedFilter;
            }

            Expression conditionTrue = expression;
            Expression conditionFalse = Expression.Constant(null, conditionTrue.Type);
            return Expression.Condition(test, conditionTrue, conditionFalse);
        }

        /// <summary>
        /// Processes query arguments and returns a request description for 
        /// the resulting query.
        /// </summary>
        /// <param name="service">Service with data and configuration information.</param>
        /// <param name="description">Description for request processed so far.</param>
        /// <returns>A new <see cref="RequestDescription"/>.</returns>
        internal static RequestDescription ProcessQuery(IDataService service, RequestDescription description)
        {
            Debug.Assert(service != null, "service != null");

            // In some cases, like CUD operations, we do not want to allow any query parameters to be specified.
            // But in V1, we didn't have this check hence we cannot fix this now. But we need to check only for 
            // V2 query parameters and stop them
            if ((service.OperationContext.Host.AstoriaHttpVerb != AstoriaVerbs.GET) &&
                description.SegmentInfos[0].TargetSource != RequestTargetSource.ServiceOperation)
            {
                RequestQueryProcessor.CheckV2EmptyQueryArguments(service);
            }

            // When the request doesn't produce an IQueryable result,
            // we can short-circuit all further processing.
            if (!(description.RequestEnumerable is IQueryable))
            {
                RequestQueryProcessor.CheckEmptyQueryArguments(service, false /*checkForOnlyV2QueryParameters*/);
                return description;
            }
            else
            {
                return new RequestQueryProcessor(service, description).ProcessQuery();
            }
        }

        /// <summary>Generic method to invoke a Skip method on an IQueryable source.</summary>
        /// <typeparam name="T">Element type of the source.</typeparam>
        /// <param name="source">Source query.</param>
        /// <param name="count">Element count to skip.</param>
        /// <returns>A new query that skips <paramref name="count"/> elements of <paramref name="source"/>.</returns>
        private static IQueryable InvokeSkip<T>(IQueryable source, int count)
        {
            Debug.Assert(source != null, "source != null");
            IQueryable<T> typedSource = (IQueryable<T>)source;
            return Queryable.Skip<T>(typedSource, count);
        }

        /// <summary>Generic method to invoke a Take method on an IQueryable source.</summary>
        /// <typeparam name="T">Element type of the source.</typeparam>
        /// <param name="source">Source query.</param>
        /// <param name="count">Element count to take.</param>
        /// <returns>A new query that takes <paramref name="count"/> elements of <paramref name="source"/>.</returns>
        private static IQueryable InvokeTake<T>(IQueryable source, int count)
        {
            Debug.Assert(source != null, "source != null");
            IQueryable<T> typedSource = (IQueryable<T>)source;
            return Queryable.Take<T>(typedSource, count);
        }
        
        /// <summary>Count the query result before $top and $skip are applied</summary>
        /// <typeparam name="TElement">Element type of the source</typeparam>
        /// <param name="query">Source query</param>
        /// <returns>The count from the provider</returns>
        private static long CountQueryResult<TElement>(IQueryable<TElement> query)
        {
            return Queryable.LongCount<TElement>(query);
        }

        /// <summary>Reads an $expand or $select clause.</summary>
        /// <param name="value">Value to read.</param>
        /// <param name="select">True if we're parsing $select clause False for $expand</param>
        /// <returns>A list of paths, each of which is a list of identifiers.</returns>
        /// <remarks>Same method is used for both $expand and $select as the syntax is almost identical.
        /// $select allows * at the end of the path while $expand doesn't.</remarks>
        private static List<List<string>> ReadExpandOrSelect(string value, bool select)
        {
            Debug.Assert(!String.IsNullOrEmpty(value), "!String.IsNullOrEmpty(value)");

            List<List<string>> result = new List<List<string>>();
            List<string> currentPath = null;
            ExpressionLexer lexer = new ExpressionLexer(value);
            while (lexer.CurrentToken.Id != TokenId.End)
            {
                string identifier;
                bool lastSegment = false;
                if (select && lexer.CurrentToken.Id == TokenId.Star)
                {
                    identifier = lexer.CurrentToken.Text;
                    lexer.NextToken();
                    lastSegment = true;
                }
                else
                {
                    identifier = lexer.ReadDottedIdentifier();
                }

                if (currentPath == null)
                {
                    currentPath = new List<string>();
                    result.Add(currentPath);
                }

                currentPath.Add(identifier);

                // Check whether we're at the end, whether we're drilling in,
                // or whether we're finishing with this path.
                TokenId tokenId = lexer.CurrentToken.Id;
                if (tokenId != TokenId.End)
                {
                    if (lastSegment || tokenId != TokenId.Slash)
                    {
                        lexer.ValidateToken(TokenId.Comma);
                        currentPath = null;
                    }

                    lexer.NextToken();
                }
            }

            return result;
        }

        /// <summary>Gets the root projection node or creates one if no one exists yet.</summary>
        /// <returns>The root node of the projection tree.</returns>
        private RootProjectionNode GetRootProjectionNode()
        {
            if (this.rootProjectionNode == null)
            {
                // Build the root of the projection and expansion tree
                this.rootProjectionNode = new RootProjectionNode(
                    this.description.LastSegmentInfo.TargetContainer,
                    this.topLevelOrderingInfo,
                    null,
                    this.skipCount,
                    this.topCount,
                    null,
                    this.expandPaths,
                    this.description.TargetResourceType);
            }

            return this.rootProjectionNode;
        }

        /// <summary>Checks and resolved all textual expand paths and removes unnecessary paths.</summary>
        private void CheckExpandPaths()
        {
            Debug.Assert(this.expandPathsAsText != null, "this.expandPaths != null");
            if (this.expandPathsAsText.Count > 0 && this.query == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QueryExpandOptionNotApplicable);
            }

            this.expandPaths = new List<ExpandSegmentCollection>(this.expandPathsAsText.Count);

            for (int i = this.expandPathsAsText.Count - 1; i >= 0; i--)
            {
                ResourceType resourceType = this.description.TargetResourceType;
                ResourceSetWrapper resourceSet = this.description.LastSegmentInfo.TargetContainer;

                List<string> path = this.expandPathsAsText[i];
                ExpandSegmentCollection segments = new ExpandSegmentCollection(path.Count);
                bool ignorePath = false;
                for (int j = 0; j < path.Count; j++)
                {
                    string pathSegment = path[j];
                    ResourceProperty property = resourceType.TryResolvePropertyName(pathSegment);

                    if (property == null)
                    {
                        if (resourceType.IsOpenType)
                        {
                            // Open navigation properties are not supported on OpenTypes.
                            throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(pathSegment));
                        }

                        throw DataServiceException.CreateSyntaxError(Strings.RequestUriProcessor_PropertyNotFound(resourceType.FullName, pathSegment));
                    }

                    if (property.TypeKind == ResourceTypeKind.EntityType)
                    {
                        Expression filter = null;
                        resourceSet = this.service.Provider.GetContainer(resourceSet, resourceType, property);
                        if (resourceSet == null)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(property.Name, resourceType.FullName));
                        }

                        resourceType = resourceSet.ResourceType;

                        bool singleResult = property.Kind == ResourcePropertyKind.ResourceReference;
                        DataServiceConfiguration.CheckResourceRightsForRead(resourceSet, singleResult);
                        filter = DataServiceConfiguration.ComposeQueryInterceptors(this.service, resourceSet);

                        if (resourceSet.PageSize != 0 && !singleResult && !this.IsCustomPaged)
                        {
                            OrderingInfo internalOrderingInfo = new OrderingInfo(true);
                            ParameterExpression p = Expression.Parameter(resourceType.InstanceType, "p");

                            bool useMetadataKeyOrder = false;
                            Dictionary<string, object> dictionary = resourceSet.ResourceSet.CustomState as Dictionary<string, object>;
                            object useMetadataKeyPropertyValue;
                            if (dictionary != null && dictionary.TryGetValue("UseMetadataKeyOrder", out useMetadataKeyPropertyValue))
                            {
                                if ((useMetadataKeyPropertyValue is bool) && ((bool)useMetadataKeyPropertyValue))
                                {
                                    useMetadataKeyOrder = true;
                                }
                            }

                            IEnumerable<ResourceProperty> properties = useMetadataKeyOrder ? resourceSet.ResourceType.Properties : resourceSet.ResourceType.KeyProperties;
                            foreach (var keyProp in properties)
                            {
                                if (!keyProp.IsOfKind(ResourcePropertyKind.Key))
                                {
                                    continue;
                                }

                                Expression e;
                                if (keyProp.CanReflectOnInstanceTypeProperty)
                                {
                                    e = Expression.Property(p, resourceType.GetPropertyInfo(keyProp));
                                }
                                else
                                {
                                    // object LateBoundMethods.GetValue(object, ResourceProperty)
                                    e = Expression.Call(
                                                null, /*instance*/
                                                DataServiceProviderMethods.GetValueMethodInfo,
                                                p,
                                                Expression.Constant(keyProp));
                                    e = Expression.Convert(e, keyProp.Type);
                                }

                                internalOrderingInfo.Add(new OrderingExpression(Expression.Lambda(e, p), true));
                            }

                            segments.Add(new ExpandSegment(property.Name, filter, resourceSet.PageSize, resourceSet, property, internalOrderingInfo));

                            // Paged expanded results force the response version to be 2.0
                            this.description.RaiseResponseVersion(2, 0);
                        }
                        else
                        {
                            // Expansion of collection could result in custom paging provider giving next link, so we need to set the null continuation token.
                            if (!singleResult && this.IsCustomPaged)
                            {
                                this.CheckAndApplyCustomPaging(null);
                            }

                            segments.Add(new ExpandSegment(property.Name, filter, this.service.Configuration.MaxResultsPerCollection, resourceSet, property, null));
                        }

                        // We need to explicitly update the feature version here since we may not bump response version
                        this.description.UpdateAndCheckEpmFeatureVersion(resourceSet, this.service);

                        // The expanded resource type may have friendly feeds. Update version of the response accordingly
                        //
                        // Note the response DSV is payload specific and since for GET we won't know if the instances to be
                        // serialized will contain any properties with FF KeepInContent=false until serialization time which
                        // happens after the headers are written, the best we can do is to determin this at the set level.
                        this.description.UpdateEpmResponseVersion(this.service.OperationContext.Host.RequestAccept, resourceSet, this.service.Provider);

                        ignorePath = false;
                    }
                    else
                    {
                        ignorePath = true;
                    }
                }

                // Even though the path might be correct, we might need to 
                // ignore because it's a primitive.
                if (ignorePath)
                {
                    this.expandPathsAsText.RemoveAt(i);
                }
                else
                {
                    this.expandPaths.Add(segments);

                    // And now build the projection and expansion tree nodes for this expand path as well
                    ExpandedProjectionNode currentNode = this.GetRootProjectionNode();
                    for (int j = 0; j < segments.Count; j++)
                    {
                        ExpandSegment segment = segments[j];
                        ExpandedProjectionNode childNode = (ExpandedProjectionNode)currentNode.FindNode(segment.Name);
                        if (childNode == null)
                        {
                            childNode = new ExpandedProjectionNode(
                                segment.Name,
                                segment.ExpandedProperty,
                                segment.Container,
                                segment.OrderingInfo,
                                segment.Filter,
                                null,
                                segment.Container.PageSize != 0 ? (int?)segment.Container.PageSize : null,
                                segment.MaxResultsExpected != Int32.MaxValue ? (int?)segment.MaxResultsExpected : null);
                            currentNode.AddNode(childNode);
                        }

                        currentNode = childNode;
                    }

                    this.GetRootProjectionNode().ExpansionsSpecified = true;
                }
            }
        }

        /// <summary>Checks that the query option is applicable to this request.</summary>
        private void CheckFilterQueryApplicable()
        {
            if (!this.filterQueryApplicable)
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QueryFilterOptionNotApplicable);
            }
        }

        /// <summary>Checks that set query options are applicable to this request.</summary>
        private void CheckSetQueryApplicable()
        {
            if (!this.setQueryApplicable)
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QuerySetOptionsNotApplicable);
            }
        }

        /// <summary>Processes the $expand argument of the request.</summary>
        private void ProcessExpand()
        {
            string expand = this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringExpand);
            if (!String.IsNullOrEmpty(expand))
            {
                this.expandPathsAsText = ReadExpandOrSelect(expand, false);
            }
            else
            {
                this.expandPathsAsText = new List<List<string>>();
            }

            this.CheckExpandPaths();
            this.service.InternalApplyingExpansions(this.query, this.expandPaths);
        }

        /// <summary>Builds the tree of <see cref="ProjectionNode"/> to represent the $select query option.</summary>
        /// <param name="selectPathsAsText">The value of the $select query option parsed into a list of lists.</param>
        /// <remarks>This method assumes that $expand was already processed. And we have the tree
        /// of <see cref="ExpandedProjectionNode"/> objects for the $expand query option already built.</remarks>
        private void ApplyProjectionsToExpandTree(List<List<string>> selectPathsAsText)
        {
            for (int i = selectPathsAsText.Count - 1; i >= 0; i--)
            {
                List<string> path = selectPathsAsText[i];
                ExpandedProjectionNode currentNode = this.GetRootProjectionNode();
                Debug.Assert(currentNode.ResourceType == this.description.TargetResourceType, "The resource type of the root doesn't match the target type of the query.");

                for (int j = 0; j < path.Count; j++)
                {
                    Debug.Assert(currentNode != null, "Can't apply projections to non-expanded nodes.");
                    string pathSegment = path[j];
                    bool lastPathSegment = (j == path.Count - 1);

                    currentNode.ProjectionFound = true;

                    // '*' is special, it means "Project all immediate properties on this level."
                    if (pathSegment == "*")
                    {
                        Debug.Assert(lastPathSegment, "The * select segment must be the last one. This should have been checked already by the ReadExpandOrSelect method.");
                        currentNode.ProjectAllImmediateProperties = true;
                        break;
                    }

                    ResourceType currentResourceType = currentNode.ResourceType;
                    ResourceProperty property = currentResourceType.TryResolvePropertyName(pathSegment);
                    if (property == null)
                    {
                        if (!currentResourceType.IsOpenType)
                        {
                            throw DataServiceException.CreateSyntaxError(Strings.RequestUriProcessor_PropertyNotFound(currentResourceType.FullName, pathSegment));
                        }

                        if (!lastPathSegment)
                        {
                            // Open navigation properties are not supported on OpenTypes.
                            throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(pathSegment));
                        }
                    }
                    else
                    {
                        switch (property.TypeKind)
                        {
                            case ResourceTypeKind.Primitive:
                                if (!lastPathSegment)
                                {
                                    throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_PrimitivePropertyUsedAsNavigationProperty(currentResourceType.FullName, pathSegment));
                                }

                                break;

                            case ResourceTypeKind.ComplexType:
                                if (!lastPathSegment)
                                {
                                    throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_ComplexPropertyAsInnerSelectSegment(currentResourceType.FullName, pathSegment));
                                }

                                break;

                            case ResourceTypeKind.EntityType:
                                break;

                            default:
                                Debug.Fail("Unexpected property type.");
                                break;
                        }
                    }

                    // If we already have such segment, reuse it. We ignore duplicates on the input.
                    ProjectionNode newNode = currentNode.FindNode(pathSegment);
                    if (newNode == null)
                    {
                        // We need expanded node to already exist, otherwise the projection is invalid
                        if (!lastPathSegment)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_ProjectedPropertyWithoutMatchingExpand(currentNode.PropertyName));
                        }

                        // If it's the last segment, just create a simple projection node for it
                        newNode = new ProjectionNode(pathSegment, property);
                        currentNode.AddNode(newNode);
                    }

                    currentNode = newNode as ExpandedProjectionNode;

                    Debug.Assert(
                        currentNode == null || currentNode.ResourceType == property.ResourceType,
                        "If we're traversing over a nav. property it's resource type must match the resource type of the expanded segment.");

                    // If this is the last segment in the path and it was a navigation property,
                    // mark it to include the entire subtree
                    if (lastPathSegment && currentNode != null)
                    {
                        currentNode.ProjectionFound = true;
                        currentNode.MarkSubtreeAsProjected();
                    }
                }
            }
        }

        /// <summary>Processes the $select argument of the request.</summary>
        private void ProcessSelect()
        {
            // Parse the $select query option into paths
            string select = this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringSelect);
            List<List<string>> selectPathsAsText;
            if (!String.IsNullOrEmpty(select))
            {
                // Throw if $select requests have been disabled by the user
                if (!this.service.Configuration.DataServiceBehavior.AcceptProjectionRequests)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataServiceConfiguration_ProjectionsNotSupportedInV1Server);
                }

                selectPathsAsText = ReadExpandOrSelect(select, true);
            }
            else
            {
                // No projections specified on the input
                if (this.rootProjectionNode != null)
                {
                    // There are expansions, but no projections
                    // Mark all the expanded nodes in the tree to include all properties.
                    this.rootProjectionNode.MarkSubtreeAsProjected();
                }

                return;
            }

            // We only allow $select on true queries (we must have IQueryable)
            if (selectPathsAsText.Count > 0 && this.query == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QuerySelectOptionNotApplicable);
            }

            // We only allow $select on entity/entityset queries. Queries which return a primitive/complex value don't support $select.
            if (this.description.TargetResourceType == null || (this.description.TargetResourceType.ResourceTypeKind != ResourceTypeKind.EntityType))
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QuerySelectOptionNotApplicable);
            }

            // $select can't be used on $links URIs as it doesn't make sense
            if (this.description.SegmentInfos.Any(si => si.TargetKind == RequestTargetKind.Link))
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_QuerySelectOptionNotApplicable);
            }

            // We require the request to have 2.0 version when using $select
            this.description.RaiseMinimumVersionRequirement(2, 0);

            // We found some projections in the query - mark it as such
            this.GetRootProjectionNode().ProjectionsSpecified = true;

            // Apply projections to the $expand tree
            this.ApplyProjectionsToExpandTree(selectPathsAsText);

            // Cleanup the tree
            if (this.rootProjectionNode != null)
            {
                // Remove all expanded nodes which are not projected
                this.rootProjectionNode.RemoveNonProjectedNodes();

                // Now cleanup the projected tree. We already eliminated explicit duplicates during construction
                //   now we want to remove redundant properties when * was used or when the subtree was already selected.
                this.rootProjectionNode.ApplyWildcardsAndSort();
            }
        }

        /// <summary>
        /// Generate the queryResults for the request
        /// </summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private void GenerateQueryResult()
        {
            if (this.description.CountOption == RequestQueryCountOption.ValueOnly)
            {
                // $count segment will be counted last, before Expand and SDP affects the query
                MethodInfo mi = countQueryResultMethodInfo.MakeGenericMethod(this.query.ElementType);
                this.description.RaiseMinimumVersionRequirement(2, 0);
                this.description.RaiseResponseVersion(2, 0);

                // set query result to Count
                this.queryResults = new long[] { (long)mi.Invoke(null, new object[] { this.query }) }
                    .AsEnumerable();
            }
            else if (this.rootProjectionNode != null)
            {
                IExpandProvider expandProvider = this.service.Provider.GetService<IExpandProvider>(this.service);
                if (expandProvider != null)
                {
                    // If top level result is paged, then we can not perform the operation since that would require
                    // passing in the ordering expressions for $skiptoken generation for top level results which
                    // the IExpandProvider interface does not support and thus we would get wrong results
                    if (this.IsStandardPaged)
                    {
                        throw new DataServiceException(500, Strings.DataService_SDP_TopLevelPagedResultWithOldExpandProvider);
                    }

                    // If there's projection we can't perform the operation since that would require
                    // passing the projection info into the IExpandProvider, which it doesn't support
                    // and thus would not perform the projection.
                    if (this.rootProjectionNode.ProjectionsSpecified)
                    {
                        throw new DataServiceException(500, Strings.DataService_Projections_ProjectionsWithOldExpandProvider);
                    }

                    // V1 behaviour of expand in this case, although this is semantically incorrect, since we are doing
                    // a select after orderby and skip top, which could mess up the results, assuming here that count
                    // has already been processed
                    this.ProcessOrderBy();
                    this.ProcessSkipAndTop();
                    this.queryResults = expandProvider.ApplyExpansions(this.query, this.rootProjectionNode.ExpandPaths);

                    // We need to mark the ExpandPaths as "possibly modified" and make serializer use those instead.
                    this.rootProjectionNode.UseExpandPathsForSerialization = true;
                }
                else
                {
                    IProjectionProvider projectionProvider = this.service.Provider.ProjectionProvider;

                    // We already have the parameter information
                    // * Ordering expressions through ObtainOrderingExpressions
                    // * Skip & Top through ObtainSkipTopCounts
                    if (projectionProvider == null)
                    {
                        Debug.Assert(!this.service.Provider.IsV1Provider, "All V1 providers should implement the IProjectionProvider interface.");

                        // We are to run a query against IDSQP. Since the IProjectionProvider interface is not public
                        //   the provider will have to be able to handle queries generated by our BasicExpandProvider
                        //   so we should make only minimalistic assumptions about the provider's abilities.
                        // In here we will assume that:
                        //   - the provider doesn't expand on demand, that is we need to generate projections for all expansions in the query
                        //   - the provider requires correct casting to "System.Object" when we assign a value to a property of type "System.Object"
                        // A side effect of these assumptions is that if the provider just propagates the calls (with small modifications) to Linq to Objects
                        //   the query should just work (nice property, as this might be rather common).
                        projectionProvider = new BasicExpandProvider(this.service.Provider, false, true);
                    }

                    this.query = projectionProvider.ApplyProjections(this.query, this.rootProjectionNode);
                    this.queryResults = this.query;
                }
            }
            else
            {
                // Special case where although there were expand expressions, they were ignored because they did not refer to entity sets
                if (!String.IsNullOrEmpty(this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringExpand)))
                {
                    this.ProjectSkipTokenForNonExpand();
                    this.ProcessOrderBy();
                    this.ProcessSkipAndTop();
                }

                this.queryResults = this.query;
            }

            Debug.Assert(this.queryResults != null, "this.queryResults != null");
        }

        /// <summary>Processes the $filter argument of the request.</summary>
        private void ProcessFilter()
        {
            string filter = this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringFilter);
            if (!String.IsNullOrEmpty(filter))
            {
                this.CheckFilterQueryApplicable();
                this.query = RequestQueryParser.Where(this.service, this.description.LastSegmentInfo.TargetContainer, this.description.TargetResourceType, this.query, filter);
            }
        }

        /// <summary>Processes the $skiptoken argument of the request.</summary>
        private void ProcessSkipToken()
        {
            // Obtain skip token from query parameters.
            String skipToken = this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringSkipToken);

            if (this.pagingApplicable)
            {
                if (this.IsCustomPaged)
                {
                    this.ApplyCustomPaging(skipToken);
                }
                else
                {
                    this.ApplyStandardPaging(skipToken);
                }
            }
            else
            if (!String.IsNullOrEmpty(skipToken))
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_SkipTokenNotAllowed);
            }
        }
        
        /// <summary>Applies standard paging to the query.</summary>
        /// <param name="skipToken">Skip token obtained from query parameters.</param>
        private void ApplyStandardPaging(string skipToken)
        {
            Debug.Assert(!this.IsCustomPaged, "Custom paging should be disabled for this function to be called.");
            if (!String.IsNullOrEmpty(skipToken))
            {
                // Parse the skipToken to obtain each positional value
                KeyInstance k = null;
                WebUtil.CheckSyntaxValid(KeyInstance.TryParseNullableTokens(skipToken, out k));

                // Ordering constraint in the presence of skipToken will have the following settings:
                // * First there will be the provided constraints based on $orderby
                // * Followed by all the key columns in the resource type

                // Validate that the skipToken had as many positional values as the number of ordering constraints
                if (this.topLevelOrderingInfo.OrderingExpressions.Count != k.PositionalValues.Count)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataService_SDP_SkipTokenNotMatchingOrdering(k.PositionalValues.Count, skipToken, this.topLevelOrderingInfo.OrderingExpressions.Count));
                }

                // Build the Where clause with the provided skip token
                this.query = RequestUriProcessor.InvokeWhereForType(
                                this.query, 
                                this.orderingParser.BuildSkipTokenFilter(this.topLevelOrderingInfo, k));

                // $skiptoken is expected to be only sent by 2.0 & above clients
                this.description.RaiseMinimumVersionRequirement(2, 0);
                this.description.RaiseResponseVersion(2, 0);
            }
        }
        
        /// <summary>Applies custom paging to the query.</summary>
        /// <param name="skipToken">Skip token obtained from query parameters.</param>
        private void ApplyCustomPaging(string skipToken)
        {
            Debug.Assert(this.IsCustomPaged, "Custom paging should be enabled for this function to be called.");
            if (!String.IsNullOrEmpty(skipToken))
            {
                // Parse the skipToken to obtain each positional value
                KeyInstance k = null;
                WebUtil.CheckSyntaxValid(KeyInstance.TryParseNullableTokens(skipToken, out k));

                ParameterExpression p = Expression.Parameter(this.description.LastSegmentInfo.TargetResourceType.InstanceType, "it");

                RequestQueryParser.ExpressionParser skipTokenParser = new RequestQueryParser.ExpressionParser(
                                            this.service,
                                            this.description.LastSegmentInfo.TargetContainer,
                                            this.description.LastSegmentInfo.TargetResourceType,
                                            p,
                                            "");

                object[] convertedValues = new object[k.PositionalValues.Count];
                int i = 0;
                foreach (var value in k.PositionalValues)
                {
                    convertedValues[i++] = skipTokenParser.ParseSkipTokenLiteral((string)value);
                }

                this.CheckAndApplyCustomPaging(convertedValues);

                // $skiptoken is expected to be only sent by 2.0 & above clients
                this.description.RaiseMinimumVersionRequirement(2, 0);
            }
            else
            {
                this.CheckAndApplyCustomPaging(null);
            }
        }

        /// <summary>Processes the $orderby argument of the request.</summary>
        private void ProcessOrderBy()
        {
            Debug.Assert(this.topLevelOrderingInfo != null, "Must have valid ordering information in ProcessOrderBy");
            if (this.topLevelOrderingInfo.OrderingExpressions.Count > 0)
            {
                this.query = RequestQueryParser.OrderBy(this.service, this.query, this.topLevelOrderingInfo);
                this.orderApplied = true;
            }
        }

        /// <summary>Processes the $inlinecount argument of the request.</summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private void ProcessCount()
        {
            string count = this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringInlineCount);

            if (!String.IsNullOrEmpty(count))
            {
                // Throw if $inlinecount requests have been disabled by the user
                if (!this.service.Configuration.DataServiceBehavior.AcceptCountRequests)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataServiceConfiguration_CountNotSupportedInV1Server);
                }

                count = count.TrimStart();
                
                // none
                if (count.Equals(XmlConstants.UriRowCountOffOption))
                {
                    return;
                }

                // only get
                if (this.service.OperationContext.Host.AstoriaHttpVerb != AstoriaVerbs.GET)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_RequestVerbCannotCountError);
                }

                // if already counting value only, then fail
                if (this.description.CountOption == RequestQueryCountOption.ValueOnly)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_InlineCountWithValueCount);
                }

                // Only applied to a set
                this.CheckSetQueryApplicable();

                MethodInfo mi = countQueryResultMethodInfo.MakeGenericMethod(this.query.ElementType);

                if (count == XmlConstants.UriRowCountAllOption)
                {
                    this.description.CountValue = (long)mi.Invoke(null, new object[] { this.query });
                    this.description.CountOption = RequestQueryCountOption.Inline;
                    this.description.RaiseMinimumVersionRequirement(2, 0);
                    this.description.RaiseResponseVersion(2, 0);
                }
                else
                {
                    throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_InvalidCountOptionError);
                }
            }
        }

        /// <summary>
        /// Builds the collection of ordering expressions including implicit ordering if paging is required at top level
        /// </summary>
        private void ObtainOrderingExpressions()
        {
            const String Comma = ",";
            const char Space = ' ';
            const String AscendingOrderIdentifier = "asc";

            StringBuilder orderBy = new StringBuilder(this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringOrderBy));
            if (orderBy.Length > 0)
            {
                this.CheckSetQueryApplicable();
            }
            
            Debug.Assert(this.topLevelOrderingInfo == null, "Must only be called once per query");

            ResourceType rt = this.description.TargetResourceType;
            Debug.Assert(rt != null || this.setQueryApplicable == false, "Resource type must be known for Ordering to be applied.");

            this.topLevelOrderingInfo = new OrderingInfo(this.IsStandardPaged);

            // We need to generate ordering expression, if either the result is paged, or we have
            // skip or top count request because in that case, the skip or top has to happen in
            // the expand provider
            if (this.IsStandardPaged || this.topCount.HasValue || this.skipCount.HasValue)
            {
                Debug.Assert(this.description.LastSegmentInfo.TargetContainer != null, "Resource set must be known for Ordering to be applied.");
                ResourceSetWrapper resourceSet = this.description.LastSegmentInfo.TargetContainer;

                // Additional ordering expressions in case paging is supported
                String separator = orderBy.Length > 0 ? Comma : String.Empty;
                bool useMetadataKeyOrder = false;
                Dictionary<string, object> dictionary = resourceSet.ResourceSet.CustomState as Dictionary<string, object>;
                object useMetadataKeyPropertyValue;
                if (dictionary != null && dictionary.TryGetValue("UseMetadataKeyOrder", out useMetadataKeyPropertyValue))
                {
                    if ((useMetadataKeyPropertyValue is bool) && ((bool)useMetadataKeyPropertyValue))
                    {
                        useMetadataKeyOrder = true;
                    }
                }

                IEnumerable<ResourceProperty> properties = useMetadataKeyOrder ? resourceSet.ResourceType.Properties : resourceSet.ResourceType.KeyProperties;
                foreach (var keyProp in properties)
                {
                    if (!keyProp.IsOfKind(ResourcePropertyKind.Key))
                    {
                        continue;
                    }

                    orderBy.Append(separator).Append(keyProp.Name).Append(Space).Append(AscendingOrderIdentifier);
                    separator = Comma;
                }

                Debug.Assert(this.query.ElementType == rt.InstanceType, "Resource type should match query element type when in this function");
            }

            String orderByText = orderBy.ToString();

            if (!String.IsNullOrEmpty(orderByText))
            {
                ParameterExpression elementParameter = Expression.Parameter(rt.InstanceType, "element");

                this.orderingParser = new RequestQueryParser.ExpressionParser(
                                            this.service, 
                                            this.description.LastSegmentInfo.TargetContainer, 
                                            rt, 
                                            elementParameter, 
                                            orderByText);
                
                IEnumerable<OrderingExpression> ordering = this.orderingParser.ParseOrdering();

                foreach (OrderingExpression o in ordering)
                {
                    this.topLevelOrderingInfo.Add(new OrderingExpression(Expression.Lambda(o.Expression, elementParameter), o.IsAscending));
                }

                if (this.IsStandardPaged)
                {
                    this.description.SkipTokenExpressionCount = this.topLevelOrderingInfo.OrderingExpressions.Count;
                    this.description.SkipTokenProperties = NeedSkipTokenVisitor.CollectSkipTokenProperties(this.topLevelOrderingInfo, rt);
                }
            }
        }

        /// <summary>
        /// Processes query arguments and returns a request description for 
        /// the resulting query.
        /// </summary>
        /// <returns>A modified <see cref="RequestDescription"/> that includes query information.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private RequestDescription ProcessQuery()
        {
            // Obtains the values of $skip and $top arguments
            this.ObtainSkipTopCounts();

            // Obtain ordering information for the current request
            this.ObtainOrderingExpressions();

            // NOTE: The order set by ProcessOrderBy may be reset by other operations other than skip/top,
            // so filtering needs to occur first.
            this.ProcessFilter();

            // $inlinecount ignores SDP, Skip and Top
            this.ProcessCount();

            // $skiptoken is just like a filter, so it should be immediately after $filter
            this.ProcessSkipToken();

            if (String.IsNullOrEmpty(this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringExpand))
                && String.IsNullOrEmpty(this.service.OperationContext.Host.GetQueryStringItem(XmlConstants.HttpQueryStringSelect)))
            {
                this.ProjectSkipTokenForNonExpand();
                this.ProcessOrderBy();
                this.ProcessSkipAndTop();
            }
            else if (this.description.CountOption == RequestQueryCountOption.ValueOnly)
            {
                // We need to take $top and $skip into account when executing $count requests.
                // The issue is if there is a projection, we push the $skip and $top into the projection node
                // and hence we miss it when $count with $select is specified.
                this.ProcessOrderBy();
                this.ProcessSkipAndTop();
            }

            // NOTE: expand goes last, as it may be reset by other operations.
            this.ProcessExpand();

            this.ProcessSelect();

            this.GenerateQueryResult();
#if DEBUG
            // We analyze the query here to detect if we have incorrectly inserted
            // calls to OTM for non-open types
            if (this.service.Provider.AreAllResourceTypesNonOpen)
            {
                // Verify that there is no call to OTMs here.
                if (!(this.queryResults is BaseServiceProvider.QueryableOverEnumerable))
                {
                    OpenTypeMethodCallDetector.CheckForOpenTypeMethodCalls((this.queryResults as IQueryable ?? this.query).Expression);
                }
            }
#endif

            this.service.InternalOnRequestQueryConstructed(this.queryResults as IQueryable ?? this.query);
            Debug.Assert(this.queryResults != null, "this.queryResults != null -- otherwise ProcessExpand didn't set it");

            return new RequestDescription(this.description, this.queryResults, this.rootProjectionNode);
        }

        /// <summary>
        /// In case $expand is not provided while the results are still paged, we need to create a wrapper
        /// for the object in order to project the skip tokens corresponding to the result sequence
        /// </summary>
        private void ProjectSkipTokenForNonExpand()
        {
            if (this.IsStandardPaged && this.description.SkipTokenProperties == null)
            {
                Type queryElementType = this.query.ElementType;

                ParameterExpression expandParameter = Expression.Parameter(queryElementType, "p");

                StringBuilder skipTokenDescription = new StringBuilder();
                
                Type skipTokenWrapperType = this.GetSkipTokenWrapperTypeAndDescription(skipTokenDescription);
                
                MemberBinding[] skipTokenBindings = this.GetSkipTokenBindings(skipTokenWrapperType, skipTokenDescription.ToString(), expandParameter);

                Type resultWrapperType = WebUtil.GetWrapperType(new Type[] { queryElementType, skipTokenWrapperType }, null);

                MemberBinding[] resultWrapperBindings = new MemberBinding[3];
                resultWrapperBindings[0] = Expression.Bind(resultWrapperType.GetProperty("ExpandedElement"), expandParameter);
                resultWrapperBindings[1] = Expression.Bind(resultWrapperType.GetProperty("Description"), Expression.Constant(XmlConstants.HttpQueryStringSkipToken));
                resultWrapperBindings[2] = Expression.Bind(
                                                resultWrapperType.GetProperty("ProjectedProperty0"),
                                                Expression.MemberInit(Expression.New(skipTokenWrapperType), skipTokenBindings));

                Expression resultBody = Expression.MemberInit(Expression.New(resultWrapperType), resultWrapperBindings);

                this.query = RequestUriProcessor.InvokeSelectForTypes(this.query, resultWrapperType, Expression.Lambda(resultBody, expandParameter));

                // Updates the ordering expressions with the ones that take the result wrapper type as parameter 
                // and dereference the ExpandedElement property
                this.UpdateOrderingInfoWithSkipTokenWrapper(resultWrapperType);
            }
        }

        /// <summary>
        /// Obtains the wrapper type for the $skiptoken along with description of properties in the wrapper
        /// </summary>
        /// <param name="skipTokenDescription">Description for the skip token properties</param>
        /// <returns>Type of $skiptoken wrapper</returns>
        private Type GetSkipTokenWrapperTypeAndDescription(StringBuilder skipTokenDescription)
        {
            const String Comma = ",";

            Type[] skipTokenTypes = new Type[this.topLevelOrderingInfo.OrderingExpressions.Count + 1];

            skipTokenTypes[0] = this.query.ElementType;

            int i = 0;
            String separator = String.Empty;
            foreach (var ordering in this.topLevelOrderingInfo.OrderingExpressions)
            {
                skipTokenTypes[i + 1] = ((LambdaExpression)ordering.Expression).Body.Type;
                skipTokenDescription.Append(separator).Append(XmlConstants.SkipTokenPropertyPrefix + i.ToString(System.Globalization.CultureInfo.InvariantCulture));
                separator = Comma;
                i++;
            }

            return WebUtil.GetWrapperType(skipTokenTypes, Strings.BasicExpandProvider_SDP_UnsupportedOrderingExpressionBreadth);
        }

        /// <summary>
        /// Given the wrapper type and description, returns bindings for the wrapper type for skip token
        /// </summary>
        /// <param name="skipTokenWrapperType">Wrapper type</param>
        /// <param name="skipTokenDescription">Description</param>
        /// <param name="expandParameter">Top level parameter type</param>
        /// <returns>Array of bindings for skip token</returns>
        private MemberBinding[] GetSkipTokenBindings(Type skipTokenWrapperType, String skipTokenDescription, ParameterExpression expandParameter)
        {
            MemberBinding[] skipTokenBindings = new MemberBinding[this.topLevelOrderingInfo.OrderingExpressions.Count + 2];
            skipTokenBindings[0] = Expression.Bind(skipTokenWrapperType.GetProperty("ExpandedElement"), expandParameter);
            skipTokenBindings[1] = Expression.Bind(skipTokenWrapperType.GetProperty("Description"), Expression.Constant(skipTokenDescription.ToString()));

            int i = 0;
            foreach (var ordering in this.topLevelOrderingInfo.OrderingExpressions)
            {
                LambdaExpression sourceLambda = (LambdaExpression)ordering.Expression;
                Expression source = ParameterReplacerVisitor.Replace(sourceLambda.Body, sourceLambda.Parameters[0], expandParameter);
                MemberInfo member = skipTokenWrapperType.GetProperty("ProjectedProperty" + i.ToString(System.Globalization.CultureInfo.InvariantCulture));
                skipTokenBindings[i + 2] = Expression.Bind(member, source);
                i++;
            }
            
            return skipTokenBindings;
        }

        /// <summary>
        /// Updates the topLevelOrderingInfo member with the new collection of expressions that 
        /// dereference the ExpandedElement property on the top level wrapper object
        /// </summary>
        /// <param name="resultWrapperType">Type of top level wrapper object</param>
        private void UpdateOrderingInfoWithSkipTokenWrapper(Type resultWrapperType)
        {
            OrderingInfo newOrderingInfo = new OrderingInfo(true);

            ParameterExpression wrapperParameter = Expression.Parameter(resultWrapperType, "w");

            foreach (var ordering in this.topLevelOrderingInfo.OrderingExpressions)
            {
                LambdaExpression oldExpression = (LambdaExpression)ordering.Expression;
                Expression newOrdering = ParameterReplacerVisitor.Replace(
                                            oldExpression.Body, 
                                            oldExpression.Parameters[0], 
                                            Expression.MakeMemberAccess(wrapperParameter, resultWrapperType.GetProperty("ExpandedElement")));
                
                newOrderingInfo.Add(new OrderingExpression(Expression.Lambda(newOrdering, wrapperParameter), ordering.IsAscending));
            }

            this.topLevelOrderingInfo = newOrderingInfo;
        }

        /// <summary>Processes the $skip and/or $top argument of the request by composing query with Skip and/or Take methods.</summary>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private void ProcessSkipAndTop()
        {
            Debug.Assert(this.query != null, "this.query != null");
            if (this.skipCount.HasValue)
            {
                Debug.Assert(this.orderApplied, "Ordering must have already been applied.");
                this.query = (IQueryable)RequestQueryProcessor
                                            .InvokeSkipMethodInfo
                                            .MakeGenericMethod(this.query.ElementType)
                                            .Invoke(null, new object[] { this.query, this.skipCount.Value });
                Debug.Assert(this.query != null, "this.query != null");
            }

            if (this.topCount.HasValue)
            {
                Debug.Assert(this.orderApplied, "Ordering must have already been applied.");
                this.query = (IQueryable)RequestQueryProcessor
                                            .InvokeTakeMethodInfo
                                            .MakeGenericMethod(this.query.ElementType)
                                            .Invoke(null, new object[] { this.query, this.topCount.Value });
                Debug.Assert(this.query != null, "this.query != null");
            }
        }

        /// <summary>
        /// Finds out the appropriate value for skip and top parameters for the current request
        /// </summary>
        private void ObtainSkipTopCounts()
        {
            int count;

            if (this.ReadSkipOrTopArgument(XmlConstants.HttpQueryStringSkip, out count))
            {
                this.skipCount = count;
            }

            int pageSize = 0;
            if (this.IsStandardPaged)
            {
                pageSize = this.description.LastSegmentInfo.TargetContainer.PageSize;
            }

            if (this.ReadSkipOrTopArgument(XmlConstants.HttpQueryStringTop, out count))
            {
                this.topCount = count;
                if (this.IsStandardPaged && pageSize < this.topCount.Value)
                {
                    // If $top is greater than or equal to page size, we will need a $skiptoken and
                    // thus our response will be 2.0
                    this.description.RaiseResponseVersion(2, 0);
                    this.topCount = pageSize;
                }
            }
            else if (this.IsStandardPaged)
            {
                // Paging forces response version of 2.0
                this.description.RaiseResponseVersion(2, 0);
                this.topCount = pageSize;
            }

            if (this.topCount != null || this.skipCount != null)
            {
                this.CheckSetQueryApplicable();
            }
        }

        /// <summary>
        /// Checks whether the specified argument should be processed and what 
        /// its value is.
        /// </summary>
        /// <param name="queryItem">Name of the query item, $top or $skip.</param>
        /// <param name="count">The value for the query item.</param>
        /// <returns>true if the argument should be processed; false otherwise.</returns>
        private bool ReadSkipOrTopArgument(string queryItem, out int count)
        {
            Debug.Assert(queryItem != null, "queryItem != null");

            string itemText = this.service.OperationContext.Host.GetQueryStringItem(queryItem);
            if (String.IsNullOrEmpty(itemText))
            {
                count = 0;
                return false;
            }

            if (!Int32.TryParse(itemText, NumberStyles.Integer, CultureInfo.InvariantCulture, out count))
            {
                throw DataServiceException.CreateSyntaxError(
                    Strings.RequestQueryProcessor_IncorrectArgumentFormat(queryItem, itemText));
            }

            return true;
        }

        /// <summary>Checks if custom paging is already applied, if not, applies it and raises response version.</summary>
        /// <param name="skipTokenValues">Values of skip tokens.</param>
        private void CheckAndApplyCustomPaging(object[] skipTokenValues)
        {
            if (!this.appliedCustomPaging)
            {
                this.service.PagingProvider.PagingProviderInterface.SetContinuationToken(
                        this.query, 
                        this.description.LastSegmentInfo.TargetResourceType, 
                        skipTokenValues);

                this.description.RaiseResponseVersion(2, 0);
                this.appliedCustomPaging = true;
            }
        }

#if DEBUG
        /// <summary>Detects calls to OpenTypeMethods members and asserts if it finds any.</summary>
        private class OpenTypeMethodCallDetector : ALinqExpressionVisitor
        {
            /// <summary>Public interface for using this class.</summary>
            /// <param name="input">Input expression.</param>
            public static void CheckForOpenTypeMethodCalls(Expression input)
            {
                new OpenTypeMethodCallDetector().Visit(input);
            }

            /// <summary>Forgiving Visit method which allows unknown expression types to pass through.</summary>
            /// <param name="exp">Input expression.</param>
            /// <returns>Visit expression.</returns>
            internal override Expression Visit(Expression exp)
            {
                try
                {
                    return base.Visit(exp);
                }
                catch (NotSupportedException)
                {
                    return exp;
                }
            }

            /// <summary>Method call visitor.</summary>
            /// <param name="m">Method call being visited.</param>
            /// <returns>Whatever is provided on input.</returns>
            internal override Expression VisitMethodCall(MethodCallExpression m)
            {
                if (m.Method.DeclaringType == typeof(OpenTypeMethods))
                {
                    throw new InvalidOperationException("Unexpected call to OpenTypeMethods found in query when the provider did not have any OpenTypes.");
                }

                return base.VisitMethodCall(m);
            }

            internal override Expression VisitBinary(BinaryExpression b)
            {
                if (b.Method != null && b.Method.DeclaringType == typeof(OpenTypeMethods))
                {
                    throw new InvalidOperationException("Unexpected call to OpenTypeMethods found in query when the provider did not have any OpenTypes.");
                }

                return base.VisitBinary(b);
            }

            internal override Expression VisitUnary(UnaryExpression u)
            {
                if (u.Method != null && u.Method.DeclaringType == typeof(OpenTypeMethods))
                {
                    throw new InvalidOperationException("Unexpected call to OpenTypeMethods found in query when the provider did not have any OpenTypes.");
                }
            
                return base.VisitUnary(u);
            }
        }
#endif
    }
}
