//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// Description: Predefined FontStyle structures that correspond to common font styles. 
//
// History:  
//  01/25/2005 mleonov - Created constants from FontStyle enum values.
//
//---------------------------------------------------------------------------

using System;
using System.Globalization;

namespace System.Windows 
{
    /// <summary>
    /// FontStyles contains predefined font style structures for common font styles.
    /// </summary>
    public static class FontStyles
    {
        /// <summary>
        /// Predefined font style : Normal.
        /// </summary>
        public static FontStyle Normal       { get { return new FontStyle(0); } }

        /// <summary>
        /// Predefined font style : Oblique.
        /// </summary>
        public static FontStyle Oblique { get { return new FontStyle(1); } }

        /// <summary>
        /// Predefined font style : Italic.
        /// </summary>
        public static FontStyle Italic { get { return new FontStyle(2); } }

        internal static bool FontStyleStringToKnownStyle(string s, IFormatProvider provider, ref FontStyle fontStyle)
        {
            if (s.Equals("Normal", StringComparison.OrdinalIgnoreCase))
            {
                fontStyle = FontStyles.Normal;
                return true;
            }
            if (s.Equals("Italic", StringComparison.OrdinalIgnoreCase))
            {
                fontStyle = FontStyles.Italic;
                return true;
            }
            if (s.Equals("Oblique", StringComparison.OrdinalIgnoreCase))
            {
                fontStyle = FontStyles.Oblique;
                return true;
            }
            return false;
        }
    }
}

