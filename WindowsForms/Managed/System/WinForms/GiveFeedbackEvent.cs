//------------------------------------------------------------------------------
// <copyright file="GiveFeedbackEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    //=--------------------------------------------------------------------------=
    // GiveFeedbackEventArgs.cs
    //=--------------------------------------------------------------------------=
    // Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
    //
    // THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    // ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    // THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    // PARTICULAR PURPOSE.
    //=--------------------------------------------------------------------------=

    using System;
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;


    /// <include file='doc\GiveFeedbackEvent.uex' path='docs/doc[@for="GiveFeedbackEventArgs"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides data for the <see cref='System.Windows.Forms.Control.GiveFeedback'/>
    ///       event.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class GiveFeedbackEventArgs : EventArgs {
        private readonly DragDropEffects effect;
        private bool useDefaultCursors;

        /// <include file='doc\GiveFeedbackEvent.uex' path='docs/doc[@for="GiveFeedbackEventArgs.GiveFeedbackEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.GiveFeedbackEventArgs'/> class.
        ///    </para>
        /// </devdoc>
        public GiveFeedbackEventArgs(DragDropEffects effect, bool useDefaultCursors) {
            this.effect = effect;
            this.useDefaultCursors = useDefaultCursors;
        }

        /// <include file='doc\GiveFeedbackEvent.uex' path='docs/doc[@for="GiveFeedbackEventArgs.Effect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of drag-and-drop operation.
        ///    </para>
        /// </devdoc>
        public DragDropEffects Effect {
            get {
                return effect;
            }
        }

        /// <include file='doc\GiveFeedbackEvent.uex' path='docs/doc[@for="GiveFeedbackEventArgs.UseDefaultCursors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a
        ///       value
        ///       indicating whether a default pointer is used.
        ///    </para>
        /// </devdoc>
        public bool UseDefaultCursors {
            get {
                return useDefaultCursors;
            }
            set {
                useDefaultCursors = value;
            }
        }
    }
}
