// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//
// Task.cs
//
// <OWNER>hyildiz</OWNER>
//
// A schedulable unit of work.
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Threading;
using System.Diagnostics;
using System.Diagnostics.Contracts;

// Disable the "reference to volatile field not treated as volatile" error.
#pragma warning disable 0420

namespace System.Threading.Tasks
{
    /// <summary>
    /// Represents the current stage in the lifecycle of a <see cref="Task"/>.
    /// </summary>
    public enum TaskStatus
    {
        /// <summary> 
        /// The task has been initialized but has not yet been scheduled.
        /// </summary>
        Created,
        /// <summary> 
        /// The task is waiting to be activated and scheduled internally by the .NET Framework infrastructure.
        /// </summary>
        WaitingForActivation,
        /// <summary>
        /// The task has been scheduled for execution but has not yet begun executing.
        /// </summary>
        WaitingToRun,
        /// <summary>
        /// The task is running but has not yet completed.
        /// </summary>
        Running,
        // /// <summary>
        // /// The task is currently blocked in a wait state.
        // /// </summary>
        // Blocked,
        /// <summary>
        /// The task has finished executing and is implicitly waiting for
        /// attached child tasks to complete.
        /// </summary>
        WaitingForChildrenToComplete,
        /// <summary>
        /// The task completed execution successfully.
        /// </summary>
        RanToCompletion,
        /// <summary>
        /// The task acknowledged cancellation by throwing an OperationCanceledException2 with its own CancellationToken
        /// while the token was in signaled state, or the task's CancellationToken was already signaled before the
        /// task started executing.
        /// </summary>
        Canceled,
        /// <summary>
        /// The task completed due to an unhandled exception.
        /// </summary>
        Faulted
    }


    /// <summary>
    /// Represents an asynchronous operation.
    /// </summary>
    /// <remarks>
    /// <para>
    /// <see cref="Task"/> instances may be created in a variety of ways. The most common approach is by
    /// using the Task type's <see cref="Factory"/> property to retrieve a <see
    /// cref="System.Threading.Tasks.TaskFactory"/> instance that can be used to create tasks for several
    /// purposes. For example, to create a <see cref="Task"/> that runs an action, the factory's StartNew
    /// method may be used:
    /// <code>
    /// // C# 
    /// var t = Task.Factory.StartNew(() => DoAction());
    /// 
    /// ' Visual Basic 
    /// Dim t = Task.Factory.StartNew(Function() DoAction())
    /// </code>
    /// </para>
    /// <para>
    /// The <see cref="Task"/> class also provides constructors that initialize the Task but that do not
    /// schedule it for execution. For performance reasons, TaskFactory's StartNew method should be the
    /// preferred mechanism for creating and scheduling computational tasks, but for scenarios where creation
    /// and scheduling must be separated, the constructors may be used, and the task's <see cref="Start()"/>
    /// method may then be used to schedule the task for execution at a later time.
    /// </para>
    /// <para>
    /// All members of <see cref="Task"/>, except for <see cref="Dispose()"/>, are thread-safe
    /// and may be used from multiple threads concurrently.
    /// </para>
    /// <para>
    /// For operations that return values, the <see cref="System.Threading.Tasks.Task{TResult}"/> class
    /// should be used.
    /// </para>
    /// <para>
    /// For developers implementing custom debuggers, several internal and private members of Task may be
    /// useful (these may change from release to release). The Int32 m_taskId field serves as the backing
    /// store for the <see cref="Id"/> property, however accessing this field directly from a debugger may be
    /// more efficient than accessing the same value through the property's getter method (the
    /// s_taskIdCounter Int32 counter is used to retrieve the next available ID for a Task). Similarly, the
    /// Int32 m_stateFlags field stores information about the current lifecycle stage of the Task,
    /// information also accessible through the <see cref="Status"/> property. The m_action System.Object
    /// field stores a reference to the Task's delegate, and the m_stateObject System.Object field stores the
    /// async state passed to the Task by the developer. Finally, for debuggers that parse stack frames, the
    /// InternalWait method serves a potential marker for when a Task is entering a wait operation.
    /// </para>
    /// </remarks>

    [DebuggerTypeProxy(typeof(SystemThreadingTasks_TaskDebugView))]
    [DebuggerDisplay("Id = {Id}, Status = {Status}, Method = {DebuggerDisplayMethodDescription}")]
    public partial class Task : IThreadPoolWorkItem, IAsyncResult //, IDisposable
    {

        // Accessing ThreadStatic variables on classes with class constructors is much slower than if the class
        // has no class constructor.  So we declasre s_currentTask inside a nested class with no constructor.
        // Do not put any static variables with explicit initialization in this class, as it will regress performance.
        private static class ThreadLocals
        {
            internal static ThreadStatic<Task> s_currentTask = new ThreadStatic<Task>();  // The currently executing task.
        }

        internal static int s_taskIdCounter; //static counter used to generate unique task IDs
        private static TaskFactory s_factory = new TaskFactory();

        private int m_taskId; // this task's unique ID. initialized only if it is ever requested

        internal object m_action;    // The body of the task.  Might be Action<object>, Action<TState> or Action.
        // If m_action is set to null it will indicate that we operate in the
        // "externally triggered completion" mode, which is exclusively meant 
        // for the signalling Task<TResult> (aka. promise). In this mode,
        // we don't call InnerInvoke() in response to a Wait(), but simply wait on
        // the completion event which will be set when the Future class calls Finish().
        // But the event would now be signalled if Cancel() is called


        internal object m_stateObject; // A state object that can be optionally supplied, passed to action.
        internal TaskScheduler m_taskScheduler; // The task scheduler this task runs under. @TODO: is this required?

        internal readonly Task m_parent; // A task's parent, or null if parent-less.
        internal ExecutionContextLightup m_capturedContext; // The context to run the task within, if any.

        internal volatile int m_stateFlags;

        // State constants for m_stateFlags;
        // The bits of m_stateFlags are allocated as follows:
        //   0x40000000 - TaskBase state flag
        //   0x3FFF0000 - Task state flags
        //   0x0000FF00 - internal TaskCreationOptions flags
        //   0x000000FF - publicly exposed TaskCreationOptions flags
        //
        // See TaskCreationOptions for bit values associated with TaskCreationOptions
        //
        private const int OptionsMask = 0xFFFF; // signifies the Options portion of m_stateFlags
        internal const int TASK_STATE_STARTED = 0x10000;
        internal const int TASK_STATE_DELEGATE_INVOKED = 0x20000;
        internal const int TASK_STATE_DISPOSED = 0x40000;
        internal const int TASK_STATE_EXCEPTIONOBSERVEDBYPARENT = 0x80000;
        internal const int TASK_STATE_CANCELLATIONACKNOWLEDGED = 0x100000;
        internal const int TASK_STATE_FAULTED = 0x200000;
        internal const int TASK_STATE_CANCELED = 0x400000;
        internal const int TASK_STATE_WAITING_ON_CHILDREN = 0x800000;
        internal const int TASK_STATE_RAN_TO_COMPLETION = 0x1000000;
        internal const int TASK_STATE_WAITINGFORACTIVATION = 0x2000000;
        internal const int TASK_STATE_COMPLETION_RESERVED = 0x4000000;
        internal const int TASK_STATE_THREAD_WAS_ABORTED = 0x8000000;

        // Values for ContingentProperties.m_internalCancellationRequested.
        internal static int CANCELLATION_REQUESTED = 0x1;

        private volatile ManualResetEventSlim m_completionEvent; // Lazily created if waiting is required.

        // We moved a number of Task properties into this class.  The idea is that in most cases, these properties never
        // need to be accessed during the life cycle of a Task, so we don't want to instantiate them every time.  Once
        // one of these properties needs to be written, we will instantiate a ContingentProperties object and set
        // the appropriate property.
        internal class ContingentProperties
        {
            public volatile int m_internalCancellationRequested; // We keep canceled in its own field because threads legally race to set it.

            internal volatile int m_completionCountdown = 1; // # of active children + 1 (for this task itself). 
            // Used for ensuring all children are done before this task can complete
            // The extra count helps prevent the race for executing the final state transition
            // (i.e. whether the last child or this task itself should call FinishStageTwo())

            public volatile TaskExceptionHolder m_exceptionsHolder; // Tracks exceptions, if any have occurred

            public volatile List<Task> m_exceptionalChildren;       // A list of child tasks that threw an exception (TCEs don't count),
            // but haven't yet been waited on by the parent, lazily initialized.

            public volatile List<TaskContinuation> m_continuations; // A list of tasks or actions to be started upon completion, lazily initialized.

            public CancellationToken m_cancellationToken;
            public Shared<CancellationTokenRegistration> m_cancellationRegistration;
        }

        internal class Shared<T>
        {
            internal T Value;
            internal Shared(T value) { Value = value; }
        }

        // This field will only be instantiated to some non-null value if any ContingentProperties need to be set.
        internal volatile ContingentProperties m_contingentProperties;

        /// <summary>
        /// A type initializer that runs with the appropriate permissions.
        /// </summary>
        // // [SecuritySafeCritical]
        static Task()
        {
            s_ecCallback = new Action<object>(ExecutionContextCallback);
        }

        // Special internal constructor to create an already-completed task.
        // if canceled==true, create a Canceled task, or else create a RanToCompletion task.
        internal Task(bool canceled, TaskCreationOptions creationOptions, CancellationToken ct)
        {
            int optionFlags = (int)creationOptions;
            if (canceled)
            {
                m_stateFlags = TASK_STATE_CANCELED | TASK_STATE_CANCELLATIONACKNOWLEDGED | optionFlags;
                m_contingentProperties = ContingentPropertyCreator();
                m_contingentProperties.m_cancellationToken = ct;
                m_contingentProperties.m_internalCancellationRequested = CANCELLATION_REQUESTED;
            }
            else
                m_stateFlags = TASK_STATE_RAN_TO_COMPLETION | optionFlags;
        }

        // Special internal constructor to create an already-Faulted task.
        // Break this out when we need it.
        //
        //internal Task(Exception exception, bool attached)
        //{
        //    Task m_parent = attached ? Task.InternalCurrent : null;
        //
        //    if(m_parent != null) m_parent.AddNewChild();
        //
        //    m_contingentProperties = new ContingentProperties();
        //    m_contingentProperties.m_exceptionsHolder = new TaskExceptionHolder(this);
        //    m_contingentProperties.m_exceptionsHolder.Add(exception);
        //    m_stateFlags = TASK_STATE_FAULTED;
        //
        //    if (m_parent != null) m_parent.ProcessChildCompletion(this);
        //}



