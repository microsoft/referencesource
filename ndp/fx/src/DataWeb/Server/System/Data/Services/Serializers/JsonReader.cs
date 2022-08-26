//---------------------------------------------------------------------
// <copyright file="JsonReader.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a reader implementaion for Json format
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;
using System.Collections.Generic;

    #endregion Namespaces.

    /// <summary>Json text reader.</summary>
    /// <remarks>Does not dispose the reader, since we don't own the underlying stream.</remarks>
    internal sealed class JsonReader
    {
        /// <summary>Compiled Regex for DateTime Format.</summary>
        private static readonly Regex DateTimeFormat = new Regex(@"^/Date\((?<ticks>-?[0-9]+)\)/", RegexOptions.Compiled);

        /// <summary>Maximum recursion limit on reader.</summary>
        private const int RecursionLimit = 200;

        /// <summary> Reader to reader text into </summary>
        private readonly StreamReader reader;

        /// <summary>Depth of recursion.</summary>
        private int recursionDepth;

        /// <summary>
        /// Creates a new instance of Json reader which readers the json text 
        /// from the given reader
        /// </summary>
        /// <param name="reader">text reader from which json payload needs to be read.
        /// Does not dispose the reader, since we don't own the underlying stream.</param>
        public JsonReader(StreamReader reader)
        {
            Debug.Assert(reader != null, "reader != null");
            this.reader = reader;
        }

        /// <summary>
        /// Converts the given value into the right type
        /// </summary>
        /// <returns>returns the clr object instance which </returns>
        public object ReadValue()
        {
            this.RecurseEnter();

            object value = null;
            bool allowNull = false;

            char ch = this.PeekNextSignificantCharacter();
            if (ch == '[')
            {
                value = this.ReadArray();
            }
            else if (ch == '{')
            {
                value = this.ReadObject();
            }
            else if ((ch == '\'') || (ch == '"'))
            {
                bool hasLeadingSlash;
                string s = this.ReadString(out hasLeadingSlash);
                value = s; // may be overwritten with a DateTime if ends up being a date/time

                // Atlas format for date/time
                if (hasLeadingSlash)
                {
                    Match match = DateTimeFormat.Match(s);
                    if (match.Success)
                    {
                        string ticksStr = match.Groups["ticks"].Value;

                        long ticks;
                        if (long.TryParse(ticksStr, NumberStyles.Integer, NumberFormatInfo.InvariantInfo, out ticks))
                        {
                            // The javascript ticks start from 1/1/1970 but FX DateTime ticks start from 1/1/0001
                            DateTime dateTime = new DateTime(ticks * 10000 + JsonWriter.DatetimeMinTimeTicks, DateTimeKind.Utc);
                            value = dateTime;
                        }
                    }
                }
            }
            else if (Char.IsDigit(ch) || (ch == '-') || (ch == '.'))
            {
                value = this.ReadNumber();
            }
            else if ((ch == 't') || (ch == 'f'))
            {
                value = this.ReadBoolean();
            }
            else if (ch == 'n')
            {
                this.ReadNull();
                allowNull = true;
            }

            if ((value == null) && (allowNull == false))
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidContent);
            }

            this.RecurseLeave();

            // if there is junk data at the end of the stream ex. {...}junk
            // then an exception will be thrown.
            if (this.recursionDepth == 0)
            {
                // at the end of the stream
                if (this.PeekNextSignificantCharacter() != '\0')
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidContent);
                }
            }

            return value;
        }

        /// <summary>
        /// Gets the next character from the reader. This function moves the enumerator by 1 position
        /// </summary>
        /// <returns>the next charater from the current reader position</returns>
        private char ReadNextCharacter()
        {
            int result = this.reader.Read();
            if (result < 0)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidContent);
            }

            Debug.Assert(result <= char.MaxValue, "result <= char.MaxValue");
            return (char)result;
        }

        /// <summary>
        /// peeks the next character from the reader. This does not move the current reader position.
        /// </summary>
        /// <returns>the next character from the current reader position</returns>
        private char PeekNextCharacter()
        {
            if (this.reader.EndOfStream)
            {
                return '\0';
            }

            int result = this.reader.Peek();
            Debug.Assert(result >= 0, "Peek must not return value < 0 since we are not at EndOfStream yet.");
            Debug.Assert(result <= char.MaxValue, "result <= char.MaxValue");
            return (char)result;
        }

        /// <summary>
        /// Returns the next count characters from the reader's current position
        /// </summary>
        /// <param name="count">number of characters to return</param>
        /// <returns>string consisting of next count characters</returns>
        private string GetCharacters(int count)
        {
            StringBuilder stringBuilder = new StringBuilder();
            for (int i = 0; i < count; i++)
            {
                char ch = this.ReadNextCharacter();
                stringBuilder.Append(ch);
            }

            return stringBuilder.ToString();
        }

        /// <summary>
        /// Sets the readers position to the next significant character position
        /// </summary>
        /// <returns>returns the next significant character without changing the current position of the reader</returns>
        private char PeekNextSignificantCharacter()
        {
            char ch = this.PeekNextCharacter();
            while ((ch != '\0') && Char.IsWhiteSpace(ch))
            {
                this.ReadNextCharacter();
                ch = this.PeekNextCharacter();
            }

            return ch;
        }

        /// <summary>
        /// Converts the given text into an arrayList
        /// </summary>
        /// <returns>returns the arraylist containing the list of objects</returns>
        private ArrayList ReadArray()
        {
            ArrayList array = new ArrayList();

            // Consume the '['
            this.ReadNextCharacter();

            while (true)
            {
                char ch = this.PeekNextSignificantCharacter();
                if (ch == '\0')
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidContent);
                }

                if (ch == ']')
                {
                    this.ReadNextCharacter();
                    return array;
                }

                if (array.Count != 0)
                {
                    if (ch != ',')
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_MissingArrayMemberSeperator);
                    }
                    else
                    {
                        this.ReadNextCharacter();
                    }
                }

                object item = this.ReadValue();
                array.Add(item);
            }
        }

        /// <summary>
        /// Reads the boolean value
        /// </summary>
        /// <returns>returns the boolean value as read from the reader</returns>
        private bool ReadBoolean()
        {
            string s = this.ReadName(/* allowQuotes */ false);

            if (s != null)
            {
                if (s.Equals(XmlConstants.XmlTrueLiteral, StringComparison.Ordinal))
                {
                    return true;
                }
                else if (s.Equals(XmlConstants.XmlFalseLiteral, StringComparison.Ordinal))
                {
                    return false;
                }
            }

            throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidKeyword(s));
        }

        /// <summary>
        /// Reads the name string from the reader
        /// </summary>
        /// <param name="allowQuotes">true, if you want to allow quotes, otherwise false</param>
        /// <returns>string name value</returns>
        private string ReadName(bool allowQuotes)
        {
            char ch = this.PeekNextSignificantCharacter();

            if ((ch == '"') || (ch == '\''))
            {
                if (allowQuotes)
                {
                    return this.ReadString();
                }
            }
            else
            {
                StringBuilder sb = new StringBuilder();

                while (true)
                {
                    ch = this.PeekNextCharacter();
                    if ((ch == '_') || Char.IsLetterOrDigit(ch) || ch == '$')
                    {
                        this.ReadNextCharacter();
                        sb.Append(ch);
                    }
                    else
                    {
                        return sb.ToString();
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Reads the null literal from the reader
        /// </summary>
        private void ReadNull()
        {
            string s = this.ReadName(/* allowQuotes */ false);

            if (s == null)
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_MissingMemberName);
            }
            else if (!s.Equals("null", StringComparison.Ordinal))
            {
                throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidKeyword(s));
            }
        }

        /// <summary>Reads the number from the reader.</summary>
        /// <returns>reads the clr number object</returns>
        private object ReadNumber()
        {
            char ch = this.ReadNextCharacter();

            StringBuilder sb = new StringBuilder();
            sb.Append(ch);
            while (true)
            {
                ch = this.PeekNextSignificantCharacter();

                if (Char.IsDigit(ch) || (ch == '.') || (ch == 'E') || (ch == 'e') || (ch == '-') || (ch == '+'))
                {
                    this.ReadNextCharacter();
                    sb.Append(ch);
                }
                else
                {
                    break;
                }
            }

            string s = sb.ToString();
            Double doubleValue;
            int intValue;

            // We will first try and convert this int32. If this succeeds, great. Otherwise, we will try
            // and convert this into a double.
            if (Int32.TryParse(s, NumberStyles.Integer, NumberFormatInfo.InvariantInfo, out intValue))
            {
                return intValue;
            }
            else if (Double.TryParse(s, NumberStyles.Float, NumberFormatInfo.InvariantInfo, out doubleValue))
            {
                return doubleValue;
            }

            throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidContent);
        }

        /// <summary>
        /// Reads the object from the reader
        /// </summary>
        /// <returns>returns hashtable containing the list of property names and values</returns>
        private JsonObjectRecords ReadObject()
        {
            JsonObjectRecords record = new JsonObjectRecords();            

            // Consume the '{'
            this.ReadNextCharacter();

            while (true)
            {
                char ch = this.PeekNextSignificantCharacter();
                if (ch == '\0')
                {
                    // Unterminated Object literal
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidContent);
                }

                if (ch == '}')
                {
                    this.ReadNextCharacter();
                    return record;
                }

                if (record.Count != 0)
                {
                    if (ch != ',')
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_MissingMemberSeperator);
                    }
                    else
                    {
                        this.ReadNextCharacter();
                    }
                }

                string name = this.ReadName(/* allowQuotes */ true);
                if (String.IsNullOrEmpty(name))
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidJsonNameSpecifiedOrExtraComma);
                }

                ch = this.PeekNextSignificantCharacter();

                // Unexpected name/value pair syntax in object literal
                if (ch != ':')
                {
                    throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_MissingNameValueSeperator(name));
                }
                else
                {
                    this.ReadNextCharacter();
                }

                object item = this.ReadValue();
                
                record.Add(name, item);
            }
        }

        /// <summary>
        /// Read the string value from the reader
        /// </summary>
        /// <returns>returns the string value read from the reader</returns>
        private string ReadString()
        {
            bool dummy;
            return this.ReadString(out dummy);
        }

        /// <summary>
        /// Read the string value from the reader
        /// </summary>
        /// <param name="hasLeadingSlash">out parameter indicating whether the string has a leading slash or not</param>
        /// <returns>returns the string value read from the reader</returns>
        private string ReadString(out bool hasLeadingSlash)
        {
            char endQuoteCharacter = this.ReadNextCharacter();
            char ch = this.ReadNextCharacter();

            hasLeadingSlash = (ch == '\\') ? true : false;
            StringBuilder sb = new StringBuilder();

            while (true)
            {
                if (ch == '\\')
                {
                    ch = this.ReadNextCharacter();
                    
                    // From 4627, section 2.5: Strings, here's the list of characters that we should be escaping
                    if (ch == 'u')
                    {
                        string unicodeSequence = this.GetCharacters(4);
                        Debug.Assert(unicodeSequence != null, "unicodeSequence != null");
                        ch = (char)Int32.Parse(unicodeSequence, NumberStyles.HexNumber, CultureInfo.InvariantCulture);
                        sb.Append(ch);
                    }
                    else if (ch == 'b')
                    {
                        sb.Append('\b');
                    }
                    else if (ch == 'f')
                    {
                        sb.Append('\f');
                    }
                    else if (ch == 'n')
                    {
                        sb.Append('\n');
                    }
                    else if (ch == 'r')
                    {
                        sb.Append('\r');
                    }
                    else if (ch == 't')
                    {
                        sb.Append('\t');
                    }
                    else if (ch == '\\' || ch == '\"' || ch == '/' || ch == '\'')
                    {
                        sb.Append(ch);
                    }
                    else
                    {
                        throw DataServiceException.CreateBadRequestError(Strings.BadRequestStream_InvalidJsonUnrecognizedEscapeSequence);
                    }
                }
                else
                if (ch == endQuoteCharacter)
                {
                    return sb.ToString();
                }
                else
                {
                    sb.Append(ch);
                }
            
                ch = this.ReadNextCharacter();
            }
        }

        /// <summary>Marks the fact that a recursive method was entered, and checks that the depth is allowed.</summary>
        private void RecurseEnter()
        {
            WebUtil.RecurseEnter(RecursionLimit, ref this.recursionDepth);
        }

        /// <summary>Marks the fact that a recursive method is leaving..</summary>
        private void RecurseLeave()
        {
            WebUtil.RecurseLeave(ref this.recursionDepth);
        }

        /// <summary>
        /// Represent one Json Object. 
        /// </summary>
        /// <remarks>This class honors the original ordering of the inner elements of the json object.</remarks>
        public class JsonObjectRecords
        {
            /// <summary>
            /// A list of keys in the object
            /// </summary>
            private List<String> orderedKeys;

            /// <summary>
            /// The actual storage of key-value pair
            /// </summary>
            private Dictionary<String, Object> entries;
            
            /// <summary>
            /// Constructor
            /// </summary>
            public JsonObjectRecords()
            {
                this.orderedKeys = new List<string>();
                this.entries = new Dictionary<string, object>(EqualityComparer<String>.Default);
            }

            /// <summary>
            /// Number of elements in this object
            /// </summary>
            public int Count
            {
                get { return this.orderedKeys.Count; }
            }

            /// <summary>
            /// A list of keys in the object, in the order which they are deserialized
            /// </summary>
            public List<String> OrderedKeys
            {
                get { return this.orderedKeys; }
            }

            /// <summary>
            /// The actual storage of key-value pair
            /// </summary>
            public Dictionary<String, Object> Entries
            {
                get { return this.entries; }
            }

            /// <summary>
            /// Add a new element into the object
            /// </summary>
            /// <param name="key">Key</param>
            /// <param name="value">Value</param>
            public void Add(String key, Object value)
            {
                // Json Object used to be a hashtable
                // It will automatically resolve duplicated keys
                // however, for that scenario, we must not update the order table
                if (!this.entries.ContainsKey(key))
                {
                    this.orderedKeys.Add(key);
                }

                this.entries[key] = value;
            }
        }
    }
}
