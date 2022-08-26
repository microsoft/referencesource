//------------------------------------------------------------------------------
// <copyright file="ISupportOleDropSource.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Windows.Forms {
    using System;

    internal interface ISupportOleDropSource {
	
        /// <include file='doc\ISupportOleDropSource.uex' path='docs/doc[@for=ISupportOleDropSource.OnQueryContinueDrag]/*'/ />
        /// <devdoc>
        /// Summary of OnQueryContinueDrag.
        /// </devdoc>
        /// <param name=qcdevent></param>	
        void OnQueryContinueDrag(QueryContinueDragEventArgs qcdevent);
	
        /// <include file='doc\ISupportOleDropSource.uex' path='docs/doc[@for=ISupportOleDropSource.OnGiveFeedback]/*'/ />
        /// <devdoc>
        /// Summary of OnGiveFeedback.
        /// </devdoc>
        /// <param name=gfbevent></param>	
        void OnGiveFeedback(GiveFeedbackEventArgs gfbevent);
    }
}
