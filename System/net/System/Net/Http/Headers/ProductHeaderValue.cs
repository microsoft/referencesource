using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    public class ProductHeaderValue : ICloneable
    {
        private string name;
        private string version;

        public string Name
        {
            get { return name; }
        }

        // We can't use Version type, since a version can be e.g. "x11"
        public string Version
        {
            get { return version; }
        }

        public ProductHeaderValue(string name)
            : this(name, null)
        {
        }

        public ProductHeaderValue(string name, string version)
        {
            HeaderUtilities.CheckValidToken(name, "name");

            if (!string.IsNullOrEmpty(version))
            {
                // It's OK to have no version specified. But if, then it must be a valid token.
                HeaderUtilities.CheckValidToken(version, "version");
                this.version = version; // only assign value if it is not empty.
            }

            this.name = name;
        }

        private ProductHeaderValue(ProductHeaderValue source)
        {
            Contract.Requires(source != null);

            this.name = source.name;
            this.version = source.version;
        }

        private ProductHeaderValue()
        {
        }

        public override string ToString()
        {
            if (string.IsNullOrEmpty(version))
            {
                return name;
            }
            return name + "/" + version;
        }

        public override bool Equals(object obj)
        {
            ProductHeaderValue other = obj as ProductHeaderValue;

            if (other == null)
            {
                return false;
            }

            return (string.Compare(name, other.name, StringComparison.OrdinalIgnoreCase) == 0) &&
                (string.Compare(version, other.version, StringComparison.OrdinalIgnoreCase) == 0);
        }

        public override int GetHashCode()
        {
            int result = name.ToLowerInvariant().GetHashCode();

            if (!string.IsNullOrEmpty(version))
            {
                result = result ^ version.ToLowerInvariant().GetHashCode();
            }

            return result;
        }

        public static ProductHeaderValue Parse(string input)
        {
            int index = 0;
            return (ProductHeaderValue)GenericHeaderParser.SingleValueProductParser.ParseValue(input, null, ref index);
        }

        public static bool TryParse(string input, out ProductHeaderValue parsedValue)
        {
            int index = 0;
            object output;
            parsedValue = null;

            if (GenericHeaderParser.SingleValueProductParser.TryParseValue(input, null, ref index, out output))
            {
                parsedValue = (ProductHeaderValue)output;
                return true;
            }
            return false;
        }

        internal static int GetProductLength(string input, int startIndex, out ProductHeaderValue parsedValue)
        {
            Contract.Requires(startIndex >= 0);

            parsedValue = null;

            if (string.IsNullOrEmpty(input) || (startIndex >= input.Length))
            {
                return 0;
            }

            // Parse the name string: <name> in '<name>/<version>'
            int nameLength = HttpRuleParser.GetTokenLength(input, startIndex);

            if (nameLength == 0)
            {
                return 0;
            }

            ProductHeaderValue result = new ProductHeaderValue();
            result.name= input.Substring(startIndex, nameLength);
            int current = startIndex + nameLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            if ((current == input.Length) || (input[current] != '/'))
            {
                parsedValue = result;
                return current - startIndex;
            }

            current++; // skip '/' delimiter
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            // Parse the name string: <version> in '<name>/<version>'
            int versionLength = HttpRuleParser.GetTokenLength(input, current);

            if (versionLength == 0)
            {
                return 0; // If there is a '/' separator it must be followed by a valid token.
            }

            result.version = input.Substring(current, versionLength);
            
            current = current + versionLength;
            current = current + HttpRuleParser.GetWhitespaceLength(input, current);

            parsedValue = result;
            return current - startIndex;
        }

        object ICloneable.Clone()
        {
            return new ProductHeaderValue(this);
        }
    }
}
