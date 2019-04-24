using System.Collections.Generic;
using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    public class TransferCodingHeaderValue : ICloneable
    {
        // Use list instead of dictionary since we may have multiple parameters with the same name.
        private ICollection<NameValueHeaderValue> parameters;
        private string value;

        public string Value 
        {
            get { return value; }
        }

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

        internal TransferCodingHeaderValue()
        {
        }

        protected TransferCodingHeaderValue(TransferCodingHeaderValue source)
        {
            Contract.Requires(source != null);

            this.value = source.value;

            if (source.parameters != null)
            {
                foreach (var parameter in source.parameters)
                {
                    this.Parameters.Add((NameValueHeaderValue)((ICloneable)parameter).Clone());
                }
            }
        }

        public TransferCodingHeaderValue(string value)
        {
            HeaderUtilities.CheckValidToken(value, "value");
            this.value = value;
        }

        public static TransferCodingHeaderValue Parse(string input)
        {
            int index = 0;
            return (TransferCodingHeaderValue)TransferCodingHeaderParser.SingleValueParser.ParseValue(
                input, null, ref index);
        }

        public static bool TryParse(string input, out TransferCodingHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (TransferCodingHeaderParser.SingleValueParser.TryParseValue(input, null, ref index, out output))
            {
                parsedValue = (TransferCodingHeaderValue)output;
                return true;
            }
            return false;
        }
        
        internal static int GetTransferCodingLength(string input, int startIndex, 
            Func<TransferCodingHeaderValue> transferCodingCreator, out TransferCodingHeaderValue parsedValue)
        {
            Contract.Requires(transferCodingCreator != null);
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            // Caller must remove leading whitespaces. If not, we'll return 0.
            int valueLength = HttpRuleParser.GetTokenLength(input, startIndex);
            
            if (valueLength == 0)
            {
                return 0;
            }

            string value = input.Substring(startIndex, valueLength);
            int current = startIndex + valueLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);
            TransferCodingHeaderValue transferCodingHeader = null;

            // If we're not done and we have a parameter delimiter, then we have a list of parameters.
            if ((current < input.Length) && (input[current] == ';'))
            {
                transferCodingHeader = transferCodingCreator();
                transferCodingHeader.value = value;

                current++; // skip delimiter.
                int parameterLength = NameValueHeaderValue.GetNameValueListLength(input, current, ';',
                    transferCodingHeader.Parameters);

                if (parameterLength == 0)
                {
                    return 0;
                }

                parsedValue = transferCodingHeader;
                return current + parameterLength - startIndex;
            }

            // We have a transfer coding without parameters.
            transferCodingHeader = transferCodingCreator();
            transferCodingHeader.value = value;
            parsedValue = transferCodingHeader;
            return current - startIndex;
        }

        public override string ToString()
        {
            return value + NameValueHeaderValue.ToString(parameters, ';', true);
        }

        public override bool Equals(object obj)
        {
            TransferCodingHeaderValue other = obj as TransferCodingHeaderValue;

            if (other == null)
            {
                return false;
            }

            return (string.Compare(value, other.value, StringComparison.OrdinalIgnoreCase) == 0) &&
                HeaderUtilities.AreEqualCollections(parameters, other.parameters);
        }

        public override int GetHashCode()
        {
            // The value string is case-insensitive.
            return value.ToLowerInvariant().GetHashCode() ^ NameValueHeaderValue.GetHashCode(parameters);
        }

        // Implement ICloneable explicitly to allow derived types to "override" the implementation.
        object ICloneable.Clone()
        {
            return new TransferCodingHeaderValue(this);
        }
    }
}
