//------------------------------------------------------------------------------
// <copyright file="TickStyle.cs" company="Microsoft">
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


    /// <include file='doc\TickStyle.uex' path='docs/doc[@for="TickStyle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the
    ///       location of tick marks in a <see cref='System.Windows.Forms.TrackBar'/>
    ///       control.
    ///    </para>
    /// </devdoc>
    public enum TickStyle {

        /// <include file='doc\TickStyle.uex' path='docs/doc[@for="TickStyle.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No tick marks appear in the control.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\TickStyle.uex' path='docs/doc[@for="TickStyle.TopLeft"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tick
        ///       marks are located on the top of horizontal control or on the left of a vertical control.
        ///    </para>
        /// </devdoc>
        TopLeft = 1,

        /// <include file='doc\TickStyle.uex' path='docs/doc[@for="TickStyle.BottomRight"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tick marks are
        ///       located on the bottom of a horizontal control or on the right side of a vertical control.
        ///    </para>
        /// </devdoc>
        BottomRight = 2,

        /// <include file='doc\TickStyle.uex' path='docs/doc[@for="TickStyle.Both"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Tick
        ///       marks are located on both sides of the control.
        ///    </para>
        /// </devdoc>
        Both = 3,

    }
}
