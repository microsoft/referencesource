//------------------------------------------------------------------------------
// <copyright file="BinaryMessageFormatter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Diagnostics;
    using System.IO;
    using System.ComponentModel;

    /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter"]/*' />
    /// <devdoc>
    ///    Formatter class that serializes and deserializes objects into
    ///    and from  MessageQueue messages using binary format.
    /// </devdoc>             
    public class BinaryMessageFormatter : IMessageFormatter
    {
        private BinaryFormatter formatter;
        internal const short VT_BINARY_OBJECT = 0x300;

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.BinaryMessageFormatter"]/*' />
        /// <devdoc>
        ///    Creates a new Binary message formatter object.
        /// </devdoc>
        public BinaryMessageFormatter()
        {
            this.formatter = new BinaryFormatter();
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.BinaryMessageFormatter1"]/*' />
        /// <devdoc>
        ///    Creates a new Binary message formatter object 
        ///    with the given properties.
        /// </devdoc>
        public BinaryMessageFormatter(FormatterAssemblyStyle topObjectFormat, FormatterTypeStyle typeFormat)
        {
            this.formatter = new BinaryFormatter();
            this.formatter.AssemblyFormat = topObjectFormat;
            this.formatter.TypeFormat = typeFormat;
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.TopObjectFormat"]/*' />
        /// <devdoc>
        ///    Determines how the top (root) object of a graph
        ///    is laid out in the serialized stream.
        /// </devdoc>
        [MessagingDescription(Res.MsgTopObjectFormat), DefaultValueAttribute(FormatterAssemblyStyle.Simple)]
        public FormatterAssemblyStyle TopObjectFormat
        {
            get
            {
                return this.formatter.AssemblyFormat;
            }

            set
            {
                this.formatter.AssemblyFormat = value;
            }
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.TypeFormat"]/*' />
        /// <devdoc>
        ///    Determines how type descriptions are laid out in the
        ///    serialized stream.
        /// </devdoc>
        [MessagingDescription(Res.MsgTypeFormat), DefaultValueAttribute(FormatterTypeStyle.TypesWhenNeeded)]
        public FormatterTypeStyle TypeFormat
        {
            get
            {
                return this.formatter.TypeFormat;
            }

            set
            {
                this.formatter.TypeFormat = value;
            }
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.CanRead"]/*' />
        /// <devdoc>
        ///    <para>When this method is called, the formatter will attempt to determine
        ///       if the contents of the message are something the formatter can deal with.</para>
        /// </devdoc>
        public bool CanRead(Message message)
        {
            if (message == null)
                throw new ArgumentNullException("message");

            int variantType = message.BodyType;
            if (variantType != VT_BINARY_OBJECT)
                return false;

            return true;
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.Clone"]/*' />
        /// <devdoc>
        ///    This method is needed to improve scalability on Receive and ReceiveAsync scenarios.  Not requiring 
        ///     thread safety on read and write.
        /// </devdoc>
        public object Clone()
        {
            return new BinaryMessageFormatter(TopObjectFormat, TypeFormat);
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.Read"]/*' />
        /// <devdoc>
        ///    This method is used to read the contents from the given message 
        ///     and create an object.
        /// </devdoc>
        public object Read(Message message)
        {
            if (message == null)
                throw new ArgumentNullException("message");

            int variantType = message.BodyType;
            if (variantType == VT_BINARY_OBJECT)
            {
                Stream stream = message.BodyStream;
                return formatter.Deserialize(stream);
            }

            throw new InvalidOperationException(Res.GetString(Res.InvalidTypeDeserialization));
        }

        /// <include file='doc\BinaryMessageFormatter.uex' path='docs/doc[@for="BinaryMessageFormatter.Write"]/*' />
        /// <devdoc>
        ///    This method is used to write the given object into the given message.  
        ///     If the formatter cannot understand the given object, an exception is thrown.
        /// </devdoc>
        public void Write(Message message, object obj)
        {
            if (message == null)
                throw new ArgumentNullException("message");

            Stream stream = new MemoryStream();
            formatter.Serialize(stream, obj);
            message.BodyType = VT_BINARY_OBJECT;
            message.BodyStream = stream;
        }
    }
}
