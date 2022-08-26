//-----------------------------------------------------------------------------
// <copyright file="CommittableTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Threading;

    using System.Transactions.Diagnostics;

    /// <include file='doc\CommittableTransaction.uex' path='docs/doc[@for="CommittableTransaction"]/*' />
    // When we serialize a CommittableTransaction, we specify the type OletxTransaction, so a CommittableTransaction never
    // actually gets deserialized.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2229:ImplementSerializationConstructors")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2240:ImplementISerializableCorrectly")]
    [Serializable]
    public sealed class CommittableTransaction : Transaction, IAsyncResult
    {
        internal bool completedSynchronously = false;

        // Create a transaction with defaults
        public CommittableTransaction(
            ) : this(System.Transactions.TransactionManager.DefaultIsolationLevel,
                     System.Transactions.TransactionManager.DefaultTimeout)
        {
        }


        // Create a transaction with the given info
        public CommittableTransaction(
            TimeSpan timeout
            ) : this(System.Transactions.TransactionManager.DefaultIsolationLevel, timeout)
        {
        }


        // Create a transaction with the given options
        public CommittableTransaction(
            TransactionOptions options
            ) : this(options.IsolationLevel, options.Timeout)
        {
        }


        internal CommittableTransaction(
            IsolationLevel isoLevel,
            TimeSpan timeout
            ) : base(isoLevel, (InternalTransaction)null)
        {
            // object to use for synchronization rather than locking on a public object
            this.internalTransaction = new InternalTransaction(timeout, this);

            // Because we passed null for the internal transaction to the base class, we need to
            // fill in the traceIdentifier field here.
            this.internalTransaction.cloneCount = 1;
            this.cloneId = 1;
            if (DiagnosticTrace.Information)
            {
                TransactionCreatedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }
        }


        public IAsyncResult BeginCommit(
            AsyncCallback asyncCallback,
            object asyncState
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "CommittableTransaction.BeginCommit"
                    );
                TransactionCommitCalledTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            lock (this.internalTransaction)
            {
                if (this.complete)
                {
                    throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
                }
                
                // this.complete will get set to true when the transaction enters a state that is
                // beyond Phase0.

                this.internalTransaction.State.BeginCommit(this.internalTransaction, true, asyncCallback, asyncState);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "CommittableTransaction.BeginCommit"
                    );
            }

            return this;
        }


        // Forward the commit to the state machine to take the appropriate action.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction."]/*' />
        public void Commit()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "CommittableTransaction.Commit"
                    );
                TransactionCommitCalledTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            lock (this.internalTransaction)
            {
                if (this.complete)
                {
                    throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
                }

                this.internalTransaction.State.BeginCommit(this.internalTransaction, false, null, null);

                // now that commit has started wait for the monitor on the transaction to know
                // if the transaction is done.
                do
                {
                    if (this.internalTransaction.State.IsCompleted(this.internalTransaction))
                    {
                        break;
                    }
                } while (System.Threading.Monitor.Wait(this.internalTransaction));

                this.internalTransaction.State.EndCommit(this.internalTransaction);
                
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "CommittableTransaction.Commit"
                    );
            }
        }


        internal override void InternalDispose()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "IDisposable.Dispose"
                    );
            }

            if (Interlocked.Exchange(ref this.disposed, Transaction.disposedTrueValue) == Transaction.disposedTrueValue)
            {
                return;
            }

            if (this.internalTransaction.State.get_Status(this.internalTransaction) == TransactionStatus.Active)
            {
                lock (this.internalTransaction)
                {
                    // Since this is the root transaction do state based dispose.
                    this.internalTransaction.State.DisposeRoot(this.internalTransaction);
                }
            }

            // Attempt to clean up the internal transaction
            long remainingITx = Interlocked.Decrement(ref this.internalTransaction.cloneCount);
            if (remainingITx == 0)
            {
                this.internalTransaction.Dispose();
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "IDisposable.Dispose"
                    );
            }
        }


        public void EndCommit(
            IAsyncResult asyncResult
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "CommittableTransaction.EndCommit"
                    );
            }

            if (asyncResult != ((object)this))
            {
                throw new ArgumentException(SR.GetString(SR.BadAsyncResult), "asyncResult");
            }

            lock (this.internalTransaction)
            {
                do
                {
                    if (this.internalTransaction.State.IsCompleted(this.internalTransaction))
                    {
                        break;
                    }
                } while (System.Threading.Monitor.Wait(this.internalTransaction));

                this.internalTransaction.State.EndCommit(this.internalTransaction);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "CommittableTransaction.EndCommit"
                    );
            }
        }

        object IAsyncResult.AsyncState
        {
            get
            {
                return this.internalTransaction.asyncState;
            }
        }

        bool IAsyncResult.CompletedSynchronously
        {
            get
            {
                return this.completedSynchronously;
            }
        }

        WaitHandle IAsyncResult.AsyncWaitHandle
        {
            get
            {
                if (this.internalTransaction.asyncResultEvent == null)
                {
                    lock (this.internalTransaction)
                    {
                        if (this.internalTransaction.asyncResultEvent == null)
                        {
                            // Demand create an event that is already signaled if the transaction has completed.
                            ManualResetEvent temp = new ManualResetEvent(
                                this.internalTransaction.State.get_Status(this.internalTransaction) != TransactionStatus.Active 
                                );

                            this.internalTransaction.asyncResultEvent = temp;
                        }
                    }
                }

                return this.internalTransaction.asyncResultEvent;
            }
        }

        bool IAsyncResult.IsCompleted
        {
            get
            {
                lock (this.internalTransaction)
                {
                    return this.internalTransaction.State.get_Status(this.internalTransaction) != TransactionStatus.Active;
                }
            }
        }
    }
}


