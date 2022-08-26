//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.Diagnostics;
    using System.ServiceProcess;    
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.Threading;

    abstract class ActivationService : ServiceBase, IActivationService
    {
        ListenerAdapter listenerAdapter;
        string protocolName;
        bool isPaused;

        protected ActivationService(string serviceName, string protocolName)
        {
            this.protocolName = protocolName;
            ServiceName = serviceName;
            CanHandlePowerEvent = false;
            AutoLog = false;
            CanStop = true;
            CanPauseAndContinue = true;
            CanShutdown = true;
        }

        public bool IsPaused { get { return isPaused; } }
        public string ActivationServiceName { get { return this.ServiceName; } }
        public string ProtocolName { get { return protocolName; } }

        public IActivatedMessageQueue CreateQueue(ListenerAdapter la, App app)
        {
            return new ActivatedMessageQueue(la, app);
        }

        public IActivatedMessageQueue FindQueue(int queueId)
        {
            return ActivatedMessageQueue.Find(queueId);
        }

        protected override void OnContinue()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceContinue, SR.GetString(SR.TraceCodeServiceContinue), this);
            }

            isPaused = false;
        }

        protected override void OnPause()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServicePause, SR.GetString(SR.TraceCodeServicePause), this);
            }

            isPaused = true;
        }

        protected override void OnShutdown()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceShutdown, SR.GetString(SR.TraceCodeServiceShutdown), this);
            }

            Shutdown();
            Stop();
        }

        protected override void OnStop()
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceStop, SR.GetString(SR.TraceCodeServiceStop), this);
            }

            Shutdown();
        }

        protected override void OnStart(string[] args)
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                ListenerTraceUtility.TraceEvent(TraceEventType.Information, ListenerTraceCode.ServiceStart, SR.GetString(SR.TraceCodeServiceStart), this);
            }

#if DEBUG
            if (DebuggableService.DelayStart(ServiceName))
            {
                (new Thread(new ThreadStart(Start))).Start();
                return;
            }
#endif
            Start();
        }

        void Start()
        {
#if DEBUG
            DebuggableService.WaitForDebugger(ServiceName);
#endif
            if (!SMSvcHost.IsWebhostSupported)
            {
                const int ERROR_NOT_SUPPORTED = 50;
                this.ExitCode = ERROR_NOT_SUPPORTED;
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new NotSupportedException(SR.GetString(SR.ServiceRequiresWas)));
            }
            isPaused = false;
            listenerAdapter = new ListenerAdapter(this);
            listenerAdapter.Open();
        }

        void Shutdown()
        {
            listenerAdapter.Close();
        }

        public void StopService()
        {
            Stop();
        }
    }
}
