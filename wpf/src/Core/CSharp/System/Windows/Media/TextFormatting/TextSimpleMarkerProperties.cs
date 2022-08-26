//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation, 2004
//
//  File:      TextSimpleMarkerProperties.cs
//
//  Contents:  Generic implementation of text marker properties
//
//  Spec:      http://team/sites/Avalon/Specs/Text%20Formatting%20API.doc
//
//  Created:   1-2-2004 Worachai Chaoweeraprasit (wchao)
//
//------------------------------------------------------------------------


using System;
using System.Collections;
using System.Windows;
using MS.Internal.TextFormatting;

using SR=MS.Internal.PresentationCore.SR;
using SRID=MS.Internal.PresentationCore.SRID;

namespace System.Windows.Media.TextFormatting
{
    /// <summary>
    /// Generic implementation of text marker properties
    /// </summary>
    public class TextSimpleMarkerProperties : TextMarkerProperties
    {
        private double          _offset;
        private TextSource      _textSource;


        /// <summary>
        /// Construct a text marker object
        /// </summary>
        /// <param name="style">marker style</param>
        /// <param name="offset">distance from line start to the end of the marker symbol</param>
        /// <param name="autoNumberingIndex">autonumbering counter of counter-style marker</param>
        /// <param name="textParagraphProperties">text paragraph properties</param>
        public TextSimpleMarkerProperties(
            TextMarkerStyle             style,
            double                      offset,
            int                         autoNumberingIndex,
            TextParagraphProperties     textParagraphProperties
            ) 
        {

            if (textParagraphProperties == null)
                throw new ArgumentNullException("textParagraphProperties");

            _offset = offset;

            if (style != TextMarkerStyle.None)
            {
                if (TextMarkerSource.IsKnownSymbolMarkerStyle(style))
                {
                    // autoNumberingIndex is ignored
                }
                else if (TextMarkerSource.IsKnownIndexMarkerStyle(style))
                {
                    // validate autoNumberingIndex
                    if (autoNumberingIndex < 1)
                    {
                        throw new ArgumentOutOfRangeException("autoNumberingIndex", SR.Get(SRID.ParameterCannotBeLessThan, 1));
                    }
                }
                else
                {
                    // invalid style
                    throw new ArgumentException(SR.Get(SRID.Enum_Invalid, typeof(TextMarkerStyle)), "style");
                }

                _textSource = new TextMarkerSource(
                    textParagraphProperties, 
                    style, 
                    autoNumberingIndex
                    );
            }
        }


        /// <summary>
        /// Distance from line start to the end of the marker symbol
        /// </summary>
        public sealed override double Offset
        {
            get { return _offset; }
        }


        /// <summary>
        /// Source of text runs used for text marker
        /// </summary>
        public sealed override TextSource TextSource
        {
            get { return _textSource; }
        }
    }
}

