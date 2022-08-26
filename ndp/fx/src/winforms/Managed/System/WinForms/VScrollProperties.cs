//------------------------------------------------------------------------------
// <copyright file="VScrollProperties.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;
    using System.Security.Permissions;
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Windows.Forms;
    
    /// <include file='doc\VScrollProperties.uex' path='docs/doc[@for="VScrollProperties"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Basic Properties for VScroll.
    ///    </para>
    /// </devdoc>
    public class VScrollProperties : ScrollProperties {
        
        /// <include file='doc\VScrollProperties.uex' path='docs/doc[@for="ScrollProperties.VScrollProperties"]/*' />
        public VScrollProperties(ScrollableControl container) : base(container) {
        }

        internal override int PageSize  {
            get {
                return ParentControl.ClientRectangle.Height;
            }
        }

        internal override int Orientation  {
            get {
                return NativeMethods.SB_VERT;
            }
        }
        
        internal override int HorizontalDisplayPosition  {
            get {
                return ParentControl.DisplayRectangle.X;
            }
        }

        internal override int VerticalDisplayPosition  {
            get {
                return -this.value;
            }
        }
    }
}