        // Special constructor for use with promise-style tasks.
        // Added promiseStyle parameter as an aid to the compiler to distinguish between (state,TCO) and
        // (action,TCO).  It should always be true.
        internal Task(object state, CancellationToken cancelationToken, TaskCreationOptions creationOptions, InternalTaskOptions internalOptions, bool promiseStyle)
        {
            Contract.Assert(promiseStyle, "Promise CTOR: promiseStyle was false");

            // Check the creationOptions. We only allow the attached/detached option to be specified for promise tasks.
            if ((creationOptions & ~(TaskCreationOptions.AttachedToParent)) != 0)
            {
                throw new ArgumentOutOfRangeException("creationOptions");
            }

            // Make sure that no illegal InternalTaskOptions are specified
            Contract.Assert((internalOptions & ~(InternalTaskOptions.PromiseTask)) == 0, "Illegal internal options in Task(obj,ct,tco,ito,bool)");

            // m_parent is readonly, and so must be set in the constructor.
            // Only set a parent if AttachedToParent is specified.
            if ((creationOptions & TaskCreationOptions.AttachedToParent) != 0)
                m_parent = Task.InternalCurrent;

            TaskConstructorCore(null, state, cancelationToken, creationOptions, internalOptions, TaskScheduler.Current);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the Task.</param>
        /// <exception cref="T:System.ArgumentNullException">The <paramref name="action"/> argument is null.</exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action action)
            : this((object)action, null, Task.InternalCurrent, CancellationToken.None, TaskCreationOptions.None, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action and <see cref="System.Threading.CancellationToken">CancellationToken</see>.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the Task.</param>
        /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// that will be assigned to the new Task.</param>
        /// <exception cref="T:System.ArgumentNullException">The <paramref name="action"/> argument is null.</exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action action, CancellationToken cancellationToken)
            : this((object)action, null, Task.InternalCurrent, cancellationToken, TaskCreationOptions.None, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action and creation options.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the task.</param>
        /// <param name="creationOptions">
        /// The <see cref="System.Threading.Tasks.TaskCreationOptions">TaskCreationOptions</see> used to
        /// customize the Task's behavior.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="action"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="creationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskCreationOptions"/>.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action action, TaskCreationOptions creationOptions)
            : this((object)action, null, Task.InternalCurrent, CancellationToken.None, creationOptions, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action and creation options.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the task.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <param name="creationOptions">
        /// The <see cref="System.Threading.Tasks.TaskCreationOptions">TaskCreationOptions</see> used to
        /// customize the Task's behavior.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="action"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="creationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskCreationOptions"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action action, CancellationToken cancellationToken, TaskCreationOptions creationOptions)
            : this((object)action, null, Task.InternalCurrent, cancellationToken, creationOptions, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }


        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action and state.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the task.</param>
        /// <param name="state">An object representing data to be used by the action.</param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="action"/> argument is null.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action<object> action, object state)
            : this((object)action, state, Task.InternalCurrent, CancellationToken.None, TaskCreationOptions.None, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action, state, snd options.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the task.</param>
        /// <param name="state">An object representing data to be used by the action.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="action"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action<object> action, object state, CancellationToken cancellationToken)
            : this((object)action, state, Task.InternalCurrent, cancellationToken, TaskCreationOptions.None, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action, state, snd options.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the task.</param>
        /// <param name="state">An object representing data to be used by the action.</param>
        /// <param name="creationOptions">
        /// The <see cref="System.Threading.Tasks.TaskCreationOptions">TaskCreationOptions</see> used to
        /// customize the Task's behavior.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="action"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="creationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskCreationOptions"/>.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action<object> action, object state, TaskCreationOptions creationOptions)
            : this((object)action, state, Task.InternalCurrent, CancellationToken.None, creationOptions, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// Initializes a new <see cref="Task"/> with the specified action, state, snd options.
        /// </summary>
        /// <param name="action">The delegate that represents the code to execute in the task.</param>
        /// <param name="state">An object representing data to be used by the action.</param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new task.</param>
        /// <param name="creationOptions">
        /// The <see cref="System.Threading.Tasks.TaskCreationOptions">TaskCreationOptions</see> used to
        /// customize the Task's behavior.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="action"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="creationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskCreationOptions"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task(Action<object> action, object state, CancellationToken cancellationToken, TaskCreationOptions creationOptions)
            : this((object)action, state, Task.InternalCurrent, cancellationToken, creationOptions, InternalTaskOptions.None, null)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            PossiblyCaptureContext(ref stackMark);
        }


        // For Task.ContinueWith() and Future.ContinueWith()
        internal Task(Action<object> action, object state, Task parent, CancellationToken cancellationToken,
            TaskCreationOptions creationOptions, InternalTaskOptions internalOptions, TaskScheduler scheduler, ref StackCrawlMark stackMark)
            : this((object)action, state, parent, cancellationToken, creationOptions, internalOptions, scheduler)
        {
            PossiblyCaptureContext(ref stackMark);
        }

        /// <summary>
        /// An internal constructor used by the factory methods on task and its descendent(s).
        /// This variant does not capture the ExecutionContext; it is up to the caller to do that.
        /// </summary>
        /// <param name="action">An action to execute.</param>
        /// <param name="state">Optional state to pass to the action.</param>
        /// <param name="parent">Parent of Task.</param>
        /// <param name="cancellationToken">A CancellationToken for the task.</param>
        /// <param name="scheduler">A task scheduler under which the task will run.</param>
        /// <param name="creationOptions">Options to control its execution.</param>
        /// <param name="internalOptions">Internal options to control its execution</param>
        internal Task(object action, object state, Task parent, CancellationToken cancellationToken,
            TaskCreationOptions creationOptions, InternalTaskOptions internalOptions, TaskScheduler scheduler)
        {
            if (action == null)
            {
                throw new ArgumentNullException("action");
            }

            Contract.Assert(action is Action || action is Action<object>);

            // This is readonly, and so must be set in the constructor
            // Keep a link to your parent if: (A) You are attached, or (B) you are self-replicating.
            // TODO: the check in the second line seems unnecessary - we already explicitly wait for replicating tasks
            if (((creationOptions & TaskCreationOptions.AttachedToParent) != 0) ||
                ((internalOptions & InternalTaskOptions.SelfReplicating) != 0)
                )
            {
                m_parent = parent;
            }

            TaskConstructorCore(action, state, cancellationToken, creationOptions, internalOptions, scheduler);
        }

        /// <summary>
        /// Common logic used by the following internal ctors:
        ///     Task()
        ///     Task(object action, object state, Task parent, TaskCreationOptions options, TaskScheduler taskScheduler)
        /// 
        /// ASSUMES THAT m_creatingTask IS ALREADY SET.
        /// 
        /// </summary>
        /// <param name="action">Action for task to execute.</param>
        /// <param name="state">Object to which to pass to action (may be null)</param>
        /// <param name="scheduler">Task scheduler on which to run thread (only used by continuation tasks).</param>
        /// <param name="cancellationToken">A CancellationToken for the Task.</param>
        /// <param name="creationOptions">Options to customize behavior of Task.</param>
        /// <param name="internalOptions">Internal options to customize behavior of Task.</param>
        internal void TaskConstructorCore(object action, object state, CancellationToken cancellationToken,
            TaskCreationOptions creationOptions, InternalTaskOptions internalOptions, TaskScheduler scheduler)
        {
            m_action = action;
            m_stateObject = state;
            m_taskScheduler = scheduler;

            // Check for validity of options
            if ((creationOptions &
                    ~(TaskCreationOptions.AttachedToParent |
                      TaskCreationOptions.LongRunning |
                      TaskCreationOptions.PreferFairness)) != 0)
            {
                throw new ArgumentOutOfRangeException("creationOptions");
            }

            // Check the validity of internalOptions
            int illegalInternalOptions =
                    (int)(internalOptions &
                            ~(InternalTaskOptions.SelfReplicating |
                            InternalTaskOptions.ChildReplica |
                            InternalTaskOptions.PromiseTask |
                            InternalTaskOptions.ContinuationTask |
                            InternalTaskOptions.QueuedByRuntime));

            Contract.Assert(illegalInternalOptions == 0, "TaskConstructorCore: Illegal internal options");


            // Throw exception if the user specifies both LongRunning and SelfReplicating
            if (((creationOptions & TaskCreationOptions.LongRunning) != 0) &&
                ((internalOptions & InternalTaskOptions.SelfReplicating) != 0))
            {
                throw new InvalidOperationException(Strings.Task_ctor_LRandSR);
            }

            // Assign options to m_stateAndOptionsFlag.
            Contract.Assert(m_stateFlags == 0, "TaskConstructorCore: non-zero m_stateFlags");
            Contract.Assert((((int)creationOptions) | OptionsMask) == OptionsMask, "TaskConstructorCore: options take too many bits");
            m_stateFlags = (int)creationOptions | (int)internalOptions;

            // For continuation tasks or TaskCompletionSource.Tasks, begin life in WaitingForActivation state
            // rather than Created state.
            if ((m_action == null) ||
                ((internalOptions & InternalTaskOptions.ContinuationTask) != 0))
            {
                m_stateFlags |= TASK_STATE_WAITINGFORACTIVATION;
            }

            // Now is the time to add the new task to the children list 
            // of the creating task if the options call for it.
            // We can safely call the creator task's AddNewChild() method to register it, 
            // because at this point we are already on its thread of execution.

            if (m_parent != null && (creationOptions & TaskCreationOptions.AttachedToParent) != 0)
            {
                m_parent.AddNewChild();
            }

            // if we have a non-null cancellationToken, allocate the contingent properties to save it
            // we need to do this as the very last thing in the construction path, because the CT registration could modify m_stateFlags
            if (cancellationToken.CanBeCanceled)
            {
                Contract.Assert((internalOptions & (InternalTaskOptions.ChildReplica | InternalTaskOptions.SelfReplicating)) == 0,
                    "TaskConstructorCore: Did not expect to see cancellable token for replica/replicating task.");
                LazyInitializer.EnsureInitialized<ContingentProperties>(ref m_contingentProperties, s_contingentPropertyCreator);
                m_contingentProperties.m_cancellationToken = cancellationToken;

                try
                {
                    cancellationToken.ThrowIfSourceDisposed();

                    // If an unstarted task has a valid CancellationToken that gets signalled while the task is still not queued
                    // we need to proactively cancel it, because it may never execute to transition itself. 
                    // The only way to accomplish this is to register a callback on the CT.
                    // We exclude Promise tasks from this, because TasckCompletionSource needs to fully control the inner tasks's lifetime (i.e. not allow external cancellations)                
                    if ((internalOptions &
                        (InternalTaskOptions.QueuedByRuntime | InternalTaskOptions.PromiseTask)) == 0)
                    {
                        CancellationTokenRegistration ctr = cancellationToken.InternalRegisterWithoutEC(s_taskCancelCallback, this);
                        m_contingentProperties.m_cancellationRegistration = new Shared<CancellationTokenRegistration>(ctr);
                    }
                }
                catch
                {
                    // If we have an exception related to our CancellationToken, then we need to subtract ourselves
                    // from our parent before throwing it.
                    if ((m_parent != null) && ((creationOptions & TaskCreationOptions.AttachedToParent) != 0))
                        m_parent.DisregardChild();
                    throw;
                }
            }
        }

        /// <summary>
        /// Checks if we registered a CT callback during construction, and deregisters it. 
        /// This should be called when we know the registration isn't useful anymore. Specifically from Finish() if the task has completed
        /// successfully or with an exception.
        /// </summary>
        internal void DeregisterCancellationCallback()
        {
            if (m_contingentProperties != null &&
                m_contingentProperties.m_cancellationRegistration != null)
            {
                // Harden against ODEs thrown from disposing of the CTR.
                // Since the task has already been put into a final state by the time this
                // is called, all we can do here is suppress the exception.
                try
                {
                    m_contingentProperties.m_cancellationRegistration.Value.Dispose();
                }
                catch (ObjectDisposedException) { }

                m_contingentProperties.m_cancellationRegistration = null;
            }
        }


        // Static delegate to be used as a cancellation callback on unstarted tasks that have a valid cancellation token.
        // This is necessary to transition them into canceled state if their cancellation token is signalled while they are still not queued
        internal static Action<Object> s_taskCancelCallback = new Action<Object>(TaskCancelCallback);
        private static void TaskCancelCallback(Object o)
        {
            Task t = (Task)o;
            t.InternalCancel(false);
        }

        // Debugger support
        private string DebuggerDisplayMethodDescription
        {
            get
            {
                Delegate d = (Delegate)m_action;
                return d != null ? d.Method.ToString() : "{null}";
            }
        }


        /// <summary>
        /// Captures the ExecutionContext so long as flow isn't suppressed.
        /// </summary>
        /// <param name="stackMark">A stack crawl mark pointing to the frame of the caller.</param>
        internal void PossiblyCaptureContext(ref StackCrawlMark stackMark)
        {
            // In the legacy .NET 3.5 build, we don't have the optimized overload of Capture()
            // available, so we call the parameterless overload.
//#if PFX_LEGACY_3_5
            m_capturedContext = ExecutionContextLightup.Instance.Capture();
//#else
//            m_capturedContext = ExecutionContext.Capture(
//                ref stackMark,
//                ExecutionContext.CaptureOptions.IgnoreSyncCtx | ExecutionContext.CaptureOptions.OptimizeDefaultCase);
//#endif
        }

        // Internal property to process TaskCreationOptions access and mutation.
        internal TaskCreationOptions Options
        {
            get
            {
                Contract.Assert((OptionsMask & 1) == 1, "OptionsMask needs a shift in Options.get");
                return (TaskCreationOptions)(m_stateFlags & OptionsMask);
            }
        }

        // Atomically OR-in newBits to m_stateFlags, while making sure that
        // no illegalBits are set.  Returns true on success, false on failure.
        internal bool AtomicStateUpdate(int newBits, int illegalBits)
        {
            int oldFlags = 0;
            return AtomicStateUpdate(newBits, illegalBits, ref oldFlags);
        }

        internal bool AtomicStateUpdate(int newBits, int illegalBits, ref int oldFlags)
        {
            SpinWait sw = new SpinWait();

            do
            {
                oldFlags = m_stateFlags;
                if ((oldFlags & illegalBits) != 0) return false;
                if (Interlocked.CompareExchange(ref m_stateFlags, oldFlags | newBits, oldFlags) == oldFlags)
                {
                    return true;
                }
                sw.SpinOnce();
            } while (true);

        }

        // Atomically mark a Task as started while making sure that it is not canceled.
        internal bool MarkStarted()
        {
            return AtomicStateUpdate(TASK_STATE_STARTED, TASK_STATE_CANCELED | TASK_STATE_STARTED);
        }


        /// <summary>
        /// Internal function that will be called by a new child task to add itself to 
        /// the children list of the parent (this).
        /// 
        /// Since a child task can only be created from the thread executing the action delegate
        /// of this task, reentrancy is neither required nor supported. This should not be called from
        /// anywhere other than the task construction/initialization codepaths.
        /// </summary>
        internal void AddNewChild()
        {
            Contract.Assert(Task.InternalCurrent == this || this.IsSelfReplicatingRoot, "Task.AddNewChild(): Called from an external context");

            LazyInitializer.EnsureInitialized<ContingentProperties>(ref m_contingentProperties, s_contingentPropertyCreator);


            if (m_contingentProperties.m_completionCountdown == 1 && !IsSelfReplicatingRoot)
            {
                // A count of 1 indicates so far there was only the parent, and this is the first child task
                // Single kid => no fuss about who else is accessing the count. Let's save ourselves 100 cycles
                // We exclude self replicating root tasks from this optimization, because further child creation can take place on 
                // other cores and with bad enough timing this write may not be visible to them.
                m_contingentProperties.m_completionCountdown++;
            }
            else
            {
                // otherwise do it safely
                Interlocked.Increment(ref m_contingentProperties.m_completionCountdown);
            }
        }

        // This is called in the case where a new child is added, but then encounters a CancellationToken-related exception.
        // We need to subtract that child from m_completionCountdown, or the parent will never complete.
        internal void DisregardChild()
        {
            Contract.Assert(Task.InternalCurrent == this, "Task.DisregardChild(): Called from an external context");
            Contract.Assert((m_contingentProperties != null) && (m_contingentProperties.m_completionCountdown >= 2), "Task.DisregardChild(): Expected parent count to be >= 2");

            Interlocked.Decrement(ref m_contingentProperties.m_completionCountdown);
        }

        /// <summary>
        /// Starts the <see cref="Task"/>, scheduling it for execution to the current <see
        /// cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>.
        /// </summary>
        /// <remarks>
        /// A task may only be started and run only once.  Any attempts to schedule a task a second time
        /// will result in an exception.
        /// </remarks>
        /// <exception cref="InvalidOperationException">
        /// The <see cref="Task"/> is not in a valid state to be started. It may have already been started,
        /// executed, or canceled, or it may have been created in a manner that doesn't support direct
        /// scheduling.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The <see cref="Task"/> instance has been disposed.
        /// </exception>
        public void Start()
        {
            Start(TaskScheduler.Current);
        }

        /// <summary>
        /// Starts the <see cref="Task"/>, scheduling it for execution to the specified <see
        /// cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>.
        /// </summary>
        /// <remarks>
        /// A task may only be started and run only once. Any attempts to schedule a task a second time will
        /// result in an exception.
        /// </remarks>
        /// <param name="scheduler">
        /// The <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> with which to associate
        /// and execute this task.
        /// </param>
        /// <exception cref="ArgumentNullException">
        /// The <paramref name="scheduler"/> argument is null.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// The <see cref="Task"/> is not in a valid state to be started. It may have already been started,
        /// executed, or canceled, or it may have been created in a manner that doesn't support direct
        /// scheduling.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The <see cref="Task"/> instance has been disposed.
        /// </exception>
        public void Start(TaskScheduler scheduler)
        {
            // Throw an exception if the task has previously been disposed.
            //ThrowIfDisposed();

            // Need to check this before (m_action == null) because completed tasks will
            // set m_action to null.  We would want to know if this is the reason that m_action == null.
            if (IsCompleted)
            {
                throw new InvalidOperationException(Strings.Task_Start_TaskCompleted);
            }

            if (m_action == null)
            {
                throw new InvalidOperationException(Strings.Task_Start_NullAction);
            }

            if (scheduler == null)
            {
                throw new ArgumentNullException("scheduler");
            }

            if ((Options & (TaskCreationOptions)InternalTaskOptions.ContinuationTask) != 0)
            {
                throw new InvalidOperationException(Strings.Task_Start_ContinuationTask);
            }

            // Make sure that Task only gets started once.  Or else throw an exception.
            if (Interlocked.CompareExchange(ref m_taskScheduler, scheduler, null) != null)
            {
                throw new InvalidOperationException(Strings.Task_Start_AlreadyStarted);
            }

            ScheduleAndStart(true);
        }

        /// <summary>
        /// Runs the <see cref="Task"/> synchronously on the current <see
        /// cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// A task may only be started and run only once. Any attempts to schedule a task a second time will
        /// result in an exception.
        /// </para>
        /// <para>
        /// Tasks executed with <see cref="RunSynchronously()"/> will be associated with the current <see
        /// cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see>.
        /// </para>
        /// <para>
        /// If the target scheduler does not support running this Task on the current thread, the Task will
        /// be scheduled for execution on the scheduler, and the current thread will block until the
        /// Task has completed execution.
        /// </para>
        /// </remarks>
        /// <exception cref="InvalidOperationException">
        /// The <see cref="Task"/> is not in a valid state to be started. It may have already been started,
        /// executed, or canceled, or it may have been created in a manner that doesn't support direct
        /// scheduling.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The <see cref="Task"/> instance has been disposed.
        /// </exception>
        public void RunSynchronously()
        {
            InternalRunSynchronously(TaskScheduler.Current);
        }

        /// <summary>
        /// Runs the <see cref="Task"/> synchronously on the <see
        /// cref="System.Threading.Tasks.TaskScheduler">scheduler</see> provided.
        /// </summary>
        /// <remarks>
        /// <para>
        /// A task may only be started and run only once. Any attempts to schedule a task a second time will
        /// result in an exception.
        /// </para>
        /// <para>
        /// If the target scheduler does not support running this Task on the current thread, the Task will
        /// be scheduled for execution on the scheduler, and the current thread will block until the
        /// Task has completed execution.
        /// </para>
        /// </remarks>
        /// <exception cref="InvalidOperationException">
        /// The <see cref="Task"/> is not in a valid state to be started. It may have already been started,
        /// executed, or canceled, or it may have been created in a manner that doesn't support direct
        /// scheduling.
        /// </exception>
        /// <exception cref="ObjectDisposedException">
        /// The <see cref="Task"/> instance has been disposed.
        /// </exception>
        /// <exception cref="ArgumentNullException">The <paramref name="scheduler"/> parameter
        /// is null.</exception>
        /// <param name="scheduler">The scheduler on which to attempt to run this task inline.</param>
        public void RunSynchronously(TaskScheduler scheduler)
        {
            if (scheduler == null)
            {
                throw new ArgumentNullException("scheduler");
            }

            InternalRunSynchronously(scheduler);
        }

        //
        // Internal version of RunSynchronously that allows a taskScheduler argument. 
        // 
        // // [SecuritySafeCritical] // Needed for QueueTask
        internal void InternalRunSynchronously(TaskScheduler scheduler)
        {
            Contract.Assert(scheduler != null, "Task.InternalRunSynchronously(): null TaskScheduler");
            //ThrowIfDisposed();

            // Can't call this method on a continuation task
            if ((Options & (TaskCreationOptions)InternalTaskOptions.ContinuationTask) != 0)
            {
                throw new InvalidOperationException(Strings.Task_RunSynchronously_Continuation);
            }

            // Can't call this method on a task that has already completed
            if (IsCompleted)
            {
                throw new InvalidOperationException(Strings.Task_RunSynchronously_TaskCompleted);
            }

            // Can't call this method on a promise-style task
            if (m_action == null)
            {
                throw new InvalidOperationException(Strings.Task_RunSynchronously_Promise);
            }

            // Make sure that Task only gets started once.  Or else throw an exception.
            if (Interlocked.CompareExchange(ref m_taskScheduler, scheduler, null) != null)
            {
                throw new InvalidOperationException(Strings.Task_RunSynchronously_AlreadyStarted);
            }

            // execute only if we win the race against concurrent cancel attempts.
            // otherwise throw an exception, because we've been canceled.
            if (MarkStarted())
            {
                bool taskQueued = false;
                try
                {
                    // We wrap TryRunInline() in a try/catch block and move an excepted task to Faulted here,
                    // but not in Wait()/WaitAll()/FastWaitAll().  Here, we know for sure that the
                    // task will not be subsequently scheduled (assuming that the scheduler adheres
                    // to the guideline that an exception implies that no state change took place),
                    // so it is safe to catch the exception and move the task to a final state.  The
                    // same cannot be said for Wait()/WaitAll()/FastWaitAll().
                    if (!scheduler.TryRunInline(this, false))
                    {
                        scheduler.QueueTask(this);
                        taskQueued = true; // only mark this after successfully queuing the task.
                    }

                    // A successful TryRunInline doesn't guarantee completion, as there may be unfinished children
                    // Also if we queued the task above, we need to wait.
                    if (!IsCompleted)
                    {
                        CompletedEvent.Wait();
                    }
                }
                catch (Exception e)
                {
                    // we 1) either received an unexpected exception originating from a custom scheduler, which needs to be wrapped in a TSE and thrown
                    //    2) or a a ThreadAbortException, which we need to skip here, because it would already have been handled in Task.Execute
                    if (!taskQueued && !(ThreadingServices.IsThreadAbort(e)))
                    {
                        // We had a problem with TryRunInline() or QueueTask().  
                        // Record the exception, marking ourselves as Completed/Faulted.
                        TaskSchedulerException tse = new TaskSchedulerException(e);
                        AddException(tse);
                        Finish(false);

                        // Mark ourselves as "handled" to avoid crashing the finalizer thread if the caller neglects to
                        // call Wait() on this task.
                        // m_contingentProperties.m_exceptionHolder *should* already exist after AddException()
                        Contract.Assert((m_contingentProperties != null) && (m_contingentProperties.m_exceptionsHolder != null),
                            "Task.InternalRunSynchronously(): Expected m_contingentProperties.m_exceptionsHolder to exist");
                        m_contingentProperties.m_exceptionsHolder.MarkAsHandled(false);

                        // And re-throw.
                        throw tse;
                    }
                    else
                    {
                        // We had a problem with CompletedEvent.Wait().  Just re-throw.
                        throw;
                    }
                }
            }
            else
            {
                Contract.Assert((m_stateFlags & TASK_STATE_CANCELED) != 0, "Task.RunSynchronously: expected TASK_STATE_CANCELED to be set");
                // Can't call this method on canceled task.
                throw new InvalidOperationException(Strings.Task_RunSynchronously_TaskCompleted);
            }
        }


        ////
        //// Helper methods for Factory StartNew methods.
        ////


        // Implicitly converts action to object and handles the meat of the StartNew() logic.
        internal static Task InternalStartNew(
            Task creatingTask, object action, object state, CancellationToken cancellationToken, TaskScheduler scheduler,
            TaskCreationOptions options, InternalTaskOptions internalOptions, ref StackCrawlMark stackMark)
        {
            // Validate arguments.
            if (scheduler == null)
            {
                throw new ArgumentNullException("scheduler");
            }

            // Create and schedule the task. This throws an InvalidOperationException if already shut down.
            // Here we add the InternalTaskOptions.QueuedByRuntime to the internalOptions, so that TaskConstructorCore can skip the cancellation token registration
            Task t = new Task(action, state, creatingTask, cancellationToken, options, internalOptions | InternalTaskOptions.QueuedByRuntime, scheduler);
            t.PossiblyCaptureContext(ref stackMark);

            t.ScheduleAndStart(false);
            return t;
        }


        /////////////
        // properties

        /// <summary>
        /// Gets a unique ID for this <see cref="Task">Task</see> instance.
        /// </summary>
        /// <remarks>
        /// Task IDs are assigned on-demand and do not necessarily represent the order in the which Task
        /// instances were created.
        /// </remarks>
        public int Id
        {
            get
            {
                if (m_taskId == 0)
                {
                    int newId = 0;
                    // We need to repeat if Interlocked.Increment wraps around and returns 0.
                    // Otherwise next time this task's Id is queried it will get a new value
                    do
                    {
                        newId = Interlocked.Increment(ref s_taskIdCounter);
                    }
                    while (newId == 0);

                    Interlocked.CompareExchange(ref m_taskId, newId, 0);
                }

                return m_taskId;
            }
        }

        /// <summary>
        /// Returns the unique ID of the currently executing <see cref="Task">Task</see>.
        /// </summary>
        public static int? CurrentId
        {
            get
            {
                Task currentTask = InternalCurrent;
                if (currentTask != null)
                    return currentTask.Id;
                else
                    return null;
            }
        }

        /// <summary>
        /// Gets the <see cref="Task">Task</see> instance currently executing, or
        /// null if none exists.
        /// </summary>
        internal static Task InternalCurrent
        {
            get { return ThreadLocals.s_currentTask.Value; }
        }

        /// <summary>
        /// Gets the <see cref="T:System.AggregateException">Exception</see> that caused the <see
        /// cref="Task">Task</see> to end prematurely. If the <see
        /// cref="Task">Task</see> completed successfully or has not yet thrown any
        /// exceptions, this will return null.
        /// </summary>
        /// <remarks>
        /// Tasks that throw unhandled exceptions store the resulting exception and propagate it wrapped in a
        /// <see cref="System.AggregateException"/> in calls to <see cref="Wait()">Wait</see>
        /// or in accesses to the <see cref="Exception"/> property.  Any exceptions not observed by the time
        /// the Task instance is garbage collected will be propagated on the finalizer thread.
        /// </remarks>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task">Task</see>
        /// has been disposed.
        /// </exception>
        public AggregateException Exception
        {
            get
            {
                AggregateException e = null;

                // If you're faulted, retrieve the exception(s)
                if (IsFaulted) e = GetExceptions(false);

                // Only return an exception in faulted state (skip manufactured exceptions)
                // A "benevolent" race condition makes it possible to return null when IsFaulted is
                // true (i.e., if IsFaulted is set just after the check to IsFaulted above).
                Contract.Assert((e == null) || IsFaulted, "Task.Exception_get(): returning non-null value when not Faulted");

                return e;
            }
        }

        /// <summary>
        /// Gets the <see cref="T:System.Threading.Tasks.TaskStatus">TaskStatus</see> of this Task. 
        /// </summary>
        public TaskStatus Status
        {
            get
            {
                TaskStatus rval;

                // get a cached copy of the state flags.  This should help us
                // to get a consistent view of the flags if they are changing during the
                // execution of this method.
                int sf = m_stateFlags;

                if ((sf & TASK_STATE_FAULTED) != 0) rval = TaskStatus.Faulted;
                else if ((sf & TASK_STATE_CANCELED) != 0) rval = TaskStatus.Canceled;
                else if ((sf & TASK_STATE_RAN_TO_COMPLETION) != 0) rval = TaskStatus.RanToCompletion;
                else if ((sf & TASK_STATE_WAITING_ON_CHILDREN) != 0) rval = TaskStatus.WaitingForChildrenToComplete;
                else if ((sf & TASK_STATE_DELEGATE_INVOKED) != 0) rval = TaskStatus.Running;
                else if ((sf & TASK_STATE_STARTED) != 0) rval = TaskStatus.WaitingToRun;
                else if ((sf & TASK_STATE_WAITINGFORACTIVATION) != 0) rval = TaskStatus.WaitingForActivation;
                else rval = TaskStatus.Created;

                return rval;
            }
        }

        /// <summary>
        /// Gets whether this <see cref="Task">Task</see> instance has completed
        /// execution due to being canceled.
        /// </summary>
        /// <remarks>
        /// A <see cref="Task">Task</see> will complete in Canceled state either if its <see cref="CancellationToken">CancellationToken</see> 
        /// was marked for cancellation before the task started executing, or if the task acknowledged the cancellation request on 
        /// its already signaled CancellationToken by throwing an 
        /// <see cref="System.OperationCanceledException">OperationCanceledException2</see> that bears the same 
        /// <see cref="System.Threading.CancellationToken">CancellationToken</see>.
        /// </remarks>
        public bool IsCanceled
        {
            get
            {
                // Return true if canceled bit is set and faulted bit is not set
                return (m_stateFlags & (TASK_STATE_CANCELED | TASK_STATE_FAULTED)) == TASK_STATE_CANCELED;
            }
        }

        /// <summary>
        /// Returns true if this task has a cancellation token and it was signaled.
        /// To be used internally in execute entry codepaths.
        /// </summary>
        internal bool IsCancellationRequested
        {
            get
            {
                // check both the internal cancellation request flag and the CancellationToken attached to this task
                return ((m_contingentProperties != null) && (m_contingentProperties.m_internalCancellationRequested == CANCELLATION_REQUESTED)) ||
                    CancellationToken.IsCancellationRequested;
            }
        }

        // Static delegate used for creating a ContingentProperties object from LazyInitializer.
        internal static Func<ContingentProperties> s_contingentPropertyCreator = new Func<ContingentProperties>(ContingentPropertyCreator);
        private static ContingentProperties ContingentPropertyCreator()
        {
            return new ContingentProperties();
        }

        /// <summary>
        /// This internal property provides access to the CancellationToken that was set on the task 
        /// when it was constructed.
        /// </summary>
        internal CancellationToken CancellationToken
        {
            get
            {
                return (m_contingentProperties == null) ? CancellationToken.None :
                                                            m_contingentProperties.m_cancellationToken;
            }
        }

        /// <summary>
        /// Gets whether this <see cref="Task"/> threw an OperationCanceledException2 while its CancellationToken was signaled.
        /// </summary>
        internal bool IsCancellationAcknowledged
        {
            get { return (m_stateFlags & TASK_STATE_CANCELLATIONACKNOWLEDGED) != 0; }
        }


        /// <summary>
        /// Gets whether this <see cref="Task">Task</see> has completed.
        /// </summary>
        /// <remarks>
        /// <see cref="IsCompleted"/> will return true when the Task is in one of the three
        /// final states: <see cref="System.Threading.Tasks.TaskStatus.RanToCompletion">RanToCompletion</see>,
        /// <see cref="System.Threading.Tasks.TaskStatus.Faulted">Faulted</see>, or
        /// <see cref="System.Threading.Tasks.TaskStatus.Canceled">Canceled</see>.
        /// </remarks>
        public bool IsCompleted
        {
            get
            {
                return ((m_stateFlags & (TASK_STATE_CANCELED | TASK_STATE_FAULTED | TASK_STATE_RAN_TO_COMPLETION)) != 0);
            }
        }

        // For use in InternalWait -- marginally faster than (Task.Status == TaskStatus.RanToCompletion)
        internal bool CompletedSuccessfully
        {
            get
            {
                int completedMask = TASK_STATE_CANCELED | TASK_STATE_FAULTED | TASK_STATE_RAN_TO_COMPLETION;
                return (m_stateFlags & completedMask) == TASK_STATE_RAN_TO_COMPLETION;
            }
        }

        /// <summary>
        /// Checks whether this task has been disposed.
        /// </summary>
        internal bool IsDisposed
        {
            get { return (m_stateFlags & TASK_STATE_DISPOSED) != 0; }
        }

        /// <summary>
        /// Throws an exception if the task has been disposed, and hence can no longer be accessed.
        /// </summary>
        /// <exception cref="T:System.ObjectDisposedException">The task has been disposed.</exception>
        internal void ThrowIfDisposed()
        {
            if (IsDisposed)
            {
                throw new ObjectDisposedException(null, Strings.Task_ThrowIfDisposed);
            }
        }


        /// <summary>
        /// Gets the <see cref="T:System.Threading.Tasks.TaskCreationOptions">TaskCreationOptions</see> used
        /// to create this task.
        /// </summary>
        public TaskCreationOptions CreationOptions
        {
            get { return Options & (TaskCreationOptions)(~InternalTaskOptions.InternalOptionsMask); }
        }

        /// <summary>
        /// Gets a <see cref="T:System.Threading.WaitHandle"/> that can be used to wait for the task to
        /// complete.
        /// </summary>
        /// <remarks>
        /// Using the wait functionality provided by <see cref="Wait()"/>
        /// should be preferred over using <see cref="IAsyncResult.AsyncWaitHandle"/> for similar
        /// functionality.
        /// </remarks>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        WaitHandle IAsyncResult.AsyncWaitHandle
        {
            // Although a slim event is used internally to avoid kernel resource allocation, this function
            // forces allocation of a true WaitHandle when called.
            get
            {
                ThrowIfDisposed();
                return CompletedEvent.WaitHandle;
            }
        }

        // Overridden by Task<TResult> to return m_futureState
        internal virtual object InternalAsyncState
        {
            get { return m_stateObject; }
        }

        /// <summary>
        /// Gets the state object supplied when the <see cref="Task">Task</see> was created,
        /// or null if none was supplied.
        /// </summary>
        public object AsyncState
        {
            get { return InternalAsyncState; }
        }

        /// <summary>
        /// Gets an indication of whether the asynchronous operation completed synchronously.
        /// </summary>
        /// <value>true if the asynchronous operation completed synchronously; otherwise, false.</value>
        bool IAsyncResult.CompletedSynchronously
        {
            get
            {
                // @TODO: do we want to faithfully return 'true' if the task ran on the same
                //     thread which created it?  Probably not, but we need to think about it.
                return false;
            }
        }

        // @TODO: can this be retrieved from TLS instead of storing it?
        /// <summary>
        /// Provides access to the TaskScheduler responsible for executing this Task.
        /// </summary>
        internal TaskScheduler ExecutingTaskScheduler
        {
            get { return m_taskScheduler; }
        }

        /// <summary>
        /// Provides access to factory methods for creating <see cref="Task"/> and <see cref="Task{TResult}"/> instances.
        /// </summary>
        /// <remarks>
        /// The factory returned from <see cref="Factory"/> is a default instance
        /// of <see cref="System.Threading.Tasks.TaskFactory"/>, as would result from using
        /// the default constructor on TaskFactory.
        /// </remarks>
        public static TaskFactory Factory { get { return s_factory; } }

        /// <summary>
        /// Provides an event that can be used to wait for completion.
        /// Only called by Wait*(), which means that we really do need to instantiate a completion event.
        /// </summary>
        internal ManualResetEventSlim CompletedEvent
        {
            get
            {
                if (m_completionEvent == null)
                {
                    bool wasCompleted = IsCompleted;
                    ManualResetEventSlim newMre = new ManualResetEventSlim(wasCompleted);
                    if (Interlocked.CompareExchange(ref m_completionEvent, newMre, null) != null)
                    {
                        // We lost the race, so we will just close the event right away.
                        newMre.Dispose();
                    }
                    else if (!wasCompleted && IsCompleted)
                    {
                        // We published the event as unset, but the task has subsequently completed.
                        // Set the event's state properly so that callers don't deadlock.
                        newMre.Set();
                    }
                }

                return m_completionEvent;
            }
        }

        /// <summary>
        /// Sets the internal completion event.
        /// </summary>
        private void SetCompleted()
        {

            ManualResetEventSlim mres = m_completionEvent;
            if (mres != null)
            {
                mres.Set();
            }
        }

        /// <summary>
        /// Determines whether this is the root task of a self replicating group.
        /// </summary>
        internal bool IsSelfReplicatingRoot
        {
            get
            {
                return ((Options & (TaskCreationOptions)InternalTaskOptions.SelfReplicating) != 0) &&
                        ((Options & (TaskCreationOptions)InternalTaskOptions.ChildReplica) == 0);
            }
        }

        /// <summary>
        /// Determines whether the task is a replica itself.
        /// </summary>
        internal bool IsChildReplica
        {
            get { return (Options & (TaskCreationOptions)InternalTaskOptions.ChildReplica) != 0; }
        }

        internal int ActiveChildCount
        {
            get { return m_contingentProperties != null ? m_contingentProperties.m_completionCountdown - 1 : 0; }
        }

        /// <summary>
        /// The property formerly known as IsFaulted.
        /// </summary>
        internal bool ExceptionRecorded
        {
            get { return (m_contingentProperties != null) && (m_contingentProperties.m_exceptionsHolder != null); }
        }

        /// <summary>
        /// Gets whether the <see cref="Task"/> completed due to an unhandled exception.
        /// </summary>
        /// <remarks>
        /// If <see cref="IsFaulted"/> is true, the Task's <see cref="Status"/> will be equal to
        /// <see cref="System.Threading.Tasks.TaskStatus.Faulted">TaskStatus.Faulted</see>, and its
        /// <see cref="Exception"/> property will be non-null.
        /// </remarks>
        public bool IsFaulted
        {
            get
            {
                // Faulted is "king" -- if that bit is present (regardless of other bits), we are faulted.
                return ((m_stateFlags & TASK_STATE_FAULTED) != 0);
            }
        }


#if DEBUG
        /// <summary>
        /// Retrieves an identifier for the task.
        /// </summary>
        internal int InternalId
        {
            get { return GetHashCode(); }
        }
#endif

        /////////////
        // methods


        /// <summary>
        /// Disposes the <see cref="Task"/>, releasing all of its unmanaged resources.  
        /// </summary>
        /// <remarks>
        /// Unlike most of the members of <see cref="Task"/>, this method is not thread-safe.
        /// Also, <see cref="Dispose()"/> may only be called on a <see cref="Task"/> that is in one of
        /// the final states: <see cref="System.Threading.Tasks.TaskStatus.RanToCompletion">RanToCompletion</see>,
        /// <see cref="System.Threading.Tasks.TaskStatus.Faulted">Faulted</see>, or
        /// <see cref="System.Threading.Tasks.TaskStatus.Canceled">Canceled</see>.
        /// </remarks>
        /// <exception cref="T:System.InvalidOperationException">
        /// The exception that is thrown if the <see cref="Task"/> is not in 
        /// one of the final states: <see cref="System.Threading.Tasks.TaskStatus.RanToCompletion">RanToCompletion</see>,
        /// <see cref="System.Threading.Tasks.TaskStatus.Faulted">Faulted</see>, or
        /// <see cref="System.Threading.Tasks.TaskStatus.Canceled">Canceled</see>.
        /// </exception>       
        internal void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Disposes the <see cref="Task"/>, releasing all of its unmanaged resources.  
        /// </summary>
        /// <param name="disposing">
        /// A Boolean value that indicates whether this method is being called due to a call to <see
        /// cref="Dispose()"/>.
        /// </param>
        /// <remarks>
        /// Unlike most of the members of <see cref="Task"/>, this method is not thread-safe.
        /// </remarks>
        internal virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Task must be completed to dispose
                if (!IsCompleted)
                {
                    throw new InvalidOperationException(Strings.Task_Dispose_NotCompleted);
                }

                var tmp = m_completionEvent; // make a copy to protect against racing Disposes.

                // Dispose of the underlying completion event
                if (tmp != null)
                {
                    // In the unlikely event that our completion event is inflated but not yet signaled,
                    // go ahead and signal the event.  If you dispose of an unsignaled MRES, then any waiters
                    // will deadlock; an ensuing Set() will not wake them up.  In the event of an AppDomainUnload,
                    // there is no guarantee that anyone else is going to signal the event, and it does no harm to 
                    // call Set() twice on m_completionEvent.
                    if (!tmp.IsSet) tmp.Set();

                    tmp.Dispose();
                    m_completionEvent = null;
                }

            }

            // We OR the flags to indicate the object has been disposed.  This is not
            // thread-safe -- trying to dispose while a task is running can lead to corruption.
            // Do this regardless of the value of "disposing".
            m_stateFlags |= TASK_STATE_DISPOSED;
        }

        /////////////
        // internal helpers


        /// <summary>
        /// Schedules the task for execution.
        /// </summary>
        /// <param name="needsProtection">If true, TASK_STATE_STARTED bit is turned on in
        /// an atomic fashion, making sure that TASK_STATE_CANCELED does not get set
        /// underneath us.  If false, TASK_STATE_STARTED bit is OR-ed right in.  This
        /// allows us to streamline things a bit for StartNew(), where competing cancellations
        /// are not a problem.</param>
        internal void ScheduleAndStart(bool needsProtection)
        {
            Contract.Assert(m_taskScheduler != null, "expected a task scheduler to have been selected");
            Contract.Assert((m_stateFlags & TASK_STATE_STARTED) == 0, "task has already started");

            // Set the TASK_STATE_STARTED bit
            if (needsProtection)
            {
                if (!MarkStarted())
                {
                    // A cancel has snuck in before we could get started.  Quietly exit.
                    return;
                }
            }
            else
            {
                m_stateFlags |= TASK_STATE_STARTED;
            }

            try
            {
                // Queue to the indicated scheduler.
                m_taskScheduler.QueueTask(this);
            }
            catch (Exception e)
            {
                if (ThreadingServices.IsThreadAbort(e))
                {
                    AddException(e);
                    FinishThreadAbortedTask(true, false);
                }
                else
                {
                    // The scheduler had a problem queueing this task.  Record the exception, leaving this task in
                    // a Faulted state.
                    TaskSchedulerException tse = new TaskSchedulerException(e);
                    AddException(tse);
                    Finish(false);

                    // Now we need to mark ourselves as "handled" to avoid crashing the finalizer thread if we are called from StartNew()
                    // or from the self replicating logic, because in both cases the exception is either propagated outside directly, or added
                    // to an enclosing parent. However we won't do this for continuation tasks, because in that case we internally eat the exception
                    // and therefore we need to make sure the user does later observe it explicitly or see it on the finalizer.

                    if ((Options & (TaskCreationOptions)InternalTaskOptions.ContinuationTask) == 0)
                    {
                        // m_contingentProperties.m_exceptionHolder *should* already exist after AddException()
                        Contract.Assert((m_contingentProperties != null) && (m_contingentProperties.m_exceptionsHolder != null),
                                "Task.InternalRunSynchronously(): Expected m_contingentProperties.m_exceptionsHolder to exist");

                        m_contingentProperties.m_exceptionsHolder.MarkAsHandled(false);
                    }

                    // re-throw the exception wrapped as a TaskSchedulerException.
                    throw tse;
                }
            }
        }


        /// <summary>
        /// Adds an exception to the list of exceptions this task has thrown.
        /// </summary>
        /// <param name="exceptionObject">An object representing either an Exception or a collection of Exceptions.</param>
        internal void AddException(object exceptionObject)
        {
            //
            // WARNING: A great deal of care went into insuring that
            // AddException() and GetExceptions() are never called
            // simultaneously.  See comment at start of GetExceptions().
            //

            // Lazily initialize the list, ensuring only one thread wins.
            LazyInitializer.EnsureInitialized<ContingentProperties>(ref m_contingentProperties, s_contingentPropertyCreator);
            if (m_contingentProperties.m_exceptionsHolder == null)
            {
                TaskExceptionHolder holder = new TaskExceptionHolder(this);
                if (Interlocked.CompareExchange(
                    ref m_contingentProperties.m_exceptionsHolder, holder, null) != null)
                {
                    // If we lost the race, suppress finalization.
                    holder.MarkAsHandled(false);
                }
            }

            // Figure this out before your enter the lock.
            Contract.Assert((exceptionObject is Exception) || (exceptionObject is IEnumerable<Exception>),
                "Task.AddException: Expected an Exception or an IEnumerable<Exception>");

            lock (m_contingentProperties)
            {
                m_contingentProperties.m_exceptionsHolder.Add(exceptionObject);
            }

        }


        /// <summary>
        /// Returns a list of exceptions by aggregating the holder's contents. Or null if
        /// no exceptions have been thrown.
        /// </summary>
        /// <param name="includeTaskCanceledExceptions">Whether to include a TCE if cancelled.</param>
        /// <returns>An aggregate exception, or null if no exceptions have been caught.</returns>
        private AggregateException GetExceptions(bool includeTaskCanceledExceptions)
        {
            //
            // WARNING: The Task/Task<TResult>/TaskCompletionSource classes
            // have all been carefully crafted to insure that GetExceptions()
            // is never called while AddException() is being called.  There
            // are locks taken on m_contingentProperties in several places:
            //
            // -- Task<TResult>.TrySetException(): The lock allows the
            //    task to be set to Faulted state, and all exceptions to
            //    be recorded, in one atomic action.  
            //
            // -- Task.Exception_get(): The lock ensures that Task<TResult>.TrySetException()
            //    is allowed to complete its operation before Task.Exception_get()
            //    can access GetExceptions().
            //
            // -- Task.ThrowIfExceptional(): The lock insures that Wait() will
            //    not attempt to call GetExceptions() while Task<TResult>.TrySetException()
            //    is in the process of calling AddException().
            //
            // For "regular" tasks, we effectively keep AddException() and GetException()
            // from being called concurrently by the way that the state flows.  Until
            // a Task is marked Faulted, Task.Exception_get() returns null.  And
            // a Task is not marked Faulted until it and all of its children have
            // completed, which means that all exceptions have been recorded.
            //
            // It might be a lot easier to follow all of this if we just required
            // that all calls to GetExceptions() and AddExceptions() were made
            // under a lock on m_contingentProperties.  But that would also
            // increase our lock occupancy time and the frequency with which we
            // would need to take the lock.
            //
            // If you add a call to GetExceptions() anywhere in the code,
            // please continue to maintain the invariant that it can't be
            // called when AddException() is being called.
            //

            // We'll lazily create a TCE if the task has been canceled.
            Exception canceledException = null;
            if (includeTaskCanceledExceptions && IsCanceled)
            {
                canceledException = new TaskCanceledException(this);
            }

            if (ExceptionRecorded)
            {
                // There are exceptions; get the aggregate and optionally add the canceled
                // exception to the aggregate (if applicable).
                Contract.Assert(m_contingentProperties != null); // ExceptionRecorded ==> m_contingentProperties != null

                // No need to lock around this, as other logic prevents the consumption of exceptions
                // before they have been completely processed.
                return m_contingentProperties.m_exceptionsHolder.CreateExceptionObject(false, canceledException);
            }
            else if (canceledException != null)
            {
                // No exceptions, but there was a cancelation. Aggregate and return it.
                return new AggregateException(canceledException);
            }

            return null;
        }

        /// <summary>
        /// Throws an aggregate exception if the task contains exceptions. 
        /// </summary>
        internal void ThrowIfExceptional(bool includeTaskCanceledExceptions)
        {
            Contract.Assert(IsCompleted, "ThrowIfExceptional(): Expected IsCompleted == true");

            Exception exception = GetExceptions(includeTaskCanceledExceptions);
            if (exception != null)
            {
                UpdateExceptionObservedStatus();
                throw exception;
            }
        }

        /// <summary>
        /// Checks whether this is an attached task, and whether we are being called by the parent task.
        /// And sets the TASK_STATE_EXCEPTIONOBSERVEDBYPARENT status flag based on that.
        /// 
        /// This is meant to be used internally when throwing an exception, and when WaitAll is gathering 
        /// exceptions for tasks it waited on. If this flag gets set, the implicit wait on children 
        /// will skip exceptions to prevent duplication.
        /// 
        /// This should only be called when this task has completed with an exception
        /// 
        /// </summary>
        internal void UpdateExceptionObservedStatus()
        {
            if ((Options & TaskCreationOptions.AttachedToParent) != 0 &&
                 Task.InternalCurrent == m_parent)
            {
                m_stateFlags |= TASK_STATE_EXCEPTIONOBSERVEDBYPARENT;
            }
        }

        /// <summary>
        /// Checks whether the TASK_STATE_EXCEPTIONOBSERVEDBYPARENT status flag is set,
        /// This will only be used by the implicit wait to prevent double throws
        /// 
        /// </summary>
        internal bool IsExceptionObservedByParent
        {
            get
            {
                return (m_stateFlags & TASK_STATE_EXCEPTIONOBSERVEDBYPARENT) != 0;
            }
        }

        /// <summary>
        /// Checks whether the body was ever invoked. Used by task scheduler code to verify custom schedulers actually ran the task.
        /// </summary>
        internal bool IsDelegateInvoked
        {
            get
            {
                return (m_stateFlags & TASK_STATE_DELEGATE_INVOKED) != 0;
            }
        }

        /// <summary>
        /// Signals completion of this particular task.
        ///
        /// The bUserDelegateExecuted parameter indicates whether this Finish() call comes following the
        /// full execution of the user delegate. 
        /// 
        /// If bUserDelegateExecuted is false, it mean user delegate wasn't invoked at all (either due to
        /// a cancellation request, or because this task is a promise style Task). In this case, the steps
        /// involving child tasks (i.e. WaitForChildren) will be skipped.
        /// 
        /// </summary>
        internal void Finish(bool bUserDelegateExecuted)
        {

            if (!bUserDelegateExecuted)
            {
                // delegate didn't execute => no children. We can safely call the remaining finish stages
                FinishStageTwo();
            }
            else
            {
                ContingentProperties properties = m_contingentProperties;

                if (properties == null ||                    // no contingent properties, no children. Safe to complete ourselves
                    (properties.m_completionCountdown == 1 && !IsSelfReplicatingRoot) ||
                    // Count of 1 => either all children finished, or there were none. Safe to complete ourselves 
                    // without paying the price of an Interlocked.Decrement.
                    // However we need to exclude self replicating root tasks from this optimization, because
                    // they can have children joining in, or finishing even after the root task delegate is done.
                    Interlocked.Decrement(ref properties.m_completionCountdown) == 0) // Reaching this sub clause means there may be remaining active children,
                // and we could be racing with one of them to call FinishStageTwo().
                // So whoever does the final Interlocked.Dec is responsible to finish.
                {

                    FinishStageTwo();
                }
                else
                {
                    // Apparently some children still remain. It will be up to the last one to process the completion of this task on their own thread.
                    // We will now yield the thread back to ThreadPool. Mark our state appropriately before getting out.

                    // We have to use an atomic update for this and make sure not to overwrite a final state, 
                    // because at this very moment the last child's thread may be concurrently completing us.
                    // Otherwise we risk overwriting the TASK_STATE_RAN_TO_COMPLETION, _CANCELED or _FAULTED bit which may have been set by that child task.
                    // Note that the concurrent update by the last child happening in FinishStageTwo could still wipe out the TASK_STATE_WAITING_ON_CHILDREN flag, 
                    // but it is not critical to maintain, therefore we dont' need to intruduce a full atomic update into FinishStageTwo

                    AtomicStateUpdate(TASK_STATE_WAITING_ON_CHILDREN, TASK_STATE_FAULTED | TASK_STATE_CANCELED | TASK_STATE_RAN_TO_COMPLETION);
                }
            }

            // Now is the time to prune exceptional children. We'll walk the list and removes the ones whose exceptions we might have observed after they threw.
            // we use a local variable for exceptional children here because some other thread may be nulling out m_contingentProperties.m_exceptionalChildren 
            List<Task> exceptionalChildren = m_contingentProperties != null ? m_contingentProperties.m_exceptionalChildren : null;

            if (exceptionalChildren != null)
            {
                lock (exceptionalChildren)
                {
                    // RemoveAll is not available in S3 - we have to use our own implementation
                    //exceptionalChildren.RemoveAll(s_IsExceptionObservedByParentPredicate); // RemoveAll has better performance than doing it ourselves

                    List<Task> exceptionalChildrenNext = new List<Task>();
                    foreach (var t in exceptionalChildren)
                    {
                        if (!s_IsExceptionObservedByParentPredicate(t)) exceptionalChildrenNext.Add(t);
                    }

                    exceptionalChildren.Clear();
                    exceptionalChildren.AddRange(exceptionalChildrenNext);
                }
            }
        }

        // statically allocated delegate for the removeall expression in Finish()
        private static Predicate<Task> s_IsExceptionObservedByParentPredicate = new Predicate<Task>((t) => { return t.IsExceptionObservedByParent; });

        /// <summary>
        /// FinishStageTwo is to be executed as soon as we known there are no more children to complete. 
        /// It can happen i) either on the thread that originally executed this task (if no children were spawned, or they all completed by the time this task's delegate quit)
        ///              ii) or on the thread that executed the last child.
        /// </summary>
        internal void FinishStageTwo()
        {
            AddExceptionsFromChildren();

            // At this point, the task is done executing and waiting for its children,
            // we can transition our task to a completion state.  
            int completionState;
            if (ExceptionRecorded)
            {
                completionState = TASK_STATE_FAULTED;
            }
            else if (IsCancellationRequested && IsCancellationAcknowledged)
            {
                // We transition into the TASK_STATE_CANCELED final state if the task's CT was signalled for cancellation, 
                // and the user delegate acknowledged the cancellation request by throwing an OCE, 
                // and the task hasn't otherwise transitioned into faulted state. (TASK_STATE_FAULTED trumps TASK_STATE_CANCELED)
                //
                // If the task threw an OCE without cancellation being requestsed (while the CT not being in signaled state),
                // then we regard it as a regular exception

                completionState = TASK_STATE_CANCELED;
            }
            else
            {
                completionState = TASK_STATE_RAN_TO_COMPLETION;
            }

            // Use Interlocked.Exchange() to effect a memory fence, preventing
            // any SetCompleted() (or later) instructions from sneak back before it.
            Interlocked.Exchange(ref m_stateFlags, m_stateFlags | completionState);

            // Set the completion event if it's been lazy allocated.
            SetCompleted();

            // if we made a cancellation registration, it's now unnecessary.
            DeregisterCancellationCallback();

            // ready to run continuations and notify parent.
            FinishStageThree();
        }


        /// <summary>
        /// Final stage of the task completion code path. Notifies the parent (if any) that another of its childre are done, and runs continuations.
        /// This function is only separated out from FinishStageTwo because these two operations are also needed to be called from CancellationCleanupLogic()
        /// </summary>
        private void FinishStageThree()
        {
            // Notify parent if this was an attached task
            if (m_parent != null
                && (((TaskCreationOptions)(m_stateFlags & OptionsMask)) & TaskCreationOptions.AttachedToParent) != 0)
            {
                m_parent.ProcessChildCompletion(this);
            }

            // Activate continuations (if any).
            FinishContinuations();

            // Need this to bound the memory usage on long/infinite continuation chains
            m_action = null;
        }

        /// <summary>
        /// This is called by children of this task when they are completed.
        /// </summary>
        internal void ProcessChildCompletion(Task childTask)
        {
            Contract.Assert(childTask.m_parent == this, "ProcessChildCompletion should only be called for a child of this task");
            Contract.Assert(childTask.IsCompleted, "ProcessChildCompletion was called for an uncompleted task");

            // if the child threw and we haven't observed it we need to save it for future reference
            if (childTask.IsFaulted && !childTask.IsExceptionObservedByParent)
            {
                // Lazily initialize the child exception list
                if (m_contingentProperties.m_exceptionalChildren == null)
                {
                    Interlocked.CompareExchange(ref m_contingentProperties.m_exceptionalChildren, new List<Task>(), null);
                }

                // In rare situations involving AppDomainUnload, it's possible (though unlikely) for FinishStageTwo() to be called
                // multiple times for the same task.  In that case, AddExceptionsFromChildren() could be nulling m_exceptionalChildren
                // out at the same time that we're processing it, resulting in a NullReferenceException here.  We'll protect
                // ourselves by caching m_exceptionChildren in a local variable.
                List<Task> tmp = m_contingentProperties.m_exceptionalChildren;
                if (tmp != null)
                {
                    lock (tmp)
                    {
                        tmp.Add(childTask);
                    }
                }

            }

            if (Interlocked.Decrement(ref m_contingentProperties.m_completionCountdown) == 0)
            {
                // This call came from the final child to complete, and apparently we have previously given up this task's right to complete itself.
                // So we need to invoke the final finish stage.

                FinishStageTwo();
            }
        }

        /// <summary>
        /// This is to be called just before the task does its final state transition. 
        /// It traverses the list of exceptional children, and appends their aggregate exceptions into this one's exception list
        /// </summary>
        internal void AddExceptionsFromChildren()
        {
            // In rare occurences during AppDomainUnload() processing, it is possible for this method to be called
            // simultaneously on the same task from two different contexts.  This can result in m_exceptionalChildren
            // being nulled out while it is being processed, which could lead to a NullReferenceException.  To
            // protect ourselves, we'll cache m_exceptionalChildren in a local variable.
            List<Task> tmp = (m_contingentProperties != null) ? m_contingentProperties.m_exceptionalChildren : null;

            if (tmp != null)
            {
                // This lock is necessary because even though AddExceptionsFromChildren is last to execute, it may still 
                // be racing with the code segment at the bottom of Finish() that prunes the exceptional child array. 
                lock (tmp)
                {
                    foreach (Task task in tmp)
                    {
                        // Ensure any exceptions thrown by children are added to the parent.
                        // In doing this, we are implicitly marking children as being "handled".
                        Contract.Assert(task.IsCompleted, "Expected all tasks in list to be completed");
                        if (task.IsFaulted && !task.IsExceptionObservedByParent)
                        {
                            TaskExceptionHolder exceptionHolder = task.m_contingentProperties.m_exceptionsHolder;
                            Contract.Assert(exceptionHolder != null);

                            // No locking necessary since child task is finished adding exceptions
                            // and concurrent CreateExceptionObject() calls do not constitute
                            // a concurrency hazard.
                            AddException(exceptionHolder.CreateExceptionObject(false, null));
                        }
                    }
                }

                // Reduce memory pressure by getting rid of the array
                m_contingentProperties.m_exceptionalChildren = null;
            }
        }

        /// <summary>
        /// Special purpose Finish() entry point to be used when the task delegate throws a ThreadAbortedException
        /// This makes a note in the state flags so that we avoid any costly synchronous operations in the finish codepath
        /// such as inlined continuations
        /// </summary>
        /// <param name="bTAEAddedToExceptionHolder">
        /// Indicates whether the ThreadAbortException was added to this task's exception holder. 
        /// This should always be true except for the case of non-root self replicating task copies.
        /// </param>
        /// <param name="delegateRan">Whether the delegate was executed.</param>
        internal void FinishThreadAbortedTask(bool bTAEAddedToExceptionHolder, bool delegateRan)
        {
            Contract.Assert(!bTAEAddedToExceptionHolder || (m_contingentProperties != null && m_contingentProperties.m_exceptionsHolder != null),
                            "FinishThreadAbortedTask() called on a task whose exception holder wasn't initialized");

            // this will only be false for non-root self replicating task copies, because all of their exceptions go to the root task.
            if (bTAEAddedToExceptionHolder)
                m_contingentProperties.m_exceptionsHolder.MarkAsHandled(false);

            // If this method has already been called for this task, or if this task has already completed, then
            // return before actually calling Finish().
            if (!AtomicStateUpdate(TASK_STATE_THREAD_WAS_ABORTED,
                            TASK_STATE_THREAD_WAS_ABORTED | TASK_STATE_RAN_TO_COMPLETION | TASK_STATE_FAULTED | TASK_STATE_CANCELED))
            {
                return;
            }

            Finish(delegateRan);

        }


        /// <summary>
        /// Executes the task. This method will only be called once, and handles bookeeping associated with
        /// self-replicating tasks, in addition to performing necessary exception marshaling.
        /// </summary>
        /// <exception cref="T:System.ObjectDisposedException">The task has already been disposed.</exception>
        private void Execute()
        {
            if (IsSelfReplicatingRoot)
            {
                ExecuteSelfReplicating(this);
            }
            else
            {
                try
                {
                    InnerInvoke();
                }

                catch (Exception exn)
                {
                    if (ThreadingServices.IsThreadAbort(exn))
                    {
                        // Don't record the TAE or call FinishThreadAbortedTask for a child replica task --
                        // it's already been done downstream.
                        if (!IsChildReplica)
                        {
                            // Record this exception in the task's exception list
                            HandleException(exn);

                            // This is a ThreadAbortException and it will be rethrown from this catch clause, causing us to 
                            // skip the regular Finish codepath. In order not to leave the task unfinished, we now call 
                            // FinishThreadAbortedTask here.
                            FinishThreadAbortedTask(true, true);
                        }
                    }
                    else
                    {
                        // Record this exception in the task's exception list
                        HandleException(exn);
                    }
                }
            }
        }

        // Allows (internal) deriving classes to support limited replication.
        // (By default, replication is basically unlimited).
        internal virtual bool ShouldReplicate()
        {
            return true;
        }

        // Allows (internal) deriving classes to instantiate the task replica as a Task super class of their choice
        // (By default, we create a regular Task instance)
        internal virtual Task CreateReplicaTask(Action<object> taskReplicaDelegate, Object stateObject, Task parentTask, TaskScheduler taskScheduler,
                                            TaskCreationOptions creationOptionsForReplica, InternalTaskOptions internalOptionsForReplica)
        {
            return new Task(taskReplicaDelegate, stateObject, parentTask, CancellationToken.None,
                            creationOptionsForReplica, internalOptionsForReplica, parentTask.ExecutingTaskScheduler);
        }

        // Allows internal deriving classes to support replicas that exit prematurely and want to pass on state to the next replica
        internal virtual Object SavedStateForNextReplica
        {
            get { return null; }

            set { /*do nothing*/ }
        }

        // Allows internal deriving classes to support replicas that exit prematurely and want to pass on state to the next replica
        internal virtual Object SavedStateFromPreviousReplica
        {
            get { return null; }

            set { /*do nothing*/ }
        }

        // Allows internal deriving classes to support replicas that exit prematurely and want to hand over the child replica that they
        // had queued, so that the replacement replica can work with that child task instead of queuing up yet another one
        internal virtual Task HandedOverChildReplica
        {
            get { return null; }

            set { /* do nothing*/ }
        }

        private static void ExecuteSelfReplicating(Task root)
        {
            TaskCreationOptions creationOptionsForReplicas = root.CreationOptions | TaskCreationOptions.AttachedToParent;
            InternalTaskOptions internalOptionsForReplicas =
                InternalTaskOptions.ChildReplica |  // child replica flag disables self replication for the replicas themselves.
                InternalTaskOptions.SelfReplicating |  // we still want to identify this as part of a self replicating group
                InternalTaskOptions.QueuedByRuntime;   // we queue and cancel these tasks internally, so don't allow CT registration to take place


            // Important Note: The child replicas we launch from here will be attached the root replica (by virtue of the root.CreateReplicaTask call)
            // because we need the root task to receive all their exceptions, and to block until all of them return


            // This variable is captured in a closure and shared among all replicas.
            bool replicasAreQuitting = false;

            // Set up a delegate that will form the body of the root and all recursively created replicas.
            Action<object> taskReplicaDelegate = null;
            taskReplicaDelegate = delegate
            {
                Task currentTask = Task.InternalCurrent;


                // Check if a child task has been handed over by a prematurely quiting replica that we might be a replacement for.
                Task childTask = currentTask.HandedOverChildReplica;

                if (childTask == null)
                {
                    // Apparently we are not a replacement task. This means we need to queue up a child task for replication to progress

                    // Down-counts a counter in the root task.
                    if (!root.ShouldReplicate()) return;

                    // If any of the replicas have quit, we will do so ourselves.
                    if (replicasAreQuitting)
                    {
                        return;
                    }

                    // Propagate a copy of the context from the root task. It may be null if flow was suppressed.
                    ExecutionContextLightup creatorContext = root.m_capturedContext;


                    childTask = root.CreateReplicaTask(taskReplicaDelegate, root.m_stateObject, root, root.ExecutingTaskScheduler,
                                                       creationOptionsForReplicas, internalOptionsForReplicas);

                    childTask.m_capturedContext = (creatorContext == null ? null : creatorContext.CreateCopy());

                    childTask.ScheduleAndStart(false);
                }



                // Finally invoke the meat of the task.
                // Note that we are directly calling root.InnerInvoke() even though we are currently be in the action delegate of a child replica 
                // This is because the actual work was passed down in that delegate, and the action delegate of the child replica simply contains this
                // replication control logic.
                try
                {
                    // passing in currentTask only so that the parallel debugger can find it
                    root.InnerInvokeWithArg(currentTask);
                }
                catch (Exception exn)
                {
                    // Record this exception in the root task's exception list
                    root.HandleException(exn);

                    if (ThreadingServices.IsThreadAbort(exn))
                    {
                        // If this is a ThreadAbortException it will escape this catch clause, causing us to skip the regular Finish codepath
                        // In order not to leave the task unfinished, we now call FinishThreadAbortedTask here
                        currentTask.FinishThreadAbortedTask(false, true);
                    }
                }

                Object savedState = currentTask.SavedStateForNextReplica;

                // check for premature exit
                if (savedState != null)
                {
                    // the replica decided to exit early
                    // we need to queue up a replacement, attach the saved state, and yield the thread right away

                    Task replacementReplica = root.CreateReplicaTask(taskReplicaDelegate, root.m_stateObject, root, root.ExecutingTaskScheduler,
                                                                    creationOptionsForReplicas, internalOptionsForReplicas);

                    // Propagate a copy of the context from the root task to the replacement task
                    ExecutionContextLightup creatorContext = root.m_capturedContext;
                    replacementReplica.m_capturedContext = (creatorContext == null ? null : creatorContext.CreateCopy());

                    replacementReplica.HandedOverChildReplica = childTask;
                    replacementReplica.SavedStateFromPreviousReplica = savedState;

                    replacementReplica.ScheduleAndStart(false);
                }
                else
                {
                    // The replica finished normally, which means it can't find more work to grab. 
                    // Time to mark replicas quitting

                    replicasAreQuitting = true;

                    // InternalCancel() could conceivably throw in the underlying scheduler's TryDequeue() method.
                    // If it does, then make sure that we record it.
                    try
                    {
                        childTask.InternalCancel(true);
                    }
                    catch (Exception e)
                    {
                        // Apparently TryDequeue threw an exception.  Before propagating that exception, InternalCancel should have
                        // attempted an atomic state transition and a call to CancellationCleanupLogic() on this task. So we know
                        // the task was properly cleaned up if it was possible. 
                        //
                        // Now all we need to do is to Record the exception in the root task.

                        root.HandleException(e);
                    }

                    // No specific action needed if the child could not be canceled
                    // because we attached it to the root task, which should therefore be receiving any exceptions from the child,
                    // and root.wait will not return before this child finishes anyway.

                }
            };

            //
            // Now we execute as the root task
            //
            taskReplicaDelegate(null);
        }

        /// <summary>
        /// IThreadPoolWorkItem override, which is the entry function for this task when the TP scheduler decides to run it.
        /// 
        /// </summary>
        // [SecurityCritical]
        void IThreadPoolWorkItem.ExecuteWorkItem()
        {
            ExecuteEntry(false);
        }

        ///// <summary>
        ///// The ThreadPool calls this if a ThreadAbortException is thrown while trying to execute this workitem.  This may occur
        ///// before Task would otherwise be able to observe it.  
        ///// </summary>
        //[SecurityCritical]
        //void IThreadPoolWorkItem.MarkAborted(ThreadAbortException tae)
        //{
        //    // If the task has marked itself as Completed, then it either a) already observed this exception (so we shouldn't handle it here)
        //    // or b) completed before the exception ocurred (in which case it shouldn't count against this Task).
        //    if (!IsCompleted)
        //    {
        //        HandleException(tae);
        //        FinishThreadAbortedTask(true, false);
        //    }
        //}

        /// <summary>
        /// Outermost entry function to execute this task. Handles all aspects of executing a task on the caller thread.
        /// Currently this is called by IThreadPoolWorkItem.ExecuteWorkItem(), and TaskManager.TryExecuteInline. 
        /// 
        /// </summary>
        /// <param name="bPreventDoubleExecution"> Performs atomic updates to prevent double execution. Should only be set to true
        /// in codepaths servicing user provided TaskSchedulers. The ConcRT or ThreadPool schedulers don't need this. </param>
        // // [SecuritySafeCritical]
        internal bool ExecuteEntry(bool bPreventDoubleExecution)
        {
            if (bPreventDoubleExecution || ((Options & (TaskCreationOptions)InternalTaskOptions.SelfReplicating) != 0))
            {
                int previousState = 0;

                // Do atomic state transition from queued to invoked. If we observe a task that's already invoked,
                // we will return false so that TaskScheduler.ExecuteTask can throw an exception back to the custom scheduler.
                // However we don't want this exception to be throw if the task was already canceled, because it's a
                // legitimate scenario for custom schedulers to dequeue a task and mark it as canceled (example: throttling scheduler)
                if (!AtomicStateUpdate(TASK_STATE_DELEGATE_INVOKED, TASK_STATE_DELEGATE_INVOKED, ref previousState)
                    && (previousState & TASK_STATE_CANCELED) == 0)
                {
                    // This task has already been invoked.  Don't invoke it again.
                    return false;
                }
            }
            else
            {
                // Remember that we started running the task delegate.
                m_stateFlags |= TASK_STATE_DELEGATE_INVOKED;
            }

            if (!IsCancellationRequested && !IsCanceled)
            {
                ExecuteWithThreadLocal();
            }
            else if (!IsCanceled)
            {
                int prevState = Interlocked.Exchange(ref m_stateFlags, m_stateFlags | TASK_STATE_CANCELED);
                if ((prevState & TASK_STATE_CANCELED) == 0)
                {
                    CancellationCleanupLogic();
                }
            }

            return true;
        }

        // A trick so we can refer to the TLS slot with a byref.
        // [SecurityCritical]
        private void ExecuteWithThreadLocal()
        {
            // Remember the current task so we can restore it after running, and then
            Task previousTask = ThreadLocals.s_currentTask.Value;

#if ETW_EVENTING    // PAL doesn't support  eventing
            // ETW event for Task Started
            if (TplEtwProvider.Log.IsEnabled(EventLevel.Verbose, ((EventKeywords)(-1))))
            {
                // previousTask holds the actual "current task" we want to report in the event
                if (previousTask != null)
                    TplEtwProvider.Log.TaskStarted(previousTask.m_taskScheduler.Id, previousTask.Id, this.Id);
                else
                    TplEtwProvider.Log.TaskStarted(TaskScheduler.Current.Id, 0, this.Id);
            }
#endif

            try
            {
                // place the current task into TLS.
                ThreadLocals.s_currentTask.Value = this;

                if (m_capturedContext == null)
                {
                    // No context, just run the task directly.
                    Execute();
                }
                else
                {
                    if (IsSelfReplicatingRoot || IsChildReplica)
                    {
                        m_capturedContext = m_capturedContext.CreateCopy();
                    }

                    // Run the task.  We need a simple shim that converts the
                    // object back into a Task object, so that we can Execute it.
//#if PFX_LEGACY_3_5
//                    s_ecCallback(this);
//#else
                    ExecutionContextLightup.Instance.Run(m_capturedContext, s_ecCallback, this);
//#endif
                }

                Finish(true);
            }
            finally
            {
                ThreadLocals.s_currentTask.Value = previousTask;
            }

#if ETW_EVENTING    // PAL doesn't support  eventing
            // ETW event for Task Completed
            if (TplEtwProvider.Log.IsEnabled(EventLevel.Verbose, ((EventKeywords)(-1))))
            {
                // previousTask holds the actual "current task" we want to report in the event
                if (previousTask != null)
                    TplEtwProvider.Log.TaskCompleted(previousTask.m_taskScheduler.Id, previousTask.Id, this.Id, IsFaulted);
                else
                    TplEtwProvider.Log.TaskCompleted(TaskScheduler.Current.Id, 0, this.Id, IsFaulted);
            }
#endif
        }

        // [SecurityCritical]
        private static Action<object> s_ecCallback;

        // [SecurityCritical]
        private static void ExecutionContextCallback(object obj)
        {
            Task task = obj as Task;
            Contract.Assert(task != null, "expected a task object");

            task.Execute();

        }


        /// <summary>
        /// The actual code which invokes the body of the task. This can be overriden in derived types.
        /// </summary>
        internal void InnerInvoke()
        {
            Contract.Assert(m_action != null, "Null action in InnerInvoke()");

            Action funcA = m_action as Action;
            if (funcA != null)
            {
                funcA();
            }
            else
            {
                Action<object> funcB = m_action as Action<object>;
                funcB(m_stateObject);
            }
        }

        /// <summary>
        /// Alternate InnerInvoke prototype to be called from ExecuteSelfReplicating() so that
        /// the Parallel Debugger can discover the actual task being invoked. 
        /// Details: Here, InnerInvoke is actually being called on the rootTask object while we are actually executing the
        /// childTask. And the debugger needs to discover the childTask, so we pass that down as an argument.
        /// The NoOptimization and NoInlining flags ensure that the childTask pointer is retained, and that this
        /// function appears on the callstack.
        /// </summary>
        /// <param name="childTask"></param>
        [MethodImpl(MethodImplOptions.NoOptimization | MethodImplOptions.NoInlining)]
        internal void InnerInvokeWithArg(Task childTask)
        {
            InnerInvoke();
        }

        /// <summary>
        /// Performs whatever handling is necessary for an unhandled exception. Normally
        /// this just entails adding the exception to the holder object. 
        /// </summary>
        /// <param name="unhandledException">The exception that went unhandled.</param>
        private void HandleException(Exception unhandledException)
        {
            Contract.Assert(unhandledException != null);

            OperationCanceledException exceptionAsOce = unhandledException as OperationCanceledException;
            if (exceptionAsOce != null && IsCancellationRequested &&
                m_contingentProperties.m_cancellationToken == exceptionAsOce.CancellationToken)
            {
                // All conditions are satisfied for us to go into canceled state in Finish().
                // Mark the acknowledgement, and return without adding the exception.
                //
                // However any OCE from the task that doesn't match the tasks' own CT, 
                // or that gets thrown without the CT being set will be treated as an ordinary exception and added to the aggreagate

                SetCancellationAcknowledged();
            }
            else
            {

                AddException(unhandledException);
            }
        }

        /// <summary>
        /// Waits for the <see cref="Task"/> to complete execution.
        /// </summary>
        /// <exception cref="T:System.AggregateException">
        /// The <see cref="Task"/> was canceled -or- an exception was thrown during
        /// the execution of the <see cref="Task"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        public void Wait()
        {
#if DEBUG
            bool waitResult =
#endif
 Wait(Timeout.Infinite, CancellationToken.None);

#if DEBUG
            Contract.Assert(waitResult, "expected wait to succeed");
#endif
        }

        /// <summary>
        /// Waits for the <see cref="Task"/> to complete execution.
        /// </summary>
        /// <param name="timeout">
        /// A <see cref="System.TimeSpan"/> that represents the number of milliseconds to wait, or a <see
        /// cref="System.TimeSpan"/> that represents -1 milliseconds to wait indefinitely.
        /// </param>
        /// <returns>
        /// true if the <see cref="Task"/> completed execution within the allotted time; otherwise, false.
        /// </returns>
        /// <exception cref="T:System.AggregateException">
        /// The <see cref="Task"/> was canceled -or- an exception was thrown during the execution of the <see
        /// cref="Task"/>.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="timeout"/> is a negative number other than -1 milliseconds, which represents an
        /// infinite time-out -or- timeout is greater than
        /// <see cref="System.Int32.MaxValue"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        public bool Wait(TimeSpan timeout)
        {
            long totalMilliseconds = (long)timeout.TotalMilliseconds;
            if (totalMilliseconds < -1 || totalMilliseconds > 0x7fffffff)
            {
                throw new ArgumentOutOfRangeException("timeout");
            }

            return Wait((int)totalMilliseconds, CancellationToken.None);
        }


        /// <summary>
        /// Waits for the <see cref="Task"/> to complete execution.
        /// </summary>
        /// <param name="cancellationToken">
        /// A <see cref="CancellationToken"/> to observe while waiting for the task to complete.
        /// </param>
        /// <exception cref="T:System.OperationCanceledException2">
        /// The <paramref name="cancellationToken"/> was canceled.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// The <see cref="Task"/> was canceled -or- an exception was thrown during the execution of the <see
        /// cref="Task"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/>
        /// has been disposed.
        /// </exception>
        public void Wait(CancellationToken cancellationToken)
        {
            Wait(Timeout.Infinite, cancellationToken);
        }


        /// <summary>
        /// Waits for the <see cref="Task"/> to complete execution.
        /// </summary>
        /// <param name="millisecondsTimeout">
        /// The number of milliseconds to wait, or <see cref="System.Threading.Timeout.Infinite"/> (-1) to
        /// wait indefinitely.</param>
        /// <returns>true if the <see cref="Task"/> completed execution within the allotted time; otherwise,
        /// false.
        /// </returns>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="millisecondsTimeout"/> is a negative number other than -1, which represents an
        /// infinite time-out.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// The <see cref="Task"/> was canceled -or- an exception was thrown during the execution of the <see
        /// cref="Task"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/>
        /// has been disposed.
        /// </exception>
        public bool Wait(int millisecondsTimeout)
        {
            return Wait(millisecondsTimeout, CancellationToken.None);
        }


        /// <summary>
        /// Waits for the <see cref="Task"/> to complete execution.
        /// </summary>
        /// <param name="millisecondsTimeout">
        /// The number of milliseconds to wait, or <see cref="System.Threading.Timeout.Infinite"/> (-1) to
        /// wait indefinitely.
        /// </param>
        /// <param name="cancellationToken">
        /// A <see cref="CancellationToken"/> to observe while waiting for the task to complete.
        /// </param>
        /// <returns>
        /// true if the <see cref="Task"/> completed execution within the allotted time; otherwise, false.
        /// </returns>
        /// <exception cref="T:System.AggregateException">
        /// The <see cref="Task"/> was canceled -or- an exception was thrown during the execution of the <see
        /// cref="Task"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/>
        /// has been disposed.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="millisecondsTimeout"/> is a negative number other than -1, which represents an
        /// infinite time-out.
        /// </exception>
        /// <exception cref="T:System.OperationCanceledException2">
        /// The <paramref name="cancellationToken"/> was canceled.
        /// </exception>
        public bool Wait(int millisecondsTimeout, CancellationToken cancellationToken)
        {
            //ThrowIfDisposed();

            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            // Return immediately if we know that we've completed "clean" -- no exceptions, no cancellations
            if (CompletedSuccessfully) return true;

            if (!InternalWait(millisecondsTimeout, cancellationToken))
                return false;

            // If an exception occurred, or the task was cancelled, throw an exception.
            ThrowIfExceptional(true);

            Contract.Assert((m_stateFlags & TASK_STATE_FAULTED) == 0, "Task.Wait() completing when in Faulted state.");

            return true;
        }

        // Convenience method that wraps any scheduler exception in a TaskSchedulerException
        // and rethrows it.
        private bool WrappedTryRunInline()
        {
            if (m_taskScheduler == null)
                return false;

            try
            {
                return m_taskScheduler.TryRunInline(this, true);
            }
            catch (Exception e)
            {
                // we 1) either received an unexpected exception originating from a custom scheduler, which needs to be wrapped in a TSE and thrown
                //    2) or a a ThreadAbortException, which we need to skip here, because it would already have been handled in Task.Execute
                if (!(ThreadingServices.IsThreadAbort(e)))
                {
                    TaskSchedulerException tse = new TaskSchedulerException(e);
                    throw tse;
                }
                else
                {
                    throw;
                }
            }
        }

        // This overload takes advantage of known values for current scheduler & statics.
        // It looks a LOT like the version above, but perf considerations prevented me
        // from having the one above call this one.
        private bool WrappedTryRunInline(TaskScheduler currentScheduler, object currentSchedulerStatics)
        {
            if (m_taskScheduler == null)
                return false;

            try
            {
                if (currentScheduler == m_taskScheduler)
                {
                    return currentScheduler.TryRunInline(this, true, currentSchedulerStatics);
                }
                else
                {
                    return m_taskScheduler.TryRunInline(this, true);
                }
            }
            catch (Exception e)
            {
                // we 1) either received an unexpected exception originating from a custom scheduler, which needs to be wrapped in a TSE and thrown
                //    2) or a a ThreadAbortException, which we need to skip here, because it would already have been handled in Task.Execute
                if (!(ThreadingServices.IsThreadAbort(e)))
                {
                    TaskSchedulerException tse = new TaskSchedulerException(e);
                    throw tse;
                }
                else
                {
                    throw;
                }
            }
        }

        /// <summary>
        /// The core wait function, which is only accesible internally. It's meant to be used in places in TPL code where 
        /// the current context is known or cached.
        /// </summary>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        internal bool InternalWait(int millisecondsTimeout, CancellationToken cancellationToken)
        {
#if ETW_EVENTING    // PAL doesn't support  eventing
            // ETW event for Task Wait Begin
            if (TplEtwProvider.Log.IsEnabled(EventLevel.Verbose, ((EventKeywords)(-1))))
            {
                Task currentTask = Task.InternalCurrent;
                TplEtwProvider.Log.TaskWaitBegin((currentTask != null ? currentTask.m_taskScheduler.Id : TaskScheduler.Current.Id), (currentTask != null ? currentTask.Id : 0),
                                                  this.Id);
            }
#endif

            bool returnValue = IsCompleted;

            // If the event hasn't already been set, we will wait.
            if (!returnValue)
            {
                //
                // we will attempt inline execution only if an infinite wait was requested
                // Inline execution doesn't make sense for finite timeouts and if a cancellation token was specified
                // because we don't know how long the task delegate will take.
                //
                TaskScheduler tm = m_taskScheduler;
                if (millisecondsTimeout == Timeout.Infinite && !cancellationToken.CanBeCanceled &&
                    WrappedTryRunInline() && IsCompleted) // TryRunInline doesn't guarantee completion, as there may be unfinished children.
                {
                    returnValue = true;
                }
                else
                {
                    returnValue = CompletedEvent.Wait(millisecondsTimeout, cancellationToken);
                }
            }

            Contract.Assert(IsCompleted || millisecondsTimeout != Timeout.Infinite);

#if ETW_EVENTING    // PAL doesn't support  eventing
            // ETW event for Task Wait End
            if (TplEtwProvider.Log.IsEnabled(EventLevel.Verbose, ((EventKeywords)(-1))))
            {
                Task currentTask = Task.InternalCurrent;
                TplEtwProvider.Log.TaskWaitEnd((currentTask != null ? currentTask.m_taskScheduler.Id : TaskScheduler.Current.Id), (currentTask != null ? currentTask.Id : 0),
                                                  this.Id);
            }
#endif

            return returnValue;
        }


        /// <summary>
        /// Cancels the <see cref="Task"/>.
        /// </summary>
        /// <param name="bCancelNonExecutingOnly"> Indiactes whether we should only cancel non-invoked tasks.
        /// For the default scheduler this option will only be serviced through TryDequeue.
        /// For custom schedulers we also attempt an atomic state transition.</param>
        /// <returns>true if the task was successfully canceled; otherwise, false.</returns>
        /// <exception cref="T:System.ObjectDisposedException">The <see cref="Task"/>
        /// has been disposed.</exception>
        internal bool InternalCancel(bool bCancelNonExecutingOnly)
        {
            Contract.Assert((Options & (TaskCreationOptions)InternalTaskOptions.PromiseTask) == 0, "Task.InternalCancel() did not expect promise-style task");
            //ThrowIfDisposed();

            bool bPopSucceeded = false;
            bool mustCleanup = false;

            TaskSchedulerException tse = null;

            // If started, and running in a task context, we can try to pop the chore.
            if ((m_stateFlags & TASK_STATE_STARTED) != 0)
            {
                TaskScheduler ts = m_taskScheduler;

                try
                {
                    bPopSucceeded = (ts != null) && ts.TryDequeue(this);
                }
                catch (Exception e)
                {
                    // TryDequeue threw. We don't know whether the task was properly dequeued or not. So we must let the rest of 
                    // the cancellation logic run its course (record the request, attempt atomic state transition and do cleanup where appropriate)
                    // Here we will only record a TaskSchedulerException, which will later be thrown at function exit.

                    if (!(ThreadingServices.IsThreadAbort(e)))
                    {
                        tse = new TaskSchedulerException(e);
                    }
                }

                bool bRequiresAtomicStartTransition = (ts != null && ts.RequiresAtomicStartTransition) || ((Options & (TaskCreationOptions)InternalTaskOptions.SelfReplicating) != 0);

                if (!bPopSucceeded && bCancelNonExecutingOnly && bRequiresAtomicStartTransition)
                {
                    // The caller requested cancellation of non-invoked tasks only, and TryDequeue was one way of doing it...
                    // Since that seems to have failed, we should now try an atomic state transition (from non-invoked state to canceled)
                    // An atomic transition here is only safe if we know we're on a custom task scheduler, which also forces a CAS on ExecuteEntry

                    // Even though this task can't have any children, we should be ready for handling any continuations that 
                    // may be attached to it (although currently 
                    // So we need to remeber whether we actually did the flip, so we can do clean up (finish continuations etc)
                    mustCleanup = AtomicStateUpdate(TASK_STATE_CANCELED, TASK_STATE_DELEGATE_INVOKED | TASK_STATE_CANCELED);


                    // PS: This is slightly different from the regular cancellation codepath 
                    // since we record the cancellation request *after* doing the state transition. 
                    // However that shouldn't matter too much because the task was never invoked, thus can't have children
                }

            }

            if (!bCancelNonExecutingOnly || bPopSucceeded || mustCleanup)
            {
                // Record the cancellation request.
                RecordInternalCancellationRequest();

                // Determine whether we need to clean up
                // This will be the case 
                //     1) if we were able to pop, and we win the race to update task state to TASK_STATE_CANCELED
                //     2) if the task seems to be yet unstarted, and we win the race to transition to
                //        TASK_STATE_CANCELED before anyone else can transition into _STARTED or _CANCELED or 
                //        _RAN_TO_COMPLETION or _FAULTED
                // Note that we do not check for TASK_STATE_COMPLETION_RESERVED.  That only applies to promise-style
                // tasks, and a promise-style task should not enter into this codepath.
                if (bPopSucceeded)
                {
                    // hitting this would mean something wrong with the AtomicStateUpdate above
                    Contract.Assert(!mustCleanup, "Possibly an invalid state transition call was made in InternalCancel()");

                    // Include TASK_STATE_DELEGATE_INVOKED in "illegal" bits to protect against the situation where
                    // TS.TryDequeue() returns true but the task is still left on the queue.
                    mustCleanup = AtomicStateUpdate(TASK_STATE_CANCELED, TASK_STATE_CANCELED | TASK_STATE_DELEGATE_INVOKED);
                }
                else if (!mustCleanup && (m_stateFlags & TASK_STATE_STARTED) == 0)
                {
                    mustCleanup = AtomicStateUpdate(TASK_STATE_CANCELED,
                        TASK_STATE_CANCELED | TASK_STATE_STARTED | TASK_STATE_RAN_TO_COMPLETION |
                        TASK_STATE_FAULTED | TASK_STATE_DELEGATE_INVOKED);
                }

                // do the cleanup (i.e. set completion event and finish continuations)
                if (mustCleanup)
                {
                    CancellationCleanupLogic();
                }
            }

            if (tse != null)
                throw tse;
            else
                return (mustCleanup);
        }

        // Breaks out logic for recording a cancellation request
        internal void RecordInternalCancellationRequest()
        {
            // Record the cancellation request.
            LazyInitializer.EnsureInitialized<ContingentProperties>(ref m_contingentProperties, s_contingentPropertyCreator);

            m_contingentProperties.m_internalCancellationRequested = CANCELLATION_REQUESTED;

        }

        // ASSUMES THAT A SUCCESSFUL CANCELLATION HAS JUST OCCURRED ON THIS TASK!!!
        // And this method should be called at most once per task.
        internal void CancellationCleanupLogic()
        {
            Contract.Assert((m_stateFlags & (TASK_STATE_CANCELED | TASK_STATE_COMPLETION_RESERVED)) != 0, "Task.CancellationCleanupLogic(): Task not canceled or reserved.");
            // I'd like to do this, but there is a small window for a race condition.  If someone calls Wait() between InternalCancel() and
            // here, that will set m_completionEvent, leading to a meaningless/harmless assertion.
            //Contract.Assert((m_completionEvent == null) || !m_completionEvent.IsSet, "Task.CancellationCleanupLogic(): Completion event already set.");

            // This may have been set already, but we need to make sure.
            Interlocked.Exchange(ref m_stateFlags, m_stateFlags | TASK_STATE_CANCELED);

            // Fire completion event if it has been lazily initialized
            SetCompleted();

            // Notify parents, fire continuations, other cleanup.
            FinishStageThree();
        }


        /// <summary>
        /// Sets the task's cancellation acknowledged flag.
        /// </summary>    
        private void SetCancellationAcknowledged()
        {
            Contract.Assert(this == Task.InternalCurrent, "SetCancellationAcknowledged() should only be called while this is still the current task");
            Contract.Assert(IsCancellationRequested, "SetCancellationAcknowledged() should not be called if the task's CT wasn't signaled");

            m_stateFlags |= TASK_STATE_CANCELLATIONACKNOWLEDGED;
        }


        //
        // Continuation passing functionality (aka ContinueWith)
        //

        /// <summary>
        /// A structure to hold continuation information.
        /// </summary>
        internal struct TaskContinuation
        {
            internal object m_task; // The delegate OR unstarted continuation task.
            internal TaskScheduler m_taskScheduler; // The TaskScheduler with which to associate the continuation task.
            internal TaskContinuationOptions m_options; // What kind of continuation.

            /// <summary>
            /// Constructs a new continuation structure.
            /// </summary>
            /// <param name="task">The task to be activated.</param>
            /// <param name="options">The continuation options.</param>
            /// <param name="scheduler">The scheduler to use for the continuation.</param>
            internal TaskContinuation(Task task, TaskScheduler scheduler, TaskContinuationOptions options)
            {
                Contract.Assert(task != null, "TaskContinuation ctor: task is null");

                m_task = task;
                m_taskScheduler = scheduler;
                m_options = options;
            }

            internal TaskContinuation(Action<Task> action)
            {
                m_task = action;
                m_taskScheduler = null;
                m_options = TaskContinuationOptions.None;
            }

            /// <summary>
            /// Invokes the continuation for the target completion task.
            /// </summary>
            /// <param name="completedTask">The completed task.</param>
            /// <param name="bCanInlineContinuationTask">Whether the continuation can be inlined.</param>
            internal void Run(Task completedTask, bool bCanInlineContinuationTask)
            {
                Contract.Assert(completedTask.IsCompleted, "ContinuationTask.Run(): completedTask not completed");

                Task task = m_task as Task;
                if (task != null)
                {
                    if (completedTask.ContinueWithIsRightKind(m_options))
                    {
                        task.m_taskScheduler = m_taskScheduler;

                        // Either run directly or just queue it up for execution, depending
                        // on whether synchronous or asynchronous execution is wanted.
                        if (bCanInlineContinuationTask && (m_options & TaskContinuationOptions.ExecuteSynchronously) != 0)
                        {
                            // Execute() won't set the TASK_STATE_STARTED flag, so we'll do it here.
                            if (!task.MarkStarted())
                            {
                                // task has been canceled.  Abort this continuation thread.
                                return;
                            }

                            try
                            {
                                if (!m_taskScheduler.TryRunInline(task, false))
                                {
                                    m_taskScheduler.QueueTask(task);
                                }
                            }
                            catch (Exception e)
                            {
                                // Either TryRunInline() or QueueTask() threw an exception. Record the exception, marking the task as Faulted.
                                // However if it was a ThreadAbortException coming from TryRunInline we need to skip here, 
                                // because it would already have been handled in Task.Execute()
                                if (!(ThreadingServices.IsThreadAbort(e) &&
                                      (task.m_stateFlags & TASK_STATE_THREAD_WAS_ABORTED) != 0))    // this ensures TAEs from QueueTask will be wrapped in TSE
                                {
                                    TaskSchedulerException tse = new TaskSchedulerException(e);
                                    task.AddException(tse);
                                    task.Finish(false);
                                }

                                // Don't re-throw.
                            }
                        }
                        else
                        {
                            try
                            {
                                task.ScheduleAndStart(true);
                            }
                            catch (TaskSchedulerException)
                            {
                                // No further action is necessary -- ScheduleAndStart() already transitioned
                                // the task to faulted.  But we want to make sure that no exception is thrown
                                // from here.
                            }
                        }
                    }
                    else
                    {
                        // The final state of this task does not match the desired
                        // continuation activation criteria; cancel it to denote this.
                        task.InternalCancel(false);
                    }
                }
                else
                {
                    // Note for the future: ContinuationAction still run synchronously regardless of ThreadAbortException handling.
                    // This is probably not too importnat right now, because the internal use of continuationActions only involve short actions.
                    // However if we ever make these public, we need to turn them into tasks if the antecedent threw a ThreadAbortException.
                    Action<Task> action = m_task as Action<Task>;
                    Contract.Assert(action != null, "TaskContinuation.Run(): Unknown m_task type.");
                    action(completedTask);
                }
            }
        }

        /// <summary>
        /// Runs all of the continuations, as appropriate.
        /// </summary>
        private void FinishContinuations()
        {
            // Grab the list of continuations, and lock it to contend with concurrent adds.
            // At this point, IsCompleted == true, so those adding will either come before
            // that and add a continuation under the lock (serializing before this), so we
            // will run it; or they will attempt to acquire the lock, get it, and then see the
            // task is completed (serializing after this), and run it on their own.
            List<TaskContinuation> continuations = (m_contingentProperties == null) ? null : m_contingentProperties.m_continuations;
            if (continuations != null)
            {
                lock (m_contingentProperties)
                {
                    // Ensure that all concurrent adds have completed.
                }

                // skip synchronous execution of continuations if this tasks thread was aborted
                bool bCanInlineContinuations = !(((m_stateFlags & TASK_STATE_THREAD_WAS_ABORTED) != 0) ||
                                                  (ThreadLightup.Current.ThreadState == ThreadState.AbortRequested));

                // Records earliest index of synchronous continuation.
                // A value of -1 means that no synchronous continuations were found.
                int firstSynchronousContinuation = -1;

                // Fire the asynchronous continuations first ...
                // Go back-to-front to make the "firstSynchronousContinuation" logic simpler.
                for (int i = continuations.Count - 1; i >= 0; i--)
                {
                    TaskContinuation tc = continuations[i];
                    // ContinuationActions, which execute synchronously, have a null scheduler
                    // Synchronous continuation tasks will have the ExecuteSynchronously option
                    if ((tc.m_taskScheduler != null) && ((tc.m_options & TaskContinuationOptions.ExecuteSynchronously) == 0))
                    {
                        tc.Run(this, bCanInlineContinuations);
                    }
                    else firstSynchronousContinuation = i;
                }

                // ... and then fire the synchronous continuations (if there are any)
                if (firstSynchronousContinuation > -1)
                {
                    for (int i = firstSynchronousContinuation; i < continuations.Count; i++)
                    {
                        TaskContinuation tc = continuations[i];
                        // ContinuationActions, which execute synchronously, have a null scheduler
                        // Synchronous continuation tasks will have the ExecuteSynchronously option
                        if ((tc.m_taskScheduler == null) || ((tc.m_options & TaskContinuationOptions.ExecuteSynchronously) != 0))
                            tc.Run(this, bCanInlineContinuations);
                    }
                }

                // Don't keep references to "spent" continuations
                m_contingentProperties.m_continuations = null;
            }
        }

        /// <summary>
        /// Helper function to determine whether the current task is in the state desired by the
        /// continuation kind under evaluation. Three possibilities exist: the task failed with
        /// an unhandled exception (OnFailed), the task was canceled before running (OnAborted),
        /// or the task completed successfully (OnCompletedSuccessfully).  Note that the last
        /// one includes completing due to cancellation.
        /// </summary>
        /// <param name="options">The continuation options under evaluation.</param>
        /// <returns>True if the continuation should be run given the task's current state.</returns>
        internal bool ContinueWithIsRightKind(TaskContinuationOptions options)
        {
            Contract.Assert(IsCompleted);

            if (IsFaulted)
            {
                return (options & TaskContinuationOptions.NotOnFaulted) == 0;
            }
            else if (IsCanceled)
            {
                return (options & TaskContinuationOptions.NotOnCanceled) == 0;
            }
            else
            {
                return (options & TaskContinuationOptions.NotOnRanToCompletion) == 0;
            }
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <param name="continuationAction">
        /// An action to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <returns>A new continuation <see cref="Task"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task"/> will not be scheduled for execution until the current task has
        /// completed, whether it completes due to running to completion successfully, faulting due to an
        /// unhandled exception, or exiting out early due to being canceled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationAction"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWith(Action<Task> continuationAction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith(continuationAction, TaskScheduler.Current, CancellationToken.None, TaskContinuationOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <param name="continuationAction">
        /// An action to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="cancellationToken"> The <see cref="CancellationToken"/> that will be assigned to the new continuation task.</param>
        /// <returns>A new continuation <see cref="Task"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task"/> will not be scheduled for execution until the current task has
        /// completed, whether it completes due to running to completion successfully, faulting due to an
        /// unhandled exception, or exiting out early due to being canceled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationAction"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWith(Action<Task> continuationAction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith(continuationAction, TaskScheduler.Current, cancellationToken, TaskContinuationOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <param name="continuationAction">
        /// An action to run when the <see cref="Task"/> completes.  When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="scheduler">
        /// The <see cref="TaskScheduler"/> to associate with the continuation task and to use for its execution.
        /// </param>
        /// <returns>A new continuation <see cref="Task"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task"/> will not be scheduled for execution until the current task has
        /// completed, whether it completes due to running to completion successfully, faulting due to an
        /// unhandled exception, or exiting out early due to being canceled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationAction"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="scheduler"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWith(Action<Task> continuationAction, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith(continuationAction, scheduler, CancellationToken.None, TaskContinuationOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <param name="continuationAction">
        /// An action to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="continuationOptions">
        /// Options for when the continuation is scheduled and how it behaves. This includes criteria, such
        /// as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.OnlyOnCanceled">OnlyOnCanceled</see>, as
        /// well as execution options, such as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.ExecuteSynchronously">ExecuteSynchronously</see>.
        /// </param>
        /// <returns>A new continuation <see cref="Task"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task"/> will not be scheduled for execution until the current task has
        /// completed. If the continuation criteria specified through the <paramref
        /// name="continuationOptions"/> parameter are not met, the continuation task will be canceled
        /// instead of scheduled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationAction"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="continuationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWith(Action<Task> continuationAction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith(continuationAction, TaskScheduler.Current, CancellationToken.None, continuationOptions, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <param name="continuationAction">
        /// An action to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="continuationOptions">
        /// Options for when the continuation is scheduled and how it behaves. This includes criteria, such
        /// as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.OnlyOnCanceled">OnlyOnCanceled</see>, as
        /// well as execution options, such as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.ExecuteSynchronously">ExecuteSynchronously</see>.
        /// </param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new continuation task.</param>
        /// <param name="scheduler">
        /// The <see cref="TaskScheduler"/> to associate with the continuation task and to use for its
        /// execution.
        /// </param>
        /// <returns>A new continuation <see cref="Task"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task"/> will not be scheduled for execution until the current task has
        /// completed. If the criteria specified through the <paramref name="continuationOptions"/> parameter
        /// are not met, the continuation task will be canceled instead of scheduled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationAction"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="continuationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="scheduler"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task ContinueWith(Action<Task> continuationAction, CancellationToken cancellationToken,
                                 TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith(continuationAction, scheduler, cancellationToken, continuationOptions, ref stackMark);
        }

        // Same as the above overload, just with a stack mark parameter.
        private Task ContinueWith(Action<Task> continuationAction, TaskScheduler scheduler,
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, ref StackCrawlMark stackMark)
        {
            //ThrowIfDisposed();

            // Throw on continuation with null action
            if (continuationAction == null)
            {
                throw new ArgumentNullException("continuationAction");
            }

            // Throw on continuation with null TaskScheduler
            if (scheduler == null)
            {
                throw new ArgumentNullException("scheduler");
            }

            TaskCreationOptions creationOptions;
            InternalTaskOptions internalOptions;
            CreationOptionsFromContinuationOptions(continuationOptions, out creationOptions, out internalOptions);

            Task thisTask = this;
            Task continuationTask = new Task(
                delegate(object obj) { continuationAction(thisTask); },
                null,
                Task.InternalCurrent,
                cancellationToken,
                creationOptions,
                internalOptions,
                null, // leave taskScheduler null until TaskContinuation.Run() is called
                ref stackMark
            );

            // Register the continuation.  If synchronous execution is requested, this may
            // actually invoke the continuation before returning.
            ContinueWithCore(continuationTask, scheduler, continuationOptions);

            return continuationTask;
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <typeparam name="TResult">
        /// The type of the result produced by the continuation.
        /// </typeparam>
        /// <param name="continuationFunction">
        /// A function to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <returns>A new continuation <see cref="Task{TResult}"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task{TResult}"/> will not be scheduled for execution until the current task has
        /// completed, whether it completes due to running to completion successfully, faulting due to an
        /// unhandled exception, or exiting out early due to being canceled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationFunction"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith<TResult>(continuationFunction, TaskScheduler.Current, CancellationToken.None,
                TaskContinuationOptions.None, ref stackMark);
        }


        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <typeparam name="TResult">
        /// The type of the result produced by the continuation.
        /// </typeparam>
        /// <param name="continuationFunction">
        /// A function to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new continuation task.</param>
        /// <returns>A new continuation <see cref="Task{TResult}"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task{TResult}"/> will not be scheduled for execution until the current task has
        /// completed, whether it completes due to running to completion successfully, faulting due to an
        /// unhandled exception, or exiting out early due to being canceled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationFunction"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction, CancellationToken cancellationToken)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith<TResult>(continuationFunction, TaskScheduler.Current, cancellationToken, TaskContinuationOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <typeparam name="TResult">
        /// The type of the result produced by the continuation.
        /// </typeparam>
        /// <param name="continuationFunction">
        /// A function to run when the <see cref="Task"/> completes.  When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="scheduler">
        /// The <see cref="TaskScheduler"/> to associate with the continuation task and to use for its execution.
        /// </param>
        /// <returns>A new continuation <see cref="Task{TResult}"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task{TResult}"/> will not be scheduled for execution until the current task has
        /// completed, whether it completes due to running to completion successfully, faulting due to an
        /// unhandled exception, or exiting out early due to being canceled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationFunction"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="scheduler"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith<TResult>(continuationFunction, scheduler, CancellationToken.None, TaskContinuationOptions.None, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <typeparam name="TResult">
        /// The type of the result produced by the continuation.
        /// </typeparam>
        /// <param name="continuationFunction">
        /// A function to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="continuationOptions">
        /// Options for when the continuation is scheduled and how it behaves. This includes criteria, such
        /// as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.OnlyOnCanceled">OnlyOnCanceled</see>, as
        /// well as execution options, such as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.ExecuteSynchronously">ExecuteSynchronously</see>.
        /// </param>
        /// <returns>A new continuation <see cref="Task{TResult}"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task{TResult}"/> will not be scheduled for execution until the current task has
        /// completed. If the continuation criteria specified through the <paramref
        /// name="continuationOptions"/> parameter are not met, the continuation task will be canceled
        /// instead of scheduled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationFunction"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="continuationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction, TaskContinuationOptions continuationOptions)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith<TResult>(continuationFunction, TaskScheduler.Current, CancellationToken.None, continuationOptions, ref stackMark);
        }

        /// <summary>
        /// Creates a continuation that executes when the target <see cref="Task"/> completes.
        /// </summary>
        /// <typeparam name="TResult">
        /// The type of the result produced by the continuation.
        /// </typeparam>
        /// <param name="continuationFunction">
        /// A function to run when the <see cref="Task"/> completes. When run, the delegate will be
        /// passed the completed task as an argument.
        /// </param>
        /// <param name="cancellationToken">The <see cref="CancellationToken"/> that will be assigned to the new continuation task.</param>
        /// <param name="continuationOptions">
        /// Options for when the continuation is scheduled and how it behaves. This includes criteria, such
        /// as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.OnlyOnCanceled">OnlyOnCanceled</see>, as
        /// well as execution options, such as <see
        /// cref="System.Threading.Tasks.TaskContinuationOptions.ExecuteSynchronously">ExecuteSynchronously</see>.
        /// </param>
        /// <param name="scheduler">
        /// The <see cref="TaskScheduler"/> to associate with the continuation task and to use for its
        /// execution.
        /// </param>
        /// <returns>A new continuation <see cref="Task{TResult}"/>.</returns>
        /// <remarks>
        /// The returned <see cref="Task{TResult}"/> will not be scheduled for execution until the current task has
        /// completed. If the criteria specified through the <paramref name="continuationOptions"/> parameter
        /// are not met, the continuation task will be canceled instead of scheduled.
        /// </remarks>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="continuationFunction"/> argument is null.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// The <paramref name="continuationOptions"/> argument specifies an invalid value for <see
        /// cref="T:System.Threading.Tasks.TaskContinuationOptions">TaskContinuationOptions</see>.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="scheduler"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">The provided <see cref="System.Threading.CancellationToken">CancellationToken</see>
        /// has already been disposed.
        /// </exception>
        [MethodImplAttribute(MethodImplOptions.NoInlining)] // Methods containing StackCrawlMark local var have to be marked non-inlineable            
        public Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction, CancellationToken cancellationToken,
                                                   TaskContinuationOptions continuationOptions, TaskScheduler scheduler)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return ContinueWith<TResult>(continuationFunction, scheduler, cancellationToken, continuationOptions, ref stackMark);
        }

