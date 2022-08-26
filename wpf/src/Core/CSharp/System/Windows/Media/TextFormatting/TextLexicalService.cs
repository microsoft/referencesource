//-----------------------------------------------------------------------
//
//  Microsoft Windows Client Platform
//  Copyright (C) Microsoft Corporation
//
//  File:      TextLexicalService.cs
//
//  Contents:  Class abstraction for lexical analysis services such as word-breaking
//             or hyphenation.
//
//  Created:   6-13-2005 Worachai Chaoweeraprasit (wchao)
//
//------------------------------------------------------------------------


using System;
using System.Globalization;
using MS.Internal.PresentationCore;


namespace System.Windows.Media.TextFormatting
{
    /// <summary>
    /// Class abstraction to be implemented by the client to provide TextFormatter
    /// with lexical service such as word-breaking or hyphenation opportunity.
    /// </summary>
#if HYPHENATION_API
    public abstract class TextLexicalService
#else
    [FriendAccessAllowed]   // used by Framework
    internal abstract class TextLexicalService
#endif
    {
        /// <summary>
        /// TextFormatter to query whether the lexical services component could provides 
        /// analysis for the specified culture.
        /// </summary>
        /// <param name="culture">Culture whose text is to be analyzed</param>
        /// <returns>Boolean value indicates whether the specified culture is supported</returns>
        public abstract bool IsCultureSupported(CultureInfo culture);


        /// <summary>
        /// TextFormatter to get the lexical breaks of the specified raw text
        /// </summary>
        /// <remarks>
        /// TextFormatter determines the boundary of the input character array based on the delimited
        /// white space characters before and after the character array. 
        /// </remarks>
        /// <param name="characterSource">character array</param>
        /// <param name="length">number of character in the character array to analyze</param>
        /// <param name="textCulture">culture of the specified character source</param>
        /// <returns>lexical breaks of the text</returns>
        public abstract TextLexicalBreaks AnalyzeText(
            char[]          characterSource,
            int             length,
            CultureInfo     textCulture
            );
    }
}

