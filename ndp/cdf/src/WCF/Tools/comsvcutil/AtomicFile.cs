//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.Diagnostics;
    using System.Configuration;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Collections.Generic;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.ServiceModel;
    using System.ServiceModel.Configuration;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;

    // Usage
    //
    // new AtomicFile
    // GetCurrent(readOnly)+
    // Delete (once), OR  GetCurrent(forUpdate)+
    // Prepare
    // Commit or Abort
    // 
    // Abort may also be called before Prepare

    class AtomicFile
    {
        bool prepared;
        string originalFileName;
        bool originalFileExists;
        string tempFileName;
        string backupOfOriginalFileName;
        bool deleteOriginalFile;

        public string OriginalFileName
        {
            get { return this.originalFileName; }
        }

        public bool OriginalFileExists
        {
            get { return this.originalFileExists; }
        }

        public AtomicFile(string fileName)
        {
            this.originalFileName = fileName;
            this.originalFileExists = File.Exists(this.originalFileName);
            this.tempFileName = null;
            this.backupOfOriginalFileName = null;
            this.deleteOriginalFile = false;
            this.prepared = false;
        }

        public bool CurrentExists()
        {
            if ((this.tempFileName != null))
            {
                return true;
            }
            else
            {
                return this.originalFileExists;
            }
        }

        public void Delete()
        {
            if (this.tempFileName != null)
            {
                // once you make changes, you cant ask for delete (we have no scenario to relax this restriction..)
                throw new InvalidOperationException();
            }

            if (!this.originalFileExists)
            {
                // cant delete if the original file didnt exist
                throw new InvalidOperationException();
            }

            this.deleteOriginalFile = true;
        }

        public string GetCurrentFileName(bool readOnly)
        {
            if (this.deleteOriginalFile)
            {
                // once you delete, you cant ask for this.
                throw new InvalidOperationException();
            }

            if (this.tempFileName != null)
            {
                return this.tempFileName;
            }
            else if (this.originalFileExists)
            {
                if (readOnly)
                {
                    return this.originalFileName;
                }
                else
                {
                    this.tempFileName = GetTempFileName();
                    File.Copy(this.originalFileName, this.tempFileName);
                    return this.tempFileName;
                }
            }
            else // there is no original file
            {
                if (readOnly)
                {
                    return null;    // nothing to read, caller should tolerate this
                }
                else
                {
                    // want to update, have no file yet, create one..
                    this.tempFileName = GetTempFileName();
                    return this.tempFileName;
                }
            }
        }

        // must be called before Preparing
        public bool HasBeenModified()
        {
            return (this.tempFileName != null || (this.originalFileExists && this.deleteOriginalFile));
        }

        public void Prepare()
        {
            if (this.tempFileName != null)
            {
                if (this.originalFileExists)
                {
                    this.backupOfOriginalFileName = GetTempFileName();
                    File.Copy(this.originalFileName, this.backupOfOriginalFileName);
                    SafeDeleteFile(this.originalFileName);
                }

                File.Copy(this.tempFileName, this.originalFileName);
                SafeDeleteFile(this.tempFileName);
            }
            else if (this.deleteOriginalFile)
            {
                this.backupOfOriginalFileName = GetTempFileName();
                File.Copy(this.originalFileName, this.backupOfOriginalFileName);
                SafeDeleteFile(this.originalFileName);
            }

            this.prepared = true;
        }

        public void Commit()
        {
            if (!this.prepared)
            {
                throw new InvalidOperationException();
            }

            SafeDeleteFile(this.backupOfOriginalFileName);
        }

        // keep track of whether we had a "clean" prepare / abort, if not clean we shouldnt delete the backups/tempFiles and point the user to them
        public void Abort()
        {
            if (this.originalFileExists)
            {
                if ((this.backupOfOriginalFileName != null) && File.Exists(this.backupOfOriginalFileName))
                {
                    SafeDeleteFile(this.originalFileName);
                    File.Copy(this.backupOfOriginalFileName, this.originalFileName);
                }
            }
            else
            {
                SafeDeleteFile(this.originalFileName);
            }

            SafeDeleteFile(this.backupOfOriginalFileName);
            SafeDeleteFile(this.tempFileName);
        }

        // safe as in handles null, and checks for existence first
        internal static void SafeDeleteFile(string fileName)
        {
            if ((!string.IsNullOrEmpty(fileName)) && File.Exists(fileName))
            {
                File.Delete(fileName);
            }
        }

        // this function guarantees to return the name of a temp file that doesnt exist
        string GetTempFileName()
        {
            return Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
        }
    }
}
