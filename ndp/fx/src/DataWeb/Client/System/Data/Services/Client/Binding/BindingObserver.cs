//---------------------------------------------------------------------
// <copyright file="BindingObserver.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   BindingObserver class
// </summary>
//
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
#region Namespaces
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;
#endregion    

    /// <summary>The BindingObserver class</summary>
    internal sealed class BindingObserver
    {
        #region Fields
        
        /// <summary>
        /// The BindingGraph maps objects tracked by the DataServiceContext to vertices in a 
        /// graph used to manage the information needed for data binding. The objects tracked 
        /// by the BindingGraph are entities, complex types and DataServiceCollections.
        /// </summary>
        private BindingGraph bindingGraph;

        #endregion

        #region Constructor
        
        /// <summary>Constructor</summary>
        /// <param name="context">The DataServiceContext associated with the BindingObserver.</param>
        /// <param name="entityChanged">EntityChanged delegate.</param>
        /// <param name="collectionChanged">EntityCollectionChanged delegate.</param>
        internal BindingObserver(DataServiceContext context, Func<EntityChangedParams, bool> entityChanged, Func<EntityCollectionChangedParams, bool> collectionChanged)
        {
            Debug.Assert(context != null, "Must have been validated during DataServiceCollection construction.");
            this.Context = context;
            this.Context.ChangesSaved += this.OnChangesSaved;
            
            this.EntityChanged = entityChanged;
            this.CollectionChanged = collectionChanged;
            
            this.bindingGraph = new BindingGraph(this);
        }
        
        #endregion

        #region Properties

        /// <summary>The DataServiceContext associated with the BindingObserver.</summary>
        internal DataServiceContext Context
        {
            get;
            private set;
        }

        /// <summary>The behavior of add operations should be Attach or Add on the context.</summary>
        internal bool AttachBehavior
        {
            get;
            set;
        }

        /// <summary>The behavior of remove operations should be Detach on the context.</summary>
        internal bool DetachBehavior
        {
            get;
            set;
        }

        /// <summary>
        /// Callback invoked when a property of an entity object tracked by the BindingObserver has changed.
        /// </summary>
        /// <remarks>
        /// Entity objects tracked by the BindingObserver implement INotifyPropertyChanged. Events of this type
        /// flow throw the EntityChangedParams. If this callback is not implemented by user code, or the user code
        /// implementation returns false, the BindingObserver executes a default implementation for the callback.
        /// </remarks>
        internal Func<EntityChangedParams, bool> EntityChanged
        {
            get;
            private set;
        }

        /// <summary>
        /// Callback invoked when an DataServiceCollection tracked by the BindingObserver has changed.
        /// </summary>
        /// <remarks>
        /// DataServiceCollection objects tracked by the BindingObserver implement INotifyCollectionChanged.  
        /// Events of this type flow throw the EntityCollectionChanged callback. If this callback is not 
        /// implemented by user code, or the user code implementation returns false, the BindingObserver executes 
        /// a default implementation for the callback.
        /// </remarks>
        internal Func<EntityCollectionChangedParams, bool> CollectionChanged
        {
            get;
            private set;
        }

        #endregion

        #region Methods
        
        /// <summary>Start tracking the specified DataServiceCollection.</summary>
        /// <typeparam name="T">An entity type.</typeparam>
        /// <param name="collection">An DataServiceCollection.</param>
        /// <param name="collectionEntitySet">The entity set of the elements in <paramref name="collection"/>.</param>
        internal void StartTracking<T>(DataServiceCollection<T> collection, string collectionEntitySet)
        {
            Debug.Assert(collection != null, "Only constructed collections are tracked.");
            
            // Verify that T corresponds to an entity type.
            if (!BindingEntityInfo.IsEntityType(typeof(T)))
            {
                throw new ArgumentException(Strings.DataBinding_DataServiceCollectionArgumentMustHaveEntityType(typeof(T)));
            }

            try
            {
                this.AttachBehavior = true;

                // Recursively traverse the entire object graph under the root collection.
                this.bindingGraph.AddCollection(null, null, collection, collectionEntitySet);
            }
            finally
            {
                this.AttachBehavior = false;
            }
        }

        /// <summary>Stop tracking the root DataServiceCollection associated with the observer.</summary>
        internal void StopTracking()
        {
            this.bindingGraph.Reset();

            this.Context.ChangesSaved -= this.OnChangesSaved;
        }

#if ASTORIA_LIGHT
        internal bool LookupParent<T>(DataServiceCollection<T> collection, out object parentEntity, out string parentProperty)
        {
            string sourceEntitySet;
            string targetEntitySet;
            this.bindingGraph.GetEntityCollectionInfo(collection, out parentEntity, out parentProperty, out sourceEntitySet, out targetEntitySet);

            return parentEntity != null;
        }
#endif

        /// <summary>Handle changes to tracked entity.</summary>
        /// <param name="source">The entity that raised the event.</param>
        /// <param name="eventArgs">Information about the event such as changed property name.</param>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        internal void OnPropertyChanged(object source, PropertyChangedEventArgs eventArgs)
        {
            Util.CheckArgumentNull(source, "source");
            Util.CheckArgumentNull(eventArgs, "eventArgs");

#if DEBUG
            Debug.Assert(this.bindingGraph.IsTracking(source), "Entity must be part of the graph if it has the event notification registered.");
#endif
            string sourceProperty = eventArgs.PropertyName;

            // When sourceProperty is null, it is assumed that all properties for the object changed
            // As a result, we should be performing an UpdateObject operation on the context.
            if (String.IsNullOrEmpty(sourceProperty))
            {
                this.HandleUpdateEntity(
                        source,
                        null,
                        null);
            }
            else
            {
                BindingEntityInfo.BindingPropertyInfo bpi;
                
                // Get the new value for the changed property.
                object sourcePropertyValue = BindingEntityInfo.GetPropertyValue(source, sourceProperty, out bpi);
                
                // Check if it is an interesting property e.g. collection, entity reference or complex type.
                if (bpi != null)
                {
                    // Disconnect the edge between source and original source property value.
                    this.bindingGraph.RemoveRelation(source, sourceProperty);

                    switch (bpi.PropertyKind)
                    {
                        case BindingPropertyKind.BindingPropertyKindCollection:
                            // If collection is already being tracked by the graph we can not have > 1 links to it.
                            if (sourcePropertyValue != null)
                            {
                                // Make sure that there is no observer on the input collection property.
                                try
                                {
                                    typeof(BindingUtils)
                                        .GetMethod("VerifyObserverNotPresent", BindingFlags.NonPublic | BindingFlags.Static)
                                        .MakeGenericMethod(bpi.PropertyInfo.CollectionType)
                                        .Invoke(null, new object[] { sourcePropertyValue, sourceProperty, source.GetType() });
                                }
                                catch (TargetInvocationException tie)
                                {
                                    throw tie.InnerException;
                                }

                                try
                                {
                                    this.AttachBehavior = true;
                                    this.bindingGraph.AddCollection(
                                            source,
                                            sourceProperty,
                                            sourcePropertyValue,
                                            null);
                                }
                                finally
                                {
                                    this.AttachBehavior = false;
                                }
                            }
                            
                            break;

                        case BindingPropertyKind.BindingPropertyKindEntity:
                            // Add the newly added entity to the graph, or update entity reference.
                            this.bindingGraph.AddEntity(
                                    source,
                                    sourceProperty,
                                    sourcePropertyValue,
                                    null,
                                    source);
                            break;

                        default:
                            Debug.Assert(bpi.PropertyKind == BindingPropertyKind.BindingPropertyKindComplex, "Must be complex type if PropertyKind is not entity or collection.");
                            
                            // Attach the newly assigned complex type object and it's child complex typed objects.
                            if (sourcePropertyValue != null)
                            {
                                this.bindingGraph.AddComplexProperty(
                                        source, 
                                        sourceProperty, 
                                        sourcePropertyValue);
                            }

                            this.HandleUpdateEntity(
                                    source,
                                    sourceProperty,
                                    sourcePropertyValue);
                            break;
                    }
                }
                else
                {
                    // For non-interesting properties i.e. value types or regular collection properties we simply call UpdateObject on the context.
                    // Note that this code also handles primitive properties of complex typed objects.
                    this.HandleUpdateEntity(
                            source, 
                            sourceProperty, 
                            sourcePropertyValue);
                }
            }
        }

        /// <summary>Handle changes to tracked DataServiceCollection.</summary>
        /// <param name="collection">The DataServiceCollection that raised the event.</param>
        /// <param name="eventArgs">Information about the event such as added/removed entities, operation.</param>
        internal void OnCollectionChanged(object collection, NotifyCollectionChangedEventArgs eventArgs)
        {
            Util.CheckArgumentNull(collection, "collection");
            Util.CheckArgumentNull(eventArgs, "eventArgs");

            Debug.Assert(BindingEntityInfo.IsDataServiceCollection(collection.GetType()), "We only register this event for DataServiceCollections.");
#if DEBUG
            Debug.Assert(this.bindingGraph.IsTracking(collection), "Collection must be part of the graph if it has the event notification registered.");
#endif
            object source;
            string sourceProperty;
            string sourceEntitySet;
            string targetEntitySet;

            this.bindingGraph.GetEntityCollectionInfo(
                    collection, 
                    out source, 
                    out sourceProperty, 
                    out sourceEntitySet, 
                    out targetEntitySet);

            switch (eventArgs.Action)
            {
                case NotifyCollectionChangedAction.Add:
                    // This event is raised by ObservableCollection.InsertItem.
                    this.OnAddToCollection(
                            eventArgs, 
                            source, 
                            sourceProperty, 
                            targetEntitySet, 
                            collection);
                    break;

                case NotifyCollectionChangedAction.Remove:
                    // This event is raised by ObservableCollection.RemoveItem.
                    this.OnDeleteFromCollection(
                            eventArgs, 
                            source, 
                            sourceProperty, 
                            collection);
                    break;

                case NotifyCollectionChangedAction.Replace:
                    // This event is raised by ObservableCollection.SetItem.
                    this.OnDeleteFromCollection(
                            eventArgs, 
                            source, 
                            sourceProperty, 
                            collection);
                            
                    this.OnAddToCollection(
                            eventArgs, 
                            source, 
                            sourceProperty, 
                            targetEntitySet, 
                            collection);
                    break;

                case NotifyCollectionChangedAction.Reset:
                    // This event is raised by ObservableCollection.Clear.
                    if (this.DetachBehavior)
                    {
                        // Detach behavior requires going through each item and detaching it from context.
                        this.RemoveWithDetachCollection(collection);
                    }
                    else
                    {
                        // Non-detach behavior requires only removing vertices of collection from graph.
                        this.bindingGraph.RemoveCollection(collection);
                    }

                    break;

#if !ASTORIA_LIGHT
                case NotifyCollectionChangedAction.Move:
                    // Do Nothing. Added for completeness.
                    break;
#endif

                default:
                    throw new InvalidOperationException(Strings.DataBinding_CollectionChangedUnknownAction(eventArgs.Action));
            }
        }

        /// <summary>Handle Adds to a tracked DataServiceCollection. Perform operations on context to reflect the changes.</summary>
        /// <param name="source">The source object that reference the target object through a navigation property.</param>
        /// <param name="sourceProperty">The navigation property in the source object that reference the target object.</param>
        /// <param name="sourceEntitySet">The entity set of the source object.</param>
        /// <param name="collection">The collection containing the target object.</param>
        /// <param name="target">The target entity to attach.</param>
        /// <param name="targetEntitySet">The entity set name of the target object.</param>
        internal void HandleAddEntity(
            object source,
            string sourceProperty,
            string sourceEntitySet,
            ICollection collection,
            object target,
            string targetEntitySet)
        {
            if (this.Context.ApplyingChanges)
            {
                return;
            }
            
            Debug.Assert(
                (source == null && sourceProperty == null) || (source != null && !String.IsNullOrEmpty(sourceProperty)), 
                "source and sourceProperty should either both be present or both be absent.");
        
            Debug.Assert(target != null, "target must be provided by the caller.");
            Debug.Assert(BindingEntityInfo.IsEntityType(target.GetType()), "target must be an entity type.");

            // Do not handle add for already detached and deleted entities.
            if (source != null && this.IsDetachedOrDeletedFromContext(source))
            {
                return;
            }
            
            // Do we need an operation on context to handle the Add operation.
            EntityDescriptor targetDescriptor = this.Context.GetEntityDescriptor(target);

            // Following are the conditions where context operation is required:
            // 1. Not a call to Load or constructions i.e. we have Add behavior and not Attach behavior
            // 2. Target entity is not being tracked
            // 3. Target is being tracked but there is no link between the source and target entity and target is in non-deleted state
            bool contextOperationRequired = !this.AttachBehavior && 
                                           (targetDescriptor == null ||
                                           (source != null && !this.IsContextTrackingLink(source, sourceProperty, target) && targetDescriptor.State != EntityStates.Deleted));

            if (contextOperationRequired)
            {
                // First give the user code a chance to handle Add operation.
                if (this.CollectionChanged != null)
                {
                    EntityCollectionChangedParams args = new EntityCollectionChangedParams(
                            this.Context,
                            source,
                            sourceProperty,
                            sourceEntitySet,
                            collection,
                            target,
                            targetEntitySet,
                            NotifyCollectionChangedAction.Add);

                    if (this.CollectionChanged(args))
                    {
                        return;
                    }
                }
            }

            // The user callback code could detach the source.
            if (source != null && this.IsDetachedOrDeletedFromContext(source))
            {
                throw new InvalidOperationException(Strings.DataBinding_BindingOperation_DetachedSource);
            }

            // Default implementation.
            targetDescriptor = this.Context.GetEntityDescriptor(target);
            
            if (source != null)
            {
                if (this.AttachBehavior)
                {
                    // If the target entity is not being currently tracked, we attach both the 
                    // entity and the link between source and target entity.
                    if (targetDescriptor == null)
                    {
                        BindingUtils.ValidateEntitySetName(targetEntitySet, target);
                        
                        this.Context.AttachTo(targetEntitySet, target);
                        this.Context.AttachLink(source, sourceProperty, target);
                    }
                    else
                    if (targetDescriptor.State != EntityStates.Deleted && !this.IsContextTrackingLink(source, sourceProperty, target))
                    {
                        // If the target is already being tracked, then we attach the link if it
                        // does not already exist between the source and target entities and the
                        // target entity is not already in Deleted state.
                        this.Context.AttachLink(source, sourceProperty, target);
                    }
                }
                else
                {
                    // The target will be added and link from source to target will get established in the code
                    // below. Note that if there is already target present then we just try to establish the link
                    // however, if the link is also already established then we don't do anything.
                    if (targetDescriptor == null)
                    {
                        // If the entity is not tracked, that means the entity needs to
                        // be added to the context. We need to call AddRelatedObject,
                        // which adds via the parent (for e.g. POST Customers(0)/Orders).
                        this.Context.AddRelatedObject(source, sourceProperty, target);
                    }
                    else
                    if (targetDescriptor.State != EntityStates.Deleted && !this.IsContextTrackingLink(source, sourceProperty, target))
                    {
                        // If the entity is already tracked, then we just add the link. 
                        // However, we would not do it if the target entity is already
                        // in a Deleted state.
                        this.Context.AddLink(source, sourceProperty, target);
                    }
                }
            }
            else
            if (targetDescriptor == null)
            {
                // The source is null when the DataServiceCollection is the root collection.
                BindingUtils.ValidateEntitySetName(targetEntitySet, target);
                
                if (this.AttachBehavior)
                {
                    // Attach the target entity.
                    this.Context.AttachTo(targetEntitySet, target);
                }
                else
                {
                    // Add the target entity.
                    this.Context.AddObject(targetEntitySet, target);
                }
            }
        }

        /// <summary>Handle Deletes from a tracked DataServiceCollection. Perform operations on context to reflect the changes.</summary>
        /// <param name="source">The source object that reference the target object through a navigation property.</param>
        /// <param name="sourceProperty">The navigation property in the source object that reference the target object.</param>
        /// <param name="sourceEntitySet">The entity set of the source object.</param>
        /// <param name="collection">The collection containing the target object.</param>
        /// <param name="target">The target entity.</param>
        /// <param name="targetEntitySet">The entity set name of the target object.</param>
        internal void HandleDeleteEntity(
            object source,
            string sourceProperty,
            string sourceEntitySet,
            ICollection collection,
            object target,
            string targetEntitySet)
        {
            if (this.Context.ApplyingChanges)
            {
                return;
            }

            Debug.Assert(
                (source == null && sourceProperty == null) || (source != null && !String.IsNullOrEmpty(sourceProperty)),
                "source and sourceProperty should either both be present or both be absent.");

            Debug.Assert(target != null, "target must be provided by the caller.");
            Debug.Assert(BindingEntityInfo.IsEntityType(target.GetType()), "target must be an entity type.");

            Debug.Assert(!this.AttachBehavior, "AttachBehavior is only allowed during Construction and Load when this method should never be entered.");

            // Do not handle delete for already detached and deleted entities.
            if (source != null && this.IsDetachedOrDeletedFromContext(source))
            {
                return;
            }

            // Do we need an operation on context to handle the Delete operation. 
            // Detach behavior is special because it is only applicable in Clear 
            // cases, where we don't callback users for detach nofications.
            bool contextOperationRequired = this.IsContextTrackingEntity(target) && !this.DetachBehavior;
            
            if (contextOperationRequired)
            {
                // First give the user code a chance to handle Delete operation.
                if (this.CollectionChanged != null)
                {
                    EntityCollectionChangedParams args = new EntityCollectionChangedParams(
                            this.Context,
                            source,
                            sourceProperty,
                            sourceEntitySet,
                            collection,
                            target,
                            targetEntitySet,
                            NotifyCollectionChangedAction.Remove);

                    if (this.CollectionChanged(args))
                    {
                        return;
                    }
                }
            }

            // The user callback code could detach the source.
            if (source != null && !this.IsContextTrackingEntity(source))
            {
                throw new InvalidOperationException(Strings.DataBinding_BindingOperation_DetachedSource);
            }

            // Default implementation. 
            // Remove the entity from the context if it is currently being tracked.
            if (this.IsContextTrackingEntity(target))
            {
                if (this.DetachBehavior)
                {
                    this.Context.Detach(target);
                }
                else
                {
                    this.Context.DeleteObject(target);
                }
            }
        }

        /// <summary>Handle changes to navigation properties of a tracked entity. Perform operations on context to reflect the changes.</summary>
        /// <param name="source">The source object that reference the target object through a navigation property.</param>
        /// <param name="sourceProperty">The navigation property in the source object that reference the target object.</param>
        /// <param name="sourceEntitySet">The entity set of the source object.</param>
        /// <param name="target">The target entity.</param>
        /// <param name="targetEntitySet">The entity set name of the target object.</param>
        internal void HandleUpdateEntityReference(
            object source,
            string sourceProperty,
            string sourceEntitySet,
            object target,
            string targetEntitySet)
        {
            if (this.Context.ApplyingChanges)
            {
                return;
            }

            Debug.Assert(source != null, "source can not be null for update operations.");
            Debug.Assert(BindingEntityInfo.IsEntityType(source.GetType()), "source must be an entity with keys.");
            Debug.Assert(!String.IsNullOrEmpty(sourceProperty), "sourceProperty must be a non-empty string for update operations.");
            Debug.Assert(!String.IsNullOrEmpty(sourceEntitySet), "sourceEntitySet must be non-empty string for update operation.");

            // Do not handle update for detached and deleted entities.
            if (this.IsDetachedOrDeletedFromContext(source))
            {
                return;
            }

            // Do we need an operation on context to handle the Update operation.
            EntityDescriptor targetDescriptor = target != null ? this.Context.GetEntityDescriptor(target) : null;

            // Following are the conditions where context operation is required:
            // 1. Not a call to Load or constructions i.e. we have Add behavior and not Attach behavior
            // 2. Target entity is not being tracked
            // 3. Target is being tracked but there is no link between the source and target entity
            bool contextOperationRequired = !this.AttachBehavior && 
                                            (targetDescriptor == null ||
                                            !this.IsContextTrackingLink(source, sourceProperty, target));

            if (contextOperationRequired)
            {
                // First give the user code a chance to handle Update link operation.
                if (this.EntityChanged != null)
                {
                    EntityChangedParams args = new EntityChangedParams(
                                                    this.Context,
                                                    source,
                                                    sourceProperty,
                                                    target,
                                                    sourceEntitySet,
                                                    targetEntitySet);

                    if (this.EntityChanged(args))
                    {
                        return;
                    }
                }
            }

            // The user callback code could detach the source.
            if (this.IsDetachedOrDeletedFromContext(source))
            {
                throw new InvalidOperationException(Strings.DataBinding_BindingOperation_DetachedSource);
            }

            // Default implementation. 
            targetDescriptor = target != null ? this.Context.GetEntityDescriptor(target) : null;

            if (target != null)
            {
                if (targetDescriptor == null)
                {
                    // If the entity set name is not known, then we must throw since we need to know the 
                    // entity set in order to add/attach the referenced object to it's entity set.
                    BindingUtils.ValidateEntitySetName(targetEntitySet, target);
                    
                    if (this.AttachBehavior)
                    {
                        this.Context.AttachTo(targetEntitySet, target);
                    }
                    else
                    {
                        this.Context.AddObject(targetEntitySet, target);
                    }
                    
                    targetDescriptor = this.Context.GetEntityDescriptor(target);
                }

                // if the entity is already tracked, then just set/attach the link. However, do
                // not try to attach the link if the target is in Deleted state.
                if (!this.IsContextTrackingLink(source, sourceProperty, target))
                {
                    if (this.AttachBehavior)
                    {
                        if (targetDescriptor.State != EntityStates.Deleted)
                        {
                            this.Context.AttachLink(source, sourceProperty, target);
                        }
                    }
                    else
                    {
                        this.Context.SetLink(source, sourceProperty, target);
                    }
                }
            }
            else
            {
                Debug.Assert(!this.AttachBehavior, "During attach operations we must never perform operations for null values.");
                
                // The target could be null in which case we just need to set the link to null.
                this.Context.SetLink(source, sourceProperty, null);
            }
        }

        /// <summary>Determine if the DataServiceContext is tracking the specified entity.</summary>
        /// <param name="entity">An entity object.</param>
        /// <returns>true if the entity is tracked; otherwise false.</returns>
        internal bool IsContextTrackingEntity(object entity)
        {
            Debug.Assert(entity != null, "entity must be provided when checking for context tracking.");
            return this.Context.GetEntityDescriptor(entity) != default(EntityDescriptor);
        }

        /// <summary>
        /// Handle changes to an entity object tracked by the BindingObserver
        /// </summary>
        /// <param name="entity">The entity object that has changed.</param>
        /// <param name="propertyName">The property of the target entity object that has changed.</param>
        /// <param name="propertyValue">The value of the changed property of the target object.</param>
        private void HandleUpdateEntity(object entity, string propertyName, object propertyValue)
        {
            Debug.Assert(!this.AttachBehavior || this.Context.ApplyingChanges, "Entity updates must not happen during Attach or construction phases, deserialization case is the exception.");

            if (this.Context.ApplyingChanges)
            {
                return;
            }

            // For complex types, we will perform notification and update on the closest ancestor entity using the farthest ancestor complex property.
            if (!BindingEntityInfo.IsEntityType(entity.GetType()))
            {
                this.bindingGraph.GetAncestorEntityForComplexProperty(ref entity, ref propertyName, ref propertyValue);
            }

            Debug.Assert(entity != null, "entity must be provided for update operations.");
            Debug.Assert(BindingEntityInfo.IsEntityType(entity.GetType()), "entity must be an entity with keys.");
            Debug.Assert(!String.IsNullOrEmpty(propertyName) || propertyValue == null, "When propertyName is null no propertyValue should be provided.");

            // Do not handle update for detached and deleted entities.
            if (this.IsDetachedOrDeletedFromContext(entity))
            {
                return;
            }

            // First give the user code a chance to handle Update operation.
            if (this.EntityChanged != null)
            {
                EntityChangedParams args = new EntityChangedParams(
                                                this.Context, 
                                                entity, 
                                                propertyName, 
                                                propertyValue, 
                                                null, 
                                                null);

                if (this.EntityChanged(args))
                {
                    return;
                }
            }

            // Default implementation.
            // The user callback code could detach the entity.
            if (this.IsContextTrackingEntity(entity))
            {
                // Let UpdateObject check the state of the entity.
                this.Context.UpdateObject(entity);
            }
        }

        /// <summary>Processes the INotifyCollectionChanged.Add event.</summary>
        /// <param name="eventArgs">Event information such as added items.</param>
        /// <param name="source">Parent entity to which collection belongs.</param>
        /// <param name="sourceProperty">Parent entity property referring to collection.</param>
        /// <param name="targetEntitySet">Entity set of the collection.</param>
        /// <param name="collection">Collection that changed.</param>
        private void OnAddToCollection(
            NotifyCollectionChangedEventArgs eventArgs,
            object source,
            String sourceProperty,
            String targetEntitySet,
            object collection)
        {
            Debug.Assert(collection != null, "Must have a valid collection to which entities are added.");
            
            if (eventArgs.NewItems != null)
            {
                foreach (object target in eventArgs.NewItems)
                {
                    if (target == null)
                    {
                        throw new InvalidOperationException(Strings.DataBinding_BindingOperation_ArrayItemNull("Add"));
                    }

                    if (!BindingEntityInfo.IsEntityType(target.GetType()))
                    {
                        throw new InvalidOperationException(Strings.DataBinding_BindingOperation_ArrayItemNotEntity("Add"));
                    }

                    // Start tracking the target entity and synchronize the context with the Add operation.
                    this.bindingGraph.AddEntity(
                            source, 
                            sourceProperty, 
                            target, 
                            targetEntitySet, 
                            collection);
                }
            }
        }

        /// <summary>Processes the INotifyCollectionChanged.Remove event.</summary>
        /// <param name="eventArgs">Event information such as deleted items.</param>
        /// <param name="source">Parent entity to which collection belongs.</param>
        /// <param name="sourceProperty">Parent entity property referring to collection.</param>
        /// <param name="collection">Collection that changed.</param>
        private void OnDeleteFromCollection(
            NotifyCollectionChangedEventArgs eventArgs,
            object source,
            String sourceProperty,
            object collection)
        {
            Debug.Assert(collection != null, "Must have a valid collection from which entities are removed.");
            Debug.Assert(
                (source == null && sourceProperty == null) || (source != null && !String.IsNullOrEmpty(sourceProperty)), 
                "source and sourceProperty must both be null or both be non-null.");

            if (eventArgs.OldItems != null)
            {
                this.DeepRemoveCollection(
                        eventArgs.OldItems, 
                        source ?? collection, 
                        sourceProperty, 
                        this.ValidateCollectionItem);
            }
        }

        /// <summary>Removes a collection from the binding graph and detaches each item.</summary>
        /// <param name="collection">Collection whose elements are to be removed and detached.</param>
        private void RemoveWithDetachCollection(object collection)
        {
            Debug.Assert(this.DetachBehavior, "Must be detaching each item in collection.");

            object source = null;
            string sourceProperty = null;
            string sourceEntitySet = null;
            string targetEntitySet = null;

            this.bindingGraph.GetEntityCollectionInfo(
                    collection,
                    out source,
                    out sourceProperty,
                    out sourceEntitySet,
                    out targetEntitySet);

            this.DeepRemoveCollection(
                    this.bindingGraph.GetCollectionItems(collection),
                    source ?? collection,
                    sourceProperty,
                    null);
        }

        /// <summary>Performs a Deep removal of all entities in a collection.</summary>
        /// <param name="collection">Collection whose items are removed from binding graph.</param>
        /// <param name="source">Parent item whose property refer to the <paramref name="collection"/> being cleared.</param>
        /// <param name="sourceProperty">Property of the <paramref name="source"/> that refers to <paramref name="collection"/>.</param>
        /// <param name="itemValidator">Validation method if any that checks the individual item in <paramref name="collection"/> for validity.</param>
        private void DeepRemoveCollection(IEnumerable collection, object source, string sourceProperty, Action<object> itemValidator)
        {
            foreach (object target in collection)
            {
                if (itemValidator != null)
                {
                    itemValidator(target);
                }

                // Accumulate the list of entities to untrack, this includes deep added entities under target.
                List<UnTrackingInfo> untrackingInfo = new List<UnTrackingInfo>();

                this.CollectUnTrackingInfo(
                        target,
                        source,
                        sourceProperty,
                        untrackingInfo);

                // Stop tracking the collection of entities found by CollectUnTrackingInfo from bottom up in object graph.
                foreach (UnTrackingInfo info in untrackingInfo)
                {
                    this.bindingGraph.Remove(
                            info.Entity,
                            info.Parent,
                            info.ParentProperty);
                }
            }

            this.bindingGraph.RemoveUnreachableVertices();
        }

        /// <summary>Handle the DataServiceContext.SaveChanges operation.</summary>
        /// <param name="sender">DataServiceContext for the observer.</param>
        /// <param name="eventArgs">Information about SaveChanges operation results.</param>
        private void OnChangesSaved(object sender, SaveChangesEventArgs eventArgs)
        {
            // Does the response status code have to be checked? SaveChanges throws on failure.
            // DataServiceResponse response = eventArgs.Response;
            this.bindingGraph.RemoveNonTrackedEntities();
        }

        /// <summary>Collects a list of entities that observer is supposed to stop tracking</summary>
        /// <param name="currentEntity">Entity being delete along with it's children</param>
        /// <param name="parentEntity">Parent of the <paramref name="currentEntity"/></param>
        /// <param name="parentProperty">Property by which <paramref name="parentEntity"/> refers to <paramref name="currentEntity"/></param>
        /// <param name="entitiesToUnTrack">List in which entities to be untracked are collected</param>
        private void CollectUnTrackingInfo(
            object currentEntity, 
            object parentEntity, 
            string parentProperty, 
            IList<UnTrackingInfo> entitiesToUnTrack)
        {
            // We need to delete the child objects first before we delete the parent
            foreach (var ed in this.Context
                                   .Entities
                                   .Where(x => x.ParentEntity == currentEntity && x.State == EntityStates.Added))
            {
                this.CollectUnTrackingInfo(
                        ed.Entity, 
                        ed.ParentEntity, 
                        ed.ParentPropertyForInsert, 
                        entitiesToUnTrack);
            }
            
            entitiesToUnTrack.Add(new UnTrackingInfo 
                                  {
                                    Entity = currentEntity, 
                                    Parent = parentEntity, 
                                    ParentProperty = parentProperty
                                  });
        }

        /// <summary>Determine if the DataServiceContext is tracking link between <paramref name="source"/> and <paramref name="target"/>.</summary>
        /// <param name="source">The source object side of the link.</param>
        /// <param name="sourceProperty">A property in the source side of the link that references the target.</param>
        /// <param name="target">The target object side of the link.</param>
        /// <returns>True if the link is tracked; otherwise false.</returns>
        private bool IsContextTrackingLink(object source, string sourceProperty, object target)
        {
            Debug.Assert(source != null, "source entity must be provided.");
            Debug.Assert(BindingEntityInfo.IsEntityType(source.GetType()), "source must be an entity with keys.");

            Debug.Assert(!String.IsNullOrEmpty(sourceProperty), "sourceProperty must be provided.");

            Debug.Assert(target != null, "target entity must be provided.");
            Debug.Assert(BindingEntityInfo.IsEntityType(target.GetType()), "target must be an entity with keys.");
            
            return this.Context.GetLinkDescriptor(source, sourceProperty, target) != default(LinkDescriptor);
        }
        
        /// <summary>Checks whether the given entity is in detached or deleted state.</summary>
        /// <param name="entity">Entity being checked.</param>
        /// <returns>true if the entity is detached or deleted, otherwise returns false.</returns>
        private bool IsDetachedOrDeletedFromContext(object entity)
        {
            Debug.Assert(entity != null, "entity must be provided.");
            Debug.Assert(BindingEntityInfo.IsEntityType(entity.GetType()), "entity must be an entity with keys.");

            EntityDescriptor descriptor = this.Context.GetEntityDescriptor(entity);
            return descriptor == null || descriptor.State == EntityStates.Deleted;
        }

        /// <summary>Entity validator that checks if the <paramref name="target"/> is of entity type.</summary>
        /// <param name="target">Entity being validated.</param>
        private void ValidateCollectionItem(object target)
        {
            if (target == null)
            {
                throw new InvalidOperationException(Strings.DataBinding_BindingOperation_ArrayItemNull("Remove"));
            }

            if (!BindingEntityInfo.IsEntityType(target.GetType()))
            {
                throw new InvalidOperationException(Strings.DataBinding_BindingOperation_ArrayItemNotEntity("Remove"));
            }
        }

        #endregion

        /// <summary>Information regarding each entity to be untracked</summary>
        private class UnTrackingInfo
        {
            /// <summary>Entity to untrack</summary>
            public object Entity 
            { 
                get; 
                set; 
            }
            
            /// <summary>Parent object of <see cref="Entity"/></summary>
            public object Parent 
            { 
                get; 
                set; 
            }
            
            /// <summary>Parent object property referring to <see cref="Entity"/></summary>
            public string ParentProperty 
            { 
                get; 
                set; 
            }
        }
    }
}
