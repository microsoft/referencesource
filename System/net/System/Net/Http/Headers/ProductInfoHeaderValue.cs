using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    public class ProductInfoHeaderValue : ICloneable
    {
        private ProductHeaderValue product;
        private string comment;

        public ProductHeaderValue Product 
        {
            get { return product; }
        }

        public string Comment 
        {
            get { return comment; }
        }

        public ProductInfoHeaderValue(string productName, string productVersion)
            : this(new ProductHeaderValue(productName, productVersion))
        {            
        }

        public ProductInfoHeaderValue(ProductHeaderValue product)
        {
            if (product == null)
            {
                throw new ArgumentNullException("product");
            }

            this.product = product;
        }

        public ProductInfoHeaderValue(string comment)
        {
            HeaderUtilities.CheckValidComment(comment, "comment");   
            this.comment = comment;
        }

        private ProductInfoHeaderValue(ProductInfoHeaderValue source)
        {
            Contract.Requires(source != null);

            this.product = source.product;
            this.comment = source.comment;
        }

        private ProductInfoHeaderValue()
        {
        }

        public override string ToString()
        {
            if (product == null)
            {
                return comment;
            }
            return product.ToString();
        }

        public override bool Equals(object obj)
        {
            ProductInfoHeaderValue other = obj as ProductInfoHeaderValue;

            if (other == null)
            {
                return false;
            }

            if (product == null)
            {
                // We compare comments using case-sensitive comparison.
                return string.CompareOrdinal(comment, other.comment) == 0;
            }

            return product.Equals(other.product);
        }

        public override int GetHashCode()
        {
            if (product == null)
            {
                return comment.GetHashCode();
            }
            return product.GetHashCode();
        }

        public static ProductInfoHeaderValue Parse(string input)
        {
            int index = 0;
            object result = ProductInfoHeaderParser.SingleValueParser.ParseValue(
                input, null, ref index);
            if (index < input.Length)
            {
                // There is some leftover data, invalid.  Normaly BaseHeaderParser.TryParseValue would 
                // handle this, but ProductInfoHeaderValue does not derive from BaseHeaderParser.
                throw new FormatException(string.Format(System.Globalization.CultureInfo.InvariantCulture, SR.net_http_headers_invalid_value, input.Substring(index)));
            }
            return (ProductInfoHeaderValue)result;
        }

        public static bool TryParse(string input, out ProductInfoHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (ProductInfoHeaderParser.SingleValueParser.TryParseValue(input, null, ref index, out output))
            {
                if (index < input.Length)
                {
                    // There is some leftover data, invalid.  Normaly BaseHeaderParser.TryParseValue would 
                    // handle this, but ProductInfoHeaderValue does not derive from BaseHeaderParser.
                    return false;
                }
                parsedValue = (ProductInfoHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetProductInfoLength(string input, int startIndex, out ProductInfoHeaderValue parsedValue)
        {
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }
            
            int current = startIndex;

            // Caller must remove leading whitespaces.
            string comment = null;
            ProductHeaderValue product = null;
            if (input[current] == '(')
            {
                int commentLength = 0;
                if (HttpRuleParser.GetCommentLength(input, current, out commentLength) != HttpParseResult.Parsed)
                {
                    return 0;
                }

                comment = input.Substring(current, commentLength);

                current = current + commentLength;
                current = current + HttpRuleParser.GetWhitespaceLength(input, current);
            }
            else
            { 
                // trailing whitespaces are removed by GetProductLength()
                int productLength = ProductHeaderValue.GetProductLength(input, current, out product);

                if (productLength == 0)
                {
                    return 0;
                }

                current = current + productLength;
            }

            parsedValue = new ProductInfoHeaderValue();
            parsedValue.product = product;
            parsedValue.comment = comment;
            return current - startIndex;
        }

        object ICloneable.Clone()
        {
            return new ProductInfoHeaderValue(this);
        }
    }
}
