//-----------------------------------------------------------------------------
// <copyright file="SinglePhaseEnlistment.cs" company="Microsoft">
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

    public class SinglePhaseEnlistment : Enlistment
    {
        internal SinglePhaseEnlistment( 
            InternalEnlistment enlistment
            ) : base(enlistment)
        {
        }

        public void Aborted()
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.Aborted"
                    );
            }

            if ( DiagnosticTrace.Warning )
            {
                EnlistmentCallbackNegativeTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.Aborted
                    );
            }

            lock ( this.internalEnlistment.SyncRoot )
            {
                this.internalEnlistment.State.Aborted( this.internalEnlistment, null );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.Aborted"
                    );
            }
        }


        // Changing the e paramater name would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public void Aborted( Exception e )
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.Aborted"
                    );
            }

            if ( DiagnosticTrace.Warning )
            {
                EnlistmentCallbackNegativeTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.Aborted
                    );
            }

            lock ( this.internalEnlistment.SyncRoot )
            {
                this.internalEnlistment.State.Aborted( this.internalEnlistment, e );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.Aborted"
                    );
            }
        }


        public void Committed()
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.Committed"
                    );
                EnlistmentCallbackPositiveTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.Committed
                    );
            }

            lock ( this.internalEnlistment.SyncRoot )
            {
                this.internalEnlistment.State.Committed( this.internalEnlistment );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.Committed"
                    );
            }
        }


        public void InDoubt()
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.InDoubt"
                    );
            }


            lock ( this.internalEnlistment.SyncRoot )
            {
                if ( DiagnosticTrace.Warning )
                {
                    EnlistmentCallbackNegativeTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        this.internalEnlistment.EnlistmentTraceId,
                        EnlistmentCallback.InDoubt
                        );
                }

                this.internalEnlistment.State.InDoubt( this.internalEnlistment, null );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.InDoubt"
                    );
            }
        }


        // Changing the e paramater name would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public void InDoubt( Exception e )
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.InDoubt"
                    );
            }


            lock ( this.internalEnlistment.SyncRoot )
            {
                if ( DiagnosticTrace.Warning )
                {
                    EnlistmentCallbackNegativeTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        this.internalEnlistment.EnlistmentTraceId,
                        EnlistmentCallback.InDoubt
                        );
                }

                this.internalEnlistment.State.InDoubt( this.internalEnlistment, e );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                    "SinglePhaseEnlistment.InDoubt"
                    );
            }
        }
    }
}


