//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Net.Sockets;
    using System.ServiceModel.Channels;

    class SocketSettings : ISocketListenerSettings
    {
        public int BufferSize
        {
            get
            {
                return ListenerConstants.SharedConnectionBufferSize;
            }
        }

        public bool TeredoEnabled
        {
            get
            {
                return ListenerConfig.NetTcp.TeredoEnabled;
            } 
        }

        public int ListenBacklog
        {
            get
            {
                return ListenerConfig.NetTcp.ListenBacklog;
            }
        }
    }
}
