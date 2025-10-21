//------------------------------------------------------------------------------
// <copyright file="RichTextBoxWordPunctuations.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\RichTextBoxWordPunctuations.uex' path='docs/doc[@for="RichTextBoxWordPunctuations"]/*' />
    /// <devdoc>
    ///     This class defines the possible kinds of punctuation tables that
    ///     can be used with the RichTextBox word wrapping and word breaking features.
    /// </devdoc>
    /* 
      CONSIDER: This should be marked with the Flags attribute, but we can't because our
              code generator cannot emit code for OR'd combinations of flags.
              
              Flags */


    [SuppressMessage("Microsoft.Design", "CA1027:MarkEnumsWithFlags")]              
    public enum RichTextBoxWordPunctuations {
        /// <include file='doc\RichTextBoxWordPunctuations.uex' path='docs/doc[@for="RichTextBoxWordPunctuations.Level1"]/*' />
        /// <devdoc>
        ///     Use pre-defined Level 1 punctuation table as default.
        /// </devdoc>
        Level1     = 0x080,

        /// <include file='doc\RichTextBoxWordPunctuations.uex' path='docs/doc[@for="RichTextBoxWordPunctuations.Level2"]/*' />
        /// <devdoc>
        ///     Use pre-defined Level 2 punctuation table as default.
        /// </devdoc>
        Level2     = 0x100,

        /// <include file='doc\RichTextBoxWordPunctuations.uex' path='docs/doc[@for="RichTextBoxWordPunctuations.Custom"]/*' />
        /// <devdoc>
        ///     Use a custom defined punctuation table.
        /// </devdoc>
        Custom     = 0x200,

        /// <include file='doc\RichTextBoxWordPunctuations.uex' path='docs/doc[@for="RichTextBoxWordPunctuations.All"]/*' />
        /// <devdoc>
        ///     Used as a mask.
        /// </devdoc>
        All = Level1 | Level2 | Custom,

    }
}
