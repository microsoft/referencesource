//---------------------------------------------------------------------
// <copyright file="DataServiceCollectionOfT.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//   DataServiceCollection class
// </summary>
//
//---------------------------------------------------------------------
namespace System.Data.Services.Client
{
    #region Namespaces.

    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;

    #endregion Namespaces.

    /// <summary>
    /// Enumeration used for cunstructing DataServiceCollection which specifies
    /// whether the collection should be in automatic change tracking mode or in manual (none)
    /// change tracking mode.
    /// </summary>
    public enum TrackingMode
    {
        /// <summary>The collection should not track changes.</summary>
        None,

        /// <summary>The collection should automatically track changes to the entities
        /// in the collection.</summary>
        AutoChangeTracking
    }

    /// <summary>
    /// An DataServiceCollection is a collection of entites.
    /// The collection implement INotifyCollectionChanged and INotifyPropertyChanged.
    /// </summary>
    /// <typeparam name="T">An entity class</typeparam>
    public class DataServiceCollection<T> : ObservableCollection<T>
    {
        #region Private fields.

        /// <summary>The BindingObserver associated with the DataServiceCollection</summary>
        private BindingObserver observer;

        /// <summary>Is this a root collection</summary>
        private bool rootCollection;

        /// <summary>The continuation for partial collections.</summary>
        private DataServiceQueryContinuation<T> continuation;

        /// <summary>True if tracking setup was deferred to first Load() call.</summary>
        private bool trackingOnLoad;

        /// <summary>Callback tracked until tracking is enabled.</summary>
        private Func<EntityChangedParams, bool> entityChangedCallback;

        /// <summary>Callback tracked until tracking is enabled.</summary>
        private Func<EntityCollectionChangedParams, bool> collectionChangedCallback;

        /// <summary>Entity set name tracked until tracking is enabled.</summary>
        private string entitySetName;

#if ASTORIA_LIGHT
        /// <summary>If there's an async operation in progress (LoadAsync methods), this field is true
        /// otherwise it's false.</summary>
        private bool asyncOperationInProgress;
#endif

        #endregion Private fields.

        /// <summary>
        /// Creates a default data service collection, with auto-change tracking enabled
        /// as soon as data is loaded into it.
        /// </summary>
        public DataServiceCollection()
            : this(null, null, TrackingMode.AutoChangeTracking, null, null, null)
        {
        }

        /// <summary>
        /// Creates a tracking DataServiceCollection and pre-loads it
        /// </summary>
        /// <param name="items">Collection to initialize the new DataServiceCollection with.</param> 
        public DataServiceCollection(IEnumerable<T> items)
            : this(null, items, TrackingMode.AutoChangeTracking, null, null, null)
        {
        }

        /// <summary>
        /// Creates a tracking DataServiceCollection and pre-loads it, enabling or disabling auto-change tracking as needed
        /// </summary>
        /// <param name="items">Collection to initialize the new DataServiceCollection with.</param> 
        /// <param name="trackingMode">Whether auto-change tracking should be enabled</param>
        public DataServiceCollection(IEnumerable<T> items, TrackingMode trackingMode)
            : this(null, items, trackingMode, null, null, null)
        {
        }

        /// <summary>
        /// Creates a data service collection associated to the provided context for
        /// the purpose of auto-change tracking.
        /// </summary>
        /// <param name="context">DataServiceContext associated with the new collection.</param>
        public DataServiceCollection(DataServiceContext context)
            : this(context, null, TrackingMode.AutoChangeTracking, null, null, null)
        {
        }

        /// <summary>Creates a new DataServiceCollection.</summary>
        /// <param name="context">DataServiceContext associated with the new collection.</param>
        /// <param name="entitySetName">The entity set of the elements in the collection.</param>
        /// <param name="entityChangedCallback">Delegate that would get called when an entity changes.</param>
        /// <param name="collectionChangedCallback">Delegate that would get called when an entity collection changes.</param>  
        public DataServiceCollection(
            DataServiceContext context, 
            string entitySetName, 
            Func<EntityChangedParams, bool> entityChangedCallback, 
            Func<EntityCollectionChangedParams, bool> collectionChangedCallback)
            : this(context, null, TrackingMode.AutoChangeTracking, entitySetName, entityChangedCallback, collectionChangedCallback)
        {
        }

