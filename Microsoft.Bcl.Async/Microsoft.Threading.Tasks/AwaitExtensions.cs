using System;
using System.Diagnostics.Contracts;
using System.Threading;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using Microsoft.Runtime.CompilerServices;

/// <summary>
///     Provides extension methods for threading-related types.
/// </summary>
public static class AwaitExtensions
{
    /// <summary>Cancels the <see cref="System.Threading.CancellationTokenSource"/> after the specified duration.</summary>
    /// <param name="source">The CancellationTokenSource.</param>
    /// <param name="dueTime">The due time in milliseconds for the source to be canceled.</param>
    public static void CancelAfter(this CancellationTokenSource source, int dueTime)
    {
        if (source == null) throw new NullReferenceException();
        if (dueTime < -1) throw new ArgumentOutOfRangeException("dueTime");
        Contract.EndContractBlock();

        Timer timer = null;
        timer = new Timer(state =>
        {
            ((IDisposable)timer).Dispose();
            TimerManager.Remove(timer);
            try { source.Cancel(); }
            catch (ObjectDisposedException) { }
        }, null, Timeout.Infinite, Timeout.Infinite);

        TimerManager.Add(timer);
        timer.Change(dueTime, Timeout.Infinite);
    }

    /// <summary>Cancels the <see cref="System.Threading.CancellationTokenSource"/> after the specified duration.</summary>
    /// <param name="source">The CancellationTokenSource.</param>
    /// <param name="dueTime">The due time for the source to be canceled.</param>
    public static void CancelAfter(this CancellationTokenSource source, TimeSpan dueTime)
    {
        long milliseconds = (long)dueTime.TotalMilliseconds;
        if (milliseconds < -1 || milliseconds > Int32.MaxValue) throw new ArgumentOutOfRangeException("dueTime");
        CancelAfter(source, (int)milliseconds);
    }

    /// <summary>Gets an awaiter used to await this <see cref="System.Threading.Tasks.Task"/>.</summary>
    /// <param name="task">The task to await.</param>
    /// <returns>An awaiter instance.</returns>
    public static TaskAwaiter GetAwaiter(this Task task)
    {
        if (task == null) throw new ArgumentNullException("task");
        return new TaskAwaiter(task);
    }

    /// <summary>Gets an awaiter used to await this <see cref="System.Threading.Tasks.Task"/>.</summary>
    /// <typeparam name="TResult">Specifies the type of data returned by the task.</typeparam>
    /// <param name="task">The task to await.</param>
    /// <returns>An awaiter instance.</returns>
    public static TaskAwaiter<TResult> GetAwaiter<TResult>(this Task<TResult> task)
    {
        if (task == null) throw new ArgumentNullException("task");
        return new TaskAwaiter<TResult>(task);
    }

    /// <summary>Creates and configures an awaitable object for awaiting the specified task.</summary>
    /// <param name="task">The task to be awaited.</param>
    /// <param name="continueOnCapturedContext">
    /// true to automatic marshag back to the original call site's current SynchronizationContext
    /// or TaskScheduler; otherwise, false.
    /// </param>
    /// <returns>The instance to be awaited.</returns>
    public static ConfiguredTaskAwaitable ConfigureAwait(this Task task, bool continueOnCapturedContext)
    {
        if (task == null) throw new ArgumentNullException("task");
        return new ConfiguredTaskAwaitable(task, continueOnCapturedContext);
    }

    /// <summary>Creates and configures an awaitable object for awaiting the specified task.</summary>
    /// <param name="task">The task to be awaited.</param>
    /// <param name="continueOnCapturedContext">
    /// true to automatic marshag back to the original call site's current SynchronizationContext
    /// or TaskScheduler; otherwise, false.
    /// </param>
    /// <returns>The instance to be awaited.</returns>
    public static ConfiguredTaskAwaitable<TResult> ConfigureAwait<TResult>(this Task<TResult> task, bool continueOnCapturedContext)
    {
        if (task == null) throw new ArgumentNullException("task");
        return new ConfiguredTaskAwaitable<TResult>(task, continueOnCapturedContext);
    }
}
