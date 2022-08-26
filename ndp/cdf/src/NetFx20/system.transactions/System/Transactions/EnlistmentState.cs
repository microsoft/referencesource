//-----------------------------------------------------------------------------
// <copyright file="EnlistmentState.cs" company="Microsoft">
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


    // Base class for all enlistment states
    abstract class EnlistmentState
    {
        internal abstract void EnterState( InternalEnlistment enlistment );


        // Double-checked locking pattern requires volatile for read/write synchronization
        internal static volatile EnlistmentStatePromoted _enlistmentStatePromoted;

        // Object for synchronizing access to the entire class( avoiding lock( typeof( ... )) )
        private static object classSyncObject;


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


        internal static EnlistmentStatePromoted _EnlistmentStatePromoted
        {
            get
            {
                if (_enlistmentStatePromoted == null)
                {
                    lock (ClassSyncObject)
                    {
                        if (_enlistmentStatePromoted == null)
                        {
                            EnlistmentStatePromoted temp = new EnlistmentStatePromoted();
                            _enlistmentStatePromoted = temp;
                        }
                    }
                }

                return _enlistmentStatePromoted;
            }
        }



        internal virtual void EnlistmentDone( InternalEnlistment enlistment )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void Prepared( InternalEnlistment enlistment )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void ForceRollback( InternalEnlistment enlistment, Exception e )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void Committed( InternalEnlistment enlistment )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void Aborted( InternalEnlistment enlistment, Exception e )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void InDoubt( InternalEnlistment enlistment, Exception e )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual byte[] RecoveryInformation( InternalEnlistment enlistment )
        {
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void InternalAborted( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }

                
        internal virtual void InternalCommitted( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void InternalIndoubt( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void ChangeStateCommitting( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void ChangeStatePromoted( InternalEnlistment enlistment, IPromotedEnlistment promotedEnlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void ChangeStateDelegated( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void ChangeStatePreparing( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }


        internal virtual void ChangeStateSinglePhaseCommit( InternalEnlistment enlistment )
        {
            Debug.Assert( false, string.Format( null, "Invalid Event for InternalEnlistment State; Current State: {0}", this.GetType() ));
            throw TransactionException.CreateEnlistmentStateException(SR.GetString(SR.TraceSourceLtm), null, enlistment == null ? Guid.Empty : enlistment.DistributedTxId);
        }
    }



    internal class EnlistmentStatePromoted : EnlistmentState
    {
        internal override void EnterState( InternalEnlistment enlistment )
        {
            enlistment.State = this;
        }


        internal override void EnlistmentDone( InternalEnlistment enlistment )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                enlistment.PromotedEnlistment.EnlistmentDone();
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }


        internal override void Prepared( InternalEnlistment enlistment )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                enlistment.PromotedEnlistment.Prepared();
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }


        internal override void ForceRollback( InternalEnlistment enlistment, Exception e )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                enlistment.PromotedEnlistment.ForceRollback( e );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }


        internal override void Committed( InternalEnlistment enlistment )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                enlistment.PromotedEnlistment.Committed();
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }


        internal override void Aborted( InternalEnlistment enlistment, Exception e )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                enlistment.PromotedEnlistment.Aborted( e );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }


        internal override void InDoubt( InternalEnlistment enlistment, Exception e )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                enlistment.PromotedEnlistment.InDoubt( e );
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }


        internal override byte[] RecoveryInformation( InternalEnlistment enlistment )
        {
            Monitor.Exit( enlistment.SyncRoot );
            try
            {
                return enlistment.PromotedEnlistment.GetRecoveryInformation();
            }
            finally
            {
#pragma warning disable 0618
                //@
                Monitor.Enter(enlistment.SyncRoot);
#pragma warning restore 0618
            }
        }
    }
}

