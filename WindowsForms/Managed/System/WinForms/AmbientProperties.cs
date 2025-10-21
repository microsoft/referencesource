//------------------------------------------------------------------------------
// <copyright file="AmbientProperties.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace System.Windows.Forms {
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Windows.Forms.Design;
    using System.ComponentModel.Design;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\AmbientProperties.uex' path='docs/doc[@for="AmbientProperties"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides ambient property values to top-level controls.
    ///    </para>
    /// </devdoc>
    //
    // NOTE: internally, this class does double duty as storage for Control's inherited properties.
    public sealed class AmbientProperties {
    
        // Public ambient properties
        private Color    backColor; 
        private Color    foreColor;
        private Cursor   cursor;
        private Font     font;
        
        /// <include file='doc\AmbientProperties.uex' path='docs/doc[@for="AmbientProperties.BackColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the ambient BackColor, or Color.Empty if there is none.
        ///    </para>
        /// </devdoc>
        public Color BackColor {
            get {
                return backColor;
            }
            set {
                backColor = value;
            }
        }

        /// <include file='doc\AmbientProperties.uex' path='docs/doc[@for="AmbientProperties.Cursor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the ambient BackColor, or null if there is none.
        ///    </para>
        /// </devdoc>
        public Cursor Cursor {
            get {
                return cursor;
            }
            set {
                cursor = value;
            }
        }

        /// <include file='doc\AmbientProperties.uex' path='docs/doc[@for="AmbientProperties.Font"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the ambient Font, or null if there is none.
        ///    </para>
        /// </devdoc>
        public Font Font {
            get {
                return font;
            }
            set {
                font = value;
            }
        }
        
        /// <include file='doc\AmbientProperties.uex' path='docs/doc[@for="AmbientProperties.ForeColor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the ambient ForeColor, or Color.Empty if there is none.
        ///    </para>
        /// </devdoc>
        public Color ForeColor {
            get {
                return foreColor;
            }
            set {
                foreColor = value;
            }
        }
    }
}

