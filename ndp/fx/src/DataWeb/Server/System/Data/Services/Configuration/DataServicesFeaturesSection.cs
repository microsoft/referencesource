//---------------------------------------------------------------------
// <copyright file="DataServicesFeaturesSection.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//---------------------------------------------------------------------

namespace System.Data.Services.Configuration
{
    using System.Configuration;

    /// <summary>
    /// Features section for data services
    /// </summary>
    public sealed class DataServicesFeaturesSection : ConfigurationSection
    {
        /// <summary>
        /// Element to specify whether replace functions should be allowed in url or not.
        /// </summary>
        [ConfigurationProperty(DataServicesConfigurationConstants.ReplaceFunctionFeatureElementName)]
        public DataServicesReplaceFunctionFeature ReplaceFunction
        {
            get
            {
                return (DataServicesReplaceFunctionFeature)base[DataServicesConfigurationConstants.ReplaceFunctionFeatureElementName];
            }

            set
            {
                base[DataServicesConfigurationConstants.ReplaceFunctionFeatureElementName] = value;
            }
        }
    }
}
