using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Diagnostics.Contracts;
using System.Threading.Tasks;

namespace System.Net.Http
{
    // Send/SendAsync() are internal & protected methods. This way customers implementing a handler that delegates 
    // creation of response messages need to derive from this type. This provides a common experience for all 
    // customers. Omitting this type and changing Send/SendAsync() to public will cause customers to implement their
    // own "delegating handlers" and also causes confusion because customers can use a handler directly to send 
    // requests (where they should use HttpClient instead).
    public abstract class DelegatingHandler : HttpMessageHandler
    {
        private HttpMessageHandler innerHandler;
        private volatile bool operationStarted = false;
        private volatile bool disposed = false;

        public HttpMessageHandler InnerHandler
        {
            get
            {
                return innerHandler;
            }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                CheckDisposedOrStarted();

                if (Logging.On) Logging.Associate(Logging.Http, this, value);
                innerHandler = value;
            }
        }

        protected DelegatingHandler()
        {
        }

        protected DelegatingHandler(HttpMessageHandler innerHandler)
        {
            InnerHandler = innerHandler;
        }

        protected internal override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            if (request == null)
            {
                throw new ArgumentNullException("request", SR.net_http_handler_norequest);
            }
            SetOperationStarted();
            return innerHandler.SendAsync(request, cancellationToken);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && !disposed)
            {
                disposed = true;
                if (innerHandler != null)
                {
                    innerHandler.Dispose();
                }
            }

            base.Dispose(disposing);
        }

        private void CheckDisposed()
        {
            if (disposed)
            {
                throw new ObjectDisposedException(GetType().FullName);
            }
        }

        private void CheckDisposedOrStarted()
        {
            CheckDisposed();
            if (operationStarted)
            {
                throw new InvalidOperationException(SR.net_http_operation_started);
            }
        }

        private void SetOperationStarted()
        {
            CheckDisposed();
            if (innerHandler == null)
            {
                throw new InvalidOperationException(SR.net_http_handler_not_assigned);
            }
            // This method flags the handler instances as "active". I.e. we executed at least one request (or are
            // in the process of doing so). This information is used to lock-down all property setters. Once a
            // Send/SendAsync operation started, no property can be changed.
            if (!operationStarted)
            {
                operationStarted = true;
            }
        }
    }
}
