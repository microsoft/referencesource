using System.Diagnostics.Contracts;
using System.Globalization;
using System.Text;

namespace System.Net.Http.Headers
{
    public class ContentRangeHeaderValue : ICloneable
    {
        private string unit;
        private long? from;
        private long? to;
        private long? length;

        public string Unit 
        {
            get { return unit; }
            set 
            {
                HeaderUtilities.CheckValidToken(value, "value");
                unit = value;
            }
        }

        public long? From 
        {
            get { return from; }
        }

        public long? To 
        {
            get { return to; }
        }

        public long? Length
        {
            get { return length; } 
        }

        public bool HasLength // e.g. "Content-Range: bytes 12-34/*"
        {
            get { return length != null; }
        }

        public bool HasRange // e.g. "Content-Range: bytes */1234"
        {
            get { return from != null; }
        } 

        public ContentRangeHeaderValue(long from, long to, long length)
        {
            // Scenario: "Content-Range: bytes 12-34/5678"

            if (length < 0)
            {
                throw new ArgumentOutOfRangeException("length");
            }
            if ((to < 0) || (to > length))
            {
                throw new ArgumentOutOfRangeException("to");
            }
            if ((from < 0) || (from > to))
            {
                throw new ArgumentOutOfRangeException("from");
            }

            this.from = from;
            this.to = to;
            this.length = length;
            this.unit = HeaderUtilities.BytesUnit;
        }

        public ContentRangeHeaderValue(long length)
        {
            // Scenario: "Content-Range: bytes */1234"

            if (length < 0)
            {
                throw new ArgumentOutOfRangeException("length");
            }

            this.length = length;
            this.unit = HeaderUtilities.BytesUnit;
        }

        public ContentRangeHeaderValue(long from, long to)
        {
            // Scenario: "Content-Range: bytes 12-34/*"

            if (to < 0)
            {
                throw new ArgumentOutOfRangeException("to");
            }
            if ((from < 0) || (from > to))
            {
                throw new ArgumentOutOfRangeException("from");
            }

            this.from = from;
            this.to = to;
            this.unit = HeaderUtilities.BytesUnit;
        }

        private ContentRangeHeaderValue()
        {             
        }

        private ContentRangeHeaderValue(ContentRangeHeaderValue source)
        {
            Contract.Requires(source != null);

            this.from = source.from;
            this.to = source.to;
            this.length = source.length;
            this.unit = source.unit;
        }

        public override bool Equals(object obj)
        {
            ContentRangeHeaderValue other = obj as ContentRangeHeaderValue;

            if (other == null)
            {
                return false;
            }

            return ((from == other.from) && (to == other.to) && (length == other.length) &&
                (string.Compare(unit, other.unit, StringComparison.OrdinalIgnoreCase) == 0));
        }

        public override int GetHashCode()
        {
            int result = unit.ToLowerInvariant().GetHashCode();

            if (HasRange)
            {
                result = result ^ from.GetHashCode() ^ to.GetHashCode();
            }

            if (HasLength)
            {
                result = result ^ length.GetHashCode();
            }

            return result;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder(unit);
            sb.Append(' ');

            if (HasRange)
            {
                sb.Append(from.Value.ToString(NumberFormatInfo.InvariantInfo));
                sb.Append('-');
                sb.Append(to.Value.ToString(NumberFormatInfo.InvariantInfo));
            }
            else
            {
                sb.Append('*');
            }

            sb.Append('/');
            if (HasLength)
            {
                sb.Append(length.Value.ToString(NumberFormatInfo.InvariantInfo));
            }
            else
            {
                sb.Append('*');
            }

            return sb.ToString();
        }

        public static ContentRangeHeaderValue Parse(string input)
        {
            int index = 0;
            return (ContentRangeHeaderValue)GenericHeaderParser.ContentRangeParser.ParseValue(input, null, ref index);
        }

