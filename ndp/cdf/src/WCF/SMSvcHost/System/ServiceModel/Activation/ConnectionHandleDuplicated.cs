//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.ServiceModel.Channels;

    delegate void ViaDecodedCallback(InitialServerConnectionReader connectionReader, ListenerSessionConnection session);
    delegate void ConnectionHandleDuplicated(ListenerSessionConnection session);
}
