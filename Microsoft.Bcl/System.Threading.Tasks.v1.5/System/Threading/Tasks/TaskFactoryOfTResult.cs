// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//
// FutureFactory.cs
//
// <OWNER>joehoag</OWNER>
//
// As with TaskFactory, TaskFactory<TResult> encodes common factory patterns into helper methods.
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

using System;
using System.Security;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Diagnostics.Contracts;

namespace System.Threading.Tasks
{
    /// <summary>
    /// Provides support for creating and scheduling
    /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task{TResult}</see> objects.
    /// </summary>
    /// <typeparam name="TResult">The type of the results that are available though 
    /// the <see cref="T:System.Threading.Tasks.Task{TResult}">Task{TResult}</see> objects that are associated with 
    /// the methods in this class.</typeparam>
    /// <remarks>
    /// <para>
    /// There are many common patterns for which tasks are relevant. The <see cref="TaskFactory{TResult}"/>
    /// class encodes some of these patterns into methods that pick up default settings, which are
    /// configurable through its constructors.
    /// </para>
    /// <para>
    /// A default instance of <see cref="TaskFactory{TResult}"/> is available through the
    /// <see cref="System.Threading.Tasks.Task{TResult}.Factory">Task{TResult}.Factory</see> property.
    /// </para>
    /// </remarks>
    
    public class TaskFactory<TResult>
    {
        // Member variables, DefaultScheduler, other properties and ctors 
        // copied right out of TaskFactory...  Lots of duplication here...
        // Should we be thinking about a TaskFactoryBase class?

        // member variables
        private CancellationToken m_defaultCancellationToken;
        private TaskScheduler m_defaultScheduler;
        private TaskCreationOptions m_defaultCreationOptions;
        private TaskContinuationOptions m_defaultContinuationOptions;

        private TaskScheduler DefaultScheduler
        {
            get
            {
                if (m_defaultScheduler == null) return TaskScheduler.Current;
                else return m_defaultScheduler;
            }
        }

        // sister method to above property -- avoids a TLS lookup
        private TaskScheduler GetDefaultScheduler(Task currTask)
        {
            if (m_defaultScheduler != null) return m_defaultScheduler;
            else if (currTask != null) return currTask.ExecutingTaskScheduler;
            else return TaskScheduler.Default;
        }

        /* Constructors */

        /// <summary>
        /// Initializes a <see cref="TaskFactory{TResult}"/> instance with the default configuration.
        /// </summary>
        /// <remarks>
        /// This constructor creates a <see cref="TaskFactory{TResult}"/> instance with a default configuration. The
        /// <see cref="TaskCreationOptions"/> property is initialized to
        /// <see cref="System.Threading.Tasks.TaskCreationOptions.None">TaskCreationOptions.None</see>, the
        /// <see cref="TaskContinuationOptions"/> property is initialized to <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.None">TaskContinuationOptions.None</see>,
        /// and the <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> property is
        /// initialized to the current scheduler (see <see
        /// cref="System.Threading.Tasks.TaskScheduler.Current">TaskScheduler.Current</see>).
        /// </remarks>
        public TaskFactory()
            : this(CancellationToken.None, TaskCreationOptions.None, TaskContinuationOptions.None, null)
        {
        }

        /// <summary>
        /// Initializes a <see cref="TaskFactory{TResult}"/> instance with the default configuration.
        /// </summary>
        /// <param name="cancellationToken">The default <see cref="CancellationToken"/> that will be assigned 
        /// to tasks created by this <see cref="TaskFactory"/> unless another CancellationToken is explicitly specified 
        /// while calling the factory methods.</param>
        /// <remarks>
        /// This constructor creates a <see cref="TaskFactory{TResult}"/> instance with a default configuration. The
        /// <see cref="TaskCreationOptions"/> property is initialized to
        /// <see cref="System.Threading.Tasks.TaskCreationOptions.None">TaskCreationOptions.None</see>, the
        /// <see cref="TaskContinuationOptions"/> property is initialized to <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.None">TaskContinuationOptions.None</see>,
        /// and the <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> property is
        /// initialized to the current scheduler (see <see
        /// cref="System.Threading.Tasks.TaskScheduler.Current">TaskScheduler.Current</see>).
        /// </remarks>
        public TaskFactory(CancellationToken cancellationToken)
            : this(cancellationToken, TaskCreationOptions.None, TaskContinuationOptions.None, null)
        {
        }

