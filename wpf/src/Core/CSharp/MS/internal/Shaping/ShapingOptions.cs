//---------------------------------------------------------------------------
//
// <copyright file=GlyphInfoList.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: ShapingOptions enum
//
// History:  
//  1/20/2005: Garyyang Created the file 
//
//---------------------------------------------------------------------------

using System;

namespace MS.Internal.Shaping
{
    /// <summary>
    /// Shaping control options
    /// </summary>
    [Flags]
    internal enum ShapingOptions
    {
        /// <summary>
        /// Default behavior
        /// </summary>
        None = 0,

        /// <summary>
        /// Make Unicode control characters visible
        /// </summary>
        DisplayControlCode  = 0x00000001,

        /// <summary>
        /// Ligatures are not to be used for shaping
        /// </summary>
        InhibitLigature     = 0x00000002,
    }    
}
