//---------------------------------------------------------------------
// <copyright file="Token.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a type to represent a parsed token.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Parsing
{
    using System;
    using System.Diagnostics;

    /// <summary>Use this class to represent a lexical token.</summary>
    [DebuggerDisplay("{Id} @ {Position}: [{Text}]")]
    internal struct Token
    {
        /// <summary>Token representing gt keyword</summary>
        internal static readonly Token GreaterThan = new Token { Text = ExpressionConstants.KeywordGreaterThan, Id = TokenId.Identifier, Position = 0 };

        /// <summary>Token representing eq keyword</summary>
        internal static readonly Token EqualsTo = new Token { Text = ExpressionConstants.KeywordEqual, Id = TokenId.Identifier, Position = 0 };

        /// <summary>Token representing lt keyword</summary>
        internal static readonly Token LessThan = new Token { Text = ExpressionConstants.KeywordLessThan, Id = TokenId.Identifier, Position = 0 };

        /// <summary>Kind of token.</summary>
        internal TokenId Id;

        /// <summary>Token text.</summary>
        internal string Text;

        /// <summary>Position of token.</summary>
        internal int Position;

        /// <summary>Checks whether this token is a comparison operator.</summary>
        internal bool IsComparisonOperator
        {
            get
            {
                if (this.Id != TokenId.Identifier)
                {
                    return false;
                }

                return
                    this.Text == ExpressionConstants.KeywordEqual ||
                    this.Text == ExpressionConstants.KeywordNotEqual ||
                    this.Text == ExpressionConstants.KeywordLessThan ||
                    this.Text == ExpressionConstants.KeywordGreaterThan ||
                    this.Text == ExpressionConstants.KeywordLessThanOrEqual ||
                    this.Text == ExpressionConstants.KeywordGreaterThanOrEqual;
            }
        }

        /// <summary>Checks whether this token is an equality operator.</summary>
        internal bool IsEqualityOperator
        {
            get
            {
                return 
                    this.Id == TokenId.Identifier &&
                    (this.Text == ExpressionConstants.KeywordEqual || 
                     this.Text == ExpressionConstants.KeywordNotEqual);
            }
        }

        /// <summary>Checks whether this token is a valid token for a key value.</summary>
        internal bool IsKeyValueToken
        {
            get
            {
                return
                    this.Id == TokenId.BinaryLiteral ||
                    this.Id == TokenId.BooleanLiteral ||
                    this.Id == TokenId.DateTimeLiteral ||
                    this.Id == TokenId.GuidLiteral ||
                    this.Id == TokenId.StringLiteral ||
                    ExpressionLexer.IsNumeric(this.Id);
            }
        }

        /// <summary>Provides a string representation of this token.</summary>
        /// <returns>String representation of this token.</returns>
        public override string ToString()
        {
            return String.Format(System.Globalization.CultureInfo.InvariantCulture, "{0} @ {1}: [{2}]", this.Id, this.Position, this.Text);
        }

        /// <summary>Gets the current identifier text.</summary>
        /// <returns>The current identifier text.</returns>
        internal string GetIdentifier()
        {
            if (this.Id != TokenId.Identifier)
            {
                throw DataServiceException.CreateSyntaxError(Strings.RequestQueryParser_IdentifierExpected(this.Position));
            }

            return this.Text;
        }

        /// <summary>Checks that this token has the specified identifier.</summary>
        /// <param name="id">Identifier to check.</param>
        /// <returns>true if this is an identifier with the specified text.</returns>
        internal bool IdentifierIs(string id)
        {
            return this.Id == TokenId.Identifier && this.Text == id;
        }
    }
}
