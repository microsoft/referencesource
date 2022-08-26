//---------------------------------------------------------------------
// <copyright file="ExpressionLexer.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a type to tokenize text.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Parsing
{
    using System;
    using System.Diagnostics;
    using System.Text;

    /// <summary>Use this class to parse an expression in the Astoria URI format.</summary>
    /// <remarks>
    /// Literals (non-normative "handy" reference - see spec for correct expression):
    /// Null        null
    /// Boolean     true | false
    /// Int32       (digit+)
    /// Int64       (digit+)(L|l)
    /// Decimal     (digit+ ['.' digit+])(M|m)
    /// Float       (digit+ ['.' digit+][e|E [+|-] digit+)(f|F)
    /// Double      (digit+ ['.' digit+][e|E [+|-] digit+)
    /// String      "'" .* "'"
    /// DateTime    datetime"'"dddd-dd-dd[T|' ']dd:mm[ss[.fffffff]]"'"
    /// Binary      (binary|X)'digit*'
    /// GUID        guid'digit*'
    /// </remarks>
    [DebuggerDisplay("ExpressionLexer ({text} @ {textPos} [{token}]")]
    internal class ExpressionLexer
    {
        #region Private fields.

        /// <summary>Suffix for single literals.</summary>
        private const char SingleSuffixLower = 'f';

        /// <summary>Suffix for single literals.</summary>
        private const char SingleSuffixUpper = 'F';

        /// <summary>Text being parsed.</summary>
        private readonly string text;

        /// <summary>Length of text being parsed.</summary>
        private readonly int textLen;

        /// <summary>Position on text being parsed.</summary>
        private int textPos;

        /// <summary>Character being processed.</summary>
        private char ch;

        /// <summary>Token being processed.</summary>
        private Token token;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="ExpressionLexer"/>.</summary>
        /// <param name="expression">Expression to parse.</param>
        internal ExpressionLexer(string expression)
        {
            Debug.Assert(expression != null, "expression != null");

            this.text = expression;
            this.textLen = this.text.Length;
            this.SetTextPos(0);
            this.NextToken();
        }

        #endregion Constructors.

        #region Internal properties.

        /// <summary>Token being processed.</summary>
        internal Token CurrentToken
        {
            get { return this.token; }
            set { this.token = value; }
        }

        /// <summary>Text being parsed.</summary>
        internal string ExpressionText
        {
            get { return this.text; }
        }

        /// <summary>Position on text being parsed.</summary>
        internal int Position
        {
            get { return this.token.Position; }
        }

        #endregion Internal properties.

        #region Internal methods.

        /// <summary>Whether the specified token identifier is a numeric literal.</summary>
        /// <param name="id">Token to check.</param>
        /// <returns>true if it's a numeric literal; false otherwise.</returns>
        internal static bool IsNumeric(TokenId id)
        {
            return 
                id == TokenId.IntegerLiteral || id == TokenId.DecimalLiteral ||
                id == TokenId.DoubleLiteral || id == TokenId.Int64Literal ||
                id == TokenId.SingleLiteral;
        }

        /// <summary>Reads the next token, skipping whitespace as necessary.</summary>
        internal void NextToken()
        {
            while (Char.IsWhiteSpace(this.ch))
            {
                this.NextChar();
            }

            TokenId t;
            int tokenPos = this.textPos;
            switch (this.ch)
            {
                case '(':
                    this.NextChar();
                    t = TokenId.OpenParen;
                    break;
                case ')':
                    this.NextChar();
                    t = TokenId.CloseParen;
                    break;
                case ',':
                    this.NextChar();
                    t = TokenId.Comma;
                    break;
                case '-':
                    bool hasNext = this.textPos + 1 < this.textLen;
                    if (hasNext && Char.IsDigit(this.text[this.textPos + 1]))
                    {
                        this.NextChar();
                        t = this.ParseFromDigit();
                        if (IsNumeric(t))
                        {
                            break;
                        }
                        
                        // If it looked like a numeric but wasn't (because it was a binary 0x... value for example), 
                        // we'll rewind and fall through to a simple '-' token.
                        this.SetTextPos(tokenPos);
                    }
                    else if (hasNext && this.text[tokenPos + 1] == XmlConstants.XmlInfinityLiteral[0])
                    {
                        this.NextChar();
                        this.ParseIdentifier();
                        string currentIdentifier = this.text.Substring(tokenPos + 1, this.textPos - tokenPos - 1);

                        if (IsInfinityLiteralDouble(currentIdentifier))
                        {
                            t = TokenId.DoubleLiteral;
                            break;
                        }
                        else if (IsInfinityLiteralSingle(currentIdentifier))
                        {
                            t = TokenId.SingleLiteral;
                            break;
                        }

                        // If it looked like '-INF' but wasn't we'll rewind and fall through to a simple '-' token.
                        this.SetTextPos(tokenPos);
                    }

                    this.NextChar();
                    t = TokenId.Minus;
                    break;
                case '=':
                    this.NextChar();
                    t = TokenId.Equal;
                    break;
                case '/':
                    this.NextChar();
                    t = TokenId.Slash;
                    break;
                case '?':
                    this.NextChar();
                    t = TokenId.Question;
                    break;
                case '.':
                    this.NextChar();
                    t = TokenId.Dot;
                    break;
                case '\'':
                    char quote = this.ch;
                    do
                    {
                        this.NextChar();
                        while (this.textPos < this.textLen && this.ch != quote)
                        {
                            this.NextChar();
                        }

                        if (this.textPos == this.textLen)
                        {
                            throw ParseError(Strings.RequestQueryParser_UnterminatedStringLiteral(this.textPos, this.text));
                        }

                        this.NextChar();
                    } 
                    while (this.ch == quote);
                    t = TokenId.StringLiteral;
                    break;
                case '*':
                    this.NextChar();
                    t = TokenId.Star;
                    break;
                default:
                    if (Char.IsLetter(this.ch) || this.ch == '_')
                    {
                        this.ParseIdentifier();
                        t = TokenId.Identifier;
                        break;
                    }

                    if (Char.IsDigit(this.ch))
                    {
                        t = this.ParseFromDigit();
                        break;
                    }
                    
                    if (this.textPos == this.textLen)
                    {
                        t = TokenId.End;
                        break;
                    }

                    throw ParseError(Strings.RequestQueryParser_InvalidCharacter(this.ch, this.textPos));
            }

            this.token.Id = t;
            this.token.Text = this.text.Substring(tokenPos, this.textPos - tokenPos);
            this.token.Position = tokenPos;

            // Handle type-prefixed literals such as binary, datetime or guid.
            this.HandleTypePrefixedLiterals();

            // Handle keywords.
            if (this.token.Id == TokenId.Identifier)
            {
                if (IsInfinityOrNaNDouble(this.token.Text))
                {
                    this.token.Id = TokenId.DoubleLiteral;
                }
                else if (IsInfinityOrNanSingle(this.token.Text))
                {
                    this.token.Id = TokenId.SingleLiteral;
                }
                else if (this.token.Text == ExpressionConstants.KeywordTrue || this.token.Text == ExpressionConstants.KeywordFalse)
                {
                    this.token.Id = TokenId.BooleanLiteral;
                }
                else if (this.token.Text == ExpressionConstants.KeywordNull)
                {
                    this.token.Id = TokenId.NullLiteral;
                }
            }
        }

        /// <summary>
        /// Starting from an identifier, reads a sequence of dots and 
        /// identifiers, and returns the text for it, with whitespace 
        /// stripped.
        /// </summary>
        /// <returns>The dotted identifier starting at the current identifie.</returns>
        internal string ReadDottedIdentifier()
        {
            this.ValidateToken(TokenId.Identifier);
            StringBuilder builder = null;
            string result = this.CurrentToken.Text;
            this.NextToken();
            while (this.CurrentToken.Id == TokenId.Dot)
            {
                this.NextToken();
                this.ValidateToken(TokenId.Identifier);
                if (builder == null)
                {
                    builder = new StringBuilder(result, result.Length + 1 + this.CurrentToken.Text.Length);
                }

                builder.Append('.');
                builder.Append(this.CurrentToken.Text);                
                this.NextToken();
            }

            if (builder != null)
            {
                result = builder.ToString();
            }

            return result;
        }

        /// <summary>Returns the next token without advancing the lexer.</summary>
        /// <returns>The next token.</returns>
        internal Token PeekNextToken()
        {
            int savedTextPos = this.textPos;
            char savedChar = this.ch;
            Token savedToken = this.token;

            this.NextToken();
            Token result = this.token;

            this.textPos = savedTextPos;
            this.ch = savedChar;
            this.token = savedToken;

            return result;
        }

        /// <summary>Validates the current token is of the specified kind.</summary>
        /// <param name="t">Expected token kind.</param>
        internal void ValidateToken(TokenId t)
        {
            if (this.token.Id != t)
            {
                throw ParseError(Strings.RequestQueryParser_SyntaxError(this.textPos));
            }
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>Checks if the <paramref name="tokenText"/> is INF or NaN.</summary>
        /// <param name="tokenText">Input token.</param>
        /// <returns>true if match found, false otherwise.</returns>
        private static bool IsInfinityOrNaNDouble(string tokenText)
        {
            if (tokenText.Length == 3)
            {
                if (tokenText[0] == XmlConstants.XmlInfinityLiteral[0])
                {
                    return IsInfinityLiteralDouble(tokenText);
                }
                else
                if (tokenText[0] == XmlConstants.XmlNaNLiteral[0])
                {
                    return String.CompareOrdinal(tokenText, 0, XmlConstants.XmlNaNLiteral, 0, 3) == 0;
                }
            }
            
            return false;
        }

        /// <summary>
        /// Checks whether <paramref name="text"/> equals to 'INF'
        /// </summary>
        /// <param name="text">Text to look in.</param>
        /// <returns>true if the substring is equal using an ordinal comparison; false otherwise.</returns>
        private static bool IsInfinityLiteralDouble(string text)
        {
            Debug.Assert(text != null, "text != null");
            return String.CompareOrdinal(text, 0, XmlConstants.XmlInfinityLiteral, 0, text.Length) == 0;
        }

        /// <summary>Checks if the <paramref name="tokenText"/> is INFf/INFF or NaNf/NaNF.</summary>
        /// <param name="tokenText">Input token.</param>
        /// <returns>true if match found, false otherwise.</returns>
        private static bool IsInfinityOrNanSingle(string tokenText)
        {
            if (tokenText.Length == 4)
            {
                if (tokenText[0] == XmlConstants.XmlInfinityLiteral[0])
                {
                    return IsInfinityLiteralSingle(tokenText);
                }
                else if (tokenText[0] == XmlConstants.XmlNaNLiteral[0])
                {
                    return (tokenText[3] == ExpressionLexer.SingleSuffixLower || tokenText[3] == ExpressionLexer.SingleSuffixUpper) && 
                            String.CompareOrdinal(tokenText, 0, XmlConstants.XmlNaNLiteral, 0, 3) == 0;
                }
            }
            
            return false;
        }

        /// <summary>
        /// Checks whether <paramref name="text"/> EQUALS to 'INFf' or 'INFF' at position 
        /// </summary>
        /// <param name="text">Text to look in.</param>
        /// <returns>true if the substring is equal using an ordinal comparison; false otherwise.</returns>
        private static bool IsInfinityLiteralSingle(string text)
        {
            Debug.Assert(text != null, "text != null");
            return text.Length == 4 &&
                   (text[3] == ExpressionLexer.SingleSuffixLower || text[3] == ExpressionLexer.SingleSuffixUpper) &&
                   String.CompareOrdinal(text, 0, XmlConstants.XmlInfinityLiteral, 0, 3) == 0;
        }

        /// <summary>Creates an exception for a parse error.</summary>
        /// <param name="message">Message text.</param>
        /// <returns>A new Exception.</returns>
        private static Exception ParseError(string message)
        {
            return DataServiceException.CreateSyntaxError(message);
        }

        /// <summary>Handles lexemes that are formed by an identifier followed by a quoted string.</summary>
        /// <remarks>This method modified the token field as necessary.</remarks>
        private void HandleTypePrefixedLiterals()
        {
            TokenId id = this.token.Id;
            if (id != TokenId.Identifier)
            {
                return;
            }

            bool quoteFollows = this.ch == '\'';
            if (!quoteFollows)
            {
                return;
            }

            string tokenText = this.token.Text;
            if (String.Equals(tokenText, "datetime", StringComparison.OrdinalIgnoreCase))
            {
                id = TokenId.DateTimeLiteral;
            }
            else if (String.Equals(tokenText, "guid", StringComparison.OrdinalIgnoreCase))
            {
                id = TokenId.GuidLiteral;
            }
            else if (String.Equals(tokenText, "binary", StringComparison.OrdinalIgnoreCase) || tokenText == "X" || tokenText == "x")
            {
                id = TokenId.BinaryLiteral;
            }
            else
            {
                return;
            }

            int tokenPos = this.token.Position;
            do
            {
                this.NextChar();
            }
            while (this.ch != '\0' && this.ch != '\'');

            if (this.ch == '\0')
            {
                throw ParseError(Strings.RequestQueryParser_UnterminatedLiteral(this.textPos, this.text));
            }

            this.NextChar();
            this.token.Id = id;
            this.token.Text = this.text.Substring(tokenPos, this.textPos - tokenPos);
        }

        /// <summary>Advanced to the next character.</summary>
        private void NextChar()
        {
            if (this.textPos < this.textLen)
            {
                this.textPos++;
            }

            this.ch = this.textPos < this.textLen ? this.text[this.textPos] : '\0';
        }

        /// <summary>Parses a token that starts with a digit.</summary>
        /// <returns>The kind of token recognized.</returns>
        private TokenId ParseFromDigit()
        {
            Debug.Assert(Char.IsDigit(this.ch), "Char.IsDigit(this.ch)");
            TokenId result;
            char startChar = this.ch;
            this.NextChar();
            if (startChar == '0' && this.ch == 'x' || this.ch == 'X')
            {
                result = TokenId.BinaryLiteral;
                do
                {
                    this.NextChar();
                }
                while (WebConvert.IsCharHexDigit(this.ch));
            }
            else
            {
                result = TokenId.IntegerLiteral;
                while (Char.IsDigit(this.ch))
                {
                    this.NextChar();
                }

                if (this.ch == '.')
                {
                    result = TokenId.DoubleLiteral;
                    this.NextChar();
                    this.ValidateDigit();

                    do
                    {
                        this.NextChar();
                    }
                    while (Char.IsDigit(this.ch));
                }

                if (this.ch == 'E' || this.ch == 'e')
                {
                    result = TokenId.DoubleLiteral;
                    this.NextChar();
                    if (this.ch == '+' || this.ch == '-')
                    {
                        this.NextChar();
                    }

                    this.ValidateDigit();
                    do
                    {
                        this.NextChar();
                    }
                    while (Char.IsDigit(this.ch));
                }

                if (this.ch == 'M' || this.ch == 'm')
                {
                    result = TokenId.DecimalLiteral;
                    this.NextChar();
                }
                else
                if (this.ch == 'd' || this.ch == 'D')
                {
                    result = TokenId.DoubleLiteral;
                    this.NextChar();
                }
                else if (this.ch == 'L' || this.ch == 'l')
                {
                    result = TokenId.Int64Literal;
                    this.NextChar();
                }
                else if (this.ch == 'f' || this.ch == 'F')
                {
                    result = TokenId.SingleLiteral;
                    this.NextChar();
                }
            }

            return result;
        }

        /// <summary>Parses an identifier by advancing the current character.</summary>
        private void ParseIdentifier()
        {
            Debug.Assert(Char.IsLetter(this.ch) || this.ch == '_', "Char.IsLetter(this.ch) || this.ch == '_'");
            do
            {
                this.NextChar();
            }
            while (Char.IsLetterOrDigit(this.ch) || this.ch == '_');
        }

        /// <summary>Sets the text position.</summary>
        /// <param name="pos">New text position.</param>
        private void SetTextPos(int pos)
        {
            this.textPos = pos;
            this.ch = this.textPos < this.textLen ? this.text[this.textPos] : '\0';
        }

        /// <summary>Validates the current character is a digit.</summary>
        private void ValidateDigit()
        {
            if (!Char.IsDigit(this.ch))
            {
                throw ParseError(Strings.RequestQueryParser_DigitExpected(this.textPos));
            }
        }

        #endregion Private methods.
    }
}
