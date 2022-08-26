//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime;
    using System.ServiceModel.Channels;
    using System.Threading;

    delegate void ListenerConnectionModeCallback(ListenerConnectionModeReader connectionModeReader);

    sealed class ListenerConnectionModeReader : InitialServerConnectionReader
    {
        Exception readException;
        ServerModeDecoder decoder;
        byte[] buffer;
        int offset;
        int size;
        ListenerConnectionModeCallback callback;
        static WaitCallback readCallback;
        byte[] accruedData;
        TimeoutHelper receiveTimeoutHelper;

        public ListenerConnectionModeReader(IConnection connection, ListenerConnectionModeCallback callback, ConnectionClosedCallback closedCallback)
            : base(connection, closedCallback)
        {
            this.callback = callback;
        }

        public int BufferOffset
        {
            get { return offset; }
        }

        public int BufferSize
        {
            get { return size; }
        }

        public long StreamPosition
        {
            get { return decoder.StreamPosition; }
        }

        public TimeSpan GetRemainingTimeout()
        {
            return this.receiveTimeoutHelper.RemainingTime();
        }

        void Complete(Exception e)
        {
            // exception will be logged by the caller
            readException = e;
            Complete();
        }

        void Complete()
        {
            callback(this);
        }

        bool ContinueReading()
        {
            while (true)
            {
                if (size == 0)
                {
                    if (readCallback == null)
                    {
                        readCallback = new WaitCallback(ReadCallback);
                    }

                    // if we already have buffered some data we need 
                    // to accrue it in case we're duping the connection
                    if (buffer != null)
                    {
                        int dataOffset = 0;
                        if (accruedData == null)
                        {
                            accruedData = new byte[offset];
                        }
                        else
                        {
                            byte[] newAccruedData = new byte[accruedData.Length + offset];
                            Buffer.BlockCopy(accruedData, 0, newAccruedData, 0, accruedData.Length);
                            dataOffset = this.accruedData.Length;
                            accruedData = newAccruedData;
                        }

                        Buffer.BlockCopy(buffer, 0, accruedData, dataOffset, offset);
                    }

                    if (Connection.BeginRead(0, Connection.AsyncReadBufferSize, GetRemainingTimeout(),
                        readCallback, this) == AsyncCompletionResult.Queued)
                    {
                        return false;
                    }
                    GetReadResult();
                }

                while (true)
                {
                    int bytesDecoded = decoder.Decode(buffer, offset, size);
                    if (bytesDecoded > 0)
                    {
                        offset += bytesDecoded;
                        size -= bytesDecoded;
                    }
                    if (decoder.CurrentState == ServerModeDecoder.State.Done)
                    {
                        return true;
                    }
                    if (size == 0)
                    {
                        break;
                    }
                }
            }
        }

        static void ReadCallback(object state)
        {
            ListenerConnectionModeReader reader = (ListenerConnectionModeReader)state;

            bool completeSelf = false;
            Exception completionException = null;
            try
            {
                if (reader.GetReadResult())
                {
                    completeSelf = reader.ContinueReading();
                }
            }
#pragma warning suppress 56500 // Microsoft, transferring exception to caller
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }

                completeSelf = true;
                completionException = e;
            }

            if (completeSelf)
            {
                reader.Complete(completionException);
            }
        }

        bool GetReadResult()
        {
            offset = 0;
            size = Connection.EndRead();
            if (size == 0)
            {
                if (this.decoder.StreamPosition == 0) // client timed out a cached connection
                {
                    base.Close(GetRemainingTimeout());
                    return false;
                }
                else
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(decoder.CreatePrematureEOFException());
                }
            }

            if (buffer == null)
            {
                buffer = Connection.AsyncReadBuffer;
            }

            return true;
        }

        public FramingMode GetConnectionMode()
        {
            if (readException != null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(readException);
            }

            return decoder.Mode;
        }

        public void StartReading(TimeSpan timeout, Action connectionDequeuedCallback)
        {
            this.receiveTimeoutHelper = new TimeoutHelper(timeout);
            this.decoder = new ServerModeDecoder();
            this.ConnectionDequeuedCallback = connectionDequeuedCallback;

            bool completeSelf;
            try
            {
                completeSelf = ContinueReading();
            }
#pragma warning suppress 56500 // Microsoft, transferring exception to caller
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }

                // exception will be logged by the caller
                this.readException = e;
                completeSelf = true;
            }

            if (completeSelf)
            {
                Complete();
            }
        }

        public byte[] AccruedData
        {
            get { return this.accruedData; }
        }
    }
}
