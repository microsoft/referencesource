//---------------------------------------------------------------------
// <copyright file="HttpProcessUtility.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a utility class to help in processing HTTP requests.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

#if ASTORIA_CLIENT
namespace System.Data.Services.Client
#else
namespace System.Data.Services
#endif
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;

    /// <summary>Provides helper methods for processing HTTP requests.</summary>
    internal static class HttpProcessUtility
    {
        /// <summary>UTF-8 encoding, without the BOM preamble.</summary>
        /// <remarks>
        /// While a BOM preamble on UTF8 is generally benign, it seems that some MIME handlers under IE6 will not 
        /// process the payload correctly when included.
        /// 
        /// Because the data service should include the encoding as part of the Content-Type in the response,
        /// there should be no ambiguity as to what encoding is being used.
        /// 
        /// For further information, see http://www.unicode.org/faq/utf_bom.html#BOM.
        /// </remarks>
        internal static readonly UTF8Encoding EncodingUtf8NoPreamble = new UTF8Encoding(false, true);

        /// <summary>Encoding to fall back to an appropriate encoding is not available.</summary>
        internal static Encoding FallbackEncoding
        {
            get
            {
                return EncodingUtf8NoPreamble;
            }
        }

        /// <summary>Encoding implied by an unspecified encoding value.</summary>
        /// <remarks>See http://tools.ietf.org/html/rfc2616#section-3.4.1 for details.</remarks>
        private static Encoding MissingEncoding
        {
            get
            {
#if ASTORIA_LIGHT   // ISO-8859-1 not available
                return Encoding.UTF8;
#else
                return Encoding.GetEncoding("ISO-8859-1", new EncoderExceptionFallback(), new DecoderExceptionFallback());
#endif
            }
        }

#if !ASTORIA_CLIENT
        
        /// <summary>Builds a Content-Type header which includes MIME type and encoding information.</summary>
        /// <param name="mime">MIME type to be used.</param>
        /// <param name="encoding">Encoding to be used in response, possibly null.</param>
        /// <returns>The value for the Content-Type header.</returns>
        internal static string BuildContentType(string mime, Encoding encoding)
        {
            Debug.Assert(mime != null, "mime != null");
            if (encoding == null)
            {
                return mime;
            }
            else
            {
                return mime + ";charset=" + encoding.WebName;
            }
        }

        /// <summary>Selects an acceptable MIME type that satisfies the Accepts header.</summary>
        /// <param name="acceptTypesText">Text for Accepts header.</param>
        /// <param name="availableTypes">
        /// Types that the server is willing to return, in descending order 
        /// of preference.
        /// </param>
        /// <returns>The best MIME type for the client</returns>
        internal static string SelectMimeType(string acceptTypesText, string[] availableTypes)
        {
            Debug.Assert(availableTypes != null, "acceptableTypes != null");
            string selectedContentType = null;
            int selectedMatchingParts = -1;
            int selectedQualityValue = 0;
            int selectedPreferenceIndex = Int32.MaxValue;
            bool acceptable = false;
            bool acceptTypesEmpty = true;
            if (!String.IsNullOrEmpty(acceptTypesText))
            {
                IEnumerable<MediaType> acceptTypes = MimeTypesFromAcceptHeader(acceptTypesText);
                foreach (MediaType acceptType in acceptTypes)
                {
                    acceptTypesEmpty = false;
                    for (int i = 0; i < availableTypes.Length; i++)
                    {
                        string availableType = availableTypes[i];
                        int matchingParts = acceptType.GetMatchingParts(availableType);
                        if (matchingParts < 0)
                        {
                            continue;
                        }

                        if (matchingParts > selectedMatchingParts)
                        {
                            // A more specific type wins.
                            selectedContentType = availableType;
                            selectedMatchingParts = matchingParts;
                            selectedQualityValue = acceptType.SelectQualityValue();
                            selectedPreferenceIndex = i;
                            acceptable = selectedQualityValue != 0;
                        }
                        else if (matchingParts == selectedMatchingParts)
                        {
                            // A type with a higher q-value wins.
                            int candidateQualityValue = acceptType.SelectQualityValue();
                            if (candidateQualityValue > selectedQualityValue)
                            {
                                selectedContentType = availableType;
                                selectedQualityValue = candidateQualityValue;
                                selectedPreferenceIndex = i;
                                acceptable = selectedQualityValue != 0;
                            }
                            else if (candidateQualityValue == selectedQualityValue)
                            {
                                // A type that is earlier in the availableTypes array wins.
                                if (i < selectedPreferenceIndex)
                                {
                                    selectedContentType = availableType;
                                    selectedPreferenceIndex = i;
                                }
                            }
                        }
                    }
                }
            }

            if (acceptTypesEmpty)
            {
                selectedContentType = availableTypes[0];
            }
            else if (!acceptable)
            {
                selectedContentType = null;
            }

            return selectedContentType;
        }

        /// <summary>Gets the appropriate MIME type for the request, throwing if there is none.</summary>
        /// <param name='acceptTypesText'>Text as it appears in an HTTP Accepts header.</param>
        /// <param name='exactContentType'>Preferred content type to match if an exact media type is given - this is in descending order of preference.</param>
        /// <param name='inexactContentType'>Preferred fallback content type for inexact matches.</param>
        /// <returns>One of exactContentType or inexactContentType.</returns>
        internal static string SelectRequiredMimeType(
            string acceptTypesText,
            string[] exactContentType,
            string inexactContentType)
        {
            Debug.Assert(exactContentType != null && exactContentType.Length != 0, "exactContentType != null && exactContentType.Length != 0");
            Debug.Assert(inexactContentType != null, "inexactContentType != null");

            string selectedContentType = null;
            int selectedMatchingParts = -1;
            int selectedQualityValue = 0;
            bool acceptable = false;
            bool acceptTypesEmpty = true;
            bool foundExactMatch = false;

            if (!String.IsNullOrEmpty(acceptTypesText))
            {
                IEnumerable<MediaType> acceptTypes = MimeTypesFromAcceptHeader(acceptTypesText);
                foreach (MediaType acceptType in acceptTypes)
                {
                    acceptTypesEmpty = false;
                    for (int i = 0; i < exactContentType.Length; i++)
                    {
                        if (WebUtil.CompareMimeType(acceptType.MimeType, exactContentType[i]))
                        {
                            selectedContentType = exactContentType[i];
                            selectedQualityValue = acceptType.SelectQualityValue();
                            acceptable = selectedQualityValue != 0;
                            foundExactMatch = true;
                            break;
                        }
                    }

                    if (foundExactMatch)
                    {
                        break;
                    }

                    int matchingParts = acceptType.GetMatchingParts(inexactContentType);
                    if (matchingParts < 0)
                    {
                        continue;
                    }

                    if (matchingParts > selectedMatchingParts)
                    {
                        // A more specific type wins.
                        selectedContentType = inexactContentType;
                        selectedMatchingParts = matchingParts;
                        selectedQualityValue = acceptType.SelectQualityValue();
                        acceptable = selectedQualityValue != 0;
                    }
                    else if (matchingParts == selectedMatchingParts)
                    {
                        // A type with a higher q-value wins.
                        int candidateQualityValue = acceptType.SelectQualityValue();
                        if (candidateQualityValue > selectedQualityValue)
                        {
                            selectedContentType = inexactContentType;
                            selectedQualityValue = candidateQualityValue;
                            acceptable = selectedQualityValue != 0;
                        }
                    }
                }
            }

            if (!acceptable && !acceptTypesEmpty)
            {
                throw Error.HttpHeaderFailure(415, Strings.DataServiceException_UnsupportedMediaType);
            }

            if (acceptTypesEmpty)
            {
                Debug.Assert(selectedContentType == null, "selectedContentType == null - otherwise accept types were not empty");
                selectedContentType = inexactContentType;
            }

            Debug.Assert(selectedContentType != null, "selectedContentType != null - otherwise no selection was made");
            return selectedContentType;
        }

        /// <summary>Gets the best encoding available for the specified charset request.</summary>
        /// <param name="acceptCharset">
        /// The Accept-Charset header value (eg: "iso-8859-5, unicode-1-1;q=0.8").
        /// </param>
        /// <returns>An Encoding object appropriate to the specifed charset request.</returns>
        internal static Encoding EncodingFromAcceptCharset(string acceptCharset)
        {
            // Determines the appropriate encoding mapping according to
            // RFC 2616.14.2 (http://tools.ietf.org/html/rfc2616#section-14.2).
            Encoding result = null;
            if (!string.IsNullOrEmpty(acceptCharset))
            {
                // PERF: keep a cache of original strings to resolved Encoding.
                List<CharsetPart> parts = new List<CharsetPart>(AcceptCharsetParts(acceptCharset));
                parts.Sort(delegate(CharsetPart x, CharsetPart y)
                {
                    return y.Quality - x.Quality;
                });

                var encoderFallback = new EncoderExceptionFallback();
                var decoderFallback = new DecoderExceptionFallback();
                foreach (CharsetPart part in parts)
                {
                    if (part.Quality > 0)
                    {
                        // When UTF-8 is specified, select the version that doesn't use the BOM.
                        if (String.Compare("utf-8", part.Charset, StringComparison.OrdinalIgnoreCase) == 0)
                        {
                            result = FallbackEncoding;
                            break;
                        }
                        else
                        {
                            try
                            {
                                result = Encoding.GetEncoding(part.Charset, encoderFallback, decoderFallback);
                                break;
                            }
                            catch (ArgumentException)
                            {
                                // This exception is thrown when the character
                                // set isn't supported - it is ignored so
                                // other possible charsets are evaluated.
                            }
                        }
                    }
                }
            }

            // No Charset was specifed, or if charsets were specified, no valid charset was found.
            // Returning a different charset is also valid.
            if (result == null)
            {
                result = FallbackEncoding;
            }

            return result;
        }
#endif

        /// <summary>Reads a Content-Type header and extracts the MIME type/subtype and encoding.</summary>
        /// <param name="contentType">The Content-Type header.</param>
        /// <param name="mime">The MIME type in standard type/subtype form, without parameters.</param>
        /// <param name="encoding">Encoding (possibly null).</param>
        /// <returns>parameters of content type</returns>
        internal static KeyValuePair<string, string>[] ReadContentType(string contentType, out string mime, out Encoding encoding)
        {
            if (String.IsNullOrEmpty(contentType))
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_ContentTypeMissing);
            }

            MediaType mediaType = ReadMediaType(contentType);
            mime = mediaType.MimeType;
            encoding = mediaType.SelectEncoding();
            return mediaType.Parameters;
        }

