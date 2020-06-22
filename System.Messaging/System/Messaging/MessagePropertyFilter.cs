//------------------------------------------------------------------------------
// <copyright file="MessagePropertyFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System;
    using System.Messaging.Interop;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Limits the
    ///       properties retrieved when receiving messages from a queue.
    ///    </para>
    /// </devdoc>
    [TypeConverter(typeof(ExpandableObjectConverter))]
    public class MessagePropertyFilter : ICloneable
    {
        internal const int ACKNOWLEDGEMENT = 1;
        internal const int ACKNOWLEDGE_TYPE = 1 << 2;
        internal const int ADMIN_QUEUE = 1 << 3;
        internal const int BODY = 1 << 4;
        internal const int LABEL = 1 << 5;
        internal const int ID = 1 << 6;
        internal const int USE_DEADLETTER_QUEUE = 1 << 7;
        internal const int RESPONSE_QUEUE = 1 << 8;
        internal const int MESSAGE_TYPE = 1 << 9;
        internal const int USE_JOURNALING = 1 << 10;
        internal const int LOOKUP_ID = 1 << 11;

        internal const int APP_SPECIFIC = 1;
        internal const int ARRIVED_TIME = 1 << 2;
        internal const int ATTACH_SENDER_ID = 1 << 3;
        internal const int AUTHENTICATED = 1 << 4;
        internal const int CONNECTOR_TYPE = 1 << 5;
        internal const int CORRELATION_ID = 1 << 6;
        internal const int CRYPTOGRAPHIC_PROVIDER_NAME = 1 << 7;
        internal const int CRYPTOGRAPHIC_PROVIDER_TYPE = 1 << 8;
        internal const int IS_RECOVERABLE = 1 << 9;
        internal const int DIGITAL_SIGNATURE = 1 << 10;
        internal const int ENCRYPTION_ALGORITHM = 1 << 11;
        internal const int EXTENSION = 1 << 12;
        internal const int FOREIGN_ADMIN_QUEUE = 1 << 13;
        internal const int HASH_ALGORITHM = 1 << 14;
        internal const int DESTINATION_QUEUE = 1 << 15;
        internal const int PRIORITY = 1 << 16;
        internal const int SECURITY_CONTEXT = 1 << 17;
        internal const int SENDER_CERTIFICATE = 1 << 18;
        internal const int SENDER_ID = 1 << 19;
        internal const int SENT_TIME = 1 << 20;
        internal const int SOURCE_MACHINE = 1 << 21;
        internal const int SYMMETRIC_KEY = 1 << 22;
        internal const int TIME_TO_BE_RECEIVED = 1 << 23;
        internal const int TIME_TO_REACH_QUEUE = 1 << 24;
        internal const int USE_AUTHENTICATION = 1 << 25;
        internal const int USE_ENCRYPTION = 1 << 26;
        internal const int USE_TRACING = 1 << 27;
        internal const int VERSION = 1 << 28;
        internal const int IS_FIRST_IN_TRANSACTION = 1 << 29;
        internal const int IS_LAST_IN_TRANSACTION = 1 << 30;
        internal const int TRANSACTION_ID = 1 << 31;

        internal int data1;
        internal int data2;
        private const int defaultBodySize = 1024;
        private const int defaultExtensionSize = 255;
        private const int defaultLabelSize = 255;
        internal int bodySize = defaultBodySize;
        internal int extensionSize = defaultExtensionSize;
        internal int labelSize = defaultLabelSize;


        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.MessagePropertyFilter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Messaging.MessagePropertyFilter'/>
        ///       class
        ///       and
        ///       sets
        ///       the
        ///       most
        ///       common
        ///       filter
        ///       properties
        ///       to
        ///       true.
        ///    </para>
        /// </devdoc>
        public MessagePropertyFilter()
        {
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Acknowledgment"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve
        ///    <see cref='System.Messaging.Message.Acknowledgment' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgAcknowledgement)]
        public bool Acknowledgment
        {
            get
            {
                return ((data1 & ACKNOWLEDGEMENT) != 0);
            }

            set
            {
                data1 = value ? data1 | ACKNOWLEDGEMENT : data1 & ~ACKNOWLEDGEMENT;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.AcknowledgeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.AcknowledgeType' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgAcknowledgeType)]
        public bool AcknowledgeType
        {
            get
            {
                return ((data1 & ACKNOWLEDGE_TYPE) != 0);
            }

            set
            {
                data1 = value ? data1 | ACKNOWLEDGE_TYPE : data1 & ~ACKNOWLEDGE_TYPE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.AdministrationQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.AdministrationQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgAdministrationQueue)]
        public bool AdministrationQueue
        {
            get
            {
                return ((data1 & ADMIN_QUEUE) != 0);
            }

            set
            {
                data1 = value ? data1 | ADMIN_QUEUE : data1 & ~ADMIN_QUEUE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.AppSpecific"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.AppSpecific' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgAppSpecific)]
        public bool AppSpecific
        {
            get
            {
                return ((data2 & APP_SPECIFIC) != 0);
            }

            set
            {
                data2 = value ? data2 | APP_SPECIFIC : data2 & ~APP_SPECIFIC;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.ArrivedTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.ArrivedTime' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgArrivedTime)]
        public bool ArrivedTime
        {
            get
            {
                return ((data2 & ARRIVED_TIME) != 0);
            }

            set
            {
                data2 = value ? data2 | ARRIVED_TIME : data2 & ~ARRIVED_TIME;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.AttachSenderId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.AttachSenderId' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgAttachSenderId)]
        public bool AttachSenderId
        {
            get
            {
                return ((data2 & ATTACH_SENDER_ID) != 0);
            }

            set
            {
                data2 = value ? data2 | ATTACH_SENDER_ID : data2 & ~ATTACH_SENDER_ID;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Authenticated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Authenticated' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgAuthenticated)]
        public bool Authenticated
        {
            get
            {
                return ((data2 & AUTHENTICATED) != 0);
            }

            set
            {
                data2 = value ? data2 | AUTHENTICATED : data2 & ~AUTHENTICATED;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.AuthenticationProviderName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.AuthenticationProviderName' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgAuthenticationProviderName)]
        public bool AuthenticationProviderName
        {
            get
            {
                return ((data2 & CRYPTOGRAPHIC_PROVIDER_NAME) != 0);
            }

            set
            {
                data2 = value ? data2 | CRYPTOGRAPHIC_PROVIDER_NAME : data2 & ~CRYPTOGRAPHIC_PROVIDER_NAME;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.AuthenticationProviderType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.AuthenticationProviderType' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgAuthenticationProviderType)]
        public bool AuthenticationProviderType
        {
            get
            {
                return ((data2 & CRYPTOGRAPHIC_PROVIDER_TYPE) != 0);
            }

            set
            {
                data2 = value ? data2 | CRYPTOGRAPHIC_PROVIDER_TYPE : data2 & ~CRYPTOGRAPHIC_PROVIDER_TYPE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Body"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Body' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgBody)]
        public bool Body
        {
            get
            {
                return ((data1 & BODY) != 0);
            }

            set
            {
                data1 = value ? data1 | BODY : data1 & ~BODY;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.ConnectorType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.ConnectorType' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgConnectorType)]
        public bool ConnectorType
        {
            get
            {
                return ((data2 & CONNECTOR_TYPE) != 0);
            }

            set
            {
                data2 = value ? data2 | CONNECTOR_TYPE : data2 & ~CONNECTOR_TYPE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.CorrelationId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.CorrelationId' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgCorrelationId)]
        public bool CorrelationId
        {
            get
            {
                return ((data2 & CORRELATION_ID) != 0);
            }

            set
            {
                data2 = value ? data2 | CORRELATION_ID : data2 & ~CORRELATION_ID;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.DefaultBodySize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the size,
        ///       in bytes, of the default body buffer.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(MessagePropertyFilter.defaultBodySize), MessagingDescription(Res.MsgDefaultBodySize)]
        public int DefaultBodySize
        {
            get
            {
                return this.bodySize;
            }

            set
            {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.DefaultSizeError));

                this.bodySize = value;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.DefaultExtensionSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the
        ///       size, in bytes, of the default extension buffer.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(MessagePropertyFilter.defaultExtensionSize), MessagingDescription(Res.MsgDefaultExtensionSize)]
        public int DefaultExtensionSize
        {
            get
            {
                return this.extensionSize;
            }

            set
            {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.DefaultSizeError));

                this.extensionSize = value;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.DefaultLabelSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the size,
        ///       in bytes, of the default label buffer.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(MessagePropertyFilter.defaultLabelSize), MessagingDescription(Res.MsgDefaultLabelSize)]
        public int DefaultLabelSize
        {
            get
            {
                return this.labelSize;
            }

            set
            {
                if (value < 0)
                    throw new ArgumentException(Res.GetString(Res.DefaultSizeError));

                this.labelSize = value;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.DestinationQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.DestinationQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgDestinationQueue)]
        public bool DestinationQueue
        {
            get
            {
                return ((data2 & DESTINATION_QUEUE) != 0);
            }

            set
            {
                data2 = value ? data2 | DESTINATION_QUEUE : data2 & ~DESTINATION_QUEUE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.DestinationSymmetricKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve
        ///    <see cref='System.Messaging.Message.DestinationSymmetricKey' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgDestinationSymmetricKey)]
        public bool DestinationSymmetricKey
        {
            get
            {
                return ((data2 & SYMMETRIC_KEY) != 0);
            }

            set
            {
                data2 = value ? data2 | SYMMETRIC_KEY : data2 & ~SYMMETRIC_KEY;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.DigitalSignature"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.DigitalSignature' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgDigitalSignature)]
        public bool DigitalSignature
        {
            get
            {
                return ((data2 & DIGITAL_SIGNATURE) != 0);
            }

            set
            {
                data2 = value ? data2 | DIGITAL_SIGNATURE : data2 & ~DIGITAL_SIGNATURE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.EncryptionAlgorithm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.EncryptionAlgorithm' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgEncryptionAlgorithm)]
        public bool EncryptionAlgorithm
        {
            get
            {
                return ((data2 & ENCRYPTION_ALGORITHM) != 0);
            }

            set
            {
                data2 = value ? data2 | ENCRYPTION_ALGORITHM : data2 & ~ENCRYPTION_ALGORITHM;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Extension"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Extension' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgExtension)]
        public bool Extension
        {
            get
            {
                return ((data2 & EXTENSION) != 0);
            }

            set
            {
                data2 = value ? data2 | EXTENSION : data2 & ~EXTENSION;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.HashAlgorithm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.HashAlgorithm' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgHashAlgorithm)]
        public bool HashAlgorithm
        {
            get
            {
                return ((data2 & HASH_ALGORITHM) != 0);
            }

            set
            {
                data2 = value ? data2 | HASH_ALGORITHM : data2 & ~HASH_ALGORITHM;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Id"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Id' qualify='true'/>
        ///       property information when receiving or peeking a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgId)]
        public bool Id
        {
            get
            {
                return ((data1 & ID) != 0);
            }

            set
            {
                data1 = value ? data1 | ID : data1 & ~ID;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.IsFirstInTransaction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.IsFirstInTransaction' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgIsFirstInTransaction)]
        public bool IsFirstInTransaction
        {
            get
            {
                return ((data2 & IS_FIRST_IN_TRANSACTION) != 0);
            }

            set
            {
                data2 = value ? data2 | IS_FIRST_IN_TRANSACTION : data2 & ~IS_FIRST_IN_TRANSACTION;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.IsLastInTransaction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.IsLastInTransaction' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgIsLastInTransaction)]
        public bool IsLastInTransaction
        {
            get
            {
                return ((data2 & IS_LAST_IN_TRANSACTION) != 0);
            }

            set
            {
                data2 = value ? data2 | IS_LAST_IN_TRANSACTION : data2 & ~IS_LAST_IN_TRANSACTION;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Label"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Label' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgLabel)]
        public bool Label
        {
            get
            {
                return ((data1 & LABEL) != 0);
            }

            set
            {
                data1 = value ? data1 | LABEL : data1 & ~LABEL;
            }
        }


        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.LookupId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.LookupId' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgLookupId)]
        public bool LookupId
        {
            get
            {
                if (!MessageQueue.Msmq3OrNewer)
                    throw new PlatformNotSupportedException(Res.GetString(Res.PlatformNotSupported));

                return ((data1 & LOOKUP_ID) != 0);
            }

            set
            {
                if (!MessageQueue.Msmq3OrNewer)
                    throw new PlatformNotSupportedException(Res.GetString(Res.PlatformNotSupported));

                data1 = value ? data1 | LOOKUP_ID : data1 & ~LOOKUP_ID;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.MessageType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.MessageType' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgMessageType)]
        public bool MessageType
        {
            get
            {
                return ((data1 & MESSAGE_TYPE) != 0);
            }

            set
            {
                data1 = value ? data1 | MESSAGE_TYPE : data1 & ~MESSAGE_TYPE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Priority"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Priority' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgPriority)]
        public bool Priority
        {
            get
            {
                return ((data2 & PRIORITY) != 0);
            }

            set
            {
                data2 = value ? data2 | PRIORITY : data2 & ~PRIORITY;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.Recoverable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.Recoverable' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgRecoverable)]
        public bool Recoverable
        {
            get
            {
                return ((data2 & IS_RECOVERABLE) != 0);
            }

            set
            {
                data2 = value ? data2 | IS_RECOVERABLE : data2 & ~IS_RECOVERABLE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.ResponseQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.ResponseQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgResponseQueue)]
        public bool ResponseQueue
        {
            get
            {
                return ((data1 & RESPONSE_QUEUE) != 0);
            }

            set
            {
                data1 = value ? data1 | RESPONSE_QUEUE : data1 & ~RESPONSE_QUEUE;
            }
        }

        // SecurityContext is send-only property, so there's no point in 
        // publicly exposing it in the filter
        internal bool SecurityContext
        {
            get
            {
                return ((data2 & SECURITY_CONTEXT) != 0);
            }

            set
            {
                data2 = value ? data2 | SECURITY_CONTEXT : data2 & ~SECURITY_CONTEXT;
            }
        }


        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SenderCertificate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.SenderCertificate' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgSenderCertificate)]
        public bool SenderCertificate
        {
            get
            {
                return ((data2 & SENDER_CERTIFICATE) != 0);
            }

            set
            {
                data2 = value ? data2 | SENDER_CERTIFICATE : data2 & ~SENDER_CERTIFICATE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SenderId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.SenderId' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgSenderId)]
        public bool SenderId
        {
            get
            {
                return ((data2 & SENDER_ID) != 0);
            }

            set
            {
                data2 = value ? data2 | SENDER_ID : data2 & ~SENDER_ID;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SenderVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.SenderVersion' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgSenderVersion)]
        public bool SenderVersion
        {
            get
            {
                return ((data2 & VERSION) != 0);
            }

            set
            {
                data2 = value ? data2 | VERSION : data2 & ~VERSION;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SentTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.SentTime' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgSentTime)]
        public bool SentTime
        {
            get
            {
                return ((data2 & SENT_TIME) != 0);
            }

            set
            {
                data2 = value ? data2 | SENT_TIME : data2 & ~SENT_TIME;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SourceMachine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.SourceMachine' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgSourceMachine)]
        public bool SourceMachine
        {
            get
            {
                return ((data2 & SOURCE_MACHINE) != 0);
            }

            set
            {
                data2 = value ? data2 | SOURCE_MACHINE : data2 & ~SOURCE_MACHINE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.TimeToBeReceived"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.TimeToBeReceived' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgTimeToBeReceived)]
        public bool TimeToBeReceived
        {
            get
            {
                return ((data2 & TIME_TO_BE_RECEIVED) != 0);
            }

            set
            {
                data2 = value ? data2 | TIME_TO_BE_RECEIVED : data2 & ~TIME_TO_BE_RECEIVED;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.TimeToReachQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.TimeToReachQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgTimeToReachQueue)]
        public bool TimeToReachQueue
        {
            get
            {
                return ((data2 & TIME_TO_REACH_QUEUE) != 0);
            }

            set
            {
                data2 = value ? data2 | TIME_TO_REACH_QUEUE : data2 & ~TIME_TO_REACH_QUEUE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.TransactionId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.TransactionId' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgTransactionId)]
        public bool TransactionId
        {
            get
            {
                return ((data2 & TRANSACTION_ID) != 0);
            }

            set
            {
                data2 = value ? data2 | TRANSACTION_ID : data2 & ~TRANSACTION_ID;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.TransactionStatusQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.TransactionStatusQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgTransactionStatusQueue)]
        public bool TransactionStatusQueue
        {
            get
            {
                return ((data2 & FOREIGN_ADMIN_QUEUE) != 0);
            }

            set
            {
                data2 = value ? data2 | FOREIGN_ADMIN_QUEUE : data2 & ~FOREIGN_ADMIN_QUEUE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.UseAuthentication"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.UseAuthentication' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgUseAuthentication)]
        public bool UseAuthentication
        {
            get
            {
                return ((data2 & USE_AUTHENTICATION) != 0);
            }

            set
            {
                data2 = value ? data2 | USE_AUTHENTICATION : data2 & ~USE_AUTHENTICATION;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.UseDeadLetterQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.UseDeadLetterQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgUseDeadLetterQueue)]
        public bool UseDeadLetterQueue
        {
            get
            {
                return ((data1 & USE_DEADLETTER_QUEUE) != 0);
            }

            set
            {
                data1 = value ? data1 | USE_DEADLETTER_QUEUE : data1 & ~USE_DEADLETTER_QUEUE;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.UseEncryption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.UseEncryption' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgUseEncryption)]
        public bool UseEncryption
        {
            get
            {
                return ((data2 & USE_ENCRYPTION) != 0);
            }

            set
            {
                data2 = value ? data2 | USE_ENCRYPTION : data2 & ~USE_ENCRYPTION;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.UseJournalQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.UseJournalQueue' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(true), MessagingDescription(Res.MsgUseJournalQueue)]
        public bool UseJournalQueue
        {
            get
            {
                return ((data1 & USE_JOURNALING) != 0);
            }

            set
            {
                data1 = value ? data1 | USE_JOURNALING : data2 & ~USE_JOURNALING;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.UseTracing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to retrieve <see cref='System.Messaging.Message.UseTracing' qualify='true'/> property information when receiving or peeking
        ///       a message.
        ///    </para>
        /// </devdoc>
        [DefaultValueAttribute(false), MessagingDescription(Res.MsgUseTracing)]
        public bool UseTracing
        {
            get
            {
                return ((data2 & USE_TRACING) != 0);
            }

            set
            {
                data2 = value ? data2 | USE_TRACING : data2 & ~USE_TRACING;
            }
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.ClearAll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies not to retrieve
        ///       any message properties when receiving a message.
        ///    </para>
        /// </devdoc>
        public void ClearAll()
        {
            data1 = 0;
            data2 = 0;
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SetDefaults"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Filters on the message properties that the
        ///       constructor sets to <see langword='true'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public void SetDefaults()
        {
            data1 = ACKNOWLEDGEMENT |
                   ACKNOWLEDGE_TYPE |
                   ADMIN_QUEUE |
                   BODY |
                   ID |
                   LABEL |
                   USE_DEADLETTER_QUEUE |
                   RESPONSE_QUEUE |
                   MESSAGE_TYPE |
                   USE_JOURNALING |
                   LOOKUP_ID;

            data2 = 0;
            DefaultBodySize = defaultBodySize;
            DefaultExtensionSize = defaultExtensionSize;
            DefaultLabelSize = defaultLabelSize;
        }

        /// <include file='doc\MessagePropertyFilter.uex' path='docs/doc[@for="MessagePropertyFilter.SetAll"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies to retrieve all
        ///       message properties when receiving a message.
        ///    </para>
        /// </devdoc>
        public void SetAll()
        {
            data1 = ACKNOWLEDGEMENT |
                   ACKNOWLEDGE_TYPE |
                   ADMIN_QUEUE |
                   BODY |
                   ID |
                   LABEL |
                   USE_DEADLETTER_QUEUE |
                   RESPONSE_QUEUE |
                   MESSAGE_TYPE |
                   USE_JOURNALING |
                   LOOKUP_ID;

            data2 = APP_SPECIFIC |
                   ARRIVED_TIME |
                   ATTACH_SENDER_ID |
                   AUTHENTICATED |
                   CONNECTOR_TYPE |
                   CORRELATION_ID |
                   CRYPTOGRAPHIC_PROVIDER_NAME |
                   CRYPTOGRAPHIC_PROVIDER_TYPE |
                   IS_RECOVERABLE |
                   DESTINATION_QUEUE |
                   DIGITAL_SIGNATURE |
                   ENCRYPTION_ALGORITHM |
                   EXTENSION |
                   FOREIGN_ADMIN_QUEUE |
                   HASH_ALGORITHM |
                   PRIORITY |
                   SECURITY_CONTEXT |
                   SENDER_CERTIFICATE |
                   SENDER_ID |
                   SENT_TIME |
                   SOURCE_MACHINE |
                   SYMMETRIC_KEY |
                   TIME_TO_BE_RECEIVED |
                   TIME_TO_REACH_QUEUE |
                   USE_AUTHENTICATION |
                   USE_ENCRYPTION |
                   USE_TRACING |
                   VERSION |
                   IS_FIRST_IN_TRANSACTION |
                   IS_LAST_IN_TRANSACTION |
                   TRANSACTION_ID;
        }


        public virtual object Clone()
        {
            return this.MemberwiseClone();
        }
    }
}

