//------------------------------------------------------------------------------
// <copyright file="AutoCompleteMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    
    /// <include file='doc\AutoCompleteMode.uex' path='docs/doc[@for="AutoCompleteMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the autocomplete mode for ComboBox and TextBox AutoComplete Feature.
    ///    </para>
    /// </devdoc>
    public enum AutoCompleteMode {

        /// <include file='doc\AutoCompleteMode.uex' path='docs/doc[@for="AutoCompleteMode.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disables the AutoComplete Feature for ComboBox and TextBox.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\AutoCompleteMode.uex' path='docs/doc[@for="AutoCompleteMode.AutoSuggest"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Displays the auxiliary drop-down list associated with the edit control, this drop-down is populated 
        ///       with one or more suggested completed strings.
        ///    </para>
        /// </devdoc>
        Suggest = 0x1,

        /// <include file='doc\AutoCompleteMode.uex' path='docs/doc[@for="AutoCompleteMode.AutoAppend"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Appends the remainder of the most likely candidate string to the existing characters,
        ///       hightlighting the appended characters.
        ///    </para>
        /// </devdoc>
        Append = 0x2,

        /// <include file='doc\AutoCompleteMode.uex' path='docs/doc[@for="AutoCompleteMode.AutoSuggestAppend"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The AutoSuggest and AutoAppend are applied in conjuction.
        ///    </para>
        /// </devdoc>
        SuggestAppend = Suggest | Append
    }
}