#if !ASTORIA_CLIENT
        /// <summary>
        /// Given the parameters, search for the parameter with the given name and returns its value.
        /// </summary>
        /// <param name="parameters">list of parameters specified.</param>
        /// <param name="parameterName">name of the parameter whose value needs to be returned.</param>
        /// <returns>returns the value of the parameter with the given name. Returns null, if the parameter is not found.</returns>
        internal static string GetParameterValue(KeyValuePair<string, string>[] parameters, string parameterName)
        {
            if (parameters == null)
            {
                return null;
            }

            foreach (KeyValuePair<string, string> parameterInfo in parameters)
            {
                if (parameterInfo.Key == parameterName)
                {
                    return parameterInfo.Value;
                }
            }

            return null;
        }

#endif

        /// <summary>Tries to read a WCF Data Service version string.</summary>
        /// <param name="text">Text to read.</param>
        /// <param name="result">Parsed version and trailing text.</param>
        /// <returns>true if the version was read successfully; false otherwise.</returns>
        internal static bool TryReadVersion(string text, out KeyValuePair<Version, string> result)
        {
            Debug.Assert(text != null, "text != null");

            // Separate version number and extra string.
            int separator = text.IndexOf(';');
            string versionText, libraryName;
            if (separator >= 0)
            {
                versionText = text.Substring(0, separator);
                libraryName = text.Substring(separator + 1).Trim();
            }
            else
            {
                versionText = text;
                libraryName = null;
            }

            result = default(KeyValuePair<Version, string>);
            versionText = versionText.Trim();

            // The Version constructor allows for a more complex syntax, including
            // build, revisions, and major/minor for revisions. We only take two
            // number parts separated by a single dot.
            bool dotFound = false;
            for (int i = 0; i < versionText.Length; i++)
            {
                if (versionText[i] == '.')
                {
                    if (dotFound)
                    {
                        return false;
                    }

                    dotFound = true;
                }
                else if (versionText[i] < '0' || versionText[i] > '9')
                {
                    return false;
                }
            }

            try
            {
                result = new KeyValuePair<Version, string>(new Version(versionText), libraryName);
                return true;
            }
            catch (Exception e)
            {
                if (e is FormatException || e is OverflowException || e is ArgumentException)
                {
                    return false;
                }

                throw;
            }
        }

        /// <summary>Gets the named encoding if specified.</summary>
        /// <param name="name">Name (possibly null or empty).</param>
        /// <returns>
        /// The named encoding if specified; the encoding for HTTP missing 
        /// charset specification otherwise.
        /// </returns>
        /// <remarks>
        /// See http://tools.ietf.org/html/rfc2616#section-3.4.1 for details.
        /// </remarks>
        private static Encoding EncodingFromName(string name)
        {
            if (name == null)
            {
                return MissingEncoding;
            }

            name = name.Trim();
            if (name.Length == 0)
            {
                return MissingEncoding;
            }
            else
            {
                try
                {
#if ASTORIA_LIGHT
                    // SQLBU 641147: Silverlight client failing with FireFox 3.0
                    // we know we can do this with Silverlight because our http stack
                    // lets the browser convert from "ISO-8859-1" to string and then the stack
                    // converts the string into UTF8 for streaming into the client.
                    // it works for both nested payloads in batch and direct requests.
                    // Coordinate any change to this with the HttpWebRequest.ReadResponse()
                    return Encoding.UTF8;
#else
                    return Encoding.GetEncoding(name);
#endif
                }
                catch (ArgumentException)
                {
                    // 400 - Bad Request
                    throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_EncodingNotSupported(name));
                }
            }
        }

