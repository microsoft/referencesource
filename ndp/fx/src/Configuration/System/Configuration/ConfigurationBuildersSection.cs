//------------------------------------------------------------------------------
// <copyright file="ConfigurationBuildersSection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Configuration
{
    using System.Collections;
    using System.Collections.Specialized;
    using System.IO;
    using System.Xml;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Diagnostics.CodeAnalysis;

    public sealed class ConfigurationBuildersSection : ConfigurationSection
    {
        private const string _ignoreLoadFailuresSwitch = "ConfigurationBuilders.IgnoreLoadFailure";   // Keep in sync with System.Web.Hosting.ApplicationManager

        public ConfigurationBuilder GetBuilderFromName(string builderName)
        {
            string[] builderChain = builderName.Split(new char[] { ',' });
            bool throwOnLoadError = (AppDomain.CurrentDomain.GetData(_ignoreLoadFailuresSwitch) == null);

            // If only one builder is specified, just use it directly
            if (builderChain.Length == 1) {
                ProviderSettings ps = Builders[builderName];

                if (ps == null) {
                    throw new ConfigurationErrorsException(SR.GetString(SR.Config_builder_not_found, builderName));
                }

                try {
                    return InstantiateBuilder(ps);
                }
                catch (FileNotFoundException) {
                    if (throwOnLoadError) {
                        throw;
                    }
                }
                catch (TypeLoadException) {
                    if (throwOnLoadError) {
                        throw;
                    }
                }
                return null;
            }


            // Otherwise, build a chain
            ConfigurationBuilderChain chain = new ConfigurationBuilderChain();
            chain.Initialize(builderName, null);

            foreach (string name in builderChain) {
                ProviderSettings ps = Builders[name.Trim()];

                if (ps == null) {
                    throw new ConfigurationErrorsException(SR.GetString(SR.Config_builder_not_found, name));
                }

                try {
                    chain.Builders.Add(InstantiateBuilder(ps));
                }
                catch (FileNotFoundException) {
                    if (throwOnLoadError) {
                        throw;
                    }
                }
                catch (TypeLoadException) {
                    if (throwOnLoadError) {
                        throw;
                    }
                }
            }

            if (chain.Builders.Count == 0) {
                return null;
            }
            return chain;
        }

        [PermissionSet(SecurityAction.Assert, Unrestricted=true)]
        [SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Justification = "This assert is potentially dangerous and shouldn't be present but is necessary for back-compat.")]
        private ConfigurationBuilder CreateAndInitializeBuilderWithAssert(Type t, ProviderSettings ps) {
            ConfigurationBuilder builder = (ConfigurationBuilder)TypeUtil.CreateInstanceWithReflectionPermission(t);
            NameValueCollection pars = ps.Parameters;
            NameValueCollection cloneParams = new NameValueCollection(pars.Count);

            foreach (string key in pars) {
                cloneParams[key] = pars[key];
            }

            try {
                builder.Initialize(ps.Name, cloneParams);
            } catch (Exception e) {
                throw ExceptionUtil.WrapAsConfigException(SR.GetString(SR.ConfigBuilder_init_error, ps.Name), e, null);
            }
            return builder;
        }

        private ConfigurationBuilder InstantiateBuilder(ProviderSettings ps)
        {
            Type t = TypeUtil.GetTypeWithReflectionPermission(ps.Type, true);
            if (!typeof(ConfigurationBuilder).IsAssignableFrom(t)) {
                throw new ConfigurationErrorsException("[" + ps.Name + "] - " + SR.GetString(SR.WrongType_of_config_builder));
            }

            // Needs to check APTCA bit.  See VSWhidbey 429996.
            if (!TypeUtil.IsTypeAllowedInConfig(t)) {
                throw new ConfigurationErrorsException("[" + ps.Name + "] - " + SR.GetString(SR.Type_from_untrusted_assembly, t.FullName));
            }

            // Needs to check Assert Fulltrust in order for runtime to work.  See VSWhidbey 429996.
            return CreateAndInitializeBuilderWithAssert(t, ps);
        }



        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////

        private static ConfigurationPropertyCollection _properties;
        private static readonly ConfigurationProperty _propBuilders =
            new ConfigurationProperty("builders", 
                                        typeof(ConfigurationBuilderSettings),
                                        new ConfigurationBuilderSettings(), 
                                        ConfigurationPropertyOptions.None);

        static ConfigurationBuildersSection()
        {
            // Property initialization
            _properties = new ConfigurationPropertyCollection();
            _properties.Add(_propBuilders);
        }

        public ConfigurationBuildersSection()
        {
        }

        protected internal override ConfigurationPropertyCollection Properties
        {
            get
            {
                return _properties;
            }
        }

        private ConfigurationBuilderSettings _Builders
        {
            get
            {
                return (ConfigurationBuilderSettings)base[_propBuilders];
            }
        }

        [ConfigurationProperty("builders")]
        public ProviderSettingsCollection Builders
        {
            get
            {
                return _Builders.Builders;
            }
        }
    }
}
