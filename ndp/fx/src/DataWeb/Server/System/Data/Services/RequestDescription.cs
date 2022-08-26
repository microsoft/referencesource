//---------------------------------------------------------------------
// <copyright file="RequestDescription.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a description of the request a client has submitted.
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
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Linq;

    #endregion Namespaces.
    
    /// <summary>
    /// Query Counting Option
    /// </summary>
    internal enum RequestQueryCountOption
    {
        /// <summary>Do not count the result set</summary>
        None,

        /// <summary>Count and return value inline (together with data)</summary>
        Inline,

        /// <summary>Count and return value only (as integer)</summary>
        ValueOnly
    }

    /// <summary>
    /// Use this class to describe the data request a client has
    /// submitted to the service.
    /// </summary>
    [DebuggerDisplay("RequestDescription={TargetSource} '{ContainerName}' -> {TargetKind} '{TargetResourceType}'")]
    internal class RequestDescription
    {
        /// <summary>
        /// The default response version of the data service. If no version is set for a particular response
        /// The DataService will respond with this version (1.0)
        /// </summary>
        internal static readonly Version DataServiceDefaultResponseVersion = new Version(1, 0);
        
        #region Private fields.

        /// <summary>
        /// Default set of known data service versions, currently 1.0 and 2.0
        /// </summary>
        private static readonly Version[] KnownDataServiceVersions = new Version[] { new Version(1, 0), new Version(2, 0) };

        /// <summary>The name of the container for results.</summary>
        private readonly string containerName;

        /// <summary>Root of the projection and expansion tree.</summary>
        /// <remarks>If this is null - no projections or expansions were part of the request.</remarks>
        private readonly RootProjectionNode rootProjectionNode;

        /// <summary>The MIME type for the requested resource, if specified.</summary>
        private readonly string mimeType;

        /// <summary>URI for the result (without the query component).</summary>
        private readonly Uri resultUri;

        /// <summary>SegmentInfo containing information about every segment in the uri</summary>
        private readonly SegmentInfo[] segmentInfos;

        /// <summary>Whether the container name should be used to name the result.</summary>
        private readonly bool usesContainerName;

        /// <summary>Query count option, whether to count the result set or not, and how</summary>
        private RequestQueryCountOption countOption;

        /// <summary>The value of the row count</summary>
        private long countValue;

        /// <summary>The minimum client version requirement</summary>
        private Version requireMinimumVersion;

        /// <summary>The server response version</summary>
        private Version responseVersion;

        /// <summary>
        /// Max version of features used in the request. We need to distinguish between feature versions and response version since 
        /// some new features (e.g. Server Projections) do not cause response version to be raised.
        /// </summary>
        private Version maxFeatureVersion;

        #endregion Private fields.
        
        /// <summary>
        /// Initializes a new RequestDescription for a query specified by the
        /// request Uri.
        /// </summary>
        /// <param name="targetKind">The kind of target for the request.</param>
        /// <param name="targetSource">The source for this target.</param>
        /// <param name="resultUri">URI to the results requested (with no query component).</param>
        internal RequestDescription(RequestTargetKind targetKind, RequestTargetSource targetSource, Uri resultUri)
        {
            WebUtil.DebugEnumIsDefined(targetKind);
            Debug.Assert(resultUri != null, "resultUri != null");
            Debug.Assert(resultUri.IsAbsoluteUri, "resultUri.IsAbsoluteUri(" + resultUri + ")");

            SegmentInfo segment = new SegmentInfo();
            segment.TargetKind = targetKind;
            segment.TargetSource = targetSource;
            segment.SingleResult = true;
            this.segmentInfos = new SegmentInfo[] { segment };
            this.resultUri = resultUri;

            this.requireMinimumVersion = new Version(1, 0);
            this.responseVersion = DataServiceDefaultResponseVersion;
            this.maxFeatureVersion = new Version(1, 0);
        }

        /// <summary>
        /// Initializes a new RequestDescription for a query specified by the
        /// request Uri.
        /// </summary>
        /// <param name="segmentInfos">list containing information about each segment of the request uri</param>
        /// <param name="containerName">Name of the container source.</param>
        /// <param name="usesContainerName">Whether the container name should be used to name the result.</param>
        /// <param name="mimeType">The MIME type for the requested resource, if specified.</param>
        /// <param name="resultUri">URI to the results requested (with no query component).</param>
        internal RequestDescription(
            SegmentInfo[] segmentInfos,
            string containerName, 
            bool usesContainerName,
            string mimeType,
            Uri resultUri)
        {
            Debug.Assert(segmentInfos != null && segmentInfos.Length != 0, "segmentInfos != null && segmentInfos.Length != 0");
            Debug.Assert(resultUri != null, "resultUri != null");
            Debug.Assert(resultUri.IsAbsoluteUri, "resultUri.IsAbsoluteUri(" + resultUri + ")");
            this.segmentInfos = segmentInfos;
            this.containerName = containerName;
            this.usesContainerName = usesContainerName;
            this.mimeType = mimeType;
            this.resultUri = resultUri;

            this.requireMinimumVersion = new Version(1, 0);
            this.responseVersion = DataServiceDefaultResponseVersion;
            this.maxFeatureVersion = new Version(1, 0);
        }

        /// <summary>Initializes a new RequestDescription based on an existing one.</summary>
        /// <param name="other">Other description to base new description on.</param>
        /// <param name="queryResults">Query results for new request description.</param>
        /// <param name="rootProjectionNode">Projection segment describing the projections on the top level of the query.</param>
        internal RequestDescription(
            RequestDescription other, 
            IEnumerable queryResults, 
            RootProjectionNode rootProjectionNode)
        {
            Debug.Assert(
                queryResults == null || other.SegmentInfos != null,
                "queryResults == null || other.SegmentInfos != null -- otherwise there isn't a segment in which to replace the query.");
            Debug.Assert(
                rootProjectionNode == null || queryResults != null,
                "rootProjectionNode == null || queryResults != null -- otherwise there isn't a query to execute and expand");

            this.containerName = other.containerName;
            this.mimeType = other.mimeType;
            this.usesContainerName = other.usesContainerName;
            this.resultUri = other.resultUri;
            this.segmentInfos = other.SegmentInfos;
            this.rootProjectionNode = rootProjectionNode;
            this.countOption = other.countOption;
            this.SkipTokenExpressionCount = other.SkipTokenExpressionCount;
            this.SkipTokenProperties = other.SkipTokenProperties;
            this.countValue = other.countValue;

            this.requireMinimumVersion = other.requireMinimumVersion;
            this.responseVersion = other.responseVersion;
            this.maxFeatureVersion = other.maxFeatureVersion;

            if (queryResults == null)
            {
                this.segmentInfos = other.SegmentInfos;
            }
            else
            {
                int lastSegmentIndex = other.SegmentInfos.Length - 1;
                SegmentInfo lastSegmentInfo = other.SegmentInfos[lastSegmentIndex];
                lastSegmentInfo.RequestEnumerable = queryResults;
            }
        }

        /// <summary>The name of the container for results.</summary>
        internal string ContainerName
        {
            [DebuggerStepThrough]
            get { return this.containerName; }
        }

        /// <summary>Root of the projection and expansion tree.</summary>
        internal RootProjectionNode RootProjectionNode
        {
            [DebuggerStepThrough]
            get { return this.rootProjectionNode; }
        }

        /// <summary>URI for the result (without the query component).</summary>
        internal Uri ResultUri
        {
            [DebuggerStepThrough]
            get { return this.resultUri; }
        }

        /// <summary>Returns the list containing the information about each segment that make up the request uri</summary>
        internal SegmentInfo[] SegmentInfos
        {
            [DebuggerStepThrough]
            get { return this.segmentInfos; }
        }

        /// <summary>The base query for the request, before client-specified composition.</summary>
        internal IEnumerable RequestEnumerable
        {
            get { return this.LastSegmentInfo.RequestEnumerable; }
        }

        /// <summary>Whether the result of this request is a single element.</summary>
        internal bool IsSingleResult
        {
            get { return this.LastSegmentInfo.SingleResult; }
        }

        /// <summary>The MIME type for the requested resource, if specified.</summary>
        internal string MimeType
        {
            [DebuggerStepThrough]
            get { return this.mimeType; }
        }

        /// <summary>The kind of target being requested.</summary>
        internal RequestTargetKind TargetKind
        {
            get { return this.LastSegmentInfo.TargetKind; }
        }

        /// <summary>The type of resource targetted by this request.</summary>
        internal ResourceType TargetResourceType
        {
            get { return this.LastSegmentInfo.TargetResourceType; }
        }

        /// <summary>The type of source for the request target.</summary>
        internal RequestTargetSource TargetSource
        {
            get { return this.LastSegmentInfo.TargetSource; }
        }

        /// <summary>
        /// Returns the resource property on which this query is targeted
        /// </summary>
        internal ResourceProperty Property
        {
            get { return this.LastSegmentInfo.ProjectedProperty; }
        }

        /// <summary>Whether the container name should be used to name the result.</summary>
        internal bool UsesContainerName
        {
            [DebuggerStepThrough]
            get { return this.usesContainerName; }
        }

        /// <summary>Returns the last segment</summary>
        internal SegmentInfo LastSegmentInfo
        {
            get { return this.segmentInfos[this.segmentInfos.Length - 1]; }
        }

        /// <summary>Returns true if the request description refers to a link uri. Otherwise returns false.</summary>
        internal bool LinkUri
        {
            get
            {
                return (this.segmentInfos.Length >= 3 && this.segmentInfos[this.segmentInfos.Length - 2].TargetKind == RequestTargetKind.Link);
            }
        }

        /// <summary>Returns the request's counting options</summary>
        internal RequestQueryCountOption CountOption
        {
            get { return this.countOption; }
            set { this.countOption = value; }
        }

        /// <summary>Number of expressions in the $skiptoken for top level expression</summary>
        internal int SkipTokenExpressionCount
        {
            get;
            set;
        }

        /// <summary>Collection of properties in the $skiptoken for top level expression</summary>
        internal ICollection<ResourceProperty> SkipTokenProperties
        {
            get;
            set;
        }

        /// <summary>Returns the value of the row count</summary>
        internal long CountValue
        {
            get { return this.countValue; }
            set { this.countValue = value; }
        }
        
        /// <summary>The minimum client version requirement</summary>
        internal Version RequireMinimumVersion
        {
            get { return this.requireMinimumVersion; }
        }

        /// <summary>The server response version</summary>
        internal Version ResponseVersion
        {
            get { return this.responseVersion; }
        }

        /// <summary>Max version of features used in the user's request</summary>
        internal Version MaxFeatureVersion
        {
            get { return this.maxFeatureVersion; }
        }

        /// <summary>
        /// Is the request for an IEnumerable&lt;T&gt; returning service operation.
        /// </summary>
        internal bool IsRequestForEnumServiceOperation
        {
            get
            {
                return this.TargetSource == RequestTargetSource.ServiceOperation && this.SegmentInfos[0].Operation.ResultKind == ServiceOperationResultKind.Enumeration;
            }
        }

        /// <summary>
        /// Get the single result from the given segment info
        /// </summary>
        /// <param name="segmentInfo">segmentInfo which contains the request query</param>
        /// <returns>query result as returned by the IQueryable query</returns>
        internal static IEnumerator GetSingleResultFromEnumerable(SegmentInfo segmentInfo)
        {
            IEnumerator queryResults = WebUtil.GetRequestEnumerator(segmentInfo.RequestEnumerable);
            bool shouldDispose = true;
            try
            {
                WebUtil.CheckResourceExists(queryResults.MoveNext(), segmentInfo.Identifier);

                CheckQueryResult(queryResults.Current, segmentInfo);

                shouldDispose = false;
                return queryResults;
            }
            finally
            {
                // Dispose the Enumerator in case of error
                if (shouldDispose)
                {
                    WebUtil.Dispose(queryResults);
                }
            }
        }

        /// <summary>
        /// Checks query result.
        /// </summary>
        /// <param name="result">Query result to be checked.</param>
        /// <param name="segmentInfo">Segment details for the <paramref name="result"/>.</param>
        internal static void CheckQueryResult(object result, SegmentInfo segmentInfo)
        {
            // e.g. /Customers(4) - if there is a direct reference to an entity, it should not be null.
            // e.g. $value also must not be null, since you are dereferencing the values
            // Any other case, having null is fine
            if (segmentInfo.IsDirectReference && result == null)
            {
                throw DataServiceException.CreateResourceNotFound(segmentInfo.Identifier);
            }

            IEnumerable enumerable;
            if (segmentInfo.TargetKind == RequestTargetKind.OpenProperty &&
                WebUtil.IsElementIEnumerable(result, out enumerable))
            {
                throw DataServiceException.CreateSyntaxError(
                    Strings.InvalidUri_OpenPropertiesCannotBeCollection(segmentInfo.Identifier));
            }
        }

        /// <summary>
        /// Create a new request description from the given request description and new entity as the result.
        /// </summary>
        /// <param name="description">Existing request description.</param>
        /// <param name="entity">entity that needs to be the result of the new request.</param>
        /// <param name="container">container to which the entity belongs to.</param>
        /// <returns>a new instance of request description containing information about the given entity.</returns>
        internal static RequestDescription CreateSingleResultRequestDescription(
            RequestDescription description, object entity, ResourceSetWrapper container)
        {
            // Create a new request description for the results that will be returned.
            SegmentInfo segmentInfo = new SegmentInfo();
            segmentInfo.RequestEnumerable = new object[] { entity };
            segmentInfo.TargetKind = description.TargetKind;
            segmentInfo.TargetSource = description.TargetSource;
            segmentInfo.SingleResult = true;
            segmentInfo.ProjectedProperty = description.Property;
            segmentInfo.TargetResourceType = container != null ? container.ResourceType : null;
            segmentInfo.TargetContainer = container;
            segmentInfo.Identifier = description.LastSegmentInfo.Identifier;
#if DEBUG
            segmentInfo.AssertValid();
#endif
            SegmentInfo[] segmentInfos = description.SegmentInfos;
            segmentInfos[segmentInfos.Length - 1] = segmentInfo;

            RequestDescription resultDescription = new RequestDescription(
                segmentInfos,
                container != null ? container.Name : null,
                description.UsesContainerName,
                description.MimeType,
                description.ResultUri);

            resultDescription.requireMinimumVersion = description.RequireMinimumVersion;
            resultDescription.responseVersion = description.ResponseVersion;
            resultDescription.maxFeatureVersion = description.MaxFeatureVersion;
            return resultDescription;
        }

        /// <summary>
        /// Checks whether etag headers are allowed (both request and response) for this request.
        /// ETag request headers are mainly If-Match and If-None-Match headers
        /// ETag response header is written only when its valid to specify one of the above mentioned request headers.
        /// </summary>
        /// <param name="description">description about the request uri.</param>
        /// <returns>true if If-Match or If-None-Match are allowed request headers for this request, otherwise false.</returns>
        internal static bool IsETagHeaderAllowed(RequestDescription description)
        {
            // IfMatch and IfNone match request headers are allowed and etag response header must be written
            // only when the request targets a single resource, which does not have $count and $links segment and there are no $expands query option specified.
            return description.IsSingleResult && description.CountOption != RequestQueryCountOption.ValueOnly && (description.RootProjectionNode == null || !description.RootProjectionNode.ExpansionsSpecified) && !description.LinkUri;
        }

        /// <summary>
        /// Verify that the request version is a version we know.
        /// </summary>
        /// <param name="requestVersion">request version from the header</param>
        /// <returns>returns true if the request version is known</returns>
        internal static bool IsKnownRequestVersion(Version requestVersion)
        {
            return KnownDataServiceVersions.Contains(requestVersion);
        }

        /// <summary>
        /// Raise the feature version if the target container contains any type with FF mapped properties that is KeepInContent=false.
        /// </summary>
        /// <param name="service">service instance</param>
        /// <returns>This RequestDescription instance</returns>
        internal RequestDescription UpdateAndCheckEpmFeatureVersion(IDataService service)
        {
            Debug.Assert(this.LastSegmentInfo != null, "this.LastSegmentInfo != null");

            if (this.LinkUri)
            {
                // For $link operations we raise the feature version if either side of $link points to a set that
                // is not V1 compatible.
                ResourceSetWrapper leftSet;
                ResourceSetWrapper rightSet;
                this.GetLinkedResourceSets(out leftSet, out rightSet);

                Debug.Assert(leftSet != null, "leftSet != null");
                Debug.Assert(rightSet != null, "rightSet != null");
                this.UpdateAndCheckEpmFeatureVersion(leftSet, service);
                this.UpdateAndCheckEpmFeatureVersion(rightSet, service);
            }
            else
            {
                // The last segment might not be a resource.  Trace backward to find the target entity resource.
                int resourceIndex = this.GetIndexOfTargetEntityResource();

                // Not every request has a target resource.  Service Operations for example can return just primitives.
                if (resourceIndex != -1)
                {
                    ResourceSetWrapper resourceSet = this.SegmentInfos[resourceIndex].TargetContainer;
                    Debug.Assert(resourceSet != null, "resourceSet != null");
                    this.UpdateAndCheckEpmFeatureVersion(resourceSet, service);
                }
            }

            return this;
        }

        /// <summary>
        /// Raise the feature version if the given set contains any type with FF mapped properties that is KeepInContent=false.
        /// </summary>
        /// <param name="resourceSet">Resource set to test</param>
        /// <param name="service">service instance</param>
        /// <returns>This RequestDescription instance</returns>
        internal RequestDescription UpdateAndCheckEpmFeatureVersion(ResourceSetWrapper resourceSet, IDataService service)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(service != null, "provider != null");

            // For feature version we only look at the SET, and bump the version if it's not V1 compatible.  We do this even if the
            // target kind is not Resource.  So we could have request and response DSV be 1.0 while feature version be 2.0.
            if (!resourceSet.EpmIsV1Compatible(service.Provider))
            {
                this.RaiseFeatureVersion(2, 0, service.Configuration);
            }

            return this;
        }

        /// <summary>Updates the response version based on response format and the target resource type</summary>
        /// <param name="acceptTypesText">text for Accepts header content required to infere response format</param>
        /// <param name="provider">data service provider instance</param>
        /// <returns>The instance for which the update of response version happens</returns>
        internal RequestDescription UpdateEpmResponseVersion(string acceptTypesText, DataServiceProviderWrapper provider)
        {
            return this.UpdateEpmResponseVersion(acceptTypesText, this.LastSegmentInfo.TargetContainer, provider);
        }

        /// <summary>Updates the response version based on response format and given resource type</summary>
        /// <param name="acceptTypesText">text for Accepts header content required to infere response format</param>
        /// <param name="resourceSet">resourceSet to check for friendly feeds presence</param>
        /// <param name="provider">data service provider instance</param>
        /// <returns>The instance for which the update of response version happens</returns>
        internal RequestDescription UpdateEpmResponseVersion(string acceptTypesText, ResourceSetWrapper resourceSet, DataServiceProviderWrapper provider)
        {
            Debug.Assert(provider != null, "provider != null");

            // Response will be 2.0 if any of the property in the types contained in the resource set has KeepInContent false
            if (this.TargetKind == RequestTargetKind.Resource)
            {
                Debug.Assert(resourceSet != null, "Must have valid resource set");
                if (!resourceSet.EpmIsV1Compatible(provider) && !this.LinkUri)
                {
                    // Friendly feeds must only bump the response version for Atom responses
                    if (WebUtil.IsAtomMimeType(acceptTypesText))
                    {
                        this.RaiseResponseVersion(2, 0);
                    }
                }
            }

            return this;
        }

        /// <summary>
        /// Returns the last segment info whose target request kind is resource
        /// </summary>
        /// <returns>The index of the parent resource</returns>
        internal int GetIndexOfTargetEntityResource()
        {
            Debug.Assert(this.segmentInfos.Length >= 1, "this.segmentInfos.Length >= 1");
            int result = -1;
            if (this.LinkUri || this.CountOption == RequestQueryCountOption.ValueOnly)
            {
                return this.SegmentInfos.Length - 1;
            }

            for (int j = this.SegmentInfos.Length - 1; j >= 0; j--)
            {
                if (this.segmentInfos[j].TargetKind == RequestTargetKind.Resource || this.segmentInfos[j].HasKeyValues)
                {
                    result = j;
                    break;
                }
            }

            return result;
        }
        
        /// <summary>
        /// Raise the minimum client version requirement for this request
        /// </summary>
        /// <param name="major">The major segment of the version</param>
        /// <param name="minor">The minor segment of the version</param>
        internal void RaiseMinimumVersionRequirement(int major, int minor)
        {
            this.requireMinimumVersion = RaiseVersion(this.requireMinimumVersion, major, minor);

            // Each time we bump the request version we need to bump the max feature version as well.
            // Note that sometimes we need to bump max feature version even if request version is not raised.
            this.maxFeatureVersion = RaiseVersion(this.maxFeatureVersion, major, minor);
        }

        /// <summary>
        /// Raise the response version for this request
        /// </summary>
        /// <param name="major">The major segment of the version</param>
        /// <param name="minor">The minor segment of the version</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Will be used for other 2.0 server features")]
        internal void RaiseResponseVersion(int major, int minor)
        {
            this.responseVersion = RaiseVersion(this.responseVersion, major, minor);

            // Each time we bump the response version we need to bump the max feature version as well.
            // Note that sometimes we need to bump max feature version even if response version is not raised.
            this.maxFeatureVersion = RaiseVersion(this.maxFeatureVersion, major, minor);
        }

        /// <summary>
        /// Raise the version for features used in the user's request
        /// </summary>
        /// <param name="major">The major segment of the version</param>
        /// <param name="minor">The minor segment of the version</param>
        /// <param name="config">Data service configuration instance to validate the feature version.</param>
        internal void RaiseFeatureVersion(int major, int minor, DataServiceConfiguration config)
        {
            this.maxFeatureVersion = RaiseVersion(this.maxFeatureVersion, major, minor);
            config.ValidateMaxProtocolVersion(this);
        }

        /// <summary>
        /// If necessary raises version to the version requested by the user.
        /// </summary>
        /// <param name="versionToRaise">Version to raise.</param>
        /// <param name="major">The major segment of the new version</param>
        /// <param name="minor">The minor segment of the new version</param>
        /// <returns>New version if the requested version is greater than the existing version.</returns>
        private static Version RaiseVersion(Version versionToRaise, int major, int minor)
        {
            if (major > versionToRaise.Major ||
                (major == versionToRaise.Major && minor > versionToRaise.Minor))
            {
                versionToRaise = new Version(major, minor);
            }

            return versionToRaise;
        }

        /// <summary>
        /// Returns the resource sets on the left and right hand sides of $link.
        /// </summary>
        /// <param name="leftSet">Resource set to the left of $link.</param>
        /// <param name="rightSet">Resource set to the right of $link.</param>
        private void GetLinkedResourceSets(out ResourceSetWrapper leftSet, out ResourceSetWrapper rightSet)
        {
            Debug.Assert(this.LinkUri, "GetLinkedResourceSets should only be called if this is a $link request.");

            int idx = 0;
            for (; idx < this.segmentInfos.Length; idx++)
            {
                if (this.segmentInfos[idx].TargetKind == RequestTargetKind.Link)
                {
                    break;
                }
            }

            Debug.Assert(idx > 0 && idx < this.segmentInfos.Length - 1, "idx > 0 && idx < this.segmentInfos.Length - 1");
            leftSet = this.segmentInfos[idx - 1].TargetContainer;
            rightSet = this.segmentInfos[idx + 1].TargetContainer;
        }
    }
}
