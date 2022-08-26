//-----------------------------------------------------------------------------
// <copyright file="TransactionInterop.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

using System;
using System.Configuration;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Transactions.Oletx;
using System.Transactions.Configuration;
using System.Transactions.Diagnostics;


namespace System.Transactions
{
    // This is here for the "DTC" in the name.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
    [ComImport,
    Guid("0fb15084-af41-11ce-bd2b-204c4f4f5020"),
    InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IDtcTransaction 
    {
        void Commit(int retaining,
            [MarshalAs(UnmanagedType.I4)] int commitType,
            int reserved);

        void Abort(IntPtr reason,
            int retaining,
            int async);

        void GetTransactionInfo( IntPtr transactionInformation);
    }

    
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
    public static class TransactionInterop
    {
        internal static OletxTransaction ConvertToOletxTransaction(
            Transaction transaction
            )
        {
            if ( null == transaction )
            {
                throw new ArgumentNullException( "transaction" );
            }

            if ( transaction.Disposed )
            {
                throw new ObjectDisposedException( "Transaction" );
            }

            if ( transaction.complete )
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), transaction.DistributedTxId);
            }

            OletxTransaction oletxTx = transaction.Promote();
            System.Diagnostics.Debug.Assert( oletxTx != null, "transaction.Promote returned null instead of throwing." );

