//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: ContainerParagraph.cs
//
// Description: TextFormatter host.
//
// History:  
//  05/05/2003 : Microsoft - moving from Avalon branch.
//
//---------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.TextFormatting;

namespace MS.Internal.PtsHost
{
    // ----------------------------------------------------------------------
    // TextFormatter host.
    // ----------------------------------------------------------------------
    internal sealed class TextFormatterHost : TextSource
    {
        internal TextFormatterHost(TextFormatter textFormatter, TextFormattingMode textFormattingMode, double pixelsPerDip)
        {
            if(textFormatter == null)
            {
                TextFormatter = TextFormatter.FromCurrentDispatcher(textFormattingMode);
            }
            else
            {
                TextFormatter = textFormatter;
            }

            PixelsPerDip = pixelsPerDip;
        }

        //-------------------------------------------------------------------
        // GetTextRun
        //-------------------------------------------------------------------
        public override TextRun GetTextRun(int textSourceCharacterIndex)
        {
            Debug.Assert(Context != null, "TextFormatter host is not initialized.");
            Debug.Assert(textSourceCharacterIndex >= 0, "Character index must be non-negative.");
            TextRun run = Context.GetTextRun(textSourceCharacterIndex);
            if (run.Properties != null)
            {
                run.Properties.PixelsPerDip = PixelsPerDip;
            }

            return run;
        }

        //-------------------------------------------------------------------
        // GetPrecedingText
        //-------------------------------------------------------------------
        public override TextSpan<CultureSpecificCharacterBufferRange> GetPrecedingText(int textSourceCharacterIndexLimit)
        {
            Debug.Assert(Context != null, "TextFormatter host is not initialized.");
            Debug.Assert(textSourceCharacterIndexLimit >= 0, "Character index must be non-negative.");
            return Context.GetPrecedingText(textSourceCharacterIndexLimit);
        }

        /// <summary>
        /// TextFormatter to map a text source character index to a text effect character index        
        /// </summary>
        /// <param name="textSourceCharacterIndex"> text source character index </param>
        /// <returns> the text effect index corresponding to the text effect character index </returns>
        public override int GetTextEffectCharacterIndexFromTextSourceCharacterIndex(
            int textSourceCharacterIndex
            )
        {
            Debug.Assert(Context != null, "TextFormatter host is not initialized.");
            Debug.Assert(textSourceCharacterIndex>= 0, "Character index must be non-negative.");
            return Context.GetTextEffectCharacterIndexFromTextSourceCharacterIndex(textSourceCharacterIndex);
        }
        
        //-------------------------------------------------------------------
        // TextFormatterHost context, object responsible for providing 
        // formatting information.
        //-------------------------------------------------------------------
        internal LineBase Context;

        //-------------------------------------------------------------------
        // TextFormatter.
        //-------------------------------------------------------------------
        internal TextFormatter TextFormatter;
    }

}
