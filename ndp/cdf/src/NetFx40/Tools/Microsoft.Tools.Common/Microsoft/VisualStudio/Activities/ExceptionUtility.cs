// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.VisualStudio.Activities
{
    using System;
    using System.Reflection;
    using System.Threading;

    internal static class ExceptionUtility
    {
        public static bool IsFatal(Exception exception)
        {
            while (exception != null)
            {
                if (((exception is OutOfMemoryException) && !(exception is InsufficientMemoryException)) || (exception is ThreadAbortException))
                {
                    return true;
                }

                if ((exception is TypeInitializationException) || (exception is TargetInvocationException))
                {
                    exception = exception.InnerException;
                }
                else
                {
                    break;
                }
            }

            return false;
        }
    }
}