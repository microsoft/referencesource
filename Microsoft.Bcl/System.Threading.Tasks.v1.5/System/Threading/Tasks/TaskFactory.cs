// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//
// TaskFactory.cs
//
// <OWNER>joehoag</OWNER>
//
// There are a plethora of common patterns for which Tasks are created.  TaskFactory encodes 
// these patterns into helper methods.  These helpers also pick up default configuration settings 
// applicable to the entire factory and configurable through its constructors.
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
    /// <see cref="T:System.Threading.Tasks.Task">Tasks</see>.
    /// </summary>
    /// <remarks>
    /// <para>
    /// There are many common patterns for which tasks are relevant. The <see cref="TaskFactory"/>
    /// class encodes some of these patterns into methods that pick up default settings, which are
    /// configurable through its constructors.
    /// </para>
    /// <para>
    /// A default instance of <see cref="TaskFactory"/> is available through the
    /// <see cref="System.Threading.Tasks.Task.Factory">Task.Factory</see> property.
    /// </para>
    /// </remarks>
    
    public class TaskFactory
    {
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

        // ctor parameters provide defaults for the factory, which can be overridden by options provided to
        // specific calls on the factory


        /// <summary>
        /// Initializes a <see cref="TaskFactory"/> instance with the default configuration.
        /// </summary>
        /// <remarks>
        /// This constructor creates a <see cref="TaskFactory"/> instance with a default configuration. The
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
        /// Initializes a <see cref="TaskFactory"/> instance with the specified configuration.
        /// </summary>
        /// <param name="cancellationToken">The default <see cref="CancellationToken"/> that will be assigned 
        /// to tasks created by this <see cref="TaskFactory"/> unless another CancellationToken is explicitly specified 
        /// while calling the factory methods.</param>
        /// <remarks>
        /// This constructor creates a <see cref="TaskFactory"/> instance with a default configuration. The
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
        /// Initializes a <see cref="TaskFactory"/> instance with the specified configuration.
        /// </summary>
        /// <param name="scheduler">
        /// The <see cref="System.Threading.Tasks.TaskScheduler">
        /// TaskScheduler</see> to use to schedule any tasks created with this TaskFactory. A null value
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
        /// Initializes a <see cref="TaskFactory"/> instance with the specified configuration.
        /// </summary>
        /// <param name="creationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskCreationOptions">
        /// TaskCreationOptions</see> to use when creating tasks with this TaskFactory.
        /// </param>
        /// <param name="continuationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> to use when creating continuation tasks with this TaskFactory.
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
        /// Initializes a <see cref="TaskFactory"/> instance with the specified configuration.
        /// </summary>
        /// <param name="cancellationToken">The default <see cref="CancellationToken"/> that will be assigned 
        /// to tasks created by this <see cref="TaskFactory"/> unless another CancellationToken is explicitly specified 
        /// while calling the factory methods.</param>
        /// <param name="creationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskCreationOptions">
        /// TaskCreationOptions</see> to use when creating tasks with this TaskFactory.
        /// </param>
        /// <param name="continuationOptions">
        /// The default <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> to use when creating continuation tasks with this TaskFactory.
        /// </param>
        /// <param name="scheduler">
        /// The default <see cref="System.Threading.Tasks.TaskScheduler">
        /// TaskScheduler</see> to use to schedule any Tasks created with this TaskFactory. A null value
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
            CheckCreationOptions(m_defaultCreationOptions);
            CheckMultiTaskContinuationOptions(m_defaultContinuationOptions);
        }

        internal static void CheckCreationOptions(TaskCreationOptions creationOptions)
        {
            // Check for validity of options
            if ((creationOptions &
                    ~(TaskCreationOptions.AttachedToParent |
                      TaskCreationOptions.LongRunning |
                      TaskCreationOptions.PreferFairness)) != 0)
            {
                throw new ArgumentOutOfRangeException("creationOptions");
            }
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
        /// TaskFactory.
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
        /// </see> value of this TaskFactory.
        /// </summary>
        /// <remarks>
        /// This property returns the default creation options for this factory.  They will be used to create all 
        /// tasks unless other options are explicitly specified during calls to this factory's methods.
        /// </remarks>
        public TaskCreationOptions CreationOptions { get { return m_defaultCreationOptions; } }

        /// <summary>
        /// Gets the <see cref="System.Threading.Tasks.TaskCreationOptions">TaskContinuationOptions
        /// </see> value of this TaskFactory.
        /// </summary>
        /// <remarks>
        /// This property returns the default continuation options for this factory.  They will be used to create 
        /// all continuation tasks unless other options are explicitly specified during calls to this factory's methods.
        /// </remarks>
        public TaskContinuationOptions ContinuationOptions { get { return m_defaultContinuationOptions; } }

        //
        // StartNew methods
        //

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref name="action"/> 
        /// argument is null.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors 
        /// and then calling 
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.  However,
        /// unless creation and scheduling must be separated, StartNew is the recommended
        /// approach for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action action)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task.InternalStartNew(currTask, action, null, m_defaultCancellationToken, GetDefaultScheduler(currTask), 
                m_defaultCreationOptions, InternalTaskOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref name="action"/> 
        /// argument is null.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors 
        /// and then calling 
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.  However,
        /// unless creation and scheduling must be separated, StartNew is the recommended
        /// approach for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action action, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task.InternalStartNew(currTask, action, null, cancellationToken, GetDefaultScheduler(currTask),
                m_defaultCreationOptions, InternalTaskOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task">Task.</see></param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="action"/>
        /// argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors and
        /// then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action action, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task.InternalStartNew(currTask, action, null, m_defaultCancellationToken, GetDefaultScheduler(currTask), creationOptions, 
                InternalTaskOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new <see cref="Task"/></param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task">Task.</see></param>
        /// <param name="scheduler">The <see
        /// cref="T:System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="action"/>
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
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors and
        /// then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action action, CancellationToken cancellationToken, TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task.InternalStartNew(Task.InternalCurrent, action, null, cancellationToken, scheduler, creationOptions, 
                InternalTaskOptions.None, ref stackMark);
        }

        // Internal version includes InternalTaskOptions for Parallel.Invoke() support.
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        internal Task StartNew(Action action, CancellationToken cancellationToken, TaskCreationOptions creationOptions, InternalTaskOptions internalOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task.InternalStartNew(Task.InternalCurrent, action, null, cancellationToken, scheduler, creationOptions, internalOptions, ref stackMark);
        }


        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="action"/>
        /// delegate.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="action"/>
        /// argument is null.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors and
        /// then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action<Object> action, Object state)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task.InternalStartNew(currTask, action, state, m_defaultCancellationToken, GetDefaultScheduler(currTask), 
                m_defaultCreationOptions, InternalTaskOptions.None, ref stackMark);
        }


        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="action"/>
        /// delegate.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new <see cref="Task"/></param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="action"/>
        /// argument is null.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors and
        /// then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action<Object> action, Object state, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task.InternalStartNew(currTask, action, state, cancellationToken, GetDefaultScheduler(currTask),
                m_defaultCreationOptions, InternalTaskOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="action"/>
        /// delegate.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task">Task.</see></param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="action"/>
        /// argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <remarks>
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors and
        /// then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action<Object> action, Object state, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task.InternalStartNew(currTask, action, state, m_defaultCancellationToken, GetDefaultScheduler(currTask), 
                creationOptions, InternalTaskOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task">Task</see>.
        /// </summary>
        /// <param name="action">The action delegate to execute asynchronously.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="action"/>
        /// delegate.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <param name="creationOptions">A TaskCreationOptions value that controls the behavior of the
        /// created
        /// <see cref="T:System.Threading.Tasks.Task">Task.</see></param>
        /// <param name="scheduler">The <see
        /// cref="T:System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The started <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the <paramref
        /// name="action"/>
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
        /// Calling StartNew is functionally equivalent to creating a Task using one of its constructors and
        /// then calling
        /// <see cref="System.Threading.Tasks.Task.Start()">Start</see> to schedule it for execution.
        /// However, unless creation and scheduling must be separated, StartNew is the recommended approach
        /// for both simplicity and performance.
        /// </remarks>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task StartNew(Action<Object> action, Object state, CancellationToken cancellationToken,
                            TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task.InternalStartNew(Task.InternalCurrent, action, state, cancellationToken, scheduler, 
                creationOptions, InternalTaskOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> StartNew<TResult>(Func<TResult> function)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, m_defaultCancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }


        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new <see cref="Task"/></param>
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
        public Task<TResult> StartNew<TResult>(Func<TResult> function, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, cancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> StartNew<TResult>(Func<TResult> function, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, m_defaultCancellationToken,
                creationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
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
        public Task<TResult> StartNew<TResult>(Func<TResult> function, CancellationToken cancellationToken, TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task<TResult>.StartNew(Task.InternalCurrent, function, cancellationToken,
                creationOptions, InternalTaskOptions.None, scheduler, ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> StartNew<TResult>(Func<Object, TResult> function, Object state)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, state, m_defaultCancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }


        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
        /// <param name="function">A function delegate that returns the future result to be available through
        /// the <see cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="function"/>
        /// delegate.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new <see cref="Task"/></param>
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
        public Task<TResult> StartNew<TResult>(Func<Object, TResult> function, Object state, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, state, cancellationToken,
                m_defaultCreationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> StartNew<TResult>(Func<Object, TResult> function, Object state, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Task currTask = Task.InternalCurrent;
            return Task<TResult>.StartNew(currTask, function, state, m_defaultCancellationToken,
                creationOptions, InternalTaskOptions.None, GetDefaultScheduler(currTask), ref stackMark);
        }

        /// <summary>
        /// Creates and starts a <see cref="T:System.Threading.Tasks.Task{TResult}"/>.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> StartNew<TResult>(Func<Object, TResult> function, Object state, CancellationToken cancellationToken,
            TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return Task<TResult>.StartNew(Task.InternalCurrent, function, state, cancellationToken,
                creationOptions, InternalTaskOptions.None, scheduler, ref stackMark);
        }

        //
        // FromAsync methods
        //

        // Common core logic for FromAsync calls.  This minimizes the chance of "drift" between overload implementations.
        private static void FromAsyncCoreLogic(IAsyncResult iar, Action<IAsyncResult> endMethod, TaskCompletionSource<object> tcs)
        {
            Exception ex = null;
            OperationCanceledException oce = null;

            try { endMethod(iar); }
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
                else tcs.TrySetResult(null);
            }
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that executes an end method action
        /// when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult whose completion should trigger the processing of the
        /// <paramref name="endMethod"/>.</param>
        /// <param name="endMethod">The action delegate that processes the completed <paramref
        /// name="asyncResult"/>.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="asyncResult"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>A <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the asynchronous
        /// operation.</returns>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task FromAsync(
            IAsyncResult asyncResult,
            Action<IAsyncResult> endMethod)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return FromAsync(asyncResult, endMethod, m_defaultCreationOptions, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that executes an end method action
        /// when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult whose completion should trigger the processing of the
        /// <paramref name="endMethod"/>.</param>
        /// <param name="endMethod">The action delegate that processes the completed <paramref
        /// name="asyncResult"/>.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="asyncResult"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>A <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the asynchronous
        /// operation.</returns>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task FromAsync(
            IAsyncResult asyncResult,
            Action<IAsyncResult> endMethod,
            TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return FromAsync(asyncResult, endMethod, creationOptions, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that executes an end method action
        /// when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <param name="asyncResult">The IAsyncResult whose completion should trigger the processing of the
        /// <paramref name="endMethod"/>.</param>
        /// <param name="endMethod">The action delegate that processes the completed <paramref
        /// name="asyncResult"/>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the task that executes the end method.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="asyncResult"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="scheduler"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>A <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the asynchronous
        /// operation.</returns>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task FromAsync(
            IAsyncResult asyncResult,
            Action<IAsyncResult> endMethod,
            TaskCreationOptions creationOptions,
            TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return FromAsync(asyncResult, endMethod, creationOptions, scheduler, ref stackMark);
        }

        // private version that supports StackCrawlMark.
        private Task FromAsync(
            IAsyncResult asyncResult,
            Action<IAsyncResult> endMethod,
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
            CheckFromAsyncOptions(creationOptions, false);

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>(null, creationOptions);

            // Just specify this task as detached. No matter what happens, we want endMethod 
            // to be called -- even if the parent is canceled.
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
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            object state)
        {
            return FromAsync(beginMethod, endMethod, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>(state, creationOptions);

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
                tcs.TrySetResult(null);
                throw;
            }

            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/>
        /// delegate.</typeparam>
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
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync<TArg1>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            TArg1 arg1,
            object state)
        {
            return FromAsync(beginMethod, endMethod, arg1, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/>
        /// delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync<TArg1>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            TArg1 arg1, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>(state, creationOptions);

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
                tcs.TrySetResult(null);
                throw;
            }

            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/>
        /// delegate.</typeparam>
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
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync<TArg1, TArg2>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            TArg1 arg1, TArg2 arg2, object state)
        {
            return FromAsync(beginMethod, endMethod, arg1, arg2, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/>
        /// delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <param name="beginMethod">The delegate that begins the asynchronous operation.</param>
        /// <param name="endMethod">The delegate that ends the asynchronous operation.</param>
        /// <param name="arg1">The first argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="arg2">The second argument passed to the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <param name="creationOptions">The TaskCreationOptions value that controls the behavior of the
        /// created <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync<TArg1, TArg2>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            TArg1 arg1, TArg2 arg2, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            CheckFromAsyncOptions(creationOptions, true);

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>(state, creationOptions);

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
                tcs.TrySetResult(null);
                throw;
            }

            return tcs.Task;
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/>
        /// delegate.</typeparam>
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
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync<TArg1, TArg2, TArg3>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            TArg1 arg1, TArg2 arg2, TArg3 arg3, object state)
        {
            return FromAsync(beginMethod, endMethod, arg1, arg2, arg3, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task">Task</see> that represents a pair of begin
        /// and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/>
        /// delegate.</typeparam>
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
        /// created <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="state">An object containing data to be used by the <paramref name="beginMethod"/>
        /// delegate.</param>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="beginMethod"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="endMethod"/> argument is null.</exception>
        /// <exception cref="T:ArgumentOutOfRangeException">The exception that is thrown when the
        /// <paramref name="creationOptions"/> argument specifies an invalid TaskCreationOptions
        /// value.</exception>
        /// <returns>The created <see cref="T:System.Threading.Tasks.Task">Task</see> that represents the
        /// asynchronous operation.</returns>
        /// <remarks>
        /// This method throws any exceptions thrown by the <paramref name="beginMethod"/>.
        /// </remarks>
        public Task FromAsync<TArg1, TArg2, TArg3>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Action<IAsyncResult> endMethod,
            TArg1 arg1, TArg2 arg2, TArg3 arg3, object state, TaskCreationOptions creationOptions)
        {
            if (beginMethod == null)
                throw new ArgumentNullException("beginMethod");
            if (endMethod == null)
                throw new ArgumentNullException("endMethod");
            CheckFromAsyncOptions(creationOptions, true);
            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>(state, creationOptions);

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
                tcs.TrySetResult(null);
                throw;
            }


            return tcs.Task;
        }

        //
        // Additional FromAsync() overloads used for inferencing convenience
        //

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that executes an end
        /// method function when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TResult>(
            IAsyncResult asyncResult, Func<IAsyncResult, TResult> endMethod)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return TaskFactory<TResult>.FromAsyncImpl(asyncResult, endMethod, m_defaultCreationOptions, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that executes an end
        /// method function when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TResult>(
            IAsyncResult asyncResult, Func<IAsyncResult, TResult> endMethod, TaskCreationOptions creationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return TaskFactory<TResult>.FromAsyncImpl(asyncResult, endMethod, creationOptions, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that executes an end
        /// method function when a specified <see cref="T:System.IAsyncResult">IAsyncResult</see> completes.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TResult>(
            IAsyncResult asyncResult, Func<IAsyncResult, TResult> endMethod, TaskCreationOptions creationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return TaskFactory<TResult>.FromAsyncImpl(asyncResult, endMethod, creationOptions, scheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TResult>(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, object state)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TResult>(
            Func<AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, object state, TaskCreationOptions creationOptions)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, state, creationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TArg1, TResult>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, TArg1 arg1, object state)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, arg1, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TArg1, TResult>(
            Func<TArg1, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, TArg1 arg1, object state, TaskCreationOptions creationOptions)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, arg1, state, creationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TArg1, TArg2, TResult>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, TArg1 arg1, TArg2 arg2, object state)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, arg1, arg2, state, m_defaultCreationOptions);
        }

        /// <summary>
        /// Creates a <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see> that represents a pair of
        /// begin and end methods that conform to the Asynchronous Programming Model pattern.
        /// </summary>
        /// <typeparam name="TArg1">The type of the first argument passed to the <paramref
        /// name="beginMethod"/> delegate.</typeparam>
        /// <typeparam name="TArg2">The type of the second argument passed to <paramref name="beginMethod"/>
        /// delegate.</typeparam>
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TArg1, TArg2, TResult>(
            Func<TArg1, TArg2, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, TArg1 arg1, TArg2 arg2, object state, TaskCreationOptions creationOptions)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, arg1, arg2, state, creationOptions);
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
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TArg1, TArg2, TArg3, TResult>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, TArg1 arg1, TArg2 arg2, TArg3 arg3, object state)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, arg1, arg2, arg3, state, m_defaultCreationOptions);
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
        /// <typeparam name="TResult">The type of the result available through the
        /// <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>.
        /// </typeparam>
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
        public Task<TResult> FromAsync<TArg1, TArg2, TArg3, TResult>(
            Func<TArg1, TArg2, TArg3, AsyncCallback, object, IAsyncResult> beginMethod,
            Func<IAsyncResult, TResult> endMethod, TArg1 arg1, TArg2 arg2, TArg3 arg3, object state, TaskCreationOptions creationOptions)
        {
            return TaskFactory<TResult>.FromAsyncImpl(beginMethod, endMethod, arg1, arg2, arg3, state, creationOptions);
        }

        /// <summary>
        /// Check validity of options passed to FromAsync method
        /// </summary>
        /// <param name="creationOptions">The options to be validated.</param>
        /// <param name="hasBeginMethod">determines type of FromAsync method that called this method</param>
        internal static void CheckFromAsyncOptions(TaskCreationOptions creationOptions, bool hasBeginMethod)
        {
            if (hasBeginMethod)
            {
                // Options detected here cause exceptions in FromAsync methods that take beginMethod as a parameter
                if ((creationOptions & TaskCreationOptions.LongRunning) != 0)
                    throw new ArgumentOutOfRangeException("creationOptions", Strings.Task_FromAsync_LongRunning);
                if ((creationOptions & TaskCreationOptions.PreferFairness) != 0)
                    throw new ArgumentOutOfRangeException("creationOptions", Strings.Task_FromAsync_PreferFairness);
            }

            // Check for general validity of options
            if ((creationOptions &
                    ~(TaskCreationOptions.AttachedToParent |
                      TaskCreationOptions.PreferFairness |
                      TaskCreationOptions.LongRunning)) != 0)
            {
                throw new ArgumentOutOfRangeException("creationOptions");
            }
        }

        //
        // ContinueWhenAll methods
        //

        // Performs some logic common to all ContinueWhenAll() overloads
        // Returns the Task<bool> off of which to continue.
        internal static Task<bool> CommonCWAllLogic(Task[] tasksCopy)
        {
            // tcs will be "fired" when final task completes
            // We want tcs.Task to be DetachedFromParent.
            //  -- If rval gets directly canceled, we don't want tcs.Task recorded as a child of Task.InternalCurrent,
            //     because Task.InternalCurrent will then not be able to complete.
            //  -- If Task.InternalCurrent gets canceled, tcs.Task would never know about it.  Again, best if it
            //     is not recorded as a child of Task.InternalCurrent.
            TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();

            // intermediate continuation tasks will decrement tasksLeft
            int tasksLeft = tasksCopy.Length;

            Action<Task> whenComplete = delegate(Task completedTask)
            {
                if (Interlocked.Decrement(ref tasksLeft) == 0) tcs.TrySetResult(true);
            };

            for (int i = 0; i < tasksCopy.Length; i++)
            {
                if (tasksCopy[i].IsCompleted) whenComplete(tasksCopy[i]); // Short-circuit the completion action, if possible
                else tasksCopy[i].AddCompletionAction(whenComplete); // simple completion action
            }

            return tcs.Task;
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see> 
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in 
        /// the <paramref name="tasks"/> array have completed.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="continuationAction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWhenAll(Task[] tasks, Action<Task[]> continuationAction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see> 
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in 
        /// the <paramref name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAll(Task[] tasks, Action<Task[]> continuationAction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in the <paramref
        /// name="tasks"/> array have completed.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAll(Task[] tasks, Action<Task[]> continuationAction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in the <paramref
        /// name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAll(Task[] tasks, Action<Task[]> continuationAction, CancellationToken cancellationToken,
            TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }


        // a private version that supports StackCrawlMark
        private static Task ContinueWhenAll(Task[] tasks, Action<Task[]> continuationAction, TaskContinuationOptions continuationOptions,
            CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            //check arguments
            CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationAction == null) throw new ArgumentNullException("continuationAction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");

            // Check the tasks array and make a defensive copy
            Task[] tasksCopy = CheckMultiContinuationTasksAndCopy(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }

            // Perform some common ContinueWhenAll() setup actions
            Task<bool> starter = CommonCWAllLogic(tasksCopy);

            // Preserve continuationOptions for this task, which is returned to the user.
            Task rval = starter.ContinueWith(finishedTask => continuationAction(tasksCopy), scheduler,
                cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see> 
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in 
        /// the <paramref name="tasks"/> array have completed.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="continuationAction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>[]> continuationAction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see> 
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in 
        /// the <paramref name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the 
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>[]> continuationAction, 
            CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in the <paramref
        /// name="tasks"/> array have completed.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>[]> continuationAction, 
            TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue.</param>
        /// <param name="continuationAction">The action delegate to execute when all tasks in the <paramref
        /// name="tasks"/> array have completed.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>[]> continuationAction, 
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationAction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }


        // a private version that supports StackCrawlMark
        private static Task ContinueWhenAll<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>[]> continuationAction, 
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            //check arguments
            CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationAction == null) throw new ArgumentNullException("continuationAction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");

            // Check the tasks array and make a defensive copy
            Task<TAntecedentResult>[] tasksCopy = CheckMultiContinuationTasksAndCopy<TAntecedentResult>(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Perform some common ContinueWhenAll() setup actions
            Task<bool> starter = CommonCWAllLogic(tasksCopy);

            // Preserve continuationOptions for this task, which is returned to the user.
            Task rval = starter.ContinueWith(finishedTask => continuationAction(tasksCopy), scheduler,
                cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TResult>(Task[] tasks, Func<Task[], TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TResult>(Task[] tasks, Func<Task[], TResult> continuationFunction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TResult>(Task[] tasks, Func<Task[], TResult> continuationFunction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TResult>(Task[] tasks, Func<Task[], TResult> continuationFunction, CancellationToken cancellationToken,
            TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // a private version that supports StackCrawlMark
        private Task<TResult> ContinueWhenAll<TResult>(Task[] tasks, Func<Task[], TResult> continuationFunction, TaskContinuationOptions continuationOptions, 
            CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // Delegate to TaskFactory<TResult>
            return TaskFactory<TResult>.ContinueWhenAll(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction,
            CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction,
            TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of a set of provided Tasks.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAll<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction,
            CancellationToken cancellationToken, TaskContinuationOptions  continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAll(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // a private version that supports StackCrawlMark
        private Task<TResult> ContinueWhenAll<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>[], TResult> continuationFunction, 
            TaskContinuationOptions continuationOptions,CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // Delegate to TaskFactory<TResult>
            return TaskFactory<TResult>.ContinueWhenAll<TAntecedentResult>(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // Utility method to abstract some common logic to a single definition.
        // Used by ContinueWhenAll/Any to bail out early on a pre-canceled token.
        private static Task CreateCanceledTask(TaskContinuationOptions continuationOptions, CancellationToken ct)
        {
            InternalTaskOptions dontcare;
            TaskCreationOptions tco;
            Task.CreationOptionsFromContinuationOptions(continuationOptions, out tco, out dontcare);
            return new Task(true, tco, ct);
        }

        //
        // ContinueWhenAny methods
        //

        // Common ContinueWhenAny logic
        // Returns Task<Task> off of which to create ultimate continuation task
        internal static Task<Task> CommonCWAnyLogic(Task[] tasksCopy)
        {
            // tcs will be "fired" when the first task completes
            TaskCompletionSource<Task> tcs = new TaskCompletionSource<Task>();

            // The first task to complete will record itself as the Result of trs/starter.
            Action<Task> whenComplete = delegate(Task t)
            {
                tcs.TrySetResult(t);
            };

            // At the completion of each task, fire whenComplete.
            for (int i = 0; i < tasksCopy.Length; i++)
            {
                if (tcs.Task.IsCompleted) break;  // Don't launch any more continuation tasks if trs has completed.

                // Shortcut if a task has already completed.
                if (tasksCopy[i].IsCompleted)
                {
                    // Short-circuit the creation of a completion task.
                    whenComplete(tasksCopy[i]);

                    // We've found a winner.  No need to create any more completion tasks.
                    break;
                }
                else tasksCopy[i].AddCompletionAction(whenComplete);
            }

            return tcs.Task;
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWhenAny(Task[] tasks, Action<Task> continuationAction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationAction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAny(Task[] tasks, Action<Task> continuationAction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationAction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAny(Task[] tasks, Action<Task> continuationAction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationAction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the <paramref
        /// name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAny(Task[] tasks, Action<Task> continuationAction, CancellationToken cancellationToken,
            TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationAction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // a private version that supports StackCrawlMark
        private Task ContinueWhenAny(Task[] tasks, Action<Task> continuationAction, TaskContinuationOptions continuationOptions, 
            CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // Check arguments
            CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationAction == null) throw new ArgumentNullException("continuationAction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");
            
            // Check the tasks array and make a defensive copy
            Task[] tasksCopy = CheckMultiContinuationTasksAndCopy(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Perform common ContinueWithAny() setup logic
            Task<Task> starter = CommonCWAnyLogic(tasksCopy);

            // returned continuation task, fired when starter completes
            Task rval = starter.ContinueWith(completedTask => continuationAction(completedTask.Result), scheduler,
                cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }


        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TResult>(Task[] tasks, Func<Task, TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TResult>(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TResult>(Task[] tasks, Func<Task, TResult> continuationFunction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TResult>(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TResult>(Task[] tasks, Func<Task, TResult> continuationFunction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TResult>(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TResult>(Task[] tasks, Func<Task, TResult> continuationFunction, CancellationToken cancellationToken,
            TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // private version that supports StackCrawlMark
        private Task<TResult> ContinueWhenAny<TResult>(Task[] tasks, Func<Task, TResult> continuationFunction, TaskContinuationOptions continuationOptions, 
            CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // Delegate to TaskFactory<TResult>
            return TaskFactory<TResult>.ContinueWhenAny(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult, TResult>(tasks, continuationFunction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction, 
            CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult, TResult>(tasks, continuationFunction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction, 
            TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult, TResult>(tasks, continuationFunction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task{TResult}">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TResult">The type of the result that is returned by the <paramref
        /// name="continuationFunction"/>
        /// delegate and associated with the created <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</typeparam>
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
        public Task<TResult> ContinueWhenAny<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction, 
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult, TResult>(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // private version that supports StackCrawlMark
        private Task<TResult> ContinueWhenAny<TAntecedentResult, TResult>(Task<TAntecedentResult>[] tasks, Func<Task<TAntecedentResult>, TResult> continuationFunction, 
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // Delegate to TaskFactory<TResult>
            return TaskFactory<TResult>.ContinueWhenAny<TAntecedentResult>(tasks, continuationFunction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array contains a null value.</exception>
        /// <exception cref="T:System.ArgumentException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is empty.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The exception that is thrown when one 
        /// of the elements in the <paramref name="tasks"/> array has been disposed.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>> continuationAction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationAction, m_defaultContinuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>> continuationAction, 
            CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationAction, m_defaultContinuationOptions, cancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>> continuationAction, 
            TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationAction, continuationOptions, m_defaultCancellationToken, DefaultScheduler, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation <see cref="T:System.Threading.Tasks.Task">Task</see>
        /// that will be started upon the completion of any Task in the provided set.
        /// </summary>
        /// <typeparam name="TAntecedentResult">The type of the result of the antecedent <paramref name="tasks"/>.</typeparam>
        /// <param name="tasks">The array of tasks from which to continue when one task completes.</param>
        /// <param name="continuationAction">The action delegate to execute when one task in the
        /// <paramref name="tasks"/> array completes.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see> 
        /// that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">The <see cref="System.Threading.Tasks.TaskContinuationOptions">
        /// TaskContinuationOptions</see> value that controls the behavior of
        /// the created continuation <see cref="T:System.Threading.Tasks.Task">Task</see>.</param>
        /// <param name="scheduler">The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>
        /// that is used to schedule the created continuation <see
        /// cref="T:System.Threading.Tasks.Task{TResult}"/>.</param>
        /// <returns>The new continuation <see cref="T:System.Threading.Tasks.Task"/>.</returns>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="tasks"/> array is null.</exception>
        /// <exception cref="T:System.ArgumentNullException">The exception that is thrown when the
        /// <paramref name="continuationAction"/> argument is null.</exception>
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
        public Task ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>> continuationAction, 
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWhenAny<TAntecedentResult>(tasks, continuationAction, continuationOptions, cancellationToken, scheduler, ref stackMark);
        }

        // private version that supports StackCrawlMark
        private Task ContinueWhenAny<TAntecedentResult>(Task<TAntecedentResult>[] tasks, Action<Task<TAntecedentResult>> continuationAction, 
            TaskContinuationOptions continuationOptions, CancellationToken cancellationToken, TaskScheduler scheduler, ref StackCrawlMark stackMark)
        {
            // check arguments
            CheckMultiTaskContinuationOptions(continuationOptions);
            if (tasks == null) throw new ArgumentNullException("tasks");
            if (continuationAction == null) throw new ArgumentNullException("continuationAction");
            if (scheduler == null) throw new ArgumentNullException("scheduler");

            // Check tasks array and make defensive copy
            Task<TAntecedentResult>[] tasksCopy = CheckMultiContinuationTasksAndCopy<TAntecedentResult>(tasks);

            // Bail early if cancellation has been requested.
            if (cancellationToken.IsCancellationRequested)
            {
                return CreateCanceledTask(continuationOptions, cancellationToken);
            }
            
            // Call common ContinueWhenAny() setup logic, extract the starter
            Task<Task> starter = CommonCWAnyLogic(tasksCopy);

            // returned continuation task, off of starter
            Task rval = starter.ContinueWith(completedTask => 
            {
                Task<TAntecedentResult> winner = completedTask.Result as Task<TAntecedentResult>;
                continuationAction(winner);
            }, scheduler, cancellationToken, continuationOptions, ref stackMark);

            return rval;
        }

        // Check task array and return a defensive copy.
        // Used with ContinueWhenAll()/ContinueWhenAny().
        internal static Task[] CheckMultiContinuationTasksAndCopy(Task[] tasks)
        {
            if (tasks == null)
                throw new ArgumentNullException("tasks");
            if (tasks.Length == 0)
                throw new ArgumentException(Strings.Task_MultiTaskContinuation_EmptyTaskList, "tasks");

            Task[] tasksCopy = new Task[tasks.Length];
            for (int i = 0; i < tasks.Length; i++)
            {
                tasksCopy[i] = tasks[i];

                if (tasksCopy[i] == null)
                    throw new ArgumentException(Strings.Task_MultiTaskContinuation_NullTask, "tasks");

                //tasksCopy[i].ThrowIfDisposed();
            }

            return tasksCopy;
        }

        internal static Task<TResult>[] CheckMultiContinuationTasksAndCopy<TResult>(Task<TResult>[] tasks)
        {
            if (tasks == null)
                throw new ArgumentNullException("tasks");
            if (tasks.Length == 0)
                throw new ArgumentException(Strings.Task_MultiTaskContinuation_EmptyTaskList, "tasks");

            Task<TResult>[] tasksCopy = new Task<TResult>[tasks.Length];
            for (int i = 0; i < tasks.Length; i++)
            {
                tasksCopy[i] = tasks[i];

                if (tasksCopy[i] == null)
                    throw new ArgumentException(Strings.Task_MultiTaskContinuation_NullTask, "tasks");

                //tasksCopy[i].ThrowIfDisposed();
            }

            return tasksCopy;
        }

        // Throw an exception if "options" argument specifies illegal options
        internal static void CheckMultiTaskContinuationOptions(TaskContinuationOptions continuationOptions)
        {
            // Construct a mask to check for illegal options
            TaskContinuationOptions NotOnAny = TaskContinuationOptions.NotOnCanceled |
                                               TaskContinuationOptions.NotOnFaulted |
                                               TaskContinuationOptions.NotOnRanToCompletion;

            // Check that LongRunning and ExecuteSynchronously are not specified together
            TaskContinuationOptions illegalMask = TaskContinuationOptions.ExecuteSynchronously | TaskContinuationOptions.LongRunning;
            if ((continuationOptions & illegalMask) == illegalMask)
            {
                throw new ArgumentOutOfRangeException("continuationOptions", Strings.Task_ContinueWith_ESandLR);
            }

            // Check that no nonsensical options are specified.
            if ((continuationOptions & ~(
                TaskContinuationOptions.LongRunning |
                TaskContinuationOptions.PreferFairness |
                TaskContinuationOptions.AttachedToParent |
                NotOnAny |
                TaskContinuationOptions.ExecuteSynchronously)) != 0)
            {
                throw new ArgumentOutOfRangeException("continuationOptions");
            }

            // Check that no "fire" options are specified.
            if ((continuationOptions & NotOnAny) != 0)
                throw new ArgumentOutOfRangeException("continuationOptions", Strings.Task_MultiTaskContinuation_FireOptions);
        }
            

    }

}
