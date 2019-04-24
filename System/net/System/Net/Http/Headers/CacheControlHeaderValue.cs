using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.Globalization;
using System.Text;

namespace System.Net.Http.Headers
{
    public class CacheControlHeaderValue : ICloneable
    {
        private const string maxAgeString = "max-age";
        private const string maxStaleString = "max-stale";
        private const string minFreshString = "min-fresh";
        private const string mustRevalidateString = "must-revalidate";
        private const string noCacheString = "no-cache";
        private const string noStoreString = "no-store";
        private const string noTransformString = "no-transform";
        private const string onlyIfCachedString = "only-if-cached";
        private const string privateString = "private";
        private const string proxyRevalidateString = "proxy-revalidate";
        private const string publicString = "public";
        private const string sharedMaxAgeString = "s-maxage";

        private static readonly HttpHeaderParser nameValueListParser = GenericHeaderParser.MultipleValueNameValueParser;
        private static readonly Action<string> checkIsValidToken = CheckIsValidToken;

        private bool noCache;
        private ICollection<string> noCacheHeaders;
        private bool noStore;
        private TimeSpan? maxAge;
        private TimeSpan? sharedMaxAge;
        private bool maxStale;
        private TimeSpan? maxStaleLimit;
        private TimeSpan? minFresh;
        private bool noTransform;
        private bool onlyIfCached;
        private bool publicField;
        private bool privateField;
        private ICollection<string> privateHeaders;
        private bool mustRevalidate;
        private bool proxyRevalidate;
        private ICollection<NameValueHeaderValue> extensions;

        public bool NoCache
        {
            get { return noCache; }
            set { noCache = value; }
        }

        public ICollection<string> NoCacheHeaders
        {
            get
            {
                if (noCacheHeaders == null)
                {
                    noCacheHeaders = new ObjectCollection<string>(checkIsValidToken);
                }
                return noCacheHeaders;
            }
        }

        public bool NoStore
        {
            get { return noStore; }
            set { noStore = value; }
        }

        public TimeSpan? MaxAge
        {
            get { return maxAge; }
            set { maxAge = value; }
        }

        public TimeSpan? SharedMaxAge
        {
            get { return sharedMaxAge; }
            set { sharedMaxAge = value; }
        }

        public bool MaxStale
        {
            get { return maxStale; }
            set { maxStale = value; }
        }

        public TimeSpan? MaxStaleLimit
        {
            get { return maxStaleLimit; }
            set { maxStaleLimit = value; }
        }

        public TimeSpan? MinFresh
        {
            get { return minFresh; }
            set { minFresh = value; }
        }

        public bool NoTransform
        {
            get { return noTransform; }
            set { noTransform = value; }
        }

        public bool OnlyIfCached
        {
            get { return onlyIfCached; }
            set { onlyIfCached = value; }
        }

        public bool Public
        {
            get { return publicField; }
            set { publicField = value; }
        }

        public bool Private
        {
            get { return privateField; }
            set { privateField = value; }
        }

        public ICollection<string> PrivateHeaders
        {
            get
            {
                if (privateHeaders == null)
                {
                    privateHeaders = new ObjectCollection<string>(checkIsValidToken);
                }
                return privateHeaders;
            }
        }

        public bool MustRevalidate
        {
            get { return mustRevalidate; }
            set { mustRevalidate = value; }
        }

        public bool ProxyRevalidate
        {
            get { return proxyRevalidate; }
            set { proxyRevalidate = value; }
        }

        public ICollection<NameValueHeaderValue> Extensions
        {
            get
            {
                if (extensions == null)
                {
                    extensions = new ObjectCollection<NameValueHeaderValue>();
                }
                return extensions;
            }
        }

        public CacheControlHeaderValue()
        {
        }

