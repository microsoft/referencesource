//-----------------------------------------------------------------------------
// <copyright file="TransactionTable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Diagnostics;
    using System.Threading;


    class CheapUnfairReaderWriterLock
    {
        object writerFinishedEvent;

        int readersIn;
        int readersOut;
        bool writerPresent;

        object syncRoot;

        // Spin lock params
        const int MAX_SPIN_COUNT = 100;
        const int SLEEP_TIME = 500;


        public CheapUnfairReaderWriterLock()
        {
        }


        object SyncRoot
        {
            get
            {
                if ( this.syncRoot == null )
                {
                    Interlocked.CompareExchange( ref this.syncRoot, new object(), null );
                }
                return this.syncRoot;
            }
        }


        bool ReadersPresent
        {
            get
            {
                return this.readersIn != this.readersOut;
            }
        }


        ManualResetEvent WriterFinishedEvent
        {
            get
            {
                if ( this.writerFinishedEvent == null )
                {
                    Interlocked.CompareExchange( ref this.writerFinishedEvent, new ManualResetEvent( true ), null );
                }
                return (ManualResetEvent)this.writerFinishedEvent;
            }
        }


        public int AcquireReaderLock()
        {
            int readerIndex = 0;
            do
            {
                if ( this.writerPresent )
                {
                    WriterFinishedEvent.WaitOne();
                }

                readerIndex = Interlocked.Increment( ref this.readersIn );

                if ( !this.writerPresent )
                {
                    break;
                }

                Interlocked.Decrement( ref this.readersIn );
            } 
            while ( true );

            return readerIndex;
        }


        public void AcquireWriterLock()
        {
#pragma warning disable 0618
            //@
            Monitor.Enter(this.SyncRoot);
#pragma warning restore 0618

            this.writerPresent = true;
            this.WriterFinishedEvent.Reset();

            do
            {
                int i = 0;
                while ( ReadersPresent && i < MAX_SPIN_COUNT )
                {
                    Thread.Sleep( 0 );
                    i++;
                }

                if ( ReadersPresent )
                {
                    Thread.Sleep( SLEEP_TIME );
                }
            } 
            while ( ReadersPresent );
        }


        public void ReleaseReaderLock()
        {
            Interlocked.Increment( ref this.readersOut );
        }


        public void ReleaseWriterLock()
        {
            try
            {
                this.writerPresent = false;
                this.WriterFinishedEvent.Set();
            }
            finally
            {
                Monitor.Exit( this.SyncRoot );
            }
        }
    }


    // This transaction table implementation uses an array of lists to avoid contention.  The list for a
    // transaction is decided by its hashcode.
    class TransactionTable
    {
        // Use a timer to initiate looking for transactions that have timed out.
        System.Threading.Timer timer;

        // Private storage noting if the timer is enabled.
        bool timerEnabled;

        // Store the timer interval
        const int timerInternalExponent = 9;
        int timerInterval;

        // Store the number of ticks.  A tick is a mark of 1 timer interval.  By counting ticks
        // we can avoid expensive calls to get the current time for every transaction creation.
        const long TicksPerMillisecond = 10000;
        long ticks;
        Int64 lastTimerTime;
        
        // Sets of arrays of transactions.
        BucketSet headBucketSet;

        // Synchronize adding transactions with shutting off the timer and started events.
        CheapUnfairReaderWriterLock rwLock;

        internal TransactionTable()
        {
            // Create a timer that is initially disabled by specifing an Infinite time to the first interval
            this.timer = new Timer( new TimerCallback(ThreadTimer), null, Timeout.Infinite, this.timerInterval );

            // Note that the timer is disabled
            this.timerEnabled = false;

            // Store the timer interval
            this.timerInterval = 1 << TransactionTable.timerInternalExponent;

            // Ticks start off at zero.
            this.ticks = 0;

            // The head of the list is long.MaxValue.  It contains all of the transactions that for
            // some reason or other don't have a timeout.
            this.headBucketSet = new BucketSet( this, long.MaxValue );

            // Allocate the lock
            rwLock = new CheapUnfairReaderWriterLock();
        }


        // Calculate the maximum number of ticks for which this transaction should live
        internal long TimeoutTicks( TimeSpan timeout )
        {
            if ( timeout != TimeSpan.Zero )
            {
                // Note: At the current setting of approximately 2 ticks per second this timer will
                //       wrap in approximately 2^64/2/60/60/24/365=292,471,208,677.5360162195585996
                //       (nearly 300 billion) years.
                long timeoutTicks = ((timeout.Ticks / TimeSpan.TicksPerMillisecond) >> 
                        TransactionTable.timerInternalExponent) + this.ticks;
                return timeoutTicks;
            }
            else
            {
                return long.MaxValue;
            }
        }


        // Absolute timeout
        internal TimeSpan RecalcTimeout( InternalTransaction tx )
        {
            return TimeSpan.FromMilliseconds( (tx.AbsoluteTimeout - this.ticks) * this.timerInterval );
        }


        // Creation time
        private Int64 CurrentTime
        {
            get
            {
                if ( this.timerEnabled )
                {
                    return this.lastTimerTime;
                }
                else
                {
                    return DateTime.UtcNow.Ticks;
                }
            }
        }


        // Add a transaction to the table.  Transactions are added to the end of the list in sorted order based on their 
        // absolute timeout.
        internal int Add( InternalTransaction txNew )
        {
            // Tell the runtime that we are modifying global state.
            Thread.BeginCriticalRegion();
            int readerIndex = 0;

            try
            {
                readerIndex = rwLock.AcquireReaderLock();
                try
                {
                    // Start the timer if needed before checking the current time since the current
                    // time can be more efficient with a running timer.
                    if ( txNew.AbsoluteTimeout != long.MaxValue )
                    {
                        if ( !this.timerEnabled )
                        {
                            if ( !this.timer.Change( this.timerInterval, this.timerInterval ))
                            {
                                throw TransactionException.CreateInvalidOperationException(
                                    SR.GetString( SR.TraceSourceLtm ), 
                                    SR.GetString(SR.UnexpectedTimerFailure), 
                                    null
                                    );
                            }
                            this.lastTimerTime = DateTime.UtcNow.Ticks;
                            this.timerEnabled = true;
                        }
                    }
                    txNew.CreationTime = CurrentTime;

                    AddIter( txNew );
                }
                finally
                {
                    rwLock.ReleaseReaderLock();
                }
            }
            finally
            {
                Thread.EndCriticalRegion();
            }

            return readerIndex;
        }


        void AddIter( InternalTransaction txNew )
        {
            //
            // Theory of operation.
            //
            // Note that the head bucket contains any transaction with essentially infinite
            // timeout (long.MaxValue).  The list is sorted in decending order.  To add
            // a node the code must walk down the list looking for a set of bucket that matches
            // the absolute timeout value for the transaction.  When it is found it passes
            // the insert down to that set.
            //
            // An importent thing to note about the list is that forward links are all weak
            // references and reverse links are all strong references.  This allows the GC
            // to clean up old links in the list so that they don't need to be removed manually.
            // However if there is still a rooted strong reference to an old link in the
            // chain that link wont fall off the list because there is a strong reference held
            // forward.
            //
            
            BucketSet currentBucketSet = this.headBucketSet;

            while ( currentBucketSet.AbsoluteTimeout != txNew.AbsoluteTimeout )
            {
                BucketSet lastBucketSet = null;
                do
                {
                    WeakReference nextSetWeak = (WeakReference)currentBucketSet.nextSetWeak;
                    BucketSet nextBucketSet = null;
                    if ( nextSetWeak != null )
                    {
                        nextBucketSet = (BucketSet)nextSetWeak.Target;
                    }
                    
                    if ( nextBucketSet == null )
                    {
                        //
                        // We've reached the end of the list either because nextSetWeak was null or
                        // because its reference was collected.  This code doesn't care.  Make a new
                        // set, attempt to attach it and move on.
                        //
                        BucketSet newBucketSet = new BucketSet( this, txNew.AbsoluteTimeout );
                        WeakReference newSetWeak = new WeakReference( newBucketSet );

                        WeakReference oldNextSetWeak = (WeakReference)Interlocked.CompareExchange( 
                            ref currentBucketSet.nextSetWeak, newSetWeak, nextSetWeak );
                        if ( oldNextSetWeak == nextSetWeak )
                        {
                            // Ladies and Gentlemen we have a winner.
                            newBucketSet.prevSet = currentBucketSet;
                        }

                        // Note that at this point we don't update currentBucketSet.  On the next loop
                        // iteration we should be able to pick up where we left off.
                    }
                    else
                    {
                        lastBucketSet = currentBucketSet;
                        currentBucketSet = nextBucketSet;
                    }
                }
                while ( currentBucketSet.AbsoluteTimeout > txNew.AbsoluteTimeout );

                if ( currentBucketSet.AbsoluteTimeout != txNew.AbsoluteTimeout )
                {
                    //
                    // Getting to here means that we've found a slot in the list where this bucket set should go.
                    //
                    BucketSet newBucketSet = new BucketSet( this, txNew.AbsoluteTimeout );
                    WeakReference newSetWeak = new WeakReference( newBucketSet );

                    newBucketSet.nextSetWeak = lastBucketSet.nextSetWeak;
                    WeakReference oldNextSetWeak = (WeakReference)Interlocked.CompareExchange( 
                        ref lastBucketSet.nextSetWeak, newSetWeak, newBucketSet.nextSetWeak );
                    if ( oldNextSetWeak == newBucketSet.nextSetWeak )
                    {
                        // Ladies and Gentlemen we have a winner.
                        if ( oldNextSetWeak != null )
                        {
                            BucketSet oldSet = (BucketSet)oldNextSetWeak.Target;
                            if ( oldSet != null )
                            {
                                // prev references are just there to root things for the GC.  If this object is 
                                // gone we don't really care.
                                oldSet.prevSet = newBucketSet;
                            }
                        }
                        newBucketSet.prevSet = lastBucketSet;
                    }

                    // Special note - We are going to loop back to the BucketSet that preceeds the one we just tried
                    // to insert because we may have lost the ---- to insert our new BucketSet into the list to another
                    // "Add" thread. By looping back, we check again to see if the BucketSet we just created actually
                    // got added. If it did, we will exit out of the outer loop and add the transaction. But if we
                    // lost the ----, we will again try to add a new BucketSet. In the latter case, the BucketSet
                    // we created during the first iteration will simply be Garbage Collected because there are no
                    // strong references to it since we never added the transaction to a bucket and the act of
                    // creating the second BucketSet with remove the backward reference that was created in the
                    // first trip thru the loop.
                    currentBucketSet = lastBucketSet;
                    lastBucketSet = null;

                    // The outer loop will iterate and pick up where we left off.
                }
            }

            //
            // Great we found a spot.
            //
            currentBucketSet.Add( txNew );
        }


        // Remove a transaction from the table.
        internal void Remove( InternalTransaction tx )
        {
            tx.tableBucket.Remove( tx );
            tx.tableBucket = null;
        }


        // Process a timer event
        private void ThreadTimer( Object state )
        {
            //
            // Theory of operation.
            //
            // To timeout transactions we must walk down the list starting from the head
            // until we find a link with an absolute timeout that is greater than our own.
            // At that point everything further down in the list is elegable to be timed
            // out.  So simply remove that link in the list and walk down from that point
            // timing out any transaction that is found.
            //
            
            // There could be a ---- between this callback being queued and the timer
            // being disabled.  If we get here when the timer is disabled, just return.
            if ( !this.timerEnabled )
            {
                return;
            }

            // Increment the number of ticks
            this.ticks++;
            this.lastTimerTime = DateTime.UtcNow.Ticks;

            //
            // First find the starting point of transactions that should time out.  Every transaction after
            // that point will timeout so once we've found it then it is just a matter of traversing the
            // structure.
            //
            BucketSet lastBucketSet = null; 
            BucketSet currentBucketSet = this.headBucketSet; // The list always has a head.

            // Acquire a writer lock before checking to see if we should disable the timer.
            // Adding of transactions acquires a reader lock and might insert a new BucketSet.
            // If that ----s with our check for a BucketSet existing, we may not timeout that
            // transaction that is being added.
            WeakReference nextWeakSet = null;
            BucketSet nextBucketSet = null;

            nextWeakSet = (WeakReference)currentBucketSet.nextSetWeak;
            if (nextWeakSet != null)
            {
                nextBucketSet = (BucketSet)nextWeakSet.Target;
            }
            
            if (nextBucketSet == null)
            {
                this.rwLock.AcquireWriterLock();
                try
                {
                    // Access the nextBucketSet again in writer lock to account for any ---- before disabling the timeout. 
                    nextWeakSet = (WeakReference)currentBucketSet.nextSetWeak;
                    if (nextWeakSet != null)
                    {
                        nextBucketSet = (BucketSet)nextWeakSet.Target;
                    }

                    if (nextBucketSet == null)
                    {
                        //
                        // Special case to allow for disabling the timer.
                        //
                        // If there are no transactions on the timeout list we can disable the
                        // timer.
                        if (!this.timer.Change(Timeout.Infinite, Timeout.Infinite))
                        {
                            throw TransactionException.CreateInvalidOperationException(
                                SR.GetString(SR.TraceSourceLtm),
                                SR.GetString(SR.UnexpectedTimerFailure),
                                null
                                );
                        }
                        this.timerEnabled = false;

                        return;
                    }
                }
                finally
                {
                    this.rwLock.ReleaseWriterLock();
                }
            }

            // Note it is slightly subtle that we always skip the head node.  This is done
            // on purpose because the head node contains transactions with essentially 
            // an infinite timeout.
            do
            {
                do
                {
                    nextWeakSet = (WeakReference)currentBucketSet.nextSetWeak;
                    if ( nextWeakSet == null )
                    {
                        // Nothing more to do.
                        return;
                    }

                    nextBucketSet = (BucketSet)nextWeakSet.Target;
                    if ( nextBucketSet == null )
                    {
                        // Again nothing more to do.
                        return;
                    }
                    lastBucketSet = currentBucketSet;
                    currentBucketSet = nextBucketSet;
                }
                while ( currentBucketSet.AbsoluteTimeout > this.ticks );

                // Tell the runtime that we are modifying global state.
                Thread.BeginCriticalRegion();
                try
                {
                    //
                    // Pinch off the list at this point making sure it is still the correct set.
                    //
                    // Note: We may lose a ---- with an "Add" thread that is inserting a BucketSet in this location in
                    // the list. If that happens, this CompareExchange will not be performed and the returned abortingSetsWeak
                    // value will NOT equal nextWeakSet. But we check for that and if this condition occurs, this iteration of
                    // the timer thread will simply return, not timing out any transactions. When the next timer interval
                    // expires, the thread will walk the list again, find the appropriate BucketSet to pinch off, and
                    // then time out the transactions. This means that it is possible for a transaction to live a bit longer,
                    // but not much.
                    WeakReference abortingSetsWeak = 
                        (WeakReference)Interlocked.CompareExchange( ref lastBucketSet.nextSetWeak, null, nextWeakSet );

                    if ( abortingSetsWeak == nextWeakSet )
                    {
                        // Yea - now proceed to abort the transactions.
                        BucketSet abortingBucketSets = null;

                        do
                        {
                            if ( abortingSetsWeak != null )
                            {
                                abortingBucketSets = (BucketSet)abortingSetsWeak.Target;
                            }
                            else
                            {
                                abortingBucketSets = null;
                            }
                            if ( abortingBucketSets != null )
                            {
                                abortingBucketSets.TimeoutTransactions();
                                abortingSetsWeak = (WeakReference)abortingBucketSets.nextSetWeak;
                            }
                        } 
                        while ( abortingBucketSets != null );

                        // That's all we needed to do.
                        break;
                    }
                }
                finally
                {
                    Thread.EndCriticalRegion();
                }

                // We missed pulling the right transactions off.  Loop back up and try again.
                currentBucketSet = lastBucketSet;
            }
            while ( true );
        }
    }


    class BucketSet
    {
        // Buckets are kept in sets.  Each element of a set will have the same absoluteTimeout.
        internal object nextSetWeak;
        internal BucketSet prevSet;

        TransactionTable table;

        long absoluteTimeout;

        internal Bucket headBucket;

        internal BucketSet( TransactionTable table, long absoluteTimeout )
        {
            this.headBucket = new Bucket( this );
            this.table = table;
            this.absoluteTimeout = absoluteTimeout;
        }


        internal long AbsoluteTimeout
        {
            get
            {
                return this.absoluteTimeout;
            }
        }


        internal void Add( InternalTransaction newTx )
        {
            while ( !this.headBucket.Add( newTx ));
        }


        internal void TimeoutTransactions()
        {
            Bucket currentBucket = this.headBucket;
            // It will always have a head.
            do
            {
                currentBucket.TimeoutTransactions();

                WeakReference nextWeakBucket = (WeakReference)currentBucket.nextBucketWeak;
                if ( nextWeakBucket != null )
                {
                    currentBucket = (Bucket)nextWeakBucket.Target;
                }
                else
                {
                    currentBucket = null;
                }
            } 
            while ( currentBucket != null );
        }
    }


    class Bucket
    {
        bool timedOut;
        int index;
        int size;
        InternalTransaction[] transactions;
        internal WeakReference nextBucketWeak;
        Bucket previous;

        BucketSet owningSet;

        internal Bucket( BucketSet owningSet )
        {
            this.timedOut = false;
            this.index = -1;
            this.size = 1024; // A possible design change here is to have this scale dynamically based on load.
            transactions = new InternalTransaction[this.size];
            this.owningSet = owningSet;
        }


        internal bool Add( InternalTransaction tx )
        {
            int currentIndex = Interlocked.Increment( ref this.index );
            if ( currentIndex < this.size )
            {
                tx.tableBucket = this;
                tx.bucketIndex = currentIndex;
                Thread.MemoryBarrier(); // This data must be written before the transaction 
                                        // could be timed out.
                this.transactions[currentIndex] = tx;

                if ( this.timedOut )
                {
                    lock ( tx )
                    {
                        tx.State.Timeout( tx );
                    }
                }
            }
            else
            {
                Bucket newBucket = new Bucket( this.owningSet );
                newBucket.nextBucketWeak = new WeakReference( this );

                Bucket oldBucket = Interlocked.CompareExchange( ref this.owningSet.headBucket, newBucket, this );
                if ( oldBucket == this )
                {
                    // ladies and gentlemen we have a winner.
                    this.previous = newBucket;
                }

                return false;
            }
            return true;
        }


        internal void Remove( InternalTransaction tx )
        {
            this.transactions[tx.bucketIndex] = null;
        }


        internal void TimeoutTransactions()
        {
            int i;
            int transactionCount = this.index;

            this.timedOut = true;
            Thread.MemoryBarrier();

            for ( i = 0; i <= transactionCount && i < this.size; i++ )
            {
                Debug.Assert( transactionCount == this.index, "Index changed timing out transactions" );
                InternalTransaction tx = this.transactions[i];
                if ( tx != null )
                {
                    lock ( tx )
                    {
                        tx.State.Timeout( tx );
                    }
                }
            }
        }
    }
}
