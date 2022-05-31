using System;

namespace System.Runtime.CompilerServices
{
    /// <summary>
    /// Represents an operation that will schedule continuations when the operation completes.
    /// </summary>
    public interface INotifyCompletion
    {
        /// <summary>Schedules the continuation action to be invoked when the instance completes.</summary>
        /// <param name="continuation">The action to invoke when the operation completes.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="continuation"/> argument is null (Nothing in Visual Basic).</exception>
        void OnCompleted(Action continuation);
    }
}
