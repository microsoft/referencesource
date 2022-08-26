//---------------------------------------------------------------------
// <copyright file="BindingGraph.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   BindingGraph class
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

    /// <summary>
    /// Color of each vertex to be used for Depth First Search
    /// </summary>
    internal enum VertexColor
    {
        /// <summary>White color means un-visited</summary>
        White,
        
        /// <summary>Gray color means added to queue for DFS</summary>
        Gray,
        
        /// <summary>Black color means already visited hence reachable from root</summary>
        Black
    }

    /// <summary>
    /// The BindingGraph maps objects tracked by the DataServiceContext to vertices in a 
    /// graph used to manage the information needed for data binding. The objects tracked 
    /// by the BindingGraph are entity type objects and observable entity collections.
    /// </summary>
    internal sealed class BindingGraph
    {
        /// <summary>The observer of the graph</summary>
        private BindingObserver observer;

        /// <summary>Graph containing entities, collections and their relationships</summary>
        private Graph graph;

        /// <summary>Constructor</summary>
        /// <param name="observer">Observer of the graph</param>
        public BindingGraph(BindingObserver observer)
        {
            this.observer = observer;
            this.graph = new Graph();
        }

        /// <summary>Adds a collection to the graph</summary>
        /// <param name="source">Source object for the collection, this object has navigation property corresponding to collection</param>
        /// <param name="sourceProperty">Property in <paramref name="source"/> that corresponds to the collection</param>
        /// <param name="collection">Collection being added</param>
        /// <param name="collectionEntitySet">Entity set of entities in the collection</param>
        /// <returns>true if a new vertex had to be created, false if it already exists</returns>
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining | System.Runtime.CompilerServices.MethodImplOptions.NoOptimization)]
        public bool AddCollection(
            object source, 
            string sourceProperty, 
            object collection, 
            string collectionEntitySet)
        {
            Debug.Assert(collection != null, "'collection' can not be null");
            Debug.Assert(
                BindingEntityInfo.IsDataServiceCollection(collection.GetType()), 
                "Argument 'collection' must be an DataServiceCollection<T> of entity type T");

            if (this.graph.ExistsVertex(collection))
            {
                return false;
            }

            Vertex collectionVertex = this.graph.AddVertex(collection);
            collectionVertex.IsCollection = true;
            collectionVertex.EntitySet = collectionEntitySet;

            ICollection collectionItf = collection as ICollection;

            if (source != null)
            {
                collectionVertex.Parent = this.graph.LookupVertex(source);
                collectionVertex.ParentProperty = sourceProperty;
                this.graph.AddEdge(source, collection, sourceProperty);
                
                // Update the observer on the child collection
                Type entityType = BindingUtils.GetCollectionEntityType(collection.GetType());
                Debug.Assert(entityType != null, "Collection must at least be inherited from DataServiceCollection<T>");

                // Fail if the collection entity type does not implement INotifyPropertyChanged.
                if (!typeof(INotifyPropertyChanged).IsAssignableFrom(entityType))
                {
                    throw new InvalidOperationException(Strings.DataBinding_NotifyPropertyChangedNotImpl(entityType));
                }
                
                typeof(BindingGraph)
                    .GetMethod("SetObserver", BindingFlags.Instance | BindingFlags.NonPublic)
                    .MakeGenericMethod(entityType)
                    .Invoke(this, new object[] { collectionItf });
            }
            else
            {
                // When there is no source, then this vertex is the root vertex
                this.graph.Root = collectionVertex;
            }

            Debug.Assert(
                    collectionVertex.Parent != null || collectionVertex.IsRootCollection, 
                    "If parent is null, then collectionVertex should be a root collection");

            // Register for collection notifications
            this.AttachCollectionNotification(collection);

            // Perform deep add, by recursively adding entities in the collection
            foreach (var item in collectionItf)
            {
                this.AddEntity(
                        source, 
                        sourceProperty, 
                        item,
                        collectionEntitySet, 
                        collection);
            }

            return true;
        }

        /// <summary>Adds an entity to the graph</summary>
        /// <param name="source">Source object for the entity, this object has navigation property that links to entity</param>
        /// <param name="sourceProperty">Property in <paramref name="source"/> that links to entity</param>
        /// <param name="target">Entity being added</param>
        /// <param name="targetEntitySet">Entity set of entity being added</param>
        /// <param name="edgeSource">Item from which the directed edge in the graph goes into <paramref name="target"/>. This can be a collection</param>
        /// <returns>true if a new vertex had to be created, false if it already exists</returns>
        /// <remarks>
        /// This method processes the current 'target' entity and then recursively moves into the graph through
        /// the navigation properties. The 'source' is a previously processed item - it is the 'parent'
        /// of the target entity.
        /// The code generated EntitySetAttribute is processed by this method.
        /// A source entity can reference the target entity directly through an entity reference navigation property, 
        /// or indirectly through a collection navigation property.
        /// </remarks>
        public bool AddEntity(
            object source, 
            string sourceProperty, 
            object target, 
            string targetEntitySet, 
            object edgeSource)
        {
            Vertex sourceVertex = this.graph.LookupVertex(edgeSource);
            Debug.Assert(sourceVertex != null, "Must have a valid edge source");
            
            Vertex entityVertex = null;
            bool addedNewEntity = false;

            if (target != null)
            {
                entityVertex = this.graph.LookupVertex(target);

                if (entityVertex == null)
                {
                    entityVertex = this.graph.AddVertex(target);
                    
                    entityVertex.EntitySet = BindingEntityInfo.GetEntitySet(target, targetEntitySet);

                    // Register for entity notifications, fail if the entity does not implement INotifyPropertyChanged.
                    if (!this.AttachEntityOrComplexObjectNotification(target))
                    {
                        throw new InvalidOperationException(Strings.DataBinding_NotifyPropertyChangedNotImpl(target.GetType()));
                    }
                    
                    addedNewEntity = true;
                }

                // Add relationship. Connect the from end to the target.
                if (this.graph.ExistsEdge(edgeSource, target, sourceVertex.IsCollection ? null : sourceProperty))
                {
                    throw new InvalidOperationException(Strings.DataBinding_EntityAlreadyInCollection(target.GetType()));
                }

                this.graph.AddEdge(edgeSource, target, sourceVertex.IsCollection ? null : sourceProperty);
            }

            if (!sourceVertex.IsCollection)
            {
                this.observer.HandleUpdateEntityReference(
                        source, 
                        sourceProperty,
                        sourceVertex.EntitySet, 
                        target,
                        entityVertex == null ? null : entityVertex.EntitySet);
            }
            else
            {
                Debug.Assert(target != null, "Target must be non-null when adding to collections");
                this.observer.HandleAddEntity(
                        source, 
                        sourceProperty,
                        sourceVertex.Parent != null ? sourceVertex.Parent.EntitySet : null,
                        edgeSource as ICollection, 
                        target,
                        entityVertex.EntitySet);
            }

            if (addedNewEntity)
            {
                // Perform recursive add operation through target's properties
                this.AddFromProperties(target);
            }

            return addedNewEntity;
        }

        /// <summary>
        /// Removes the <paramref name="item"/> from the binding graph
        /// </summary>
        /// <param name="item">Item to remove</param>
        /// <param name="parent">Parent of the <paramref name="item"/></param>
        /// <param name="parentProperty">Parent property that refers to <paramref name="item"/></param>
        public void Remove(object item, object parent, string parentProperty)
        {
            Vertex vertexToRemove = this.graph.LookupVertex(item);
            if (vertexToRemove == null)
            {
                return;
            }

            Debug.Assert(!vertexToRemove.IsRootCollection, "Root collections are never removed");

            // Parent will always be non-null for deletes from collections, this will include 
            // both root and child collections. For root collections, parentProperty will be null.
            Debug.Assert(parent != null, "Parent has to be present.");

            // When parentProperty is null, parent is itself a root collection
            if (parentProperty != null)
            {
                BindingEntityInfo.BindingPropertyInfo bpi = BindingEntityInfo.GetObservableProperties(parent.GetType())
                                                                             .Single(p => p.PropertyInfo.PropertyName == parentProperty);
                Debug.Assert(bpi.PropertyKind == BindingPropertyKind.BindingPropertyKindCollection, "parentProperty must refer to an DataServiceCollection");

                parent = bpi.PropertyInfo.GetValue(parent);
            }

            object source = null;
            string sourceProperty = null;
            string sourceEntitySet = null;
            string targetEntitySet = null;

            this.GetEntityCollectionInfo(
                    parent,
                    out source,
                    out sourceProperty,
                    out sourceEntitySet,
                    out targetEntitySet);

            targetEntitySet = BindingEntityInfo.GetEntitySet(item, targetEntitySet);

            this.observer.HandleDeleteEntity(
                            source,
                            sourceProperty,
                            sourceEntitySet,
                            parent as ICollection,
                            item,
                            targetEntitySet);

            this.graph.RemoveEdge(parent, item, null);
        }

        /// <summary>Removes the collection from the graph</summary>
        /// <param name="collection">Collection to remove</param>
        public void RemoveCollection(object collection)
        {
            Vertex collectionVertex = this.graph.LookupVertex(collection);
            Debug.Assert(collectionVertex != null, "Must be tracking the vertex for the collection");
            
            foreach (Edge collectionEdge in collectionVertex.OutgoingEdges.ToList())
            {
                this.graph.RemoveEdge(collection, collectionEdge.Target.Item, null);
            }

            // This is where actual removal from graph happens, detach notifications for removed object
            this.RemoveUnreachableVertices();
        }

        /// <summary>Removes a relationship between two items based on source and relation label</summary>
        /// <param name="source">Source item</param>
        /// <param name="relation">Label for relation</param>
        public void RemoveRelation(object source, string relation)
        {
            Edge edge = this.graph
                            .LookupVertex(source)
                            .OutgoingEdges
                            .SingleOrDefault(e => e.Source.Item == source && e.Label == relation);
            if (edge != null)
            {
                this.graph.RemoveEdge(edge.Source.Item, edge.Target.Item, edge.Label);
            }

            // This is where actual removal from graph happens, detach notifications for removed object
            this.RemoveUnreachableVertices();
        }

