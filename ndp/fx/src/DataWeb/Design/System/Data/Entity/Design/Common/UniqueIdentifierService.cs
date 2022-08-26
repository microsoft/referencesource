//---------------------------------------------------------------------
// <copyright file="Emitter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// @owner       Microsoft
// @backupOwner Microsoft
//---------------------------------------------------------------------


using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
namespace System.Data.Services.Design.Common
{
    /// <summary>
    /// Service making names within a scope unique. Initialize a new instance
    /// for every scope.
    /// 
    /// 


    internal sealed class UniqueIdentifierService
    {
        internal UniqueIdentifierService(bool caseSensitive)
        {
            _knownIdentifiers = new HashSet<string>(caseSensitive ? StringComparer.Ordinal : StringComparer.OrdinalIgnoreCase);
            _identifierToAdjustedIdentifier = new Dictionary<object, string>();
        }

        private readonly HashSet<string> _knownIdentifiers;
        private readonly Dictionary<object, string> _identifierToAdjustedIdentifier;

        /// <summary>
        /// Given an identifier, makes it unique within the scope by adding
        /// a suffix (1, 2, 3, ...), and returns the adjusted identifier.
        /// </summary>
        /// <param name="identifier">Identifier. Must not be null or empty.</param>
        /// <param name="value">Object associated with this identifier in case it is required to
        /// retrieve the adjusted identifier. If not null, must not exist in the current scope already.</param>
        /// <returns>Identifier adjusted to be unique within the scope.</returns>
        internal string AdjustIdentifier(string identifier, object value)
        {
            Debug.Assert(!string.IsNullOrEmpty(identifier), "identifier is null or empty");

            // find a unique name by adding suffix as necessary
            int numberOfConflicts = 0;
            string adjustedIdentifier = identifier;
            while (_knownIdentifiers.Contains(adjustedIdentifier))
            {
                ++numberOfConflicts;
                adjustedIdentifier = identifier + numberOfConflicts.ToString(CultureInfo.InvariantCulture);
            }

            // remember the identifier in this scope
            Debug.Assert(!_knownIdentifiers.Contains(adjustedIdentifier), "we just made it unique");
            _knownIdentifiers.Add(adjustedIdentifier);

            if (null != value)
            {
                Debug.Assert(!_identifierToAdjustedIdentifier.ContainsKey(value), "should never register one value twice");
                _identifierToAdjustedIdentifier.Add(value, adjustedIdentifier);
            }

            return adjustedIdentifier;
        }

        
        /// <summary>
        /// Simple overload when you don't need to track back to an object
        /// </summary>
        /// <param name="identifier"></param>
        /// <returns></returns>
        internal string AdjustIdentifier(string identifier)
        {
            return AdjustIdentifier(identifier, null);
        }
    }
}
