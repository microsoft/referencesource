//------------------------------------------------------------------------------
// <copyright file="ConfigurationBuilderCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Configuration
{
    using System.Collections.Specialized;
    using System.Runtime.Serialization;
    using System.Configuration.Provider;
    using System.Xml;

    ////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////
    public class ConfigurationBuilderCollection : ProviderCollection
    {
        public override void Add(ProviderBase builder)
        {
            if (builder == null)
            {
                throw new ArgumentNullException("builder");
            }

            if (!(builder is ConfigurationBuilder))
           {
               throw new ArgumentException(SR.GetString(SR.Config_provider_must_implement_type, typeof(ConfigurationBuilder).ToString()), "builder");
           }

            base.Add(builder);
        }

        new public ConfigurationBuilder this[string name]
        {
            get
            {
                return (ConfigurationBuilder)base[name];
            }
        }
    }
}
