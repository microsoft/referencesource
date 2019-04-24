using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    [SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix",
        Justification = "This is not a collection")]
    public sealed class HttpContentHeaders : HttpHeaders
    {
        private static readonly Dictionary<string, HttpHeaderParser> parserStore;
        private static readonly HashSet<string> invalidHeaders;

        private Func<long?> calculateLengthFunc;
        private bool contentLengthSet;

        private HttpHeaderValueCollection<string> allow;
        private HttpHeaderValueCollection<string> contentEncoding;
        private HttpHeaderValueCollection<string> contentLanguage;

        public ICollection<string> Allow
        {
            get
            {
                if (allow == null)
                {
                    allow = new HttpHeaderValueCollection<string>(HttpKnownHeaderNames.Allow,
                        this, HeaderUtilities.TokenValidator);
                }
                return allow;
            }
        }

        public ContentDispositionHeaderValue ContentDisposition
        {
            get { return (ContentDispositionHeaderValue)GetParsedValues(HttpKnownHeaderNames.ContentDisposition); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.ContentDisposition, value); }
        }

        // Must be a collection (and not provide properties like "GZip", "Deflate", etc.) since the 
        // order matters!
        public ICollection<string> ContentEncoding
        {
            get
            {
                if (contentEncoding == null)
                {
                    contentEncoding = new HttpHeaderValueCollection<string>(HttpKnownHeaderNames.ContentEncoding,
                        this, HeaderUtilities.TokenValidator);
                }
                return contentEncoding;
            }
        }

        public ICollection<string> ContentLanguage
        {
            get
            {
                if (contentLanguage == null)
                {
                    contentLanguage = new HttpHeaderValueCollection<string>(HttpKnownHeaderNames.ContentLanguage,
                        this, HeaderUtilities.TokenValidator);
                }
                return contentLanguage;
            }
        }

        public long? ContentLength
        {
            get
            {
                // 'Content-Length' can only hold one value. So either we get 'null' back or a boxed long value.
                object storedValue = GetParsedValues(HttpKnownHeaderNames.ContentLength);

                // Only try to calculate the length if the user didn't set the value explicitly using the setter.
                if (!contentLengthSet && (storedValue == null))
                {
                    // If we don't have a value for Content-Length in the store, try to let the content calculate
                    // it's length. If the content object is able to calculate the length, we'll store it in the
                    // store.
                    long? calculatedLength = calculateLengthFunc();

                    if (calculatedLength != null)
                    {
                        SetParsedValue(HttpKnownHeaderNames.ContentLength, (object)calculatedLength.Value);
                    }

                    return calculatedLength;
                }

                if (storedValue == null)
                {
                    return null;
                }
                else
                {
                    return (long)storedValue;
                }
            }
            set
            {
                SetOrRemoveParsedValue(HttpKnownHeaderNames.ContentLength, value); // box long value
                contentLengthSet = true;
            }
        }

        public Uri ContentLocation
        {
            get { return (Uri)GetParsedValues(HttpKnownHeaderNames.ContentLocation); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.ContentLocation, value); }
        }

        [SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays",
            Justification = "In this case the 'value' is the byte array. I.e. the array is treated as a value.")]
        public byte[] ContentMD5
        {
            get { return (byte[])GetParsedValues(HttpKnownHeaderNames.ContentMD5); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.ContentMD5, value); }
        }

        public ContentRangeHeaderValue ContentRange
        {
            get { return (ContentRangeHeaderValue)GetParsedValues(HttpKnownHeaderNames.ContentRange); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.ContentRange, value); }
        }

        public MediaTypeHeaderValue ContentType
        {
            get { return (MediaTypeHeaderValue)GetParsedValues(HttpKnownHeaderNames.ContentType); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.ContentType, value); }
        }

        public DateTimeOffset? Expires
        {
            get { return HeaderUtilities.GetDateTimeOffsetValue(HttpKnownHeaderNames.Expires, this); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.Expires, value); }
        }

        public DateTimeOffset? LastModified
        {
            get { return HeaderUtilities.GetDateTimeOffsetValue(HttpKnownHeaderNames.LastModified, this); }
            set { SetOrRemoveParsedValue(HttpKnownHeaderNames.LastModified, value); }
        }

        internal HttpContentHeaders(Func<long?> calculateLengthFunc)
        {
            this.calculateLengthFunc = calculateLengthFunc;

            SetConfiguration(parserStore, invalidHeaders);
        }

        static HttpContentHeaders()
        {
            parserStore = new Dictionary<string, HttpHeaderParser>(StringComparer.OrdinalIgnoreCase);

            parserStore.Add(HttpKnownHeaderNames.Allow, GenericHeaderParser.TokenListParser);
            parserStore.Add(HttpKnownHeaderNames.ContentDisposition, GenericHeaderParser.ContentDispositionParser);
            parserStore.Add(HttpKnownHeaderNames.ContentEncoding, GenericHeaderParser.TokenListParser);
            parserStore.Add(HttpKnownHeaderNames.ContentLanguage, GenericHeaderParser.TokenListParser);
            parserStore.Add(HttpKnownHeaderNames.ContentLength, Int64NumberHeaderParser.Parser);
            parserStore.Add(HttpKnownHeaderNames.ContentLocation, UriHeaderParser.RelativeOrAbsoluteUriParser);
            parserStore.Add(HttpKnownHeaderNames.ContentMD5, ByteArrayHeaderParser.Parser);
            parserStore.Add(HttpKnownHeaderNames.ContentRange, GenericHeaderParser.ContentRangeParser);
            parserStore.Add(HttpKnownHeaderNames.ContentType, MediaTypeHeaderParser.SingleValueParser);
            parserStore.Add(HttpKnownHeaderNames.Expires, DateHeaderParser.Parser);
            parserStore.Add(HttpKnownHeaderNames.LastModified, DateHeaderParser.Parser);

            invalidHeaders = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
             HttpRequestHeaders.AddKnownHeaders(invalidHeaders);
             HttpResponseHeaders.AddKnownHeaders(invalidHeaders);
            HttpGeneralHeaders.AddKnownHeaders(invalidHeaders);
        }

        internal static void AddKnownHeaders(HashSet<string> headerSet)
        {
            Contract.Requires(headerSet != null);

            headerSet.Add(HttpKnownHeaderNames.Allow);
            headerSet.Add(HttpKnownHeaderNames.ContentDisposition);
            headerSet.Add(HttpKnownHeaderNames.ContentEncoding);
            headerSet.Add(HttpKnownHeaderNames.ContentLanguage);
            headerSet.Add(HttpKnownHeaderNames.ContentLength);
            headerSet.Add(HttpKnownHeaderNames.ContentLocation);
            headerSet.Add(HttpKnownHeaderNames.ContentMD5);
            headerSet.Add(HttpKnownHeaderNames.ContentRange);
            headerSet.Add(HttpKnownHeaderNames.ContentType);
            headerSet.Add(HttpKnownHeaderNames.Expires);
            headerSet.Add(HttpKnownHeaderNames.LastModified);
        }
    }
}
