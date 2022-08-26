//------------------------------------------------------------------------------
// <copyright file="ControlEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;


    /// <include file='doc\ControlEventHandler.uex' path='docs/doc[@for="ControlEventHandler"]/*' />
    /// <devdoc>
    ///      Describes a delegate for an event that has a ControlEventArgs as
    ///      a parameter.
    /// </devdoc>
    public delegate void ControlEventHandler(object sender, ControlEventArgs e);
}