        private CacheControlHeaderValue(CacheControlHeaderValue source)
        {
            Contract.Requires(source != null);

            noCache = source.noCache;
            noStore = source.noStore;
            maxAge = source.maxAge;
            sharedMaxAge = source.sharedMaxAge;
            maxStale = source.maxStale;
            maxStaleLimit = source.maxStaleLimit;
            minFresh = source.minFresh;
            noTransform = source.noTransform;
            onlyIfCached = source.onlyIfCached;
            publicField = source.publicField;
            privateField = source.privateField;
            mustRevalidate = source.mustRevalidate;
            proxyRevalidate = source.proxyRevalidate;

            if (source.noCacheHeaders != null)
            {
                foreach (var noCacheHeader in source.noCacheHeaders)
                {
                    NoCacheHeaders.Add(noCacheHeader);
                }
            }

            if (source.privateHeaders != null)
            {
                foreach (var privateHeader in source.privateHeaders)
                {
                    PrivateHeaders.Add(privateHeader);
                }
            }

            if (source.extensions != null)
            {
                foreach (var extension in source.extensions)
                {
                    Extensions.Add((NameValueHeaderValue)((ICloneable)extension).Clone());
                }
            }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            AppendValueIfRequired(sb, noStore, noStoreString);
            AppendValueIfRequired(sb, noTransform, noTransformString);
            AppendValueIfRequired(sb, onlyIfCached, onlyIfCachedString);
            AppendValueIfRequired(sb, publicField, publicString);
            AppendValueIfRequired(sb, mustRevalidate, mustRevalidateString);
            AppendValueIfRequired(sb, proxyRevalidate, proxyRevalidateString);

            if (noCache)
            {
                AppendValueWithSeparatorIfRequired(sb, noCacheString);
                if ((noCacheHeaders != null) && (noCacheHeaders.Count > 0))
                {
                    sb.Append("=\"");
                    AppendValues(sb, noCacheHeaders);
                    sb.Append('\"');
                }
            }

            if (maxAge.HasValue)
            {
                AppendValueWithSeparatorIfRequired(sb, maxAgeString);
                sb.Append('=');
                sb.Append(((int)maxAge.Value.TotalSeconds).ToString(NumberFormatInfo.InvariantInfo));
            }

            if (sharedMaxAge.HasValue)
            {
                AppendValueWithSeparatorIfRequired(sb, sharedMaxAgeString);
                sb.Append('=');
                sb.Append(((int)sharedMaxAge.Value.TotalSeconds).ToString(NumberFormatInfo.InvariantInfo));
            }

            if (maxStale)
            {
                AppendValueWithSeparatorIfRequired(sb, maxStaleString);
                if (maxStaleLimit.HasValue)
                {
                    sb.Append('=');
                    sb.Append(((int)maxStaleLimit.Value.TotalSeconds).ToString(NumberFormatInfo.InvariantInfo));
                }
            }

            if (minFresh.HasValue)
            {
                AppendValueWithSeparatorIfRequired(sb, minFreshString);
                sb.Append('=');
                sb.Append(((int)minFresh.Value.TotalSeconds).ToString(NumberFormatInfo.InvariantInfo));
            }

            if (privateField)
            {
                AppendValueWithSeparatorIfRequired(sb, privateString);
                if ((privateHeaders != null) && (privateHeaders.Count > 0))
                {
                    sb.Append("=\"");
                    AppendValues(sb, privateHeaders);
                    sb.Append('\"');
                }
            }

            NameValueHeaderValue.ToString(extensions, ',', false, sb);

            return sb.ToString();
        }

        public override bool Equals(object obj)
        {
            CacheControlHeaderValue other = obj as CacheControlHeaderValue;

            if (other == null)
            {
                return false;
            }

            if ((noCache != other.noCache) || (noStore != other.noStore) || (maxAge != other.maxAge) ||
                (sharedMaxAge != other.sharedMaxAge) || (maxStale != other.maxStale) ||
                (maxStaleLimit != other.maxStaleLimit) || (minFresh != other.minFresh) ||
                (noTransform != other.noTransform) || (onlyIfCached != other.onlyIfCached) ||
                (publicField != other.publicField) || (privateField != other.privateField) ||
                (mustRevalidate != other.mustRevalidate) || (proxyRevalidate != other.proxyRevalidate))
            {
                return false;
            }

            if (!HeaderUtilities.AreEqualCollections(noCacheHeaders, other.noCacheHeaders,
                StringComparer.OrdinalIgnoreCase))
            {
                return false;
            }

            if (!HeaderUtilities.AreEqualCollections(privateHeaders, other.privateHeaders,
                StringComparer.OrdinalIgnoreCase))
            {
                return false;
            }

            if (!HeaderUtilities.AreEqualCollections(extensions, other.extensions))
            {
                return false;
            }

            return true;
        }

