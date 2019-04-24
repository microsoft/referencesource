using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.Text;

namespace System.Net.Http.Headers
{
    public class RangeHeaderValue : ICloneable
    {
        private string unit;
        private ICollection<RangeItemHeaderValue> ranges;

        public string Unit 
        {
            get { return unit; }
            set 
            {
                HeaderUtilities.CheckValidToken(value, "value");
                unit = value; 
            }
        }

        public ICollection<RangeItemHeaderValue> Ranges 
        {
            get
            {
                if (ranges == null)
                {
                    ranges = new ObjectCollection<RangeItemHeaderValue>();
                }
                return ranges;
            }
        }

        public RangeHeaderValue()
        {
            this.unit = HeaderUtilities.BytesUnit;
        }

        public RangeHeaderValue(long? from, long? to)
        {
            // convenience ctor: "Range: bytes=from-to"
            this.unit = HeaderUtilities.BytesUnit;
            Ranges.Add(new RangeItemHeaderValue(from, to));
        }

        private RangeHeaderValue(RangeHeaderValue source)
        {
            Contract.Requires(source != null);
            
            this.unit = source.unit;
            if (source.ranges != null)
            {
                foreach (RangeItemHeaderValue range in source.ranges)
                {
                    this.Ranges.Add((RangeItemHeaderValue)((ICloneable)range).Clone());
                }
            }
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder(unit);
            sb.Append('=');
            
            bool first = true;
            foreach (RangeItemHeaderValue range in Ranges)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    sb.Append(", ");
                }

                sb.Append(range.From);
                sb.Append('-');
                sb.Append(range.To);
            }

            return sb.ToString();
        }
        
        public override bool Equals(object obj)
        {
            RangeHeaderValue other = obj as RangeHeaderValue;

            if (other == null)
            {
                return false;
            }

            return (string.Compare(unit, other.unit, StringComparison.OrdinalIgnoreCase) == 0) &&
                HeaderUtilities.AreEqualCollections(Ranges, other.Ranges);
        }

        public override int GetHashCode()
        {
            int result = unit.ToLowerInvariant().GetHashCode();

            foreach (RangeItemHeaderValue range in Ranges)
            {
                result = result ^ range.GetHashCode();
            }

            return result;
        }

        public static RangeHeaderValue Parse(string input)
        {
            int index = 0;
            return (RangeHeaderValue)GenericHeaderParser.RangeParser.ParseValue(input, null, ref index);
        }

        public static bool TryParse(string input, out RangeHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (GenericHeaderParser.RangeParser.TryParseValue(input, null, ref index, out output))
            {
                parsedValue = (RangeHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetRangeLength(string input, int startIndex, out object parsedValue)
        {
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            // Parse the unit string: <unit> in '<unit>=<from1>-<to1>, <from2>-<to2>'
            int unitLength = HttpRuleParser.GetTokenLength(input, startIndex);

            if (unitLength == 0)
            {
                return 0;
            }

            RangeHeaderValue result = new RangeHeaderValue();
            result.unit = input.Substring(startIndex, unitLength);
            int current = startIndex + unitLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            if ((current == input.Length) || (input[current] != '='))
            {
                return 0;
            }

            current++; // skip '=' separator
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            int rangesLength = RangeItemHeaderValue.GetRangeItemListLength(input, current, result.Ranges);

            if (rangesLength == 0)
            {
                return 0;
            }

            current = current + rangesLength;
            Contract.Assert(current == input.Length, "GetRangeItemListLength() should consume the whole string or fail.");

            parsedValue = result;
            return current - startIndex;
        }

        object ICloneable.Clone()
        {
            return new RangeHeaderValue(this);
        }
    }
}
