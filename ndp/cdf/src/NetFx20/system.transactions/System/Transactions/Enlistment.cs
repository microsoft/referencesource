//-----------------------------------------------------------------------------
// <copyright file="Enlistment.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Threading;
    using System.Transactions;
    using System.Transactions.Diagnostics;

    internal interface IPromotedEnlistment
    {
        void EnlistmentDone();

        void Prepared();

        void ForceRollback();

        void ForceRollback(Exception e);

        void Committed();

        void Aborted();

        void Aborted(Exception e);

        void InDoubt();

        void InDoubt(Exception e);

        byte[] GetRecoveryInformation();

        InternalEnlistment InternalEnlistment
        {
            get;
            set;
        }
    }


    //
    // InternalEnlistment by itself can support a Phase0 volatile enlistment.
    // There are derived classes to support durable, phase1 volatile & PSPE
    // enlistments.
    //
    class InternalEnlistment : ISinglePhaseNotificationInternal
    {
        // Storage for the state of the enlistment.
        internal EnlistmentState twoPhaseState;

        // Interface implemented by the enlistment owner for notifications
        protected IEnlistmentNotification twoPhaseNotifications;

        // Store a reference to the single phase notification interface in case
        // the enlisment supports it.
        protected ISinglePhaseNotification singlePhaseNotifications;

        // Reference to the containing transaction.
        protected InternalTransaction transaction;

        // Reference to the lightweight transaction.
        Transaction atomicTransaction;

        // The EnlistmentTraceIdentifier for this enlistment.
        private EnlistmentTraceIdentifier traceIdentifier;

        // Unique value amongst all enlistments for a given internal transaction.
        int enlistmentId;

        internal Guid DistributedTxId
        {
            get
            {
                Guid returnValue = Guid.Empty;

                if (this.Transaction != null )
                {
                    returnValue = this.Transaction.DistributedTxId;
                }
                return returnValue;
            }
        }

        // Parent Enlistment Object
        Enlistment enlistment;
        PreparingEnlistment preparingEnlistment;
        SinglePhaseEnlistment singlePhaseEnlistment;

        // If this enlistment is promoted store the object it delegates to.
        IPromotedEnlistment promotedEnlistment;

        // For Recovering Enlistments
        protected InternalEnlistment(
            Enlistment enlistment,
            IEnlistmentNotification twoPhaseNotifications
            )
        {
            Debug.Assert(this is RecoveringInternalEnlistment, "this is RecoveringInternalEnlistment");
            this.enlistment = enlistment;
            this.twoPhaseNotifications = twoPhaseNotifications;
            this.enlistmentId = 1;
            this.traceIdentifier = EnlistmentTraceIdentifier.Empty;
        }


        // For Promotable Enlistments
        protected InternalEnlistment(
            Enlistment enlistment,
            InternalTransaction transaction, 
            Transaction atomicTransaction
            )
        {
            Debug.Assert(this is PromotableInternalEnlistment, "this is PromotableInternalEnlistment");
            this.enlistment = enlistment;
            this.transaction = transaction;
            this.atomicTransaction = atomicTransaction;
            this.enlistmentId = transaction.enlistmentCount++;
            this.traceIdentifier = EnlistmentTraceIdentifier.Empty;
        }


        internal InternalEnlistment(
            Enlistment enlistment,
            InternalTransaction transaction, 
            IEnlistmentNotification twoPhaseNotifications,
            ISinglePhaseNotification singlePhaseNotifications,
            Transaction atomicTransaction
            )
        {
            this.enlistment = enlistment;
            this.transaction = transaction;
            this.twoPhaseNotifications = twoPhaseNotifications;
            this.singlePhaseNotifications = singlePhaseNotifications;
            this.atomicTransaction = atomicTransaction;
            this.enlistmentId = transaction.enlistmentCount++;
            this.traceIdentifier = EnlistmentTraceIdentifier.Empty;
        }


        internal InternalEnlistment(
            Enlistment enlistment,
            IEnlistmentNotification twoPhaseNotifications,
            InternalTransaction transaction,
            Transaction atomicTransaction
            )
        {
            this.enlistment = enlistment;
            this.twoPhaseNotifications = twoPhaseNotifications;
            this.transaction = transaction;
            this.atomicTransaction = atomicTransaction;
        }


        internal EnlistmentState State
        {
            get
            {
                return this.twoPhaseState;
            }
            
            set
            {
                this.twoPhaseState = value;
            }
        }


        internal Enlistment Enlistment
        {
            get
            {
                return this.enlistment;
            }
        }


        internal PreparingEnlistment PreparingEnlistment
        {
            get
            {
                if (this.preparingEnlistment == null)
                {
                    // If there is a ---- here one of the objects would simply be garbage collected.
                    this.preparingEnlistment = new PreparingEnlistment(this);
                }
                return this.preparingEnlistment;
            }
        }


        internal SinglePhaseEnlistment SinglePhaseEnlistment
        {
            get
            {
                if (this.singlePhaseEnlistment == null)
                {
                    // If there is a ---- here one of the objects would simply be garbage collected.
                    this.singlePhaseEnlistment = new SinglePhaseEnlistment(this);
                }
                return this.singlePhaseEnlistment;
            }
        }


        internal InternalTransaction Transaction
        {
            get
            {
                return this.transaction;
            }
        }


        internal virtual object SyncRoot
        {
            get
            {
                Debug.Assert(this.transaction != null, "this.transaction != null");
                return this.transaction;
            }
        }


        internal IEnlistmentNotification EnlistmentNotification
        {
            get
            {
                return this.twoPhaseNotifications;
            }
        }


        internal ISinglePhaseNotification SinglePhaseNotification
        {
            get
            {
                return this.singlePhaseNotifications;
            }
        }


        internal virtual IPromotableSinglePhaseNotification PromotableSinglePhaseNotification
        {
            get
            {
                Debug.Assert(false, "PromotableSinglePhaseNotification called for a non promotable enlistment.");
                throw new NotImplementedException();
            }
        }


        internal IPromotedEnlistment PromotedEnlistment
        {
            get
            {
                return this.promotedEnlistment;
            }

            set
            {
                this.promotedEnlistment = value;
            }
        }


        internal EnlistmentTraceIdentifier EnlistmentTraceId
        {
            get
            {
                if (this.traceIdentifier == EnlistmentTraceIdentifier.Empty)
                {
                    lock (this.SyncRoot)
                    {
                        if (this.traceIdentifier == EnlistmentTraceIdentifier.Empty)
                        {
                            EnlistmentTraceIdentifier temp;
                            if (null != this.atomicTransaction)
                            {
                                temp = new EnlistmentTraceIdentifier(
                                    Guid.Empty,
                                    this.atomicTransaction.TransactionTraceId,
                                    this.enlistmentId
                                    );
                            }
                            else
                            {
                                temp = new EnlistmentTraceIdentifier(
                                    Guid.Empty,
                                    new TransactionTraceIdentifier(
                                        InternalTransaction.InstanceIdentifier +
                                            Convert.ToString(Interlocked.Increment(ref InternalTransaction.nextHash), CultureInfo.InvariantCulture),
                                        0),
                                    this.enlistmentId
                                    );
                            }
                            Thread.MemoryBarrier();
                            this.traceIdentifier = temp;
                        }
                    }
                }
                return this.traceIdentifier;
            }
        }


        internal virtual void FinishEnlistment()
        {
            // Note another enlistment finished.
            this.Transaction.phase0Volatiles.preparedVolatileEnlistments++;
            CheckComplete();
        }


        internal virtual void CheckComplete()
        {
            // Make certain we increment the right list.
            Debug.Assert(this.Transaction.phase0Volatiles.preparedVolatileEnlistments <=
                this.Transaction.phase0Volatiles.volatileEnlistmentCount + this.Transaction.phase0Volatiles.dependentClones);

            // Check to see if all of the volatile enlistments are done.
            if (this.Transaction.phase0Volatiles.preparedVolatileEnlistments == 
                this.Transaction.phase0VolatileWaveCount + this.Transaction.phase0Volatiles.dependentClones)
            {
                this.Transaction.State.Phase0VolatilePrepareDone(this.Transaction);
            }
        }


        internal virtual Guid ResourceManagerIdentifier
        {
            get
            {
                Debug.Assert(false, "ResourceManagerIdentifier called for non durable enlistment");
                throw new NotImplementedException();
            }
        }


        void ISinglePhaseNotificationInternal.SinglePhaseCommit(
            IPromotedEnlistment singlePhaseEnlistment
            )
        {
            bool spcCommitted = false; 
            this.promotedEnlistment = singlePhaseEnlistment;
            try
            {
                this.singlePhaseNotifications.SinglePhaseCommit(this.SinglePhaseEnlistment);
                spcCommitted = true; 
            }
            finally
            {
                if (!spcCommitted)
                {
                    this.SinglePhaseEnlistment.InDoubt();
                }
            }
        }


        void IEnlistmentNotificationInternal.Prepare(
            IPromotedEnlistment preparingEnlistment
            )
        {
            this.promotedEnlistment = preparingEnlistment;
            this.twoPhaseNotifications.Prepare(this.PreparingEnlistment);
        }


        void IEnlistmentNotificationInternal.Commit(
            IPromotedEnlistment enlistment
            )
        {
            this.promotedEnlistment = enlistment;
            this.twoPhaseNotifications.Commit(this.Enlistment);
        }
        

        void IEnlistmentNotificationInternal.Rollback(
            IPromotedEnlistment enlistment
            )
        {
            this.promotedEnlistment = enlistment;
            this.twoPhaseNotifications.Rollback(this.Enlistment);
        }


        void IEnlistmentNotificationInternal.InDoubt(
            IPromotedEnlistment enlistment
            )
        {
            this.promotedEnlistment = enlistment;
            this.twoPhaseNotifications.InDoubt(this.Enlistment);
        }

    }


    class DurableInternalEnlistment : InternalEnlistment
    {
        // Resource Manager Identifier for this enlistment if it is durable
        internal Guid resourceManagerIdentifier;

        internal DurableInternalEnlistment(
            Enlistment enlistment,
            Guid resourceManagerIdentifier,
            InternalTransaction transaction, 
            IEnlistmentNotification twoPhaseNotifications,
            ISinglePhaseNotification singlePhaseNotifications,
            Transaction atomicTransaction
            ) :
            base(enlistment, transaction, twoPhaseNotifications, singlePhaseNotifications, atomicTransaction)
        {
            this.resourceManagerIdentifier = resourceManagerIdentifier;
        }


        protected DurableInternalEnlistment(
            Enlistment enlistment,
            IEnlistmentNotification twoPhaseNotifications
            ) : base(enlistment, twoPhaseNotifications)
        {
        }


        internal override Guid ResourceManagerIdentifier
        {
            get
            {
                return resourceManagerIdentifier;
            }
        }
    }


    //
    // Since RecoveringInternalEnlistment does not have a transaction it must take
    // a separate object as its sync root.
    //
    class RecoveringInternalEnlistment : DurableInternalEnlistment
    {
        object syncRoot;

        internal RecoveringInternalEnlistment(
            Enlistment enlistment,
            IEnlistmentNotification twoPhaseNotifications,
            object syncRoot
            ) : base(enlistment, twoPhaseNotifications)
        {
            this.syncRoot = syncRoot;
        }
        
        internal override object SyncRoot
        {
            get
            {
                return this.syncRoot;
            }
        }
    }


    class PromotableInternalEnlistment : InternalEnlistment
    {
        // This class acts as the durable single phase enlistment for a
        // promotable single phase enlistment.
        IPromotableSinglePhaseNotification promotableNotificationInterface;

        internal PromotableInternalEnlistment(
            Enlistment enlistment,
            InternalTransaction transaction, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction
            ) :
            base(enlistment, transaction, atomicTransaction)
        {
            this.promotableNotificationInterface = promotableSinglePhaseNotification;
        }


        internal override IPromotableSinglePhaseNotification PromotableSinglePhaseNotification
        {
            get
            {
                return this.promotableNotificationInterface;
            }
        }
    }


    // This class supports volatile enlistments
    //
    internal class Phase1VolatileEnlistment : InternalEnlistment
    {
        public Phase1VolatileEnlistment(
            Enlistment enlistment,
            InternalTransaction transaction, 
            IEnlistmentNotification twoPhaseNotifications,
            ISinglePhaseNotification singlePhaseNotifications,
            Transaction atomicTransaction
            )
            : base(enlistment, transaction, twoPhaseNotifications, singlePhaseNotifications, atomicTransaction)
        {
        }


        internal override void FinishEnlistment()
        {
            // Note another enlistment finished.
            this.transaction.phase1Volatiles.preparedVolatileEnlistments++;
            CheckComplete();
        }


        internal override void CheckComplete()
        {
            // Make certain we increment the right list.
            Debug.Assert(this.transaction.phase1Volatiles.preparedVolatileEnlistments <= 
                this.transaction.phase1Volatiles.volatileEnlistmentCount +
                this.transaction.phase1Volatiles.dependentClones);

            // Check to see if all of the volatile enlistments are done.
            if (this.transaction.phase1Volatiles.preparedVolatileEnlistments == 
                this.transaction.phase1Volatiles.volatileEnlistmentCount + 
                this.transaction.phase1Volatiles.dependentClones)
            {
                this.transaction.State.Phase1VolatilePrepareDone(this.transaction);
            }
        }
    }


    public class Enlistment
    {
        // Interface for communicating with the state machine.
        internal InternalEnlistment internalEnlistment;

        internal Enlistment(
            InternalEnlistment internalEnlistment
            )
        {
            this.internalEnlistment = internalEnlistment;
        }


        internal Enlistment(
            Guid resourceManagerIdentifier,
            InternalTransaction transaction, 
            IEnlistmentNotification twoPhaseNotifications,
            ISinglePhaseNotification singlePhaseNotifications,
            Transaction atomicTransaction
            )
        {
            this.internalEnlistment = new DurableInternalEnlistment(
                this,
                resourceManagerIdentifier,
                transaction,
                twoPhaseNotifications,
                singlePhaseNotifications,
                atomicTransaction
                );
        }


        internal Enlistment(
            InternalTransaction transaction, 
            IEnlistmentNotification twoPhaseNotifications,
            ISinglePhaseNotification singlePhaseNotifications,
            Transaction atomicTransaction,
            EnlistmentOptions enlistmentOptions
            )
        {
            if ((enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0)
            {
                this.internalEnlistment = new InternalEnlistment(
                    this,
                    transaction,
                    twoPhaseNotifications,
                    singlePhaseNotifications,
                    atomicTransaction
                    );
            }
            else
            {
                    this.internalEnlistment = new Phase1VolatileEnlistment(
                    this,
                    transaction,
                    twoPhaseNotifications,
                    singlePhaseNotifications,
                    atomicTransaction
                    );
            }
        }


        // This constructor is for a promotable single phase enlistment.
        internal Enlistment(
            InternalTransaction transaction, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction
            )
        {
            this.internalEnlistment = new PromotableInternalEnlistment(
                this,
                transaction,
                promotableSinglePhaseNotification,
                atomicTransaction
                );
        }


        internal Enlistment(
            IEnlistmentNotification twoPhaseNotifications,
            InternalTransaction transaction,
            Transaction atomicTransaction
            )
        {
            this.internalEnlistment = new InternalEnlistment(
                this,
                twoPhaseNotifications,
                transaction,
                atomicTransaction
                );
        }


        internal Enlistment(
            IEnlistmentNotification twoPhaseNotifications,
            object syncRoot
            )
        {
            this.internalEnlistment = new RecoveringInternalEnlistment(
                this,
                twoPhaseNotifications,
                syncRoot
                );
        }


        public void Done()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Enlistment.Done"
                    );
                EnlistmentCallbackPositiveTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.Done
                    );
            }

            lock (this.internalEnlistment.SyncRoot)
            {
                this.internalEnlistment.State.EnlistmentDone(this.internalEnlistment);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Enlistment.Done"
                    );
            }
        }


        internal InternalEnlistment InternalEnlistment
        {
            get
            {
                return this.internalEnlistment;
            }
        }
    }
}

