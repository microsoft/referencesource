//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.ServiceModel.Channels;

    class ListenerSessionConnectionReader : DupHandleConnectionReader
    {
        ServerSessionDecoder decoder;

        public ListenerSessionConnectionReader(IConnection connection,
            Action connectionDequeuedCallback, TransportType transportType, 
            long streamPosition, int offset, int size, 
            ConnectionClosedCallback closedCallback, ViaDecodedCallback viaDecodedCallback)
            : base(connection, connectionDequeuedCallback, transportType, offset, size, closedCallback, viaDecodedCallback)
        {
            this.decoder = new ServerSessionDecoder(streamPosition, ListenerConstants.MaxUriSize, ListenerConstants.SharedMaxContentTypeSize);
        }

        protected override bool CanDupHandle(out Uri via)
        {
            if (decoder.CurrentState == ServerSessionDecoder.State.PreUpgradeStart)
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
