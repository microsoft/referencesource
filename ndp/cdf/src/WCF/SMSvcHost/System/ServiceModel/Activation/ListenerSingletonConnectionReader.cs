//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.ServiceModel.Channels;

    class ListenerSingletonConnectionReader : DupHandleConnectionReader
    {
        ServerSingletonDecoder decoder;

        public ListenerSingletonConnectionReader(IConnection connection, 
            Action connectionDequeuedCallback, TransportType transportType, 
            long streamPosition, int offset, int size, 
            ConnectionClosedCallback closedCallback, ViaDecodedCallback viaDecodedCallback)
            : base(connection, connectionDequeuedCallback, transportType, offset, size, closedCallback, viaDecodedCallback)
        {
            this.decoder = new ServerSingletonDecoder(streamPosition, ListenerConstants.MaxUriSize, ListenerConstants.SharedMaxContentTypeSize);
        }

        protected override bool CanDupHandle(out Uri via)
        {
            if (decoder.CurrentState == ServerSingletonDecoder.State.ReadingContentTypeRecord)
            {
                via = decoder.Via;
                return true;
            }
            else
            {
                via = null;
                return false;
            }
        }

        protected override int Decode(byte[] buffer, int offset, int size)
        {
            return decoder.Decode(buffer, offset, size);
        }

        protected override Exception CreatePrematureEOFException()
        {
            return decoder.CreatePrematureEOFException();
        }
    }
}
