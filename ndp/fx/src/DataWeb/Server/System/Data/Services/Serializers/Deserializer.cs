//---------------------------------------------------------------------
// <copyright file="Deserializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a base deserializer for all deserializers.
// </summary>
//
// @owner Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data.Services.Parsing;
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;

    /// <summary>
    /// Provides a abstract base deserializer class
    /// </summary>
    internal abstract class Deserializer : IDisposable
    {
        /// <summary>Maximum recursion limit on deserializer.</summary>
        private const int RecursionLimit = 100;

        /// <summary>Data service for which the deserializer will act.</summary>
        private readonly IDataService service;

        /// <summary>Tracker for actions taken during deserialization.</summary>
        private readonly UpdateTracker tracker;

        /// <summary> Indicates whether the payload is for update or not </summary>
        private readonly bool update;

        /// <summary>Depth of recursion.</summary>
        private int recursionDepth;

        /// <summary>number of resources (entity or complex type) referred in this request.</summary>
        private int objectCount;

        /// <summary>Request description for the top level target entity.</summary>
        private RequestDescription description;

        /// <summary>Initializes a new <see cref="Deserializer"/> for the specified stream.</summary>
        /// <param name="update">indicates whether this is a update operation or not</param>
        /// <param name="dataService">Data service for which the deserializer will act.</param>
        /// <param name="tracker">Tracker to use for modifications.</param>
        internal Deserializer(bool update, IDataService dataService, UpdateTracker tracker)
        {
            Debug.Assert(dataService != null, "dataService != null");

            this.service = dataService;
            this.tracker = tracker;
            this.update = update;
        }

        /// <summary>Initializes a new <see cref="Deserializer"/> based on a different one.</summary>
        /// <param name="parent">Parent deserializer for the new instance.</param>
        internal Deserializer(Deserializer parent)
        {
            Debug.Assert(parent != null, "parent != null");
            this.recursionDepth = parent.recursionDepth;
            this.service = parent.service;
            this.tracker = parent.tracker;
            this.update = parent.update;
        }

        /// <summary>Tracker for actions taken during deserialization.</summary>
        internal UpdateTracker Tracker
        {
            [DebuggerStepThrough]
            get { return this.tracker; }
        }

        /// <summary>returns the content format for the deserializer</summary>
        protected abstract ContentFormat ContentFormat
        {
            get;
        }

        /// <summary>Data service for which the deserializer will act.</summary>
        protected IDataService Service
        {
            [DebuggerStepThrough]
            get { return this.service; }
        }

        /// <summary>Return the IUpdatable object to use to make changes to entity states</summary>
        protected UpdatableWrapper Updatable
        {
            get
            {
                return this.Service.Updatable;
            }
        }

        /// <summary>
        /// Returns true if the request method is a PUT (update) method
        /// </summary>
        protected bool Update
        {
            [DebuggerStepThrough]
            get { return this.update; }
        }

        /// <summary>Returns the current count of number of objects referred by this request.</summary>
        protected int MaxObjectCount
        {
            get { return this.objectCount; }
        }

        /// <summary>Request description for the top level target entity.</summary>
        protected RequestDescription RequestDescription
        {
            get { return this.description; }
        }

        /// <summary>Releases resources held onto by this object.</summary>
        void IDisposable.Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Creates a new <see cref="Deserializer"/> for the specified stream.
        /// </summary>
        /// <param name="description">description about the request uri.</param>
        /// <param name="dataService">Data service for which the deserializer will act.</param>
        /// <param name="update">indicates whether this is a update operation or not</param>
        /// <param name="tracker">Tracker to use for modifications.</param>
        /// <returns>A new instance of <see cref="Deserializer"/>.</returns>
        internal static Deserializer CreateDeserializer(RequestDescription description, IDataService dataService, bool update, UpdateTracker tracker)
        {
            string mimeType;
            System.Text.Encoding encoding;
            DataServiceHostWrapper host = dataService.OperationContext.Host;
            HttpProcessUtility.ReadContentType(host.RequestContentType, out mimeType, out encoding);
            ContentFormat requestFormat = WebUtil.SelectRequestFormat(mimeType, description);
            Stream requestStream = host.RequestStream;
            Debug.Assert(requestStream != null, "requestStream != null");
            Debug.Assert(tracker != null, "Change tracker must always be created.");
            Deserializer deserializer = null;

            Debug.Assert(
                (!update /*POST*/ && dataService.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.POST) ||
                (update /*PUT,MERGE*/ && (dataService.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.MERGE || dataService.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.PUT)),
                "For PUT and MERGE, update must be true; for POST, update must be false");

            switch (requestFormat)
            {
                case ContentFormat.Json:
                    deserializer = new JsonDeserializer(
                        requestStream,
                        encoding,
                        update,
                        dataService,
                        tracker);
                    break;
                case ContentFormat.Atom:
                    SyndicationFormatterFactory factory = new Atom10FormatterFactory();
                    deserializer = new SyndicationDeserializer(
                        requestStream,  // stream
                        encoding,       // encoding
                        dataService,    // dataService
                        update,
                        factory,
                        tracker);       // factory
                    break;
                case ContentFormat.PlainXml:
                    deserializer = new PlainXmlDeserializer(
                        requestStream,
                        encoding,
                        dataService,
                        update,
                        tracker);
                    break;
                default:
                    throw new DataServiceException(415, Strings.BadRequest_UnsupportedRequestContentType(host.RequestContentType));
            }

            Debug.Assert(deserializer != null, "deserializer != null");

            return deserializer;
        }

        /// <summary>
        /// Converts the given value to the expected type as per the deserializer rules
        /// </summary>
        /// <param name="value">value to the converted</param>
        /// <param name="property">property information whose value is the first parameter</param>
        /// <param name="contentFormat">specifies the content format of the payload</param>
        /// <param name="provider">underlying data service provider.</param>
        /// <returns>object which is in sync with the properties type</returns>
        internal static object ConvertValues(object value, ResourceProperty property, ContentFormat contentFormat, DataServiceProviderWrapper provider)
        {
            Debug.Assert(property.TypeKind == ResourceTypeKind.Primitive, "This method must be called for primitive types only");

            if (contentFormat == ContentFormat.Json)
            {
                return JsonDeserializer.ConvertValues(value, property.Name, property.Type, provider);
            }
            else if (contentFormat == ContentFormat.Atom || contentFormat == ContentFormat.PlainXml)
            {
                return PlainXmlDeserializer.ConvertValuesForXml(value, property.Name, property.Type);
            }
            else
            {
                Debug.Assert(
                    contentFormat == ContentFormat.Binary || contentFormat == ContentFormat.Text,
                    "expecting binary or text");

                // Do not do any coversions for them
                return value;
            }
        }

        /// <summary>
        /// Update the resource specified in the given request description
        /// </summary>
        /// <param name="description">description about the request uri</param>
        /// <param name="dataService">data service type to which the request was made</param>
        /// <param name="stream">Stream from which request body should be read.</param>
        /// <returns>The tracked modifications.</returns>
        internal static RequestDescription HandlePutRequest(RequestDescription description, IDataService dataService, Stream stream)
        {
            Debug.Assert(stream != null, "stream != null");
            Debug.Assert(dataService != null, "dataService != null");

            object requestValue = null;
            ContentFormat requestFormat;
            object entityGettingModified = null;
            ResourceSetWrapper container = null;
            string mimeType;
            Encoding encoding;
            DataServiceHostWrapper host = dataService.OperationContext.Host;
            HttpProcessUtility.ReadContentType(host.RequestContentType, out mimeType, out encoding);

            UpdateTracker tracker = UpdateTracker.CreateUpdateTracker(dataService);
            Debug.Assert(tracker != null, "Change tracker must always be created.");

            // If its a primitive value that is getting modified, then we need to use the text or binary
            // serializer depending on the mime type
            if (description.TargetKind == RequestTargetKind.OpenPropertyValue ||
                description.TargetKind == RequestTargetKind.PrimitiveValue)
            {
                string contentType;
                requestFormat = WebUtil.GetResponseFormatForPrimitiveValue(description.TargetResourceType, out contentType);
                if (!WebUtil.CompareMimeType(contentType, mimeType))
                {
                    if (description.TargetResourceType != null)
                    {
                        throw new DataServiceException(415, Strings.BadRequest_InvalidContentType(host.RequestContentType, description.TargetResourceType.Name));
                    }
                    else
                    {
                        throw new DataServiceException(415, Strings.BadRequest_InvalidContentTypeForOpenProperty(host.RequestContentType, description.ContainerName));
                    }
                }

                if (requestFormat == ContentFormat.Binary)
                {
                    byte[] propertyValue = ReadByteStream(stream);
                    if (description.Property != null && description.Property.Type == typeof(System.Data.Linq.Binary))
                    {
                        requestValue = new System.Data.Linq.Binary(propertyValue);
                    }
                    else
                    {
                        requestValue = propertyValue;
                    }
                }
                else
                {
                    Debug.Assert(requestFormat == ContentFormat.Text, "requestFormat == ContentFormat.Text");
                    Debug.Assert(encoding != null, "encoding != null");
                    StreamReader requestReader = new StreamReader(stream, encoding);
                    string propertyValue = Deserializer.ReadStringFromStream(requestReader);

                    if (description.Property != null && propertyValue != null)
                    {
                        try
                        {
                            // Convert the property value to the correct type
                            requestValue = WebConvert.StringToPrimitive((string)propertyValue, description.Property.Type);
                        }
                        catch (FormatException e)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInConvertingPropertyValue(description.Property.Name, description.Property.Type), e);
                        }
                    }
                    else
                    {
                        // For open types, there is no conversion required. There is not enough information to do the conversion.
                        requestValue = propertyValue;
                    }
                }
            }
            else if (description.TargetKind == RequestTargetKind.MediaResource)
            {
                requestFormat = ContentFormat.Binary;
                requestValue = stream;
            }
            else
            {
                requestFormat = WebUtil.SelectRequestFormat(mimeType, description);
                using (Deserializer deserializer = Deserializer.CreateDeserializer(description, dataService, true /*update*/, tracker))
                {
                    if (description.LinkUri)
                    {
                        string uri = deserializer.GetLinkUriFromPayload();

                        // No need to check for null - if the uri in the payload is /Customer(1)/BestFriend,
                        // and the value is null, it means that the user wants to set the current link to null
                        // i.e. in other words, unbind the relationship.
                        object linkResource = deserializer.GetTargetResourceToBind(uri, true /*checkNull*/);
                        entityGettingModified = Deserializer.HandleBindOperation(description, linkResource, deserializer.Service, deserializer.Tracker);
                        container = description.LastSegmentInfo.TargetContainer;
                    }
                    else
                    {
                        requestValue = deserializer.ReadEntity(description);

                        if (requestValue == null &&
                            description.LastSegmentInfo.HasKeyValues &&
                            description.TargetSource == RequestTargetSource.EntitySet)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_CannotSetTopLevelResourceToNull(description.ResultUri.OriginalString));
                        }
                    }
                }
            }

            // Update the property value, if the request target is property
            if (!description.LinkUri && IsQueryRequired(description, requestValue, dataService.Provider))
            {
                // Get the parent entity and its container and the resource to modify
                object resourceToModify = GetResourceToModify(
                    description, dataService, false /*allowCrossReferencing*/, out entityGettingModified, out container, true /*checkETag*/);

                tracker.TrackAction(entityGettingModified, container, UpdateOperations.Change);

                Deserializer.ModifyResource(description, resourceToModify, requestValue, requestFormat, dataService);
            }

            tracker.FireNotifications();

            if (entityGettingModified == null)
            {
                entityGettingModified = requestValue;
                container = description.LastSegmentInfo.TargetContainer;
            }

            return RequestDescription.CreateSingleResultRequestDescription(description, entityGettingModified, container);
        }

        /// <summary>
        /// Gets the resource to modify.
        /// </summary>
        /// <param name="description">description about the target request</param>
        /// <param name="service">data service type to which the request was made</param>
        /// <param name="entityResource">entity resource which is getting modified.</param>
        /// <param name="container">entity container of the entity which is getting modified.</param>
        /// <returns>Returns the object that needs to get modified</returns>
        internal static object GetResourceToModify(RequestDescription description, IDataService service, out object entityResource, out ResourceSetWrapper container)
        {
            return GetResourceToModify(description, service, false /*allowCrossReference*/, out entityResource, out container, false /*checkETag*/);
        }

        /// <summary>
        /// Returns the last segment info whose target request kind is resource
        /// </summary>
        /// <param name="description">description about the target request</param>
        /// <param name="service">data service type to which the request was made</param>
        /// <param name="allowCrossReferencing">whether cross-referencing is allowed for the resource in question.</param>
        /// <param name="entityResource">entity resource which is getting modified.</param>
        /// <param name="entityContainer">entity container of the entity which is getting modified.</param>
        /// <param name="checkETag">whether to check the etag for the entity resource that is getting modified.</param>
        /// <returns>Returns the object that needs to get modified</returns>
        internal static object GetResourceToModify(
            RequestDescription description,
            IDataService service,
            bool allowCrossReferencing,
            out object entityResource,
            out ResourceSetWrapper entityContainer,
            bool checkETag)
        {
            Debug.Assert(description.SegmentInfos.Length >= 2, "description.SegmentInfos.Length >= 2");

            UpdatableWrapper updatable = service.Updatable;

            if (!allowCrossReferencing && description.RequestEnumerable == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ResourceCanBeCrossReferencedOnlyForBindOperation);
            }

            // Set the index of the modifying resource
            int modifyingResourceIndex = -1;
            if (
                description.TargetKind == RequestTargetKind.OpenPropertyValue ||
                description.TargetKind == RequestTargetKind.PrimitiveValue)
            {
                modifyingResourceIndex = description.SegmentInfos.Length - 3;
            }
            else
            {
                modifyingResourceIndex = description.SegmentInfos.Length - 2;
            }

            // Get the index of the entity resource that is getting modified
            int entityResourceIndex = -1;
            if (description.LinkUri)
            {
                entityResourceIndex = modifyingResourceIndex;
            }
            else
            {
                for (int j = modifyingResourceIndex; j >= 0; j--)
                {
                    if (description.SegmentInfos[j].TargetKind == RequestTargetKind.Resource ||
                        description.SegmentInfos[j].HasKeyValues)
                    {
                        entityResourceIndex = j;
                        break;
                    }
                }
            }

            Debug.Assert(entityResourceIndex != -1, "This method should never be called for request that doesn't have a parent resource");
            entityContainer = description.SegmentInfos[entityResourceIndex].TargetContainer;
            if (entityContainer != null)
            {
                DataServiceHostWrapper host = service.OperationContext.Host;

                // Since this is the entity which is going to get modified, then we need to check for rights
                if (host.AstoriaHttpVerb == AstoriaVerbs.PUT)
                {
                    DataServiceConfiguration.CheckResourceRights(entityContainer, EntitySetRights.WriteReplace);
                }
                else if (host.AstoriaHttpVerb == AstoriaVerbs.MERGE)
                {
                    DataServiceConfiguration.CheckResourceRights(entityContainer, EntitySetRights.WriteMerge);
                }
                else
                {
                    Debug.Assert(
                        host.AstoriaHttpVerb == AstoriaVerbs.POST ||
                        host.AstoriaHttpVerb == AstoriaVerbs.DELETE,
                        "expecting POST and DELETE methods");
                    DataServiceConfiguration.CheckResourceRights(entityContainer, EntitySetRights.WriteMerge | EntitySetRights.WriteReplace);
                }
            }

            entityResource = service.GetResource(description, entityResourceIndex, null);
            if (entityResource == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_DereferencingNullPropertyValue(description.SegmentInfos[entityResourceIndex].Identifier));
            }

            // now walk from the entity resource to the resource to modify.
            // for open types, as you walk, if the intermediate resource is an entity,
            // update the entityResource accordingly.
            object resourceToModify = entityResource;
            for (int i = entityResourceIndex + 1; i <= modifyingResourceIndex; i++)
            {
                resourceToModify = updatable.GetValue(resourceToModify, description.SegmentInfos[i].Identifier);
            }

            if (entityContainer == null)
            {
                Debug.Assert(
                    description.TargetKind == RequestTargetKind.OpenProperty ||
                    description.TargetKind == RequestTargetKind.OpenPropertyValue,
                    "its a open property target. Hence resource set must be null");

                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(description.SegmentInfos[entityResourceIndex].Identifier));
            }

            // If checkETag is true, then we need to check the etag for the resource
            // Note that MediaResource has a separate etag, we don't need to check the MLE etag if the target kind is MediaResource
            if (checkETag && !Deserializer.IsCrossReferencedSegment(description.SegmentInfos[modifyingResourceIndex], service) && description.TargetKind != RequestTargetKind.MediaResource)
            {
                service.Updatable.SetETagValues(entityResource, entityContainer);
            }

            return resourceToModify;
        }

        /// <summary>
        /// Modify the value of the given resource to the given value
        /// </summary>
        /// <param name="description">description about the request</param>
        /// <param name="resourceToBeModified">resource that needs to be modified</param>
        /// <param name="requestValue">the new value for the target resource</param>
        /// <param name="contentFormat">specifies the content format of the payload</param>
        /// <param name="service">Service this request is against</param>
        internal static void ModifyResource(RequestDescription description, object resourceToBeModified, object requestValue, ContentFormat contentFormat, IDataService service)
        {
            if (description.TargetKind == RequestTargetKind.OpenProperty ||
                description.TargetKind == RequestTargetKind.OpenPropertyValue)
            {
                Debug.Assert(!description.LastSegmentInfo.HasKeyValues, "CreateSegments must have caught the problem already.");
                SetOpenPropertyValue(resourceToBeModified, description.ContainerName, requestValue, service);
            }
            else if (description.TargetKind == RequestTargetKind.MediaResource)
            {
                SetStreamPropertyValue(resourceToBeModified, (Stream)requestValue, service, description);
            }
            else
            {
                Debug.Assert(
                    description.TargetKind == RequestTargetKind.Primitive ||
                    description.TargetKind == RequestTargetKind.ComplexObject ||
                    description.TargetKind == RequestTargetKind.PrimitiveValue,
                    "unexpected target kind encountered");

                // update the primitive value
                ResourceProperty propertyToUpdate = description.LastSegmentInfo.ProjectedProperty;
                SetPropertyValue(propertyToUpdate, resourceToBeModified, requestValue, contentFormat, service);
            }
        }

        /// <summary>
        /// Get the resource referred by the given segment
        /// </summary>
        /// <param name="segmentInfo">information about the segment.</param>
        /// <param name="fullTypeName">full name of the resource referred by the segment.</param>
        /// <param name="service">data service type to which the request was made</param>
        /// <param name="checkForNull">whether to check if the resource is null or not.</param>
        /// <returns>returns the resource returned by the provider.</returns>
        internal static object GetResource(SegmentInfo segmentInfo, string fullTypeName, IDataService service, bool checkForNull)
        {
            if (segmentInfo.TargetContainer != null)
            {
                Debug.Assert(
                    segmentInfo.TargetKind != RequestTargetKind.OpenProperty &&
                    segmentInfo.TargetKind != RequestTargetKind.OpenPropertyValue,
                    "container can be null only for open types");

                DataServiceConfiguration.CheckResourceRights(segmentInfo.TargetContainer, EntitySetRights.ReadSingle);
            }

            object resource = service.Updatable.GetResource((IQueryable)segmentInfo.RequestEnumerable, fullTypeName);
            if (resource == null &&
                (segmentInfo.HasKeyValues || checkForNull))
            {
                throw DataServiceException.CreateResourceNotFound(segmentInfo.Identifier);
            }

            return resource;
        }

        /// <summary>
        /// Creates a Media Link Entry.
        /// </summary>
        /// <param name="fullTypeName">Full type name for the MLE to be created.</param>
        /// <param name="requestStream">Request stream from the host.</param>
        /// <param name="service">Service this request is against.</param>
        /// <param name="description">Description of the target request.</param>
        /// <param name="tracker">Update tracker instance to fire change interceptor calls</param>
        /// <returns>Newly created Media Link Entry.</returns>
        internal static object CreateMediaLinkEntry(string fullTypeName, Stream requestStream, IDataService service, RequestDescription description, UpdateTracker tracker)
        {
            Debug.Assert(!string.IsNullOrEmpty(fullTypeName), "!string.IsNullOrEmpty(fullTypeName)");
            Debug.Assert(requestStream != null, "requestStream != null");
            Debug.Assert(service != null, "service != null");
            Debug.Assert(description != null, "description != null");
            Debug.Assert(tracker != null, "tracker != null");

            object entity = service.Updatable.CreateResource(description.LastSegmentInfo.TargetContainer.Name, fullTypeName);
            tracker.TrackAction(entity, description.LastSegmentInfo.TargetContainer, UpdateOperations.Add);
            SetStreamPropertyValue(entity, requestStream, service, description);
            return entity;
        }

        /// <summary>
        /// Copy the contents of the request stream into the default stream of the specified entity.
        /// </summary>
        /// <param name="resourceToBeModified">Entity with the associated stream which we will write to.</param>
        /// <param name="requestStream">Request stream from the host</param>
        /// <param name="service">Service this is request is against</param>
        /// <param name="description">Description of the target request.</param>
        internal static void SetStreamPropertyValue(object resourceToBeModified, Stream requestStream, IDataService service, RequestDescription description)
        {
            Debug.Assert(resourceToBeModified != null, "resourceToBeModified != null");
            Debug.Assert(requestStream.CanRead, "requestStream.CanRead");
            Debug.Assert(service != null, "service != null");

            resourceToBeModified = service.Updatable.ResolveResource(resourceToBeModified);

            ResourceType resourceType = service.Provider.GetResourceType(resourceToBeModified);
            if (!resourceType.IsMediaLinkEntry)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidUriForMediaResource(service.OperationContext.AbsoluteRequestUri));
            }

            if (service.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.MERGE)
            {
                throw DataServiceException.CreateMethodNotAllowed(
                    Strings.BadRequest_InvalidUriForMergeOperation(service.OperationContext.AbsoluteRequestUri),
                    DataServiceConfiguration.GetAllowedMethods(service.Configuration, description));
            }

            using (Stream writeStream = service.StreamProvider.GetWriteStream(resourceToBeModified, service.OperationContext))
            {
                WebUtil.CopyStream(requestStream, writeStream, service.StreamProvider.StreamBufferSize);
            }
        }

        /// <summary>
        /// Gets the resource from the segment enumerable.
        /// </summary>
        /// <param name="segmentInfo">segment from which resource needs to be returned.</param>
        /// <returns>returns the resource contained in the request enumerable.</returns>
        internal static object GetCrossReferencedResource(SegmentInfo segmentInfo)
        {
            Debug.Assert(segmentInfo.RequestEnumerable != null, "The segment should always have the result");
            object[] results = (object[])segmentInfo.RequestEnumerable;
            Debug.Assert(results != null && results.Length == 1, "results != null && results.Length == 1");
            Debug.Assert(results[0] != null, "results[0] != null");
            return results[0];
        }

        /// <summary>
        /// Test if the given segment is a cross referenced segment in a batch operation
        /// </summary>
        /// <param name="segmentInfo">Segment in question</param>
        /// <param name="service">service instance</param>
        /// <returns>True if the given segment is a cross referenced segment</returns>
        internal static bool IsCrossReferencedSegment(SegmentInfo segmentInfo, IDataService service)
        {
            if (segmentInfo.Identifier.StartsWith("$", StringComparison.Ordinal) && service.GetSegmentForContentId(segmentInfo.Identifier) != null)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Handle bind operation
        /// </summary>
        /// <param name="description">information about the request uri.</param>
        /// <param name="linkResource">the child resource which needs to be linked.</param>
        /// <param name="service">data service instance</param>
        /// <param name="tracker">update tracker instance to fire change interceptor calls</param>
        /// <returns>returns the parent object to which an new object was linked to.</returns>
        internal static object HandleBindOperation(RequestDescription description, object linkResource, IDataService service, UpdateTracker tracker)
        {
            Debug.Assert(description != null, "description != null");
            Debug.Assert(linkResource != null, "linkResource != null");
            Debug.Assert(service != null, "service != null");
            Debug.Assert(tracker != null, "tracker != null");

            object entityGettingModified;
            ResourceSetWrapper container;

            object resourceToBeModified = Deserializer.GetResourceToModify(description, service, true /*allowCrossReference*/, out entityGettingModified, out container, false /*checkETag*/);
            Debug.Assert(resourceToBeModified == entityGettingModified, "Since this is a link operation, modifying resource must be the entity resource");

            // If the container we are modifying contains any type with FF mapped KeepInContent=false properties, we need to raise the FeatureVersion.
            description.UpdateAndCheckEpmFeatureVersion(container, service);

            tracker.TrackAction(entityGettingModified, container, UpdateOperations.Change);
            Debug.Assert(description.Property != null, "description.Property != null");
            if (description.IsSingleResult)
            {
                service.Updatable.SetReference(entityGettingModified, description.Property.Name, linkResource);
            }
            else
            {
                service.Updatable.AddReferenceToCollection(entityGettingModified, description.Property.Name, linkResource);
            }

            return entityGettingModified;
        }

        /// <summary>
        /// Deserializes the given stream into clr object as specified in the payload
        /// </summary>
        /// <param name="requestDescription">description about the target request</param>
        /// <returns>the object instance that it created and populated from the reader</returns>
        internal object ReadEntity(RequestDescription requestDescription)
        {
            Debug.Assert(requestDescription != null, "requestDescription != null");
            this.description = requestDescription;

            if (requestDescription.TargetKind == RequestTargetKind.Resource)
            {
                Debug.Assert(requestDescription.LastSegmentInfo != null, "requestDescription.LastSegmentInfo != null");
                Debug.Assert(requestDescription.LastSegmentInfo.TargetContainer != null, "requestDescription.LastSegmentInfo.TargetContainer != null");
                Debug.Assert(requestDescription.TargetResourceType != null, "requestDescription.TargetResourceType != null");
                this.RequestDescription.UpdateAndCheckEpmFeatureVersion(this.description.LastSegmentInfo.TargetContainer, this.Service);
            }

            // If the description points to a resource,
            // we need to materialize the object and return back.
            SegmentInfo segmentInfo = requestDescription.LastSegmentInfo;
            if (!this.Update)
            {
                Debug.Assert(!segmentInfo.SingleResult, "POST operation is allowed only on collections");
                SegmentInfo adjustedSegment = new SegmentInfo();
                adjustedSegment.TargetKind = segmentInfo.TargetKind;
                adjustedSegment.TargetSource = segmentInfo.TargetSource;
                adjustedSegment.SingleResult = true;
                adjustedSegment.ProjectedProperty = segmentInfo.ProjectedProperty;
                adjustedSegment.TargetResourceType = segmentInfo.TargetResourceType;
                adjustedSegment.TargetContainer = segmentInfo.TargetContainer;
                adjustedSegment.Identifier = segmentInfo.Identifier;
                segmentInfo = adjustedSegment;
            }

            return this.CreateSingleObject(segmentInfo);
        }

        /// <summary>
        /// Handles post request.
        /// </summary>
        /// <param name="requestDescription">description about the uri for the post operation.</param>
        /// <returns>returns the resource that is getting inserted or binded - as specified in the payload.</returns>
        internal object HandlePostRequest(RequestDescription requestDescription)
        {
            Debug.Assert(!this.Update, "This method must be called for POST operations only");
            Debug.Assert(requestDescription != null, "requestDescription != null");
            object resourceInPayload;

            if (requestDescription.LinkUri)
            {
                string uri = this.GetLinkUriFromPayload();
                resourceInPayload = this.GetTargetResourceToBind(uri, true /*checkNull*/);
                Debug.Assert(resourceInPayload != null, "link resource cannot be null");
                Deserializer.HandleBindOperation(requestDescription, resourceInPayload, this.Service, this.Tracker);
            }
            else
            {
                if (requestDescription.LastSegmentInfo.TargetContainer != null)
                {
                    DataServiceConfiguration.CheckResourceRights(requestDescription.LastSegmentInfo.TargetContainer, EntitySetRights.WriteAppend);
                }

                resourceInPayload = this.ReadEntity(requestDescription);
                if (requestDescription.TargetSource == RequestTargetSource.Property)
                {
                    Debug.Assert(requestDescription.Property.Kind == ResourcePropertyKind.ResourceSetReference, "Expecting POST resource set property");
                    Deserializer.HandleBindOperation(requestDescription, resourceInPayload, this.Service, this.Tracker);
                }
                else
                {
                    Debug.Assert(requestDescription.TargetSource == RequestTargetSource.EntitySet, "Expecting POST on entity set");
                    this.tracker.TrackAction(resourceInPayload, requestDescription.LastSegmentInfo.TargetContainer, UpdateOperations.Add);
                }
            }

            return resourceInPayload;
        }

        /// <summary>
        /// Update the object count value to the given value.
        /// </summary>
        /// <param name="value">value to be set for object count.</param>
        internal void UpdateObjectCount(int value)
        {
            Debug.Assert(0 <= value, "MaxObjectCount cannot be initialized to a negative number");
            Debug.Assert(value <= this.Service.Configuration.MaxObjectCountOnInsert, "On initialize, the value should be less than max object count");
            this.objectCount = value;
        }

        /// <summary>
        /// Set the value of the given resource property to the new value
        /// </summary>
        /// <param name="resourceProperty">property whose value needs to be updated</param>
        /// <param name="declaringResource">instance of the declaring type of the property for which the property value needs to be updated</param>
        /// <param name="propertyValue">new value for the property</param>
        /// <param name="contentFormat">specifies the content format of the payload</param>
        /// <param name="service">Service this is request is against</param>
        protected static void SetPropertyValue(ResourceProperty resourceProperty, object declaringResource, object propertyValue, ContentFormat contentFormat, IDataService service)
        {
            Debug.Assert(
                resourceProperty.TypeKind == ResourceTypeKind.ComplexType ||
                resourceProperty.TypeKind == ResourceTypeKind.Primitive,
                "Only primitive and complex type values must be set via this method");

            // For open types, resource property can be null
            if (resourceProperty.TypeKind == ResourceTypeKind.Primitive)
            {
                // Only do the conversion if the provider explicitly asked us to do the configuration.
                if (service.Configuration.EnableTypeConversion)
                {
                    // First convert the value of the property to the expected type, as specified in the resource property
                    propertyValue = ConvertValues(propertyValue, resourceProperty, contentFormat, service.Provider);
                }
            }

            service.Updatable.SetValue(declaringResource, resourceProperty.Name, propertyValue);
        }

        /// <summary>
        /// Set the value of the open property
        /// </summary>
        /// <param name="declaringResource">instance of the declaring type of the property for which the property value needs to be updated</param>
        /// <param name="propertyName">name of the open property to update</param>
        /// <param name="propertyValue">new value for the property</param>
        /// <param name="service">Service this request is against</param>
        protected static void SetOpenPropertyValue(object declaringResource, string propertyName, object propertyValue, IDataService service)
        {
            service.Updatable.SetValue(declaringResource, propertyName, propertyValue);
        }

        /// <summary>
        /// Reads the content from the stream reader and returns it as string
        /// </summary>
        /// <param name="streamReader">stream reader from which the content needs to be read</param>
        /// <returns>string containing the content as read from the stream reader</returns>
        protected static string ReadStringFromStream(StreamReader streamReader)
        {
            return streamReader.ReadToEnd();
        }

        /// <summary>
        /// Make sure binding operations cannot be performed in PUT operations
        /// </summary>
        /// <param name="requestVerb">http method name for the request.</param>
        protected static void CheckForBindingInPutOperations(AstoriaVerbs requestVerb)
        {
            // Cannot bind in PUT operations.
            if (requestVerb == AstoriaVerbs.PUT)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_CannotUpdateRelatedEntitiesInPut);
            }
        }

        /// <summary>Creates a new SegmentInfo for the specified <paramref name="property"/>.</summary>
        /// <param name="property">Property to create segment info for (possibly null).</param>
        /// <param name="propertyName">Name for the property.</param>
        /// <param name="propertySet">Target resource set for the property.</param>
        /// <param name="singleResult">Whether a single result is expected.</param>
        /// <returns>
        /// A new <see cref="SegmentInfo"/> instance that describes the specfied <paramref name="property"/>
        /// as a target, or an open proprty if <paramref name="property"/> is null.
        /// </returns>
        protected static SegmentInfo CreateSegment(ResourceProperty property, string propertyName, ResourceSetWrapper propertySet, bool singleResult)
        {
            SegmentInfo result = new SegmentInfo();
            result.TargetSource = RequestTargetSource.Property;
            result.SingleResult = singleResult;
            result.Identifier = propertyName;
            if (property == null)
            {
                result.TargetKind = RequestTargetKind.OpenProperty;
            }
            else
            {
                result.TargetKind = RequestTargetKind.Resource;
                result.Identifier = propertyName;
                result.ProjectedProperty = property;
                result.TargetResourceType = property.ResourceType;
                result.TargetContainer = propertySet;
            }

            return result;
        }

        /// <summary>
        /// Create the object from the given payload and return the top level object
        /// </summary>
        /// <param name="segmentInfo">info about the object being created</param>
        /// <returns>instance of the object created</returns>
        protected abstract object CreateSingleObject(SegmentInfo segmentInfo);

        /// <summary>
        /// Get the resource referred by the uri in the payload
        /// </summary>
        /// <returns>resource referred by the uri in the payload.</returns>
        protected abstract string GetLinkUriFromPayload();

        /// <summary>Provides an opportunity to clean-up resources.</summary>
        /// <param name="disposing">
        /// Whether the call is being made from an explicit call to 
        /// IDisposable.Dispose() rather than through the finalizer.
        /// </param>
        protected virtual void Dispose(bool disposing)
        {
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

        /// <summary>
        /// Returns the target/child resource to bind to an resource, which might be getting inserted or modified.
        /// Since this is a target resource, null is a valid value here (for e.g. /Customers(1)/BestFriend value
        /// can be null)
        /// </summary>
        /// <param name="uri">uri referencing to the resource to be returned.</param>
        /// <param name="checkNull">whether the resource can be null or not.</param>
        /// <returns>returns the resource as referenced by the uri. Throws 404 if the checkNull is true and the resource returned is null.</returns>
        protected object GetTargetResourceToBind(string uri, bool checkNull)
        {
            // 

            Uri referencedUri = RequestUriProcessor.GetAbsoluteUriFromReference(uri, this.Service.OperationContext);
            RequestDescription requestDescription = RequestUriProcessor.ProcessRequestUri(referencedUri, this.Service);

            // Get the resource
            object resourceCookie = this.Service.GetResource(requestDescription, requestDescription.SegmentInfos.Length - 1, null);
            if (checkNull)
            {
                WebUtil.CheckResourceExists(resourceCookie != null, requestDescription.LastSegmentInfo.Identifier);
            }

            return resourceCookie;
        }

        /// <summary>
        /// Gets a resource referenced by the given segment info.
        /// </summary>
        /// <param name="resourceType">resource type whose instance needs to be created</param>
        /// <param name="segmentInfo">segment info containing the description of the uri</param>
        /// <param name="verifyETag">verify etag value of the current resource with one specified in the request header</param>
        /// <param name="checkForNull">validate that the resource cannot be null.</param>
        /// <param name="replaceResource">reset the resource as referred by the segment.</param>
        /// <returns>a new instance of the given resource type with key values populated</returns>
        protected object GetObjectFromSegmentInfo(
            ResourceType resourceType,
            SegmentInfo segmentInfo,
            bool verifyETag,
            bool checkForNull,
            bool replaceResource)
        {
            Debug.Assert(resourceType == null && !verifyETag || resourceType != null, "For etag verification, resource type must be specified");

            if (segmentInfo.RequestEnumerable == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ResourceCanBeCrossReferencedOnlyForBindOperation);
            }

            object resourceCookie;
            if (Deserializer.IsCrossReferencedSegment(segmentInfo, this.service))
            {
                resourceCookie = Deserializer.GetCrossReferencedResource(segmentInfo);
            }
            else
            {
                resourceCookie = Deserializer.GetResource(
                    segmentInfo, 
                    resourceType != null ? resourceType.FullName : null, 
                    this.Service, 
                    checkForNull);

                // We only need to check etag if the resource is not cross-referenced. If the resource is cross-referenced,
                // there is no good way to checking etag for that resource, since it might have 
                if (verifyETag)
                {
                    this.service.Updatable.SetETagValues(resourceCookie, segmentInfo.TargetContainer);
                }
            }

            if (replaceResource)
            {
                Debug.Assert(checkForNull, "For resetting resource, the value cannot be null");
                resourceCookie = this.Updatable.ResetResource(resourceCookie);
                WebUtil.CheckResourceExists(resourceCookie != null, segmentInfo.Identifier);
            }

            return resourceCookie;
        }

        /// <summary>
        /// Check and increment the object count
        /// </summary>
        protected void CheckAndIncrementObjectCount()
        {
            Debug.Assert(this.Update && this.objectCount == 0 || !this.Update, "For updates, the object count is never tracked");
            Debug.Assert(this.objectCount <= this.Service.Configuration.MaxObjectCountOnInsert, "The object count should never exceed the limit");

            if (!this.Update)
            {
                this.objectCount++;

                if (this.objectCount > this.Service.Configuration.MaxObjectCountOnInsert)
                {
                    throw new DataServiceException(413, Strings.BadRequest_ExceedsMaxObjectCountOnInsert(this.Service.Configuration.MaxObjectCountOnInsert));
                }
            }
        }

        /// <summary>
        /// Bump the minimum DSV requirement if the given resource type has friendly feed mappings with KeepInContent=false.
        /// Note that the minimum DSV requirement is type (i.e. payload) specific and is applicable for Atom format only.
        /// </summary>
        /// <param name="resourceType">Resource type to inspect</param>
        /// <param name="topLevel">True if resourceType is the type for the top level element in the Atom payload.</param>
        protected void UpdateAndCheckEpmRequestResponseDSV(ResourceType resourceType, bool topLevel)
        {
            Debug.Assert(this.Service != null, "this.Service != null");
            Debug.Assert(resourceType != null, "Must have valid resource type");
            Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "resourceType.ResourceTypeKind == ResourceTypeKind.EntityType");
            Debug.Assert(this.RequestDescription != null, "this.RequestDescription != null");

            if (!resourceType.EpmIsV1Compatible)
            {
                // Only raise the minimum version requirement if this is an Atom request.
                if (this is SyndicationDeserializer)
                {
                    this.RequestDescription.RaiseMinimumVersionRequirement(2, 0);
                }

                // For POST operations we need to raise the response version for Atom if resourceType is the type for the top
                // level element since we will serialize the newly created instance of this type in the response payload.
                // For PUT/MEREGE we respond with 204 and therefore no backcompat issue with 1.0
                if (!this.Update && topLevel)
                {
                    // Friendly feeds must only bump the response version for Atom responses
                    if (WebUtil.IsAtomMimeType(this.Service.OperationContext.Host.RequestAccept))
                    {
                        this.RequestDescription.RaiseResponseVersion(2, 0);
                    }
                }
            }

            WebUtil.CheckVersion(this.Service, this.RequestDescription);
        }

        /// <summary>
        /// Read the byte from the given input request stream
        /// </summary>
        /// <param name="stream">input/request stream from which data needs to be read</param>
        /// <returns>byte array containing all the data read</returns>
        private static byte[] ReadByteStream(Stream stream)
        {
            byte[] data;

            // try to read data from the stream 1k at a time
            long numberOfBytesRead = 0;
            int result = 0;
            List<byte[]> byteData = new List<byte[]>();

            do
            {
                data = new byte[4000];
                result = stream.Read(data, 0, data.Length);
                numberOfBytesRead += result;
                byteData.Add(data);
            }
            while (result == data.Length);

            // Find out the total number of bytes read and copy data from byteData to data
            data = new byte[numberOfBytesRead];
            for (int i = 0; i < byteData.Count - 1; i++)
            {
                Buffer.BlockCopy(byteData[i], 0, data, i * 4000, 4000);
            }

            // For the last thing, copy the remaining number of bytes, not always 4000
            Buffer.BlockCopy(byteData[byteData.Count - 1], 0, data, (byteData.Count - 1) * 4000, result);
            return data;
        }

        /// <summary>
        /// Returns true if we need to query the provider before updating.
        /// </summary>
        /// <param name="requestDescription">request description</param>
        /// <param name="requestValue">value corresponding to the payload for this request</param>
        /// <param name="provider">provider against which the request was targeted</param>
        /// <returns>returns true if we need to issue an query to satishfy the request</returns>
        private static bool IsQueryRequired(RequestDescription requestDescription, object requestValue, DataServiceProviderWrapper provider)
        {
            Debug.Assert(requestDescription.IsSingleResult, "requestDescription.IsSingleResult");

            if (requestDescription.TargetKind == RequestTargetKind.PrimitiveValue ||
                requestDescription.TargetKind == RequestTargetKind.Primitive ||
                requestDescription.TargetKind == RequestTargetKind.OpenPropertyValue ||
                requestDescription.TargetKind == RequestTargetKind.MediaResource ||
                requestDescription.TargetKind == RequestTargetKind.ComplexObject)
            {
                return true;
            }

            if (requestDescription.TargetKind == RequestTargetKind.OpenProperty)
            {
                Debug.Assert(!requestDescription.LastSegmentInfo.HasKeyValues, "CreateSegments must have caught this issue.");
                
                // if the value is null, then just set it, since we don't know the type
                if (requestValue == null || WebUtil.IsPrimitiveType(requestValue.GetType()))
                {
                    return true;
                }

                // otherwise just set the complex type properties
                if (WebUtil.GetNonPrimitiveResourceType(provider, requestValue).ResourceTypeKind == ResourceTypeKind.ComplexType)
                {
                    return true;
                }
            }

            return false;
        }
    }
}