#if !ASTORIA_CLIENT
        /// <summary>Creates a new exception for parsing errors.</summary>
        /// <param name="message">Message for error.</param>
        /// <returns>A new exception that can be thrown for a parsing error.</returns>
        private static DataServiceException CreateParsingException(string message)
        {
            // Status code "400"  ; Section 10.4.1: Bad Request
            return Error.HttpHeaderFailure(400, message);
        }
#endif

        /// <summary>Reads the type and subtype specifications for a MIME type.</summary>
        /// <param name='text'>Text in which specification exists.</param>
        /// <param name='textIndex'>Pointer into text.</param>
        /// <param name='type'>Type of media found.</param>
        /// <param name='subType'>Subtype of media found.</param>
        private static void ReadMediaTypeAndSubtype(string text, ref int textIndex, out string type, out string subType)
        {
            Debug.Assert(text != null, "text != null");
            int textStart = textIndex;
            if (ReadToken(text, ref textIndex))
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeUnspecified);
            }

            if (text[textIndex] != '/')
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeRequiresSlash);
            }

            type = text.Substring(textStart, textIndex - textStart);
            textIndex++;

            int subTypeStart = textIndex;
            ReadToken(text, ref textIndex);

            if (textIndex == subTypeStart)
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeRequiresSubType);
            }

            subType = text.Substring(subTypeStart, textIndex - subTypeStart);
        }

        /// <summary>Reads a media type definition as used in a Content-Type header.</summary>
        /// <param name="text">Text to read.</param>
        /// <returns>The <see cref="MediaType"/> defined by the specified <paramref name="text"/></returns>
        /// <remarks>All syntactic errors will produce a 400 - Bad Request status code.</remarks>
        private static MediaType ReadMediaType(string text)
        {
            Debug.Assert(text != null, "text != null");

            string type;
            string subType;
            int textIndex = 0;
            ReadMediaTypeAndSubtype(text, ref textIndex, out type, out subType);

            KeyValuePair<string, string>[] parameters = null;
            while (!SkipWhitespace(text, ref textIndex))
            {
                if (text[textIndex] != ';')
                {
                    throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeRequiresSemicolonBeforeParameter);
                }

                textIndex++;
                if (SkipWhitespace(text, ref textIndex))
                {
                    // ';' should be a leading separator, but we choose to be a 
                    // bit permissive and allow it as a final delimiter as well.
                    break;
                }

                ReadMediaTypeParameter(text, ref textIndex, ref parameters);
            }

            return new MediaType(type, subType, parameters);
        }

        /// <summary>
        /// Reads a token on the specified text by advancing an index on it.
        /// </summary>
        /// <param name="text">Text to read token from.</param>
        /// <param name="textIndex">Index for the position being scanned on text.</param>
        /// <returns>true if the end of the text was reached; false otherwise.</returns>
        private static bool ReadToken(string text, ref int textIndex)
        {
            while (textIndex < text.Length && IsHttpToken(text[textIndex]))
            {
                textIndex++;
            }

            return (textIndex == text.Length);
        }

        /// <summary>
        /// Skips whitespace in the specified text by advancing an index to
        /// the next non-whitespace character.
        /// </summary>
        /// <param name="text">Text to scan.</param>
        /// <param name="textIndex">Index to begin scanning from.</param>
        /// <returns>true if the end of the string was reached, false otherwise.</returns>
        private static bool SkipWhitespace(string text, ref int textIndex)
        {
            Debug.Assert(text != null, "text != null");
            Debug.Assert(text.Length >= 0, "text >= 0");
            Debug.Assert(textIndex <= text.Length, "text <= text.Length");

            while (textIndex < text.Length && Char.IsWhiteSpace(text, textIndex))
            {
                textIndex++;
            }

            return (textIndex == text.Length);
        }

