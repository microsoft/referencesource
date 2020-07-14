//------------------------------------------------------------------------------
// <copyright file="ReceiveCompletedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;

    /// <include file='doc\ReceiveCompletedEventArgs.uex' path='docs/doc[@for="ReceiveCompletedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='System.Messaging.MessageQueue.ReceiveCompleted'/> 
    /// event.</para>
    /// </devdoc>
    public class ReceiveCompletedEventArgs : EventArgs
    {
        private IAsyncResult result;
        private Message message;
        private MessageQueue sender;

        /// <include file='doc\ReceiveCompletedEventArgs.uex' path='docs/doc[@for="ReceiveCompletedEventArgs.ReceiveCompletedEventArgs"]/*' />
        /// <internalonly/>            
        internal ReceiveCompletedEventArgs(MessageQueue sender, IAsyncResult result)
        {
            this.result = result;
            this.sender = sender;
        }

        /// <include file='doc\ReceiveCompletedEventArgs.uex' path='docs/doc[@for="ReceiveCompletedEventArgs.AsyncResult"]/*' />
        /// <devdoc>
        ///    <para>Contains the result of the asynchronous
        ///       operation requested.</para>
        /// </devdoc>
        public IAsyncResult AsyncResult
        {
            get
            {
                return this.result;
            }

            set
            {
                this.result = value;
            }
        }

        /// <include file='doc\ReceiveCompletedEventArgs.uex' path='docs/doc[@for="ReceiveCompletedEventArgs.Message"]/*' />
        /// <devdoc>
        ///    <para>The end result of the posted asynchronous receive
        ///       operation.</para>
        /// </devdoc>
        public Message Message
        {
            get
            {
                if (this.message == null)
                {
                    try
                    {
                        this.message = this.sender.EndReceive(result);
                    }
                    catch
                    {
                        throw;
                    }
                }

                return this.message;
            }
        }
    }
}
