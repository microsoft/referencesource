//------------------------------------------------------------------------------
// <copyright file="FlushIntention.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Drawing2D {

    using System.Diagnostics;

    using System;

    // FlushIntentionFlush merely means that all the pending commands have been passed to
    // the hardware, and that the final results will be shown as soon as the hardware finishes
    // its rendering.  FlushIntentionSync means to wait for the hardware to actually finish its
    // rendering before returning - this is important for animation and timing loops.
    /// <include file='doc\FlushIntention.uex' path='docs/doc[@for="FlushIntention"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies whether commands in the graphics stack are terminated (flushed)
    ///       immediately or executed as soon as possible.
    ///    </para>
    /// </devdoc>
    public enum FlushIntention {

        // Flush all batched rendering operations
        /// <include file='doc\FlushIntention.uex' path='docs/doc[@for="FlushIntention.Flush"]/*' />
        /// <devdoc>
        ///    Specifies the stack of all graphics
        ///    operations is flushed immediately.
        /// </devdoc>
        Flush = 0,

        // Flush all batched rendering operations and wait for them to complete
        /// <include file='doc\FlushIntention.uex' path='docs/doc[@for="FlushIntention.Sync"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that all graphics operations on the stack are execyted as soon as
        ///       possible. This synchronizes the graphics state.
        ///    </para>
        /// </devdoc>
        Sync = 1
    }
}

