using System.Net.Http.Headers;
using System.Text;

namespace System.Net.Http
{
    public class HttpResponseMessage : IDisposable
    {
        private const HttpStatusCode defaultStatusCode = HttpStatusCode.OK;

        private HttpStatusCode statusCode;
        private HttpResponseHeaders headers;
        private string reasonPhrase;
        private HttpRequestMessage requestMessage;
        private Version version;
        private HttpContent content;
        private bool disposed;

        public Version Version
        {
            get { return version; }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                CheckDisposed();

                version = value;
            }
        }

        public HttpContent Content
        {
            get { return content; }
            set 
            {
                CheckDisposed();

                if (Logging.On)
                {
                    if (value == null)
                    {
                        Logging.PrintInfo(Logging.Http, this, SR.net_http_log_content_null);
                    }
                    else
                    {
                        Logging.Associate(Logging.Http, this, value);
                    }
                }
                
                content = value; 
            } 
        }

        public HttpStatusCode StatusCode
        {
            get { return statusCode; }
            set 
            {
                if (((int)value < 0) || ((int)value > 999))
                {
                    throw new ArgumentOutOfRangeException("value");
                }
                CheckDisposed();

                statusCode = value; 
            }
        }

        public string ReasonPhrase
        {
            get 
            {
                if (reasonPhrase != null)
                {
                    return reasonPhrase;
                }
                // Provide a default if one was not set
                return HttpStatusDescription.Get(StatusCode);
            }
            set
            {
                if ((value != null) && ContainsNewLineCharacter(value))
                {
                    throw new FormatException(SR.net_http_reasonphrase_format_error);
                }
                CheckDisposed();

                reasonPhrase = value; // It's OK to have a 'null' reason phrase
            }
        }

        public HttpResponseHeaders Headers
        {
            get
            {
                if (headers == null)
                {
                    headers = new HttpResponseHeaders();
                }
                return headers;
            }
        }

        public HttpRequestMessage RequestMessage
        {
            get { return requestMessage; }
            set 
            {
                CheckDisposed();
                if (Logging.On && (value != null)) Logging.Associate(Logging.Http, this, value);
                requestMessage = value; 
            }
        }

        public bool IsSuccessStatusCode
        {
            get { return ((int)statusCode >= 200) && ((int)statusCode <= 299); }
        }

        public HttpResponseMessage()
            : this(defaultStatusCode)
        {
        }

        public HttpResponseMessage(HttpStatusCode statusCode)
        {
            if (Logging.On) Logging.Enter(Logging.Http, this, ".ctor", "StatusCode: " + (int)statusCode + ", ReasonPhrase: '" + reasonPhrase + "'");

            if (((int)statusCode < 0) || ((int)statusCode > 999))
            {
                throw new ArgumentOutOfRangeException("statusCode");
            }

            this.statusCode = statusCode;
            this.version = HttpUtilities.DefaultVersion;            
            
            if (Logging.On) Logging.Exit(Logging.Http, this, ".ctor", null);
        }

        public HttpResponseMessage EnsureSuccessStatusCode()
        {
            if (!IsSuccessStatusCode)
            {
                // Disposing the content should help users: If users call EnsureSuccessStatusCode(), an exception is
                // thrown if the response status code is != 2xx. I.e. the behavior is similar to a failed request (e.g.
                // connection failure). Users don't expect to dispose the content in this case: If an exception is 
                // thrown, the object is responsible fore cleaning up its state.
                if (content != null)
                {
                    content.Dispose();
                }

                throw new HttpRequestException(string.Format(System.Globalization.CultureInfo.InvariantCulture, SR.net_http_message_not_success_statuscode, (int)statusCode,
                    ReasonPhrase));
            }
            return this;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.Append("StatusCode: ");
            sb.Append((int)statusCode);

            sb.Append(", ReasonPhrase: '");
            sb.Append(ReasonPhrase ?? "<null>");

            sb.Append("', Version: ");
            sb.Append(version);

            sb.Append(", Content: ");
            sb.Append(content == null ? "<null>" : content.GetType().FullName);

            sb.Append(", Headers:\r\n");
            sb.Append(HeaderUtilities.DumpHeaders(headers, content == null ? null : content.Headers));

            return sb.ToString();
        }

        private bool ContainsNewLineCharacter(string value)
        {
            foreach (char character in value)
            {
                if ((character == HttpRuleParser.CR) || (character == HttpRuleParser.LF))
                {
                    return true;
                }
            }
            return false;
        }

        #region IDisposable Members

        protected virtual void Dispose(bool disposing)
        {
            // The reason for this type to implement IDisposable is that it contains instances of types that implement
            // IDisposable (content). 
            if (disposing && !disposed)
            {
                disposed = true;
                if (content != null)
                {
                    content.Dispose();
                }
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion

        private void CheckDisposed()
        {
            if (disposed)
            {
                throw new ObjectDisposedException(this.GetType().FullName);
            }
        }
    }
}
