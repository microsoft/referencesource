//------------------------------------------------------------------------------
// <copyright file="ConfigurationBuilderSettings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Configuration
{
    using System.Collections;
    using System.Collections.Specialized;
    using System.Xml;
    using System.Globalization;

    public class ConfigurationBuilderSettings : ConfigurationElement
    {
        private ConfigurationPropertyCollection _properties;
        private readonly ConfigurationProperty _propBuilders =
            new ConfigurationProperty(null, typeof(ProviderSettingsCollection), null, ConfigurationPropertyOptions.IsDefaultCollection);

        public ConfigurationBuilderSettings()
        {
            // Property initialization
            _properties = new ConfigurationPropertyCollection();
            _properties.Add(_propBuilders);
        }

        protected internal override ConfigurationPropertyCollection Properties
        {
            get
            {
                return _properties;
            }
        }


        [ConfigurationProperty("", IsDefaultCollection = true, Options = ConfigurationPropertyOptions.IsDefaultCollection)]
        public ProviderSettingsCollection Builders
        {
            get
            {
                return (ProviderSettingsCollection)base[_propBuilders];
            }
        }
    }
}