#if DEBUG
        /// <summary>Checks to see if an object is being tracked by the graph</summary>
        /// <param name="item">Object being checked</param>
        /// <returns>true if the object exists in the graph, false otherwise</returns>
        public bool IsTracking(object item)
        {
            return this.graph.ExistsVertex(item);
        }
#endif
        /// <summary>Remove all non-tracked entities from the graph</summary>
        public void RemoveNonTrackedEntities()
        {
            // Cleanup all untracked entities
            foreach (var entity in this.graph.Select(o => BindingEntityInfo.IsEntityType(o.GetType()) && !this.observer.IsContextTrackingEntity(o)))
            {
                this.graph.ClearEdgesForVertex(this.graph.LookupVertex(entity));
            }
            
            this.RemoveUnreachableVertices();
        }

        /// <summary>
        /// Returns a sequence of items belonging to a collection. Uses the children of a collection
        /// vertex for this enumeration.
        /// </summary>
        /// <param name="collection">Collection being enumerated.</param>
        /// <returns>Sequence of items belonging to the collection.</returns>
        public IEnumerable<object> GetCollectionItems(object collection)
        {
            Vertex collectionVertex = this.graph.LookupVertex(collection);
            Debug.Assert(collectionVertex != null, "Must be tracking the vertex for the collection");
            foreach (Edge collectionEdge in collectionVertex.OutgoingEdges.ToList())
            {
                yield return collectionEdge.Target.Item;
            }
        }

        /// <summary>Reset the graph after detaching notifications for everything</summary>
        public void Reset()
        {
            this.graph.Reset(this.DetachNotifications);
        }

        /// <summary>Removes the un-reachable vertices from the graph and un-registers notification handlers</summary>
        public void RemoveUnreachableVertices()
        {
            // This is where actual removal from graph happens, detach notifications for removed object
            this.graph.RemoveUnreachableVertices(this.DetachNotifications);
        }

        /// <summary>Get the binding information for a collection</summary>
        /// <param name="collection">Collection</param>
        /// <param name="source">The source object that reference the target object through a navigation property.</param>
        /// <param name="sourceProperty">The navigation property in the source object that reference the target object.</param>
        /// <param name="sourceEntitySet">The entity set of the source object.</param>
        /// <param name="targetEntitySet">The entity set name of the target object.</param>
        public void GetEntityCollectionInfo(
            object collection,
            out object source,
            out string sourceProperty,
            out string sourceEntitySet,
            out string targetEntitySet)
        {
            Debug.Assert(collection != null, "Argument 'collection' cannot be null.");
            Debug.Assert(this.graph.ExistsVertex(collection), "Vertex corresponding to 'collection' must exist in the graph.");
            
            this.graph
                .LookupVertex(collection)
                .GetEntityCollectionInfo(
                    out source, 
                    out sourceProperty, 
                    out sourceEntitySet, 
                    out targetEntitySet);
        }

        /// <summary>
        /// Obtains the closest ancestor entity type in the graph corresponding to a complex object vertex.
        /// </summary>
        /// <param name="entity">On input this is a complex object, on output it is the closest entity ancestor.</param>
        /// <param name="propertyName">On input this is a complex object's member property name, on output it is the name of complex type property of the ancestor.</param>
        /// <param name="propertyValue">On input this is a complex object's member property value, on output it is the value of complex type property of the ancestor.</param>
        public void GetAncestorEntityForComplexProperty(
            ref object entity, 
            ref string propertyName, 
            ref object propertyValue)
        {
            Vertex childVertex = this.graph.LookupVertex(entity);
            Debug.Assert(childVertex != null, "Must have a vertex in the graph corresponding to the entity.");
            Debug.Assert(childVertex.IsComplex == true, "Vertex must correspond to a complex object.");
            
            while (childVertex.IsComplex)
            {
                propertyName = childVertex.IncomingEdges[0].Label;
                propertyValue = childVertex.Item;

                Debug.Assert(childVertex.Parent != null, "Complex properties must always have parent vertices.");
                entity = childVertex.Parent.Item;

                childVertex = childVertex.Parent;
            }
        }

        /// <summary>
        /// Adds a complex typed property to the graph for an object, also traverses all the child complex properties and adds them.
        /// </summary>
        /// <param name="source">Source entity object.</param>
        /// <param name="sourceProperty">Source entity property of complex type.</param>
        /// <param name="target">Target complex object property value.</param>
        public void AddComplexProperty(object source, string sourceProperty, object target)
        {
            Vertex parentVertex = this.graph.LookupVertex(source);
            Debug.Assert(parentVertex != null, "Must have a valid parent entity for complex properties.");
            Debug.Assert(target != null, "Must have non-null complex object reference.");

            Vertex complexVertex = this.graph.LookupVertex(target);

            if (complexVertex == null)
            {
                complexVertex = this.graph.AddVertex(target);
                complexVertex.Parent = parentVertex;
                complexVertex.IsComplex = true;

                // Register for complex type notifications, fail if the complex type does not implement INotifyPropertyChanged.
                if (!this.AttachEntityOrComplexObjectNotification(target))
                {
                    throw new InvalidOperationException(Strings.DataBinding_NotifyPropertyChangedNotImpl(target.GetType()));
                }
            }
            else
            {
                throw new InvalidOperationException(Strings.DataBinding_ComplexObjectAssociatedWithMultipleEntities(target.GetType()));
            }

            this.graph.AddEdge(source, target, sourceProperty);

            // Add nested properties for the complex object.
            this.AddFromProperties(target);
        }
        
        /// <summary>Add items to the graph, from the <paramref name="entity"/> object's properties</summary>
        /// <param name="entity">Object whose properties are to be explored</param>
        private void AddFromProperties(object entity)
        {
            // Once the entity is attached to the graph, we need to traverse all it's properties 
            // and add related entities and collections to this entity. 
            foreach (BindingEntityInfo.BindingPropertyInfo bpi in BindingEntityInfo.GetObservableProperties(entity.GetType()))
            {
                object propertyValue = bpi.PropertyInfo.GetValue(entity);

                if (propertyValue != null)
                {
                    switch (bpi.PropertyKind)
                    {
                        case BindingPropertyKind.BindingPropertyKindCollection:
                            this.AddCollection(
                                    entity,
                                    bpi.PropertyInfo.PropertyName,
                                    propertyValue,
                                    null);
                            
                            break;
                            
                        case BindingPropertyKind.BindingPropertyKindEntity:
                            this.AddEntity(
                                    entity,
                                    bpi.PropertyInfo.PropertyName,
                                    propertyValue,
                                    null,
                                    entity);
                            
                            break;
                            
                        default:
                            Debug.Assert(bpi.PropertyKind == BindingPropertyKind.BindingPropertyKindComplex, "Must be complex type if PropertyKind is not entity or collection.");
                            this.AddComplexProperty(
                                    entity, 
                                    bpi.PropertyInfo.PropertyName, 
                                    propertyValue);
                            break;
                    }
                }
            }
        }

        /// <summary>Attach the CollectionChanged handler to an DataServiceCollection.</summary>
        /// <param name="target">An DataServiceCollection.</param>
        private void AttachCollectionNotification(object target)
        {
            Debug.Assert(target != null, "Argument 'target' cannot be null");

            INotifyCollectionChanged notify = target as INotifyCollectionChanged;
            Debug.Assert(notify != null, "DataServiceCollection must implement INotifyCollectionChanged");

            notify.CollectionChanged -= this.observer.OnCollectionChanged;
            notify.CollectionChanged += this.observer.OnCollectionChanged;
        }

        /// <summary>Attach the PropertyChanged handler to an entity or complex object.</summary>
        /// <param name="target">An entity or complex object.</param>
        /// <returns>True if the target is attached; otherwise false.</returns>
        private bool AttachEntityOrComplexObjectNotification(object target)
        {
            Debug.Assert(target != null, "Argument 'target' cannot be null");

            INotifyPropertyChanged notify = target as INotifyPropertyChanged;
            if (notify != null)
            {
                notify.PropertyChanged -= this.observer.OnPropertyChanged;
                notify.PropertyChanged += this.observer.OnPropertyChanged;
                return true;
            }

            return false;
        }

        /// <summary>Detach CollectionChanged or PropertyChanged handlers from the target</summary>
        /// <param name="target">An entity object or collection</param>
        private void DetachNotifications(object target)
        {
            Debug.Assert(target != null, "Argument 'target' cannot be null");
            
            this.DetachCollectionNotifications(target);

            INotifyPropertyChanged notifyPropertyChanged = target as INotifyPropertyChanged;
            if (notifyPropertyChanged != null)
            {
                notifyPropertyChanged.PropertyChanged -= this.observer.OnPropertyChanged;
            }
        }

        /// <summary>Detach CollectionChanged handlers from the target</summary>
        /// <param name="target">A collection object</param>
        private void DetachCollectionNotifications(object target)
        {
            Debug.Assert(target != null, "Argument 'target' cannot be null");

            INotifyCollectionChanged notifyCollectionChanged = target as INotifyCollectionChanged;
            if (notifyCollectionChanged != null)
            {
                notifyCollectionChanged.CollectionChanged -= this.observer.OnCollectionChanged;
            }
        }

        /// <summary>
        /// Sets the observer for a child DataServiceCollection
        /// </summary>
        /// <typeparam name="T">Entity type for the collection</typeparam>
        /// <param name="collection">Non-typed collection interface</param>
        private void SetObserver<T>(ICollection collection)
        {
            DataServiceCollection<T> oec = collection as DataServiceCollection<T>;
            oec.Observer = this.observer;
        }

        /// <summary>Graph implementation for tracking entities, collections for binding</summary>
        internal sealed class Graph
        {
            /// <summary>Vertices of the graph, which also hold edges</summary>
            private Dictionary<object, Vertex> vertices;

            /// <summary>The root vertex for the graph, DFS traversals start from this vertex</summary>
            private Vertex root;
            
            /// <summary>Constructor</summary>
            public Graph()
            {
                this.vertices = new Dictionary<object, Vertex>(ReferenceEqualityComparer<object>.Instance);
            }

            /// <summary>Root vertex of the graph</summary>
            public Vertex Root
            {
                get
                {
                    Debug.Assert(this.root != null, "Must have a non-null root vertex when this call is made.");
                    return this.root;
                }
                
                set
                {
                    Debug.Assert(this.root == null, "Must only initialize root vertex once.");   
                    Debug.Assert(this.ExistsVertex(value.Item), "Must already have the assigned vertex in the graph.");
                    this.root = value;
                }
            }

            /// <summary>Adds vertex to the graph</summary>
            /// <param name="item">Item corresponding to vertex</param>
            /// <returns>Newly created vertex</returns>
            public Vertex AddVertex(object item)
            {
                Vertex v = new Vertex(item);
                this.vertices.Add(item, v);
                return v;
            }

            /// <summary>Removes all edges going out of and coming into the given vertex</summary>
            /// <param name="v">Vertex whose edges are to be cleared</param>
            public void ClearEdgesForVertex(Vertex v)
            {
                foreach (Edge e in v.OutgoingEdges.Concat(v.IncomingEdges).ToList())
                {
                    this.RemoveEdge(e.Source.Item, e.Target.Item, e.Label);
                }
            }

            /// <summary>
            /// Checks if a vertex exists corresponding to given <paramref name="item"/>
            /// </summary>
            /// <param name="item">Item to lookup</param>
            /// <returns>true if vertex found, false otherwise</returns>
            public bool ExistsVertex(object item)
            {
                Vertex v;
                return this.vertices.TryGetValue(item, out v);
            }

            /// <summary>Looksup the vertex corresponding to <paramref name="item"/></summary>
            /// <param name="item">Item to lookup</param>
            /// <returns>Vertex corresponding to item</returns>
            public Vertex LookupVertex(object item)
            {
                Vertex v;
                this.vertices.TryGetValue(item, out v);
                return v;
            }

            /// <summary>
            /// Adds edge between vertices corresponding to <paramref name="source"/> and <paramref name="target"/>
            /// objects which will be labeled with <paramref name="label"/>
            /// </summary>
            /// <param name="source">Outgoing end of the edge</param>
            /// <param name="target">Incoming end of the edge</param>
            /// <param name="label">Label for the vertex</param>
            /// <returns>Newly created edge</returns>
            public Edge AddEdge(object source, object target, string label)
            {
                Vertex s = this.vertices[source];
                Vertex t = this.vertices[target];
                Edge e = new Edge { Source = s, Target = t, Label = label };
                s.OutgoingEdges.Add(e);
                t.IncomingEdges.Add(e);
                return e;
            }

            /// <summary>
            /// Removes edge between vertices corresponding to <paramref name="source"/> and <paramref name="target"/>
            /// objects which was labeled with <paramref name="label"/>
            /// </summary>
            /// <param name="source">Outgoing end of the edge</param>
            /// <param name="target">Incoming end of the edge</param>
            /// <param name="label">Label for the vertex</param>
            public void RemoveEdge(object source, object target, string label)
            {
                Vertex s = this.vertices[source];
                Vertex t = this.vertices[target];
                Edge e = new Edge { Source = s, Target = t, Label = label };
                s.OutgoingEdges.Remove(e);
                t.IncomingEdges.Remove(e);
            }

            /// <summary>
            /// Checks if an edge exists between <paramref name="source"/> and <paramref name="target"/> labeled
            /// with <paramref name="label"/>
            /// </summary>
            /// <param name="source">Outgoing end of the edge</param>
            /// <param name="target">Incoming end of the edge</param>
            /// <param name="label">Label for the vertex</param>
            /// <returns>true if an edge exists between source and target with given label, false otherwise</returns>
            public bool ExistsEdge(object source, object target, string label)
            {
                Edge e = new Edge { Source = this.vertices[source], Target = this.vertices[target], Label = label };
                return this.vertices[source].OutgoingEdges.Any(r => r.Equals(e));
            }

            /// <summary>
            /// Selects collection of objects tracked by the graph based on the given filter
            /// </summary>
            /// <param name="filter">Filter for the objects</param>
            /// <returns>Filtered list of objects tracked by the graph</returns>
            public IList<object> Select(Func<object, bool> filter)
            {
                return this.vertices.Keys.Where(filter).ToList();
            }

            /// <summary>
            /// Removes everything from the graph after applying <paramref name="action"/>
            /// </summary>
            /// <param name="action">Action to apply before removal of each node</param>
            public void Reset(Action<object> action)
            {
                foreach (object obj in this.vertices.Keys)
                {
                    action(obj);
                }

                this.vertices.Clear();
            }

            /// <summary>Remove all vertices from graph that are unreachable from the root collection vertex</summary>
            /// <param name="detachAction">Action to perform for each removed vertex</param>
            public void RemoveUnreachableVertices(Action<object> detachAction)
            {
                try
                {
                    foreach (Vertex v in this.UnreachableVertices())
                    {
                        this.ClearEdgesForVertex(v);
                        detachAction(v.Item);
                        this.vertices.Remove(v.Item);
                    }
                }
                finally
                {
                    // Reset color for all vertices back to white.
                    foreach (Vertex v in this.vertices.Values)
                    {
                        v.Color = VertexColor.White;
                    }
                }
            }
            
            /// <summary>Collects all vertices unreachable from the root collection vertex</summary>
            /// <returns>Sequence of vertices that are unreachable from the root collection vertex</returns>
            /// <remarks>
            /// Performs a depth first traversal of the graph starting from the root collection
            /// vertex and checks if some vertices were unreachable was reached while doing the traversal.
            /// Alogrithm from Introduction to Algorithms 22.2 by Cormen et al.
            /// </remarks>
            private IEnumerable<Vertex> UnreachableVertices()
            {
                Queue<Vertex> q = new Queue<Vertex>();
                
                this.Root.Color = VertexColor.Gray;
                q.Enqueue(this.Root);
                
                while (q.Count != 0)
                {
                    Vertex current = q.Dequeue();
                    
                    foreach (Edge e in current.OutgoingEdges)
                    {
                        if (e.Target.Color == VertexColor.White)
                        {
                            e.Target.Color = VertexColor.Gray;
                            q.Enqueue(e.Target);
                        }
                    }
                    
                    current.Color = VertexColor.Black;
                }
                
                return this.vertices.Values.Where(v => v.Color == VertexColor.White).ToList();
            }
        }

        /// <summary>Vertex of the <see cref="Graph"/></summary>
        internal sealed class Vertex
        {
            /// <summary>Collection of incoming edges for the vertex</summary>
            private List<Edge> incomingEdges;
            
            /// <summary>Collection of outgoing edges for the vertex</summary>
            private List<Edge> outgoingEdges;

            /// <summary>Constructor</summary>
            /// <param name="item">Item corresponding to vertex</param>
            public Vertex(object item)
            {
                Debug.Assert(item != null, "item must be non-null");
                this.Item = item;
                this.Color = VertexColor.White;
            }

            /// <summary>Item corresponding to the vertex</summary>
            public object Item
            {
                get;
                private set;
            }

            /// <summary>Entity set of the item held by the vertex</summary>
            public string EntitySet
            {
                get;
                set;
            }

            /// <summary>Is item a collection object</summary>
            public bool IsCollection
            {
                get;
                set;
            }

            /// <summary>Is item a complex type object</summary>
            public bool IsComplex
            {
                get;
                set;
            }

            /// <summary>Parent vertex, only exists for non-top level collection vertices or complex objects</summary>
            public Vertex Parent
            {
                get;
                set;
            }

            /// <summary>Property of the <see cref="Parent"/> object that associates this vertex with it's parent</summary>
            public string ParentProperty
            {
                get;
                set;
            }

            /// <summary>Is item a root collection object</summary>
            public bool IsRootCollection
            {
                get
                {
                    return this.IsCollection && this.Parent == null;
                }
            }

            /// <summary>Color of the vertex</summary>
            public VertexColor Color
            {
                get;
                set;
            }

            /// <summary>Edges coming into this vertex</summary>
            public IList<Edge> IncomingEdges
            {
                get
                {
                    if (this.incomingEdges == null)
                    {
                        this.incomingEdges = new List<Edge>();
                    }

                    return this.incomingEdges;
                }
            }

            /// <summary>Edges going out of this vertex</summary>
            public IList<Edge> OutgoingEdges
            {
                get
                {
                    if (this.outgoingEdges == null)
                    {
                        this.outgoingEdges = new List<Edge>();
                    }

                    return this.outgoingEdges;
                }
            }

            /// <summary>Get the binding information for a collection vertex</summary>
            /// <param name="source">The source object that reference the target object through a navigation property corresponding to current collection vertex.</param>
            /// <param name="sourceProperty">The navigation property in the source object that reference the target object.</param>
            /// <param name="sourceEntitySet">The entity set of the source object.</param>
            /// <param name="targetEntitySet">The entity set of the target object.</param>
            public void GetEntityCollectionInfo(
                out object source,
                out string sourceProperty,
                out string sourceEntitySet,
                out string targetEntitySet)
            {
                Debug.Assert(this.IsCollection, "Must be a collection to be in this method");

                if (!this.IsRootCollection)
                {
                    Debug.Assert(this.Parent != null, "Parent must be non-null for child collection");
                    
                    source = this.Parent.Item;
                    Debug.Assert(source != null, "Source object must be present for child collection");

                    sourceProperty = this.ParentProperty;
                    Debug.Assert(sourceProperty != null, "Source entity property associated with a child collection must be non-null");

#if DEBUG
                    PropertyInfo propertyInfo = source.GetType().GetProperty(sourceProperty);
                    Debug.Assert(propertyInfo != null, "Unable to get information for the source entity property associated with a child collection");
#endif
                    sourceEntitySet = this.Parent.EntitySet;
                }
                else
                {
                    Debug.Assert(this.Parent == null, "Parent must be null for top level collection");
                    source = null;
                    sourceProperty = null;
                    sourceEntitySet = null;
                }

                targetEntitySet = this.EntitySet;
            }
        }

        /// <summary>
        /// Edge between two vertices of graph, directed and labeled
        /// </summary>
        internal sealed class Edge : IEquatable<Edge>
        {
            /// <summary>Source vertex</summary>
            public Vertex Source
            {
                get;
                set;
            }

            /// <summary>Target vertex</summary>
            public Vertex Target
            {
                get;
                set;
            }

            /// <summary>Label of the edge</summary>
            public string Label
            {
                get;
                set;
            }

            /// <summary>IEquatable override</summary>
            /// <param name="other">Comparand</param>
            /// <returns>true if equal, false otherwise</returns>
            public bool Equals(Edge other)
            {
                return other != null &&
                    Object.ReferenceEquals(this.Source, other.Source) &&
                    Object.ReferenceEquals(this.Target, other.Target) &&
                    this.Label == other.Label;
            }
        }    
    }
}
