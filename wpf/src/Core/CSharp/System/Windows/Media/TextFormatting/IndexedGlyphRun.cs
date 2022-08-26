//---------------------------------------------------------------------------
//
// <copyright file=IndexedGlyphRun.cs company=Microsoft>
//    Copyright (C) Microsoft Corporation.  All rights reserved.
// </copyright>
// 
//
// Description: IndexedGlyphRun class
//
// History:  
//  2/2/2004: Garyyang Created the file 
//
//---------------------------------------------------------------------------

using System;
using System.Windows.Media;

namespace System.Windows.Media.TextFormatting
{
    /// <summary>
    /// GlyphRun indexed with text source character index. It allows clients to map a text source character index 
    /// to the corresponding GlyphRun.
    /// </summary>
    public sealed class IndexedGlyphRun
    {
        /// <summary>
        /// Internal constructor. 
        /// </summary>
        internal IndexedGlyphRun(
            int      textSourceCharacterIndex,
            int      textSourceCharacterLength,
            GlyphRun glyphRun
            )
        {
            _textSourceCharacterIndex  = textSourceCharacterIndex;
            _length                    = textSourceCharacterLength;
            _glyphRun                  = glyphRun;            
        }


        //----------------------------------
        // Public properties
        //----------------------------------

        /// <summary>
        /// gets the text source character index corresponding to the beginning of the GlyphRun
        /// </summary>
        public int TextSourceCharacterIndex
        {
            get
            { 
                return _textSourceCharacterIndex;
            }
        }

        /// <summary>
        /// gets the text source character length corresponding to this GlyphRun. The text source character
        /// length does not necessarily equal to the character count in GlyphRun.
        /// </summary>
        public int TextSourceLength
        {
            get 
            { 
                return _length;
            }
        }

        /// <summary>
        /// gets the GlyphRun object
        /// </summary>
        public GlyphRun GlyphRun 
        {
            get 
            {
                return _glyphRun;
            }                
        }

        //-------------------------
        // private members 
        //-------------------------
        private GlyphRun _glyphRun;
        private int      _textSourceCharacterIndex;
        private int      _length;
    }    
}
