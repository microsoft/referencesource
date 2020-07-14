//------------------------------------------------------------------------------
// <copyright file="MessageQueueErrorCode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>    
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1008:EnumsShouldHaveZeroValue")]
    public enum MessageQueueErrorCode
    {

        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.Base"]/*' />
        /// <internalonly/>
        Base = unchecked((int)0xC00E0000),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.Generic"]/*' />
        /// <devdoc>
        ///     GenericError.
        /// </devdoc>
        Generic = unchecked((int)0xC00E0001),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.Property"]/*' />
        /// <internalonly/>
        Property = unchecked((int)0xC00E0002),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.QueueNotFound"]/*' />
        /// <devdoc>
        ///     The queue is not registered in the DS.
        /// </devdoc>
        QueueNotFound = unchecked((int)0xC00E0003),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.QueueExists"]/*' />
        /// <devdoc>
        ///     A queue with the same pathname is already registered.
        /// </devdoc>
        QueueExists = unchecked((int)0xC00E0005),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.InvalidParameter"]/*' />
        /// <internalonly/>
        InvalidParameter = unchecked((int)0xC00E0006),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.InvalidHandle"]/*' />
        /// <internalonly/>
        InvalidHandle = unchecked((int)0xC00E0007),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.OperationCanceled"]/*' />
        /// <devdoc>
        ///     The operation was cancelled before it could be completed.
        /// </devdoc>
        OperationCanceled = unchecked((int)0xC00E0008),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.SharingViolation"]/*' />
        /// <devdoc>
        ///     Sharing violation. The queue is already opened for
        ///     exclusive receive.
        /// </devdoc>
        SharingViolation = unchecked((int)0xC00E0009),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.ServiceNotAvailable"]/*' />
        /// <devdoc>
        ///     The Message Queues service is not available.
        /// </devdoc>
        ServiceNotAvailable = unchecked((int)0xC00E000B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MachineNotFound"]/*' />
        /// <devdoc>
        ///     The specified machine could not be found.
        /// </devdoc>
        MachineNotFound = unchecked((int)0xC00E000D),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalSort"]/*' />
        /// <internalonly/>
        IllegalSort = unchecked((int)0xC00E0010),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalUser"]/*' />
        /// <devdoc>
        ///     The user is an illegal user.
        /// </devdoc>
        IllegalUser = unchecked((int)0xC00E0011),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoDs"]/*' />
        /// <devdoc>
        ///     No connection with this site's controller(s).
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        NoDs = unchecked((int)0xC00E0013),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalQueuePathName"]/*' />
        /// <devdoc>
        ///     Illegal queue path name.
        /// </devdoc>
        IllegalQueuePathName = unchecked((int)0xC00E0014),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalPropertyValue"]/*' />
        /// <devdoc>
        ///     Illegal property value.
        /// </devdoc>
        IllegalPropertyValue = unchecked((int)0xC00E0018),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalPropertyVt"]/*' />
        /// <internalonly/>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        IllegalPropertyVt = unchecked((int)0xC00E0019),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.BufferOverflow"]/*' />
        /// <internalonly/>
        BufferOverflow = unchecked((int)0xC00E001A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IOTimeout"]/*' />
        /// <devdoc>
        ///     The Receive or Peek Message timeout has expired.
        /// </devdoc>
        IOTimeout = unchecked((int)0xC00E001B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalCursorAction"]/*' />
        /// <internalonly/>
        IllegalCursorAction = unchecked((int)0xC00E001C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MessageAlreadyReceived"]/*' />
        /// <devdoc>
        ///     A message that is currently pointed at by the cursor has been removed from
        ///     the queue by another process or by another call to MQReceiveMessage
        ///     without the use of this cursor.
        /// </devdoc>
        MessageAlreadyReceived = unchecked((int)0xC00E001D),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalFormatName"]/*' />
        /// <devdoc>
        ///     The given format name is invalid.
        /// </devdoc>
        IllegalFormatName = unchecked((int)0xC00E001E),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.FormatNameBufferTooSmall"]/*' />
        /// <internalonly/>
        FormatNameBufferTooSmall = unchecked((int)0xC00E001F),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.UnsupportedFormatNameOperation"]/*' />
        /// <devdoc>
        ///     The requested operation for the specified format name is not
        ///     supported (e.g., delete a direct queue format name).
        /// </devdoc>
        UnsupportedFormatNameOperation = unchecked((int)0xC00E0020),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalSecurityDescriptor"]/*' />
        /// <internalonly/>
        IllegalSecurityDescriptor = unchecked((int)0xC00E0021),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.SenderIdBufferTooSmall"]/*' />
        /// <internalonly/>
        SenderIdBufferTooSmall = unchecked((int)0xC00E0022),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.SecurityDescriptorBufferTooSmall"]/*' />
        /// <internalonly/>
        SecurityDescriptorBufferTooSmall = unchecked((int)0xC00E0023),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotImpersonateClient"]/*' />
        /// <devdoc>
        ///     The RPC server can not impersonate the client application, hence security
        ///     credentials could not be verified.
        /// </devdoc>
        CannotImpersonateClient = unchecked((int)0xC00E0024),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.AccessDenied"]/*' />
        /// <devdoc>
        ///     Access is denied.
        /// </devdoc>
        AccessDenied = unchecked((int)0xC00E0025),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.PrivilegeNotHeld"]/*' />
        /// <devdoc>
        ///     Client does not have the required privileges to perform the operation.
        /// </devdoc>
        PrivilegeNotHeld = unchecked((int)0xC00E0026),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.InsufficientResources"]/*' />
        /// <devdoc>
        ///     Insufficient resources to perform operation.
        /// </devdoc>
        InsufficientResources = unchecked((int)0xC00E0027),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.UserBufferTooSmall"]/*' />
        /// <internalonly/>
        UserBufferTooSmall = unchecked((int)0xC00E0028),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MessageStorageFailed"]/*' />
        /// <devdoc>
        ///     Could not store a recoverable or journal message. Message was not sent.
        /// </devdoc>
        MessageStorageFailed = unchecked((int)0xC00E002A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.SenderCertificateBufferTooSmall"]/*' />
        /// <internalonly/>
        SenderCertificateBufferTooSmall = unchecked((int)0xC00E002B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.InvalidCertificate"]/*' />
        /// <devdoc>
        ///     The user certificate is not valid.
        /// </devdoc>
        InvalidCertificate = unchecked((int)0xC00E002C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CorruptedInternalCertificate"]/*' />
        /// <devdoc>
        ///     The internal MSMQ certificate is corrupted.
        /// </devdoc>
        CorruptedInternalCertificate = unchecked((int)0xC00E002D),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoInternalUserCertificate"]/*' />
        /// <devdoc>
        ///     The internal MSMQ certificate for the user does not exist.
        /// </devdoc>
        NoInternalUserCertificate = unchecked((int)0xC00E002F),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CorruptedSecurityData"]/*' />
        /// <devdoc>
        ///     A cryptogrphic function has failed.
        /// </devdoc>
        CorruptedSecurityData = unchecked((int)0xC00E0030),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CorruptedPersonalCertStore"]/*' />
        /// <devdoc>
        ///     The personal certificate store is corrupted.
        /// </devdoc>
        CorruptedPersonalCertStore = unchecked((int)0xC00E0031),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.ComputerDoesNotSupportEncryption"]/*' />
        /// <devdoc>
        ///     The computer does not support encryption operations.
        /// </devdoc>
        ComputerDoesNotSupportEncryption = unchecked((int)0xC00E0033),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.BadSecurityContext"]/*' />
        /// <internalonly/>
        BadSecurityContext = unchecked((int)0xC00E0035),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CouldNotGetUserSid"]/*' />
        /// <devdoc>
        ///     Could not get the SID information out of the thread token.
        /// </devdoc>
        CouldNotGetUserSid = unchecked((int)0xC00E0036),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CouldNotGetAccountInfo"]/*' />
        /// <devdoc>
        ///     Could not get the account information for the user.
        /// </devdoc>        
        CouldNotGetAccountInfo = unchecked((int)0xC00E0037),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalCriteriaColumns"]/*' />
        /// <internalonly/>
        IllegalCriteriaColumns = unchecked((int)0xC00E0038),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalPropertyId"]/*' />
        /// <internalonly/>
        IllegalPropertyId = unchecked((int)0xC00E0039),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalRelation"]/*' />
        /// <internalonly/>
        IllegalRelation = unchecked((int)0xC00E003A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalPropertySize"]/*' />
        /// <internalonly/>
        IllegalPropertySize = unchecked((int)0xC00E003B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalRestrictionPropertyId"]/*' />
        /// <internalonly/>
        IllegalRestrictionPropertyId = unchecked((int)0xC00E003C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalQueueProperties"]/*' />
        /// <internalonly/>
        IllegalQueueProperties = unchecked((int)0xC00E003D),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.PropertyNotAllowed"]/*' />
        /// <devdoc>
        ///     Invalid property for the requested operation.
        /// </devdoc>
        PropertyNotAllowed = unchecked((int)0xC00E003E),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.InsufficientProperties"]/*' />
        /// <devdoc>
        ///     Not all the required properties for the operation were specified
        ///     in the input parameters.
        /// </devdoc>
        InsufficientProperties = unchecked((int)0xC00E003F),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MachineExists"]/*' />
        /// <devdoc>
        ///     Computer with the same name already exists in the site.
        /// </devdoc>
        MachineExists = unchecked((int)0xC00E0040),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalMessageProperties"]/*' />
        /// <internalonly/>
        IllegalMessageProperties = unchecked((int)0xC00E0041),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.DsIsFull"]/*' />
        /// <devdoc>
        ///     DS is full.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        DsIsFull = unchecked((int)0xC00E0042),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.DsError"]/*' />
        /// <devdoc>
        ///     internal DS error.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        DsError = unchecked((int)0xC00E0043),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.InvalidOwner"]/*' />
        /// <devdoc>
        ///     Invalid object owner. For example CreateQueue failed because the Queue Manager
        ///     object is invalid.
        /// </devdoc>
        InvalidOwner = unchecked((int)0xC00E0044),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.UnsupportedAccessMode"]/*' />
        /// <devdoc>
        ///     The specified access mode is not supported.
        /// </devdoc>
        UnsupportedAccessMode = unchecked((int)0xC00E0045),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.ResultBufferTooSmall"]/*' />
        /// <internalonly/>
        ResultBufferTooSmall = unchecked((int)0xC00E0046),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.DeleteConnectedNetworkInUse"]/*' />
        /// <devdoc>
        ///     The Connected Network can not be deleted, it is in use.
        /// </devdoc>
        DeleteConnectedNetworkInUse = unchecked((int)0xC00E0048),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoResponseFromObjectServer"]/*' />
        /// <devdoc>
        ///     No response from object owner.
        /// </devdoc>
        NoResponseFromObjectServer = unchecked((int)0xC00E0049),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.ObjectServerNotAvailable"]/*' />
        /// <devdoc>
        ///     Object owner is not reachable.
        /// </devdoc>
        ObjectServerNotAvailable = unchecked((int)0xC00E004A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.QueueNotAvailable"]/*' />
        /// <devdoc>
        ///     Error while reading from a queue residing on a remote computer.
        /// </devdoc>
        QueueNotAvailable = unchecked((int)0xC00E004B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.DtcConnect"]/*' />
        /// <devdoc>
        ///     Cannot connect to MS DTC.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        DtcConnect = unchecked((int)0xC00E004C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.TransactionImport"]/*' />
        /// <devdoc>
        ///     Cannot import the transaction.
        /// </devdoc>
        TransactionImport = unchecked((int)0xC00E004E),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.TransactionUsage"]/*' />
        /// <devdoc>
        ///     Wrong transaction usage.
        /// </devdoc>
        TransactionUsage = unchecked((int)0xC00E0050),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.TransactionSequence"]/*' />
        /// <devdoc>
        ///     Wrong transaction operations sequence.
        /// </devdoc>
        TransactionSequence = unchecked((int)0xC00E0051),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MissingConnectorType"]/*' />
        /// <devdoc>
        ///     Connector Type is mandatory when sending Acknowledgment or secure message.
        /// </devdoc>
        MissingConnectorType = unchecked((int)0xC00E0055),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.StaleHandle"]/*' />
        /// <internalonly/>
        StaleHandle = unchecked((int)0xC00E0056),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.TransactionEnlist"]/*' />
        /// <devdoc>
        ///     Cannot enlist the transaction.
        /// </devdoc>
        TransactionEnlist = unchecked((int)0xC00E0058),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.QueueDeleted"]/*' />
        /// <devdoc>
        ///     The queue was deleted. Messages can not be received anymore using this
        ///     queue instance. The queue should be closed.
        /// </devdoc>
        QueueDeleted = unchecked((int)0xC00E005A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalContext"]/*' />
        /// <devdoc>
        ///     Invalid context parameter.
        /// </devdoc>
        IllegalContext = unchecked((int)0xC00E005B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalSortPropertyId"]/*' />
        /// <internalonly/>
        IllegalSortPropertyId = unchecked((int)0xC00E005C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.LabelBufferTooSmall"]/*' />
        /// <internalonly/>
        LabelBufferTooSmall = unchecked((int)0xC00E005E),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MqisServerEmpty"]/*' />
        /// <devdoc>
        ///     The list of MQIS servers (in registry) is empty.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        MqisServerEmpty = unchecked((int)0xC00E005F),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MqisReadOnlyMode"]/*' />
        /// <devdoc>
        ///     MQIS database is in read-only mode.
        /// </devdoc>        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        MqisReadOnlyMode = unchecked((int)0xC00E0060),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.SymmetricKeyBufferTooSmall"]/*' />
        /// <internalonly/>
        SymmetricKeyBufferTooSmall = unchecked((int)0xC00E0061),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.SignatureBufferTooSmall"]/*' />
        /// <internalonly/>
        SignatureBufferTooSmall = unchecked((int)0xC00E0062),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.ProviderNameBufferTooSmall"]/*' />
        /// <internalonly/>
        ProviderNameBufferTooSmall = unchecked((int)0xC00E0063),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalOperation"]/*' />
        /// <devdoc>
        ///     The operation is illegal on foreign message queuing system.
        /// </devdoc>
        IllegalOperation = unchecked((int)0xC00E0064),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.WriteNotAllowed"]/*' />
        /// <devdoc>
        ///     Another MQIS server is being installed, write operations to the
        ///     database are not allowed at this stage.
        /// </devdoc>
        WriteNotAllowed = unchecked((int)0xC00E0065),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.WksCantServeClient"]/*' />
        /// <devdoc>
        ///     MSMQ independent clients cannot serve MSMQ dependent clients.
        /// </devdoc>
        WksCantServeClient = unchecked((int)0xC00E0066),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.DependentClientLicenseOverflow"]/*' />
        /// <devdoc>
        ///     The number of dependent clients served by this MSMQ server reached
        ///     its upper limit.
        /// </devdoc>
        DependentClientLicenseOverflow = unchecked((int)0xC00E0067),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CorruptedQueueWasDeleted"]/*' />
        /// <devdoc>
        ///     Ini file for queue in LQS was deleted because it was corrupted.
        /// </devdoc>
        CorruptedQueueWasDeleted = unchecked((int)0xC00E0068),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.RemoteMachineNotAvailable"]/*' />
        /// <devdoc>
        ///     The remote machine is not available.
        /// </devdoc>
        RemoteMachineNotAvailable = unchecked((int)0xC00E0069),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.UnsupportedOperation"]/*' />
        /// <devdoc>
        ///  The operation is not supported for a WORKGROUP installation computer.
        /// </devdoc>
        UnsupportedOperation = unchecked((int)0xC00E006A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.EncryptionProviderNotSupported"]/*' />
        /// <devdoc>
        ///  The Cryptographic Service Provider  is not supported by Message Queuing.
        /// </devdoc>
        EncryptionProviderNotSupported = unchecked((int)0xC00E006B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotSetCryptographicSecurityDescriptor"]/*' />
        /// <devdoc>
        ///  Unable to set the security descriptor for the cryptographic keys.
        /// </devdoc>
        CannotSetCryptographicSecurityDescriptor = unchecked((int)0xC00E006C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CertificateNotProvided"]/*' />
        /// <devdoc>
        ///  A user attempted  to send an authenticated message without a certificate.
        /// </devdoc>
        CertificateNotProvided = unchecked((int)0xC00E006D),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.QDnsPropertyNotSupported"]/*' />
        /// <internalonly/>
        QDnsPropertyNotSupported = unchecked((int)0xC00E006E),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotCreateCertificateStore"]/*' />
        /// <devdoc>
        ///  Unable to create a certificate store for the internal certificate.
        /// </devdoc>
        CannotCreateCertificateStore = unchecked((int)0xC00E006F),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotOpenCertificateStore"]/*' />
        /// <devdoc>
        ///  Unable to  open the certificates store for the internal certificate.
        /// </devdoc>
        CannotOpenCertificateStore = unchecked((int)0xC00E0070),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalEnterpriseOperation"]/*' />
        /// <devdoc>
        ///  The operation is invalid for a msmqServices object.
        /// </devdoc>
        IllegalEnterpriseOperation = unchecked((int)0xC00E0071),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotGrantAddGuid"]/*' />
        /// <devdoc>
        ///  Failed to grant the "Add Guid" permission to current user.
        /// </devdoc>
        CannotGrantAddGuid = unchecked((int)0xC00E0072),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotLoadMsmqOcm"]/*' />
        /// <devdoc>
        ///  Can't load the MSMQOCM.DLL library.
        /// </devdoc>        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        CannotLoadMsmqOcm = unchecked((int)0xC00E0073),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoEntryPointMsmqOcm"]/*' />
        /// <devdoc>
        ///  Cannot locate an entry point in the MSMQOCM.DLL library.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        NoEntryPointMsmqOcm = unchecked((int)0xC00E0074),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoMsmqServersOnDc"]/*' />
        /// <devdoc>
        ///  Failed to find Message Queuing servers on domain controllers.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        NoMsmqServersOnDc = unchecked((int)0xC00E0075),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotJoinDomain"]/*' />
        /// <devdoc>
        ///  Failed to join MSMQ enterprise on Windows 2000 domain.
        /// </devdoc>
        CannotJoinDomain = unchecked((int)0xC00E0076),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotCreateOnGlobalCatalog"]/*' />
        /// <devdoc>
        ///  Failed to create an object on a specified Global Catalog server.
        /// </devdoc>
        CannotCreateOnGlobalCatalog = unchecked((int)0xC00E0077),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.GuidNotMatching"]/*' />
        /// <devdoc>
        ///  Failed to create msmqConfiguration object with GUID that match machine installation. You must uninstall MSMQ and then reinstall it.
        /// </devdoc>
        GuidNotMatching = unchecked((int)0xC00E0078),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.PublicKeyNotFound"]/*' />
        /// <devdoc>
        ///  Unable to find the public key for computer.
        /// </devdoc>
        PublicKeyNotFound = unchecked((int)0xC00E0079),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.PublicKeyDoesNotExist"]/*' />
        /// <devdoc>
        ///  The public key for the computer does not exist.
        /// </devdoc>
        PublicKeyDoesNotExist = unchecked((int)0xC00E007A),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.IllegalPrivateProperties"]/*' />
        /// <internalonly/>
        IllegalPrivateProperties = unchecked((int)0xC00E007B),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoGlobalCatalogInDomain"]/*' />
        /// <devdoc>
        ///  Unable to find Global Catalog servers in the specified domain.
        /// </devdoc>
        NoGlobalCatalogInDomain = unchecked((int)0xC00E007C),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.NoMsmqServersOnGlobalCatalog"]/*' />
        /// <devdoc>
        ///  Failed to find Message Queuing servers on Global Catalog domain controllers.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        NoMsmqServersOnGlobalCatalog = unchecked((int)0xC00E007D),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotGetDistinguishedName"]/*' />
        /// <devdoc>
        ///  Failed to retrieve the distinguished name of local computer.
        /// </devdoc>
        CannotGetDistinguishedName = unchecked((int)0xC00E007E),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotHashDataEx"]/*' />
        /// <devdoc>
        ///  Unable to hash data for an authenticated message.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        CannotHashDataEx = unchecked((int)0xC00E007F),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotSignDataEx"]/*' />
        /// <devdoc>
        ///  Unable to sign data before sending an authenticated message.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        CannotSignDataEx = unchecked((int)0xC00E0080),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.CannotCreateHashEx"]/*' />
        /// <devdoc>
        ///  Unable to create hash object for an authenticated message.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        CannotCreateHashEx = unchecked((int)0xC00E0081),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.FailVerifySignatureEx"]/*' />
        /// <devdoc>
        ///  Signature of recieved message is not valid.
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUpperCased")]
        FailVerifySignatureEx = unchecked((int)0xC00E0082),
        /// <include file='doc\MessageQueueErrorCode.uex' path='docs/doc[@for="MessageQueueErrorCode.MessageNotFound"]/*' />
        /// <devdoc>
        ///  The message referenced by the lookup identifier does not exist.
        /// </devdoc>
        MessageNotFound = unchecked((int)0xC00E0088),

    }
}
