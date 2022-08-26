//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2004
//
//  File:      ITextSymbols.cs
//
//  Contents:  Definition of text symbols
//
//  Spec:      http://team/sites/Avalon/Specs/Text%20Formatting%20API.doc
//
//  Created:   1-2-2004 Worachai Chaoweeraprasit (wchao)
//  History:  1-19-2004 Garyyang: Change the class into internal interface
//
//------------------------------------------------------------------------


using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.TextFormatting;

using MS.Internal.Shaping;

namespace MS.Internal.TextFormatting
{
    /// <summary>
    /// Provide definition for a group of symbols that may be represented by 
    /// multiple distinct TextShapeableSymbols. TextSymbols produces a 
    /// collection of TextShapeableSymbols objects.
    /// </summary>
    internal interface ITextSymbols
    {
        /// <summary>
        /// Get a list of TextShapeableSymbols object within the specified character range
        /// </summary>
        /// <param name="glyphingCache">Glyphing cache</param>        
        /// <param name="characterBufferReference">reference to character buffer of the first character to obtain TextShapeableSymbols</param>
        /// <param name="characterLength">number of characters to obtain TextShapeableSymbols</param>
        /// <param name="rightToLeft">flag indicates whether the specified character string is to be written from right to left</param>
        /// <param name="isRightToLeftParagraph">flag indicates whether the paragraph is to be written from right to left</param>
        /// <param name="digitCulture">specifies a culture used for number substitution; can be null to disable number substitution</param>
        /// <param name="textModifierScope">specifies the text modifier currently in scope, if any; can be null</param>
        /// <returns>list of TextShapeableSymbols objects</returns>
        IList<TextShapeableSymbols> GetTextShapeableSymbols(
            GlyphingCache               glyphingCache,
            CharacterBufferReference    characterBufferReference,
            int                         characterLength,
            bool                        rightToLeft,
            bool                        isRightToLeftParagraph,
            CultureInfo                 digitCulture,
            TextModifierScope           textModifierScope,
            TextFormattingMode          textFormattingMode,
            bool                        isSideways
            );
    }
}
