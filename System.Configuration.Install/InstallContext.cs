//------------------------------------------------------------------------------
// <copyright file="InstallContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Configuration.Install {

    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;
    using System.Globalization;
    
    /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext"]/*' />
    /// <devdoc>
    ///    <para>Contains information about the current installation.</para>
    /// </devdoc>
    public class InstallContext {

        private StringDictionary parameters;

        /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext.InstallContext"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Configuration.Install.InstallContext'/> class; a log file for the
        ///       installation is not created.
        ///    </para>
        /// </devdoc>
        public InstallContext() : this(null, null) {
        }

        /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext.InstallContext1"]/*' />
        /// <devdoc>
        ///    <para>Initializes a new instance of the
        ///    <see cref='System.Configuration.Install.InstallContext'/> class and creates a log file for 
        ///       the installation.</para>
        /// </devdoc>
        public InstallContext(string logFilePath, string[] commandLine) {
            parameters = ParseCommandLine(commandLine);
            if (Parameters["logfile"] == null && logFilePath != null) {
                Parameters["logfile"] = logFilePath;
            }
        }

        /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext.Parameters"]/*' />
        /// <devdoc>
        ///    <para>Gets the
        ///       command-line arguments that were entered
        ///       when the installation executable was run.</para>
        /// </devdoc>
        public StringDictionary Parameters {
            get { 
                return parameters;
            }
        }

        /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext.IsParameterTrue"]/*' />
        /// <devdoc>
        ///    <para>Determines whether the specified command line parameter
        ///       is <see langword='true'/>.</para>
        /// </devdoc>
        public bool IsParameterTrue(string paramName) {
            string paramValue = Parameters[paramName.ToLower(CultureInfo.InvariantCulture)];
            if (paramValue == null)
                return false;
            return string.Compare(paramValue, "true", StringComparison.OrdinalIgnoreCase) == 0 || string.Compare(paramValue, "yes", StringComparison.OrdinalIgnoreCase) == 0
                || string.Compare(paramValue, "1", StringComparison.OrdinalIgnoreCase) == 0 || "".Equals(paramValue);
        }

        internal void LogMessageHelper(string message)
        {
            StreamWriter log = null;

            try
            {
                if (!String.IsNullOrEmpty(this.Parameters["logfile"]))
                {
                    log = new StreamWriter(this.Parameters["logfile"], true, Encoding.UTF8);                                                      
                    log.WriteLine(message);
                }
            }
            finally
            {
                if (log != null)
                    log.Close();
            }
        }

        /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext.LogMessage"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the message
        ///       to the log file for the installation and
        ///       to the console.
        ///    </para>
        /// </devdoc>
        public void LogMessage(string message) {
            try
            {
                LogMessageHelper(message);
            }
            catch (Exception)
            {
                try
                {
                    // Fallback to temp folder
                    this.Parameters["logfile"] = Path.Combine(Path.GetTempPath(), Path.GetFileName(this.Parameters["logfile"]));
                    LogMessageHelper(message);
                }
                catch (Exception)
                {
                    // Fallback to no log at all
                    this.Parameters["logfile"] = null;
                }
            }
                    
            if (IsParameterTrue("LogToConsole") || Parameters["logtoconsole"] == null)
                Console.WriteLine(message);
        }

        /// <include file='doc\InstallContext.uex' path='docs/doc[@for="InstallContext.ParseCommandLine"]/*' />
        /// <devdoc>
        /// <para>Parses the command-line arguments into a <see cref='T:System.Windows.Forms.StringDictionary'/>.</para>
        /// </devdoc>
        protected static StringDictionary ParseCommandLine(string[] args) {
            StringDictionary options = new StringDictionary();
            if (args == null)
                return options;
            for (int i = 0; i < args.Length; i++) {
                // chop off the leading / or -
                if (args[i].StartsWith("/", StringComparison.Ordinal) || args[i].StartsWith("-", StringComparison.Ordinal))
                    args[i] = args[i].Substring(1);
                // find the '='
                int equalsPos = args[i].IndexOf('=');
                // if there is no equals, add the parameter with a value of ""
                if (equalsPos < 0)
                    options[args[i].ToLower(CultureInfo.InvariantCulture)] = "";
                else
                    options[args[i].Substring(0, equalsPos).ToLower(CultureInfo.InvariantCulture)] = args[i].Substring(equalsPos+1);
            }
            return options;
        }

    }

}