        /// <summary>Creates a new DataServiceCollection.</summary>
        /// <param name="items">Enumeration of items to initialize the new DataServiceCollection with.</param>       
        /// <param name="trackingMode">The tracking mode for the new collection.</param>
        /// <param name="entitySetName">The name of the entity set the elements in the collection belong to.</param>
        /// <param name="entityChangedCallback">Delegate that gets called when an entity changes.</param>
        /// <param name="collectionChangedCallback">Delegate that gets called when an entity collection changes.</param> 
        public DataServiceCollection(
            IEnumerable<T> items, 
            TrackingMode trackingMode, 
            string entitySetName, 
            Func<EntityChangedParams, bool> entityChangedCallback, 
            Func<EntityCollectionChangedParams, bool> collectionChangedCallback)
            : this(null, items, trackingMode, entitySetName, entityChangedCallback, collectionChangedCallback)
        {
        }

        /// <summary>Creates a new DataServiceCollection.</summary>
        /// <param name="context"><see cref="DataServiceContext"/> associated with the new collection.</param>
        /// <param name="items">Enumeration of items to initialize the new DataServiceCollection with.</param>       
        /// <param name="trackingMode">The tracking mode for the new collection.</param>
        /// <param name="entitySetName">The name of the entity set the elements in the collection belong to.</param>
        /// <param name="entityChangedCallback">Delegate that gets called when an entity changes.</param>
        /// <param name="collectionChangedCallback">Delegate that gets called when an entity collection changes.</param> 
        public DataServiceCollection(
            DataServiceContext context,
            IEnumerable<T> items,
            TrackingMode trackingMode,
            string entitySetName,
            Func<EntityChangedParams, bool> entityChangedCallback,
            Func<EntityCollectionChangedParams, bool> collectionChangedCallback)
        {
            if (trackingMode == TrackingMode.AutoChangeTracking)
            {
                if (context == null)
                {
                    if (items == null)
                    {
                        // Enable tracking on first Load/LoadAsync call, when we can obtain a context
                        this.trackingOnLoad = true;

                        // Save off these for when we enable tracking later
                        this.entitySetName = entitySetName;
                        this.entityChangedCallback = entityChangedCallback;
                        this.collectionChangedCallback = collectionChangedCallback;
                    }
                    else
                    {
                        // This throws if no context can be obtained, no need to check here
                        context = DataServiceCollection<T>.GetContextFromItems(items);
                    }
                }

                if (!this.trackingOnLoad)
                {
                    if (items != null)
                    {
                        DataServiceCollection<T>.ValidateIteratorParameter(items);
                    }

                    this.StartTracking(context, items, entitySetName, entityChangedCallback, collectionChangedCallback);
                }
            }
            else if (items != null)
            {
                this.Load(items);
            }
        }

        /// <summary>Creates new DataServiceCollection.</summary>
        /// <param name="atomMaterializer">The materializer</param>
        /// <param name="context"><see cref="DataServiceContext"/> associated with the new collection.</param>
        /// <param name="items">Enumeration of items to initialize the new DataServiceCollection with.</param>       
        /// <param name="trackingMode">The tracking mode for the new collection.</param>
        /// <param name="entitySetName">The name of the entity set the elements in the collection belong to.</param>
        /// <param name="entityChangedCallback">Delegate that gets called when an entity changes.</param>
        /// <param name="collectionChangedCallback">Delegate that gets called when an entity collection changes.</param> 
        /// <remarks>This is the internal constructor called from materializer and used inside our projection queries.</remarks>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1800", Justification = "Constructor and debug-only code can't reuse cast.")]
        internal DataServiceCollection(
            object atomMaterializer,
            DataServiceContext context,
            IEnumerable<T> items,
            TrackingMode trackingMode,
            string entitySetName,
            Func<EntityChangedParams, bool> entityChangedCallback,
            Func<EntityCollectionChangedParams, bool> collectionChangedCallback)
            : this(
                context != null ? context : ((AtomMaterializer)atomMaterializer).Context, 
                items, 
                trackingMode, 
                entitySetName, 
                entityChangedCallback, 
                collectionChangedCallback)
        {
            Debug.Assert(atomMaterializer != null, "atomMaterializer != null");
            Debug.Assert(((AtomMaterializer)atomMaterializer).Context != null, "Context != null");

            if (items != null)
            {
                ((AtomMaterializer)atomMaterializer).PropagateContinuation(items, this);
            }
        }

        #region Properties

        /// <summary>The continuation for additional results; null if none are available.</summary>
        public DataServiceQueryContinuation<T> Continuation
        {
            get { return this.continuation; }
            set { this.continuation = value; }
        }
        
