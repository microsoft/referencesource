//-----------------------------------------------------------------------------
// <copyright file="TransactionState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Transactions.Diagnostics;

    // The TransactionState object defines the basic set of operations that
    // are available for a transaction.  It is a base type and the base 
    // implementations all throw exceptions.  For a particular state a derived
    // implementation will inheret from this object and implement the appropriate
    // operations for that state.
    internal abstract class TransactionState
    {
        // The state machines themselves are designed to be internally consistent.  So the only externally visable
        // state transition is to active.  All other state transitions must happen within the state machines 
        // themselves.
        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile TransactionStateActive _transactionStateActive;
        private static volatile TransactionStateSubordinateActive _transactionStateSubordinateActive;
        private static volatile TransactionStatePhase0 _transactionStatePhase0;
        private static volatile TransactionStateVolatilePhase1 _transactionStateVolatilePhase1;
        private static volatile TransactionStateVolatileSPC _transactionStateVolatileSPC;
        private static volatile TransactionStateSPC _transactionStateSPC;
        private static volatile TransactionStateAborted _transactionStateAborted;
        private static volatile TransactionStateCommitted _transactionStateCommitted;
        private static volatile TransactionStateInDoubt _transactionStateInDoubt;

        private static volatile TransactionStatePromoted _transactionStatePromoted;
        private static volatile TransactionStateNonCommittablePromoted _transactionStateNonCommittablePromoted;
        private static volatile TransactionStatePromotedP0Wave _transactionStatePromotedP0Wave;
        private static volatile TransactionStatePromotedCommitting _transactionStatePromotedCommitting;
        private static volatile TransactionStatePromotedPhase0 _transactionStatePromotedPhase0;
        private static volatile TransactionStatePromotedPhase1 _transactionStatePromotedPhase1;
        private static volatile TransactionStatePromotedP0Aborting _transactionStatePromotedP0Aborting;
        private static volatile TransactionStatePromotedP1Aborting _transactionStatePromotedP1Aborting;
        private static volatile TransactionStatePromotedAborted _transactionStatePromotedAborted;
        private static volatile TransactionStatePromotedCommitted _transactionStatePromotedCommitted;
        private static volatile TransactionStatePromotedIndoubt _transactionStatePromotedIndoubt;

        private static volatile TransactionStateDelegated _transactionStateDelegated;
        private static volatile TransactionStateDelegatedSubordinate _transactionStateDelegatedSubordinate;
        private static volatile TransactionStateDelegatedP0Wave _transactionStateDelegatedP0Wave;
        private static volatile TransactionStateDelegatedCommitting _transactionStateDelegatedCommitting;
        private static volatile TransactionStateDelegatedAborting _transactionStateDelegatedAborting;
        private static volatile TransactionStatePSPEOperation _transactionStatePSPEOperation;

        private static volatile TransactionStateDelegatedNonMSDTC _transactionStateDelegatedNonMSDTC;
        private static volatile TransactionStatePromotedNonMSDTCPhase0 _transactionStatePromotedNonMSDTCPhase0;
        private static volatile TransactionStatePromotedNonMSDTCVolatilePhase1 _transactionStatePromotedNonMSDTCVolatilePhase1;
        private static volatile TransactionStatePromotedNonMSDTCSinglePhaseCommit _transactionStatePromotedNonMSDTCSinglePhaseCommit;
        private static volatile TransactionStatePromotedNonMSDTCAborted _transactionStatePromotedNonMSDTCAborted;
        private static volatile TransactionStatePromotedNonMSDTCCommitted _transactionStatePromotedNonMSDTCCommitted;
        private static volatile TransactionStatePromotedNonMSDTCIndoubt _transactionStatePromotedNonMSDTCIndoubt;

        // Object for synchronizing access to the entire class( avoiding lock( typeof( ... )) )
        private static object classSyncObject;


        internal static TransactionStateActive _TransactionStateActive
        {
            get
            {
                if (_transactionStateActive == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateActive == null)
                        {
                            TransactionStateActive temp = new TransactionStateActive();
                            _transactionStateActive = temp;
                        }
                    }
                }

                return _transactionStateActive;
            }
        }


        internal static TransactionStateSubordinateActive _TransactionStateSubordinateActive
        {
            get
            {
                if (_transactionStateSubordinateActive == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateSubordinateActive == null)
                        {
                            TransactionStateSubordinateActive temp = new TransactionStateSubordinateActive();
                            _transactionStateSubordinateActive = temp;
                        }
                    }
                }

                return _transactionStateSubordinateActive;
            }
        }


        internal static TransactionStatePSPEOperation _TransactionStatePSPEOperation
        {
            get
            {
                if (_transactionStatePSPEOperation == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePSPEOperation == null)
                        {
                            TransactionStatePSPEOperation temp = new TransactionStatePSPEOperation();
                            _transactionStatePSPEOperation = temp;
                        }
                    }
                }

                return _transactionStatePSPEOperation;
            }
        }


        protected static TransactionStatePhase0 _TransactionStatePhase0
        {
            get
            {
                if (_transactionStatePhase0 == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePhase0 == null)
                        {
                            TransactionStatePhase0 temp = new TransactionStatePhase0();
                            _transactionStatePhase0 = temp; 
                        }
                    }
                }

                return _transactionStatePhase0;
            }
        }
   
        protected static TransactionStateVolatilePhase1 _TransactionStateVolatilePhase1
        {
            get
            {
                if (_transactionStateVolatilePhase1 == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateVolatilePhase1 == null)
                        {
                            TransactionStateVolatilePhase1 temp = new TransactionStateVolatilePhase1();
                            _transactionStateVolatilePhase1 = temp;
                        }
                    }
                }

                return _transactionStateVolatilePhase1;
            }
        }

        protected static TransactionStateVolatileSPC _TransactionStateVolatileSPC
        {
            get
            {
                if (_transactionStateVolatileSPC == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateVolatileSPC == null)
                        {
                            TransactionStateVolatileSPC temp = new TransactionStateVolatileSPC();
                            _transactionStateVolatileSPC = temp;
                        }
                    }
                }

                return _transactionStateVolatileSPC;
            }
        }

        
        protected static TransactionStateSPC _TransactionStateSPC
        {
            get
            {
                if (_transactionStateSPC == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateSPC == null)
                        {
                            TransactionStateSPC temp = new TransactionStateSPC();
                            _transactionStateSPC = temp;
                        }
                    }
                }

                return _transactionStateSPC;
            }
        }

        
        protected static TransactionStateAborted _TransactionStateAborted
        {
            get
            {
                if (_transactionStateAborted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateAborted == null)
                        {
                            TransactionStateAborted temp = new TransactionStateAborted();
                            _transactionStateAborted = temp;
                        }
                    }
                }

                return _transactionStateAborted;
            }
        }

        
        protected static TransactionStateCommitted _TransactionStateCommitted
        {
            get
            {
                if (_transactionStateCommitted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateCommitted == null)
                        {
                            TransactionStateCommitted temp = new TransactionStateCommitted();
                            _transactionStateCommitted = temp;
                        }
                    }
                }

                return _transactionStateCommitted;
            }
        }

        
        protected static TransactionStateInDoubt _TransactionStateInDoubt
        {
            get
            {
                if (_transactionStateInDoubt == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateInDoubt == null)
                        {
                            TransactionStateInDoubt temp = new TransactionStateInDoubt();
                            _transactionStateInDoubt = temp;
                        }
                    }
                }

                return _transactionStateInDoubt;
            }
        }
        

        internal static TransactionStatePromoted _TransactionStatePromoted
        {
            get
            {
                if (_transactionStatePromoted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromoted == null)
                        {
                            TransactionStatePromoted temp = new TransactionStatePromoted();
                            _transactionStatePromoted = temp;
                        }
                    }
                }

                return _transactionStatePromoted;
            }
        }

        
        internal static TransactionStateNonCommittablePromoted _TransactionStateNonCommittablePromoted
        {
            get
            {
                if (_transactionStateNonCommittablePromoted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateNonCommittablePromoted == null)
                        {
                            TransactionStateNonCommittablePromoted temp = new TransactionStateNonCommittablePromoted();
                            _transactionStateNonCommittablePromoted = temp;
                        }
                    }
                }

                return _transactionStateNonCommittablePromoted;
            }
        }


        protected static TransactionStatePromotedP0Wave _TransactionStatePromotedP0Wave
        {
            get
            {
                if (_transactionStatePromotedP0Wave == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedP0Wave == null)
                        {
                            TransactionStatePromotedP0Wave temp = new TransactionStatePromotedP0Wave();
                            _transactionStatePromotedP0Wave = temp;
                        }
                    }
                }

                return _transactionStatePromotedP0Wave;
            }
        }

        
        protected static TransactionStatePromotedCommitting _TransactionStatePromotedCommitting
        {
            get
            {
                if (_transactionStatePromotedCommitting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedCommitting == null)
                        {
                            TransactionStatePromotedCommitting temp = new TransactionStatePromotedCommitting();
                            _transactionStatePromotedCommitting = temp;
                        }
                    }
                }

                return _transactionStatePromotedCommitting;
            }
        }

        protected static TransactionStatePromotedPhase0 _TransactionStatePromotedPhase0
        {
            get
            {
                if (_transactionStatePromotedPhase0 == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedPhase0 == null)
                        {
                            TransactionStatePromotedPhase0 temp = new TransactionStatePromotedPhase0();
                            _transactionStatePromotedPhase0 = temp;
                        }
                    }
                }

                return _transactionStatePromotedPhase0;
            }
        }

        
        protected static TransactionStatePromotedPhase1 _TransactionStatePromotedPhase1
        {
            get
            {
                if (_transactionStatePromotedPhase1 == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedPhase1 == null)
                        {
                            TransactionStatePromotedPhase1 temp = new TransactionStatePromotedPhase1();
                            _transactionStatePromotedPhase1 = temp;
                        }
                    }
                }

                return _transactionStatePromotedPhase1;
            }
        }

        
        protected static TransactionStatePromotedP0Aborting _TransactionStatePromotedP0Aborting
        {
            get
            {
                if (_transactionStatePromotedP0Aborting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedP0Aborting == null)
                        {
                            TransactionStatePromotedP0Aborting temp = new TransactionStatePromotedP0Aborting();
                            _transactionStatePromotedP0Aborting = temp;
                        }
                    }
                }

                return _transactionStatePromotedP0Aborting;
            }
        }


        protected static TransactionStatePromotedP1Aborting _TransactionStatePromotedP1Aborting
        {
            get
            {
                if (_transactionStatePromotedP1Aborting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedP1Aborting == null)
                        {
                            TransactionStatePromotedP1Aborting temp = new TransactionStatePromotedP1Aborting();
                            _transactionStatePromotedP1Aborting = temp;
                        }
                    }
                }

                return _transactionStatePromotedP1Aborting;
            }
        }


        protected static TransactionStatePromotedAborted _TransactionStatePromotedAborted
        {
            get
            {
                if (_transactionStatePromotedAborted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedAborted == null)
                        {
                            TransactionStatePromotedAborted temp = new TransactionStatePromotedAborted();
                            _transactionStatePromotedAborted = temp;
                        }
                    }
                }

                return _transactionStatePromotedAborted;
            }
        }


        protected static TransactionStatePromotedCommitted _TransactionStatePromotedCommitted
        {
            get
            {
                if (_transactionStatePromotedCommitted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedCommitted == null)
                        {
                            TransactionStatePromotedCommitted temp = new TransactionStatePromotedCommitted();
                            _transactionStatePromotedCommitted = temp;
                        }
                    }
                }

                return _transactionStatePromotedCommitted;
            }
        }


        protected static TransactionStatePromotedIndoubt _TransactionStatePromotedIndoubt
        {
            get
            {
                if (_transactionStatePromotedIndoubt == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedIndoubt == null)
                        {
                            TransactionStatePromotedIndoubt temp = new TransactionStatePromotedIndoubt();
                            _transactionStatePromotedIndoubt = temp;
                        }
                    }
                }

                return _transactionStatePromotedIndoubt;
            }
        }


        protected static TransactionStateDelegated _TransactionStateDelegated
        {
            get
            {
                if (_transactionStateDelegated == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateDelegated == null)
                        {
                            TransactionStateDelegated temp = new TransactionStateDelegated();
                            _transactionStateDelegated = temp;
                        }
                    }
                }

                return _transactionStateDelegated;
            }
        }

        internal static TransactionStateDelegatedSubordinate _TransactionStateDelegatedSubordinate
        {
            get
            {
                if (_transactionStateDelegatedSubordinate == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateDelegatedSubordinate == null)
                        {
                            TransactionStateDelegatedSubordinate temp = new TransactionStateDelegatedSubordinate();
                            _transactionStateDelegatedSubordinate = temp;
                        }
                    }
                }

                return _transactionStateDelegatedSubordinate;
            }
        }


        protected static TransactionStateDelegatedP0Wave _TransactionStateDelegatedP0Wave
        {
            get
            {
                if (_transactionStateDelegatedP0Wave == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateDelegatedP0Wave == null)
                        {
                            TransactionStateDelegatedP0Wave temp = new TransactionStateDelegatedP0Wave();
                            _transactionStateDelegatedP0Wave = temp;
                        }
                    }
                }

                return _transactionStateDelegatedP0Wave;
            }
        }

        
        protected static TransactionStateDelegatedCommitting _TransactionStateDelegatedCommitting
        {
            get
            {
                if (_transactionStateDelegatedCommitting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateDelegatedCommitting == null)
                        {
                            TransactionStateDelegatedCommitting temp = new TransactionStateDelegatedCommitting();
                            _transactionStateDelegatedCommitting = temp;
                        }
                    }
                }

                return _transactionStateDelegatedCommitting;
            }
        }

        protected static TransactionStateDelegatedAborting _TransactionStateDelegatedAborting
        {
            get
            {
                if (_transactionStateDelegatedAborting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateDelegatedAborting == null)
                        {
                            TransactionStateDelegatedAborting temp = new TransactionStateDelegatedAborting();
                            _transactionStateDelegatedAborting = temp;
                        }
                    }
                }

                return _transactionStateDelegatedAborting;
            }
        }


        protected static TransactionStateDelegatedNonMSDTC _TransactionStateDelegatedNonMSDTC
        {
            get
            {
                if (_transactionStateDelegatedNonMSDTC == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStateDelegatedNonMSDTC == null)
                        {
                            TransactionStateDelegatedNonMSDTC temp = new TransactionStateDelegatedNonMSDTC();
                            _transactionStateDelegatedNonMSDTC = temp;
                        }
                    }
                }

                return _transactionStateDelegatedNonMSDTC;
            }
        }

        protected static TransactionStatePromotedNonMSDTCPhase0 _TransactionStatePromotedNonMSDTCPhase0
        {
            get
            {
                if (_transactionStatePromotedNonMSDTCPhase0 == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedNonMSDTCPhase0 == null)
                        {
                            TransactionStatePromotedNonMSDTCPhase0 temp = new TransactionStatePromotedNonMSDTCPhase0();
                            _transactionStatePromotedNonMSDTCPhase0 = temp;
                        }
                    }
                }

                return _transactionStatePromotedNonMSDTCPhase0;
            }
        }

        protected static TransactionStatePromotedNonMSDTCVolatilePhase1 _TransactionStatePromotedNonMSDTCVolatilePhase1
        {
            get
            {
                if (_transactionStatePromotedNonMSDTCVolatilePhase1 == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedNonMSDTCVolatilePhase1 == null)
                        {
                            TransactionStatePromotedNonMSDTCVolatilePhase1 temp = new TransactionStatePromotedNonMSDTCVolatilePhase1();
                            _transactionStatePromotedNonMSDTCVolatilePhase1 = temp;
                        }
                    }
                }

                return _transactionStatePromotedNonMSDTCVolatilePhase1;
            }
        }

        protected static TransactionStatePromotedNonMSDTCSinglePhaseCommit _TransactionStatePromotedNonMSDTCSinglePhaseCommit
        {
            get
            {
                if (_transactionStatePromotedNonMSDTCSinglePhaseCommit == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedNonMSDTCSinglePhaseCommit == null)
                        {
                            TransactionStatePromotedNonMSDTCSinglePhaseCommit temp = new TransactionStatePromotedNonMSDTCSinglePhaseCommit();
                            _transactionStatePromotedNonMSDTCSinglePhaseCommit = temp;
                        }
                    }
                }

                return _transactionStatePromotedNonMSDTCSinglePhaseCommit;
            }
        }

        protected static TransactionStatePromotedNonMSDTCAborted _TransactionStatePromotedNonMSDTCAborted
        {
            get
            {
                if (_transactionStatePromotedNonMSDTCAborted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedNonMSDTCAborted == null)
                        {
                            TransactionStatePromotedNonMSDTCAborted temp = new TransactionStatePromotedNonMSDTCAborted();
                            _transactionStatePromotedNonMSDTCAborted = temp;
                        }
                    }
                }

                return _transactionStatePromotedNonMSDTCAborted;
            }
        }

        protected static TransactionStatePromotedNonMSDTCCommitted _TransactionStatePromotedNonMSDTCCommitted
        {
            get
            {
                if (_transactionStatePromotedNonMSDTCCommitted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedNonMSDTCCommitted == null)
                        {
                            TransactionStatePromotedNonMSDTCCommitted temp = new TransactionStatePromotedNonMSDTCCommitted();
                            _transactionStatePromotedNonMSDTCCommitted = temp;
                        }
                    }
                }

                return _transactionStatePromotedNonMSDTCCommitted;
            }
        }

        protected static TransactionStatePromotedNonMSDTCIndoubt _TransactionStatePromotedNonMSDTCIndoubt
        {
            get
            {
                if (_transactionStatePromotedNonMSDTCIndoubt == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_transactionStatePromotedNonMSDTCIndoubt == null)
                        {
                            TransactionStatePromotedNonMSDTCIndoubt temp = new TransactionStatePromotedNonMSDTCIndoubt();
                            _transactionStatePromotedNonMSDTCIndoubt = temp;
                        }
                    }
                }

                return _transactionStatePromotedNonMSDTCIndoubt;
            }
        }

        // Helper object for static synchronization
        internal static object ClassSyncObject
        {
            get
            {
                if ( classSyncObject == null )
                {
                    object o = new object();
                    Interlocked.CompareExchange( ref classSyncObject, o, null );
                }
                return classSyncObject;
            }
        }


        internal void CommonEnterState( InternalTransaction tx )
        {
            Debug.Assert( tx.State != this, "Changing to the same state." );
            tx.State = this;

#if DEBUG
            tx.stateHistory[tx.currentStateHist] = this;
            if ( ++tx.currentStateHist > InternalTransaction.MaxStateHist )
            {
                tx.currentStateHist = 0;
            }
#endif
        }



        // Every state must override EnterState
        internal abstract void EnterState( InternalTransaction tx );


        internal virtual void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void EndCommit( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));

            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void Rollback( InternalTransaction tx, Exception e )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier, 
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier, 
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }
        

        internal virtual Enlistment EnlistVolatile(
            InternalTransaction tx,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void CheckForFinishedTransaction( InternalTransaction tx )
        {
            // Aborted & InDoubt states should throw exceptions.
        }


        // If a specific state does not have a story for identifiers then
        // it simply gets a guid.  This would be to handle cases like aborted
        // and committed where the transaction has not been promoted and
        // cannot be promoted so it doesn't matter what guid is returned.
        //
        // This leaves two specific sets of states that MUST override this...
        // 1) Any state where the transaction could be promoted.
        // 2) Any state where the transaction is already promoted.
        internal virtual Guid get_Identifier( InternalTransaction tx )
        {
            return Guid.Empty;
        }


        // Every state derived from the base must override status
        internal abstract TransactionStatus get_Status( InternalTransaction tx );

        
        internal virtual void AddOutcomeRegistrant( InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual bool EnlistPromotableSinglePhase( 
            InternalTransaction tx, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void CompleteBlockingClone( InternalTransaction tx )
        {
        }


        internal virtual void CompleteAbortingClone( InternalTransaction tx )
        {
        }


        internal virtual void CreateBlockingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void CreateAbortingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }

        
        internal virtual void ChangeStateTransactionCommitted( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void InDoubtFromEnlistment( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void ChangeStatePromotedAborted( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void ChangeStatePromotedCommitted( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void InDoubtFromDtc( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void ChangeStatePromotedPhase0( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void ChangeStatePromotedPhase1( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void ChangeStateAbortedDuringPromotion( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException();
        }


        internal virtual void Timeout( InternalTransaction tx )
        {
        }


        internal virtual void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }

        
        internal virtual void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal virtual void RestartCommitIfNeeded( InternalTransaction tx )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for State; Current State: {0}", this.GetType() ));
            if ( DiagnosticTrace.Error )
            {
                InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), "" );
            }

            throw new InvalidOperationException( );
        }


        internal virtual bool ContinuePhase0Prepares()
        {
            return false;
        }


        internal virtual bool ContinuePhase1Prepares()
        {
            return false;
        }


        internal virtual void Promote( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }

        internal virtual byte[] PromotedToken( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal virtual Enlistment PromoteAndEnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier,
            IPromotableSinglePhaseNotification promotableNotification,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal virtual void SetDistributedTransactionId(InternalTransaction tx,
                    IPromotableSinglePhaseNotification promotableNotification,
                    Guid distributedTransactionIdentifier)
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal virtual void DisposeRoot(InternalTransaction tx)
        {
        }


        internal virtual bool IsCompleted( InternalTransaction tx )
        {
            tx.needPulse = true;

            return false;
        }

        protected void AddVolatileEnlistment( ref VolatileEnlistmentSet enlistments, Enlistment enlistment )
        {
            // Grow the enlistment array if necessary.
            if ( enlistments.volatileEnlistmentCount == enlistments.volatileEnlistmentSize )
            {
                InternalEnlistment[] newEnlistments = 
                    new InternalEnlistment[enlistments.volatileEnlistmentSize + InternalTransaction.volatileArrayIncrement];

                if ( enlistments.volatileEnlistmentSize > 0 )
                {
                    Array.Copy(
                        enlistments.volatileEnlistments, 
                        newEnlistments, 
                        enlistments.volatileEnlistmentSize
                        );
                }

                enlistments.volatileEnlistmentSize += InternalTransaction.volatileArrayIncrement;
                enlistments.volatileEnlistments = newEnlistments;
            }

            // Add a new element to the end of the list
            enlistments.volatileEnlistments[enlistments.volatileEnlistmentCount] = enlistment.InternalEnlistment;
            enlistments.volatileEnlistmentCount++;

            // Make it's state active.
            VolatileEnlistmentState._VolatileEnlistmentActive.EnterState( 
                enlistments.volatileEnlistments[enlistments.volatileEnlistmentCount - 1]);
        }
    }



    // ActiveStates
    //
    // All states for which the transaction is not done should derive from this state.
    internal abstract class ActiveStates : TransactionState
    {
        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.Active;
        }


        internal override void AddOutcomeRegistrant( InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate )
        {
            tx.transactionCompletedDelegate = (TransactionCompletedEventHandler)
                System.Delegate.Combine( tx.transactionCompletedDelegate, transactionCompletedDelegate );
        }
    }



    // EnlistableStates
    //
    // States for which it is ok to enlist.
    internal abstract class EnlistableStates : ActiveStates
    {
        internal override Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier, 
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            // Can't support an enlistment that dosn't support SPC
            tx.promoteState.EnterState( tx );
            // Note that just because we did an EnterState above does not mean that the state will be
            // the same when the next method is called.
            return tx.State.EnlistDurable( tx, resourceManagerIdentifier, enlistmentNotification, enlistmentOptions, atomicTransaction );
        }


        internal override Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier, 
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            if (tx.durableEnlistment != null || (enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0)
            {
                // These circumstances cause promotion
                tx.promoteState.EnterState( tx );
                return tx.State.EnlistDurable( tx, resourceManagerIdentifier, enlistmentNotification, enlistmentOptions, atomicTransaction );
            }

            // Create a durable enlistment
            Enlistment en = new Enlistment( resourceManagerIdentifier, tx, enlistmentNotification, enlistmentNotification, atomicTransaction );
            tx.durableEnlistment = en.InternalEnlistment;
            DurableEnlistmentState._DurableEnlistmentActive.EnterState( tx.durableEnlistment );

            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.durableEnlistment.EnlistmentTraceId,
                    EnlistmentType.Durable,
                    EnlistmentOptions.None
                    );
            }

            return en;
        }


        internal override void Timeout( InternalTransaction tx )
        {
            if ( DiagnosticTrace.Warning )
            {
                TransactionTimeoutTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

            TimeoutException e = new TimeoutException( SR.GetString( SR.TraceTransactionTimeout ));
            this.Rollback( tx, e );
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            // This is not allowed if the transaction's PromoterType is not MSDTC.
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            // Promote the transaction.
            tx.promoteState.EnterState( tx );
 
            // Forward this call
            tx.State.GetObjectData( tx, serializationInfo, context );
        }


        internal override void CompleteBlockingClone( InternalTransaction tx )
        {
            // A blocking clone simulates a phase 0 volatile

            // decrement the number of dependentClones
            tx.phase0Volatiles.dependentClones--;
            Debug.Assert( tx.phase0Volatiles.dependentClones >= 0 );

            // Make certain we increment the right list.
            Debug.Assert( tx.phase0Volatiles.preparedVolatileEnlistments <= 
                tx.phase0Volatiles.volatileEnlistmentCount + tx.phase0Volatiles.dependentClones );

            // Check to see if all of the volatile enlistments are done.
            if ( tx.phase0Volatiles.preparedVolatileEnlistments == 
                tx.phase0VolatileWaveCount + tx.phase0Volatiles.dependentClones )
            {
                tx.State.Phase0VolatilePrepareDone( tx );
            }
        }


        internal override void CompleteAbortingClone( InternalTransaction tx )
        {
            // A blocking clone simulates a phase 1 volatile
            // 
            // Unlike a blocking clone however the aborting clones need to be accounted
            // for specifically.  So when one is complete remove it from the list.
            tx.phase1Volatiles.dependentClones--;
            Debug.Assert( tx.phase1Volatiles.dependentClones >= 0 );
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            // A blocking clone simulates a phase 0 volatile
            tx.phase0Volatiles.dependentClones++;
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            // An aborting clone simulates a phase 1 volatile
            tx.phase1Volatiles.dependentClones++;
        }


        internal override void Promote( InternalTransaction tx )
        {

            tx.promoteState.EnterState( tx );
            tx.State.CheckForFinishedTransaction( tx );
        }

        internal override byte[] PromotedToken(InternalTransaction tx)
        {
            if (tx.promotedToken == null)
            {
                tx.promoteState.EnterState(tx);
                tx.State.CheckForFinishedTransaction(tx);
            }

            return tx.promotedToken;
        }
    }



    // TransactionStateActive 
    //
    // Transaction state before commit has been called
    internal class TransactionStateActive : EnlistableStates
    {
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );

            // Yeah it's active.
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Store the given values
            tx.asyncCommit = asyncCommit;
            tx.asyncCallback = asyncCallback;
            tx.asyncState = asyncState;

            // Start the process for commit.
            _TransactionStatePhase0.EnterState( tx );
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            // Start the process for abort.  From the active state we can transition directly
            // to the aborted state.

            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            _TransactionStateAborted.EnterState( tx );
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Enlistment enlistment = new Enlistment( tx, enlistmentNotification, null, atomicTransaction, enlistmentOptions );
            if ( (enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0 )
            {
                AddVolatileEnlistment( ref tx.phase0Volatiles, enlistment );
            }
            else
            {
                AddVolatileEnlistment( ref tx.phase1Volatiles, enlistment );
            }

            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    enlistment.InternalEnlistment.EnlistmentTraceId,
                    EnlistmentType.Volatile,
                    enlistmentOptions
                    );
            }

            return enlistment;
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Enlistment enlistment = new Enlistment( tx, enlistmentNotification, enlistmentNotification, atomicTransaction, enlistmentOptions );
            if ( (enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0 )
            {
                AddVolatileEnlistment( ref tx.phase0Volatiles, enlistment );
            }
            else
            {
                AddVolatileEnlistment( ref tx.phase1Volatiles, enlistment );
            }

            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    enlistment.InternalEnlistment.EnlistmentTraceId,
                    EnlistmentType.Volatile,
                    enlistmentOptions
                    );
            }

            return enlistment;
        }


        internal override bool EnlistPromotableSinglePhase( 
            InternalTransaction tx, IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            // Delegation will fail if there is a durable enlistment
            if ( tx.durableEnlistment != null )
            {
                return false;
            }

            _TransactionStatePSPEOperation.PSPEInitialize( tx, promotableSinglePhaseNotification, promoterType );

            // Create a durable enlistment.
            Enlistment en = new Enlistment( tx, promotableSinglePhaseNotification, atomicTransaction );
            tx.durableEnlistment = en.InternalEnlistment;
            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.durableEnlistment.EnlistmentTraceId,
                    EnlistmentType.PromotableSinglePhase,
                    EnlistmentOptions.None
                    );
            }

            // Specify the promoter for the transaction.
            tx.promoter = promotableSinglePhaseNotification;

            // Change the state that the transaction will promote to.  Normally this would be simply
            // be TransactionStatePromoted.  However it now needs to promote to a delegated state.
            // If the PromoterType is NOT TransactionInterop.PromoterTypeDtc, then the promoteState needs
            // to be _TransactionStateDelegatedNonMSDTC.
            // tx.PromoterType was set in PSPEInitialize.
            Debug.Assert(tx.promoterType != Guid.Empty, "InternalTransaction.PromoterType was not set in PSPEInitialize");
            if (tx.promoterType == TransactionInterop.PromoterTypeDtc)
            {
                tx.promoteState = _TransactionStateDelegated;
            }
            else
            {
                tx.promoteState = _TransactionStateDelegatedNonMSDTC;
            }

            // Pud the enlistment in an active state
            DurableEnlistmentState._DurableEnlistmentActive.EnterState( tx.durableEnlistment );

            // Hand back the enlistment.
            return true;
        }


        // Volatile prepare is done for
        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            // Ignore this event at the moment.  It can be checked again in Phase0
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            // Ignore this event at the moment.  It can be checked again in Phase1
        }


        internal override void DisposeRoot( InternalTransaction tx )
        {
            tx.State.Rollback( tx, null );
        }

    }


    // TransactionStateSubordinateActive
    //
    // This is a transaction that is a very basic subordinate to some external TM.
    internal class TransactionStateSubordinateActive : TransactionStateActive
    {
        // Every state must override EnterState
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );

            Debug.Assert( tx.promoter != null, "Transaction Promoter is Null entering SubordinateActive" );
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            // Start the process for abort.  From the active state we can transition directly
            // to the aborted state.

            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            ((ISimpleTransactionSuperior)tx.promoter).Rollback();
            _TransactionStateAborted.EnterState( tx );
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            tx.promoteState.EnterState( tx );
            return tx.State.EnlistVolatile( tx, enlistmentNotification, enlistmentOptions, atomicTransaction );
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            tx.promoteState.EnterState( tx );
            return tx.State.EnlistVolatile( tx, enlistmentNotification, enlistmentOptions, atomicTransaction );
        }


        // Every state derived from the base must override status
        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            tx.promoteState.EnterState( tx );
            return tx.State.get_Status( tx );
        }

        
        internal override void AddOutcomeRegistrant( InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate )
        {
            tx.promoteState.EnterState( tx );
            tx.State.AddOutcomeRegistrant( tx, transactionCompletedDelegate );
        }


        internal override bool EnlistPromotableSinglePhase( 
            InternalTransaction tx, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            return false;
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            tx.promoteState.EnterState( tx );
            tx.State.CreateBlockingClone( tx );
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            tx.promoteState.EnterState( tx );
            tx.State.CreateAbortingClone( tx );
        }
    }


    // TransactionStatePhase0
    //
    // A transaction that is in the beginning stage of committing.
    internal class TransactionStatePhase0 : EnlistableStates
    {
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );

            // Get a copy of the current volatile enlistment count before entering this loop so that other 
            // threads don't affect the operation of this loop.
            int volatileCount = tx.phase0Volatiles.volatileEnlistmentCount;
            int dependentCount = tx.phase0Volatiles.dependentClones;

            // Store the number of phase0 volatiles for this wave.
            tx.phase0VolatileWaveCount = volatileCount;

            // Check for volatile enlistments
            if ( tx.phase0Volatiles.preparedVolatileEnlistments < volatileCount + dependentCount )
            {
                // Broadcast prepare to the phase 0 enlistments
                for ( int i = 0; i < volatileCount; i++ )
                {
                    tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing( tx.phase0Volatiles.volatileEnlistments[i]);
                    if ( !tx.State.ContinuePhase0Prepares() )
                    {
                        break;
                    }
                }
            }
            else
            {
                // No volatile enlistments.  Start phase 1.
                _TransactionStateVolatilePhase1.EnterState( tx );
            }
        }


        internal override Enlistment EnlistDurable( 
            InternalTransaction tx, 
            Guid resourceManagerIdentifier, 
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            Enlistment en = base.EnlistDurable(tx, resourceManagerIdentifier, enlistmentNotification, 
                enlistmentOptions, atomicTransaction );

            // Calling durable enlist in Phase0 may cause the transaction to promote.  Leverage the promoted
            tx.State.RestartCommitIfNeeded( tx );
            return en;
        }


        internal override Enlistment EnlistDurable(
            InternalTransaction tx, 
            Guid resourceManagerIdentifier, 
            ISinglePhaseNotification enlistmentNotification, 
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            Enlistment en = base.EnlistDurable(tx, resourceManagerIdentifier, enlistmentNotification, 
                enlistmentOptions, atomicTransaction );

            // Calling durable enlist in Phase0 may cause the transaction to promote.  Leverage the promoted
            tx.State.RestartCommitIfNeeded( tx );
            return en;
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Enlistment enlistment = new Enlistment( tx, enlistmentNotification, null, atomicTransaction, enlistmentOptions );
            if ( (enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0 )
            {
                AddVolatileEnlistment( ref tx.phase0Volatiles, enlistment );
            }
            else
            {
                AddVolatileEnlistment( ref tx.phase1Volatiles, enlistment );
            }

            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    enlistment.InternalEnlistment.EnlistmentTraceId,
                    EnlistmentType.Volatile,
                    enlistmentOptions
                    );
            }

            return enlistment;
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Enlistment enlistment = new Enlistment( tx, enlistmentNotification, enlistmentNotification, atomicTransaction, enlistmentOptions );

            if ( (enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0 )
            {
                AddVolatileEnlistment( ref tx.phase0Volatiles, enlistment );
            }
            else
            {
                AddVolatileEnlistment( ref tx.phase1Volatiles, enlistment );
            }

            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    enlistment.InternalEnlistment.EnlistmentTraceId,
                    EnlistmentType.Volatile,
                    enlistmentOptions
                    );
            }

            return enlistment;
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            ChangeStateTransactionAborted( tx, e );
        }


        // Support PSPE enlistment during Phase0 prepare notification.
        
        internal override bool EnlistPromotableSinglePhase( 
            InternalTransaction tx, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
    
            // Delegation will fail if there is a durable enlistment
            if ( tx.durableEnlistment != null )
            {
                return false;
            }

            // Initialize PSPE Operation and call initialize on IPromotableSinglePhaseNotification
            _TransactionStatePSPEOperation.Phase0PSPEInitialize( tx, promotableSinglePhaseNotification, promoterType );

            // Create a durable enlistment.
            Enlistment en = new Enlistment( tx, promotableSinglePhaseNotification, atomicTransaction );
            tx.durableEnlistment = en.InternalEnlistment;
            if ( DiagnosticTrace.Information )
            {
                EnlistmentTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.durableEnlistment.EnlistmentTraceId,
                    EnlistmentType.PromotableSinglePhase,
                    EnlistmentOptions.None
                    );
            }

            // Specify the promoter for the transaction.
            tx.promoter = promotableSinglePhaseNotification;

            // Change the state that the transaction will promote to.  Normally this would be simply
            // be TransactionStatePromoted.  However it now needs to promote to a delegated state.
            // If the PromoterType is NOT TransactionInterop.PromoterTypeDtc, then the promoteState needs
            // to be _TransactionStateDelegatedNonMSDTC.
            // tx.PromoterType was set in Phase0PSPEInitialize.
            Debug.Assert(tx.promoterType != Guid.Empty, "InternalTransaction.PromoterType was not set in Phase0PSPEInitialize");
            if (tx.promoterType == TransactionInterop.PromoterTypeDtc)
            {
                tx.promoteState = _TransactionStateDelegated;
            }
            else
            {
                tx.promoteState = _TransactionStateDelegatedNonMSDTC;
            }

            // Put the enlistment in an active state
            DurableEnlistmentState._DurableEnlistmentActive.EnterState( tx.durableEnlistment );
           
            // Hand back the enlistment.
            return true;
        }

        // Volatile prepare is done for
        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            // Check to see if any Phase0Volatiles have been added in Phase0.
            // If so go through the list again.
            
            // Get a copy of the current volatile enlistment count before entering this loop so that other 
            // threads don't affect the operation of this loop.
            int volatileCount = tx.phase0Volatiles.volatileEnlistmentCount;
            int dependentCount = tx.phase0Volatiles.dependentClones;

            // Store the number of phase0 volatiles for this wave.
            tx.phase0VolatileWaveCount = volatileCount;

            // Check for volatile enlistments
            if ( tx.phase0Volatiles.preparedVolatileEnlistments < volatileCount + dependentCount )
            {
                // Broadcast prepare to the phase 0 enlistments
                for ( int i = 0; i < volatileCount; i++ )
                {
                    tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(tx.phase0Volatiles.volatileEnlistments[i]);
                    if ( !tx.State.ContinuePhase0Prepares() )
                    {
                        break;
                    }
                }
            }
            else
            {
                // No volatile enlistments.  Start phase 1.
                _TransactionStateVolatilePhase1.EnterState( tx );
            }
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            // Ignore this for now it can be checked again in Phase 1
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            // Commit does not need to be restarted
        }

        internal override bool ContinuePhase0Prepares()
        {
            return true;
        }

        internal override void Promote( InternalTransaction tx )
        {

            tx.promoteState.EnterState( tx );
            tx.State.CheckForFinishedTransaction( tx );
            tx.State.RestartCommitIfNeeded( tx );
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }
            
            _TransactionStateAborted.EnterState( tx );
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            // This is not allowed if the transaction's PromoterType is not MSDTC.
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            // Promote the transaction.
            tx.promoteState.EnterState( tx );

            // Forward this call
            tx.State.GetObjectData( tx, serializationInfo, context );

            // Restart the commit process.
            tx.State.RestartCommitIfNeeded( tx );
        }

    }

    // TransactionStateVolatilePhase1 
    //
    // Represents the transaction state during phase 1 preparing volatile enlistments
    internal class TransactionStateVolatilePhase1 : ActiveStates
    {
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );

            // Mark the committable transaction as complete.
            tx.committableTransaction.complete = true;

            // If at this point there are phase1 dependent clones abort the transaction
            if ( tx.phase1Volatiles.dependentClones != 0 )
            {
                _TransactionStateAborted.EnterState( tx );
                return;
            }

            if ( tx.phase1Volatiles.volatileEnlistmentCount == 1 && tx.durableEnlistment == null 
                && tx.phase1Volatiles.volatileEnlistments[0].SinglePhaseNotification != null )
            {
                // This is really a case of SPC for volatiles
                _TransactionStateVolatileSPC.EnterState( tx );
            }
            else if ( tx.phase1Volatiles.volatileEnlistmentCount > 0 )
            {
                // Broadcast prepare to the phase 0 enlistments
                for ( int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++ )
                {
                    tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(tx.phase1Volatiles.volatileEnlistments[i]);
                    if ( !tx.State.ContinuePhase1Prepares() )
                    {
                        break;
                    }
                }
            }
            else
            {
                // No volatile phase 1 enlistments.  Start phase durable SPC.
                _TransactionStateSPC.EnterState( tx );
            }
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            ChangeStateTransactionAborted( tx, e );
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }
            
            _TransactionStateAborted.EnterState( tx );
        }


        // Volatile prepare is done for
        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            _TransactionStateSPC.EnterState( tx );
        }


        internal override bool ContinuePhase1Prepares()
        {
            return true;
        }

        internal override void Timeout( InternalTransaction tx )
        {
            if ( DiagnosticTrace.Warning )
            {
                TransactionTimeoutTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

            TimeoutException e = new TimeoutException( SR.GetString( SR.TraceTransactionTimeout ));
            this.Rollback( tx, e );
        }
    }

    
    // TransactionStateVolatileSPC 
    //
    // Represents the transaction state during phase 1 when issuing SPC to a volatile enlistment
    internal class TransactionStateVolatileSPC : ActiveStates
    {
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );

            Debug.Assert( tx.phase1Volatiles.volatileEnlistmentCount == 1, 
                "There must be exactly 1 phase 1 volatile enlistment for TransactionStateVolatileSPC" );

            tx.phase1Volatiles.volatileEnlistments[0].twoPhaseState.ChangeStateSinglePhaseCommit(
                tx.phase1Volatiles.volatileEnlistments[0]);
        }


        internal override void ChangeStateTransactionCommitted( InternalTransaction tx )
        {
            // The durable enlistment must have committed.  Go to the committed state.
            _TransactionStateCommitted.EnterState( tx );
        }


        internal override void InDoubtFromEnlistment( InternalTransaction tx )
        {
            // The transaction is indoubt
            _TransactionStateInDoubt.EnterState( tx );
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            // The durable enlistment must have aborted.  Go to the aborted state.
            _TransactionStateAborted.EnterState( tx );
        }
    }

    
    
    // TransactionStateSPC 
    //
    // Represents the transaction state during phase 1
    internal class TransactionStateSPC : ActiveStates
    {
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );

            // Check for a durable enlistment
            if ( tx.durableEnlistment != null )
            {
                // Send SPC to the durable enlistment
                tx.durableEnlistment.State.ChangeStateCommitting( tx.durableEnlistment );
            }
            else
            {
                // No durable enlistments.  Go to the committed state.
                _TransactionStateCommitted.EnterState( tx );
            }
        }


        internal override void ChangeStateTransactionCommitted( InternalTransaction tx )
        {
            // The durable enlistment must have committed.  Go to the committed state.
            _TransactionStateCommitted.EnterState( tx );
        }


        internal override void InDoubtFromEnlistment( InternalTransaction tx )
        {
            // The transaction is indoubt
            _TransactionStateInDoubt.EnterState( tx );
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }
            
            // The durable enlistment must have aborted.  Go to the aborted state.
            _TransactionStateAborted.EnterState( tx );
        }
    }


    // TransactionStateEnded
    //
    // This state indicates that the transaction is in some form of ended state.
    internal abstract class TransactionStateEnded : TransactionState
    {
        internal override void EnterState( InternalTransaction tx )
        {
            if ( tx.needPulse )
            {
                System.Threading.Monitor.Pulse( tx );
            }
        }

        
        internal override void AddOutcomeRegistrant( InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate )
        {
            if ( transactionCompletedDelegate != null )
            {
                TransactionEventArgs args = new TransactionEventArgs();
                args.transaction = tx.outcomeSource.InternalClone();
                transactionCompletedDelegate( args.transaction, args );
            }
        }


        internal override bool IsCompleted( InternalTransaction tx )
        {
            return true;
        }
    }


    // TransactionStateAborted
    //
    // The transaction has been aborted.  Abort is itempotent and can be called again but any
    // other operations on the transaction should fail.
    internal class TransactionStateAborted : TransactionStateEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );
            
            // Set the transaction state
            CommonEnterState( tx );

            // Do NOT mark the committable transaction as complete because it is aborting.

            // Notify the enlistments that the transaction has aborted
            for ( int i = 0; i < tx.phase0Volatiles.volatileEnlistmentCount; i++ )
            {
                tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.InternalAborted(tx.phase0Volatiles.volatileEnlistments[i]);
            }

            for ( int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++ )
            {
                tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.InternalAborted(tx.phase1Volatiles.volatileEnlistments[i]);
            }

            // Notify the durable enlistment
            if ( tx.durableEnlistment != null )
            {
                tx.durableEnlistment.State.InternalAborted( tx.durableEnlistment );
            }

            // Remove this from the timeout list
            TransactionManager.TransactionTable.Remove( tx );

            if ( DiagnosticTrace.Warning )
            {
                TransactionAbortedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

            // Fire Completion for anyone listening
            tx.FireCompletion( );

            // Check to see if we need to release some waiter.
            if ( tx.asyncCommit )
            {
                tx.SignalAsyncCompletion();
            }
        }

        
        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.Aborted;
        }

        
        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            // Abort is itempotent.  Ignore this if the transaction is already aborted.
        }


        internal override void BeginCommit(InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState)
        {
            // End Commit Must throw a TransactionAbortedException to let the caller know that the tx aborted.
            throw CreateTransactionAbortedException( tx );
        }


        internal override void EndCommit( InternalTransaction tx )
        {
            // End Commit Must throw a TransactionAbortedException to let the caller know that the tx aborted.
            throw CreateTransactionAbortedException( tx );
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            // Commit does not need to be restarted.
        }


        internal override void Timeout( InternalTransaction tx )
        {
            // The transaction has aborted already
        }


        // When all enlisments respond to prepare this event will fire.
        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            // Since the transaction is aborted ignore it.
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            // Since the transaction is aborted ignore it.
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            // Yes, yes, yes... I already know.
        }


        internal override void ChangeStatePromotedAborted(InternalTransaction tx)
        {
            // The transaction must have aborted during promotion
        }


        internal override void ChangeStateAbortedDuringPromotion(InternalTransaction tx)
        {
            // This is fine too.
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            throw CreateTransactionAbortedException( tx );
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            throw CreateTransactionAbortedException( tx );
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            throw CreateTransactionAbortedException( tx );
        }


        internal override void CheckForFinishedTransaction( InternalTransaction tx )
        {
            throw CreateTransactionAbortedException( tx );
        }


        private TransactionException CreateTransactionAbortedException( InternalTransaction tx )
        {
            return TransactionAbortedException.Create( SR.GetString( SR.TraceSourceLtm), SR.GetString( SR.TransactionAborted ), tx.innerException, tx.DistributedTxId);
        }
    }



    // TransactionStateCommitted
    //
    // This state indicates that the transaction has been committed.  Basically any
    // operations on the transaction should fail at this point.
    internal class TransactionStateCommitted : TransactionStateEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );
            
            // Set the transaction state
            CommonEnterState( tx );

            // Notify the phase 0 enlistments that the transaction has aborted
            for ( int i = 0; i < tx.phase0Volatiles.volatileEnlistmentCount; i++ )
            {
                tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.InternalCommitted(tx.phase0Volatiles.volatileEnlistments[i]);
            }

            // Notify the phase 1 enlistments that the transaction has aborted
            for ( int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++ )
            {
                tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.InternalCommitted(tx.phase1Volatiles.volatileEnlistments[i]);
            }

            // Remove this from the timeout list
            TransactionManager.TransactionTable.Remove( tx );

            if ( DiagnosticTrace.Verbose )
            {
                TransactionCommittedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

            // Fire Completion for anyone listening
            tx.FireCompletion( );

            // Check to see if we need to release some waiter.
            if ( tx.asyncCommit )
            {
                tx.SignalAsyncCompletion();
            }
        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.Committed;
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void EndCommit(InternalTransaction tx)
        {
            // End Commit does nothing because life is wonderful and we are happy!
        }
    }


    // TransactionStateInDoubt
    //
    // This state indicates that the transaction is in doubt
    internal class TransactionStateInDoubt : TransactionStateEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );
            
            // Set the transaction state
            CommonEnterState( tx );

            // Notify the phase 0 enlistments that the transaction has aborted
            for ( int i = 0; i < tx.phase0Volatiles.volatileEnlistmentCount; i++ )
            {
                tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.InternalIndoubt(tx.phase0Volatiles.volatileEnlistments[i]);
            }

            // Notify the phase 1 enlistments that the transaction has aborted
            for ( int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++ )
            {
                tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.InternalIndoubt(tx.phase1Volatiles.volatileEnlistments[i]);
            }

            // Remove this from the timeout list
            TransactionManager.TransactionTable.Remove( tx );

            if ( DiagnosticTrace.Warning )
            {
                TransactionInDoubtTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

            // Fire Completion for anyone listening
            tx.FireCompletion( );

            // Check to see if we need to release some waiter.
            if ( tx.asyncCommit )
            {
                tx.SignalAsyncCompletion();
            }
        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.InDoubt;
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void EndCommit(InternalTransaction tx)
        {
            throw TransactionInDoubtException.Create( SR.GetString( SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }


        internal override void CheckForFinishedTransaction( InternalTransaction tx )
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }
    }



    // TransactionStatePromotedBase
    //
    // This is the base class for promoted states.  It's main function is to pass calls
    // through to the distributed transaction.
    internal abstract class TransactionStatePromotedBase : TransactionState
    {
        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            // Since the distributed transaction manager will always tell the ltm about state
            // changes via the enlistment that the Ltm has with it, the Ltm can tell client
            // code what it thinks the state is on behalf of the distributed tm.  Doing so
            // prevents ----s with state changes of the promoted tx to the Ltm being
            // told about those changes.
            return TransactionStatus.Active;
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );
            // Don't get in the way for new volatile enlistments

            // Don't hold locks while calling into the promoted tx
            System.Threading.Monitor.Exit( tx );
            try
            {
                Enlistment en = new Enlistment( enlistmentNotification, tx, atomicTransaction );
                EnlistmentState._EnlistmentStatePromoted.EnterState( en.InternalEnlistment );

                en.InternalEnlistment.PromotedEnlistment = 
                    tx.PromotedTransaction.EnlistVolatile( 
                        en.InternalEnlistment, enlistmentOptions );
                return en;
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );
            // Don't get in the way for new volatile enlistments

            // Don't hold locks while calling into the promoted tx
            System.Threading.Monitor.Exit( tx );
            try
            {
                Enlistment en = new Enlistment( enlistmentNotification, tx, atomicTransaction );
                EnlistmentState._EnlistmentStatePromoted.EnterState( en.InternalEnlistment );

                en.InternalEnlistment.PromotedEnlistment = 
                    tx.PromotedTransaction.EnlistVolatile( 
                        en.InternalEnlistment, enlistmentOptions );
                return en;
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }

        
        internal override Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier, 
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );

            tx.ThrowIfPromoterTypeIsNotMSDTC();

            // Don't hold locks while calling into the promoted tx
            System.Threading.Monitor.Exit( tx );
            try
            {
                Enlistment en = new Enlistment( 
                    resourceManagerIdentifier,
                    tx, 
                    enlistmentNotification,
                    null,
                    atomicTransaction 
                    );
                EnlistmentState._EnlistmentStatePromoted.EnterState( en.InternalEnlistment );

                en.InternalEnlistment.PromotedEnlistment = 
                    tx.PromotedTransaction.EnlistDurable( 
                        resourceManagerIdentifier, 
                        (DurableInternalEnlistment)en.InternalEnlistment, 
                        false, 
                        enlistmentOptions 
                        );
                return en;
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier, 
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );

            tx.ThrowIfPromoterTypeIsNotMSDTC();

            // Don't hold locks while calling into the promoted tx
            System.Threading.Monitor.Exit( tx );
            try
            {
                Enlistment en = new Enlistment( 
                    resourceManagerIdentifier,
                    tx, 
                    enlistmentNotification,
                    enlistmentNotification,
                    atomicTransaction 
                    );
                EnlistmentState._EnlistmentStatePromoted.EnterState( en.InternalEnlistment );

                en.InternalEnlistment.PromotedEnlistment = 
                    tx.PromotedTransaction.EnlistDurable( 
                        resourceManagerIdentifier, 
                        (DurableInternalEnlistment)en.InternalEnlistment, 
                        true, 
                        enlistmentOptions 
                        );
                return en;
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );
            // Forward this on to the promoted transaction.

            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            // Don't hold locks while calling into the promoted tx
            System.Threading.Monitor.Exit( tx );
            try
            {
                tx.PromotedTransaction.Rollback( );
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override Guid get_Identifier( InternalTransaction tx )
        {
            if (tx != null && tx.PromotedTransaction != null)
            {
                return tx.PromotedTransaction.Identifier;
            }
            else
            {
                return Guid.Empty;
            }
        }


        internal override void AddOutcomeRegistrant( InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate )
        {
            // Add this guy to the list of people to be notified of the outcome.
            tx.transactionCompletedDelegate = (TransactionCompletedEventHandler)
                System.Delegate.Combine( tx.transactionCompletedDelegate, transactionCompletedDelegate );
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Store the given values
            tx.asyncCommit = asyncCommit;
            tx.asyncCallback = asyncCallback;
            tx.asyncState = asyncState;

            // Start the commit process.
            _TransactionStatePromotedCommitting.EnterState( tx );
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            _TransactionStatePromotedP0Wave.EnterState( tx );
        }


        internal override bool EnlistPromotableSinglePhase( 
            InternalTransaction tx, IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            // The transaction has been promoted and cannot support a promotable singe phase enlistment
            return false;
        }


        internal override void CompleteBlockingClone( InternalTransaction tx )
        {
            // First try to complete one of the internal blocking clones
            if ( tx.phase0Volatiles.dependentClones > 0 )
            {
                // decrement the number of clones
                tx.phase0Volatiles.dependentClones--;

                // Make certain we increment the right list.
                Debug.Assert( tx.phase0Volatiles.preparedVolatileEnlistments <= 
                    tx.phase0Volatiles.volatileEnlistmentCount + tx.phase0Volatiles.dependentClones );

                // Check to see if all of the volatile enlistments are done.
                if ( tx.phase0Volatiles.preparedVolatileEnlistments == 
                    tx.phase0VolatileWaveCount + tx.phase0Volatiles.dependentClones )
                {
                    tx.State.Phase0VolatilePrepareDone( tx );
                }
            }
            else
            {
                // Otherwise this must be a dependent clone created after promotion
                tx.phase0WaveDependentCloneCount--;
                Debug.Assert( tx.phase0WaveDependentCloneCount >= 0 );
                if ( tx.phase0WaveDependentCloneCount == 0 )
                {
                    Oletx.OletxDependentTransaction dtx = tx.phase0WaveDependentClone;
                    tx.phase0WaveDependentClone = null;

                    System.Threading.Monitor.Exit( tx );
                    try
                    {
                        try                    
                        {
                            dtx.Complete();
                        }
                        finally
                        {
                            dtx.Dispose();
                        }
                    }
                    finally
                    {
#pragma warning disable 0618
                        //@
                        System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
                    }
                }
            }
        }


        internal override void CompleteAbortingClone( InternalTransaction tx )
        {
            // If we have a phase1Volatile.VolatileDemux, we have a phase1 volatile enlistment
            // on the promoted transaction and it will take care of checking for incomplete aborting
            // dependent clones in its Prepare processing.
            if ( null != tx.phase1Volatiles.VolatileDemux )
            {
                tx.phase1Volatiles.dependentClones--;
                Debug.Assert( tx.phase1Volatiles.dependentClones >= 0 );
            }
            else
                // We need to deal with the aborting clones ourself, possibly completing the aborting
                // clone we have on the promoted transaction.
            {
                tx.abortingDependentCloneCount--;
                Debug.Assert( 0 <= tx.abortingDependentCloneCount );
                if ( 0 == tx.abortingDependentCloneCount )
                {
                    // We need to complete our dependent clone on the promoted transaction and null it out
                    // so if we get a new one, a new one will be created on the promoted transaction.
                    Oletx.OletxDependentTransaction dtx = tx.abortingDependentClone;
                    tx.abortingDependentClone = null;

                    System.Threading.Monitor.Exit( tx );
                    try
                    {
                        try                    
                        {
                            dtx.Complete();
                        }
                        finally
                        {
                            dtx.Dispose();
                        }
                    }
                    finally
                    {
#pragma warning disable 0618
                        //@
                        System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
                    }
                }
            }
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            // Once the transaction is promoted leverage the distributed
            // transaction manager for blocking dependent clones so that they
            // will handle phase 0 waves.
            if ( tx.phase0WaveDependentClone == null )
            {
                tx.phase0WaveDependentClone = tx.PromotedTransaction.DependentClone( true );
            }

            tx.phase0WaveDependentCloneCount++;
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            // If we have a VolatileDemux in phase1Volatiles, then we have a phase1 volatile enlistment
            // on the promoted transaction, so we can depend on that to deal with our aborting dependent clones.
            if ( null != tx.phase1Volatiles.VolatileDemux )
            {
                tx.phase1Volatiles.dependentClones++;
            }
            else
                // We promoted without creating a phase1 volatile enlistment on the promoted transaction,
                // so we let the promoted transaction deal with the aboring clone.
            {
                if ( null == tx.abortingDependentClone )
                {
                    tx.abortingDependentClone = tx.PromotedTransaction.DependentClone( false );
                }
                tx.abortingDependentCloneCount++;
            }
        }


        internal override bool ContinuePhase0Prepares()
        {
            return true;
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );

            // This is not allowed if the transaction's PromoterType is not MSDTC.
            tx.ThrowIfPromoterTypeIsNotMSDTC();

            // Simply get call get object data for the promoted transaction.
            ISerializable serializableTx = tx.PromotedTransaction as ISerializable;
            if ( serializableTx == null )
            {
                // The LTM can only support this if the Distributed TM Supports it.
                throw new NotSupportedException();
            }

            // Before forwarding this call to the promoted tx make sure to change
            // the full type info so that only if the promoted tx does not set this
            // then it should be set correctly.
            serializationInfo.FullTypeName = tx.PromotedTransaction.GetType().FullName;

            // Now forward the call.
            serializableTx.GetObjectData( serializationInfo, context );
        }


        internal override void ChangeStatePromotedAborted( InternalTransaction tx )
        {
            _TransactionStatePromotedAborted.EnterState( tx );
        }


        internal override void ChangeStatePromotedCommitted( InternalTransaction tx )
        {
            _TransactionStatePromotedCommitted.EnterState( tx );
        }


        internal override void InDoubtFromDtc( InternalTransaction tx )
        {
            _TransactionStatePromotedIndoubt.EnterState( tx );
        }


        internal override void InDoubtFromEnlistment(InternalTransaction tx)
        {
            _TransactionStatePromotedIndoubt.EnterState( tx );
        }


        internal override void ChangeStateAbortedDuringPromotion( InternalTransaction tx )
        {
            _TransactionStateAborted.EnterState( tx );
        }


        internal override void Timeout( InternalTransaction tx )
        {
            // LTM gives up the ability to control Tx timeout when it promotes.
            try
            {
                if ( tx.innerException == null )
                {
                    tx.innerException = new TimeoutException( SR.GetString( SR.TraceTransactionTimeout ));;
                }
                tx.PromotedTransaction.Rollback();

                if ( DiagnosticTrace.Warning )
                {
                    TransactionTimeoutTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        tx.TransactionTraceId
                        );
                }
            }
            catch ( TransactionException te )
            {
                // This could fail for any number of reasons based on the state of the transaction.
                // The Ltm tries anyway because PSPE transactions have no timeout specified and some
                // distributed transaction managers may not honer the timeout correctly.

                // The exception needs to be caught because we don't want it to go unhandled on the
                // timer thread.
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        te );
                }
            }
        }


        internal override void Promote( InternalTransaction tx )
        {
            // do nothing, we are already promoted
        }

        internal override byte[] PromotedToken(InternalTransaction tx)
        {
            // Since we are in TransactionStatePromotedBase or one if its derived classes, we
            // must already be promoted. So return the InternalTransaction's promotedToken.
            Debug.Assert(tx.promotedToken != null, "InternalTransaction.promotedToken is null in TransactionStateDelegatedNonMSDTCBase or one of its derived classes.");
            return tx.promotedToken;
        }

        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            // Early done notifications may come from volatiles at any time.
            // The state machine will handle all enlistments being complete in later phases.
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            // Early done notifications may come from volatiles at any time.
            // The state machine will handle all enlistments being complete in later phases.
        }
    }


    // TransactionStateNonCommittablePromoted
    //
    // This state indicates that the transaction has been promoted and all further actions on
    // the transaction should be forwarded to the promoted transaction.
    internal class TransactionStateNonCommittablePromoted : TransactionStatePromotedBase
    {
        internal override void EnterState( InternalTransaction tx )
        {
            // Set the transaction state
            CommonEnterState( tx );
 
            // Let the distributed transaction know that we want to know about the outcome.
            tx.PromotedTransaction.realOletxTransaction.InternalTransaction = tx;
        }        
    }

    // TransactionStatePromoted
    //
    // This state indicates that the transaction has been promoted and all further actions on
    // the transaction should be forwarded to the promoted transaction.
    internal class TransactionStatePromoted : TransactionStatePromotedBase
    {
        internal override void EnterState( InternalTransaction tx )
        {
            Debug.Assert((tx.promoterType == Guid.Empty) || (tx.promoterType == TransactionInterop.PromoterTypeDtc), "Promoted to MSTC but PromoterType is not TransactionInterop.PromoterTypeDtc");
            // The promoterType may not yet be set. This state assumes we are promoting to MSDTC.
            tx.SetPromoterTypeToMSDTC();

            if ( tx.outcomeSource.isoLevel == IsolationLevel.Snapshot )
            {
                throw TransactionException.CreateInvalidOperationException( SR.GetString( SR.TraceSourceLtm ), 
                    SR.GetString( SR.CannotPromoteSnapshot ), null);
            }
            
            // Set the transaction state
            CommonEnterState( tx );

            // Create a transaction with the distributed transaction manager
            Oletx.OletxCommittableTransaction distributedTx = null;
            try
            {
                TimeSpan newTimeout;
                if ( tx.AbsoluteTimeout == long.MaxValue )
                {
                    // The transaction has no timeout
                    newTimeout = TimeSpan.Zero;
                }
                else
                {
                    newTimeout = TransactionManager.TransactionTable.RecalcTimeout( tx );
                    if ( newTimeout <= TimeSpan.Zero )
                    {
                        return;
                    }
                }

                // Just create a new transaction.
                TransactionOptions options = new TransactionOptions();

                options.IsolationLevel = tx.outcomeSource.isoLevel;
                options.Timeout = newTimeout;

                // Create a new distributed transaction.
                distributedTx = 
                    TransactionManager.DistributedTransactionManager.CreateTransaction( options );
                distributedTx.savedLtmPromotedTransaction = tx.outcomeSource;

                if ( DiagnosticTrace.Information )
                {
                    TransactionPromotedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ), 
                        tx.TransactionTraceId,
                        distributedTx.TransactionTraceId
                        );
                }
            }
            catch ( TransactionException te )
            {
                // There was an exception trying to create the distributed transaction.
                // Save the exception and let the transaction get aborted by the finally block.
                tx.innerException = te;                
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        te );
                }
                return;
            }
            finally
            {
                if ( distributedTx == null )
                {
                    // There was an exception trying to create the distributed transaction abort
                    // the local transaction and exit.
                    tx.State.ChangeStateAbortedDuringPromotion( tx );
                }
            }

            // Associate the distributed transaction with the local transaction.
            tx.PromotedTransaction = distributedTx;

            // Add a weak reference to the transaction to the promotedTransactionTable.
            Hashtable promotedTransactionTable = TransactionManager.PromotedTransactionTable;
            lock ( promotedTransactionTable )
            {
                // Since we are adding this reference to the table create an object that will clean that
                // entry up.
                tx.finalizedObject = new FinalizedObject( tx, distributedTx.Identifier );

                WeakReference weakRef = new WeakReference( tx.outcomeSource, false );
                promotedTransactionTable[distributedTx.Identifier] = weakRef;
            }
            TransactionManager.FireDistributedTransactionStarted( tx.outcomeSource );

            // Once we have a promoted transaction promote the enlistments.
            PromoteEnlistmentsAndOutcome( tx );
        }


        protected bool PromotePhaseVolatiles( 
            InternalTransaction tx, 
            ref VolatileEnlistmentSet volatiles, 
            bool phase0 )
        {
            if ( volatiles.volatileEnlistmentCount + volatiles.dependentClones > 0 )
            {
                if ( phase0 )
                {
                    // Create a volatile demultiplexer for the transaction
                    volatiles.VolatileDemux = new Phase0VolatileDemultiplexer( tx );
                }
                else
                {
                    // Create a volatile demultiplexer for the transaction
                    volatiles.VolatileDemux = new Phase1VolatileDemultiplexer( tx );
                }

                volatiles.VolatileDemux.oletxEnlistment = tx.PromotedTransaction.EnlistVolatile( volatiles.VolatileDemux, 
                    phase0 ? EnlistmentOptions.EnlistDuringPrepareRequired : EnlistmentOptions.None );
            }

            return true;
        }


        internal virtual bool PromoteDurable( InternalTransaction tx )
        {
            // Promote the durable enlistment if one exists.
            if ( tx.durableEnlistment != null )
            {
                // Directly enlist the durable enlistment with the resource manager.
                InternalEnlistment enlistment = tx.durableEnlistment;
                IPromotedEnlistment oletxEnlistment = tx.PromotedTransaction.EnlistDurable( 
                    enlistment.ResourceManagerIdentifier, 
                    (DurableInternalEnlistment)enlistment, 
                    enlistment.SinglePhaseNotification != null, 
                    EnlistmentOptions.None 
                    );

                // Promote the enlistment.
                tx.durableEnlistment.State.ChangeStatePromoted( tx.durableEnlistment, oletxEnlistment );
            }

            return true;
        }


        internal virtual void PromoteEnlistmentsAndOutcome( InternalTransaction tx )
        {
            // Failures from this point on will simply abort the two types of transaction
            // seperately.  Note that this may cause duplicate internal aborted events to
            // be sent to some of the enlistments however the enlistment state machines 
            // can handle the duplicate notification.

            bool enlistmentsPromoted = false;

            // Tell the RealOletxTransaction that we want a callback for the outcome.
            tx.PromotedTransaction.RealTransaction.InternalTransaction = tx;

            // Promote Phase 0 Volatiles
            try
            {
                enlistmentsPromoted = PromotePhaseVolatiles( tx, ref tx.phase0Volatiles, true );
            }
            catch ( TransactionException te )
            {
                // 

                // Record the exception information.
                tx.innerException = te;
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        te );
                }

                return;
            }
            finally
            {
                if ( !enlistmentsPromoted )
                {
                    tx.PromotedTransaction.Rollback( );

                    // Now abort this transaction.
                    tx.State.ChangeStateAbortedDuringPromotion( tx );
                }
            }

            enlistmentsPromoted = false;

            try
            {
                enlistmentsPromoted = PromotePhaseVolatiles( tx, ref tx.phase1Volatiles, false );
            }
            catch ( TransactionException te )
            {
                // Record the exception information.
                tx.innerException = te;
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        te );
                }

                return;
            }
            finally
            {
                if ( !enlistmentsPromoted )
                {
                    tx.PromotedTransaction.Rollback( );

                    // Now abort this transaction.
                    tx.State.ChangeStateAbortedDuringPromotion( tx );
                }
            }

            enlistmentsPromoted = false;

            // Promote the durable enlistment
            try
            {
                enlistmentsPromoted = PromoteDurable( tx );
            }
            catch ( TransactionException te )
            {
                // Record the exception information.
                tx.innerException = te;
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        te );
                }

                return;
            }
            finally
            {
                if ( !enlistmentsPromoted )
                {
                    tx.PromotedTransaction.Rollback( );

                    // Now abort this transaction.
                    tx.State.ChangeStateAbortedDuringPromotion( tx );
                }
            }
        }


        internal override void DisposeRoot( InternalTransaction tx )
        {
            tx.State.Rollback( tx, null );
        }
    }


    // TransactionStatePromotedP0Wave
    //
    // This state indicates that the transaction has been promoted during phase 0.  This
    // is a holding state until the current phase 0 wave is complete.  When the current
    // wave is complete the state changes to committing.
    internal class TransactionStatePromotedP0Wave : TransactionStatePromotedBase
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Don't allow this again.
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            try
            {
                // Now that the previous wave is done continue start committing the transaction.
                _TransactionStatePromotedCommitting.EnterState( tx );
            }
            catch ( TransactionException e )
            {
                // In this state we don't want a transaction exception from BeginCommit to randomly 
                // bubble up to the application or go unhandled.  So catch the exception and if the 
                // inner exception for the transaction has not already been set then set it.
                if ( tx.innerException == null )
                {
                    tx.innerException = e;
                }
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        e );
                }
            }
                
        }


        internal override bool ContinuePhase0Prepares()
        {
            return true;
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            // This change state event at this point would be caused by one of the enlistments
            // aborting.  Really change to P0Aborting
            _TransactionStatePromotedP0Aborting.EnterState( tx );
        }
    }


    // TransactionStatePromotedCommitting
    //
    // The transaction has been promoted but is in the process of committing.
    internal class TransactionStatePromotedCommitting : TransactionStatePromotedBase
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            // Use the asynchronous commit provided by the promoted transaction
            Oletx.OletxCommittableTransaction ctx = (Oletx.OletxCommittableTransaction)tx.PromotedTransaction;
            ctx.BeginCommit( tx );
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Don't allow this again.
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void ChangeStatePromotedPhase0( InternalTransaction tx )
        {
            _TransactionStatePromotedPhase0.EnterState( tx );
        }


        internal override void ChangeStatePromotedPhase1( InternalTransaction tx )
        {
            _TransactionStatePromotedPhase1.EnterState( tx );
        }
    }

    // TransactionStatePromotedPhase0
    //
    // This state indicates that the transaction has been promoted and started the process
    // of committing.  The transaction had volatile phase0 enlistments and is acting as a 
    // proxy to the TM for those phase0 enlistments.
    internal class TransactionStatePromotedPhase0 : TransactionStatePromotedCommitting
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            // Get a copy of the current volatile enlistment count before entering this loop so that other 
            // threads don't affect the operation of this loop.
            int volatileCount = tx.phase0Volatiles.volatileEnlistmentCount;
            int dependentCount = tx.phase0Volatiles.dependentClones;

            // Store the number of phase0 volatiles for this wave.
            tx.phase0VolatileWaveCount = volatileCount;

            // Check to see if we still need to send out volatile prepare notifications or if
            // they are all done.  They may be done if the transaction was already in phase 0
            // before it got promoted.
            if ( tx.phase0Volatiles.preparedVolatileEnlistments < 
                volatileCount + dependentCount )
            {
                // Broadcast preprepare to the volatile subordinates
                for ( int i = 0; i < volatileCount; i++ )
                {
                    tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(
                        tx.phase0Volatiles.volatileEnlistments[i]);

                    if ( !tx.State.ContinuePhase0Prepares() )
                    {
                        break;
                    }
                }
            }
            else
            {
                Phase0VolatilePrepareDone( tx );
            }
        }


        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            Debug.Assert( tx.phase0Volatiles.VolatileDemux != null, "Volatile Demux must exist for VolatilePrepareDone when promoted." );

            System.Threading.Monitor.Exit( tx );
            try
            {
                // Tell the distributed TM that the volatile enlistments are prepared
                tx.phase0Volatiles.VolatileDemux.oletxEnlistment.Prepared();
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override bool ContinuePhase0Prepares()
        {
            return true;
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }
            
            // This change state event at this point would be caused by one of the enlistments
            // aborting.  Really change to P0Aborting
            _TransactionStatePromotedP0Aborting.EnterState( tx );
        }
    }


    // TransactionStatePromotedPhase1
    //
    // This state indicates that the transaction has been promoted and started the process
    // of committing.  The transaction had volatile phase1 enlistments and is acting as a 
    // proxy to the TM for those phase1 enlistments.
    internal class TransactionStatePromotedPhase1 : TransactionStatePromotedCommitting
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            if ( tx.committableTransaction != null )
            {
                // If we have a committable transaction then mark it as complete.
                tx.committableTransaction.complete = true;
            }

            // If at this point there are phase1 dependent clones abort the transaction
            if ( tx.phase1Volatiles.dependentClones != 0 )
            {
                tx.State.ChangeStateTransactionAborted( tx, null );
                return;
            }

            // Get a copy of the current volatile enlistment count before entering this loop so that other 
            // threads don't affect the operation of this loop.
            int volatileCount = tx.phase1Volatiles.volatileEnlistmentCount;

            // Check to see if we still need to send out volatile prepare notifications or if
            // they are all done.  They may be done if the transaction was already in phase 0
            // before it got promoted.
            if ( tx.phase1Volatiles.preparedVolatileEnlistments < volatileCount )
            {
                // Broadcast preprepare to the volatile subordinates
                for ( int i = 0; i < volatileCount; i++ )
                {
                    tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(
                        tx.phase1Volatiles.volatileEnlistments[i]);
                    if ( !tx.State.ContinuePhase1Prepares() )
                    {
                        break;
                    }
                }
            }
            else
            {
                Phase1VolatilePrepareDone( tx );
            }
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }
            
            // This change state event at this point would be caused by one of the enlistments
            // aborting.  Really change to P1Aborting
            _TransactionStatePromotedP1Aborting.EnterState( tx );
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            Debug.Assert( tx.phase1Volatiles.VolatileDemux != null, "Volatile Demux must exist for VolatilePrepareDone when promoted." );

            System.Threading.Monitor.Exit( tx );
            try
            {
                // Tell the distributed TM that the volatile enlistments are prepared
                tx.phase1Volatiles.VolatileDemux.oletxEnlistment.Prepared();
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override bool ContinuePhase1Prepares()
        {
            return true;
        }


        internal override Enlistment EnlistVolatile( InternalTransaction tx, IEnlistmentNotification enlistmentNotification, EnlistmentOptions enlistmentOptions, Transaction atomicTransaction )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }


        internal override Enlistment EnlistVolatile( InternalTransaction tx, ISinglePhaseNotification enlistmentNotification, EnlistmentOptions enlistmentOptions, Transaction atomicTransaction )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }


        internal override Enlistment EnlistDurable( InternalTransaction tx, Guid resourceManagerIdentifier, IEnlistmentNotification enlistmentNotification, EnlistmentOptions enlistmentOptions, Transaction atomicTransaction )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }


        internal override Enlistment EnlistDurable( InternalTransaction tx, Guid resourceManagerIdentifier, ISinglePhaseNotification enlistmentNotification, EnlistmentOptions enlistmentOptions, Transaction atomicTransaction )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }
    }



    // TransactionStatePromotedAborting
    //
    // This state indicates that the transaction has been promoted but aborted.  Once the volatile
    // enlistments have finished responding the tx can be finished.
    internal abstract class TransactionStatePromotedAborting : TransactionStatePromotedBase
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );
        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.Aborted;
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Don't allow this again.
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void ChangeStatePromotedAborted( InternalTransaction tx )
        {
            _TransactionStatePromotedAborted.EnterState( tx );
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            // Don't do this yet wait until all of the notifications come back.
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            // Commit cannot be restarted
        }

    }


    // TransactionStatePromotedP0Aborting
    //
    // This state indicates that the transaction has been promoted but aborted by a phase 0 volatile
    // enlistment.  Once the volatile enlistments have finished responding the tx can be finished.
    internal class TransactionStatePromotedP0Aborting : TransactionStatePromotedAborting
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            ChangeStatePromotedAborted( tx );

            // If we have a volatilePreparingEnlistment tell it to roll back.
            if ( tx.phase0Volatiles.VolatileDemux.preparingEnlistment != null )
            {
                System.Threading.Monitor.Exit( tx );
                try
                {
                    // Tell the distributed TM that the tx aborted.
                    tx.phase0Volatiles.VolatileDemux.oletxEnlistment.ForceRollback();
                }
                finally
                {
#pragma warning disable 0618
                    //@
                    System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
                }
            }
            else
            {
                // Otherwise make sure that the transaction rolls back.
                tx.PromotedTransaction.Rollback();
            }
        }


        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            // If this happens as a ---- it is just fine.
        }
    }


    // TransactionStatePromotedP1Aborting
    //
    // This state indicates that the transaction has been promoted but aborted by a phase 1 volatile
    // enlistment.  Once the volatile enlistments have finished responding the tx can be finished.
    internal class TransactionStatePromotedP1Aborting : TransactionStatePromotedAborting
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            Debug.Assert( tx.phase1Volatiles.VolatileDemux != null, "Volatile Demux must exist." );

            ChangeStatePromotedAborted( tx );

            System.Threading.Monitor.Exit( tx );
            try
            {
                // Tell the distributed TM that the tx aborted.
                tx.phase1Volatiles.VolatileDemux.oletxEnlistment.ForceRollback();
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            // If this happens as a ---- it is fine.
        }
    }


    // TransactionStatePromotedEnded
    //
    // This is a common base class for committed, aborted, and indoubt states of a promoted
    // transaction.
    internal abstract class TransactionStatePromotedEnded : TransactionStateEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );
            
            CommonEnterState( tx );

            if ( !ThreadPool.QueueUserWorkItem( SignalMethod, tx ))
            {
                throw TransactionException.CreateInvalidOperationException(
                    SR.GetString( SR.TraceSourceLtm ), 
                    SR.GetString(SR.UnexpectedFailureOfThreadPool),
                    null,
                    tx == null ? Guid.Empty : tx.DistributedTxId
                    );
            }
        }


        internal override void AddOutcomeRegistrant( InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate )
        {
            if ( transactionCompletedDelegate != null )
            {
                TransactionEventArgs args = new TransactionEventArgs( );
                args.transaction = tx.outcomeSource.InternalClone();
                transactionCompletedDelegate( args.transaction, args );
            }
        }


        internal override void EndCommit( InternalTransaction tx )
        {
            // Test the outcome of the transaction and respond accordingly.
            Debug.Assert( tx.PromotedTransaction != null, "Promoted state not valid for transaction." );
            PromotedTransactionOutcome( tx );
        }


        internal override void CompleteBlockingClone( InternalTransaction tx )
        {
            // The transaction is finished ignore these.
        }


        internal override void CompleteAbortingClone( InternalTransaction tx )
        {
            // The transaction is finished ignore these.
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }

        internal override Guid get_Identifier( InternalTransaction tx )
        {
            return tx.PromotedTransaction.Identifier;
        }

        internal override void Promote( InternalTransaction tx )
        {
            // do nothing, we are already promoted
        }
        
        protected abstract void PromotedTransactionOutcome( InternalTransaction tx );

        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile WaitCallback signalMethod;
        private static WaitCallback SignalMethod
        {
            get
            {
                if ( signalMethod == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( signalMethod == null )
                        {
                            signalMethod = new WaitCallback( SignalCallback );
                        }
                    }
                }

                return signalMethod;
            }
        }


        private static void SignalCallback( object state )
        {
            InternalTransaction tx = (InternalTransaction)state;
            lock ( tx )
            {
                tx.SignalAsyncCompletion();
                TransactionManager.TransactionTable.Remove( tx );
            }
        }

    }

    
    // TransactionStatePromotedAborted
    //
    // This state indicates that the transaction has been promoted and the outcome
    // of the transaction is aborted.
    internal class TransactionStatePromotedAborted : TransactionStatePromotedEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );

            // Tell all the enlistments the outcome.
            if ( tx.phase1Volatiles.VolatileDemux != null )
            {
                tx.phase1Volatiles.VolatileDemux.BroadcastRollback( ref tx.phase1Volatiles );
            }

            if ( tx.phase0Volatiles.VolatileDemux != null )
            {
                tx.phase0Volatiles.VolatileDemux.BroadcastRollback( ref tx.phase0Volatiles );
            }

            // Fire Completion for anyone listening
            tx.FireCompletion( );
            // We don't need to do the AsyncCompletion stuff.  If it was needed, it was done out of SignalCallback.

            if ( DiagnosticTrace.Warning )
            {
                TransactionAbortedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.Aborted;
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            // Already done.
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            throw TransactionAbortedException.Create( SR.GetString( SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }


        internal override void CreateBlockingClone( InternalTransaction tx )
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }


        internal override void CreateAbortingClone( InternalTransaction tx )
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            // Commit cannot be restarted
        }


        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            // Since the transaction is aborted ignore it.
        }


        internal override void Phase1VolatilePrepareDone( InternalTransaction tx )
        {
            // Since the transaction is aborted ignore it.
        }


        internal override void ChangeStatePromotedPhase0( InternalTransaction tx )
        {
            throw new TransactionAbortedException(tx.innerException, tx.DistributedTxId);
        }

        internal override void ChangeStatePromotedPhase1( InternalTransaction tx )
        {
            throw new TransactionAbortedException(tx.innerException, tx.DistributedTxId);
        }

        internal override void ChangeStatePromotedAborted( InternalTransaction tx )
        {
            // This call may come from multiple events.  Support being told more than once.
        }


        internal override void ChangeStateTransactionAborted( InternalTransaction tx, Exception e )
        {
            // This may come from a promotable single phase enlistments abort response.
        }


        protected override void PromotedTransactionOutcome( InternalTransaction tx )
        {
            if ( ( null == tx.innerException ) && ( null != tx.PromotedTransaction ) )
            {
                tx.innerException = tx.PromotedTransaction.InnerException;
            }
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }


        internal override void CheckForFinishedTransaction( InternalTransaction tx )
        {
            throw new TransactionAbortedException(tx.innerException, tx.DistributedTxId);
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }


        internal override void InDoubtFromDtc( InternalTransaction tx )
        {
            // Getting this event would mean that a PSPE enlistment has told us the
            // transaction outcome.  It is possible that a PSPE enlistment would know
            // the transaction outcome when DTC does not.  So ignore the indoubt
            // notification from DTC.
        }


        internal override void InDoubtFromEnlistment( InternalTransaction tx )
        {
            // In this case DTC has told us the outcome but a PSPE enlistment
            // is telling us that it does not know the outcome of the transaction.
            // So ignore the notification from the enlistment.
        }
    }

    
    
    // TransactionStatePromotedCommitted
    //
    // This state indicates that the transaction has been promoted and the outcome
    // of the transaction is committed
    internal class TransactionStatePromotedCommitted : TransactionStatePromotedEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );

            // Tell all the enlistments the outcome.
            if ( tx.phase1Volatiles.VolatileDemux != null )
            {
                tx.phase1Volatiles.VolatileDemux.BroadcastCommitted( ref tx.phase1Volatiles );
            }

            if ( tx.phase0Volatiles.VolatileDemux != null )
            {
                tx.phase0Volatiles.VolatileDemux.BroadcastCommitted( ref tx.phase0Volatiles );
            }

            // Fire Completion for anyone listening
            tx.FireCompletion( );
            // We don't need to do the AsyncCompletion stuff.  If it was needed, it was done out of SignalCallback.

            if ( DiagnosticTrace.Verbose )
            {
                TransactionCommittedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.Committed;
        }


        internal override void ChangeStatePromotedCommitted( InternalTransaction tx )
        {
            // This call may come from multiple different events.  Support being told more than once.
        }


        protected override void PromotedTransactionOutcome( InternalTransaction tx )
        {
            // This is a happy transaction.
        }


        internal override void InDoubtFromDtc( InternalTransaction tx )
        {
            // Getting this event would mean that a PSPE enlistment has told us the
            // transaction outcome.  It is possible that a PSPE enlistment would know
            // the transaction outcome when DTC does not.  So ignore the indoubt
            // notification from DTC.
        }


        internal override void InDoubtFromEnlistment( InternalTransaction tx )
        {
            // In this case DTC has told us the outcome but a PSPE enlistment
            // is telling us that it does not know the outcome of the transaction.
            // So ignore the notification from the enlistment.
        }
    }
    
    
    
    // TransactionStatePromotedIndoubt
    //
    // This state indicates that the transaction has been promoted but the outcome
    // of the transaction is indoubt.
    internal class TransactionStatePromotedIndoubt : TransactionStatePromotedEnded
    {
        internal override void EnterState( InternalTransaction tx )
        {
            base.EnterState( tx );

            // Tell all the enlistments the outcome.
            if ( tx.phase1Volatiles.VolatileDemux != null )
            {
                tx.phase1Volatiles.VolatileDemux.BroadcastInDoubt( ref tx.phase1Volatiles );
            }

            if ( tx.phase0Volatiles.VolatileDemux != null )
            {
                tx.phase0Volatiles.VolatileDemux.BroadcastInDoubt( ref tx.phase0Volatiles );
            }

            // Fire Completion for anyone listening
            tx.FireCompletion( );
            // We don't need to do the AsyncCompletion stuff.  If it was needed, it was done out of SignalCallback.

            if ( DiagnosticTrace.Warning )
            {
                TransactionInDoubtTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.TransactionTraceId
                    );
            }

        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            return TransactionStatus.InDoubt;
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            // Commit cannot be restarted
        }


        internal override void ChangeStatePromotedPhase0( InternalTransaction tx )
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }

        internal override void ChangeStatePromotedPhase1( InternalTransaction tx )
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }


        internal override void InDoubtFromDtc( InternalTransaction tx )
        {
            // This call may actually come from multiple sources that ----.
            // Since we already took action based on the first notification ignore the
            // others.
        }


        internal override void InDoubtFromEnlistment( InternalTransaction tx )
        {
            // This call may actually come from multiple sources that ----.
            // Since we already took action based on the first notification ignore the
            // others.
        }


        protected override void PromotedTransactionOutcome( InternalTransaction tx )
        {
            if ( ( null == tx.innerException ) && ( null != tx.PromotedTransaction ) )
            {
                tx.innerException = tx.PromotedTransaction.InnerException;
            }
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }


        internal override void CheckForFinishedTransaction( InternalTransaction tx )
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }


        internal override void GetObjectData( InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context )
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }


        internal override void ChangeStatePromotedAborted( InternalTransaction tx )
        {
            // Transaction outcome can come from different directions.  In the case of InDoubt 
            // transactions it is possible that one source knowns the actual outcome for
            // the transaction.  However since the transaction does not know if it will receive
            // a different answer for the outcome it accepts the first answer it gets.
            // By the time we receive a better answer the clients of this transaction
            // have already been informed that the transaction is InDoubt.
        }


        internal override void ChangeStatePromotedCommitted( InternalTransaction tx )
        {
            // See comment in ChangeStatePromotedAborted
        }
    }


    // TransactionStateDelegatedBase
    //
    // This state is the base state for delegated transactions
    internal abstract class TransactionStateDelegatedBase : TransactionStatePromoted
    {
        internal override void EnterState( InternalTransaction tx )
        {
            if ( tx.outcomeSource.isoLevel == IsolationLevel.Snapshot )
            {
                throw TransactionException.CreateInvalidOperationException( SR.GetString( SR.TraceSourceLtm ),
                    SR.GetString(SR.CannotPromoteSnapshot), null, tx == null ? Guid.Empty : tx.DistributedTxId);
            }

            // Assign the state
            CommonEnterState( tx );
           
            // Create a transaction with the distributed transaction manager
            Oletx.OletxTransaction distributedTx = null;
            try
            {
                // Ask the delegation interface to promote the transaction.
                if ( DiagnosticTrace.Verbose && tx.durableEnlistment != null )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        tx.durableEnlistment.EnlistmentTraceId,
                        NotificationCall.Promote
                        );
                }


                distributedTx = _TransactionStatePSPEOperation.PSPEPromote( tx );
            }
            catch ( TransactionPromotionException e )
            {
                tx.innerException = e;
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        e );
                }
            }
            finally
            {
                if ( ((object)distributedTx) == null )
                {
                    // There was an exception trying to create the distributed transaction abort
                    // the local transaction and exit.
                    tx.State.ChangeStateAbortedDuringPromotion( tx );
                }
            }

            if ( ((object)distributedTx) == null )
            {
                return;
            }

            // If tx.PromotedTransaction is already set to the distributedTx that was
            // returned, then the PSPE enlistment must have used
            // Transaction.PSPEPromoteAndConvertToEnlistDurable to promote the transaction
            // within the same AppDomain. So we don't need to add the distributedTx to the
            // PromotedTransactionTable and we don't need to call
            // FireDistributedTransactionStarted and we don't need to promote the 
            // enlistments. That was all done when the transaction was changed to
            // TransactionStatePromoted.
            if (tx.PromotedTransaction != distributedTx)
            {
                // Associate the distributed transaction with the local transaction.
                tx.PromotedTransaction = distributedTx;

                // Add a weak reference to the transaction to the promotedTransactionTable.
                Hashtable promotedTransactionTable = TransactionManager.PromotedTransactionTable;
                lock (promotedTransactionTable)
                {
                    // Since we are adding this reference to the table create an object that will clean that
                    // entry up.
                    tx.finalizedObject = new FinalizedObject(tx, tx.PromotedTransaction.Identifier);

                    WeakReference weakRef = new WeakReference(tx.outcomeSource, false);
                    promotedTransactionTable[tx.PromotedTransaction.Identifier] = weakRef;
                }
                TransactionManager.FireDistributedTransactionStarted(tx.outcomeSource);

                if (DiagnosticTrace.Information)
                {
                    TransactionPromotedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        tx.TransactionTraceId,
                        distributedTx.TransactionTraceId
                        );
                }

                // Once we have a promoted transaction promote the enlistments.
                PromoteEnlistmentsAndOutcome(tx);
            }
        }
    }


    // TransactionStateDelegated
    //
    // This state represents a transaction that had a promotable single phase enlistment that then
    // was promoted.  Most of the functionality is inherited from transaction state promoted
    // except for the way that commit happens.
    internal class TransactionStateDelegated : TransactionStateDelegatedBase
    {
        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Store the given values
            tx.asyncCommit = asyncCommit;
            tx.asyncCallback = asyncCallback;
            tx.asyncState = asyncState;

            // Initiate the commit process.
            _TransactionStateDelegatedCommitting.EnterState( tx );
        }


        internal override bool PromoteDurable( InternalTransaction tx )
        {
            // Let the enlistment know that it has been delegated.  For this type of enlistment that
            // is really all that needs to be done.
            tx.durableEnlistment.State.ChangeStateDelegated( tx.durableEnlistment );

            return true;
        }


        internal override void RestartCommitIfNeeded( InternalTransaction tx )
        {
            _TransactionStateDelegatedP0Wave.EnterState( tx );
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            // Pass the Rollback through the promotable single phase enlistment to be
            // certain it is notified.

            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            _TransactionStateDelegatedAborting.EnterState( tx );
        }
    }


    // TransactionStatePromotedNonMSDTCBase
    //
    // This is the base class for non-MSDTC promoted states.  It's main function is to pass calls
    // through to the distributed transaction.
    internal abstract class TransactionStatePromotedNonMSDTCBase : TransactionState
    {
        internal override TransactionStatus get_Status(InternalTransaction tx)
        {
            return TransactionStatus.Active;
        }

        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Enlistment enlistment = new Enlistment(tx, enlistmentNotification, null, atomicTransaction, enlistmentOptions);
            if ((enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0)
            {
                AddVolatileEnlistment(ref tx.phase0Volatiles, enlistment);
            }
            else
            {
                AddVolatileEnlistment(ref tx.phase1Volatiles, enlistment);
            }

            if (DiagnosticTrace.Information)
            {
                EnlistmentTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    enlistment.InternalEnlistment.EnlistmentTraceId,
                    EnlistmentType.Volatile,
                    enlistmentOptions
                    );
            }

            return enlistment;
        }

        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            Enlistment enlistment = new Enlistment(tx, enlistmentNotification, enlistmentNotification, atomicTransaction, enlistmentOptions);

            if ((enlistmentOptions & EnlistmentOptions.EnlistDuringPrepareRequired) != 0)
            {
                AddVolatileEnlistment(ref tx.phase0Volatiles, enlistment);
            }
            else
            {
                AddVolatileEnlistment(ref tx.phase1Volatiles, enlistment);
            }

            if (DiagnosticTrace.Information)
            {
                EnlistmentTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    enlistment.InternalEnlistment.EnlistmentTraceId,
                    EnlistmentType.Volatile,
                    enlistmentOptions
                    );
            }

            return enlistment;
        }

        internal override Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier,
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw new TransactionPromotionException(string.Format(CultureInfo.CurrentCulture,
                SR.GetString(SR.PromoterTypeUnrecognized), tx.promoterType.ToString()),
                tx.innerException);
        }

        internal override Enlistment EnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw new TransactionPromotionException(string.Format(CultureInfo.CurrentCulture,
                SR.GetString(SR.PromoterTypeUnrecognized), tx.promoterType.ToString()),
                tx.innerException);
        }

        internal override bool EnlistPromotableSinglePhase(
            InternalTransaction tx, IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            // The transaction has been promoted and cannot support a promotable singe phase enlistment
            return false;
        }

        internal override void Rollback(InternalTransaction tx, Exception e)
        {
            // Start the process for abort.  Transitioning to the Aborted state will cause
            // the tx.durableEnlistment to get aborted, which is how the non-MSDTC
            // transaction promoter will get notified of the abort.
            Debug.Assert(tx.durableEnlistment != null, "PromotedNonMSDTC state is not valid for transaction");

            if (tx.innerException == null)
            {
                tx.innerException = e;
            }

            _TransactionStateAborted.EnterState(tx);
        }

        internal override Guid get_Identifier(InternalTransaction tx)
        {
            // In this state, we know that the we are dealing with a non-MSDTC promoter, so get the identifier from the internal transaction.
            return tx.distributedTransactionIdentifierNonMSDTC;
        }


        internal override void AddOutcomeRegistrant(InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate)
        {
            // Add this guy to the list of people to be notified of the outcome.
            tx.transactionCompletedDelegate = (TransactionCompletedEventHandler)
                System.Delegate.Combine(tx.transactionCompletedDelegate, transactionCompletedDelegate);
        }


        // Start the commit processing by transitioning to TransactionStatePromotedNonMSDTCPhase0.
        internal override void BeginCommit(InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState)
        {
            tx.asyncCommit = asyncCommit;
            tx.asyncCallback = asyncCallback;
            tx.asyncState = asyncState;

            _TransactionStatePromotedNonMSDTCPhase0.EnterState(tx);
        }
    
        internal override void CompleteBlockingClone(InternalTransaction tx)
        {
            // First try to complete one of the internal blocking clones
            if (tx.phase0Volatiles.dependentClones > 0)
            {
                // decrement the number of clones
                tx.phase0Volatiles.dependentClones--;

                // Make certain we increment the right list.
                Debug.Assert(tx.phase0Volatiles.preparedVolatileEnlistments <=
                    tx.phase0Volatiles.volatileEnlistmentCount + tx.phase0Volatiles.dependentClones);

                // Check to see if all of the volatile enlistments are done.
                if (tx.phase0Volatiles.preparedVolatileEnlistments ==
                    tx.phase0VolatileWaveCount + tx.phase0Volatiles.dependentClones)
                {
                    tx.State.Phase0VolatilePrepareDone(tx);
                }
            }
        }

        internal override void CompleteAbortingClone(InternalTransaction tx)
        {
            // A blocking clone simulates a phase 1 volatile
            // 
            // Unlike a blocking clone however the aborting clones need to be accounted
            // for specifically.  So when one is complete remove it from the list.
            tx.phase1Volatiles.dependentClones--;
            Debug.Assert(tx.phase1Volatiles.dependentClones >= 0);
        }

        internal override void CreateBlockingClone(InternalTransaction tx)
        {
            // A blocking clone simulates a phase 0 volatile
            tx.phase0Volatiles.dependentClones++;
        }

        internal override void CreateAbortingClone(InternalTransaction tx)
        {
            // An aborting clone simulates a phase 1 volatile
            tx.phase1Volatiles.dependentClones++;
        }

        internal override bool ContinuePhase0Prepares()
        {
            return true;
        }

        internal override void GetObjectData(InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context)
        {
            throw new TransactionPromotionException(string.Format(CultureInfo.CurrentCulture,
                SR.GetString(SR.PromoterTypeUnrecognized), tx.promoterType.ToString()),
                tx.innerException);
        }

        internal override void ChangeStateTransactionAborted(InternalTransaction tx, Exception e)
        {
            // Just transition to Aborted. The PSPE will be told to rollback thru the durableEnlistment.
            // This is also overridden in TransactionStatePromotedNonMSDTCSinglePhaseCommit
            // that does something slightly differently.
            if (tx.innerException == null)
            {
                tx.innerException = e;
            }

            _TransactionStateAborted.EnterState(tx);
        }

        internal override void InDoubtFromEnlistment(InternalTransaction tx)
        {
            _TransactionStatePromotedNonMSDTCIndoubt.EnterState(tx);
        }

        internal override void ChangeStateAbortedDuringPromotion(InternalTransaction tx)
        {
            _TransactionStateAborted.EnterState(tx);
        }

        internal override void Timeout(InternalTransaction tx)
        {
            if (DiagnosticTrace.Warning)
            {
                TransactionTimeoutTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    tx.TransactionTraceId
                    );
            }

            TimeoutException e = new TimeoutException(SR.GetString(SR.TraceTransactionTimeout));
            this.Rollback(tx, e);
        }

        internal override void Promote(InternalTransaction tx)
        {
            // do nothing, we are already promoted
        }

        internal override void Phase0VolatilePrepareDone(InternalTransaction tx)
        {
            // Early done notifications may come from volatiles at any time.
            // The state machine will handle all enlistments being complete in later phases.
        }

        internal override void Phase1VolatilePrepareDone(InternalTransaction tx)
        {
            // Early done notifications may come from volatiles at any time.
            // The state machine will handle all enlistments being complete in later phases.
        }

        internal override byte[] PromotedToken(InternalTransaction tx)
        {
            // Since we are in TransactionStateDelegatedNonMSDTCBase or one if its derived classes, we
            // must already be promoted. So return the InternalTransaction's promotedToken.
            Debug.Assert(tx.promotedToken != null, "InternalTransaction.promotedToken is null in TransactionStateDelegatedNonMSDTCBase or one of its derived classes.");
            return tx.promotedToken;
        }

        internal override void DisposeRoot(InternalTransaction tx)
        {
            tx.State.Rollback(tx, null);
        }
    }

    // TransactionStatePromotedNonMSDTCPhase0
    //
    // A transaction that is in the beginning stage of committing.
    internal class TransactionStatePromotedNonMSDTCPhase0 : TransactionStatePromotedNonMSDTCBase
    {
        internal override void EnterState(InternalTransaction tx)
        {
            // Set the transaction state
            CommonEnterState(tx);

            // Get a copy of the current volatile enlistment count before entering this loop so that other 
            // threads don't affect the operation of this loop.
            int volatileCount = tx.phase0Volatiles.volatileEnlistmentCount;
            int dependentCount = tx.phase0Volatiles.dependentClones;

            // Store the number of phase0 volatiles for this wave.
            tx.phase0VolatileWaveCount = volatileCount;

            // Check for volatile enlistments
            if (tx.phase0Volatiles.preparedVolatileEnlistments < volatileCount + dependentCount)
            {
                // Broadcast prepare to the phase 0 enlistments
                for (int i = 0; i < volatileCount; i++)
                {
                    tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(tx.phase0Volatiles.volatileEnlistments[i]);
                    if (!tx.State.ContinuePhase0Prepares())
                    {
                        break;
                    }
                }
            }
            else
            {
                // No volatile enlistments.  Start phase 1.
                _TransactionStatePromotedNonMSDTCVolatilePhase1.EnterState(tx);
            }
        }

        internal override void BeginCommit(InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState)
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal override void Rollback(InternalTransaction tx, Exception e)
        {
            ChangeStateTransactionAborted(tx, e);
        }

        // Volatile prepare is done for Phase0 enlistments
        internal override void Phase0VolatilePrepareDone(InternalTransaction tx)
        {
            // Check to see if any Phase0Volatiles have been added in Phase0.
            // If so go through the list again.

            // Get a copy of the current volatile enlistment count before entering this loop so that other 
            // threads don't affect the operation of this loop.
            int volatileCount = tx.phase0Volatiles.volatileEnlistmentCount;
            int dependentCount = tx.phase0Volatiles.dependentClones;

            // Store the number of phase0 volatiles for this wave.
            tx.phase0VolatileWaveCount = volatileCount;

            // Check for volatile enlistments
            if (tx.phase0Volatiles.preparedVolatileEnlistments < volatileCount + dependentCount)
            {
                // Broadcast prepare to the phase 0 enlistments
                for (int i = 0; i < volatileCount; i++)
                {
                    tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(tx.phase0Volatiles.volatileEnlistments[i]);
                    if (!tx.State.ContinuePhase0Prepares())
                    {
                        break;
                    }
                }
            }
            else
            {
                // No volatile enlistments.  Start phase 1.
                _TransactionStatePromotedNonMSDTCVolatilePhase1.EnterState(tx);
            }
        }

        internal override void Phase1VolatilePrepareDone(InternalTransaction tx)
        {
            // Ignore this for now it can be checked again in Phase 1
        }

        internal override bool ContinuePhase0Prepares()
        {
            return true;
        }
    }

    // TransactionStatePromotedNonMSDTCVolatilePhase1 
    //
    // Represents the transaction state during phase 1 preparing volatile enlistments
    internal class TransactionStatePromotedNonMSDTCVolatilePhase1 : TransactionStatePromotedNonMSDTCBase 
    {
        internal override void EnterState(InternalTransaction tx)
        {
            // Set the transaction state
            CommonEnterState(tx);

            // Mark the committable transaction as complete.
            tx.committableTransaction.complete = true;

            // If at this point there are phase1 dependent clones abort the transaction
            if (tx.phase1Volatiles.dependentClones != 0)
            {
                ChangeStateTransactionAborted(tx, null);
                return;
            }

            if (tx.phase1Volatiles.volatileEnlistmentCount > 0)
            {
                // Broadcast prepare to the phase 0 enlistments
                for (int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++)
                {
                    tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.ChangeStatePreparing(tx.phase1Volatiles.volatileEnlistments[i]);
                    if (!tx.State.ContinuePhase1Prepares())
                    {
                        break;
                    }
                }
            }
            else
            {
                // No volatile phase 1 enlistments.  Transition to the state that will do SinglePhaseCommit to the PSPE.
                _TransactionStatePromotedNonMSDTCSinglePhaseCommit.EnterState(tx);
            }
        }


        internal override void BeginCommit(InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState)
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal override void Rollback(InternalTransaction tx, Exception e)
        {
            ChangeStateTransactionAborted(tx, e);
        }

        // Volatile prepare is done for Phase1
        internal override void Phase1VolatilePrepareDone(InternalTransaction tx)
        {
            _TransactionStatePromotedNonMSDTCSinglePhaseCommit.EnterState(tx);
        }

        internal override bool ContinuePhase1Prepares()
        {
            return true;
        }

        internal override Enlistment EnlistVolatile(
           InternalTransaction tx,
           IEnlistmentNotification enlistmentNotification,
           EnlistmentOptions enlistmentOptions,
           Transaction atomicTransaction
           )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override bool EnlistPromotableSinglePhase(
            InternalTransaction tx, IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override void CreateBlockingClone(InternalTransaction tx)
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override void CreateAbortingClone(InternalTransaction tx)
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }
    }

    // TransactionStatePromotedNonMSDTCSinglePhaseCommit
    //
    // The transaction has been delegated to a NON-MSDTC promoter and is in the process of committing.
    internal class TransactionStatePromotedNonMSDTCSinglePhaseCommit : TransactionStatePromotedNonMSDTCBase
    {
        internal override void EnterState(InternalTransaction tx)
        {
            CommonEnterState(tx);

            if (DiagnosticTrace.Verbose)
            {
                EnlistmentNotificationCallTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    tx.durableEnlistment.EnlistmentTraceId,
                    NotificationCall.SinglePhaseCommit
                    );
            }

            // We are about to tell the PSPE to do the SinglePhaseCommit. It is too late for us to timeout the transaction.
            // Remove this from the timeout list
            TransactionManager.TransactionTable.Remove(tx);

            tx.durableEnlistment.State.ChangeStateCommitting(tx.durableEnlistment);
        }

        internal override void BeginCommit(InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState)
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal override void Rollback(InternalTransaction tx, Exception e)
        {
            // We have told the PSPE enlistment to do a single phase commit. It's too late to rollback.
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal override void ChangeStateTransactionCommitted(InternalTransaction tx)
        {
            // The durable enlistment must have committed.  Go to the committed state.
            _TransactionStatePromotedNonMSDTCCommitted.EnterState(tx);
        }

        internal override void InDoubtFromEnlistment(InternalTransaction tx)
        {
            // The transaction is indoubt
            _TransactionStatePromotedNonMSDTCIndoubt.EnterState(tx);
        }

        internal override void ChangeStateTransactionAborted(InternalTransaction tx, Exception e)
        {
            if (tx.innerException == null)
            {
                tx.innerException = e;
            }

            // The durable enlistment must have aborted.  Go to the aborted state.
            _TransactionStatePromotedNonMSDTCAborted.EnterState(tx);
        }

        internal override void ChangeStateAbortedDuringPromotion(InternalTransaction tx)
        {
            _TransactionStateAborted.EnterState(tx);
        }

        internal override Enlistment EnlistVolatile(
           InternalTransaction tx,
           IEnlistmentNotification enlistmentNotification,
           EnlistmentOptions enlistmentOptions,
           Transaction atomicTransaction
           )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override Enlistment EnlistVolatile(
            InternalTransaction tx,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override bool EnlistPromotableSinglePhase(
            InternalTransaction tx, IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Transaction atomicTransaction,
            Guid promoterType
            )
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override void CreateBlockingClone(InternalTransaction tx)
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }

        internal override void CreateAbortingClone(InternalTransaction tx)
        {
            throw TransactionException.Create(SR.GetString(SR.TooLate), tx == null ? Guid.Empty : tx.DistributedTxId);
        }
    }

    // TransactionStatePromotedNonMSDTCEnded
    //
    // This is a common base class for committed, aborted, and indoubt states of a non-MSDTC promoted
    // transaction.
    internal abstract class TransactionStatePromotedNonMSDTCEnded : TransactionStateEnded
    {
        internal override void EnterState(InternalTransaction tx)
        {
            base.EnterState(tx);

            CommonEnterState(tx);

            if (!ThreadPool.QueueUserWorkItem(SignalMethod, tx))
            {
                throw TransactionException.CreateInvalidOperationException(
                    SR.GetString(SR.TraceSourceLtm),
                    SR.GetString(SR.UnexpectedFailureOfThreadPool),
                    null,
                    tx == null ? Guid.Empty : tx.DistributedTxId
                    );
            }
        }

        internal override void AddOutcomeRegistrant(InternalTransaction tx, TransactionCompletedEventHandler transactionCompletedDelegate)
        {
            if (transactionCompletedDelegate != null)
            {
                TransactionEventArgs args = new TransactionEventArgs();
                args.transaction = tx.outcomeSource.InternalClone();
                transactionCompletedDelegate(args.transaction, args);
            }
        }

        internal override void EndCommit(InternalTransaction tx)
        {
            // Test the outcome of the transaction and respond accordingly.
            PromotedTransactionOutcome(tx);
        }

        internal override void CompleteBlockingClone(InternalTransaction tx)
        {
            // The transaction is finished ignore these.
        }

        internal override void CompleteAbortingClone(InternalTransaction tx)
        {
            // The transaction is finished ignore these.
        }

        internal override void CreateBlockingClone(InternalTransaction tx)
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal override void CreateAbortingClone(InternalTransaction tx)
        {
            throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
        }

        internal override Guid get_Identifier(InternalTransaction tx)
        {
            // In this state, we know that the we are dealing with a non-MSDTC promoter, so get the identifier from the internal transaction.
            return tx.distributedTransactionIdentifierNonMSDTC;
        }

        internal override void Promote(InternalTransaction tx)
        {
            // do nothing, we are already promoted
        }

        protected abstract void PromotedTransactionOutcome(InternalTransaction tx);

        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile WaitCallback signalMethod;
        private static WaitCallback SignalMethod
        {
            get
            {
                if (signalMethod == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (signalMethod == null)
                        {
                            signalMethod = new WaitCallback(SignalCallback);
                        }
                    }
                }

                return signalMethod;
            }
        }


        private static void SignalCallback(object state)
        {
            InternalTransaction tx = (InternalTransaction)state;
            lock (tx)
            {
                tx.SignalAsyncCompletion();
            }
        }

    }

    // TransactionStatePromotedNonMSDTCAborted
    //
    // This state indicates that the transaction has been promoted to a non-MSDTC promoter and the outcome
    // of the transaction is aborted.
    internal class TransactionStatePromotedNonMSDTCAborted : TransactionStatePromotedNonMSDTCEnded
    {
        internal override void EnterState(InternalTransaction tx)
        {
            base.EnterState(tx);

            // Notify the enlistments that the transaction has aborted
            for (int i = 0; i < tx.phase0Volatiles.volatileEnlistmentCount; i++)
            {
                tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.InternalAborted(tx.phase0Volatiles.volatileEnlistments[i]);
            }

            for (int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++)
            {
                tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.InternalAborted(tx.phase1Volatiles.volatileEnlistments[i]);
            }

            // Notify the durable enlistment
            if (tx.durableEnlistment != null)
            {
                tx.durableEnlistment.State.InternalAborted(tx.durableEnlistment);
            }

            // Fire Completion for anyone listening
            tx.FireCompletion();
            // We don't need to do the AsyncCompletion stuff.  If it was needed, it was done out of SignalCallback.

            if (DiagnosticTrace.Warning)
            {
                TransactionAbortedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    tx.TransactionTraceId
                    );
            }

        }

        internal override TransactionStatus get_Status(InternalTransaction tx)
        {
            return TransactionStatus.Aborted;
        }

        internal override void Rollback(InternalTransaction tx, Exception e)
        {
            // Already done.
        }

        internal override void BeginCommit(InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState)
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }

        internal override void CreateBlockingClone(InternalTransaction tx)
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }

        internal override void CreateAbortingClone(InternalTransaction tx)
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }

        internal override void Phase0VolatilePrepareDone(InternalTransaction tx)
        {
            // Since the transaction is aborted ignore it.
        }

        internal override void Phase1VolatilePrepareDone(InternalTransaction tx)
        {
            // Since the transaction is aborted ignore it.
        }

        internal override void ChangeStateTransactionAborted(InternalTransaction tx, Exception e)
        {
            // This may come from a promotable single phase enlistments abort response.
        }

        protected override void PromotedTransactionOutcome(InternalTransaction tx)
        {
            if ((null == tx.innerException) && (null != tx.PromotedTransaction))
            {
                tx.innerException = tx.PromotedTransaction.InnerException;
            }
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }

        internal override void CheckForFinishedTransaction(InternalTransaction tx)
        {
            throw new TransactionAbortedException(tx.innerException, tx.DistributedTxId);
        }

        internal override void GetObjectData(InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context)
        {
            throw TransactionAbortedException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }
    }

    // TransactionStatePromotedNonMSDTCCommitted
    //
    // This state indicates that the transaction has been non-MSDTC promoted and the outcome
    // of the transaction is committed
    internal class TransactionStatePromotedNonMSDTCCommitted : TransactionStatePromotedNonMSDTCEnded
    {
        internal override void EnterState(InternalTransaction tx)
        {
            base.EnterState(tx);

            // Notify the phase 0 enlistments that the transaction has committed
            for (int i = 0; i < tx.phase0Volatiles.volatileEnlistmentCount; i++)
            {
                tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.InternalCommitted(tx.phase0Volatiles.volatileEnlistments[i]);
            }

            // Notify the phase 1 enlistments that the transaction has committed
            for (int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++)
            {
                tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.InternalCommitted(tx.phase1Volatiles.volatileEnlistments[i]);
            }

            // Fire Completion for anyone listening
            tx.FireCompletion();
            // We don't need to do the AsyncCompletion stuff.  If it was needed, it was done out of SignalCallback.

            if (DiagnosticTrace.Verbose)
            {
                TransactionCommittedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    tx.TransactionTraceId
                    );
            }

        }

        internal override TransactionStatus get_Status(InternalTransaction tx)
        {
            return TransactionStatus.Committed;
        }

        protected override void PromotedTransactionOutcome(InternalTransaction tx)
        {
            // This is a happy transaction.
        }
    }

    // TransactionStatePromotedNonMSDTCIndoubt
    //
    // This state indicates that the transaction has been non-MSDTC promoted but the outcome
    // of the transaction is indoubt.
    internal class TransactionStatePromotedNonMSDTCIndoubt : TransactionStatePromotedNonMSDTCEnded
    {
        internal override void EnterState(InternalTransaction tx)
        {
            base.EnterState(tx);

            // Notify the phase 0 enlistments that the transaction is indoubt
            for (int i = 0; i < tx.phase0Volatiles.volatileEnlistmentCount; i++)
            {
                tx.phase0Volatiles.volatileEnlistments[i].twoPhaseState.InternalIndoubt(tx.phase0Volatiles.volatileEnlistments[i]);
            }

            // Notify the phase 1 enlistments that the transaction is indoubt
            for (int i = 0; i < tx.phase1Volatiles.volatileEnlistmentCount; i++)
            {
                tx.phase1Volatiles.volatileEnlistments[i].twoPhaseState.InternalIndoubt(tx.phase1Volatiles.volatileEnlistments[i]);
            }

            // Fire Completion for anyone listening
            tx.FireCompletion();
            // We don't need to do the AsyncCompletion stuff.  If it was needed, it was done out of SignalCallback.

            if (DiagnosticTrace.Warning)
            {
                TransactionInDoubtTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    tx.TransactionTraceId
                    );
            }

        }

        internal override TransactionStatus get_Status(InternalTransaction tx)
        {
            return TransactionStatus.InDoubt;
        }

        internal override void ChangeStatePromotedPhase0(InternalTransaction tx)
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }

        internal override void ChangeStatePromotedPhase1(InternalTransaction tx)
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }

        protected override void PromotedTransactionOutcome(InternalTransaction tx)
        {
            if ((null == tx.innerException) && (null != tx.PromotedTransaction))
            {
                tx.innerException = tx.PromotedTransaction.InnerException;
            }
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }

        internal override void CheckForFinishedTransaction(InternalTransaction tx)
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }

        internal override void GetObjectData(InternalTransaction tx, SerializationInfo serializationInfo, StreamingContext context)
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionIndoubt), tx.innerException, tx.DistributedTxId);
        }

        internal override void CreateBlockingClone(InternalTransaction tx)
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }

        internal override void CreateAbortingClone(InternalTransaction tx)
        {
            throw TransactionInDoubtException.Create(SR.GetString(SR.TraceSourceLtm), SR.GetString(SR.TransactionAborted), tx.innerException, tx.DistributedTxId);
        }
    }

    // TransactionStateDelegatedNonMSDTC
    //
    // This state is the base state for delegated transactions to non-MSDTC promoters.
    internal class TransactionStateDelegatedNonMSDTC : TransactionStatePromotedNonMSDTCBase
    {
        internal override void EnterState(InternalTransaction tx)
        {
            // Assign the state
            CommonEnterState(tx);

            // We are never going to have an OletxTransaction for this one.
            Oletx.OletxTransaction distributedTx = null;
            try
            {
                // Ask the delegation interface to promote the transaction.
                if (DiagnosticTrace.Verbose && tx.durableEnlistment != null)
                {
                    EnlistmentNotificationCallTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        tx.durableEnlistment.EnlistmentTraceId,
                        NotificationCall.Promote
                        );
                }


                distributedTx = _TransactionStatePSPEOperation.PSPEPromote(tx);
                Debug.Assert((distributedTx == null), string.Format(null, "PSPEPromote for non-MSDTC promotion returned a distributed transaction."));
                Debug.Assert((tx.promotedToken != null), string.Format(null, "PSPEPromote for non-MSDTC promotion did not set InternalTransaction.PromotedToken."));
            }
            catch (TransactionPromotionException e)
            {
                tx.innerException = e;
                if (DiagnosticTrace.Verbose)
                {
                    ExceptionConsumedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        e);
                }
            }
            finally
            {
                if (tx.promotedToken == null)
                {
                    // There was an exception trying to promote the transaction.
                    tx.State.ChangeStateAbortedDuringPromotion(tx);
                }
            }
        }
    }

    // TransactionStateDelegatedSubordinate
    //
    // This state represents a transaction that is subordinate to another TM and has been
    // promoted.
    internal class TransactionStateDelegatedSubordinate : TransactionStateDelegatedBase
    {
        internal override bool PromoteDurable( InternalTransaction tx )
        {
            return true;
        }


        internal override void Rollback( InternalTransaction tx, Exception e )
        {
            // Pass the Rollback through the promotable single phase enlistment to be
            // certain it is notified.

            if ( tx.innerException == null )
            {
                tx.innerException = e;
            }

            tx.PromotedTransaction.Rollback();
            _TransactionStatePromotedAborted.EnterState( tx );
        }


        internal override void ChangeStatePromotedPhase0( InternalTransaction tx )
        {
            _TransactionStatePromotedPhase0.EnterState( tx );
        }


        internal override void ChangeStatePromotedPhase1( InternalTransaction tx )
        {
            _TransactionStatePromotedPhase1.EnterState( tx );
        }
    }


    // TransactionStatePSPEOperation
    //
    // Someone is trying to enlist for promotable single phase.  Don't allow them to do anything
    // ----.
    internal class TransactionStatePSPEOperation : TransactionState
    {
        internal override void EnterState(InternalTransaction tx)
        {
            // No one should ever use this particular version.  It has to be overridden because
            // the base is abstract.
            throw new InvalidOperationException();
        }


        internal override TransactionStatus get_Status( InternalTransaction tx )
        {
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal void PSPEInitialize( 
            InternalTransaction tx, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Guid promoterType)
        {
            Debug.Assert( tx.State == _TransactionStateActive, "PSPEPromote called from state other than TransactionStateActive" );
            CommonEnterState( tx );

            try
            {
                // Try to initialize the pspn.  If an exception is thrown let it propigate
                // all the way up to the caller.
                promotableSinglePhaseNotification.Initialize();

                // Set the PromoterType for the transaction.
                tx.promoterType = promoterType;
            }
            finally
            {
                _TransactionStateActive.CommonEnterState( tx );
            }
        }

        // This method will call the intialize method on IPromotableSinglePhaseNotification.
        // The tx state will be set to _TransactionStatePhase0 to receive and process further
        // enlistments during Phase0. 
        
        internal void Phase0PSPEInitialize( 
            InternalTransaction tx, 
            IPromotableSinglePhaseNotification promotableSinglePhaseNotification,
            Guid promoterType)
        {
            Debug.Assert( tx.State == _TransactionStatePhase0, "Phase0PSPEInitialize called from state other than _TransactionStatePhase0" );
            CommonEnterState( tx );

            try
            {
                // Try to initialize the PSPE.  If an exception is thrown let it propagate
                // all the way up to the caller.
                promotableSinglePhaseNotification.Initialize();

                // Set the PromoterType for the transaction.
                tx.promoterType = promoterType;
            }
            finally
            {
                 _TransactionStatePhase0.CommonEnterState(tx);
            }
        }
        

        internal Oletx.OletxTransaction PSPEPromote( InternalTransaction tx )
        {
            bool changeToReturnState = true;

            TransactionState returnState = tx.State;
            Debug.Assert( returnState == _TransactionStateDelegated || 
                returnState == _TransactionStateDelegatedSubordinate ||
                returnState == _TransactionStateDelegatedNonMSDTC, 
                "PSPEPromote called from state other than TransactionStateDelegated[NonMSDTC]" );
            CommonEnterState( tx );

            Oletx.OletxTransaction distributedTx = null;
            try
            {
                if (tx.attemptingPSPEPromote)
                {
                    // There should not already be a PSPEPromote call outstanding.
                    throw TransactionException.CreateInvalidOperationException(
                            SR.GetString(SR.TraceSourceLtm),
                            SR.GetString(SR.PromotedReturnedInvalidValue),
                            null,
                            tx.DistributedTxId
                            );
                }
                tx.attemptingPSPEPromote = true;

                Byte[] propagationToken = tx.promoter.Promote();

                // If the PromoterType is NOT MSDTC, then we can't assume that the returned
                // byte[] is an MSDTC propagation token and we can't create an OletxTransaction from it.
                if (tx.promoterType != TransactionInterop.PromoterTypeDtc)
                {
                    if (propagationToken == null)
                    {
                        throw TransactionException.CreateInvalidOperationException(
                                SR.GetString(SR.TraceSourceLtm),
                                SR.GetString(SR.PromotedReturnedInvalidValue),
                                null,
                                tx.DistributedTxId
                                );
                    }

                    tx.promotedToken = propagationToken;
                    return null;
                }

                // From this point forward, we know that the PromoterType is TransactionInterop.PromoterTypeDtc so we can
                // treat the propagationToken as an MSDTC propagation token. If one was returned.
                if ( propagationToken == null )
                {
                    // If the returned propagationToken is null AND the tx.PromotedTransaction is null, the promote failed.
                    // But if the PSPE promoter used PSPEPromoteAndConvertToEnlistDurable, tx.PromotedTransaction will NOT be null
                    // at this point and we just use tx.PromotedTransaction as distributedTx and we don't bother to change to the
                    // "return state" because the transaction is already in the state it needs to be in.
                    if (tx.PromotedTransaction == null)
                    {
                        // The PSPE has returned an invalid promoted transaction.
                        throw TransactionException.CreateInvalidOperationException(
                                SR.GetString(SR.TraceSourceLtm),
                                SR.GetString(SR.PromotedReturnedInvalidValue),
                                null,
                                tx.DistributedTxId
                                );
                    }
                    // The transaction has already transitioned to TransactionStatePromoted, so we don't want
                    // to change the state to the "returnState" because TransactionStateDelegatedBase.EnterState, would
                    // try to promote the enlistments again.
                    changeToReturnState = false;
                    distributedTx = tx.PromotedTransaction;
                }

                // At this point, if we haven't yet set distributedTx, we need to get it using the returned
                // propagation token. The PSPE promoter must NOT have used PSPEPromoteAndConvertToEnlistDurable.
                if (distributedTx == null)
                {
                    try
                    {
                        distributedTx = TransactionInterop.GetOletxTransactionFromTransmitterPropigationToken(
                                            propagationToken
                                            );
                    }
                    catch (ArgumentException e)
                    {
                        // The PSPE has returned an invalid promoted transaction.
                        throw TransactionException.CreateInvalidOperationException(
                                SR.GetString(SR.TraceSourceLtm),
                                SR.GetString(SR.PromotedReturnedInvalidValue),
                                e,
                                tx.DistributedTxId
                                );
                    }

                    if (TransactionManager.FindPromotedTransaction(distributedTx.Identifier) != null)
                    {
                        // If there is already a promoted transaction then someone has committed an error.
                        distributedTx.Dispose();
                        throw TransactionException.CreateInvalidOperationException(
                                SR.GetString(SR.TraceSourceLtm),
                                SR.GetString(SR.PromotedTransactionExists),
                                null,
                                tx.DistributedTxId
                                );
                    }
                }
            }
            finally
            {
                tx.attemptingPSPEPromote = false;
                // If we get here and changeToReturnState is false, the PSPE enlistment must have requested that we
                // promote and convert the enlistment to a durable enlistment
                // (Transaction.PSPEPromoteAndConvertToEnlistDurable). In that case, the internal transaction is
                // already in TransactionStatePromoted, so we don't want to put it BACK into TransactionStateDelegatedBase.
                if (changeToReturnState)
                {
                    returnState.CommonEnterState(tx);
                }
            }

            return distributedTx;
        }

        internal override Enlistment PromoteAndEnlistDurable(
            InternalTransaction tx,
            Guid resourceManagerIdentifier,
            IPromotableSinglePhaseNotification promotableNotification,
            ISinglePhaseNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions,
            Transaction atomicTransaction
            )
        {
            // This call is only allowed if we have an outstanding call to ITransactionPromoter.Promote.
            if (!tx.attemptingPSPEPromote)
            {
                throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
            }

            if (promotableNotification != tx.promoter)
            {
                throw TransactionException.CreateInvalidOperationException(
                        SR.GetString(SR.TraceSourceLtm),
                        SR.GetString(SR.InvalidIPromotableSinglePhaseNotificationSpecified),
                        null,
                        tx.DistributedTxId
                        );
            }

            Enlistment enlistment;

            // First promote the transaction. We do this by simply changing the state of the transaction to Promoted.
            // In TransactionStateActive.EnlistPromotableSinglePhase, tx.durableEnlistment was set to point at the InternalEnlistment
            // for that PSPE enlistment. We are going to replace that with a "true" durable enlistment here. But we need to
            // set tx.durableEnlistment to null BEFORE we promote because if we don't the promotion will attempt to promote
            // the tx.durableEnlistment. Because we are doing the EnlistDurable AFTER promotion, it will be a "promoted"
            // durable enlistment and we can safely set tx.durableEnlistment to the InternalEnlistment of that Enlistment.
            tx.durableEnlistment = null;
            tx.promoteState = TransactionState._TransactionStatePromoted;
            tx.promoteState.EnterState(tx);

            // Now we need to create the durable enlistment that will replace the PSPE enlistment. Use the internalEnlistment of
            // this newly created durable enlistment as the tx.durableEnlistment.
            enlistment = tx.State.EnlistDurable(tx, resourceManagerIdentifier, enlistmentNotification, enlistmentOptions, atomicTransaction);
            tx.durableEnlistment = enlistment.InternalEnlistment;

            return enlistment;
        }

        // TransactionStatePSPEOperation is the only state where this is allowed and we further check to make sure there is
        // an outstanding call to ITransactionPromoter.Promote and that the specified promotableNotification matches the
        // transaction's promoter object.
        internal override void SetDistributedTransactionId(InternalTransaction tx,
                    IPromotableSinglePhaseNotification promotableNotification,
                    Guid distributedTransactionIdentifier)
        {
            // This call is only allowed if we have an outstanding call to ITransactionPromoter.Promote.
            if (!tx.attemptingPSPEPromote)
            {
                throw TransactionException.CreateTransactionStateException(SR.GetString(SR.TraceSourceLtm), tx.innerException, tx.DistributedTxId);
            }

            if (promotableNotification != tx.promoter)
            {
                throw TransactionException.CreateInvalidOperationException(
                        SR.GetString(SR.TraceSourceLtm),
                        SR.GetString(SR.InvalidIPromotableSinglePhaseNotificationSpecified),
                        null,
                        tx.DistributedTxId
                        );
            }

            tx.distributedTransactionIdentifierNonMSDTC = distributedTransactionIdentifier;
        }
    }


    // TransactionStateDelegatedP0Wave
    //
    // This state is exactly the same as TransactionStatePromotedP0Wave with
    // the exception that when commit is restarted it is restarted in a different
    // way.
    internal class TransactionStateDelegatedP0Wave : TransactionStatePromotedP0Wave
    {
        internal override void Phase0VolatilePrepareDone( InternalTransaction tx )
        {
            _TransactionStateDelegatedCommitting.EnterState( tx );
        }
    }


    // TransactionStateDelegatedCommitting
    //
    // The transaction has been promoted but is in the process of committing.
    internal class TransactionStateDelegatedCommitting : TransactionStatePromotedCommitting
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            // Forward this on to the promotable single phase enlisment
            System.Threading.Monitor.Exit( tx );

            if ( DiagnosticTrace.Verbose )
            {
                EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    tx.durableEnlistment.EnlistmentTraceId,
                    NotificationCall.SinglePhaseCommit
                    );
            }

            try
            {
                tx.durableEnlistment.PromotableSinglePhaseNotification.SinglePhaseCommit( 
                    tx.durableEnlistment.SinglePhaseEnlistment );
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }
    }


    // TransactionStateDelegatedAborting
    //
    // The transaction has been promoted but is in the process of committing.
    internal class TransactionStateDelegatedAborting : TransactionStatePromotedAborted
    {
        internal override void EnterState( InternalTransaction tx )
        {
            CommonEnterState( tx );

            // The distributed TM is driving the commit processing, so marking of complete
            // is done in TransactionStatePromotedPhase0Aborting.EnterState or
            // TransactionStatePromotedPhase1Aborting.EnterState.

            // Release the lock
            System.Threading.Monitor.Exit( tx );
            try
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        tx.durableEnlistment.EnlistmentTraceId,
                        NotificationCall.Rollback
                        );
                }

                tx.durableEnlistment.PromotableSinglePhaseNotification.Rollback( 
                    tx.durableEnlistment.SinglePhaseEnlistment );
            }
            finally
            {
#pragma warning disable 0618
                //@
                System.Threading.Monitor.Enter(tx);
#pragma warning restore 0618
            }
        }


        internal override void BeginCommit( InternalTransaction tx, bool asyncCommit, AsyncCallback asyncCallback, object asyncState )
        {
            // Initiate the commit process.
            throw TransactionException.CreateTransactionStateException( SR.GetString( SR.TraceSourceLtm ), tx.innerException, tx.DistributedTxId);
        }


        internal override void ChangeStatePromotedAborted( InternalTransaction tx )
        {
            _TransactionStatePromotedAborted.EnterState( tx );
        }
    }
}
