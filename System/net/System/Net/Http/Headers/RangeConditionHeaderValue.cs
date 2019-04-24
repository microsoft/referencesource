using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    public class RangeConditionHeaderValue : ICloneable
    {
        private DateTimeOffset? date;
        private EntityTagHeaderValue entityTag;

        public DateTimeOffset? Date
        {
            get { return date; }
        }

        public EntityTagHeaderValue EntityTag
        {
            get { return entityTag; }
        }

        public RangeConditionHeaderValue(DateTimeOffset date)
        {
            this.date = date;
        }

        public RangeConditionHeaderValue(EntityTagHeaderValue entityTag)
        {
            if (entityTag == null)
            {
                throw new ArgumentNullException("entityTag");
            }            

            this.entityTag = entityTag;
        }

        public RangeConditionHeaderValue(string entityTag)
            : this(new EntityTagHeaderValue(entityTag))
        {
        }

        private RangeConditionHeaderValue(RangeConditionHeaderValue source)
        {
            Contract.Requires(source != null);

            this.entityTag = source.entityTag;
            this.date = source.date;
        }

        private RangeConditionHeaderValue()
        {
        }

        public override string ToString()
        {
            if (entityTag == null)
            {
                return HttpRuleParser.DateToString(date.Value);
            }
            return entityTag.ToString();
        }

        public override bool Equals(object obj)
        {
            RangeConditionHeaderValue other = obj as RangeConditionHeaderValue;

            if (other == null)
            {
                return false;
            }

            if (entityTag == null)
            {
                return (other.date != null) && (date.Value == other.date.Value);
            }

            return entityTag.Equals(other.entityTag);
        }

        public override int GetHashCode()
        {
            if (entityTag == null)
            {
                return date.Value.GetHashCode();
            }

            return entityTag.GetHashCode();
        }

        public static RangeConditionHeaderValue Parse(string input)
        {
            int index = 0;
            return (RangeConditionHeaderValue)GenericHeaderParser.RangeConditionParser.ParseValue(
                input, null, ref index);
        }

        public static bool TryParse(string input, out RangeConditionHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (GenericHeaderParser.RangeConditionParser.TryParseValue(input, null, ref index, out output))
            {
                parsedValue = (RangeConditionHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetRangeConditionLength(string input, int startIndex, out object parsedValue)
        {
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            // Make sure we have at least 2 characters
            if (string.IsNullOrEmpty(input) || (startIndex + 1 >= input.Length))
            {
                return 0;
            }

            int current = startIndex;

            // Caller must remove leading whitespaces.
            DateTimeOffset date = DateTimeOffset.MinValue;
            EntityTagHeaderValue entityTag = null;

            // Entity tags are quoted strings optionally preceded by "W/". By looking at the first two character we
            // can determine whether the string is en entity tag or a date.
            char firstChar = input[current];
            char secondChar = input[current + 1];

            if ((firstChar == '\"') || (((firstChar == 'w') || (firstChar == 'W')) && (secondChar == '/')))
            {
                // trailing whitespaces are removed by GetEntityTagLength()
                int entityTagLength = EntityTagHeaderValue.GetEntityTagLength(input, current, out entityTag);

                if (entityTagLength == 0)
                {
                    return 0;
                }

                current = current + entityTagLength;

                // RangeConditionHeaderValue only allows 1 value. There must be no delimiter/other chars after an 
                // entity tag.
                if (current != input.Length)
                {
                    return 0; 
                }
            }
            else
            {
                if (!HttpRuleParser.TryStringToDate(input.Substring(current), out date))
                {
                    return 0;
                }

                // If we got a valid date, then the parser consumed the whole string (incl. trailing whitespaces).
                current = input.Length;
            }

            RangeConditionHeaderValue result = new RangeConditionHeaderValue();
            if (entityTag == null)
            {
                result.date = date;
            }
            else
            {
                result.entityTag = entityTag;
            }

            parsedValue = result;
            return current - startIndex;
        }

        object ICloneable.Clone()
        {
            return new RangeConditionHeaderValue(this);
        }
    }
}