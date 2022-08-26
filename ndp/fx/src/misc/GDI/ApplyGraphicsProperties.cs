//------------------------------------------------------------------------------
// <copyright file="TextFormatFlags.cs" company="Microsoft">
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
    ///     Enumeration defining the different Graphics properties to apply to a WindowsGraphics when creating it
    ///     from a Graphics object.
    /// </devdoc>

    [Flags]
#if WINFORMS_PUBLIC_GRAPHICS_LIBRARY
    public
#else
    internal
#endif
    enum ApplyGraphicsProperties
    {
        // No properties to be applied to the DC obtained from the Graphics object.
        None                = 0x00000000,
        // Apply clipping region.
        Clipping            = 0x00000001,   
        // Apply coordinate transformation.
        TranslateTransform  = 0x00000002,
        // Apply all supported Graphics properties.
        All                 = Clipping | TranslateTransform
    }
}
