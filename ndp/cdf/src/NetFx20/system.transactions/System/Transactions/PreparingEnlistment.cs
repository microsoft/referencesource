//-----------------------------------------------------------------------------
// <copyright file="PreparingEnlistment.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

#define DEMAND_TX_PERM

namespace System.Transactions
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using System.Transactions;
    using System.Transactions.Diagnostics;

    public class PreparingEnlistment : Enlistment
    {
        internal PreparingEnlistment(
            InternalEnlistment enlistment
            ) : base(enlistment)
        {
        }


        public void Prepared()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.Prepared"
                    );
                EnlistmentCallbackPositiveTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.Prepared
                    );
            }

            lock (this.internalEnlistment.SyncRoot)
            {
                this.internalEnlistment.State.Prepared(this.internalEnlistment);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.Prepared"
                    );
            }
        }


        public void ForceRollback()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.ForceRollback"
                    );
            }

            if (DiagnosticTrace.Warning)
            {
                EnlistmentCallbackNegativeTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.ForceRollback
                    );
            }

            lock (this.internalEnlistment.SyncRoot)
            {
                this.internalEnlistment.State.ForceRollback(this.internalEnlistment, null);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.ForceRollback"
                    );
            }
        }


        // Changing the e paramater name would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public void ForceRollback(Exception e)
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.ForceRollback"
                    );
            }

            if (DiagnosticTrace.Warning)
            {
                EnlistmentCallbackNegativeTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.internalEnlistment.EnlistmentTraceId,
                    EnlistmentCallback.ForceRollback
                    );
            }

            lock (this.internalEnlistment.SyncRoot)
            {
                this.internalEnlistment.State.ForceRollback(this.internalEnlistment, e);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.ForceRollback"
                    );
            }
        }


        public byte[] RecoveryInformation()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "PreparingEnlistment.RecoveryInformation"
                    );
            }

            try
            {
                lock (this.internalEnlistment.SyncRoot)
                {
                    return this.internalEnlistment.State.RecoveryInformation(this.internalEnlistment);
                }
            }
            finally
            {
                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "PreparingEnlistment.RecoveryInformation"
                        );
                }
            }
        }
    }
}


