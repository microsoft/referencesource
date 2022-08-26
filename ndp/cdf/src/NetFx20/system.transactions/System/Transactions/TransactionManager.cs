//-----------------------------------------------------------------------------
// <copyright file="TransactionManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Configuration;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Transactions.Configuration;
using System.Transactions.Diagnostics;
using System.Xml;
using System.Xml.Serialization;

[assembly:BestFitMapping(false)]
#pragma warning disable 618
[assembly:SecurityPermissionAttribute(SecurityAction.RequestMinimum)]
#pragma warning restore 618

// No external data is ever passed to PrivilegedConfigurationManager.GetSection
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", 
            Scope = "member", 
            Target = "System.Configuration.PrivilegedConfigurationManager.GetSection(System.String):System.Object")]

namespace System.Transactions
{
    public delegate Transaction HostCurrentTransactionCallback();

    public delegate void TransactionStartedEventHandler(object sender, TransactionEventArgs e);

    public static class TransactionManager
    {
        // Revovery Information Version
        private const Int32 recoveryInformationVersion1 = 1;
        private const Int32 currentRecoveryVersion = recoveryInformationVersion1;
        
        // Record if the platform has been validated.
        internal static bool _platformValidated;

        // Hashtable of promoted transactions, keyed by identifier guid.  This is used by
        // FindPromotedTransaction to support transaction equivalence when a transaction is
        // serialized and then deserialized back in this app-domain.
        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile Hashtable promotedTransactionTable;

        // Sorted Table of transaction timeouts
        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile TransactionTable transactionTable;