#if !ASTORIA_CLIENT
        /// <summary>
        /// Verfies whether the specified character is a valid separator in
        /// an HTTP header list of element.
        /// </summary>
        /// <param name="c">Character to verify.</param>
        /// <returns>true if c is a valid character for separating elements; false otherwise.</returns>
        private static bool IsHttpElementSeparator(char c)
        {
            return c == ',' || c == ' ' || c == '\t';
        }

        /// <summary>
        /// "Reads" a literal from the specified string by verifying that
        /// the exact text can be found at the specified position.
        /// </summary>
        /// <param name="text">Text within which a literal should be checked.</param>
        /// <param name="textIndex">Index in text where the literal should be found.</param>
        /// <param name="literal">Literal to check at the specified position.</param>
        /// <returns>true if the end of string is found; false otherwise.</returns>
        private static bool ReadLiteral(string text, int textIndex, string literal)
        {
            if (String.Compare(text, textIndex, literal, 0, literal.Length, StringComparison.Ordinal) != 0)
            {
                // Failed to find expected literal.
                throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
            }

            return textIndex + literal.Length == text.Length;
        }

        /// <summary>
        /// Converts the specified character from the ASCII range to a digit.
        /// </summary>
        /// <param name="c">Character to convert.</param>
        /// <returns>
        /// The Int32 value for c, or -1 if it is an element separator.
        /// </returns>
        private static int DigitToInt32(char c)
        {
            if (c >= '0' && c <= '9')
            {
                return (int)(c - '0');
            }
            else
            {
                if (IsHttpElementSeparator(c))
                {
                    return -1;
                }
                else
                {
                    throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
                }
            }
        }

        /// <summary>Returns all MIME types from the specified (non-blank) <paramref name='text' />.</summary>
        /// <param name='text'>Non-blank text, as it appears on an HTTP Accepts header.</param>
        /// <returns>An enumerable object with media type descriptions.</returns>
        private static IEnumerable<MediaType> MimeTypesFromAcceptHeader(string text)
        {
            Debug.Assert(!String.IsNullOrEmpty(text), "!String.IsNullOrEmpty(text)");
            List<MediaType> mediaTypes = new List<MediaType>();
            int textIndex = 0;
            while (!SkipWhitespace(text, ref textIndex))
            {
                string type;
                string subType;
                ReadMediaTypeAndSubtype(text, ref textIndex, out type, out subType);

                KeyValuePair<string, string>[] parameters = null;
                while (!SkipWhitespace(text, ref textIndex))
                {
                    if (text[textIndex] == ',')
                    {
                        textIndex++;
                        break;
                    }

                    if (text[textIndex] != ';')
                    {
                        throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeRequiresSemicolonBeforeParameter);
                    }

                    textIndex++;
                    if (SkipWhitespace(text, ref textIndex))
                    {
                        // ';' should be a leading separator, but we choose to be a 
                        // bit permissive and allow it as a final delimiter as well.
                        break;
                    }

                    ReadMediaTypeParameter(text, ref textIndex, ref parameters);
                }

                mediaTypes.Add(new MediaType(type, subType, parameters));
            }

            return mediaTypes;
        }
