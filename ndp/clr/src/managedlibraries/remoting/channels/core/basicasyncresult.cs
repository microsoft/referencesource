// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
**  File:    BasicAsyncResult.cs
** 
**  Purpose: Base class for async result implementations.
**
**  Date:    Oct 21, 2001
**
===========================================================*/


using System;
using System.Threading;

namespace System.Runtime.Remoting.Channels
{

    internal class BasicAsyncResult : IAsyncResult
    {
        private AsyncCallback _asyncCallback;
        private Object _asyncState;
        private Object _returnValue;
        private Exception _exception;

        private bool _bIsComplete;

        private ManualResetEvent _manualResetEvent;

        
        internal BasicAsyncResult(AsyncCallback callback, Object state)
        {
            _asyncCallback = callback;
            _asyncState = state;
        } // BasicAsyncResult


        public Object AsyncState
        {
            get { return _asyncState; }
        } // AsyncState

        public WaitHandle AsyncWaitHandle
        {
            get 
            { 
                bool bSavedIsComplete = _bIsComplete;
                if (_manualResetEvent == null)
                {
                    lock (this)
                    {
                        if (_manualResetEvent == null)
                        {
                            _manualResetEvent = new ManualResetEvent(bSavedIsComplete);
                        }                        
                    }
                }

                if (!bSavedIsComplete && _bIsComplete)
                    _manualResetEvent.Set();

                return (WaitHandle)_manualResetEvent;
            }
        } // AsyncWaitHandle

        public bool CompletedSynchronously
        {
            get { return false; }
        } // CompletedSynchronously

        public bool IsCompleted
        {
            get { return _bIsComplete; }
        } // IsCompleted

        internal Exception Exception { get { return _exception; } }

        internal void SetComplete(Object returnValue, Exception exception)
        {
            _returnValue = returnValue;
            _exception = exception;

            CleanupOnComplete();

            _bIsComplete = true;

            try
            {
                if (_manualResetEvent != null)
                    _manualResetEvent.Set();
            }
            catch (Exception e)
            {
                if (_exception == null)
                    _exception = e;
            }


            // invoke the callback
            if (_asyncCallback != null)
                _asyncCallback(this);
        } // SetComplete                


        internal virtual void CleanupOnComplete()
        {
        }
    
    } // BasicAsyncResult

} // namespace System.Runtime.Remoting.Channels