        private static TransactionStartedEventHandler distributedTransactionStartedDelegate;
        public static event TransactionStartedEventHandler DistributedTransactionStarted
        {
            [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
            add
            {
                if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

                lock ( ClassSyncObject )
                {
                    TransactionManager.distributedTransactionStartedDelegate = (TransactionStartedEventHandler)System.Delegate.Combine(TransactionManager.distributedTransactionStartedDelegate, value);
                    if ( value != null )
                    {
                        ProcessExistingTransactions( value );
                    }
                }
            }

            [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
            remove
            {
                if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

                lock ( ClassSyncObject )
                {
                    TransactionManager.distributedTransactionStartedDelegate = (TransactionStartedEventHandler)System.Delegate.Remove (TransactionManager.distributedTransactionStartedDelegate, value);
                }
            }
        }


        internal static void ProcessExistingTransactions( TransactionStartedEventHandler eventHandler )
        {
            lock ( PromotedTransactionTable )
            {
                foreach ( DictionaryEntry entry in PromotedTransactionTable )
                {
                    WeakReference weakRef = (WeakReference)entry.Value;
                    Transaction tx = (Transaction)weakRef.Target;
                    if ( tx != null )
                    {
                        TransactionEventArgs args = new TransactionEventArgs();
                        args.transaction = tx.InternalClone();
                        eventHandler( args.transaction, args );
                    }
                }
            }
        }


        internal static void FireDistributedTransactionStarted( Transaction transaction )
        {
            TransactionStartedEventHandler localStartedEventHandler = null;
            lock ( ClassSyncObject )
            {
                localStartedEventHandler = TransactionManager.distributedTransactionStartedDelegate;
            }

            if ( null != localStartedEventHandler )
            {
                TransactionEventArgs args = new TransactionEventArgs();
                args.transaction = transaction.InternalClone();
                localStartedEventHandler(args.transaction, args);
            }
        }


        // Data storage for current delegate
        internal static HostCurrentTransactionCallback currentDelegate = null;
        internal static bool currentDelegateSet = false;

        // CurrentDelegate
        //
        // Store a delegate to be used to query for an external current transaction.
        public static HostCurrentTransactionCallback HostCurrentCallback
        {
            // get_HostCurrentCallback is used from get_CurrentTransaction, which doesn't have any permission requirements.
            // We don't expose what is returned from this property in that case.  But we don't want just anybody being able
            // to retrieve the value.
            [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
            get
            {
                if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

                // Note do not add trace notifications to this method.  It is called
                // at the startup of SQLCLR and tracing has too much working set overhead.
                return currentDelegate;
            }

            [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
            set
            {
                if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

                // Note do not add trace notifications to this method.  It is called
                // at the startup of SQLCLR and tracing has too much working set overhead.
                if ( value == null )
                {
                    throw new ArgumentNullException( "value" );
                }

                lock ( ClassSyncObject )
                {
                    if ( currentDelegateSet )
                    {
                        throw new InvalidOperationException( SR.GetString( SR.CurrentDelegateSet ));
                    }
                    currentDelegateSet = true;
                }

                currentDelegate = value;
            }
        }


        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        static public Enlistment Reenlist(
            Guid resourceManagerIdentifier,
            byte[] recoveryInformation,
            IEnlistmentNotification enlistmentNotification
            )
        {
            if ( resourceManagerIdentifier == Guid.Empty )
            {
                throw new ArgumentException( SR.GetString( SR.BadResourceManagerId ), "resourceManagerIdentifier" );
            }

            if ( null == recoveryInformation )
            {
                throw new ArgumentNullException( "recoveryInformation" );
            }

            if ( null == enlistmentNotification )
            {
                throw new ArgumentNullException( "enlistmentNotification" );
            }

            if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionManager.Reenlist"
                    );
            }

            if ( DiagnosticTrace.Information )
            {
                ReenlistTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    resourceManagerIdentifier
                    );
            }

            // Put the recovery information into a stream.
            MemoryStream stream = new MemoryStream( recoveryInformation );
            int recoveryInformationVersion = 0;
            string nodeName = null;
            byte[] resourceManagerRecoveryInformation = null;
            
            try
            {
                BinaryReader reader = new BinaryReader( stream );
                recoveryInformationVersion = reader.ReadInt32();

                if ( recoveryInformationVersion == TransactionManager.recoveryInformationVersion1 )
                {
                    nodeName = reader.ReadString();

                    resourceManagerRecoveryInformation = reader.ReadBytes( recoveryInformation.Length - checked((int)stream.Position) );
                }
                else
                {
                    if ( DiagnosticTrace.Error )
                    {
                        TransactionExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                            SR.GetString( SR.UnrecognizedRecoveryInformation )
                            );
                    }
                    throw new ArgumentException( SR.GetString( SR.UnrecognizedRecoveryInformation ), "recoveryInformation" );
                }
            }
            catch ( System.IO.EndOfStreamException e )
            {
                if ( DiagnosticTrace.Error )
                {
                    TransactionExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        SR.GetString( SR.UnrecognizedRecoveryInformation )
                        );
                }
                throw new ArgumentException( SR.GetString( SR.UnrecognizedRecoveryInformation ), "recoveryInformation", e);
            }
            catch ( System.FormatException e )
            {
                if ( DiagnosticTrace.Error )
                {
                    TransactionExceptionTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        SR.GetString( SR.UnrecognizedRecoveryInformation )
                        );
                }
                throw new ArgumentException( SR.GetString( SR.UnrecognizedRecoveryInformation ), "recoveryInformation", e);
            }
            finally
            {
                stream.Close();
            }

            Oletx.OletxTransactionManager transactionManager = CheckTransactionManager(nodeName);

            // Now ask the Transaction Manager to reenlist.
            object syncRoot = new object();
            Enlistment returnValue = new Enlistment( enlistmentNotification, syncRoot );
            EnlistmentState._EnlistmentStatePromoted.EnterState( returnValue.InternalEnlistment );