        /// <summary>Observer for the collection.</summary>
        /// <remarks>The setter would get called only for child collections in the graph.</remarks>
        internal BindingObserver Observer
        {
            get
            {
                return this.observer;
            }
        
            set
            {
                Debug.Assert(!this.rootCollection, "Must be a child collection to have the Observer setter called.");
                Debug.Assert(typeof(System.ComponentModel.INotifyPropertyChanged).IsAssignableFrom(typeof(T)), "The entity type must be trackable (by implementing INotifyPropertyChanged interface)");
                this.observer = value;
            }
        }

        /// <summary>
        /// Whether this collection is actively tracking
        /// </summary>
        internal bool IsTracking
        {
            get { return this.observer != null; }
        }
        
        #endregion

        /// <summary>Loads the collection from another collection.</summary>
        /// <param name="items">Collection whose elements will be loaded into the DataServiceCollection.</param>
        /// <remarks>
        /// When tracking is enabled, the behavior of Load would be to attach all those entities that are not already tracked by the context
        /// associated with the collection. The operation will go deep into the input entities so that all related
        /// entities are attached to the context if not already present. All entities in <paramref name="items"/>
        /// will be tracked after Load is done.
        /// Load method checks for duplication. The collection will ignore any duplicated items been loaded.
        /// For large amount of items, consider DataServiceContext.LoadProperty instead.
        /// </remarks>
        public void Load(IEnumerable<T> items)
        {
            DataServiceCollection<T>.ValidateIteratorParameter(items);

            if (this.trackingOnLoad)
            {
                // This throws if no context can be obtained, no need to check here
                DataServiceContext context = DataServiceCollection<T>.GetContextFromItems(items);

                this.trackingOnLoad = false;

                this.StartTracking(context, items, this.entitySetName, this.entityChangedCallback, this.collectionChangedCallback);
            }
            else
            {
                this.StartLoading();
                try
                {
                    this.InternalLoadCollection(items);
                }
                finally
                {
                    this.FinishLoading();
                }
            }
        }

#if ASTORIA_LIGHT
        /// <summary>A completion event for the <see cref="LoadAsync(IQueryable&lt;T&gt;)"/>, <see cref="LoadAsync()"/> 
        /// and <see cref="LoadNextPartialSetAsync"/> method.</summary>
        /// <remarks>This event is raised exactly once for each call to the <see cref="LoadAsync(IQueryable&lt;T&gt;)"/>, 
        /// <see cref="LoadAsync()"/> or <see cref="LoadNextPartialSetAsync"/> method. It is called both when the operation 
        /// succeeded and/or when it failed.</remarks>
        public event EventHandler<LoadCompletedEventArgs> LoadCompleted;

        /// <summary>Loads the collection asynchronously from a DataServiceQuery instance.</summary>
        /// <param name="query">A query of type DataServiceQuery.</param>
        /// <remarks>This method uses the event-based async pattern. 
        /// The method returns immediately without waiting for the query to complete. Then it calls the handler of the
        /// <see cref="LoadCompleted"/> event exactly once on the UI thread. The event will be raised regradless
        /// if the query succeeded or not.
        /// This class only support one asynchronous operation in flight.</remarks>
        public void LoadAsync(IQueryable<T> query)
        {
            Util.CheckArgumentNull(query, "query");
            DataServiceQuery<T> dsq = query as DataServiceQuery<T>;
            if (dsq == null)
            {
                throw new ArgumentException(Strings.DataServiceCollection_LoadAsyncRequiresDataServiceQuery, "query");
            }

            if (this.asyncOperationInProgress)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_MultipleLoadAsyncOperationsAtTheSameTime);
            }

            if (this.trackingOnLoad)
            {
                this.StartTracking(((DataServiceQueryProvider)dsq.Provider).Context,
                                   null,
                                   this.entitySetName,
                                   this.entityChangedCallback,
                                   this.collectionChangedCallback);
                this.trackingOnLoad = false;
            }

