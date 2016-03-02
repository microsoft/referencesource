using Microsoft.Runtime.CompilerServices;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Diagnostics.Contracts;
using System.Linq;

namespace System.Threading.Tasks
{
    /// <summary>Provides methods for creating and manipulating tasks.</summary>
    public static class TaskEx
    {
        #region Run
        /// <summary>Creates a task that runs the specified action.</summary>
        /// <param name="action">The action to execute asynchronously.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="action"/> argument is null.</exception>
        public static Task Run(Action action)
        {
            return Run(action, CancellationToken.None);
        }

        /// <summary>Creates a task that runs the specified action.</summary>
        /// <param name="action">The action to execute.</param>
        /// <param name="cancellationToken">The CancellationToken to use to request cancellation of this task.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="action"/> argument is null.</exception>
        public static Task Run(Action action, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(action, cancellationToken, TaskCreationOptions.None, TaskScheduler.Default);
        }

        /// <summary>Creates a task that runs the specified function.</summary>
        /// <param name="function">The function to execute asynchronously.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="function"/> argument is null.</exception>
        public static Task<TResult> Run<TResult>(Func<TResult> function)
        {
            return Run<TResult>(function, CancellationToken.None);
        }

        /// <summary>Creates a task that runs the specified function.</summary>
        /// <param name="function">The action to execute.</param>
        /// <param name="cancellationToken">The CancellationToken to use to cancel the task.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="function"/> argument is null.</exception>
        public static Task<TResult> Run<TResult>(Func<TResult> function, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(function, cancellationToken, TaskCreationOptions.None, TaskScheduler.Default);
        }

        /// <summary>Creates a task that runs the specified function.</summary>
        /// <param name="function">The action to execute asynchronously.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="function"/> argument is null.</exception>
        public static Task Run(Func<Task> function)
        {
            return Run(function, CancellationToken.None);
        }

        /// <summary>Creates a task that runs the specified function.</summary>
        /// <param name="function">The function to execute.</param>
        /// <param name="cancellationToken">The CancellationToken to use to request cancellation of this task.</param>
        /// <returns>A task that represents the completion of the function.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="function"/> argument is null.</exception>
        public static Task Run(Func<Task> function, CancellationToken cancellationToken)
        {
            return Run<Task>(function, cancellationToken).Unwrap();
        }

        /// <summary>Creates a task that runs the specified function.</summary>
        /// <param name="function">The function to execute asynchronously.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="function"/> argument is null.</exception>
        public static Task<TResult> Run<TResult>(Func<Task<TResult>> function)
        {
            return Run(function, CancellationToken.None);
        }

        /// <summary>Creates a task that runs the specified function.</summary>
        /// <param name="function">The action to execute.</param>
        /// <param name="cancellationToken">The CancellationToken to use to cancel the task.</param>
        /// <returns>A task that represents the completion of the action.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="function"/> argument is null.</exception>
        public static Task<TResult> Run<TResult>(Func<Task<TResult>> function, CancellationToken cancellationToken)
        {
            return Run<Task<TResult>>(function, cancellationToken).Unwrap();
        }
        #endregion

        #region Delay
        /// <summary>Starts a Task that will complete after the specified due time.</summary>
        /// <param name="dueTime">The delay in milliseconds before the returned task completes.</param>
        /// <returns>The timed Task.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// The <paramref name="dueTime"/> argument must be non-negative or -1 and less than or equal to Int32.MaxValue.
        /// </exception>
        public static Task Delay(int dueTime)
        {
            return Delay(dueTime, CancellationToken.None);
        }

        /// <summary>Starts a Task that will complete after the specified due time.</summary>
        /// <param name="dueTime">The delay before the returned task completes.</param>
        /// <returns>The timed Task.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// The <paramref name="dueTime"/> argument must be non-negative or -1 and less than or equal to Int32.MaxValue.
        /// </exception>
        public static Task Delay(TimeSpan dueTime)
        {
            return Delay(dueTime, CancellationToken.None);
        }

        /// <summary>Starts a Task that will complete after the specified due time.</summary>
        /// <param name="dueTime">The delay before the returned task completes.</param>
        /// <param name="cancellationToken">A CancellationToken that may be used to cancel the task before the due time occurs.</param>
        /// <returns>The timed Task.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// The <paramref name="dueTime"/> argument must be non-negative or -1 and less than or equal to Int32.MaxValue.
        /// </exception>
        public static Task Delay(TimeSpan dueTime, CancellationToken cancellationToken)
        {
            long totalMilliseconds = (long)dueTime.TotalMilliseconds;
            if (totalMilliseconds < -1 || totalMilliseconds > Int32.MaxValue) throw new ArgumentOutOfRangeException("dueTime", ArgumentOutOfRange_TimeoutNonNegativeOrMinusOne);
            Contract.EndContractBlock();
            return Delay((int)totalMilliseconds, cancellationToken);
        }



