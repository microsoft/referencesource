//------------------------------------------------------------------------------
// <copyright file="CheckState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\CheckState.uex' path='docs/doc[@for="CheckState"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the state of a control,
    ///       such as
    ///       a check
    ///       box, that can be checked, unchecked, or
    ///       set to an indeterminate state.
    ///    </para>
    /// </devdoc>
    public enum CheckState {

        /// <include file='doc\CheckState.uex' path='docs/doc[@for="CheckState.Unchecked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The control is unchecked.
        ///
        ///    </para>
        /// </devdoc>
        Unchecked = 0,

        /// <include file='doc\CheckState.uex' path='docs/doc[@for="CheckState.Checked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The control is checked.
        ///
        ///    </para>
        /// </devdoc>
        Checked = 1,

        /// <include file='doc\CheckState.uex' path='docs/doc[@for="CheckState.Indeterminate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The control
        ///       is indeterminate. An indeterminate control generally has a shaded appearance.
        ///       
        ///    </para>
        /// </devdoc>
        Indeterminate = 2,

    }
}