#endif

        /// <summary>Read a parameter for a media type/range.</summary>
        /// <param name="text">Text to read from.</param>
        /// <param name="textIndex">Pointer in text.</param>
        /// <param name="parameters">Array with parameters to grow as necessary.</param>
        private static void ReadMediaTypeParameter(string text, ref int textIndex, ref KeyValuePair<string, string>[] parameters)
        {
            int startIndex = textIndex;
            if (ReadToken(text, ref textIndex))
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeMissingValue);
            }

            string parameterName = text.Substring(startIndex, textIndex - startIndex);
            if (text[textIndex] != '=')
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_MediaTypeMissingValue);
            }

            textIndex++;

            string parameterValue = ReadQuotedParameterValue(parameterName, text, ref textIndex);

            // Add the parameter name/value pair to the list.
            if (parameters == null)
            {
                parameters = new KeyValuePair<string, string>[1];
            }
            else
            {
                KeyValuePair<string, string>[] grow = new KeyValuePair<string, string>[parameters.Length + 1];
                Array.Copy(parameters, grow, parameters.Length);
                parameters = grow;
            }

            parameters[parameters.Length - 1] = new KeyValuePair<string, string>(parameterName, parameterValue);
        }

        /// <summary>
        /// Reads Mime type parameter value for a particular parameter in the Content-Type/Accept headers.
        /// </summary>
        /// <param name="parameterName">Name of parameter.</param>
        /// <param name="headerText">Header text.</param>
        /// <param name="textIndex">Parsing index in <paramref name="headerText"/>.</param>
        /// <returns>String representing the value of the <paramref name="parameterName"/> parameter.</returns>
        private static string ReadQuotedParameterValue(string parameterName, string headerText, ref int textIndex)
        {
            StringBuilder parameterValue = new StringBuilder();
            
            // Check if the value is quoted.
            bool valueIsQuoted = false;
            if (textIndex < headerText.Length)
            {
                if (headerText[textIndex] == '\"')
                {
                    textIndex++;
                    valueIsQuoted = true;
                }
            }

            while (textIndex < headerText.Length)
            {
                char currentChar = headerText[textIndex];

                if (currentChar == '\\' || currentChar == '\"')
                {
                    if (!valueIsQuoted)
                    {
                        throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_EscapeCharWithoutQuotes(parameterName));
                    }

                    textIndex++;

                    // End of quoted parameter value.
                    if (currentChar == '\"')
                    {
                        valueIsQuoted = false;
                        break;
                    }

                    if (textIndex >= headerText.Length)
                    {
                        throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_EscapeCharAtEnd(parameterName));
                    }

                    currentChar = headerText[textIndex];        
                }
                else
                if (!IsHttpToken(currentChar))
                {
                    // If the given character is special, we stop processing.
                    break;
                }

                parameterValue.Append(currentChar);
                textIndex++;
            }

            if (valueIsQuoted)
            {
                throw Error.HttpHeaderFailure(400, Strings.HttpProcessUtility_ClosingQuoteNotFound(parameterName));
            }

            return parameterValue.ToString();
        }

