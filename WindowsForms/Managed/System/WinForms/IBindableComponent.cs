//------------------------------------------------------------------------------
// <copyright file="IBindableComponent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.ComponentModel;

    /// <include file='doc\IBindableComponent.uex' path='docs/doc[@for="IBindableComponent"]/*' />
    /// <devdoc>
    /// </devdoc>
    public interface IBindableComponent : IComponent {

        /// <include file='doc\IBindableComponent.uex' path='docs/doc[@for="IBindableComponent.DataBindings"]/*' />
        /// <devdoc>
        /// </devdoc>
        ControlBindingsCollection DataBindings { get; }

        /// <include file='doc\IBindableComponent.uex' path='docs/doc[@for="IBindableComponent.BindingContext"]/*' />
        /// <devdoc>
        /// </devdoc>
        BindingContext BindingContext { get; set; }

    }
}