        // Same as the above overload, just with a stack mark parameter.
        private Task<TResult> ContinueWith<TResult>(Func<Task, TResult> continuationFunction, TaskScheduler scheduler,
            CancellationToken cancellationToken, TaskContinuationOptions continuationOptions, ref StackCrawlMark stackMark)
        {
            //ThrowIfDisposed();

            // Throw on continuation with null function
            if (continuationFunction == null)
            {
                throw new ArgumentNullException("continuationFunction");
            }

            // Throw on continuation with null task scheduler
            if (scheduler == null)
            {
                throw new ArgumentNullException("scheduler");
            }

            TaskCreationOptions creationOptions;
            InternalTaskOptions internalOptions;
            CreationOptionsFromContinuationOptions(continuationOptions, out creationOptions, out internalOptions);

            Task thisTask = this;
            Task<TResult> continuationTask = new Task<TResult>(
                delegate() { return continuationFunction(thisTask); },
                Task.InternalCurrent,
                cancellationToken,
                creationOptions,
                internalOptions,
                null, // leave taskScheduler null until TaskContinuation.Run() is called
                ref stackMark
            );

            // Register the continuation.  If synchronous execution is requested, this may
            // actually invoke the continuation before returning.
            ContinueWithCore(continuationTask, scheduler, continuationOptions);

            return continuationTask;
        }

