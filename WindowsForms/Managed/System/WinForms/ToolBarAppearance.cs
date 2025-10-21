//------------------------------------------------------------------------------
// <copyright file="ToolBarAppearance.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;


    /// <include file='doc\ToolBarAppearance.uex' path='docs/doc[@for="ToolBarAppearance"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of toolbar to display.
    ///    </para>
    /// </devdoc>
    public enum ToolBarAppearance {

        /// <include file='doc\ToolBarAppearance.uex' path='docs/doc[@for="ToolBarAppearance.Normal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The
        ///       toolbar and buttons appear as standard three dimensional controls.
        ///    </para>
        /// </devdoc>
        Normal      = 0,

        /// <include file='doc\ToolBarAppearance.uex' path='docs/doc[@for="ToolBarAppearance.Flat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The toolbar and buttons appear flat, but the buttons change to three
        ///       dimensional as the mouse pointer moves over them.
        ///    </para>
        /// </devdoc>
        Flat        = 1,


    }
}