#if !ASTORIA_CLIENT
        /// <summary>
        /// Reads the numeric part of a quality value substring, normalizing it to 0-1000
        /// rather than the standard 0.000-1.000 ranges.
        /// </summary>
        /// <param name="text">Text to read qvalue from.</param>
        /// <param name="textIndex">Index into text where the qvalue starts.</param>
        /// <param name="qualityValue">After the method executes, the normalized qvalue.</param>
        /// <remarks>
        /// For more information, see RFC 2616.3.8.
        /// </remarks>
        private static void ReadQualityValue(string text, ref int textIndex, out int qualityValue)
        {
            char digit = text[textIndex++];
            if (digit == '0')
            {
                qualityValue = 0;
            }
            else if (digit == '1')
            {
                qualityValue = 1;
            }
            else
            {
                throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
            }

            if (textIndex < text.Length && text[textIndex] == '.')
            {
                textIndex++;

                int adjustFactor = 1000;
                while (adjustFactor > 1 && textIndex < text.Length)
                {
                    char c = text[textIndex];
                    int charValue = DigitToInt32(c);
                    if (charValue >= 0)
                    {
                        textIndex++;
                        adjustFactor /= 10;
                        qualityValue *= 10;
                        qualityValue += charValue;
                    }
                    else
                    {
                        break;
                    }
                }

                qualityValue = qualityValue *= adjustFactor;
                if (qualityValue > 1000)
                {
                    // Too high of a value in qvalue.
                    throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
                }
            }
            else
            {
                qualityValue *= 1000;
            }
        }

        /// <summary>
        /// Enumerates each charset part in the specified Accept-Charset header.
        /// </summary>
        /// <param name="headerValue">Non-null and non-empty header value for Accept-Charset.</param>
        /// <returns>
        /// A (non-sorted) enumeration of CharsetPart elements, which include
        /// a charset name and a quality (preference) value, normalized to 0-1000.
        /// </returns>
        private static IEnumerable<CharsetPart> AcceptCharsetParts(string headerValue)
        {
            Debug.Assert(!String.IsNullOrEmpty(headerValue), "!String.IsNullOrEmpty(headerValuer)");

            // PERF: optimize for common patterns.
            bool commaRequired = false; // Whether a comma should be found
            int headerIndex = 0;        // Index of character being procesed on headerValue.
            int headerStart;            // Index into headerValue for the start of the charset name.
            int headerNameEnd;          // Index into headerValue for the end of the charset name (+1).
            int headerEnd;              // Index into headerValue for this charset part (+1).
            int qualityValue;           // Normalized qvalue for this charset.

            while (headerIndex < headerValue.Length)
            {
                if (SkipWhitespace(headerValue, ref headerIndex))
                {
                    yield break;
                }

                if (headerValue[headerIndex] == ',')
                {
                    commaRequired = false;
                    headerIndex++;
                    continue;
                }

                if (commaRequired)
                {
                    // Comma missing between charset elements.
                    throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
                }

                headerStart = headerIndex;
                headerNameEnd = headerStart;

                bool endReached = ReadToken(headerValue, ref headerNameEnd);
                if (headerNameEnd == headerIndex)
                {
                    // Invalid charset name.
                    throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
                }

                if (endReached)
                {
                    qualityValue = 1000;
                    headerEnd = headerNameEnd;
                }
                else
                {
                    char afterNameChar = headerValue[headerNameEnd];
                    if (IsHttpSeparator(afterNameChar))
                    {
                        if (afterNameChar == ';')
                        {
                            if (ReadLiteral(headerValue, headerNameEnd, ";q="))
                            {
                                // Unexpected end of qvalue.
                                throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
                            }

                            headerEnd = headerNameEnd + 3;
                            ReadQualityValue(headerValue, ref headerEnd, out qualityValue);
                        }
                        else
                        {
                            qualityValue = 1000;
                            headerEnd = headerNameEnd;
                        }
                    }
                    else
                    {
                        // Invalid separator character.
                        throw CreateParsingException(Strings.HttpContextServiceHost_MalformedHeaderValue);
                    }
                }

                yield return new CharsetPart(headerValue.Substring(headerStart, headerNameEnd - headerStart), qualityValue);

                // Prepare for next charset; we require at least one comma before we process it.
                commaRequired = true;
                headerIndex = headerEnd;
            }
        }
