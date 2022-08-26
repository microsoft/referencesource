//------------------------------------------------------------------------------
// <copyright file="StringAlignment.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing {

    using System.Drawing;
    using System;

    /**
     * used for both vertical and horizontal alignment.
     */
    /// <include file='doc\StringAlignment.uex' path='docs/doc[@for="StringAlignment"]/*' />
    /// <devdoc>
    ///    Specifies the alignment of a text string
    ///    relative to its layout rectangle.
    /// </devdoc>
    public enum StringAlignment
    {
        // left or top in English
        /// <include file='doc\StringAlignment.uex' path='docs/doc[@for="StringAlignment.Near"]/*' />
        /// <devdoc>
        ///    Specifies the text be aligned near the
        ///    layout. In a left-to-right layout, the near position is left. In a right-to-left
        ///    layout, the near position is right.
        /// </devdoc>
        Near        = 0,
        
        /// <include file='doc\StringAlignment.uex' path='docs/doc[@for="StringAlignment.Center"]/*' />
        /// <devdoc>
        ///    Specifies that text is aligned in the
        ///    center of the layout rectangle.
        /// </devdoc>
        Center      = 1,
        
        // NO ALTERNATE SPELLINGS!
        // Centre      = 1,
        
        // right or bottom in English
        /// <include file='doc\StringAlignment.uex' path='docs/doc[@for="StringAlignment.Far"]/*' />
        /// <devdoc>
        ///    Specifies that text is aligned far from the
        ///    origin position of the layout rectangle. In a left-to-right layout, the far
        ///    position is right. In a right-to-left layout, the far position is left.
        /// </devdoc>
        Far         = 2
    }
}
