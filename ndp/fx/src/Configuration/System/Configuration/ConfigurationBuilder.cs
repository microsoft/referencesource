//------------------------------------------------------------------------------
// <copyright file="ConfigurationBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Configuration
{
    using System.Configuration.Provider;
    using System.Xml;

    public abstract class ConfigurationBuilder : ProviderBase
    {
        public virtual XmlNode ProcessRawXml(XmlNode rawXml) {
            return rawXml;
        }

        public virtual ConfigurationSection ProcessConfigurationSection(ConfigurationSection configSection) {
            return configSection;
        }
    }
}
