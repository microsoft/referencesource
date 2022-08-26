//------------------------------------------------------------------------------
// <copyright file="ICommandExecutor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;

    /// <include file='doc\ICommandExecutor.uex' path='docs/doc[@for="ICommandExecutor"]/*' />
    /// <devdoc>
    /// </devdoc>
    /// <internalonly/>
    public interface ICommandExecutor {
        /// <include file='doc\ICommandExecutor.uex' path='docs/doc[@for="ICommandExecutor.Execute"]/*' />
        void Execute();
    }
}
