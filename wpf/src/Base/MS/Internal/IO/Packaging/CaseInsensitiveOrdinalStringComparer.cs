//-----------------------------------------------------------------------------
//
// <copyright file="CaseInsensitiveOrdinalStringComparer.cs" company="Microsoft">
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
//
// Description:
//  Common container-related operations that can be shared among internal
//  components.
//
// History:
//  02/17/2006: Microsoft: Creation
//
//-----------------------------------------------------------------------------

using System;
using System.Collections;   // for IEqualityComparer

using MS.Internal;          // for Invariant.Assert

namespace MS.Internal.IO.Packaging
{
    /// <summary>
    /// IEqualityComparer implementation for case insensistive ordinal strings
    /// </summary>
    internal class CaseInsensitiveOrdinalStringComparer :
                                IEqualityComparer, IComparer
    {
        // Performs Case Insensitive Ordinal String Comparison.
        bool IEqualityComparer.Equals(Object x, Object y)
        {
            Invariant.Assert((x is String) && (y is String));
            return (String.CompareOrdinal(((String) x).ToUpperInvariant(),
                                ((String) y).ToUpperInvariant()) == 0);
        }

        int IComparer.Compare(Object x, Object y)
        {
            Invariant.Assert((x is String) && (y is String));

            return String.CompareOrdinal(((String) x).ToUpperInvariant(),
                                ((String) y).ToUpperInvariant());
        }

        // Hash on object identity.
        int IEqualityComparer.GetHashCode(Object str)
        {
            Invariant.Assert(str is String);

            return ((String) str).ToUpperInvariant().GetHashCode();
        }
    }
}
