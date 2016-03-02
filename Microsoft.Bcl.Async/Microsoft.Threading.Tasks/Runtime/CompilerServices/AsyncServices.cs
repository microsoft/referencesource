// NOTE: If you change this copy, please also change the copy under the BCL partition
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace System.Runtime.CompilerServices
{
    internal static class AsyncServices
    {
        /// <summary>Throws the exception on the ThreadPool.</summary>
        /// <param name="exception">The exception to propagate.</param>
        /// <param name="targetContext">The target context on which to propagate the exception.  Null to use the ThreadPool.</param>
        internal static void ThrowAsync(Exception exception, SynchronizationContext targetContext)
        {
            // If the user supplied a SynchronizationContext...
            if (targetContext != null)
            {
                try
                {
                    // Post the throwing of the exception to that context, and return.
                    targetContext.Post(state => { throw PrepareExceptionForRethrow((Exception)state); }, exception);
                    return;
                }
                catch (Exception postException)
                {
                    // If something goes horribly wrong in the Post, we'll 
                    // propagate both exceptions on the ThreadPool
                    exception = new AggregateException(exception, postException);
                }
            }

            // Propagate the exception(s) on the ThreadPool
            ThreadPool.QueueUserWorkItem(state => { throw PrepareExceptionForRethrow((Exception)state); }, exception);
        }

        /// <summary>Copies the exception's stack trace so its stack trace isn't overwritten.</summary>
        /// <param name="exc">The exception to prepare.</param>
        internal static Exception PrepareExceptionForRethrow(Exception exc)
        {
#if EXCEPTION_STACK_PRESERVE
            Contract.Assume(exc != null);
            if (s_prepForRemoting != null)
            {
                try { s_prepForRemoting.Invoke(exc, s_emptyParams); }
                catch { }
            }
#endif
            return exc;
        }

#if EXCEPTION_STACK_PRESERVE
        /// <summary>A MethodInfo for the Exception.PrepForRemoting method.</summary>
        private readonly static MethodInfo s_prepForRemoting = GetPrepForRemotingMethodInfo();
        /// <summary>An empty array to use with MethodInfo.Invoke.</summary>
        private readonly static Object[] s_emptyParams = new object[0];

        /// <summary>Gets the MethodInfo for the internal PrepForRemoting method on Exception.</summary>
        /// <returns>The MethodInfo if it could be retrieved, or else null.</returns>
        private static MethodInfo GetPrepForRemotingMethodInfo()
        {
            try
            {
                return typeof(Exception).GetMethod("PrepForRemoting", BindingFlags.NonPublic | BindingFlags.Instance);
            }
            catch { return null; }
        }
#endif
    }
}
