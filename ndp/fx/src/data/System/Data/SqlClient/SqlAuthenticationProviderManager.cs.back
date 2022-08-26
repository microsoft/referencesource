//------------------------------------------------------------------------------
// <copyright file="SqlAuthenticationProviderManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
// <owner current="true" primary="true">lxeu</owner>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using Collections.Generic;
    using Collections.Concurrent;
    using Configuration;
    using Linq;

    /// <summary>
    /// Authentication provider manager.
    /// </summary>
    internal class SqlAuthenticationProviderManager {
        private const string ActiveDirectoryPassword = "active directory password";
        private const string ActiveDirectoryIntegrated = "active directory integrated";
        private const string ActiveDirectoryInteractive = "active directory interactive";

        static SqlAuthenticationProviderManager() {
            var activeDirectoryAuthNativeProvider = new ActiveDirectoryNativeAuthenticationProvider();
            SqlAuthenticationProviderConfigurationSection configurationSection;
            try {
                configurationSection = (SqlAuthenticationProviderConfigurationSection)ConfigurationManager.GetSection(SqlAuthenticationProviderConfigurationSection.Name);
            } catch (ConfigurationErrorsException e) {
                throw SQL.CannotGetAuthProviderConfig(e);
            }
            Instance = new SqlAuthenticationProviderManager(configurationSection);
            Instance.SetProvider(SqlAuthenticationMethod.ActiveDirectoryIntegrated, activeDirectoryAuthNativeProvider);
            Instance.SetProvider(SqlAuthenticationMethod.ActiveDirectoryPassword, activeDirectoryAuthNativeProvider);
        }
        public static readonly SqlAuthenticationProviderManager Instance;

        private readonly string _typeName;
        private readonly SqlAuthenticationInitializer _initializer;
        private readonly IReadOnlyCollection<SqlAuthenticationMethod> _authenticationsWithAppSpecifiedProvider;
        private readonly ConcurrentDictionary<SqlAuthenticationMethod, SqlAuthenticationProvider> _providers;
        private readonly SqlClientLogger _sqlAuthLogger = new SqlClientLogger();

        /// <summary>
        /// Constructor.
        /// </summary>
        public SqlAuthenticationProviderManager(SqlAuthenticationProviderConfigurationSection configSection) {
            _typeName = GetType().Name;
            var methodName = "Ctor";
            _providers = new ConcurrentDictionary<SqlAuthenticationMethod, SqlAuthenticationProvider>();
            var authenticationsWithAppSpecifiedProvider = new HashSet<SqlAuthenticationMethod>();
            _authenticationsWithAppSpecifiedProvider = authenticationsWithAppSpecifiedProvider;

            if (configSection == null) {
                _sqlAuthLogger.LogInfo(_typeName, methodName, "No SqlAuthProviders configuration section found.");
                return;
            }

            // Create user-defined auth initializer, if any.
            //
            if (!string.IsNullOrEmpty(configSection.InitializerType)) {
                try {
                    var initializerType = Type.GetType(configSection.InitializerType, true);
                    _initializer = (SqlAuthenticationInitializer)Activator.CreateInstance(initializerType);
                    _initializer.Initialize();
                } catch (Exception e) {
                    throw SQL.CannotCreateSqlAuthInitializer(configSection.InitializerType, e);
                }
                _sqlAuthLogger.LogInfo(_typeName, methodName, "Created user-defined SqlAuthenticationInitializer.");
            } else {
                _sqlAuthLogger.LogInfo(_typeName, methodName, "No user-defined SqlAuthenticationInitializer found.");
            }

            // add user-defined providers, if any.
            //
            if (configSection.Providers != null && configSection.Providers.Count > 0) {
                foreach (ProviderSettings providerSettings in configSection.Providers) {
                    SqlAuthenticationMethod authentication = AuthenticationEnumFromString(providerSettings.Name);
                    SqlAuthenticationProvider provider;
                    try {
                        var providerType = Type.GetType(providerSettings.Type, true);
                        provider = (SqlAuthenticationProvider)Activator.CreateInstance(providerType);
                    } catch (Exception e) {
                        throw SQL.CannotCreateAuthProvider(authentication.ToString(), providerSettings.Type, e);
                    }
                    if (!provider.IsSupported(authentication)) throw SQL.UnsupportedAuthenticationByProvider(authentication.ToString(), providerSettings.Type);

                    _providers[authentication] = provider;
                    authenticationsWithAppSpecifiedProvider.Add(authentication);
                    _sqlAuthLogger.LogInfo(_typeName, methodName, $"Added user-defined auth provider: {providerSettings.Type} for authentication {authentication}.");
                }
            } else {
                _sqlAuthLogger.LogInfo(_typeName, methodName, "No user-defined auth providers.");
            }
        }

        /// <summary>
        /// Get an authentication provider by method.
        /// </summary>
        /// <param name="authenticationMethod">Authentication method.</param>
        /// <returns>Authentication provider or null if not found.</returns>
        public SqlAuthenticationProvider GetProvider(SqlAuthenticationMethod authenticationMethod) {
            SqlAuthenticationProvider value;
            return _providers.TryGetValue(authenticationMethod, out value) ? value : null;
        }

        /// <summary>
        /// Set an authentication provider by method.
        /// </summary>
        /// <param name="authenticationMethod">Authentication method.</param>
        /// <param name="provider">Authentication provider.</param>
        /// <returns>True if succeeded, false otherwise, e.g., the existing provider disallows overriding.</returns>
        public bool SetProvider(SqlAuthenticationMethod authenticationMethod, SqlAuthenticationProvider provider) {
            if (!provider.IsSupported(authenticationMethod)) throw SQL.UnsupportedAuthenticationByProvider(authenticationMethod.ToString(), provider.GetType().Name);

            var methodName = "SetProvider";
            if (_authenticationsWithAppSpecifiedProvider.Contains(authenticationMethod)) {
                _sqlAuthLogger.LogError(_typeName, methodName, $"Failed to add provider {GetProviderType(provider)} because a user-defined provider with type {GetProviderType(_providers[authenticationMethod])} already existed for authentication {authenticationMethod}.");
                return false;
            }
            _providers.AddOrUpdate(authenticationMethod, provider, (key, oldProvider) => {
                if (oldProvider != null) {
                    oldProvider.BeforeUnload(authenticationMethod);
                }
                if (provider != null) {
                    provider.BeforeLoad(authenticationMethod);
                }
                _sqlAuthLogger.LogInfo(_typeName, methodName, $"Added auth provider {GetProviderType(provider)}, overriding existed provider {GetProviderType(oldProvider)} for authentication {authenticationMethod}.");
                return provider;
            });
            return true;
        }

        private static SqlAuthenticationMethod AuthenticationEnumFromString(string authentication) {
            switch (authentication.ToLowerInvariant()) {
                case ActiveDirectoryIntegrated: return SqlAuthenticationMethod.ActiveDirectoryIntegrated;
                case ActiveDirectoryPassword: return SqlAuthenticationMethod.ActiveDirectoryPassword;
                case ActiveDirectoryInteractive: return SqlAuthenticationMethod.ActiveDirectoryInteractive;
                default:
                    throw SQL.UnsupportedAuthentication(authentication);
            }
        }

        private static string GetProviderType(SqlAuthenticationProvider provider) {
            if (provider == null) return "null";
            return provider.GetType().FullName;
        }
    }

    /// <summary>
    /// The configuration section definition for reading app.config.
    /// </summary>
    internal class SqlAuthenticationProviderConfigurationSection : ConfigurationSection {
        public const string Name = "SqlAuthenticationProviders";

        /// <summary>
        /// User-defined auth providers.
        /// </summary>
        [ConfigurationProperty("providers")]
        public ProviderSettingsCollection Providers => (ProviderSettingsCollection)base["providers"];

        /// <summary>
        /// User-defined initializer.
        /// </summary>
        [ConfigurationProperty("initializerType")]
        public string InitializerType => base["initializerType"] as string;
    }

    /// <summary>
    /// The abstract initializer class that users can implement to initialize their component before SqlAuthenticationProviderManager starts.
    /// </summary>
    public abstract class SqlAuthenticationInitializer {
        /// <summary>
        /// The initialize callback from SqlAuthenticationProviderManager. This is called before SqlAuthenticationProviderManager loads providers.
        /// </summary>
        public abstract void Initialize();
    }
}
