//------------------------------------------------------------------------------
// <copyright file="AssemblyInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using System.IO;
    using System.Reflection;
    using System.Collections;    
    using System.ComponentModel;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Xml;

    /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller"]/*' />
    /// <devdoc>
    ///    Loads an assembly, finds all installers in it, and runs them.
    /// </devdoc>
    public class AssemblyInstaller : Installer {

        private Assembly assembly;
        private string[] commandLine;
        private bool useNewContext;
        private static bool helpPrinted = false;
		private bool initialized = false;
        
        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.AssemblyInstaller"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AssemblyInstaller() : base() {
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.AssemblyInstaller1"]/*' />
        /// <devdoc>
        ///    Creates a new AssemblyInstaller for the given assembly and passes
        ///    the given command line arguments to its Context object.
        /// </devdoc>
        public AssemblyInstaller(string fileName, string[] commandLine) : base() {
            this.Path = System.IO.Path.GetFullPath(fileName);
            this.commandLine = commandLine;
            this.useNewContext = true;
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.AssemblyInstaller2"]/*' />
        /// <devdoc>
        ///    Creates a new AssemblyInstaller for the given assembly and passes
        ///    the given command line arguments to its Context object.
        /// </devdoc>
        public AssemblyInstaller(Assembly assembly, string[] commandLine) : base() {
            this.Assembly = assembly;
            this.commandLine = commandLine;
            this.useNewContext = true;
        }
        
        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Assembly"]/*' />
        /// <devdoc>
        ///      The actual assembly that will be installed
        /// </devdoc>
        [ResDescription("Desc_AssemblyInstaller_Assembly")]
		public Assembly Assembly {
            get {
                return this.assembly;
            }
            set {
                this.assembly = value;
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.CommandLine"]/*' />
        /// <devdoc>
        ///    The command line to use when creating a new context for the assembly's install.
        /// </devdoc>
        [ResDescription("Desc_AssemblyInstaller_CommandLine")]
		public string[] CommandLine {
            get {
                return commandLine;
            }
            set {
                commandLine = value;
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.HelpText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string HelpText {
            get {
                if (Path != null && Path.Length > 0) {
                    Context = new InstallContext(null, new string[0]);
					if(!initialized)
						InitializeFromAssembly();
                }
                if (helpPrinted)
                    return base.HelpText;
                else  {
                    helpPrinted = true;
                    return Res.GetString(Res.InstallAssemblyHelp) + "\r\n" + base.HelpText;
                }
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Path"]/*' />
        /// <devdoc>
        ///    The path to the assembly to install
        /// </devdoc>
        [ResDescription("Desc_AssemblyInstaller_Path")]
		public string Path {
            get {
                if (this.assembly == null) {
                    return null;
                }
                else {
                    return this.assembly.Location;
                }
            }
            set {           
                if (value == null) {
                    this.assembly = null;
                }
                this.assembly = Assembly.LoadFrom(value);
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.UseNewContext"]/*' />
        /// <devdoc>
        ///    If true, creates a new InstallContext object for the assembly's install.
        /// </devdoc>
        [ResDescription("Desc_AssemblyInstaller_UseNewContext")]
		public bool UseNewContext {
            get {
                return useNewContext;
            }
            set {
                useNewContext = value;
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.CheckIfInstallable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Finds the installers in the specified assembly, creates
        ///       a new instance of <see cref='System.Configuration.Install.AssemblyInstaller'/>
        ///       , and adds the installers to its installer collection.
        ///    </para>
        /// </devdoc>
        public static void CheckIfInstallable(string assemblyName) {
            AssemblyInstaller tester = new AssemblyInstaller();
            tester.UseNewContext = false;
            tester.Path = assemblyName;
            tester.CommandLine = new string[0];
            tester.Context = new InstallContext(null, new string[0]);

            // this does the actual check and throws if necessary.
            tester.InitializeFromAssembly();
            if (tester.Installers.Count == 0)
                throw new InvalidOperationException(Res.GetString(Res.InstallNoPublicInstallers, assemblyName));
        }

        private InstallContext CreateAssemblyContext() {
            InstallContext context = new InstallContext(System.IO.Path.ChangeExtension(Path, ".InstallLog"), CommandLine);
                
			if(this.Context != null)
				context.Parameters["logtoconsole"] = this.Context.Parameters["logtoconsole"];
            context.Parameters["assemblypath"] = Path;
            return context;
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.InitializeFromAssembly"]/*' />
        /// <devdoc>
        /// this is common code that's called from Install, Commit, Rollback, and Uninstall. It
        /// loads the assembly, finds all Installer types in it, and adds them to the Installers
        /// collection. It also prints some useful information to the Context log.
        /// </devdoc>
        private void InitializeFromAssembly() {
            // Get the set of installers to use out of the assembly. This will load the assembly
            // so that its types are accessible later. An assembly cannot be unloaded. 

            Type[] installerTypes = null;
            try {
                installerTypes = GetInstallerTypes(assembly);
            }
            catch (Exception e) {
                Context.LogMessage(Res.GetString(Res.InstallException, Path));
                Installer.LogException(e, Context);
                Context.LogMessage(Res.GetString(Res.InstallAbort, Path));
                throw new InvalidOperationException(Res.GetString(Res.InstallNoInstallerTypes, Path), e);
            }

            if (installerTypes == null || installerTypes.Length == 0) {
                Context.LogMessage(Res.GetString(Res.InstallNoPublicInstallers, Path));
                // this is not an error, so don't throw. Just don't do anything.
                return;
            }

            // create instances of each of those, and add them to the Installers collection.
            for (int i = 0; i < installerTypes.Length; i++) {
                try {
                    Installer installer = (Installer) Activator.CreateInstance(installerTypes[i], 
                                                                                                    BindingFlags.Instance | 
                                                                                                    BindingFlags.Public | 
                                                                                                    BindingFlags.CreateInstance, 
                                                                                                    null, 
                                                                                                    new object[0],
                                                                                                    null);                    
                    Installers.Add(installer);
                }
                catch (Exception e) {
                    Context.LogMessage(Res.GetString(Res.InstallCannotCreateInstance, installerTypes[i].FullName));
                    Installer.LogException(e, Context);
                    throw new InvalidOperationException(Res.GetString(Res.InstallCannotCreateInstance, installerTypes[i].FullName), e);
                }
            }
			initialized = true;
        }

        // returns the path of the installstate file, based on the path of the assembly
        private string GetInstallStatePath(String assemblyPath) {
            string returnPath;
            string installStateDir = Context.Parameters["InstallStateDir"];

            // never use the passed-in savedState. Always look next to the assembly, unless the parameter "InstallStateDir" is specified.
            assemblyPath = System.IO.Path.ChangeExtension(assemblyPath, ".InstallState");

            if (!String.IsNullOrEmpty(installStateDir))
            {
                returnPath = System.IO.Path.Combine(installStateDir,System.IO.Path.GetFileName(assemblyPath));
            }
            else
            {
                returnPath = assemblyPath;
            }

            return returnPath;
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Commit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Commit(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityCommitting));
			if(!initialized)
				InitializeFromAssembly();

            // Get the location of the InstallState file
            string filePath = GetInstallStatePath(Path);

            FileStream file = new FileStream(filePath, FileMode.Open, FileAccess.Read);

            XmlReaderSettings xmlReaderSettings = new XmlReaderSettings();
            xmlReaderSettings.CheckCharacters = false;
            xmlReaderSettings.CloseInput = false; // We'll close the filestream ourselves

            XmlReader xmlFile = null;
            if (null != file)
            {
                xmlFile = XmlReader.Create(file, xmlReaderSettings);
            }

            try {
                if (null != xmlFile)
                {
                    NetDataContractSerializer serializer = new NetDataContractSerializer();
                    savedState = (Hashtable)serializer.ReadObject(xmlFile);
                }
            }
            finally {
                if (null != xmlFile)
                {
                    xmlFile.Close();
                }
                if (null != file)
                {
                    file.Close();
                }
                if (Installers.Count == 0) {
                    Context.LogMessage(Res.GetString(Res.RemovingInstallState));
                    File.Delete(filePath);
                } 
            }

            base.Commit(savedState);
        }

        // returns the set of types that implement Installer in the given assembly
        private static Type[] GetInstallerTypes(Assembly assem) {
            ArrayList typeList = new ArrayList();            
            
            Module[] mods = assem.GetModules();
            for (int i = 0; i < mods.Length; i++) {
                Type[] types = mods[i].GetTypes();
                for (int j = 0; j < types.Length; j++) {
                #if DEBUG
                if (CompModSwitches.InstallerDesign.TraceVerbose) {
                    Debug.WriteLine("Looking at type '" + types[j].FullName + "'");
                    Debug.WriteLine("  Is it an installer? " + (typeof(Installer).IsAssignableFrom(types[j]) ? "Yes" : "No"));
                    Debug.WriteLine("  Is it abstract? " + (types[j].IsAbstract ? "Yes" : "No"));
                    Debug.WriteLine("  Is it public? " + (types[j].IsPublic ? "Yes" : "No"));
                    Debug.WriteLine("  Does it have the RunInstaller attribute? " + (((RunInstallerAttribute) TypeDescriptor.GetAttributes(types[j])[typeof(RunInstallerAttribute)]).RunInstaller ? "Yes" : "No"));
                }
                #endif
                    if (typeof(Installer).IsAssignableFrom(types[j]) && !types[j].IsAbstract && types[j].IsPublic
                        && ((RunInstallerAttribute) TypeDescriptor.GetAttributes(types[j])[typeof(RunInstallerAttribute)]).RunInstaller)
                        typeList.Add(types[j]);
                }
            }

            return (Type[]) typeList.ToArray(typeof(Type));
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Install"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Install(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityInstalling));
			if(!initialized)
				InitializeFromAssembly();

            Hashtable savedStateHash = new Hashtable();
            savedState = savedStateHash;

            try {
                // Do the install
                base.Install(savedState);
            }
            finally {
                // and write out the results
                FileStream file = new FileStream(GetInstallStatePath(Path), FileMode.Create);

                XmlWriterSettings xmlWriteSettings = new XmlWriterSettings();
                xmlWriteSettings.Encoding = Encoding.UTF8;
                xmlWriteSettings.CheckCharacters = false;
                xmlWriteSettings.CloseOutput = false; // We'll close the filestream ourselves

                XmlWriter xmlFile = XmlWriter.Create(file, xmlWriteSettings);
                try {
                    NetDataContractSerializer serializer = new NetDataContractSerializer();
                    serializer.WriteObject(xmlFile, savedState);
                }
                finally {
                    xmlFile.Close();
                    file.Close();
                }
            }
        }

        private void PrintStartText(string activity) {
            if (UseNewContext) {
                InstallContext newContext = CreateAssemblyContext();
                // give a warning in the main log file that we're switching over to the assembly-specific file
                if (Context != null) {
                    Context.LogMessage(Res.GetString(Res.InstallLogContent, Path));
                    Context.LogMessage(Res.GetString(Res.InstallFileLocation, newContext.Parameters["logfile"]));
                }
                Context = newContext;
            }

            // print out some info on the install
            Context.LogMessage(string.Format(System.Globalization.CultureInfo.InvariantCulture, activity, Path));
            Context.LogMessage(Res.GetString(Res.InstallLogParameters));
            if (Context.Parameters.Count == 0)
                Context.LogMessage("   " + Res.GetString(Res.InstallLogNone));
            IDictionaryEnumerator en = (IDictionaryEnumerator) Context.Parameters.GetEnumerator();
            while (en.MoveNext())
            {
                string key = (string)en.Key;
                string value = (string)en.Value;

                // hide password parameters
                if (key.Equals("password", StringComparison.InvariantCultureIgnoreCase))
                {
                    value = "********";
                }

                Context.LogMessage("   " + key + " = " + value);
            }
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Rollback"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Rollback(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityRollingBack));
			if(!initialized)
				InitializeFromAssembly();
            // Get the location of the InstallState file
            string filePath = GetInstallStatePath(Path);

            FileStream file = new FileStream(filePath, FileMode.Open, FileAccess.Read);

            XmlReaderSettings xmlReaderSettings = new XmlReaderSettings();
            xmlReaderSettings.CheckCharacters = false;
            xmlReaderSettings.CloseInput = false; // We'll close the filestream ourselves

            XmlReader xmlFile = null;

            if (null != file)
            {
                xmlFile = XmlReader.Create(file, xmlReaderSettings);
            }
            try {
                if (null != xmlFile)
                {
                    NetDataContractSerializer serializer = new NetDataContractSerializer();
                    savedState = (Hashtable)serializer.ReadObject(xmlFile);
                }
            }
            finally {
                if (null != xmlFile)
                {
                    xmlFile.Close();
                }
                if (null != file)
                {
                    file.Close();
                }
            }

            try {
                base.Rollback(savedState);
            }                
            finally {
                File.Delete(filePath);
            }                
        }

        /// <include file='doc\AssemblyInstaller.uex' path='docs/doc[@for="AssemblyInstaller.Uninstall"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Uninstall(IDictionary savedState) {
            PrintStartText(Res.GetString(Res.InstallActivityUninstalling));
            if(!initialized)
				InitializeFromAssembly();
            // Get the location of the InstallState file
            string filePath = GetInstallStatePath(Path);
            if (filePath != null && File.Exists(filePath)) {
                FileStream file = new FileStream(filePath, FileMode.Open, FileAccess.Read);

                XmlReaderSettings xmlReaderSettings = new XmlReaderSettings();
                xmlReaderSettings.CheckCharacters = false;
                xmlReaderSettings.CloseInput = false; // We'll close the filestream ourselves

                XmlReader xmlFile = null;

                if (null != file)
                {
                    xmlFile = XmlReader.Create(file, xmlReaderSettings);
                }
                try {
                    if (null != xmlFile)
                    {
                        NetDataContractSerializer serializer = new NetDataContractSerializer();
                        savedState = (Hashtable)serializer.ReadObject(xmlFile);
                    }
                }
                catch {
                    Context.LogMessage(Res.GetString(Res.InstallSavedStateFileCorruptedWarning, Path, filePath));
                    savedState = null;
                }
                finally {
                    if (null != xmlFile)
                    {
                        xmlFile.Close();
                    }
                    if (null != file)
                    {
                        file.Close();
                    }
                }
            }
            else
                 savedState = null;

            base.Uninstall(savedState);

            if (filePath != null && filePath.Length != 0)
                try {
                    File.Delete(filePath);
                }
                catch {
                    //Throw an exception if we can't delete the file (but we were able to read from it)                    
                    throw new InvalidOperationException(Res.GetString(Res.InstallUnableDeleteFile, filePath));
                }
        }
    }
}
