//------------------------------------------------------------------------------
// <copyright file="TempFiles.cs" company="Microsoft">
// 
// <OWNER>Microsoft</OWNER>
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.Win32;
    using Microsoft.Win32.SafeHandles;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.Threading.Tasks;
    using System.ComponentModel;
    using System.Security.Cryptography;
    using System.Globalization;
    using System.Runtime.Versioning;

    /// <devdoc>
    ///    <para>Represents a collection of temporary file names that are all based on a
    ///       single base filename located in a temporary directory.</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    [Serializable]
    public class TempFileCollection : ICollection, IDisposable {
        string basePath;
        string tempDir;
        bool keepFiles;
        Hashtable files;
        [NonSerialized]
        private WindowsIdentity currentIdentity = null;
        [NonSerialized]
        private string highIntegrityDirectory = null;

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TempFileCollection() : this(null, false) {
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TempFileCollection(string tempDir) : this(tempDir, false) { 
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
#if !FEATURE_PAL
        [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Assert, ControlPrincipal = true)]
#endif
        public TempFileCollection(string tempDir, bool keepFiles) {
            this.keepFiles = keepFiles;
            this.tempDir = tempDir;
#if !FEATURE_CASE_SENSITIVE_FILESYSTEM            
            files = new Hashtable(StringComparer.OrdinalIgnoreCase);
#else
            files = new Hashtable();
#endif
#if !FEATURE_PAL
            WindowsImpersonationContext impersonation = Executor.RevertImpersonation();
            try
            {
                currentIdentity = WindowsIdentity.GetCurrent();
            }
            finally
            {
                Executor.ReImpersonate(impersonation);
            }
#endif
        }

        /// <internalonly/>
        /// <devdoc>
        /// <para> To allow it's stuff to be cleaned up</para>
        /// </devdoc>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        protected virtual void Dispose(bool disposing) {
            // It is safe to call Delete from here even if Dispose is called from Finalizer
            // because the graph of objects is guaranteed to be there and
            // neither Hashtable nor String have a finalizer of their own that could 
            // be called before TempFileCollection Finalizer
            Delete();
            DeleteHighIntegrityDirectory();
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ~TempFileCollection() {
            Dispose(false);
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AddExtension(string fileExtension) {
            return AddExtension(fileExtension, keepFiles);
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AddExtension(string fileExtension, bool keepFile) {
            if (fileExtension == null || fileExtension.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidNullEmptyArgument, "fileExtension"), "fileExtension");  // fileExtension not specified
            string fileName = BasePath + "." + fileExtension;
            AddFile(fileName, keepFile);
            return fileName;
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddFile(string fileName, bool keepFile) {
            if (fileName == null || fileName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidNullEmptyArgument, "fileName"), "fileName");  // fileName not specified

            if (files[fileName] != null)
                throw new ArgumentException(SR.GetString(SR.DuplicateFileName, fileName), "fileName");  // duplicate fileName
            files.Add(fileName, (object)keepFile);
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return files.Keys.GetEnumerator();
        }

        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return files.Keys.GetEnumerator();
        }

        /// <internalonly/>
        void ICollection.CopyTo(Array array, int start) {
            files.Keys.CopyTo(array, start);
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(string[] fileNames, int start) {
            files.Keys.CopyTo(fileNames, start);
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {
                return files.Count;
            }
        }

        /// <internalonly/>
        int ICollection.Count {
            get { return files.Count; }
        }

        /// <internalonly/>
        object ICollection.SyncRoot {
            get { return null; }
        }

        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get { return false; }
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TempDir {
            get { return tempDir == null ? string.Empty : tempDir; }
        }
                                              
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string BasePath {
            get {
                EnsureTempNameCreated();
                return basePath;
            }
        }

        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        void EnsureTempNameCreated() {
            if (basePath == null) {

                string tempFileName = null;
                FileStream tempFileStream;
                bool uniqueFile = false;
                int retryCount = 5000;
                do {
                    try {
                        basePath = GetTempFileName(TempDir);

                        string full = Path.GetFullPath(basePath);

                        new FileIOPermission(FileIOPermissionAccess.AllAccess, full).Demand();

                        // make sure the filename is unique. 
                        tempFileName = basePath + ".tmp";
                        using (tempFileStream = new FileStream(tempFileName, FileMode.CreateNew, FileAccess.Write)) { }
                        uniqueFile = true;
                    }
                    catch (IOException e) {
                        retryCount--;

                        uint HR_ERROR_FILE_EXISTS = unchecked(((uint)0x80070000) | NativeMethods.ERROR_FILE_EXISTS);
                        if (retryCount == 0 || Marshal.GetHRForException(e) != HR_ERROR_FILE_EXISTS)
                            throw;

                        uniqueFile = false;
                    }
                }while (!uniqueFile);
                files.Add(tempFileName, keepFiles);
            }
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool KeepFiles {
            get { return keepFiles; }
            set { keepFiles = value; }
        }

        bool KeepFile(string fileName) {
            object keep = files[fileName];
            if (keep == null) return false;
            return (bool)keep; 
        }

        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)]
        public void Delete() {
            if (files != null && files.Count > 0) {
                string[] fileNames = new string[files.Count];
                files.Keys.CopyTo(fileNames, 0);
                foreach (string fileName in fileNames) {
                    if (!KeepFile(fileName)) {
                        Delete(fileName);
                        files.Remove(fileName);
                    }
                }
            }
        }

        private void DeleteHighIntegrityDirectory()
        {
            try
            {
                if (currentIdentity != null && Directory.Exists(highIntegrityDirectory))
                {
                    // Remove the no delete policy from the directory.
                    RemoveAceOnTempDirectory(highIntegrityDirectory, currentIdentity.User.ToString());
                    if (Directory.GetFiles(highIntegrityDirectory).Length == 0)
                    {
                        // Delete the high integrity directory if all files are deleted.
                        Directory.Delete(highIntegrityDirectory, true);
                    }
                }
            }
            catch { } // Ignore all errors.
        }

        // This function deletes files after reverting impersonation.
        [ResourceExposure(ResourceScope.None)]
        [ResourceConsumption(ResourceScope.Process, ResourceScope.Process)]
        internal void SafeDelete() {
#if !FEATURE_PAL
            WindowsImpersonationContext impersonation = Executor.RevertImpersonation();
#endif
            try{
                Delete();
            }
            finally {               
#if !FEATURE_PAL
                Executor.ReImpersonate(impersonation);
#endif
            }
        }

        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        void Delete(string fileName) {
            try {
                File.Delete(fileName);
            }
            catch {
                // Ignore all exceptions
            }
        }

        [ResourceExposure(ResourceScope.Machine)]
        [ResourceConsumption(ResourceScope.Machine)]
        string GetTempFileName(string tempDir) {
            string fileName;
            string randomFileName = Path.GetFileNameWithoutExtension(Path.GetRandomFileName());

            if (String.IsNullOrEmpty(tempDir)) {
                tempDir = Path.GetTempPath();

                if (!LocalAppContextSwitches.DisableTempFileCollectionDirectoryFeature && currentIdentity != null &&
                    new WindowsPrincipal(currentIdentity).IsInRole(WindowsBuiltInRole.Administrator))
                {
                    tempDir = Path.Combine(tempDir, randomFileName);

                    // Create high integrity dir and set no delete policy for all files under the directory.
                    // In case of failure, throw exception.
                    CreateTempDirectoryWithAce(tempDir, currentIdentity.User.ToString());
                    highIntegrityDirectory = tempDir;
                }
            }

            if (tempDir.EndsWith("\\", StringComparison.Ordinal))
                fileName = tempDir + randomFileName;
            else
                fileName = tempDir + "\\" + randomFileName;

            return fileName;
        }

        private static void CreateTempDirectoryWithAce(string directory, string identity)
        {
            // Dacl Sddl string:
            // D: Dacl type
            // D; Deny access
            // OI; Object inherit ace
            // SD; Standard delete function
            // wIdentity.User Sid of the given user.
            // A; Allow access
            // OICI; Object inherit, container inherit
            // FA File access
            // BA Built-in administrators
            // S: Sacl type
            // ML;; Mandatory Label
            // NW;;; No write policy
            // HI High integrity processes only
            string sddl = "D:(D;OI;SD;;;" + identity + ")(A;OICI;FA;;;BA)S:(ML;OI;NW;;;HI)";

            SafeLocalMemHandle acl = null;
            SafeLocalMemHandle.ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, NativeMethods.SDDL_REVISION_1, out acl, IntPtr.Zero);

            // Create the directory with the acl
            NativeMethods.CreateDirectory(directory, acl);
        }

        private static void RemoveAceOnTempDirectory(string directory, string identity)
        {
            string sddl = "D:(A;OICI;FA;;;" + identity + ")(A;OICI;FA;;;BA)";

            SafeLocalMemHandle acl = null;
            SafeLocalMemHandle.ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, NativeMethods.SDDL_REVISION_1, out acl, IntPtr.Zero);

            NativeMethods.SetNamedSecurityInfo(directory, acl);
        }
    }
}