        public override int GetHashCode()
        {
            // Use a different bit for bool fields: bool.GetHashCode() will return 0 (false) or 1 (true). So we would
            // end up having the same hash code for e.g. two instances where one has only noCache set and the other
            // only noStore.
            int result = noCache.GetHashCode() ^ (noStore.GetHashCode() << 1) ^ (maxStale.GetHashCode() << 2) ^
                (noTransform.GetHashCode() << 3) ^ (onlyIfCached.GetHashCode() << 4) ^
                (publicField.GetHashCode() << 5) ^ (privateField.GetHashCode() << 6) ^
                (mustRevalidate.GetHashCode() << 7) ^ (proxyRevalidate.GetHashCode() << 8);

            // XOR the hashcode of timespan values with different numbers to make sure two instances with the same
            // timespan set on different fields result in different hashcodes.
            result = result ^ (maxAge.HasValue ? maxAge.Value.GetHashCode() ^ 1 : 0) ^
                (sharedMaxAge.HasValue ? sharedMaxAge.Value.GetHashCode() ^ 2 : 0) ^
                (maxStaleLimit.HasValue ? maxStaleLimit.Value.GetHashCode() ^ 4 : 0) ^
                (minFresh.HasValue ? minFresh.Value.GetHashCode() ^ 8 : 0);

            if ((noCacheHeaders != null) && (noCacheHeaders.Count > 0))
            {
                foreach (var noCacheHeader in noCacheHeaders)
                {
                    result = result ^ noCacheHeader.ToLowerInvariant().GetHashCode();
                }
            }

            if ((privateHeaders != null) && (privateHeaders.Count > 0))
            {
                foreach (var privateHeader in privateHeaders)
                {
                    result = result ^ privateHeader.ToLowerInvariant().GetHashCode();
                }
            }

            if ((extensions != null) && (extensions.Count > 0))
            {
                foreach (var extension in extensions)
                {
                    result = result ^ extension.GetHashCode();
                }
            }

            return result;
        }

        public static CacheControlHeaderValue Parse(string input)
        {
            int index = 0;
            return (CacheControlHeaderValue)CacheControlHeaderParser.Parser.ParseValue(input, null, ref index);
        }