            BeginLoadAsyncOperation(
                asyncCallback => dsq.BeginExecute(asyncCallback, null),
                asyncResult => 
                    {
                        QueryOperationResponse<T> response = (QueryOperationResponse<T>)dsq.EndExecute(asyncResult);
                        this.Load(response);
                        return response;
                    });
        }

        /// <summary>Loads the collection asynchronously for a property represented by the DataServiceCollection.</summary>
        /// <remarks>This method loads the content of a property represented by this DataServiceCollection.
        /// If this instance is not associated with any property and entity the method will fail.
        /// This method uses the event-based async pattern. 
        /// The method returns immediately without waiting for the query to complete. Then it calls the handler of the
        /// <see cref="LoadCompleted"/> event exactly once on the UI thread. The event will be raised regradless
        /// if the query succeeded or not.
        /// This class only support one asynchronous operation in flight.</remarks>
        public void LoadAsync()
        {
            if (!this.IsTracking)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_OperationForTrackedOnly);
            }

            object parent;
            string property;
            if (!this.observer.LookupParent(this, out parent, out property))
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_LoadAsyncNoParamsWithoutParentEntity);
            }

            if (this.asyncOperationInProgress)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_MultipleLoadAsyncOperationsAtTheSameTime);
            }

            BeginLoadAsyncOperation(
                asyncCallback => this.observer.Context.BeginLoadProperty(parent, property, asyncCallback, null),
                asyncResult => (QueryOperationResponse)this.observer.Context.EndLoadProperty(asyncResult));
        }

        /// <summary>Loads next partial set for this collection.</summary>
        /// <returns>If this collection doesn't have a continuation token (this.Continuation == null) then this method
        /// returns false and does not issue any request.
        /// If there is a continuation token the method will return true and will start a request to load
        /// the next partial set based on that continuation token.</returns>
        /// <remarks>This method is the same as <see cref="LoadAsync(IQueryable&lt;T&gt;)"/> except that it runs the query as defined
        /// by the continuation token of this collection.
        /// The method returns immediately without waiting for the query to complete. Then it calls the handler of the
        /// <see cref="LoadCompleted"/> event exactly once on the UI thread. The event will be raised regradless
        /// if the query succeeded or not. Even if the method returns false, the event will be raised (immeditaly)
        /// This class only support one asynchronous operation in flight.</remarks>
        public bool LoadNextPartialSetAsync()
        {
            if (!this.IsTracking)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_OperationForTrackedOnly);
            }

            if (this.asyncOperationInProgress)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_MultipleLoadAsyncOperationsAtTheSameTime);
            }

            if (this.Continuation == null)
            {
                if (this.LoadCompleted != null)
                {
                    this.LoadCompleted(this, new LoadCompletedEventArgs(null, null));
                }
                return false;
            }
            
            BeginLoadAsyncOperation(
                asyncCallback => this.observer.Context.BeginExecute(this.Continuation, asyncCallback, null),
                asyncResult =>
                    {
                        QueryOperationResponse<T> response = (QueryOperationResponse<T>)this.observer.Context.EndExecute<T>(asyncResult);
                        this.Load(response);
                        return response;
                    });

            return true;
        }

#endif

        /// <summary>Loads a single entity into the collection.</summary>
        /// <param name="item">Entity to be added.</param>
        /// <remarks>
        /// When tracking is enabled, the behavior of Load would be to attach the entity if it is not already tracked by the context
        /// associated with the collection. The operation will go deep into the input entity so that all related
        /// entities are attached to the context if not already present. The <paramref name="item"/> will be 
        /// tracked after Load is done.        
        /// Load method checks for duplication. The collection will ignore any duplicated items been loaded.
        /// </remarks>
        public void Load(T item)
        {
            // When loading a single item,
            if (item == null)
            {
                throw Error.ArgumentNull("item");
            }

            this.StartLoading();
            try
            {
                if (!this.Contains(item))
                {
                    this.Add(item);
                }
            }
            finally
            {
                this.FinishLoading();
            }
        }

        /// <summary>
        /// Clears a collection and optionally detaches all the items in it including deep added items from the context
        /// </summary>
        /// <param name="stopTracking">true if detach from context must happen, false otherwise. This parameter only affects tracked collections.</param>
        public void Clear(bool stopTracking)
        {
            if (!this.IsTracking)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_OperationForTrackedOnly);
            }

            if (!stopTracking)
            {
                // non-binding or just clear
                this.Clear();
            }
            else
            {
                Debug.Assert(this.observer.Context != null, "Must have valid context when the collection is being observed.");
                try
                {
                    this.observer.DetachBehavior = true;
                    this.Clear();
                }
                finally
                {
                    this.observer.DetachBehavior = false;
                }
            }
        }
        
        /// <summary>Stop tracking the collection. The operation is only allowed for root collections.</summary>
        /// <remarks>
        /// All the entitities in the root collection and all it's related objects will be untracked at the
        /// end of this operation.
        /// </remarks>
        public void Detach()
        {
            if (!this.IsTracking)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_OperationForTrackedOnly);
            }

            // Operation only allowed on root collections.
            if (!this.rootCollection)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_CannotStopTrackingChildCollection);
            }
            
            this.observer.StopTracking();
            this.observer = null;
            
            this.rootCollection = false;
        }