            returnValue.InternalEnlistment.PromotedEnlistment = 
                transactionManager.ReenlistTransaction(
                    resourceManagerIdentifier,
                    resourceManagerRecoveryInformation,
                    (RecoveringInternalEnlistment)returnValue.InternalEnlistment
                    );

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionManager.Reenlist"
                    );
            }

            return returnValue;
        }


        private static Oletx.OletxTransactionManager CheckTransactionManager(string nodeName)
        {
            Oletx.OletxTransactionManager tm = DistributedTransactionManager;
            if ( !((tm.NodeName == null && (nodeName == null || nodeName.Length == 0)) || 
                  (tm.NodeName != null && tm.NodeName.Equals(nodeName))) )
            {
                throw new ArgumentException( SR.GetString(SR.InvalidRecoveryInformation), "recoveryInformation" );
            }
            return tm;
        }


        [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name = "FullTrust")]
        static public void RecoveryComplete(
            Guid resourceManagerIdentifier
            )
        {
            if ( resourceManagerIdentifier == Guid.Empty )
            {
                throw new ArgumentException( SR.GetString( SR.BadResourceManagerId ), "resourceManagerIdentifier" );
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionManager.RecoveryComplete"
                    );
            }

            if ( DiagnosticTrace.Information )
            {
                RecoveryCompleteTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    resourceManagerIdentifier
                    );
            }

            DistributedTransactionManager.ResourceManagerRecoveryComplete( resourceManagerIdentifier );

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionManager.RecoveryComplete"
                    );
            }
        }

        
        // Object for synchronizing access to the entire class( avoiding lock( typeof( ... )) )
        private static object classSyncObject;
        
        // Helper object for static synchronization
        static object ClassSyncObject
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


        internal static System.Transactions.IsolationLevel DefaultIsolationLevel
        {
            get
            {
                if ( DiagnosticTrace.Verbose )
                {
                    MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionManager.get_DefaultIsolationLevel"
                        );
                    MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionManager.get_DefaultIsolationLevel"
                        );
                }

                return IsolationLevel.Serializable;
            }
        }


        static DefaultSettingsSection defaultSettings;
        static DefaultSettingsSection DefaultSettings
        {
            get
            {
                if ( defaultSettings == null )
                {
                    defaultSettings = DefaultSettingsSection.GetSection();
                }

                return defaultSettings;
            }
        }


        static MachineSettingsSection machineSettings;
        static MachineSettingsSection MachineSettings
        {
            get
            {
                if ( machineSettings == null )
                {
                    machineSettings = MachineSettingsSection.GetSection();
                }

                return machineSettings;
            }
        }


        private static bool _defaultTimeoutValidated;
        private static TimeSpan _defaultTimeout;
        public static TimeSpan DefaultTimeout
        {
            get
            {
                if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

                if ( DiagnosticTrace.Verbose )
                {
                    MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionManager.get_DefaultTimeout"
                        );
                }

                if ( !_defaultTimeoutValidated )
                {
                    _defaultTimeout = ValidateTimeout( DefaultSettings.Timeout );
                    // If the timeout value got adjusted, it must have been greater than MaximumTimeout.
                    if ( _defaultTimeout != DefaultSettings.Timeout )
                    {
                        if ( DiagnosticTrace.Warning )
                        {
                            ConfiguredDefaultTimeoutAdjustedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ) );
                        }
                    }
                    _defaultTimeoutValidated = true;
                }

                if ( DiagnosticTrace.Verbose )
                {
                    MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionManager.get_DefaultTimeout"
                        );
                }
                return _defaultTimeout;
            }
        }


        // Double-checked locking pattern requires volatile for read/write synchronization
        private static volatile bool _cachedMaxTimeout;
        private static TimeSpan _maximumTimeout;
        public static TimeSpan MaximumTimeout
        {
            get
            {
                if ( !TransactionManager._platformValidated ) TransactionManager.ValidatePlatform();

                if ( DiagnosticTrace.Verbose )
                {
                    MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionManager.get_DefaultMaximumTimeout"
                        );
                }

                if ( !_cachedMaxTimeout )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( !_cachedMaxTimeout )
                        {
                            TimeSpan temp = MachineSettings.MaxTimeout;
                            _maximumTimeout = temp;
                            _cachedMaxTimeout = true;
                        }
                    }
                }

                if ( DiagnosticTrace.Verbose )
                {
                    MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                        "TransactionManager.get_DefaultMaximumTimeout"
                        );
                }

                return _maximumTimeout;
            }
        }


        // This routine writes the "header" for the recovery information, based on the
        // type of the calling object and its provided parameter collection.  This information
        // we be read back by the static Reenlist method to create the necessary transaction
        // manager object with the right parameters in order to do a ReenlistTransaction call.
        internal static byte[] GetRecoveryInformation(
            string startupInfo,
            byte[] resourceManagerRecoveryInformation
            )
        {
            if ( DiagnosticTrace.Verbose )
            {
                MethodEnteredTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionManager.GetRecoveryInformation"
                    );
            }

            MemoryStream stream = new MemoryStream();
            byte[] returnValue = null;

            try
            {
                // Manually write the recovery information
                BinaryWriter writer = new BinaryWriter( stream );

                writer.Write( TransactionManager.currentRecoveryVersion );
                if ( startupInfo != null )
                {
                    writer.Write( startupInfo );
                }
                else
                {
                    writer.Write( "" );
                }
                writer.Write( resourceManagerRecoveryInformation );
                writer.Flush();
                returnValue = stream.ToArray();
            }
            finally
            {
                stream.Close();
            }

            if ( DiagnosticTrace.Verbose )
            {
                MethodExitedTraceRecord.Trace( SR.GetString( SR.TraceSourceBase ),
                    "TransactionManager.GetRecoveryInformation"
                    );
            }
 
            return returnValue;
        }

        internal static byte[] ConvertToByteArray( object thingToConvert )
        {
            MemoryStream streamToWrite = new MemoryStream();
            byte[] returnValue = null;
            
            try
            {
                // First seralize the type to the stream.
                IFormatter formatter = new BinaryFormatter();
                formatter.Serialize( streamToWrite, thingToConvert );
    
                returnValue = new byte[streamToWrite.Length];
    
                streamToWrite.Position = 0;
                streamToWrite.Read( returnValue, 0, Convert.ToInt32( streamToWrite.Length, CultureInfo.InvariantCulture ) );
            }
            finally
            {
                streamToWrite.Close();
            }

            return returnValue;
       }
        
        /// <summary>
        /// This static function throws an ArgumentOutOfRange if the specified IsolationLevel is not within
        /// the range of valid values.
        /// </summary>
        /// <param name="transactionIsolationLevel">
        /// The IsolationLevel value to validate.
        /// </param>
        internal static void ValidateIsolationLevel( IsolationLevel transactionIsolationLevel )
        {
            switch ( transactionIsolationLevel )
            {
                case IsolationLevel.Serializable:
                case IsolationLevel.RepeatableRead:
                case IsolationLevel.ReadCommitted:
                case IsolationLevel.ReadUncommitted:
                case IsolationLevel.Unspecified:
                case IsolationLevel.Chaos:
                case IsolationLevel.Snapshot:
                    break;
                default:
                {
                    throw new ArgumentOutOfRangeException( "transactionIsolationLevel" );
                }
            }
        }


        /// <summary>
        /// This static function throws an ArgumentOutOfRange if the specified TimeSpan does not meet
        /// requirements of a valid transaction timeout.  Timeout values must be positive.
        /// </summary>
        /// <param name="transactionTimeout">
        /// The TimeSpan value to validate.
        /// </param>
        internal static TimeSpan ValidateTimeout( TimeSpan transactionTimeout )
        {
            if ( transactionTimeout < TimeSpan.Zero )
            {
                throw new ArgumentOutOfRangeException( "transactionTimeout" );
            }

            if ( TransactionManager.MaximumTimeout != TimeSpan.Zero )
            {
                if ( transactionTimeout > TransactionManager.MaximumTimeout || transactionTimeout == TimeSpan.Zero )
                {
                    return TransactionManager.MaximumTimeout;
                }
            }

            return transactionTimeout;
        }


        internal static Transaction FindPromotedTransaction(
            Guid transactionIdentifier
            )
        {
            Hashtable promotedTransactionTable = PromotedTransactionTable;
                WeakReference weakRef = (WeakReference) promotedTransactionTable[transactionIdentifier];
                if ( null != weakRef )
                {
                Transaction tx = weakRef.Target as Transaction;
                    if ( null != tx )
                    {
                        return tx.InternalClone();
                    }
                    else  // an old, moldy weak reference.  Let's get rid of it.
                    {
                        lock ( promotedTransactionTable )
                        {
                            promotedTransactionTable.Remove( transactionIdentifier );
                        }
                    }
                }

            return null;
        }


        internal static Transaction FindOrCreatePromotedTransaction(
            Guid transactionIdentifier,
            Oletx.OletxTransaction oletx
            )
        {
            Transaction tx = null;
            Hashtable promotedTransactionTable = PromotedTransactionTable;
            lock (promotedTransactionTable)
            {
                WeakReference weakRef = (WeakReference) promotedTransactionTable[transactionIdentifier];
                if ( null != weakRef )
                {
                    tx = weakRef.Target as Transaction;
                    if ( null != tx )
                    {
                        // If we found a transaction then dispose the oletx
                        oletx.Dispose();
                        return tx.InternalClone();
                    }
                    else  // an old, moldy weak reference.  Let's get rid of it.
                    {
                        lock ( promotedTransactionTable )
                        {
                            promotedTransactionTable.Remove( transactionIdentifier );
                        }
                    }
                }

                tx = new Transaction( oletx );

                // Since we are adding this reference to the table create an object that will clean that
                // entry up.
                tx.internalTransaction.finalizedObject = new FinalizedObject( tx.internalTransaction, oletx.Identifier );

                weakRef = new WeakReference( tx, false );
                promotedTransactionTable[oletx.Identifier] = weakRef;
            }
            oletx.savedLtmPromotedTransaction = tx;

            TransactionManager.FireDistributedTransactionStarted( tx );

            return tx;
        }


        // Table for promoted transactions
        internal static Hashtable PromotedTransactionTable
        {
            get
            {
                if ( promotedTransactionTable == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( promotedTransactionTable == null )
                        {
                            Hashtable temp = new Hashtable( 100 );
                            promotedTransactionTable = temp;
                        }
                    }
                }

                return promotedTransactionTable;
            }
        }


        // Table for transaction timeouts
        internal static TransactionTable TransactionTable
        {
            get
            {
                if ( transactionTable == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( transactionTable == null )
                        {
                            TransactionTable temp = new TransactionTable();
                            transactionTable = temp;
                        }
                    }
                }

                return transactionTable;
            }
        }


        // Fault in a DistributedTransactionManager if one has not already been created.
        // Double-checked locking pattern requires volatile for read/write synchronization
        internal static volatile Oletx.OletxTransactionManager distributedTransactionManager;
        internal static Oletx.OletxTransactionManager DistributedTransactionManager
        {
            get
            {
                // If the distributed transaction manager is not configured, throw an exception
                if ( distributedTransactionManager == null )
                {
                    lock ( ClassSyncObject )
                    {
                        if ( distributedTransactionManager == null )
                        {
                            Oletx.OletxTransactionManager temp = new Oletx.OletxTransactionManager( 
                                DefaultSettings.DistributedTransactionManagerName);
                            distributedTransactionManager = temp;
                        }
                    }
                }

                return distributedTransactionManager;
            }
        }


        internal static void ValidatePlatform()
        {
            if ( PlatformID.Win32NT != Environment.OSVersion.Platform )
            {
                throw new PlatformNotSupportedException( SR.GetString( SR.OnlySupportedOnWinNT ) );
            }

            // Note that this is purposly not synchronized because who cares if there is a ---- to set it.
            _platformValidated = true;       
        }

    }
}