        /// <summary>Starts a Task that will complete after the specified due time.</summary>
        /// <param name="dueTime">The delay in milliseconds before the returned task completes.</param>
        /// <param name="cancellationToken">A CancellationToken that may be used to cancel the task before the due time occurs.</param>
        /// <returns>The timed Task.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// The <paramref name="dueTime"/> argument must be non-negative or -1 and less than or equal to Int32.MaxValue.
        /// </exception>
        public static Task Delay(int dueTime, CancellationToken cancellationToken)
        {
            // Validate arguments
            if (dueTime < -1) throw new ArgumentOutOfRangeException("dueTime", ArgumentOutOfRange_TimeoutNonNegativeOrMinusOne);
            Contract.EndContractBlock();

            // Fast-paths if the timeout has expired or the token has had cancellation requested
            if (cancellationToken.IsCancellationRequested) return new Task(() => { }, cancellationToken);
            if (dueTime == 0) return s_preCompletedTask;

            // Create the timed task
            var tcs = new TaskCompletionSource<bool>();
            var ctr = default(CancellationTokenRegistration);

            Timer timer = null;
            // Create the timer but don't start it yet.  If we start it now,
            // it might fire before ctr has been set to the right registration.
            timer = new Timer(state =>
            {
                // Clean up both the cancellation token and the timer, and try to transition to completed
                ctr.Dispose();
                timer.Dispose();
                tcs.TrySetResult(true);

                TimerManager.Remove(timer);

            }, null, Timeout.Infinite, Timeout.Infinite);

            // Make sure the timer stays rooted.  The full Framework
            // does this automatically when constructing a timer with the simple
            // overload that doesn't take state, in which case the timer itself
            // is the state.  But Compact Framework doesn't root, so we need to.
            TimerManager.Add(timer);

            // Register with the cancellation token if a cancelable one was provided.  This must be
            // done after initializing timer, as we call timer.Dispose from within the callback
            // and that callback will fire synchronously if the token has already been canceled
            // by the time we call Register.
            if (cancellationToken.CanBeCanceled)
            {
                // When cancellation occurs, cancel the timer and try to transition to canceled.
                // There could be a race, but it's benign.
                ctr = cancellationToken.Register(() =>
                {
                    timer.Dispose();
                    tcs.TrySetCanceled();
                    TimerManager.Remove(timer);
                });
            }

            // Start the timer and hand back the task...
            timer.Change(dueTime, Timeout.Infinite);
            return tcs.Task;
        }

        private const string ArgumentOutOfRange_TimeoutNonNegativeOrMinusOne = "The timeout must be non-negative or -1, and it must be less than or equal to Int32.MaxValue.";

        /// <summary>An already completed task.</summary>
        private static Task s_preCompletedTask = TaskEx.FromResult(false);
        #endregion

        #region WhenAll
        /// <summary>Creates a Task that will complete only when all of the provided collection of Tasks has completed.</summary>
        /// <param name="tasks">The Tasks to monitor for completion.</param>
        /// <returns>A Task that represents the completion of all of the provided tasks.</returns>
        /// <remarks>
        /// If any of the provided Tasks faults, the returned Task will also fault, and its Exception will contain information
        /// about all of the faulted tasks.  If no Tasks fault but one or more Tasks is canceled, the returned
        /// Task will also be canceled.
        /// </remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        public static Task WhenAll(params Task[] tasks)
        {
            return WhenAll((IEnumerable<Task>)tasks);
        }

        /// <summary>Creates a Task that will complete only when all of the provided collection of Tasks has completed.</summary>
        /// <param name="tasks">The Tasks to monitor for completion.</param>
        /// <returns>A Task that represents the completion of all of the provided tasks.</returns>
        /// <remarks>
        /// If any of the provided Tasks faults, the returned Task will also fault, and its Exception will contain information
        /// about all of the faulted tasks.  If no Tasks fault but one or more Tasks is canceled, the returned
        /// Task will also be canceled.
        /// </remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        [SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures")]
        public static Task<TResult[]> WhenAll<TResult>(params Task<TResult>[] tasks)
        {
            return WhenAll((IEnumerable<Task<TResult>>)tasks);
        }