        /// <summary>
        /// Converts TaskContinuationOptions to TaskCreationOptions, and also does
        /// some validity checking along the way.
        /// </summary>
        /// <param name="continuationOptions">Incoming TaskContinuationOptions</param>
        /// <param name="creationOptions">Outgoing TaskCreationOptions</param>
        /// <param name="internalOptions">Outgoing InternalTaskOptions</param>
        internal static void CreationOptionsFromContinuationOptions(
            TaskContinuationOptions continuationOptions,
            out TaskCreationOptions creationOptions,
            out InternalTaskOptions internalOptions)
        {
            // This is used a couple of times below
            TaskContinuationOptions NotOnAnything =
                TaskContinuationOptions.NotOnCanceled |
                TaskContinuationOptions.NotOnFaulted |
                TaskContinuationOptions.NotOnRanToCompletion;

            TaskContinuationOptions creationOptionsMask =
                TaskContinuationOptions.PreferFairness |
                TaskContinuationOptions.LongRunning |
                TaskContinuationOptions.AttachedToParent;


            // Check that LongRunning and ExecuteSynchronously are not specified together
            TaskContinuationOptions illegalMask = TaskContinuationOptions.ExecuteSynchronously | TaskContinuationOptions.LongRunning;
            if ((continuationOptions & illegalMask) == illegalMask)
            {
                throw new ArgumentOutOfRangeException("continuationOptions", Strings.Task_ContinueWith_ESandLR);
            }

            // Check that no illegal options were specified
            if ((continuationOptions &
                ~(creationOptionsMask | NotOnAnything |
                  TaskContinuationOptions.ExecuteSynchronously)) != 0)
            {
                throw new ArgumentOutOfRangeException("continuationOptions");
            }

            // Check that we didn't specify "not on anything"
            if ((continuationOptions & NotOnAnything) == NotOnAnything)
            {
                throw new ArgumentOutOfRangeException("continuationOptions", Strings.Task_ContinueWith_NotOnAnything);
            }

            creationOptions = (TaskCreationOptions)(continuationOptions & creationOptionsMask);
            internalOptions = InternalTaskOptions.ContinuationTask;
        }


