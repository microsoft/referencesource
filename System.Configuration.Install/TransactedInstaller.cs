//------------------------------------------------------------------------------
// <copyright file="TransactedInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {

    using System.Diagnostics;

    using System;
    using System.IO;
    using System.Reflection;
    using System.Collections;
    using System.ComponentModel;
    
    /// <include file='doc\TransactedInstaller.uex' path='docs/doc[@for="TransactedInstaller"]/*' />
    /// <devdoc>
    /// A transacted install will either completely succeed or fail and leave the
    /// machine in its initial state. To run installers in a transaction, add them
    /// to the Installers collection of this class.
    /// </devdoc>
    public class TransactedInstaller : Installer {

        // A transacted install will either completely succeed or fail and leave the
        // machine in its initial state. The Install method is called on each of the
        // installers. If they all succeed, then the Commit method is called on each
        // of them. If any of the Install methods fails, then that installer's Rollback
        // method is called, as are the Rollback methods on all the installers whose
        // Install methods ran before the failure.
        /// <include file='doc\TransactedInstaller.uex' path='docs/doc[@for="TransactedInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Install(IDictionary savedState) {
            if (Context == null)
                Context = new InstallContext();

            Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoTransacted));
            try {
                bool success = true;
                try {
                    Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoBeginInstall));
                    base.Install(savedState);
                }
                catch (Exception e) {
                    success = false;
                    Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoException));
                    Installer.LogException(e, Context);

                    Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoBeginRollback));
                    try {
                        Rollback(savedState);
                    }
                    catch (Exception) {
                    }
                    Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoRollbackDone));

                    // make sure the failure goes all the way to the top.
                    throw new InvalidOperationException(Res.GetString(Res.InstallRollback), e);
                }
                if (success) {
                    Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoBeginCommit));
                    try {
                        Commit(savedState);
                    }
                    finally {
                        Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoCommitDone));
                    }
                }
            }
            finally {
                Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoTransactedDone));
            }
        }

        /// <include file='doc\TransactedInstaller.uex' path='docs/doc[@for="TransactedInstaller.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            if (Context == null)
                Context = new InstallContext();

            Context.LogMessage(Environment.NewLine + Environment.NewLine + Res.GetString(Res.InstallInfoBeginUninstall));
            try {
                base.Uninstall(savedState);
            }
            finally {
                Context.LogMessage(Environment.NewLine + Res.GetString(Res.InstallInfoUninstallDone));
            }
        }
    }
}