        /// <summary>Creates a Task that will complete only when all of the provided collection of Tasks has completed.</summary>
        /// <param name="tasks">The Tasks to monitor for completion.</param>
        /// <returns>A Task that represents the completion of all of the provided tasks.</returns>
        /// <remarks>
        /// If any of the provided Tasks faults, the returned Task will also fault, and its Exception will contain information
        /// about all of the faulted tasks.  If no Tasks fault but one or more Tasks is canceled, the returned
        /// Task will also be canceled.
        /// </remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        public static Task WhenAll(IEnumerable<Task> tasks)
        {
            return WhenAllCore<object>(tasks, (completedTasks, tcs) =>
                tcs.TrySetResult(null));
        }

        /// <summary>Creates a Task that will complete only when all of the provided collection of Tasks has completed.</summary>
        /// <param name="tasks">The Tasks to monitor for completion.</param>
        /// <returns>A Task that represents the completion of all of the provided tasks.</returns>
        /// <remarks>
        /// If any of the provided Tasks faults, the returned Task will also fault, and its Exception will contain information
        /// about all of the faulted tasks.  If no Tasks fault but one or more Tasks is canceled, the returned
        /// Task will also be canceled.
        /// </remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        [SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures")]
        public static Task<TResult[]> WhenAll<TResult>(IEnumerable<Task<TResult>> tasks)
        {
            return WhenAllCore<TResult[]>(tasks.Cast<Task>(), (completedTasks, tcs) =>
                tcs.TrySetResult(completedTasks.Select(t => ((Task<TResult>)t).Result).ToArray()));
        }

