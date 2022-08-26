//---------------------------------------------------------------------
// <copyright file="KeyInstance.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a class used to represent a key value for a resource.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System.Collections.Generic;
    using System.Data.Services.Parsing;
    using System.Data.Services.Providers;
    using System.Diagnostics;

    /// <summary>Provides a class used to represent a key for a resource.</summary>
    /// <remarks>
    /// Internally, every key instance has a collection of values. These values
    /// can be named or positional, depending on how they were specified
    /// if parsed from a URI.
    /// </remarks>
    internal class KeyInstance
    {
        /// <summary>Empty key singleton.</summary>
        private static readonly KeyInstance Empty = new KeyInstance();

        /// <summary>Named values.</summary>
        private readonly Dictionary<string, object> namedValues;

        /// <summary>Positional values.</summary>
        private readonly List<object> positionalValues;

        /// <summary>Initializes a new empty <see cref="KeyInstance"/> instance.</summary>
        private KeyInstance()
        {
        }

        /// <summary>Initializes a new <see cref="KeyInstance"/> instance.</summary>
        /// <param name='namedValues'>Named values.</param>
        /// <param name='positionalValues'>Positional values for this instance.</param>
        /// <remarks>
        /// One of namedValues or positionalValues should be non-null, but not both.
        /// </remarks>
        private KeyInstance(Dictionary<string, object> namedValues, List<object> positionalValues)
        {
            Debug.Assert(
                (namedValues == null) != (positionalValues == null),
                "namedValues == null != positionalValues == null -- one or the other should be assigned, but not both");
            this.namedValues = namedValues;
            this.positionalValues = positionalValues;
        }

        /// <summary>Whether the values have a name.</summary>
        internal bool AreValuesNamed
        {
            get { return this.namedValues != null; }
        }

        /// <summary>Checks whether this key has any values.</summary>
        internal bool IsEmpty
        {
            get { return this == Empty; }
        }

        /// <summary>Returns a dictionary of named values when they AreValuesNamed is true.</summary>
        internal IDictionary<string, object> NamedValues
        {
            get { return this.namedValues; }
        }

        /// <summary>Returns a list of values when they AreValuesNamed is false.</summary>
        internal IList<object> PositionalValues
        {
            get { return this.positionalValues; }
        }

        /// <summary>Number of values in the key.</summary>
        internal int ValueCount
        {
            get
            {
                if (this == Empty)
                {
                    return 0;
                }
                else if (this.namedValues != null)
                {
                    return this.namedValues.Count;
                }
                else
                {
                    Debug.Assert(this.positionalValues != null, "this.positionalValues != null");
                    return this.positionalValues.Count;
                }
            }
        }

        /// <summary>Attempts to parse key values from the specified text.</summary>
        /// <param name='text'>Text to parse (not null).</param>
        /// <param name='instance'>After invocation, the parsed key instance.</param>
        /// <returns>
        /// true if the key instance was parsed; false if there was a 
        /// syntactic error.
        /// </returns>
        /// <remarks>
        /// The returned instance contains only string values. To get typed values, a call to
        /// <see cref="TryConvertValues"/> is necessary.
        /// </remarks>
        internal static bool TryParseKeysFromUri(string text, out KeyInstance instance)
        {
            return TryParseFromUri(text, true /*allowNamedValues*/, false /*allowNull*/, out instance);
        }

        /// <summary>Attempts to parse nullable values (only positional values, no name-value pairs) from the specified text.</summary>
        /// <param name='text'>Text to parse (not null).</param>
        /// <param name='instance'>After invocation, the parsed key instance.</param>
        /// <returns>
        /// true if the given values were parsed; false if there was a 
        /// syntactic error.
        /// </returns>
        /// <remarks>
        /// The returned instance contains only string values. To get typed values, a call to
        /// <see cref="TryConvertValues"/> is necessary.
        /// </remarks>
        internal static bool TryParseNullableTokens(string text, out KeyInstance instance)
        {
            return TryParseFromUri(text, false /*allowNamedValues*/, true /*allowNull*/, out instance);
        }

        /// <summary>Tries to convert values to the keys of the specified type.</summary>
        /// <param name="type">Type with key information for conversion.</param>
        /// <returns>true if all values were converted; false otherwise.</returns>
        internal bool TryConvertValues(ResourceType type)
        {
            Debug.Assert(type != null, "type != null");
            Debug.Assert(!this.IsEmpty, "!this.IsEmpty -- caller should check");
            Debug.Assert(type.KeyProperties.Count == this.ValueCount, "type.KeyProperties.Count == this.ValueCount -- will change with containment");
            if (this.namedValues != null)
            {
                for (int i = 0; i < type.KeyProperties.Count; i++)
                {
                    ResourceProperty property = type.KeyProperties[i];
                    object unconvertedValue;
                    if (!this.namedValues.TryGetValue(property.Name, out unconvertedValue))
                    {
                        return false;
                    }

                    string valueText = (string)unconvertedValue;
                    object convertedValue;
                    if (!WebConvert.TryKeyStringToPrimitive(valueText, property.Type, out convertedValue))
                    {
                        return false;
                    }

                    this.namedValues[property.Name] = convertedValue;
                }
            }
            else
            {
                Debug.Assert(this.positionalValues != null, "positionalValues != null -- otherwise this is Empty");
                for (int i = 0; i < type.KeyProperties.Count; i++)
                {
                    string valueText = (string)this.positionalValues[i];
                    object convertedValue;
                    if (!WebConvert.TryKeyStringToPrimitive(valueText, type.KeyProperties[i].Type, out convertedValue))
                    {
                        return false;
                    }

                    this.positionalValues[i] = convertedValue;
                }
            }

            return true;
        }

        /// <summary>Attempts to parse key values from the specified text.</summary>
        /// <param name='text'>Text to parse (not null).</param>
        /// <param name="allowNamedValues">Set to true if the parser should accept named values
        ///   so syntax like Name='value'. If this is false, the parsing will fail on such constructs.</param>
        /// <param name="allowNull">Set to true if the parser should accept null values.
        ///   If set to false, the parser will fail on null values.</param>
        /// <param name='instance'>After invocation, the parsed key instance.</param>
        /// <returns>
        /// true if the key instance was parsed; false if there was a 
        /// syntactic error.
        /// </returns>
        /// <remarks>
        /// The returned instance contains only string values. To get typed values, a call to
        /// <see cref="TryConvertValues"/> is necessary.
        /// </remarks>
        private static bool TryParseFromUri(string text, bool allowNamedValues, bool allowNull, out KeyInstance instance)
        {
            Debug.Assert(text != null, "text != null");

            Dictionary<string, object> namedValues = null;
            List<object> positionalValues = null;

            ExpressionLexer lexer = new ExpressionLexer(text);
            Token currentToken = lexer.CurrentToken;
            if (currentToken.Id == TokenId.End)
            {
                instance = Empty;
                return true;
            }

            instance = null;
            do
            {
                if (currentToken.Id == TokenId.Identifier && allowNamedValues)
                {
                    // Name-value pair.
                    if (positionalValues != null)
                    {
                        // We cannot mix named and non-named values.
                        return false;
                    }

                    string identifier = lexer.CurrentToken.GetIdentifier();
                    lexer.NextToken();
                    if (lexer.CurrentToken.Id != TokenId.Equal)
                    {
                        return false;
                    }

                    lexer.NextToken();
                    if (!lexer.CurrentToken.IsKeyValueToken)
                    {
                        return false;
                    }

                    string namedValue = lexer.CurrentToken.Text;
                    WebUtil.CreateIfNull(ref namedValues);
                    if (namedValues.ContainsKey(identifier))
                    {
                        // Duplicate name.
                        return false;
                    }

                    namedValues.Add(identifier, namedValue);
                }
                else if (currentToken.IsKeyValueToken || (allowNull && currentToken.Id == TokenId.NullLiteral))
                {
                    // Positional value.
                    if (namedValues != null)
                    {
                        // We cannot mix named and non-named values.
                        return false;
                    }

                    WebUtil.CreateIfNull(ref positionalValues);
                    positionalValues.Add(lexer.CurrentToken.Text);
                }
                else
                {
                    return false;
                }

                // Read the next token. We should be at the end, or find
                // we have a comma followed by something.
                lexer.NextToken();
                currentToken = lexer.CurrentToken;
                if (currentToken.Id == TokenId.Comma)
                {
                    lexer.NextToken();
                    currentToken = lexer.CurrentToken;
                    if (currentToken.Id == TokenId.End)
                    {
                        // Trailing comma.
                        return false;
                    }
                }
            }
            while (currentToken.Id != TokenId.End);

            instance = new KeyInstance(namedValues, positionalValues);
            return true;
        }
    }
}
