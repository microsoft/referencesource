//------------------------------------------------------------------------------
// <copyright file="TextImageRelation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    // Defined in such a way that you can cast the relation to an AnchorStyle and the direction of
    // the AnchorStyle points to where the image goes.  (e.g., (AnchorStyle)ImageBeforeText -> Left))
    /// <include file='doc\TextImageRelation.uex' path='docs/doc[@for="TextImageRelation"]/*' />
    public enum TextImageRelation {
        /// <include file='doc\TextImageRelation.uex' path='docs/doc[@for="TextImageRelation.Overlay"]/*' />
        Overlay = AnchorStyles.None,
        /// <include file='doc\TextImageRelation.uex' path='docs/doc[@for="TextImageRelation.ImageBeforeText"]/*' />
        ImageBeforeText = AnchorStyles.Left,
        /// <include file='doc\TextImageRelation.uex' path='docs/doc[@for="TextImageRelation.TextBeforeImage"]/*' />
        TextBeforeImage = AnchorStyles.Right,
        /// <include file='doc\TextImageRelation.uex' path='docs/doc[@for="TextImageRelation.ImageAboveText"]/*' />
        ImageAboveText = AnchorStyles.Top,
        /// <include file='doc\TextImageRelation.uex' path='docs/doc[@for="TextImageRelation.TextAboveImage"]/*' />
        TextAboveImage = AnchorStyles.Bottom
    };
}
