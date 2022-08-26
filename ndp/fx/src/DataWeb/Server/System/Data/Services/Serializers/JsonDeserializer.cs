//---------------------------------------------------------------------
// <copyright file="JsonDeserializer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a deserializer for json content.
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
    using System.Data.Services.Providers;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;

    #endregion Namespaces.

    /// <summary>
    /// Provides a deserializer for json content.
    /// </summary>
    internal class JsonDeserializer : Deserializer
    {
        /// <summary> json reader which reads json content</summary>
        private readonly JsonReader jsonReader;

        /// <summary>Initializes a new <see cref="JsonSerializer"/> for the specified stream.</summary>
        /// <param name="requestStream">Input stream from which JSON content must be read.</param>
        /// <param name="encoding">Encoding to use for the stream.</param>
        /// <param name="update">indicates whether this is a update operation or not</param>
        /// <param name="dataService">Data service for which the deserializer will act.</param>
        /// <param name="tracker">Tracker to use for modifications.</param>
        internal JsonDeserializer(Stream requestStream, Encoding encoding, bool update, IDataService dataService, UpdateTracker tracker)
            : base(update, dataService, tracker)
        {
            Debug.Assert(requestStream != null, "requestStream != null");

            // JsonReader is using StreamReader.Peek() method. However if the underlying stream does not support seeking 
            // StreamReader.Peek() will always return -1 what causes the JsonReader to think that all data has already been 
            // read and the Json payload is invalid. We need to wrap non-seekable stream with BufferedStream to make it seekable.
            
            // Since in batch cases, we use our own implementation of Stream to read the batch content, we do not want to use BufferedStream in
            // that case, since Peek just works fine in that case. Also, if we use BufferedStream, we get wierd behaviour since BufferedStream
            // tries to read few characters than the batchBoundary and our batch stream implementation does not handle that case well.
            bool useGivenStream = requestStream.CanSeek || BatchStream.IsBatchStream(requestStream);            
            this.jsonReader = new JsonReader(new StreamReader(useGivenStream ? requestStream : new BufferedStream(requestStream), encoding));
        }

        /// <summary>returns the content format for the deserializer</summary>
        protected override ContentFormat ContentFormat
        {
            get
            {
                return ContentFormat.Json;
            }
        }

        /// <summary>
        /// Converts the given value to the expected type as per json reader rules
        /// Make sure these rules are in sync with jsonwriter.
        /// </summary>
        /// <param name="value">value to the converted</param>
        /// <param name="propertyName">name of the property whose value is getting converted</param>
        /// <param name="typeToBeConverted">clr type to which the value needs to be converted to</param>
        /// <param name="provider">underlying data service provider.</param>
        /// <returns>object which is in sync with the properties type</returns>
        internal static object ConvertValues(object value, string propertyName, Type typeToBeConverted, DataServiceProviderWrapper provider)
        {
            if (value == null)
            {
                return null;
            }

            Type propertyType = Nullable.GetUnderlyingType(typeToBeConverted) ?? typeToBeConverted;

            try
            {
                string stringValue = value as string;
                if (stringValue != null)
                {
                    if (propertyType == typeof(byte[]))
                    {
                        return Convert.FromBase64String(stringValue);
                    }
                    else if (propertyType == typeof(System.Data.Linq.Binary))
                    {
                        return new System.Data.Linq.Binary(Convert.FromBase64String(stringValue));
                    }
                    else if (propertyType == typeof(System.Xml.Linq.XElement))
                    {
                        return System.Xml.Linq.XElement.Parse(stringValue, System.Xml.Linq.LoadOptions.PreserveWhitespace);
                    }
                    else if (propertyType == typeof(Guid))
                    {
                        return new Guid(stringValue);
                    }
                    else
                    {
                        // For string types, we support conversion to all possible primitive types
                        return Convert.ChangeType(value, propertyType, CultureInfo.InvariantCulture);
                    }
                }
                else if (value is Int32)
                {
                    int intValue = (int)value;
                    if (propertyType == typeof(Int16))
                    {
                        return Convert.ToInt16(intValue);
                    }
                    else if (propertyType == typeof(Byte))
                    {
                        return Convert.ToByte(intValue);
                    }
                    else if (propertyType == typeof(SByte))
                    {
                        return Convert.ToSByte(intValue);
                    }
                    else if (propertyType == typeof(Single))
                    {
                        return Convert.ToSingle(intValue);
                    }
                    else if (propertyType == typeof(Double))
                    {
                        return Convert.ToDouble(intValue);
                    }
                    else if (propertyType == typeof(Decimal) ||
                             propertyType == typeof(Int64))
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInConvertingNumericValues(propertyName));
                    }
                    else if (propertyType != typeof(Int32) && !provider.IsV1Provider)
                    {
                        // In V1, whenever we encountered a conversion which was unsafe, we would just return and most likely, it
                        // would fail when the provider tried and set the value to the property since the type won't match.
                        // Ideally, we should have thrown here, instead of allowing it to pass to the provider.
                        // Now in V2, with provider becoming public, and another configuration option available (EnableTypeConversion),
                        // it seems wrong to pass the value to the provider without doing the conversion when the EnableTypeConversion is set to true
                        // But since we can't break the behaviour for V1 providers, we will be doing this only for custom V2 providers.
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInConvertingNumericValues(propertyName));
                    }
                }
                else if (value is Double)
                {
                    Double doubleValue = (Double)value;
                    if (propertyType == typeof(Single))
                    {
                        return Convert.ToSingle(doubleValue);
                    }
                    else if (propertyType != typeof(Double) && !provider.IsV1Provider)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInConvertingNumericValues(propertyName));
                    }
                }
            }
            catch (Exception e)
            {
                if (WebUtil.IsCatchableExceptionType(e))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInConvertingPropertyValue(propertyName, propertyType.Name), e);
                }
            }

            // otherwise just return the value without doing any conversion
            return value;
        }

        /// <summary>
        /// Assumes the payload to represent a single object and processes accordingly
        /// </summary>
        /// <param name="segmentInfo">info about the object being created</param>
        /// <returns>the newly formed object that the payload represents</returns>
        protected override object CreateSingleObject(SegmentInfo segmentInfo)
        {
            object jsonObject = this.jsonReader.ReadValue();
            bool existingRelationship;
            return this.CreateObject(jsonObject, segmentInfo, true /*topLevel*/, out existingRelationship);
        }

        /// <summary>Provides an opportunity to clean-up resources.</summary>
        /// <param name="disposing">
        /// Whether the call is being made from an explicit call to 
        /// IDisposable.Dispose() rather than through the finalizer.
        /// </param>
        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);

            // Do not dispose the stream reader, since we don't own the underlying stream.
        }

        /// <summary>
        /// Get the resource referred by the uri in the payload
        /// </summary>
        /// <returns>resource referred by the uri in the payload.</returns>
        protected override string GetLinkUriFromPayload()
        {
            // top level json content must be JsonObjectRecords, since we don't allow multiple inserts
            // at the top level
            JsonReader.JsonObjectRecords jsonObject = this.jsonReader.ReadValue() as JsonReader.JsonObjectRecords;
            if (jsonObject == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidResourceEntity);
            }

            string uri = ReadUri(jsonObject.Entries);
            if (String.IsNullOrEmpty(uri))
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_MissingUriForLinkOperation);
            }

            return uri;
        }

        /// <summary>
        /// Gets the array list object
        /// </summary>
        /// <param name="jsonObject">object representing json array </param>
        /// <returns>strongly type array list object that json object represents</returns>
        private static ArrayList GetArrayList(object jsonObject)
        {
            ArrayList arrayList = jsonObject as ArrayList;
            if (arrayList == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ResourceSetPropertyMustBeArray);
            }

            return arrayList;
        }

        /// <summary>
        /// Verifies if the given element value is a deferred element or not
        /// </summary>
        /// <param name="element">element value</param>
        /// <returns>true if this value is a deferred content else returns false</returns>
        private static bool IsDeferredElement(object element)
        {
            JsonReader.JsonObjectRecords records = element as JsonReader.JsonObjectRecords;

            if (records != null && records.Count == 1 && records.OrderedKeys[0] == XmlConstants.JsonDeferredString)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Returns true if the payload is correct for the top level non-entity target.
        /// </summary>
        /// <param name="jsonObject">json object representing the data in the payload.</param>
        /// <param name="segment">information about the last segment in the request uri.</param>
        /// <param name="resource">resource object as specified in the payload.</param>
        /// <returns>returns true if the payload is correct for non-entity resource.</returns>
        private static bool HandleTopLevelNonEntityProperty(JsonReader.JsonObjectRecords jsonObject, SegmentInfo segment, out object resource)
        {
            Debug.Assert(jsonObject != null, "jsonObject != null");
            Debug.Assert(segment != null, "segment != null");
            resource = null;

            if (segment.TargetKind == RequestTargetKind.Primitive ||
                segment.TargetKind == RequestTargetKind.OpenProperty ||
                segment.TargetKind == RequestTargetKind.ComplexObject)
            {
                if (jsonObject.Count == 1 && jsonObject.Entries.TryGetValue(segment.Identifier, out resource))
                {
                    // For open property, assume it to be a primitive or complex payload.
                    // If its targeting an entity, then the type must be specified
                    return true;
                }
                else if (segment.TargetKind != RequestTargetKind.OpenProperty)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidResourceEntity);
                }
            }

            // its an entity resource payload
            return false;
        }

        /// <summary>
        /// Read the uri from the json object
        /// </summary>
        /// <param name="metadata">metadata object which contains the uri.</param>
        /// <returns>returns the uri as specified in the object.</returns>
        private static string ReadUri(Dictionary<String, Object> metadata)
        {
            string uri = null;

            // Get the uri for the metadata element
            object uriObject;
            if (metadata.TryGetValue(XmlConstants.JsonUriString, out uriObject))
            {
                uri = uriObject as string;
                if (uri == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidUriMetadata);
                }
            }

            return uri;
        }

        /// <summary>
        /// Create the object given the list of the properties. One of the properties will be __metadata property
        /// which will contain type information
        /// </summary>
        /// <param name="jsonObject">list of the properties and values specified in the payload</param>
        /// <param name="segmentInfo">info about the object being created</param>
        /// <param name="topLevel">true if the current object is a top level one, otherwise false</param>
        /// <param name="existingRelationship">does this resource already binded to its parent</param>
        /// <returns>instance of the object created</returns>
        private object CreateObject(object jsonObject, SegmentInfo segmentInfo, bool topLevel, out bool existingRelationship)
        {
            this.RecurseEnter();

            existingRelationship = true;
            bool existingResource = true;
            object resource = null;
            ResourceType resourceType;
            JsonReader.JsonObjectRecords jsonObjectRecord;

            if (topLevel)
            {
                // Every top level json content must be JsonObjectRecords - primitive, complex or entity
                jsonObjectRecord = jsonObject as JsonReader.JsonObjectRecords;
                if (jsonObjectRecord == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidResourceEntity);
                }

                object nonEntityResource;
                if (HandleTopLevelNonEntityProperty(jsonObjectRecord, segmentInfo, out nonEntityResource))
                {
                    // if the segment refers to primitive type, then return the value
                    if (segmentInfo.TargetKind == RequestTargetKind.Primitive ||
                        nonEntityResource == null ||
                        (segmentInfo.TargetKind == RequestTargetKind.OpenProperty && WebUtil.IsPrimitiveType(nonEntityResource.GetType())))
                    {
                        return nonEntityResource;
                    }

                    jsonObject = nonEntityResource;
                }
            }
            else if (
                jsonObject == null ||
                (segmentInfo.TargetKind == RequestTargetKind.OpenProperty && WebUtil.IsPrimitiveType(jsonObject.GetType())) ||
                segmentInfo.TargetKind == RequestTargetKind.Primitive)
            {
                // For reference properties, we do not know if there was already some relationship setup
                // By setting it to null, we are unbinding the old relationship and hence existing relationship
                // is false
                // For open properties, if its null, there is no way we will be able to deduce the type
                existingRelationship = false;
                return jsonObject;
            }

            // Otherwise top level json content must be JsonObjectRecords, since we don't allow multiple inserts
            // at the top level
            jsonObjectRecord = jsonObject as JsonReader.JsonObjectRecords;
            if (jsonObjectRecord == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidResourceEntity);
            }

            ResourceType targetResourceType = null;
            if (segmentInfo.TargetKind != RequestTargetKind.OpenProperty)
            {
                targetResourceType = segmentInfo.TargetResourceType;
                Debug.Assert(targetResourceType != null, "Should be able to resolve type for well known segments");
                Debug.Assert(
                    targetResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType || targetResourceType.ResourceTypeKind == ResourceTypeKind.EntityType,
                    "targetType must be entity type or complex type");
            }

            // Get the type and uri from the metadata element, if specified
            string uri;
            bool metadataElementSpecified;
            resourceType = this.GetTypeAndUriFromMetadata(
                jsonObjectRecord.Entries,
                targetResourceType,
                topLevel,
                out uri,
                out metadataElementSpecified);

            Debug.Assert((resourceType != null && resourceType.ResourceTypeKind != ResourceTypeKind.Primitive) || uri != null, "Either uri or resource type must be specified");

            if ((uri != null || resourceType.ResourceTypeKind == ResourceTypeKind.EntityType) && segmentInfo.TargetKind == RequestTargetKind.OpenProperty)
            {
                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(segmentInfo.Identifier));
            }

            this.CheckAndIncrementObjectCount();
            if ((resourceType != null && resourceType.ResourceTypeKind != ResourceTypeKind.ComplexType) ||
                uri != null)
            {
                // For inserts/updates, its okay not to specify anything in the payload.
                // Someone might just want to create a entity with default values or
                // merge nothing or replace everything with default values.
                if (this.Update)
                {
                    if (!topLevel)
                    {
                        if (metadataElementSpecified && jsonObjectRecord.Count > 1 ||
                            !metadataElementSpecified)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_DeepUpdateNotSupported);
                        }
                        else if (uri == null)
                        {
                            throw DataServiceException.CreateBadRequestError(Strings.BadRequest_UriMissingForUpdateForDeepUpdates);
                        }
                    }

                    if (topLevel)
                    {
                        // Checking for merge vs replace semantics
                        // Only checking for top level resource entity
                        // since we don't support update of deep resources
                        resource = GetObjectFromSegmentInfo(
                            resourceType,
                            segmentInfo,
                            true /*checkETag*/,
                            true /*checkForNull*/,
                            this.Service.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.PUT /*replaceResource*/);
                    }
                    else
                    {
                        // case of binding at the first level.
                        existingRelationship = false;
                        return this.GetTargetResourceToBind(uri, false /*checkNull*/);
                    }
                }
                else
                {
                    // For insert, its a new resource that is getting created or an existing resource
                    // getting binded. Either case, its a new relationship.
                    existingRelationship = false;

                    // For POST operations, the following rules holds true:
                    // 1> If the uri is specified for navigation properties and no other property is specified, then its a bind operation.
                    // Otherwise, ignore the uri and insert the new resource.
                    if (uri != null)
                    {
                        if (segmentInfo.TargetSource == RequestTargetSource.Property && jsonObjectRecord.Count == 1)
                        {
                            this.RecurseLeave();
                            return this.GetTargetResourceToBind(uri, false /*checkNull*/);
                        }
                    }
                }
            }

            Debug.Assert(resourceType != null, "resourceType != null");
            if (resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
            {
                Debug.Assert(resource == null, "resource == null");
                resource = this.Updatable.CreateResource(null, resourceType.FullName);
                existingResource = false;
            }
            else if (!this.Update)
            {
                Debug.Assert(resource == null, "resource == null");
                if (segmentInfo.TargetKind == RequestTargetKind.Resource)
                {
                    // check for append rights whenever we need to create a resource
                    DataServiceConfiguration.CheckResourceRights(segmentInfo.TargetContainer, EntitySetRights.WriteAppend);

                    resource = this.Updatable.CreateResource(segmentInfo.TargetContainer.Name, resourceType.FullName);

                    // If resourceType is FF mapped with KeepInContent=false and the response format is Atom, we need to raise the response DSV version
                    // Note that we only need to do this for POST since PUT responds with 204 and DSV=1.0
                    //
                    // Errr, mismatching request and response formats don't meet the bar at this point, commenting out the fix...
                    //
                    //// this.UpdateAndCheckEpmRequestResponseDSV(resourceType, topLevel);

                    this.Tracker.TrackAction(resource, segmentInfo.TargetContainer, UpdateOperations.Add);
                }
                else
                {
                    Debug.Assert(segmentInfo.TargetKind == RequestTargetKind.OpenProperty, "segmentInfo.TargetKind == RequestTargetKind.OpenProperty");

                    // Open navigation properties are not supported on OpenTypes.
                    throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(segmentInfo.Identifier));
                }

                existingResource = false;
            }

            bool changed = this.PopulateProperties(jsonObjectRecord, resource, segmentInfo.TargetContainer, resourceType);

            // For put operations, you need not specify any property and that means reset all the properties.
            // hence for put operations, change is always true.
            changed = changed || this.Service.OperationContext.Host.AstoriaHttpVerb == AstoriaVerbs.PUT;
            if (changed && existingResource && segmentInfo.TargetContainer != null)
            {
                this.Tracker.TrackAction(resource, segmentInfo.TargetContainer, UpdateOperations.Change);
            }

            this.RecurseLeave();
            return resource;
        }

        /// <summary>
        /// Populate the properties of the given resource
        /// </summary>
        /// <param name="jsonObject">JsonObjectRecords containing property name and values</param>
        /// <param name="resource">instance of the resource whose properties needs to be populated</param>
        /// <param name="parentResourceSet">resource set where <paramref name="resource"/> belongs to</param>
        /// <param name="parentResourceType">resource type whose properties needs to be populated</param>
        /// <returns>true if any properties were set; false otherwise.</returns>
        private bool PopulateProperties(JsonReader.JsonObjectRecords jsonObject, object resource, ResourceSetWrapper parentResourceSet, ResourceType parentResourceType)
        {
            // Update all the properties specified in the payload. 
            // Don't touch the properties which are not specified. Its upto the provider to interpret
            // the meaning of things which are not specified
            bool changed = false;
            List<ResourceProperty> navProperties = new List<ResourceProperty>();
            List<object> navPropertyValues = new List<object>();

            #region Handle Non-Nav Properties
            foreach (string propertyName in jsonObject.OrderedKeys)
            {
                // Ignore the metadata property
                if (propertyName == XmlConstants.JsonMetadataString)
                {
                    continue;
                }

                // Check if the property exists and try and set the value
                ResourceProperty resourceProperty = parentResourceType.TryResolvePropertyName(propertyName);
                if (resourceProperty == null && parentResourceType.IsOpenType == false)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(propertyName, parentResourceType.FullName));
                }

                // Get the property value, set it appropriately, and mark the object as changed.
                object propertyValue = jsonObject.Entries[propertyName];
                bool existingRelationship;

                // If its a open property
                if (resourceProperty == null)
                {
                    this.HandleOpenTypeProperties(resource, propertyName, propertyValue);
                    changed = true;
                }
                else if (resourceProperty.TypeKind == ResourceTypeKind.ComplexType)
                {
                    SegmentInfo segmentInfo = CreateSegment(resourceProperty, resourceProperty.Name, null, true /* singleResult */);
                    segmentInfo.TargetKind = RequestTargetKind.ComplexObject;
                    propertyValue = this.CreateObject(propertyValue, segmentInfo, false /*topLevel*/, out existingRelationship);
                    SetPropertyValue(resourceProperty, resource, propertyValue, ContentFormat.Json, this.Service);
                    changed = true;
                }
                else if (resourceProperty.TypeKind == ResourceTypeKind.Primitive)
                {
                    // Ignoring the value of key properties in PUT payload
                    if (!this.Update || !resourceProperty.IsOfKind(ResourcePropertyKind.Key))
                    {
                        SetPropertyValue(resourceProperty, resource, propertyValue, ContentFormat.Json, this.Service);
                    }

                    changed = true;
                }
                else
                {
                    Debug.Assert(ResourceTypeKind.EntityType == resourceProperty.TypeKind, "only expecting nav properties");

                    if (IsDeferredElement(propertyValue))
                    {
                        // Skip the deferred element
                        continue;
                    }
                    else
                    {
                        navProperties.Add(resourceProperty);
                        navPropertyValues.Add(propertyValue);
                    }
                }
            }

            #endregion Non-Navigation Properties

            #region Handle Navigation Properties

            Debug.Assert(navProperties.Count == navPropertyValues.Count, "nav properties and nav property values count must be the same");
            
            // The reason why we need to do this is so that we can gaurantee that the nav properties are getting set at the end.
            // This is nice, since we already do this in the atom deserializer. Hence its consistent. Second, we wanted to
            // give a gaurantee that when FK and nav properties are specified in the payload, nav properties always win.
            for (int i = 0; i < navProperties.Count; i++)
            {
                this.HandleNavigationProperty(parentResourceSet, parentResourceType, resource, navProperties[i], navPropertyValues[i]);
                changed = true;
            }            
            
            #endregion Handle Navigation Properties

            return changed;
        }

        /// <summary>
        /// Handle the open type property
        /// </summary>
        /// <param name="parentResource">parent resource to which the open property belongs to</param>
        /// <param name="propertyName">name of the property</param>
        /// <param name="propertyValue">value of the property</param>
        private void HandleOpenTypeProperties(object parentResource, string propertyName, object propertyValue)
        {
            bool existingRelationship;

            if (IsDeferredElement(propertyValue))
            {
                // Skip the deferred element
                return;
            }

            // Check if its a collection or not
            ArrayList arrayList = propertyValue as ArrayList;
            if (arrayList == null)
            {
                SegmentInfo openPropertySegmentInfo = CreateSegment(null, propertyName, null, true /* singleResult */);
                propertyValue = this.CreateObject(
                   propertyValue,
                   openPropertySegmentInfo,
                   false /*topLevel*/,
                   out existingRelationship);

                // Resolve the type of the value
                if (propertyValue == null || WebUtil.IsPrimitiveType(propertyValue.GetType()))
                {
                    // For open properties, just set the value since we only support primitive type properties as 
                    // open properties
                    SetOpenPropertyValue(parentResource, propertyName, propertyValue, this.Service);
                }
                else
                {
                    ResourceType openPropertyResourceType = WebUtil.GetResourceType(this.Service.Provider, propertyValue);
                    if (openPropertyResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                    {
                        this.Updatable.SetValue(parentResource, propertyName, propertyValue);
                    }
                    else
                    {
                        Debug.Assert(openPropertyResourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "resource must be of entity type");

                        // Open navigation properties are not supported on OpenTypes.
                        throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
                    }
                }
            }
            else
            {
                // Open navigation properties are not supported on OpenTypes.
                throw DataServiceException.CreateBadRequestError(Strings.OpenNavigationPropertiesNotSupportedOnOpenTypes(propertyName));
            }
        }

        /// <summary>
        /// Gets the type and uri specified in the metadata object in the given json object.
        /// </summary>
        /// <param name="jsonObjectTable">jsonObject which contains the metadata information</param>
        /// <param name="expectedType">expected type that this segment of the uri is targeted to</param>
        /// <param name="topLevel">whether the segment represents the top level object.</param>
        /// <param name="uri">uri as specified in the metadata object. If its not specified, this is set to null</param>
        /// <param name="metadataElementSpecified">returns true if the metadata element was specified</param>
        /// <returns>typename and uri as specified in the metadata object</returns>
        private ResourceType GetTypeAndUriFromMetadata(
            Dictionary<String, Object> jsonObjectTable,
            ResourceType expectedType,
            bool topLevel,
            out string uri,
            out bool metadataElementSpecified)
        {
            metadataElementSpecified = false;

            // Get the metadata object
            object metadataObject;
            ResourceType targetType = expectedType;
            bool typeNameSpecified = false;
            uri = null;

            if (jsonObjectTable.TryGetValue(XmlConstants.JsonMetadataString, out metadataObject))
            {
                metadataElementSpecified = true;
                JsonReader.JsonObjectRecords metadata = metadataObject as JsonReader.JsonObjectRecords;
                if (metadata == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidMetadataContent);
                }

                // Get the type information from the metadata object. if the type name is not specified,
                // then return the expectedType as the target type
                object objectTypeName;
                if (metadata.Entries.TryGetValue(XmlConstants.JsonTypeString, out objectTypeName))
                {
                    string typeName = objectTypeName as string;
                    if (string.IsNullOrEmpty(typeName))
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidTypeMetadata);
                    }

                    // Resolve resource type name
                    targetType = this.Service.Provider.TryResolveResourceType(typeName);
                    if (targetType == null || targetType.ResourceTypeKind == ResourceTypeKind.Primitive)
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeName(typeName));
                    }

                    if (expectedType != null && !expectedType.IsAssignableFrom(targetType))
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidTypeSpecified(typeName, expectedType.FullName));
                    }

                    typeNameSpecified = true;
                }

                uri = JsonDeserializer.ReadUri(metadata.Entries);
            }

            // Type information is optional for bind operations. 
            // Top level operations cannot be bind operations, since uri need to have $links
            // for top level bind operations and that's a different code path.
            // For bind operations, uri must be specified and nothing else should be specified.
            bool bindOperation = !topLevel && uri != null && jsonObjectTable.Count == 1;

            // type name must be specified for POST or PUT/MERGE operations.
            if (!typeNameSpecified)
            {
                if (!bindOperation)
                {
                    if (expectedType == null)
                    {
                        // For open properties, you must specify the type information
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_MissingTypeInformationForOpenTypeProperties);
                    }
                    else if (this.Service.Provider.HasDerivedTypes(expectedType))
                    {
                        // For types that take part in inheritance, type information must be specified.
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequest_TypeInformationMustBeSpecifiedForInhertiance);
                    }
                }
                else
                {
                    // If the type name is not specified, we should set the type name to null, since in case of inheritance,
                    // we don't want to guess the type information.
                    targetType = null;
                }
            }

            return targetType;
        }

        /// <summary>
        /// Handle the navigation properties as specified in the payload
        /// </summary>
        /// <param name="parentResourceSet">resource set where <paramref name="resource"/> belongs to</param>        
        /// <param name="parentResourceType">resource type declaring the navigation property.</param>
        /// <param name="resource">instance of the resource declaring the navigation property.</param>
        /// <param name="resourceProperty">resource property containing metadata about the navigation property.</param>
        /// <param name="propertyValue">value of the navigation property.</param>
        private void HandleNavigationProperty(ResourceSetWrapper parentResourceSet, ResourceType parentResourceType, object resource, ResourceProperty resourceProperty, object propertyValue)
        {
            Debug.Assert(parentResourceSet != null, "parentResourceSet != null");
            Debug.Assert(parentResourceType != null, "parentResourceType != null");
            Debug.Assert(resourceProperty != null && resourceProperty.TypeKind == ResourceTypeKind.EntityType, "its must be a nav property");

            bool existingRelationship;
            Deserializer.CheckForBindingInPutOperations(this.Service.OperationContext.Host.AstoriaHttpVerb);

            ResourceSetWrapper propertySet = this.Service.Provider.GetContainer(parentResourceSet, parentResourceType, resourceProperty);
            if (propertySet == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_InvalidPropertyNameSpecified(resourceProperty.Name, parentResourceType.FullName));
            }

            // FeatureVersion needs to be 2.0 if any of the property in the types contained in the resource set has KeepInContent false
            this.RequestDescription.UpdateAndCheckEpmFeatureVersion(propertySet, this.Service);

            if (resourceProperty.Kind == ResourcePropertyKind.ResourceReference)
            {
                SegmentInfo segmentInfo = CreateSegment(resourceProperty, resourceProperty.Name, propertySet, true /* singleResult */);
                segmentInfo.TargetKind = RequestTargetKind.Resource;

                // For navigation property, allow both inserts and binding in this case
                propertyValue = this.CreateObject(propertyValue, segmentInfo, false /*topLevel*/, out existingRelationship);
                if (!existingRelationship)
                {
                    this.Updatable.SetReference(resource, resourceProperty.Name, propertyValue);
                }
            }
            else if (resourceProperty.Kind == ResourcePropertyKind.ResourceSetReference)
            {
                if (propertyValue == null)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequest_CannotSetCollectionsToNull(resourceProperty.Name));
                }

                ArrayList resourceCollection = GetArrayList(propertyValue);
                SegmentInfo segmentInfo = CreateSegment(resourceProperty, resourceProperty.Name, propertySet, true /* singleResult */);
                foreach (object resourceObject in resourceCollection)
                {
                    object resourceInstance = this.CreateObject(resourceObject, segmentInfo, false /*topLevel*/, out existingRelationship);
                    Debug.Assert(resourceInstance != null, "resourceInstance != null");

                    if (!existingRelationship)
                    {
                        this.Updatable.AddReferenceToCollection(resource, resourceProperty.Name, resourceInstance);
                    }
                }
            }
        }
    }
}