#if ASTORIA_LIGHT
        public new void Add(T item)
        {
            if (this.IsTracking)
            {
                INotifyPropertyChanged notify = item as INotifyPropertyChanged;
                if (notify == null)
                {
                    throw new InvalidOperationException(Strings.DataBinding_NotifyPropertyChangedNotImpl(item.GetType()));
                }
            }
            base.Add(item);
        }
#endif

        /// <summary>
        /// Override to prevent additions to the collection in "deferred tracking" mode
        /// </summary>
        /// <param name="index">Index of element to add</param>
        /// <param name="item">Element to add</param>
        protected override void InsertItem(int index, T item)
        {
            if (this.trackingOnLoad)
            {
                throw new InvalidOperationException(Strings.DataServiceCollection_InsertIntoTrackedButNotLoadedCollection);
            }

            base.InsertItem(index, item);
        }

        /// <summary>
        /// Verifies that input iterator parameter is not null and in case
        /// of Silverlight, it is not of DataServiceQuery type.
        /// </summary>
        /// <param name="items">Input iterator parameter.</param>
        private static void ValidateIteratorParameter(IEnumerable<T> items)
        {
            Util.CheckArgumentNull(items, "items");
#if ASTORIA_LIGHT
            DataServiceQuery<T> dsq = items as DataServiceQuery<T>;
            if (dsq != null)
            {
                throw new ArgumentException(Strings.DataServiceCollection_DataServiceQueryCanNotBeEnumerated);
            }
#endif
        }

        /// <summary>
        /// Obtain the DataServiceContext from the incoming enumerable
        /// </summary>
        /// <param name="items">An IEnumerable that may be a DataServiceQuery or QueryOperationResponse object</param>
        /// <returns>DataServiceContext instance associated with the input</returns>
        private static DataServiceContext GetContextFromItems(IEnumerable<T> items)
        {
            Debug.Assert(items != null, "items != null");

            DataServiceQuery<T> dataServiceQuery = items as DataServiceQuery<T>;
            if (dataServiceQuery != null)
            {
                DataServiceQueryProvider queryProvider = dataServiceQuery.Provider as DataServiceQueryProvider;
                Debug.Assert(queryProvider != null, "Got DataServiceQuery with unknown query provider.");
                DataServiceContext context = queryProvider.Context;
                Debug.Assert(context != null, "Query provider must always have valid context.");
                return context;
            }

            QueryOperationResponse queryOperationResponse = items as QueryOperationResponse;
            if (queryOperationResponse != null)
            {
                Debug.Assert(queryOperationResponse.Results != null, "Got QueryOperationResponse without valid results.");
                DataServiceContext context = queryOperationResponse.Results.Context;
                Debug.Assert(context != null, "Materializer must always have valid context.");
                return context;
            }

            throw new ArgumentException(Strings.DataServiceCollection_CannotDetermineContextFromItems);
        }

        /// <summary>
        /// Populate this collection with another collection of items
        /// </summary>
        /// <param name="items">The items to populate this collection with</param>
        private void InternalLoadCollection(IEnumerable<T> items)
        {
            Debug.Assert(items != null, "items != null");
#if !ASTORIA_LIGHT
            // For SDP, we must execute the Query implicitly
            DataServiceQuery<T> query = items as DataServiceQuery<T>;
            if (query != null)
            {
                items = query.Execute() as QueryOperationResponse<T>;
            }
#else
            Debug.Assert(!(items is DataServiceQuery), "SL Client using DSQ as items...should have been caught by ValidateIteratorParameter.");
#endif

            foreach (T item in items)
            {
                // if this is too slow, consider hashing the set
                // or just use LoadProperties                    
                if (!this.Contains(item))
                {
                    this.Add(item);
                }
            }

            QueryOperationResponse<T> response = items as QueryOperationResponse<T>;
            if (response != null)
            {
                // this should never be throwing (since we've enumerated already)!
                // Note: Inner collection's nextPartLinkUri is set by the materializer
                this.continuation = response.GetContinuation();
            }
            else
            {
                this.continuation = null;
            }
        }

        /// <summary>
        /// Prepare the collection for loading. For tracked collections, we enter the attaching state
        /// </summary>
        private void StartLoading()
        {
            if (this.IsTracking)
            {
                // Observer must be present on the target collection which implies that the operation would fail on default constructed objects.
                if (this.observer.Context == null)
                {
                    throw new InvalidOperationException(Strings.DataServiceCollection_LoadRequiresTargetCollectionObserved);
                }

                this.observer.AttachBehavior = true;
            }
        }

        /// <summary>
        /// Reset the collection after loading. For tracked collections, we exit the attaching state.
        /// </summary>
        private void FinishLoading()
        {
            if (this.IsTracking)
            {
                this.observer.AttachBehavior = false;
            }
        }

        /// <summary>Initialize and start tracking an DataServiceCollection</summary>
        /// <param name="context">The context</param>
        /// <param name="items">Collection to initialize with</param>
        /// <param name="entitySet">The entity set of the elements in the collection.</param>
        /// <param name="entityChanged">delegate that needs to be called when an entity changes.</param>
        /// <param name="collectionChanged">delegate that needs to be called when an entity collection is changed.</param>
        private void StartTracking(
            DataServiceContext context,
            IEnumerable<T> items,
            String entitySet,
            Func<EntityChangedParams, bool> entityChanged,
            Func<EntityCollectionChangedParams, bool> collectionChanged)
        {
            Debug.Assert(context != null, "Must have a valid context to initialize.");
            Debug.Assert(this.observer == null, "Must have no observer which implies Initialize should only be called once.");

            // Add everything from the input collection.
            if (items != null)
            {
                this.InternalLoadCollection(items);
            }

            this.observer = new BindingObserver(context, entityChanged, collectionChanged);

            this.observer.StartTracking(this, entitySet);

            this.rootCollection = true;
        }

