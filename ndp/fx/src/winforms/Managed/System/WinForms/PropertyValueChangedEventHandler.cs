//------------------------------------------------------------------------------
// <copyright file="PropertyValueChangedEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


/*
 */
namespace System.Windows.Forms {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    
    

    /// <include file='doc\PropertyValueChangedEventHandler.uex' path='docs/doc[@for="PropertyValueChangedEventHandler"]/*' />
    /// <devdoc>
    /// The event handler class that is invoked when a property
    /// in the grid is modified by the user.
    /// </devdoc>
    public delegate void PropertyValueChangedEventHandler(object s, PropertyValueChangedEventArgs e);
}
