//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    using Collections.Generic;
    using ComponentModel;
    using Runtime;
    using Runtime.Serialization;
    using System.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Dispatcher;
    using Threading;

    [DataContract(Name = "ServiceHealth", Namespace = ServiceHealthModel.Namespace)]
    public class ServiceHealthModel
    {
        public const string Namespace = "http://schemas.microsoft.com/net/2018/08/health";

        public ServiceHealthModel()
        {
        }

        public ServiceHealthModel(ServiceHostBase serviceHost)
        {
            if (serviceHost == null)
            {
                throw new ArgumentNullException(nameof(serviceHost));
            }

            this.Date = DateTimeOffset.Now;
            this.ServiceProperties = new ServicePropertiesModel(serviceHost);
            this.ProcessInformation = new ProcessInformationModel(serviceHost);
            this.ProcessThreads = new ProcessThreadsModel();
            this.ServiceEndpoints = GetServiceEndpoints(serviceHost);
            this.ChannelDispatchers = GetChannelDispatchers(serviceHost);
        }

        public ServiceHealthModel(ServiceHostBase serviceHost, DateTimeOffset serviceStartTime)
            : this(serviceHost)
        {
            if (serviceHost == null)
            {
                throw new ArgumentNullException(nameof(serviceHost));
            }

            this.ProcessInformation.SetServiceStartDate(serviceStartTime);
        }

        [DataMember]
        public DateTimeOffset Date { get; private set; }

        [DataMember]
        public ServicePropertiesModel ServiceProperties { get; private set; }

        [DataMember]
        public ProcessInformationModel ProcessInformation { get; private set; }

        [DataMember]
        public ProcessThreadsModel ProcessThreads { get; private set; }

        [DataMember]
        public ServiceEndpointModel[] ServiceEndpoints { get; private set; }

        [DataMember]
        public ChannelDispatcherModel[] ChannelDispatchers { get; private set; }

        private static ServiceEndpointModel[] GetServiceEndpoints(ServiceHostBase serviceHost)
        {
            ServiceEndpointCollection endpoints = serviceHost.Description?.Endpoints;
            List<ServiceEndpointModel> endpointList = new List<ServiceEndpointModel>(endpoints?.Count ?? 0);

            if (endpoints != null && endpoints.Count > 0)
            {
                foreach (var endpoint in endpoints)
                {
                    endpointList.Add(new ServiceEndpointModel(endpoint));
                }
            }

            return endpointList.ToArray();
        }

        private static ChannelDispatcherModel[] GetChannelDispatchers(ServiceHostBase serviceHost)
        {
            ChannelDispatcherCollection channelDispatchers = serviceHost?.ChannelDispatchers;
            List<ChannelDispatcherModel> channelDispatchersList = new List<ChannelDispatcherModel>(channelDispatchers?.Count ?? 0);

            if (channelDispatchers != null && channelDispatchers.Count > 0)
            {
                foreach (var channelDispatcher in channelDispatchers)
                {
                    channelDispatchersList.Add(new ChannelDispatcherModel(channelDispatcher));
                }
            }

            return channelDispatchersList.ToArray();
        }

        [DataContract(Name = "ServiceProperties", Namespace = ServiceHealthModel.Namespace)]
        public class ServicePropertiesModel
        {
            public ServicePropertiesModel()
            {
            }

            public ServicePropertiesModel(ServiceHostBase serviceHost)
            {
                if (serviceHost == null)
                {
                    throw new ArgumentNullException(nameof(serviceHost));
                }

                this.Name = ServiceHealthBehavior.GetServiceName(serviceHost);
                this.State = serviceHost.State;
                this.ServiceTypeName = serviceHost.Description?.ServiceType?.FullName;

                ServiceBehaviorAttribute serviceBehavior = serviceHost.Description?.Behaviors.Find<ServiceBehaviorAttribute>();
                if (serviceBehavior != null)
                {
                    this.InstanceContextMode = serviceBehavior.InstanceContextMode;
                    this.ConcurrencyMode = serviceBehavior.ConcurrencyMode;
                }

                this.ServiceBehaviorNames = GetServiceBehaviorNames(serviceHost);
                this.ServiceThrottle = new ServiceThrottleModel(serviceHost.ServiceThrottle);
                this.BaseAddresses = GetBaseAddresses(serviceHost);
            }

            [DataMember]
            public string Name { get; private set; }

            [DataMember]
            public CommunicationState State { get; private set; }

            [DataMember]
            public string ServiceTypeName { get; private set; }

            [DataMember]
            public InstanceContextMode? InstanceContextMode { get; private set; }

            [DataMember]
            public ConcurrencyMode? ConcurrencyMode { get; private set; }

            [DataMember]
            public ServiceThrottleModel ServiceThrottle { get; private set; }

            [DataMember]
            public string[] BaseAddresses { get; private set; }

            [DataMember]
            public string[] ServiceBehaviorNames { get; private set; }

            private static string[] GetServiceBehaviorNames(ServiceHostBase serviceHost)
            {
                var behaviors = serviceHost?.Description?.Behaviors;
                List<string> result = new List<string>(behaviors?.Count ?? 0);

                if (behaviors != null)
                {
                    foreach (IServiceBehavior behavior in behaviors)
                    {
                        result.Add(behavior.GetType().FullName);
                    }
                }

                return result.ToArray();
            }

            private static string[] GetBaseAddresses(ServiceHostBase serviceHost)
            {
                if (serviceHost?.BaseAddresses != null)
                {
                    int count = serviceHost.BaseAddresses.Count;
                    string[] baseAddresses = new string[count];

                    for (int i = 0; i < count; i++)
                    {
                        baseAddresses[i] = serviceHost.BaseAddresses[i].ToString();
                    }

                    return baseAddresses;
                }

                return null;
            }
        }

        [DataContract(Name = "ServiceThrottle", Namespace = ServiceHealthModel.Namespace)]
        public class ServiceThrottleModel
        {
            public ServiceThrottleModel()
            {
            }

            public ServiceThrottleModel(ServiceThrottle serviceThrottle)
            {
                this.HasThrottle = serviceThrottle != null;

                if (serviceThrottle == null)
                {
                    return;
                }

                this.CallsCount = serviceThrottle.Calls.Count;
                this.CallsCapacity = serviceThrottle.Calls.Capacity;

                this.SessionsCount = serviceThrottle.Sessions.Count;
                this.SessionsCapacity = serviceThrottle.Sessions.Capacity;

                this.InstanceContextsCount = serviceThrottle.InstanceContexts.Count;
                this.InstanceContextsCapacity = serviceThrottle.InstanceContexts.Capacity;
            }

            [DataMember]
            public bool HasThrottle { get; private set; }

            [DataMember]
            [DefaultValue(0)]
            public int CallsCount { get; private set; }

            [DataMember]
            [DefaultValue(0)]
            public int CallsCapacity { get; private set; }

            [DataMember]
            [DefaultValue(0)]
            public int SessionsCount { get; private set; }

            [DataMember]
            [DefaultValue(0)]
            public int SessionsCapacity { get; private set; }

            [DataMember]
            [DefaultValue(0)]
            public int InstanceContextsCount { get; private set; }

            [DataMember]
            [DefaultValue(0)]
            public int InstanceContextsCapacity { get; private set; }
        }

        [DataContract(Name = "ProcessInformation", Namespace = ServiceHealthModel.Namespace)]
        public class ProcessInformationModel
        {
            static string processName;
            static string gcMode;
            static int bitness;

            static ProcessInformationModel()
            {
                gcMode = GCSettings.IsServerGC ? "Server" : "Workstation";
                processName = GetProcessName();
                bitness = IntPtr.Size * 8;
            }

            public ProcessInformationModel()
            {
            }

            public ProcessInformationModel(ServiceHostBase serviceHost)
            {
                if (serviceHost == null)
                {
                    throw new ArgumentNullException(nameof(serviceHost));
                }

                this.ProcessName = processName;
                this.GCMode = gcMode;
                this.ProcessStartDate = GetProcessStartDate();
                this.Threads = new ProcessThreadsModel();
                this.Bitness = bitness;
            }

            [DataMember]
            public string ProcessName { get; private set; }

            [DataMember]
            public int Bitness { get; private set; }

            [DataMember]
            public DateTimeOffset ProcessStartDate { get; private set; }

            [DataMember]
            public DateTimeOffset ServiceStartDate { get; private set; }

            [DataMember]
            public TimeSpan Uptime { get; private set; }

            [DataMember]
            public string GCMode { get; private set; }

            [DataMember]
            public ProcessThreadsModel Threads { get; private set; }

            public void SetServiceStartDate(DateTimeOffset serviceStartTime)
            {
                this.ServiceStartDate = serviceStartTime;
                this.Uptime = DateTimeOffset.Now - serviceStartTime;
            }

            private static string GetProcessName()
            {
                try
                {
                    return IO.Path.GetFileName(System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName);
                }
                catch
                {
                    return string.Empty;
                }
            }

            private static DateTimeOffset GetProcessStartDate()
            {
                try
                {
                    return System.Diagnostics.Process.GetCurrentProcess().StartTime;
                }
                catch
                {
                    return DateTimeOffset.MinValue;
                }
            }
        }

        [DataContract(Name = "ProcessThreads", Namespace = ServiceHealthModel.Namespace)]
        public class ProcessThreadsModel
        {
            public ProcessThreadsModel()
            {
                int availableWorkerThreads;
                int availableCompletionPortThreads;
                int minWorkerThreads;
                int minCompletionPortThreads;
                int maxWorkerThreads;
                int maxCompletionPortThreads;
                int threadCount = -1;

                ThreadPool.GetAvailableThreads(out availableWorkerThreads, out availableCompletionPortThreads);
                ThreadPool.GetMinThreads(out minWorkerThreads, out minCompletionPortThreads);
                ThreadPool.GetMaxThreads(out maxWorkerThreads, out maxCompletionPortThreads);

                try
                {
                    threadCount = Process.GetCurrentProcess().Threads.Count;
                }
                catch
                {
                }

                this.AvailableWorkerThreads = availableWorkerThreads;
                this.AvailableCompletionPortThreads = availableCompletionPortThreads;
                this.MinWorkerThreads = minWorkerThreads;
                this.MinCompletionPortThreads = minCompletionPortThreads;
                this.MaxWorkerThreads = maxWorkerThreads;
                this.MaxCompletionPortThreads = maxCompletionPortThreads;
                this.NativeThreadCount = threadCount;
            }

            [DataMember]
            public int AvailableWorkerThreads { get; private set; }

            [DataMember]
            public int AvailableCompletionPortThreads { get; private set; }

            [DataMember]
            public int MinWorkerThreads { get; private set; }

            [DataMember]
            public int MinCompletionPortThreads { get; private set; }

            [DataMember]
            public int MaxWorkerThreads { get; private set; }

            [DataMember]
            public int MaxCompletionPortThreads { get; private set; }

            [DataMember]
            public int NativeThreadCount { get; private set; }
        }

        [DataContract(Name = "ServiceEndpoint", Namespace = ServiceHealthModel.Namespace)]
        public class ServiceEndpointModel
        {
            public ServiceEndpointModel()
            {
            }

            public ServiceEndpointModel(ServiceEndpoint endpoint)
            {
                if (endpoint == null)
                {
                    return;
                }

                this.Address = endpoint.Address?.Uri?.ToString();
                this.BindingName = endpoint.Binding?.Name;
                this.ContractName = endpoint.Contract?.ContractType.FullName;

                KeyedByTypeCollection<IEndpointBehavior> endpointBehaviors = endpoint?.Behaviors;

                List<string> behaviorNames = new List<string>();

                if (endpointBehaviors != null)
                {
                    foreach (IEndpointBehavior behavior in endpointBehaviors)
                    {
                        behaviorNames.Add(behavior.GetType().FullName);
                    }
                }
            }

            [DataMember]
            public string Address { get; private set; }

            [DataMember]
            public string BindingName { get; private set; }

            [DataMember]
            public string ContractName { get; private set; }

            [DataMember]
            public string[] BehaviorNames { get; private set; }
        }

        [DataContract(Name = "ChannelDispatcher", Namespace = ServiceHealthModel.Namespace)]
        public class ChannelDispatcherModel
        {
            public ChannelDispatcherModel()
            {
            }

            public ChannelDispatcherModel(ChannelDispatcherBase channelDispatcher)
            {
                if (channelDispatcher == null)
                {
                    return;
                }

                IChannelListener listener = channelDispatcher?.Listener;

                if (listener != null)
                {
                    this.ListenerUri = listener.Uri?.ToString();
                    this.ListenerState = listener.State;

                    TransportChannelListener transportListener = listener as TransportChannelListener;

                    if (transportListener != null)
                    {
                        this.MessageEncoder = transportListener?.MessageEncoderFactory?.Encoder.GetType().FullName;
                    }
                }

                ChannelDispatcher dispatcher = channelDispatcher as ChannelDispatcher;

                if (dispatcher != null)
                {
                    this.State = dispatcher.State;
                    this.BindingName = dispatcher.BindingName;
                    this.ServiceThrottle = new ServiceThrottleModel(dispatcher.ServiceThrottle);
                    this.CommunicationTimeouts = new CommunicationTimeoutsModel(dispatcher.DefaultCommunicationTimeouts);

                    if (dispatcher.Endpoints != null && dispatcher.Endpoints.Count > 0)
                    {
                        EndpointDispatcher endpointDispatcher = dispatcher.Endpoints[0];

                        if (endpointDispatcher != null)
                        {
                            this.ContractName = endpointDispatcher.ContractName;
                            this.IsSystemEndpoint = endpointDispatcher.IsSystemEndpoint;
                            this.MessageInspectors = GetMessageInspectors(endpointDispatcher);
                        }
                    }
                }
            }

            [DataMember]
            public string ListenerUri { get; private set; }

            [DataMember]
            public CommunicationState? ListenerState { get; private set; }

            [DataMember]
            public string MessageEncoder { get; private set; }

            [DataMember]
            public CommunicationState? State { get; private set; }

            [DataMember]
            public string BindingName { get; private set; }

            [DataMember]
            public string ContractName { get; private set; }

            [DataMember]
            public bool IsSystemEndpoint { get; private set; }

            [DataMember]
            public string[] MessageInspectors { get; private set; }

            [DataMember]
            public ServiceThrottleModel ServiceThrottle { get; private set; }

            [DataMember]
            public CommunicationTimeoutsModel CommunicationTimeouts { get; private set; }

            private static string[] GetMessageInspectors(EndpointDispatcher endpointDispatcher)
            {
                SynchronizedCollection<IDispatchMessageInspector> messageInspectors = endpointDispatcher?.DispatchRuntime?.MessageInspectors;
                List<string> messageInspectorNames = new List<string>(messageInspectors?.Count ?? 0);

                if (messageInspectors != null && messageInspectors.Count > 0)
                {
                    foreach (IDispatchMessageInspector inspector in messageInspectors)
                    {
                        messageInspectorNames.Add(inspector.GetType().FullName);
                    }
                }

                return messageInspectorNames.ToArray();
            }
        }

        [DataContract]
        public class CommunicationTimeoutsModel
        {
            public CommunicationTimeoutsModel()
            {
            }

            public CommunicationTimeoutsModel(IDefaultCommunicationTimeouts timeouts)
            {
                this.HasTimeouts = timeouts != null;

                if (timeouts == null)
                {
                    return;
                }

                this.CloseTimeout = timeouts.CloseTimeout;
                this.OpenTimeout = timeouts.OpenTimeout;
                this.ReceiveTimeout = timeouts.ReceiveTimeout;
                this.SendTimeout = timeouts.SendTimeout;
            }

            [DataMember]
            public bool HasTimeouts { get; private set; }

            [DataMember]
            public TimeSpan CloseTimeout { get; private set; }

            [DataMember]
            public TimeSpan OpenTimeout { get; private set; }

            [DataMember]
            public TimeSpan ReceiveTimeout { get; private set; }

            [DataMember]
            public TimeSpan SendTimeout { get; private set; }
        }
    }
}

