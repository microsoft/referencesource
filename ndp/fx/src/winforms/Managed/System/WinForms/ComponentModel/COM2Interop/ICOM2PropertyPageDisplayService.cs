//------------------------------------------------------------------------------
// <copyright file="ICOM2PropertyPageDisplayService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.ComponentModel.Com2Interop {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using Microsoft.Win32;

    /// <include file='doc\ICOM2PropertyPageDisplayService.uex' path='docs/doc[@for="ICom2PropertyPageDisplayService"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface ICom2PropertyPageDisplayService {
        /// <include file='doc\ICOM2PropertyPageDisplayService.uex' path='docs/doc[@for="ICom2PropertyPageDisplayService.ShowPropertyPage"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void ShowPropertyPage(string title, object component, int dispid, Guid pageGuid, IntPtr parentHandle);
    }

}
