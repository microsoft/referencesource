//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    using Collections.Generic;
    using System.Globalization;
    using Net;
    using System.Resources;
    using Runtime.Serialization;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Dispatcher;
    using Xml;

    public class ServiceHealthBehavior : ServiceHealthBehaviorBase
    {
        const string TimeSpanFormat = @"dd\.hh\:mm\:ss";
        private static readonly IComparer<int> descendingComparer = new DescendingComparer<int>();

        protected virtual bool HasXmlSupport { get; } = true;

        public override void HandleHealthRequest(ServiceHostBase serviceHost, Message httpGetRequest, string[] queries, out Message replyMessage)
        {
            const string noContentStr = "noContent";
            const string xmlStr = "xml";

            if (serviceHost == null)
            {
                throw new ArgumentNullException(nameof(serviceHost));
            }

            if (httpGetRequest == null)
            {
                throw new ArgumentNullException(nameof(httpGetRequest));
            }

            if (queries == null)
            {
                throw new ArgumentNullException(nameof(queries));
            }

            replyMessage = null;

            bool isXml = false;
            bool noContent = false;
            bool result;

            foreach (string q in queries)
            {
                if (TryParseBooleanQueryParameter(noContentStr, q, true, out result))
                {
                    noContent = result;
                }
                else if (TryParseBooleanQueryParameter(xmlStr, q, true, out result))
                {
                    isXml = result;
                }
            }

            HttpStatusCode httpStatusCode = GetHttpResponseCode(serviceHost, queries);

            if (this.HealthDetailsEnabled && !noContent)
            {
                if (isXml && HasXmlSupport)
                {
                    XmlDocument doc = GetXmlDocument(serviceHost);
                    if (doc != null)
                    {
                        replyMessage = new XmlDocumentMessage(doc);
                    }
                }
                else
                {
                    ServiceHealthSectionCollection healthInfo = GetServiceHealthSections(serviceHost);

                    if (healthInfo != null && healthInfo.Count > 0)
                    {
                        string name = ServiceHealthBehavior.GetServiceName(serviceHost);
                        replyMessage = new ServiceHealthMessage(healthInfo, name, (int)httpStatusCode);
                    }
                }
            }

            if (replyMessage == null)
            {
                replyMessage = new EmptyMessage();
            }

            AddHttpProperty(replyMessage, httpStatusCode, isXml);
        }

        protected virtual ServiceHealthSectionCollection GetServiceHealthSections(ServiceHostBase serviceHost)
        {
            if (serviceHost == null)
            {
                throw new ArgumentNullException(nameof(serviceHost));
            }

            ServiceHealthSectionCollection serviceHealthSections = new ServiceHealthSectionCollection();

            ServiceHealthModel serviceHealthModel = new ServiceHealthModel(serviceHost, this.ServiceStartTime);

            #region Service Properties
            ServiceHealthSection serviceProperties = serviceHealthSections.CreateSection(SR.GetString(SR.ServiceHealthBehavior_WCFServiceProperties), "#0C5DA4", "#ffffff");
            ServiceHealthDataCollection servicePropertiesCollection = serviceProperties.CreateElementsCollection();

            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ServiceName), serviceHealthModel.ServiceProperties.Name);
            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_State), FormatCommunicationState(serviceHealthModel.ServiceProperties.State));
            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ServiceType), serviceHealthModel.ServiceProperties.ServiceTypeName);
            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_InstanceContextMode), serviceHealthModel.ServiceProperties.InstanceContextMode?.ToString());
            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ConcurrencyMode), serviceHealthModel.ServiceProperties.ConcurrencyMode?.ToString());

            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_BaseAddresses), serviceHealthModel.ServiceProperties.BaseAddresses);
            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ServiceThrottles), FormatServiceThrottle(serviceHealthModel.ServiceProperties.ServiceThrottle));
            servicePropertiesCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ServiceBehaviors), serviceHealthModel.ServiceProperties.ServiceBehaviorNames);
            #endregion

            #region Process Information
            ServiceHealthSection processInformation = serviceHealthSections.CreateSection(SR.GetString(SR.ServiceHealthBehavior_ProcessInformation), "#2C4079", "#ffffff");
            ServiceHealthDataCollection processInformationCollection = processInformation.CreateElementsCollection();

            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ProcessName), serviceHealthModel.ProcessInformation.ProcessName);
            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ProcessBitness), serviceHealthModel.ProcessInformation.Bitness.ToString());
            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ProcessRunningSince), serviceHealthModel.ProcessInformation.ProcessStartDate.ToString());
            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ServiceRunningSince), serviceHealthModel.ProcessInformation.ServiceStartDate.ToString());
            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Uptime), serviceHealthModel.ProcessInformation.Uptime.ToString(TimeSpanFormat));
            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_GCMode), serviceHealthModel.ProcessInformation.GCMode);
            processInformationCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Threads), FormatThreads(serviceHealthModel.ProcessInformation.Threads));
            #endregion

            #region Endpoints
            if (serviceHealthModel.ServiceEndpoints != null && serviceHealthModel.ServiceEndpoints.Length > 0)
            {
                ServiceHealthSection endpointsSection = serviceHealthSections.CreateSection(SR.GetString(SR.ServiceHealthBehavior_Endpoints), "#3e7185", "#ffffff");

                foreach (ServiceHealthModel.ServiceEndpointModel serviceEndpoint in serviceHealthModel.ServiceEndpoints)
                {
                    ServiceHealthDataCollection endpointCollection = endpointsSection.CreateElementsCollection();

                    endpointCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Address), serviceEndpoint.Address);
                    endpointCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Binding), serviceEndpoint.BindingName);
                    endpointCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Contract), serviceEndpoint.ContractName);
                    endpointCollection.Add(SR.GetString(SR.ServiceHealthBehavior_EndpointBehaviors), serviceEndpoint.BehaviorNames);
                }
            }
            #endregion

            #region Channel Dispatchers
            if (serviceHealthModel.ChannelDispatchers != null && serviceHealthModel.ChannelDispatchers.Length > 0)
            {
                ServiceHealthSection channelDispatcherSection = serviceHealthSections.CreateSection(SR.GetString(SR.ServiceHealthBehavior_ChannelDispatchers), "#406BE8", "#ffffff");

                foreach (ServiceHealthModel.ChannelDispatcherModel channelDispatcher in serviceHealthModel.ChannelDispatchers)
                {
                    ServiceHealthDataCollection channelDispatcherCollection = channelDispatcherSection.CreateElementsCollection();
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ListenerUri), channelDispatcher.ListenerUri);
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ListenerState), FormatCommunicationState(channelDispatcher.ListenerState));                    
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Binding), channelDispatcher.BindingName);
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_State), FormatCommunicationState(channelDispatcher.State));
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_MessageEncoder), channelDispatcher.MessageEncoder);
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_Contract), channelDispatcher.ContractName);
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_IsSystemEndpoint), channelDispatcher.IsSystemEndpoint.ToString());
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_ChannelTimeouts), FormatCommunicationTimeouts(channelDispatcher.CommunicationTimeouts));
                    channelDispatcherCollection.Add(SR.GetString(SR.ServiceHealthBehavior_MessageInspectors), channelDispatcher.MessageInspectors);
                }
            }
            #endregion

            return serviceHealthSections;
        }

        protected virtual HttpStatusCode GetHttpResponseCode(ServiceHostBase serviceHost, string[] queries)
        {
            const string OnServiceFailure = "OnServiceFailure";
            const string OnDispatcherFailure = "OnDispatcherFailure";
            const string OnListenerFailure = "OnListenerFailure";
            const string OnThrottlePercentExceeded = "OnThrottlePercentExceeded";

            const HttpStatusCode defaultErrorCode = HttpStatusCode.ServiceUnavailable;

            if (serviceHost == null)
            {
                throw new ArgumentNullException(nameof(serviceHost));
            }

            if (queries == null || queries.Length == 0)
            {
                return HttpStatusCode.OK;
            }

            bool useDefaultSettings = true;
            HttpStatusCode resultCode;

            for (int i = 0; i < queries.Length; i++)
            {
                if (TryParseHttpStatusCodeQueryParameter(OnServiceFailure, queries[i], defaultErrorCode, out resultCode))
                {
                    useDefaultSettings = false;
                    if (serviceHost.State > CommunicationState.Opened)
                    {
                        return resultCode;
                    }
                }
                else if (TryParseHttpStatusCodeQueryParameter(OnDispatcherFailure, queries[i], defaultErrorCode, out resultCode))
                {
                    useDefaultSettings = false;
                    if (serviceHost.ChannelDispatchers != null)
                    {
                        foreach (var dispatcherBase in serviceHost.ChannelDispatchers)
                        {
                            ChannelDispatcher dispatcher = dispatcherBase as ChannelDispatcher;

                            if (dispatcher != null && dispatcher.State > CommunicationState.Opened)
                            {
                                return resultCode;
                            }
                        }
                    }
                }
                else if (TryParseHttpStatusCodeQueryParameter(OnListenerFailure, queries[i], defaultErrorCode, out resultCode))
                {
                    useDefaultSettings = false;
                    if (serviceHost.ChannelDispatchers != null)
                    {
                        foreach (var dispatcherBase in serviceHost.ChannelDispatchers)
                        {
                            if (dispatcherBase.Listener != null && dispatcherBase.Listener.State > CommunicationState.Opened)
                            {
                                return resultCode;
                            }
                        }
                    }
                }
                else
                {
                    string[] kvp = queries[i].Split('=');

                    if (serviceHost.ServiceThrottle != null && string.Compare(kvp[0], OnThrottlePercentExceeded, StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        useDefaultSettings = false;

                        if (kvp.Length == 2)
                        {
                            //OnThrottlePercentExceeded always expects a value.

                            string key = kvp[0];
                            string value = kvp[1];

                            string[] throttleValues =
                                value.Split(new char[] { ',', ';' }, StringSplitOptions.RemoveEmptyEntries);

                            int qsPercent;
                            int qsStatusCode;
                            SortedDictionary<int, int> throttles = new SortedDictionary<int, int>(descendingComparer);

                            foreach (string v in throttleValues)
                            {
                                string[] percentThrottlePair = v.Split(new char[] { ':' }, StringSplitOptions.RemoveEmptyEntries);

                                if (percentThrottlePair.Length == 1)
                                {
                                    //Add default error code if only a percentage was added.
                                    percentThrottlePair = new string[] { percentThrottlePair[0], ((int)defaultErrorCode).ToString() };
                                }

                                if (percentThrottlePair.Length == 2 && int.TryParse(percentThrottlePair[0], out qsPercent) && qsPercent >= 0 && qsPercent <= 100)
                                {
                                    if (!throttles.ContainsKey(qsPercent))
                                    {
                                        if (!int.TryParse(percentThrottlePair[1], out qsStatusCode) || !EnsureHttpStatusCode(qsStatusCode))
                                        {
                                            qsStatusCode = (int)defaultErrorCode;
                                        }

                                        throttles.Add(qsPercent, qsStatusCode);
                                    }
                                }
                            }

                            if (throttles.Count > 0)
                            {
                                ServiceThrottle throttle = serviceHost.ServiceThrottle;
                                int callsPercent = throttle.Calls.Capacity == 0 ? 0 : throttle.Calls.Count * 100 / throttle.Calls.Capacity;
                                int sessionsPercent = throttle.Sessions.Capacity == 0 ? 0 : throttle.Sessions.Count * 100 / throttle.Sessions.Capacity;
                                int instancesPercent = throttle.InstanceContexts.Capacity == 0 ? 0 : throttle.InstanceContexts.Count * 100 / throttle.InstanceContexts.Capacity;

                                foreach (KeyValuePair<int, int> throttleKvp in throttles)
                                {
                                    int percent = throttleKvp.Key;
                                    int code = throttleKvp.Value;

                                    if (callsPercent >= percent || sessionsPercent >= percent || instancesPercent >= percent)
                                    {
                                        return (HttpStatusCode)code;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (useDefaultSettings)
            {
                bool hasError = useDefaultSettings && serviceHost.State > CommunicationState.Opened;

                if (!hasError)
                {
                    if (serviceHost.ChannelDispatchers != null)
                    {
                        foreach (var dispatcherBase in serviceHost.ChannelDispatchers)
                        {
                            ChannelDispatcher dispatcher = dispatcherBase as ChannelDispatcher;

                            if ((dispatcherBase.Listener != null && dispatcherBase.Listener.State > CommunicationState.Opened)
                                || (dispatcher != null && dispatcher.State > CommunicationState.Opened))
                            {
                                hasError = true;
                                break;
                            }
                        }
                    }
                }

                if (hasError)
                {
                    return defaultErrorCode;
                }
            }

            return HttpStatusCode.OK;
        }

        protected virtual XmlDocument GetXmlDocument(ServiceHostBase serviceHost)
        {
            if (serviceHost == null)
            {
                throw new ArgumentNullException(nameof(serviceHost));
            }

            ServiceHealthModel model = new ServiceHealthModel(serviceHost, this.ServiceStartTime);
            return SerializeToXml<ServiceHealthModel>(model);
        }

        private static XmlDocument SerializeToXml<T>(T source) where T : class
        {
            if (source == null)
            {
                throw new ArgumentNullException(nameof(source));
            }

            var document = new XmlDocument();
            var navigator = document.CreateNavigator();

            using (var writer = navigator.AppendChild())
            {
                var serializer = new DataContractSerializer(typeof(T));
                serializer.WriteObject(writer, source);
            }

            return document;
        }

        internal static string GetServiceName(ServiceHostBase serviceHost)
        {
            string name = serviceHost.Description?.Name;

            if (string.IsNullOrWhiteSpace(name))
            {
                name = serviceHost.Description?.ServiceType.Name;
            }

            return name;
        }

        protected static bool TryParseHttpStatusCodeQueryParameter(string parameterName, string parameter, HttpStatusCode defaultErrorCode, out HttpStatusCode result)
        {
            if (parameterName == null)
            {
                throw new ArgumentNullException(parameterName);
            }

            if (parameter == null)
            {
                throw new ArgumentNullException(parameter);
            }

            result = defaultErrorCode;
            string[] kvp = parameter.Split('=');

            if (String.Compare(kvp[0], parameterName, StringComparison.OrdinalIgnoreCase) == 0)
            {
                if (kvp.Length == 2 && !string.IsNullOrWhiteSpace(kvp[1]))
                {
                    int resultCode;
                    result = int.TryParse(kvp[1], out resultCode) && EnsureHttpStatusCode(resultCode) ? (HttpStatusCode)resultCode : defaultErrorCode;
                }

                return true;
            }

            return false;
        }

        protected static bool TryParseBooleanQueryParameter(string parameterName, string parameter, bool defaultValue, out bool result)
        {
            if (parameterName == null)
            {
                throw new ArgumentNullException(parameterName);
            }

            if (parameter == null)
            {
                throw new ArgumentNullException(parameter);
            }

            result = defaultValue;
            string[] kvp = parameter.Split('=');

            if (String.Compare(kvp[0], parameterName, StringComparison.OrdinalIgnoreCase) == 0)
            {
                if (kvp.Length == 2)
                {
                    bool.TryParse(kvp[1], out result);
                }

                return true;
            }

            return false;
        }

        protected static void AddHttpProperty(Message message, HttpStatusCode status, bool isXml)
        {
            const string HtmlContentType = "text/html; charset=UTF-8";
            const string XmlContentType = "text/xml; charset=UTF-8";

            if (message == null)
            {
                throw new ArgumentNullException(nameof(message));
            }

            string contentType = isXml ? XmlContentType : HtmlContentType;

            HttpResponseMessageProperty responseProperty = new HttpResponseMessageProperty();
            responseProperty.StatusCode = status;
            responseProperty.Headers.Add(HttpResponseHeader.ContentType, contentType);
            message.Properties.Add(HttpResponseMessageProperty.Name, responseProperty);
        }

        protected static bool EnsureHttpStatusCode(int code)
        {
            return code >= 200 && code <= 599;
        }

        private string[] FormatCommunicationTimeouts(ServiceHealthModel.CommunicationTimeoutsModel timeouts)
        {
            string[] timeoutsArray = new string[4];

            if (timeouts != null && timeouts.HasTimeouts)
            {
                timeoutsArray[0] = $"{SR.GetString(SR.ServiceHealthBehavior_Close)}: <b>{timeouts.CloseTimeout.ToString(TimeSpanFormat)}</b>";
                timeoutsArray[1] = $"{SR.GetString(SR.ServiceHealthBehavior_Open)}: <b>{timeouts.OpenTimeout.ToString(TimeSpanFormat)}</b>";
                timeoutsArray[2] = $"{SR.GetString(SR.ServiceHealthBehavior_Receive)}: <b>{timeouts.ReceiveTimeout.ToString(TimeSpanFormat)}</b>";
                timeoutsArray[3] = $"{SR.GetString(SR.ServiceHealthBehavior_Send)}: <b>{timeouts.SendTimeout.ToString(TimeSpanFormat)}</b>";
            }

            return timeoutsArray;
        }

        private string[] FormatThreads(ServiceHealthModel.ProcessThreadsModel processThreads)
        {
            string[] threadsArray = new string[3];

            threadsArray[0] = $"{SR.GetString(SR.ServiceHealthBehavior_NativeThreadCount)}: <b>{processThreads.NativeThreadCount}</b>";
            threadsArray[1] = $"{SR.GetString(SR.ServiceHealthBehavior_WorkerThreads)}: {SR.GetString(SR.ServiceHealthBehavior_Available)}: <b>{processThreads.AvailableWorkerThreads}</b> {SR.GetString(SR.ServiceHealthBehavior_MaxLimit)}: <b>{processThreads.MaxWorkerThreads}</b> {SR.GetString(SR.ServiceHealthBehavior_MinLimit)}: <b>{processThreads.MinWorkerThreads}</b>";
            threadsArray[2] = $"{SR.GetString(SR.ServiceHealthBehavior_CompletionPortThreads)}: {SR.GetString(SR.ServiceHealthBehavior_Available)}: <b>{processThreads.AvailableCompletionPortThreads}</b> {SR.GetString(SR.ServiceHealthBehavior_MaxLimit)}: <b>{processThreads.MaxCompletionPortThreads}</b> {SR.GetString(SR.ServiceHealthBehavior_MinLimit)}: <b>{processThreads.MinCompletionPortThreads}</b>";

            return threadsArray;
        }

        private string[] FormatServiceThrottle(ServiceHealthModel.ServiceThrottleModel serviceThrottle)
        {
            string[] throttlesArray = new string[3];

            if (serviceThrottle != null && serviceThrottle.HasThrottle)
            {
                throttlesArray[0] = FormatThrottle(SR.GetString(SR.ServiceHealthBehavior_ConcurrentCalls), serviceThrottle.CallsCount, serviceThrottle.CallsCapacity);
                throttlesArray[1] = FormatThrottle(SR.GetString(SR.ServiceHealthBehavior_Sessions), serviceThrottle.SessionsCount, serviceThrottle.SessionsCapacity);
                throttlesArray[2] = FormatThrottle(SR.GetString(SR.ServiceHealthBehavior_Instances), serviceThrottle.InstanceContextsCount, serviceThrottle.InstanceContextsCapacity);
            }

            return throttlesArray;
        }

        private string FormatThrottle(string label, int count, int capacity)
        {
            return $"{label}: <b>{count}</b>/<b>{capacity}</b>";
        }

        private string FormatCommunicationState(CommunicationState? state)
        {
            if (state.HasValue)
            {
                return state.Value.ToString();
            }

            return string.Empty;
        }

        private class EmptyMessage : ContentOnlyMessage
        {
            public EmptyMessage()
                : base()
            {
            }

            protected override void OnWriteBodyContents(XmlDictionaryWriter writer)
            {
            }
        }

        private class XmlDocumentMessage : ContentOnlyMessage
        {
            private XmlDocument document;

            public XmlDocumentMessage(XmlDocument document)
                : base()
            {
                this.document = document;
            }

            protected override void OnWriteBodyContents(XmlDictionaryWriter writer)
            {
                this.document.WriteTo(writer);
            }
        }

        private class ServiceHealthMessage : ContentOnlyMessage
        {
            private XmlDictionaryWriter writer;
            private ServiceHealthSectionCollection healthInfo;
            private int httpStatusCode;
            private string serviceName;

            public ServiceHealthMessage(ServiceHealthSectionCollection healthInfo, string serviceName, int httpStatusCode)
                : base()
            {
                this.healthInfo = healthInfo;
                this.httpStatusCode = httpStatusCode;
                this.serviceName = serviceName;
            }

            protected override void OnWriteBodyContents(XmlDictionaryWriter writer)
            {
                this.writer = writer;

                this.writer.WriteStartElement("html");
                this.writer.WriteAttributeString("lang", GetISOLanguageNameFromResourceManager(SR.Resources));
                this.writer.WriteStartElement("head");
                WriteStyleSheet();

                this.writer.WriteElementString("title", serviceName);
                this.writer.WriteEndElement(); // </head>
                this.writer.WriteStartElement("body");

                this.writer.WriteStartElement("div");
                this.writer.WriteAttributeString("role", "main");

                WriteTitleHeader(serviceName);

                WriteServiceHealthSectionCollection();

                this.writer.WriteEndElement(); // </div>
                this.writer.WriteEndElement(); // </body>
                this.writer.WriteEndElement(); // </html>
            }

            private string GetISOLanguageNameFromResourceManager(ResourceManager rm)
            {
                try
                {
                    CultureInfo ci = CultureInfo.CurrentCulture;
                    while (ci.Name.Length > 0)
                    {
                        if (rm.GetResourceSet(ci, false, false) != null)
                            return ci.TwoLetterISOLanguageName;

                        ci = ci.Parent;
                    }
                }
                // catch all exceptions to prevent any breaks
                catch (Exception) { }

                return "en";
            }

            private void WriteServiceHealthSectionCollection()
            {
                if (this.healthInfo == null || this.healthInfo.Count == 0)
                {
                    return;
                }

                foreach (var section in this.healthInfo)
                {
                    WriteSectionTitle(section);

                    bool isEven = false;

                    foreach (var elements in section)
                    {
                        this.writer.WriteStartElement("div");
                        this.writer.WriteAttributeString("class", "section");

                        this.writer.WriteStartElement("div");
                        

                        if (isEven)
                        {
                            this.writer.WriteAttributeString("class", "section subsection_even");
                        }
                        else
                        {
                            this.writer.WriteAttributeString("class", "section subsection_odd");
                        }

                        this.writer.WriteStartElement("dl");
                        this.writer.WriteAttributeString("class", "formatted_list");

                        WriteServiceHealthElements(elements);

                        this.writer.WriteEndElement(); // </dl>
                        this.writer.WriteEndElement(); // </div>
                        this.writer.WriteEndElement(); // </div>

                        isEven = !isEven;
                    }
                }
            }

            private void WriteServiceHealthElements(ServiceHealthDataCollection elements)
            {
                foreach (var element in elements)
                {
                    if (element.Values != null && element.Values.Length > 0)
                    {
                        if (element.Values.Length == 1)
                        {
                            WriteElement(element.Key, element.Values[0]);
                        }
                        else
                        {
                            WriteElement(element.Key, element.Values);
                        }
                    }
                }
            }

            private void WriteElement(string label, string value)
            {
                if (!string.IsNullOrWhiteSpace(value))
                {
                    this.writer.WriteStartElement("div");
                    this.writer.WriteStartElement("dt");
                    this.writer.WriteStartElement("span");
                    this.writer.WriteAttributeString("class", "label");
                    this.writer.WriteString($"{label}: ");
                    this.writer.WriteEndElement(); // </span>
                    this.writer.WriteEndElement(); // </dt>
                    this.writer.WriteStartElement("dd");
                    this.writer.WriteRaw($"{value}\r\n");
                    this.writer.WriteEndElement(); // </dd>
                    this.writer.WriteEndElement(); // </div>
                }
            }

            private void WriteElement(string label, string[] values)
            {
                this.writer.WriteRaw("<br />");
                this.writer.WriteStartElement("div");
                this.writer.WriteStartElement("dt");
                this.writer.WriteStartElement("span");
                this.writer.WriteAttributeString("class", "label");
                this.writer.WriteString($"{label}: ");
                this.writer.WriteEndElement(); // </span>
                this.writer.WriteEndElement(); // </dt>
                this.writer.WriteRaw("<dd>");
                this.writer.WriteRaw("<ul>");

                foreach (string value in values)
                {
                    if (!string.IsNullOrWhiteSpace(value))
                    {
                        this.writer.WriteRaw("<li>");
                        this.writer.WriteRaw($"{value}<br />\r\n");
                        this.writer.WriteRaw("</li>");
                    }
                }

                this.writer.WriteRaw("</ul>");
                this.writer.WriteRaw("</dd>");
                this.writer.WriteEndElement(); // </div>
                this.writer.WriteRaw(Environment.NewLine);
            }

            private void WriteStyleSheet()
            {
                this.writer.WriteStartElement("style");
                this.writer.WriteAttributeString("type", "text/css");
                this.writer.WriteString("body { margin: 0px; color: #000000; font-family: Segoe UI; background-color: #ffffff }");
                this.writer.WriteString(".header { width: 100%; margin: 0px; padding: 5px 25px; text-transform: lowercase; background-color: #dceeff; font-size: 20px; }");
                this.writer.WriteString(".header_title { width: 1%; white-space: nowrap; font-weight: bold; font-size: 26px; text-transform: none; }");
                this.writer.WriteString(".header_statuscode { width: 1%; white-space: nowrap; }");
                this.writer.WriteString(".header_datetime { float: right }");
                this.writer.WriteString(".section { width: 100%; margin: 0px; padding: 5px 25px; font-size: 13px; }");
                this.writer.WriteString(".subsection_even { padding: 0px; background: #fafafa; border: 1px solid #666666; }");
                this.writer.WriteString(".subsection_odd { padding: 0px; }");
                this.writer.WriteString(".title { font-weight: bold; font-size: 1.08em; text-transform: uppercase; }");
                this.writer.WriteString(".content { font-size: 12px; padding: 0 30px; }");
                this.writer.WriteString(".label { font-weight: bolder; font-size: 1.05em; }");
                this.writer.WriteString(".bullet { font-size: 1.13em; font-weight: bold; }");
                this.writer.WriteString("ul { list-style-type: none; margin: 0 auto; padding-left: 0px}");
                this.writer.WriteString(".formatted_list dt,");
                this.writer.WriteString(".formatted_list dd {display: inline-block; vertical-align: top;}");
                this.writer.WriteString(".formatted_list dt {width:  150px;}");
                this.writer.WriteEndElement(); // </style>
            }

            private void WriteTitleHeader(string title)
            {
                this.writer.WriteStartElement("div");
                this.writer.WriteAttributeString("class", "header");

                this.writer.WriteStartElement("span");
                this.writer.WriteAttributeString("class", "header_title");
                this.writer.WriteString(title);
                this.writer.WriteString(" ");
                this.writer.WriteEndElement(); // </span>

                this.writer.WriteStartElement("span");
                this.writer.WriteAttributeString("class", "header_statuscode");
                this.writer.WriteString(this.httpStatusCode >= 0 ? $"HTTP/{this.httpStatusCode}" : " ");
                this.writer.WriteEndElement(); // </span>
                
                this.writer.WriteStartElement("span");
                this.writer.WriteAttributeString("class", "header_datetime");
                this.writer.WriteString(DateTime.Now.ToString());
                this.writer.WriteEndElement(); // </span>

                this.writer.WriteEndElement(); // </div>
            }

            private void WriteSectionTitle(ServiceHealthSection section)
            {
                this.writer.WriteStartElement("h1");
                this.writer.WriteAttributeString("class", "section title");
                this.writer.WriteAttributeString("style", $"background: {section.BackgroundColor}; color: {section.ForegroundColor}");
                this.writer.WriteString(section.Title);
                this.writer.WriteEndElement(); // </h1>
            }
        }

        private class DescendingComparer<T> : IComparer<T> where T : IComparable<T>
        {
            public int Compare(T x, T y)
            {
                return y.CompareTo(x);
            }
        }
    }
}

