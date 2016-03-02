using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Runtime.CompilerServices
{
    /// <summary>Provides a base class used to cache tasks of a specific return type.</summary>
    /// <typeparam name="TResult">Specifies the type of results the cached tasks return.</typeparam>
    internal class AsyncMethodTaskCache<TResult>
    {
        /// <summary>
        /// A singleton cache for this result type.
        /// This may be null if there are no cached tasks for this TResult.
        /// </summary>
        internal readonly static AsyncMethodTaskCache<TResult> Singleton = CreateCache();

        /// <summary>Creates a non-disposable task.</summary>
        /// <param name="result">The result for the task.</param>
        /// <returns>The cacheable task.</returns>
        internal static TaskCompletionSource<TResult> CreateCompleted(TResult result)
        {
            var tcs = new TaskCompletionSource<TResult>();
            tcs.TrySetResult(result);
            return tcs;
        }

        /// <summary>Creates a cache.</summary>
        /// <returns>A task cache for this result type.</returns>
        private static AsyncMethodTaskCache<TResult> CreateCache()
        {
            // Get the result type
            var resultType = typeof(TResult);

            // Return a new cache for this particular kind of task.
            // If we don't have a specialized cache for this type, return null.
            if (resultType == typeof(Boolean))
            {
                return (AsyncMethodTaskCache<TResult>)(object)new AsyncMethodBooleanTaskCache();
            }
            else if (resultType == typeof(int))
            {
                return (AsyncMethodTaskCache<TResult>)(object)new AsyncMethodInt32TaskCache();
            }
            else return null;
        }

        /// <summary>Gets a cached task if one exists.</summary>
        /// <param name="result">The result for which we want a cached task.</param>
        /// <returns>A cached task if one exists; otherwise, null.</returns>
        internal virtual TaskCompletionSource<TResult> FromResult(TResult result)
        {
            return CreateCompleted(result);
        }

        /// <summary>Provides a cache for Boolean tasks.</summary>
        private sealed class AsyncMethodBooleanTaskCache : AsyncMethodTaskCache<Boolean>
        {
            /// <summary>A true task.</summary>
            internal readonly TaskCompletionSource<Boolean> m_true = CreateCompleted(true);
            /// <summary>A false task.</summary>
            internal readonly TaskCompletionSource<Boolean> m_false = CreateCompleted(false);

            /// <summary>Gets a cached task for the Boolean result.</summary>
            /// <param name="result">true or false</param>
            /// <returns>A cached task for the Boolean result.</returns>
            internal sealed override TaskCompletionSource<Boolean> FromResult(Boolean result)
            {
                return result ? m_true : m_false;
            }
        }

        /// <summary>Provides a cache for zero Int32 tasks.</summary>
        private sealed class AsyncMethodInt32TaskCache : AsyncMethodTaskCache<Int32>
        {
            /// <summary>The cache of Task{Int32}.</summary>
            internal readonly static TaskCompletionSource<Int32>[] Int32Tasks = CreateInt32Tasks();
            /// <summary>The minimum value, inclusive, for which we want a cached task.</summary>
            internal const Int32 INCLUSIVE_INT32_MIN = -1;
            /// <summary>The maximum value, exclusive, for which we want a cached task.</summary>
            internal const Int32 EXCLUSIVE_INT32_MAX = 9;
            /// <summary>Creates an array of cached tasks for the values in the range [INCLUSIVE_MIN,EXCLUSIVE_MAX).</summary>
            private static TaskCompletionSource<Int32>[] CreateInt32Tasks()
            {
                Contract.Assert(EXCLUSIVE_INT32_MAX >= INCLUSIVE_INT32_MIN, "Expected max to be at least min");
                var tasks = new TaskCompletionSource<Int32>[EXCLUSIVE_INT32_MAX - INCLUSIVE_INT32_MIN];
                for (int i = 0; i < tasks.Length; i++)
                {
                    tasks[i] = CreateCompleted(i + INCLUSIVE_INT32_MIN);
                }
                return tasks;
            }


            /// <summary>Gets a cached task for the zero Int32 result.</summary>
            /// <param name="result">The integer value</param>
            /// <returns>A cached task for the Int32 result or null if not cached.</returns>
            internal sealed override TaskCompletionSource<Int32> FromResult(Int32 result)
            {
                return (result >= INCLUSIVE_INT32_MIN && result < EXCLUSIVE_INT32_MAX) ?
                    Int32Tasks[result - INCLUSIVE_INT32_MIN] :
                    CreateCompleted(result);
            }
        }
    }

}
