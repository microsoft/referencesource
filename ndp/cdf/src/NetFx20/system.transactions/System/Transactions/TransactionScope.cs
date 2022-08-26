//-----------------------------------------------------------------------------
// <copyright file="TransactionScope.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

using System;
using System.Diagnostics;
using SysES = System.EnterpriseServices;
using System.Runtime.CompilerServices;
using System.Runtime.Remoting.Messaging;
using System.Runtime.InteropServices;
using System.Threading;
using System.Transactions.Diagnostics;

namespace System.Transactions
{
    public enum TransactionScopeOption
    {
        Required,
        RequiresNew,
        Suppress,
    }

    //
    //  The legacy TransactionScope uses TLS to store the ambient transaction. TLS data doesn't flow across thread continuations and hence legacy TransactionScope does not compose well with
    //  new .Net async programming model constructs like Tasks and async/await. To enable TransactionScope to work with Task and async/await, a new TransactionScopeAsyncFlowOption
    //  is introduced. When users opt-in the async flow option, ambient transaction will automatically flow across thread continuations and user can compose TransactionScope with Task and/or    
    //  async/await constructs. 
    // 
    public enum TransactionScopeAsyncFlowOption
    {
        Suppress, // Ambient transaction will be stored in TLS and will not flow across thread continuations. 
        Enabled,  // Ambient transaction will be stored in CallContext and will flow across thread continuations. This option will enable TransactionScope to compose well with Task and async/await.
    }

    public enum EnterpriseServicesInteropOption
    {
        None = 0,
        Automatic = 1,
        Full = 2
    }


    public sealed class TransactionScope : IDisposable
    {
        public TransactionScope() : this( TransactionScopeOption.Required )
        {
        }

        public TransactionScope(TransactionScopeOption scopeOption)
            : this(scopeOption, TransactionScopeAsyncFlowOption.Suppress)
        {

        }

        public TransactionScope(TransactionScopeAsyncFlowOption asyncFlowOption) 
            : this(TransactionScopeOption.Required, asyncFlowOption)
        {
        }

