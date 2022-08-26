//------------------------------------------------------------------------------
// <copyright file="Brush.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if WINFORMS_NAMESPACE
namespace System.Windows.Forms.Internal
#elif DRAWING_NAMESPACE
namespace System.Drawing.Internal
#else
namespace System.Experimental.Gdi
#endif
{
    using System;

    /// <devdoc>
    ///    Specifies the layout of a device context.
    /// </devdoc>
    [Flags]
#if WINFORMS_PUBLIC_GRAPHICS_LIBRARY
    public
#else
    internal
#endif
    enum DeviceContextLayout
    {   
        /// <devdoc>
        ///     Right to left.
        /// </devdoc>
        Normal = 0x00000000,

        /// <devdoc>
        ///     Right to left. LAYOUT_RTL
        /// </devdoc>
        RightToLeft = 0x00000001,

        /// <devdoc>
        ///     Bottom to top. LAYOUT_BTT
        /// </devdoc>
        BottomToTop = 0x00000002,
        
        /// <devdoc>
        ///     Vertical before horizontal. LAYOUT_VBH
        /// </devdoc>
        VerticalBeforeHorizontal = 0x00000004,

        /// <devdoc>
        ///     Disables any reflection during BitBlt and StretchBlt operations. LAYOUT_BITMAPORIENTATIONPRESERVED
        /// </devdoc>
        BitmapOrientationPreserved = 0x00000008
    }
}
