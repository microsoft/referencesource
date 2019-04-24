using System.Diagnostics.Contracts;
using System.Globalization;

namespace System.Net.Http.Headers
{
    public class StringWithQualityHeaderValue : ICloneable
    {
        private string value;
        private double? quality;

        public string Value 
        {
            get { return value; }
        }

        public double? Quality 
        {
            get { return quality; }
        }

        public StringWithQualityHeaderValue(string value)
        {
            HeaderUtilities.CheckValidToken(value, "value");

            this.value = value;
        }

        public StringWithQualityHeaderValue(string value, double quality)
        {
            HeaderUtilities.CheckValidToken(value, "value");

            if ((quality < 0) || (quality > 1))
            {
                throw new ArgumentOutOfRangeException("quality");
            }
            
            this.value = value;
            this.quality = quality;
        }

        private StringWithQualityHeaderValue(StringWithQualityHeaderValue source)
        {
            Contract.Requires(source != null);

            this.value = source.value;
            this.quality = source.quality;
        }

        private StringWithQualityHeaderValue()
        {
        }

        public override string ToString()
        {
            if (quality.HasValue)
            {
                return value + "; q=" + quality.Value.ToString("0.0##", NumberFormatInfo.InvariantInfo);
            }

            return value;
        }

        public override bool Equals(object obj)
        {
            StringWithQualityHeaderValue other = obj as StringWithQualityHeaderValue;

            if (other == null)
            {
                return false;
            }

            if (string.Compare(value, other.value, StringComparison.OrdinalIgnoreCase) != 0)
            {
                return false;
            }
            
            if (quality.HasValue)
            {
                // Note that we don't consider double.Epsilon here. We really consider two values equal if they're
                // actually equal. This makes sure that we also get the same hashcode for two values considered equal
                // by Equals(). 
                return other.quality.HasValue && (quality.Value == other.quality.Value);
            }
            
            // If we don't have a quality value, then 'other' must also have no quality assigned in order to be 
            // considered equal.
            return !other.quality.HasValue; 
        }

        public override int GetHashCode()
        {
            int result = value.ToLowerInvariant().GetHashCode();

            if (quality.HasValue)
            {
                result = result ^ quality.Value.GetHashCode();
            }

            return result;
        }

        public static StringWithQualityHeaderValue Parse(string input)
        {
            int index = 0;
            return (StringWithQualityHeaderValue)GenericHeaderParser.SingleValueStringWithQualityParser.ParseValue(
                input, null, ref index);
        }

        public static bool TryParse(string input, out StringWithQualityHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (GenericHeaderParser.SingleValueStringWithQualityParser.TryParseValue(
                input, null, ref index, out output))
            {
                parsedValue = (StringWithQualityHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetStringWithQualityLength(string input, int startIndex, out object parsedValue)
        {
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            // Parse the value string: <value> in '<value>; q=<quality>'
            int valueLength = HttpRuleParser.GetTokenLength(input, startIndex);

            if (valueLength == 0)
            {
                return 0;
            }

            StringWithQualityHeaderValue result = new StringWithQualityHeaderValue();
            result.value = input.Substring(startIndex, valueLength);
            int current = startIndex + valueLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            if ((current == input.Length) || (input[current] != ';'))
            {
                parsedValue = result;
                return current - startIndex; // we have a valid token, but no quality.
            }

            current++; // skip ';' separator
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            // If we found a ';' separator, it must be followed by a quality information
            if (!TryReadQuality(input, result, ref current))
            {
                return 0;
            }

            parsedValue = result;
            return current - startIndex;
        }

        private static bool TryReadQuality(string input, StringWithQualityHeaderValue result, ref int index)
        {
            int current = index;

            // See if we have a quality value by looking for "q"
            if ((current == input.Length) || ((input[current] != 'q') && (input[current] != 'Q')))
            {
                return false;
            }

            current++; // skip 'q' identifier
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            // If we found "q" it must be followed by "="
            if ((current == input.Length) || (input[current] != '='))
            {
                return false;
            }

            current++; // skip '=' separator
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            if (current == input.Length)
            {
                return false;
            }

            int qualityLength = HttpRuleParser.GetNumberLength(input, current, true);

            if (qualityLength == 0)
            {
                return false;
            }

            double quality = 0;
            if (!double.TryParse(input.Substring(current, qualityLength), NumberStyles.AllowDecimalPoint,
                NumberFormatInfo.InvariantInfo, out quality))
            {
                return false; 
            }

            if ((quality < 0) || (quality > 1))
            {
                return false;
            }

            result.quality = quality;

            current = current + qualityLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            index = current;
            return true;
        }

        object ICloneable.Clone()
        {
            return new StringWithQualityHeaderValue(this);
        }
    }
}
