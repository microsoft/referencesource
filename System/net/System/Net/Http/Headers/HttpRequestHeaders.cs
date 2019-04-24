using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    [SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix",
        Justification = "This is not a collection")]
    public sealed class HttpRequestHeaders : HttpHeaders
    {
        private static readonly Dictionary<string, HttpHeaderParser> parserStore;
        private static readonly HashSet<string> invalidHeaders;

        private HttpGeneralHeaders generalHeaders;
        private HttpHeaderValueCollection<MediaTypeWithQualityHeaderValue> accept;
        private HttpHeaderValueCollection<NameValueWithParametersHeaderValue> expect;
        private bool expectContinueSet;
        private HttpHeaderValueCollection<EntityTagHeaderValue> ifMatch;
        private HttpHeaderValueCollection<EntityTagHeaderValue> ifNoneMatch;
        private HttpHeaderValueCollection<TransferCodingWithQualityHeaderValue> te;
        private HttpHeaderValueCollection<ProductInfoHeaderValue> userAgent;
        private HttpHeaderValueCollection<StringWithQualityHeaderValue> acceptCharset;
        private HttpHeaderValueCollection<StringWithQualityHeaderValue> acceptEncoding;
        private HttpHeaderValueCollection<StringWithQualityHeaderValue> acceptLanguage;

        #region Request Headers

        public HttpHeaderValueCollection<MediaTypeWithQualityHeaderValue> Accept
        {
            get
            {
                if (accept == null)
                {
                    accept = new HttpHeaderValueCollection<MediaTypeWithQualityHeaderValue>(
                        HttpKnownHeaderNames.Accept, this);
                }
                return accept;
            }
        }

        [SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId = "Charset",
            Justification = "The HTTP header name is 'Accept-Charset'.")]
        public HttpHeaderValueCollection<StringWithQualityHeaderValue> AcceptCharset
        {
            get
            {
                if (acceptCharset == null)
                {
                    acceptCharset = new HttpHeaderValueCollection<StringWithQualityHeaderValue>(
                        HttpKnownHeaderNames.AcceptCharset, this);
                }
                return acceptCharset;
            }
        }

        public HttpHeaderValueCollection<StringWithQualityHeaderValue> AcceptEncoding
        {
            get
            {
                if (acceptEncoding == null)
                {
                    acceptEncoding = new HttpHeaderValueCollection<StringWithQualityHeaderValue>(
                        HttpKnownHeaderNames.AcceptEncoding, this);
                }
                return acceptEncoding;
            }
        }

        public HttpHeaderValueCollection<StringWithQualityHeaderValue> AcceptLanguage
        {
            get
            {
                if (acceptLanguage == null)
                {
                    acceptLanguage = new HttpHeaderValueCollection<StringWithQualityHeaderValue>(
                        HttpKnownHeaderNames.AcceptLanguage, this);
                }
                return acceptLanguage;
            }
        }

        public AuthenticationHeaderValue Authorization
        {
            get { return (AuthenticationHeaderValue)GetParsedValues(HttpKnownHeaderNames.Authorization); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.Authorization, value); }
        }

        public HttpHeaderValueCollection<NameValueWithParametersHeaderValue> Expect
        {
            get { return ExpectCore; }
        }

        // Note that ServicePoint.Expect100Continue is on by default. However, by default we don't set any header
        // value. I.e. ExpectContinue returns 'null' by default. The fact that HWR will add "Expect: 100-continue"
        // anyways is a transport channel feature and can be turned off by setting ExpectContinue to 'false'.
        // Remember: these headers are also used on the server side, where ExpectContinue should only be true
        // if the received request message actually has this header value set.
        public bool? ExpectContinue
        {
            get
            {
                if (ExpectCore.IsSpecialValueSet)
                {
                    return true;
                }
                if (expectContinueSet)
                {
                    return false;
                }
                return null;
            }
            set
            {
                if (value == true)
                {
                    expectContinueSet = true;
                    ExpectCore.SetSpecialValue();
                }
                else
                {
                    expectContinueSet = value != null;
                    ExpectCore.RemoveSpecialValue();
                }
            }
        }

        public string From
        {
            get { return (string)GetParsedValues(HttpKnownHeaderNames.From); }
            set
            {
                // null and empty string are equivalent. In this case it means, remove the From header value (if any).
                if (value == string.Empty)
                {
                    value = null;
                }

                if ((value != null) && !HeaderUtilities.IsValidEmailAddress(value))
                {
                    throw new FormatException(SR.net_http_headers_invalid_from_header);
                }
                SetOrRemoveParsedValue(HttpKnownHeaderNames.From, value);
            }
        }

        public string Host
        {
            get { return (string)GetParsedValues(HttpKnownHeaderNames.Host); }
            set
            {
                // null and empty string are equivalent. In this case it means, remove the Host header value (if any).
                if (value == string.Empty)
                {
                    value = null;
                }

                string host = null;
                if ((value != null) && (HttpRuleParser.GetHostLength(value, 0, false, out host) != value.Length))
                {
                    throw new FormatException(SR.net_http_headers_invalid_host_header);
                }
                SetOrRemoveParsedValue(HttpKnownHeaderNames.Host, value);
            }
        }

        public HttpHeaderValueCollection<EntityTagHeaderValue> IfMatch
        {
            get
            {
                if (ifMatch == null)
                {
                    ifMatch = new HttpHeaderValueCollection<EntityTagHeaderValue>(
                        HttpKnownHeaderNames.IfMatch, this);
                }
                return ifMatch;
            }
        }

        public DateTimeOffset? IfModifiedSince
        {
            get { return HeaderUtilities.GetDateTimeOffsetValue(HttpKnownHeaderNames.IfModifiedSince, this); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.IfModifiedSince, value); }
        }

        public HttpHeaderValueCollection<EntityTagHeaderValue> IfNoneMatch
        {
            get
            {
                if (ifNoneMatch == null)
                {
                    ifNoneMatch = new HttpHeaderValueCollection<EntityTagHeaderValue>(
                        HttpKnownHeaderNames.IfNoneMatch, this);
                }
                return ifNoneMatch;
            }
        }

        public RangeConditionHeaderValue IfRange
        {
            get { return (RangeConditionHeaderValue)GetParsedValues(HttpKnownHeaderNames.IfRange); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.IfRange, value); }
        }

        public DateTimeOffset? IfUnmodifiedSince
        {
            get { return HeaderUtilities.GetDateTimeOffsetValue(HttpKnownHeaderNames.IfUnmodifiedSince, this); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.IfUnmodifiedSince, value); }
        }

        public int? MaxForwards
        {
            get
            {
                object storedValue = GetParsedValues(HttpKnownHeaderNames.MaxForwards);
                if (storedValue != null)
                {
                    return (int)storedValue;
                }
                return null;
            }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.MaxForwards, value); }
        }


        public AuthenticationHeaderValue ProxyAuthorization
        {
            get { return (AuthenticationHeaderValue)GetParsedValues(HttpKnownHeaderNames.ProxyAuthorization); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.ProxyAuthorization, value); }
        }

        public RangeHeaderValue Range
        {
            get { return (RangeHeaderValue)GetParsedValues(HttpKnownHeaderNames.Range); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.Range, value); }
        }

        public Uri Referrer
        {
            get { return (Uri)GetParsedValues(HttpKnownHeaderNames.Referer); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.Referer, value); }
        }

        public HttpHeaderValueCollection<TransferCodingWithQualityHeaderValue> TE
        {
            get
            {
                if (te == null)
                {
                    te = new HttpHeaderValueCollection<TransferCodingWithQualityHeaderValue>(
                        HttpKnownHeaderNames.TE, this);
                }
                return te;
            }
        }

        public HttpHeaderValueCollection<ProductInfoHeaderValue> UserAgent
        {
            get
            {
                if (userAgent == null)
                {
                    userAgent = new HttpHeaderValueCollection<ProductInfoHeaderValue>(HttpKnownHeaderNames.UserAgent,
                        this);
                }
                return userAgent;
            }
        }

        private HttpHeaderValueCollection<NameValueWithParametersHeaderValue> ExpectCore
        {
            get
            {
                if (expect == null)
                {
                    expect = new HttpHeaderValueCollection<NameValueWithParametersHeaderValue>(
                        HttpKnownHeaderNames.Expect, this, HeaderUtilities.ExpectContinue);
                }
                return expect;
            }
        }

        #endregion

        #region General Headers

        public CacheControlHeaderValue CacheControl
        {
            get { return generalHeaders.CacheControl; }
            set { generalHeaders.CacheControl = value; }
        }

        public HttpHeaderValueCollection<string> Connection
        {
            get { return generalHeaders.Connection; }
        }

        public bool? ConnectionClose
        {
            get { return generalHeaders.ConnectionClose; }
            set { generalHeaders.ConnectionClose = value; }
        }

        public DateTimeOffset? Date
        {
            get { return generalHeaders.Date; }
            set { generalHeaders.Date = value; }
        }

        public HttpHeaderValueCollection<NameValueHeaderValue> Pragma
        {
            get { return generalHeaders.Pragma; }
        }

        public HttpHeaderValueCollection<string> Trailer
        {
            get { return generalHeaders.Trailer; }
        }

        // Like ContentEncoding: Order matters!
        public HttpHeaderValueCollection<TransferCodingHeaderValue> TransferEncoding
        {
            get { return generalHeaders.TransferEncoding; }
        }

        public bool? TransferEncodingChunked
        {
            get { return generalHeaders.TransferEncodingChunked; }
            set { generalHeaders.TransferEncodingChunked = value; }
        }

        public HttpHeaderValueCollection<ProductHeaderValue> Upgrade
        {
            get { return generalHeaders.Upgrade; }
        }

        public HttpHeaderValueCollection<ViaHeaderValue> Via
        {
            get { return generalHeaders.Via; }
        }

        public HttpHeaderValueCollection<WarningHeaderValue> Warning
        {
            get { return generalHeaders.Warning; }
        }

        #endregion

        internal HttpRequestHeaders()
        {
            this.generalHeaders = new HttpGeneralHeaders(this);

            base.SetConfiguration(parserStore, invalidHeaders);
        }

        static HttpRequestHeaders()
        {
            parserStore = new Dictionary<string, HttpHeaderParser>(StringComparer.OrdinalIgnoreCase);

            parserStore.Add(HttpKnownHeaderNames.Accept, MediaTypeHeaderParser.MultipleValuesParser);
            parserStore.Add(HttpKnownHeaderNames.AcceptCharset, GenericHeaderParser.MultipleValueStringWithQualityParser);
            parserStore.Add(HttpKnownHeaderNames.AcceptEncoding, GenericHeaderParser.MultipleValueStringWithQualityParser);
            parserStore.Add(HttpKnownHeaderNames.AcceptLanguage, GenericHeaderParser.MultipleValueStringWithQualityParser);
            parserStore.Add(HttpKnownHeaderNames.Authorization, GenericHeaderParser.SingleValueAuthenticationParser);
            parserStore.Add(HttpKnownHeaderNames.Expect, GenericHeaderParser.MultipleValueNameValueWithParametersParser);
            parserStore.Add(HttpKnownHeaderNames.From, GenericHeaderParser.MailAddressParser);
            parserStore.Add(HttpKnownHeaderNames.Host, GenericHeaderParser.HostParser);
            parserStore.Add(HttpKnownHeaderNames.IfMatch, GenericHeaderParser.MultipleValueEntityTagParser);
            parserStore.Add(HttpKnownHeaderNames.IfModifiedSince, DateHeaderParser.Parser);
            parserStore.Add(HttpKnownHeaderNames.IfNoneMatch, GenericHeaderParser.MultipleValueEntityTagParser);
            parserStore.Add(HttpKnownHeaderNames.IfRange, GenericHeaderParser.RangeConditionParser);
            parserStore.Add(HttpKnownHeaderNames.IfUnmodifiedSince, DateHeaderParser.Parser);
            parserStore.Add(HttpKnownHeaderNames.MaxForwards, Int32NumberHeaderParser.Parser);
            parserStore.Add(HttpKnownHeaderNames.ProxyAuthorization, GenericHeaderParser.SingleValueAuthenticationParser);
            parserStore.Add(HttpKnownHeaderNames.Range, GenericHeaderParser.RangeParser);
            parserStore.Add(HttpKnownHeaderNames.Referer, UriHeaderParser.RelativeOrAbsoluteUriParser);
            parserStore.Add(HttpKnownHeaderNames.TE, TransferCodingHeaderParser.MultipleValueWithQualityParser);
            parserStore.Add(HttpKnownHeaderNames.UserAgent, ProductInfoHeaderParser.MultipleValueParser);

            HttpGeneralHeaders.AddParsers(parserStore);

            invalidHeaders = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            HttpContentHeaders.AddKnownHeaders(invalidHeaders);
            // Note: Reserved response header names are allowed as custom request header names.  Reserved response
            // headers have no defined meaning or format when used on a request.  This enables a server to accept
            // any headers sent from the client as either content headers or request headers.
        }

        internal static void AddKnownHeaders(HashSet<string> headerSet)
        {
            Contract.Requires(headerSet != null);

            headerSet.Add(HttpKnownHeaderNames.Accept);
            headerSet.Add(HttpKnownHeaderNames.AcceptCharset);
            headerSet.Add(HttpKnownHeaderNames.AcceptEncoding);
            headerSet.Add(HttpKnownHeaderNames.AcceptLanguage);
            headerSet.Add(HttpKnownHeaderNames.Authorization);
            headerSet.Add(HttpKnownHeaderNames.Expect);
            headerSet.Add(HttpKnownHeaderNames.From);
            headerSet.Add(HttpKnownHeaderNames.Host);
            headerSet.Add(HttpKnownHeaderNames.IfMatch);
            headerSet.Add(HttpKnownHeaderNames.IfModifiedSince);
            headerSet.Add(HttpKnownHeaderNames.IfNoneMatch);
            headerSet.Add(HttpKnownHeaderNames.IfRange);
            headerSet.Add(HttpKnownHeaderNames.IfUnmodifiedSince);
            headerSet.Add(HttpKnownHeaderNames.MaxForwards);
            headerSet.Add(HttpKnownHeaderNames.ProxyAuthorization);
            headerSet.Add(HttpKnownHeaderNames.Range);
            headerSet.Add(HttpKnownHeaderNames.Referer);
            headerSet.Add(HttpKnownHeaderNames.TE);
            headerSet.Add(HttpKnownHeaderNames.UserAgent);
        }

        internal override void AddHeaders(HttpHeaders sourceHeaders)
        {
            base.AddHeaders(sourceHeaders);
            HttpRequestHeaders sourceRequestHeaders = sourceHeaders as HttpRequestHeaders;
            Contract.Assert(sourceRequestHeaders != null);

            // Copy special values, but do not overwrite
            generalHeaders.AddSpecialsFrom(sourceRequestHeaders.generalHeaders);

            bool? expectContinue = ExpectContinue;
            if (!expectContinue.HasValue)
            {
                ExpectContinue = sourceRequestHeaders.ExpectContinue;
            }
        }
    }
}
