//---------------------------------------------------------------------
// <copyright file="ObjectContextServiceProvider.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides the interface definition for web data service
//      data sources.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Providers
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data;
    using System.Data.EntityClient;
    using System.Data.Metadata.Edm;
    using System.Data.Objects;
    using System.Data.Objects.DataClasses;
    using System.Data.Services.Caching;
    using System.Data.Services.Common;
    using System.Data.Services.Serializers;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Xml;
    using System.Xml.Linq;

    #endregion Namespaces.

    /// <summary>
    /// Provides a reflection-based provider implementation.
    /// </summary>
    [DebuggerDisplay("ObjectContextServiceProvider: {Type}")]
    internal partial class ObjectContextServiceProvider : BaseServiceProvider, IDataServiceUpdateProvider
    {
        #region Private fields.

        /// <summary>
        /// List of objects that we need to be replaced. The key value indicates the current instance
        /// that will be replaced during SaveChanges. All the property changes are expected to happen
        /// on the value instance. At the time of SaveChanges, all the changes applied to the Value 
        /// instance are then applied to the instance present in Key and then it is saved.
        /// Since EF will always return the same reference for same key value by looking up the first
        /// level cache, we can assume reference equality for the objects thus obtained.
        /// </summary>
        private Dictionary<object, object> objectsToBeReplaced = new Dictionary<object, object>(ReferenceEqualityComparer<object>.Instance);

        /// <summary>List of cspace types for which ospace metadata couldn't be found.</summary>
        private List<StructuralType> typesWithoutOSpaceMetadata;

        #endregion Private fields.

        /// <summary>
        /// Initializes a new System.Data.Services.ReflectionServiceProvider instance.
        /// </summary>
        /// <param name="metadata">Metadata for this provider.</param>
        /// <param name="dataServiceInstance">instance of the data service.</param>
        internal ObjectContextServiceProvider(MetadataCacheItem metadata, object dataServiceInstance)
            : base(metadata, dataServiceInstance)
        {
            this.typesWithoutOSpaceMetadata = new List<StructuralType>();
        }

        /// <summary>Gets a value indicating whether null propagation is required in expression trees.</summary>
        public override bool IsNullPropagationRequired
        {
            get { return false; }
        }

        /// <summary>Namespace name for the EDM container.</summary>
        public override string ContainerNamespace
        {
            get { return this.Type.Namespace; }
        }

        /// <summary>Name of the EDM container</summary>
        public override string ContainerName
        {
            get { return this.ObjectContext.DefaultContainerName; }
        }

        /// <summary>Strongly-types instance being reflected upon.</summary>
        private ObjectContext ObjectContext
        {
            get
            {
                return (ObjectContext)this.CurrentDataSource;
            }
        }

        /// <summary>
        /// Gets the ResourceAssociationSet instance when given the source association end.
        /// </summary>
        /// <param name="resourceSet">Resource set of the source association end.</param>
        /// <param name="resourceType">Resource type of the source association end.</param>
        /// <param name="resourceProperty">Resource property of the source association end.</param>
        /// <returns>ResourceAssociationSet instance.</returns>
        public override ResourceAssociationSet GetResourceAssociationSet(ResourceSet resourceSet, ResourceType resourceType, ResourceProperty resourceProperty)
        {
            Debug.Assert(resourceSet != null, "resourceSet != null");
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(resourceProperty != null, "resourceProperty != null");
            Debug.Assert(resourceType == DataServiceProviderWrapper.GetDeclaringTypeForProperty(resourceType, resourceProperty), "resourceType should be the declaring type for resourceProperty");

            // Get source set
            EntitySet sourceEntitySet = this.GetEntitySet(resourceSet.Name);
            Debug.Assert(sourceEntitySet != null, "entitySet != null -- GetEntitySet should never return null");

            // Get the source type
            EntityType sourceEntityType = this.ObjectContext.MetadataWorkspace.GetItem<EntityType>(resourceType.FullName, DataSpace.CSpace);
            Debug.Assert(sourceEntityType != null, "entityType != null");

            // Get source navigation property
            NavigationProperty sourceNavigationProperty;
            sourceEntityType.NavigationProperties.TryGetValue(resourceProperty.Name, false /*ignoreCase*/, out sourceNavigationProperty);
            Debug.Assert(sourceNavigationProperty != null, "navigationProperty != null");
            Debug.Assert(sourceEntityType == (EntityType)sourceNavigationProperty.DeclaringType, "sourceEntityType == (EntityType)sourceNavigationProperty.DeclaringType");

            ResourceAssociationSet result = null;
            foreach (AssociationSet associationSet in sourceEntitySet.EntityContainer.BaseEntitySets.OfType<AssociationSet>())
            {
                if (associationSet.ElementType == sourceNavigationProperty.RelationshipType)
                {
                    // from AssociationSetEnd
                    AssociationSetEnd setEnd = associationSet.AssociationSetEnds[sourceNavigationProperty.FromEndMember.Name];
                    if (setEnd.EntitySet == sourceEntitySet)
                    {
                        // from ResourceAssociationSetEnd
                        ResourceAssociationSetEnd thisAssociationSetEnd = new ResourceAssociationSetEnd(resourceSet, resourceType, resourceProperty);

                        // to AssociationSetEnd
                        setEnd = associationSet.AssociationSetEnds[sourceNavigationProperty.ToEndMember.Name];

                        // Get the target resource set
                        EntitySet targetEntitySet = setEnd.EntitySet;
                        string targetEntitySetName = GetEntitySetName(targetEntitySet.Name, targetEntitySet.EntityContainer.Name, this.ObjectContext.DefaultContainerName == targetEntitySet.EntityContainer.Name);
                        ResourceSet targetResourceSet;
                        ((IDataServiceMetadataProvider)this).TryResolveResourceSet(targetEntitySetName, out targetResourceSet);
                        Debug.Assert(targetResourceSet != null, "targetResourceSet != null");

                        // Get the target resource type
                        EntityType targetEntityType = (EntityType)((RefType)sourceNavigationProperty.ToEndMember.TypeUsage.EdmType).ElementType;
                        ResourceType targetResourceType;
                        ((IDataServiceMetadataProvider)this).TryResolveResourceType(targetEntityType.FullName, out targetResourceType);
                        Debug.Assert(targetResourceType != null, "targetResourceType != null");

                        // Get the target resource property
                        ResourceProperty targetResourceProperty = null;
                        foreach (NavigationProperty navProperty in targetEntityType.NavigationProperties)
                        {
                            if (navProperty.ToEndMember == sourceNavigationProperty.FromEndMember)
                            {
                                targetResourceProperty = targetResourceType.TryResolvePropertyName(navProperty.Name);
                                break;
                            }
                        }

                        // to ResourceAssociationSetEnd
                        ResourceAssociationSetEnd relatedAssociationSetEnd = new ResourceAssociationSetEnd(
                            targetResourceSet,
                            targetResourceType,
                            (resourceType == targetResourceType && resourceProperty == targetResourceProperty) ? null : targetResourceProperty);

                        result = new ResourceAssociationSet(associationSet.Name, thisAssociationSetEnd, relatedAssociationSetEnd);
                        break;
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Returns the collection of open properties name and value for the given resource instance.
        /// </summary>
        /// <param name="target">instance of the resource.</param>
        /// <returns>Returns the collection of open properties name and value for the given resource instance. Currently not supported for ObjectContext provider.</returns>
        public override IEnumerable<KeyValuePair<string, object>> GetOpenPropertyValues(object target)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the value of the open property.
        /// </summary>
        /// <param name="target">instance of the resource type.</param>
        /// <param name="propertyName">name of the property.</param>
        /// <returns>the value of the open property. Currently this is not supported for ObjectContext providers.</returns>
        public override object GetOpenPropertyValue(object target, string propertyName)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Returns the requested service
        /// </summary>
        /// <param name="serviceType">type of service you are requesting for.</param>
        /// <returns>returns the instance of the requested service.</returns>
        public override object GetService(Type serviceType)
        {
            if (serviceType == typeof(IDataServiceUpdateProvider))
            {
                return this;
            }

            return base.GetService(serviceType);
        }

        /// <summary>Applies expansions and projections to the specified <paramref name="source"/>.</summary>
        /// <param name="source"><see cref="IQueryable"/> object to expand and apply projections to.</param>
        /// <param name="projection">The root node of the tree which describes
        /// the projections and expansions to be applied to the <paramref name="source"/>.</param>
        /// <returns>
        /// An <see cref="IQueryable"/> object, with the results including 
        /// the expansions and projections specified in <paramref name="projection"/>. 
        /// </returns>
        /// <remarks>
        /// The returned <see cref="IQueryable"/> may implement the <see cref="IExpandedResult"/> interface 
        /// to provide enumerable objects for the expansions; otherwise, the expanded
        /// information is expected to be found directly in the enumerated objects. If paging is 
        /// requested by providing a non-empty list in <paramref name="projection"/>.OrderingInfo then
        /// it is expected that the topmost <see cref="IExpandedResult"/> would have a $skiptoken property 
        /// which will be an <see cref="IExpandedResult"/> in itself and each of it's sub-properties will
        /// be named SkipTokenPropertyXX where XX represents numbers in increasing order starting from 0. Each of 
        /// SkipTokenPropertyXX properties will be used to generated the $skiptoken to support paging.
        /// If projections are required, the provider may choose to return <see cref="IQueryable"/>
        /// which returns instances of <see cref="IProjectedResult"/>. In that case property values are determined
        /// by calling the <see cref="IProjectedResult.GetProjectedPropertyValue"/> method instead of
        /// accessing properties of the returned object directly.
        /// If both expansion and projections are required, the provider may choose to return <see cref="IQueryable"/>
        /// of <see cref="IExpandedResult"/> which in turn returns <see cref="IProjectedResult"/> from its
        /// <see cref="IExpandedResult.ExpandedElement"/> property.
        /// </remarks>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        public override IQueryable ApplyProjections(
            IQueryable source,
            ProjectionNode projection)
        {
            Debug.Assert(projection is RootProjectionNode, "We always get the special root node.");
            RootProjectionNode rootNode = (RootProjectionNode)projection;
            Debug.Assert(rootNode.OrderingInfo != null, "We always get non-null OrderingInfo");

            // Start by adding all the containers for the root type, in case the query originates from
            // a service operation and we don't know what the origin is.
            HashSet<ResourceSet> containers = new HashSet<ResourceSet>(EqualityComparer<ResourceSet>.Default);
            foreach (var set in this.EntitySets)
            {
                if (set.Value.ResourceType.InstanceType.IsAssignableFrom(source.ElementType))
                {
                    containers.Add(set.Value);
                }
            }

            // when we are not expanding ObjectQueries (maybe IQueryable from from service ops), 
            // then we should use basic expand provider
            bool useBasicExpandProvider = !(typeof(ObjectQuery).IsAssignableFrom(source.GetType()));

            useBasicExpandProvider |= ShouldUseBasicExpandProvider(rootNode, containers, 0);

            // Any use of ordering, skip, top settings or projections excludes the possibility of using Include method
            if (useBasicExpandProvider || rootNode.OrderingInfo.OrderingExpressions.Count > 0 ||
                rootNode.SkipCount.HasValue || rootNode.TakeCount.HasValue || rootNode.ProjectionsSpecified)
            {
                return new BasicExpandProvider(this.ProviderWrapper, false, false).ApplyProjections(source, projection);
            }

            MethodInfo includeMethod = typeof(ObjectContextServiceProvider).GetMethod("Include", BindingFlags.Static | BindingFlags.NonPublic);
            includeMethod = includeMethod.MakeGenericMethod(source.ElementType);

            return VisitDottedExpandPaths<IQueryable>(
                rootNode,
                (queryable, dottedPath) => (IQueryable)includeMethod.Invoke(null, new object[] { queryable, dottedPath }),
                source,
                new List<string>());
        }

        /// <summary>
        /// Writes the metadata in EDMX format into the specified <paramref name="xmlWriter"/>.
        /// </summary>
        /// <param name="xmlWriter">Writer to which metadata XML should be written.</param>
        /// <param name="provider">Data provider from which metadata should be gathered.</param>
        /// <param name="service">Data service instance.</param>
        public void GetMetadata(XmlWriter xmlWriter, DataServiceProviderWrapper provider, IDataService service)
        {
            Debug.Assert(xmlWriter != null, "xmlWriter != null");
            InitializeObjectItemCollection(this.ObjectContext, this.Type.Assembly);

            var metadataManager = new MetadataManager(this.ObjectContext.MetadataWorkspace, this.GetDefaultEntityContainer(), provider, service);
            MetadataEdmSchemaVersion metadataEdmSchemaVersion = this.EdmSchemaVersion;
            string dataServiceVersion = MetadataSerializer.GetVersionsForMetadata(provider.Types, ref metadataEdmSchemaVersion);
            MetadataSerializer.WriteTopLevelSchemaElements(xmlWriter, dataServiceVersion);
            bool entityContainerDefinitionWritten = false;

            Double currentVersion = ((EdmItemCollection)this.ObjectContext.MetadataWorkspace.GetItemCollection(DataSpace.CSpace)).EdmVersion;
            if (currentVersion == 2.0)
            {
                metadataEdmSchemaVersion = MetadataEdmSchemaVersion.Version2Dot0;
            }

            // Write all the types in their respective namespaces
            foreach (KeyValuePair<string, HashSet<EdmType>> typesInNamespace in metadataManager.NamespaceAlongWithTypes)
            {
                MetadataSerializer.WriteSchemaElement(xmlWriter, typesInNamespace.Key, metadataEdmSchemaVersion);

                // Write the entity container in the same namespace as that of the type
                if (!entityContainerDefinitionWritten && typesInNamespace.Key == this.Type.Namespace)
                {
                    WriteEntityContainers(xmlWriter, this.GetDefaultEntityContainer(), provider, metadataManager);
                    entityContainerDefinitionWritten = true;
                }

                WriteEdmTypes(xmlWriter, typesInNamespace.Value, metadataManager);
                xmlWriter.WriteEndElement();
            }

            // If the entity container is in a different namespace than the types, then write the entity container definition
            // in a different namespace
            if (!entityContainerDefinitionWritten)
            {
                MetadataSerializer.WriteSchemaElement(xmlWriter, this.Type.Namespace, metadataEdmSchemaVersion);
                WriteEntityContainers(xmlWriter, this.GetDefaultEntityContainer(), provider, metadataManager);
                xmlWriter.WriteEndElement();
            }

            // These end elements balance the elements written out in WriteTopLevelSchemaElements
            xmlWriter.WriteEndElement();
            xmlWriter.WriteEndElement();
            xmlWriter.Flush();
        }

        #region IUpdatable Members

        /// <summary>
        /// Creates the resource of the given type and belonging to the given container
        /// </summary>
        /// <param name="containerName">container name to which the resource needs to be added</param>
        /// <param name="fullTypeName">full type name i.e. Namespace qualified type name of the resource</param>
        /// <returns>object representing a resource of given type and belonging to the given container</returns>
        public object CreateResource(string containerName, string fullTypeName)
        {
            ResourceType resourceType;
            ((IDataServiceMetadataProvider)this).TryResolveResourceType(fullTypeName, out resourceType);
            Debug.Assert(resourceType != null, "resourceType != null");

            if (resourceType.InstanceType.IsAbstract)
            {
                throw DataServiceException.CreateBadRequestError(Strings.CannotCreateInstancesOfAbstractType(resourceType.FullName));
            }

            object resource;
            if (containerName != null)
            {
                Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.EntityType, "resourceType.ResourceTypeKind == ResourceTypeKind.EntityType - expecting an entity type");
                resource = CreateObject(this.ObjectContext, resourceType.InstanceType);
                this.ObjectContext.AddObject(containerName, resource);
            }
            else
            {
                // When the container name is null, it means we are trying to create a instance of complex types.
                Debug.Assert(resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType, "resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType - expecting a complex type");
                resource = resourceType.ConstructorDelegate();
            }

            return resource;
        }

        /// <summary>
        /// Gets the resource of the given type that the query points to
        /// </summary>
        /// <param name="query">query pointing to a particular resource</param>
        /// <param name="fullTypeName">full type name i.e. Namespace qualified type name of the resource</param>
        /// <returns>object representing a resource of given type and as referenced by the query</returns>
        public object GetResource(IQueryable query, string fullTypeName)
        {
            Debug.Assert(query != null, "query != null");

            ObjectQuery objectQuery = query as ObjectQuery;
            Debug.Assert(objectQuery != null, "objectQuery != null - otherwise we're passed an IQueryable we didn't produce");

            objectQuery.MergeOption = MergeOption.AppendOnly;
            object result = null;
            foreach (object resource in objectQuery)
            {
                if (result != null)
                {
                    throw new InvalidOperationException(Strings.SingleResourceExpected);
                }

                result = resource;
            }

            if (result != null && fullTypeName != null)
            {
                ResourceType resourceType = this.GetSingleResource(result);
                Debug.Assert(resourceType != null, "the result must return a known type");
                if (resourceType.FullName != fullTypeName)
                {
                    throw DataServiceException.CreateBadRequestError(Strings.TargetElementTypeOfTheUriSpecifiedDoesNotMatchWithTheExpectedType(resourceType.FullName, fullTypeName));
                }
            }

            return result;
        }

        /// <summary>
        /// Resets the value of the given resource to its default value
        /// </summary>
        /// <param name="resource">resource whose value needs to be reset</param>
        /// <returns>same resource with its value reset</returns>
        public object ResetResource(object resource)
        {
            Debug.Assert(resource != null, "resource != null");

            ResourceType resourceType = this.GetSingleResource(resource);
            if (resourceType.ResourceTypeKind == ResourceTypeKind.EntityType)
            {
                // For entity types, do the following:
                // create a new instance of the same type and set the key values on it
                object newInstance = CreateObject(this.ObjectContext, resourceType.InstanceType);

                // set the key value on the new instance
                foreach (ResourceProperty property in resourceType.KeyProperties)
                {
                    object propertyValue = ((IDataServiceQueryProvider)this).GetPropertyValue(resource, property);
                    resourceType.SetValue(newInstance, propertyValue, property);
                }
                
                // When reset resource, we return the old instance since it's the one
                // EF actually attached to.
                // but all property modification will be done on the newInstance
                // upon save changes, the modifications on newInstance will be merged with
                // the old instance (property by property).
                this.objectsToBeReplaced.Add(resource, newInstance);

                ObjectStateEntry objectStateEntry = this.ObjectContext.ObjectStateManager.GetObjectStateEntry(resource);
                if (objectStateEntry.State == EntityState.Added)
                {
                    // in case when the resource is been added, and we PUT to that resource
                    // we'll actually insert the newInstance and detach the old one. 
                    // So we need to return the newInstance instead.
                    this.ObjectContext.AddObject(this.GetEntitySetName(objectStateEntry), newInstance);
                    return newInstance;
                }                
            }
            else if (resourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
            {
                // For complex types, just return a brand new instance.
                return resourceType.ConstructorDelegate();
            }

            return resource;
        }

        /// <summary>
        /// Sets the value of the given property on the target object
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="propertyValue">value of the property</param>
        public void SetValue(object targetResource, string propertyName, object propertyValue)
        {
            ResourceType resourceType = this.GetSingleResource(targetResource);
            Debug.Assert(resourceType != null, "resourceType != null");
            ResourceProperty resourceProperty = resourceType.TryResolvePropertyName(propertyName);
            Debug.Assert(resourceProperty != null, "resourceProperty != null");

            // is target Resource going to be replaced?
            // See comment in ResetResources
            object replacedTarget;
            if (this.objectsToBeReplaced.TryGetValue(targetResource, out replacedTarget))
            {
                resourceType.SetValue(replacedTarget, propertyValue, resourceProperty);
            }
            else
            {
                resourceType.SetValue(targetResource, propertyValue, resourceProperty);
            }
        }

        /// <summary>
        /// Gets the value of the given property on the target object
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <returns>the value of the property for the given target resource</returns>
        public object GetValue(object targetResource, string propertyName)
        {
            ResourceType resourceType = this.GetSingleResource(targetResource);
            Debug.Assert(resourceType != null, "resourceType != null");
            ResourceProperty resourceProperty = resourceType.TryResolvePropertyName(propertyName);
            Debug.Assert(resourceProperty != null, "resourceProperty != null");
            object resource = ((IDataServiceQueryProvider)this).GetPropertyValue(targetResource, resourceProperty);
            return resource;
        }

        /// <summary>
        /// Sets the value of the given reference property on the target object
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="propertyValue">value of the property</param>
        public void SetReference(object targetResource, string propertyName, object propertyValue)
        {
            this.UpdateRelationship(targetResource, propertyName, propertyValue, null);
        }

        /// <summary>
        /// Adds the given value to the collection
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="resourceToBeAdded">value of the property which needs to be added</param>
        public void AddReferenceToCollection(object targetResource, string propertyName, object resourceToBeAdded)
        {
            this.UpdateRelationship(targetResource, propertyName, resourceToBeAdded, true /*addRelationship*/);
        }

        /// <summary>
        /// Removes the given value from the collection
        /// </summary>
        /// <param name="targetResource">target object which defines the property</param>
        /// <param name="propertyName">name of the property whose value needs to be updated</param>
        /// <param name="resourceToBeRemoved">value of the property which needs to be removed</param>
        public void RemoveReferenceFromCollection(object targetResource, string propertyName, object resourceToBeRemoved)
        {
            this.UpdateRelationship(targetResource, propertyName, resourceToBeRemoved, false /*addRelationship*/);
        }

        /// <summary>
        /// Delete the given resource
        /// </summary>
        /// <param name="resource">resource that needs to be deleted</param>
        public void DeleteResource(object resource)
        {
            this.ObjectContext.DeleteObject(resource);
        }

        /// <summary>
        /// Saves all the pending changes made till now
        /// </summary>
        public void SaveChanges()
        {
            // handle the resource which need to be replaced.
            foreach (KeyValuePair<object, object> objectToBeReplaced in this.objectsToBeReplaced)
            {
                ObjectStateEntry objectStateEntry = this.ObjectContext.ObjectStateManager.GetObjectStateEntry(objectToBeReplaced.Key);
                string entitySetName = this.GetEntitySetName(objectStateEntry);
                if (objectStateEntry.State == EntityState.Added)
                {
                    // In the case of batching if we do a PUT after POST in the same changeset, we need to detach and re-add.
                    this.ObjectContext.Detach(objectToBeReplaced.Key);
                    this.ObjectContext.AddObject(entitySetName, objectToBeReplaced.Value);
                }
                else
                {
#pragma warning disable 618
                    // Apply property changes as specified in the new object.
                    this.ObjectContext.ApplyPropertyChanges(entitySetName, objectToBeReplaced.Value);
#pragma warning restore 618
                }
            }

            // clear all these once we have processed all the entities that need to be replaced.
            this.objectsToBeReplaced.Clear();

            try
            {
                // Save Changes
                this.ObjectContext.SaveChanges();
            }
            catch (OptimisticConcurrencyException e)
            {
                throw DataServiceException.CreatePreConditionFailedError(Strings.Serializer_ETagValueDoesNotMatch, e);
            }
        }

        /// <summary>
        /// Returns the actual instance of the resource represented by the given resource object
        /// </summary>
        /// <param name="resource">object representing the resource whose instance needs to be fetched</param>
        /// <returns>The actual instance of the resource represented by the given resource object</returns>
        public object ResolveResource(object resource)
        {
            Debug.Assert(resource != null, "resource != null");
            return resource;
        }

        /// <summary>
        /// Revert all the pending changes.
        /// </summary>
        public void ClearChanges()
        {
            // Detach all the existing entries in the object context
            foreach (ObjectStateEntry entry in this.ObjectContext.ObjectStateManager.GetObjectStateEntries(EntityState.Added | EntityState.Deleted | EntityState.Modified | EntityState.Unchanged))
            {
                // entry.State != Entry.Detached: Also, if they are stub entries (no entity object
                // associated with the entry yet - these get automatically populated when we query the related entity),
                // they also get automatically detached once we detach the associated entity).
                // Both Entity and IsRelationship property throws if the entry is detached.
                // !entry.IsRelationship: We just need to remove the key entries.
                // While detaching the key entries, the relationship entries associated 
                // with the key entries also get detached. 
                // entry.Entity != null:  Since they are no ordering gaurantees, the stub key entries 
                // can come first, we need to skip these.
                if (entry.State != EntityState.Detached && !entry.IsRelationship && entry.Entity != null)
                {
                    this.ObjectContext.Detach(entry.Entity);
                }
            }

            // clear the list of objects that need to be special handled during save changes for replace semantics
            this.objectsToBeReplaced.Clear();
        }

        #endregion

        #region IConcurrencyProvider Methods

        /// <summary>
        /// Set the etag values for the given resource.
        /// </summary>
        /// <param name="resource">resource for which etag values need to be set.</param>
        /// <param name="checkForEquality">true if we need to compare the property values for equality. If false, then we need to compare values for non-equality.</param>
        /// <param name="concurrencyValues">list of the etag property names, along with their values.</param>
        public void SetConcurrencyValues(object resource, bool? checkForEquality, IEnumerable<KeyValuePair<string, object>> concurrencyValues)
        {
            // Now this method will need to check for cases when etag are specified
            if (checkForEquality == null)
            {
                ResourceType resourceType = this.GetResourceType(resource);
                throw DataServiceException.CreateBadRequestError(Strings.DataService_CannotPerformOperationWithoutETag(resourceType.FullName));
            }

            Debug.Assert(checkForEquality.Value, "If-None-Match header is currently not supported for Update/Delete operations");
            ObjectStateEntry objectStateEntry = this.ObjectContext.ObjectStateManager.GetObjectStateEntry(resource);
            Debug.Assert(objectStateEntry != null, "ObjectStateEntry must be found");

            OriginalValueRecord originalValues = objectStateEntry.GetUpdatableOriginalValues();
            foreach (KeyValuePair<string, object> etag in concurrencyValues)
            {
                int propertyOrdinal = originalValues.GetOrdinal(etag.Key);
                originalValues.SetValue(propertyOrdinal, etag.Value);
            }
        }

        #endregion IConcurrencyProvider Methods

        /// <summary>
        /// Trasnslates content kind to string for csdl
        /// </summary>
        /// <param name="contentKind">ContentKind</param>
        /// <returns>String corresponding to contentKind</returns>
        internal static String MapSyndicationTextContentKindToEpmContentKind(SyndicationTextContentKind contentKind)
        {
            switch (contentKind)
            {
                case SyndicationTextContentKind.Plaintext:
                    return XmlConstants.SyndContentKindPlaintext;
                case SyndicationTextContentKind.Html:
                    return XmlConstants.SyndContentKindHtml;
                default:
                    Debug.Assert(contentKind == SyndicationTextContentKind.Xhtml, "Unexpected syndication text content kind");
                    return XmlConstants.SyndContentKindXHtml;
            }
        }

        /// <summary>
        /// Translates syndication item property to string for csdl
        /// </summary>
        /// <param name="property">Syndication property to translate</param>
        /// <returns>TargetPath corresponding to SyndicationItemProperty</returns>
        internal static String MapSyndicationPropertyToEpmTargetPath(SyndicationItemProperty property)
        {
            switch (property)
            {
                case SyndicationItemProperty.AuthorEmail:
                    return XmlConstants.SyndAuthorEmail;
                case SyndicationItemProperty.AuthorName:
                    return XmlConstants.SyndAuthorName;
                case SyndicationItemProperty.AuthorUri:
                    return XmlConstants.SyndAuthorUri;
                case SyndicationItemProperty.ContributorEmail:
                    return XmlConstants.SyndContributorEmail;
                case SyndicationItemProperty.ContributorName:
                    return XmlConstants.SyndContributorName;
                case SyndicationItemProperty.ContributorUri:
                    return XmlConstants.SyndContributorUri;
                case SyndicationItemProperty.Updated:
                    return XmlConstants.SyndUpdated;
                case SyndicationItemProperty.Published:
                    return XmlConstants.SyndPublished;
                case SyndicationItemProperty.Rights:
                    return XmlConstants.SyndRights;
                case SyndicationItemProperty.Summary:
                    return XmlConstants.SyndSummary;
                default:
                    Debug.Assert(property == SyndicationItemProperty.Title, "Unexpected SyndicationItemProperty value");
                    return XmlConstants.SyndTitle;
            }
        }

        /// <summary>
        /// Checks whether the provider implements IUpdatable.
        /// </summary>
        /// <returns>returns true if the provider implements IUpdatable. otherwise returns false.</returns>
        internal override bool ImplementsIUpdatable()
        {
            return true;
        }

        /// <summary>
        /// Get the list of etag property names given the entity set name and the instance of the resource
        /// </summary>
        /// <param name="containerName">name of the entity set</param>
        /// <param name="resourceType">Type of the resource whose etag properties need to be fetched</param>
        /// <returns>list of etag property names</returns>
        internal IList<ResourceProperty> GetETagProperties(string containerName, ResourceType resourceType)
        {
            Debug.Assert(!String.IsNullOrEmpty(containerName), "container name must not be empty");
            Debug.Assert(resourceType != null, "Type cannot be null");

            EntitySetBase entitySet = this.GetEntitySet(containerName);
            EntityType entityType = this.ObjectContext.MetadataWorkspace.GetItem<EntityType>(resourceType.FullName, DataSpace.CSpace);
            Debug.Assert(entityType != null, "entityType != null");
            List<ResourceProperty> etagProperties = new List<ResourceProperty>();

            // Workspace associated directly with the ObjectContext has metadata only about OSpace, CSpace and OCSpace.
            // Since GetRequiredOriginalValueMembers depends on mapping information (CSSpace metadata),
            // we need to make sure we call this API on a workspace which has information about the CS Mapping.
            // Hence getting workspace from the underlying Entity connection.
            MetadataWorkspace workspace = ((EntityConnection)this.ObjectContext.Connection).GetMetadataWorkspace();

            #pragma warning disable 618

            foreach (EdmMember member in workspace.GetRequiredOriginalValueMembers(entitySet, entityType))
            {
                ResourceProperty property = resourceType.TryResolvePropertyName(member.Name);
                Debug.Assert(property != null, "property != null");
                Debug.Assert(property.TypeKind == ResourceTypeKind.Primitive, "property.TypeKind == ResourceTypeKind.Primitive");

                // Ignore key properties if they are part of etag, since the uri already has the key information
                // and it makes no sense to duplicate them in etag
                if (!property.IsOfKind(ResourcePropertyKind.Key))
                {
                    etagProperties.Add(property);
                }
            }

            #pragma warning restore 618

            return etagProperties;
        }

        /// <summary>Checks that the applied configuration is consistent.</summary>
        /// <param name='dataSourceInstance'>Instance of the data source for the provider.</param>
        /// <param name="configuration">Data service configuration instance with access right info.</param>
        /// <remarks>At this point in initialization, metadata trimming hasn't taken place.</remarks>
        protected override void CheckConfigurationConsistency(object dataSourceInstance, DataServiceConfiguration configuration)
        {
            base.CheckConfigurationConsistency(dataSourceInstance, configuration);

            // Check that rights are consistent in MEST scenarios.
            //
            // Strictly we should only check for consistent visibility
            // for all entity sets of a given type, however the current
            // metadata design doesn't differentiate between resource 
            // container types and resource set instances on
            // associations, and therefore all rights are checked at 
            // the resource type level, which forces this check to have 
            // consistent rights.
            //
            // The only exception could be references which are not connected
            // (technically those that are not targets, but in EDM all
            // associations are two-way). These can have entity sets
            // with different rights, enforced at the container level.

            // Discover connected types.
            HashSet<ResourceType> connectedTypes = new HashSet<ResourceType>(EqualityComparer<ResourceType>.Default);
            foreach (ResourceType type in ((IDataServiceMetadataProvider)this).Types)
            {
                foreach (ResourceProperty property in type.PropertiesDeclaredOnThisType)
                {
                    if (property.TypeKind == ResourceTypeKind.EntityType)
                    {
                        connectedTypes.Add(property.ResourceType);
                    }
                }
            }

            // Discover containers of same type with conflicting rights.
            Dictionary<ResourceType, ResourceSet> typeRights = new Dictionary<ResourceType, ResourceSet>(ReferenceEqualityComparer<ResourceType>.Instance);
            foreach (KeyValuePair<string, ResourceSet> containerEntry in this.EntitySets)
            {
                Debug.Assert(containerEntry.Key != null, "containerEntry.Key != null");
                Debug.Assert(containerEntry.Value != null, "containerEntry.Value != null");

                ResourceType resourceType = containerEntry.Value.ResourceType;

                // Disregard types that are not connected to any other types.
                if (!connectedTypes.Contains(resourceType))
                {
                    continue;
                }

                ResourceSet previouslyFoundContainer;
                if (typeRights.TryGetValue(resourceType, out previouslyFoundContainer))
                {
                    EntitySetRights containerRights = configuration.GetResourceSetRights(containerEntry.Value);
                    EntitySetRights previouslyFoundContainerRights = configuration.GetResourceSetRights(previouslyFoundContainer);
                    if (containerRights != previouslyFoundContainerRights)
                    {
                        throw new InvalidOperationException(Strings.ObjectContext_DifferentContainerRights(
                            previouslyFoundContainer.Name,
                            previouslyFoundContainerRights,
                            containerEntry.Value.Name,
                            containerRights));
                    }
                }
                else
                {
                    typeRights.Add(resourceType, containerEntry.Value);
                }
            }

            CheckNavigationPropertiesBound(dataSourceInstance);
        }

        /// <summary>
        /// Populates metadata from the given object context
        /// </summary>
        /// <param name="knownTypes">dictionary of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of already known entity sets</param>
        protected override void PopulateMetadata(
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IDictionary<string, ResourceSet> entitySets)
        {
            Debug.Assert(knownTypes != null, "knownTypes != null");
            Debug.Assert(entitySets != null, "entitySets != null");
            Debug.Assert(this.ObjectContext != null, "this.ObjectContext != null");

            InitializeObjectItemCollection(this.ObjectContext, this.Type.Assembly);
            MetadataWorkspace metadataWorkspace = this.ObjectContext.MetadataWorkspace;

            // Create Resource types for all the top level entity types and complexTypes
            foreach (StructuralType edmType in metadataWorkspace.GetItems<StructuralType>(DataSpace.CSpace))
            {
                if (edmType.BuiltInTypeKind == BuiltInTypeKind.EntityType ||
                    edmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType)
                {
                    // Populates metadata for the given types and all its base types
                    if (PopulateTypeMetadata(metadataWorkspace, edmType, knownTypes, childTypes) == null)
                    {
                        this.typesWithoutOSpaceMetadata.Add(edmType);
                    }
                }
            }

            foreach (EntityContainer entityContainer in metadataWorkspace.GetItems<EntityContainer>(DataSpace.CSpace))
            {
                bool defaultEntityContainer = entityContainer.Name == this.ObjectContext.DefaultContainerName;

                // Get the list of entity sets (Ignore the relationship sets, since we won't allow that to be queried directly
                foreach (EntitySetBase entitySetBase in entityContainer.BaseEntitySets)
                {
                    // Ignore all the association sets for the type being, since we are caching only entity sets
                    if (entitySetBase.BuiltInTypeKind != BuiltInTypeKind.EntitySet)
                    {
                        continue;
                    }

                    EntitySet entitySet = (EntitySet)entitySetBase;
                    Type elementType = GetClrTypeForCSpaceType(metadataWorkspace, entitySet.ElementType);
                    ResourceType resourceType = knownTypes[elementType];
                    string entitySetName = GetEntitySetName(entitySet.Name, entitySet.EntityContainer.Name, defaultEntityContainer);
                    ResourceSet resourceContainer = new ResourceSet(entitySetName, resourceType);
                    entitySets.Add(entitySetName, resourceContainer);
                }
            }

            // Now go and populate the member information for each resource type
            foreach (ResourceType resourceType in knownTypes.Values)
            {
                if (resourceType.ResourceTypeKind == ResourceTypeKind.Primitive)
                {
                    continue;
                }

                PopulateMemberMetadata(resourceType, metadataWorkspace, knownTypes);
                this.GetEpmInfoForResourceType(metadataWorkspace, resourceType, resourceType);
                resourceType.EpmInfoInitialized = true;
            }
        }

        /// <summary>
        /// Creates the object query for the given resource set and returns it
        /// </summary>
        /// <param name="resourceContainer">resource set for which IQueryable instance needs to be created</param>
        /// <returns>returns the IQueryable instance for the given resource set</returns>
        protected override IQueryable GetResourceContainerInstance(ResourceSet resourceContainer)
        {
            Debug.Assert(resourceContainer != null, "resourceContainer != null");
            ObjectQuery result = this.InternalGetResourceContainerInstance(resourceContainer);
            result.MergeOption = MergeOption.NoTracking;
            return result;
        }

        /// <summary>
        /// Populate types for metadata specified by the provider
        /// </summary>
        /// <param name="userSpecifiedTypes">list of types specified by the provider</param>
        /// <param name="knownTypes">list of already known types</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets as specified in the data source type</param>
        protected override void PopulateMetadataForUserSpecifiedTypes(
            IEnumerable<Type> userSpecifiedTypes,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IEnumerable<ResourceSet> entitySets)
        {
            foreach (Type type in userSpecifiedTypes)
            {
                if (this.PopulateMetadataForType(type, knownTypes, childTypes, entitySets) == null)
                {
                    throw new InvalidOperationException(Strings.BadProvider_InvalidTypeSpecified(type.FullName));
                }
            }

            // If there is a type in the model, for which we couldn't load the metadata, we should throw.
            if (this.typesWithoutOSpaceMetadata.Count != 0)
            {
                throw new InvalidOperationException(Strings.ObjectContext_UnableToLoadMetadataForType(this.typesWithoutOSpaceMetadata[0].FullName));
            }

            this.typesWithoutOSpaceMetadata = null;
        }

        /// <summary>
        /// Populate metadata for the given clr type.
        /// </summary>
        /// <param name="type">type whose metadata needs to be loaded.</param>
        /// <param name="knownTypes">list of already known resource types.</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <param name="entitySets">list of entity sets as specified in the data source.</param>
        /// <returns>resource type containing metadata for the given clr type.</returns>
        protected override ResourceType PopulateMetadataForType(
            Type type,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes,
            IEnumerable<ResourceSet> entitySets)
        {
            Debug.Assert(!WebUtil.IsPrimitiveType(type), "Why are we trying to load metadata for a primitive type?");

            ResourceType resourceType;
            if (!knownTypes.TryGetValue(type, out resourceType))
            {
                InitializeObjectItemCollection(this.ObjectContext, type.Assembly);
                ObjectItemCollection objectItemCollection = (ObjectItemCollection)this.ObjectContext.MetadataWorkspace.GetItemCollection(DataSpace.OSpace);
                StructuralType ospaceType, cspaceType;
                if (objectItemCollection.TryGetItem<StructuralType>(type.FullName, out ospaceType))
                {
                    if (this.ObjectContext.MetadataWorkspace.TryGetEdmSpaceType(ospaceType, out cspaceType))
                    {
                        ResourceType baseType = null;
                        if (cspaceType.BaseType != null)
                        {
                            baseType = this.PopulateMetadataForType(type.BaseType, knownTypes, childTypes, entitySets);
                        }

                        resourceType = CreateResourceType(cspaceType, type, baseType, knownTypes, childTypes);
                        this.typesWithoutOSpaceMetadata.Remove(cspaceType);
                    }
                }
            }

            return resourceType;
        }

        /// <summary>
        /// Returns the resource type for the corresponding clr type.
        /// </summary>
        /// <param name="type">clrType whose corresponding resource type needs to be returned</param>
        /// <returns>Returns the resource type</returns>
        protected override ResourceType ResolveNonPrimitiveType(System.Type type)
        {
            Type actualType = ObjectContext.GetObjectType(type);
            return base.ResolveNonPrimitiveType(actualType);
        }

        /// <summary>
        /// Checks that all navigation properties are bound to some association set for every entity set.
        /// </summary>
        /// <param name='dataSourceInstance'>Instance of the data source for the provider.</param>
        private static void CheckNavigationPropertiesBound(object dataSourceInstance)
        {
            // For every navigation property, ensure that all of the EntitySets that can
            // take their EntityType have an AssociationSet of the appropriate Association type.
            Debug.Assert(dataSourceInstance != null, "dataSourceInstance != null");
            MetadataWorkspace workspace = ((ObjectContext)dataSourceInstance).MetadataWorkspace;
            foreach (EntityType type in workspace.GetItems<EntityType>(DataSpace.CSpace))
            {
                foreach (NavigationProperty navigationProperty in type.NavigationProperties)
                {
                    foreach (EntitySet entitySet in GetEntitySetsForType(workspace, type))
                    {
                        IEnumerable<EntitySet> entitySetsWithAssocation = GetEntitySetsWithAssociationSets(
                            workspace,
                            navigationProperty.RelationshipType,
                            navigationProperty.FromEndMember);
                        if (!entitySetsWithAssocation.Contains(entitySet))
                        {
                            throw new InvalidOperationException(Strings.ObjectContext_NavigationPropertyUnbound(
                                navigationProperty.Name,
                                type.FullName,
                                entitySet.Name));
                        }
                    }
                }
            }
        }

        /// <summary>Gets the CLR type mapped to the specified C-Space type.</summary>
        /// <param name="workspace">Workspace in which the type is defined.</param>
        /// <param name="edmType">C-Space type whose matching clr type needs to be looked up.</param>
        /// <returns>The resolved <see cref="Type"/> for the given <paramref name="edmType"/>.</returns>
        private static Type GetClrTypeForCSpaceType(MetadataWorkspace workspace, StructuralType edmType)
        {
            Debug.Assert(workspace != null, "workspace != null");
            Debug.Assert(edmType != null, "edmType != null");
            Debug.Assert(
               edmType.BuiltInTypeKind == BuiltInTypeKind.EntityType || edmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType,
               "Must be entityType or complexType");

            StructuralType ospaceType;
            if (workspace.TryGetObjectSpaceType(edmType, out ospaceType))
            {
                ObjectItemCollection objectItemCollection = (ObjectItemCollection)workspace.GetItemCollection(DataSpace.OSpace);
                return objectItemCollection.GetClrType(ospaceType);
            }

            return null;
        }

        /// <summary>
        /// Gets all <see cref="EntitySet"/> instance that may hold an entity of type <paramref name="type"/>.
        /// </summary>
        /// <param name="workspace">Workspace with metadata.</param>
        /// <param name="type">Entity type to get entity sets for.</param>
        /// <returns>An enumeration of <see cref="EntitySet"/> instances that can hold <paramref name="type"/>.</returns>
        private static IEnumerable<EntitySet> GetEntitySetsForType(MetadataWorkspace workspace, EntityType type)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(workspace != null, "workspace != null");
            foreach (EntityContainer container in workspace.GetItems<EntityContainer>(DataSpace.CSpace))
            {
                foreach (EntitySet entitySet in container.BaseEntitySets.OfType<EntitySet>())
                {
                    if (IsAssignableFrom(entitySet.ElementType, type))
                    {
                        yield return entitySet;
                    }
                }
            }
        }

        /// <summary>
        /// Gets all entity sets that participate as members for the specified <paramref name="associationType"/>.
        /// </summary>
        /// <param name="workspace">Workspace with metadata.</param>
        /// <param name="associationType">Type of assocation to check.</param>
        /// <param name="member">Member of association to check.</param>
        /// <returns>
        /// All <see cref="EntitySet"/> instances that are are on the <paramref name="member"/> role for 
        /// some association of <paramref name="associationType"/>.
        /// </returns>
        private static IEnumerable<EntitySet> GetEntitySetsWithAssociationSets(
            MetadataWorkspace workspace,
            RelationshipType associationType,
            RelationshipEndMember member)
        {
            Debug.Assert(workspace != null, "workspace != null");
            Debug.Assert(associationType != null, "associationType != null");
            Debug.Assert(member != null, "member != null");
            foreach (EntityContainer container in workspace.GetItems<EntityContainer>(DataSpace.CSpace))
            {
                foreach (AssociationSet associationSet in container.BaseEntitySets.OfType<AssociationSet>())
                {
                    if (associationSet.ElementType == associationType)
                    {
                        foreach (AssociationSetEnd end in associationSet.AssociationSetEnds)
                        {
                            if (end.CorrespondingAssociationEndMember == member)
                            {
                                yield return end.EntitySet;
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Checks the <paramref name="type"/> and all of its base types for the HasStream attribute.
        /// </summary>
        /// <param name="type">Type to check</param>
        /// <returns>True if HasStream="true" stream property is defined on the given entity type or any of its base types.</returns>
        private static bool GetDefaultStreamPropertyFromEntityTypeHierarchy(StructuralType type)
        {            
            while (type != null)
            {
                if (GetEntityTypeDefaultStreamProperty(type))
                {
                    return true;
                }

                type = type.BaseType as StructuralType;
            }

            return false;
        }

        /// <summary>Reads the HasStream attribute from the specified <paramref name="type"/>.</summary>
        /// <param name="type">Type to read attribute from.</param>
        /// <returns>True if HasStream="true" stream property is defined for the entity type.</returns>
        private static bool GetEntityTypeDefaultStreamProperty(StructuralType type)
        {
            Debug.Assert(type != null, "type != null");

            const string HasStreamAttribute = XmlConstants.DataWebMetadataNamespace + ":" + XmlConstants.DataWebAccessHasStreamAttribute;
            bool result = false;
            MetadataProperty property;
            if (type.MetadataProperties.TryGetValue(HasStreamAttribute, false /* ignoreCase */, out property))
            {
                string text = (string)property.Value;
                if (String.IsNullOrEmpty(text))
                {
                    throw new InvalidOperationException(Strings.ObjectContext_HasStreamAttributeEmpty(type.Name));
                }

                if (text != XmlConstants.DataWebAccessDefaultStreamPropertyValue)
                {
                    // In the future we might support multiple stream properties. For now we only support $default.
                    throw new NotSupportedException(Strings.ObjectContext_UnsupportedStreamProperty(text, type.Name));
                }

                result = true;
            }

            return result;
        }

        /// <summary>Sets the MIME type, if specified for the specified member.</summary>
        /// <param name="resourceProperty">resource property whose mime type needs to be updated.</param>
        /// <param name="csdlMember">C-Space member for which we need to find the C-Space mime type attribute.</param>
        private static void SetMimeTypeForMappedMember(ResourceProperty resourceProperty, EdmMember csdlMember)
        {
            const string MimePropertyName = XmlConstants.DataWebMetadataNamespace + ":" + XmlConstants.DataWebMimeTypeAttributeName;
            MetadataProperty property;
            if (csdlMember.MetadataProperties.TryGetValue(MimePropertyName, false /* ignoreCase */, out property))
            {
                string mimeType = (string)property.Value;
                resourceProperty.MimeType = mimeType;
            }
        }

        /// <summary>Generic method to invoke an Include method on an ObjectQuery source.</summary>
        /// <typeparam name="T">Element type of the source.</typeparam>
        /// <param name="query">Source query.</param>
        /// <param name="dottedPath">Path to include.</param>
        /// <returns>A new query that includes <paramref name="dottedPath"/> in <paramref name="query"/>.</returns>
        private static ObjectQuery<T> Include<T>(IQueryable query, string dottedPath)
        {
            Debug.Assert(query != null, "query != null");
            Debug.Assert(dottedPath != null, "dottedPath != null");

            ObjectQuery<T> typedQuery = (ObjectQuery<T>)query;
            return typedQuery.Include(dottedPath);
        }

        /// <summary>Checks whether <paramref name="derivedType"/> may be assigned to <paramref name="baseType"/>.</summary>
        /// <param name="baseType">Type to check assignment to.</param>
        /// <param name="derivedType">Type to check assignment from.</param>
        /// <returns>
        /// true if an instance of <paramref name="derivedType" /> can be assigned to a variable of 
        /// <paramref name="baseType"/>; false otherwise.
        /// </returns>
        private static bool IsAssignableFrom(EntityType baseType, EntityType derivedType)
        {
            while (derivedType != null)
            {
                if (derivedType == baseType)
                {
                    return true;
                }

                derivedType = (EntityType)derivedType.BaseType;
            }

            return false;
        }

        /// <summary>Checks whether the specified type is a known primitive type.</summary>
        /// <param name="type">Type to check.</param>
        /// <returns>true if the specified type is known to be a primitive type; false otherwise.</returns>
        private static bool IsPrimitiveType(EdmType type)
        {
            Debug.Assert(type != null, "type != null");
            if (type.BuiltInTypeKind != BuiltInTypeKind.PrimitiveType)
            {
                return false;
            }
            else
            {
                Debug.Assert(
                    WebUtil.IsPrimitiveType(((PrimitiveType)type).ClrEquivalentType),
                    "WebUtil.IsPrimitiveType(((PrimitiveType)type).ClrEquivalentType) - all EDM primitive types are Astoria primitive types");
                return true;
            }
        }

        /// <summary>Joins the list of segment identifiers by dots.</summary>
        /// <param name='segments'>List of segments to join.</param>
        /// <returns>A string with the identifiers joined by dots.</returns>
        private static string JoinIdentifiers(List<string> segments)
        {
            Debug.Assert(segments != null, "segments != null");
            
            int capacity = 0;
            foreach (string segment in segments)
            {
                capacity += segment.Length;
            }

            capacity += segments.Count - 1;

            StringBuilder builder = new StringBuilder(capacity);
            foreach (string segment in segments)
            {
                if (builder.Length > 0)
                {
                    builder.Append('.');
                }

                builder.Append(segment);
            }

            return builder.ToString();
        }

        /// <summary>Walks all the expansion paths in the subtree of the <paramref name="expandedNode"/>
        /// and will execute the <paramref name="action"/> for each of the unique paths with the path
        /// serialized as a dotted string as its argument.</summary>
        /// <typeparam name="T">The type of the state which is modified by the actions.</typeparam>
        /// <param name="expandedNode">The root node to start from.</param>
        /// <param name="action">The action to execute for each unique path.</param>
        /// <param name="state">The state object to pass to the action.</param>
        /// <param name="pathSegments">List of path segments so far (including the <paramref name="expandedNode"/>)</param>
        /// <returns>The modified state as a result to call to all the actions.</returns>
        private static T VisitDottedExpandPaths<T>(ExpandedProjectionNode expandedNode, Func<T, string, T> action, T state, List<string> pathSegments)
        {
            bool foundChildExpansionNode = false;
            Debug.Assert(
                expandedNode.PropertyName.Length == 0 || pathSegments[pathSegments.Count - 1] == expandedNode.PropertyName,
                "The last segment doesn't match the node the function was called on.");

            foreach (ProjectionNode node in expandedNode.Nodes)
            {
                ExpandedProjectionNode childExpandedNode = node as ExpandedProjectionNode;
                if (childExpandedNode != null)
                {
                    foundChildExpansionNode = true;

                    pathSegments.Add(childExpandedNode.PropertyName);
                    state = VisitDottedExpandPaths(childExpandedNode, action, state, pathSegments);
                    pathSegments.RemoveAt(pathSegments.Count - 1);
                }
            }

            if (!foundChildExpansionNode && pathSegments.Count > 0)
            {
                state = action(state, JoinIdentifiers(pathSegments));
            }

            return state;
        }

        /// <summary>
        /// Populates the metadata for the given type and its base type
        /// </summary>
        /// <param name="workspace">metadata workspace containing all the metadata information</param>
        /// <param name="edmType"> type whose metadata needs to be populated </param>
        /// <param name="knownTypes">list of known types </param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <returns>returns the resource type corresponding to the given edmType</returns>
        private static ResourceType PopulateTypeMetadata(
            MetadataWorkspace workspace,
            StructuralType edmType,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes)
        {
            Debug.Assert(
                edmType.BuiltInTypeKind == BuiltInTypeKind.EntityType ||
                edmType.BuiltInTypeKind == BuiltInTypeKind.ComplexType, 
                "type must be entity or complex type");

            ResourceType resourceType = null;
            Type clrType = GetClrTypeForCSpaceType(workspace, edmType);
            if (clrType != null && !knownTypes.TryGetValue(clrType, out resourceType))
            {
                ResourceType baseType = null;
                if (edmType.BaseType != null)
                {
                    baseType = PopulateTypeMetadata(workspace, (StructuralType)edmType.BaseType, knownTypes, childTypes);
                }

                resourceType = CreateResourceType(edmType, clrType, baseType, knownTypes, childTypes);
            }

            return resourceType;
        }

        /// <summary>
        /// Creates a new instance of resource type given the cspace structural type and mapping clr type.
        /// </summary>
        /// <param name="cspaceType">cspace structural type.</param>
        /// <param name="clrType">mapping clr type for the given structural type.</param>
        /// <param name="baseResourceType">the base resource type for the given resource type.</param>
        /// <param name="knownTypes">list of already known resource types.</param>
        /// <param name="childTypes">list of already known types and their immediate children</param>
        /// <returns>the new resource type instance created for the given cspace type.</returns>
        private static ResourceType CreateResourceType(
            StructuralType cspaceType,
            Type clrType,
            ResourceType baseResourceType,
            IDictionary<Type, ResourceType> knownTypes,
            IDictionary<ResourceType, List<ResourceType>> childTypes)
        {
            ResourceTypeKind resourceTypeKind = cspaceType.BuiltInTypeKind == BuiltInTypeKind.EntityType ? ResourceTypeKind.EntityType : ResourceTypeKind.ComplexType;
            
            // We do not support open types in Object Context provider yet.
            ResourceType resourceType = new ResourceType(clrType, resourceTypeKind, baseResourceType, cspaceType.NamespaceName, cspaceType.Name, clrType.IsAbstract);
            if (GetDefaultStreamPropertyFromEntityTypeHierarchy(cspaceType))
            {
                resourceType.IsMediaLinkEntry = true;
            }

            knownTypes.Add(clrType, resourceType);
            childTypes.Add(resourceType, null);
            if (baseResourceType != null)
            {
                Debug.Assert(childTypes.ContainsKey(baseResourceType), "childTypes.ContainsKey(baseResourceType)");
                if (childTypes[baseResourceType] == null)
                {
                    childTypes[baseResourceType] = new List<ResourceType>();
                }

                childTypes[baseResourceType].Add(resourceType);
            }

            return resourceType;
        }

        /// <summary>
        /// Populates the member metadata for the given type
        /// </summary>
        /// <param name="resourceType">resource type whose member metadata needs to be filled</param>
        /// <param name="workspace">workspace containing the metadata information</param>
        /// <param name="knownTypes">list of already known types</param>
        private static void PopulateMemberMetadata(
            ResourceType resourceType,
            MetadataWorkspace workspace,
            IDictionary<Type, ResourceType> knownTypes)
        {
            Debug.Assert(resourceType != null, "resourceType != null");
            Debug.Assert(workspace != null, "workspace != null");

            // Find the type from the OSpace
            StructuralType edmType = workspace.GetItem<StructuralType>(resourceType.FullName, DataSpace.CSpace);
            foreach (EdmMember member in edmType.Members)
            {
                // only look at the instance members
                if (member.DeclaringType != edmType)
                {
                    continue;
                }

                ResourcePropertyKind kind = (ResourcePropertyKind)(-1);
                PropertyInfo propertyInfo = resourceType.InstanceType.GetProperty(member.Name);
                ResourceType propertyType = null;
                switch (member.TypeUsage.EdmType.BuiltInTypeKind)
                {
                    case BuiltInTypeKind.PrimitiveType:
                        propertyType = ResourceType.GetPrimitiveResourceType(propertyInfo.PropertyType);

                        if (propertyType == null)
                        {
                            throw new NotSupportedException(Strings.ObjectContext_PrimitiveTypeNotSupported(member.Name, edmType.Name, member.TypeUsage.EdmType.Name));
                        }

                        if (edmType.BuiltInTypeKind == BuiltInTypeKind.EntityType &&
                            ((EntityType)edmType).KeyMembers.Contains(member))
                        {
                            kind = ResourcePropertyKind.Key | ResourcePropertyKind.Primitive;
                        }
                        else
                        {
                            kind = ResourcePropertyKind.Primitive;
                        }

                        break;
                    case BuiltInTypeKind.ComplexType:
                        kind = ResourcePropertyKind.ComplexType;
                        propertyType = knownTypes[propertyInfo.PropertyType];
                        break;
                    case BuiltInTypeKind.EntityType:
                        kind = ResourcePropertyKind.ResourceReference;
                        propertyType = knownTypes[propertyInfo.PropertyType];
                        break;
                    case BuiltInTypeKind.CollectionType:
                        kind = ResourcePropertyKind.ResourceSetReference;
                        Type propertyClrType = GetClrTypeForCSpaceType(workspace, (EntityType)((CollectionType)member.TypeUsage.EdmType).TypeUsage.EdmType);
                        Debug.Assert(!WebUtil.IsPrimitiveType(propertyClrType), "We don't support collections of primitives, we shouldn't see one here");
                        propertyType = knownTypes[propertyClrType];
                        break;
                    default:
                        Debug.Assert(false, "Invalid member type encountered on " + member.Name + " - " + member.TypeUsage.EdmType.BuiltInTypeKind);
                        break;
                }

                Debug.Assert(propertyType != null, "propertyType != null");
                ResourceProperty resourceProperty = new ResourceProperty(propertyInfo.Name, kind, propertyType);
                SetMimeTypeForMappedMember(resourceProperty, member);
                resourceType.AddProperty(resourceProperty);
            }
        }

        /// <summary>
        /// Finds a non-syndication mapping related property from the collection of extended properties from an EFx resource property
        /// </summary>
        /// <param name="metadataExtendedProperties">Collection of metadata extended properties of <paramref name="memberName"/></param>        
        /// <param name="currentlyAllowed">Flag indicating whether the extended property should exist in the current context</param>
        /// <param name="epmPropertyName">Name of the property of resource type</param>
        /// <param name="typeName">Type to which the property belongs</param>
        /// <param name="memberName">Name of the member whose extended properties we are searching from</param>
        /// <returns>The corresponding MetadataProperty object if found, null otherwise</returns>
        private static MetadataProperty GetNonSyndicationExtendedProperty(
            IEnumerable<MetadataProperty> metadataExtendedProperties, 
            bool currentlyAllowed, 
            String epmPropertyName,
            String typeName, 
            String memberName)
        {
            MetadataProperty epmProperty = FindSingletonExtendedProperty(metadataExtendedProperties, epmPropertyName, typeName, memberName);
            if (epmProperty != null)
            {
                if (!currentlyAllowed)
                {
                    throw new InvalidOperationException(memberName == null ? 
                                    Strings.ObjectContext_InvalidAttributeForNonSyndicationItemsType(epmPropertyName, typeName) :
                                    Strings.ObjectContext_InvalidAttributeForNonSyndicationItemsMember(epmPropertyName, memberName, typeName));
                }
            }

            return epmProperty;
        }

        /// <summary>Gets the NavigationProperty for the member of a set type if available.</summary>
        /// <param name="set">Set for which to return navigation property.</param>
        /// <param name="member">Relationship end member for the navigation property.</param>
        /// <returns>The NavigationProperty for the member of a set type if available; null otherwise.</returns>
        private static NavigationProperty PropertyForEnd(EntitySet set, RelationshipEndMember member)
        {
            Debug.Assert(set != null, "set != null");
            Debug.Assert(member != null, "member != null");
            foreach (NavigationProperty p in set.ElementType.NavigationProperties)
            {
                if (p.FromEndMember == member)
                {
                    return p;
                }
            }
            
            return null;
        }

        /// <summary>Creates a string array with property names.</summary>
        /// <param name="properties">Properties to include in name.</param>
        /// <returns>A string array with property names.</returns>
        private static string[] PropertyNamesToStrings(ReadOnlyMetadataCollection<EdmProperty> properties)
        {
            Debug.Assert(properties != null, "properties != null");
            string[] result = new string[properties.Count];
            for (int i = 0; i < properties.Count; i++)
            {
                result[i] = properties[i].Name;
            }
            
            return result;
        }

        /// <summary>
        /// Write the child element of the referential constraint
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="nodeName">name of the xmlnode : Principal or Dependent</param>
        /// <param name="roleName">role name</param>
        /// <param name="properties">list of properties</param>
        private static void WriteReferentialConstraintChildElement(
            XmlWriter xmlWriter,
            string nodeName,
            string roleName,
            ReadOnlyMetadataCollection<EdmProperty> properties)
        {
            // Write the principal role
            xmlWriter.WriteStartElement(nodeName);
            xmlWriter.WriteAttributeString(XmlConstants.Role, roleName);

            foreach (EdmProperty property in properties)
            {
                xmlWriter.WriteStartElement(XmlConstants.PropertyRef);
                xmlWriter.WriteAttributeString(XmlConstants.Name, property.Name);
                xmlWriter.WriteEndElement();
            }

            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Write the metadata for the given facets in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="typeUsage">typeusage whose metadata needs to be written</param>
        private static void WriteFacets(XmlWriter xmlWriter, TypeUsage typeUsage)
        {
            foreach (Facet facet in typeUsage.Facets)
            {
                if (facet.Value != null)
                {
                    WriteFacetValue(xmlWriter, facet.Name, facet.Value);
                }
            }
        }

        /// <summary>
        /// Write the metadata for the given facet
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="facetName">name of the facet</param>
        /// <param name="facetValue">value of the facet</param>
        private static void WriteFacetValue(XmlWriter xmlWriter, string facetName, object facetValue)
        {
            Type facetValueType = facetValue.GetType();

            // We need to special case the boolean facets, since ToString returns True and False as values
            // and for xml, they need to be lower case
            if (facetValueType == typeof(bool))
            {
                if ((bool)facetValue)
                {
                    xmlWriter.WriteAttributeString(facetName, XmlConstants.XmlTrueLiteral);
                }
                else
                {
                    xmlWriter.WriteAttributeString(facetName, XmlConstants.XmlFalseLiteral);
                }
            }
            else if (facetValueType == typeof(int))
            {
                xmlWriter.WriteAttributeString(facetName, XmlConvert.ToString((int)facetValue));
            }
            else if (facetValueType.IsEnum)
            {
                xmlWriter.WriteAttributeString(facetName, facetValue.ToString());
            }
            else if (facetValueType == typeof(byte))
            {
                xmlWriter.WriteAttributeString(facetName, XmlConvert.ToString((byte)facetValue));
            }
            else if (facetValueType == typeof(DateTime))
            {
                xmlWriter.WriteAttributeString(facetName, XmlConvert.ToString((DateTime)facetValue, "yyyy-MM-dd HH:mm:ss.fffZ"));
            }
            else
            {
                xmlWriter.WriteAttributeString(facetName, facetValue.ToString());
            }
        }

        /// <summary>
        /// Write the user annotations to the csdl
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="metadataProperties">list of metadata properties from which extended ones needs to be written</param>
        /// <param name="writeCustomElements">if true, this function writes out the custom elements, otherwise it writes the custom attributes to the given xml writer.</param>
        private static void WriteUserDefinedAnnotations(
            XmlWriter xmlWriter,
            ReadOnlyMetadataCollection<MetadataProperty> metadataProperties,
            bool writeCustomElements)
        {
            Debug.Assert(xmlWriter != null, "xmlWriter != null");
            Debug.Assert(metadataProperties != null, "metadataProperties != null");
            Debug.Assert(
                (!writeCustomElements && xmlWriter.WriteState == WriteState.Element) ||
                (writeCustomElements && xmlWriter.WriteState == WriteState.Element || xmlWriter.WriteState == WriteState.Content), 
                "For custom attributes, the write state must be element. For custom elements, writer can be in element or content state");

            foreach (MetadataProperty metadataProperty in metadataProperties)
            {
                if (metadataProperty.PropertyKind == PropertyKind.Extended)
                {
                    // Temporary fix for beta1 to ignore the store generated pattern attribute, since
                    // that causes V1 code gen to fail
                    if (metadataProperty.Name.Equals(XmlConstants.StoreGeneratedPattern))
                    {
                        continue;
                    }

                    int index = metadataProperty.Name.LastIndexOf(":", StringComparison.Ordinal);
                    Debug.Assert(index != -1, "empty space is a reserved namespace for edm. So this should never be the case");
                    string xmlNamespace = metadataProperty.Name.Substring(0, index);
                    string attributeName = metadataProperty.Name.Substring(index + 1);

                    if (!writeCustomElements && (metadataProperty.Value == null || metadataProperty.Value.GetType() != typeof(XElement)))
                    {
                        xmlWriter.WriteAttributeString(attributeName, xmlNamespace, (string)metadataProperty.Value);
                    }
                    else if (writeCustomElements && metadataProperty.Value != null && metadataProperty.Value.GetType() == typeof(XElement))
                    {
                        XElement annotation = (XElement)metadataProperty.Value;
                        annotation.WriteTo(xmlWriter);
                    }
                }
            }
        }

        /// <summary>
        /// Returns the entity set name for the given entity set. If this entity set belongs to the default container name,
        /// then it returns the entity set name, otherwise qualifies it with the entitycontainer name
        /// </summary>
        /// <param name="entitySetName">entity set name</param>
        /// <param name="entityContainerName">entity container name</param>
        /// <param name="containedInDefaultEntityContainer">true if the given entity set belongs to the default entity container</param>
        /// <returns>returns the entity set name</returns>
        private static string GetEntitySetName(string entitySetName, string entityContainerName, bool containedInDefaultEntityContainer)
        {
            if (containedInDefaultEntityContainer)
            {
                return entitySetName;
            }
            else
            {
                return entityContainerName + "." + entitySetName;
            }
        }

        /// <summary>
        /// Returns the escaped entity set name for the given entity set. If this entity set belongs to the default container name,
        /// then it returns the escaped entity set name, otherwise it escapes both the container and set name
        /// </summary>
        /// <param name="qualifiedEntitySetName">qualified entity set name whose name needs to be escaped</param>
        /// <returns>returns the escaped entityset name</returns>
        private static string GetEscapedEntitySetName(string qualifiedEntitySetName)
        {
            int indexOfLastPeriod = qualifiedEntitySetName.LastIndexOf('.');

            if (-1 == indexOfLastPeriod)
            {
                return "[" + qualifiedEntitySetName + "]";
            }
            else
            {
                return "[" + qualifiedEntitySetName.Substring(0, indexOfLastPeriod) + "].[" + qualifiedEntitySetName.Substring(indexOfLastPeriod + 1) + "]";
            }
        }

        /// <summary>
        /// Returns the edm schema multiplicity value for the given multiplicity enum
        /// </summary>
        /// <param name="multiplicity">enum multiplicity value</param>
        /// <returns>returns edm schema multiplicity value</returns>
        private static string GetMultiplicity(RelationshipMultiplicity multiplicity)
        {
            string multiplicityValue;
            if (RelationshipMultiplicity.Many == multiplicity)
            {
                multiplicityValue = XmlConstants.Many;
            }
            else if (RelationshipMultiplicity.One == multiplicity)
            {
                multiplicityValue = XmlConstants.One;
            }
            else
            {
                Debug.Assert(
                    RelationshipMultiplicity.ZeroOrOne == multiplicity,
                    "Invalid value for multiplicity encountered");
                multiplicityValue = XmlConstants.ZeroOrOne;
            }

            return multiplicityValue;
        }

        /// <summary>Initializes metadata for the given object context.</summary>
        /// <param name="objectContext">Instance of data source to use if pure static analysis isn't possible.</param>
        /// <param name="assembly">assembly whose metadata needs to be loaded.</param>
        private static void InitializeObjectItemCollection(ObjectContext objectContext, Assembly assembly)
        {
            objectContext.MetadataWorkspace.LoadFromAssembly(assembly);
        }

        /// <summary>
        /// Write the metadata for the given members in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="declaringType">type whose members metadata needs to be written</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteMembers(XmlWriter xmlWriter, StructuralType declaringType, MetadataManager metadataManager)
        {
            foreach (EdmMember member in metadataManager.GetPropertiesDeclaredInThisType(declaringType))
            {
                if (member.BuiltInTypeKind == BuiltInTypeKind.EdmProperty)
                {
                    xmlWriter.WriteStartElement(XmlConstants.Property);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, member.Name);
                    xmlWriter.WriteAttributeString(XmlConstants.Type, member.TypeUsage.EdmType.FullName);
                    WriteFacets(xmlWriter, member.TypeUsage);
                }
                else
                {
                    Debug.Assert(
                        BuiltInTypeKind.NavigationProperty == member.BuiltInTypeKind,
                        "Invalid Member type encountered");
                    NavigationProperty navProperty = (NavigationProperty)member;
                    xmlWriter.WriteStartElement(XmlConstants.NavigationProperty);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, navProperty.Name);
                    xmlWriter.WriteAttributeString(XmlConstants.Relationship, navProperty.RelationshipType.FullName);
                    xmlWriter.WriteAttributeString(XmlConstants.FromRole, navProperty.FromEndMember.Name);
                    xmlWriter.WriteAttributeString(XmlConstants.ToRole, navProperty.ToEndMember.Name);
                }

                WriteUserDefinedAnnotations(xmlWriter, member.MetadataProperties, false /*writeCustomElements*/);
                WriteUserDefinedAnnotations(xmlWriter, member.MetadataProperties, true /*writeCustomElements*/);
                xmlWriter.WriteEndElement();
            }
        }

        /// <summary>
        /// Write the metadata for the given association set element in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="associationSet">association set whose metadata needs to be written</param>
        private static void WriteAssociationSet(XmlWriter xmlWriter, AssociationSet associationSet)
        {
            xmlWriter.WriteStartElement(XmlConstants.AssociationSet);
            xmlWriter.WriteAttributeString(XmlConstants.Name, associationSet.Name);
            xmlWriter.WriteAttributeString(XmlConstants.Association, associationSet.ElementType.FullName);

            // Write the user defined annotations
            WriteUserDefinedAnnotations(xmlWriter, associationSet.MetadataProperties, false /*writeCustomElements*/);
            for (int i = 0; i < associationSet.AssociationSetEnds.Count; i++)
            {
                xmlWriter.WriteStartElement(XmlConstants.End);
                xmlWriter.WriteAttributeString(XmlConstants.Role, associationSet.AssociationSetEnds[i].Name);
                xmlWriter.WriteAttributeString(XmlConstants.EntitySet, associationSet.AssociationSetEnds[i].EntitySet.Name);

                // Write the user defined annotations
                WriteUserDefinedAnnotations(xmlWriter, associationSet.AssociationSetEnds[i].MetadataProperties, false /*writeCustomElements*/);
                WriteUserDefinedAnnotations(xmlWriter, associationSet.AssociationSetEnds[i].MetadataProperties, true /*writeCustomElements*/);
                xmlWriter.WriteEndElement();
            }

            WriteUserDefinedAnnotations(xmlWriter, associationSet.MetadataProperties, true /*writeCustomElements*/);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Write the entity set element in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="entitySet">entity set whose metadata needs to be written</param>
        private static void WriteEntitySet(XmlWriter xmlWriter, EntitySet entitySet)
        {
            xmlWriter.WriteStartElement(XmlConstants.EntitySet);
            xmlWriter.WriteAttributeString(XmlConstants.Name, entitySet.Name);
            xmlWriter.WriteAttributeString(XmlConstants.EntityType, entitySet.ElementType.FullName);

            // Write the user defined annotations
            WriteUserDefinedAnnotations(xmlWriter, entitySet.MetadataProperties, false /*writeCustomElements*/);
            WriteUserDefinedAnnotations(xmlWriter, entitySet.MetadataProperties, true /*writeCustomElements*/);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Writes the metadata for all the edmTypes
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="edmTypes">edmtypes whose metadata needs to be written</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteEdmTypes(XmlWriter xmlWriter, IEnumerable<EdmType> edmTypes, MetadataManager metadataManager)
        {
            // Top Level EdmTypes are EntityType, ComplexType and AssociationType.
            foreach (EdmType edmType in edmTypes)
            {
                switch (edmType.BuiltInTypeKind)
                {
                    case BuiltInTypeKind.EntityType:
                        WriteEntityType(xmlWriter, (EntityType)edmType, metadataManager);
                        break;
                    case BuiltInTypeKind.ComplexType:
                        WriteComplexType(xmlWriter, (ComplexType)edmType, metadataManager);
                        break;
                    case BuiltInTypeKind.AssociationType:
                        WriteAssociationType(xmlWriter, (AssociationType)edmType);
                        break;
                    default:
                        Debug.Assert(false, "Unexpected EdmType encountered");
                        break;
                }
            }
        }

        /// <summary>
        /// Write the metadata for the given EntityType element in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="entityType">entity type whose metadata needs to be written</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteEntityType(XmlWriter xmlWriter, EntityType entityType, MetadataManager metadataManager)
        {
            xmlWriter.WriteStartElement(XmlConstants.EntityType);
            xmlWriter.WriteAttributeString(XmlConstants.Name, entityType.Name);
            if (entityType.Abstract)
            {
                xmlWriter.WriteAttributeString(XmlConstants.Abstract, XmlConstants.XmlTrueLiteral);
            }

            if (entityType.BaseType != null)
            {
                xmlWriter.WriteAttributeString(XmlConstants.BaseType, entityType.BaseType.FullName);

                // Write the user defined annotations
                WriteUserDefinedAnnotations(xmlWriter, entityType.MetadataProperties, false /*writeCustomElements*/);
            }
            else
            {
                // Write the user defined annotations
                WriteUserDefinedAnnotations(xmlWriter, entityType.MetadataProperties, false /*writeCustomElements*/);

                // If there is no base type, then the key must be defined on this entity type.
                xmlWriter.WriteStartElement(XmlConstants.Key);
                foreach (EdmMember member in entityType.KeyMembers)
                {
                    xmlWriter.WriteStartElement(XmlConstants.PropertyRef);
                    xmlWriter.WriteAttributeString(XmlConstants.Name, member.Name);
                    xmlWriter.WriteEndElement();
                }

                xmlWriter.WriteEndElement();
            }

            WriteMembers(xmlWriter, entityType, metadataManager);
            WriteUserDefinedAnnotations(xmlWriter, entityType.MetadataProperties, true /*writeCustomElements*/);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Write the metadata for the given complex type element in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="complexType">complex type whose metadata needs to be written</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteComplexType(XmlWriter xmlWriter, ComplexType complexType, MetadataManager metadataManager)
        {
            xmlWriter.WriteStartElement(XmlConstants.ComplexType);
            xmlWriter.WriteAttributeString(XmlConstants.Name, complexType.Name);

            // Write the user defined annotations
            WriteUserDefinedAnnotations(xmlWriter, complexType.MetadataProperties, false /*writeCustomElements*/);

            // Write the metadata for complex type properties
            WriteMembers(xmlWriter, complexType, metadataManager);
            WriteUserDefinedAnnotations(xmlWriter, complexType.MetadataProperties, true /*writeCustomElements*/);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Write the metadata for the given association type element in the given xml writer
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="associationType">association type whose metadata needs to be written</param>
        private static void WriteAssociationType(XmlWriter xmlWriter, AssociationType associationType)
        {
            xmlWriter.WriteStartElement(XmlConstants.Association);
            xmlWriter.WriteAttributeString(XmlConstants.Name, associationType.Name);

            // Write the user defined annotations
            WriteUserDefinedAnnotations(xmlWriter, associationType.MetadataProperties, false /*writeCustomElements*/);

            foreach (AssociationEndMember end in associationType.RelationshipEndMembers)
            {
                xmlWriter.WriteStartElement(XmlConstants.End);
                xmlWriter.WriteAttributeString(XmlConstants.Role, end.Name);

                // The ends are of reference type
                xmlWriter.WriteAttributeString(XmlConstants.Type, ((RefType)end.TypeUsage.EdmType).ElementType.FullName);

                // Write the multiplicity value
                xmlWriter.WriteAttributeString(XmlConstants.Multiplicity, GetMultiplicity(end.RelationshipMultiplicity));

                // Write the user defined annotations (only attributes)
                WriteUserDefinedAnnotations(xmlWriter, end.MetadataProperties, false /*writeCustomElements*/);

                // write the action value
                if (OperationAction.None != end.DeleteBehavior)
                {
                    xmlWriter.WriteStartElement(XmlConstants.OnDelete);
                    xmlWriter.WriteAttributeString(XmlConstants.Action, end.DeleteBehavior.ToString());
                    xmlWriter.WriteEndElement();
                }

                WriteUserDefinedAnnotations(xmlWriter, end.MetadataProperties, true /*writeCustomElements*/);
                xmlWriter.WriteEndElement();
            }

            foreach (ReferentialConstraint referentialConstraint in associationType.ReferentialConstraints)
            {
                xmlWriter.WriteStartElement(XmlConstants.ReferentialConstraint);

                // Write the user defined annotations
                WriteUserDefinedAnnotations(xmlWriter, referentialConstraint.MetadataProperties, false /*writeCustomElements*/);
                WriteReferentialConstraintChildElement(xmlWriter, XmlConstants.Principal, referentialConstraint.FromRole.Name, referentialConstraint.FromProperties);
                WriteReferentialConstraintChildElement(xmlWriter, XmlConstants.Dependent, referentialConstraint.ToRole.Name, referentialConstraint.ToProperties);
                WriteUserDefinedAnnotations(xmlWriter, referentialConstraint.MetadataProperties, true /*writeCustomElements*/);
                xmlWriter.WriteEndElement();
            }

            WriteUserDefinedAnnotations(xmlWriter, associationType.MetadataProperties, true /*writeCustomElements*/);
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Writes all the entity container definition in the given xml writer.
        /// Also emits a special annotation for the default entity container so that
        /// client can figure out which one is the default entity container name
        /// </summary>
        /// <param name="xmlWriter">xmlWriter to which metadata needs to be written</param>
        /// <param name="defaultEntityContainer">default entity container</param>
        /// <param name="provider">Data provider from which metadata should be gathered.</param>
        /// <param name="metadataManager">metadata manager instance</param>
        private static void WriteEntityContainers(
            XmlWriter xmlWriter,
            EntityContainer defaultEntityContainer,
            DataServiceProviderWrapper provider,
            MetadataManager metadataManager)
        {
            foreach (EntityContainer container in metadataManager.EntityContainers)
            {
                xmlWriter.WriteStartElement(XmlConstants.EntityContainer);
                xmlWriter.WriteAttributeString(XmlConstants.Name, container.Name);

                // Write the user defined annotations
                WriteUserDefinedAnnotations(xmlWriter, container.MetadataProperties, false /*writeCustomElements*/);
                if (container == defaultEntityContainer)
                {
                    MetadataSerializer.WriteDataWebMetadata(
                        xmlWriter, XmlConstants.IsDefaultEntityContainerAttribute, XmlConstants.XmlTrueLiteral);
                    MetadataSerializer.WriteServiceOperations(xmlWriter, provider);
                }

                List<EntitySet> entitySets;
                if (metadataManager.EntitySets.TryGetValue(container, out entitySets))
                {
                    foreach (EntitySet entitySet in entitySets)
                    {
                        WriteEntitySet(xmlWriter, entitySet);
                    }
                }

                List<AssociationSet> associationSets;
                if (metadataManager.AssociationSets.TryGetValue(container, out associationSets))
                {
                    foreach (AssociationSet associationSet in associationSets)
                    {
                        WriteAssociationSet(xmlWriter, associationSet);
                    }
                }

                WriteUserDefinedAnnotations(xmlWriter, container.MetadataProperties, true /*writeCustomElements*/);
                xmlWriter.WriteEndElement();
            }
        }

        /// <summary>
        /// Returns true if the subtree of expansions rooted in the specified <paramref name="expandedNode"/>
        /// contains either a filter or paging/maxresult constraint or if it expands two properties
        /// pointing into the same resource set.
        /// </summary>
        /// <param name="expandedNode">The root of the expansions tree to inspect.</param>
        /// <param name="resourceSets">Already visited resource sets.</param>
        /// <param name="depth">The depth of the expandedNode. The method will throw if depth greater than 8 is reached.</param>
        /// <returns>True if BasicExpandProvider should be used to process a query with this tree
        /// or false otherwise.</returns>
        private static bool ShouldUseBasicExpandProvider(ExpandedProjectionNode expandedNode, HashSet<ResourceSet> resourceSets, int depth)
        {
            // Note that we have to walk the entire tree even though we already found a reason to use the BasicExpandProvider
            //   the reason is that we also check for the maximum depth of expansions.
            bool shouldUseBasicExpandProvider = false;

            if (depth > 8)
            {
                throw DataServiceException.CreateBadRequestError(Strings.ObjectContext_ExpandTooDeep);
            }

            foreach (ProjectionNode node in expandedNode.Nodes)
            {
                ExpandedProjectionNode childExpandedNode = node as ExpandedProjectionNode;
                if (childExpandedNode != null)
                {
                    shouldUseBasicExpandProvider |= childExpandedNode.HasFilterOrMaxResults;

                    // If we expand properties that come from a container we've already encountered,
                    // we should use the basic provider; otherwise the EF materializer will throw because
                    // tracking is turned off for querying.
                    Debug.Assert(
                        childExpandedNode.ResourceSetWrapper != null,
                        "expandedNode.ResourceSetWrapper != null -- otherwise we have an open property in EF or forgot to bind a segment");
                    shouldUseBasicExpandProvider |= !resourceSets.Add(childExpandedNode.ResourceSetWrapper.ResourceSet);

                    shouldUseBasicExpandProvider |= ShouldUseBasicExpandProvider(childExpandedNode, resourceSets, depth + 1);
                }
            }

            return shouldUseBasicExpandProvider;
        }

        /// <summary>
        /// Create a new instance of the given clrtype using ObjectContext.CreateObject method
        /// </summary>
        /// <param name="context">current object context instance.</param>
        /// <param name="clrType">clrType whose instance needs to be created.</param>
        /// <returns>the instance returned by ObjectContext.</returns>
        private static object CreateObject(ObjectContext context, Type clrType)
        {
            // this.ObjectContext.CreateObject<T>()
            MethodInfo createObjectMethod = context.GetType().GetMethod("CreateObject", BindingFlags.Public | BindingFlags.Instance);
            return createObjectMethod.MakeGenericMethod(clrType).Invoke(context, null);
        }

        /// <summary>
        /// Checks if the given association is a FK association with cardinality 1 to 1 or 0..1 to 1
        /// </summary>
        /// <param name="association">metadata for the association.</param>
        /// <returns>Returns true if the given association is a FK association with cardinality 1 to 1 or 0..1 to 1.</returns>
        private static bool IsOneToOneFKAssocation(AssociationType association)
        {
            // first check if the relationship type is a FK relationship. If no, then return , since updates are not supported only for FK relationships
            if (!association.IsForeignKey)
            {
                return false;
            }

            // check if the relationship is 1 to 1 or 1 to 0..1 relationship (FK relationships are not supported for 0..1 to 0..1 cardinality)
            // if its neither, we do support updates and there is no need to throw.
            if (association.RelationshipEndMembers[0].RelationshipMultiplicity == RelationshipMultiplicity.Many ||
                association.RelationshipEndMembers[1].RelationshipMultiplicity == RelationshipMultiplicity.Many)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Given a resource type, builds the EntityPropertyMappingInfo for each EntityPropertyMappingAttribute on it
        /// </summary>
        /// <param name="workspace">The EFx metadata workspace to which the resource type belongs</param>
        /// <param name="baseResourceType">Resouce type for which EntityPropertyMappingAttribute discovery is happening</param>
        /// <param name="descendentResourceType">The most inherited resource type in the hierarchy</param>
        private void GetEpmInfoForResourceType(MetadataWorkspace workspace, ResourceType baseResourceType, ResourceType descendentResourceType)
        {
            if (baseResourceType.BaseType != null)
            {
                this.GetEpmInfoForResourceType(workspace, baseResourceType.BaseType, descendentResourceType);
            }

            // Get epm information provided at the entity type declaration level
            StructuralType edmType = workspace.GetItem<StructuralType>(baseResourceType.FullName, DataSpace.CSpace);
            IEnumerable<MetadataProperty> extendedProperties = edmType.MetadataProperties.Where(mp => mp.PropertyKind == PropertyKind.Extended);

            foreach (EpmPropertyInformation propertyInformation in GetEpmPropertyInformation(extendedProperties, edmType.Name, null))
            {
                ResourceProperty redefinedProperty = this.GetResourcePropertyFromEpmPath(
                                                            baseResourceType,
                                                            propertyInformation.SourcePath);
                if (redefinedProperty == null)
                {
                    throw new InvalidOperationException(Strings.ObjectContext_UnknownPropertyNameInEpmAttributes(propertyInformation.SourcePath, baseResourceType.Name));
                }

                this.GetEpmInfoForResourceProperty(
                    propertyInformation,
                    descendentResourceType,
                    redefinedProperty,
                    baseResourceType);
            }

            // Get epm information provided at the entity type property level
            foreach (EdmMember member in edmType.Members.Where(m => m.DeclaringType == edmType))
            {
                ResourceProperty resourceProperty = baseResourceType.TryResolvePropertiesDeclaredOnThisTypeByName(member.Name);
                Debug.Assert(resourceProperty != null, "resourceProperty must not be null");
                IEnumerable<MetadataProperty> extendedMemberProperties = member.MetadataProperties.Where(mdp => mdp.PropertyKind == PropertyKind.Extended);

                foreach (EpmPropertyInformation propertyInformation in GetEpmPropertyInformation(extendedMemberProperties, edmType.Name, member.Name))
                {
                    ResourceProperty propertyToUse = resourceProperty;

                    if (resourceProperty.IsOfKind(ResourcePropertyKind.ComplexType) && propertyInformation.PathGiven)
                    {
                        propertyInformation.SourcePath = resourceProperty.Name + "/" + propertyInformation.SourcePath;
                        propertyToUse = this.GetResourcePropertyFromEpmPath(
                                                baseResourceType,
                                                propertyInformation.SourcePath);
                    }

                    this.GetEpmInfoForResourceProperty(
                        propertyInformation,
                        descendentResourceType,
                        propertyToUse,
                        baseResourceType);
                }
            }
        }

        /// <summary>
        /// Given a resource type and its resource proeperty builds the EntityPropertyMappingInfo for the EntityPropertyMappingAttribute on it
        /// </summary>
        /// <param name="propertyInformation">EPM information for current property</param>
        /// <param name="descendentResourceType">The most inherited resource type in the hierarchy</param>
        /// <param name="resourceProperty">Resource property for which to get the information</param>
        /// <param name="definingType">Type that defined this property</param>
        private void GetEpmInfoForResourceProperty(
            EpmPropertyInformation propertyInformation,
            ResourceType descendentResourceType,
            ResourceProperty resourceProperty,
            ResourceType definingType)
        {
            if (propertyInformation.IsAtom)
            {
                if (resourceProperty.IsOfKind(ResourcePropertyKind.ComplexType))
                {
                    throw new InvalidOperationException(Strings.ObjectContext_SyndicationMappingForComplexPropertiesNotAllowed);
                }
                else
                {
                    EntityPropertyMappingAttribute epmAttr = new EntityPropertyMappingAttribute(
                                        propertyInformation.SourcePath,
                                        propertyInformation.SyndicationItem,
                                        propertyInformation.ContentKind,
                                        propertyInformation.KeepInContent);

                    descendentResourceType.BuildEpmInfo(epmAttr, definingType, true);
                }
            }
            else
            {
                if (resourceProperty.IsOfKind(ResourcePropertyKind.ComplexType))
                {
                    foreach (EntityPropertyMappingAttribute epmAttr in this.GetEpmAttrsFromComplexProperty(
                                                                        resourceProperty,
                                                                        propertyInformation.SourcePath,
                                                                        propertyInformation.TargetPath,
                                                                        propertyInformation.NsPrefix,
                                                                        propertyInformation.NsUri,
                                                                        propertyInformation.KeepInContent))
                    {
                        descendentResourceType.BuildEpmInfo(epmAttr, definingType, true);
                    }
                }
                else
                {
                    EntityPropertyMappingAttribute epmAttr = new EntityPropertyMappingAttribute(
                                        propertyInformation.SourcePath,
                                        propertyInformation.TargetPath,
                                        propertyInformation.NsPrefix,
                                        propertyInformation.NsUri,
                                        propertyInformation.KeepInContent);

                    descendentResourceType.BuildEpmInfo(epmAttr, definingType, true);
                }
            }
        }

        /// <summary>
        /// Obtains the ResourceProperty corresponding to a given sourcePath
        /// </summary>
        /// <param name="baseResourceType">Resource type in which to look for property</param>
        /// <param name="sourcePath">Source Path</param>
        /// <returns>ResourceProperty object corresponding to the property given through source path</returns>
        private ResourceProperty GetResourcePropertyFromEpmPath(ResourceType baseResourceType, String sourcePath)
        {
            String[] propertyPath = sourcePath.Split('/');

            if (baseResourceType.TryResolvePropertiesDeclaredOnThisTypeByName(propertyPath[0]) == null)
            {
                return baseResourceType.BaseType != null ? this.GetResourcePropertyFromEpmPath(baseResourceType.BaseType, sourcePath) : null;
            }
            else
            {
                ResourceProperty resourceProperty = null;
                foreach (var pathSegment in propertyPath)
                {
                    resourceProperty = baseResourceType.TryResolvePropertiesDeclaredOnThisTypeByName(pathSegment);

                    // Property not declared in this type
                    if (resourceProperty == null)
                    {
                        throw new InvalidOperationException(Strings.EpmSourceTree_InaccessiblePropertyOnType(pathSegment, baseResourceType.Name));
                    }

                    baseResourceType = resourceProperty.ResourceType;
                }

                return resourceProperty;
            }
        }

        /// <summary>
        /// Returns a sequence of attributes corresponding to a complex type with recursion
        /// </summary>
        /// <param name="complexProperty">Complex typed property</param>
        /// <param name="epmSourcePath">Source path</param>
        /// <param name="epmTargetPath">Target path</param>
        /// <param name="epmNsPrefix">Namespace prefix</param>
        /// <param name="epmNsUri">Namespace Uri</param>
        /// <param name="epmKeepInContent">KeepInContent setting</param>
        /// <returns>Sequence of entity property mapping information for complex type properties</returns>
        private IEnumerable<EntityPropertyMappingAttribute> GetEpmAttrsFromComplexProperty(
            ResourceProperty complexProperty,
            String epmSourcePath,
            String epmTargetPath,
            String epmNsPrefix,
            String epmNsUri,
            bool epmKeepInContent)
        {
            foreach (ResourceProperty subProperty in complexProperty.ResourceType.Properties)
            {
                String sourcePath = epmSourcePath + "/" + subProperty.Name;
                String targetPath = epmTargetPath + "/" + subProperty.Name;

                if (subProperty.IsOfKind(ResourcePropertyKind.ComplexType))
                {
                    foreach (EntityPropertyMappingAttribute epmAttr in this.GetEpmAttrsFromComplexProperty(subProperty, sourcePath, targetPath, epmNsPrefix, epmNsUri, epmKeepInContent))
                    {
                        yield return epmAttr;
                    }
                }
                else
                {
                    yield return new EntityPropertyMappingAttribute(
                                        sourcePath,
                                        targetPath,
                                        epmNsPrefix,
                                        epmNsUri,
                                        epmKeepInContent);
                }
            }
        }

        /// <summary>
        /// Get the entity set metadata object given the qualified entity set name
        /// </summary>
        /// <param name="qualifiedEntitySetName">qualified entity set name i.e. if the entity set
        /// belongs to entity container other than the default one, then the entity container name should
        /// be part of the qualified name</param>
        /// <returns>the entity set metadata object</returns>
        private EntitySet GetEntitySet(string qualifiedEntitySetName)
        {
            Debug.Assert(
                !String.IsNullOrEmpty(qualifiedEntitySetName),
                "!String.IsNullOrEmpty(qualifiedEntitySetName) -- otherwise qualifiedEntitySetName didn't come from internal metadata");

            string entityContainerName;
            string entitySetName;

            // entity set name is fully qualified
            int index = qualifiedEntitySetName.LastIndexOf('.');
            if (index != -1)
            {
                entityContainerName = qualifiedEntitySetName.Substring(0, index);
                entitySetName = qualifiedEntitySetName.Substring(index + 1);
            }
            else
            {
                entityContainerName = this.ObjectContext.DefaultContainerName;
                entitySetName = qualifiedEntitySetName;
            }

            EntityContainer entityContainer = this.ObjectContext.MetadataWorkspace.GetEntityContainer(entityContainerName, DataSpace.CSpace);
            Debug.Assert(
                entityContainer != null, 
                "entityContainer != null -- otherwise entityContainerName '" + entityContainerName + "' didn't come from metadata");

            EntitySet entitySet = entityContainer.GetEntitySetByName(entitySetName, false /*ignoreCase*/);
            Debug.Assert(
                entitySet != null,
                "entitySet != null -- otherwise entitySetName '" + entitySetName + "' didn't come from metadata");

            return entitySet;
        }

        /// <summary>
        /// Get the default entity container
        /// </summary>
        /// <returns>returns the default entity container</returns>
        private EntityContainer GetDefaultEntityContainer()
        {
            EntityContainer entityContainer;

            if (String.IsNullOrEmpty(this.ObjectContext.DefaultContainerName))
            {
                return null;
            }

            if (!this.ObjectContext.MetadataWorkspace.TryGetEntityContainer(
                    this.ObjectContext.DefaultContainerName, DataSpace.CSpace, out entityContainer))
            {
                throw new InvalidOperationException(
                    Strings.ObjectContext_InvalidDefaultEntityContainerName(this.ObjectContext.DefaultContainerName));
            }

            return entityContainer;
        }

        /// <summary>Creates the object query for the given resource set and returns it.</summary>
        /// <param name="container">resource set for which a query instance needs to be created.</param>
        /// <returns>Returns the ObjectQuery instance for the given resource set.</returns>
        private ObjectQuery InternalGetResourceContainerInstance(ResourceSet container)
        {
            Debug.Assert(container != null, "container != null");
            if (container.ReadFromContextDelegate == null)
            {
                Type[] parameterTypes = new Type[] { typeof(object) };
                string escapedEntitySetName = GetEscapedEntitySetName(container.Name);
                MethodInfo genericMethod = typeof(ObjectContext).GetMethod("CreateQuery", WebUtil.PublicInstanceBindingFlags).MakeGenericMethod(container.ResourceType.InstanceType);

                // ((ObjectContext)arg0).CreateQuery("escapedEntitySetName", new ObjectParameter[0]);
                System.Reflection.Emit.DynamicMethod readerMethod = new System.Reflection.Emit.DynamicMethod("queryable_reader", typeof(IQueryable), parameterTypes, false);
                var generator = readerMethod.GetILGenerator();
                generator.Emit(System.Reflection.Emit.OpCodes.Ldarg_0);
                generator.Emit(System.Reflection.Emit.OpCodes.Castclass, typeof(ObjectContext));
                generator.Emit(System.Reflection.Emit.OpCodes.Ldstr, escapedEntitySetName);
                generator.Emit(System.Reflection.Emit.OpCodes.Ldc_I4_0);
                generator.Emit(System.Reflection.Emit.OpCodes.Newarr, typeof(ObjectParameter));
                generator.Emit(System.Reflection.Emit.OpCodes.Call, genericMethod);
                generator.Emit(System.Reflection.Emit.OpCodes.Ret);
                container.ReadFromContextDelegate = (Func<object, IQueryable>)readerMethod.CreateDelegate(typeof(Func<object, IQueryable>));
            }

            return (ObjectQuery)container.ReadFromContextDelegate(this.ObjectContext);
        }

        /// <summary>
        /// Returns the entity set name for the given object state entry.
        /// </summary>
        /// <param name="entry">object state entry for the object whose entity set name needs to be retreived.</param>
        /// <returns>entity set name for the given entity entry.</returns>
        private string GetEntitySetName(ObjectStateEntry entry)
        {
            return ObjectContextServiceProvider.GetEntitySetName(
                entry.EntitySet.Name,
                entry.EntitySet.EntityContainer.Name,
                entry.EntitySet.EntityContainer.Name == this.ObjectContext.DefaultContainerName);
        }

        /// <summary>
        /// Update the relationship for the given entity.
        /// </summary>
        /// <param name="targetResource">source entity.</param>
        /// <param name="propertyName">navigation property which needs to get updated.</param>
        /// <param name="propertyValue">target entity - the other end of the relationship.</param>
        /// <param name="addRelationship">null for reference properties, true if relationship needs to be added for collection properties, else false.</param>
        private void UpdateRelationship(object targetResource, string propertyName, object propertyValue, bool? addRelationship)
        {
            Debug.Assert(targetResource != null, "targetResource != null");
            Debug.Assert(!String.IsNullOrEmpty(propertyName), "!String.IsNullOrEmpty(propertyName)");

            ResourceType resourceType = this.GetSingleResource(targetResource);
            
            // Get the NavigationProperty metadata to get the association and ToRole metadata
            EntityType cspaceEntityType = this.ObjectContext.MetadataWorkspace.GetItem<EntityType>(resourceType.FullName, DataSpace.CSpace);
            NavigationProperty navProperty = cspaceEntityType.NavigationProperties[propertyName];

            // Get the resource type and resource property to find the type of the property
            ResourceProperty resourceProperty = resourceType.TryResolvePropertyName(propertyName);
            Debug.Assert(resourceProperty != null && resourceProperty.TypeKind == ResourceTypeKind.EntityType, "property must be a navigation property");

            // Get the relation end 
            var stateEntry = this.ObjectContext.ObjectStateManager.GetObjectStateEntry(targetResource);
            IRelatedEnd relatedEnd = stateEntry.RelationshipManager.GetRelatedEnd(navProperty.RelationshipType.Name, navProperty.ToEndMember.Name);

            try
            {
                if (resourceProperty.Kind == ResourcePropertyKind.ResourceReference)
                {
                    Debug.Assert(addRelationship == null, "addRelationship == null for reference properties");

                    // For FK relationships with 1 to 1 or 1 to 0..1 cardinalities, we need to handle it in a different manner
                    if (IsOneToOneFKAssocation((AssociationType)navProperty.RelationshipType))
                    {
                        // If the parent entity is not in the added state and the other end is not yet loaded, then load it
                        if (stateEntry.State != EntityState.Added && !relatedEnd.IsLoaded)
                        {
                            relatedEnd.Load();
                        }
                    }

                    // if the property value is null, then just set the entity key to null. 
                    // if the key is null, then this will be a no-op, but if the key is not null, then it will
                    // delete the relationship
                    if (propertyValue == null)
                    {
                        ((EntityReference)relatedEnd).EntityKey = null;
                    }
                    else
                    {
                        EntityKey key = ((EntityReference)relatedEnd).EntityKey;

                        // If the key is null, that means there was no relationship and user is trying to set the relationship for the first time
                        // if there is a key, make sure the relationship is set to something new.
                        // The reason why we need to do a equality check is because when we set the EntityKey to null, it will put the dependent
                        // in deleted state, and setting the same entity key again, will try to add it. Setting the key to the same value is
                        // a very common pattern in the astoria client and hence we need to make sure that scenario works.
                        if (key == null ||
                            !key.Equals(this.ObjectContext.ObjectStateManager.GetObjectStateEntry(propertyValue).EntityKey))
                        {
                            // First set the reference property to null, in case there is an existing property, since relatedEnd.Add throws,
                            // if the reference property is not null. Then update the property to the new value, if required.
                            ((EntityReference)relatedEnd).EntityKey = null;
                            relatedEnd.Add(propertyValue);
                        }
                    }
                }
                else if (addRelationship == true)
                {
                    Debug.Assert(propertyValue != null, "propertyValue != null");
                    relatedEnd.Add(propertyValue);
                }
                else
                {
                    // The reason why we need to attach first, before removing is ObjectContext might not know about the relationship yet.
                    // Hence attaching establishes the relationship, and then removing marks it as delete.
                    Debug.Assert(propertyValue != null, "propertyValue != null");
                    relatedEnd.Attach(propertyValue);
                    relatedEnd.Remove(propertyValue);
                }
            }
            catch (InvalidOperationException exception)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequest_ErrorInSettingPropertyValue(propertyName), exception);
            }
        }

        /// <summary>
        /// Finds all visible resource sets, types, service ops, association sets, etc. from the given provider
        /// </summary>
        private sealed class MetadataManager
        {
            #region Private Fields
            
            /// <summary>data service provider instance</summary>
            private DataServiceProviderWrapper provider;

            /// <summary>EF workspace</summary>
            private MetadataWorkspace workspace;

            /// <summary>Default entity container</summary>
            private EntityContainer defaultEntityContainer;

            /// <summary>List of entity containers</summary>
            private List<EntityContainer> entityContainers = new List<EntityContainer>();

            /// <summary>List of namespace along with the types in that namespace</summary>
            private Dictionary<string, HashSet<EdmType>> edmTypes = new Dictionary<string, HashSet<EdmType>>(StringComparer.Ordinal);

            /// <summary>List of entity containers along with entity sets</summary>
            private Dictionary<EntityContainer, List<EntitySet>> entitySets = new Dictionary<EntityContainer, List<EntitySet>>(EqualityComparer<EntityContainer>.Default);

            /// <summary>List of entity containers along with association sets</summary>
            private Dictionary<EntityContainer, List<AssociationSet>> associationSets = new Dictionary<EntityContainer, List<AssociationSet>>(EqualityComparer<EntityContainer>.Default);

            /// <summary>Set to true if we see any visible MLE.</summary>
            private bool hasVisibleMediaLinkEntry;

            #endregion Private Fields

            #region Constructor

            /// <summary>Constructs a metadata manager instance</summary>
            /// <param name="workspace">workspace containing the metadata</param>
            /// <param name="defaultEntityContainer">default entity container</param>
            /// <param name="provider">data service provider instance to check type visibility</param>
            /// <param name="service">Data service instance.</param>
            internal MetadataManager(MetadataWorkspace workspace, EntityContainer defaultEntityContainer, DataServiceProviderWrapper provider, IDataService service)
            {
                Debug.Assert(workspace != null, "workspace != null");
                Debug.Assert(provider != null, "provider != null");

                this.workspace = workspace;
                this.defaultEntityContainer = defaultEntityContainer;
                this.provider = provider;

                // Add entity sets and association sets
                this.PopulateEntityAndAssociationSets();

                // Add entity types reachable from visible sets
                foreach (ResourceSetWrapper resourceSet in this.provider.ResourceSets)
                {
                    this.PopulateTypesForSet(resourceSet);
                }

                // Add resource types reachable from service operations
                foreach (ServiceOperationWrapper serviceOperation in this.provider.ServiceOperations)
                {
                    this.PopulateTypeForServiceOperation(serviceOperation);
                }

                // Add association types
                this.PopulateAssociationTypes();

                // If we have encountered a visible MLE type, we need to make sure there is a stream provider implementation
                // for this service or else we can end up with in-stream errors during runtime.
                if (this.hasVisibleMediaLinkEntry)
                {
                    // LoadStreamProvider will throw if we can't locate a stream provider implementation.
                    DataServiceStreamProviderWrapper.LoadStreamProvider(service);
                }
            }

            #endregion Constructor

            #region Properties

            /// <summary>Default entity container</summary>
            internal IEnumerable<EntityContainer> EntityContainers
            {
                get { return this.entityContainers; }
            }

            /// <summary>List of visible resource sets</summary>
            internal Dictionary<EntityContainer, List<EntitySet>> EntitySets
            {
                get { return this.entitySets; }
            }

            /// <summary>List of visible association sets</summary>
            internal Dictionary<EntityContainer, List<AssociationSet>> AssociationSets
            {
                get { return this.associationSets; }
            }
            
            /// <summary>Returns the list of namespace and the types in those namespaces.</summary>
            internal IEnumerable<KeyValuePair<string, HashSet<EdmType>>> NamespaceAlongWithTypes
            {
                get { return this.edmTypes; }
            }

            #endregion Properties

            #region Methods

            /// <summary>Gets the list of visible edm properties from a type.</summary>
            /// <param name="declaringType">Type in question.</param>
            /// <returns>List of visible edm properties.</returns>
            internal IEnumerable<EdmMember> GetPropertiesDeclaredInThisType(StructuralType declaringType)
            {
                foreach (EdmMember member in declaringType.Members)
                {
                    // Ignore members which are not defined on this type
                    if (member.DeclaringType != declaringType)
                    {
                        continue;
                    }

                    if (member.BuiltInTypeKind == BuiltInTypeKind.NavigationProperty)
                    {
                        EdmType targetType = ((RefType)((NavigationProperty)member).ToEndMember.TypeUsage.EdmType).ElementType;
                        HashSet<EdmType> typesInNamespace;
                        string typeNamespace = targetType.NamespaceName;
                        if (!this.edmTypes.TryGetValue(typeNamespace, out typesInNamespace) ||
                            !typesInNamespace.Contains(targetType))
                        {
                            continue;
                        }
                    }

                    yield return member;
                }
            }

            /// <summary>
            /// Add association types
            /// </summary>
            private void PopulateAssociationTypes()
            {
                foreach (AssociationType associationType in this.workspace.GetItems<AssociationType>(DataSpace.CSpace))
                {
                    // Skip association types were any ends are not visible.
                    bool shouldAdd = true;
                    foreach (AssociationEndMember member in associationType.AssociationEndMembers)
                    {
                        EdmType endType = ((RefType)member.TypeUsage.EdmType).ElementType;
                        string typeNamespace = endType.NamespaceName;
                        HashSet<EdmType> typesInNamespace;
                        if (!this.edmTypes.TryGetValue(typeNamespace, out typesInNamespace) || !typesInNamespace.Contains(endType))
                        {
                            shouldAdd = false;
                            break;
                        }
                    }

                    if (shouldAdd)
                    {
                        this.AddVisibleEdmType(associationType);
                    }
                }
            }

            /// <summary>
            /// Add entity sets and association sets
            /// </summary>
            private void PopulateEntityAndAssociationSets()
            {
                foreach (EntityContainer container in this.workspace.GetItems<EntityContainer>(DataSpace.CSpace))
                {
                    this.entityContainers.Add(container);

                    string containerName = container == this.defaultEntityContainer ? "" : container.Name;

                    List<EntitySet> entitySetInContainer;
                    if (!this.entitySets.TryGetValue(container, out entitySetInContainer))
                    {
                        entitySetInContainer = new List<EntitySet>();
                        this.entitySets.Add(container, entitySetInContainer);
                    }

                    List<AssociationSet> associationSetInContainer;
                    if (!this.associationSets.TryGetValue(container, out associationSetInContainer))
                    {
                        associationSetInContainer = new List<AssociationSet>();
                        this.associationSets.Add(container, associationSetInContainer);
                    }

                    foreach (EntitySetBase set in container.BaseEntitySets)
                    {
                        if (set.BuiltInTypeKind == BuiltInTypeKind.EntitySet)
                        {
                            // Skip entity sets with no backing metadata.
                            string lookupName = set.Name;
                            if (!String.IsNullOrEmpty(containerName))
                            {
                                lookupName = containerName + "." + lookupName;
                            }

                            if (this.provider.TryResolveResourceSet(lookupName) != null)
                            {
                                entitySetInContainer.Add((EntitySet)set);
                            }
                        }
                        else if (set.BuiltInTypeKind == BuiltInTypeKind.AssociationSet)
                        {
                            // Skip association sets where any end is hidden.
                            AssociationSet associationSet = (AssociationSet)set;
                            bool shouldAdd = true;
                            foreach (AssociationSetEnd end in associationSet.AssociationSetEnds)
                            {
                                string lookupName = end.EntitySet.Name;
                                if (!String.IsNullOrEmpty(containerName))
                                {
                                    lookupName = containerName + "." + lookupName;
                                }

                                if (this.provider.TryResolveResourceSet(lookupName) == null)
                                {
                                    shouldAdd = false;
                                    break;
                                }
                            }

                            if (shouldAdd)
                            {
                                associationSetInContainer.Add((AssociationSet)set);
                            }
                        }
                    }
                }
            }

            /// <summary>Add an edm type to the list of visible types</summary>
            /// <param name="edmType">edmType to add</param>
            private void AddVisibleEdmType(EdmType edmType)
            {
                HashSet<EdmType> typesInSameNamespace;
                string typeNamespace = edmType.NamespaceName;
                if (!this.edmTypes.TryGetValue(typeNamespace, out typesInSameNamespace))
                {
                    typesInSameNamespace = new HashSet<EdmType>(EqualityComparer<EdmType>.Default);
                    this.edmTypes.Add(typeNamespace, typesInSameNamespace);
                }

                // Add the type to the list of types in the same namespace
                typesInSameNamespace.Add(edmType);
            }

            /// <summary>Add a resource type to the list of visible types</summary>
            /// <param name="resourceType">resource type to add</param>
            private void AddVisibleResourceType(ResourceType resourceType)
            {                
                EdmType edmType = this.workspace.GetItem<EdmType>(resourceType.FullName, DataSpace.CSpace);
                Debug.Assert(edmType != null, "edmType != null");
                this.AddVisibleEdmType(edmType);
                if (resourceType.IsMediaLinkEntry)
                {
                    this.hasVisibleMediaLinkEntry = true;
                }
            }

            /// <summary>Recursively add complex types reachable through resource properties of the given type</summary>
            /// <param name="resourceType">resource type to inspect</param>
            private void AddComplexPropertTypes(ResourceType resourceType)
            {
                Debug.Assert(resourceType != null, "resourceType != null");

                foreach (ResourceProperty property in resourceType.PropertiesDeclaredOnThisType)
                {
                    if (property.ResourceType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                    {
                        this.AddVisibleResourceType(property.ResourceType);
                        this.AddComplexPropertTypes(property.ResourceType);
                    }
                }
            }

            /// <summary>Populate all reachable types from a resource set</summary>
            /// <param name="resourceSet">resource set to inspect</param>
            private void PopulateTypesForSet(ResourceSetWrapper resourceSet)
            {
                // Derived types of a visible type are visible
                foreach (ResourceType derivedType in this.provider.GetDerivedTypes(resourceSet.ResourceType))
                {
                    this.AddVisibleResourceType(derivedType);
                    this.AddComplexPropertTypes(derivedType);
                }

                // Base types of a visible type are visible
                ResourceType resourceType = resourceSet.ResourceType;
                while (resourceType != null)
                {
                    this.AddVisibleResourceType(resourceType);
                    this.AddComplexPropertTypes(resourceType);
                    resourceType = resourceType.BaseType;
                }
            }

            /// <summary>Populate resource types returned by the given service operation.</summary>
            /// <param name="serviceOperation">Service operation to inspect</param>
            private void PopulateTypeForServiceOperation(ServiceOperationWrapper serviceOperation)
            {
                ResourceType resultType = serviceOperation.ResultType;
                if (resultType != null && resultType.ResourceTypeKind == ResourceTypeKind.ComplexType)
                {
                    this.AddVisibleResourceType(resultType);
                    this.AddComplexPropertTypes(resultType);
                }

                Debug.Assert(
                    resultType == null || resultType.ResourceTypeKind != ResourceTypeKind.EntityType ||
                    (this.edmTypes.ContainsKey(resultType.Namespace) && 
                    this.edmTypes[resultType.Namespace].Contains(this.workspace.GetItem<EntityType>(resultType.FullName, DataSpace.CSpace))),
                    "If a result type is an entity type, it must be visible through an entity set.");
            }

            #endregion Methods
        }
    }
}
