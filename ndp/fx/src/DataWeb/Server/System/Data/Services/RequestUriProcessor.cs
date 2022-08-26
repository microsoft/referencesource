//---------------------------------------------------------------------
// <copyright file="RequestUriProcessor.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class capable of processing Astoria request Uris.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Client;
    using System.Data.Services.Parsing;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Linq;
    using System.Linq.Expressions;
    using System.Reflection;

    #endregion Namespaces.

    // Syntax for Astoria segments:
    // segment          ::= identifier [ query ]
    // query            ::= "(" key ")"
    // key              ::= keyvalue *["," keyvalue]
    // keyvalue         ::= *quotedvalue | *unquotedvalue
    // quotedvalue      ::= "'" *qvchar "'"
    // qvchar           ::= char-except-quote | "''"
    // unquotedvalue    ::= char

    /// <summary>
    /// Use this class to process a web data service request Uri.
    /// </summary>
    internal static class RequestUriProcessor
    {
        /// <summary>MethodInfo for the RequestUriProcessor.InvokeWhere method.</summary>
        private static readonly MethodInfo InvokeWhereMethodInfo = typeof(RequestUriProcessor).GetMethod("InvokeWhere", BindingFlags.NonPublic | BindingFlags.Static);

        /// <summary>Recursion limit on segment length.</summary>
        private const int RecursionLimit = 100;

        /// <summary>
        /// Parses the request Uri that the host is exposing and returns
        /// information about the intended results.
        /// </summary>
        /// <param name="absoluteRequestUri">Request uri that needs to get processed.</param>
        /// <param name="service">Data service for which the request is being processed.</param>
        /// <returns>
        /// An initialized RequestDescription instance describing what the
        /// request is for.
        /// </returns>
        /// <exception cref="DataServiceException">
        /// A <see cref="DataServiceException" /> with status code 404 (Not Found) is returned if an identifier
        /// in a segment cannot be resolved; 400 (Bad Request) is returned if a syntax
        /// error is found when processing a restriction (parenthesized text) or
        /// in the query portion.
        /// </exception>
        /// <remarks>
        /// Very important: no rights are checked on the last segment of the request.
        /// </remarks>
        internal static RequestDescription ProcessRequestUri(Uri absoluteRequestUri, IDataService service)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(absoluteRequestUri != null, "absoluteRequestUri != null");
            Debug.Assert(absoluteRequestUri.IsAbsoluteUri, "absoluteRequestUri.IsAbsoluteUri(" + absoluteRequestUri + ")");

            string[] segments = EnumerateSegments(absoluteRequestUri, service.OperationContext.AbsoluteServiceUri);
            if (segments.Length > RecursionLimit)
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestUriProcessor_TooManySegments);
            }

            SegmentInfo[] segmentInfos = CreateSegments(segments, service);
            SegmentInfo lastSegment = (segmentInfos.Length == 0) ? null : segmentInfos[segmentInfos.Length - 1];
            RequestTargetKind targetKind = (lastSegment == null) ? RequestTargetKind.ServiceDirectory : lastSegment.TargetKind;

            // Create a ResultDescription from the processed segments.
            RequestDescription resultDescription;
            Uri resultUri = GetResultUri(service.OperationContext);
            if (targetKind == RequestTargetKind.Metadata ||
                targetKind == RequestTargetKind.Batch ||
                targetKind == RequestTargetKind.ServiceDirectory)
            {
                resultDescription = new RequestDescription(targetKind, RequestTargetSource.None, resultUri);
            }
            else if (targetKind == RequestTargetKind.VoidServiceOperation)
            {
                Debug.Assert(lastSegment != null, "lastSegment != null");
                Debug.Assert(lastSegment.TargetSource == RequestTargetSource.ServiceOperation, "targetSource == RequestTargetSource.ServiceOperation");

                service.Provider.InvokeServiceOperation(lastSegment.Operation, lastSegment.OperationParameters);

                resultDescription = new RequestDescription(
                    RequestTargetKind.VoidServiceOperation,     // targetKind
                    RequestTargetSource.ServiceOperation,       // targetSource
                    resultUri);                                 // resultUri
            }
            else
            {
                Debug.Assert(lastSegment != null, "lastSegment != null");
                Debug.Assert(
                    targetKind == RequestTargetKind.ComplexObject ||
                    targetKind == RequestTargetKind.OpenProperty ||
                    targetKind == RequestTargetKind.OpenPropertyValue ||
                    targetKind == RequestTargetKind.Primitive ||
                    targetKind == RequestTargetKind.PrimitiveValue ||
                    targetKind == RequestTargetKind.Resource ||
                    targetKind == RequestTargetKind.MediaResource,
                    "Known targetKind " + targetKind);
                RequestTargetSource targetSource = lastSegment.TargetSource;
                ResourceProperty projectedProperty = lastSegment.ProjectedProperty;
                string containerName =
                    (lastSegment.TargetKind != RequestTargetKind.PrimitiveValue &&
                     lastSegment.TargetKind != RequestTargetKind.OpenPropertyValue &&
                     lastSegment.TargetKind != RequestTargetKind.MediaResource) ?
                     lastSegment.Identifier :
                     segmentInfos[segmentInfos.Length - 2].Identifier;
                bool usesContainerName =
                    (targetSource == RequestTargetSource.Property &&
                     projectedProperty != null &&
                     projectedProperty.Kind != ResourcePropertyKind.ResourceSetReference) ||
                    (targetSource == RequestTargetSource.ServiceOperation &&
                     lastSegment.Operation.ResultKind == ServiceOperationResultKind.QueryWithSingleResult);
                string mimeType =
                    (targetSource == RequestTargetSource.Property && projectedProperty != null) ? projectedProperty.MimeType :
                    (targetSource == RequestTargetSource.ServiceOperation) ? lastSegment.Operation.MimeType :
                    null;

                RequestQueryCountOption countOption = 
                    lastSegment.Identifier == XmlConstants.UriCountSegment ? 
                        RequestQueryCountOption.ValueOnly : 
                        RequestQueryCountOption.None;

                // Throw if $count and $inlinecount requests have been disabled by the user
                if (countOption != RequestQueryCountOption.None && !service.Configuration.DataServiceBehavior.AcceptCountRequests)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.DataServiceConfiguration_CountNotSupportedInV1Server);
                }

                resultDescription = new RequestDescription(
                    segmentInfos,
                    containerName,
                    usesContainerName,
                    mimeType,
                    resultUri)
                    {
                        CountOption = lastSegment.Identifier == XmlConstants.UriCountSegment ? RequestQueryCountOption.ValueOnly : RequestQueryCountOption.None
                    };

                // Update the feature version if the target set contains any type with FF KeepInContent=false.
                resultDescription.UpdateAndCheckEpmFeatureVersion(service);

                // Update the response DSV for GET if the target set contains any type with FF KeepInContent=false.
                //
                // Note the response DSV is payload specific and since for GET we won't know if the instances to be
                // serialized will contain any properties with FF KeepInContent=false until serialization time which
                // happens after the headers are written, the best we can do is to determin this at the set level.
                //
                // For CUD operations we'll raise the version based on the actual instances at deserialization time.
                if (service.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.GET)
                {
                    resultDescription.UpdateEpmResponseVersion(service.OperationContext.Host.RequestAccept, service.Provider);
                }
            }

            // Process query options ($filter, $orderby, $expand, etc.)
            resultDescription = RequestQueryProcessor.ProcessQuery(service, resultDescription);

            // $count, $inlinecount and $select are 2.0 features - raise the max version of used features accordingly
            if (resultDescription.CountOption != RequestQueryCountOption.None ||
                (resultDescription.RootProjectionNode != null && resultDescription.RootProjectionNode.ProjectionsSpecified))
            {
                resultDescription.RaiseFeatureVersion(2, 0, service.Configuration);
            }

            return resultDescription;
        }
        
        /// <summary>Appends a segment with the specified escaped <paramref name='text' />.</summary>
        /// <param name='uri'>URI to append to.</param>
        /// <param name='text'>Segment text, already escaped.</param>
        /// <returns>A new URI with a new segment escaped.</returns>
        internal static Uri AppendEscapedSegment(Uri uri, string text)
        {
            Debug.Assert(uri != null, "uri != null");
            Debug.Assert(text != null, "text != null");

            UriBuilder builder = new UriBuilder(uri);
            if (!builder.Path.EndsWith("/", StringComparison.Ordinal))
            {
                builder.Path += "/";
            }

            builder.Path += text;
            return builder.Uri;
        }

        /// <summary>Appends a segment with the specified unescaped <paramref name='text' />.</summary>
        /// <param name='uri'>URI to append to.</param>
        /// <param name='text'>Segment text, already escaped.</param>
        /// <returns>A new URI with a new segment escaped.</returns>
        internal static Uri AppendUnescapedSegment(Uri uri, string text)
        {
            Debug.Assert(uri != null, "uri != null");
            Debug.Assert(text != null, "text != null");
            return AppendEscapedSegment(uri, Uri.EscapeDataString(text));
        }

        /// <summary>Gets the absolute URI that a reference (typically from a POST or PUT body) points to.</summary>
        /// <param name="reference">Textual, URI-encoded reference.</param>
        /// <param name="operationContext">Context for current operation.</param>
        /// <returns>The absolute URI that <paramref name="reference"/> resolves to.</returns>
        internal static Uri GetAbsoluteUriFromReference(string reference, DataServiceOperationContext operationContext)
        {
            return GetAbsoluteUriFromReference(reference, operationContext.AbsoluteServiceUri);
        }

        /// <summary>Gets the absolute URI that a reference (typically from a POST or PUT body) points to.</summary>
        /// <param name="reference">Textual, URI-encoded reference.</param>
        /// <param name="absoluteServiceUri">Absolure URI for service, used to validate that the URI points within.</param>
        /// <returns>The absolute URI that <paramref name="reference"/> resolves to.</returns>
        internal static Uri GetAbsoluteUriFromReference(string reference, Uri absoluteServiceUri)
        {
            Debug.Assert(!String.IsNullOrEmpty(reference), "!String.IsNullOrEmpty(reference) -- caller should check and throw appropriate message");
            Debug.Assert(absoluteServiceUri != null, "absoluteServiceUri != null");
            Debug.Assert(absoluteServiceUri.IsAbsoluteUri, "absoluteServiceUri.IsAbsoluteUri(" + absoluteServiceUri + ")");

            Uri referenceAsUri = new Uri(reference, UriKind.RelativeOrAbsolute);
            if (!referenceAsUri.IsAbsoluteUri)
            {
                string slash = String.Empty;
                if (absoluteServiceUri.OriginalString.EndsWith("/", StringComparison.Ordinal))
                {
                    if (reference.StartsWith("/", StringComparison.Ordinal))
                    {
                        reference = reference.Substring(1, reference.Length - 1);
                    }
                }
                else if (!reference.StartsWith("/", StringComparison.Ordinal))
                {
                    slash = "/";
                }

                referenceAsUri = new Uri(absoluteServiceUri.OriginalString + slash + reference);
            }

            if (!UriUtil.UriInvariantInsensitiveIsBaseOf(absoluteServiceUri, referenceAsUri))
            {
                string message = Strings.BadRequest_RequestUriDoesNotHaveTheRightBaseUri(referenceAsUri, absoluteServiceUri);
                throw DataServiceException.CreateBadRequestError(message);
            }

            Debug.Assert(
                referenceAsUri.IsAbsoluteUri, 
                "referenceAsUri.IsAbsoluteUri(" + referenceAsUri + ") - otherwise composition from absolute yielded relative");

            return referenceAsUri;
        }

        /// <summary>Gets the specified <paramref name="uri"/> as a string suitable for an HTTP request.</summary>
        /// <param name="uri"><see cref="Uri"/> to get string for.</param>
        /// <returns>A string suitable for an HTTP request.</returns>
        internal static string GetHttpRequestUrl(Uri uri)
        {
            Debug.Assert(uri != null, "uri != null");
            if (uri.IsAbsoluteUri)
            {
                return uri.GetComponents(UriComponents.HttpRequestUrl, UriFormat.UriEscaped);
            }
            else
            {
                return uri.OriginalString;
            }
        }

        /// <summary>Gets the URI to the results, without the query component.</summary>
        /// <param name="operationContext">OperationContext with request information.</param>
        /// <returns>The URI to the results, without the query component.</returns>
        internal static Uri GetResultUri(DataServiceOperationContext operationContext)
        {
            Debug.Assert(operationContext != null, "operationContext != null");
            Uri requestUri = operationContext.AbsoluteRequestUri;
            UriBuilder resultBuilder = new UriBuilder(requestUri);
            resultBuilder.Query = null;

            // This is fix for bug 565322.
            // Since we don't allow uri to compose on collections, () must be present
            // as the last thing in the uri, if present. We need to remove the () from
            // the uri, since its a optional thing and we want to return a canonical
            // uri from the server.
            if (resultBuilder.Path.EndsWith("()", StringComparison.Ordinal))
            {
                resultBuilder.Path = resultBuilder.Path.Substring(0, resultBuilder.Path.Length - 2);
            }

            return resultBuilder.Uri;
        }

        /// <summary>
        /// Returns an object that can enumerate the segments in the specified path (eg: /foo/bar -&gt; foo, bar).
        /// </summary>
        /// <param name="absoluteRequestUri">A valid path portion of an uri.</param>
        /// <param name="baseUri">baseUri for the request that is getting processed.</param>
        /// <returns>An enumerable object of unescaped segments.</returns>
        internal static string[] EnumerateSegments(Uri absoluteRequestUri, Uri baseUri)
        {
            Debug.Assert(absoluteRequestUri != null, "absoluteRequestUri != null");
            Debug.Assert(absoluteRequestUri.IsAbsoluteUri, "absoluteRequestUri.IsAbsoluteUri(" + absoluteRequestUri.IsAbsoluteUri + ")");
            Debug.Assert(baseUri != null, "baseUri != null");
            Debug.Assert(baseUri.IsAbsoluteUri, "baseUri.IsAbsoluteUri(" + baseUri + ")");

            if (!UriUtil.UriInvariantInsensitiveIsBaseOf(baseUri, absoluteRequestUri))
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_RequestUriDoesNotHaveTheRightBaseUri(absoluteRequestUri, baseUri));
            }

            try
            {
                // 
                Uri uri = absoluteRequestUri;
                int numberOfSegmentsToSkip = 0;

                // Since there is a svc part in the segment, we will need to skip 2 segments
                numberOfSegmentsToSkip = baseUri.Segments.Length;

                string[] uriSegments = uri.Segments;
                int populatedSegmentCount = 0;
                for (int i = numberOfSegmentsToSkip; i < uriSegments.Length; i++)
                {
                    string segment = uriSegments[i];
                    if (segment.Length != 0 && segment != "/")
                    {
                        populatedSegmentCount++;
                    }
                }

                string[] segments = new string[populatedSegmentCount];
                int segmentIndex = 0;
                for (int i = numberOfSegmentsToSkip; i < uriSegments.Length; i++)
                {
                    string segment = uriSegments[i];
                    if (segment.Length != 0 && segment != "/")
                    {
                        if (segment[segment.Length - 1] == '/')
                        {
                            segment = segment.Substring(0, segment.Length - 1);
                        }

                        segments[segmentIndex++] = Uri.UnescapeDataString(segment);
                    }
                }

                Debug.Assert(segmentIndex == segments.Length, "segmentIndex == segments.Length -- otherwise we mis-counted populated/skipped segments.");
                return segments;
            }
            catch (UriFormatException)
            {
                throw DataServiceException.CreateSyntaxError();
            }
        }

        /// <summary>
        /// Given the uri, extract the key values from the uri
        /// </summary>
        /// <param name="absoluteRequestUri">uri from which the key values needs to be extracted</param>
        /// <param name="service">Data context for which the request is being processed.</param>
        /// <param name="containerName">returns the name of the source resource set name</param>
        /// <returns>key values as specified in the uri</returns>
        internal static KeyInstance ExtractKeyValuesFromUri(Uri absoluteRequestUri, IDataService service, out string containerName)
        {
            Debug.Assert(absoluteRequestUri != null, "absoluteRequestUri != null");
            Debug.Assert(absoluteRequestUri.IsAbsoluteUri, "absoluteRequestUri.IsAbsoluteUri(" + absoluteRequestUri + ")");
            Debug.Assert(service != null, "service != null");

            RequestDescription description = RequestUriProcessor.ProcessRequestUri(absoluteRequestUri, service);

            // 


            if (description.TargetKind != RequestTargetKind.Resource || !description.IsSingleResult || description.TargetSource != RequestTargetSource.EntitySet)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_MustSpecifyCanonicalUriInPayload(absoluteRequestUri));
            }

            // Dereferencing an object by specifying its key values implied the right to read it.
            Debug.Assert(description.LastSegmentInfo.SingleResult, "description.LastSegmentInfo.SingleResult");
            DataServiceConfiguration.CheckResourceRightsForRead(description.LastSegmentInfo.TargetContainer, true /* singleResult */);

            containerName = description.ContainerName;

            Debug.Assert(description.LastSegmentInfo.Key != null && !description.LastSegmentInfo.Key.IsEmpty, "Key Must be specified");
            return description.LastSegmentInfo.Key;
        }

        /// <summary>Invokes Queryable.Select for the specified query and selector.</summary>
        /// <param name="query">Query to invoke .Select method on.</param>
        /// <param name="projectedType">Type that will be projected out.</param>
        /// <param name="selector">Selector lambda expression.</param>
        /// <returns>The resulting query.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static IQueryable InvokeSelectForTypes(IQueryable query, Type projectedType, LambdaExpression selector)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(projectedType != null, "projectedType != null");
            Debug.Assert(selector != null, "selector != null");
            MethodInfo method = typeof(RequestUriProcessor).GetMethod("InvokeSelect", BindingFlags.Static | BindingFlags.NonPublic);
            method = method.MakeGenericMethod(query.ElementType, projectedType);
            return (IQueryable)method.Invoke(null, new object[] { query, selector });
        }

        /// <summary>Invokes Queryable.Where for the specified query and predicate.</summary>
        /// <param name="query">Query to invoke .Where method on.</param>
        /// <param name="predicate">Predicate to pass as argument.</param>
        /// <returns>The resulting query.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal static IQueryable InvokeWhereForType(IQueryable query, LambdaExpression predicate)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(predicate != null, "predicate != null");
            MethodInfo whereMethod = InvokeWhereMethodInfo.MakeGenericMethod(query.ElementType);
            return (IQueryable)whereMethod.Invoke(null, new object[] { query, predicate });
        }

        /// <summary>Composes the filter portion of a segment onto the specifies query.</summary>
        /// <param name="filter">Filter portion of segment, possibly null.</param>
        /// <param name="segment">Segment on which to compose.</param>
        private static void ComposeQuery(string filter, SegmentInfo segment)
        {
            Debug.Assert(filter != null, "filter != null");
            Debug.Assert(segment != null, "segment!= null");
            Debug.Assert(segment.SingleResult == false, "segment.SingleResult == false");

            ResourceType resourceType = segment.TargetResourceType;
            segment.Key = ExtractKeyValues(resourceType, filter);
            segment.SingleResult = !segment.Key.IsEmpty;
            if (segment.SingleResult)
            {
                if (!segment.Key.AreValuesNamed && segment.Key.ValueCount > 1)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.RequestUriProcessor_KeysMustBeNamed);
                }

                segment.RequestQueryable = SelectResourceByKey(segment.RequestQueryable, resourceType, segment.Key);
                segment.SingleResult = true;
            }
        }

        /// <summary>Creates the first <see cref="SegmentInfo"/> for a request.</summary>
        /// <param name="service">Service for which the request is being processed.</param>
        /// <param name="identifier">Identifier portion of segment.</param>
        /// <param name="checkRights">Whether rights should be checked on this segment.</param>
        /// <param name="queryPortion">Query portion with key; possibly null.</param>
        /// <param name="isLastSegment">True if the first segment is also the last segment.</param>
        /// <param name="crossReferencingUrl">whether this segment references some other segment.</param>
        /// <returns>A description of the information on the segment.</returns>
        private static SegmentInfo CreateFirstSegment(IDataService service, string identifier, bool checkRights, string queryPortion, bool isLastSegment, out bool crossReferencingUrl)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(identifier != null, "identifier != null");

            crossReferencingUrl = false;
            SegmentInfo segment = new SegmentInfo();
            segment.Identifier = identifier;

            // Look for well-known system entry points.
            if (segment.Identifier == XmlConstants.UriMetadataSegment)
            {
                WebUtil.CheckSyntaxValid(queryPortion == null);
                segment.TargetKind = RequestTargetKind.Metadata;
                return segment;
            }

            if (segment.Identifier == XmlConstants.UriBatchSegment)
            {
                WebUtil.CheckSyntaxValid(queryPortion == null);
                segment.TargetKind = RequestTargetKind.Batch;
                return segment;
            }

            if (segment.Identifier == XmlConstants.UriCountSegment)
            {
                // $count on root: throw
                throw DataServiceException.CreateResourceNotFound(Strings.RequestUriProcessor_CountOnRoot);
            }

            // Look for a service operation.
            segment.Operation = service.Provider.TryResolveServiceOperation(segment.Identifier);
            if (segment.Operation != null)
            {
                WebUtil.DebugEnumIsDefined(segment.Operation.ResultKind);
                segment.TargetSource = RequestTargetSource.ServiceOperation;
                if (service.OperationContext.RequestMethod != segment.Operation.Method)
                {
                    throw DataServiceException.CreateMethodNotAllowed(Strings.RequestUriProcessor_MethodNotAllowed, segment.Operation.Method);
                }

                segment.TargetContainer = segment.Operation.ResourceSet;

                if (segment.Operation.ResultKind != ServiceOperationResultKind.Void)
                {
                    segment.TargetResourceType = segment.Operation.ResultType;
                }
                else
                {
                    segment.TargetResourceType = null;
                }

                segment.OperationParameters = ReadOperationParameters(service.OperationContext.Host, segment.Operation);
                switch (segment.Operation.ResultKind)
                {
                    case ServiceOperationResultKind.QueryWithMultipleResults:
                    case ServiceOperationResultKind.QueryWithSingleResult:
                        try
                        {
                            segment.RequestQueryable = (IQueryable)service.Provider.InvokeServiceOperation(segment.Operation, segment.OperationParameters);
                        }
                        catch (TargetInvocationException exception)
                        {
                            ErrorHandler.HandleTargetInvocationException(exception);
                            throw;
                        }

                        WebUtil.CheckResourceExists(segment.RequestQueryable != null, segment.Identifier);
                        segment.SingleResult = (segment.Operation.ResultKind == ServiceOperationResultKind.QueryWithSingleResult);
                        break;

                    case ServiceOperationResultKind.DirectValue:
                    case ServiceOperationResultKind.Enumeration:
                        object methodResult;
                        try
                        {
                            methodResult = service.Provider.InvokeServiceOperation(segment.Operation, segment.OperationParameters);
                        }
                        catch (TargetInvocationException exception)
                        {
                            ErrorHandler.HandleTargetInvocationException(exception);
                            throw;
                        }

                        segment.SingleResult = (segment.Operation.ResultKind == ServiceOperationResultKind.DirectValue);
                        WebUtil.CheckResourceExists(segment.SingleResult || methodResult != null, segment.Identifier);   // Enumerations shouldn't be null.
                        segment.RequestEnumerable = segment.SingleResult ? new object[1] { methodResult } : (IEnumerable)methodResult;
                        segment.TargetResourceType = segment.Operation.ResultType;
                        segment.TargetKind = TargetKindFromType(segment.TargetResourceType);
                        WebUtil.CheckSyntaxValid(queryPortion == null);
                        RequestQueryProcessor.CheckEmptyQueryArguments(service, false /*checkForOnlyV2QueryParameters*/);
                        break;

                    default:
                        Debug.Assert(segment.Operation.ResultKind == ServiceOperationResultKind.Void, "segment.Operation.ResultKind == ServiceOperationResultKind.Nothing");
                        segment.TargetKind = RequestTargetKind.VoidServiceOperation;
                        break;
                }

                if (segment.RequestQueryable != null)
                {
                    segment.TargetKind = TargetKindFromType(segment.TargetResourceType);
                    if (queryPortion != null)
                    {
                        WebUtil.CheckSyntaxValid(!segment.SingleResult);
                        ComposeQuery(queryPortion, segment);
                    }
                }

                if (checkRights)
                {
                    DataServiceConfiguration.CheckServiceRights(segment.Operation, segment.SingleResult);
                }

                return segment;
            }

            SegmentInfo newSegmentInfo = service.GetSegmentForContentId(segment.Identifier);
            if (newSegmentInfo != null)
            {
                newSegmentInfo.Identifier = segment.Identifier;
                crossReferencingUrl = true;
                return newSegmentInfo;
            }

            // Look for an entity set.
            ResourceSetWrapper container = service.Provider.TryResolveResourceSet(segment.Identifier);
            WebUtil.CheckResourceExists(container != null, segment.Identifier);

            if (ShouldRequestQuery(service, isLastSegment, false, queryPortion))
            {
#if DEBUG
                segment.RequestQueryable = service.Provider.GetQueryRootForResourceSet(container, service);
#else
                segment.RequestQueryable = service.Provider.GetQueryRootForResourceSet(container);
#endif
                WebUtil.CheckResourceExists(segment.RequestQueryable != null, segment.Identifier);
            }

            segment.TargetContainer = container;
            segment.TargetResourceType = container.ResourceType;
            segment.TargetSource = RequestTargetSource.EntitySet;
            segment.TargetKind = RequestTargetKind.Resource;
            segment.SingleResult = false;
            if (queryPortion != null)
            {
                ComposeQuery(queryPortion, segment);
            }

            if (checkRights)
            {
                DataServiceConfiguration.CheckResourceRightsForRead(container, segment.SingleResult);
            }

            if (segment.RequestQueryable != null)
            {
                // We only need to invoke the query interceptor if we called get query root.
                segment.RequestQueryable = DataServiceConfiguration.ComposeResourceContainer(service, container, segment.RequestQueryable);
            }

            return segment;
        }

        /// <summary>Whether a query should be requested and composed with interceptors for a segment.</summary>
        /// <param name="service">Service under which request is being analyzed.</param>
        /// <param name="isLastSegment">Whether this is the last segment of the URI.</param>
        /// <param name="isAfterLink">Is the current segment being checked after a $links segment.</param>
        /// <param name="queryPortion">Query portion of the segment.</param>
        /// <returns>true if the segments should be read and composed with interceptors; false otherwise.</returns>
        /// <remarks>
        /// For V1 providers we always get the query root or else we introduce a breaking change.
        /// If this is an insert operation and the current segment is the first and last segment,
        /// we don't need to get the query root as we won't even invoke the query.
        /// Note that we need to make sure we only skip the query root if the query portion is null, this
        /// is because in the deep insert case, we can be doing a binding to a single entity and we would
        /// need the query root for that entity.
        /// We shall also skip requesting the query if the request is for an update on $links for non-V1 providers.
        /// </remarks>
        private static bool ShouldRequestQuery(IDataService service, bool isLastSegment, bool isAfterLink, string queryPortion)
        {
            Debug.Assert(service != null, "service != null");

            if (service.Provider.IsV1Provider)
            {
                return true;
            }

            AstoriaVerbs verbUsed = service.OperationContext.Host.AstoriaHttpVerb;
            bool isPostQueryForSet = isLastSegment && verbUsed == AstoriaVerbs.POST && string.IsNullOrEmpty(queryPortion);
            bool isUpdateQueryForLinks = isAfterLink && (verbUsed == AstoriaVerbs.PUT || verbUsed == AstoriaVerbs.MERGE);

            return !(isPostQueryForSet || isUpdateQueryForLinks);
        }

        /// <summary>Creates a <see cref="SegmentInfo"/> array for the given <paramref name="segments"/>.</summary>
        /// <param name="segments">Segments to process.</param>
        /// <param name="service">Service for which segments are being processed.</param>
        /// <returns>Segment information describing the given <paramref name="segments"/>.</returns>
        private static SegmentInfo[] CreateSegments(string[] segments, IDataService service)
        {
            Debug.Assert(segments != null, "segments != null");
            Debug.Assert(service != null, "service != null");
            SegmentInfo previous = null;
            SegmentInfo[] segmentInfos = new SegmentInfo[segments.Length];
            bool crossReferencingUri = false;
            bool postLinkSegment = false;
            for (int i = 0; i < segments.Length; i++)
            {
                string segmentText = segments[i];
                bool checkRights = (i != segments.Length - 1);  // Whether rights should be checked on this segment.
                string identifier;
                bool hasQuery = ExtractSegmentIdentifier(segmentText, out identifier);
                string queryPortion = hasQuery ? segmentText.Substring(identifier.Length) : null;
                SegmentInfo segment;

                // We allow a single trailing '/', which results in an empty segment.
                // However System.Uri removes it, so any empty segment we see is a 404 error.
                if (identifier.Length == 0)
                {
                    throw DataServiceException.ResourceNotFoundError(Strings.RequestUriProcessor_EmptySegmentInRequestUrl);
                }

                if (previous == null)
                {
                    segment = CreateFirstSegment(service, identifier, checkRights, queryPortion, segments.Length == 1, out crossReferencingUri);
                }
                else if (previous.TargetKind == RequestTargetKind.Batch ||
                         previous.TargetKind == RequestTargetKind.Metadata ||
                         previous.TargetKind == RequestTargetKind.PrimitiveValue ||
                         previous.TargetKind == RequestTargetKind.VoidServiceOperation ||
                         previous.TargetKind == RequestTargetKind.OpenPropertyValue ||
                         previous.TargetKind == RequestTargetKind.MediaResource)
                {
                    // Nothing can come after a $metadata, $value or $batch segment.
                    // Nothing can come after a service operation with void return type.
                    throw DataServiceException.ResourceNotFoundError(Strings.RequestUriProcessor_MustBeLeafSegment(previous.Identifier));
                }
                else if (postLinkSegment && identifier != XmlConstants.UriCountSegment)
                {
                    // DEVNOTE(Microsoft): [Resources]/$links/[Property]/$count is valid
                    throw DataServiceException.ResourceNotFoundError(Strings.RequestUriProcessor_CannotSpecifyAfterPostLinkSegment(identifier, XmlConstants.UriLinkSegment));
                }
                else if (previous.TargetKind == RequestTargetKind.Primitive)
                {
                    // $value is the well-known identifier to return a primitive property
                    // in its natural MIME format.
                    if (identifier != XmlConstants.UriValueSegment)
                    {
                        throw DataServiceException.ResourceNotFoundError(Strings.RequestUriProcessor_ValueSegmentAfterScalarPropertySegment(previous.Identifier, identifier));
                    }

                    WebUtil.CheckSyntaxValid(!hasQuery);
                    segment = new SegmentInfo(previous);
                    segment.Identifier = identifier;
                    segment.SingleResult = true;
                    segment.TargetKind = RequestTargetKind.PrimitiveValue;
                }
                else if (previous.TargetKind == RequestTargetKind.Resource &&
                          previous.SingleResult &&
                          identifier == XmlConstants.UriLinkSegment)
                {
                    segment = new SegmentInfo(previous);
                    segment.Identifier = identifier;
                    segment.TargetKind = RequestTargetKind.Link;
                }
                else
                {
                    Debug.Assert(
                        previous.TargetKind == RequestTargetKind.ComplexObject ||
                        previous.TargetKind == RequestTargetKind.Resource ||
                        previous.TargetKind == RequestTargetKind.OpenProperty ||
                        previous.TargetKind == RequestTargetKind.Link,
                        "previous.TargetKind(" + previous.TargetKind + ") can have properties");

                    postLinkSegment = (previous.TargetKind == RequestTargetKind.Link);

                    // Enumerable and DirectValue results cannot be composed at all.
                    if (previous.Operation != null && (previous.Operation.ResultKind == ServiceOperationResultKind.Enumeration || previous.Operation.ResultKind == ServiceOperationResultKind.DirectValue))
                    {
                        throw DataServiceException.ResourceNotFoundError(
                            Strings.RequestUriProcessor_IEnumerableServiceOperationsCannotBeFurtherComposed(previous.Identifier));
                    }

                    if (!previous.SingleResult && identifier != XmlConstants.UriCountSegment)
                    {
                        // $count can be applied to a collection
                        throw DataServiceException.CreateBadRequestError(
                            Strings.RequestUriProcessor_CannotQueryCollections(previous.Identifier));
                    }

                    segment = new SegmentInfo();
                    segment.Identifier = identifier;
                    segment.TargetSource = RequestTargetSource.Property;

                    // A segment will correspond to a property in the object model;
                    // if we are processing an open type, anything further in the
                    // URI also represents an open type property.
                    if (previous.TargetResourceType == null)
                    {
                        Debug.Assert(previous.TargetKind == RequestTargetKind.OpenProperty, "For open properties, the target resource type must be null");
                        segment.ProjectedProperty = null;
                    }
                    else
                    {
                        Debug.Assert(previous.TargetKind != RequestTargetKind.OpenProperty, "Since the query element type is known, this can't be open property");
                        Debug.Assert(previous.TargetResourceType != null, "Previous wasn't open, so it should have a resource type");
                        segment.ProjectedProperty = previous.TargetResourceType.TryResolvePropertyName(identifier);
                    }

                    if (identifier == XmlConstants.UriCountSegment)
                    {
                        if (previous.TargetKind != RequestTargetKind.Resource)
                        {
                            throw DataServiceException.CreateResourceNotFound(Strings.RequestUriProcessor_CountNotSupported(previous.Identifier));
                        }

                        if (previous.SingleResult)
                        {
                            throw DataServiceException.CreateResourceNotFound(Strings.RequestUriProcessor_CannotQuerySingletons(previous.Identifier, identifier));
                        }

                        if (service.OperationContext.Host.AstoriaHttpVerb != AstoriaVerbs.GET)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.RequestQueryProcessor_RequestVerbCannotCountError);
                        }

                        segment.RequestEnumerable = previous.RequestEnumerable;
                        segment.SingleResult = true;
                        segment.TargetKind = RequestTargetKind.PrimitiveValue;
                        segment.TargetResourceType = previous.TargetResourceType;
                        segment.TargetContainer = previous.TargetContainer;
                    }
                    else if (identifier == XmlConstants.UriValueSegment &&
                        (previous.TargetKind == RequestTargetKind.OpenProperty || (previous.TargetKind == RequestTargetKind.Resource)))
                    {
                        segment.RequestEnumerable = previous.RequestEnumerable;
                        segment.SingleResult = true;
                        segment.TargetResourceType = previous.TargetResourceType;

                        if (previous.TargetKind == RequestTargetKind.OpenProperty)
                        {
                            segment.TargetKind = RequestTargetKind.OpenPropertyValue;
                        }
                        else
                        {
                            // If the previous segment is an entity, we expect it to be an MLE. We cannot validate our assumption
                            // until later when we get the actual instance of the entity because the type hierarchy can contain
                            // a mix of MLE and non-MLE types.
                            segment.TargetKind = RequestTargetKind.MediaResource;

                            // There is no valid query option for Media Resource.
                            RequestQueryProcessor.CheckEmptyQueryArguments(service, false /*checkForOnlyV2QueryParameters*/);
                        }
                    }
                    else if (segment.ProjectedProperty == null)
                    {
                        // Handle an open type property. If the current leaf isn't an 
                        // object (which implies it's already an open type), then
                        // it should be marked as an open type.
                        if (previous.TargetResourceType != null)
                        {
                            WebUtil.CheckResourceExists(previous.TargetResourceType.IsOpenType, segment.Identifier);
                        }

                        // Open navigation properties are not supported on OpenTypes.  We should throw on the following cases:
                        // 1. the segment after $links is always a navigation property pointing to a resource
                        // 2. if the segment hasQuery, it is pointing to a resource
                        // 3. if this is a POST operation, the target has to be either a set of links or an entity set
                        if (previous.TargetKind == RequestTargetKind.Link || hasQuery || service.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.POST)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(segment.Identifier));
                        }

                        segment.TargetResourceType = null;
                        segment.TargetKind = RequestTargetKind.OpenProperty;
                        segment.SingleResult = true;

                        if (!crossReferencingUri)
                        {
                            segment.RequestQueryable = SelectOpenProperty(previous.RequestQueryable, identifier);
                        }
                    }
                    else
                    {
                        // Handle a strongly-typed property.
                        segment.TargetResourceType = segment.ProjectedProperty.ResourceType;
                        ResourcePropertyKind propertyKind = segment.ProjectedProperty.Kind;
                        segment.SingleResult = (propertyKind != ResourcePropertyKind.ResourceSetReference);
                        if (!crossReferencingUri)
                        {
                            if (segment.ProjectedProperty.CanReflectOnInstanceTypeProperty)
                            {
                                segment.RequestQueryable = segment.SingleResult ?
                                    SelectElement(previous.RequestQueryable, segment.ProjectedProperty) :
                                    SelectMultiple(previous.RequestQueryable, segment.ProjectedProperty);
                            }
                            else
                            {
                                segment.RequestQueryable = segment.SingleResult ?
                                    SelectLateBoundProperty(
                                        previous.RequestQueryable,
                                        segment.ProjectedProperty) :
                                    SelectLateBoundPropertyMultiple(
                                        previous.RequestQueryable,
                                        segment.ProjectedProperty);
                            }
                        }

                        if (previous.TargetKind == RequestTargetKind.Link &&
                            segment.ProjectedProperty.TypeKind != ResourceTypeKind.EntityType)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.RequestUriProcessor_LinkSegmentMustBeFollowedByEntitySegment(identifier, XmlConstants.UriLinkSegment));
                        }

                        switch (propertyKind)
                        {
                            case ResourcePropertyKind.ComplexType:
                                segment.TargetKind = RequestTargetKind.ComplexObject;
                                break;
                            case ResourcePropertyKind.ResourceReference:
                            case ResourcePropertyKind.ResourceSetReference:
                                segment.TargetKind = RequestTargetKind.Resource;
                                segment.TargetContainer = service.Provider.GetContainer(previous.TargetContainer, previous.TargetResourceType, segment.ProjectedProperty);
                                if (segment.TargetContainer == null)
                                {
                                    throw DataServiceException.CreateResourceNotFound(segment.ProjectedProperty.Name);
                                }

                                break;
                            default:
                                Debug.Assert(segment.ProjectedProperty.IsOfKind(ResourcePropertyKind.Primitive), "must be primitive type property");
                                segment.TargetKind = RequestTargetKind.Primitive;
                                break;
                        }

                        if (hasQuery)
                        {
                            WebUtil.CheckSyntaxValid(!segment.SingleResult);
                            if (!crossReferencingUri)
                            {
                                ComposeQuery(queryPortion, segment);
                            }
                            else
                            {
                                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ResourceCanBeCrossReferencedOnlyForBindOperation);
                            }
                        }

                        // Do security checks and authorization query composition.
                        if (segment.TargetContainer != null)
                        {
                            if (checkRights)
                            {
                                DataServiceConfiguration.CheckResourceRightsForRead(segment.TargetContainer, segment.SingleResult);
                            }

                            if (!crossReferencingUri && ShouldRequestQuery(service, i == segments.Length - 1, postLinkSegment, queryPortion))
                            {
                                segment.RequestQueryable = DataServiceConfiguration.ComposeResourceContainer(service, segment.TargetContainer, segment.RequestQueryable);
                            }
                        }
                    }
                }

