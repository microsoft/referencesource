//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// Description: Modifiability enum used by LocalizabilityAttribute 
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
    /// Modifiability of the attribute's targeted value in baml
    /// </summary>
    // Less restrictive value has a higher numeric value
    // NOTE: Enum values must be made in sync with the enum parsing logic in 
    // Framework/MS/Internal/Globalization/LocalizationComments.cs
#if PBTCOMPILER
    internal enum Modifiability 
#else
    public enum Modifiability
#endif    
    {
        /// <summary>
        /// Targeted value is not modifiable by localizers.
        /// </summary>
        Unmodifiable = 0,

        /// <summary>
        /// Targeted value is modifiable by localizers.
        /// </summary>
        Modifiable   = 1,

        /// <summary>
        /// Targeted value's modifiability inherits from the the parent nodes.
        /// </summary>
        Inherit      = 2, 
    }
}    