        public TransactionScope(
            TransactionScopeOption scopeOption,
            TransactionScopeAsyncFlowOption asyncFlowOption
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption )"
                    );
            }

            ValidateAndSetAsyncFlowOption(asyncFlowOption);
            
            if ( NeedToCreateTransaction( scopeOption ) )
            {
                committableTransaction = new CommittableTransaction();
                expectedCurrent = committableTransaction.Clone();
            }

            if ( DiagnosticTrace.Information )
            {
                if ( null == expectedCurrent )
                {
                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        TransactionTraceIdentifier.Empty,
                        TransactionScopeResult.NoTransaction
                        );
                }
                else
                {
                    TransactionScopeResult scopeResult;

                    if ( null == committableTransaction )
                    {
                        scopeResult = TransactionScopeResult.UsingExistingCurrent;
                    }
                    else
                    {
                        scopeResult = TransactionScopeResult.CreatedTransaction;
                    }

                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        expectedCurrent.TransactionTraceId,
                        scopeResult
                        );
                }
            }

            PushScope();
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption )"
                    );
            }
        }


        public TransactionScope(TransactionScopeOption scopeOption, TimeSpan scopeTimeout)
            : this(scopeOption, scopeTimeout, TransactionScopeAsyncFlowOption.Suppress)
        {

        }
        
        public TransactionScope(
            TransactionScopeOption scopeOption,
            TimeSpan scopeTimeout,
            TransactionScopeAsyncFlowOption asyncFlowOption
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption, TimeSpan )"
                    );
            }

            ValidateScopeTimeout( "scopeTimeout", scopeTimeout );
            TimeSpan txTimeout = TransactionManager.ValidateTimeout( scopeTimeout );

            ValidateAndSetAsyncFlowOption(asyncFlowOption);

            if ( NeedToCreateTransaction( scopeOption ))
            {
                this.committableTransaction = new CommittableTransaction( txTimeout );
                this.expectedCurrent = committableTransaction.Clone();
            }

            if ( (null != this.expectedCurrent) && (null == this.committableTransaction) && (TimeSpan.Zero != scopeTimeout) )
            {
                // 
                scopeTimer = new Timer(
                    TransactionScope.TimerCallback,
                    this,
                    scopeTimeout,
                    TimeSpan.Zero
                    );
            }

            if ( DiagnosticTrace.Information )
            {
                if ( null == expectedCurrent )
                {
                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        TransactionTraceIdentifier.Empty,
                        TransactionScopeResult.NoTransaction
                        );
                }
                else
                {
                    TransactionScopeResult scopeResult;

                    if ( null == committableTransaction )
                    {
                        scopeResult = TransactionScopeResult.UsingExistingCurrent;
                    }
                    else
                    {
                        scopeResult = TransactionScopeResult.CreatedTransaction;
                    }

                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        expectedCurrent.TransactionTraceId,
                        scopeResult
                        );
                }
            }

            PushScope();
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption, TimeSpan )"
                    );
            }
        }


        public TransactionScope(TransactionScopeOption scopeOption, TransactionOptions transactionOptions)
            : this(scopeOption, transactionOptions, TransactionScopeAsyncFlowOption.Suppress)
        {

        }

        public TransactionScope(
            TransactionScopeOption scopeOption,
            TransactionOptions transactionOptions,
            TransactionScopeAsyncFlowOption asyncFlowOption
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption, TransactionOptions )"
                    );
            }

            ValidateScopeTimeout( "transactionOptions.Timeout", transactionOptions.Timeout );
            TimeSpan scopeTimeout = transactionOptions.Timeout;

            transactionOptions.Timeout = TransactionManager.ValidateTimeout( transactionOptions.Timeout );
            TransactionManager.ValidateIsolationLevel( transactionOptions.IsolationLevel );

            ValidateAndSetAsyncFlowOption(asyncFlowOption);
            
            if ( NeedToCreateTransaction( scopeOption ) )
            {
                this.committableTransaction = new CommittableTransaction( transactionOptions );
                this.expectedCurrent = committableTransaction.Clone();
            }
            else
            {
                if ( null != this.expectedCurrent )
                {
                    // If the requested IsolationLevel is stronger than that of the specified transaction, throw.
                    if ( (IsolationLevel.Unspecified != transactionOptions.IsolationLevel) && ( expectedCurrent.IsolationLevel != transactionOptions.IsolationLevel ) )
                    {
                        throw new ArgumentException( SR.GetString( SR.TransactionScopeIsolationLevelDifferentFromTransaction ), "transactionOptions.IsolationLevel" );
                    }
                }
            }

            if ( (null != this.expectedCurrent) && (null == this.committableTransaction) && (TimeSpan.Zero != scopeTimeout) )
            {
                // 
                scopeTimer = new Timer(
                    TransactionScope.TimerCallback,
                    this,
                    scopeTimeout,
                    TimeSpan.Zero
                    );
            }

            if ( DiagnosticTrace.Information )
            {
                if ( null == expectedCurrent )
                {
                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        TransactionTraceIdentifier.Empty,
                        TransactionScopeResult.NoTransaction
                        );
                }
                else
                {
                    TransactionScopeResult scopeResult;

                    if ( null == committableTransaction )
                    {
                        scopeResult = TransactionScopeResult.UsingExistingCurrent;
                    }
                    else
                    {
                        scopeResult = TransactionScopeResult.CreatedTransaction;
                    }

                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        expectedCurrent.TransactionTraceId,
                        scopeResult
                        );
                }
            }

            PushScope();
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption, TransactionOptions )"
                    );
            }
        }

        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public TransactionScope(
            TransactionScopeOption scopeOption,
            TransactionOptions transactionOptions,
            EnterpriseServicesInteropOption interopOption
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption, TransactionOptions, EnterpriseServicesInteropOption )"
                    );
            }

            ValidateScopeTimeout( "transactionOptions.Timeout", transactionOptions.Timeout );
            TimeSpan scopeTimeout = transactionOptions.Timeout;

            transactionOptions.Timeout = TransactionManager.ValidateTimeout( transactionOptions.Timeout );
            TransactionManager.ValidateIsolationLevel( transactionOptions.IsolationLevel );

            ValidateInteropOption( interopOption );
            this.interopModeSpecified = true;
            this.interopOption = interopOption;

            if ( NeedToCreateTransaction( scopeOption ) )
            {
                committableTransaction = new CommittableTransaction( transactionOptions );
                expectedCurrent = committableTransaction.Clone();
            }
            else
            {
                if ( null != expectedCurrent )
                {
                    // If the requested IsolationLevel is stronger than that of the specified transaction, throw.
                    if ( (IsolationLevel.Unspecified != transactionOptions.IsolationLevel) && ( expectedCurrent.IsolationLevel != transactionOptions.IsolationLevel ) )
                    {
                        throw new ArgumentException( SR.GetString( SR.TransactionScopeIsolationLevelDifferentFromTransaction ), "transactionOptions.IsolationLevel" );
                    }
                }
            }

            if ( (null != this.expectedCurrent) && (null == this.committableTransaction) && (TimeSpan.Zero != scopeTimeout) )
            {
                // 
                scopeTimer = new Timer(
                    TransactionScope.TimerCallback,
                    this,
                    scopeTimeout,
                    TimeSpan.Zero
                    );
            }

            if ( DiagnosticTrace.Information )
            {
                if ( null == expectedCurrent )
                {
                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        TransactionTraceIdentifier.Empty,
                        TransactionScopeResult.NoTransaction
                        );
                }
                else
                {
                    TransactionScopeResult scopeResult;

                    if ( null == committableTransaction )
                    {
                        scopeResult = TransactionScopeResult.UsingExistingCurrent;
                    }
                    else
                    {
                        scopeResult = TransactionScopeResult.CreatedTransaction;
                    }

                    TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        expectedCurrent.TransactionTraceId,
                        scopeResult
                        );
                }
            }

            PushScope();
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( TransactionScopeOption, TransactionOptions, EnterpriseServicesInteropOption )"
                    );
            }
        }
       
        public TransactionScope(Transaction transactionToUse)
            : this(transactionToUse, TransactionScopeAsyncFlowOption.Suppress)
        {

        }

        public TransactionScope(
            Transaction transactionToUse,
            TransactionScopeAsyncFlowOption asyncFlowOption
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( Transaction )"
                    );
            }

            ValidateAndSetAsyncFlowOption(asyncFlowOption);
            
            Initialize( 
                transactionToUse,
                TimeSpan.Zero,
                false
                );
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( Transaction )"
                    );
            }
        }

        public TransactionScope(Transaction transactionToUse, TimeSpan scopeTimeout)
            : this(transactionToUse, scopeTimeout, TransactionScopeAsyncFlowOption.Suppress)
        {

        }

        public TransactionScope(
            Transaction transactionToUse,
            TimeSpan scopeTimeout,
            TransactionScopeAsyncFlowOption asyncFlowOption
            )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( Transaction, TimeSpan )"
                    );
            }

            ValidateAndSetAsyncFlowOption(asyncFlowOption);
            
            Initialize( 
                transactionToUse,
                scopeTimeout,
                false
                );
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( Transaction, TimeSpan )"
                    );
            }
        }

        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        public TransactionScope(
            Transaction transactionToUse,
            TimeSpan scopeTimeout,
            EnterpriseServicesInteropOption interopOption
        )
        {
            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                 MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                     "TransactionScope.ctor( Transaction, TimeSpan, EnterpriseServicesInteropOption )"
                     );
            }

            ValidateInteropOption( interopOption );
            this.interopOption = interopOption;

            Initialize( 
                transactionToUse,
                scopeTimeout,
                true
                );

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.ctor( Transaction, TimeSpan, EnterpriseServicesInteropOption )"
                    );
            }
        }

        private bool NeedToCreateTransaction(
            TransactionScopeOption scopeOption
            )
        {
            bool retVal = false;

            CommonInitialize();

            // If the options specify NoTransactionNeeded, that trumps everything else.
            switch ( scopeOption )
            {
                case TransactionScopeOption.Suppress:
                    expectedCurrent = null;
                    retVal = false;
                    break;
                                
                case TransactionScopeOption.Required:
                    expectedCurrent = this.savedCurrent;
                    // If current is null, we need to create one.
                    if ( null == expectedCurrent )
                    {
                        retVal = true;
                    }
                    break;

                case TransactionScopeOption.RequiresNew:
                    retVal = true;
                    break;

                default:
                    throw new ArgumentOutOfRangeException( "scopeOption" );
            }

            return retVal;
        }


        private void Initialize(
            Transaction transactionToUse,
            TimeSpan scopeTimeout,
            bool interopModeSpecified
            )
        {
            if ( null == transactionToUse )
            {
                throw new ArgumentNullException( "transactionToUse" );
            }

            ValidateScopeTimeout( "scopeTimeout", scopeTimeout );

            CommonInitialize();

            if ( TimeSpan.Zero != scopeTimeout )
            {
                scopeTimer = new Timer(
                    TransactionScope.TimerCallback,
                    this,
                    scopeTimeout,
                    TimeSpan.Zero
                    );
            }

            this.expectedCurrent = transactionToUse;
            this.interopModeSpecified = interopModeSpecified;

            if ( DiagnosticTrace.Information )
            {
                TransactionScopeCreatedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    expectedCurrent.TransactionTraceId,
                    TransactionScopeResult.TransactionPassed
                    );
            }

            PushScope();
        }


        // We don't have a finalizer (~TransactionScope) because all it would be able to do is try to 
        // operate on other managed objects (the transaction), which is not safe to do because they may
        // already have been finalized.

        // FXCop wants us to dispose savedCurrent, which is a Transaction, and thus disposable.  But we don't
        // want to do that here.  We want to restore Current to that value.  We do dispose the expected current.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed")]
        public void Dispose()
        {
            bool successful = false;

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.Dispose"
                    );
            }
            if ( this.disposed )
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionScope.Dispose"
                        );
                }
                return;
            }

            // Dispose for a scope can only be called on the thread where the scope was created.
            if ((this.scopeThread != Thread.CurrentThread) && !this.AsyncFlowEnabled)
            {               
                if ( DiagnosticTrace.Error )
                {
                    InvalidOperationExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        SR.GetString( SR.InvalidScopeThread )
                        );
                }

                throw new InvalidOperationException( SR.GetString( SR.InvalidScopeThread ));
            }

            Exception exToThrow = null;

            try
            {
                // Single threaded from this point
                this.disposed = true;

                // First, lets pop the "stack" of TransactionScopes and dispose each one that is above us in
                // the stack, making sure they are NOT consistent before disposing them.

                // Optimize the first lookup by getting both the actual current scope and actual current
                // transaction at the same time.
                TransactionScope actualCurrentScope = this.threadContextData.CurrentScope;
                Transaction contextTransaction = null;
                Transaction current = Transaction.FastGetTransaction( actualCurrentScope, this.threadContextData, out contextTransaction );

                if ( !Equals( actualCurrentScope ))
                {
                    // Ok this is bad.  But just how bad is it.  The worst case scenario is that someone is
                    // poping scopes out of order and has placed a new transaction in the top level scope.
                    // Check for that now.
                    if ( actualCurrentScope == null )
                    {
                        // Something must have gone wrong trying to clean up a bad scope
                        // stack previously.
                        // Make a best effort to abort the active transaction.
                        Transaction rollbackTransaction = this.committableTransaction;
                        if ( rollbackTransaction == null )
                        {
                            rollbackTransaction = this.dependentTransaction;
                        }
                        Debug.Assert( rollbackTransaction != null );
                        rollbackTransaction.Rollback();

                        successful = true;
                        throw TransactionException.CreateInvalidOperationException( SR.GetString( SR.TraceSourceBase ),
                            SR.GetString(SR.TransactionScopeInvalidNesting), null, rollbackTransaction.DistributedTxId);
                    }
                    // Verify that expectedCurrent is the same as the "current" current if we the interopOption value is None.
                    else if ( EnterpriseServicesInteropOption.None == actualCurrentScope.interopOption )
                    {
                        if ( ( ( null != actualCurrentScope.expectedCurrent ) && ( ! actualCurrentScope.expectedCurrent.Equals( current ) ) )
                            ||
                            ( ( null != current ) && ( null == actualCurrentScope.expectedCurrent ) )
                            )
                        {
                            if ( DiagnosticTrace.Warning )
                            {
                                TransactionTraceIdentifier myId;
                                TransactionTraceIdentifier currentId;

                                if ( null == current )
                                {
                                    currentId = TransactionTraceIdentifier.Empty;
                                }
                                else
                                {
                                    currentId = current.TransactionTraceId;
                                }

                                if ( null == this.expectedCurrent )
                                {
                                    myId = TransactionTraceIdentifier.Empty;
                                }
                                else
                                {
                                    myId = this.expectedCurrent.TransactionTraceId;
                                }

                                TransactionScopeCurrentChangedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                                    myId,
                                    currentId
                                    );
                            }

                            exToThrow = TransactionException.CreateInvalidOperationException(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionScopeIncorrectCurrent), null,
                                current == null ? Guid.Empty : current.DistributedTxId);

                            // If there is a current transaction, abort it.
                            if ( null != current )
                            {
                                try
                                {
                                    current.Rollback();
                                }
                                catch ( TransactionException )
                                {
                                    // we are already going to throw and exception, so just ignore this one.
                                }
                                catch ( ObjectDisposedException )
                                {
                                    // Dito
                                }
                            }
                        }
                    }

                    // Now fix up the scopes
                    while ( !Equals( actualCurrentScope ))
                    {
                        if ( null == exToThrow )
                        {
                            exToThrow = TransactionException.CreateInvalidOperationException( SR.GetString( SR.TraceSourceBase ), SR.GetString( SR.TransactionScopeInvalidNesting ), null,
                                current == null ? Guid.Empty : current.DistributedTxId );
                        }
                    
                        if ( DiagnosticTrace.Warning )
                        {
                            if ( null == actualCurrentScope.expectedCurrent )
                            {
                                TransactionScopeNestedIncorrectlyTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                                    TransactionTraceIdentifier.Empty
                                    );
                            }
                            else
                            {
                                TransactionScopeNestedIncorrectlyTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                                    actualCurrentScope.expectedCurrent.TransactionTraceId
                                    );
                            }

                        }

                        actualCurrentScope.complete = false;
                        try
                        {
                            actualCurrentScope.InternalDispose();
                        }
                        catch ( TransactionException )
                        {
                            // we are already going to throw an exception, so just ignore this one.
                        }

                        actualCurrentScope = this.threadContextData.CurrentScope;

                        // We want to fail this scope, too, because work may have been done in one of these other
                        // nested scopes that really should have been done in my scope.
                        this.complete = false;
                    }
                }
                else
                {
                    // Verify that expectedCurrent is the same as the "current" current if we the interopOption value is None.
                    // If we got here, actualCurrentScope is the same as "this".
                    if ( EnterpriseServicesInteropOption.None == this.interopOption )
                    {
                        if ((( null != this.expectedCurrent ) && ( ! this.expectedCurrent.Equals( current )))
                            || (( null != current ) && ( null == this.expectedCurrent ))
                            )
                        {
                            if ( DiagnosticTrace.Warning )
                            {
                                TransactionTraceIdentifier myId;
                                TransactionTraceIdentifier currentId;

                                if ( null == current )
                                {
                                    currentId = TransactionTraceIdentifier.Empty;
                                }
                                else
                                {
                                    currentId = current.TransactionTraceId;
                                }

                                if ( null == this.expectedCurrent )
                                {
                                    myId = TransactionTraceIdentifier.Empty;
                                }
                                else
                                {
                                    myId = this.expectedCurrent.TransactionTraceId;
                                }

                                TransactionScopeCurrentChangedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                                    myId,
                                    currentId
                                    );
                            }

                            if ( null == exToThrow )
                            {
                                exToThrow = TransactionException.CreateInvalidOperationException(SR.GetString(SR.TraceSourceBase), SR.GetString(SR.TransactionScopeIncorrectCurrent), null,
                                    current == null ? Guid.Empty : current.DistributedTxId);
                            }
                        
                            // If there is a current transaction, abort it.
                            if ( null != current )
                            {
                                try
                                {
                                    current.Rollback();
                                }
                                catch ( TransactionException )
                                {
                                    // we are already going to throw and exception, so just ignore this one.
                                }
                                catch ( ObjectDisposedException )
                                {
                                    // Dito
                                }
                            }
                            // Set consistent to false so that the subsequent call to
                            // InternalDispose below will rollback this.expectedCurrent.
                            this.complete = false;
                        }
                    }
                }
                successful = true;
            }
            finally
            {
                if ( !successful )
                {
                    PopScope();
                }
            }

            // No try..catch here.  Just let any exception thrown by InternalDispose go out.
            InternalDispose();

            if ( null != exToThrow )
            {
                throw exToThrow;
            }
            
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.Dispose"
                    );
            }
        }


        private void InternalDispose()
        {
            // Set this if it is called internally.
            this.disposed = true;

            try
            {
                PopScope();

                if ( DiagnosticTrace.Information )
                {
                    if ( null == this.expectedCurrent )
                    {
                        TransactionScopeDisposedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                            TransactionTraceIdentifier.Empty
                            );
                    }
                    else
                    {
                        TransactionScopeDisposedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                            this.expectedCurrent.TransactionTraceId
                            );
                    }
                }

                // If Transaction.Current is not null, we have work to do.  Otherwise, we don't, except to replace
                // the previous value.
                if ( null != this.expectedCurrent )
                {
                    if ( !this.complete )
                    {
                        if ( DiagnosticTrace.Warning )
                        {
                            TransactionScopeIncompleteTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                                this.expectedCurrent.TransactionTraceId
                                );
                        }

                        //
                        // Note: Rollback is not called on expected current because someone could conceiveably
                        //       dispose expectedCurrent out from under the transaction scope.
                        //
                        Transaction rollbackTransaction = this.committableTransaction;
                        if ( rollbackTransaction == null )
                        {
                            rollbackTransaction = this.dependentTransaction;
                        }
                        Debug.Assert( rollbackTransaction != null );
                        rollbackTransaction.Rollback();
                    }
                    else
                    {
                        // If we are supposed to commit on dispose, cast to CommittableTransaction and commit it.
                        if ( null != this.committableTransaction )
                        {
                            this.committableTransaction.Commit();
                        }
                        else 
                        {
                            Debug.Assert( null != this.dependentTransaction, "null != this.dependentTransaction" );
                            this.dependentTransaction.Complete();
                        }
                    }
                }
            }
            finally
            {
                if ( null != scopeTimer )
                {
                    scopeTimer.Dispose();
                }

                if ( null != this.committableTransaction )
                {
                    this.committableTransaction.Dispose();

                    // If we created the committable transaction then we placed a clone in expectedCurrent
                    // and it needs to be disposed as well.
                    this.expectedCurrent.Dispose();
                }

                if ( null != this.dependentTransaction )
                {
                    this.dependentTransaction.Dispose();
                }
            }
        }


        public void Complete()
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.Complete"
                    );
            }
            if ( this.disposed )
            {
                throw new ObjectDisposedException( "TransactionScope" );
            }

            if ( this.complete )
            {
                throw TransactionException.CreateInvalidOperationException( SR.GetString( SR.TraceSourceBase ), SR.GetString(SR.DisposeScope), null);
            }

            this.complete = true;
            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionScope.Complete"
                    );
            }
        }            


        private static void TimerCallback(
            object state
            )
        {
            TransactionScope scope = state as TransactionScope;
            if ( null == scope )
            {
                if ( DiagnosticTrace.Critical )
                {
                    InternalErrorTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        SR.GetString( SR.TransactionScopeTimerObjectInvalid )
                        );
                }

                throw TransactionException.Create( SR.GetString( SR.TraceSourceBase ), SR.GetString( SR.InternalError) + SR.GetString( SR.TransactionScopeTimerObjectInvalid ), null );
            }

            scope.Timeout();
        }


        private void Timeout()
        {
            if ( ( !this.complete ) && ( null != this.expectedCurrent ) )
            {
                if ( DiagnosticTrace.Warning )
                {
                    TransactionScopeTimeoutTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        this.expectedCurrent.TransactionTraceId
                        );
                }
                try
                {
                    this.expectedCurrent.Rollback();
                }
                catch ( ObjectDisposedException ex )
                {
                    // Tolerate the fact that the transaction has already been disposed.
                    if ( DiagnosticTrace.Verbose )
                    {
                        ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                            ex );
                    }
                }
                catch ( TransactionException txEx )
                {
                    // Tolerate transaction exceptions - VSWhidbey 466868.
                    if ( DiagnosticTrace.Verbose )
                    {
                        ExceptionConsumedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                            txEx );
                    }
                }
            }
        }


        private void CommonInitialize()
        {
            this.ContextKey = new ContextKey();
            this.complete = false;
            this.dependentTransaction = null;
            this.disposed = false;
            this.committableTransaction = null;
            this.expectedCurrent = null;
            this.scopeTimer = null;
            this.scopeThread = Thread.CurrentThread;

            Transaction.GetCurrentTransactionAndScope( 
                            this.AsyncFlowEnabled ? TxLookup.DefaultCallContext : TxLookup.DefaultTLS,
                            out this.savedCurrent, 
                            out this.savedCurrentScope, 
                            out this.contextTransaction
                            );

            // Calling validate here as we need to make sure the existing parent ambient transaction scope is already looked up to see if we have ES interop enabled.  
            ValidateAsyncFlowOptionAndESInteropOption();            
        }


        // PushScope
        //
        // Push a transaction scope onto the stack.
        private void PushScope()
        {
            // Fixup the interop mode before we set current.
            if ( !this.interopModeSpecified )
            {
                // Transaction.InteropMode will take the interop mode on
                // for the scope in currentScope into account.
                this.interopOption = Transaction.InteropMode(this.savedCurrentScope);
            }

            // async function yield at await points and main thread can continue execution. We need to make sure the TLS data are restored appropriately.
            SaveTLSContextData();
            
            if (this.AsyncFlowEnabled)
            {
                // Async Flow is enabled and CallContext will be used for ambient transaction. 
                this.threadContextData = CallContextCurrentData.CreateOrGetCurrentData(this.ContextKey);

                if (this.savedCurrentScope == null && this.savedCurrent == null)
                {
                    // Clear TLS data so that transaction doesn't leak from current thread.
                    ContextData.TLSCurrentData = null;                        
                }
            }
            else
            {
                // Legacy TransactionScope. Use TLS to track ambient transaction context.
                this.threadContextData = ContextData.TLSCurrentData;
                CallContextCurrentData.ClearCurrentData(this.ContextKey, false);
            }

            // This call needs to be done first
            SetCurrent( expectedCurrent ); 
            this.threadContextData.CurrentScope = this;

        }

        // PopScope
        //
        // Pop the current transaction scope off the top of the stack
        private void PopScope()
        {
            bool shouldRestoreContextData = true;
            
            // Clear the current TransactionScope CallContext data
            if (this.AsyncFlowEnabled)
            {
                CallContextCurrentData.ClearCurrentData(this.ContextKey, true);
            }

            if (this.scopeThread == Thread.CurrentThread)
            {
                // async function yield at await points and main thread can continue execution. We need to make sure the TLS data are restored appropriately.
                // Restore the TLS only if the thread Ids match.
                RestoreSavedTLSContextData();
            }

            // Restore threadContextData to parent CallContext or TLS data
            if (this.savedCurrentScope != null)
            {
                if (this.savedCurrentScope.AsyncFlowEnabled)
                {
                    this.threadContextData = CallContextCurrentData.CreateOrGetCurrentData(this.savedCurrentScope.ContextKey);
                }
                else
                {
                    if (this.savedCurrentScope.scopeThread != Thread.CurrentThread) 
                    {
                        // Clear TLS data so that transaction doesn't leak from current thread.
                        shouldRestoreContextData = false;
                        ContextData.TLSCurrentData = null; 
                    }
                    else
                    {
                        this.threadContextData = ContextData.TLSCurrentData;
                    }
                                        
                    CallContextCurrentData.ClearCurrentData(this.savedCurrentScope.ContextKey, false);
                }
            }
            else
            {
                // No parent TransactionScope present
                
                // Clear any CallContext data
                CallContextCurrentData.ClearCurrentData(null, false);

                if (this.scopeThread != Thread.CurrentThread) 
                {
                    // Clear TLS data so that transaction doesn't leak from current thread.
                    shouldRestoreContextData = false;
                    ContextData.TLSCurrentData = null; 
                }
                else
                {
                    // Restore the current data to TLS.
                    ContextData.TLSCurrentData = this.threadContextData; 
                }
            }

            // prevent restoring the context in an unexpected thread due to thread switch during TransactionScope's Dispose 
            if (shouldRestoreContextData)
            {
                this.threadContextData.CurrentScope = this.savedCurrentScope;
                RestoreCurrent();
            }
        }

        // SetCurrent
        //
        // Place the given value in current by whatever means necessary for interop mode.
        private void SetCurrent( Transaction newCurrent )
        {
            // Keep a dependent clone of current if we don't have one and we are not committable
            if ( this.dependentTransaction == null && this.committableTransaction == null )
            {
                if ( newCurrent != null )
                {
                    this.dependentTransaction = newCurrent.DependentClone( DependentCloneOption.RollbackIfNotComplete );
                }
            }

            switch ( this.interopOption )
            {
                case EnterpriseServicesInteropOption.None:
                    this.threadContextData.CurrentTransaction = newCurrent;
                    break;

                case EnterpriseServicesInteropOption.Automatic:
                    Transaction.VerifyEnterpriseServicesOk();

                    if ( Transaction.UseServiceDomainForCurrent() )
                    {
                        PushServiceDomain( newCurrent );
                    }
                    else
                    {
                        this.threadContextData.CurrentTransaction = newCurrent;
                    }
                    break;

                case EnterpriseServicesInteropOption.Full:
                    Transaction.VerifyEnterpriseServicesOk();

                    PushServiceDomain( newCurrent );
                    break;
            }
        }

        private void SaveTLSContextData()
        {
            if (this.savedTLSContextData == null)
            {
                this.savedTLSContextData = new ContextData(false);
            }

            this.savedTLSContextData.CurrentScope = ContextData.TLSCurrentData.CurrentScope;
            this.savedTLSContextData.CurrentTransaction = ContextData.TLSCurrentData.CurrentTransaction;
            this.savedTLSContextData.DefaultComContextState = ContextData.TLSCurrentData.DefaultComContextState;
            this.savedTLSContextData.WeakDefaultComContext = ContextData.TLSCurrentData.WeakDefaultComContext;
        }

        private void RestoreSavedTLSContextData()
        {
            if (this.savedTLSContextData != null)
            {
                ContextData.TLSCurrentData.CurrentScope = this.savedTLSContextData.CurrentScope;
                ContextData.TLSCurrentData.CurrentTransaction = this.savedTLSContextData.CurrentTransaction;
                ContextData.TLSCurrentData.DefaultComContextState = this.savedTLSContextData.DefaultComContextState;
                ContextData.TLSCurrentData.WeakDefaultComContext = this.savedTLSContextData.WeakDefaultComContext;
            }
        }
        
        // PushServiceDomain
        //
        // Create a new service domain with the given transaction.
        [System.Runtime.CompilerServices.MethodImplAttribute(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)] 
        // PushServiceDomain will only be called if a transaction scope is created with an 
        // EnterpriseServicesInteropOption.  TransactionScope constructors that take a ESIO are protected by
        // a FullTrust link demand.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods")]
        private void PushServiceDomain( Transaction newCurrent )
        {
            // If we are not changing the transaction for the current ServiceDomain then
            // don't call CoEnterServiceDomain
            if ((newCurrent != null && newCurrent.Equals( SysES.ContextUtil.SystemTransaction )) ||
                (newCurrent == null && SysES.ContextUtil.SystemTransaction == null ))
            {
                return;
            }

            SysES.ServiceConfig serviceConfig = new SysES.ServiceConfig();

            try
            {
                // If a transaction is specified place it in BYOT.  Otherwise the
                // default transaction option for ServiceConfig is disabled.  So 
                // if this is a not supported scope it will be cleared.
                if ( newCurrent != null )
                {
                    // To work around an SWC bug in Com+ we need to create 2
                    // service domains.

                    // Com+ will by default try to inherit synchronization from
                    // an existing context.  Turn that off.
                    serviceConfig.Synchronization = SysES.SynchronizationOption.RequiresNew;
                    SysES.ServiceDomain.Enter( serviceConfig );
                    this.createdDoubleServiceDomain = true;

                    serviceConfig.Synchronization = SysES.SynchronizationOption.Required;
                    serviceConfig.BringYourOwnSystemTransaction = newCurrent;
                }
                SysES.ServiceDomain.Enter( serviceConfig );
                this.createdServiceDomain = true;
            }
            catch ( COMException e )
            {
                if ( System.Transactions.Oletx.NativeMethods.XACT_E_NOTRANSACTION == e.ErrorCode )
                {
                    throw TransactionException.Create(SR.GetString(SR.TraceSourceBase),
                        SR.GetString(SR.TransactionAlreadyOver),
                        e,
                        newCurrent == null ? Guid.Empty : newCurrent.DistributedTxId);
                }

                throw TransactionException.Create(SR.GetString(SR.TraceSourceBase), e.Message, e, newCurrent == null ? Guid.Empty : newCurrent.DistributedTxId);
            }
            finally
            {
                if ( !this.createdServiceDomain )
                {
                    // If we weren't successful in creating both service domains then
                    // make sure to exit one of them.
                    if ( this.createdDoubleServiceDomain )
                    {
                        SysES.ServiceDomain.Leave();
                    }
                }
            }
                
        }


        [System.Runtime.CompilerServices.MethodImplAttribute(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)] 
        // PushServiceDomain will only be called if a transaction scope is created with an 
        // EnterpriseServicesInteropOption.  TransactionScope constructors that take a ESIO are protected by
        // a FullTrust link demand.  Therefore JitSafeLeaveServiceDomain is not exposed directly to a partial
        // trust caller.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods")]
        private void JitSafeLeaveServiceDomain()
        {
            if ( this.createdDoubleServiceDomain )
            {
                // This is an ugly work around to a bug in Com+ that prevents the use of BYOT and SWC with
                // anything other than required synchronization.
                SysES.ServiceDomain.Leave();
            }
            SysES.ServiceDomain.Leave();
        }

        // RestoreCurrent
        //
        // Restore current to it's previous value depending on how it was changed for this scope.
        private void RestoreCurrent()
        {
            if ( this.createdServiceDomain )
            {
                JitSafeLeaveServiceDomain();
            }

            // Only restore the value that was actually in the context.
            this.threadContextData.CurrentTransaction = this.contextTransaction;        
        }


        // ValidateInteropOption
        //
        // Validate a given interop Option
        private void ValidateInteropOption( EnterpriseServicesInteropOption interopOption )
        {
            if ( interopOption < EnterpriseServicesInteropOption.None || interopOption > EnterpriseServicesInteropOption.Full )
            {
                throw new ArgumentOutOfRangeException( "interopOption" );
            }
        }


        // ValidateScopeTimeout
        //
        // Scope timeouts are not governed by MaxTimeout and therefore need a special validate function
        private void ValidateScopeTimeout( string paramName, TimeSpan scopeTimeout )
        {
            if ( scopeTimeout < TimeSpan.Zero )
            {
                throw new ArgumentOutOfRangeException( paramName );
            }
        }

        private void ValidateAndSetAsyncFlowOption(TransactionScopeAsyncFlowOption asyncFlowOption)
        {
            if (asyncFlowOption < TransactionScopeAsyncFlowOption.Suppress || asyncFlowOption > TransactionScopeAsyncFlowOption.Enabled)
            {
                throw new ArgumentOutOfRangeException( "asyncFlowOption" );
            }

            if (asyncFlowOption == TransactionScopeAsyncFlowOption.Enabled)
            {
                this.AsyncFlowEnabled = true;
            }
        }

        // The validate method assumes that the existing parent ambient transaction scope is already looked up.  
        private void ValidateAsyncFlowOptionAndESInteropOption()
        {
            if (this.AsyncFlowEnabled)
            {
                EnterpriseServicesInteropOption currentInteropOption = this.interopOption;
                if ( !this.interopModeSpecified )
                {
                    // Transaction.InteropMode will take the interop mode on
                    // for the scope in currentScope into account.
                    currentInteropOption = Transaction.InteropMode(this.savedCurrentScope);
                }

                if (currentInteropOption != EnterpriseServicesInteropOption.None)
                {
                    throw new NotSupportedException(SR.GetString(SR.AsyncFlowAndESInteropNotSupported));    
                }
            }
        }
        
        // Denotes the action to take when the scope is disposed.
        bool complete;
        internal bool ScopeComplete
        {
            get
            {
                return this.complete;
            }
        }

        // Storage location for the previous current transaction.
        Transaction savedCurrent;

        // To ensure that we don't restore a value for current that was
        // returned to us by an external entity keep the value that was actually
        // in TLS when the scope was created.
        Transaction contextTransaction;

        // Storage for the value to restore to current
        TransactionScope savedCurrentScope;

        // Store a reference to the context data object for this scope.
        ContextData threadContextData;

        ContextData savedTLSContextData;
        
        // Store a reference to the value that this scope expects for current
        Transaction expectedCurrent;

        // Store a reference to the committable form of this transaction if 
        // the scope made one.
        CommittableTransaction committableTransaction;

        // Store a reference to the scopes transaction guard.
        DependentTransaction dependentTransaction;

        // Note when the scope is disposed.
        bool disposed;

        // 

        Timer scopeTimer;

        // Store a reference to the thread on which the scope was created so that we can
        // check to make sure that the dispose pattern for scope is being used correctly.
        Thread scopeThread;
      
        // Store a member to let us know if a new service domain has been created for this
        // scope.
        bool createdServiceDomain;
        bool createdDoubleServiceDomain;

        // Store the interop mode for this transaction scope.
        bool interopModeSpecified = false;
        EnterpriseServicesInteropOption interopOption;
        internal EnterpriseServicesInteropOption InteropMode
        {
            get
            {
                return this.interopOption;
            }
        }

        internal ContextKey ContextKey
        {
            get;
            private set;
        }
        
        internal bool AsyncFlowEnabled
        {
            get;
            private set;
        }
    }
}
