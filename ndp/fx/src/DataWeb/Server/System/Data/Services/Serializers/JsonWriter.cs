//---------------------------------------------------------------------
// <copyright file="JsonWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a writer implementaion for Json format
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Xml;

    /// <summary>
    /// Json text writer
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1001:TypesThatOwnDisposableFieldsShouldBeDisposable", Justification = "Original writer is disposed of?")]
    internal sealed class JsonWriter
    {
        /// <summary> const tick value for caculating tick values</summary>
        internal static readonly long DatetimeMinTimeTicks = (new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc)).Ticks;
        
        /// <summary> Json datetime format </summary>
        private const string JsonDateTimeFormat = @"\/Date({0})\/";

        /// <summary>Text used to start a data object wrapper in JSON.</summary>
        private const string JsonDataWrapper = "\"d\" : ";

        /// <summary>"results" header for Json data array</summary>
        private const string JsonResultName = "results";

        /// <summary> Writer to write text into </summary>
        private readonly IndentedTextWriter writer;

        /// <summary> scope of the json text - object, array, etc</summary>
        private readonly Stack<Scope> scopes;

        /// <summary>
        /// Creates a new instance of Json writer
        /// </summary>
        /// <param name="writer">writer to which text needs to be written</param>
        public JsonWriter(TextWriter writer)
        {
            this.writer = new IndentedTextWriter(writer);
            this.scopes = new Stack<Scope>();
        }

        /// <summary>
        /// Various scope types for Json writer
        /// </summary>
        private enum ScopeType
        {
            /// <summary> array scope </summary>
            Array = 0,

            /// <summary> object scope</summary>
            Object = 1
        }

        /// <summary>
        /// End the current scope
        /// </summary>
        public void EndScope()
        {
            if (this.scopes.Count == 0)
            {
                throw new InvalidOperationException("No active scope to end.");
            }

            this.writer.WriteLine();
            this.writer.Indent--;

            Scope scope = this.scopes.Pop();
            if (scope.Type == ScopeType.Array)
            {
                this.writer.Write("]");
            }
            else
            {
                this.writer.Write("}");
            }
        }

        /// <summary>
        /// Start the array scope
        /// </summary>
        public void StartArrayScope()
        {
            this.StartScope(ScopeType.Array);
        }

        /// <summary>
        /// Write the "d" wrapper text
        /// </summary>
        public void WriteDataWrapper()
        {
            this.writer.Write(JsonDataWrapper);
        }

        /// <summary>
        /// Write the "results" header for the data array
        /// </summary>
        public void WriteDataArrayName()
        {
            this.WriteName(JsonResultName);
        }

        /// <summary>
        /// Start the object scope
        /// </summary>
        public void StartObjectScope()
        {
            this.StartScope(ScopeType.Object);
        }

        /// <summary>
        /// Write the name for the object property
        /// </summary>
        /// <param name="name">name of the object property </param>
        public void WriteName(string name)
        {
            if (String.IsNullOrEmpty(name))
            {
                throw new ArgumentNullException("name");
            }

            if (this.scopes.Count == 0)
            {
                throw new InvalidOperationException("No active scope to write into.");
            }

            if (this.scopes.Peek().Type != ScopeType.Object)
            {
                throw new InvalidOperationException("Names can only be written into Object Scopes.");
            }

            Scope currentScope = this.scopes.Peek();
            if (currentScope.Type == ScopeType.Object)
            {
                if (currentScope.ObjectCount != 0)
                {
                    this.writer.WriteTrimmed(", ");
                }

                currentScope.ObjectCount++;
            }

            this.WriteCore(QuoteJScriptString(name), true /*quotes*/);
            this.writer.WriteTrimmed(": ");
        }

        /// <summary>
        /// Write the bool value
        /// </summary>
        /// <param name="value">bool value to be written</param>
        public void WriteValue(bool value)
        {
            this.WriteCore(value ? XmlConstants.XmlTrueLiteral : XmlConstants.XmlFalseLiteral, /* quotes */ false);
        }

        /// <summary>
        /// Write the int value
        /// </summary>
        /// <param name="value">int value to be written</param>
        public void WriteValue(int value)
        {
            this.WriteCore(value.ToString(CultureInfo.InvariantCulture), /* quotes */ false);
        }

        /// <summary>
        /// Write the float value
        /// </summary>
        /// <param name="value">float value to be written</param>
        public void WriteValue(float value)
        {
            if (double.IsInfinity(value) || double.IsNaN(value))
            {
                this.WriteCore(value.ToString(null, CultureInfo.InvariantCulture), true /*quotes*/);
            }
            else
            {
                // float.ToString() supports a max scale of six,
                // whereas float.MinValue and float.MaxValue have 8 digits scale. Hence we need
                // to use XmlConvert in all other cases, except infinity
                this.WriteCore(XmlConvert.ToString(value), /* quotes */ false);
            }
        }

        /// <summary>
        /// Write the short value
        /// </summary>
        /// <param name="value">short value to be written</param>
        public void WriteValue(short value)
        {
            this.WriteCore(value.ToString(CultureInfo.InvariantCulture), /* quotes */ false);
        }

        /// <summary>
        /// Write the long value
        /// </summary>
        /// <param name="value">long value to be written</param>
        public void WriteValue(long value)
        {
            // Since Json only supports number, we need to convert long into string to prevent data loss
            this.WriteCore(value.ToString(CultureInfo.InvariantCulture), /* quotes */ true);
        }

        /// <summary>
        /// Write the double value
        /// </summary>
        /// <param name="value">double value to be written</param>
        public void WriteValue(double value)
        {
            if (double.IsInfinity(value) || double.IsNaN(value))
            {
                this.WriteCore(value.ToString(null, CultureInfo.InvariantCulture), true /*quotes*/);
            }
            else
            {
                // double.ToString() supports a max scale of 14,
                // whereas float.MinValue and float.MaxValue have 16 digits scale. Hence we need
                // to use XmlConvert in all other cases, except infinity
                this.WriteCore(XmlConvert.ToString(value), /* quotes */ false);
            }
        }

        /// <summary>
        /// Write the Guid value
        /// </summary>
        /// <param name="value">double value to be written</param>
        public void WriteValue(Guid value)
        {
            this.WriteCore(value.ToString(), /* quotes */ true);
        }

        /// <summary>
        /// Write the decimal value
        /// </summary>
        /// <param name="value">decimal value to be written</param>
        public void WriteValue(decimal value)
        {
            // Since Json doesn't have decimal support (it only has one data type - number),
            // we need to convert decimal to string to prevent data loss
            this.WriteCore(value.ToString(CultureInfo.InvariantCulture), /* quotes */ true);
        }

        /// <summary>
        /// Write the DateTime value
        /// </summary>
        /// <param name="dateTime">dateTime value to be written</param>
        public void WriteValue(DateTime dateTime)
        {
            // taken from the Atlas serializer
            // DevDiv 41127: Never confuse atlas serialized strings with dates
            // Serialized date: "\/Date(123)\/"
            // sb.Append(@"""\/Date(");
            // sb.Append((datetime.ToUniversalTime().Ticks - DatetimeMinTimeTicks) / 10000);
            // sb.Append(@")\/""");
            switch (dateTime.Kind)
            {
                case DateTimeKind.Local:
                    dateTime = dateTime.ToUniversalTime();
                    break;
                case DateTimeKind.Unspecified:
                    dateTime = new DateTime(dateTime.Ticks, DateTimeKind.Utc);
                    break;
                case DateTimeKind.Utc:
                    break;
            }

            System.Diagnostics.Debug.Assert(dateTime.Kind == DateTimeKind.Utc, "dateTime.Kind == DateTimeKind.Utc");
            this.WriteCore(
                String.Format(
                    CultureInfo.InvariantCulture,
                    JsonWriter.JsonDateTimeFormat, 
                    ((dateTime.Ticks - DatetimeMinTimeTicks) / 10000)),
                true);
        }

        /// <summary>
        /// Write the byte value
        /// </summary>
        /// <param name="value">byte value to be written</param>
        public void WriteValue(byte value)
        {
            this.WriteCore(value.ToString(CultureInfo.InvariantCulture), /* quotes */ false);
        }

        /// <summary>
        /// Write the sbyte value
        /// </summary>
        /// <param name="value">sbyte value to be written</param>
        public void WriteValue(sbyte value)
        {
            this.WriteCore(value.ToString(CultureInfo.InvariantCulture), /* quotes */ false);
        }

        /// <summary>
        /// Write the string value
        /// </summary>
        /// <param name="s">string value to be written</param>
        public void WriteValue(string s)
        {
            if (s == null)
            {
                this.WriteCore("null", /* quotes */ false);
            }
            else
            {
                this.WriteCore(QuoteJScriptString(s), /* quotes */ true);
            }
        }

        /// <summary>
        /// Clears all buffers for the current writer
        /// </summary>
        public void Flush()
        {
            this.writer.Flush();
        }

        /// <summary>
        /// Returns the string value with special characters escaped
        /// </summary>
        /// <param name="s">input string value</param>
        /// <returns>Returns the string value with special characters escaped.</returns>
        private static string QuoteJScriptString(string s)
        {
            if (String.IsNullOrEmpty(s))
            {
                return String.Empty;
            }

            StringBuilder b = null;
            int startIndex = 0;
            int count = 0;
            for (int i = 0; i < s.Length; i++)
            {
                char c = s[i];

                // Append the unhandled characters (that do not require special treament)
                // to the string builder when special characters are detected.
                if (c == '\r' || c == '\t' || c == '\"' ||
                    c == '\\' || c == '\n' || c < ' ' || c > 0x7F || c == '\b' || c == '\f')
                {
                    // Flush out the unescaped characters we've built so far.
                    if (b == null)
                    {
                        b = new StringBuilder(s.Length + 6);
                    }

                    if (count > 0)
                    {
                        b.Append(s, startIndex, count);
                    }

                    startIndex = i + 1;
                    count = 0;
                }

                switch (c)
                {
                    case '\r':
                        b.Append("\\r");
                        break;
                    case '\t':
                        b.Append("\\t");
                        break;
                    case '\"':
                        b.Append("\\\"");
                        break;
                    case '\\':
                        b.Append("\\\\");
                        break;
                    case '\n':
                        b.Append("\\n");
                        break;
                    case '\b':
                        b.Append("\\b");
                        break;
                    case '\f':
                        b.Append("\\f");
                        break;
                    default:
                        if ((c < ' ') || (c > 0x7F))
                        {
                            b.AppendFormat(CultureInfo.InvariantCulture, "\\u{0:x4}", (int)c);
                        }
                        else
                        {
                            count++;
                        }

                        break;
                }
            }

            string processedString = s;
            if (b != null)
            {
                if (count > 0)
                {
                    b.Append(s, startIndex, count);
                }

                processedString = b.ToString();
            }

            return processedString;
        }

        /// <summary>
        /// Write the string value with/without quotes
        /// </summary>
        /// <param name="text">string value to be written</param>
        /// <param name="quotes">put quotes around the value if this value is true</param>
        private void WriteCore(string text, bool quotes)
        {
            if (this.scopes.Count != 0)
            {
                Scope currentScope = this.scopes.Peek();
                if (currentScope.Type == ScopeType.Array)
                {
                    if (currentScope.ObjectCount != 0)
                    {
                        this.writer.WriteTrimmed(", ");
                    }

                    currentScope.ObjectCount++;
                }
            }

            if (quotes)
            {
                this.writer.Write('"');
            }

            this.writer.Write(text);
            if (quotes)
            {
                this.writer.Write('"');
            }
        }

        /// <summary>
        /// Start the scope given the scope type
        /// </summary>
        /// <param name="type">scope type</param>
        private void StartScope(ScopeType type)
        {
            if (this.scopes.Count != 0)
            {
                Scope currentScope = this.scopes.Peek();
                if ((currentScope.Type == ScopeType.Array) &&
                    (currentScope.ObjectCount != 0))
                {
                    this.writer.WriteTrimmed(", ");
                }

                currentScope.ObjectCount++;
            }

            Scope scope = new Scope(type);
            this.scopes.Push(scope);

            if (type == ScopeType.Array)
            {
                this.writer.Write("[");
            }
            else
            {
                this.writer.Write("{");
            }

            this.writer.Indent++;
            this.writer.WriteLine();
        }

        /// <summary>
        /// class representing scope information
        /// </summary>
        private sealed class Scope
        {
            /// <summary> keeps the count of the nested scopes </summary>
            private int objectCount;

            /// <summary> keeps the type of the scope </summary>
            private ScopeType type;

            /// <summary>
            /// Creates a new instance of scope type
            /// </summary>
            /// <param name="type">type of the scope</param>
            public Scope(ScopeType type)
            {
                this.type = type;
            }

            /// <summary>
            /// Get/Set the object count for this scope
            /// </summary>
            public int ObjectCount
            {
                get
                {
                    return this.objectCount;
                }

                set
                {
                    this.objectCount = value;
                }
            }

            /// <summary>
            /// Gets the scope type for this scope
            /// </summary>
            public ScopeType Type
            {
                get
                {
                    return this.type;
                }
            }
        }
    }
}
