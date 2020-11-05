//------------------------------------------------------------------------------
// <copyright file="InstallEventHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {

    using System.Diagnostics;

    using System;

    /// <include file='doc\InstallEventHandler.uex' path='docs/doc[@for="InstallEventHandler"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see cref='System.Configuration.Install.Installer.OnBeforeInstall'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnAfterInstall'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnCommitting'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnCommitted'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnBeforeRollback'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnAfterRollback'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnBeforeUninstall'/> event, or
    ///    the <see cref='System.Configuration.Install.Installer.OnAfterUninstall'/>
    ///    event of
    ///    the <see cref='System.Configuration.Install.Installer'/>.</para>
    /// </devdoc>

    public delegate void InstallEventHandler(object sender, InstallEventArgs e);

}