            return oletxTx;
        }


        /// <summary>
        /// This is the PromoterType value that indicates that the transaction is promoting to MSDTC.
        /// 
        /// If using the variation of Transaction.EnlistPromotableSinglePhase that takes a PromoterType and the
        /// ITransactionPromoter being used promotes to MSDTC, then this is the value that should be 
        /// specified for the PromoterType parameter to EnlistPromotableSinglePhase.
        /// 
        /// If using the variation of Transaction.EnlistPromotableSinglePhase that assumes promotion to MSDTC and
        /// it that returns false, the caller can compare this value with Transaction.PromoterType to
        /// verify that the transaction promoted, or will promote, to MSDTC. If the Transaction.PromoterType
        /// matches this value, then the caller can continue with its enlistment with MSDTC. But if it
        /// does not match, the caller will not be able to enlist with MSDTC.
        /// </summary>
        public static readonly Guid PromoterTypeDtc = new Guid("14229753-FFE1-428D-82B7-DF73045CB8DA");

        // This is here for the DangerousGetHandle call.  We need to do it.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods")]
        public static byte[] GetExportCookie(
            Transaction transaction,
            byte[] whereabouts
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            byte[] cookie = null;

            if ( null == transaction )
            {
                throw new ArgumentNullException( "transaction" );
            }

            if ( null == whereabouts )
            {
                throw new ArgumentNullException( "whereabouts" );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetExportCookie"
                    );
            }

            // Copy the whereabouts so that it cannot be modified later.
            byte[] whereaboutsCopy = new byte[whereabouts.Length];
            Array.Copy(whereabouts, whereaboutsCopy, whereabouts.Length); 
            whereabouts = whereaboutsCopy;

            int cookieIndex = 0;
            UInt32 cookieSize = 0;
            CoTaskMemHandle cookieBuffer = null;

            // First, make sure we are working with an OletxTransaction.
            OletxTransaction oletxTx = TransactionInterop.ConvertToOletxTransaction( transaction );

            try
            {
                oletxTx.realOletxTransaction.TransactionShim.Export(
                    Convert.ToUInt32(whereabouts.Length),
                    whereabouts,
                    out cookieIndex,
                    out cookieSize,
                    out cookieBuffer );

                // allocate and fill in the cookie
                cookie = new byte[cookieSize];
                Marshal.Copy( cookieBuffer.DangerousGetHandle(), cookie, 0, Convert.ToInt32(cookieSize) );
            }
            catch (COMException comException)
            {
                OletxTransactionManager.ProxyException( comException );

                // We are unsure of what the exception may mean.  It is possible that 
                // we could get E_FAIL when trying to contact a transaction manager that is
                // being blocked by a fire wall.  On the other hand we may get a COMException
                // based on bad data.  The more common situation is that the data is fine
                // (since it is generated by Microsoft code) and the problem is with 
                // communication.  So in this case we default for unknown exceptions to
                // assume that the problem is with communication.
                throw TransactionManagerCommunicationException.Create( SR.GetString( SR.TraceSourceOletx ), comException );
            }
            finally
            {
                if ( null != cookieBuffer )
                {
                    cookieBuffer.Close();
                }
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetExportCookie"
                    );
            }
            
            return cookie;
        }

        public static Transaction GetTransactionFromExportCookie(
            byte[] cookie
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( null == cookie )
            {
                throw new ArgumentNullException( "cookie" );
            }

            if ( cookie.Length < 32 )
            {
                throw new ArgumentException( SR.GetString( SR.InvalidArgument ), "cookie" );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransactionFromExportCookie"
                    );
            }

            byte[] cookieCopy = new byte[cookie.Length];
            Array.Copy(cookie, cookieCopy, cookie.Length);
            cookie = cookieCopy;

            Transaction transaction = null;
            ITransactionShim transactionShim = null;
            Guid txIdentifier = Guid.Empty;
            OletxTransactionIsolationLevel oletxIsoLevel = OletxTransactionIsolationLevel.ISOLATIONLEVEL_SERIALIZABLE;
            OutcomeEnlistment outcomeEnlistment = null;
            OletxTransaction oleTx = null;

            // Extract the transaction guid from the propagation token to see if we already have a
            // transaction object for the transaction.
            byte[] guidByteArray = new byte[16];
            for (int i = 0; i < guidByteArray.Length; i++ )
            {
                // In a cookie, the transaction guid is preceeded by a signature guid.
                guidByteArray[i] = cookie[i + 16];
            }

            Guid txId = new Guid( guidByteArray );

            // First check to see if there is a promoted LTM transaction with the same ID.  If there
            // is, just return that.
            transaction = TransactionManager.FindPromotedTransaction( txId );
            if ( null != transaction )
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                        "TransactionInterop.GetTransactionFromExportCookie"
                        );
                }
                return transaction;
            }

            // We need to create a new transaction
            RealOletxTransaction realTx = null;
            OletxTransactionManager oletxTm = TransactionManager.DistributedTransactionManager;

            oletxTm.dtcTransactionManagerLock.AcquireReaderLock( -1 );
            try
            {
                outcomeEnlistment = new OutcomeEnlistment();
                IntPtr outcomeEnlistmentHandle = IntPtr.Zero;
                RuntimeHelpers.PrepareConstrainedRegions();
                try
                {
                    outcomeEnlistmentHandle = HandleTable.AllocHandle( outcomeEnlistment );
                    oletxTm.DtcTransactionManager.ProxyShimFactory.Import(
                        Convert.ToUInt32(cookie.Length),
                        cookie,
                        outcomeEnlistmentHandle,
                        out txIdentifier,
                        out oletxIsoLevel,
                        out transactionShim );
                }
                finally
                {
                    if ( transactionShim == null && outcomeEnlistmentHandle != IntPtr.Zero )
                    {
                        HandleTable.FreeHandle( outcomeEnlistmentHandle );
                    }
                }
            }
            catch ( COMException comException )
            {
                OletxTransactionManager.ProxyException( comException );

                // We are unsure of what the exception may mean.  It is possible that 
                // we could get E_FAIL when trying to contact a transaction manager that is
                // being blocked by a fire wall.  On the other hand we may get a COMException
                // based on bad data.  The more common situation is that the data is fine
                // (since it is generated by Microsoft code) and the problem is with 
                // communication.  So in this case we default for unknown exceptions to
                // assume that the problem is with communication.
                throw TransactionManagerCommunicationException.Create( SR.GetString( SR.TraceSourceOletx ), comException );
            }
            finally
            {
                oletxTm.dtcTransactionManagerLock.ReleaseReaderLock();
            }

            // We need to create a new RealOletxTransaction.
            realTx = new RealOletxTransaction(
                oletxTm,
                transactionShim,
                outcomeEnlistment,
                txIdentifier,
                oletxIsoLevel,
                false );

            // Now create the associated OletxTransaction.
            oleTx = new OletxTransaction( realTx );

            // If a transaction is found then FindOrCreate will Dispose the oletx
            // created.
            transaction = TransactionManager.FindOrCreatePromotedTransaction( txId, oleTx );

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransactionFromExportCookie"
                    );
            }

            return transaction;
        }


        public static byte[] GetTransmitterPropagationToken(
            Transaction transaction
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( null == transaction )
            {
                throw new ArgumentNullException( "transaction" );
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransmitterPropagationToken"
                    );
            }
            
            // First, make sure we are working with an OletxTransaction.
            OletxTransaction oletxTx = TransactionInterop.ConvertToOletxTransaction( transaction );

            byte[] token = GetTransmitterPropagationToken( oletxTx );

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransmitterPropagationToken"
                    );
            }

            return token;
        }


        // This is here for the DangerousGetHandle call.  We need to do it.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods")]
        internal static byte[] GetTransmitterPropagationToken(
            OletxTransaction oletxTx
            )
        {
            byte[] propagationToken = null;
            CoTaskMemHandle propagationTokenBuffer = null;
            UInt32 tokenSize = 0;

            try
            {
                oletxTx.realOletxTransaction.TransactionShim.GetPropagationToken(
                    out tokenSize,
                    out propagationTokenBuffer );
                propagationToken = new byte[tokenSize];
                Marshal.Copy( propagationTokenBuffer.DangerousGetHandle(), propagationToken, 0, Convert.ToInt32(tokenSize) );
            }
            catch (COMException comException)
            {
                OletxTransactionManager.ProxyException( comException );
                throw;
            }
            finally
            {
                if ( null != propagationTokenBuffer )
                {
                    propagationTokenBuffer.Close();
                }
            }

            return propagationToken;
        }



        public static Transaction GetTransactionFromTransmitterPropagationToken(
            byte[] propagationToken
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            Transaction returnValue = null;

            if ( null == propagationToken )
            {
                throw new ArgumentNullException( "propagationToken" );
            }

            if ( propagationToken.Length < 24 )
            {
                throw new ArgumentException( SR.GetString( SR.InvalidArgument ), "propagationToken" );
            }
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransactionFromTransmitterPropagationToken"
                    );
            }

            // Extract the transaction guid from the propagation token to see if we already have a
            // transaction object for the transaction.
            byte[] guidByteArray = new byte[16];
            for (int i = 0; i < guidByteArray.Length; i++ )
            {
                // In a propagation token, the transaction guid is preceeded by two version DWORDs.
                guidByteArray[i] = propagationToken[i + 8];
            }

            Guid txId = new Guid( guidByteArray );

            // First check to see if there is a promoted LTM transaction with the same ID.  If there
            // is, just return that.
            Transaction tx = TransactionManager.FindPromotedTransaction( txId );
            if ( null != tx )
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                        "TransactionInterop.GetTransactionFromTransmitterPropagationToken"
                        );
                }
                return tx;
            }

            OletxTransaction oleTx = TransactionInterop.GetOletxTransactionFromTransmitterPropigationToken( propagationToken );

            // If a transaction is found then FindOrCreate will Dispose the oletx
            // created.
            returnValue = TransactionManager.FindOrCreatePromotedTransaction( txId, oleTx );

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransactionFromTransmitterPropagationToken"
                    );
            }
            return returnValue;
        }



        internal static OletxTransaction GetOletxTransactionFromTransmitterPropigationToken(
            byte[] propagationToken
            )
        {
            Guid identifier;
            OletxTransactionIsolationLevel oletxIsoLevel;
            OutcomeEnlistment outcomeEnlistment;
            ITransactionShim transactionShim = null;

            if ( null == propagationToken )
            {
                throw new ArgumentNullException( "propagationToken" );
            }

            if ( propagationToken.Length < 24 )
            {
                throw new ArgumentException( SR.GetString( SR.InvalidArgument ), "propagationToken" );
            }

            byte[] propagationTokenCopy = new byte[propagationToken.Length];
            Array.Copy(propagationToken, propagationTokenCopy, propagationToken.Length);
            propagationToken = propagationTokenCopy;

            // First we need to create an OletxTransactionManager from Config.
            OletxTransactionManager oletxTm = TransactionManager.DistributedTransactionManager;

            oletxTm.dtcTransactionManagerLock.AcquireReaderLock( -1 );
            try
            {
                outcomeEnlistment = new OutcomeEnlistment();
                IntPtr outcomeEnlistmentHandle = IntPtr.Zero;
                RuntimeHelpers.PrepareConstrainedRegions();
                try
                {
                    outcomeEnlistmentHandle = HandleTable.AllocHandle( outcomeEnlistment );
                    oletxTm.DtcTransactionManager.ProxyShimFactory.ReceiveTransaction(
                        Convert.ToUInt32(propagationToken.Length),
                        propagationToken,
                        outcomeEnlistmentHandle,
                        out identifier,
                        out oletxIsoLevel,
                        out transactionShim
                        );
                }
                finally
                {
                    if ( transactionShim == null && outcomeEnlistmentHandle != IntPtr.Zero )
                    {
                        HandleTable.FreeHandle( outcomeEnlistmentHandle );
                    }
                }
            }
            catch ( COMException comException )
            {
                OletxTransactionManager.ProxyException( comException );

                // We are unsure of what the exception may mean.  It is possible that 
                // we could get E_FAIL when trying to contact a transaction manager that is
                // being blocked by a fire wall.  On the other hand we may get a COMException
                // based on bad data.  The more common situation is that the data is fine
                // (since it is generated by Microsoft code) and the problem is with 
                // communication.  So in this case we default for unknown exceptions to
                // assume that the problem is with communication.
                throw TransactionManagerCommunicationException.Create( SR.GetString( SR.TraceSourceOletx ), comException );
            }
            finally
            {
                oletxTm.dtcTransactionManagerLock.ReleaseReaderLock();
            }

            RealOletxTransaction realTx = null;

            realTx = new RealOletxTransaction(
                oletxTm,
                transactionShim,
                outcomeEnlistment,
                identifier,
                oletxIsoLevel,
                false );
            
            return new OletxTransaction( realTx );
        }


        // This is here for the "Dtc" in the name.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public static IDtcTransaction GetDtcTransaction(
            Transaction transaction
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( null == transaction )
            {
                throw new ArgumentNullException( "transaction" );
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetDtcTransaction"
                    );
            }
            
            IDtcTransaction transactionNative = null;

            // First, make sure we are working with an OletxTransaction.
            OletxTransaction oletxTx = TransactionInterop.ConvertToOletxTransaction( transaction );

            try
            {
                oletxTx.realOletxTransaction.TransactionShim.GetITransactionNative( out transactionNative );
            }
            catch ( COMException comException )
            {
                OletxTransactionManager.ProxyException( comException );
                throw;
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetDtcTransaction"
                    );
            }
            return transactionNative;
        }

        // This is here for the "DTC" in the name.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public static Transaction GetTransactionFromDtcTransaction(
            IDtcTransaction transactionNative
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            bool tooLate = false;
            ITransactionShim transactionShim = null;
            Guid txIdentifier = Guid.Empty;
            OletxTransactionIsolationLevel oletxIsoLevel = OletxTransactionIsolationLevel.ISOLATIONLEVEL_SERIALIZABLE;
            OutcomeEnlistment outcomeEnlistment = null;
            RealOletxTransaction realTx = null;
            OletxTransaction oleTx = null;

            if ( null == transactionNative )
            {
                throw new ArgumentNullException( "transactionNative" );
            }

            Transaction transaction = null;

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransactionFromDtc"
                    );
            }

            // Let's get the guid of the transaction from the proxy to see if we already
            // have an object.
            ITransactionNativeInternal myTransactionNative = transactionNative as ITransactionNativeInternal;
            if ( null == myTransactionNative )
            {
                throw new ArgumentException( SR.GetString( SR.InvalidArgument ), "transactionNative" );
            }

            OletxXactTransInfo xactInfo;
            try
            {
                myTransactionNative.GetTransactionInfo( out xactInfo );
            }
            catch ( COMException ex )
            {
                if ( Oletx.NativeMethods.XACT_E_NOTRANSACTION != ex.ErrorCode )
                {
                    throw;
                }

                // If we get here, the transaction has appraently already been committed or aborted.  Allow creation of the
                // OletxTransaction, but it will be marked with a status of InDoubt and attempts to get its Identifier
                // property will result in a TransactionException.
                tooLate = true;
                xactInfo.uow = Guid.Empty;

            }

            OletxTransactionManager oletxTm = TransactionManager.DistributedTransactionManager;
            if ( ! tooLate )
            {
                // First check to see if there is a promoted LTM transaction with the same ID.  If there
                // is, just return that.
                transaction = TransactionManager.FindPromotedTransaction( xactInfo.uow );
                if ( null != transaction )
                {
                    if ( DiagnosticTrace.Verbose )
                    {
                        MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                            "TransactionInterop.GetTransactionFromDtcTransaction"
                            );
                    }
                    return transaction;
                }

                // We need to create a new RealOletxTransaction...
                oletxTm.dtcTransactionManagerLock.AcquireReaderLock( -1 );
                try
                {
                    outcomeEnlistment = new OutcomeEnlistment();
                    IntPtr outcomeEnlistmentHandle = IntPtr.Zero;
                    RuntimeHelpers.PrepareConstrainedRegions();
                    try
                    {
                        outcomeEnlistmentHandle = HandleTable.AllocHandle( outcomeEnlistment );
                        oletxTm.DtcTransactionManager.ProxyShimFactory.CreateTransactionShim(
                            transactionNative,
                            outcomeEnlistmentHandle,
                            out txIdentifier,
                            out oletxIsoLevel,
                            out transactionShim );
                    }
                    finally
                    {
                        if ( transactionShim == null && outcomeEnlistmentHandle != IntPtr.Zero )
                        {
                            HandleTable.FreeHandle( outcomeEnlistmentHandle );
                        }
                    }
                }
                catch ( COMException comException )
                {
                    OletxTransactionManager.ProxyException( comException );
                    throw;
                }
                finally
                {
                    oletxTm.dtcTransactionManagerLock.ReleaseReaderLock();
                }

                // We need to create a new RealOletxTransaction.
                realTx = new RealOletxTransaction(
                    oletxTm,
                    transactionShim,
                    outcomeEnlistment,
                    txIdentifier,
                    oletxIsoLevel,
                    false );

                oleTx = new OletxTransaction( realTx );

                // If a transaction is found then FindOrCreate will Dispose the oletx
                // created.
                transaction = TransactionManager.FindOrCreatePromotedTransaction( xactInfo.uow, oleTx );
            }
            else
            {
                // It was too late to do a clone of the provided ITransactionNative, so we are just going to
                // create a RealOletxTransaction without a transaction shim or outcome enlistment.
                realTx = new RealOletxTransaction(
                    oletxTm,
                    null,
                    null,
                    txIdentifier,
                    OletxTransactionIsolationLevel.ISOLATIONLEVEL_SERIALIZABLE,
                    false );

                oleTx = new OletxTransaction( realTx );
                transaction = new Transaction( oleTx );
                TransactionManager.FireDistributedTransactionStarted( transaction );
                oleTx.savedLtmPromotedTransaction = transaction;

                InternalTransaction.DistributedTransactionOutcome(transaction.internalTransaction, TransactionStatus.InDoubt);
            }


            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetTransactionFromDtc"
                    );
            }
            return transaction;
        }

        public static byte[] GetWhereabouts( 
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            byte[] returnValue = null;

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetWhereabouts"
                    );
            }

            OletxTransactionManager oletxTm = TransactionManager.DistributedTransactionManager;
            if ( null == oletxTm )
            {
                throw new ArgumentException( SR.GetString( SR.ArgumentWrongType ), "transactionManager" );
            }

            oletxTm.dtcTransactionManagerLock.AcquireReaderLock( -1 );
            try
            {
                returnValue = oletxTm.DtcTransactionManager.Whereabouts;
            }
            finally
            {
                oletxTm.dtcTransactionManagerLock.ReleaseReaderLock();
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceOletx ),
                    "TransactionInterop.GetWhereabouts"
                    );
            }
            return returnValue;

        }
    }
}