        /// <summary>
        /// Initializes a <see cref="TaskFactory{TResult}"/> instance with the specified configuration.
        /// </summary>
        /// <param name="scheduler">
        /// The <see cref="System.Threading.Tasks.TaskScheduler">
        /// TaskScheduler</see> to use to schedule any tasks created with this TaskFactory{TResult}. A null value
        /// indicates that the current TaskScheduler should be used.
        /// </param>
        /// <remarks>
        /// With this constructor, the
        /// <see cref="TaskCreationOptions"/> property is initialized to
        /// <see cref="System.Threading.Tasks.TaskCreationOptions.None">TaskCreationOptions.None</see>, the
        /// <see cref="TaskContinuationOptions"/> property is initialized to <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.None">TaskContinuationOptions.None</see>,
        /// and the <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> property is
        /// initialized to <paramref name="scheduler"/>, unless it's null, in which case the property is
        /// initialized to the current scheduler (see <see
        /// cref="System.Threading.Tasks.TaskScheduler.Current">TaskScheduler.Current</see>).
        /// </remarks>
        public TaskFactory(TaskScheduler scheduler) // null means to use TaskScheduler.Current
            : this(CancellationToken.None, TaskCreationOptions.None, TaskContinuationOptions.None, scheduler)
        {
        }

        /// <summary>
        /// Initializes a <see cref="TaskFactory{TResult}"/> instance with the specified configuration.
        /// </summary>
        /// <param name="creationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskCreationOptions">
        /// TaskCreationOptions</see> to use when creating tasks with this TaskFactory{TResult}.
        /// </param>
        /// <param name="continuationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> to use when creating continuation tasks with this TaskFactory{TResult}.
        /// </param>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument or the <paramref name="continuationOptions"/>
        /// argument specifies an invalid value.
        /// </exception>
        /// <remarks>
        /// With this constructor, the
        /// <see cref="TaskCreationOptions"/> property is initialized to <paramref name="creationOptions"/>,
        /// the
        /// <see cref="TaskContinuationOptions"/> property is initialized to <paramref
        /// name="continuationOptions"/>, and the <see
        /// cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> property is initialized to the
        /// current scheduler (see <see
        /// cref="System.Threading.Tasks.TaskScheduler.Current">TaskScheduler.Current</see>).
        /// </remarks>
        public TaskFactory(TaskCreationOptions creationOptions, TaskContinuationOptions continuationOptions)
            : this(CancellationToken.None, creationOptions, continuationOptions, null)
        {
        }

