//------------------------------------------------------------------------------
// <copyright file="MessageQueueException.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Text;
    using System.Messaging.Interop;
    using System.ComponentModel;
    using System.Security;
    using Microsoft.Win32;
    using System.Globalization;

    /// <include file='doc\MessageQueueException.uex' path='docs/doc[@for="MessageQueueException"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Is thrown if a Microsoft Message
    ///       Queue Server (MSMQ) internal error occurs.
    ///    </para>
    /// </devdoc>
    [Serializable]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1032:ImplementStandardExceptionConstructors")]
    public class MessageQueueException : ExternalException, ISerializable
    {

        private readonly int nativeErrorCode;

        /// <include file='doc\MessageQueueException.uex' path='docs/doc[@for="MessageQueueException.MessageQueueException"]/*' />
        /// <internalonly/>        
        internal MessageQueueException(int error)
        {
            nativeErrorCode = error;
        }

        /// <include file='doc\MessageQueueException.uex' path='docs/doc[@for="MessageQueueException.MessageQueueException"]/*' />
        /// <internalonly/>
        protected MessageQueueException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            nativeErrorCode = info.GetInt32("NativeErrorCode");
        }

        /// <include file='doc\MessageQueueException.uex' path='docs/doc[@for="MessageQueueException.MessageQueueErrorCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public MessageQueueErrorCode MessageQueueErrorCode
        {
            get
            {
                return (MessageQueueErrorCode)nativeErrorCode;
            }
        }

        /// <include file='doc\MessageQueueException.uex' path='docs/doc[@for="MessageQueueException.Message"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string Message
        {
            get
            {
                try
                {
                    return Res.GetString(Convert.ToString(nativeErrorCode, 16).ToUpper(CultureInfo.InvariantCulture));
                }
                catch
                {
                    return GetUnknownErrorMessage(nativeErrorCode);
                }
            }
        }

        private static string GetUnknownErrorMessage(int error)
        {
            //get the system error message...
            string errorMsg = "";

            StringBuilder sb = new StringBuilder(256);
            int result = SafeNativeMethods.FormatMessage(SafeNativeMethods.FORMAT_MESSAGE_IGNORE_INSERTS |
                                       SafeNativeMethods.FORMAT_MESSAGE_FROM_SYSTEM |
                                       SafeNativeMethods.FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                       IntPtr.Zero, error, 0, sb, sb.Capacity + 1, IntPtr.Zero);
            if (result != 0)
            {
                int i = sb.Length;
                while (i > 0)
                {
                    char ch = sb[i - 1];
                    if (ch > 32 && ch != '.') break;
                    i--;
                }
                errorMsg = sb.ToString(0, i);
            }
            else
            {
                errorMsg = Res.GetString("UnknownError", Convert.ToString(error, 16));
            }

            return errorMsg;
        }

        /// <include file='doc\MessageQueueException.uex' path='docs/doc[@for="MessageQueueException.GetObjectData"]/*' />
        [SecurityCritical]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info == null)
            {
                throw new ArgumentNullException("info");
            }
            info.AddValue("NativeErrorCode", nativeErrorCode);
            base.GetObjectData(info, context);
        }

    }
}
