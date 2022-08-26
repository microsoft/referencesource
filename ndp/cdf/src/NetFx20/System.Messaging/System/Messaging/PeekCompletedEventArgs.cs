//------------------------------------------------------------------------------
// <copyright file="PeekCompletedEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;

    using System;

    /// <include file='doc\PeekCompletedEventArgs.uex' path='docs/doc[@for="PeekCompletedEventArgs"]/*' />
    /// <devdoc>
    /// <para>Provides data for the <see cref='System.Messaging.MessageQueue.PeekCompleted'/> event. When your asynchronous 
    ///    operation calls an event handler, an instance of this class is passed to the
    ///    handler.</para>
    /// </devdoc>
    public class PeekCompletedEventArgs : EventArgs
    {
        private IAsyncResult result;
        private Message message;
        private MessageQueue sender;

        /// <include file='doc\PeekCompletedEventArgs.uex' path='docs/doc[@for="PeekCompletedEventArgs.PeekCompletedEventArgs"]/*' />
        /// <internalonly/>            
        internal PeekCompletedEventArgs(MessageQueue sender, IAsyncResult result)
        {
            this.result = result;
            this.sender = sender;
        }

        /// <include file='doc\PeekCompletedEventArgs.uex' path='docs/doc[@for="PeekCompletedEventArgs.AsyncResult"]/*' />
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

        /// <include file='doc\PeekCompletedEventArgs.uex' path='docs/doc[@for="PeekCompletedEventArgs.Message"]/*' />
        /// <devdoc>
        ///    <para>The end result of the posted asynchronous peek
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
                        this.message = this.sender.EndPeek(result);
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
