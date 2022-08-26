//------------------------------------------------------------------------------
// <copyright file="MessageQueueTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Threading;
    using System.Diagnostics;
    using System.Messaging.Interop;

    /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class MessageQueueTransaction : IDisposable
    {
        private ITransaction internalTransaction;
        private MessageQueueTransactionStatus transactionStatus;
        private bool disposed;

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.MessageQueueTransaction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new Message Queuing internal transaction context.
        ///    </para>
        /// </devdoc>                
        public MessageQueueTransaction()
        {
            this.transactionStatus = MessageQueueTransactionStatus.Initialized;
        }

        internal ITransaction InnerTransaction
        {
            get
            {
                return this.internalTransaction;
            }
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Status"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The status of the transaction that this object represents.
        ///    </para>
        /// </devdoc>   
        public MessageQueueTransactionStatus Status
        {
            get
            {
                return this.transactionStatus;
            }
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Abort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Rolls back the pending internal transaction.
        ///    </para>
        /// </devdoc>
        public void Abort()
        {
            lock (this)
            {
                if (this.internalTransaction == null)
                    throw new InvalidOperationException(Res.GetString(Res.TransactionNotStarted));
                else
                {
                    this.AbortInternalTransaction();
                }
            }
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.AbortInternalTransaction"]/*' />
        /// <internalonly/>                                 
        private void AbortInternalTransaction()
        {
            int status = this.internalTransaction.Abort(0, 0, 0);
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            this.internalTransaction = null;
            this.transactionStatus = MessageQueueTransactionStatus.Aborted;
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Begin"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Begins a new Message Queuing internal transaction context.
        ///    </para>
        /// </devdoc>
        public void Begin()
        {
            //Won't allow begining a new transaction after the object has been disposed.
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);

            lock (this)
            {
                if (internalTransaction != null)
                    throw new InvalidOperationException(Res.GetString(Res.TransactionStarted));
                else
                {
                    int status = SafeNativeMethods.MQBeginTransaction(out this.internalTransaction);
                    if (MessageQueue.IsFatalError(status))
                    {
                        this.internalTransaction = null;
                        throw new MessageQueueException(status);
                    }

                    this.transactionStatus = MessageQueueTransactionStatus.Pending;
                }
            }
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.BeginQueueOperation"]/*' />
        /// <internalonly/>                                           
        internal ITransaction BeginQueueOperation()
        {
#pragma warning disable 0618
            //@
            Monitor.Enter(this);
#pragma warning restore 0618
            return this.internalTransaction;
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Commit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Commits a pending internal transaction.
        ///    </para>
        /// </devdoc>
        public void Commit()
        {
            lock (this)
            {
                if (this.internalTransaction == null)
                    throw new InvalidOperationException(Res.GetString(Res.TransactionNotStarted));
                else
                {
                    int status = this.internalTransaction.Commit(0, 0, 0);
                    if (MessageQueue.IsFatalError(status))
                        throw new MessageQueueException(status);

                    this.internalTransaction = null;
                    this.transactionStatus = MessageQueueTransactionStatus.Committed;
                }
            }
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes this transaction instance, if it is in a 
        ///       pending status, the transaction will be aborted.  
        ///    </para>
        /// </devdoc>   
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Dispose1"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock (this)
                {
                    if (internalTransaction != null)
                        this.AbortInternalTransaction();
                }
            }

            this.disposed = true;
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.Finalize"]/*' />
        /// <internalonly/>   
        ~MessageQueueTransaction()
        {
            Dispose(false);
        }

        /// <include file='doc\MessageQueueTransaction.uex' path='docs/doc[@for="MessageQueueTransaction.EndQueueOperation"]/*' />
        /// <internalonly/>        
        internal void EndQueueOperation()
        {
            Monitor.Exit(this);
        }
    }
}