#if DEBUG
                segment.AssertValid();
#endif
                segmentInfos[i] = segment;
                previous = segment;
            }

            if (segments.Length != 0 && previous.TargetKind == RequestTargetKind.Link)
            {
                throw DataServiceException.CreateBadRequestError(Strings.RequestUriProcessor_MissingSegmentAfterLink(XmlConstants.UriLinkSegment));
            }

            return segmentInfos;
        }

        /// <summary>Returns an object that can enumerate key values.</summary>
        /// <param name="resourceType">resource type whose keys need to be extracted</param>
        /// <param name="filter">Key (query) part of an Astoria segment.</param>
        /// <returns>An object that can enumerate key values.</returns>
        private static KeyInstance ExtractKeyValues(ResourceType resourceType, string filter)
        {
            KeyInstance key;
            filter = RemoveFilterParens(filter);
            WebUtil.CheckSyntaxValid(KeyInstance.TryParseKeysFromUri(filter, out key));

            if (!key.IsEmpty)
            {
                // Make sure the keys specified in the uri matches with the number of keys in the metadata
                if (resourceType.KeyProperties.Count != key.ValueCount)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_KeyCountMismatch(resourceType.FullName));
                }

                WebUtil.CheckSyntaxValid(key.TryConvertValues(resourceType));
            }

            return key;
        }

        /// <summary>Extracts the identifier part of the unescaped Astoria segment.</summary>
        /// <param name="segment">Unescaped Astoria segment.</param>
        /// <param name="identifier">On returning, the identifier in the segment.</param>
        /// <returns>true if keys follow the identifier.</returns>
        private static bool ExtractSegmentIdentifier(string segment, out string identifier)
        {
            Debug.Assert(segment != null, "segment != null");

            int filterStart = 0;
            while (filterStart < segment.Length && segment[filterStart] != '(')
            {
                filterStart++;
            }

            identifier = segment.Substring(0, filterStart);
            return filterStart < segment.Length;
        }

        /// <summary>Generic method to invoke a Select method on an IQueryable source.</summary>
        /// <typeparam name="TSource">Element type of the source.</typeparam>
        /// <typeparam name="TResult">Result type of the projection.</typeparam>
        /// <param name="source">Source query.</param>
        /// <param name="selector">Lambda expression that turns TSource into TResult.</param>
        /// <returns>A new query that projects TSource into TResult.</returns>
        private static IQueryable InvokeSelect<TSource, TResult>(IQueryable source, LambdaExpression selector)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(selector != null, "selector != null");

            IQueryable<TSource> typedSource = (IQueryable<TSource>)source;
            Expression<Func<TSource, TResult>> typedSelector = (Expression<Func<TSource, TResult>>)selector;
            return Queryable.Select<TSource, TResult>(typedSource, typedSelector);
        }

        /// <summary>Generic method to invoke a SelectMany method on an IQueryable source.</summary>
        /// <typeparam name="TSource">Element type of the source.</typeparam>
        /// <typeparam name="TResult">Result type of the projection.</typeparam>
        /// <param name="source">Source query.</param>
        /// <param name="selector">Lambda expression that turns TSource into IEnumerable&lt;TResult&gt;.</param>
        /// <returns>A new query that projects TSource into IEnumerable&lt;TResult&gt;.</returns>
        private static IQueryable InvokeSelectMany<TSource, TResult>(IQueryable source, LambdaExpression selector)
        {
            Debug.Assert(source != null, "source != null");
            Debug.Assert(selector != null, "selector != null");

            IQueryable<TSource> typedSource = (IQueryable<TSource>)source;
            Expression<Func<TSource, IEnumerable<TResult>>> typedSelector = (Expression<Func<TSource, IEnumerable<TResult>>>)selector;
            return Queryable.SelectMany<TSource, TResult>(typedSource, typedSelector);
        }

        /// <summary>Generic method to invoke a Where method on an IQueryable source.</summary>
        /// <typeparam name="TSource">Element type of the source.</typeparam>
        /// <param name="query">Source query.</param>
        /// <param name="predicate">Lambda expression that filters the result of the query.</param>
        /// <returns>A new query that filters the query.</returns>
        private static IQueryable InvokeWhere<TSource>(IQueryable query, LambdaExpression predicate)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(predicate != null, "predicate != null");

            IQueryable<TSource> typedQueryable = (IQueryable<TSource>)query;
            
            // If the type of predicate is not TSource, we need to replace the parameter with
            // a downcasted parameter type if predicate's input is a base class of TSource.
            if (predicate.Parameters[0].Type != typeof(TSource) && 
                predicate.Parameters[0].Type.IsAssignableFrom(typeof(TSource)))
            {
                predicate = RequestUriProcessor.ReplaceParameterTypeForLambda(predicate, typeof(TSource));
            }

            Expression<Func<TSource, bool>> typedPredicate = (Expression<Func<TSource, bool>>)predicate;
            return Queryable.Where<TSource>(typedQueryable, typedPredicate);
        }

        /// <summary>Replaced the type of input parameter with the given <paramref name="targetType"/></summary>
        /// <param name="input">Input lambda expression.</param>
        /// <param name="targetType">Type of the new parameter that will be replaced.</param>
        /// <returns>New lambda expression with parameter of new type.</returns>
        private static LambdaExpression ReplaceParameterTypeForLambda(LambdaExpression input, Type targetType)
        {
            Debug.Assert(input.Parameters.Count == 1, "Assuming a single parameter for input lambda expression in this function.");
            
            ParameterExpression p = Expression.Parameter(targetType, input.Parameters[0].Name);
            
            return Expression.Lambda(
                    ParameterReplacerVisitor.Replace(input.Body, input.Parameters[0], p), 
                    p);
        }

        /// <summary>
        /// Reads the parameters for the specified <paramref name="operation"/> from the
        /// <paramref name="host"/>.
        /// </summary>
        /// <param name="host">Host with request information.</param>
        /// <param name="operation">Operation with parameters to be read.</param>
        /// <returns>A new object[] with parameter values.</returns>
        private static object[] ReadOperationParameters(DataServiceHostWrapper host, ServiceOperationWrapper operation)
        {
            Debug.Assert(host != null, "host != null");

            object[] operationParameters = new object[operation.Parameters.Count];
            for (int i = 0; i < operation.Parameters.Count; i++)
            {
                Type parameterType = operation.Parameters[i].ParameterType.InstanceType;
                string queryStringValue = host.GetQueryStringItem(operation.Parameters[i].Name);
                Type underlyingType = Nullable.GetUnderlyingType(parameterType);
                if (String.IsNullOrEmpty(queryStringValue))
                {
                    WebUtil.CheckSyntaxValid(parameterType.IsClass || underlyingType != null);
                    operationParameters[i] = null;
                }
                else
                {
                    // We choose to be a little more flexible than with keys and
                    // allow surrounding whitespace (which is never significant).
                    // See SQLBUDT #555944.
                    queryStringValue = queryStringValue.Trim();
                    Type targetType = underlyingType ?? parameterType;
                    if (WebConvert.IsKeyTypeQuoted(targetType))
                    {
                        WebUtil.CheckSyntaxValid(WebConvert.IsKeyValueQuoted(queryStringValue));
                    }

                    WebUtil.CheckSyntaxValid(WebConvert.TryKeyStringToPrimitive(queryStringValue, targetType, out operationParameters[i]));
                }
            }

            return operationParameters;
        }

        /// <summary>Removes the parens around the filter part of a query.</summary>
        /// <param name='filter'>Filter with parens included.</param>
        /// <returns>Filter without parens.</returns>
        private static string RemoveFilterParens(string filter)
        {
            Debug.Assert(filter != null, "filter != null");
            WebUtil.CheckSyntaxValid(filter.Length > 0 && filter[0] == '(' && filter[filter.Length - 1] == ')');
            return filter.Substring(1, filter.Length - 2);
        }

        /// <summary>Project a property with a single element out of the specified query.</summary>
        /// <param name="query">Base query to project from.</param>
        /// <param name="property">Property to project.</param>
        /// <returns>A query with a composed primitive property projection.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private static IQueryable SelectElement(IQueryable query, ResourceProperty property)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(property.Kind != ResourcePropertyKind.ResourceSetReference, "property != ResourcePropertyKind.ResourceSetReference");

            ParameterExpression parameter = Expression.Parameter(query.ElementType, "element");
            MemberExpression body = Expression.Property(parameter, property.Name);
            LambdaExpression selector = Expression.Lambda(body, parameter);

            MethodInfo method = typeof(RequestUriProcessor).GetMethod("InvokeSelect", BindingFlags.Static | BindingFlags.NonPublic);
            method = method.MakeGenericMethod(query.ElementType, property.Type);
            return (IQueryable)method.Invoke(null, new object[] { query, selector });
        }

        /// <summary>Project a property with multiple elements out of the specified query.</summary>
        /// <param name="query">Base query to project from.</param>
        /// <param name="property">Property to project.</param>
        /// <returns>A query with a composed primitive property projection.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private static IQueryable SelectMultiple(IQueryable query, ResourceProperty property)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(property.Kind == ResourcePropertyKind.ResourceSetReference, "property == ResourcePropertyKind.ResourceSetReference");

            Type enumerableElement = BaseServiceProvider.GetIEnumerableElement(property.Type);
            Debug.Assert(enumerableElement != null, "Providers should never expose a property as a resource-set if it doesn't implement IEnumerable`1.");

            ParameterExpression parameter = Expression.Parameter(query.ElementType, "element");
            UnaryExpression body = 
                Expression.ConvertChecked(
                    Expression.Property(parameter, property.Name),
                    typeof(IEnumerable<>).MakeGenericType(enumerableElement));
            LambdaExpression selector = Expression.Lambda(body, parameter);

            MethodInfo method = typeof(RequestUriProcessor).GetMethod("InvokeSelectMany", BindingFlags.Static | BindingFlags.NonPublic);
            method = method.MakeGenericMethod(query.ElementType, enumerableElement);
            return (IQueryable)method.Invoke(null, new object[] { query, selector });
        }

        /// <summary>Project a property with a single element out of the specified query over an late bound (possibily open) property.</summary>
        /// <param name="query">Base query to project from.</param>
        /// <param name="propertyName">Name of property to project.</param>
        /// <returns>A query with a composed property projection.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private static IQueryable SelectOpenProperty(IQueryable query, string propertyName)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(propertyName != null, "propertyName != null");

            ParameterExpression parameter = Expression.Parameter(query.ElementType, "element");
            Expression body = Expression.Call(null /* instance */, OpenTypeMethods.GetValueOpenPropertyMethodInfo, parameter, Expression.Constant(propertyName));

            LambdaExpression selector = Expression.Lambda(body, parameter);
            MethodInfo method = typeof(RequestUriProcessor).GetMethod("InvokeSelect", BindingFlags.Static | BindingFlags.NonPublic);
            method = method.MakeGenericMethod(query.ElementType, typeof(object));
            return (IQueryable)method.Invoke(null, new object[] { query, selector });
        }

        /// <summary>Project a property with a single element out of the specified query over an late bound (possibily open) property.</summary>
        /// <param name="query">Base query to project from.</param>
        /// <param name="property">Resource property containing the metadata for the late bound property.</param>
        /// <returns>A query with a composed property projection.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private static IQueryable SelectLateBoundProperty(IQueryable query, ResourceProperty property)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(property != null && !property.CanReflectOnInstanceTypeProperty, "property != null && !property.CanReflectOnInstanceTypeProperty");

            ParameterExpression parameter = Expression.Parameter(query.ElementType, "element");
            Expression body = Expression.Call(null /*instance*/, DataServiceProviderMethods.GetValueMethodInfo, parameter, Expression.Constant(property));
            body = Expression.Convert(body, property.Type);

            LambdaExpression selector = Expression.Lambda(body, parameter);
            MethodInfo method = typeof(RequestUriProcessor).GetMethod("InvokeSelect", BindingFlags.Static | BindingFlags.NonPublic);
            method = method.MakeGenericMethod(query.ElementType, property.Type);
            return (IQueryable)method.Invoke(null, new object[] { query, selector });
        }

        /// <summary>Project a property with a single element out of the specified query over an late bound (possibily open) property.</summary>
        /// <param name="query">Base query to project from.</param>
        /// <param name="property">Resource property containing the metadata for the late bound property.</param>
        /// <returns>A query with a composed property projection.</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        private static IQueryable SelectLateBoundPropertyMultiple(IQueryable query, ResourceProperty property)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(property != null && property.Kind == ResourcePropertyKind.ResourceSetReference && !property.CanReflectOnInstanceTypeProperty, "property != null && property.Kind == ResourcePropertyKind.ResourceSetReference && !property.CanReflectOnInstanceTypeProperty");

            Type enumerableElement = BaseServiceProvider.GetIEnumerableElement(property.Type);
            ParameterExpression parameter = Expression.Parameter(query.ElementType, "element");
            MethodInfo getter = DataServiceProviderMethods.GetSequenceValueMethodInfo.MakeGenericMethod(enumerableElement);
            Expression body = Expression.Call(null /*instance*/, getter, parameter, Expression.Constant(property));
            LambdaExpression selector = Expression.Lambda(body, parameter);

            MethodInfo method = typeof(RequestUriProcessor).GetMethod("InvokeSelectMany", BindingFlags.Static | BindingFlags.NonPublic);
            method = method.MakeGenericMethod(query.ElementType, enumerableElement);
            return (IQueryable)method.Invoke(null, new object[] { query, selector });
        }

        /// <summary>Selects a single resource by key values.</summary>
        /// <param name="query">Base query for resources</param>
        /// <param name="resourceType">resource type whose keys are specified</param>
        /// <param name="key">Key values for the given resource type.</param>
        /// <returns>A new query that selects the single resource that matches the specified key values.</returns>
        private static IQueryable SelectResourceByKey(IQueryable query, ResourceType resourceType, KeyInstance key)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(key != null && key.ValueCount != 0, "key != null && key.ValueCount != 0");
            Debug.Assert(resourceType.KeyProperties.Count == key.ValueCount, "resourceType.KeyProperties.Count == key.ValueCount");

            for (int i = 0; i < resourceType.KeyProperties.Count; i++)
            {
                ResourceProperty keyProperty = resourceType.KeyProperties[i];
                Debug.Assert(keyProperty.IsOfKind(ResourcePropertyKind.Key), "keyProperty.IsOfKind(ResourcePropertyKind.Key)");

                object keyValue;
                if (key.AreValuesNamed)
                {
                    keyValue = key.NamedValues[keyProperty.Name];
                }
                else
                {
                    keyValue = key.PositionalValues[i];
                }

                ParameterExpression parameter = Expression.Parameter(query.ElementType, "element");
                Expression e;
                if (keyProperty.CanReflectOnInstanceTypeProperty)
                {
                    e = Expression.Property(parameter, keyProperty.Name);
                }
                else
                {
                    e = Expression.Call(null /*instance*/, DataServiceProviderMethods.GetValueMethodInfo, parameter, Expression.Constant(keyProperty));
                    e = Expression.Convert(e, keyProperty.Type);
                }

                BinaryExpression body = Expression.Equal((Expression)e, Expression.Constant(keyValue));
                LambdaExpression predicate = Expression.Lambda(body, parameter);
                query = InvokeWhereForType(query, predicate);
            }

            return query;
        }

        /// <summary>Determines a matching target kind from the specified type.</summary>
        /// <param name="type">ResourceType of element to get kind for.</param>
        /// <returns>An appropriate <see cref="RequestTargetKind"/> for the specified <paramref name="type"/>.</returns>
        private static RequestTargetKind TargetKindFromType(ResourceType type)
        {
            Debug.Assert(type != null, "type != null");

            switch (type.ResourceTypeKind)
            {
                case ResourceTypeKind.ComplexType:
                    return RequestTargetKind.ComplexObject;
                case ResourceTypeKind.EntityType:
                    return RequestTargetKind.Resource;
                default:
                    Debug.Assert(type.ResourceTypeKind == ResourceTypeKind.Primitive, "typeKind == ResourceTypeKind.Primitive");
                    return RequestTargetKind.Primitive;
            }
        }
    }
}
