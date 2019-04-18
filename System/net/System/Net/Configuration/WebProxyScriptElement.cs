//------------------------------------------------------------------------------
// <copyright file="WebProxyScriptElement.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net.Configuration
{
    using System;
    using System.ComponentModel;
    using System.Configuration;
    using System.Security.Permissions;

    public sealed class WebProxyScriptElement : ConfigurationElement
    {
        public WebProxyScriptElement()
        {
            this.properties.Add(this.autoConfigUrlRetryInterval);
            this.properties.Add(this.downloadTimeout);
            /* Not used with Managed JScript
            this.properties.Add(this.executionTimeout);
            */
        }

        protected override void PostDeserialize()
        {
            // Perf optimization. If the configuration is coming from machine.config
            // It is safe and we don't need to check for permissions.
            if (EvaluationContext.IsMachineLevel)
                return;

            try {
                ExceptionHelper.WebPermissionUnrestricted.Demand();
            } catch (Exception exception) {

                throw new ConfigurationErrorsException(
                              SR.GetString(SR.net_config_element_permission,
                                           ConfigurationStrings.WebProxyScript),
                              exception);
            }
        }

        // After failing to download AutoConfigUrl script, WinHttpAutoProxySvc service will create a background thread and
        // keep retrying download attempts. Retry intervals are 15 seconds, 60 seconds and if that also fails, every 10 minutes.
        // We set the retry interval in .NET layer to 10 minutes after failing to download the script,
        // to reduce the overhead calling into WinHttpGetProxyForUrl.
        [ConfigurationProperty(ConfigurationStrings.AutoConfigUrlRetryInterval, DefaultValue = 600)] // 600 seconds, 10 minutes.
        public int AutoConfigUrlRetryInterval
        {
            get { return (int) this[this.autoConfigUrlRetryInterval]; }
            set { this[this.autoConfigUrlRetryInterval] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.DownloadTimeout, DefaultValue = "00:01:00")]
        public TimeSpan DownloadTimeout
        {
            get { return (TimeSpan) this[this.downloadTimeout]; }
            set { this[this.downloadTimeout] = value; }
        }

/* Not used with Managed JScript
        [ConfigurationProperty(ConfigurationStrings.ExecutionTimeout, DefaultValue = "00:00:05")]
        public TimeSpan ExecutionTimeout
        {
            get { return (TimeSpan) this[this.executionTimeout]; }
            set { this[this.executionTimeout] = value; }
        }
*/

        protected override ConfigurationPropertyCollection Properties
        {
            get
            {
                return this.properties;
            }
        }

        ConfigurationPropertyCollection properties = new ConfigurationPropertyCollection();

        readonly ConfigurationProperty autoConfigUrlRetryInterval =
            new ConfigurationProperty(ConfigurationStrings.AutoConfigUrlRetryInterval,
                                      typeof(int),
                                      600,
                                      null,
                                      new RetryIntervalValidator(),
                                      ConfigurationPropertyOptions.None);

        readonly ConfigurationProperty downloadTimeout =
            new ConfigurationProperty(ConfigurationStrings.DownloadTimeout,
                                      typeof(TimeSpan),
                                      TimeSpan.FromMinutes(1),
                                      null,
                                      new TimeSpanValidator(new TimeSpan(0, 0, 0), TimeSpan.MaxValue, false),
                                      ConfigurationPropertyOptions.None);

        /* Not used with Managed JScript
                readonly ConfigurationProperty executionTimeout =
                    new ConfigurationProperty(ConfigurationStrings.ExecutionTimeout,
                                              typeof(TimeSpan),
                                              TimeSpan.FromSeconds(5),
                                              ConfigurationPropertyOptions.None);
        */

        private class RetryIntervalValidator : ConfigurationValidatorBase
        {
            public override bool CanValidate(Type type)
            {
                return type == typeof(int);
            }
            public override void Validate(object value)
            {
                int size = (int)value;
                if (size < 0)
                {
                    throw new ArgumentOutOfRangeException("value", size,
                        SR.GetString(SR.ArgumentOutOfRange_Bounds_Lower_Upper, 0, Int32.MaxValue));
                }
            }
        }
    }
}
