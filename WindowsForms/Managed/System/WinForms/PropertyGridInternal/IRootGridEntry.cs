//------------------------------------------------------------------------------
// <copyright file="IRootGridEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.PropertyGridInternal {
    using System.Runtime.InteropServices;

    using System.Diagnostics;

        using System;
        using System.Collections;
        using System.Reflection;
        using System.ComponentModel;
        using System.ComponentModel.Design;
        using System.Windows.Forms;
        using System.Drawing;
        using Microsoft.Win32;

        /// <include file='doc\IRootGridEntry.uex' path='docs/doc[@for="IRootGridEntry"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public interface IRootGridEntry{
                /// <include file='doc\IRootGridEntry.uex' path='docs/doc[@for="IRootGridEntry.BrowsableAttributes"]/*' />
                /// <devdoc>
                ///    <para>[To be supplied.]</para>
                /// </devdoc>
                AttributeCollection BrowsableAttributes {
                     get;
                     set;
                }
                /// <include file='doc\IRootGridEntry.uex' path='docs/doc[@for="IRootGridEntry.ResetBrowsableAttributes"]/*' />
                /// <devdoc>
                ///    <para>[To be supplied.]</para>
                /// </devdoc>

                void ResetBrowsableAttributes();
                /// <include file='doc\IRootGridEntry.uex' path='docs/doc[@for="IRootGridEntry.ShowCategories"]/*' />
                /// <devdoc>
                ///    <para>[To be supplied.]</para>
                /// </devdoc>
                void ShowCategories(bool showCategories);
        }
}
