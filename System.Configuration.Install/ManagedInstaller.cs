//------------------------------------------------------------------------------
// <copyright file="ManagedInstaller.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {
    using System.Runtime.Remoting;

    using System.Diagnostics;

    using System;
    using System.Reflection;
    using System.Configuration;
    using System.Configuration.Install;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Globalization;
    
    /// <include file='doc\ManagedInstaller.uex' path='docs/doc[@for="ManagedInstallerClass"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [ComVisible(true), GuidAttribute("42EB0342-0393-448f-84AA-D4BEB0283595")]
    public class ManagedInstallerClass : IManagedInstaller {                            

        /// <include file='doc\ManagedInstaller.uex' path='docs/doc[@for="ManagedInstallerClass.IManagedInstaller.ManagedInstall"]/*' />
        /// <internalonly/>
        int IManagedInstaller.ManagedInstall(string argString, int hInstall) {
            try {
                string[] args = StringToArgs(argString);
                InstallHelper(args);
            }
            catch (Exception e) {
                Exception temp = e;
                StringBuilder msg = new StringBuilder();
                while (temp != null) {
                    msg.Append(temp.Message);
                    temp = temp.InnerException;
                    if (temp != null)
                        msg.Append(" --> ");
                }

                int hRecord = NativeMethods.MsiCreateRecord(2);
                if (hRecord != 0) {
                    int err = NativeMethods.MsiRecordSetInteger(hRecord, 1, 1001 ); // 1 = Error, 2 = Warning
                    if (err == 0) {
                        err = NativeMethods.MsiRecordSetStringW(hRecord, 2, msg.ToString());
                        if (err == 0) {
                            NativeMethods.MsiProcessMessage(hInstall, NativeMethods.INSTALLMESSAGE_ERROR, hRecord);
                        }
                    }
                }
                
                return -1;
            }
            
            return 0;
        }

        /// <include file='doc\ManagedInstaller.uex' path='docs/doc[@for="ManagedInstallerClass.InstallHelper"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void InstallHelper(string[] args) { 
            bool uninstall = false;
            bool isAssemblyName = false;
            TransactedInstaller topLevelInstaller = new TransactedInstaller();
            bool showHelp = false;

            try {

                /*
                StreamWriter stream = new StreamWriter("c:\\installutilargs.txt", true);
                stream.WriteLine("----------");
                for (int i = 0; i < args.Length; i++)
                    stream.WriteLine(args[i]);
                stream.Close();
                */
                

                // strategy: Use a TransactedInstaller to manage the top-level installation work.
                // It will perform rollback/commit as necessary. Go through the assemblies on the
                // command line and add an AssemblyInstaller for each of them to the TransactedInstaller's
                // Installers collection.
                //
                // as we walk the parameters, we'll encounter either a filename or a
                // parameter. If we get to a filename, create an assembly installer with
                // all of the parameters we've seen _so far_. This way parameters can
                // be different for the different assemblies.

                ArrayList parameters = new ArrayList();
                for (int i = 0; i < args.Length; i++) {
                    if (args[i].StartsWith("/", StringComparison.Ordinal) || args[i].StartsWith("-", StringComparison.Ordinal)) {
                        string str = args[i].Substring(1);
                        if (string.Compare(str, "u", StringComparison.OrdinalIgnoreCase) == 0 || string.Compare(str, "uninstall", StringComparison.OrdinalIgnoreCase) == 0)
                            uninstall = true;
                        else if (string.Compare(str, "?", StringComparison.OrdinalIgnoreCase) == 0 || string.Compare(str, "help", StringComparison.OrdinalIgnoreCase) == 0)
                            showHelp = true;
                        else if (string.Compare(str, "AssemblyName", StringComparison.OrdinalIgnoreCase) == 0)
                            isAssemblyName = true;
                        else                             
                            parameters.Add(args[i]);
                    }
                    else {
                        Assembly asm = null;
                        try {
                            if (isAssemblyName) {
                                asm = Assembly.Load(args[i]);
                            }
                            else {
                                asm = Assembly.LoadFrom(args[i]);
                            }
                        }
                        catch (Exception e) {
                            if (args[i].IndexOf('=') != -1) {
                                // probably a mistake where /key=value was written as key=value,
                                // try to explain that
                                throw new ArgumentException(Res.GetString(Res.InstallFileDoesntExistCommandLine, args[i]), e);
                            }
                            else {
                                // the assembly.Load{From} gives a good descriptive error - pass it up
                                throw;
                            }
                        }

                        AssemblyInstaller installer = new AssemblyInstaller(asm, (string[]) parameters.ToArray(typeof(string)));
                        topLevelInstaller.Installers.Add(installer);
                    }
                }

                if (showHelp || topLevelInstaller.Installers.Count == 0) {
                    // we may have seen some options, but they didn't tell us to do any
                    // work. Or they gave us /? or /help. Show the help screen.
                    showHelp = true;
                    topLevelInstaller.Installers.Add(new AssemblyInstaller());
                    throw new InvalidOperationException(GetHelp(topLevelInstaller));
                }

                topLevelInstaller.Context = new InstallContext("InstallUtil.InstallLog", (string[]) parameters.ToArray(typeof(string)));

            }
            catch (Exception e) {
                if (showHelp) {
                    // it's just the help message
                    throw e;
                }
                else {
                    throw new InvalidOperationException(Res.GetString(Res.InstallInitializeException, e.GetType().FullName, e.Message));
                }
            }

            try {
                // MSI mode.
                // If the parameter /installtype=notransaction is specified, then we don't want to run
                // a TransactedInstaller. Instead, we just use that installer as a container for all
                // the AssemblyInstallers we want to run.
                string installType = topLevelInstaller.Context.Parameters["installtype"];
                if (installType != null && string.Compare(installType, "notransaction", StringComparison.OrdinalIgnoreCase) == 0) {
                    // this is a non-transacted install. Check the value of the Action parameter
                    // to see what to do
                    string action = topLevelInstaller.Context.Parameters["action"];
                    if (action != null && string.Compare(action, "rollback", StringComparison.OrdinalIgnoreCase) == 0) {
                        topLevelInstaller.Context.LogMessage( Res.GetString(Res.InstallRollbackNtRun) );
                        for (int i = 0; i < topLevelInstaller.Installers.Count; i++)
                            topLevelInstaller.Installers[i].Rollback(null);
                        return;
                    }
                    if (action != null && string.Compare(action, "commit", StringComparison.OrdinalIgnoreCase) == 0) {
                        topLevelInstaller.Context.LogMessage( Res.GetString(Res.InstallCommitNtRun) );
                        for (int i = 0; i < topLevelInstaller.Installers.Count; i++)
                            topLevelInstaller.Installers[i].Commit(null);
                        return;
                    }
                    if (action != null && string.Compare(action, "uninstall", StringComparison.OrdinalIgnoreCase) == 0) {
                        topLevelInstaller.Context.LogMessage( Res.GetString(Res.InstallUninstallNtRun) );
                        for (int i = 0; i < topLevelInstaller.Installers.Count; i++)
                            topLevelInstaller.Installers[i].Uninstall(null);
                        return;
                    }
                    // they said notransaction, and they didn't tell us to do rollback, commit,
                    // or uninstall. They must mean install.
                    topLevelInstaller.Context.LogMessage( Res.GetString(Res.InstallInstallNtRun) );
                    for (int i = 0; i < topLevelInstaller.Installers.Count; i++)
                        topLevelInstaller.Installers[i].Install(null);
                    return;

                }

                // transacted mode - we'll only get here if /installtype=notransaction wasn't specified.
                if (!uninstall) {
                    IDictionary stateSaver = new Hashtable();
                    topLevelInstaller.Install(stateSaver);
                    // we don't bother writing out the saved state for this guy, because each assembly
                    // we're installing gets its own saved-state file.
                }
                else {
                    topLevelInstaller.Uninstall(null);
                }

            }

            catch (Exception e) {
                /*
                StreamWriter stream = new StreamWriter("c:\\installutilargs.txt", true);
                stream.WriteLine("Caught exception: " + e.GetType().FullName + ": " + e.Message);
                stream.WriteLine(e.StackTrace);
                stream.Close();
                */

                throw e;
            }

            /*
            StreamWriter stream2 = new StreamWriter("c:\\installutilargs.txt", true);
            stream2.WriteLine("Caught no exceptions. Returning 0.");
            stream2.Close();
            */

            return;
        }

        private static string GetHelp(Installer installerWithHelp) {
            return Res.GetString(Res.InstallHelpMessageStart)   + Environment.NewLine + 
                   installerWithHelp.HelpText                   + Environment.NewLine +
                   Res.GetString(Res.InstallHelpMessageEnd)     + Environment.NewLine;
        }

        static string[] StringToArgs(string cmdLine) {
            ArrayList args = new ArrayList();
            StringBuilder nextArg = null;
            bool inString = false;
            bool escaped = false;
            
            for (int pos = 0; pos < cmdLine.Length; pos++) {
                char ch = cmdLine[pos];            
    
                if (nextArg == null) {
                    if (Char.IsWhiteSpace(ch)) {
                        continue;
                    } else {
                        nextArg = new StringBuilder();
                    }
                }
     
                if (inString) {
                    if (escaped) {
                        if (ch != '\\' && ch != '\"')                 
                            nextArg.Append('\\');                     
                        escaped = false;
                        nextArg.Append(ch);  
                    } else if (ch == '\"') {
                        inString = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else {
                        nextArg.Append(ch);
                    }
                }
                else {
                    if (Char.IsWhiteSpace(ch)) {
                        args.Add(nextArg.ToString());
                        nextArg = null;              
                        escaped = false;      
                    } else if (escaped) {
                        nextArg.Append(ch);
                        escaped = false;
                    } else if (ch == '^') {
                        escaped = true;
                    } else if (ch == '\"') {
                        inString = true;
                    } else {
                        nextArg.Append(ch);
                    }                
                }
                
            }
    
            if (nextArg != null)
                args.Add(nextArg.ToString());
    
            string[] argsArray = new string[args.Count];
            args.CopyTo(argsArray);
    
            return argsArray;
        }

    }

}
