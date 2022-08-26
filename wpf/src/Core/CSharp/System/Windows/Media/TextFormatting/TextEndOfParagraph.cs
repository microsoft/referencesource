//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation
//
//  File:      TextEndOfParagraph.cs
//
//  Contents:  Implementation of text paragraph break control 
//
//  Spec:      http://team/sites/Avalon/Specs/Text%20Formatting%20API.doc
//
//  Created:   1-2-2004 Worachai Chaoweeraprasit (wchao)
//
//------------------------------------------------------------------------


using System;
using System.Collections;
using System.Windows;


namespace System.Windows.Media.TextFormatting
{
    /// <summary>
    /// Specialized line break control used to mark the end of a paragraph
    /// </summary>
    public class TextEndOfParagraph : TextEndOfLine
    {
        #region Constructors

        /// <summary>
        /// Construct a paragraph break run
        /// </summary>
        /// <param name="length">number of characters</param>
        public TextEndOfParagraph(int length) : base(length)
        {}


        /// <summary>
        /// Construct a paragraph break run
        /// </summary>
        /// <param name="length">number of characters</param>
        /// <param name="textRunProperties">linebreak text run properties</param>
        public TextEndOfParagraph(
            int                 length, 
            TextRunProperties   textRunProperties
            )
            : base(
                length, 
                textRunProperties
                )
        {}

        #endregion
    }
}

