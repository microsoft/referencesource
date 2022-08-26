//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.ServiceModel.Description
{
    using System.Collections;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Runtime.Diagnostics;
    using System.ServiceModel.Channels;
    using System.ServiceModel.Diagnostics;
    using System.ServiceModel.Dispatcher;

    public abstract class ServiceHealthBehaviorBase : IServiceBehavior
    {
        Uri httpGetUrl;
        Uri httpsGetUrl;

        Binding httpGetBinding;
        Binding httpsGetBinding;

        [DefaultValue(true)]
        public bool HealthDetailsEnabled { get; set; } = true;

        [DefaultValue(true)]
        public bool HttpGetEnabled { get; set; } = true;

        [DefaultValue(null)]
        [TypeConverter(typeof(UriTypeConverter))]
        public Uri HttpGetUrl
        {
            get { return this.httpGetUrl; }
            set
            {
                if (value != null && value.IsAbsoluteUri && value.Scheme != Uri.UriSchemeHttp)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.SFxServiceMetadataBehaviorUrlMustBeHttpOrRelative,
                        nameof(HttpGetUrl), Uri.UriSchemeHttp, value.ToString(), value.Scheme));
                }
                this.httpGetUrl = value;
            }
        }

        [DefaultValue(true)]
        public bool HttpsGetEnabled { get; set; } = true;

        [DefaultValue(null)]
        [TypeConverter(typeof(UriTypeConverter))]
        public Uri HttpsGetUrl
        {
            get { return this.httpsGetUrl; }
            set
            {
                if (value != null && value.IsAbsoluteUri && value.Scheme != Uri.UriSchemeHttps)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.SFxServiceMetadataBehaviorUrlMustBeHttpOrRelative,
                        nameof(HttpsGetUrl), Uri.UriSchemeHttps, value.ToString(), value.Scheme));
                }
                this.httpsGetUrl = value;
            }
        }

        public Binding HttpGetBinding
        {
            get { return this.httpGetBinding; }
            set
            {
                if (value != null)
                {
                    if (!value.Scheme.Equals(Uri.UriSchemeHttp, StringComparison.OrdinalIgnoreCase))
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.SFxBindingSchemeDoesNotMatch,
                            value.Scheme, value.GetType().ToString(), Uri.UriSchemeHttp));
                    }
                    CustomBinding customBinding = new CustomBinding(value);
                    TextMessageEncodingBindingElement textMessageEncodingBindingElement = customBinding.Elements.Find<TextMessageEncodingBindingElement>();
                    if (textMessageEncodingBindingElement != null && !textMessageEncodingBindingElement.MessageVersion.IsMatch(MessageVersion.None))
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.SFxIncorrectMessageVersion,
                            textMessageEncodingBindingElement.MessageVersion.ToString(), MessageVersion.None.ToString()));
                    }
                    HttpTransportBindingElement httpTransportBindingElement = customBinding.Elements.Find<HttpTransportBindingElement>();
                    if (httpTransportBindingElement != null)
                    {
                        httpTransportBindingElement.Method = "GET";
                    }
                    this.httpGetBinding = customBinding;
                }
            }
        }

        public Binding HttpsGetBinding
        {
            get { return this.httpsGetBinding; }
            set
            {
                if (value != null)
                {
                    if (!value.Scheme.Equals(Uri.UriSchemeHttps, StringComparison.OrdinalIgnoreCase))
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.SFxBindingSchemeDoesNotMatch,
                            value.Scheme, value.GetType().ToString(), Uri.UriSchemeHttps));
                    }
                    CustomBinding customBinding = new CustomBinding(value);
                    TextMessageEncodingBindingElement textMessageEncodingBindingElement = customBinding.Elements.Find<TextMessageEncodingBindingElement>();
                    if (textMessageEncodingBindingElement != null && !textMessageEncodingBindingElement.MessageVersion.IsMatch(MessageVersion.None))
                    {
                        throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(SR.GetString(SR.SFxIncorrectMessageVersion,
                            textMessageEncodingBindingElement.MessageVersion.ToString(), MessageVersion.None.ToString()));
                    }
                    HttpsTransportBindingElement httpsTransportBindingElement = customBinding.Elements.Find<HttpsTransportBindingElement>();
                    if (httpsTransportBindingElement != null)
                    {
                        httpsTransportBindingElement.Method = "GET";
                    }
                    this.httpsGetBinding = customBinding;
                }
            }
        }

        protected DateTimeOffset ServiceStartTime { get; private set; }

        void IServiceBehavior.Validate(ServiceDescription description, ServiceHostBase serviceHostBase)
        {
            var behaviors = description.Behaviors;

            foreach (var behavior in behaviors)
            {
                if (behavior != this && behavior is ServiceHealthBehaviorBase)
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgument(nameof(ServiceHealthBehaviorBase), SR.GetString(SR.DuplicateBehavior1, this.GetType().FullName));
                }
            }

            this.ServiceStartTime = DateTimeOffset.Now;
        }

        void IServiceBehavior.AddBindingParameters(ServiceDescription description, ServiceHostBase serviceHostBase, Collection<ServiceEndpoint> endpoints, BindingParameterCollection parameters)
        {
            if (parameters == null)
            {
                throw DiagnosticUtility.ExceptionUtility.ThrowHelperArgumentNull(nameof(parameters));
            }

            bool shouldAddBehavior = true;
            foreach (var param in parameters)
            {
                if (param is ServiceHealthBehaviorBase)
                {
                    shouldAddBehavior = false;
                    break;
                }
            }

            if (shouldAddBehavior)
            {
                parameters.Add(this);
            }
        }

        void IServiceBehavior.ApplyDispatchBehavior(ServiceDescription description, ServiceHostBase serviceHostBase)
        {
            if (!(this.HttpGetEnabled || this.HttpsGetEnabled))
                return;

            ServiceMetadataExtension mex = ServiceMetadataExtension.EnsureServiceMetadataExtension(description, serviceHostBase);
            CreateHealthEndpoints(description, serviceHostBase, mex);
        }

        public abstract void HandleHealthRequest(ServiceHostBase serviceHost, Message httpGetRequest, string[] queries, out Message replyMessage);

        private bool EnsureHealthDispatcher(ServiceHostBase host, ServiceMetadataExtension mex, Uri url, string scheme)
        {
            Uri address = host.GetVia(scheme, url == null ? new Uri(string.Empty, UriKind.Relative) : url);

            if (address == null)
            {
                return false;
            }

            ChannelDispatcher channelDispatcher = EnsureGetDispatcher(host, mex, address);
            ((ServiceMetadataExtension.HttpGetImpl)channelDispatcher.Endpoints[0].DispatchRuntime.SingletonInstanceContext.UserObject).HealthBehavior = this;
            return true;
        }

        private ChannelDispatcher EnsureGetDispatcher(ServiceHostBase host, ServiceMetadataExtension mex, Uri listenUri)
        {
            const string ServiceHealthBehaviorHttpGetBinding = "ServiceHealthBehaviorHttpGetBinding";

            ChannelDispatcher channelDispatcher = mex.FindGetDispatcher(listenUri);

            Binding binding;
            if (channelDispatcher == null)
            {
                if (listenUri.Scheme == Uri.UriSchemeHttp)
                {
                    binding = this.HttpGetBinding ?? MetadataExchangeBindings.HttpGet;
                }
                else if (listenUri.Scheme == Uri.UriSchemeHttps)
                {
                    binding = this.HttpsGetBinding ?? MetadataExchangeBindings.HttpsGet;
                }
                else
                {
                    throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new ArgumentException(SR.GetString(SR.SFxGetChannelDispatcherDoesNotSupportScheme, nameof(ChannelDispatcher), Uri.UriSchemeHttp, Uri.UriSchemeHttps)));
                }

                channelDispatcher = mex.CreateGetDispatcher(listenUri, binding, ServiceHealthBehaviorHttpGetBinding);

                host.ChannelDispatchers.Add(channelDispatcher);
            }

            if (host.ServiceThrottle != null)
            {
                ServiceThrottle throttle = new ServiceThrottle(host);
                throttle.MaxConcurrentCalls = host.ServiceThrottle.Calls.Capacity;
                throttle.MaxConcurrentSessions = host.ServiceThrottle.Sessions.Capacity;
                throttle.MaxConcurrentInstances = host.ServiceThrottle.InstanceContexts.Capacity;
                channelDispatcher.ServiceThrottle = throttle;
            }

            channelDispatcher.IsServiceThrottleReplaced = true;

            return channelDispatcher;
        }

        private void CreateHealthEndpoints(ServiceDescription description, ServiceHostBase host, ServiceMetadataExtension mex)
        {
            const string ServiceHeathBehaviorHttpHealthUrl = "ServiceHeathBehaviorHttpHealthUrl";
            const string ServiceHeathBehaviorHttpHealthEnabled = "ServiceHeathBehaviorHttpHealthEnabled";
            const string ServiceHeathBehaviorHttpsHealthUrl = "ServiceHeathBehaviorHttpsHealthUrl";
            const string ServiceHeathBehaviorHttpsHealthEnabled = "ServiceHeathBehaviorHttpsHealthEnabled";


            if (this.HttpGetEnabled)
            {
                if (!EnsureHealthDispatcher(host, mex, this.httpGetUrl, Uri.UriSchemeHttp))
                {
                    TraceWarning(this.httpGetUrl, ServiceHeathBehaviorHttpHealthUrl, ServiceHeathBehaviorHttpHealthEnabled);
                }
            }

            if (this.HttpsGetEnabled)
            {
                if (!EnsureHealthDispatcher(host, mex, this.httpsGetUrl, Uri.UriSchemeHttps))
                {
                    TraceWarning(this.httpsGetUrl, ServiceHeathBehaviorHttpsHealthUrl, ServiceHeathBehaviorHttpsHealthEnabled);
                }
            }
        }

        private static void TraceWarning(Uri address, string urlProperty, string enabledProperty)
        {
            if (DiagnosticUtility.ShouldTraceInformation)
            {
                Hashtable h = new Hashtable(2)
                {
                    { enabledProperty, "true" },
                    { urlProperty, (address == null) ? string.Empty : address.ToString() }
                };
                TraceUtility.TraceEvent(TraceEventType.Information, TraceCode.WarnServiceHealthEnabledNoBaseAddress,
                    SR.GetString(SR.TraceCodeWarnServiceHealthPageEnabledNoBaseAddress), new DictionaryTraceRecord(h), null, null);
            }
        }
    }
}

