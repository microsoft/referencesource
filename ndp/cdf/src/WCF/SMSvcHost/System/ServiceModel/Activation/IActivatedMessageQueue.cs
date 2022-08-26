//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.ServiceModel;
    using System.ServiceModel.Channels;

    interface IActivatedMessageQueue
    {
        App App { get; }
        ListenerChannelContext ListenerChannelContext { get; }
        bool HasStartedQueueInstances { get; }
        void OnQueueInstancesStopped();
        void Delete();
        void LaunchQueueInstance();
        ListenerExceptionStatus Register(BaseUriWithWildcard url);
        void SetEnabledState(bool enabled);
        void UnregisterAll();
    }
}
