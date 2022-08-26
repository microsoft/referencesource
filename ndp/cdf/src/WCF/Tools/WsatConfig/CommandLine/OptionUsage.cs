//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Diagnostics;
    using System.Globalization;

    static class OptionUsage
    {
        public static void Print()
        {
            Console.WriteLine();
            Console.WriteLine(SR.GetString(SR.ConsoleUsageLine01));
            Console.WriteLine(SR.GetString(SR.ConsoleUsageLine02));

            Console.WriteLine(SR.GetString(SR.ConsoleUsageLineOptions));

            OptionHelpGenerator.WriteNetworkUsage();
            Console.WriteLine();

            OptionHelpGenerator.WritePortUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteEndpointCertsUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteAccountsUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteAccountsCertsUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteClusterVirtualServerUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteTimeoutUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteMaxTimeoutUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteTraceLevelUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteTraceActivityUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteTracePropUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteTracePiiUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteShowUsage();
            Console.WriteLine();

            OptionHelpGenerator.WriteRestartUsage();
        }

        static class OptionHelpGenerator
        {
            static OptionHelpGenerator() { } // beforefeildInit

            const int IntentLength = 36;
            const string optionHelpPattern = "{0,33} - {1}{2}";
            const string optionOnlyPattern = "{0,33}   ";

            static ToolStringBuilder builder = new ToolStringBuilder(IntentLength);

            internal static void WriteNetworkUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.Network, string.Empty, SR.GetString(SR.ConsoleUsageLineNetworkUse), SR.GetString(SR.ConsoleUsageLineNetworkHelp)));
            }

            internal static void WritePortUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.Port, string.Empty, SR.GetString(SR.ConsoleUsageLinePortUse), SR.GetString(SR.ConsoleUsageLinePortHelp)));
            }

            internal static void WriteEndpointCertsUsage()
            {
                List<string> optionUses = new List<string>();
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineEndpointCertsUse1));
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineEndpointCertsUse2));

                WriteMultiUsesUsage(CommandLineOption.EndpointCert, optionUses, SR.GetString(SR.ConsoleUsageLineEndpointCertsHelp));
            }

            internal static void WriteAccountsCertsUsage()
            {
                List<string> optionUses = new List<string>();
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineAccountsCertsUse1));
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineAccountsCertsUse2));

                WriteMultiUsesUsage(CommandLineOption.AccountsCerts, optionUses, SR.GetString(SR.ConsoleUsageLineAccountsCertsHelp));
            }

            internal static void WriteClusterVirtualServerUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.ClusterVirtualServer, string.Empty, SR.GetString(SR.ConsoleUsageLineClusterVirtualServerUse), SR.GetString(SR.ConsoleUsageLineClusterVirtualServerHelp)));
            }

            internal static void WriteTraceLevelUsage()
            {
                List<string> optionUses = new List<string>();
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineTraceLevelUse1));
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineTraceLevelUse2));
                optionUses.Add(SR.GetString(SR.ConsoleUsageLineTraceLevelUse3));

                WriteMultiUsesUsage(CommandLineOption.TraceLevel, optionUses, SR.GetString(SR.ConsoleUsageLineTraceLevelHelp));
            }

            internal static void WriteAccountsUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.Accounts, string.Empty, SR.GetString(SR.ConsoleUsageLineAccountsUse), SR.GetString(SR.ConsoleUsageLineAccountsHelp)));
            }

            internal static void WriteTimeoutUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.DefaultTimeout, string.Empty, SR.GetString(SR.ConsoleUsageLineTimeoutUse), SR.GetString(SR.ConsoleUsageLineTimeoutHelp)));
            }

            internal static void WriteMaxTimeoutUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.MaxTimeout, string.Empty, SR.GetString(SR.ConsoleUsageLineMaxTimeoutUse), SR.GetString(SR.ConsoleUsageLineMaxTimeoutHelp)));
            }

            internal static void WriteTraceActivityUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.TraceActivity, string.Empty, SR.GetString(SR.ConsoleUsageLineTraceActivityUse), SR.GetString(SR.ConsoleUsageLineTraceActivityHelp)));
            }

            internal static void WriteTracePropUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.TraceProp, string.Empty, SR.GetString(SR.ConsoleUsageLineTracePropUse), SR.GetString(SR.ConsoleUsageLineTracePropHelp)));
            }

            internal static void WriteTracePiiUsage()
            {
                builder.WriteParagraph(GenerateParameterHelp(CommandLineOption.TracePii, string.Empty, SR.GetString(SR.ConsoleUsageLineTracePIIUse), SR.GetString(SR.ConsoleUsageLineTracePIIHelp)));
            }

            internal static void WriteShowUsage()
            {
                builder.WriteParagraph(GenerateFlagHelp(CommandLineOption.Show, string.Empty, SR.GetString(SR.ConsoleUsageLineShowHelp)));
            }

            internal static void WriteRestartUsage()
            {
                builder.WriteParagraph(GenerateFlagHelp(CommandLineOption.Restart, string.Empty, SR.GetString(SR.ConsoleUsageLineRestartHelp)));
            }

            static string GetShortForm(string optionAbbr)
            {
                Debug.Assert(string.IsNullOrEmpty(optionAbbr));
                    
                return string.Empty;
            }

            static string GenerateParameterHelp(string option, string optionAbbr, string optionUse, string helpText)
            {
                string optionString = String.Format(CultureInfo.InvariantCulture, "-{0}:{1}", option, optionUse);
                return GenerateOptionHelp(optionString, optionAbbr, helpText);
            }

            static string GenerateFlagHelp(string option, string optionAbbr, string helpText)
            {
                string optionString = String.Format(CultureInfo.InvariantCulture, "-{0}", option);
                return GenerateOptionHelp(optionString, optionAbbr, helpText);
            }

            static string GenerateOptionHelp(string optionString, string optionAbbr, string helpText)
            {
                string shortForm = GetShortForm(optionAbbr);
                return string.Format(CultureInfo.InvariantCulture, optionHelpPattern, optionString, helpText, shortForm);
            }

            static void WriteMultiUsesUsage(string option, List<string> optionUses, string helpText)
            {
                Debug.Assert(optionUses.Count > 1);
                string majorText = GenerateParameterHelp(option, string.Empty, optionUses[0], helpText);

                builder.WriteSingleLine(ref majorText);
                for (int i = 1; i < optionUses.Count; ++i)
                {
                    Console.Write(string.Format(CultureInfo.InvariantCulture, optionOnlyPattern, optionUses[i]));
                    if (string.IsNullOrEmpty(majorText))
                    {
                        Console.WriteLine();
                    }
                    else
                    {
                        if (i == optionUses.Count - 1)
                        {
                            builder.WriteParagraph(majorText);
                        }
                        else
                        {
                            builder.WriteSingleLine(ref majorText);
                        }
                    }
                }
            }
        }

        class ToolStringBuilder
        {

            int indentLength;
            int cursorLeft;
            int lineWidth;
            StringBuilder stringBuilder;

            public ToolStringBuilder(int indentLength)
            {
                this.indentLength = indentLength;
            }

            void Reset()
            {
                this.stringBuilder = new StringBuilder();
                this.cursorLeft = GetConsoleCursorLeft();
                this.lineWidth = GetBufferWidth();
            }

            public void WriteParagraph(string text)
            {
                this.Reset();
                this.AppendParagraph(text);
                WriteLine(this.stringBuilder.ToString());
                this.stringBuilder = null;
            }

            public void WriteSingleLine(ref string text)
            {
                this.Reset();
                this.AppendLineText(ref text);
                WriteLine(this.stringBuilder.ToString());
                this.stringBuilder = null;
            }

            void WriteLine(string lineText)
            {
                if (cursorLeft == lineWidth)
                {
                    Console.Write(lineText);
                }
                else
                {
                    Console.WriteLine(lineText);
                }
            }

            void AppendParagraph(string text)
            {
                Debug.Assert(this.stringBuilder != null, "stringBuilder cannot be null");

                if (indentLength >= lineWidth)
                {
                    stringBuilder.AppendLine(text);
                }
                else
                {
                    int index = 0;
                    while (index < text.Length)
                    {
                        this.AppendWord(text, ref index);
                        this.AppendWhitespace(text, ref index);
                    }
                }
            }

            void AppendLineText(ref string text)
            {

                if (indentLength >= lineWidth)
                {
                    stringBuilder.AppendLine(text);
                    text = string.Empty;
                }
                else
                {
                    int index = 0;
                    while (index < text.Length)
                    {
                        int oldIndex = index;
                        this.AppendWord(text, ref index, true);
                        this.AppendWhitespace(text, ref index);
                        if (oldIndex == index)
                        {
                            break;
                        }
                    }
                    text = text.Substring(index);
                }
            }

            void AppendWord(string text, ref int index)
            {
                AppendWord(text, ref index, false);
            }

            void AppendWord(string text, ref int index, bool onlyWithinCurrentLine)
            {
                // If we're at the beginning of a new line we should indent.
                if ((this.cursorLeft == 0) && (index != 0))
                    AppendIndent();

                int wordLength = FindWordLength(text, index);

                // Now that we know how long the string is we can:
                //   1. print it on the current line if we have enough space
                //   2. print it on the next line if we don't have space 
                //      on the current line and it will fit on the next line
                //   3. print whatever will fit on the current line 
                //      and overflow to the next line.
                if (wordLength < this.HangingLineWidth)
                {
                    if (wordLength > this.BufferWidth)
                    {
                        if (onlyWithinCurrentLine)
                        {
                            return;
                        }
                        else
                        {
                            this.AppendLineBreak();
                            this.AppendIndent();
                        }
                    }
                    this.stringBuilder.Append(text, index, wordLength);
                    this.cursorLeft += wordLength;
                }
                else
                {
                    AppendWithOverflow(text, ref index, ref wordLength);
                }

                index += wordLength;
            }

            void AppendWithOverflow(string test, ref int start, ref int wordLength)
            {
                do
                {
                    this.stringBuilder.Append(test, start, this.BufferWidth);
                    start += this.BufferWidth;
                    wordLength -= this.BufferWidth;
                    this.AppendLineBreak();

                    if (wordLength > 0)
                        this.AppendIndent();

                } while (wordLength > this.BufferWidth);

                if (wordLength > 0)
                {
                    this.stringBuilder.Append(test, start, wordLength);
                    this.cursorLeft += wordLength;
                }
            }

            void AppendWhitespace(string text, ref int index)
            {
                while ((index < text.Length) && char.IsWhiteSpace(text[index]))
                {
                    if (BufferWidth == 0)
                    {
                        this.AppendLineBreak();
                    }

                    // For each whitespace character:
                    //   1. If we're at a newline character we insert 
                    //      a new line and reset the cursor.
                    //   2. If the whitespace character is at the beginning of a new 
                    //      line, we insert an indent instead of the whitespace
                    //   3. Insert the whitespace 
                    if (AtNewLine(text, index))
                    {
                        this.AppendLineBreak();
                        index += Environment.NewLine.Length;
                    }
                    else if (this.cursorLeft == 0 && index != 0)
                    {
                        AppendIndent();
                        index++;
                    }
                    else
                    {
                        this.stringBuilder.Append(text[index]);
                        index++;
                        cursorLeft++;
                    }
                }
            }

            void AppendIndent()
            {
                this.stringBuilder.Append(' ', this.indentLength);
                this.cursorLeft += this.indentLength;
            }

            void AppendLineBreak()
            {
                if (BufferWidth != 0)
                    this.stringBuilder.AppendLine();
                this.cursorLeft = 0;
            }

            int BufferWidth
            {
                get
                {
                    return this.lineWidth - this.cursorLeft;
                }
            }

            int HangingLineWidth
            {
                get
                {
                    return this.lineWidth - this.indentLength;
                }
            }

            static int FindWordLength(string text, int index)
            {
                for (int end = index; end < text.Length; end++)
                {
                    if (char.IsWhiteSpace(text[end]))
                        return end - index;
                }
                return text.Length - index;
            }

            static bool AtNewLine(string text, int index)
            {

                if ((index + Environment.NewLine.Length) > text.Length)
                {
                    return false;
                }

                for (int i = 0; i < Environment.NewLine.Length; i++)
                {
                    if (Environment.NewLine[i] != text[index + i])
                    {
                        return false;
                    }
                }

                return true;
            }

            static int GetConsoleCursorLeft()
            {
                try
                {
                    return Console.CursorLeft;
                }
#pragma warning suppress 56500
                catch
                {
                    return 0;
                }
            }

            static int GetBufferWidth()
            {
                try
                {
                    bool junk = Console.CursorVisible;
                    return Console.BufferWidth;
                }
#pragma warning suppress 56500
                catch
                {
                    return int.MaxValue;
                }
            }
        }
    }
}
