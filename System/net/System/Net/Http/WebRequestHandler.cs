using System.Globalization;
using System.Net.Cache;
using System.Net.Security;
using System.Security;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;

namespace System.Net.Http
{
    public class WebRequestHandler : HttpClientHandler
    {
        #region Fields

        private AuthenticationLevel authenticationLevel;
        private TokenImpersonationLevel impersonationLevel;
        private bool allowPipelining;
        private bool unsafeAuthenticatedConnectionSharing;
        private int maxResponseHeadersLength;
        private int readWriteTimeout;
        private RequestCachePolicy cachePolicy;
#if !NET_4
        private TimeSpan continueTimeout;
        private RemoteCertificateValidationCallback serverCertificateValidationCallback;
#endif
        private X509CertificateCollection clientCertificates;

        #endregion Fields

        #region Properties

        public AuthenticationLevel AuthenticationLevel
        {
            get { return authenticationLevel; }
            set
            {
                CheckDisposedOrStarted();
                authenticationLevel = value;
            }
        }

        public TokenImpersonationLevel ImpersonationLevel
        {
            get { return impersonationLevel; }
            set
            {
                CheckDisposedOrStarted();
                impersonationLevel = value;
            }
        }

        public bool AllowPipelining
        {
            get { return allowPipelining; }
            set
            {
                CheckDisposedOrStarted();
                allowPipelining = value;
            }
        }

        public bool UnsafeAuthenticatedConnectionSharing
        {
            get { return unsafeAuthenticatedConnectionSharing; }
            [SecuritySafeCritical]
            set
            {
                CheckDisposedOrStarted();
                ExceptionHelper.WebPermissionUnrestricted.Demand();
                unsafeAuthenticatedConnectionSharing = value;
            }
        }

        public new int MaxResponseHeadersLength
        {
            get { return maxResponseHeadersLength; }
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                CheckDisposedOrStarted();
                maxResponseHeadersLength = value;
            }
        }

        public int ReadWriteTimeout
        {
            get { return readWriteTimeout; }
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                CheckDisposedOrStarted();
                readWriteTimeout = value;
            }
        }

        public RequestCachePolicy CachePolicy
        {
            get { return cachePolicy; }
            set
            {
                CheckDisposedOrStarted();
                cachePolicy = value;
            }
        }


        public TimeSpan ContinueTimeout
        {
#if NET_4
            get { throw new NotImplementedException(); }
            set { throw new NotImplementedException(); }
#else
            get { return continueTimeout; }
            set
            {
                if (value.TotalMilliseconds > int.MaxValue)
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                CheckDisposedOrStarted();
                continueTimeout = value;
            }
#endif
        }

        public new X509CertificateCollection ClientCertificates
        {
            get
            {
                if (ClientCertificateOptions != ClientCertificateOption.Manual)
                {
                    throw new InvalidOperationException(String.Format(CultureInfo.InvariantCulture,
                        WrSR.net_http_invalid_enable_first, "ClientCertificateOptions", "Manual"));
                }
                if (clientCertificates == null)
                {
                    clientCertificates = new X509CertificateCollection();
                }
                return clientCertificates;
            }
        }

        // This was copied from ServicePointManager to make it more granular.
        public RemoteCertificateValidationCallback ServerCertificateValidationCallback
        {
#if NET_4
            get { throw new NotImplementedException(); }
            set { throw new NotImplementedException(); }
#else
            get
            {
                return serverCertificateValidationCallback;
            }
            set
            {
                CheckDisposedOrStarted();
                serverCertificateValidationCallback = value;
            }
#endif
        }

        #endregion Properties

        #region Constructor

        public WebRequestHandler()
        {
            // Set HWR default values
            this.allowPipelining = true;
            this.authenticationLevel = AuthenticationLevel.MutualAuthRequested;
            this.cachePolicy = WebRequest.DefaultCachePolicy;
            this.impersonationLevel = TokenImpersonationLevel.Delegation;
            this.maxResponseHeadersLength = HttpWebRequest.DefaultMaximumResponseHeadersLength;
#if NET_4
            this.readWriteTimeout = 5 * 60 * 1000; // 5 minutes
#else
            this.readWriteTimeout = HttpWebRequest.DefaultReadWriteTimeout;
            this.continueTimeout = new TimeSpan(0, 0, 0, 0, HttpWebRequest.DefaultContinueTimeout);
#endif
            this.unsafeAuthenticatedConnectionSharing = false;
            this.clientCertificates = null; // only create collection when required.
        }

        #endregion Constructor

        #region Request Setup

        internal override void InitializeWebRequest(HttpRequestMessage request, HttpWebRequest webRequest)
        {
            // WebRequestHandler specific properties
            webRequest.AuthenticationLevel = authenticationLevel;
            webRequest.CachePolicy = cachePolicy;
            webRequest.ImpersonationLevel = impersonationLevel;
            webRequest.MaximumResponseHeadersLength = maxResponseHeadersLength;
            webRequest.Pipelined = allowPipelining;
            webRequest.ReadWriteTimeout = readWriteTimeout;
            webRequest.UnsafeAuthenticatedConnectionSharing = unsafeAuthenticatedConnectionSharing;
#if !NET_4
            webRequest.ContinueTimeout = (int)continueTimeout.TotalMilliseconds;
            if (serverCertificateValidationCallback != null)
            {
                webRequest.ServerCertificateValidationCallback = serverCertificateValidationCallback;
            }
#endif
            if ((ClientCertificateOptions == ClientCertificateOption.Manual)
                && (clientCertificates != null) && (clientCertificates.Count > 0))
            {
                webRequest.ClientCertificates = clientCertificates;
            }
        }

        #endregion Request Setup
    }
}