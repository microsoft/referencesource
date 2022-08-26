//------------------------------------------------------------------------------
// <copyright file="ICompletion.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
#if false
namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;

    /// <include file='doc\ICompletion.uex' path='docs/doc[@for="ICompletion"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public interface ICompletion { 
       /// <include file='doc\ICompletion.uex' path='docs/doc[@for="ICompletion.CompletionStatusChanged"]/*' />
       /// <devdoc>
        ///     This function will be called by the ThreadPool's worker threads when a
        ///     packet is ready.
        ///     
        /// </devdoc>
        void CompletionStatusChanged(bool status, int size, NativeMethods.OVERLAPPED overlapped);
    }
}
#endif
