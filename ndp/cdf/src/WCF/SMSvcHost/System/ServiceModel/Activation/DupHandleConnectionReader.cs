//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.Threading;

    // This class takes a connection, reads enough so that we can dispatch it to another process,
    // and then hands off the connection. This is done for all non-multiplexed modes (singleton, duplex, simplex)
    abstract class DupHandleConnectionReader : InitialServerConnectionReader
    {
        byte[] connectionBuffer;
        ViaDecodedCallback viaDecodedCallback;
        byte[] dataRead;
        int offset;
        static WaitCallback readCallback;
        int size;
        TransportType transportType;
        TimeoutHelper receiveTimeoutHelper;

        protected DupHandleConnectionReader(IConnection connection,
            Action connectionDequeuedCallback, TransportType transportType,
            int offset, int size, ConnectionClosedCallback closedCallback, ViaDecodedCallback viaDecodedCallback)
            : base(connection, closedCallback, ListenerConstants.MaxUriSize, ListenerConstants.SharedMaxContentTypeSize)
        {
            this.transportType = transportType;
            this.offset = offset;
            this.size = size;
            this.viaDecodedCallback = viaDecodedCallback;
            this.ConnectionDequeuedCallback = connectionDequeuedCallback;
        }

        TimeSpan GetRemainingTimeout()
        {
            return this.receiveTimeoutHelper.RemainingTime();
        }

        void AbortAndCount(Exception exception)
        {
            if (transportType == TransportType.Tcp)
            {
                ListenerPerfCounters.IncrementProtocolFailuresTcp();
            }
            else if (transportType == TransportType.NamedPipe)
            {
                ListenerPerfCounters.IncrementProtocolFailuresNamedPipe();
            }

            base.Abort(exception);
        }

        protected abstract bool CanDupHandle(out Uri viaString);

        void ContinueReading()
        {
            try
            {
                for (;;)
                {
                    if (size == 0)
                    {
                        if (readCallback == null)
                            readCallback = ReadCallback;

                        if (Connection.BeginRead(0, connectionBuffer.Length, GetRemainingTimeout(),
                            readCallback, this) == AsyncCompletionResult.Queued)
                        {
                            break;
                        }
                        GetReadResult();
                    }

                    Fx.Assert(size > 0, "");
                    for (;;)
                    {
                        int bytesDecoded = Decode(connectionBuffer, offset, size);
                        if (bytesDecoded > 0)
                        {
                            offset += bytesDecoded;
                            size -= bytesDecoded;
                        }

                        Uri via = null;
                        if (CanDupHandle(out via))
                        {
                            ListenerSessionConnection session = new ListenerSessionConnection(
                                this.Connection, this.dataRead, via, this.GetConnectionDequeuedCallback());
                            viaDecodedCallback(this, session);
                            this.ReleaseConnection();
                            return;
                        }

                        if (size == 0)
                            break;
                    }
                }
            }
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }

                AbortAndCount(e);
            }
        }

        protected abstract int Decode(byte[] buffer, int offset, int size);
        protected abstract Exception CreatePrematureEOFException();

        void GetReadResult()
        {
            offset = 0;
            size = Connection.EndRead();
            if (size == 0)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(CreatePrematureEOFException());
            }
            else
            {
                // update our snapshot of data to shuttle over to the target process
                byte[] newDataRead = new byte[dataRead.Length + size];
                Buffer.BlockCopy(dataRead, 0, newDataRead, 0, dataRead.Length);
                Buffer.BlockCopy(connectionBuffer, 0, newDataRead, this.dataRead.Length, size);
                dataRead = newDataRead;
            }
        }

        static void ReadCallback(object state)
        {
            DupHandleConnectionReader reader = (DupHandleConnectionReader)state;
            try
            {
                reader.GetReadResult();
                reader.ContinueReading();
            }
            catch (Exception e)
            {
                if (Fx.IsFatal(e))
                {
                    throw;
                }
                reader.AbortAndCount(e);
            }
        }

        public void StartReading(byte[] accruedData, TimeSpan timeout)
        {
            int accruedDataOffset = 0;
            this.receiveTimeoutHelper = new TimeoutHelper(timeout);

            if (accruedData != null)
            {
                this.dataRead = new byte[accruedData.Length + offset + size];
                Buffer.BlockCopy(accruedData, 0, this.dataRead, 0, accruedData.Length);
                accruedDataOffset = accruedData.Length;
            }
            else
            {
                this.dataRead = new byte[offset + size];
            }

            this.connectionBuffer = Connection.AsyncReadBuffer;
            Buffer.BlockCopy(this.connectionBuffer, 0, dataRead, accruedDataOffset, offset + size);

            ContinueReading();
        }
    }
}
