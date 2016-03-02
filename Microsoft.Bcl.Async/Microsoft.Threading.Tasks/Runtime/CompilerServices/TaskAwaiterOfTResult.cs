// NOTE: The reason this type does exist in the BCL System.Threading.Tasks contract is because we need to be able to construct one of these in the AwaitExtensions 
// class. The equivalent type in the current platform does not have an accessible constructor, hence the AwaitExtensions would fail when run on platforms
// where System.Threading.Tasks gets unified.
using System;
using System.Diagnostics.Contracts;
using System.Runtime.CompilerServices;
using System.Security;
using System.Threading.Tasks;

namespace Microsoft.Runtime.CompilerServices
{
    /// <summary>Provides an awaiter for awaiting a <see cref="System.Threading.Tasks.Task{TResult}"/>.</summary>
    /// <remarks>This type is intended for compiler use only.</remarks>
    public struct TaskAwaiter<TResult> : ICriticalNotifyCompletion
    {
        /// <summary>The task being awaited.</summary>
        private readonly Task<TResult> m_task;

        /// <summary>Initializes the <see cref="TaskAwaiter{TResult}"/>.</summary>
        /// <param name="task">The <see cref="System.Threading.Tasks.Task{TResult}"/> to be awaited.</param>
        internal TaskAwaiter(Task<TResult> task)
        {
            Contract.Assert(task != null);
            m_task = task;
        }

        /// <summary>Gets whether the task being awaited is completed.</summary>
        /// <remarks>This property is intended for compiler user rather than use directly in code.</remarks>
        /// <exception cref="System.NullReferenceException">The awaiter was not properly initialized.</exception>
        public bool IsCompleted { get { return m_task.IsCompleted; } }

        /// <summary>Schedules the continuation onto the <see cref="System.Threading.Tasks.Task"/> associated with this <see cref="TaskAwaiter"/>.</summary>
        /// <param name="continuation">The action to invoke when the await operation completes.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="continuation"/> argument is null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.NullReferenceException">The awaiter was not properly initialized.</exception>
        /// <remarks>This method is intended for compiler user rather than use directly in code.</remarks>
        public void OnCompleted(Action continuation)
        {
            TaskAwaiter.OnCompletedInternal(m_task, continuation, TaskAwaiter.CONTINUE_ON_CAPTURED_CONTEXT_DEFAULT);
        }

        /// <summary>Schedules the continuation onto the <see cref="System.Threading.Tasks.Task"/> associated with this <see cref="TaskAwaiter"/>.</summary>
        /// <param name="continuation">The action to invoke when the await operation completes.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="continuation"/> argument is null (Nothing in Visual Basic).</exception>
        /// <exception cref="System.InvalidOperationException">The awaiter was not properly initialized.</exception>
        /// <remarks>This method is intended for compiler user rather than use directly in code.</remarks>
#if !SILVERLIGHT
        //[SecurityCritical]
#endif
        public void UnsafeOnCompleted(Action continuation)
        {
            TaskAwaiter.OnCompletedInternal(m_task, continuation, TaskAwaiter.CONTINUE_ON_CAPTURED_CONTEXT_DEFAULT);
        }

        /// <summary>Ends the await on the completed <see cref="System.Threading.Tasks.Task{TResult}"/>.</summary>
        /// <returns>The result of the completed <see cref="System.Threading.Tasks.Task{TResult}"/>.</returns>
        /// <exception cref="System.NullReferenceException">The awaiter was not properly initialized.</exception>
        /// <exception cref="System.InvalidOperationException">The task was not yet completed.</exception>
        /// <exception cref="System.Threading.Tasks.TaskCanceledException">The task was canceled.</exception>
        /// <exception cref="System.Exception">The task completed in a Faulted state.</exception>
        public TResult GetResult()
        {
            TaskAwaiter.ValidateEnd(m_task);
            return m_task.Result;
        }
    }

}
