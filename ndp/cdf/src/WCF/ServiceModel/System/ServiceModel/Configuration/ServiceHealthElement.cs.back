//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace System.ServiceModel.Configuration
{
    using System.Configuration;
    using System.ServiceModel.Description;

    public sealed partial class ServiceHealthElement : BehaviorExtensionElement
    {
        public ServiceHealthElement()
        {
        }

        [ConfigurationProperty(ConfigurationStrings.HealthDetailsEnabled, DefaultValue = true)]
        public bool HealthDetailsEnabled
        {
            get { return (bool)base[ConfigurationStrings.HealthDetailsEnabled]; }
            set { base[ConfigurationStrings.HealthDetailsEnabled] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpGetEnabled, DefaultValue = true)]
        public bool HttpGetEnabled
        {
            get { return (bool)base[ConfigurationStrings.HttpGetEnabled]; }
            set { base[ConfigurationStrings.HttpGetEnabled] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpGetUrl)]
        public Uri HttpGetUrl
        {
            get { return (Uri)base[ConfigurationStrings.HttpGetUrl]; }
            set { base[ConfigurationStrings.HttpGetUrl] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpsGetEnabled, DefaultValue = true)]
        public bool HttpsGetEnabled
        {
            get { return (bool)base[ConfigurationStrings.HttpsGetEnabled]; }
            set { base[ConfigurationStrings.HttpsGetEnabled] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpsGetUrl)]
        public Uri HttpsGetUrl
        {
            get { return (Uri)base[ConfigurationStrings.HttpsGetUrl]; }
            set { base[ConfigurationStrings.HttpsGetUrl] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpGetBinding, DefaultValue = "")]
        [StringValidator(MinLength = 0)]
        public string HttpGetBinding
        {
            get { return (string)base[ConfigurationStrings.HttpGetBinding]; }
            set { base[ConfigurationStrings.HttpGetBinding] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpsGetBinding, DefaultValue = "")]
        [StringValidator(MinLength = 0)]
        public string HttpsGetBinding
        {
            get { return (string)base[ConfigurationStrings.HttpsGetBinding]; }
            set { base[ConfigurationStrings.HttpsGetBinding] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpGetBindingConfiguration, DefaultValue = "")]
        [StringValidator(MinLength = 0)]
        public string HttpGetBindingConfiguration
        {
            get { return (string)base[ConfigurationStrings.HttpGetBindingConfiguration]; }
            set { base[ConfigurationStrings.HttpGetBindingConfiguration] = value; }
        }

        [ConfigurationProperty(ConfigurationStrings.HttpsGetBindingConfiguration, DefaultValue = "")]
        [StringValidator(MinLength = 0)]
        public string HttpsGetBindingConfiguration
        {
            get { return (string)base[ConfigurationStrings.HttpsGetBindingConfiguration]; }
            set { base[ConfigurationStrings.HttpsGetBindingConfiguration] = value; }
        }

        public override void CopyFrom(ServiceModelExtensionElement from)
        {
            base.CopyFrom(from);

            ServiceHealthElement source = (ServiceHealthElement)from;
            this.HealthDetailsEnabled = source.HealthDetailsEnabled;
            this.HttpGetEnabled = source.HttpGetEnabled;
            this.HttpGetUrl = source.HttpGetUrl;
            this.HttpsGetEnabled = source.HttpsGetEnabled;
            this.HttpsGetUrl = source.HttpsGetUrl;
            this.HttpGetBinding = source.HttpGetBinding;
            this.HttpsGetBinding = source.HttpsGetBinding;
            this.HttpGetBindingConfiguration = source.HttpGetBindingConfiguration;
            this.HttpsGetBindingConfiguration = source.HttpsGetBindingConfiguration;
    }

        protected internal override object CreateBehavior()
        {
            ServiceHealthBehavior behavior = new ServiceHealthBehavior();

            behavior.HealthDetailsEnabled = this.HealthDetailsEnabled;
            behavior.HttpGetEnabled = this.HttpGetEnabled;
            behavior.HttpGetUrl = this.HttpGetUrl;
            behavior.HttpsGetEnabled = this.HttpsGetEnabled;
            behavior.HttpsGetUrl = this.HttpsGetUrl;

            if (!String.IsNullOrEmpty(this.HttpGetBinding))
            {
                behavior.HttpGetBinding = ConfigLoader.LookupBinding(this.HttpGetBinding, this.HttpGetBindingConfiguration);
            }

            if (!String.IsNullOrEmpty(this.HttpsGetBinding))
            {
                behavior.HttpsGetBinding = ConfigLoader.LookupBinding(this.HttpsGetBinding, this.HttpsGetBindingConfiguration);
            }

            return behavior;
        }

        public override Type BehaviorType { get; } = typeof(ServiceHealthBehavior);
    }
}




