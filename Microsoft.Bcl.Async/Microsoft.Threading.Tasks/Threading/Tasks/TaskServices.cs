using System.ComponentModel;

namespace System.Threading.Tasks
{
    internal class TaskServices
    {
        /// <summary>Returns a canceled task.</summary>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>The canceled task.</returns>
        public static Task FromCancellation(System.Threading.CancellationToken cancellationToken)
        {
            if (!cancellationToken.IsCancellationRequested) throw new System.ArgumentOutOfRangeException("cancellationToken");
            return new Task(() => { }, cancellationToken);
        }

        /// <summary>Returns a canceled task.</summary>
        /// <typeparam name="TResult">Specifies the type of the result.</typeparam>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>The canceled task.</returns>
        public static Task<TResult> FromCancellation<TResult>(System.Threading.CancellationToken cancellationToken)
        {
            if (!cancellationToken.IsCancellationRequested) throw new System.ArgumentOutOfRangeException("cancellationToken");
            return new Task<TResult>(() => default(TResult), cancellationToken);
        }

        /// <summary>
        /// Completes the Task if the user state matches the TaskCompletionSource.
        /// </summary>
        /// <typeparam name="T">Specifies the type of data returned by the Task.</typeparam>
        /// <param name="tcs">The TaskCompletionSource.</param>
        /// <param name="e">The completion event arguments.</param>
        /// <param name="requireMatch">Whether we require the tcs to match the e.UserState.</param>
        /// <param name="getResult">A function that gets the result with which to complete the task.</param>
        /// <param name="unregisterHandler">An action used to unregister work when the operaiton completes.</param>
        public static void HandleEapCompletion<T>(
            TaskCompletionSource<T> tcs, bool requireMatch, AsyncCompletedEventArgs e, Func<T> getResult, Action unregisterHandler)
        {
            // Transfers the results from the AsyncCompletedEventArgs and getResult() to the
            // TaskCompletionSource, but only AsyncCompletedEventArg's UserState matches the TCS
            // (this check is important if the same WebClient is used for multiple, asynchronous
            // operations concurrently).  Also unregisters the handler to avoid a leak.
            if (!requireMatch || e.UserState == tcs)
            {
                try { unregisterHandler(); }
                finally
                {
                    if (e.Cancelled) tcs.TrySetCanceled();
                    else if (e.Error != null) tcs.TrySetException(e.Error);
                    else tcs.TrySetResult(getResult());
                }
            }
        }
    }
}
