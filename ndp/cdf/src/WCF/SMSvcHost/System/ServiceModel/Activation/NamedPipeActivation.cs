//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime.Diagnostics;
    using System.ServiceModel.Channels;
    using System.Diagnostics;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Diagnostics;

    class NamedPipeActivation : ActivationService
    {
        NamedPipeSharing serviceCore;

        internal NamedPipeActivation()
            : base(ListenerConstants.NamedPipeActivationServiceName, Uri.UriSchemeNetPipe)
        {
            serviceCore = new NamedPipeSharing();
        }

        protected override void OnContinue()
        {
            base.OnContinue();
            serviceCore.OnContinue();
        }

#if DEBUG
        protected override void OnCustomCommand(int command)
        {
            serviceCore.OnCustomCommand(command);
        }
#endif

        protected override void OnPause()
        {
            base.OnPause();
            serviceCore.OnPause();
        }

        protected override void OnShutdown()
        {
            base.OnShutdown();
            base.RequestAdditionalTime(ListenerConstants.ServiceStopTimeout);
            serviceCore.OnShutdown();
        }

        protected override void OnStart(string[] args)
        {
            try
            {
                ListenerConfig.EnsureInitializedForNetPipe();

                base.OnStart(args);
                // we don't support delay starting the sharing piece for named pipes
                serviceCore.Start();
            }
            catch (Exception exception)
            {
                // Log the error to eventlog.
                ListenerTraceUtility.EventLog.LogEvent(TraceEventType.Error,
                    (ushort)System.Runtime.Diagnostics.EventLogCategory.ListenerAdapter,
                    (uint)System.Runtime.Diagnostics.EventLogEventId.ServiceStartFailed,
                    false,
                    exception.ToString());

                throw;
            }
        }

        protected override void OnStop()
        {
            base.OnStop();
            base.RequestAdditionalTime(ListenerConstants.ServiceStopTimeout);
            serviceCore.OnStop();
        }

        class NamedPipeSharing : SharingService
        {
            internal NamedPipeSharing()
                : base(TransportType.NamedPipe, ListenerConstants.NamedPipeActivationServiceName, ListenerConstants.NamedPipeSharedMemoryName)
            {
            }
        }
    }
}