        /// <summary>
        /// Registers the continuation and possibly runs it (if the task is already finished).
        /// </summary>
        /// <param name="continuationTask">The continuation task itself.</param>
        /// <param name="scheduler">TaskScheduler with which to associate continuation task.</param>
        /// <param name="options">Restrictions on when the continuation becomes active.</param>
        internal void ContinueWithCore(Task continuationTask, TaskScheduler scheduler, TaskContinuationOptions options)
        {
            Contract.Assert(continuationTask != null, "Task.ContinueWithCore(): null continuationTask");
            Contract.Assert((!continuationTask.IsCompleted) || continuationTask.CancellationToken.IsCancellationRequested,
                "Task.ContinueWithCore(): continuationTask is completed and its CT is not signaled");

            // It doesn't really do any harm to queue up an already-completed continuation, but it's
            // a little wasteful.  So we'll make an attempt at avoiding it (although the race condition
            // here could still result in a completed continuation task being queued.)
            if (continuationTask.IsCompleted) return;

            TaskContinuation continuation = new TaskContinuation(continuationTask, scheduler, options);

            // If the task has not finished, we will enqueue the continuation to fire when completed.
            if (!IsCompleted)
            {
                // If not created yet, we will atomically initialize the queue of actions.
                LazyInitializer.EnsureInitialized<ContingentProperties>(ref m_contingentProperties, s_contingentPropertyCreator);
                if (m_contingentProperties.m_continuations == null)
                {
                    Interlocked.CompareExchange(ref m_contingentProperties.m_continuations, new List<TaskContinuation>(), null);
                }

                // Now we must serialize access to the list itself.
                lock (m_contingentProperties)
                {
                    // We have to check IsCompleted again here, since the task may have
                    // finished before we got around to acquiring the lock on the actions.
                    // There is a race condition here, but it's OK.  If the thread that
                    // finishes the task notices a non-null actions queue it will try to
                    // acquire a lock on it.  Thus it will have to wait for the current
                    // thread to exit the critical region we're currently in.
                    if (!IsCompleted)
                    {
                        m_contingentProperties.m_continuations.Add(continuation);
                        return;
                    }
                }
            }

            // If we fell through, the task has already completed.  We'll invoke the action inline.
            // Only start the continuation if the right kind was established.
            continuation.Run(this, true);
        }