        /// <summary>
        /// Initializes a <see cref="TaskFactory{TResult}"/> instance with the specified configuration.
        /// </summary>
        /// <param name="cancellationToken">The default <see cref="CancellationToken"/> that will be assigned 
        /// to tasks created by this <see cref="TaskFactory"/> unless another CancellationToken is explicitly specified 
        /// while calling the factory methods.</param>
        /// <param name="creationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskCreationOptions">
        /// TaskCreationOptions</see> to use when creating tasks with this TaskFactory{TResult}.
        /// </param>
        /// <param name="continuationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> to use when creating continuation tasks with this TaskFactory{TResult}.
        /// </param>
        /// <param name="scheduler">
        /// The default <see cref="System.Threading.Tasks.TaskScheduler">
        /// TaskScheduler</see> to use to schedule any Tasks created with this TaskFactory{TResult}. A null value
        /// indicates that TaskScheduler.Current should be used.
        /// </param>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument or the <paramref name="continuationOptions"/>
        /// argumentspecifies an invalid value.
        /// </exception>
        /// <remarks>
        /// With this constructor, the
        /// <see cref="TaskCreationOptions"/> property is initialized to <paramref name="creationOptions"/>,
        /// the
        /// <see cref="TaskContinuationOptions"/> property is initialized to <paramref
        /// name="continuationOptions"/>, and the <see
        /// cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> property is initialized to
        /// <paramref name="scheduler"/>, unless it's null, in which case the property is initialized to the
        /// current scheduler (see <see
        /// cref="System.Threading.Tasks.TaskScheduler.Current">TaskScheduler.Current</see>).
        /// </remarks>
        public TaskFactory(CancellationToken cancellationToken, TaskCreationOptions creationOptions, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            m_defaultCancellationToken = cancellationToken;
            m_defaultScheduler = scheduler;
            m_defaultCreationOptions = creationOptions;
            m_defaultContinuationOptions = continuationOptions;
            TaskFactory.CheckCreationOptions(m_defaultCreationOptions);
            TaskFactory.CheckMultiTaskContinuationOptions(m_defaultContinuationOptions);

        }

        /* Properties */

        /// <summary>
        /// Gets the default <see cref="System.Threading.CancellationToken">CancellationToken</see> of this
        /// TaskFactory.
        /// </summary>
        /// <remarks>
        /// This property returns the default <see cref="CancellationToken"/> that will be assigned to all 
        /// tasks created by this factory unless another CancellationToken value is explicitly specified 
        /// during the call to the factory methods.
        /// </remarks>
        public CancellationToken CancellationToken { get { return m_defaultCancellationToken; } }

        /// <summary>
        /// Gets the <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> of this
        /// TaskFactory{TResult}.
        /// </summary>
        /// <remarks>
        /// This property returns the default scheduler for this factory.  It will be used to schedule all 
        /// tasks unless another scheduler is explicitly specified during calls to this factory's methods.  
        /// If null, <see cref="System.Threading.Tasks.TaskScheduler.Current">TaskScheduler.Current</see> 
        /// will be used.
        /// </remarks>
        public TaskScheduler Scheduler { get { return m_defaultScheduler; } }

        /// <summary>
        /// Gets the <see cref="System.Threading.Tasks.TaskCreationOptions">TaskCreationOptions
        /// </see> value of this TaskFactory{TResult}.
        /// </summary>
        /// <remarks>
        /// This property returns the default creation options for this factory.  They will be used to create all 
        /// tasks unless other options are explicitly specified during calls to this factory's methods.
        /// </remarks>
        public TaskCreationOptions CreationOptions { get { return m_defaultCreationOptions; } }

        /// <summary>
        /// Gets the <see cref="System.Threading.Tasks.TaskCreationOptions">TaskContinuationOptions
        /// </see> value of this TaskFactory{TResult}.
        /// </summary>
        /// <remarks>
        /// This property returns the default continuation options for this factory.  They will be used to create 
        /// all continuation tasks unless other options are explicitly specified during calls to this factory's methods.
        /// </remarks>
        public TaskContinuationOptions ContinuationOptions { get { return m_defaultContinuationOptions; } }