#endif

        /// <summary>
        /// Determines whether the specified character is a valid HTTP separator.
        /// </summary>
        /// <param name="c">Character to verify.</param>
        /// <returns>true if c is a separator; false otherwise.</returns>
        /// <remarks>
        /// See RFC 2616 2.2 for further information.
        /// </remarks>
        private static bool IsHttpSeparator(char c)
        {
            return
                c == '(' || c == ')' || c == '<' || c == '>' || c == '@' ||
                c == ',' || c == ';' || c == ':' || c == '\\' || c == '"' ||
                c == '/' || c == '[' || c == ']' || c == '?' || c == '=' ||
                c == '{' || c == '}' || c == ' ' || c == '\x9';
        }

        /// <summary>
        /// Determines whether the specified character is a valid HTTP header token character.
        /// </summary>
        /// <param name="c">Character to verify.</param>
        /// <returns>true if c is a valid HTTP header token character; false otherwise.</returns>
        private static bool IsHttpToken(char c)
        {
            // A token character is any character (0-127) except control (0-31) or
            // separators. 127 is DEL, a control character.
            return c < '\x7F' && c > '\x1F' && !IsHttpSeparator(c);
        }

#if !ASTORIA_CLIENT
        /// <summary>Provides a struct to encapsulate a charset name and its relative desirability.</summary>
        private struct CharsetPart
        {
            /// <summary>Name of the charset.</summary>
            internal readonly string Charset;

            /// <summary>Charset quality (desirability), normalized to 0-1000.</summary>
            internal readonly int Quality;

            /// <summary>
            /// Initializes a new CharsetPart with the specified values.
            /// </summary>
            /// <param name="charset">Name of charset.</param>
            /// <param name="quality">Charset quality (desirability), normalized to 0-1000.</param>
            internal CharsetPart(string charset, int quality)
            {
                Debug.Assert(charset != null, "charset != null");
                Debug.Assert(charset.Length > 0, "charset.Length > 0");
                Debug.Assert(0 <= quality && quality <= 1000, "0 <= quality && quality <= 1000");

                this.Charset = charset;
                this.Quality = quality;
            }
        }
