//------------------------------------------------------------------------------
// <copyright file="MessageEnumerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging
{

    using System.Messaging.Interop;

    public sealed class Cursor : IDisposable
    {

        private CursorHandle handle;
        private bool disposed;


        internal Cursor(MessageQueue queue)
        {
            CursorHandle result;
            int status = SafeNativeMethods.MQCreateCursor(queue.MQInfo.ReadHandle, out result);
            if (MessageQueue.IsFatalError(status))
                throw new MessageQueueException(status);

            this.handle = result;
        }


        internal CursorHandle Handle
        {
            get
            {
                if (disposed)
                {
                    throw new ObjectDisposedException(GetType().Name);
                }

                return handle;
            }
        }


        public void Close()
        {
            if (handle != null)
            {
                handle.Close();
                handle = null;
            }
        }


        public void Dispose()
        {
            this.Close();
            this.disposed = true;
        }

    }
}
