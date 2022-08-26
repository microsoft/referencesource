//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading;
    using System.Runtime;

    using Microsoft.Win32.SafeHandles;

    delegate void HandleLogFullCallback(uint errorCode);

    sealed class LogManagementAsyncResult : OverlappedAsyncResult
    {
        LogStore logStore;
        CLFS_MGMT_NOTIFICATION notification;

        bool registered = false;
        bool active = false;

        // CLFS will hang if the user doesn't advance the base of the log or report failure to 
        // to advance the base of the log in a tail pinned event. If user does any other append
        // in tail pinned event, the user will not be able to append subsequently. A conservative
        // timeout is used to break this hang in the event user ends up doing an append in tail pinned event.

        const int MaxLogFullNotificationTimeOut = 30000; // In milli seconds... arbitrary.

        List<HandleLogFullCallback> handleLogFullCallbackList;

        internal LogManagementAsyncResult(LogStore logStore)
            : base(null, null)
        {
            this.logStore = logStore;
            this.notification = new CLFS_MGMT_NOTIFICATION();
            this.handleLogFullCallbackList = null;
        }

        object SyncRoot
        {
            get { return this.notification; }
        }

        event EventHandler<TailPinnedEventArgs> InternalTailPinned;
        public event EventHandler<TailPinnedEventArgs> TailPinned
        {
            add
            {
                EnsureStarted();
                InternalTailPinned += value;
            }

            remove
            {
                InternalTailPinned -= value;
            }
        }

        public uint HandleLogFull(HandleLogFullCallback callback)
        {
            ManualResetEvent doneEvent = null;
            uint queuedErrorCode = Error.ERROR_SUCCESS;

            if (callback == null)
            {
                doneEvent = new ManualResetEvent(false);
                callback = delegate(uint code)
                {
                    queuedErrorCode = code;
                    doneEvent.Set();
                };
            }

            uint errorCode = Error.ERROR_IO_PENDING;
            lock (this.SyncRoot)
            {
                EnsureStarted();

                if (this.handleLogFullCallbackList == null)
                {
                    this.handleLogFullCallbackList = new List<HandleLogFullCallback>();

                    errorCode = UnsafeNativeMethods.HandleLogFull(
                        this.logStore.Handle);
                    if (errorCode != Error.ERROR_IO_PENDING)
                    {
                        return errorCode;
                    }
                }

                this.handleLogFullCallbackList.Add(callback);
            }

            if (doneEvent != null)
            {
                if (doneEvent.WaitOne(MaxLogFullNotificationTimeOut, false))
                {
                    // Got ClfsMgmtLogFullHandlerNotification notification from CLFS
                    errorCode = queuedErrorCode;
                }
                else
                {
                    // Time out! We never got ClfsMgmtLogFullHandlerNotification notification from CLFS!
                    // Remove the Callback from the Callback list. 
                    lock (this.SyncRoot)
                    {
                        this.handleLogFullCallbackList.Remove(callback);
                    }

                    errorCode = Error.ERROR_LOG_FULL;
                }

                doneEvent.Close();
            }

            return errorCode;
        }

        internal override void IOCompleted(uint errorCode)
        {
            // We get ERROR_OPERATION_ABORTED here if the handle is
            // closed.
            //
            if (errorCode == Error.ERROR_OPERATION_ABORTED)
                return;

            try
            {
                HandleNotification(errorCode);

                lock (this.SyncRoot)
                {
                    if (this.active)
                    {
                        // There is no synchronous return path for this call.
                        // We don't need to call Free() in a finally block, 
                        // because any exception will failfast the process
                        Pack(this.notification);
                        unsafe
                        {
                            UnsafeNativeMethods.ReadLogNotification(
                                this.logStore.Handle,
                                this.notification,
                                NativeOverlapped);
                        }
                    }
                }
            }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
            catch (Exception e)
            {
                // I don't know what to do with errors here... if I
                // can't re-issue a read, when will I get new
                // notifications?
                //
                // If ReadLogNotification fails, we cannot continue to process CLFS notifications.
                // There is really nothing we can do to continue executing correctly.
                DiagnosticUtility.InvokeFinalHandler(e);
            }
        }

        void EnsureStarted()
        {
            if (!this.registered)
            {
                UnsafeNativeMethods.RegisterManageableLogClient(
                    this.logStore.Handle,
                    IntPtr.Zero);

                this.registered = true;
            }

            if (!this.active)
            {
                bool throwing = true;
                try
                {
                    Pack(this.notification);
                    unsafe
                    {
                        UnsafeNativeMethods.ReadLogNotification(
                            this.logStore.Handle,
                            this.notification,
                            NativeOverlapped);
                    }
                    throwing = false;
                }
                finally
                {
                    if (throwing)
                    {
                        Free();
                    }
                }

                this.active = true;
            }
        }

        void HandleLogFullComplete(uint errorCode)
        {
            List<HandleLogFullCallback> callbacks;
            lock (this.SyncRoot)
            {
                callbacks = this.handleLogFullCallbackList;
                this.handleLogFullCallbackList = null;
            }

            try
            {
                foreach (HandleLogFullCallback callback in callbacks)
                {
                    callback(errorCode);
                }
            }
#pragma warning suppress 56500 // This is a callback exception
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                    throw;

                throw DiagnosticUtility.ExceptionUtility.ThrowHelperCallback(e);
            }
        }

        void HandleNotification(uint errorCode)
        {
            CLFS_MGMT_NOTIFICATION_TYPE nt;
            nt = (CLFS_MGMT_NOTIFICATION_TYPE)this.notification.Notification;

            switch (nt)
            {
                case CLFS_MGMT_NOTIFICATION_TYPE.ClfsMgmtAdvanceTailNotification:
                    if (errorCode == Error.ERROR_SUCCESS)
                    {
                        HandleTailPinned();
                    }
                    else
                    {
                        // Errr... what?
                    }
                    break;

                case CLFS_MGMT_NOTIFICATION_TYPE.ClfsMgmtLogFullHandlerNotification:
                    HandleLogFullComplete(errorCode);
                    break;

                case CLFS_MGMT_NOTIFICATION_TYPE.ClfsMgmtLogUnpinnedNotification:
                    // Don't do anything right now.
                    break;
            }
        }

        void HandleTailPinned()
        {
            TailPinnedEventArgs args;
            SequenceNumber targetLsn;

            targetLsn = new SequenceNumber(this.notification.Lsn);
            args = new TailPinnedEventArgs(targetLsn);

            EventHandler<TailPinnedEventArgs> handler;
            handler = this.InternalTailPinned;
            if (handler != null)
            {
                try
                {
                    handler(this, args);
                }
#pragma warning suppress 56500 // This is a callback exception
                catch (Exception e)
                {
                    try
                    {
                        int errorCode = Marshal.GetHRForException(e);
                        UnsafeNativeMethods.LogTailAdvanceFailure(this.logStore.Handle, errorCode);
                    }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
                    catch (Exception exception)
                    {
                        // If LogTailAdvanceFailure fails, there's nothing we can do to recover.
                        // If we keep executing, the log's tail will be pinned forever.
                        // CLFS will never know that we have finished with its notification.
                        DiagnosticUtility.InvokeFinalHandler(exception);
                    }

                    if (Fx.IsFatal(e))
                        throw;
                }
            }
            else
            {
                // User hasn't subscribed to TailPinnned. If RetryAppend is true, IO.Log will register for 
                // notification and invoke CLFS policy to handle LogFull Condition.
                // IO.Log must report to CLFS that the log tail cannot be advanced. CLFS expects either
                // base to be advanced or report the failure else it will hang waiting for the outcome.
                //

                try
                {
                    UnsafeNativeMethods.LogTailAdvanceFailure(this.logStore.Handle, 0);
                }
#pragma warning suppress 56500 // We will be terminating the process with any exception in this call
                catch (Exception exception)
                {
                    // If LogTailAdvanceFailure fails, there's nothing we can do to recover.
                    // If we keep executing, the log's tail will be pinned forever.
                    // CLFS will never know that we have finished with its notification.
                    DiagnosticUtility.InvokeFinalHandler(exception);
                }

            }
        }
    }
}
