//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:      IShapeableTextCollector.cs
//
//  Contents:  Definition of shapeable text object collector
//
//  Created:   2-24-2004 Worachai Chaoweeraprasit (wchao)
//
//------------------------------------------------------------------------


using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;

using System.Windows.Media;
using System.Windows.Media.TextFormatting;
using MS.Internal.Shaping;

namespace MS.Internal.TextFormatting
{
    /// <summary>
    /// Collector of shapeable text objects
    /// </summary>
    internal interface IShapeableTextCollector
    {
        /// <summary>
        /// Add shapeable text object to the list
        /// </summary>
        /// <param name="shapeableList">list of shapeable text objects</param>
        /// <param name="characterBufferRange">character buffer range</param>
        /// <param name="textRunProperties">text run properites, possibly modified</param>
        /// <param name="textItem">text item</param>
        /// <param name="shapeTypeface">shape typeface</param>
        /// <param name="emScale">scaling factor to em</param>
        /// <param name="nullShape">always yield missing glyphs</param>
        /// <returns>a shapeable object</returns>
        void Add(
            IList<TextShapeableSymbols>              shapeableList,
            CharacterBufferRange                     characterBufferRange,
            TextRunProperties                        textRunProperties,
            MS.Internal.Text.TextInterface.ItemProps textItem,
            ShapeTypeface                            shapeTypeface,
            double                                   emScale,
            bool                                     nullShape,
            TextFormattingMode                       textFormattingMode
            );
    }
}
