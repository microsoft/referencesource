// NOTE: The reason this type does exist in the BCL System.Threading.Tasks contract is because we need to be able to construct one of these in the AwaitExtensions 
// class. The equivalent type in the current platform does not have an accessible constructor, hence the AwaitExtensions would fail when run on platforms
// where System.Threading.Tasks gets unified.
using System;
using System.Runtime.CompilerServices;
using System.Security;
using System.Threading.Tasks;

namespace Microsoft.Runtime.CompilerServices
{
    /// <summary>Provides an awaitable context for switching into a target environment.</summary>
    /// <remarks>This type is intended for compiler use only.</remarks>
    public struct YieldAwaitable
    {
        /// <summary>Gets an awaiter for this <see cref="YieldAwaitable"/>.</summary>
        /// <returns>An awaiter for this awaitable.</returns>
        /// <remarks>This method is intended for compiler user rather than use directly in code.</remarks>
        public YieldAwaiter GetAwaiter() { return new YieldAwaiter(); } // if not initialized properly, m_target will just be null

        /// <summary>Provides an awaiter that switches into a target environment.</summary>
        /// <remarks>This type is intended for compiler use only.</remarks>
        public struct YieldAwaiter : ICriticalNotifyCompletion
        {
            /// <summary>A completed task.</summary>
            private readonly static Task s_completed = TaskEx.FromResult(0);

            /// <summary>Gets whether a yield is not required.</summary>
            /// <remarks>This property is intended for compiler user rather than use directly in code.</remarks>
            public bool IsCompleted { get { return false; } } // yielding is always required for YieldAwaiter, hence false

            /// <summary>Posts the <paramref name="continuation"/> back to the current context.</summary>
            /// <param name="continuation">The action to invoke asynchronously.</param>
            /// <exception cref="System.InvalidOperationException">The awaiter was not properly initialized.</exception>
            public void OnCompleted(Action continuation) { s_completed.GetAwaiter().OnCompleted(continuation); }

            /// <summary>Posts the <paramref name="continuation"/> back to the current context.</summary>
            /// <param name="continuation">The action to invoke asynchronously.</param>
            /// <exception cref="System.InvalidOperationException">The awaiter was not properly initialized.</exception>
            //[SecurityCritical]
            public void UnsafeOnCompleted(Action continuation) { s_completed.GetAwaiter().UnsafeOnCompleted(continuation); }

            /// <summary>Ends the await operation.</summary>
            public void GetResult() { } // Nop. It exists purely because the compiler pattern demands it.
        }
    }
}
