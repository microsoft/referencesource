//------------------------------------------------------------------------------
// <copyright file="ComponentInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;    

    /// <include file='doc\ComponentInstaller.uex' path='docs/doc[@for="ComponentInstaller"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class ComponentInstaller : Installer {

        /// <include file='doc\ComponentInstaller.uex' path='docs/doc[@for="ComponentInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        /// Copies properties from the given component to this installer. This method
        /// will be called at design-time when the user clicks 'Add Installer' on a
        /// component that has specified this class as its installer. The installer
        /// should take all information it can from the live component and store it
        /// to be used at install time.
        /// </devdoc>
        public abstract void CopyFromComponent(IComponent component);

        /// <include file='doc\ComponentInstaller.uex' path='docs/doc[@for="ComponentInstaller.IsEquivalentInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool IsEquivalentInstaller(ComponentInstaller otherInstaller) {
            return false;
        }
    }

}
