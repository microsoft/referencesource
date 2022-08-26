//-----------------------------------------------------------------------------
// <copyright file="TransactionInformation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Transactions.Diagnostics;

    public class TransactionInformation
    {
        private InternalTransaction internalTransaction;
        
        internal TransactionInformation( InternalTransaction internalTransaction )
        {
            this.internalTransaction = internalTransaction;
        }


        public string LocalIdentifier
        {
            get
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        "TransactionInformation.get_LocalIdentifier"
                        );
                }

                try
                {
                    return this.internalTransaction.TransactionTraceId.TransactionIdentifier;
                }
                finally
                {
                    if ( DiagnosticTrace.Verbose )
                    {
                        MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                            "TransactionInformation.get_LocalIdentifier"
                            );
                    }
                }
            }
        }


        public Guid DistributedIdentifier 
        { 
            get 
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        "TransactionInformation.get_DistributedIdentifier"
                        );
                }

                try
                {
                    // syncronize to avoid potential ---- between accessing the DistributerIdentifier
                    // and getting the transaction information entry populated...
                    
                    lock (this.internalTransaction)
                    {
                        return this.internalTransaction.State.get_Identifier( this.internalTransaction );
                    }
                }
                finally
                {
                    if ( DiagnosticTrace.Verbose )
                    {
                        MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                            "TransactionInformation.get_DistributedIdentifier"
                            );
                    }
                }

            }
        }


        public DateTime CreationTime
        {
            get 
            {
                return new DateTime( this.internalTransaction.CreationTime );
            }
        }


        public TransactionStatus Status
        {
            get 
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                        "TransactionInformation.get_Status"
                        );
                }

                try
                {
                    return this.internalTransaction.State.get_Status( this.internalTransaction );
                }
                finally
                {
                    if ( DiagnosticTrace.Verbose )
                    {
                        MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceLtm ),
                            "TransactionInformation.get_Status"
                            );
                    }
                }
            }
        }
    }
}


