using System.Diagnostics.Contracts;

namespace System.Net.Http.Headers
{
    internal class MediaTypeHeaderParser : BaseHeaderParser
    {
        private bool supportsMultipleValues;
        private Func<MediaTypeHeaderValue> mediaTypeCreator;

        internal static readonly MediaTypeHeaderParser SingleValueParser = new MediaTypeHeaderParser(false,
            CreateMediaType);
        internal static readonly MediaTypeHeaderParser SingleValueWithQualityParser = new MediaTypeHeaderParser(false,
            CreateMediaTypeWithQuality);
        internal static readonly MediaTypeHeaderParser MultipleValuesParser = new MediaTypeHeaderParser(true,
            CreateMediaTypeWithQuality);

        private MediaTypeHeaderParser(bool supportsMultipleValues, Func<MediaTypeHeaderValue> mediaTypeCreator)
            : base(supportsMultipleValues)
        {            
            Contract.Requires(mediaTypeCreator != null);

            this.supportsMultipleValues = supportsMultipleValues;
            this.mediaTypeCreator = mediaTypeCreator;
        }

        protected override int GetParsedValueLength(string value, int startIndex, object storeValue, 
            out object parsedValue)
        {
            MediaTypeHeaderValue temp = null;
            int resultLength = MediaTypeHeaderValue.GetMediaTypeLength(value, startIndex, mediaTypeCreator, out temp);

            parsedValue = temp;
            return resultLength;
        }

        private static MediaTypeHeaderValue CreateMediaType()
        {
            return new MediaTypeHeaderValue();
        }

        private static MediaTypeHeaderValue CreateMediaTypeWithQuality()
        {
            return new MediaTypeWithQualityHeaderValue();
        }
    }
}
