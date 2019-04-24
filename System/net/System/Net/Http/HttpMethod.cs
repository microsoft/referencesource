using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics.Contracts;

namespace System.Net.Http
{
    public class HttpMethod : IEquatable<HttpMethod>
    {
        private string method;

        private static readonly HttpMethod getMethod = new HttpMethod("GET");
        private static readonly HttpMethod putMethod = new HttpMethod("PUT");
        private static readonly HttpMethod postMethod = new HttpMethod("POST");
        private static readonly HttpMethod deleteMethod = new HttpMethod("DELETE");
        private static readonly HttpMethod headMethod = new HttpMethod("HEAD");
        private static readonly HttpMethod optionsMethod = new HttpMethod("OPTIONS");
        private static readonly HttpMethod traceMethod = new HttpMethod("TRACE");

        // Don't expose CONNECT as static property, since it's used by the transport to connect to a proxy. 
        // CONNECT is not used by users directly.

        public static HttpMethod Get
        {
            get { return getMethod; }
        }

        public static HttpMethod Put
        {
            get { return putMethod; }
        }

        public static HttpMethod Post
        {
            get { return postMethod; }
        }

        public static HttpMethod Delete
        {
            get { return deleteMethod; }
        }

        public static HttpMethod Head
        {
            get { return headMethod; }
        }

        public static HttpMethod Options
        {
            get { return optionsMethod; }
        }

        public static HttpMethod Trace
        {
            get { return traceMethod; }
        }

        public string Method
        {
            get { return method; }
        }

        public HttpMethod(string method)
        {
            if (string.IsNullOrEmpty(method))
            {
                throw new ArgumentException(SR.net_http_argument_empty_string, "method");
            }
            if (HttpRuleParser.GetTokenLength(method, 0) != method.Length)
            {
                throw new FormatException(SR.net_http_httpmethod_format_error);
            }

            this.method = method;
        }

        #region IEquatable<HttpMethod> Members

        public bool Equals(HttpMethod other)
        {
            if ((object)other == null)
            {
                return false;
            }

            if (object.ReferenceEquals(method, other.method))
            {
                // Strings are interned, so there is a good chance that two equal methods use the same reference
                // (unless they differ in case).
                return true;
            }

            return (string.Compare(method, other.method, StringComparison.OrdinalIgnoreCase) == 0);
        }

        #endregion

        public override bool Equals(object obj)
        {
            return Equals(obj as HttpMethod);
        }

        public override int GetHashCode()
        {
            return method.ToUpperInvariant().GetHashCode();
        }

        public override string ToString()
        {
            return method.ToString();
        }

        public static bool operator ==(HttpMethod left, HttpMethod right)
        {            
            if ((object)left == null)
            {
                return ((object)right == null);
            }
            else if ((object)right == null)
            {
                return ((object)left == null);
            }

            return left.Equals(right);
        }

        public static bool operator !=(HttpMethod left, HttpMethod right)
        {
            return !(left == right);
        }
    }
}
