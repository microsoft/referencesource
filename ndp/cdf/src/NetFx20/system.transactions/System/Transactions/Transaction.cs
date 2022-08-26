//-----------------------------------------------------------------------------
// <copyright file="Transaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

#define USE_ISINTRANSACTION

namespace System.Transactions
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using SysES = System.EnterpriseServices;
    using System.Runtime;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Runtime.Remoting.Lifetime;
    using System.Runtime.Remoting.Messaging;
    using System.Security.Permissions;
    using System.Threading;
    using System.Transactions.Diagnostics;

    using System.Transactions.Configuration;


    internal enum EnterpriseServicesState
    {
        Unknown = 0,
        Available = -1,
        Unavailable = 1
    }


    public class TransactionEventArgs : EventArgs
    {
        internal Transaction transaction;
        public Transaction Transaction 
        { 
            get 
            {
                return this.transaction;
            }
        }
    }

    public delegate void TransactionCompletedEventHandler(object sender, TransactionEventArgs e);


    public enum IsolationLevel
    {
        Serializable = 0,
        RepeatableRead = 1,
        ReadCommitted = 2,
        ReadUncommitted = 3,
        Snapshot = 4,
        Chaos = 5,
        Unspecified = 6,
    }


    public enum TransactionStatus
    {
        Active = 0,
        Committed = 1,
        Aborted = 2,
        InDoubt = 3
    }


    public enum DependentCloneOption
    {
        BlockCommitUntilComplete = 0,
        RollbackIfNotComplete = 1,
    }


    [Flags]
    public enum EnlistmentOptions
    {
        None = 0x0,
        EnlistDuringPrepareRequired = 0x1,
    }


    // When we serialize a Transaction, we specify the type OletxTransaction, so a Transaction never
    // actually gets deserialized.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2229:ImplementSerializationConstructors")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2240:ImplementISerializableCorrectly")]
    [Serializable]
    public class Transaction : IDisposable, ISerializable
    {
        private static EnterpriseServicesState _enterpriseServicesOk = EnterpriseServicesState.Unknown;
        internal static bool EnterpriseServicesOk
        {
            get
            {
                if (_enterpriseServicesOk == EnterpriseServicesState.Unknown)
                {
                    if (null != Type.GetType("System.EnterpriseServices.ContextUtil, " + AssemblyRef.SystemEnterpriseServices, false))
                    {
                        _enterpriseServicesOk = EnterpriseServicesState.Available;
                    }
                    else
                    {
                        _enterpriseServicesOk = EnterpriseServicesState.Unavailable;
                    }
                }
                return (_enterpriseServicesOk == EnterpriseServicesState.Available);
            }
        }


        internal static void VerifyEnterpriseServicesOk()
        {
            if (!EnterpriseServicesOk)
            {
                throw new NotSupportedException(SR.GetString(SR.EsNotSupported));
            }
        }

        private static Guid IID_IObjContext = new Guid("000001c6-0000-0000-C000-000000000046");

        [System.Runtime.CompilerServices.MethodImplAttribute(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)] 
        // Get Transaction is a non destructive action.  The transaction that is returned is given back to the 
        // calling code but that is precicely what System.Transactions is supposed to do.  The threat model 
        // for System.Transactions notes that you should be careful about the code you call when you have a
        // transaction in Current.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods")]
        // The handle returned here by DangerousGetHandle is not stored anywhere so this is not a reliability issue.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands")]
        private static Transaction JitSafeGetContextTransaction(ContextData contextData)
        {
            // Attempt to see if we are in the default context in which case we don't need to 
            // call SystemTransaction at all.
            SafeIUnknown defaultContext = null;

            if (contextData.WeakDefaultComContext != null)
            {
                defaultContext = (SafeIUnknown)contextData.WeakDefaultComContext.Target;
            }
            
            if (contextData.DefaultComContextState == DefaultComContextState.Unknown ||
                (contextData.DefaultComContextState == DefaultComContextState.Available &&
                 defaultContext == null)
              )
            {
                try
                {
                    NativeMethods.CoGetDefaultContext(-1, ref IID_IObjContext, out defaultContext);
                    contextData.WeakDefaultComContext = new WeakReference(defaultContext);
                    contextData.DefaultComContextState = DefaultComContextState.Available;
                }
                catch (System.EntryPointNotFoundException e)
                {
                    if (DiagnosticTrace.Verbose)
                    {
                        ExceptionConsumedTraceRecord.Trace(SR.GetString(SR.TraceSourceBase),
                            e);
                    }
                    contextData.DefaultComContextState = DefaultComContextState.Unavailable;
                }
            }

            if (contextData.DefaultComContextState == DefaultComContextState.Available)
            {
                IntPtr contextToken = IntPtr.Zero;
                NativeMethods.CoGetContextToken(out contextToken);

                // Check to see if the context token is the default context.
                if (defaultContext.DangerousGetHandle() == contextToken)
                {
                    return null;
                }
            }
            

#if USE_ISINTRANSACTION
            if (!SysES.ContextUtil.IsInTransaction)
            {
                return null;
            }
#endif

            return (Transaction)SysES.ContextUtil.SystemTransaction;
        }

        
        // GetContextTransaction
        //
        // Get a transaction from Com+ through EnterpriseServices
        internal static Transaction GetContextTransaction(ContextData contextData)
        {
            if (EnterpriseServicesOk)
            {
                return JitSafeGetContextTransaction(contextData);
            }
            return null;
        }


        // UseServiceDomain
        //
        // Property tells parts of system.transactions if it should use a
        // service domain for current.
        [System.Runtime.CompilerServices.MethodImplAttribute(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)] 
        // IsDefaultContext is a non destructive call and the information it provides is not exposed directly
        // to code that is calling Transaction.Current.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods")]
        internal static bool UseServiceDomainForCurrent()
        {
            return !SysES.ContextUtil.IsDefaultContext();
        }


        // InteropMode
        //
        // This property figures out the current interop mode based on the
        // top of the transaction scope stack as well as the default mode
        // from config.
        internal static EnterpriseServicesInteropOption InteropMode(TransactionScope currentScope)
        {
            if (currentScope != null)
            {
                return currentScope.InteropMode;
            }

            return EnterpriseServicesInteropOption.None;
        }


        internal static Transaction FastGetTransaction(TransactionScope currentScope, ContextData contextData, out Transaction contextTransaction)
        {
            Transaction current = null;
            contextTransaction = null;
            
            contextTransaction = contextData.CurrentTransaction;

            switch (InteropMode(currentScope))
            {
                case EnterpriseServicesInteropOption.None:

                    current = contextTransaction;

                    // If there is a transaction in the execution context or if there is a current transaction scope
                    // then honer the transaction context.
                    if (current == null && currentScope == null)
                    {
                        // Otherwise check for an external current.
                        if (TransactionManager.currentDelegateSet)
                        {
                            current = TransactionManager.currentDelegate();
                        }
                        else
                        {
                            current = GetContextTransaction(contextData);
                        }
                    }
                    break;

                case EnterpriseServicesInteropOption.Full:
                    current = GetContextTransaction(contextData);
                    break;

                case EnterpriseServicesInteropOption.Automatic:

                    if (UseServiceDomainForCurrent())
                    {
                        current = GetContextTransaction(contextData);
                    }
                    else
                    {
                        current = contextData.CurrentTransaction;
                    }

                    break;
            }

            return current;
        }


        // GetCurrentTransactionAndScope
        //
        // Returns both the current transaction and scope.  This is implemented for optimizations
        // in TransactionScope because it is required to get both of them in several cases.
        internal static void GetCurrentTransactionAndScope(
            TxLookup defaultLookup,
            out Transaction current, 
            out TransactionScope currentScope, 
            out Transaction contextTransaction
            )
        {
            current = null;
            currentScope = null;
            contextTransaction = null;
            
            ContextData contextData = ContextData.LookupContextData(defaultLookup);
            if (contextData != null)
            {
                currentScope = contextData.CurrentScope;
                current = FastGetTransaction(currentScope, contextData, out contextTransaction);
            }
        }

        public static Transaction Current
        {
            get
            { 
                if (DiagnosticTrace.Verbose)
                {
                    MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceBase),
                        "Transaction.get_Current"
                        );
                }

                Transaction current = null;                
                TransactionScope currentScope = null;
                Transaction contextValue = null;
                GetCurrentTransactionAndScope(TxLookup.Default, out current, out currentScope, out contextValue);

                if (currentScope != null)
                {
                    if (currentScope.ScopeComplete)
                    {
                        throw new InvalidOperationException(SR.GetString(SR.TransactionScopeComplete));
                    }
                }

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceBase),
                        "Transaction.get_Current"
                        );
                }

                return current;
            }

            set
            {
                if (!TransactionManager._platformValidated) TransactionManager.ValidatePlatform();

                if (DiagnosticTrace.Verbose)
                {
                    MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceBase),
                        "Transaction.set_Current"
                        );
                }

                // Bring your own Transaction(BYOT) is supported only for legacy scenarios. 
                // This transaction won't be flown across thread continuations.
                if (InteropMode(ContextData.TLSCurrentData.CurrentScope) != EnterpriseServicesInteropOption.None)
                {
                    if (DiagnosticTrace.Error)
                    {
                        InvalidOperationExceptionTraceRecord.Trace(SR.GetString(SR.TraceSourceBase),
                            SR.GetString(SR.CannotSetCurrent)
                            );
                    }

                    throw new InvalidOperationException(SR.GetString(SR.CannotSetCurrent));
                }

                // Support only legacy scenarios using TLS. 
                ContextData.TLSCurrentData.CurrentTransaction = value;
                // Clear CallContext data.
                CallContextCurrentData.ClearCurrentData(null, false);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceBase),
                        "Transaction.set_Current"
                        );
                }
            }
        }

        // Storage for the transaction isolation level
        internal IsolationLevel isoLevel;

        // Storage for the consistent flag
        internal bool complete = false;

        // Record an identifier for this clone
        internal int cloneId; 

        // Storage for a disposed flag
        internal const int disposedTrueValue = 1;
        internal int disposed = 0;
        internal bool Disposed { get { return this.disposed == Transaction.disposedTrueValue; } }

        internal Guid DistributedTxId
        {
            get
            {
                Guid returnValue = Guid.Empty;

                if (this.internalTransaction != null)
                {
                    returnValue = this.internalTransaction.DistributedTxId;
                }
                return returnValue;
            }
        }

        // Internal synchronization object for transactions.  It is not safe to lock on the 
        // transaction object because it is public and users of the object may lock it for 
        // other purposes.
        internal InternalTransaction internalTransaction;

        // The TransactionTraceIdentifier for the transaction instance.
        internal TransactionTraceIdentifier traceIdentifier;

        // Not used by anyone
        private Transaction() { }

        // Create a transaction with the given settings
        //
        internal Transaction(
            IsolationLevel isoLevel,
            InternalTransaction internalTransaction
            )
        {
            TransactionManager.ValidateIsolationLevel(isoLevel);

            this.isoLevel = isoLevel;

            // Never create a transaction with an IsolationLevel of Unspecified.
            if (IsolationLevel.Unspecified == this.isoLevel)
            {
                this.isoLevel = System.Transactions.TransactionManager.DefaultIsolationLevel;
            }

            if (internalTransaction != null)
            {
                this.internalTransaction = internalTransaction;
                this.cloneId = Interlocked.Increment(ref this.internalTransaction.cloneCount);
            }
            else
            {
                // Null is passed from the constructor of a CommittableTransaction.  That
                // constructor will fill in the traceIdentifier because it has allocated the
                // internal transaction.
            }
        }


        internal Transaction(
            Oletx.OletxTransaction oleTransaction
            )
        {
            this.isoLevel = oleTransaction.IsolationLevel;
            this.internalTransaction = new InternalTransaction(this, oleTransaction);
            this.cloneId = Interlocked.Increment(ref this.internalTransaction.cloneCount);
        }


        internal Transaction(
            IsolationLevel isoLevel,
            ISimpleTransactionSuperior superior
            )
        {
            TransactionManager.ValidateIsolationLevel(isoLevel);

            if (superior == null)
            {
                throw new ArgumentNullException("superior");
            }

            this.isoLevel = isoLevel;

            // Never create a transaction with an IsolationLevel of Unspecified.
            if (IsolationLevel.Unspecified == this.isoLevel)
            {
                this.isoLevel = System.Transactions.TransactionManager.DefaultIsolationLevel;
            }

            this.internalTransaction = new InternalTransaction(this, superior);
            // ISimpleTransactionSuperior is defined to also promote to MSDTC.
            this.internalTransaction.SetPromoterTypeToMSDTC();
            this.cloneId = 1;
        }

        #region System.Object Overrides

        // Don't use the identifier for the hash code.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return this.internalTransaction.TransactionHash;
        }


        // Don't allow equals to get the identifier
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.Equals"]/*' />
        public override bool Equals(object obj)
        {
            Transaction transaction = obj as Transaction;

            // If we can't cast the object as a Transaction, it must not be equal
            // to this, which is a Transaction.
            if (null == transaction)
            {
                return false;
            }

            // Check the internal transaction object for equality.
            return this.internalTransaction.TransactionHash == transaction.internalTransaction.TransactionHash;
        }

        // This would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public static bool operator ==(Transaction x, Transaction y) 
        {
            if (((object)x) != null)
            {
                return x.Equals(y);
            }
            return ((object)y) == null;                
        }

        // This would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public static bool operator !=(Transaction x, Transaction y) 
        {
            if (((object)x) != null)
            {
                return !x.Equals(y);
            }
            return ((object)y) != null;
        }


        #endregion

        public TransactionInformation TransactionInformation
        {
            get
            {
                if (DiagnosticTrace.Verbose)
                {
                    MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.get_TransactionInformation"
                        );
                }

                if (Disposed)
                {
                    throw new ObjectDisposedException("Transaction");
                }

                TransactionInformation txInfo = this.internalTransaction.transactionInformation;
                if (txInfo == null)
                {
                    // A ---- would only result in an extra allocation
                    txInfo = new TransactionInformation(this.internalTransaction);
                    this.internalTransaction.transactionInformation = txInfo;
                }

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.get_TransactionInformation"
                        );
                }

                return txInfo;
            }
        }


        // Return the Isolation Level for the given transaction
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.IsolationLevel"]/*' />
        public IsolationLevel IsolationLevel
        {
            get
            {
                if (DiagnosticTrace.Verbose)
                {
                    MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.get_IsolationLevel"
                        );
                }
                if (Disposed)
                {
                    throw new ObjectDisposedException("Transaction");
                }

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.get_IsolationLevel"
                        );
                }
                return this.isoLevel;
            }
        }

        /// <summary>
        /// Gets the PromoterType value for the transaction.
        /// </summary>
        /// <value>
        /// If the transaction has not yet been promoted and does not yet have a promotable single phase enlistment,
        /// this property value will be Guid.Empty.
        /// 
        /// If the transaction has been promoted or has a promotable single phase enlistment, this will return the
        /// promoter type specified by the transaction promoter.
        /// 
        /// If the transaction is, or will be, promoted to MSDTC, the value will be TransactionInterop.PromoterTypeDtc.
        /// </value>
        public Guid PromoterType
        {
            get
            {
                if (DiagnosticTrace.Verbose)
                {
                    MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.get_PromoterType"
                        );
                }

                if (Disposed)
                {
                    throw new ObjectDisposedException("Transaction");
                }

                lock (this.internalTransaction)
                {
                    return this.internalTransaction.promoterType;
                }
            }
        }

        /// <summary>
        /// Gets the PromotedToken for the transaction.
        /// 
        /// If the transaction has not already been promoted, retrieving this value will cause promotion. Before retrieving the
        /// PromotedToken, the Transaction.PromoterType value should be checked to see if it is a promoter type (Guid) that the
        /// caller understands. If the caller does not recognize the PromoterType value, retreiving the PromotedToken doesn't
        /// have much value because the caller doesn't know how to utilize it. But if the PromoterType is recognized, the
        /// caller should know how to utilize the PromotedToken to communicate with the promoting distributed transaction
        /// coordinator to enlist on the distributed transaction.
        /// 
        /// If the value of a transaction's PromoterType is TransactionInterop.PromoterTypeDtc, then that transaction's
        /// PromotedToken will be an MSDTC-based TransmitterPropagationToken.
        /// </summary>
        /// <returns> 
        /// The byte[] that can be used to enlist with the distributed transaction coordinator used to promote the transaction.
        /// The format of the byte[] depends upon the value of Transaction.PromoterType.
        /// </returns>
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public byte[] GetPromotedToken()
        {
            // We need to ask the current transaction state for the PromotedToken because depending on the state
            // we may need to induce a promotion.
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.GetPromotedToken"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            // We always make a copy of the promotedToken stored in the internal transaction.
            byte[] internalPromotedToken;
            lock (this.internalTransaction)
            {
                internalPromotedToken = this.internalTransaction.State.PromotedToken(this.internalTransaction);
            }

            byte[] toReturn = new byte[internalPromotedToken.Length];
            Array.Copy(internalPromotedToken, toReturn, toReturn.Length);
            return toReturn;
        }

        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public Enlistment EnlistDurable(
            Guid resourceManagerIdentifier, 
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.EnlistDurable( IEnlistmentNotification )"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (resourceManagerIdentifier == Guid.Empty)
            {
                throw new ArgumentException(SR.GetString(SR.BadResourceManagerId), "resourceManagerIdentifier");
            }

            if (enlistmentNotification == null)
            {
                throw new ArgumentNullException("enlistmentNotification");
            }

            if (enlistmentOptions != EnlistmentOptions.None && enlistmentOptions != EnlistmentOptions.EnlistDuringPrepareRequired)
            {
                throw new ArgumentOutOfRangeException("enlistmentOptions");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                Enlistment enlistment = this.internalTransaction.State.EnlistDurable(this.internalTransaction, 
                    resourceManagerIdentifier, enlistmentNotification, enlistmentOptions, this);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.EnlistDurable( IEnlistmentNotification )"
                        );
                }
                return enlistment;
            }
        }


        // Forward request to the state machine to take the appropriate action.
        //
        /// <include file='doc\Transaction' path='docs/doc[@for="Transaction.EnlistDurable"]/*' />
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public Enlistment EnlistDurable(
            Guid resourceManagerIdentifier, 
            ISinglePhaseNotification singlePhaseNotification,
            EnlistmentOptions enlistmentOptions
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.EnlistDurable( ISinglePhaseNotification )"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (resourceManagerIdentifier == Guid.Empty)
            {
                throw new ArgumentException(SR.GetString(SR.BadResourceManagerId), "resourceManagerIdentifier");
            }

            if (singlePhaseNotification == null)
            {
                throw new ArgumentNullException("singlePhaseNotification");
            }

            if (enlistmentOptions != EnlistmentOptions.None && enlistmentOptions != EnlistmentOptions.EnlistDuringPrepareRequired)
            {
                throw new ArgumentOutOfRangeException("enlistmentOptions");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                Enlistment enlistment = this.internalTransaction.State.EnlistDurable(this.internalTransaction,
                    resourceManagerIdentifier, singlePhaseNotification, enlistmentOptions, this);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.EnlistDurable( ISinglePhaseNotification )"
                        );
                }
                return enlistment;
            }
        }


        public void Rollback()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.Rollback"
                    );
            }

            if (DiagnosticTrace.Warning)
            {
                TransactionRollbackCalledTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            lock (this.internalTransaction)
            {
                Debug.Assert(this.internalTransaction.State != null);
                this.internalTransaction.State.Rollback(this.internalTransaction, null);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.Rollback"
                    );
            }
        }


        // Changing the e paramater name would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public void Rollback(Exception e)
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.Rollback"
                    );
            }

            if (DiagnosticTrace.Warning)
            {
                TransactionRollbackCalledTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            lock (this.internalTransaction)
            {
                Debug.Assert(this.internalTransaction.State != null);
                this.internalTransaction.State.Rollback(this.internalTransaction, e);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.Rollback"
                    );
            }
        }


        // Forward request to the state machine to take the appropriate action.
        //
        /// <include file='doc\Transaction' path='docs/doc[@for="Transaction.EnlistVolatile"]/*' />
        public Enlistment EnlistVolatile(
            IEnlistmentNotification enlistmentNotification,
            EnlistmentOptions enlistmentOptions
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.EnlistVolatile( IEnlistmentNotification )"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (enlistmentNotification == null)
            {
                throw new ArgumentNullException("enlistmentNotification");
            }

            if (enlistmentOptions != EnlistmentOptions.None && enlistmentOptions != EnlistmentOptions.EnlistDuringPrepareRequired)
            {
                throw new ArgumentOutOfRangeException("enlistmentOptions");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                Enlistment enlistment = this.internalTransaction.State.EnlistVolatile(this.internalTransaction,
                    enlistmentNotification, enlistmentOptions, this);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.EnlistVolatile( IEnlistmentNotification )"
                        );
                }
                return enlistment;
            }
        }


        // Forward request to the state machine to take the appropriate action.
        //
        /// <include file='doc\Transaction' path='docs/doc[@for="Transaction.EnlistVolatile"]/*' />
        public Enlistment EnlistVolatile(
            ISinglePhaseNotification singlePhaseNotification,
            EnlistmentOptions enlistmentOptions
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.EnlistVolatile( ISinglePhaseNotification )"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (singlePhaseNotification == null)
            {
                throw new ArgumentNullException("singlePhaseNotification");
            }

            if (enlistmentOptions != EnlistmentOptions.None && enlistmentOptions != EnlistmentOptions.EnlistDuringPrepareRequired)
            {
                throw new ArgumentOutOfRangeException("enlistmentOptions");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                Enlistment enlistment = this.internalTransaction.State.EnlistVolatile(this.internalTransaction,
                    singlePhaseNotification, enlistmentOptions, this);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.EnlistVolatile( ISinglePhaseNotification )"
                        );
                }
                return enlistment;
            }
        }



        // Create a clone of the transaction that forwards requests to this object.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.Clone"]/*' />
        public Transaction Clone()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.Clone"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }
            
            Transaction clone = InternalClone();

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.Clone"
                    );
            }
            return clone;
        }


        internal Transaction InternalClone()
        {
            Transaction clone = new Transaction(this.isoLevel,
                this.internalTransaction);

            if (DiagnosticTrace.Verbose)
            {
                CloneCreatedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    clone.TransactionTraceId
                    );
            }

            return clone;
        }


        // Create a dependent clone of the transaction that forwards requests to this object.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.Clone"]/*' />
        public DependentTransaction DependentClone(
            DependentCloneOption cloneOption
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.DependentClone"
                    );
            }

            if (cloneOption != DependentCloneOption.BlockCommitUntilComplete
                && cloneOption != DependentCloneOption.RollbackIfNotComplete)
            {
                throw new ArgumentOutOfRangeException("cloneOption");
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            DependentTransaction clone = new DependentTransaction(
                this.isoLevel, this.internalTransaction, cloneOption == DependentCloneOption.BlockCommitUntilComplete);

            if (DiagnosticTrace.Information)
            {
                DependentCloneCreatedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    clone.TransactionTraceId,
                    cloneOption
                    );
            }
            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.DependentClone"
                    );
            }
            return clone;
        }


        internal TransactionTraceIdentifier TransactionTraceId
        {
            get
            {
                if (this.traceIdentifier == TransactionTraceIdentifier.Empty)
                {
                    lock (this.internalTransaction)
                    {
                        if (this.traceIdentifier == TransactionTraceIdentifier.Empty)
                        {
                            TransactionTraceIdentifier temp = new TransactionTraceIdentifier(
                                this.internalTransaction.TransactionTraceId.TransactionIdentifier,
                                this.cloneId);
                            Thread.MemoryBarrier();
                            this.traceIdentifier = temp;
                        }
                    }
                }
                return this.traceIdentifier;
            }
        }


        // Forward request to the state machine to take the appropriate action.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.TransactionCompleted"]/*' />
        public event TransactionCompletedEventHandler TransactionCompleted
        {
            add
            {
                if (Disposed)
                {
                    throw new ObjectDisposedException("Transaction");
                }

                lock (this.internalTransaction)
                {
                    // Register for completion with the inner transaction
                    this.internalTransaction.State.AddOutcomeRegistrant(this.internalTransaction, value);
                }
            }

            remove
            {
                lock (this.internalTransaction)
                {
                    this.internalTransaction.transactionCompletedDelegate = (TransactionCompletedEventHandler)
                        System.Delegate.Remove(this.internalTransaction.transactionCompletedDelegate, value);
                }
            }
        }


        public void Dispose()
        {
            InternalDispose();
        }


        // Handle Transaction Disposal.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.Dispose"]/*' />
        internal virtual void InternalDispose()
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "IDisposable.Dispose"
                    );
            }

            if (Interlocked.Exchange(ref this.disposed, Transaction.disposedTrueValue) == Transaction.disposedTrueValue)
            {
                return;
            }

            // Attempt to clean up the internal transaction
            long remainingITx = Interlocked.Decrement(ref this.internalTransaction.cloneCount);
            if (remainingITx == 0)
            {
                this.internalTransaction.Dispose();
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "IDisposable.Dispose"
                    );
            }
        }


        // Ask the state machine for serialization info.
        //
        /// <include file='doc\Transaction.uex' path='docs/doc[@for="Transaction.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter = true)]
        void ISerializable.GetObjectData(
            SerializationInfo serializationInfo,
            StreamingContext context
            )
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "ISerializable.GetObjectData"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (serializationInfo == null)
            {
                throw new ArgumentNullException("serializationInfo");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                this.internalTransaction.State.GetObjectData(this.internalTransaction, serializationInfo, context);
            }

            if (DiagnosticTrace.Information)
            {
                TransactionSerializedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    this.TransactionTraceId
                    );
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "ISerializable.GetObjectData"
                    );
            }
        }


        /// <summary>
        /// Create a promotable single phase enlistment that promotes to MSDTC.
        /// </summary>
        /// <param name="promotableSinglePhaseNotification">The object that implements the IPromotableSinglePhaseNotification interface.</param>
        /// <returns>
        /// True if the enlistment is successful.
        /// 
        /// False if the transaction already has a durable enlistment or promotable single phase enlistment or
        /// if the transaction has already promoted. In this case, the caller will need to enlist in the transaction through other
        /// means, such as Transaction.EnlistDurable or retreive the MSDTC export cookie or propagation token to enlist with MSDTC.
        /// </returns>
        // We apparently didn't spell Promotable like FXCop thinks it should be spelled.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public bool EnlistPromotableSinglePhase(IPromotableSinglePhaseNotification promotableSinglePhaseNotification)
        {
            return this.EnlistPromotableSinglePhase(promotableSinglePhaseNotification, TransactionInterop.PromoterTypeDtc);
        }

        /// <summary>
        /// Create a promotable single phase enlistment that promotes to a distributed transaction manager other than MSDTC.
        /// </summary>
        /// <param name="promotableSinglePhaseNotification">The object that implements the IPromotableSinglePhaseNotification interface.</param>
        /// <param name="promoterType">
        /// The promoter type Guid that identifies the format of the byte[] that is returned by the ITransactionPromoter.Promote
        /// call that is implemented by the IPromotableSinglePhaseNotificationObject, and thus the promoter of the transaction.
        /// </param>
        /// <returns>
        /// True if the enlistment is successful.
        /// 
        /// False if the transaction already has a durable enlistment or promotable single phase enlistment or
        /// if the transaction has already promoted. In this case, the caller will need to enlist in the transaction through other
        /// means.
        /// 
        /// If the Transaction.PromoterType matches the promoter type supported by the caller, then the
        /// Transaction.PromotedToken can be retrieved and used to enlist directly with the identified distributed transaction manager.
        ///
        /// How the enlistment is created with the distributed transaction manager identified by the Transaction.PromoterType
        /// is defined by that distributed transaction manager.
        /// </returns>
        // We apparently didn't spell Promotable like FXCop thinks it should be spelled.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public bool EnlistPromotableSinglePhase(IPromotableSinglePhaseNotification promotableSinglePhaseNotification, Guid promoterType)
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.EnlistPromotableSinglePhase"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (promotableSinglePhaseNotification == null)
            {
                throw new ArgumentNullException("promotableSinglePhaseNotification");
            }

            if (promoterType == Guid.Empty)
            {
                throw new ArgumentException(SR.GetString(SR.PromoterTypeInvalid));
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            bool succeeded = false;

            lock (this.internalTransaction)
            {
                succeeded = this.internalTransaction.State.EnlistPromotableSinglePhase(this.internalTransaction, promotableSinglePhaseNotification, this, promoterType);
            }

            if (DiagnosticTrace.Verbose)
            {
                MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.EnlistPromotableSinglePhase"
                    );
            }

            return succeeded;
        }


        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public Enlistment PromoteAndEnlistDurable(Guid resourceManagerIdentifier,
                                                  IPromotableSinglePhaseNotification promotableNotification,
                                                  ISinglePhaseNotification enlistmentNotification,
                                                  EnlistmentOptions enlistmentOptions)
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceOletx),
                    "Transaction.PromoteAndEnlistDurable"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (resourceManagerIdentifier == Guid.Empty)
            {
                throw new ArgumentException(SR.GetString(SR.BadResourceManagerId), "resourceManagerIdentifier");
            }

            if (promotableNotification == null)
            {
                throw new ArgumentNullException("promotableNotification");
            }

            if (enlistmentNotification == null)
            {
                throw new ArgumentNullException("enlistmentNotification");
            }

            if (enlistmentOptions != EnlistmentOptions.None && enlistmentOptions != EnlistmentOptions.EnlistDuringPrepareRequired)
            {
                throw new ArgumentOutOfRangeException("enlistmentOptions");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                Enlistment enlistment = this.internalTransaction.State.PromoteAndEnlistDurable(this.internalTransaction,
                    resourceManagerIdentifier, promotableNotification, enlistmentNotification, enlistmentOptions, this);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceOletx),
                        "Transaction.PromoteAndEnlistDurable"
                        );
                }
                return enlistment;
            }
        }

        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public void SetDistributedTransactionIdentifier(IPromotableSinglePhaseNotification promotableNotification,
                                                        Guid distributedTransactionIdentifier)
        {
            if (DiagnosticTrace.Verbose)
            {
                MethodEnteredTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                    "Transaction.SetDistributedTransactionIdentifier"
                    );
            }

            if (Disposed)
            {
                throw new ObjectDisposedException("Transaction");
            }

            if (promotableNotification == null)
            {
                throw new ArgumentNullException("promotableNotification");
            }

            if (distributedTransactionIdentifier == Guid.Empty)
            {
                throw new ArgumentException("distributedTransactionIdentifier");
            }

            if (this.complete)
            {
                throw TransactionException.CreateTransactionCompletedException(SR.GetString(SR.TraceSourceLtm), this.DistributedTxId);
            }

            lock (this.internalTransaction)
            {
                this.internalTransaction.State.SetDistributedTransactionId(this.internalTransaction,
                    promotableNotification,
                    distributedTransactionIdentifier);

                if (DiagnosticTrace.Verbose)
                {
                    MethodExitedTraceRecord.Trace(SR.GetString(SR.TraceSourceLtm),
                        "Transaction.SetDistributedTransactionIdentifier"
                        );
                }
                return;
            }
        }

        internal Oletx.OletxTransaction Promote()
        {
            lock (this.internalTransaction)
            {
                // This method is only called when we expect to be promoting to MSDTC.
                this.internalTransaction.ThrowIfPromoterTypeIsNotMSDTC();
                this.internalTransaction.State.Promote(this.internalTransaction);
                return this.internalTransaction.PromotedTransaction;
            }
        }
    }


    //
    // The following code & data is related to management of Transaction.Current
    //

    enum DefaultComContextState
    {
        Unknown = 0,
        Unavailable = -1,
        Available = 1
    }


    [System.Security.SuppressUnmanagedCodeSecurity]
    static class NativeMethods
    {
        // User code is not allowed to pass arbitrary data to either of these methods.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage")]
        [System.Runtime.InteropServices.DllImport("Ole32"), System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        internal static extern void CoGetContextToken(out IntPtr contextToken);

        // User code is not allowed to pass arbitrary data to either of these methods.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage")]
        [System.Runtime.InteropServices.DllImport("Ole32"), System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        internal static extern void CoGetDefaultContext(Int32 aptType, ref Guid contextInterface, out SafeIUnknown safeUnknown);
    }

    //
    //  The TxLookup enum is used internally to detect where the ambient context needs to be stored or looked up. 
    //  Default                  - Used internally when looking up Transaction.Current.  
    //  DefaultCallContext - Used when TransactionScope with async flow option is enabled. Internally we will use CallContext to store the ambient transaction.
    //  Default TLS            - Used for legacy/syncronous TransactionScope. Internally we will use TLS to store the ambient transaction.  
    //
    internal enum TxLookup
    {
        Default,
        DefaultCallContext,
        DefaultTLS,
    }

    //
    //  CallContextCurrentData holds the ambient transaction and uses CallContext and ConditionalWeakTable to track the ambient transaction.
    //  For async flow scenarios, we should not allow flowing of transaction across app domains. To prevent transaction from flowing across
    //  AppDomain/Remoting boundaries, we are using ConditionalWeakTable to hold the actual ambient transaction and store only a object reference 
    //  in CallContext. When TransactionScope is used to invoke a call across AppDomain/Remoting boundaries, only the object reference will be sent   
    //  across and not the actaul ambient transaction which is stashed away in the ConditionalWeakTable. 
    //
    internal static class CallContextCurrentData
    {
        private static string CurrentTransactionProperty = "TxCurrent_" + Guid.NewGuid().ToString();

        // ConditionalWeakTable is used to automatically remove the entries that are no longer referenced. This will help prevent leaks in async nested TransactionScope
        // usage and when child nested scopes are not syncronized properly. 
        static readonly ConditionalWeakTable<ContextKey, ContextData> ContextDataTable = new ConditionalWeakTable<ContextKey, ContextData>();

        // 
        //  Set CallContext data with the given contextKey. 
        //  return the ContextData if already present in contextDataTable, otherwise return the default value. 
        // 
        static public ContextData CreateOrGetCurrentData(ContextKey contextKey)
        {
           CallContext.LogicalSetData(CurrentTransactionProperty, contextKey);
           return ContextDataTable.GetValue(contextKey, (env) => new ContextData(true));
        }

        static public void ClearCurrentData(ContextKey contextKey, bool removeContextData)
        {
            // Get the current ambient CallContext.
            ContextKey key = (ContextKey)CallContext.LogicalGetData(CurrentTransactionProperty);
            if (contextKey != null || key != null)
            {
                // removeContextData flag is used for perf optimization to avoid removing from the table in certain nested TransactionScope usage. 
                if (removeContextData)
                {
                    // if context key is passed in remove that from the contextDataTable, otherwise remove the default context key. 
                    ContextDataTable.Remove(contextKey ?? key);
                }

                if (key != null)
                {
                    CallContext.LogicalSetData(CurrentTransactionProperty, null);
                }
            }
        }

        static public bool TryGetCurrentData(out ContextData currentData)
        {
            currentData = null;
            ContextKey contextKey = (ContextKey)CallContext.LogicalGetData(CurrentTransactionProperty);
            if (contextKey == null)
            {
               return false;
            }
            else
            {
               return ContextDataTable.TryGetValue(contextKey, out currentData);
            }
        }
    }

    //
    // MarshalByRefObject is needed for cross AppDomain scenarios where just using object will end up with a different reference when call is made across serialization boundary.
    //
    class ContextKey : MarshalByRefObject
    {
        // Set the lease lifetime according to the AppSetting Transactions:ContextKeyLeaseLifetimeInMinutes. Only change it if the value is greater than 0.
        public override object InitializeLifetimeService()
        {
            ILease lease = (ILease)base.InitializeLifetimeService();
            int leaseLifetime = AppSettings.ContextKeyRemotingLeaseLifetimeInMinutes;
            // If the specified lease lifetime is less than 0, we don't change the value from the AppDomain.
            // This is how you opt-out of this fix.
            // A specified value of 0 was filtered out when initializing the AppSettings and it was mapped to
            // the default value, treating it as if it wasn't specified.
            if ((lease != null) && (lease.CurrentState == LeaseState.Initial) && (leaseLifetime > 0))
            {
                lease.InitialLeaseTime = TimeSpan.FromMinutes(leaseLifetime);
                lease.RenewOnCallTime = TimeSpan.FromMinutes(leaseLifetime);
            }
            return lease;
        }
    }

    class ContextData
    {
        internal TransactionScope CurrentScope;
        internal Transaction CurrentTransaction;

        internal DefaultComContextState DefaultComContextState;
        internal WeakReference WeakDefaultComContext;

        internal bool asyncFlow;
        
        [ThreadStatic]
        private static ContextData staticData;
      
        internal ContextData(bool asyncFlow)
        {
            this.asyncFlow = asyncFlow;
        }
        
        internal static ContextData TLSCurrentData
        {
            get
            {
                ContextData data = staticData;
                if (data == null)
                {
                    data = new ContextData(false);
                    staticData = data;
                }
                
                return data;
            }
            set 
            {
                if (value == null && staticData != null)
                {
                    // set each property to null to retain one TLS ContextData copy.
                    staticData.CurrentScope = null;
                    staticData.CurrentTransaction = null;
                    staticData.DefaultComContextState = DefaultComContextState.Unknown;
                    staticData.WeakDefaultComContext = null;
                }
                else
                {
                    staticData = value;
                }
            }
        }

        internal static ContextData LookupContextData(TxLookup defaultLookup)
        {
            ContextData currentData = null;          
            if (CallContextCurrentData.TryGetCurrentData(out currentData))
            {
                if (currentData.CurrentScope == null && currentData.CurrentTransaction == null && defaultLookup != TxLookup.DefaultCallContext)
                {
                    // Clear Call Context Data
                    CallContextCurrentData.ClearCurrentData(null, true);
                    return ContextData.TLSCurrentData;
                }
               
                return currentData;
            }
            else
            {
                return ContextData.TLSCurrentData;                        
            }
        }
    }
}