        public static bool TryParse(string input, out ContentRangeHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (GenericHeaderParser.ContentRangeParser.TryParseValue(input, null, ref index, out output))
            {
                parsedValue = (ContentRangeHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetContentRangeLength(string input, int startIndex, out object parsedValue)
        {
            Contract.Requires(startIndex >= 0);
            
            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            // Parse the unit string: <unit> in '<unit> <from>-<to>/<length>'
            int unitLength = HttpRuleParser.GetTokenLength(input, startIndex);

            if (unitLength == 0)
            {
                return 0;
            }

            string unit = input.Substring(startIndex, unitLength);
            int current = startIndex + unitLength;
            int separatorLength = HttpRuleParser.GetWhitespaceLength(input, current);

            if (separatorLength == 0)
            {
                return 0;
            }

            current = current + separatorLength;

            if (current == input.Length)
            {
                return 0;
            }

            // Read range values <from> and <to> in '<unit> <from>-<to>/<length>'
            int fromStartIndex = current;
            int fromLength = 0;
            int toStartIndex = 0;
            int toLength = 0;
            if (!TryGetRangeLength(input, ref current, out fromLength, out toStartIndex, out toLength))
            {
                return 0;
            }

            // After the range is read we expect the length separator '/'
            if ((current == input.Length) || (input[current] != '/'))
            {
                return 0;
            }

            current++; // Skip '/' separator
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);
            
            if (current == input.Length)
            {
                return 0;
            }

            // We may not have a length (e.g. 'bytes 1-2/*'). But if we do, parse the length now.
            int lengthStartIndex = current;
            int lengthLength = 0;
            if (!TryGetLengthLength(input, ref current, out lengthLength))
            {
                return 0;
            }
            
            if (!TryCreateContentRange(input, unit, fromStartIndex, fromLength, toStartIndex, toLength, 
                lengthStartIndex, lengthLength, out parsedValue))
            {
                return 0;
            }
            
            return current - startIndex;
        }

        private static bool TryGetLengthLength(string input, ref int current, out int lengthLength)
        {
            lengthLength = 0;

            if (input[current] == '*')
            {
                current++;
            }
            else
            {
                // Parse length value: <length> in '<unit> <from>-<to>/<length>'
                lengthLength = HttpRuleParser.GetNumberLength(input, current, false);

                if ((lengthLength == 0) || (lengthLength > HttpRuleParser.MaxInt64Digits))
                {
                    return false;
                }

                current = current + lengthLength;
            }

            current = current + HttpRuleParser.GetWhitespaceLength(input, current);
            return true;
        }

        private static bool TryGetRangeLength(string input, ref int current, out int fromLength, out int toStartIndex, 
            out int toLength)
        {
            fromLength = 0;
            toStartIndex = 0;
            toLength = 0;

            // Check if we have a value like 'bytes */133'. If yes, skip the range part and continue parsing the 
            // length separator '/'.
            if (input[current] == '*')
            {
                current++;
            }
            else
            {
                // Parse first range value: <from> in '<unit> <from>-<to>/<length>'
                fromLength = HttpRuleParser.GetNumberLength(input, current, false);

                if ((fromLength == 0) || (fromLength > HttpRuleParser.MaxInt64Digits))
                {
                    return false;
                }

                current = current + fromLength;
                current = current + HttpRuleParser.GetWhitespaceLength(input, current);

                // Afer the first value, the '-' character must follow.
                if ((current == input.Length) || (input[current] != '-'))
                {
                    // We need a '-' character otherwise this can't be a valid range.
                    return false;
                }

                current++; // skip the '-' character
                current = current + HttpRuleParser.GetWhitespaceLength(input, current);

                if (current == input.Length)
                {
                    return false;
                }

                // Parse second range value: <to> in '<unit> <from>-<to>/<length>'
                toStartIndex = current;
                toLength = HttpRuleParser.GetNumberLength(input, current, false);

                if ((toLength == 0) || (toLength > HttpRuleParser.MaxInt64Digits))
                {
                    return false;
                }

                current = current + toLength;
            }

            current = current + HttpRuleParser.GetWhitespaceLength(input, current);
            return true;
        }

        private static bool TryCreateContentRange(string input, string unit, int fromStartIndex, int fromLength, 
            int toStartIndex, int toLength, int lengthStartIndex, int lengthLength, out object parsedValue)
        {
            parsedValue = null;

            long from = 0;
            if ((fromLength > 0) && !HeaderUtilities.TryParseInt64(input.Substring(fromStartIndex, fromLength), out from))
            {
                return false;
            }

            long to = 0;
            if ((toLength > 0) && !HeaderUtilities.TryParseInt64(input.Substring(toStartIndex, toLength), out to))
            {
                return false;
            }

            // 'from' must not be greater than 'to'
            if ((fromLength > 0) && (toLength > 0) && (from > to))
            {
                return false;
            }

            long length = 0;
            if ((lengthLength > 0) && !HeaderUtilities.TryParseInt64(input.Substring(lengthStartIndex, lengthLength),
                out length))
            {
                return false;
            }

            // 'from' and 'to' must be less than 'length'
            if ((toLength > 0) && (lengthLength > 0) && (to >= length))
            {
                return false;
            }

            ContentRangeHeaderValue result = new ContentRangeHeaderValue();
            result.unit = unit;

            if (fromLength > 0)
            {
                result.from = from;
                result.to = to;
            }

            if (lengthLength > 0)
            {
                result.length = length;
            }

            parsedValue = result;
            return true;
        }

        object ICloneable.Clone()
        {
            return new ContentRangeHeaderValue(this);
        }
    }
}
