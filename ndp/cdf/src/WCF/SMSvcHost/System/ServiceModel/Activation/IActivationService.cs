//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;

    interface IActivationService
    {
        string ActivationServiceName { get; }
        string ProtocolName { get; }
        IActivatedMessageQueue CreateQueue(ListenerAdapter la, App app);
        IActivatedMessageQueue FindQueue(int queueId);
        void StopService();
    }
}

