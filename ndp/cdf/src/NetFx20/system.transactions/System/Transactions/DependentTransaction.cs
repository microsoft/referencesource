//-----------------------------------------------------------------------------
// <copyright file="DependentTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Runtime.Serialization;
    using System.Transactions.Diagnostics;

    /// <include file='doc\DependentTransaction.uex' path='docs/doc[@for="DependentTransaction"]/*' />
    // When we serialize a DependentTransaction, we specify the type OletxTransaction, so a DependentTransaction never
    // actually gets deserialized.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2229:ImplementSerializationConstructors")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2240:ImplementISerializableCorrectly")]
    [Serializable]
    public sealed class DependentTransaction : Transaction
    {
        bool blocking;

        // Create a transaction with the given settings
        //
        internal DependentTransaction(
            IsolationLevel isoLevel,
            InternalTransaction internalTransaction,
            bool blocking
            ) : base(isoLevel, internalTransaction)
        {
            this.blocking = blocking;
            lock (this.internalTransaction)
            {
                if (blocking)
                {
                    this.internalTransaction.State.CreateBlockingClone(this.internalTransaction);
                }
                else
                {
                    this.internalTransaction.State.CreateAbortingClone(this.internalTransaction);
                }
            }
        }

        public void Complete()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "DependentTransaction.Complete"
                    );
            }
            lock (this.internalTransaction)
            {
                if (Disposed)
                {
                    throw new ObjectDisposedException("Transaction");
                }

                if (this.complete)
                {
                    throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
                }

                this.complete = true;

                if (blocking)
                {
                    this.internalTransaction.State.CompleteBlockingClone(this.internalTransaction);
                }
                else
                {
                    this.internalTransaction.State.CompleteAbortingClone(this.internalTransaction);
                }
            }

            if (DiagnosticTrace.Information)
            {
                DependentCloneCompleteTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }
            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "DependentTransaction.Complete"
                    );
            }
        }
    }
}

