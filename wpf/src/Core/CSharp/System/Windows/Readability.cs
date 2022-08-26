//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// Description: Readability enum used by LocalizabilityAttribute 
//
// History:  
//  06/17/2005 garyyang - Moved the enum into a separate file.
//
//---------------------------------------------------------------------------

#if PBTCOMPILER
namespace MS.Internal.Globalization
#else
namespace System.Windows
#endif
{
    /// <summary>
    /// Readability of the attribute's targeted value in Baml
    /// </summary>
    // Less restrictive value has a higher numeric value
    // NOTE: Enum values must be made in sync with the enum parsing logic in 
    // Framework/MS/Internal/Globalization/LocalizationComments.cs    
#if PBTCOMPILER
    internal enum Readability 
#else
    public enum Readability 
#endif    
    {
        /// <summary>
        /// Targeted value is not readable.
        /// </summary>
        Unreadable = 0,

        /// <summary>
        /// Targeted value is readable text.
        /// </summary>
        Readable   = 1,

        /// <summary>
        /// Targeted value's readability inherites from parent nodes.
        /// </summary>
        Inherit    = 2,            
    }
}

