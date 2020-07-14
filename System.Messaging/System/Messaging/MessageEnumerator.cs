//------------------------------------------------------------------------------
// <copyright file="MessageEnumerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Collections;
    using System.Messaging.Interop;

    /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator"]/*' />
    /// <devdoc>
    ///    <para>Provides (forward-only)
    ///       cursor semantics to enumerate the messages contained in
    ///       a queue.</para>
    ///    <note type="rnotes">
    ///       Translate into English?
    ///    </note>
    /// </devdoc>
    public class MessageEnumerator : MarshalByRefObject, IEnumerator, IDisposable
    {
        private MessageQueue owner;
        private CursorHandle handle = System.Messaging.Interop.CursorHandle.NullHandle;
        private int index = 0;
        private bool disposed = false;
        private bool useCorrectRemoveCurrent = false; //needed in fix for 88615

        internal MessageEnumerator(MessageQueue owner, bool useCorrectRemoveCurrent)
        {
            this.owner = owner;
            this.useCorrectRemoveCurrent = useCorrectRemoveCurrent;
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.Current"]/*' />
        /// <devdoc>
        /// <para>Gets the current <see cref='System.Messaging.Message'/> pointed to
        ///    by this enumerator.</para>
        /// </devdoc>
        public Message Current
        {
            get
            {
                if (this.index == 0)
                    throw new InvalidOperationException(Res.GetString(Res.NoCurrentMessage));

                return this.owner.ReceiveCurrent(TimeSpan.Zero, NativeMethods.QUEUE_ACTION_PEEK_CURRENT, this.Handle,
                                                              this.owner.MessageReadPropertyFilter, null,
                                                              MessageQueueTransactionType.None);
            }
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.IEnumerator.Current"]/*' />
        /// <internalonly/>
        object IEnumerator.Current
        {
            get
            {
                return this.Current;
            }
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.CursorHandle"]/*' />
        /// <devdoc>
        ///    <para>Gets the native Message Queuing cursor handle used to browse messages
        ///       in the queue.</para>
        /// </devdoc>        
        public IntPtr CursorHandle
        {
            get { return this.Handle.DangerousGetHandle(); }
        }


        internal CursorHandle Handle
        {
            get
            {
                //Cursor handle doesn't demand permissions since GetEnumerator will demand somehow.
                if (this.handle.IsInvalid)
                {
                    //Cannot allocate the a new cursor if the object has been disposed, since finalization has been suppressed.
                    if (this.disposed)
                        throw new ObjectDisposedException(GetType().Name);

                    CursorHandle result;
                    int status = SafeNativeMethods.MQCreateCursor(this.owner.MQInfo.ReadHandle, out result);
                    if (MessageQueue.IsFatalError(status))
                        throw new MessageQueueException(status);

                    this.handle = result;
                }
                return this.handle;
            }
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Frees the resources associated with the enumerator.
        ///    </para>
        /// </devdoc>
        public void Close()
        {
            this.index = 0;
            if (!this.handle.IsInvalid)
            {
                this.handle.Close();
            }
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.Dispose"]/*' />
        /// <devdoc>
        /// </devdoc>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed")]
        public void Dispose()
        {
            Dispose(true);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.Dispose1"]/*' />
        /// <devdoc>
        ///    <para>
        ///    </para>
        /// </devdoc>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                this.Close();
            }

            this.disposed = true;
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.MoveNext"]/*' />
        /// <devdoc>
        ///    <para>Advances the enumerator to the next message in the queue, if one
        ///       is currently available.</para>
        /// </devdoc>
        public bool MoveNext()
        {
            return MoveNext(TimeSpan.Zero);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.MoveNext1"]/*' />
        /// <devdoc>
        ///    <para>Advances the enumerator to the next message in the
        ///       queue. If the enumerator is positioned at the end of the queue, <see cref='System.Messaging.MessageEnumerator.MoveNext'/> waits until a message is available or the
        ///       given <paramref name="timeout"/>
        ///       expires.</para>
        /// </devdoc>
        public unsafe bool MoveNext(TimeSpan timeout)
        {
            long timeoutInMilliseconds = (long)timeout.TotalMilliseconds;
            if (timeoutInMilliseconds < 0 || timeoutInMilliseconds > UInt32.MaxValue)
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "timeout", timeout.ToString()));

            int status = 0;
            int action = NativeMethods.QUEUE_ACTION_PEEK_NEXT;
            //Peek current or next?
            if (this.index == 0)
                action = NativeMethods.QUEUE_ACTION_PEEK_CURRENT;

            status = owner.StaleSafeReceiveMessage((uint)timeoutInMilliseconds, action, null, null, null, this.Handle, (IntPtr)NativeMethods.QUEUE_TRANSACTION_NONE);
            //If the cursor reached the end of the queue.
            if (status == (int)MessageQueueErrorCode.IOTimeout)
            {
                this.Close();
                return false;
            }
            //If all messages were removed.
            else if (status == (int)MessageQueueErrorCode.IllegalCursorAction)
            {
                this.index = 0;
                this.Close();
                return false;
            }

            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            ++this.index;
            return true;
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.RemoveCurrent"]/*' />
        /// <devdoc>
        ///    <para> Removes the current message from
        ///       the queue and returns the message to the calling application.</para>
        /// </devdoc>
        public Message RemoveCurrent()
        {
            return RemoveCurrent(TimeSpan.Zero, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.RemoveCurrent1"]/*' />
        /// <devdoc>
        ///    <para> Removes the current message from
        ///       the queue and returns the message to the calling application.</para>
        /// </devdoc>
        public Message RemoveCurrent(MessageQueueTransaction transaction)
        {
            if (transaction == null)
                throw new ArgumentNullException("transaction");

            return RemoveCurrent(TimeSpan.Zero, transaction, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.RemoveCurrent5"]/*' />
        /// <devdoc>
        ///    <para> Removes the current message from
        ///       the queue and returns the message to the calling application.</para>
        /// </devdoc>
        public Message RemoveCurrent(MessageQueueTransactionType transactionType)
        {
            if (!ValidationUtility.ValidateMessageQueueTransactionType(transactionType))
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));

            return RemoveCurrent(TimeSpan.Zero, null, transactionType);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.RemoveCurrent2"]/*' />
        /// <devdoc>
        ///    <para> Removes the current message from
        ///       the queue and returns the message to the calling application within the timeout specified.</para>
        /// </devdoc>
        public Message RemoveCurrent(TimeSpan timeout)
        {
            return RemoveCurrent(timeout, null, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.RemoveCurrent3"]/*' />
        /// <devdoc>
        ///    <para> Removes the current message from
        ///       the queue and returns the message to the calling application within the timeout specified.</para>
        /// </devdoc>
        public Message RemoveCurrent(TimeSpan timeout, MessageQueueTransaction transaction)
        {
            if (transaction == null)
                throw new ArgumentNullException("transaction");

            return RemoveCurrent(timeout, transaction, MessageQueueTransactionType.None);
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.RemoveCurrent4"]/*' />
        /// <devdoc>
        ///    <para> Removes the current message from
        ///       the queue and returns the message to the calling application within the timeout specified.</para>
        /// </devdoc>
        public Message RemoveCurrent(TimeSpan timeout, MessageQueueTransactionType transactionType)
        {
            if (!ValidationUtility.ValidateMessageQueueTransactionType(transactionType))
                throw new InvalidEnumArgumentException("transactionType", (int)transactionType, typeof(MessageQueueTransactionType));

            return RemoveCurrent(timeout, null, transactionType);
        }

        private Message RemoveCurrent(TimeSpan timeout, MessageQueueTransaction transaction, MessageQueueTransactionType transactionType)
        {
            long timeoutInMilliseconds = (long)timeout.TotalMilliseconds;
            if (timeoutInMilliseconds < 0 || timeoutInMilliseconds > UInt32.MaxValue)
                throw new ArgumentException(Res.GetString(Res.InvalidParameter, "timeout", timeout.ToString()));

            if (this.index == 0)
                return null;

            Message message = this.owner.ReceiveCurrent(timeout, NativeMethods.QUEUE_ACTION_RECEIVE,
                                                                               this.Handle, this.owner.MessageReadPropertyFilter, transaction, transactionType);

            if (!useCorrectRemoveCurrent) --this.index;

            return message;
        }

        /// <include file='doc\MessageEnumerator.uex' path='docs/doc[@for="MessageEnumerator.Reset"]/*' />
        /// <devdoc>
        ///    <para> Resets the current enumerator, so it points to
        ///       the head of the queue.</para>
        /// </devdoc>
        public void Reset()
        {
            this.Close();
        }
    }
}
