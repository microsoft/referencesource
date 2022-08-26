//-----------------------------------------------------------------------------
// <copyright file="VolatileEnlistmentState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Threading;
    using System.Transactions.Diagnostics;

    internal delegate void FinishVolatileDelegate( InternalEnlistment enlistment );

    // Base class for all volatile enlistment states
    internal abstract class VolatileEnlistmentState : EnlistmentState
    {
        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile VolatileEnlistmentActive _volatileEnlistmentActive;
        private static volatile VolatileEnlistmentPreparing _volatileEnlistmentPreparing;
        private static volatile VolatileEnlistmentPrepared _volatileEnlistmentPrepared;
        private static volatile VolatileEnlistmentSPC _volatileEnlistmentSPC;
        private static volatile VolatileEnlistmentPreparingAborting _volatileEnlistmentPreparingAborting;
        private static volatile VolatileEnlistmentAborting _volatileEnlistmentAborting;
        private static volatile VolatileEnlistmentCommitting _volatileEnlistmentCommitting;
        private static volatile VolatileEnlistmentInDoubt _volatileEnlistmentInDoubt;
        private static volatile VolatileEnlistmentEnded _volatileEnlistmentEnded;
        private static volatile VolatileEnlistmentDone _volatileEnlistmentDone;

        // Object for synchronizing access to the entire class( avoiding lock( typeof( ... )) )
        private static object classSyncObject;

        internal static VolatileEnlistmentActive _VolatileEnlistmentActive
        {
            get
            {
                if (_volatileEnlistmentActive == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentActive == null)
                        {
                            VolatileEnlistmentActive temp = new VolatileEnlistmentActive();
                            _volatileEnlistmentActive = temp;
                        }
                    }
                }

                return _volatileEnlistmentActive;
            }
        }


        protected static VolatileEnlistmentPreparing _VolatileEnlistmentPreparing
        {
            get
            {
                if (_volatileEnlistmentPreparing == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentPreparing == null)
                        {
                            VolatileEnlistmentPreparing temp = new VolatileEnlistmentPreparing();
                            _volatileEnlistmentPreparing = temp;
                        }
                    }
                }

                return _volatileEnlistmentPreparing;
            }
        }

        
        protected static VolatileEnlistmentPrepared _VolatileEnlistmentPrepared
        {
            get
            {
                if (_volatileEnlistmentPrepared == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentPrepared == null)
                        {
                            VolatileEnlistmentPrepared temp = new VolatileEnlistmentPrepared();
                            _volatileEnlistmentPrepared = temp;
                        }
                    }
                }

                return _volatileEnlistmentPrepared;
            }
        }

        
        protected static VolatileEnlistmentSPC _VolatileEnlistmentSPC
        {
            get
            {
                if (_volatileEnlistmentSPC == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentSPC == null)
                        {
                            VolatileEnlistmentSPC temp = new VolatileEnlistmentSPC();
                            _volatileEnlistmentSPC = temp;
                        }
                    }
                }

                return _volatileEnlistmentSPC;
            }
        }

        
        protected static VolatileEnlistmentPreparingAborting _VolatileEnlistmentPreparingAborting
        {
            get
            {
                if (_volatileEnlistmentPreparingAborting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentPreparingAborting == null)
                        {
                            VolatileEnlistmentPreparingAborting temp = new VolatileEnlistmentPreparingAborting();
                            _volatileEnlistmentPreparingAborting = temp;
                        }
                    }
                }

                return _volatileEnlistmentPreparingAborting;
            }
        }

        
        protected static VolatileEnlistmentAborting _VolatileEnlistmentAborting
        {
            get
            {
                if (_volatileEnlistmentAborting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentAborting == null)
                        {
                            VolatileEnlistmentAborting temp = new VolatileEnlistmentAborting();
                            _volatileEnlistmentAborting = temp;
                        }
                    }
                }

                return _volatileEnlistmentAborting;
            }
        }

        
        protected static VolatileEnlistmentCommitting _VolatileEnlistmentCommitting
        {
            get
            {
                if (_volatileEnlistmentCommitting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentCommitting == null)
                        {
                            VolatileEnlistmentCommitting temp = new VolatileEnlistmentCommitting();
                            _volatileEnlistmentCommitting = temp;
                        }
                    }
                }

                return _volatileEnlistmentCommitting;
            }
        }

        protected static VolatileEnlistmentInDoubt _VolatileEnlistmentInDoubt
        {
            get
            {
                if (_volatileEnlistmentInDoubt == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentInDoubt == null)
                        {
                            VolatileEnlistmentInDoubt temp = new VolatileEnlistmentInDoubt();
                            _volatileEnlistmentInDoubt = temp;
                        }
                    }
                }

                return _volatileEnlistmentInDoubt;
            }
        }


        protected static VolatileEnlistmentEnded _VolatileEnlistmentEnded
        {
            get
            {
                if (_volatileEnlistmentEnded == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentEnded == null)
                        {
                            VolatileEnlistmentEnded temp = new VolatileEnlistmentEnded();
                            _volatileEnlistmentEnded = temp;
                        }
                    }
                }

                return _volatileEnlistmentEnded;
            }
        }

        
        protected static VolatileEnlistmentDone _VolatileEnlistmentDone
        {
            get
            {
                if (_volatileEnlistmentDone == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_volatileEnlistmentDone == null)
                        {
                            VolatileEnlistmentDone temp = new VolatileEnlistmentDone();
                            _volatileEnlistmentDone = temp;
                        }
                    }
                }

                return _volatileEnlistmentDone;
            }
        }

        

        // Helper object for static synchronization
        private static object ClassSyncObject
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

        // Override of get_RecoveryInformation to be more specific with the exception string.
        internal override byte[] RecoveryInformation(InternalEnlistment enlistment)
        {
            throw TransactionException.CreateInvalidOperationException( SR.GetString( SR.TraceSourceLtm ),
                SR.GetString( SR.VolEnlistNoRecoveryInfo), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId );
        }

    }



    // Active state for a volatile enlistment indicates that the enlistment has been created 
    // but no one has begun committing or aborting the transaction.  From this state the enlistment
    // can abort the transaction or call read only to indicate that it does not want to 
    // participate further in the transaction.
    internal class VolatileEnlistmentActive : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            // Yeah it's active.
        }


        #region IEnlistment Related Events

        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // End this enlistment
            _VolatileEnlistmentDone.EnterState( enlistment );

            // Note another enlistment finished.
            enlistment.FinishEnlistment();
        }

        #endregion


        #region State Change Events

        internal override void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentPreparing.EnterState( enlistment );
        }


        internal override void ChangeStateSinglePhaseCommit( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentSPC.EnterState( enlistment );
        }


        #endregion


        #region Internal Events

        internal override void InternalAborted(InternalEnlistment enlistment)
        {
            // Change the enlistment state to aborting.
            _VolatileEnlistmentAborting.EnterState( enlistment );
        }

        #endregion
    }



    // Preparing state is the time after prepare has been called but no response has been received.
    internal class VolatileEnlistmentPreparing : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            Monitor.Exit( enlistment.Transaction );
            try // Don't hold this lock while calling into the application code.
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        enlistment.EnlistmentTraceId,
                        NotificationCall.Prepare
                        );
                }

                enlistment.EnlistmentNotification.Prepare( enlistment.PreparingEnlistment );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentDone.EnterState( enlistment );

            // Process Finished InternalEnlistment
            enlistment.FinishEnlistment();
        }


        internal override void Prepared( InternalEnlistment enlistment )
        {
            // Change the enlistments state to prepared.
            _VolatileEnlistmentPrepared.EnterState( enlistment );

            // Process Finished InternalEnlistment
            enlistment.FinishEnlistment();
        }


        // The enlistment says to abort start the abort sequence.
        internal override void ForceRollback( InternalEnlistment enlistment, Exception e )
        {
            // Change enlistment state to aborting
            _VolatileEnlistmentEnded.EnterState( enlistment );

            // Start the transaction aborting
            enlistment.Transaction.State.ChangeStateTransactionAborted( enlistment.Transaction, e );

            // Process Finished InternalEnlistment
            enlistment.FinishEnlistment();
        }


        internal override void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            // If the transaction promotes during phase 0 then the transition to
            // the promoted phase 0 state for the transaction may cause this 
            // notification to be delivered again.  So in this case it should be
            // ignored.
        }


        internal override void InternalAborted( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentPreparingAborting.EnterState( enlistment );
        }
    }



    // SPC state for a volatile enlistment is the point at which there is exactly 1 enlisment
    // and it supports SPC.  The TM will send a single phase commit to the enlistment and wait
    // for the response from the TM.
    internal class VolatileEnlistmentSPC : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            bool spcCommitted = false; 
            // Set the enlistment state
            enlistment.State = this;

            // Send Single Phase Commit to the enlistment
            if ( DiagnosticTrace.Verbose )
            {
                EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    enlistment.EnlistmentTraceId,
                    NotificationCall.SinglePhaseCommit
                    );
            }

            Monitor.Exit( enlistment.Transaction );
            try // Don't hold this lock while calling into the application code.
            {
                enlistment.SinglePhaseNotification.SinglePhaseCommit( enlistment.SinglePhaseEnlistment );
                spcCommitted = true; 
            }
            finally
            {
                if (!spcCommitted)
                {
                    //If we have an exception thrown in SPC, we don't know the if the enlistment is committed or not
                    //reply indoubt 
                    enlistment.SinglePhaseEnlistment.InDoubt();
                }
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentEnded.EnterState( enlistment );
            enlistment.Transaction.State.ChangeStateTransactionCommitted( enlistment.Transaction );
        }


        internal override void Committed( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentEnded.EnterState( enlistment );
            enlistment.Transaction.State.ChangeStateTransactionCommitted( enlistment.Transaction );
        }


        internal override void Aborted( InternalEnlistment enlistment, Exception e )
        {
            _VolatileEnlistmentEnded.EnterState( enlistment );

            enlistment.Transaction.State.ChangeStateTransactionAborted( enlistment.Transaction, e );
        }


        internal override void InDoubt( InternalEnlistment enlistment, Exception e )
        {
            _VolatileEnlistmentEnded.EnterState( enlistment );

            if ( enlistment.Transaction.innerException == null )
            {
                enlistment.Transaction.innerException = e;
            }

            enlistment.Transaction.State.InDoubtFromEnlistment( enlistment.Transaction );
        }

    }

    
    // Prepared state for a volatile enlistment is the point at which prepare has been called
    // and the enlistment has responded prepared.  No enlistment operations are valid at this
    // point.  The RM must wait for the TM to take the next action.
    internal class VolatileEnlistmentPrepared : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            // Wait for Committed
        }


        internal override void InternalAborted(InternalEnlistment enlistment)
        {
            _VolatileEnlistmentAborting.EnterState( enlistment );
        }


        internal override void InternalCommitted( InternalEnlistment enlistment )
        {
            _VolatileEnlistmentCommitting.EnterState( enlistment );
        }

        
        internal override void InternalIndoubt( InternalEnlistment enlistment )
        {
            // Change the enlistment state to InDoubt.
            _VolatileEnlistmentInDoubt.EnterState( enlistment );
        }

        internal override void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            // This would happen in the second pass of a phase 0 wave.
        }
    }



    // Aborting state is when Rollback has been sent to the enlistment.
    internal class VolatileEnlistmentPreparingAborting : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // Move this enlistment to the ended state
            _VolatileEnlistmentEnded.EnterState( enlistment );
        }


        internal override void Prepared( InternalEnlistment enlistment )
        {
            // The enlistment has respondend so changes it's state to aborting.
            _VolatileEnlistmentAborting.EnterState( enlistment );

            // Process Finished InternalEnlistment
            enlistment.FinishEnlistment();
        }


        // The enlistment says to abort start the abort sequence.
        internal override void ForceRollback( InternalEnlistment enlistment, Exception e )
        {
            // Change enlistment state to aborting
            _VolatileEnlistmentEnded.EnterState( enlistment );

            // Record the exception in the transaction
            if ( enlistment.Transaction.innerException == null )
            {
                // Arguably this is the second call to ForceRollback and not the call that
                // aborted the transaction but just in case.
                enlistment.Transaction.innerException = e;
            }

            // Process Finished InternalEnlistment
            enlistment.FinishEnlistment();
        }


        internal override void InternalAborted( InternalEnlistment enlistment )
        {
            // If this event comes from multiple places just ignore it.  Continue
            // waiting for the enlistment to respond so that we can respond to it.
        }
    }



    // Aborting state is when Rollback has been sent to the enlistment.
    internal class VolatileEnlistmentAborting : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            Monitor.Exit( enlistment.Transaction );
            try // Don't hold this lock while calling into the application code.
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        enlistment.EnlistmentTraceId,
                        NotificationCall.Rollback
                        );
                }

                enlistment.EnlistmentNotification.Rollback( enlistment.SinglePhaseEnlistment );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            // This enlistment was told to abort before being told to prepare
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // Move this enlistment to the ended state
            _VolatileEnlistmentEnded.EnterState( enlistment );
        }

        internal override void InternalAborted( InternalEnlistment enlistment )
        {
            // Already working on it.
        }
    }


    
    // Committing state is when Commit has been sent to the enlistment.
    internal class VolatileEnlistmentCommitting : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            Monitor.Exit( enlistment.Transaction );
            try // Don't hold this lock while calling into the application code.
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        enlistment.EnlistmentTraceId,
                        NotificationCall.Commit
                        );
                }

                // Forward the notification to the enlistment
                enlistment.EnlistmentNotification.Commit( enlistment.Enlistment );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // Move this enlistment to the ended state
            _VolatileEnlistmentEnded.EnterState( enlistment );
        }
    }

    
    // InDoubt state is for an enlistment that has sent indoubt but has not been responeded to.
    internal class VolatileEnlistmentInDoubt : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            Monitor.Exit( enlistment.Transaction );
            try // Don't hold this lock while calling into the application code.
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        enlistment.EnlistmentTraceId,
                        NotificationCall.InDoubt
                        );
                }

                // Forward the notification to the enlistment
                enlistment.EnlistmentNotification.InDoubt( enlistment.PreparingEnlistment );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // Move this enlistment to the ended state
            _VolatileEnlistmentEnded.EnterState( enlistment );
        }
    }


    // Ended state is the state that is entered when the transaction has committed,
    // aborted, or said read only for an enlistment.  At this point there are no valid
    // operations on the enlistment.
    internal class VolatileEnlistmentEnded : VolatileEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            // Nothing to do.
        }


        internal override void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            // This enlistment was told to abort before being told to prepare
        }


        internal override void InternalAborted( InternalEnlistment enlistment )
        {
            // Ignore this in case the enlistment gets here before
            // the transaction tells it to do so
        }

        
        internal override void InternalCommitted( InternalEnlistment enlistment )
        {
            // Ignore this in case the enlistment gets here before
            // the transaction tells it to do so
        }


        internal override void InternalIndoubt(InternalEnlistment enlistment)
        {
            // Ignore this in case the enlistment gets here before
            // the transaction tells it to do so
        }

        internal override void InDoubt(InternalEnlistment enlistment, Exception e)
        {
            // Ignore this in case the enlistment gets here before
            // the transaction tells it to do so
        }
    }


    // At some point either early or late the enlistment responded ReadOnly
    internal class VolatileEnlistmentDone : VolatileEnlistmentEnded
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            // Nothing to do.
        }


        internal override void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            enlistment.CheckComplete();
        }
    }
}

