//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Activation
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Runtime;
    using System.Runtime.Diagnostics;
    using System.ServiceModel;
    using System.ServiceModel.Activation.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Description;

    abstract class SharingService
    {
        static object thisLock = new object();

        Guid controlServiceGuid;
        bool isPaused;
        ServiceHostBase controlServiceHost;
        SharedMemory sharedMemory;
        string sharedMemoryName;
        string serviceName;
        TransportType transportType;

        public const bool CanHandlePowerEvent = false;
        public const bool AutoLog = false;
        public const bool CanStop = true;
        public const bool CanShutdown = true;

        protected SharingService(TransportType transportType, string serviceName, string sharedMemoryName)
        {
            this.serviceName = serviceName;
            this.transportType = transportType;
            this.sharedMemoryName = sharedMemoryName;
        }

        static Binding CreateRegisterBinding(TransportType transportType)
        {
            NetNamedPipeBinding binding = new NetNamedPipeBinding(NetNamedPipeSecurityMode.None);
            binding.MaxReceivedMessageSize = ListenerConstants.RegistrationMaxReceivedMessageSize;
            CustomBinding customBinding = new CustomBinding(binding);
            NamedPipeTransportBindingElement namedPipeBindingElement = customBinding.Elements.Find<NamedPipeTransportBindingElement>();
            namedPipeBindingElement.ExposeConnectionProperty = true;
            namedPipeBindingElement.AllowedUsers = ListenerConfig.GetAllowAccounts(transportType);
            customBinding.ReceiveTimeout = TimeSpan.MaxValue;
            return customBinding;
        }

        public bool IsPaused { get { return isPaused; } }
        static object ThisLock { get { return thisLock; } }

#if DEBUG
        bool IsHealthy()
        {
            try
            {
                return controlServiceGuid.ToString().Equals(SharedMemory.Read(sharedMemoryName));
            }
            catch (Win32Exception)
            {
                return false;
            }
        }
#endif

        public void OnContinue()
        {
            isPaused = false;
        }

        public void OnPause()
        {
            isPaused = true;
        }

        public void OnShutdown()
        {
            Shutdown();
        }

        public void Start()
        {
            isPaused = false;

            GrantPermissionToAllowedAccounts();
            StartControlService();
            CreateSharedMemory();
        }

        void GrantPermissionToAllowedAccounts()
        {
            // SECURITY
            // we need to do this to allow services to lookup our LogonSid and ProcessToken User
            lock (ThisLock)
            {
                Utility.AddRightGrantedToAccounts(ListenerConfig.GetAllowAccounts(this.transportType),
                    ListenerUnsafeNativeMethods.PROCESS_QUERY_INFORMATION, true);

                Utility.AddRightGrantedToAccounts(ListenerConfig.GetAllowAccounts(this.transportType),
                    ListenerUnsafeNativeMethods.TOKEN_QUERY, false);
            }
        }

        void StartControlService()
        {
            controlServiceHost = null;
            Exception lastException = null;
            for (int iteration = 0; iteration < ListenerConstants.MaxRetries; iteration++)
            {
                controlServiceGuid = Guid.NewGuid();
                string listenerEndPoint = controlServiceGuid.ToString();
                try
                {
                    Type contractType;
                    if (transportType == TransportType.Tcp)
                    {
                        contractType = typeof(TcpWorkerProcess);
                    }
                    else
                    {
                        contractType = typeof(NamedPipeWorkerProcess);
                    }

                    ServiceHost typedServiceHost = new ServiceHost(contractType,
                        Utility.FormatListenerEndpoint(serviceName, listenerEndPoint));
                    typedServiceHost.ServiceThrottle.MaxConcurrentSessions = ListenerConstants.RegistrationMaxConcurrentSessions;
                    typedServiceHost.Description.Behaviors.Remove(typeof(ServiceMetadataBehavior));
                    typedServiceHost.AddServiceEndpoint(typeof(IConnectionRegister),
                        CreateRegisterBinding(this.transportType), string.Empty);

                    controlServiceHost = typedServiceHost;
                    controlServiceHost.Open();
                    break;
                }
                catch (CommunicationException exception)
                {
                    if (TD.ServiceStartPipeErrorIsEnabled())
                    {
                        Uri formattedUri = Utility.FormatListenerEndpoint(serviceName, listenerEndPoint);
                        TD.ServiceStartPipeError((formattedUri != null) ? formattedUri.ToString() : string.Empty);
                    }

                    if (DiagnosticUtility.ShouldTraceWarning)
                    {
                        ListenerTraceUtility.TraceEvent(TraceEventType.Warning, ListenerTraceCode.ServiceStartPipeError, SR.GetString(SR.TraceCodeServiceStartPipeError), this, exception);
                    }

                    lastException = exception;
                    controlServiceHost = null;
                }
            }

            if (controlServiceHost == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                    new InvalidOperationException(SR.GetString(SR.ServiceStartErrorEndpoint, this.serviceName),
                    lastException));
            }
        }

        void CreateSharedMemory()
        {
            try
            {
                sharedMemory = SharedMemory.Create(ListenerConstants.GlobalPrefix + sharedMemoryName, controlServiceGuid,
                    ListenerConfig.GetAllowAccounts(this.transportType));

                Debug.Print("SharedMemory.Create() sharedMemoryName: " + sharedMemoryName);
            }
            catch (Win32Exception exception)
            {
                Debug.Print("SharedMemory.Create() exception: " + exception);
                DiagnosticUtility.EventLog.LogEvent(TraceEventType.Error,
                    (ushort)EventLogCategory.SharingService,
                    (uint)EventLogEventId.StartErrorPublish,
                    exception.ToString());

                throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(
                    new InvalidOperationException(SR.GetString(SR.ServiceStartErrorPublish, this.serviceName),
                    exception));
            }
        }

        public void OnStop()
        {
            Shutdown();
        }

        void Shutdown()
        {
            try
            {
                if (sharedMemory != null)
                {
                    sharedMemory.Dispose();
                }

                MessageQueue.CloseAll(transportType);
            }
#pragma warning suppress 56500 // Microsoft, catch block unconditionally fails fast
            catch (Exception exception)
            {
                if (DiagnosticUtility.ShouldTraceError)
                {
                    ListenerTraceUtility.TraceEvent(TraceEventType.Error, ListenerTraceCode.ServiceShutdownError, SR.GetString(SR.TraceCodeServiceShutdownError), this, exception);
                }

                if (Fx.IsFatal(exception))
                {
                    throw;
                }

                // We exit the service gracefully so that other services that share the process will not be affected.
            }
            finally
            {
                if (controlServiceHost != null)
                {
                    controlServiceHost.Abort();
                }
            }
        }

#if DEBUG
        public void OnCustomCommand(int command)
        {
            switch (command)
            {
                case (int)CustomCommand.DumpTable:
                    RoutingTable.DumpTables(transportType);
                    break;
                case (int)CustomCommand.CheckHealth:
                    Fx.Assert(IsHealthy(), "Not healthy, killing ourselves!");
                    break;
                default:
                    break;
            }
        }
#endif
    }
#if DEBUG
    enum CustomCommand
    {
        DumpTable = 129,
        CheckHealth = 130,
    }
#endif
}
