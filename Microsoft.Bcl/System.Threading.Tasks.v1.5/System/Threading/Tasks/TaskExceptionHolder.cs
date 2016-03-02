// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//
// TaskExceptionHolder.cs
//
// <OWNER>hyildiz</OWNER>
//
// An abstraction for holding and aggregating exceptions.
//
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Disable the "reference to volatile field not treated as volatile" error.
#pragma warning disable 0420

namespace System.Threading.Tasks
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Contracts;

    /// <summary>
    /// An exception holder manages a list of exceptions for one particular task.
    /// It offers the ability to aggregate, but more importantly, also offers intrinsic
    /// support for propagating unhandled exceptions that are never observed. It does
    /// this by aggregating and throwing if the holder is ever GC'd without the holder's
    /// contents ever having been requested (e.g. by a Task.Wait, Task.get_Exception, etc).
    /// </summary>
    internal class TaskExceptionHolder
    {
        private List<Exception> m_exceptions; // One or more exceptions to be aggregated.
        private bool m_isHandled; // Whether the holder was observed.
        private Task m_task; // The task to which these exceptions belong.

        /// <summary>
        /// Creates a new holder; it will be registered for finalization.
        /// </summary>
        /// <param name="task">The task this holder belongs to.</param>
        internal TaskExceptionHolder(Task task)
        {
            Contract.Assert(task != null, "Expected a non-null task.");
            
            //EnsureADUnloadCallbackRegistered();

            m_exceptions = new List<Exception>(1);
            m_task = task;
        }

        //private static volatile bool s_domainUnloadStarted;
        //private static volatile EventHandler s_adUnloadEventHandler;

        //private static void EnsureADUnloadCallbackRegistered()
        //{
        //    if (s_adUnloadEventHandler == null && 
        //        Interlocked.CompareExchange( ref s_adUnloadEventHandler,
        //                                     new EventHandler(AppDomainUnloadCallback), 
        //                                     null) == null)
        //    {
        //    }
        //}

        //private static void AppDomainUnloadCallback(object sender, EventArgs e)
        //{
        //    s_domainUnloadStarted = true;
        //}

        /// <summary>
        /// A finalizer that repropagates unhandled exceptions.
        /// </summary>
        ~TaskExceptionHolder()
        {
            // Raise unhandled exceptions only when we know that neither the process or nor the appdomain is being torn down.
            // We need to do this filtering because all TaskExceptionHolders will be finalized during shutdown or unload
            // regardles of reachability of the task (i.e. even if the user code was about to observe the task's exception),
            // which can otherwise lead to spurious crashes during shutdown.
            if (!m_isHandled && !Environment.HasShutdownStarted)
            {
                // We don't want to crash the finalizer thread if any ThreadAbortExceptions 
                // occur in the list or in any nested AggregateExceptions.  
                // (Don't rethrow ThreadAbortExceptions.)
                foreach (Exception exp in m_exceptions)
                {
                    AggregateException aggExp = exp as AggregateException;
                    if (aggExp != null)
                    {
                        AggregateException flattenedAggExp = aggExp.Flatten();
                        foreach (Exception innerExp in flattenedAggExp.InnerExceptions)
                        {
                            if (ThreadingServices.IsThreadAbort(innerExp))
                                return;
                        }
                    }
                    else if (ThreadingServices.IsThreadAbort(exp))
                    {
                        return;
                    }
                }

                // We will only propagate if this is truly unhandled. The reason this could
                // ever occur is somewhat subtle: if a Task's exceptions are observed in some
                // other finalizer, and the Task was finalized before the holder, the holder
                // will have been marked as handled before even getting here.

                // Give users a chance to keep this exception from crashing the process
                
                // First, publish the unobserved exception and allow users to observe it
                AggregateException exceptionToThrow = new AggregateException(
                    Strings.TaskExceptionHolder_UnhandledException,
                    m_exceptions);
                UnobservedTaskExceptionEventArgs ueea = new UnobservedTaskExceptionEventArgs(exceptionToThrow);
                TaskScheduler.PublishUnobservedTaskException(m_task, ueea);
                
                //// Now, if we are still unobserved, throw the exception
                //if (!ueea.m_observed)
                //{
                //    throw exceptionToThrow;
                //}
            }
        }

        /// <summary>
        /// Add an exception to the internal list.  This will ensure the holder is
        /// in the proper state (handled/unhandled) depending on the list's contents.
        /// </summary>
        /// <param name="exceptionObject">An exception object (either an Exception or an 
        /// IEnumerable{Exception}) to add to the list.</param>
        internal void Add(object exceptionObject)
        {
            Contract.Assert(exceptionObject != null);
            Contract.Assert(m_exceptions != null);

            Contract.Assert(exceptionObject is Exception || exceptionObject is IEnumerable<Exception>,
                "TaskExceptionHolder.Add(): Expected Exception or IEnumerable<Exception>");

            Exception exception = exceptionObject as Exception;
            if (exception != null) m_exceptions.Add(exception);
            else
            {
                IEnumerable<Exception> exColl = exceptionObject as IEnumerable<Exception>;
                if (exColl != null) m_exceptions.AddRange(exColl);
                else
                {
                    throw new ArgumentException(Strings.TaskExceptionHolder_UnknownExceptionType,"exceptionObject");
                }
            }
                

            // If all of the exceptions are ThreadAbortExceptions and/or
            // AppDomainUnloadExceptions, we do not want the finalization
            // probe to propagate them, so we consider the holder to be
            // handled.  If a subsequent exception comes in of a different
            // kind, we will reactivate the holder.
            for (int i = 0; i < m_exceptions.Count; i++)
            {
                if (m_exceptions[i].GetType() != ThreadingServices.ThreadAbortExceptionType &&
                    m_exceptions[i].GetType() != ThreadingServices.AppDomainUnloadedExceptionType)
                {
                    MarkAsUnhandled();
                    break;
                }
                else if (i == m_exceptions.Count - 1)
                {
                    MarkAsHandled(false);
                }
            }
        }

        /// <summary>
        /// A private helper method that ensures the holder is considered
        /// unhandled, i.e. it is registered for finalization.
        /// </summary>
        private void MarkAsUnhandled()
        {
            // If a thread partially observed this thread's exceptions, we
            // should revert back to "not handled" so that subsequent exceptions
            // must also be seen. Otherwise, some could go missing. We also need
            // to reregister for finalization.
            if (m_isHandled)
            {
                GC.ReRegisterForFinalize(this);
                m_isHandled = false;
            }
        }

        /// <summary>
        /// A private helper method that ensures the holder is considered
        /// handled, i.e. it is not registered for finalization.
        /// </summary>
        /// <param name="calledFromFinalizer">Whether this is called from the finalizer thread.</param> 
        internal void MarkAsHandled(bool calledFromFinalizer)
        {
            if (!m_isHandled)
            {
                if (!calledFromFinalizer)
                {
                    GC.SuppressFinalize(this);
                }

                m_isHandled = true;
            }
        }

        /// <summary>
        /// Allocates a new aggregate exception and adds the contents of the list to
        /// it. By calling this method, the holder assumes exceptions to have been
        /// "observed", such that the finalization check will be subsequently skipped.
        /// </summary>
        /// <param name="calledFromFinalizer">Whether this is being called from a finalizer.</param>
        /// <param name="includeThisException">An extra exception to be included (optionally).</param>
        /// <returns>The aggregate exception to throw.</returns>
        internal AggregateException CreateExceptionObject(bool calledFromFinalizer, Exception includeThisException)
        {
            Contract.Assert(m_exceptions.Count > 0, "Expected at least one exception.");

            // Mark as handled and aggregate the exceptions.
            MarkAsHandled(calledFromFinalizer);

            List<Exception> exceptions = m_exceptions;

            // If the caller wants a specific exception to be included, add it now.
            if (includeThisException != null)
            {
                exceptions = new List<Exception>(exceptions);
                exceptions.Add(includeThisException);
            }

            // Manufacture the aggregate.
            return new AggregateException(exceptions);
        }

    }
}
