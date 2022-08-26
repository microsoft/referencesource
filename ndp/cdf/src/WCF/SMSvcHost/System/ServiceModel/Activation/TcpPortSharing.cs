//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System.Runtime.Diagnostics;
    using System.ServiceProcess;
    using System.Threading;
    using System.ServiceModel.Activation.Diagnostics;
    using System.Diagnostics;    

    class TcpPortSharing : ServiceBase
    {
        TcpPortSharingCore serviceCore;

        internal TcpPortSharing()
            : base()
        {
            this.serviceCore = new TcpPortSharingCore();

            this.ServiceName = ListenerConstants.TcpPortSharingServiceName;
            this.CanPauseAndContinue = true;
            this.CanHandlePowerEvent = SharingService.CanHandlePowerEvent;
            this.AutoLog = SharingService.AutoLog;
            this.CanStop = SharingService.CanStop;
            this.CanShutdown = SharingService.CanShutdown;
        }

        internal bool IsPaused { get { return serviceCore.IsPaused; } }

        protected override void OnContinue()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceContinue, SR.GetString(SR.TraceCodeServiceContinue), this);
            }

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
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServicePause, SR.GetString(SR.TraceCodeServicePause), this);
            }

            serviceCore.OnPause();
        }

        protected override void OnShutdown()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceShutdown, SR.GetString(SR.TraceCodeServiceShutdown), this);
            }

            base.RequestAdditionalTime(ListenerConstants.ServiceStopTimeout);
            serviceCore.OnShutdown();
            Stop();
        }

        protected override void OnStart(string[] args)
        {
            try
            {
                if (DiagnosticUtility.ShouldTraceInformation)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceStart, SR.GetString(SR.TraceCodeServiceStart), this);
                }

                ListenerConfig.EnsureInitializedForNetTcp();

#if DEBUG
                if (DebuggableService.DelayStart(ServiceName))
                {
                    (new Thread(new ThreadStart(Start))).Start();
                    return;
                }
#endif
                Start();
            }
            catch (Exception exception)
            {
                // Log the error to eventlog.
                ListenerTraceUtility.EventLog.LogEvent(TraceEventType.Error,
                    (ushort)EventLogCategory.SharingService,
                    (uint)EventLogEventId.ServiceStartFailed,
                    false,
                    exception.ToString());

                throw;
            }
        }

        void Start()
        {
#if DEBUG
            DebuggableService.WaitForDebugger(ServiceName);
#endif
            serviceCore.Start();
        }

        protected override void OnStop()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceStop, SR.GetString(SR.TraceCodeServiceStop), this);
            }

            base.RequestAdditionalTime(ListenerConstants.ServiceStopTimeout);
            serviceCore.OnStop();
        }

        class TcpPortSharingCore : SharingService
        {
            internal TcpPortSharingCore()
                : base(TransportType.Tcp, ListenerConstants.TcpPortSharingServiceName, ListenerConstants.TcpSharedMemoryName)
            {
            }
        }
    }
}
