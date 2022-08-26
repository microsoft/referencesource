//------------------------------------------------------------------------------
// <copyright file="ActiveDirectoryAuthenticationTimeoutRetryHelper.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
// <owner current="true" primary="true">lxeu</owner>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using ComponentModel;
    using Security.Cryptography;
    using Text;

    /// <summary>
    /// AD auth retry states.
    /// </summary>
    internal enum ActiveDirectoryAuthenticationTimeoutRetryState {
        NotStarted = 0,
        Retrying,
        HasLoggedIn,
    }

    /// <summary>
    /// AD auth retry helper.
    /// </summary>
    internal class ActiveDirectoryAuthenticationTimeoutRetryHelper {
        private ActiveDirectoryAuthenticationTimeoutRetryState _state = ActiveDirectoryAuthenticationTimeoutRetryState.NotStarted;
        private SqlFedAuthToken _token;
        private readonly string _typeName;
        private readonly SqlClientLogger _sqlAuthLogger = new SqlClientLogger();

        /// <summary>
        /// Constructor.
        /// </summary>
        public ActiveDirectoryAuthenticationTimeoutRetryHelper() {
            _typeName = GetType().Name;
        }

        /// <summary>
        /// Retry state.
        /// </summary>
        public ActiveDirectoryAuthenticationTimeoutRetryState State {
            get { return _state; }
            set {
                switch (_state) {
                    case ActiveDirectoryAuthenticationTimeoutRetryState.NotStarted:
                        if (value != ActiveDirectoryAuthenticationTimeoutRetryState.Retrying
                            && value != ActiveDirectoryAuthenticationTimeoutRetryState.HasLoggedIn) {
                            throw new InvalidOperationException($"Cannot transit from {_state} to {value}.");
                        }
                        break;
                    case ActiveDirectoryAuthenticationTimeoutRetryState.Retrying:
                        if (value != ActiveDirectoryAuthenticationTimeoutRetryState.HasLoggedIn) {
                            throw new InvalidOperationException($"Cannot transit from {_state} to {value}.");
                        }
                        break;
                    case ActiveDirectoryAuthenticationTimeoutRetryState.HasLoggedIn:
                        throw new InvalidOperationException($"Cannot transit from {_state} to {value}.");
                    default:
                        throw new InvalidOperationException($"Unsupported state: {value}.");
                }
                _sqlAuthLogger.LogInfo(_typeName, "SetState", $"State changed from {_state} to {value}.");
                _state = value;
            }
        }

        /// <summary>
        /// Cached token.
        /// </summary>
        public SqlFedAuthToken CachedToken {
            get {
                if (_sqlAuthLogger.IsLoggingEnabled) {
                    _sqlAuthLogger.LogInfo(_typeName, "GetCachedToken", $"Retrieved cached token {GetTokenHash(_token)}.");
                }
                return _token;
            }
            set {
                if (_sqlAuthLogger.IsLoggingEnabled) {
                    _sqlAuthLogger.LogInfo(_typeName, "SetCachedToken", $"CachedToken changed from {GetTokenHash(_token)} to {GetTokenHash(value)}.");
                }
                _token = value;
            }
        }

        /// <summary>
        /// Whether login can be retried after a client/server connection timeout due to a long-time token acquisition.
        /// </summary>
        public bool CanRetryWithSqlException(SqlException sqlex) {
            var methodName = "CheckCanRetry";
            if (_sqlAuthLogger.LogAssert(_state == ActiveDirectoryAuthenticationTimeoutRetryState.NotStarted, _typeName, methodName, $"Cannot retry due to state == {_state}.")
                && _sqlAuthLogger.LogAssert(CachedToken != null, _typeName, methodName, $"Cannot retry when cached token is null.")
                && _sqlAuthLogger.LogAssert(IsConnectTimeoutError(sqlex), _typeName, methodName, $"Cannot retry when exception is not timeout.")) {
                _sqlAuthLogger.LogInfo(_typeName, methodName, "All checks passed.");
                return true;
            }
            return false;
        }

        private static bool IsConnectTimeoutError(SqlException sqlex) {
            var innerException = sqlex.InnerException as Win32Exception;
            if (innerException == null) return false;
            return innerException.NativeErrorCode == 10054 // Server timeout
                   || innerException.NativeErrorCode == 258; // Client timeout
        }

        private static string GetTokenHash(SqlFedAuthToken token) {
            if (token == null) return "null";

            // Here we mimic how ADAL calculates hash for token. They use UTF8 instead of Unicode.
            var originalTokenString = SqlAuthenticationToken.AccessTokenStringFromBytes(token.accessToken);
            var bytesInUtf8 = Encoding.UTF8.GetBytes(originalTokenString);
            using (var sha256 = SHA256.Create()) {
                var hash = sha256.ComputeHash(bytesInUtf8);
                return Convert.ToBase64String(hash);
            }
        }
    }
}
