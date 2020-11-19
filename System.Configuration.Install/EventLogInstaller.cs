//------------------------------------------------------------------------------
// <copyright file="EventLogInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 */
namespace System.Diagnostics {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using Microsoft.Win32;
    using System.Configuration.Install;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;

    /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller"]/*' />
    /// <devdoc>
    /// This class acts as an installer for the EventLog component. Essentially, it calls
    /// EventLog.CreateEventSource.
    /// </devdoc>
    public class EventLogInstaller : ComponentInstaller {

        private EventSourceCreationData sourceData = new EventSourceCreationData(null, null);
        private UninstallAction uninstallAction = System.Configuration.Install.UninstallAction.Remove;

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.CategoryResourceFile"]/*' />
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        Editor("System.Windows.Forms.Design.FileNameEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
        ComVisible(false),
        ResDescription(Res.Desc_CategoryResourceFile)
        ]
        public string CategoryResourceFile {
            get {
                return sourceData.CategoryResourceFile;
            }
            set {
                sourceData.CategoryResourceFile = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.CategoryCount"]/*' />
        [
        ComVisible(false),
        ResDescription(Res.Desc_CategoryCount)
        ]
        public int CategoryCount {
            get {
                return sourceData.CategoryCount;
            }
            set {
                sourceData.CategoryCount = value;
            }
        }


        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Log"]/*' />
        /// <devdoc>
        /// The log in which the source will be created
        /// </devdoc>
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        ResDescription(Res.Desc_Log)
        ]
        public string Log {
            get {
                if (sourceData.LogName == null && sourceData.Source != null)
                    // they've told us a source, but they haven't told us a log name.
                    // try to deduce the log name from the source name.
                    sourceData.LogName = EventLog.LogNameFromSourceName(sourceData.Source, ".");
                return sourceData.LogName;
            }
            set {
                sourceData.LogName = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.MessageResourceFile"]/*' />
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        Editor("System.Windows.Forms.Design.FileNameEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
        ComVisible(false),
        ResDescription(Res.Desc_MessageResourceFile)
        ]
        public string MessageResourceFile {
            get {
                return sourceData.MessageResourceFile;
            }
            set {
                sourceData.MessageResourceFile = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.ParameterResourceFile"]/*' />
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        Editor("System.Windows.Forms.Design.FileNameEditor, " + AssemblyRef.SystemDesign, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing),
        ComVisible(false),
        ResDescription(Res.Desc_ParameterResourceFile)
        ]
        public string ParameterResourceFile {
            get {
                return sourceData.ParameterResourceFile;
            }
            set {
                sourceData.ParameterResourceFile = value;
            }
        }


        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Source"]/*' />
        /// <devdoc>
        /// The source to be created
        /// </devdoc>
        [
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        ResDescription(Res.Desc_Source)
        ]
        public string Source {
            get {
                return sourceData.Source;
            }
            set {
                sourceData.Source = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.UninstallAction"]/*' />
        /// <devdoc>
        /// Determines whether the event log is removed at uninstall time.
        /// </devdoc>
        [DefaultValue(UninstallAction.Remove),
        ResDescription(Res.Desc_UninstallAction)
        ]
        public UninstallAction UninstallAction {
            get {
                return uninstallAction;
            }
            set {
                if (!Enum.IsDefined(typeof(UninstallAction), value))
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(UninstallAction));

                uninstallAction = value;
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.CopyFromComponent"]/*' />
        /// <devdoc>
        /// A method on ComponentInstaller that lets us copy properties.
        /// </devdoc>
        public override void CopyFromComponent(IComponent component) {
            EventLog log = component as EventLog;

            if (log == null)
                throw new ArgumentException(Res.GetString(Res.NotAnEventLog));

            if (log.Log == null || log.Log == string.Empty || log.Source == null || log.Source == string.Empty) {
                throw new ArgumentException(Res.GetString(Res.IncompleteEventLog));
            }

            Log = log.Log;
            Source = log.Source;
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Install"]/*' />
        /// <devdoc>
        /// Called when we should perform the install. Inherited from Installer.
        /// </devdoc>
        public override void Install(IDictionary stateSaver) {
            base.Install(stateSaver);

            Context.LogMessage(Res.GetString(Res.CreatingEventLog, Source, Log));

            if (Environment.OSVersion.Platform != PlatformID.Win32NT) {
                throw new PlatformNotSupportedException(Res.GetString(Res.WinNTRequired));
            }

            stateSaver["baseInstalledAndPlatformOK"] = true;

            // remember whether the log was already there and if the source was already registered
            bool logExists = EventLog.Exists(Log, ".");
            stateSaver["logExists"] = logExists;


            bool alreadyRegistered = EventLog.SourceExists(Source, ".");
            stateSaver["alreadyRegistered"] = alreadyRegistered;

            if (alreadyRegistered) {
                string oldLog = EventLog.LogNameFromSourceName(Source, ".");
                if (oldLog == Log) {
                    // The source exists, and it's on the right log, so we do nothing
                    // here.  If oldLog != Log, we'll try to create the source below
                    // and it will fail, because the source already exists on another
                    // log.
                    return;
                }
            }

            // do the installation.
            EventLog.CreateEventSource(sourceData);
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.IsEquivalentInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsEquivalentInstaller(ComponentInstaller otherInstaller) {
            EventLogInstaller other = otherInstaller as EventLogInstaller;
            if (other == null)
                return false;

            return other.Source == Source;
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Rollback"]/*' />
        /// <devdoc>
        /// Called when this or another component in the installation has failed.
        /// </devdoc>
        public override void Rollback(IDictionary savedState) {
            base.Rollback(savedState);

            Context.LogMessage(Res.GetString(Res.RestoringEventLog, Source));

            if (savedState["baseInstalledAndPlatformOK"] != null) {
                bool logExists = (bool) savedState["logExists"];
                if (!logExists)
                    EventLog.Delete(Log, ".");
                else {
                    bool alreadyRegistered;
                    object alreadyRegisteredObj = savedState["alreadyRegistered"];
                    if (alreadyRegisteredObj == null)
                        alreadyRegistered = false;
                    else
                        alreadyRegistered = (bool) alreadyRegisteredObj;
                    
                    if (!alreadyRegistered) {
                        // delete the source we installed, assuming it succeeded. Then put back whatever used to be there.
                        if (EventLog.SourceExists(Source, "."))
                            EventLog.DeleteEventSource(Source, ".");
                    }
                }
            }
        }

        /// <include file='doc\EventLogInstaller.uex' path='docs/doc[@for="EventLogInstaller.Uninstall"]/*' />
        /// <devdoc>
        /// Called to remove the event log source from the machine.
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            base.Uninstall(savedState);
            if (UninstallAction == UninstallAction.Remove) {
                Context.LogMessage(Res.GetString(Res.RemovingEventLog, Source));
                if (EventLog.SourceExists(Source, ".")) {
                    if ( string.Compare(Log, Source, StringComparison.OrdinalIgnoreCase) != 0 ) // If log has the same name, don't delete the source.
                        EventLog.DeleteEventSource(Source, ".");
                }
                else
                    Context.LogMessage(Res.GetString(Res.LocalSourceNotRegisteredWarning, Source));

                // now test to see if the log has any more sources in it. If not, we
                // should remove the log entirely.
                // we have to do this by inspecting the registry.
                RegistryKey key = Registry.LocalMachine;
                RegistryKey logKey = null;
                try {
                    key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", false);
                    if (key != null) 
                        logKey = key.OpenSubKey(Log, false);
                    if (logKey != null) {
                        string[] keyNames = logKey.GetSubKeyNames();
                        if ( keyNames == null || keyNames.Length == 0 ||
                             (keyNames.Length == 1 && string.Compare(keyNames[0], Log, StringComparison.OrdinalIgnoreCase) ==0) // the only key has the same name as log
                           ) {
                            Context.LogMessage(Res.GetString(Res.DeletingEventLog, Log));
                            // there are no sources in this log. Delete the log.
                            EventLog.Delete(Log, ".");
                        }
                    }
                }
                finally {
                    if (key != null)
                        key.Close();
                    if (logKey != null)
                        logKey.Close();
                }
            }
            // otherwise it's UninstallAction.NoAction, so we shouldn't do anything.
        }

    }
}
