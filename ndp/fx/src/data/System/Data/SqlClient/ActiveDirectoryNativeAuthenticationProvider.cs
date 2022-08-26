//------------------------------------------------------------------------------
// <copyright file="ActiveDirectoryNativeAuthenticationProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
// <owner current="true" primary="true">lxeu</owner>
//------------------------------------------------------------------------------

using System.Threading.Tasks;

namespace System.Data.SqlClient {

    /// <summary>
    /// Default auth provider for AD Integrated.
    /// </summary>
    internal class ActiveDirectoryNativeAuthenticationProvider : SqlAuthenticationProvider {
        private readonly string _type = typeof(ActiveDirectoryNativeAuthenticationProvider).Name;
        private readonly SqlClientLogger _logger = new SqlClientLogger();

        /// <summary>
        /// Get token.
        /// </summary>
        public override Task<SqlAuthenticationToken> AcquireTokenAsync(SqlAuthenticationParameters parameters) => Task.Run(() => {
            long expiresOnFileTime = 0;
            byte[] token;
            if (parameters.AuthenticationMethod == SqlAuthenticationMethod.ActiveDirectoryIntegrated) {
                token = ADALNativeWrapper.ADALGetAccessTokenForWindowsIntegrated(parameters.Authority, parameters.Resource, parameters.ConnectionId, ActiveDirectoryAuthentication.AdoClientId, ref expiresOnFileTime);
                return new SqlAuthenticationToken(token, DateTimeOffset.FromFileTime(expiresOnFileTime));
            } else {
                token = ADALNativeWrapper.ADALGetAccessToken(parameters.UserId, parameters.Password, parameters.Authority, parameters.Resource, parameters.ConnectionId, ActiveDirectoryAuthentication.AdoClientId, ref expiresOnFileTime);
                return new SqlAuthenticationToken(token, DateTimeOffset.FromFileTime(expiresOnFileTime));
            }
        });

        /// <summary>
        /// Checks support for authentication type in lower case.
        /// </summary>
        public override bool IsSupported(SqlAuthenticationMethod authentication) {
            return authentication == SqlAuthenticationMethod.ActiveDirectoryIntegrated
                || authentication == SqlAuthenticationMethod.ActiveDirectoryPassword;
        }

        public override void BeforeLoad(SqlAuthenticationMethod authentication) {
            _logger.LogInfo(_type, "BeforeLoad", $"being loaded into SqlAuthProviders for {authentication}.");
        }

        public override void BeforeUnload(SqlAuthenticationMethod authentication) {
            _logger.LogInfo(_type, "BeforeUnload", $"being unloaded from SqlAuthProviders for {authentication}.");
        }
    }
}