        /* StartNew */

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<TResult> function)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, m_defaultCancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<TResult> function, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, cancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<TResult> function, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, m_defaultCancellationToken,
                creationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <param name="scheduler">The <see
        /// cref="T:System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created <see cref="T:System.Threading.Tasks.Task{TResult}">
        /// Task{TResult}</see>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="scheduler"/>
        /// argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<TResult> function, CancellationToken cancellationToken, TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task<TResult>.StartNew(Task.InternalCurrent, function, cancellationToken,
                creationOptions, InternalTaskOptions.None, scheduler, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="function"/>
        /// delegate.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<Object, TResult> function, Object state)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, state, m_defaultCancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="function"/>
        /// delegate.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<Object, TResult> function, Object state, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, state, cancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="function"/>
        /// delegate.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<Object, TResult> function, Object state, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, state, m_defaultCancellationToken,
                creationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="function"/>
        /// delegate.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="scheduler">The <see
        /// cref="T:System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created <see cref="T:System.Threading.Tasks.Task{TResult}">
        /// Task{TResult}</see>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="function"/>
        /// argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="scheduler"/>
        /// argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a <see cref="Task{TResult}"/> using one
        /// of its constructors and then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> StartNew(Func<Object, TResult> function, Object state, CancellationToken cancellationToken, TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task<TResult>.StartNew(Task.InternalCurrent, function, state, cancellationToken,
                creationOptions, InternalTaskOptions.None, scheduler, ref stackMark);
        }

        //
        // APM Factory methods
        //

        // Common core logic for FromAsync calls.  This minimizes the chance of "drift" between overload implementations.
        private static void FromAsyncCoreLogic(IAsyncResult iar, Func<IAsyncResult, TResult> endMethod, TaskCompletionSource<TResult> tcs)
        {
            Exception ex = null;
            OperationCanceledException oce = null;
            TResult result = default(TResult);

            try { result = endMethod(iar); }
            catch (OperationCanceledException _oce) { oce = _oce; }
            catch (Exception e) { ex = e; }
            finally
            {
                if (oce != null) tcs.TrySetCanceled();
                else if (ex != null)
                {
                    bool bWonSetException = tcs.TrySetException(ex);
                    if (bWonSetException && ThreadingServices.IsThreadAbort(ex))
                    {
                        tcs.Task.m_contingentProperties.m_exceptionsHolder.MarkAsHandled(false);
                    }
                }
                else tcs.TrySetResult(result);
            }
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that executes an end
        /// method function when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult whose completion should trigger the processing of the
        /// <paramref name="endMethod"/>.</param>
        /// <param name="endMethod">The function delegate that processes the completed <paramref
        /// name="asyncResult"/>.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="asyncResult"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>A <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents the
        /// asynchronous operation.</returns>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> FromAsync(
            IAsyncResult asyncResult,
            Func<IAsyncResult, TResult> endMethod)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return FromAsyncImpl(asyncResult, endMethod, m_defaultCreationOptions, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that executes an end
        /// method function when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult whose completion should trigger the processing of the
        /// <paramref name="endMethod"/>.</param>
        /// <param name="endMethod">The function delegate that processes the completed <paramref
        /// name="asyncResult"/>.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="asyncResult"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>A <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents the
        /// asynchronous operation.</returns>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> FromAsync(
            IAsyncResult asyncResult,
            Func<IAsyncResult, TResult> endMethod,
            TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return FromAsyncImpl(asyncResult, endMethod, creationOptions, DefaultScheduler, ref stackMark);
        }



        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that executes an end
        /// method function when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult whose completion should trigger the processing of the
        /// <paramref name="endMethod"/>.</param>
        /// <param name="endMethod">The function delegate that processes the completed <paramref
        /// name="asyncResult"/>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the task that executes the end method.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="asyncResult"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="scheduler"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>A <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents the
        /// asynchronous operation.</returns>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> FromAsync(
            IAsyncResult asyncResult,
            Func<IAsyncResult, TResult> endMethod,
            TaskCreationOptions creationOptions,
            TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return FromAsyncImpl(asyncResult, endMethod, creationOptions, scheduler, ref stackMark);
        }

        // internal overload that supports StackCrawlMark
        // We also need this logic broken out into a static method so that the similar TaskFactory.FromAsync()
        // method can access the logic w/o declaring a TaskFactory<TResult> instance.
        internal static Task<TResult> FromAsyncImpl(
            IAsyncResult asyncResult,
            Func<IAsyncResult, TResult> endMethod,
            TaskCreationOptions creationOptions,
            TaskScheduler scheduler,
            ref StackCrawlMark stackMark)
        {
            if (asyncResult == null)
                throw new ArgumentNullException("asyncResult");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            if (scheduler == null)
                throw new ArgumentNullException("scheduler");
            TaskFactory.CheckFromAsyncOptions(creationOptions, false);

            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>(creationOptions);

            // Just specify this task as detached. No matter what happens, we want endMethod 
            // to be called -- even if the parent is canceled.  So we don't want to flow 
            // RespectParentCancellation.
            Task t = new Task(delegate 
                {
                    FromAsyncCoreLogic(asyncResult, endMethod, tcs);
                }, 
                (object)null, Task.InternalCurrent,
                CancellationToken.None, TaskCreationOptions.None, InternalTaskOptions.None, null, ref stackMark);

            if (asyncResult.IsCompleted)
            {
                try { t.RunSynchronously(scheduler); }
                catch (Exception e) { tcs.TrySetException(e); } // catch and log any scheduler exceptions
            }
            else
            {
#if !REGISTER_WAIT_FOR_SINGLE_OBJECT
                // NOTE: This is an unfortunate hack to work around a RegisterWaitForSingleObject bug in WinPhone 7.
                ThreadPool.QueueUserWorkItem(state =>
                {
                    var wh = (WaitHandle)state;
                    wh.WaitOne();
                    try { t.RunSynchronously(scheduler); }
                    catch (Exception e) { tcs.TrySetException(e); } // catch and log any scheduler exceptions
                }, asyncResult.AsyncWaitHandle);
#else

                ThreadPool.RegisterWaitForSingleObject(
                    asyncResult.AsyncWaitHandle,
                    delegate 
                    {
                        try { t.RunSynchronously(scheduler); }
                        catch (Exception e) { tcs.TrySetException(e); } // catch and log any scheduler exceptions
                    },
                    null,
                    Timeout.Infinite,
                    true);
#endif
            }

            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, object state)
        {
            return FromAsyncImpl(beginMethod, endMethod, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, object state, TaskCreationOptions creationOptions)
        {
            return FromAsyncImpl(beginMethod, endMethod, state, creationOptions);
        }

        // We need this logic broken out into a static method so that the similar TaskFactory.FromAsync()
        // method can access the logic w/o declaring a TaskFactory<TResult> instance.
        internal static Task<TResult> FromAsyncImpl(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            TaskFactory.CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>(state, creationOptions);

            try
            {
                beginMethod(iar =>
                {
                    FromAsyncCoreLogic(iar, endMethod, tcs);
                }, state);
            }
            catch
            {
                // Make sure we don't leave tcs "dangling".
                tcs.TrySetResult(default(TResult));
                throw;
            }

            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync<TArg1>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, object state)
        {
            return FromAsyncImpl(beginMethod, endMethod, arg1, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync<TArg1>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, object state, TaskCreationOptions creationOptions)
        {
            return FromAsyncImpl(beginMethod, endMethod, arg1, state, creationOptions);
        }

        // We need this logic broken out into a static method so that the similar TaskFactory.FromAsync()
        // method can access the logic w/o declaring a TaskFactory<TResult> instance.
        internal static Task<TResult> FromAsyncImpl<TArg1>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            TaskFactory.CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>(state, creationOptions);

            try
            {
                beginMethod(arg1, iar =>
                {
                    FromAsyncCoreLogic(iar, endMethod, tcs);
                }, state);
            }
            catch
            {
                // Make sure we don't leave tcs "dangling".
                tcs.TrySetResult(default(TResult));
                throw;
            }

            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg2">The second argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync<TArg1, TArg2>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, TArg2 arg2, object state)
        {
            return FromAsyncImpl(beginMethod, endMethod, arg1, arg2, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg2">The second argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync<TArg1, TArg2>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, TArg2 arg2, object state, TaskCreationOptions creationOptions)
        {
            return FromAsyncImpl(beginMethod, endMethod, arg1, arg2, state, creationOptions);
        }

        // We need this logic broken out into a static method so that the similar TaskFactory.FromAsync()
        // method can access the logic w/o declaring a TaskFactory<TResult> instance.
        internal static Task<TResult> FromAsyncImpl<TArg1, TArg2>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, TArg2 arg2, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            TaskFactory.CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>(state, creationOptions);

            try
            {
                beginMethod(arg1, arg2, iar =>
                {
                    FromAsyncCoreLogic(iar, endMethod, tcs);
                }, state);
            }
            catch
            {
                // Make sure we don't leave tcs "dangling".
                tcs.TrySetResult(default(TResult));
                throw;
            }


            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <typeparam name="TArg3">The type of the third argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg2">The second argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg3">The third argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync<TArg1, TArg2, TArg3>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, TArg2 arg2, TArg3 arg3, object state)
        {
            return FromAsyncImpl(beginMethod, endMethod, arg1, arg2, arg3, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <typeparam name="TArg3">The type of the third argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg2">The second argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg3">The third argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that
        /// represents the asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task<TResult> FromAsync<TArg1, TArg2, TArg3>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, TArg2 arg2, TArg3 arg3, object state, TaskCreationOptions creationOptions)
        {
            return FromAsyncImpl(beginMethod, endMethod, arg1, arg2, arg3, state, creationOptions);
        }

        // We need this logic broken out into a static method so that the similar TaskFactory.FromAsync()
        // method can access the logic w/o declaring a TaskFactory<TResult> instance.
        internal static Task<TResult> FromAsyncImpl<TArg1, TArg2, TArg3>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod,
            TArg1 arg1, TArg2 arg2, TArg3 arg3, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            TaskFactory.CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<TResult> tcs = new TaskCompletionSource<TResult>(state, creationOptions);

            try
            {
                beginMethod(arg1, arg2, arg3, iar =>
                {
                    FromAsyncCoreLogic(iar, endMethod, tcs);
                }, state);
            }
            catch
            {
                // Make sure we don't leave tcs "dangling".
                tcs.TrySetResult(default(TResult));
                throw;
            }

            return tcs.Task;
        }

        // Utility method to create a canceled future-style task.
        // Used by ContinueWhenAll/Any to bail out early on a pre-canceled token.
        private static Task<TResult> CreateCanceledTask(TaskContinuationOptions continuationOptions, CancellationToken ct)
        {
            TaskCreationOptions tco;
            InternalTaskOptions dontcare;
            Task.CreationOptionsFromContinuationOptions(continuationOptions, out tco, out dontcare);
            return new Task<TResult>(true, default(TResult), tco, ct);
        }

        //
        // ContinueWhenAll() methods
        //

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> 
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in 
        /// the <paramref name="tasks"/> array have completed.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll(Task[] tasks, Func<Task[], TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> 
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in 
        /// the <paramref name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll(Task[] tasks, Func<Task[], TResult> continuationFunction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in the <paramref
        /// name="tasks"/> array have completed.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAll.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll(Task[] tasks, Func<Task[],TResult> continuationFunction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in the <paramref
        /// name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="scheduler"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAll.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll(Task[] tasks, Func<Task[],TResult> continuationFunction,  
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }


        // Private method to support StackCrawlMark.
        internal static Task<TResult> ContinueWhenAll(Task[] tasks, Func<Task[],TResult> continuationFunction,
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // check arguments
            TaskFactory.CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationFunction == null) throw new ArgumentNullException("continuationFunction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");
            
            // Check tasks array and make defensive copy
            Task[] tasksCopy = TaskFactory.CheckMultiContinuationTasksAndCopy(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Perform common ContinueWhenAll() setup logic, extract starter task
            Task<bool> starter = TaskFactory.CommonCWAllLogic(tasksCopy);

            // returned continuation task, off of starter
            Task<TResult> rval = starter.ContinueWith(finishedTask => { return continuationFunction(tasksCopy); }, scheduler,
                cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in the
        /// <paramref name="tasks"/> array have completed.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll<TAntecedentResult>(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in the
        /// <paramref name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction, 
            CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll<TAntecedentResult>(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in the
        /// <paramref name="tasks"/> array have completed.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAll.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction,
            TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll<TAntecedentResult>(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationFunction">The function delegate to execute when all tasks in the
        /// <paramref name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="scheduler"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAll.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction,
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll<TAntecedentResult>(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // internal (instead of private) so that it can be called from the similar TaskFactory.ContinueWhenAll() method.
        internal static Task<TResult> ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction,
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // check arguments
            TaskFactory.CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationFunction == null) throw new ArgumentNullException("continuationFunction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");

            // Check tasks array and make defensive copy
            Task<TAntecedentResult>[] tasksCopy = TaskFactory.CheckMultiContinuationTasksAndCopy<TAntecedentResult>(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Call common ContinueWhenAll() setup logic, extract starter task.
            Task<bool> starter = TaskFactory.CommonCWAllLogic(tasksCopy);

            // returned continuation task, off of starter
            Task<TResult> rval = starter.ContinueWith(finishedTask => { return continuationFunction(tasksCopy); }, scheduler,
                cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }


        //
        // ContinueWhenAny() methods
        //

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny(Task[] tasks, Func<Task, TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny(Task[] tasks, Func<Task, TResult> continuationFunction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAny.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny(Task[] tasks, Func<Task, TResult> continuationFunction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="scheduler"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAny.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny(Task[] tasks, Func<Task, TResult> continuationFunction,  
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // internal version that supports StackCrawlMark
        internal static Task<TResult> ContinueWhenAny(Task[] tasks, Func<Task, TResult> continuationFunction,
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // check arguments
            TaskFactory.CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationFunction == null) throw new ArgumentNullException("continuationFunction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");
            
            // Check tasks array and make defensive copy
            Task[] tasksCopy = TaskFactory.CheckMultiContinuationTasksAndCopy(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Call common ContinueWhenAny() setup logic, extract starter
            Task<Task> starter = TaskFactory.CommonCWAnyLogic(tasksCopy);

            // returned continuation task, off of starter
            Task<TResult> rval = starter.ContinueWith(completedTask =>
            {
                return continuationFunction(completedTask.Result);
            }, scheduler, cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction, 
            CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAny.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction,
            TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationFunction">The function delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationFunction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="scheduler"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="continuationOptions"/> argument specifies an invalid TaskContinuationOptions
        /// value.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// The NotOn* and OnlyOn* <see cref="System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>, 
        /// which constrain for which <see cref="System.Threading.Tasks.TaskStatus">TaskStatus</see> states a continuation 
        /// will be executed, are illegal with ContinueWhenAny.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction,
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // internal version to support StackCrawlMark
        internal static Task<TResult> ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction,
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // check arguments
            TaskFactory.CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationFunction == null) throw new ArgumentNullException("continuationFunction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");

            // Check tasks array and make defensive copy
            Task<TAntecedentResult>[] tasksCopy = TaskFactory.CheckMultiContinuationTasksAndCopy<TAntecedentResult>(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Call common ContinueWhenAny setup logic, extract starter
            Task<Task> starter = TaskFactory.CommonCWAnyLogic(tasksCopy);

            // returned continuation task, off of starter
            Task<TResult> rval = starter.ContinueWith(completedTask => 
            {
                Task<TAntecedentResult> winner = completedTask.Result as Task<TAntecedentResult>;
                return continuationFunction(winner); 
            }, scheduler, cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }

    
    }

}
