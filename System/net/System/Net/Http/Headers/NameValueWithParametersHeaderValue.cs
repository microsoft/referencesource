using System.Collections.Generic;
using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    // According to the RFC, in places where a "parameter" is required, the value is mandatory 
    // (e.g. Media-Type, Accept). However, we don't introduce a dedicated type for this.
    public class NameValueWithParametersHeaderValue : NameValueHeaderValue, ICloneable
    {
        private static readonly Func<NameValueHeaderValue> nameValueCreator = CreateNameValue;

        private ICollection<NameValueHeaderValue> parameters;

        public ICollection<NameValueHeaderValue> Parameters
        {
            get
            {
                if (parameters == null)
                {
                    parameters = new ObjectCollection<NameValueHeaderValue>();
                }
                return parameters;
            }
        }

        public NameValueWithParametersHeaderValue(string name)
            : base(name)
        {
        }

        public NameValueWithParametersHeaderValue(string name, string value)
            : base(name, value)
        {
        }

        internal NameValueWithParametersHeaderValue()
        {
        }

        protected NameValueWithParametersHeaderValue(NameValueWithParametersHeaderValue source)
            : base(source)
        {
            if (source.parameters != null)
            {
                foreach (var parameter in source.parameters)
                {
                    this.Parameters.Add((NameValueHeaderValue)((ICloneable)parameter).Clone());
                }
            }
        }

        public override bool Equals(object obj)
        {
            bool result = base.Equals(obj);

            if (result)
            {
                NameValueWithParametersHeaderValue other = obj as NameValueWithParametersHeaderValue;

                if (other == null)
                {
                    return false;
                }
                return HeaderUtilities.AreEqualCollections(parameters, other.parameters);
            }

            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode() ^ NameValueHeaderValue.GetHashCode(parameters);
        }

        public override string ToString()
        {
            return base.ToString() + NameValueHeaderValue.ToString(parameters, ';', true);
        }

        public static new NameValueWithParametersHeaderValue Parse(string input)
        {
            int index = 0;
            return (NameValueWithParametersHeaderValue)GenericHeaderParser.SingleValueNameValueWithParametersParser
                .ParseValue(input, null, ref index);
        }

        public static bool TryParse(string input, out NameValueWithParametersHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (GenericHeaderParser.SingleValueNameValueWithParametersParser.TryParseValue(input, 
                null, ref index, out output))
            {
                parsedValue = (NameValueWithParametersHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetNameValueWithParametersLength(string input, int startIndex, out object parsedValue)
        {
            Contract.Requires(input != null);
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            NameValueHeaderValue nameValue = null;
            int nameValueLength = NameValueHeaderValue.GetNameValueLength(input, startIndex, 
                nameValueCreator, out nameValue);

            if (nameValueLength == 0)
            {
                return 0;
            }

            int current = startIndex + nameValueLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);
            NameValueWithParametersHeaderValue nameValueWithParameters =
                nameValue as NameValueWithParametersHeaderValue;
            Contract.Assert(nameValueWithParameters != null);

            // So far we have a valid name/value pair. Check if we have also parameters for the name/value pair. If
            // yes, parse parameters. E.g. something like "name=value; param1=value1; param2=value2".
            if ((current < input.Length) && (input[current] == ';'))
            {
                current++; // skip delimiter.
                int parameterLength = NameValueHeaderValue.GetNameValueListLength(input, current, ';',
                    nameValueWithParameters.Parameters);

                if (parameterLength == 0)
                {
                    return 0;
                }

                parsedValue = nameValueWithParameters;
                return current + parameterLength - startIndex;
            }

            // We have a name/value pair without parameters.
            parsedValue = nameValueWithParameters;
            return current - startIndex;
        }

        private static NameValueHeaderValue CreateNameValue()
        {
            return new NameValueWithParametersHeaderValue();
        }

        object ICloneable.Clone()
        {
            return new NameValueWithParametersHeaderValue(this);
        }
    }
}
