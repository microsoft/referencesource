//---------------------------------------------------------------------
// <copyright file="IndentedTextWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a writer implementation for Json format
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Text;

    /// <summary>Writes the Json text in indented format.</summary>
    /// <remarks>
    /// There are many more methods implemented in previous versions
    /// of this file to handle more type and newline cases.
    /// </remarks>
    internal sealed class IndentedTextWriter : TextWriter
    {
        /// <summary> writer to which Json text needs to be written</summary>
        private TextWriter writer;

        /// <summary> keeps track of the indentLevel</summary>
        private int indentLevel;

        /// <summary> keeps track of pending tabs</summary>
        private bool tabsPending;

        /// <summary> string representation of tab</summary>
        private string tabString;

        /// <summary>
        /// Creates a new instance of IndentedTextWriter over the given text writer
        /// </summary>
        /// <param name="writer">writer which IndentedTextWriter wraps</param>
        public IndentedTextWriter(TextWriter writer) : base(CultureInfo.InvariantCulture)
        {
            this.writer = writer;
            this.tabString = "    ";
        }

        /// <summary> Returns the Encoding for the given writer </summary>
        public override Encoding Encoding
        {
            get
            {
                return this.writer.Encoding;
            }
        }

        /// <summary> Returns the new line character </summary>
        public override string NewLine
        {
            get
            {
                return this.writer.NewLine;
            }
        }

        /// <summary> returns the current indent level </summary>
        public int Indent
        {
            get
            {
                return this.indentLevel;
            }

            set
            {
                Debug.Assert(value >= 0, "value >= 0");
                if (value < 0)
                {
                    value = 0;
                }

                this.indentLevel = value;
            }
        }

        /// <summary> Closes the underlying writer</summary>
        public override void Close()
        {
            // This is done to make sure we don't accidently close the underlying stream.
            // Since we don't own the stream, we should never close it.
            throw new NotImplementedException();
        }

        /// <summary> Clears all the buffer of the current writer </summary>
        public override void Flush()
        {
            this.writer.Flush();
        }

        /// <summary>
        /// Writes the given string value to the underlying writer
        /// </summary>
        /// <param name="s">string value to be written</param>
        public override void Write(string s)
        {
            this.OutputTabs();
            this.writer.Write(s);
        }

        /// <summary>
        /// Writes the given char value to the underlying writer 
        /// </summary>
        /// <param name="value">char value to be written</param>
        public override void Write(char value)
        {
            this.OutputTabs();
            this.writer.Write(value);
        }

        /// <summary>
        /// Writes the trimmed text if minimizeWhiteSpeace is set to true
        /// </summary>
        /// <param name="text">string value to be written</param>
        public void WriteTrimmed(string text)
        {
            this.Write(text);
        }

        /// <summary> Writes the tabs depending on the indent level </summary>
        private void OutputTabs()
        {
            if (this.tabsPending)
            {
                for (int i = 0; i < this.indentLevel; i++)
                {
                    this.writer.Write(this.tabString);
                }

                this.tabsPending = false;
            }
        }
    }
}
