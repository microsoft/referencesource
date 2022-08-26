//------------------------------------------------------------------------------
// <copyright file="BindingCompleteContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms
{
    using System;

    /// <include file='doc\BindingCompleteContext.uex' path='docs/doc[@for="BindingCompleteContext"]/*' />
    /// <devdoc>
    ///     Indicates the direction of a binding operation.
    /// </devdoc>
    public enum BindingCompleteContext {

        /// <include file='doc\BindingCompleteContext.uex' path='docs/doc[@for="BindingCompleteContext.ControlUpdate"]/*' />
        /// <devdoc>
        ///     Control value is being updated from data source value.
        /// </devdoc>
        ControlUpdate = 0,

        /// <include file='doc\BindingCompleteContext.uex' path='docs/doc[@for="BindingCompleteContext.DataSourceUpdate"]/*' />
        /// <devdoc>
        ///     Data source value is being updated from control value.
        /// </devdoc>
        DataSourceUpdate = 1,
    }
}