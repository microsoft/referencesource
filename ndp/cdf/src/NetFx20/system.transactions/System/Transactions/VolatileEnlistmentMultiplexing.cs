//-----------------------------------------------------------------------------
// <copyright file="VolatileEnlistmentMultiplexing.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using System.Transactions;
    using System.Transactions.Diagnostics;


    // The volatile Demultiplexer is a fanout point for promoted volatile enlistments.
    // When a transaction is promoted a single volatile enlistment is created in the new
    // transaction for all volitile enlistments on the transaction.  When the VolatileDemux
    // receives a preprepare it will fan that notification out to all of the enlistments
    // on the transaction.  When it has gathered all of the responses it will send a 
    // single vote back to the DistributedTransactionManager.
    internal abstract class VolatileDemultiplexer : IEnlistmentNotificationInternal
    {
        // Reference the transactions so that we have access to it's enlistments
        protected InternalTransaction transaction;

        // Store the IVolatileEnlistment interface to call back to the Distributed TM
        internal IPromotedEnlistment oletxEnlistment;
        internal IPromotedEnlistment preparingEnlistment;

        public VolatileDemultiplexer( InternalTransaction transaction )
        {
            this.transaction = transaction;
        }


        internal void BroadcastCommitted( ref VolatileEnlistmentSet volatiles )
        {
            // Broadcast preprepare to the volatile subordinates
            for ( int i = 0; i < volatiles.volatileEnlistmentCount; i++ )
            {
                volatiles.volatileEnlistments[i].twoPhaseState.InternalCommitted( 
                    volatiles.volatileEnlistments[i]);
            }
        }


        // This broadcast is used by the state machines and therefore must be internal.
        internal void BroadcastRollback( ref VolatileEnlistmentSet volatiles )
        {
            // Broadcast preprepare to the volatile subordinates
            for ( int i = 0; i < volatiles.volatileEnlistmentCount; i++ )
            {
                volatiles.volatileEnlistments[i].twoPhaseState.InternalAborted( 
                    volatiles.volatileEnlistments[i]);
            }
        }


        internal void BroadcastInDoubt( ref VolatileEnlistmentSet volatiles )
        {
            // Broadcast preprepare to the volatile subordinates
            for ( int i = 0; i < volatiles.volatileEnlistmentCount; i++ )
            {
                volatiles.volatileEnlistments[i].twoPhaseState.InternalIndoubt( 
                    volatiles.volatileEnlistments[i]);
            }
        }


        // Object for synchronizing access to the entire class( avoiding lock( typeof( ... )) )
        private static object classSyncObject;
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


        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile WaitCallback prepareCallback;
        private static WaitCallback PrepareCallback
        {
            get
            {
                if ( prepareCallback == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( prepareCallback == null )
                        {
                            WaitCallback temp = new WaitCallback( PoolablePrepare );
                            prepareCallback = temp;
                        }
                    }
                }

                return prepareCallback;
            }
        }
        
        protected static void PoolablePrepare( object state )
        {
            VolatileDemultiplexer demux = (VolatileDemultiplexer) state;

            // Don't block an enlistment thread (or a thread pool thread).  So
            // try to get the transaction lock but if unsuccessfull give up and
            // queue this operation to try again later.
            bool tookLock = false;
            try
            {
                System.Threading.Monitor.TryEnter(demux.transaction, 250, ref tookLock);
                if (tookLock)
                {
                    demux.InternalPrepare();
                }
                else
                {
                    if (!ThreadPool.QueueUserWorkItem(PrepareCallback, demux))
                    {
                        throw TransactionException.CreateInvalidOperationException(
                            SR.GetString(SR.TraceSourceLtm),
                            SR.GetString(SR.UnexpectedFailureOfThreadPool),
                            null
                            );
                    }
                }
            }
            finally
            {
                if (tookLock)
                {
                    System.Threading.Monitor.Exit(demux.transaction);
                }
            }
        }


        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile WaitCallback commitCallback;
        private static WaitCallback CommitCallback
        {
            get
            {
                if ( commitCallback == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( commitCallback == null )
                        {
                            WaitCallback temp = new WaitCallback( PoolableCommit );
                            commitCallback = temp;
                        }
                    }
                }

                return commitCallback;
            }
        }
            
        
        protected static void PoolableCommit( object state )
        {
            VolatileDemultiplexer demux = (VolatileDemultiplexer) state;

            // Don't block an enlistment thread (or a thread pool thread).  So
            // try to get the transaction lock but if unsuccessfull give up and
            // queue this operation to try again later.
            bool tookLock = false;
            try
            {
                System.Threading.Monitor.TryEnter(demux.transaction, 250, ref tookLock);
                if (tookLock)
                {
                    demux.InternalCommit();
                }
                else
                {
                    if (!ThreadPool.QueueUserWorkItem(CommitCallback, demux))
                    {
                        throw TransactionException.CreateInvalidOperationException(
                            SR.GetString(SR.TraceSourceLtm),
                            SR.GetString(SR.UnexpectedFailureOfThreadPool),
                            null
                            );
                    }
                }
            }
            finally
            {
                if (tookLock)
                {
                    System.Threading.Monitor.Exit(demux.transaction);
                }
            }
        }



        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile WaitCallback rollbackCallback;
        private static WaitCallback RollbackCallback
        {
            get
            {
                if ( rollbackCallback  == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if (rollbackCallback  == null )
                        {
                            WaitCallback temp = new WaitCallback( PoolableRollback );
                            rollbackCallback = temp;
                        }
                    }
                }

                return rollbackCallback;
            }
        }

        protected static void PoolableRollback( object state )
        {
            VolatileDemultiplexer demux = (VolatileDemultiplexer) state;

            // Don't block an enlistment thread (or a thread pool thread).  So
            // try to get the transaction lock but if unsuccessfull give up and
            // queue this operation to try again later.
            bool tookLock = false;
            try
            {
                System.Threading.Monitor.TryEnter(demux.transaction, 250, ref tookLock);
                if (tookLock)
                {
                    demux.InternalRollback();
                }
                else
                {
                    if (!ThreadPool.QueueUserWorkItem(RollbackCallback, demux))
                    {
                        throw TransactionException.CreateInvalidOperationException(
                            SR.GetString(SR.TraceSourceLtm),
                            SR.GetString(SR.UnexpectedFailureOfThreadPool),
                            null
                            );
                    }
                }
            }
            finally
            {
                if (tookLock)
                {
                    System.Threading.Monitor.Exit(demux.transaction);
                }
            }
        }


        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile WaitCallback inDoubtCallback;
        private static WaitCallback InDoubtCallback
        {
            get
            {
                if (inDoubtCallback  == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if (inDoubtCallback  == null )
                        {
                            WaitCallback temp = new WaitCallback( PoolableInDoubt );
                            inDoubtCallback = temp;
                        }
                    }
                }

                return inDoubtCallback;
            }
        }


        protected static void PoolableInDoubt( object state )
        {
            VolatileDemultiplexer demux = (VolatileDemultiplexer) state;

            // Don't block an enlistment thread (or a thread pool thread).  So
            // try to get the transaction lock but if unsuccessfull give up and
            // queue this operation to try again later.
            bool tookLock = false;
            try
            {
                System.Threading.Monitor.TryEnter(demux.transaction, 250, ref tookLock);
                if (tookLock)
                {
                    demux.InternalInDoubt();
                }
                else
                {
                    if (!ThreadPool.QueueUserWorkItem(InDoubtCallback, demux))
                    {
                        throw TransactionException.CreateInvalidOperationException(
                            SR.GetString(SR.TraceSourceLtm),
                            SR.GetString(SR.UnexpectedFailureOfThreadPool),
                            null
                            );
                    }
                }
            }
            finally
            {
                if (tookLock)
                {
                    System.Threading.Monitor.Exit(demux.transaction);
                }
            }
        }

        
        protected abstract void InternalPrepare();
        protected abstract void InternalCommit();
        protected abstract void InternalRollback();
        protected abstract void InternalInDoubt();

        #region IEnlistmentNotification Members

        // Fanout Preprepare notifications
        public abstract void Prepare( IPromotedEnlistment en );

        public abstract void Commit( IPromotedEnlistment en );

        public abstract void Rollback( IPromotedEnlistment en );

        public abstract void InDoubt( IPromotedEnlistment en );

        #endregion

    }


    // This class implements the phase 0 version of a volatile demux.
    internal class Phase0VolatileDemultiplexer : VolatileDemultiplexer
    {
        public Phase0VolatileDemultiplexer( InternalTransaction transaction ) : base( transaction ) { }

        protected override void InternalPrepare()
        {
            try
            {
                this.transaction.State.ChangeStatePromotedPhase0( this.transaction );
            }
            catch ( TransactionAbortedException e )
            {
                this.oletxEnlistment.ForceRollback(e);
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        e );
                }
            }
            catch ( TransactionInDoubtException e )
            {
                this.oletxEnlistment.EnlistmentDone();
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        e );
                }
            }
        }

        protected override void InternalCommit()
        {
            // Respond immediately to the TM
            this.oletxEnlistment.EnlistmentDone();

            this.transaction.State.ChangeStatePromotedCommitted( this.transaction );
        }

        protected override void InternalRollback()
        {
            // Respond immediately to the TM
            this.oletxEnlistment.EnlistmentDone();

            this.transaction.State.ChangeStatePromotedAborted( this.transaction );
        }

        protected override void InternalInDoubt()
        {
            this.transaction.State.InDoubtFromDtc( this.transaction );
        }

        #region IEnlistmentNotification Members

        // Fanout Preprepare notifications
        public override void Prepare( IPromotedEnlistment en )
        {
            this.preparingEnlistment = en;
            PoolablePrepare( this );
        }


        public override void Commit( IPromotedEnlistment en )
        {
            this.oletxEnlistment = en;
            PoolableCommit( this );
        }


        public override void Rollback( IPromotedEnlistment en )
        {
            this.oletxEnlistment = en;
            PoolableRollback( this );
        }


        public override void InDoubt( IPromotedEnlistment en )
        {
            this.oletxEnlistment = en;
            PoolableInDoubt( this );
        }

        #endregion
    }

    // This class implements the phase 1 version of a volatile demux.
    internal class Phase1VolatileDemultiplexer : VolatileDemultiplexer
    {
        public Phase1VolatileDemultiplexer( InternalTransaction transaction ) : base( transaction ) { }

        protected override void InternalPrepare()
        {
            try
            {
                this.transaction.State.ChangeStatePromotedPhase1( this.transaction );
            }
            catch ( TransactionAbortedException e )
            {
                this.oletxEnlistment.ForceRollback(e);
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        e );
                }
            }
            catch ( TransactionInDoubtException e )
            {
                this.oletxEnlistment.EnlistmentDone();
                if ( DiagnosticTrace.Verbose )
                {
                    ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        e );
                }
            }
        }


        protected override void InternalCommit()
        {
            // Respond immediately to the TM
            this.oletxEnlistment.EnlistmentDone();

            this.transaction.State.ChangeStatePromotedCommitted( this.transaction );
        }


        protected override void InternalRollback()
        {
            // Respond immediately to the TM
            this.oletxEnlistment.EnlistmentDone();

            this.transaction.State.ChangeStatePromotedAborted( this.transaction );
        }


        protected override void InternalInDoubt()
        {
            this.transaction.State.InDoubtFromDtc( this.transaction );
        }


        // Fanout Preprepare notifications
        public override void Prepare( IPromotedEnlistment en )
        {
            this.preparingEnlistment = en;
            PoolablePrepare( this );
        }


        public override void Commit( IPromotedEnlistment en )
        {
            this.oletxEnlistment = en;
            PoolableCommit( this );
        }


        public override void Rollback( IPromotedEnlistment en )
        {
            this.oletxEnlistment = en;
            PoolableRollback( this );
        }


        public override void InDoubt( IPromotedEnlistment en )
        {
            this.oletxEnlistment = en;
            PoolableInDoubt( this );
        }
    }



    internal struct VolatileEnlistmentSet
    {
        internal InternalEnlistment[] volatileEnlistments;
        internal int volatileEnlistmentCount;
        internal int volatileEnlistmentSize;
        internal int dependentClones;

        // Track the number of volatile enlistments that have prepared.
        internal int preparedVolatileEnlistments;

        // This is a single pinpoint enlistment to represent all volatile enlistments that
        // may exist on a promoted transaction.  This member should only be initialized if
        // a transaction is promoted.
        private VolatileDemultiplexer volatileDemux;
        internal VolatileDemultiplexer VolatileDemux
        {
            get
            {
                return this.volatileDemux;
            }
            set
            {
                Debug.Assert( this.volatileDemux == null, "volatileDemux can only be set once." );
                this.volatileDemux = value;
            }
        }
    }
}
