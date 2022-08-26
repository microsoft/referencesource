//------------------------------------------------------------------------------
// <copyright file="ConfigurationBuilderChain.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Configuration
{
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Configuration.Provider;
    using System.Xml;

    internal class ConfigurationBuilderChain : ConfigurationBuilder
    {
        List<ConfigurationBuilder> _builders;

        public List<ConfigurationBuilder> Builders { get { return _builders; } }

        public override void Initialize(string name, NameValueCollection config) {
            _builders = new List<ConfigurationBuilder>();
            base.Initialize(name, config);
        }

        public override XmlNode ProcessRawXml(XmlNode rawXml) {
            XmlNode processedXml = rawXml;
            String currentBuilderName = null;

            try {
                foreach (ConfigurationBuilder b in _builders) {
                    currentBuilderName = b.Name;
                    processedXml = b.ProcessRawXml(processedXml);
                }
                return processedXml;
            }
            catch (Exception e) {
                throw ExceptionUtil.WrapAsConfigException(SR.GetString(SR.ConfigBuilder_processXml_error_short,
                                                        currentBuilderName), e, null);
            }
        }

        public override ConfigurationSection ProcessConfigurationSection(ConfigurationSection configSection) {
            ConfigurationSection processedConfigSection = configSection;
            String currentBuilderName = null;

            try {
                foreach (ConfigurationBuilder b in _builders) {
                    currentBuilderName = b.Name;
                    processedConfigSection = b.ProcessConfigurationSection(processedConfigSection);
                }
                return processedConfigSection;
            }
            catch (Exception e) {
                throw ExceptionUtil.WrapAsConfigException(SR.GetString(SR.ConfigBuilder_processSection_error,
                                                            currentBuilderName, configSection.SectionInformation.Name), e, null);
            }
        }
    }
}