#endif

        /// <summary>Use this class to represent a media type definition.</summary>
        [DebuggerDisplay("MediaType [{type}/{subType}]")]
        private sealed class MediaType
        {
            /// <summary>Parameters specified on the media type.</summary>
            private readonly KeyValuePair<string, string>[] parameters;

            /// <summary>Sub-type specification (for example, 'plain').</summary>
            private readonly string subType;

            /// <summary>Type specification (for example, 'text').</summary>
            private readonly string type;

            /// <summary>
            /// Initializes a new <see cref="MediaType"/> read-only instance.
            /// </summary>
            /// <param name="type">Type specification (for example, 'text').</param>
            /// <param name="subType">Sub-type specification (for example, 'plain').</param>
            /// <param name="parameters">Parameters specified on the media type.</param>
            internal MediaType(string type, string subType, KeyValuePair<string, string>[] parameters)
            {
                Debug.Assert(type != null, "type != null");
                Debug.Assert(subType != null, "subType != null");

                this.type = type;
                this.subType = subType;
                this.parameters = parameters;
            }

            /// <summary>Returns the MIME type in standard type/subtype form, without parameters.</summary>
            internal string MimeType
            {
                get { return this.type + "/" + this.subType; }
            }

            /// <summary>media type parameters</summary>
            internal KeyValuePair<string, string>[] Parameters
            {
                get { return this.parameters; }
            }

#if !ASTORIA_CLIENT
            /// <summary>Gets a number of non-* matching types, or -1 if not matching at all.</summary>
            /// <param name="candidate">Candidate MIME type to match.</param>
            /// <returns>The number of non-* matching types, or -1 if not matching at all.</returns>
            internal int GetMatchingParts(string candidate)
            {
                Debug.Assert(candidate != null, "candidate must not be null.");

                int result = -1;
                if (candidate.Length > 0)
                {
                    if (this.type == "*")
                    {
                        result = 0;
                    }
                    else
                    {
                        int separatorIdx = candidate.IndexOf('/');
                        if (separatorIdx >= 0) 
                        {
                            string candidateType = candidate.Substring(0, separatorIdx);
                            if (WebUtil.CompareMimeType(this.type, candidateType))
                            {
                                if (this.subType == "*")
                                {
                                    result = 1;
                                }
                                else
                                {
                                    string candidateSubType = candidate.Substring(candidateType.Length + 1);
                                    if (WebUtil.CompareMimeType(this.subType, candidateSubType))
                                    {
                                        result = 2;
                                    }
                                }
                            }
                        }
                    }
                }

                return result;
            }

            /// <summary>Selects a quality value for the specified type.</summary>
            /// <returns>The quality value, in range from 0 through 1000.</returns>
            /// <remarks>See http://tools.ietf.org/html/rfc2616#section-14.1 for further details.</remarks>
            internal int SelectQualityValue()
            {
                if (this.parameters != null)
                {
                    foreach (KeyValuePair<string, string> parameter in this.parameters)
                    {
                        if (String.Equals(parameter.Key, XmlConstants.HttpQValueParameter, StringComparison.OrdinalIgnoreCase))
                        {
                            string qvalueText = parameter.Value.Trim();
                            if (qvalueText.Length > 0)
                            {
                                int result;
                                int textIndex = 0;
                                ReadQualityValue(qvalueText, ref textIndex, out result);
                                return result;
                            }
                        }
                    }
                }

                return 1000;
            }
#endif

            /// <summary>
            /// Selects the encoding appropriate for this media type specification
            /// (possibly null).
            /// </summary>
            /// <returns>
            /// The encoding explicitly defined on the media type specification, or
            /// the default encoding for well-known media types.
            /// </returns>
            /// <remarks>
            /// As per http://tools.ietf.org/html/rfc2616#section-3.7, the type, 
            /// subtype and parameter name attributes are case-insensitive.
            /// </remarks>
            internal Encoding SelectEncoding()
            {
                if (this.parameters != null)
                {
                    foreach (KeyValuePair<string, string> parameter in this.parameters)
                    {
                        if (String.Equals(parameter.Key, XmlConstants.HttpCharsetParameter, StringComparison.OrdinalIgnoreCase))
                        {
                            string encodingName = parameter.Value.Trim();
                            if (encodingName.Length > 0)
                            {
                                return EncodingFromName(parameter.Value);
                            }
                        }
                    }
                }

                // Select the default encoding for this media type.
                if (String.Equals(this.type, XmlConstants.MimeTextType, StringComparison.OrdinalIgnoreCase))
                {
                    // HTTP 3.7.1 Canonicalization and Text Defaults
                    // "text" subtypes default to ISO-8859-1
                    //
                    // Unless the subtype is XML, in which case we should default
                    // to us-ascii. Instead we return null, to let the encoding
                    // in the <?xml ...?> PI win (http://tools.ietf.org/html/rfc3023#section-3.1)
                    if (String.Equals(this.subType, XmlConstants.MimeXmlSubType, StringComparison.OrdinalIgnoreCase))
                    {
                        return null;
                    }
                    else
                    {
                        return MissingEncoding;
                    }
                }
                else if (String.Equals(this.type, XmlConstants.MimeApplicationType, StringComparison.OrdinalIgnoreCase) &&
                    String.Equals(this.subType, XmlConstants.MimeJsonSubType, StringComparison.OrdinalIgnoreCase))
                {
                    // http://tools.ietf.org/html/rfc4627#section-3
                    // The default encoding is UTF-8.
                    return FallbackEncoding;
                }
                else
                {
                    return null;
                }
            }
        }
    }
}
