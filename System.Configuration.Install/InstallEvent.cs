//------------------------------------------------------------------------------
// <copyright file="InstallEvent.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {

    using System.Diagnostics;

    using System;
    using System.Collections;


/// <include file='doc\InstallEvent.uex' path='docs/doc[@for="InstallEventArgs"]/*' />
/// <devdoc>
///    <para>
///       Provides data for the <see cref='System.Configuration.Install.Installer.OnBeforeInstall'/>, <see cref='System.Configuration.Install.Installer.OnAfterInstall'/>, <see cref='System.Configuration.Install.Installer.OnCommitting'/>, <see cref='System.Configuration.Install.Installer.OnCommitted'/>, <see cref='System.Configuration.Install.Installer.OnBeforeRollback'/>, <see cref='System.Configuration.Install.Installer.OnAfterRollback'/>, <see cref='System.Configuration.Install.Installer.OnBeforeUninstall'/>, and <see cref='System.Configuration.Install.Installer.OnAfterUninstall'/> events.
///    </para>
/// </devdoc>
    public class InstallEventArgs : EventArgs {
        private IDictionary savedState;
        /// <include file='doc\InstallEvent.uex' path='docs/doc[@for="InstallEventArgs.InstallEventArgs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Configuration.Install.InstallEventArgs'/>
        ///       class, leaving the <paramref name="savedState"/>
        ///       field empty.
        ///    </para>
        /// </devdoc>
        public InstallEventArgs() : base() {
        }

        /// <include file='doc\InstallEvent.uex' path='docs/doc[@for="InstallEventArgs.InstallEventArgs1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Configuration.Install.InstallEventArgs'/> class, with the saved state
        ///       parameter.
        ///    </para>
        /// </devdoc>
        public InstallEventArgs(IDictionary savedState) : base() {
            this.savedState = savedState;
        }
        
        /// <include file='doc\InstallEvent.uex' path='docs/doc[@for="InstallEventArgs.SavedState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An <see cref='System.Collections.IDictionary'/>
        ///       that represents the current state of the installation.
        ///    </para>
        /// </devdoc>
        public IDictionary SavedState {
            get {
                return this.savedState;
            }
        }
    }
}
