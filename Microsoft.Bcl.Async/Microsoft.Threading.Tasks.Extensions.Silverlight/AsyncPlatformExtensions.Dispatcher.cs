using System;
using System.Net;
using System.Threading.Tasks;
using System.Windows;

/// <summary>
///     Provides asynchronous wrappers for .NET Framework operations.
/// </summary>
public static partial class AsyncPlatformExtensions
{
    /// <summary>Asynchronously invokes an Action on the Dispatcher.</summary>
    /// <param name="dispatcher">The Dispatcher.</param>
    /// <param name="action">The action to invoke.</param>
    /// <returns>A Task that represents the execution of the action.</returns>
    public static Task InvokeAsync(this System.Windows.Threading.Dispatcher dispatcher, Action action)
    {
        if (dispatcher == null) throw new ArgumentNullException("dispatcher");
        if (action == null) throw new ArgumentNullException("action");

        var tcs = new TaskCompletionSource<VoidTaskResult>();
        dispatcher.BeginInvoke(new Action(() =>
        {
            try
            {
                action();
                tcs.TrySetResult(default(VoidTaskResult));
            }
            catch (Exception exc)
            {
                tcs.TrySetException(exc);
            }
        }));
        return tcs.Task;
    }

    /// <summary>Asynchronously invokes an Action on the Dispatcher.</summary>
    /// <param name="dispatcher">The Dispatcher.</param>
    /// <param name="function">The function to invoke.</param>
    /// <returns>A Task that represents the execution of the function.</returns>
    public static Task<TResult> InvokeAsync<TResult>(this System.Windows.Threading.Dispatcher dispatcher, Func<TResult> function)
    {
        if (dispatcher == null) throw new ArgumentNullException("dispatcher");
        if (function == null) throw new ArgumentNullException("function");

        var tcs = new TaskCompletionSource<TResult>();
        dispatcher.BeginInvoke(new Action(() =>
        {
            try
            {
                var result = function();
                tcs.TrySetResult(result);
            }
            catch (Exception exc)
            {
                tcs.TrySetException(exc);
            }
        }));
        return tcs.Task;
    }
}

/// <summary>Used with Task(of void)</summary>
internal struct VoidTaskResult { }
