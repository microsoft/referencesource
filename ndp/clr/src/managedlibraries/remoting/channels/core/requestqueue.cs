// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// Request Queue
//      queues up the requests to avoid thread pool starvation,
//      making sure that there are always available threads to process requests
//<



namespace System.Runtime.Remoting.Channels {
    using System.Threading;
    using System.Collections;

    internal class RequestQueue {
        // configuration params
        private int _minExternFreeThreads;
        private int _minLocalFreeThreads;
        private int _queueLimit;

        // two queues -- one for local requests, one for external
        private Queue _localQueue = new Queue();
        private Queue _externQueue = new Queue();

        // total count
        private int _count;

        // work items queued to pick up new work
        private WaitCallback _workItemCallback;
        private int _workItemCount;
        private const int _workItemLimit = 2;


        // helpers
        private static bool IsLocal(SocketHandler sh) {
            return sh.IsLocal();
        }

        private void QueueRequest(SocketHandler sh, bool isLocal) {
            lock (this) {
                if (isLocal)
                    _localQueue.Enqueue(sh);
                else 
                    _externQueue.Enqueue(sh);

                _count++;
            }
        }

        private SocketHandler DequeueRequest(bool localOnly) {
            Object sh = null;

            if (_count > 0) {
                lock (this) {
                    if (_localQueue.Count > 0) {
                        sh = _localQueue.Dequeue();
                        _count--;
                    }
                    else if (!localOnly && _externQueue.Count > 0) {
                        sh = _externQueue.Dequeue();
                        _count--;
                    }
                }
            }

            return (SocketHandler)sh;
        }

        // ctor
        internal RequestQueue(int minExternFreeThreads, int minLocalFreeThreads, int queueLimit) {
            _minExternFreeThreads = minExternFreeThreads;
            _minLocalFreeThreads = minLocalFreeThreads;
            _queueLimit = queueLimit;
            
            _workItemCallback = new WaitCallback(this.WorkItemCallback);
        }


        // method called to process the next request
        internal void ProcessNextRequest(SocketHandler sh)
        {
            sh = GetRequestToExecute(sh);

            if (sh != null)
                sh.ProcessRequestNow();
        } // ProcessNextRequest
        

        // method called when data arrives for incoming requests
        internal SocketHandler GetRequestToExecute(SocketHandler sh) {
            int workerThreads, ioThreads;
            ThreadPool.GetAvailableThreads(out workerThreads, out ioThreads);
            int freeThreads = (ioThreads > workerThreads) ? workerThreads : ioThreads;

            // fast path when there are threads available and nothing queued
            if (freeThreads >= _minExternFreeThreads && _count == 0)
                return sh;

            bool isLocal = IsLocal(sh);

            // fast path when there are threads for local requests available and nothing queued
            if (isLocal && freeThreads >= _minLocalFreeThreads && _count == 0)
                return sh;

            // reject if queue limit exceeded
            if (_count >= _queueLimit) {
                sh.RejectRequestNowSinceServerIsBusy();
                return null;
            }

            // can't execute the current request on the current thread -- need to queue
            QueueRequest(sh, isLocal);

            // maybe can execute a request previously queued
            if (freeThreads >= _minExternFreeThreads) {
                sh = DequeueRequest(false); // enough threads to process even external requests
            }
            else if (freeThreads >= _minLocalFreeThreads) {
                sh = DequeueRequest(true);  // enough threads to process only local requests
            }
            else{
                sh = null;
            }

            if (sh == null){                  // not enough threads -> do nothing on this thread
                ScheduleMoreWorkIfNeeded(); // try to schedule to worker thread
            }

            return sh;
        }

        // method called from SocketHandler at the end of request
        internal void ScheduleMoreWorkIfNeeded() {
            // is queue empty?
            if (_count == 0)
                return;

            // already scheduled enough work items
            if (_workItemCount >= _workItemLimit)
                return;

            // queue the work item
            Interlocked.Increment(ref _workItemCount);
            ThreadPool.UnsafeQueueUserWorkItem(_workItemCallback, null);
        }

        // method called to pick up more work
        private void WorkItemCallback(Object state) {
            Interlocked.Decrement(ref _workItemCount);

            // is queue empty?
            if (_count == 0)
                return;

            int workerThreads, ioThreads;
            ThreadPool.GetAvailableThreads(out workerThreads, out ioThreads);

            bool bHandledRequest = false;
            // service another request if enough worker threads are available
            if (workerThreads >= _minLocalFreeThreads)
            {
                // pick up request from the queue
                SocketHandler sh = DequeueRequest(workerThreads < _minExternFreeThreads);
                if (sh != null)
                {
                    sh.ProcessRequestNow();
                    bHandledRequest = true;
                }
            }

            if (!bHandledRequest){
                Thread.Sleep(250);
                ScheduleMoreWorkIfNeeded();
            }
        }

    }
}
