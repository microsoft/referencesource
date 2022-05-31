using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Security;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace System.Runtime.CompilerServices
{
    /// <summary>
    /// Provides a builder for asynchronous methods that return void.
    /// This type is intended for compiler use only.
    /// </summary>
    public struct AsyncVoidMethodBuilder : IAsyncMethodBuilder
    {
        /// <summary>The synchronization context associated with this operation.</summary>
        private readonly SynchronizationContext m_synchronizationContext;
        /// <summary>State related to the IAsyncStateMachine.</summary>
        private AsyncMethodBuilderCore m_coreState; // mutable struct: must not be readonly
        /// <summary>An object used by the debugger to uniquely identify this builder.  Lazily initialized.</summary>
        private object m_objectIdForDebugger;

        /// <summary>Temporary support for disabling crashing if tasks go unobserved.</summary>
        static AsyncVoidMethodBuilder()
        {
            try { PreventUnobservedTaskExceptions(); }
            catch { }
        }

        /// <summary>Registers with UnobservedTaskException to suppress exception crashing.</summary>
        internal static void PreventUnobservedTaskExceptions()
        {
            if (Interlocked.CompareExchange(ref s_preventUnobservedTaskExceptionsInvoked, 1, 0) == 0)
            {
                TaskScheduler.UnobservedTaskException += (s, e) => e.SetObserved();
            }
        }
        /// <summary>Non-zero if PreventUnobservedTaskExceptions has already been invoked.</summary>
        private static int s_preventUnobservedTaskExceptionsInvoked;

        /// <summary>Initializes a new <see cref="AsyncVoidMethodBuilder"/>.</summary>
        /// <returns>The initialized <see cref="AsyncVoidMethodBuilder"/>.</returns>
        public static AsyncVoidMethodBuilder Create()
        {
            // Capture the current sync context.  If there isn't one, use the dummy s_noContextCaptured
            // instance; this allows us to tell the state of no captured context apart from the state
            // of an improperly constructed builder instance.
            return new AsyncVoidMethodBuilder(SynchronizationContext.Current);
        }

        /// <summary>Initializes the <see cref="AsyncVoidMethodBuilder"/>.</summary>
        /// <param name="synchronizationContext">The synchronizationContext associated with this operation. This may be null.</param>
        private AsyncVoidMethodBuilder(SynchronizationContext synchronizationContext)
        {
            m_synchronizationContext = synchronizationContext;
            if (synchronizationContext != null) synchronizationContext.OperationStarted();

            m_coreState = default(AsyncMethodBuilderCore);
            m_objectIdForDebugger = null;
        }

        /// <summary>Initiates the builder's execution with the associated state machine.</summary>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="stateMachine">The state machine instance, passed by reference.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="stateMachine"/> argument was null (Nothing in Visual Basic).</exception>
        [DebuggerStepThrough]
        public void Start<TStateMachine>(ref TStateMachine stateMachine) where TStateMachine : IAsyncStateMachine
        {
            m_coreState.Start(ref stateMachine); // argument validation handled by AsyncMethodBuilderCore
        }

        /// <summary>Associates the builder with the state machine it represents.</summary>
        /// <param name="stateMachine">The heap-allocated state machine object.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="stateMachine"/> argument was null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.InvalidOperationException">The builder is incorrectly initialized.</exception>
        public void SetStateMachine(IAsyncStateMachine stateMachine)
        {
            m_coreState.SetStateMachine(stateMachine); // argument validation handled by AsyncMethodBuilderCore
        }

        /// <summary>Perform any initialization necessary prior to lifting the builder to the heap.</summary>
        void IAsyncMethodBuilder.PreBoxInitialization() { /* no initialization is necessary for AsyncVoidMethodBuilder */ }

        /// <summary>
        /// Schedules the specified state machine to be pushed forward when the specified awaiter completes.
        /// </summary>
        /// <typeparam name="TAwaiter">Specifies the type of the awaiter.</typeparam>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="awaiter">The awaiter.</param>
        /// <param name="stateMachine">The state machine.</param>
        public void AwaitOnCompleted<TAwaiter, TStateMachine>(
            ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : INotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            try
            {
                var continuation = m_coreState.GetCompletionAction(ref this, ref stateMachine);
                Contract.Assert(continuation != null, "GetCompletionAction should always return a valid action.");
                awaiter.OnCompleted(continuation);
            }
            catch (Exception exc)
            {
                // Prevent exceptions from leaking to the call site, which could
                // then allow multiple flows of execution through the same async method
                // if the awaiter had already scheduled the continuation by the time
                // the exception was thrown.  We propagate the exception on the
                // ThreadPool because we can trust it to not throw, unlike
                // if we were to go to a user-supplied SynchronizationContext,
                // whose Post method could easily throw.
                AsyncServices.ThrowAsync(exc, targetContext: null);
            }
        }

        /// <summary>
        /// Schedules the specified state machine to be pushed forward when the specified awaiter completes.
        /// </summary>
        /// <typeparam name="TAwaiter">Specifies the type of the awaiter.</typeparam>
        /// <typeparam name="TStateMachine">Specifies the type of the state machine.</typeparam>
        /// <param name="awaiter">The awaiter.</param>
        /// <param name="stateMachine">The state machine.</param>
        public void AwaitUnsafeOnCompleted<TAwaiter, TStateMachine>(
            ref TAwaiter awaiter, ref TStateMachine stateMachine)
            where TAwaiter : ICriticalNotifyCompletion
            where TStateMachine : IAsyncStateMachine
        {
            try
            {
                var continuation = m_coreState.GetCompletionAction(ref this, ref stateMachine);
                Contract.Assert(continuation != null, "GetCompletionAction should always return a valid action.");
                awaiter.UnsafeOnCompleted(continuation);
            }
            catch (Exception e)
            {
                AsyncServices.ThrowAsync(e, targetContext: null);
            }
        }

        /// <summary>Completes the method builder successfully.</summary>
        public void SetResult()
        {
            if (m_synchronizationContext != null)
            {
                NotifySynchronizationContextOfCompletion();
            }
        }

        /// <summary>Faults the method builder with an exception.</summary>
        /// <param name="exception">The exception that is the cause of this fault.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="exception"/> argument is null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.InvalidOperationException">The builder is not initialized.</exception>
        public void SetException(Exception exception)
        {
            if (exception == null) throw new ArgumentNullException("exception");
            Contract.EndContractBlock();

            if (m_synchronizationContext != null)
            {
                // If we captured a synchronization context, Post the throwing of the exception to it 
                // and decrement its outstanding operation count.
                try
                {
                    AsyncServices.ThrowAsync(exception, targetContext: m_synchronizationContext);
                }
                finally
                {
                    NotifySynchronizationContextOfCompletion();
                }
            }
            else
            {
                // Otherwise, queue the exception to be thrown on the ThreadPool.  This will
                // result in a crash unless legacy exception behavior is enabled by a config
                // file or a CLR host.
                AsyncServices.ThrowAsync(exception, targetContext: null);
            }
        }

        /// <summary>Notifies the current synchronization context that the operation completed.</summary>
        private void NotifySynchronizationContextOfCompletion()
        {
            Contract.Assert(m_synchronizationContext != null, "Must only be used with a non-null context.");
            try
            {
                m_synchronizationContext.OperationCompleted();
            }
            catch (Exception exc)
            {
                // If the interaction with the SynchronizationContext goes awry,
                // fall back to propagating on the ThreadPool.
                AsyncServices.ThrowAsync(exc, targetContext: null);
            }
        }

        /// <summary>
        /// Gets an object that may be used to uniquely identify this builder to the debugger.
        /// </summary>
        /// <remarks>
        /// This property lazily instantiates the ID in a non-thread-safe manner.  
        /// It must only be used by the debugger and only in a single-threaded manner.
        /// </remarks>
        private object ObjectIdForDebugger
        {
            get
            {
                if (m_objectIdForDebugger == null) m_objectIdForDebugger = new object();
                return m_objectIdForDebugger;
            }
        }
    }

}