#if ASTORIA_LIGHT
        /// <summary>Helper method to start a LoadAsync operation.</summary>
        /// <param name="beginCall">Function which calls the Begin method for the load. It should take <see cref="AsyncCallback"/>
        /// parameter which should be used as the callback for the Begin call. It should return <see cref="IAsyncResult"/>
        /// of the started asynchronous operation (or throw).</param>
        /// <param name="endCall">Function which calls the End method for the load. It should take <see cref="IAsyncResult"/>
        /// which represents the asynchronous operation in flight. It should return <see cref="QueryOperationResponse"/>
        /// with the result of the operation (or throw).</param>
        /// <remarks>The method takes care of error handling as well as maintaining the <see cref="asyncOperationInProgress"/>.
        /// Note that it does not check the <see cref="asyncOperationInProgress"/> to disallow multiple operations in flight.
        /// The method makes sure that the <paramref name="endCall"/> will be called from the UI thread. It makes no assumptions
        /// about the calling thread of this method.
        /// The method does not process the results of the <paramref name="endCall"/>, it just raises the <see cref="LoadCompleted"/>
        /// event as appropriate. If there's some processing to be done for the results it should all be done by the
        /// <paramref name="endCall"/> method before it returns.</remarks>
        private void BeginLoadAsyncOperation(
            Func<AsyncCallback, IAsyncResult> beginCall,
            Func<IAsyncResult, QueryOperationResponse> endCall)
        {
            Debug.Assert(!this.asyncOperationInProgress, "Trying to start a new LoadAsync while another is still in progress. We should have thrown.");

            // NOTE: this is Silverlight-only, use BackgroundWorker instead of Deployment.Current.Dispatcher
            // to do this in Microsoft/WCF once we decide to add it there as well.
            // Note that we must mark the operation as in progress before we actually call Begin
            //   as the async operation might end immediately inside the Begin call and we have no control
            //   over the ordering between the End callback thread, the thread Begin is called from 
            //   and the UI thread on which we process the end event.
            this.asyncOperationInProgress = true;
            try
            {
                IAsyncResult asyncResult = beginCall(
                    ar => System.Windows.Deployment.Current.Dispatcher.BeginInvoke(() =>
                    {
                        try
                        {
                            QueryOperationResponse result = endCall(ar);
                            this.asyncOperationInProgress = false;
                            if (this.LoadCompleted != null)
                            {
                                this.LoadCompleted(this, new LoadCompletedEventArgs(result, null));
                            }
                        }
                        catch (Exception ex)
                        {
                            this.asyncOperationInProgress = false;
                            if (this.LoadCompleted != null)
                            {
                                this.LoadCompleted(this, new LoadCompletedEventArgs(null, ex));
                            }
                        }
                    }));
            }
            catch (Exception)
            {
                this.asyncOperationInProgress = false;
                throw;
            }
        }
#endif
    }
}
