using System.Diagnostics.Contracts;
using System.IO;
using System.Net.Http.Headers;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http
{
    // HttpMessageHandler.SendAsync is protected/internal so this class is needed to allow 
    // developers to write custom clients that invoke handlers.
    public class HttpMessageInvoker : IDisposable
    {
        private volatile bool disposed;
        private bool disposeHandler;
        private HttpMessageHandler handler;

        public HttpMessageInvoker(HttpMessageHandler handler)
            : this(handler, true)
        {
        }

        public HttpMessageInvoker(HttpMessageHandler handler, bool disposeHandler)
        {
            if (Logging.On) Logging.Enter(Logging.Http, this, ".ctor", handler);

            if (handler == null)
            {
                throw new ArgumentNullException("handler");
            }

            if (Logging.On) Logging.Associate(Logging.Http, this, handler);

            this.handler = handler;
            this.disposeHandler = disposeHandler;

            if (Logging.On) Logging.Exit(Logging.Http, this, ".ctor", null);
        }

        public virtual Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, 
            CancellationToken cancellationToken)
        {
            if (request == null)
            {
                throw new ArgumentNullException("request");
            }
            CheckDisposed();

            if (Logging.On) Logging.Enter(Logging.Http, this, "SendAsync", 
                Logging.GetObjectLogHash(request) + ": " + request);
            
            Task<HttpResponseMessage> task = handler.SendAsync(request, cancellationToken);

            if (Logging.On) Logging.Exit(Logging.Http, this, "SendAsync", task);

            return task;
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposing && !disposed)
            {
                disposed = true;

                if (disposeHandler)
                {
                    handler.Dispose();
                }
            }
        }

        private void CheckDisposed()
        {
            if (disposed)
            {
                throw new ObjectDisposedException(GetType().FullName);
            }
        }
    }
}
