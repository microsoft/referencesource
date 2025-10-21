//------------------------------------------------------------------------------
// <copyright file="UpDownEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;

    /// <include file='doc\UpDownEvent.uex' path='docs/doc[@for="UpDownEventArgs"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Provides data for the UpDownEvent
    ///    </para>
    /// </devdoc>
    public class UpDownEventArgs : EventArgs {

        int buttonID;

        /// <include file='doc\UpDownEvent.uex' path='docs/doc[@for="UpDownEventArgs.UpDownEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public UpDownEventArgs(int buttonPushed) {
            buttonID = buttonPushed;
        }

        /// <include file='doc\UpDownEvent.uex' path='docs/doc[@for="UpDownEventArgs.ButtonID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ButtonID {
            get {
                return buttonID;
            }
        }
    }
}
