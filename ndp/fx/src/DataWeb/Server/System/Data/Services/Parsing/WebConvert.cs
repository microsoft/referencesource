//---------------------------------------------------------------------
// <copyright file="WebConvert.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides methods to convert URI and payload values.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Parsing
{
    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Xml;
#if ASTORIA_CLIENT
    using System.Data.Services.Client;
#else
    using System.Globalization;
#endif

    /// <summary>Use this class to convert URI and payload values.</summary>
    internal static class WebConvert
    {
        /// <summary>Constant table of nibble-to-hex convertion values.</summary>
        private const string HexValues = "0123456789ABCDEF";

        /// <summary>Prefix to hex-encoded values.</summary>
        private const string XmlHexEncodePrefix = "0x";

#if ASTORIA_SERVER
        /// <summary>XML whitespace characters to trim around literals.</summary>
        private static char[] XmlWhitespaceChars = new char[] { ' ', '\t', '\n', '\r' };

        /// <summary>Determines whether the specified character is a valid hexadecimal digit.</summary>
        /// <param name="c">Character to check.</param>
        /// <returns>true if <paramref name="c"/> is a valid hex digit; false otherwise.</returns>
        internal static bool IsCharHexDigit(char c)
        {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        }

#endif

        /// <summary>Converts the given byte[] into string.</summary>
        /// <param name="byteArray">byte[] that needs to be converted.</param>
        /// <returns>String containing hex values representing the byte[].</returns>
        internal static string ConvertByteArrayToKeyString(byte[] byteArray)
        {
            StringBuilder hexBuilder = new StringBuilder(3 + byteArray.Length * 2);
            hexBuilder.Append(XmlConstants.XmlBinaryPrefix);
            hexBuilder.Append("'");
            for (int i = 0; i < byteArray.Length; i++)
            {
                hexBuilder.Append(HexValues[byteArray[i] >> 4]);
                hexBuilder.Append(HexValues[byteArray[i] & 0x0F]);
            }

            hexBuilder.Append("'");
#if DEBUG
            string escapee = hexBuilder.ToString(2, hexBuilder.Length-3);
            Debug.Assert(escapee == Uri.EscapeDataString(escapee), "binary representation is not expected to require escaping as a data string");
#endif
            return hexBuilder.ToString();
        }

#if ASTORIA_SERVER

        /// <summary>Checks whether the specified text is a correctly formatted quoted value.</summary>
        /// <param name='text'>Text to check.</param>
        /// <returns>true if the text is correctly formatted, false otherwise.</returns>
        internal static bool IsKeyValueQuoted(string text)
        {
            Debug.Assert(text != null, "text != null");
            if (text.Length < 2 || text[0] != '\'' || text[text.Length - 1] != '\'')
            {
                return false;
            }
            else
            {
                int startIndex = 1;
                while (startIndex < text.Length - 1)
                {
                    int match = text.IndexOf('\'', startIndex, text.Length - startIndex - 1);
                    if (match == -1)
                    {
                        break;
                    }
                    else if (match == text.Length - 2 || text[match + 1] != '\'')
                    {
                        return false;
                    }
                    else
                    {
                        startIndex = match + 2;
                    }
                }

                return true;
            }
        }

#endif

        /// <summary>
        /// Determines whether the values for the specified types should be 
        /// quoted in URI keys.
        /// </summary>
        /// <param name='type'>Type to check.</param>
        /// <returns>
        /// true if values of <paramref name='type' /> require quotes; false otherwise.
        /// </returns>
        internal static bool IsKeyTypeQuoted(Type type)
        {
            Debug.Assert(type != null, "type != null");
            return type == typeof(System.Xml.Linq.XElement) || type == typeof(string);
        }

        /// <summary>Converts the specified value to a encoded, serializable string for URI key.</summary>
        /// <param name="value">Non-null value to convert.</param>
        /// <param name="result">out parameter for value converted to a serializable string for URI key.</param>
        /// <returns>true/ false indicating success</returns>
        internal static bool TryKeyPrimitiveToString(object value, out string result)
        {
            Debug.Assert(value != null, "value != null");
            if (value.GetType() == typeof(byte[]))
            {
                result = ConvertByteArrayToKeyString((byte[])value);
            }
#if !ASTORIA_CLIENT
            else if (value.GetType() == typeof(System.Data.Linq.Binary))
            {
                return TryKeyPrimitiveToString(((System.Data.Linq.Binary)value).ToArray(), out result);
            }
#endif
            else
            {
                if (!TryXmlPrimitiveToString(value, out result))
                {
                    return false;
                }

                Debug.Assert(result != null, "result != null");
                if (IsKeyTypeQuoted(value.GetType()))
                {
                    result = result.Replace("'", "''");
                }

                // required for strings as data, DateTime for ':', numbers for '+'
                // we specifically do not want to encode leading and trailing "'" wrapping strings/datetime/guid
                result = Uri.EscapeDataString(result);

                if (value.GetType() == typeof(DateTime))
                {
                    result = XmlConstants.LiteralPrefixDateTime + "'" + result + "'";
                }
                else if (value.GetType() == typeof(Decimal))
                {
                    result = result + XmlConstants.XmlDecimalLiteralSuffix;
                }
                else if (value.GetType() == typeof(Guid))
                {
                    result = XmlConstants.LiteralPrefixGuid + "'" + result + "'";
                }
                else if (value.GetType() == typeof(Int64))
                {
                    result = result + XmlConstants.XmlInt64LiteralSuffix;
                }
                else if (value.GetType() == typeof(Single))
                {
                    result = result + XmlConstants.XmlSingleLiteralSuffix;
                }
#if ASTORIA_SERVER
                else if (value.GetType() == typeof(double))
                {
                    double d = (double)value;
                    if (!Double.IsInfinity(d) && !Double.IsNaN(d))
                    {
                        result = result + XmlConstants.XmlDoubleLiteralSuffix;
                    }
                }
#else
                else if (value.GetType() == typeof(double))
                {
                    result = AppendDecimalMarkerToDouble(result);
                }
#endif
                else if (IsKeyTypeQuoted(value.GetType()))
                {
                    result = "'" + result + "'";
                }
            }

            return true;
        }

#if ASTORIA_SERVER
        /// <summary>Removes quotes from the single-quotes text.</summary>
        /// <param name="text">Text to remove quotes from.</param>
        /// <returns>The specified <paramref name="text"/> with single quotes removed.</returns>
        internal static string RemoveQuotes(string text)
        {
            Debug.Assert(!String.IsNullOrEmpty(text), "!String.IsNullOrEmpty(text)");

            char quote = text[0];
            Debug.Assert(quote == '\'', "quote == '\''");
            Debug.Assert(text[text.Length - 1] == '\'', "text should end with '\''.");

            string s = text.Substring(1, text.Length - 2);
            int start = 0;
            while (true)
            {
                int i = s.IndexOf(quote, start);
                if (i < 0)
                {
                    break;
                }

                Debug.Assert(i + 1 < s.Length && s[i + 1] == '\'', @"Each single quote should be propertly escaped with double single quotes.");
                s = s.Remove(i, 1);
                start = i + 1;
            }

            return s;
        }

        /// <summary>Converts a string to a byte[] value.</summary>
        /// <param name="text">String text to convert.</param>
        /// <param name="targetValue">After invocation, converted value.</param>
        /// <returns>true if the value was converted; false otherwise.</returns>
        internal static bool TryKeyStringToByteArray(string text, out byte[] targetValue)
        {
            Debug.Assert(text != null, "text != null");

            if (!TryRemoveLiteralPrefix(XmlConstants.LiteralPrefixBinary, ref text) &&
                !TryRemoveLiteralPrefix(XmlConstants.XmlBinaryPrefix, ref text))
            {
                targetValue = null;
                return false;
            }

            if (!TryRemoveQuotes(ref text))
            {
                targetValue = null;
                return false;
            }

            if ((text.Length % 2) != 0)
            {
                targetValue = null;
                return false;
            }

            byte[] result = new byte[text.Length / 2];
            int resultIndex = 0;
            int textIndex = 0;
            while (resultIndex < result.Length)
            {
                char ch0 = text[textIndex];
                char ch1 = text[textIndex + 1];
                if (!IsCharHexDigit(ch0) || !IsCharHexDigit(ch1))
                {
                    targetValue = null;
                    return false;
                }

                result[resultIndex] = (byte)((byte)(HexCharToNibble(ch0) << 4) + HexCharToNibble(ch1));
                textIndex += 2;
                resultIndex++;
            }

            targetValue = result;
            return true;
        }

        /// <summary>Converts a string to a DateTime value.</summary>
        /// <param name="text">String text to convert.</param>
        /// <param name="targetValue">After invocation, converted value.</param>
        /// <returns>true if the value was converted; false otherwise.</returns>
        internal static bool TryKeyStringToDateTime(string text, out DateTime targetValue)
        {
            if (!TryRemoveLiteralPrefix(XmlConstants.LiteralPrefixDateTime, ref text))
            {
                targetValue = default(DateTime);
                return false;
            }

            if (!TryRemoveQuotes(ref text))
            {
                targetValue = default(DateTime);
                return false;
            }

            try
            {
                targetValue = XmlConvert.ToDateTime(text, XmlDateTimeSerializationMode.RoundtripKind);
                return true;
            }
            catch (FormatException)
            {
                targetValue = default(DateTime);
                return false;
            }
        }

        /// <summary>Converts a string to a GUID value.</summary>
        /// <param name="text">String text to convert.</param>
        /// <param name="targetValue">After invocation, converted value.</param>
        /// <returns>true if the value was converted; false otherwise.</returns>
        internal static bool TryKeyStringToGuid(string text, out Guid targetValue)
        {
            if (!TryRemoveLiteralPrefix(XmlConstants.LiteralPrefixGuid, ref text))
            {
                targetValue = default(Guid);
                return false;
            }

            if (!TryRemoveQuotes(ref text))
            {
                targetValue = default(Guid);
                return false;
            }

            try
            {
                targetValue = XmlConvert.ToGuid(text);
                return true;
            }
            catch (FormatException)
            {
                targetValue = default(Guid);
                return false;
            }
        }

        /// <summary>Converts a string to a primitive value.</summary>
        /// <param name="text">String text to convert.</param>
        /// <param name="targetType">Type to convert string to.</param>
        /// <param name="targetValue">After invocation, converted value.</param>
        /// <returns>true if the value was converted; false otherwise.</returns>
        internal static bool TryKeyStringToPrimitive(string text, Type targetType, out object targetValue)
        {
            Debug.Assert(text != null, "text != null");
            Debug.Assert(targetType != null, "targetType != null");

            targetType = Nullable.GetUnderlyingType(targetType) ?? targetType;

            byte[] byteArrayValue;
            bool binaryResult = TryKeyStringToByteArray(text, out byteArrayValue);
            if (targetType == typeof(byte[]) || targetType == typeof(System.Data.Linq.Binary))
            {
                // The object cast is required because otherwise the compiler uses the implicit byte[]
                // to Binary conversion and always returns Binary.
                targetValue =
                    (byteArrayValue != null && targetType == typeof(System.Data.Linq.Binary)) ?
                    (object)new System.Data.Linq.Binary(byteArrayValue) : (object)byteArrayValue;
                return binaryResult;
            }
            else if (binaryResult)
            {
                string keyValue = Encoding.UTF8.GetString(byteArrayValue);
                return TryKeyStringToPrimitive(keyValue, targetType, out targetValue);
            }
            // These have separate handlers for convenience - reuse them.
            else if (targetType == typeof(Guid))
            {
                Guid guidValue;
                bool result = TryKeyStringToGuid(text, out guidValue);
                targetValue = guidValue;
                return result;
            }
            else if (targetType == typeof(DateTime))
            {
                DateTime dateTimeValue;
                bool result = TryKeyStringToDateTime(text, out dateTimeValue);
                targetValue = dateTimeValue;
                return result;
            }

            bool quoted = WebConvert.IsKeyTypeQuoted(targetType);
            if (quoted != WebConvert.IsKeyValueQuoted(text))
            {
                targetValue = null;
                return false;
            }

            if (quoted)
            {
                Debug.Assert(IsKeyValueQuoted(text), "IsKeyValueQuoted(text) - otherwise caller didn't check this before");
                text = RemoveQuotes(text);
            }

            try
            {
                if (typeof(String) == targetType)
                {
                    targetValue = text;
                }
                else if (typeof(Boolean) == targetType)
                {
                    targetValue = XmlConvert.ToBoolean(text);
                }
                else if (typeof(Byte) == targetType)
                {
                    targetValue = XmlConvert.ToByte(text);
                }
                else if (typeof(SByte) == targetType)
                {
                    targetValue = XmlConvert.ToSByte(text);
                }
                else if (typeof(Int16) == targetType)
                {
                    targetValue = XmlConvert.ToInt16(text);
                }
                else if (typeof(Int32) == targetType)
                {
                    targetValue = XmlConvert.ToInt32(text);
                }
                else if (typeof(Int64) == targetType)
                {
                    if (TryRemoveLiteralSuffix(XmlConstants.XmlInt64LiteralSuffix, ref text))
                    {
                        targetValue = XmlConvert.ToInt64(text);
                    }
                    else
                    {
                        targetValue = default(Int64);
                        return false;
                    }
                }
                else if (typeof(Single) == targetType)
                {
                    if (TryRemoveLiteralSuffix(XmlConstants.XmlSingleLiteralSuffix, ref text))
                    {
                        targetValue = XmlConvert.ToSingle(text);
                    }
                    else
                    {
                        targetValue = default(Single);
                        return false;
                    }
                }
                else if (typeof(Double) == targetType)
                {
                    TryRemoveLiteralSuffix(XmlConstants.XmlDoubleLiteralSuffix, ref text);
                    targetValue = XmlConvert.ToDouble(text);
                }
                else if (typeof(Decimal) == targetType)
                {
                    if (TryRemoveLiteralSuffix(XmlConstants.XmlDecimalLiteralSuffix, ref text))
                    {
                        try
                        {
                            targetValue = XmlConvert.ToDecimal(text);
                        }
                        catch (FormatException)
                        {
                            // we need to support exponential format for decimals since we used to support them in V1
                            decimal result;
                            if (Decimal.TryParse(text, NumberStyles.Float, NumberFormatInfo.InvariantInfo, out result))
                            {
                                targetValue = result;
                            }
                            else
                            {
                                targetValue = default(Decimal);
                                return false;
                            }
                        }
                    }
                    else
                    {
                        targetValue = default(Decimal);
                        return false;
                    }
                }
                else
                {
                    Debug.Assert(typeof(System.Xml.Linq.XElement) == targetType, "XElement == " + targetType);
                    targetValue = System.Xml.Linq.XElement.Parse(text, System.Xml.Linq.LoadOptions.PreserveWhitespace);
                }

                return true;
            }
            catch (FormatException)
            {
                targetValue = null;
                return false;
            }
        }

        /// <summary>Converts a string to a primitive value.</summary>
        /// <param name="text">String text to convert.</param>
        /// <param name="targetType">Type to convert string to.</param>
        /// <returns>value converted to the target type.</returns>
        internal static object StringToPrimitive(string text, Type targetType)
        {
            Debug.Assert(text != null, "text != null");
            Debug.Assert(targetType != null, "targetType != null");

            object targetValue = null;
            targetType = Nullable.GetUnderlyingType(targetType) ?? targetType;

            if (typeof(String) == targetType)
            {
                targetValue = text;
            }
            else if (typeof(Boolean) == targetType)
            {
                targetValue = XmlConvert.ToBoolean(text);
            }
            else if (typeof(Byte) == targetType)
            {
                targetValue = XmlConvert.ToByte(text);
            }
            else if (typeof(byte[]) == targetType)
            {
                targetValue = Convert.FromBase64String(text);
            }
            else if (typeof(System.Data.Linq.Binary) == targetType)
            {
                targetValue = new System.Data.Linq.Binary(Convert.FromBase64String(text));
            }
            else if (typeof(SByte) == targetType)
            {
                targetValue = XmlConvert.ToSByte(text);
            }
            else if (typeof(DateTime) == targetType)
            {
                targetValue = XmlConvert.ToDateTime(text, XmlDateTimeSerializationMode.RoundtripKind);
            }
            else if (typeof(Decimal) == targetType)
            {
                targetValue = XmlConvert.ToDecimal(text);
            }
            else if (typeof(Double) == targetType)
            {
                targetValue = XmlConvert.ToDouble(text);
            }
            else if (typeof(Guid) == targetType)
            {
                targetValue = new Guid(text);
            }
            else if (typeof(Int16) == targetType)
            {
                targetValue = XmlConvert.ToInt16(text);
            }
            else if (typeof(Int32) == targetType)
            {
                targetValue = XmlConvert.ToInt32(text);
            }
            else if (typeof(Int64) == targetType)
            {
                targetValue = XmlConvert.ToInt64(text);
            }
            else if (typeof(System.Xml.Linq.XElement) == targetType)
            {
                targetValue = System.Xml.Linq.XElement.Parse(text, System.Xml.Linq.LoadOptions.PreserveWhitespace);
            }
            else
            {
                Debug.Assert(typeof(Single) == targetType, "typeof(Single) == targetType(" + targetType + ")");
                targetValue = XmlConvert.ToSingle(text);
            }

            return targetValue;
        }

        /// <summary>Removes quotes from the single-quotes text.</summary>
        /// <param name="text">Text to remove quotes from.</param>
        /// <returns>Whether quotes were successfully removed.</returns>
        internal static bool TryRemoveQuotes(ref string text)
        {
            if (text.Length < 2)
            {
                return false;
            }

            char quote = text[0];
            if (quote != '\'' || text[text.Length - 1] != quote)
            {
                return false;
            }

            string s = text.Substring(1, text.Length - 2);
            int start = 0;
            while (true)
            {
                int i = s.IndexOf(quote, start);
                if (i < 0)
                {
                    break;
                }

                s = s.Remove(i, 1);
                if (s.Length < i + 1 || s[i] != quote)
                {
                    return false;
                }

                start = i + 1;
            }

            text = s;
            return true;
        }
#endif

        /// <summary>Converts the specified value to a serializable string for XML content.</summary>
        /// <param name="value">Non-null value to convert.</param>
        /// <param name="result">The specified value converted to a serializable string for XML content. </param>
        /// <returns>boolean value indicating conversion successful conversion</returns>
        internal static bool TryXmlPrimitiveToString(object value, out string result)
        {
            Debug.Assert(value != null, "value != null");
            result = null;

            Type valueType = value.GetType();
            valueType = Nullable.GetUnderlyingType(valueType) ?? valueType;

            if (typeof(String) == valueType)
            {
                result = (string)value;
            }
            else if (typeof(Boolean) == valueType)
            {
                result = XmlConvert.ToString((bool)value);
            }
            else if (typeof(Byte) == valueType)
            {
                result = XmlConvert.ToString((byte)value);
            }
            else if (typeof(DateTime) == valueType)
            {
                result = XmlConvert.ToString((DateTime)value, XmlDateTimeSerializationMode.RoundtripKind);
            }
            else if (typeof(Decimal) == valueType)
            {
                result = XmlConvert.ToString((decimal)value);
            }
            else if (typeof(Double) == valueType)
            {
                result = XmlConvert.ToString((double)value);
            }
            else if (typeof(Guid) == valueType)
            {
                result = value.ToString();
            }
            else if (typeof(Int16) == valueType)
            {
                result = XmlConvert.ToString((Int16)value);
            }
            else if (typeof(Int32) == valueType)
            {
                result = XmlConvert.ToString((Int32)value);
            }
            else if (typeof(Int64) == valueType)
            {
                result = XmlConvert.ToString((Int64)value);
            }
            else if (typeof(SByte) == valueType)
            {
                result = XmlConvert.ToString((SByte)value);
            }
            else if (typeof(Single) == valueType)
            {
                result = XmlConvert.ToString((Single)value);
            }
            else if (typeof(byte[]) == valueType)
            {
                byte[] byteArray = (byte[])value;
                result = Convert.ToBase64String(byteArray);
            }
#if ASTORIA_SERVER
            else if (typeof(System.Data.Linq.Binary) == valueType)
            {
                return TryXmlPrimitiveToString(((System.Data.Linq.Binary)value).ToArray(), out result);
            }
#else
#if !ASTORIA_LIGHT
            else if (ClientConvert.IsBinaryValue(value))
            {
                return ClientConvert.TryKeyBinaryToString(value, out result);
            }
#endif
#endif
            else if (typeof(System.Xml.Linq.XElement) == valueType)
            {
                result = ((System.Xml.Linq.XElement)value).ToString(System.Xml.Linq.SaveOptions.None);
            }
            else
            {
                result = null;
                return false;
            }

            Debug.Assert(result != null, "result != null");
            return true;
        }

#if ASTORIA_SERVER
        /// <summary>Returns the 4 bits that correspond to the specified character.</summary>
        /// <param name="c">Character in the 0-F range to be converted.</param>
        /// <returns>The 4 bits that correspond to the specified character.</returns>
        /// <exception cref="FormatException">Thrown when 'c' is not in the '0'-'9','a'-'f' range.</exception>
        private static byte HexCharToNibble(char c)
        {
            Debug.Assert(IsCharHexDigit(c));
            switch (c)
            {
                case '0':
                    return 0;
                case '1':
                    return 1;
                case '2':
                    return 2;
                case '3':
                    return 3;
                case '4':
                    return 4;
                case '5':
                    return 5;
                case '6':
                    return 6;
                case '7':
                    return 7;
                case '8':
                    return 8;
                case '9':
                    return 9;
                case 'a':
                case 'A':
                    return 10;
                case 'b':
                case 'B':
                    return 11;
                case 'c':
                case 'C':
                    return 12;
                case 'd':
                case 'D':
                    return 13;
                case 'e':
                case 'E':
                    return 14;
                case 'f':
                case 'F':
                    return 15;
                default:
                    throw new InvalidOperationException();
            }
        }

        /// <summary>
        /// Check and strip the input <paramref name="text"/> for literal <paramref name="suffix"/>
        /// </summary>
        /// <param name="text">The string to check</param>
        /// <param name="suffix">The suffix value</param>
        /// <returns>A string that has been striped of the suffix</returns>
        private static bool TryRemoveLiteralSuffix(string suffix, ref string text)
        {
            Debug.Assert(text != null, "text != null");
            Debug.Assert(suffix != null, "suffix != null");

            text = text.Trim(XmlWhitespaceChars);
            if (text.Length <= suffix.Length || !text.EndsWith(suffix, StringComparison.OrdinalIgnoreCase))
            {
                return false;
            }
            else
            {
                text = text.Substring(0, text.Length - suffix.Length);
                return true;
            }
        }

        /// <summary>
        /// Tries to remove a literal <paramref name="prefix"/> from the specified <paramref name="text"/>.
        /// </summary>
        /// <param name="prefix">Prefix to remove; one-letter prefixes are case-sensitive, others insensitive.</param>
        /// <param name="text">Text to attempt to remove prefix from.</param>
        /// <returns>true if the prefix was found and removed; false otherwise.</returns>
        private static bool TryRemoveLiteralPrefix(string prefix, ref string text)
        {
            Debug.Assert(prefix != null, "prefix != null");
            if (text.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                text = text.Remove(0, prefix.Length);
                return true;
            }
            else
            {
                return false;
            }
        }
#else
        /// <summary>Appends the decimal marker to string form of double value if necessary.</summary>
        /// <param name="input">Input string.</param>
        /// <returns>String with decimal marker optionally added.</returns>
        private static string AppendDecimalMarkerToDouble(string input)
        {
            foreach (char c in input)
            {
                if (!Char.IsDigit(c))
                {
                    return input;
                }
            }

            return input + ".0";
        }
#endif
    }
}
