//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Runtime.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel;
    using System.Diagnostics;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Diagnostics;

    class TcpActivation : ActivationService
    {
        internal TcpActivation()
            : base(ListenerConstants.TcpActivationServiceName, Uri.UriSchemeNetTcp)
        {
        }

        protected override void OnStart(string[] args)
        {
            try
            {
                base.OnStart(args);
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

#if DEBUG
        protected override void OnCustomCommand(int command)
        {
            switch (command)
            {
                case (int)CustomCommand.DumpTable:
                    RoutingTable.DumpTables(TransportType.Tcp);
                    break;
                default:
                    break;
            }
        }
#endif

    }
}