        // Adds a lightweight completion action to a task.  This is similar to a continuation
        // task except that it is stored as an action, and thus does not require the allocation/
        // execution resources of a continuation task.
        //
        // Used internally by ContinueWhenAll() and ContinueWhenAny().
        internal void AddCompletionAction(Action<Task> action)
        {
            if (!IsCompleted)
            {
                LazyInitializer.EnsureInitialized<ContingentProperties>(ref m_contingentProperties, s_contingentPropertyCreator);
                TaskContinuation tc = new TaskContinuation(action);
                if (m_contingentProperties.m_continuations == null)
                {
                    Interlocked.CompareExchange(ref m_contingentProperties.m_continuations, new List<TaskContinuation>(), null);
                }

                // Serialize access to the continuations list
                lock (m_contingentProperties)
                {
                    // We have to check IsCompleted again here, since the task may have
                    // finished before we got around to acquiring the lock on the actions.
                    // There is a race condition here, but it's OK.  If the thread that
                    // finishes the task notices a non-null actions queue it will try to
                    // acquire a lock on it.  Thus it will have to wait for the current
                    // thread to exit the critical region we're currently in.
                    if (!IsCompleted)
                    {
                        m_contingentProperties.m_continuations.Add(tc);
                        return;
                    }
                }
            }

            // If we got this far, we've already completed
            action(this);
        }