        /// <summary>Creates a Task that will complete only when all of the provided collection of Tasks has completed.</summary>
        /// <param name="tasks">The Tasks to monitor for completion.</param>
        /// <param name="setResultAction">
        /// A callback invoked when all of the tasks complete successfully in the RanToCompletion state.
        /// This callback is responsible for storing the results into the TaskCompletionSource.
        /// </param>
        /// <returns>A Task that represents the completion of all of the provided tasks.</returns>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        private static Task<TResult> WhenAllCore<TResult>(
            IEnumerable<Task> tasks,
            Action<Task[], TaskCompletionSource<TResult>> setResultAction)
        {
            // Validate arguments
            if (tasks == null) throw new ArgumentNullException("tasks");
            Contract.EndContractBlock();
            Contract.Assert(setResultAction != null);

            // Create a TCS to represent the completion of all of the tasks.  This TCS's Task is
            // completed by a ContinueWhenAll continuation
            var tcs = new TaskCompletionSource<TResult>();
            var taskArr = tasks as Task[] ?? tasks.ToArray();
            if (taskArr.Length == 0)
            {
                setResultAction(taskArr, tcs);
            }
            else
            {
                Task.Factory.ContinueWhenAll(taskArr, completedTasks =>
                {
                    // Get exceptions for any faulted or canceled tasks
                    List<Exception> exceptions = null;
                    bool canceled = false;
                    foreach (var task in completedTasks)
                    {
                        if (task.IsFaulted) AddPotentiallyUnwrappedExceptions(ref exceptions, task.Exception);
                        else if (task.IsCanceled) canceled = true;
                    }

                    // Set up the resulting task.
                    if (exceptions != null && exceptions.Count > 0) tcs.TrySetException(exceptions);
                    else if (canceled) tcs.TrySetCanceled();
                    else setResultAction(completedTasks, tcs);
                }, CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
            }

            // Return the resulting task
            return tcs.Task;
        }
        #endregion

        #region WhenAny
        /// <summary>Creates a Task that will complete when any of the tasks in the provided collection completes.</summary>
        /// <param name="tasks">The Tasks to be monitored.</param>
        /// <returns>
        /// A Task that represents the completion of any of the provided Tasks.  The completed Task is this Task's result.
        /// </returns>
        /// <remarks>Any Tasks that fault will need to have their exceptions observed elsewhere.</remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        public static Task<Task> WhenAny(params Task[] tasks)
        {
            return WhenAny((IEnumerable<Task>)tasks);
        }

        /// <summary>Creates a Task that will complete when any of the tasks in the provided collection completes.</summary>
        /// <param name="tasks">The Tasks to be monitored.</param>
        /// <returns>
        /// A Task that represents the completion of any of the provided Tasks.  The completed Task is this Task's result.
        /// </returns>
        /// <remarks>Any Tasks that fault will need to have their exceptions observed elsewhere.</remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        public static Task<Task> WhenAny(IEnumerable<Task> tasks)
        {
            // Validate arguments
            if (tasks == null) throw new ArgumentNullException("tasks");
            Contract.EndContractBlock();

            // Create a task that will complete with 
            var tcs = new TaskCompletionSource<Task>();
            Task.Factory.ContinueWhenAny(tasks as Task[] ?? tasks.ToArray(),
                completed => tcs.TrySetResult(completed),
                CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
            return tcs.Task;
        }

        /// <summary>Creates a Task that will complete when any of the tasks in the provided collection completes.</summary>
        /// <param name="tasks">The Tasks to be monitored.</param>
        /// <returns>
        /// A Task that represents the completion of any of the provided Tasks.  The completed Task is this Task's result.
        /// </returns>
        /// <remarks>Any Tasks that fault will need to have their exceptions observed elsewhere.</remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        [SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures")]
        public static Task<Task<TResult>> WhenAny<TResult>(params Task<TResult>[] tasks)
        {
            return WhenAny((IEnumerable<Task<TResult>>)tasks);
        }

        /// <summary>Creates a Task that will complete when any of the tasks in the provided collection completes.</summary>
        /// <param name="tasks">The Tasks to be monitored.</param>
        /// <returns>
        /// A Task that represents the completion of any of the provided Tasks.  The completed Task is this Task's result.
        /// </returns>
        /// <remarks>Any Tasks that fault will need to have their exceptions observed elsewhere.</remarks>
        /// <exception cref="System.ArgumentNullException">The <paramref name="tasks"/> argument is null.</exception>
        /// <exception cref="System.ArgumentException">The <paramref name="tasks"/> argument contains a null reference.</exception>
        [SuppressMessage("Microsoft.Design", "CA1006:DoNotNestGenericTypesInMemberSignatures")]
        public static Task<Task<TResult>> WhenAny<TResult>(IEnumerable<Task<TResult>> tasks)
        {
            // Validate arguments
            if (tasks == null) throw new ArgumentNullException("tasks");
            Contract.EndContractBlock();

            // Create a task that will complete with 
            var tcs = new TaskCompletionSource<Task<TResult>>();
            Task.Factory.ContinueWhenAny(tasks as Task<TResult>[] ?? tasks.ToArray(),
                completed => tcs.TrySetResult(completed),
                CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
            return tcs.Task;
        }
        #endregion

        #region FromResult
        /// <summary>Creates an already completed <see cref="Task{TResult}"/> from the specified result.</summary>
        /// <param name="result">The result from which to create the completed task.</param>
        /// <returns>The completed task.</returns>
        public static Task<TResult> FromResult<TResult>(TResult result)
        {
            var tcs = new TaskCompletionSource<TResult>(result);
            tcs.TrySetResult(result);
            return tcs.Task;
        }
        #endregion

        #region Yield
        /// <summary>Creates an awaitable that asynchronously yields back to the current context when awaited.</summary>
        /// <returns>
        /// A context that, when awaited, will asynchronously transition back into the current context.
        /// If SynchronizationContext.Current is non-null, that is treated as the current context.
        /// Otherwise, TaskScheduler.Current is treated as the current context.
        /// </returns>
        public static YieldAwaitable Yield() { return new YieldAwaitable(); }
        #endregion

        #region Private Helpers
        /// <summary>Adds the target exception to the list, initializing the list if it's null.</summary>
        /// <param name="targetList">The list to which to add the exception and initialize if the list is null.</param>
        /// <param name="exception">The exception to add, and unwrap if it's an aggregate.</param>
        private static void AddPotentiallyUnwrappedExceptions(ref List<Exception> targetList, Exception exception)
        {
            var ae = exception as AggregateException;

            Contract.Assert(exception != null);
            Contract.Assert(ae == null || ae.InnerExceptions.Count > 0);

            // Initialize the list if necessary
            if (targetList == null) targetList = new List<Exception>();

            // If the exception is an aggregate and it contains only one exception, add just its inner exception
            if (ae != null)
            {
                targetList.Add(ae.InnerExceptions.Count == 1 ? exception.InnerException : exception);
            }
            // Otherwise, add the exception
            else targetList.Add(exception);
        }
        #endregion
    }
}
