//------------------------------------------------------------------------------
// <copyright file="Installer.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration.Install {
    using System.Text;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Configuration;
    using System;
    using Microsoft.Win32;
    using System.Collections;    
    using System.Windows.Forms;

    /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides
    ///       a basic framework for doing installations.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("AfterInstall")
    ]
    public class Installer : Component {

        private InstallerCollection installers;
        private InstallContext context;
        internal Installer parent;

        private InstallEventHandler afterCommitHandler;
        private InstallEventHandler afterInstallHandler;
        private InstallEventHandler afterRollbackHandler;
        private InstallEventHandler afterUninstallHandler;
        private InstallEventHandler beforeCommitHandler;
        private InstallEventHandler beforeInstallHandler;
        private InstallEventHandler beforeRollbackHandler;
        private InstallEventHandler beforeUninstallHandler;

        private const string wrappedExceptionSource = Res.WrappedExceptionSource;

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Context"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets information about the current installation.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public InstallContext Context {
            get {
                return context;
            }
            set {
                context = value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.HelpText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the help text for all of the <see cref='System.Configuration.Install.Installer'/> objects contained in
        ///       the <see cref='System.Configuration.Install.Installer.Installers'/> collection of this installer instance.
        ///    </para>
        /// </devdoc>
        [ResDescription("Desc_Installer_HelpText")]
		public virtual string HelpText {
            get {
                StringBuilder containedHelp = new StringBuilder();
                for (int i = 0; i < Installers.Count; i++) {
                    string help = Installers[i].HelpText;
                    if (help.Length > 0) {
                        containedHelp.Append("\r\n");
                        containedHelp.Append(help);
                    }
                }
                return containedHelp.ToString();
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Installers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the collection of <see cref='System.Configuration.Install.Installer'/> objects contained in this <see cref='System.Configuration.Install.Installer'/>
        ///       instance.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
        public InstallerCollection Installers {
            get {
                if (installers == null)
                    installers = new InstallerCollection(this);

                return installers;
            }
        }

        
        internal bool InstallerTreeContains(Installer target) {
            if (Installers.Contains(target))
                return true;

            foreach (Installer child in Installers)
                if (child.InstallerTreeContains(target))
                    return true;

            return false;
        }
        
        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Parent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Configuration.Install.Installer'/> whose <see cref='System.Configuration.Install.InstallerCollection'/> contains this <see cref='System.Configuration.Install.Installer'/>
        ///       instance.
        ///    </para>
        /// </devdoc>
        [
            Browsable(true), 
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), 
            TypeConverter(typeof(System.Configuration.Install.InstallerParentConverter)),
            ResDescription("Desc_Installer_Parent")
        ]
        public Installer Parent {
            get {
                return parent;
            }
            set {
                if (value == this)
                    throw new InvalidOperationException(Res.GetString(Res.InstallBadParent));

                if (value == parent) {
                    // nothing to do
                    return;
                }

                if (value != null && InstallerTreeContains(value)) {
                    throw new InvalidOperationException(Res.GetString(Res.InstallRecursiveParent));
                }

                if (parent != null) {
                    int index = parent.Installers.IndexOf(this);
                    if (index != -1)
                        parent.Installers.RemoveAt(index);
                }
                parent = value;
                if (parent != null && !parent.Installers.Contains(this))
                    parent.Installers.Add(this);
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Committed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs after the
        ///       installations handled by the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance are committed.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler Committed {
            add {
                afterCommitHandler += value;
            }
            remove {
                afterCommitHandler -= value;
            }
        }
        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.AfterInstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs after the <see cref='System.Configuration.Install.Installer.Install'/> methods
        ///       of all the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance have run.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler AfterInstall {
            add {
                afterInstallHandler += value;
            }
            remove {
                afterInstallHandler -= value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.AfterRollback"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs after the
        ///       installations handled by the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of
        ///       this instance are rolled back.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler AfterRollback {
            add {
                afterRollbackHandler += value;
            }
            remove {
                afterRollbackHandler -= value;
            }
        }


        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.AfterUninstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs after the
        ///       installations handled by the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance are uninstalled.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler AfterUninstall {
            add {
                afterUninstallHandler += value;
            }
            remove {
                afterUninstallHandler -= value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Committing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs before the
        ///       installations handled by the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance are committed.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler Committing {
            add {
                beforeCommitHandler += value;
            }                                                                                                                    
            remove {
                beforeCommitHandler -= value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.BeforeInstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs before the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance are run.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler BeforeInstall {
            add {
                beforeInstallHandler += value;
            }
            remove {
                beforeInstallHandler -= value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.BeforeRollback"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs before the
        ///       installations handled by the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance are rolled back.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler BeforeRollback {
            add {
                beforeRollbackHandler += value;
            }
            remove {
                beforeRollbackHandler -= value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.BeforeUninstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs before the
        ///       installations handled by the installers in the <see cref='System.Configuration.Install.InstallerCollection'/> of this instance are uninstalled.
        ///    </para>
        /// </devdoc>
        public event InstallEventHandler BeforeUninstall {
            add {
                beforeUninstallHandler += value;
            }
            remove {
                beforeUninstallHandler -= value;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Commit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Completes the install transaction.
        ///    </para>
        /// </devdoc>
        public virtual void Commit(IDictionary savedState) {
            // check arguments
            if (savedState == null)
                throw new ArgumentException(Res.GetString(Res.InstallNullParameter, "savedState"));
            if (savedState["_reserved_lastInstallerAttempted"] == null || savedState["_reserved_nestedSavedStates"] == null)
                throw new ArgumentException(Res.GetString(Res.InstallDictionaryMissingValues, "savedState"));

            Exception savedException = null;
            // raise the OnCommitting event
            try {
                OnCommitting(savedState);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityWarning), "OnCommitting", e);
                Context.LogMessage(Res.GetString(Res.InstallCommitException));
                savedException = e;
            }

            // do the commit

            int lastInstallerAttempted = (int) savedState["_reserved_lastInstallerAttempted"];
            IDictionary[] nestedSavedStates = (IDictionary[]) savedState["_reserved_nestedSavedStates"];
            // do more validation
            if (lastInstallerAttempted + 1 != nestedSavedStates.Length || lastInstallerAttempted >= Installers.Count)
                throw new ArgumentException(Res.GetString(Res.InstallDictionaryCorrupted, "savedState"));
            for (int i = 0; i < Installers.Count; i++) {
                // set all the contexts first.  see note in Install
                Installers[i].Context = Context;
            }
            for (int i = 0; i <= lastInstallerAttempted; i++) {
                try {
                    Installers[i].Commit(nestedSavedStates[i]);
                }
                catch (Exception e) {
                    if (!IsWrappedException(e)) {
                        // only print the message if this is not a wrapper around an exception we already printed out.
                        Context.LogMessage(Res.GetString(Res.InstallLogCommitException, Installers[i].ToString()));
                        LogException(e, Context);
                        Context.LogMessage(Res.GetString(Res.InstallCommitException));
                    }
                    savedException = e;
                }
            }
            // make sure if the nested installers changed thier state, that gets saved out.
            savedState["_reserved_nestedSavedStates"] = nestedSavedStates;
            // no point in keeping this around, since we know they all succeeded.
            savedState.Remove("_reserved_lastInstallerAttempted");

            // raise the OnCommitted event
            try {
                OnCommitted(savedState);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityWarning), "OnCommitted", e);
                Context.LogMessage(Res.GetString(Res.InstallCommitException));
                savedException = e;
            }

            if (savedException != null) {
                Exception wrappedException = savedException;
                if (!IsWrappedException(savedException)) {
                    wrappedException = new InstallException(Res.GetString(Res.InstallCommitException), savedException);
                    wrappedException.Source = wrappedExceptionSource;
                }
                throw wrappedException;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Install"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs the installation.
        ///    </para>
        /// </devdoc>
        public virtual void Install(IDictionary stateSaver) {
            // validate argument
            if (stateSaver == null)
                throw new ArgumentException(Res.GetString(Res.InstallNullParameter, "stateSaver"));

            // raise the OnBeforeInstall event
            try {
                OnBeforeInstall(stateSaver);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityError), "OnBeforeInstall", e);
                throw new InvalidOperationException(Res.GetString(Res.InstallEventException, "OnBeforeInstall", GetType().FullName), e);
            }

            // perform the install
            int lastInstallerAttempted = -1;
            ArrayList savedStates = new ArrayList();
            try {
                for (int i = 0; i < Installers.Count; i++) {
                    // Pass down our context to each of the contained installers.
                    // We set all of the contexts before calling any installers just
                    // in case one contained installer gets another to examine its
                    // context before installation (as in the case of ServiceInstallers).
                    Installers[i].Context = Context;
                }
                for (int i = 0; i < Installers.Count; i++) {
                    Installer installer = Installers[i];
                    // each contained installer gets a new IDictionary to write to. This way
                    // there can be no name conflicts between installers.
                    IDictionary nestedStateSaver = new Hashtable();
                    try {
                        lastInstallerAttempted = i;
                        installer.Install(nestedStateSaver);
                    }
                    finally {
                        savedStates.Add(nestedStateSaver);
                    }
                }
            }
            finally {
                stateSaver.Add("_reserved_lastInstallerAttempted", lastInstallerAttempted);
                stateSaver.Add("_reserved_nestedSavedStates", savedStates.ToArray(typeof(IDictionary)));
            }

            // raise the OnAfterInstall event
            try {
                OnAfterInstall(stateSaver);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityError), "OnAfterInstall", e);
                throw new InvalidOperationException(Res.GetString(Res.InstallEventException, "OnAfterInstall", GetType().FullName), e);
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.LogException"]/*' />
        /// <devdoc>
        /// Writes exception information for the given inner exception and any
        /// inner exceptions it may have to the given context object.
        /// </devdoc>
        internal static void LogException(Exception e, InstallContext context) {
            bool toplevel = true;
            while (e != null) {
                if (toplevel) {
                    context.LogMessage(e.GetType().FullName + ": " + e.Message);
                    toplevel = false;
                }
                else
                    context.LogMessage(Res.GetString(Res.InstallLogInner, e.GetType().FullName, e.Message));
                if (context.IsParameterTrue("showcallstack"))
                    context.LogMessage(e.StackTrace);
                e = e.InnerException;
            }
        }

        private bool IsWrappedException(Exception e) {
            return e is InstallException && e.Source == wrappedExceptionSource && e.TargetSite.ReflectedType == typeof(Installer);
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnCommitted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnCommitted'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnCommitted(IDictionary savedState) {
            if (afterCommitHandler != null)
                afterCommitHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnAfterInstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnAfterInstall'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnAfterInstall(IDictionary savedState) {
            if (afterInstallHandler != null)
                afterInstallHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnAfterRollback"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnAfterRollback'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnAfterRollback(IDictionary savedState) {
            if (afterRollbackHandler != null)
                afterRollbackHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnAfterUninstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnAfterUninstall'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnAfterUninstall(IDictionary savedState) {
            if (afterUninstallHandler != null)
                afterUninstallHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnCommitting"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnCommitting'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnCommitting(IDictionary savedState) {
            if (beforeCommitHandler != null)
                beforeCommitHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnBeforeInstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnBeforeInstall'/>
        ///       event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnBeforeInstall(IDictionary savedState) {
            if (beforeInstallHandler != null)
                beforeInstallHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnBeforeRollback"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnBeforeRollback'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnBeforeRollback(IDictionary savedState) {
            if (beforeRollbackHandler != null)
                beforeRollbackHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.OnBeforeUninstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Configuration.Install.Installer.OnBeforeUninstall'/> event.
        ///    </para>
        /// </devdoc>
        protected virtual void OnBeforeUninstall(IDictionary savedState) {
            if (beforeUninstallHandler != null)
                beforeUninstallHandler(this, new InstallEventArgs(savedState));
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Rollback"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Restores the machine to the state it was in before
        ///       the <see cref='System.Configuration.Install.Installer.Install'/>
        ///       method was called.
        ///    </para>
        /// </devdoc>
        public virtual void Rollback(IDictionary savedState) {
            // check arguments
            if (savedState == null)
                throw new ArgumentException(Res.GetString(Res.InstallNullParameter, "savedState"));
            if (savedState["_reserved_lastInstallerAttempted"] == null || savedState["_reserved_nestedSavedStates"] == null)
                throw new ArgumentException(Res.GetString(Res.InstallDictionaryMissingValues, "savedState"));

            // raise the OnBeforeRollback event
            Exception savedException = null;
            try {
                OnBeforeRollback(savedState);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityWarning), "OnBeforeRollback", e);
                Context.LogMessage(Res.GetString(Res.InstallRollbackException));
                savedException = e;
            }

            // do the rollback

            int lastInstallerAttempted = (int) savedState["_reserved_lastInstallerAttempted"];
            IDictionary[] nestedSavedStates = (IDictionary[]) savedState["_reserved_nestedSavedStates"];
            // do more validation
            if (lastInstallerAttempted + 1 != nestedSavedStates.Length || lastInstallerAttempted >= Installers.Count)
                throw new ArgumentException(Res.GetString(Res.InstallDictionaryCorrupted, "savedState"));
            for (int i = Installers.Count - 1; i >= 0; i--) {
                // set all the contexts first.  see note in Install
                Installers[i].Context = Context;
            }
            for (int i = lastInstallerAttempted; i >= 0; i--) {
                try {
                    Installers[i].Rollback(nestedSavedStates[i]);
                }
                catch (Exception e) {
                    if (!IsWrappedException(e)) {
                        // only print the message if this is not a wrapper around an exception we already printed out.
                        Context.LogMessage(Res.GetString(Res.InstallLogRollbackException, Installers[i].ToString()));
                        LogException(e, Context);
                        Context.LogMessage(Res.GetString(Res.InstallRollbackException));
                    }
                    savedException = e;
                }
            }
            // no point in doing anything to the saved state, because it'll all be
            // thrown away at the top anyway.

            // raise the OnAfterRollback event
            try {
                OnAfterRollback(savedState);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityWarning), "OnAfterRollback", e);
                Context.LogMessage(Res.GetString(Res.InstallRollbackException));
                savedException = e;
            }

            if (savedException != null) {
                Exception wrappedException = savedException;
                if (!IsWrappedException(savedException)) {
                    wrappedException = new InstallException(Res.GetString(Res.InstallRollbackException), savedException);
                    wrappedException.Source = wrappedExceptionSource;
                }
                throw wrappedException;
            }
        }

        /// <include file='doc\Installer.uex' path='docs/doc[@for="Installer.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes an installation.
        ///    </para>
        /// </devdoc>
        public virtual void Uninstall(IDictionary savedState) {
            // raise the OnBeforeUninstall event
            Exception savedException = null;
            try {
                OnBeforeUninstall(savedState);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityWarning), "OnBeforeUninstall", e);
                Context.LogMessage(Res.GetString(Res.InstallUninstallException));
                savedException = e;
            }

            // do the uninstall

            // uninstall is special: savedState can be null. (The state file may have been deleted since
            // the application was installed.) If it isn't, we read state out as usual. Otherwise we create
            // some new, empty state to pass to the contained installers.
            IDictionary[] nestedSavedStates;
            if (savedState != null) {
                nestedSavedStates = (IDictionary[]) savedState["_reserved_nestedSavedStates"];
                if (nestedSavedStates == null || nestedSavedStates.Length != Installers.Count)
                    throw new ArgumentException(Res.GetString(Res.InstallDictionaryCorrupted, "savedState"));
            }
            else
                nestedSavedStates = new IDictionary[Installers.Count];
            //go in reverse order when uninstalling
            for (int i = Installers.Count - 1; i >= 0; i--) {
                // set all the contexts first.  see note in Install
                Installers[i].Context = Context;
            }

            for (int i = Installers.Count - 1; i >= 0; i--) {
                try {
                    Installers[i].Uninstall(nestedSavedStates[i]);
                }
                catch (Exception e) {
                    if (!IsWrappedException(e)) {
                        // only print the message if this is not a wrapper around an exception we already printed out.
                        Context.LogMessage(Res.GetString(Res.InstallLogUninstallException, Installers[i].ToString()));
                        LogException(e, Context);
                        Context.LogMessage(Res.GetString(Res.InstallUninstallException));
                    }
                    savedException  = e;
                }
            }

            // raise the OnAfterUninstall event
            try {
                OnAfterUninstall(savedState);
            }
            catch (Exception e) {
                WriteEventHandlerError(Res.GetString(Res.InstallSeverityWarning), "OnAfterUninstall", e);
                Context.LogMessage(Res.GetString(Res.InstallUninstallException));
                savedException = e;
            }

            if (savedException != null) {
                Exception wrappedException = savedException;
                if (!IsWrappedException(savedException)) {
                    wrappedException = new InstallException(Res.GetString(Res.InstallUninstallException), savedException);
                    wrappedException.Source = wrappedExceptionSource;
                }
                throw wrappedException;
            }
        }

        private void WriteEventHandlerError(string severity, string eventName, Exception e) {
            Context.LogMessage(Res.GetString(Res.InstallLogError, severity, eventName, GetType().FullName));
            LogException(e, Context);
        }
    }

    internal class InstallerParentConverter : ReferenceConverter {

        public InstallerParentConverter(Type type) : base(type) {
        }

        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            StandardValuesCollection baseValues = base.GetStandardValues(context);

            object component = context.Instance;
            int sourceIndex = 0, targetIndex = 0;
            // we want to return the same list, but with the current component removed.
            // (You can't set an installer's parent to itself.)
            // optimization: assume the current component will always be in the list.
            object[] newValues = new object[baseValues.Count - 1];
            while (sourceIndex < baseValues.Count) {
                if (baseValues[sourceIndex] != component) {
                    newValues[targetIndex] = baseValues[sourceIndex];
                    targetIndex++;
                }
                sourceIndex++;
            }

            return new StandardValuesCollection(newValues);
        }
    }


}