        //
        // Wait methods
        //

        /// <summary>
        /// Waits for all of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// At least one of the <see cref="Task"/> instances was canceled -or- an exception was thrown during
        /// the execution of at least one of the <see cref="Task"/> instances.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static void WaitAll(params Task[] tasks)
        {
#if DEBUG
            bool waitResult =
#endif
 WaitAll(tasks, Timeout.Infinite);

#if DEBUG
            Contract.Assert(waitResult, "expected wait to succeed");
#endif
        }

        /// <summary>
        /// Waits for all of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <returns>
        /// true if all of the <see cref="Task"/> instances completed execution within the allotted time;
        /// otherwise, false.
        /// </returns>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="timeout">
        /// A <see cref="System.TimeSpan"/> that represents the number of milliseconds to wait, or a <see
        /// cref="System.TimeSpan"/> that represents -1 milliseconds to wait indefinitely.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// At least one of the <see cref="Task"/> instances was canceled -or- an exception was thrown during
        /// the execution of at least one of the <see cref="Task"/> instances.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="timeout"/> is a negative number other than -1 milliseconds, which represents an
        /// infinite time-out -or- timeout is greater than
        /// <see cref="System.Int32.MaxValue"/>.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static bool WaitAll(Task[] tasks, TimeSpan timeout)
        {
            long totalMilliseconds = (long)timeout.TotalMilliseconds;
            if (totalMilliseconds < -1 || totalMilliseconds > 0x7fffffff)
            {
                throw new ArgumentOutOfRangeException("timeout");
            }

            return WaitAll(tasks, (int)totalMilliseconds);

        }

        /// <summary>
        /// Waits for all of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <returns>
        /// true if all of the <see cref="Task"/> instances completed execution within the allotted time;
        /// otherwise, false.
        /// </returns>
        /// <param name="millisecondsTimeout">
        /// The number of milliseconds to wait, or <see cref="System.Threading.Timeout.Infinite"/> (-1) to
        /// wait indefinitely.</param>
        /// <param name="tasks">An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// At least one of the <see cref="Task"/> instances was canceled -or- an exception was thrown during
        /// the execution of at least one of the <see cref="Task"/> instances.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="millisecondsTimeout"/> is a negative number other than -1, which represents an
        /// infinite time-out.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static bool WaitAll(Task[] tasks, int millisecondsTimeout)
        {
            return WaitAll(tasks, millisecondsTimeout, CancellationToken.None);
        }

        /// <summary>
        /// Waits for all of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <returns>
        /// true if all of the <see cref="Task"/> instances completed execution within the allotted time;
        /// otherwise, false.
        /// </returns>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="cancellationToken">
        /// A <see cref="CancellationToken"/> to observe while waiting for the tasks to complete.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// At least one of the <see cref="Task"/> instances was canceled -or- an exception was thrown during
        /// the execution of at least one of the <see cref="Task"/> instances.
        /// </exception>
        /// <exception cref="T:System.OperationCanceledException2">
        /// The <paramref name="cancellationToken"/> was canceled.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static void WaitAll(Task[] tasks, CancellationToken cancellationToken)
        {
            WaitAll(tasks, Timeout.Infinite, cancellationToken);
        }

        /// <summary>
        /// Waits for all of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <returns>
        /// true if all of the <see cref="Task"/> instances completed execution within the allotted time;
        /// otherwise, false.
        /// </returns>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="millisecondsTimeout">
        /// The number of milliseconds to wait, or <see cref="System.Threading.Timeout.Infinite"/> (-1) to
        /// wait indefinitely.
        /// </param>
        /// <param name="cancellationToken">
        /// A <see cref="CancellationToken"/> to observe while waiting for the tasks to complete.
        /// </param>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.AggregateException">
        /// At least one of the <see cref="Task"/> instances was canceled -or- an exception was thrown during
        /// the execution of at least one of the <see cref="Task"/> instances.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="millisecondsTimeout"/> is a negative number other than -1, which represents an
        /// infinite time-out.
        /// </exception>
        /// <exception cref="T:System.OperationCanceledException2">
        /// The <paramref name="cancellationToken"/> was canceled.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static bool WaitAll(Task[] tasks, int millisecondsTimeout, CancellationToken cancellationToken)
        {
            if (tasks == null)
            {
                throw new ArgumentNullException("tasks");
            }
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("timeout");
            }

#if DEBUG
            Contract.Assert(tasks != null && millisecondsTimeout >= -1, "invalid arguments passed to WaitAll");
#endif
            cancellationToken.ThrowIfCancellationRequested(); // early check before we make any allocations

            //
            // In this WaitAll() implementation we have 2 alternate code paths for a task to be handled:
            // CODEPATH1: skip an already completed task, CODEPATH2: actually wait on task's handle
            // We make sure that the exception behavior of Task.Wait() is replicated the same for tasks handled in either of these codepaths
            //

            List<Exception> exceptions = null;
            List<Task> waitedOnTaskList = null; // tasks whose async handles we actually grabbed

            bool returnValue = true;
            Task currentTask = Task.InternalCurrent;
            TaskScheduler currentTm = (currentTask == null) ? TaskScheduler.Default : currentTask.ExecutingTaskScheduler;
            object currentTmStatics = currentTm.GetThreadStatics();

