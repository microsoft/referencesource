//------------------------------------------------------------------------------
// <copyright file="BindingCompleteState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;

    /// <include file='doc\BindingCompleteState.uex' path='docs/doc[@for="BindingCompleteState"]/*' />
    /// <devdoc>
    ///     Indicates the result of a completed binding operation.
    /// </devdoc>
    public enum BindingCompleteState {

        /// <include file='doc\BindingCompleteState.uex' path='docs/doc[@for="BindingCompleteState.Success"]/*' />
        /// <devdoc>
        ///     Binding operation completed successfully.
        /// </devdoc>
        Success = 0,

        /// <include file='doc\BindingCompleteState.uex' path='docs/doc[@for="BindingCompleteState.DataError"]/*' />
        /// <devdoc>
        ///     Binding operation failed with a data error.
        /// </devdoc>
        DataError = 1,

        /// <include file='doc\BindingCompleteState.uex' path='docs/doc[@for="BindingCompleteState.Exception"]/*' />
        /// <devdoc>
        ///     Binding operation failed with an exception.
        /// </devdoc>
        Exception = 2,
    }
}