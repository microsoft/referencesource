using System;

namespace System.Runtime.CompilerServices
{
    /// <summary>
    /// Represents an awaiter used to schedule continuations when an await operation completes.
    /// </summary>
    public interface ICriticalNotifyCompletion : INotifyCompletion
    {
        /// <summary>Schedules the continuation action to be invoked when the instance completes.</summary>
        /// <param name="continuation">The action to invoke when the operation completes.</param>
        /// <exception cref="System.ArgumentNullException">The <paramref name="continuation"/> argument is null (Nothing in Visual Basic).</exception>
        /// <remarks>Unlike OnCompleted, UnsafeOnCompleted need not propagate ExecutionContext information.</remarks>
        void UnsafeOnCompleted(Action continuation);
    }

}