            // Collects incomplete tasks in "waitedOnTaskList"
            for (int i = tasks.Length - 1; i >= 0; i--)
            {
                Task task = tasks[i];

                if (task == null)
                {
                    throw new ArgumentException(Strings.Task_WaitMulti_NullTask,"tasks");
                }

                //task.ThrowIfDisposed();

                bool taskIsCompleted = task.IsCompleted;
                if (!taskIsCompleted)
                {
                    // try inlining the task only if we have an infinite timeout and an empty cancellation token
                    if (millisecondsTimeout != Timeout.Infinite || cancellationToken.CanBeCanceled)
                    {
                        // We either didn't attempt inline execution because we had a non-infinite timeout or we had a cancellable token.
                        // In all cases we need to do a full wait on the task (=> add its event into the list.)
                        if (waitedOnTaskList == null)
                        {
                            waitedOnTaskList = new List<Task>(tasks.Length);
                        }
                        waitedOnTaskList.Add(task);
                    }
                    else
                    {
                        // We are eligible for inlining.
                        taskIsCompleted = task.WrappedTryRunInline(currentTm, currentTmStatics) &&
                            task.IsCompleted; // A successful TryRunInline doesn't guarantee completion

                        if (!taskIsCompleted)
                        {
                            // Inlining didn't work.
                            // Do a full wait on the task (=> add its event into the list.)
                            if (waitedOnTaskList == null)
                            {
                                waitedOnTaskList = new List<Task>(tasks.Length);
                            }
                            waitedOnTaskList.Add(task);

                        }
                    }
                }

                if (taskIsCompleted)
                {
                    // The task has finished. Make sure we aggregate its exceptions.
                    AddExceptionsForCompletedTask(ref exceptions, task);

                }
            }

            if (waitedOnTaskList != null)
            {
                Contract.Assert(waitedOnTaskList.Count > 0);
                WaitHandle[] waitHandles = new WaitHandle[waitedOnTaskList.Count];
                for (int i = 0; i < waitHandles.Length; i++)
                {
                    waitHandles[i] = waitedOnTaskList[i].CompletedEvent.WaitHandle;
                }
                returnValue = WaitAllSTAAnd64Aware(waitHandles, millisecondsTimeout, cancellationToken);

                // If the wait didn't time out, ensure exceptions are propagated.
                if (returnValue)
                {
                    for (int i = 0; i < waitedOnTaskList.Count; i++)
                    {
                        AddExceptionsForCompletedTask(ref exceptions, waitedOnTaskList[i]);
                    }
                }

                // We need to prevent the tasks array from being GC'ed until we come out of the wait.
                // This is necessary so that the Parallel Debugger can traverse it during the long wait and deduce waiter/waitee relationships
                GC.KeepAlive(tasks);
            }

            // If one or more threw exceptions, aggregate them.
            if (exceptions != null)
            {
                throw new AggregateException(exceptions);
            }

            return returnValue;
        }

        /// <summary>
        /// Waits for a set of handles in a STA-aware way.  In other words, it will wait for each
        /// of the events individually if we're on a STA thread, because MsgWaitForMultipleObjectsEx
        /// can't do a true wait-all due to its hidden message queue event. This is not atomic,
        /// of course, but we only wait on one-way (MRE) events anyway so this is OK.
        /// </summary>
        /// <param name="waitHandles">An array of wait handles to wait on.</param>
        /// <param name="millisecondsTimeout">The timeout to use during waits.</param>
        /// <param name="cancellationToken">The cancellationToken that enables a wait to be canceled.</param>
        /// <returns>True if all waits succeeded, false if a timeout occurred.</returns>
        private static bool WaitAllSTAAnd64Aware(WaitHandle[] waitHandles, int millisecondsTimeout, CancellationToken cancellationToken)
        {
            // We're on an STA thread, so we can't use the real Win32 wait-all.
            // We instead walk the list, and wait on each individually.  Perf will
            // be poor because we'll incur O(N) context switches as we wake up.

            // CancellationToken enabled waits will also choose this codepath regardless of apartment state.
            // The ability to use WaitAny to probe for cancellation during one by one waits makes it easy to support CT without introducing racy code paths.

            WaitHandle[] cancelableWHPair = null; // to be used with WaitAny if we have a CancellationToken

            if (cancellationToken.CanBeCanceled)
            {
                cancelableWHPair = new WaitHandle[2]; // one for the actual wait handle, other for the cancellation event 
                cancelableWHPair[1] = cancellationToken.WaitHandle;
            }

            for (int i = 0; i < waitHandles.Length; i++)
            {
                long startTicks = (millisecondsTimeout == Timeout.Infinite) ? 0 : DateTime.UtcNow.Ticks;

                if (cancellationToken.CanBeCanceled)
                {
                    // do a WaitAny on the WH of interest and the cancellation event.
                    cancelableWHPair[0] = waitHandles[i];
                    int waitRetCode = WaitHandle.WaitAny(cancelableWHPair, millisecondsTimeout);
                    if (waitRetCode == WaitHandle.WaitTimeout)
                        return false;

                    // we could have come out of the wait due to index 1 (i.e. cancellationToken.WaitHandle), check and throw
                    cancellationToken.ThrowIfCancellationRequested();

                    // the wait should have returned 0, otherwise we have a bug in CT or the code above
                    Contract.Assert(waitRetCode == 0, "Unexpected waitcode from WaitAny with cancellation event");
                }
                else
                {

                    if (!waitHandles[i].WaitOne(millisecondsTimeout))
                        return false;
                }


                // Adjust the timeout.
                if (millisecondsTimeout != Timeout.Infinite)
                {
                    long elapsedMilliseconds = (DateTime.UtcNow.Ticks - startTicks) / TimeSpan.TicksPerMillisecond;
                    if (elapsedMilliseconds > int.MaxValue || elapsedMilliseconds > millisecondsTimeout)
                        return false;
                    millisecondsTimeout -= (int)elapsedMilliseconds;
                }
            }

            return true;
        }

        /// <summary>
        /// Internal WaitAll implementation which is meant to be used with small number of tasks,
        /// optimized for Parallel.Invoke and other structured primitives.
        /// </summary>
        internal static void FastWaitAll(Task[] tasks)
        {
#if DEBUG
            Contract.Assert(tasks != null);
#endif

            List<Exception> exceptions = null;
            TaskScheduler currentTm = TaskScheduler.Current;
            object currentTmStatics = currentTm.GetThreadStatics();

            // Collects incomplete tasks in "waitedOnTaskList" and their cooperative events in "cooperativeEventList"
            for (int i = tasks.Length - 1; i >= 0; i--)
            {
                if (!tasks[i].IsCompleted)
                {
                    // Just attempting to inline here... result doesn't matter.
                    // We'll do a second pass to do actual wait on each task, and to aggregate their exceptions.
                    // If the task is inlined here, it will register as IsCompleted in the second pass
                    // and will just give us the exception.

                    tasks[i].WrappedTryRunInline(currentTm, currentTmStatics);
                }
            }

            // Wait on the tasks.
            for (int i = tasks.Length - 1; i >= 0; i--)
            {
                tasks[i].CompletedEvent.Wait(); // Just a boolean check if the task is already done.
                AddExceptionsForCompletedTask(ref exceptions, tasks[i]);
            }

            // If one or more threw exceptions, aggregate them.
            if (exceptions != null)
            {
                throw new AggregateException(exceptions);
            }
        }

        /// <summary>
        /// This internal function is only meant to be called by WaitAll()
        /// If the completed task is canceled or it has other exceptions, here we will add those
        /// into the passed in exception list (which will be lazily initialized here).
        /// </summary>
        internal static void AddExceptionsForCompletedTask(ref List<Exception> exceptions, Task t)
        {
            AggregateException ex = t.GetExceptions(true);
            if (ex != null)
            {
                // make sure the task's exception observed status is set appropriately
                // it's possible that WaitAll was called by the parent of an attached child,
                // this will make sure it won't throw again in the implicit wait
                t.UpdateExceptionObservedStatus();

                if (exceptions == null)
                {
                    exceptions = new List<Exception>(ex.InnerExceptions.Count);
                }

                exceptions.AddRange(ex.InnerExceptions);
            }
        }


        /// <summary>
        /// Waits for any of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <returns>The index of the completed task in the <paramref name="tasks"/> array argument.</returns>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static int WaitAny(params Task[] tasks)
        {
            int waitResult = WaitAny(tasks, Timeout.Infinite);
            Contract.Assert(tasks.Length == 0 || waitResult != -1, "expected wait to succeed");
            return waitResult;
        }

        /// <summary>
        /// Waits for any of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="timeout">
        /// A <see cref="System.TimeSpan"/> that represents the number of milliseconds to wait, or a <see
        /// cref="System.TimeSpan"/> that represents -1 milliseconds to wait indefinitely.
        /// </param>
        /// <returns>
        /// The index of the completed task in the <paramref name="tasks"/> array argument, or -1 if the
        /// timeout occurred.
        /// </returns>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="timeout"/> is a negative number other than -1 milliseconds, which represents an
        /// infinite time-out -or- timeout is greater than
        /// <see cref="System.Int32.MaxValue"/>.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static int WaitAny(Task[] tasks, TimeSpan timeout)
        {
            long totalMilliseconds = (long)timeout.TotalMilliseconds;
            if (totalMilliseconds < -1 || totalMilliseconds > 0x7fffffff)
            {
                throw new ArgumentOutOfRangeException("timeout");
            }

            return WaitAny(tasks, (int)totalMilliseconds);
        }

        /// <summary>
        /// Waits for any of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="cancellationToken">
        /// A <see cref="CancellationToken"/> to observe while waiting for a task to complete.
        /// </param>
        /// <returns>
        /// The index of the completed task in the <paramref name="tasks"/> array argument.
        /// </returns>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:System.OperationCanceledException2">
        /// The <paramref name="cancellationToken"/> was canceled.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static int WaitAny(Task[] tasks, CancellationToken cancellationToken)
        {
            return WaitAny(tasks, Timeout.Infinite, cancellationToken);
        }

        /// <summary>
        /// Waits for any of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="millisecondsTimeout">
        /// The number of milliseconds to wait, or <see cref="System.Threading.Timeout.Infinite"/> (-1) to
        /// wait indefinitely.
        /// </param>
        /// <returns>
        /// The index of the completed task in the <paramref name="tasks"/> array argument, or -1 if the
        /// timeout occurred.
        /// </returns>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="millisecondsTimeout"/> is a negative number other than -1, which represents an
        /// infinite time-out.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static int WaitAny(Task[] tasks, int millisecondsTimeout)
        {
            return WaitAny(tasks, millisecondsTimeout, CancellationToken.None);
        }

        /// <summary>
        /// Waits for any of the provided <see cref="Task"/> objects to complete execution.
        /// </summary>
        /// <param name="tasks">
        /// An array of <see cref="Task"/> instances on which to wait.
        /// </param>
        /// <param name="millisecondsTimeout">
        /// The number of milliseconds to wait, or <see cref="System.Threading.Timeout.Infinite"/> (-1) to
        /// wait indefinitely.
        /// </param>
        /// <param name="cancellationToken">
        /// A <see cref="CancellationToken"/> to observe while waiting for a task to complete.
        /// </param>
        /// <returns>
        /// The index of the completed task in the <paramref name="tasks"/> array argument, or -1 if the
        /// timeout occurred.
        /// </returns>
        /// <exception cref="T:System.ArgumentNullException">
        /// The <paramref name="tasks"/> argument is null.
        /// </exception>
        /// <exception cref="T:System.ArgumentException">
        /// The <paramref name="tasks"/> argument contains a null element.
        /// </exception>
        /// <exception cref="T:System.ObjectDisposedException">
        /// The <see cref="Task"/> has been disposed.
        /// </exception>
        /// <exception cref="T:ArgumentOutOfRangeException">
        /// <paramref name="millisecondsTimeout"/> is a negative number other than -1, which represents an
        /// infinite time-out.
        /// </exception>
        /// <exception cref="T:System.OperationCanceledException2">
        /// The <paramref name="cancellationToken"/> was canceled.
        /// </exception>
        [MethodImpl(MethodImplOptions.NoOptimization)]  // this is needed for the parallel debugger
        public static int WaitAny(Task[] tasks, int millisecondsTimeout, CancellationToken cancellationToken)
        {
            if (tasks == null)
            {
                throw new ArgumentNullException("tasks");
            }
            if (millisecondsTimeout < -1)
            {
                throw new ArgumentOutOfRangeException("millisecondsTimeout");
            }

            if (tasks.Length > 0)
            {
                var tcs = new TaskCompletionSource<Task>();
                Task.Factory.ContinueWhenAny(tasks, completed => tcs.TrySetResult(completed),
                    CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
                if (tcs.Task.Wait(millisecondsTimeout, cancellationToken))
                {
                    var resultTask = tcs.Task.Result;
                    for (int i = 0; i < tasks.Length; i++)
                    {
                        if (tasks[i] == resultTask)
                            return i;
                    }
                }
            }

            // We need to prevent the tasks array from being GC'ed until we come out of the wait.
            // This is necessary so that the Parallel Debugger can traverse it during the long wait and deduce waiter/waitee relationships
            GC.KeepAlive(tasks);

            return -1;
        }

    }

    // Proxy class for better debugging experience
    internal class SystemThreadingTasks_TaskDebugView
    {
        private Task m_task;

        public SystemThreadingTasks_TaskDebugView(Task task)
        {
            m_task = task;
        }

        public object AsyncState { get { return m_task.AsyncState; } }
        public TaskCreationOptions CreationOptions { get { return m_task.CreationOptions; } }
        public Exception Exception { get { return m_task.Exception; } }
        public int Id { get { return m_task.Id; } }
        public bool CancellationPending { get { return (m_task.Status == TaskStatus.WaitingToRun) && m_task.CancellationToken.IsCancellationRequested; } }
        public TaskStatus Status { get { return m_task.Status; } }
    }

    /// <summary>
    /// Specifies flags that control optional behavior for the creation and execution of tasks.
    /// </summary>
    [Flags]

    public enum TaskCreationOptions
    {
        /// <summary>
        /// Specifies that the default behavior should be used.
        /// </summary>
        None = 0x0,

        /// <summary>
        /// A hint to a <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> to schedule a
        /// task in as fair a manner as possible, meaning that tasks scheduled sooner will be more likely to
        /// be run sooner, and tasks scheduled later will be more likely to be run later.
        /// </summary>
        PreferFairness = 0x01,

        /// <summary>
        /// Specifies that a task will be a long-running, course-grained operation. It provides a hint to the
        /// <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> that oversubscription may be
        /// warranted. 
        /// </summary>
        LongRunning = 0x02,

        /// <summary>
        /// Specifies that a task is attached to a parent in the task hierarchy.
        /// </summary>
        AttachedToParent = 0x04,
    }


    /// <summary>
    /// Task creation flags which are only used internally.
    /// </summary>
    [Flags]

    internal enum InternalTaskOptions
    {
        /// <summary> Specifies "No internal task options" </summary>
        None,

        /// <summary>Used to filter out internal vs. public task creation options.</summary>
        InternalOptionsMask = 0x0000FF00,

        ChildReplica = 0x0100,
        ContinuationTask = 0x0200,
        PromiseTask = 0x0400,
        SelfReplicating = 0x0800,

        /// <summary>Specifies that the task will be queued by the runtime before handing it over to the user. 
        /// This flag will be used to skip the cancellationtoken registration step, which is only meant for unstarted tasks.</summary>
        QueuedByRuntime = 0x2000
    }

    /// <summary>
    /// Specifies flags that control optional behavior for the creation and execution of continuation tasks.
    /// </summary>
    [Flags]

    public enum TaskContinuationOptions
    {
        /// <summary>
        /// Default = "Continue on any, no task options, run asynchronously"
        /// Specifies that the default behavior should be used.  Continuations, by default, will
        /// be scheduled when the antecedent task completes, regardless of the task's final <see
        /// cref="System.Threading.Tasks.TaskStatus">TaskStatus</see>.
        /// </summary>
        None = 0,

        // These are identical to their meanings and values in TaskCreationOptions

        /// <summary>
        /// A hint to a <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> to schedule a
        /// task in as fair a manner as possible, meaning that tasks scheduled sooner will be more likely to
        /// be run sooner, and tasks scheduled later will be more likely to be run later.
        /// </summary>
        PreferFairness = 0x01,

        /// <summary>
        /// Specifies that a task will be a long-running, course-grained operation.  It provides
        /// a hint to the <see cref="System.Threading.Tasks.TaskScheduler">TaskScheduler</see> that
        /// oversubscription may be warranted.
        /// </summary>
        LongRunning = 0x02,
        /// <summary>
        /// Specifies that a task is attached to a parent in the task hierarchy.
        /// </summary>
        AttachedToParent = 0x04,

        // These are specific to continuations

        /// <summary>
        /// Specifies that the continuation task should not be scheduled if its antecedent ran to completion.
        /// This option is not valid for multi-task continuations.
        /// </summary>
        NotOnRanToCompletion = 0x10000,
        /// <summary>
        /// Specifies that the continuation task should not be scheduled if its antecedent threw an unhandled
        /// exception. This option is not valid for multi-task continuations.
        /// </summary>
        NotOnFaulted = 0x20000,
        /// <summary>
        /// Specifies that the continuation task should not be scheduled if its antecedent was canceled. This
        /// option is not valid for multi-task continuations.
        /// </summary>
        NotOnCanceled = 0x40000,
        /// <summary>
        /// Specifies that the continuation task should be scheduled only if its antecedent ran to
        /// completion. This option is not valid for multi-task continuations.
        /// </summary>
        OnlyOnRanToCompletion = NotOnFaulted | NotOnCanceled,
        /// <summary>
        /// Specifies that the continuation task should be scheduled only if its antecedent threw an
        /// unhandled exception. This option is not valid for multi-task continuations.
        /// </summary>
        OnlyOnFaulted = NotOnRanToCompletion | NotOnCanceled,
        /// <summary>
        /// Specifies that the continuation task should be scheduled only if its antecedent was canceled.
        /// This option is not valid for multi-task continuations.
        /// </summary>
        OnlyOnCanceled = NotOnRanToCompletion | NotOnFaulted,
        /// <summary>
        /// Specifies that the continuation task should be executed synchronously. With this option
        /// specified, the continuation will be run on the same thread that causes the antecedent task to
        /// transition into its final state. If the antecedent is already complete when the continuation is
        /// created, the continuation will run on the thread creating the continuation.  Only very
        /// short-running continuations should be executed synchronously.
        /// </summary>
        ExecuteSynchronously = 0x80000
    }
}

