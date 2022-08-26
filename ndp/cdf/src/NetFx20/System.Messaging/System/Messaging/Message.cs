//------------------------------------------------------------------------------
// <copyright file="Message.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Messaging
{
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Globalization;
    using System.Messaging.Interop;
    using System.IO;
    using System.Security.Permissions;
    using Microsoft.Win32;

    /// <include file='doc\Message.uex' path='docs/doc[@for="Message"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides access to the properties needed to define a
    ///       Message Queuing message.
    ///    </para>
    /// </devdoc>
    [Designer("System.Messaging.Design.MessageDesigner, " + AssemblyRef.SystemDesign)]
    public class Message : Component
    {
        private const int GenericIdSize = 16;
        private const int MessageIdSize = 20;
        private const int DefaultQueueNameSize = 255;
        private const int DefaultCryptographicProviderNameSize = 255;
        private const int DefaultDigitalSignatureSize = 255;
        private const int DefaultSenderCertificateSize = 255;
        private const int DefaultSenderIdSize = 255;
        private const int DefaultSymmetricKeySize = 255;
        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.InfiniteTimeout"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that there is no timeout.
        ///    </para>
        /// </devdoc>
        public static readonly TimeSpan InfiniteTimeout = TimeSpan.FromSeconds(UInt32.MaxValue);

        private MessagePropertyFilter filter;
        private string machineName;
        private bool receiveCreated;
        private object cachedBodyObject;
        private Stream cachedBodyStream;
        private IMessageFormatter cachedFormatter;
        private MessageQueue cachedResponseQueue;
        private MessageQueue cachedTransactionStatusQueue;
        private MessageQueue cachedAdminQueue;
        private MessageQueue cachedDestinationQueue;
        internal MessagePropertyVariants properties;

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Message"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Messaging.Message'/> class with an empty body.
        ///    </para>
        /// </devdoc>
        public Message()
        {
            properties = new MessagePropertyVariants();
            receiveCreated = false;
            this.filter = new MessagePropertyFilter();

            //Always add Id
            properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_MSGID, new byte[MessageIdSize]);
            this.filter.Id = true;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Message1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Messaging.Message'/>
        ///       class, serializing the object passed as
        ///       an argument.
        ///    </para>
        /// </devdoc>
        public Message(object body)
            : this()
        {
            this.Body = body;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Message2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Message(object body, IMessageFormatter formatter)
            : this()
        {
            this.Formatter = formatter;
            this.Body = body;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Message3"]/*' />
        /// <internalonly/>                
        internal Message(MessagePropertyFilter filter)
        {
            properties = new MessagePropertyVariants();
            receiveCreated = true;
            this.filter = filter;
            if (filter.data1 != 0)
            {
                int data = filter.data1;

                if (0 != (data & MessagePropertyFilter.ACKNOWLEDGEMENT))
                    properties.SetUI2(NativeMethods.MESSAGE_PROPID_CLASS, (short)0);

                if (0 != (data & MessagePropertyFilter.ACKNOWLEDGE_TYPE))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_ACKNOWLEDGE, (byte)0);

                if (0 != (data & MessagePropertyFilter.ADMIN_QUEUE))
                {
                    properties.SetString(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE, new byte[DefaultQueueNameSize * 2]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN, DefaultQueueNameSize);
                }
                if (0 != (data & MessagePropertyFilter.BODY))
                {
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, new byte[filter.bodySize]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE, filter.bodySize);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_TYPE, 0);
                }
                if (0 != (data & MessagePropertyFilter.LABEL))
                {
                    properties.SetString(NativeMethods.MESSAGE_PROPID_LABEL, new byte[filter.labelSize * 2]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_LABEL_LEN, filter.labelSize);
                }
                if (0 != (data & MessagePropertyFilter.ID))
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_MSGID, new byte[MessageIdSize]);

                if (0 != (data & MessagePropertyFilter.LOOKUP_ID))
                    properties.SetUI8(NativeMethods.MESSAGE_PROPID_LOOKUPID, (long)0);

                if (0 != (data & MessagePropertyFilter.USE_DEADLETTER_QUEUE))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)0);

                if (0 != (data & MessagePropertyFilter.RESPONSE_QUEUE))
                {
                    properties.SetString(NativeMethods.MESSAGE_PROPID_RESP_QUEUE, new byte[DefaultQueueNameSize * 2]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN, DefaultQueueNameSize);
                }
                //Acknowledgment and MessageType are overloaded in MQ.
                if ((0 == (data & MessagePropertyFilter.ACKNOWLEDGEMENT)) && (0 != (data & MessagePropertyFilter.MESSAGE_TYPE)))
                    properties.SetUI2(NativeMethods.MESSAGE_PROPID_CLASS, (short)0);

                //Journaling and Deadletter are overloaded in MSMQ
                if ((0 == (data & MessagePropertyFilter.USE_DEADLETTER_QUEUE)) && (0 != (data & MessagePropertyFilter.USE_JOURNALING)))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)0);
            }

            if (filter.data2 != 0)
            {
                int data = filter.data2;
                if (0 != (data & MessagePropertyFilter.APP_SPECIFIC))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_APPSPECIFIC, 0);
                if (0 != (data & MessagePropertyFilter.ARRIVED_TIME))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_ARRIVEDTIME, 0);
                if (0 != (data & MessagePropertyFilter.ATTACH_SENDER_ID))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_TYPE, 0);
                if (0 != (data & MessagePropertyFilter.AUTHENTICATED))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_AUTHENTICATED, (byte)0);

                if (0 != (data & MessagePropertyFilter.CONNECTOR_TYPE))
                    properties.SetGuid(NativeMethods.MESSAGE_PROPID_CONNECTOR_TYPE, new byte[GenericIdSize]);
                if (0 != (data & MessagePropertyFilter.CORRELATION_ID))
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_CORRELATIONID, new byte[MessageIdSize]);
                if (0 != (data & MessagePropertyFilter.CRYPTOGRAPHIC_PROVIDER_NAME))
                {
                    properties.SetString(NativeMethods.MESSAGE_PROPID_PROV_NAME, new byte[DefaultCryptographicProviderNameSize * 2]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_PROV_NAME_LEN, DefaultCryptographicProviderNameSize);
                }
                if (0 != (data & MessagePropertyFilter.CRYPTOGRAPHIC_PROVIDER_TYPE))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_PROV_TYPE, 0);
                if (0 != (data & MessagePropertyFilter.IS_RECOVERABLE))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_DELIVERY, (byte)0);
                if (0 != (data & MessagePropertyFilter.DESTINATION_QUEUE))
                {
                    properties.SetString(NativeMethods.MESSAGE_PROPID_DEST_QUEUE, new byte[DefaultQueueNameSize * 2]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_DEST_QUEUE_LEN, DefaultQueueNameSize);
                }
                if (0 != (data & MessagePropertyFilter.DIGITAL_SIGNATURE))
                {
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SIGNATURE, new byte[DefaultDigitalSignatureSize]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN, DefaultDigitalSignatureSize);
                }
                if (0 != (data & MessagePropertyFilter.ENCRYPTION_ALGORITHM))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_ENCRYPTION_ALG, 0);
                if (0 != (data & MessagePropertyFilter.EXTENSION))
                {
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_EXTENSION, new byte[filter.extensionSize]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_EXTENSION_LEN, filter.extensionSize);
                }
                if (0 != (data & MessagePropertyFilter.FOREIGN_ADMIN_QUEUE))
                {
                    properties.SetString(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE, new byte[DefaultQueueNameSize * 2]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN, DefaultQueueNameSize);
                }
                if (0 != (data & MessagePropertyFilter.HASH_ALGORITHM))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_HASH_ALG, 0);
                if (0 != (data & MessagePropertyFilter.IS_FIRST_IN_TRANSACTION))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_FIRST_IN_XACT, (byte)0);
                if (0 != (data & MessagePropertyFilter.IS_LAST_IN_TRANSACTION))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_LAST_IN_XACT, (byte)0);
                if (0 != (data & MessagePropertyFilter.PRIORITY))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_PRIORITY, (byte)0);
                if (0 != (data & MessagePropertyFilter.SENDER_CERTIFICATE))
                {
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDER_CERT, new byte[DefaultSenderCertificateSize]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENDER_CERT_LEN, DefaultSenderCertificateSize);
                }
                if (0 != (data & MessagePropertyFilter.SENDER_ID))
                {
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDERID, new byte[DefaultSenderIdSize]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_LEN, DefaultSenderIdSize);
                }
                if (0 != (data & MessagePropertyFilter.SENT_TIME))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENTTIME, 0);
                if (0 != (data & MessagePropertyFilter.SOURCE_MACHINE))
                    properties.SetGuid(NativeMethods.MESSAGE_PROPID_SRC_MACHINE_ID, new byte[GenericIdSize]);
                if (0 != (data & MessagePropertyFilter.SYMMETRIC_KEY))
                {
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY, new byte[DefaultSymmetricKeySize]);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN, DefaultSymmetricKeySize);
                }
                if (0 != (data & MessagePropertyFilter.TIME_TO_BE_RECEIVED))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_TIME_TO_BE_RECEIVED, 0);
                if (0 != (data & MessagePropertyFilter.TIME_TO_REACH_QUEUE))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_TIME_TO_REACH_QUEUE, 0);
                if (0 != (data & MessagePropertyFilter.TRANSACTION_ID))
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_XACTID, new byte[MessageIdSize]);
                if (0 != (data & MessagePropertyFilter.USE_AUTHENTICATION))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_AUTH_LEVEL, 0);
                if (0 != (data & MessagePropertyFilter.USE_ENCRYPTION))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_PRIV_LEVEL, 0);
                if (0 != (data & MessagePropertyFilter.USE_TRACING))
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_TRACE, (byte)0);
                if (0 != (data & MessagePropertyFilter.VERSION))
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_VERSION, 0);
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Acknowledgment"]/*' />
        /// <devdoc>
        ///    <para>Gets the classification
        ///       of acknowledgment messages that Message Queuing posts.</para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAcknowledgement)]
        public Acknowledgment Acknowledgment
        {
            get
            {
                if (!this.filter.Acknowledgment)
                {
                    //This message cannot be an acknowledgment, because it has not been sent.
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.NotAcknowledgement));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Acknowledgment"));
                }

                //Casting unsigned short to int, mask off sign extension.
                int res = ((int)properties.GetUI2(NativeMethods.MESSAGE_PROPID_CLASS)) & 0x0000FFFF;
                return (Acknowledgment)res;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AcknowledgeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the type of acknowledgment
        ///       message requested
        ///       from
        ///       Message Queuing when a message arrives in the queue.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAcknowledgeType)]
        public AcknowledgeTypes AcknowledgeType
        {
            get
            {
                if (!this.filter.AcknowledgeType)
                {
                    //Return the default.
                    if (!receiveCreated)
                        return AcknowledgeTypes.None;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "AcknowledgeType"));
                }

                return (AcknowledgeTypes)this.properties.GetUI1(NativeMethods.MESSAGE_PROPID_ACKNOWLEDGE);
            }

            set
            {
                //If default
                if (value == AcknowledgeTypes.None)
                {
                    this.filter.AcknowledgeType = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_ACKNOWLEDGE);
                }
                else
                {
                    this.filter.AcknowledgeType = true;
                    this.properties.SetUI1(NativeMethods.MESSAGE_PROPID_ACKNOWLEDGE, (byte)value);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AdministrationQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the queue used for acknowledgment
        ///       messages.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAdministrationQueue)]
        public MessageQueue AdministrationQueue
        {
            get
            {
                if (!this.filter.AdministrationQueue)
                {
                    //This property has not been set, lets return an undefined value.
                    if (!receiveCreated)
                        return null;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "AdministrationQueue"));
                }

                if (this.cachedAdminQueue == null)
                {
                    if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN) != 0)
                    {
                        string queueFormatName = StringFromBytes(properties.GetString(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE),
                                                                 properties.GetUI4(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN));

                        this.cachedAdminQueue = new MessageQueue("FORMATNAME:" + queueFormatName);
                    }
                }

                return this.cachedAdminQueue;
            }

            set
            {
                if (value != null)
                    this.filter.AdministrationQueue = true;
                else
                {
                    //If default
                    if (this.filter.AdministrationQueue)
                    {
                        this.filter.AdministrationQueue = false;
                        properties.Remove(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE);
                        properties.Remove(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN);
                    }
                }

                this.cachedAdminQueue = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AppSpecific"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       application-generated information regarding the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAppSpecific)]
        public int AppSpecific
        {
            get
            {
                if (!this.filter.AppSpecific)
                {
                    //Return the default.
                    if (!receiveCreated)
                        return 0;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "AppSpecific"));
                }

                return properties.GetUI4(NativeMethods.MESSAGE_PROPID_APPSPECIFIC);
            }

            set
            {
                //If default
                if (value == 0)
                {
                    this.filter.AppSpecific = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_APPSPECIFIC);
                }
                else
                {
                    this.filter.AppSpecific = true;
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_APPSPECIFIC, value);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.ArrivedTime"]/*' />
        /// <devdoc>
        ///    Indicates when the message arrived at the queue.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgArrivedTime)]
        public DateTime ArrivedTime
        {
            get
            {
                if (!this.filter.ArrivedTime)
                {
                    //Undefined at this point, throw an exception.
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.ArrivedTimeNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "ArrivedTime"));
                }

                //Number of seconds ellapsed since 1/1/1970
                DateTime time = new DateTime(1970, 1, 1);
                time = time.AddSeconds(properties.GetUI4(NativeMethods.MESSAGE_PROPID_ARRIVEDTIME)).ToLocalTime();
                return time;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AttachSenderId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the sender ID is to be attached
        ///       to the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAttachSenderId)]
        public bool AttachSenderId
        {
            get
            {
                if (!this.filter.AttachSenderId)
                {
                    //SenderId is attached by default.
                    if (!receiveCreated)
                        return true;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "AttachSenderId"));
                }

                int type = properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_TYPE);
                if (type == NativeMethods.MESSAGE_SENDERID_TYPE_NONE)
                    return false;

                return true;
            }

            set
            {
                //If default.
                if (value)
                {
                    this.filter.AttachSenderId = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_SENDERID_TYPE);
                }
                else
                {
                    this.filter.AttachSenderId = true;
                    if (value)
                        properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_TYPE, NativeMethods.MESSAGE_SENDERID_TYPE_SID);
                    else
                        properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_TYPE, NativeMethods.MESSAGE_SENDERID_TYPE_NONE);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Authenticated"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the message was
        ///       authenticated.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAuthenticated)]
        public bool Authenticated
        {
            get
            {
                if (!this.filter.Authenticated)
                {
                    //Authentication is undefined, there is nothing to return here.
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.AuthenticationNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Authenticated"));
                }

                return (properties.GetUI1(NativeMethods.MESSAGE_PROPID_AUTHENTICATED) != 0);
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AuthenticationProviderName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the cryptographic
        ///       provider used to generate the digital signature of the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAuthenticationProviderName)]
        public string AuthenticationProviderName
        {
            get
            {
                if (!this.filter.AuthenticationProviderName)
                {
                    //Return default
                    if (!receiveCreated)
                        return "Microsoft Base Cryptographic Provider, Ver. 1.0";

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "AuthenticationProviderName"));
                }

                if (this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_PROV_NAME_LEN) != 0)
                    return StringFromBytes(this.properties.GetString(NativeMethods.MESSAGE_PROPID_PROV_NAME),
                                           properties.GetUI4(NativeMethods.MESSAGE_PROPID_PROV_NAME_LEN));
                else
                    return "";
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                //Should not remove if default, the default value might change in future MQ clients
                //if (value.CompareTo("Microsoft Base Cryptographic Provider, Ver. 1.0") == 0) {                    
                //    this.filter.AuthenticationProviderName = false;
                //    properties.Remove(NativeMethods.MESSAGE_PROPID_PROV_NAME);
                //    properties.Remove(NativeMethods.MESSAGE_PROPID_PROV_NAME_LEN);                    
                //}
                //else {
                this.filter.AuthenticationProviderName = true;
                properties.SetString(NativeMethods.MESSAGE_PROPID_PROV_NAME, StringToBytes(value));
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_PROV_NAME_LEN, value.Length);
                //}
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AuthenticationProviderType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the type of cryptographic provider used to
        ///       generate the digital signature of the
        ///       message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgAuthenticationProviderType)]
        public CryptographicProviderType AuthenticationProviderType
        {
            get
            {
                //Return default
                if (!this.filter.AuthenticationProviderType)
                {
                    if (!receiveCreated)
                        return CryptographicProviderType.RsaFull;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "AuthenticationProviderType"));
                }

                return (CryptographicProviderType)properties.GetUI4(NativeMethods.MESSAGE_PROPID_PROV_TYPE);
            }

            set
            {
                if (!ValidationUtility.ValidateCryptographicProviderType(value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(CryptographicProviderType));

                //Should not remove if default, the default value might change in future MQ clients
                //if (value == CryptographicProviderType.RsaFull) {                    
                //    this.filter.AuthenticationProviderType = false;
                //    properties.Remove(NativeMethods.MESSAGE_PROPID_PROV_TYPE);                    
                //}
                //else {                    
                this.filter.AuthenticationProviderType = true;
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_PROV_TYPE, (int)value);
                //}
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Body"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the serialized
        ///       contents of the message.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public object Body
        {
            get
            {
                if (!this.filter.Body)
                {
                    if (!receiveCreated)
                        return null;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Body"));
                }

                if (this.cachedBodyObject == null)
                {
                    if (this.Formatter == null)
                        throw new InvalidOperationException(Res.GetString(Res.FormatterMissing));

                    this.cachedBodyObject = this.Formatter.Read(this);
                }

                return this.cachedBodyObject;
            }

            set
            {
                this.filter.Body = true;
                this.cachedBodyObject = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.BodyStream"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the information in the body of
        ///       the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Editor("System.ComponentModel.Design.BinaryEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
        MessagingDescription(Res.MsgBodyStream)]
        public Stream BodyStream
        {
            get
            {
                if (!this.filter.Body)
                {
                    if (!receiveCreated)
                    {
                        this.filter.Body = true;
                        if (this.cachedBodyStream == null)
                            this.cachedBodyStream = new MemoryStream();

                        return this.cachedBodyStream;
                    }

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Body"));
                }

                if (this.cachedBodyStream == null)
                    this.cachedBodyStream = new MemoryStream(properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY),
                                                                                                0, properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE));

                return this.cachedBodyStream;
            }

            set
            {
                if (value != null)
                    this.filter.Body = true;
                else
                {
                    this.filter.Body = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_BODY);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_BODY_TYPE);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_BODY_SIZE);
                }

                this.cachedBodyStream = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.BodyType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the type of data the message body contains.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgBodyType)]
        public int BodyType
        {
            get
            {
                if (!this.filter.Body)
                {
                    //Return default.
                    if (!receiveCreated)
                        return 0;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Body"));
                }

                return this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_TYPE);
            }

            set
            {
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_TYPE, value);
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.ConnectorType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Is required whenever an application sets a message property that is
        ///       typically set by MSMQ. It is typically used in the following two cases.
        ///       Whenever a message is passed by a connector application, the connector
        ///       type is required so that the sending and receiving applications know how
        ///       to interpret the security and acknowledgment properties of the messages.
        ///       When sending application-encrypted messages, this property tells the
        ///       MSMQ run time to use the symmetric key.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgConnectorType)]
        public Guid ConnectorType
        {
            get
            {
                if (!this.filter.ConnectorType)
                {
                    //Return default.
                    if (!receiveCreated)
                        return Guid.Empty;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "ConnectorType"));
                }

                return new Guid(this.properties.GetGuid(NativeMethods.MESSAGE_PROPID_CONNECTOR_TYPE));
            }

            set
            {
                //If default
                if (value.Equals(Guid.Empty))
                {
                    this.filter.ConnectorType = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_CONNECTOR_TYPE);
                }
                else
                {
                    this.filter.ConnectorType = true;
                    properties.SetGuid(NativeMethods.MESSAGE_PROPID_CONNECTOR_TYPE, ((Guid)value).ToByteArray());
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.CorrelationId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the message identifier used by
        ///       acknowledgment and report messages to reference the original
        ///       message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgCorrelationId)]
        public string CorrelationId
        {
            get
            {
                if (!this.filter.CorrelationId)
                {
                    //Return default
                    if (!receiveCreated)
                        return String.Empty;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "CorrelationId"));
                }

                return IdFromByteArray(this.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_CORRELATIONID));
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                //If default
                if (value.Length == 0)
                {
                    this.filter.CorrelationId = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_CORRELATIONID);
                }
                else
                {
                    this.filter.CorrelationId = true;
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_CORRELATIONID, IdToByteArray(value));
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.DefaultBodySize"]/*' />
        /// <devdoc>
        ///    The default body  buffer size to create,  
        ///    when the message is received.
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private int DefaultBodySize
        {
            get
            {
                return this.filter.DefaultBodySize;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.DefaultExtensionSize"]/*' />
        /// <devdoc>
        ///    The default extension  buffer size to create,  
        ///    when the message is received.
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private int DefaultExtensionSize
        {
            get
            {
                return this.filter.DefaultExtensionSize;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.DefaultLabelSize"]/*' />
        /// <devdoc>
        ///    The default label  buffer size to create,  
        ///    when the message is received.
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        private int DefaultLabelSize
        {
            get
            {
                return this.filter.DefaultLabelSize;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.DestinationQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Identifies the original destination queue for a message. It is typically
        ///       used to determine the original destination of a message that is in a journal
        ///       or dead-letter queue, however it can also be used when sending a
        ///       response message back to a response queue.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgDestinationQueue)]
        public MessageQueue DestinationQueue
        {
            get
            {
                if (!this.filter.DestinationQueue)
                {
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.DestinationQueueNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "DestinationQueue"));
                }

                if (this.cachedDestinationQueue == null)
                {
                    if (this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_QUEUE_LEN) != 0)
                    {
                        string queueFormatName = StringFromBytes(properties.GetString(NativeMethods.MESSAGE_PROPID_DEST_QUEUE),
                                                                 properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_QUEUE_LEN));
                        this.cachedDestinationQueue = new MessageQueue("FORMATNAME:" + queueFormatName);
                    }
                }

                return this.cachedDestinationQueue;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.DestinationSymmetricKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the symmetric key used to encrypt messages.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        MessagingDescription(Res.MsgDestinationSymmetricKey)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly")]
        public byte[] DestinationSymmetricKey
        {
            get
            {
                if (!this.filter.DestinationSymmetricKey)
                {
                    if (!receiveCreated)
                        return new byte[0];

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "DestinationSymmetricKey"));
                }

                byte[] bytes = new byte[properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN)];
                Array.Copy(properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY), bytes, bytes.Length);
                return bytes;
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                //If default
                if (value.Length == 0)
                {
                    this.filter.DestinationSymmetricKey = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN);
                }
                else
                {
                    this.filter.DestinationSymmetricKey = true;
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY, value);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN, value.Length);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.DigitalSignature"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the digital signature used to authenticate
        ///       the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgDigitalSignature)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly")]
        public byte[] DigitalSignature
        {
            get
            {
                if (!this.filter.DigitalSignature)
                {
                    if (!receiveCreated)
                        return new byte[0];

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "DigitalSignature"));
                }

                byte[] bytes = new byte[properties.GetUI4(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN)];
                Array.Copy(properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_SIGNATURE), bytes, bytes.Length);
                return bytes;
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (value.Length == 0)
                {
                    this.filter.DigitalSignature = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_SIGNATURE);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN);
                }
                else
                {
                    this.filter.DigitalSignature = true;
                    this.filter.UseAuthentication = true;

                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SIGNATURE, value);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN, value.Length);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.EncryptionAlgorithm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the encryption algorithm used to encrypt the
        ///       body of a private message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgEncryptionAlgorithm)]
        public EncryptionAlgorithm EncryptionAlgorithm
        {
            get
            {
                if (!this.filter.EncryptionAlgorithm)
                {
                    //Return default.
                    if (!receiveCreated)
                        return EncryptionAlgorithm.Rc2;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "EncryptionAlgorithm"));
                }

                return (EncryptionAlgorithm)properties.GetUI4(NativeMethods.MESSAGE_PROPID_ENCRYPTION_ALG);
            }
            set
            {
                if (!ValidationUtility.ValidateEncryptionAlgorithm(value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(EncryptionAlgorithm));

                //Should not remove if default, the default value might change in future MQ clients
                //if (value == EncryptionAlgorithm.Rc2) {                
                //    this.filter.EncryptionAlgorithm = false;
                //    properties.Remove(NativeMethods.MESSAGE_PROPID_ENCRYPTION_ALG);                    
                //}
                //else {                    
                this.filter.EncryptionAlgorithm = true;
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_ENCRYPTION_ALG, (int)value);
                //}
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Extension"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       additional information associated with the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        MessagingDescription(Res.MsgExtension)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly")]
        public byte[] Extension
        {
            get
            {
                if (!this.filter.Extension)
                {
                    //Return default.
                    if (!receiveCreated)
                        return new byte[0];

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Extension"));
                }

                byte[] bytes = new byte[properties.GetUI4(NativeMethods.MESSAGE_PROPID_EXTENSION_LEN)];
                Array.Copy(properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_EXTENSION), bytes, bytes.Length);
                return bytes;
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                //If default
                if (value.Length == 0)
                {
                    this.filter.Extension = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_EXTENSION);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_EXTENSION_LEN);
                }
                else
                {
                    this.filter.Extension = true;
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_EXTENSION, value);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_EXTENSION_LEN, value.Length);
                }
            }
        }
        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Formatter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the formatter used to read or write an object into the message
        ///       body.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public IMessageFormatter Formatter
        {
            get
            {
                return this.cachedFormatter;
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                this.cachedFormatter = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.HashAlgorithm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the hashing
        ///       algorithm used when authenticating messages.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgHashAlgorithm)]
        public HashAlgorithm HashAlgorithm
        {
            get
            {
                if (!this.filter.HashAlgorithm)
                {
                    //This property has not been set, lets return an empty queue.
                    if (!receiveCreated)
                    {
                        if (LocalAppContextSwitches.UseMD5ForDefaultHashAlgorithm)
                        {
                            return HashAlgorithm.Md5;
                        }
                        return HashAlgorithm.Sha512;
                    }

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "HashAlgorithm"));
                }

                return (HashAlgorithm)properties.GetUI4(NativeMethods.MESSAGE_PROPID_HASH_ALG);
            }

            set
            {
                if (!ValidationUtility.ValidateHashAlgorithm(value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(HashAlgorithm));

                //Should not remove if default since MQ3.0 changed the default algorithm
                //if (value == HashAlgorithm.Md5) {                    
                //    this.filter.HashAlgorithm = false;
                //    properties.Remove(NativeMethods.MESSAGE_PROPID_HASH_ALG);                    
                //}
                //else {                    
                this.filter.HashAlgorithm = true;
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_HASH_ALG, (int)value);
                //}
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Id"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the Message Queuing-generated identifier of the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgId)]
        public string Id
        {
            get
            {
                if (!this.filter.Id)
                {
                    //The Id is undefined at this point
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.IdNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Id"));
                }

                return IdFromByteArray(this.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_MSGID));
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.IsFirstInTransaction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating
        ///       whether the message was the first message sent in a transaction.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgIsFirstInTransaction)]
        public bool IsFirstInTransaction
        {
            get
            {
                if (!this.filter.IsFirstInTransaction)
                {
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "IsFirstInTransaction"));
                }

                return (properties.GetUI1(NativeMethods.MESSAGE_PROPID_FIRST_IN_XACT) != 0);
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.IsLastInTransaction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the message was
        ///       the last message sent in a transaction.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgIsLastInTransaction)]
        public bool IsLastInTransaction
        {
            get
            {
                if (!this.filter.IsLastInTransaction)
                {
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "IsLastInTransaction"));
                }

                return (properties.GetUI1(NativeMethods.MESSAGE_PROPID_LAST_IN_XACT) != 0);
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Label"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the message label.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgLabel)]
        public string Label
        {
            get
            {
                if (!this.filter.Label)
                {
                    //Return default
                    if (!receiveCreated)
                        return String.Empty;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Label"));
                }

                if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_LABEL_LEN) != 0)
                    return StringFromBytes(this.properties.GetString(NativeMethods.MESSAGE_PROPID_LABEL),
                                           properties.GetUI4(NativeMethods.MESSAGE_PROPID_LABEL_LEN));
                else
                    return "";
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                //If default
                if (value.Length == 0)
                {
                    this.filter.Label = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_LABEL);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_LABEL_LEN);
                }
                else
                {
                    this.filter.Label = true;
                    properties.SetString(NativeMethods.MESSAGE_PROPID_LABEL, StringToBytes(value));
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_LABEL_LEN, value.Length);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.LookupId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Message Queue-generated lookup identifier of the message.
        ///    </para>
        /// </devdoc>
        public long LookupId
        {
            get
            {
                if (!MessageQueue.Msmq3OrNewer)
                    throw new PlatformNotSupportedException(Res.GetString(Res.PlatformNotSupported));

                if (!this.filter.LookupId)
                {
                    //Return default
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.LookupIdNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "LookupId"));
                }

                return this.properties.GetUI8(NativeMethods.MESSAGE_PROPID_LOOKUPID);
            }
        }

        internal void SetLookupId(long value)
        {
            this.filter.LookupId = true;
            this.properties.SetUI8(NativeMethods.MESSAGE_PROPID_LOOKUPID, value);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.MessageType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the message (normal, acknowledgment, or report).
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgMessageType)]
        public MessageType MessageType
        {
            get
            {
                if (!this.filter.MessageType)
                {
                    //Return default
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.MessageTypeNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "MessageType"));
                }

                int cls = properties.GetUI2(NativeMethods.MESSAGE_PROPID_CLASS);
                if (cls == NativeMethods.MESSAGE_CLASS_NORMAL)
                    return MessageType.Normal;

                if (cls == NativeMethods.MESSAGE_CLASS_REPORT)
                    return MessageType.Report;

                return MessageType.Acknowledgment;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Priority"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the message priority, used to determine
        ///       where the
        ///       message is placed in the
        ///       queue.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgPriority)]
        public MessagePriority Priority
        {
            get
            {
                if (!this.filter.Priority)
                {
                    //Return default
                    if (!receiveCreated)
                        return MessagePriority.Normal;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Priority"));
                }

                return (MessagePriority)properties.GetUI1(NativeMethods.MESSAGE_PROPID_PRIORITY);
            }

            set
            {
                if (!ValidationUtility.ValidateMessagePriority(value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(MessagePriority));

                //If default
                if (value == MessagePriority.Normal)
                {
                    this.filter.Priority = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_PRIORITY);
                }
                else
                {
                    this.filter.Priority = true;
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_PRIORITY, (byte)value);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Recoverable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value
        ///       indicating whether the message is guaranteed to be delivered in the event of
        ///       a computer failure or network problem.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgRecoverable)]
        public bool Recoverable
        {
            get
            {
                if (!this.filter.Recoverable)
                {
                    //Return default
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "Recoverable"));
                }

                return properties.GetUI1(NativeMethods.MESSAGE_PROPID_DELIVERY) == NativeMethods.MESSAGE_DELIVERY_RECOVERABLE;
            }

            set
            {
                //If default
                if (!value)
                {
                    this.filter.Recoverable = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_DELIVERY);
                }
                else
                {
                    this.filter.Recoverable = true;
                    properties.SetUI1(NativeMethods.MESSAGE_PROPID_DELIVERY, (byte)NativeMethods.MESSAGE_DELIVERY_RECOVERABLE);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.ResponseQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the queue which receives application-generated
        ///       response messages.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgResponseQueue)]
        public MessageQueue ResponseQueue
        {
            get
            {
                if (!this.filter.ResponseQueue)
                {
                    //This property has not been set, lets return an undefined value.
                    if (!receiveCreated)
                        return null;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "ResponseQueue"));
                }

                if (this.cachedResponseQueue == null)
                {
                    if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN) != 0)
                    {
                        string queueFormatName = StringFromBytes(properties.GetString(NativeMethods.MESSAGE_PROPID_RESP_QUEUE),
                                                                 properties.GetUI4(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN));

                        this.cachedResponseQueue = new MessageQueue("FORMATNAME:" + queueFormatName);
                    }
                }

                return this.cachedResponseQueue;
            }

            set
            {
                //If default
                if (value != null)
                    this.filter.ResponseQueue = true;
                else
                {
                    if (this.filter.ResponseQueue)
                    {
                        this.filter.ResponseQueue = false;
                        properties.Remove(NativeMethods.MESSAGE_PROPID_RESP_QUEUE);
                        properties.Remove(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN);
                    }
                }

                this.cachedResponseQueue = value;
            }
        }


        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public SecurityContext SecurityContext
        {
            get
            {
                if (!this.filter.SecurityContext)
                    return null;

                IntPtr handle = (IntPtr)(int)properties.GetUI4(NativeMethods.MESSAGE_PROPID_SECURITY_CONTEXT);
                return new SecurityContext(new SecurityContextHandle(handle));
            }

            set
            {
                if (value == null)
                {
                    this.filter.SecurityContext = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_SECURITY_CONTEXT);
                }
                else
                {
                    this.filter.SecurityContext = true;
                    // Can't store IntPtr because property type is UI4, but IntPtr can be 64 bits  
                    int handle = value.Handle.DangerousGetHandle().ToInt32(); // this is safe because MSMQ always returns 32-bit handle
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SECURITY_CONTEXT, handle);
                }

            }
        }


        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.SenderCertificate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the security certificate used to authenticate messages.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgSenderCertificate)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly")]
        public byte[] SenderCertificate
        {
            get
            {
                if (!this.filter.SenderCertificate)
                {
                    //Return default
                    if (!receiveCreated)
                        return new byte[0];

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "SenderCertificate"));
                }

                byte[] bytes = new byte[properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENDER_CERT_LEN)];
                Array.Copy(properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDER_CERT), bytes, bytes.Length);
                return bytes;
            }

            set
            {
                if (value == null)
                    throw new ArgumentNullException("value");

                //If default
                if (value.Length == 0)
                {
                    this.filter.SenderCertificate = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_SENDER_CERT);
                    properties.Remove(NativeMethods.MESSAGE_PROPID_SENDER_CERT_LEN);
                }
                else
                {
                    this.filter.SenderCertificate = true;
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDER_CERT, value);
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_SENDER_CERT_LEN, value.Length);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.SenderId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       This property is set by MSMQ, and is used primarily by the
        ///       receiving Queue Manager when authenticating a message. The receiving
        ///       Queue Manager uses the sender identifier in this property to verify where
        ///       the message originated and to verify the sender has access rights to a queue.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgSenderId)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays")]
        public byte[] SenderId
        {
            get
            {
                if (!this.filter.SenderId)
                {
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.SenderIdNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "SenderId"));
                }

                byte[] bytes = new byte[properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_LEN)];
                Array.Copy(properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDERID), bytes, bytes.Length);
                return bytes;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.SenderVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the version of Message Queuing used to send the message.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgSenderVersion)]
        public long SenderVersion
        {
            get
            {
                if (!this.filter.SenderVersion)
                {
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.VersionNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "SenderVersion"));
                }

                return (long)((uint)properties.GetUI4(NativeMethods.MESSAGE_PROPID_VERSION));
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.SentTime"]/*' />
        /// <devdoc>
        ///    Indicates the date and time that the message was sent by
        ///    the source Queue Manager.
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgSentTime)]
        public DateTime SentTime
        {
            get
            {
                if (!this.filter.SentTime)
                {
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.SentTimeNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "SentTime"));
                }

                //Number of seconds ellapsed since 1/1/1970
                DateTime time = new DateTime(1970, 1, 1);
                time = time.AddSeconds(properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENTTIME)).ToLocalTime();
                return time;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.SourceMachine"]/*' />
        /// <devdoc>
        ///    Specifies the computer where the message originated.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgSourceMachine)]
        public string SourceMachine
        {
            get
            {
                if (!this.filter.SourceMachine)
                {
                    if (!receiveCreated)
                        throw new InvalidOperationException(Res.GetString(Res.SourceMachineNotSet));

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "SourceMachine"));
                }

                if (this.machineName == null)
                {
                    byte[] bytes = this.properties.GetGuid(NativeMethods.MESSAGE_PROPID_SRC_MACHINE_ID);
                    GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);

                    MachinePropertyVariants machineProperties = new MachinePropertyVariants();
                    machineProperties.SetNull(NativeMethods.MACHINE_PATHNAME);
                    int status = UnsafeNativeMethods.MQGetMachineProperties(null, handle.AddrOfPinnedObject(), machineProperties.Lock());
                    machineProperties.Unlock();
                    handle.Free();

                    IntPtr memoryHandle = machineProperties.GetIntPtr(NativeMethods.MACHINE_PATHNAME);
                    if (memoryHandle != (IntPtr)0)
                    {
                        //Using Unicode API even on Win9x
                        this.machineName = Marshal.PtrToStringUni(memoryHandle);
                        SafeNativeMethods.MQFreeMemory(memoryHandle);
                    }

                    if (MessageQueue.IsFatalError(status))
                        throw new MessageQueueException(status);
                }

                return this.machineName;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.TimeToBeReceived"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the time limit for the message to be retrieved from the target
        ///       queue.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        TypeConverter(typeof(System.Messaging.Design.TimeoutConverter)),
        MessagingDescription(Res.MsgTimeToBeReceived)]
        public TimeSpan TimeToBeReceived
        {
            get
            {
                if (!this.filter.TimeToBeReceived)
                {
                    //Return default
                    if (!receiveCreated)
                        return InfiniteTimeout;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "TimeToBeReceived"));
                }

                return TimeSpan.FromSeconds((uint)properties.GetUI4(NativeMethods.MESSAGE_PROPID_TIME_TO_BE_RECEIVED));
            }

            set
            {
                long timeoutInSeconds = (long)value.TotalSeconds;
                if (timeoutInSeconds < 0)
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "TimeToBeReceived", value.ToString()));

                if (timeoutInSeconds > UInt32.MaxValue)
                    timeoutInSeconds = UInt32.MaxValue;

                //If default
                if (timeoutInSeconds == UInt32.MaxValue)
                {
                    this.filter.TimeToBeReceived = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_TIME_TO_BE_RECEIVED);
                }
                else
                {
                    this.filter.TimeToBeReceived = true;
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_TIME_TO_BE_RECEIVED, (int)((uint)timeoutInSeconds));
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.TimeToReachQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the time limit for the message to reach
        ///       the queue.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        TypeConverter(typeof(System.Messaging.Design.TimeoutConverter)),
        MessagingDescription(Res.MsgTimeToReachQueue)]
        public TimeSpan TimeToReachQueue
        {
            get
            {
                if (!this.filter.TimeToReachQueue)
                {
                    //Return default
                    if (!receiveCreated)
                        return InfiniteTimeout;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "TimeToReachQueue"));
                }

                return TimeSpan.FromSeconds((uint)properties.GetUI4(NativeMethods.MESSAGE_PROPID_TIME_TO_REACH_QUEUE));
            }

            set
            {
                long timeoutInSeconds = (long)value.TotalSeconds;
                if (timeoutInSeconds < 0)
                    throw new ArgumentException(Res.GetString(Res.InvalidProperty, "TimeToReachQueue", value.ToString()));

                if (timeoutInSeconds > UInt32.MaxValue)
                    timeoutInSeconds = UInt32.MaxValue;

                if (timeoutInSeconds == UInt32.MaxValue)
                {
                    this.filter.TimeToReachQueue = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_TIME_TO_REACH_QUEUE);
                }
                else
                {
                    this.filter.TimeToReachQueue = true;
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_TIME_TO_REACH_QUEUE, (int)((uint)timeoutInSeconds));
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.TransactionId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       identifier for the transaction of which the message was a part.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgTransactionId)]
        public string TransactionId
        {
            get
            {
                if (!this.filter.TransactionId)
                {
                    //Return default
                    if (!receiveCreated)
                        return String.Empty;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "TransactionId"));
                }

                return IdFromByteArray(this.properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_XACTID));
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.TransactionStatusQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       transaction status queue on the source computer.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgTransactionStatusQueue)]
        public MessageQueue TransactionStatusQueue
        {
            get
            {
                if (!this.filter.TransactionStatusQueue)
                {
                    //This property has not been set, lets return an undefined value.
                    if (!receiveCreated)
                        return null;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "TransactionStatusQueue"));
                }

                if (this.cachedTransactionStatusQueue == null)
                {
                    if (this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN) != 0)
                    {
                        string queueFormatName = StringFromBytes(properties.GetString(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE),
                                                                 properties.GetUI4(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN));

                        this.cachedTransactionStatusQueue = new MessageQueue("FORMATNAME:" + queueFormatName);
                    }
                }

                return this.cachedTransactionStatusQueue;
            }

            set
            {
                //If default
                if (value != null)
                    this.filter.TransactionStatusQueue = true;
                else
                {
                    if (this.filter.TransactionStatusQueue)
                    {
                        this.filter.TransactionStatusQueue = false;
                        properties.Remove(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE);
                        properties.Remove(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN);
                    }
                }

                this.cachedTransactionStatusQueue = value;
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.UseAuthentication"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets a value indicating whether a message must be authenticated.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgUseAuthentication)]
        public bool UseAuthentication
        {
            get
            {
                if (!this.filter.UseAuthentication)
                {
                    //Return default
                    if (!receiveCreated)
                    {

                        // Actually, we dont know what default is:
                        // Algorithm to determine whether or not messages 
                        // should be authenticated by default is non-trivial 
                        // and should not be reproduced in System.Messaging. 
                        // 
                        // One idea is to add a new native API, 
                        // MQGetDefaultPropertyValue, to retrieve default values.
                        // (Microsoft, Nov 3 2004)
                        return false;
                    }

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "UseAuthentication"));
                }

                return (properties.GetUI4(NativeMethods.MESSAGE_PROPID_AUTH_LEVEL) != NativeMethods.MESSAGE_AUTHENTICATION_LEVEL_NONE);
            }

            set
            {
                //default is different on different versions of MSMQ, 
                //so dont make any assumptions and explicitly pass what user requested
                this.filter.UseAuthentication = true;
                if (!value)
                {
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_AUTH_LEVEL, NativeMethods.MESSAGE_AUTHENTICATION_LEVEL_NONE);
                }
                else
                {
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_AUTH_LEVEL, NativeMethods.MESSAGE_AUTHENTICATION_LEVEL_ALWAYS);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.UseDeadLetterQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether
        ///       a copy of an undeliverable message should be sent to a dead-letter queue.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgUseDeadLetterQueue)]
        public bool UseDeadLetterQueue
        {
            get
            {
                if (!this.filter.UseDeadLetterQueue)
                {
                    //Return default
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "UseDeadLetterQueue"));
                }

                return ((properties.GetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL) & NativeMethods.MESSAGE_JOURNAL_DEADLETTER) != 0);
            }

            set
            {
                //If Default
                if (!value)
                {
                    if (this.filter.UseDeadLetterQueue)
                    {
                        this.filter.UseDeadLetterQueue = false;
                        if (!this.filter.UseJournalQueue)
                            properties.Remove(NativeMethods.MESSAGE_PROPID_JOURNAL);
                        else
                            properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)(properties.GetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL) & ~NativeMethods.MESSAGE_JOURNAL_DEADLETTER));
                    }
                }
                else
                {
                    if (!this.filter.UseDeadLetterQueue && !this.filter.UseJournalQueue)
                        properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)NativeMethods.MESSAGE_JOURNAL_DEADLETTER);
                    else
                        properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)(properties.GetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL) | NativeMethods.MESSAGE_JOURNAL_DEADLETTER));

                    this.filter.UseDeadLetterQueue = true;
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.UseEncryption"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether to encrypt messages.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgUseEncryption)]
        public bool UseEncryption
        {
            get
            {
                if (!this.filter.UseEncryption)
                {
                    //Return default
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "UseEncryption"));
                }

                return (properties.GetUI4(NativeMethods.MESSAGE_PROPID_PRIV_LEVEL) != NativeMethods.MESSAGE_PRIVACY_LEVEL_NONE);
            }

            set
            {
                //If default
                if (!value)
                {
                    this.filter.UseEncryption = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_PRIV_LEVEL);
                }
                else
                {
                    this.filter.UseEncryption = true;
                    properties.SetUI4(NativeMethods.MESSAGE_PROPID_PRIV_LEVEL, NativeMethods.MESSAGE_PRIVACY_LEVEL_BODY);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.UseJournalQueue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether a copy of the message should be kept in a machine
        ///       journal on the originating computer.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgUseJournalQueue)]
        public bool UseJournalQueue
        {
            get
            {
                if (!this.filter.UseJournalQueue)
                {
                    //Return default
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "UseJournalQueue"));
                }

                return ((properties.GetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL) & NativeMethods.MESSAGE_JOURNAL_JOURNAL) != 0);
            }

            set
            {
                //If Default
                if (!value)
                {
                    if (this.filter.UseJournalQueue)
                    {
                        this.filter.UseJournalQueue = false;
                        if (!this.filter.UseDeadLetterQueue)
                            properties.Remove(NativeMethods.MESSAGE_PROPID_JOURNAL);
                        else
                            properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)(properties.GetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL) & ~NativeMethods.MESSAGE_JOURNAL_JOURNAL));
                    }
                }
                else
                {
                    if (!this.filter.UseDeadLetterQueue && !this.filter.UseJournalQueue)
                        properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)NativeMethods.MESSAGE_JOURNAL_JOURNAL);
                    else
                        properties.SetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL, (byte)(properties.GetUI1(NativeMethods.MESSAGE_PROPID_JOURNAL) | NativeMethods.MESSAGE_JOURNAL_JOURNAL));

                    this.filter.UseJournalQueue = true;
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.UseTracing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets a value indicating whether to trace a message as
        ///       it moves toward its destination queue.
        ///    </para>
        /// </devdoc>
        [ReadOnly(true), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MessagingDescription(Res.MsgUseTracing)]
        public bool UseTracing
        {
            get
            {
                if (!this.filter.UseTracing)
                {
                    //Return default
                    if (!receiveCreated)
                        return false;

                    throw new InvalidOperationException(Res.GetString(Res.MissingProperty, "UseTracing"));
                }

                return (properties.GetUI1(NativeMethods.MESSAGE_PROPID_TRACE) != NativeMethods.MESSAGE_TRACE_NONE);
            }

            set
            {
                //If Default
                if (!value)
                {
                    this.filter.UseTracing = false;
                    properties.Remove(NativeMethods.MESSAGE_PROPID_TRACE);
                }
                else
                {
                    this.filter.UseTracing = true;

                    if (!value)
                        properties.SetUI1(NativeMethods.MESSAGE_PROPID_TRACE, (byte)NativeMethods.MESSAGE_TRACE_NONE);
                    else
                        properties.SetUI1(NativeMethods.MESSAGE_PROPID_TRACE, (byte)NativeMethods.MESSAGE_TRACE_SEND_ROUTE_TO_REPORT_QUEUE);
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AdjustMemory"]/*' />
        /// <internalonly/>        
        internal void AdjustMemory()
        {
            if (filter.AdministrationQueue)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN);
                if (size > Message.DefaultQueueNameSize)
                    properties.SetString(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE, new byte[size * 2]);
            }

            if (filter.Body)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE);
                if (size > DefaultBodySize)
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, new byte[size]);
            }

            if (filter.AuthenticationProviderName)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_PROV_NAME_LEN);
                if (size > Message.DefaultCryptographicProviderNameSize)
                    properties.SetString(NativeMethods.MESSAGE_PROPID_PROV_NAME, new byte[size * 2]);
            }

            if (filter.DestinationQueue)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_QUEUE_LEN);
                if (size > Message.DefaultQueueNameSize)
                    properties.SetString(NativeMethods.MESSAGE_PROPID_DEST_QUEUE, new byte[size * 2]);
            }

            if (filter.Extension)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_EXTENSION_LEN);
                if (size > DefaultExtensionSize)
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_EXTENSION, new byte[size]);
            }

            if (filter.TransactionStatusQueue)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN);
                if (size > Message.DefaultQueueNameSize)
                    properties.SetString(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE, new byte[size * 2]);
            }

            if (filter.Label)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_LABEL_LEN);
                if (size > DefaultLabelSize)
                    properties.SetString(NativeMethods.MESSAGE_PROPID_LABEL, new byte[size * 2]);
            }

            if (filter.ResponseQueue)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN);
                if (size > Message.DefaultQueueNameSize)
                    properties.SetString(NativeMethods.MESSAGE_PROPID_RESP_QUEUE, new byte[size * 2]);
            }

            if (filter.SenderCertificate)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENDER_CERT_LEN);
                if (size > Message.DefaultSenderCertificateSize)
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDER_CERT, new byte[size]);
            }

            if (filter.SenderId)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENDERID_LEN);
                if (size > Message.DefaultSenderIdSize)
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SENDERID, new byte[size]);
            }

            if (filter.DestinationSymmetricKey)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN);
                if (size > Message.DefaultSymmetricKeySize)
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY, new byte[size]);
            }

            if (filter.DigitalSignature)
            {
                int size = properties.GetUI4(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN);
                if (size > Message.DefaultDigitalSignatureSize)
                    properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_SIGNATURE, new byte[size]);
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.AdjustToSend"]/*' />
        /// <internalonly/>     
        internal void AdjustToSend()
        {
            //Write cached properties
            string queueFormatName;
            if (this.filter.AdministrationQueue && this.cachedAdminQueue != null)
            {
                queueFormatName = this.cachedAdminQueue.FormatName;
                properties.SetString(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE, StringToBytes(queueFormatName));
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN, queueFormatName.Length);
            }

            if (this.filter.ResponseQueue && this.cachedResponseQueue != null)
            {
                queueFormatName = this.cachedResponseQueue.FormatName;
                properties.SetString(NativeMethods.MESSAGE_PROPID_RESP_QUEUE, StringToBytes(queueFormatName));
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN, queueFormatName.Length);
            }

            if (this.filter.TransactionStatusQueue && this.cachedTransactionStatusQueue != null)
            {
                queueFormatName = this.cachedTransactionStatusQueue.FormatName;
                properties.SetString(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE, StringToBytes(queueFormatName));
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN, queueFormatName.Length);
            }

            if (this.filter.Body && this.cachedBodyObject != null)
            {
                if (this.Formatter == null)
                    this.Formatter = new XmlMessageFormatter();

                this.Formatter.Write(this, this.cachedBodyObject);
            }

            if (this.filter.Body && this.cachedBodyStream != null)
            {
                this.cachedBodyStream.Position = 0;
                byte[] bytes = new byte[(int)this.cachedBodyStream.Length];
                this.cachedBodyStream.Read(bytes, 0, bytes.Length);
                properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, bytes);
                properties.SetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE, bytes.Length);
            }

            if (this.receiveCreated)
            {
                lock (this)
                {
                    if (this.receiveCreated)
                    {
                        //We don't want to send the buffers as they were allocated
                        //when receiving, they might be to big.
                        //Adjust sizes
                        if (this.filter.Body)
                        {
                            int bodySize = properties.GetUI4(NativeMethods.MESSAGE_PROPID_BODY_SIZE);
                            byte[] bodyArray = properties.GetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY);

                            Debug.Assert(bodySize <= bodyArray.Length, "Allocated body array size is bigger than BODY_SIZE property");

                            if (bodySize < bodyArray.Length)
                            { // need to reallocate body array

                                byte[] bytes = new byte[bodySize];
                                Array.Copy(bodyArray, bytes, bodySize);

                                properties.SetUI1Vector(NativeMethods.MESSAGE_PROPID_BODY, bytes);
                            }
                        }
                        if (this.filter.Extension)
                        {
                            this.properties.AdjustSize(NativeMethods.MESSAGE_PROPID_EXTENSION,
                                                       this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_EXTENSION_LEN));
                        }
                        if (this.filter.SenderCertificate)
                        {
                            this.properties.AdjustSize(NativeMethods.MESSAGE_PROPID_SENDER_CERT,
                                                       this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_SENDER_CERT_LEN));
                        }
                        if (this.filter.DestinationSymmetricKey)
                        {
                            this.properties.AdjustSize(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY,
                                                       this.properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN));
                        }

                        //Ghost properties.
                        if (this.filter.Acknowledgment || this.filter.MessageType)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_CLASS);
                        if (this.filter.ArrivedTime)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_ARRIVEDTIME);
                        if (this.filter.Authenticated)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_AUTHENTICATED);
                        if (this.filter.DestinationQueue)
                        {
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_DEST_QUEUE);
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_DEST_QUEUE_LEN);
                            this.cachedDestinationQueue = null;
                        }
                        if (this.filter.IsFirstInTransaction)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_FIRST_IN_XACT);
                        if (this.filter.IsLastInTransaction)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_LAST_IN_XACT);
                        if (this.filter.SenderId)
                        {
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_SENDERID);
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_SENDERID_LEN);
                        }
                        if (this.filter.SentTime)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_SENTTIME);
                        if (this.filter.SourceMachine)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_SRC_MACHINE_ID);
                        if (this.filter.TransactionId)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_XACTID);
                        if (this.filter.SenderVersion)
                            this.properties.Ghost(NativeMethods.MESSAGE_PROPID_VERSION);

                        //Ghost invalid returned properties

                        if (this.filter.AdministrationQueue)
                        {
                            if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN) == 0)
                            {
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE);
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_ADMIN_QUEUE_LEN);
                            }
                        }
                        //Encryption algorithm cannot be set if not using Encryption
                        if (this.filter.EncryptionAlgorithm)
                        {
                            if ((this.filter.UseEncryption && !this.UseEncryption) || !this.filter.UseEncryption)
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_ENCRYPTION_ALG);
                        }
                        if (this.filter.DigitalSignature)
                        {
                            if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN) == 0)
                            {
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_SIGNATURE);
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_SIGNATURE_LEN);
                            }
                        }
                        if (this.filter.DestinationSymmetricKey)
                        {
                            if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN) == 0)
                            {
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY);
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_DEST_SYMM_KEY_LEN);
                            }
                        }
                        if (this.filter.ResponseQueue)
                        {
                            if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN) == 0)
                            {
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_RESP_QUEUE);
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_RESP_QUEUE_LEN);
                            }
                        }
                        if (this.filter.TransactionStatusQueue)
                        {
                            if (properties.GetUI4(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN) == 0)
                            {
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE);
                                this.properties.Ghost(NativeMethods.MESSAGE_PROPID_XACT_STATUS_QUEUE_LEN);
                            }
                        }

                        this.receiveCreated = false;
                    }
                }
            }
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.IdFromByteArray"]/*' />
        /// <internalonly/>        
        private string IdFromByteArray(byte[] bytes)
        {
            StringBuilder result = new StringBuilder();
            byte[] guidBytes = new byte[GenericIdSize];
            Array.Copy(bytes, guidBytes, GenericIdSize);
            int id = BitConverter.ToInt32(bytes, GenericIdSize);
            result.Append((new Guid(guidBytes)).ToString());
            result.Append("\\");
            result.Append(id);
            return result.ToString();
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.IdToByteArray"]/*' />
        /// <internalonly/>        
        private byte[] IdToByteArray(string id)
        {
            string[] pieces = id.Split(new char[] { '\\' });
            if (pieces.Length != 2)
                throw new InvalidOperationException(Res.GetString(Res.InvalidId));

            Guid guid;
            try
            {
                guid = new Guid(pieces[0]);
            }
            catch (FormatException)
            {
                throw new InvalidOperationException(Res.GetString(Res.InvalidId));
            }

            int integerId;
            try
            {
                integerId = Convert.ToInt32(pieces[1], CultureInfo.InvariantCulture);
            }
            catch (FormatException)
            {
                throw new InvalidOperationException(Res.GetString(Res.InvalidId));
            }
            catch (OverflowException)
            {
                throw new InvalidOperationException(Res.GetString(Res.InvalidId));
            }

            byte[] bytes = new byte[MessageIdSize];
            Array.Copy(guid.ToByteArray(), bytes, GenericIdSize);
            Array.Copy(BitConverter.GetBytes(integerId), 0, bytes, GenericIdSize, 4);
            return bytes;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Lock"]/*' />
        /// <internalonly/>        
        internal MessagePropertyVariants.MQPROPS Lock()
        {
            return this.properties.Lock();
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.StringFromBytes"]/*' />
        /// <internalonly/>        
        internal static string StringFromBytes(byte[] bytes, int len)
        {
            //If the string ends with 0, lets trim it.
            if (len != 0 && bytes[len * 2 - 1] == 0 && bytes[len * 2 - 2] == 0)
                --len;

            char[] charBuffer = new char[len];
            Encoding.Unicode.GetChars(bytes, 0, len * 2, charBuffer, 0);
            return new String(charBuffer, 0, len);
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.StringToBytes"]/*' />
        /// <internalonly/>        
        internal static byte[] StringToBytes(string value)
        {
            int size = value.Length * 2 + 1;
            byte[] byteBuffer = new byte[size];
            byteBuffer[size - 1] = 0;
            Encoding.Unicode.GetBytes(value.ToCharArray(), 0, value.Length, byteBuffer, 0);
            return byteBuffer;
        }

        /// <include file='doc\Message.uex' path='docs/doc[@for="Message.Unlock"]/*' />
        /// <internalonly/>        
        internal void Unlock()
        {
            this.properties.Unlock();
        }
    }
}
