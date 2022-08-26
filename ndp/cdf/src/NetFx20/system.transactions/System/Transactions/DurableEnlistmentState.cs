//-----------------------------------------------------------------------------
// <copyright file="DurableEnlistmentState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Security;
    using System.Threading;
    using System.Transactions.Diagnostics;

    // Base class for all durable enlistment states
    internal abstract class DurableEnlistmentState : EnlistmentState
    {
        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile DurableEnlistmentActive _durableEnlistmentActive;
        private static volatile DurableEnlistmentAborting _durableEnlistmentAborting;
        private static volatile DurableEnlistmentCommitting _durableEnlistmentCommitting;
        private static volatile DurableEnlistmentDelegated _durableEnlistmentDelegated;
        private static volatile DurableEnlistmentEnded _durableEnlistmentEnded;

        // Object for synchronizing access to the entire class( avoiding lock( typeof( ... )) )
        private static object classSyncObject;

        internal static DurableEnlistmentActive _DurableEnlistmentActive
        {
            get
            {
                if (_durableEnlistmentActive == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_durableEnlistmentActive == null)
                        {
                            DurableEnlistmentActive temp = new DurableEnlistmentActive();
                            _durableEnlistmentActive = temp;
                        }
                    }
                }

                return _durableEnlistmentActive;
            }
        }


        protected static DurableEnlistmentAborting _DurableEnlistmentAborting
        {
            get
            {
                if (_durableEnlistmentAborting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_durableEnlistmentAborting == null)
                        {
                            DurableEnlistmentAborting temp = new DurableEnlistmentAborting();
                            _durableEnlistmentAborting = temp;
                        }
                    }
                }

                return _durableEnlistmentAborting;
            }
        }


        protected static DurableEnlistmentCommitting _DurableEnlistmentCommitting
        {
            get
            {
                if (_durableEnlistmentCommitting == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_durableEnlistmentCommitting == null)
                        {
                            DurableEnlistmentCommitting temp = new DurableEnlistmentCommitting();
                            _durableEnlistmentCommitting = temp;
                        }
                    }
                }

                return _durableEnlistmentCommitting;
            }
        }


        protected static DurableEnlistmentDelegated _DurableEnlistmentDelegated
        {
            get
            {
                if (_durableEnlistmentDelegated == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_durableEnlistmentDelegated == null)
                        {
                            DurableEnlistmentDelegated temp = new DurableEnlistmentDelegated();
                            _durableEnlistmentDelegated = temp;
                        }
                    }
                }

                return _durableEnlistmentDelegated;
            }
        }


        protected static DurableEnlistmentEnded _DurableEnlistmentEnded
        {
            get
            {
                if (_durableEnlistmentEnded == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_durableEnlistmentEnded == null)
                        {
                            DurableEnlistmentEnded temp = new DurableEnlistmentEnded();
                            _durableEnlistmentEnded = temp;
                        }
                    }
                }

                return _durableEnlistmentEnded;
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
    }



    // Active state for a durable enlistment.  In this state the transaction can be aborted 
    // asynchronously by calling abort.
    internal class DurableEnlistmentActive : DurableEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            // Yeah it's active
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // Mark the enlistment as done.
            _DurableEnlistmentEnded.EnterState( enlistment );
        }


        internal override void InternalAborted( InternalEnlistment enlistment )
        {
            // Transition to the aborting state
            _DurableEnlistmentAborting.EnterState( enlistment );
        }


        internal override void ChangeStateCommitting( InternalEnlistment enlistment )
        {
            // Transition to the committing state
            _DurableEnlistmentCommitting.EnterState( enlistment );
        }


        internal override void ChangeStatePromoted( InternalEnlistment enlistment, IPromotedEnlistment promotedEnlistment )
        {
            // Save the promoted enlistment because future notifications must be sent here.
            enlistment.PromotedEnlistment = promotedEnlistment;

            // The transaction is being promoted promote the enlistment as well
            _EnlistmentStatePromoted.EnterState( enlistment );
        }


        internal override void ChangeStateDelegated( InternalEnlistment enlistment )
        {
            // This is a valid state transition.
            _DurableEnlistmentDelegated.EnterState( enlistment );
        }
    }



    // Aborting state for a durable enlistment.  In this state the transaction has been aborted,
    // by someone other than the enlistment.
    //
    internal class DurableEnlistmentAborting : DurableEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            Monitor.Exit( enlistment.Transaction );
            try
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        enlistment.EnlistmentTraceId,
                        NotificationCall.Rollback
                        );
                }

                // Send the Rollback notification to the enlistment
                if ( enlistment.SinglePhaseNotification != null )
                {
                    enlistment.SinglePhaseNotification.Rollback( enlistment.SinglePhaseEnlistment );
                }
                else
                {
                    enlistment.PromotableSinglePhaseNotification.Rollback( enlistment.SinglePhaseEnlistment );
                }
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void Aborted( InternalEnlistment enlistment, Exception e )
        {
            if ( enlistment.Transaction.innerException == null )
            {
                enlistment.Transaction.innerException = e;
            }
            
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );
        }
    }

    
    // Committing state is when SPC has been sent to an enlistment but no response
    // has been received.
    //
    internal class DurableEnlistmentCommitting : DurableEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            bool spcCommitted = false; 
            // Set the enlistment state
            enlistment.State = this;
            
            Monitor.Exit( enlistment.Transaction );
            try
            {
                if ( DiagnosticTrace.Verbose )
                {
                    EnlistmentNotificationCallTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        enlistment.EnlistmentTraceId,
                        NotificationCall.SinglePhaseCommit
                        );
                }

                // Send the Commit notification to the enlistment
                if ( enlistment.SinglePhaseNotification != null )
                {
                    enlistment.SinglePhaseNotification.SinglePhaseCommit( enlistment.SinglePhaseEnlistment );
                }
                else
                {
                    enlistment.PromotableSinglePhaseNotification.SinglePhaseCommit( enlistment.SinglePhaseEnlistment );
                }
                spcCommitted = true; 
            }
            finally
            {
                if (!spcCommitted)
                {
                    enlistment.SinglePhaseEnlistment.InDoubt();
                }
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.Transaction);
#pragma warning restore 0618
            }
        }


        internal override void EnlistmentDone(InternalEnlistment enlistment)
        {
            // EnlistmentDone should be treated the same as Committed from this state.
            // This eliminates a ---- between the SPC call and the EnlistmentDone call.

            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            // Make the transaction commit
            enlistment.Transaction.State.ChangeStateTransactionCommitted( enlistment.Transaction );
        }


        internal override void Committed( InternalEnlistment enlistment )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            // Make the transaction commit
            enlistment.Transaction.State.ChangeStateTransactionCommitted( enlistment.Transaction );
        }


        internal override void Aborted( InternalEnlistment enlistment, Exception e )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            // Start the transaction aborting
            enlistment.Transaction.State.ChangeStateTransactionAborted( enlistment.Transaction, e );
        }


        internal override void InDoubt( InternalEnlistment enlistment, Exception e )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            if ( enlistment.Transaction.innerException == null )
            {
                enlistment.Transaction.innerException = e;
            }

            // Make the transaction in dobut
            enlistment.Transaction.State.InDoubtFromEnlistment( enlistment.Transaction );
        }
    }


    // Delegated state for a durable enlistment represents an enlistment that was
    // origionally a PromotableSinglePhaseEnlisment that where promotion has happened.
    // These enlistments don't need to participate in the commit process anymore.
    internal class DurableEnlistmentDelegated : DurableEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;

            // At this point the durable enlistment should have someone to forward to.
            Debug.Assert( enlistment.PromotableSinglePhaseNotification != null );
        }


        internal override void Committed( InternalEnlistment enlistment )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            // Change the transaction to committed.
            enlistment.Transaction.State.ChangeStatePromotedCommitted( enlistment.Transaction );
        }


        internal override void Aborted( InternalEnlistment enlistment, Exception e )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            if ( enlistment.Transaction.innerException == null )
            {
                enlistment.Transaction.innerException = e;
            }

            // Start the transaction aborting
            enlistment.Transaction.State.ChangeStatePromotedAborted( enlistment.Transaction );
        }


        internal override void InDoubt( InternalEnlistment enlistment, Exception e )
        {
            // Transition to the ended state
            _DurableEnlistmentEnded.EnterState( enlistment );

            if ( enlistment.Transaction.innerException == null )
            {
                enlistment.Transaction.innerException = e;
            }

            // Tell the transaction that the enlistment is InDoubt.  Note that
            // for a transaction that has been delegated and then promoted there
            // are two chances to get a better answer than indoubt.  So it may be that
            // the TM will have a better answer.
            enlistment.Transaction.State.InDoubtFromEnlistment( enlistment.Transaction );
        }
    }


    // Ended state is the state that is entered when the durable enlistment has committed,
    // aborted, or said read only for an enlistment.  At this point there are no valid
    // operations on the enlistment.
    internal class DurableEnlistmentEnded : DurableEnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            // Set the enlistment state
            enlistment.State = this;
        }

        internal override void InternalAborted(InternalEnlistment enlistment)
        {
            // From the Aborting state the transaction may tell the enlistment to abort.  At this point 
            // it already knows.  Eat this message.
        }

        internal override void InDoubt(InternalEnlistment enlistment, Exception e)
        {
            // Ignore this in case the enlistment gets here before
            // the transaction tells it to do so
        }
    }
}