        public static bool TryParse(string input, out CacheControlHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (CacheControlHeaderParser.Parser.TryParseValue(input, null, ref index, out output))
            {
                parsedValue = (CacheControlHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetCacheControlLength(string input, int startIndex, CacheControlHeaderValue storeValue,
            out CacheControlHeaderValue parsedValue)
        {
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            // Cache-Control header consists of a list of name/value pairs, where the value is optional. So use an
            // instance of NameValueHeaderParser to parse the string.
            int current = startIndex;
            object nameValue = null;
            List<NameValueHeaderValue> nameValueList = new List<NameValueHeaderValue>();
            while (current < input.Length)
            {
                if (!nameValueListParser.TryParseValue(input, null, ref current, out nameValue))
                {
                    return 0;
                }

                nameValueList.Add(nameValue as NameValueHeaderValue);
            }

            // If we get here, we were able to successfully parse the string as list of name/value pairs. Now analyze
            // the name/value pairs.

            // Cache-Control is a header supporting lists of values. However, expose the header as an instance of
            // CacheControlHeaderValue. So if we already have an instance of CacheControlHeaderValue, add the values
            // from this string to the existing instances. 
            CacheControlHeaderValue result = storeValue;
            if (result == null)
            {
                result = new CacheControlHeaderValue();
            }

            if (!TrySetCacheControlValues(result, nameValueList))
            {
                return 0;
            }

            // If we had an existing store value and we just updated that instance, return 'null' to indicate that 
            // we don't have a new instance of CacheControlHeaderValue, but just updated an existing one. This is the
            // case if we have multiple 'Cache-Control' headers set in a request/response message.
            if (storeValue == null)
            {
                parsedValue = result;
            }

            // If we get here we successfully parsed the whole string.
            return input.Length - startIndex;
        }

        private static bool TrySetCacheControlValues(CacheControlHeaderValue cc,
            List<NameValueHeaderValue> nameValueList)
        {
            foreach (NameValueHeaderValue nameValue in nameValueList)
            {
                bool success = true;
                string name = nameValue.Name.ToLowerInvariant();

                switch (name)
                {
                    case noCacheString:
                        success = TrySetOptionalTokenList(nameValue, ref cc.noCache, ref cc.noCacheHeaders);
                        break;

                    case noStoreString:
                        success = TrySetTokenOnlyValue(nameValue, ref cc.noStore);
                        break;

                    case maxAgeString:
                        success = TrySetTimeSpan(nameValue, ref cc.maxAge);
                        break;

                    case maxStaleString:
                        success = ((nameValue.Value == null) || TrySetTimeSpan(nameValue, ref cc.maxStaleLimit));
                        if (success)
                        {
                            cc.maxStale = true;
                        }
                        break;

                    case minFreshString:
                        success = TrySetTimeSpan(nameValue, ref cc.minFresh);
                        break;

                    case noTransformString:
                        success = TrySetTokenOnlyValue(nameValue, ref cc.noTransform);
                        break;

                    case onlyIfCachedString:
                        success = TrySetTokenOnlyValue(nameValue, ref cc.onlyIfCached);
                        break;

                    case publicString:
                        success = TrySetTokenOnlyValue(nameValue, ref cc.publicField);
                        break;

                    case privateString:
                        success = TrySetOptionalTokenList(nameValue, ref cc.privateField, ref cc.privateHeaders);
                        break;

                    case mustRevalidateString:
                        success = TrySetTokenOnlyValue(nameValue, ref cc.mustRevalidate);
                        break;

                    case proxyRevalidateString:
                        success = TrySetTokenOnlyValue(nameValue, ref cc.proxyRevalidate);
                        break;

                    case sharedMaxAgeString:
                        success = TrySetTimeSpan(nameValue, ref cc.sharedMaxAge);
                        break;

                    default:
                        cc.Extensions.Add(nameValue); // success is always true
                        break;
                }

                if (!success)
                {
                    return false;
                }
            }

            return true;
        }

        private static bool TrySetTokenOnlyValue(NameValueHeaderValue nameValue, ref bool boolField)
        {
            if (nameValue.Value != null)
            {
                return false;
            }

            boolField = true;
            return true;
        }

        private static bool TrySetOptionalTokenList(NameValueHeaderValue nameValue, ref bool boolField,
            ref ICollection<string> destination)
        {
            Contract.Requires(nameValue != null);

            if (nameValue.Value == null)
            {
                boolField = true;
                return true;
            }

            // We need the string to be at least 3 chars long: 2x quotes and at least 1 character. Also make sure we
            // have a quoted string. Note that NameValueHeaderValue will never have leading/trailing whitespaces.
            string valueString = nameValue.Value;
            if ((valueString.Length < 3) || (valueString[0] != '\"') || (valueString[valueString.Length - 1] != '\"'))
            {
                return false;
            }

            // We have a quoted string. Now verify that the string contains a list of valid tokens separated by ','.
            int current = 1; // skip the initial '"' character.
            int maxLength = valueString.Length - 1; // -1 because we don't want to parse the final '"'.
            bool separatorFound = false;
            int originalValueCount = destination == null ? 0 : destination.Count;
            while (current < maxLength)
            {
                current = HeaderUtilities.GetNextNonEmptyOrWhitespaceIndex(valueString, current, true,
                    out separatorFound);

                if (current == maxLength)
                {
                    break;
                }

                int tokenLength = HttpRuleParser.GetTokenLength(valueString, current);

                if (tokenLength == 0)
                {
                    // We already skipped whitespaces and separators. If we don't have a token it must be an invalid
                    // character.
                    return false; 
                }

                if (destination == null)
                {
                    destination = new ObjectCollection<string>(checkIsValidToken);
                }
                
                destination.Add(valueString.Substring(current, tokenLength));
                
                current = current + tokenLength;
            }

            // After parsing a valid token list, we expect to have at least one value
            if ((destination != null) && (destination.Count > originalValueCount))
            {
                boolField = true;
                return true;
            }
            
            return false;
        }

        private static bool TrySetTimeSpan(NameValueHeaderValue nameValue, ref TimeSpan? timeSpan)
        {
            Contract.Requires(nameValue != null);
            
            if (nameValue.Value == null)
            {
                return false;
            }

            int seconds;
            if (!HeaderUtilities.TryParseInt32(nameValue.Value, out seconds))
            {
                return false;
            }

            timeSpan = new TimeSpan(0, 0, seconds);

            return true;
        }

        private static void AppendValueIfRequired(StringBuilder sb, bool appendValue, string value)
        {
            if (appendValue)
            {
                AppendValueWithSeparatorIfRequired(sb, value);
            }
        }

        private static void AppendValueWithSeparatorIfRequired(StringBuilder sb, string value)
        {
            if (sb.Length > 0)
            {
                sb.Append(", ");
            }
            sb.Append(value);
        }

        private static void AppendValues(StringBuilder sb, IEnumerable<string> values)
        {
            bool first = true;
            foreach (string value in values)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    sb.Append(", ");
                }

                sb.Append(value);
            }
        }

        private static void CheckIsValidToken(string item)
        {
            HeaderUtilities.CheckValidToken(item, "item");
        }

        object ICloneable.Clone()
        {
            return new CacheControlHeaderValue(this);
        }
    }
}
