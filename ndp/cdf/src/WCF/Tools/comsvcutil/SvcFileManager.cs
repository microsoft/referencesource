//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace Microsoft.Tools.ServiceModel.ComSvcConfig
{
    using System;
    using System.ServiceModel.Channels;
    using System.Diagnostics;
    using System.Configuration;
    using System.Globalization;
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
    using System.ServiceModel.Activation;
    using System.ServiceModel.Configuration;
    using System.ServiceModel.ComIntegration;
    using Microsoft.Tools.ServiceModel;
    using Microsoft.Tools.ServiceModel.SvcUtil;

    class SvcFileManager
    {
        string webDirectoryPath;
        Dictionary<Guid, SvcFile> svcFiles;

        public SvcFileManager(string webDirectoryPath)
        {
            this.webDirectoryPath = webDirectoryPath;
            this.svcFiles = new Dictionary<Guid, SvcFile>();

            string[] fileNames = Directory.GetFiles(webDirectoryPath, "*.svc");

            foreach (string fileName in fileNames)
            {
                SvcFile svcFile = SvcFile.OpenExisting(fileName);
                if (svcFile != null)
                {
                    this.svcFiles.Add(svcFile.Clsid, svcFile);
                }
            }
        }

        public void Abort()
        {
            foreach (SvcFile file in svcFiles.Values)
            {
                file.Abort();
            }
        }

        public void Add(Guid appid, Guid clsid)
        {
            SvcFile svcFile = null;
            if (this.svcFiles.TryGetValue(clsid, out svcFile))
            {
                // we found an existing SVC file, all is good
                // We should never be adding and deleting SVC files at the same time..
                Debug.Assert(svcFile.State != SvcFileState.Deleted, "svcFile.State != SvcFileState.Deleted");
            }
            else
            {
                svcFile = SvcFile.CreateNew(this.webDirectoryPath, appid, clsid);
                this.svcFiles.Add(clsid, svcFile);
            }
        }

        public bool Remove(Guid appid, Guid clsid)
        {
            SvcFile svcFile = null;
            if (this.svcFiles.TryGetValue(clsid, out svcFile))
            {
                // we found an existing SVC file

                // We should never be adding and deleting SVC files at the same time..
                Debug.Assert(svcFile.State != SvcFileState.Added, "svcFile.State != SvcFileState.Added");

                if (svcFile.State == SvcFileState.Deleted)
                {
                    // already marked for deletion
                    return true;
                }
                else
                {
                    Debug.Assert(svcFile.State == SvcFileState.Existing, "svcFile.State == SvcFileState.Existing");
                    svcFile.Delete();
                    return true;
                }
            }
            else
            {
                // didn't find any SVC file to remove
                return false;
            }
        }

        public void Prepare()
        {
            foreach (SvcFile file in svcFiles.Values)
            {
                file.Prepare();
            }
        }

        public void Commit()
        {
            foreach (SvcFile file in svcFiles.Values)
            {
                file.Commit();
            }
        }

        public bool ResolveClsid(Guid clsid, out Guid appid)
        {
            SvcFile svcFile;
            appid = Guid.Empty;
            if (this.svcFiles.TryGetValue(clsid, out svcFile))
            {
                if (svcFile.State == SvcFileState.Deleted)
                {
                    return false;   // we cant resolve because we think its deleted
                }
                else
                {
                    appid = svcFile.Appid;
                    return true;
                }
            }
            else
            {
                return false;   // Clsid not found
            }
        }

        enum SvcFileState
        {
            Existing,
            Added,
            Deleted
        }

        class SvcFile
        {
            Guid appid;
            Guid clsid;
            SvcFileState state;
            AtomicFile svcFile;

            const string factoryAttributeName = "Factory";
            const string serviceAttributeName = "Service";

            SvcFile(Guid appid, Guid clsid, SvcFileState state, AtomicFile svcFile)
            {
                this.appid = appid;
                this.clsid = clsid;
                this.state = state;
                this.svcFile = svcFile;
            }

            public static SvcFile CreateNew(string webDirectoryPath, Guid appid, Guid clsid)
            {
                ComAdminAppInfo adminAppInfo = ComAdminWrapper.GetAppInfo(appid.ToString("B"));
                if (null == adminAppInfo)
                {
                    throw Tool.CreateException(SR.GetString(SR.CannotFindAppInfo, appid.ToString("B")), null);
                }

                ComAdminClassInfo adminClassInfo = adminAppInfo.FindClass(clsid.ToString("B"));
                if (null == adminClassInfo)
                {
                    throw Tool.CreateException(SR.GetString(SR.CannotFindClassInfo, clsid.ToString("B")), null);
                }

                string fileName = webDirectoryPath + "\\" + adminClassInfo.Name;

                if (File.Exists(fileName + ".svc"))
                {
                    int count = 1;

                    while (File.Exists(fileName + "." + count.ToString(CultureInfo.InvariantCulture) + ".svc"))
                    {
                        count++;
                    }

                    fileName = fileName + "." + count.ToString(CultureInfo.InvariantCulture);
                }

                fileName = fileName + ".svc";

                string comPlusString = clsid.ToString("B") + "," + appid.ToString("B");

                using (StreamWriter sw = File.CreateText(fileName))
                {
                    sw.WriteLine("<%@ServiceHost {0}=\"{1}\" {2}=\"{3}\" %>",
                        factoryAttributeName,
                        typeof(WasHostedComPlusFactory).FullName,
                        serviceAttributeName,
                        comPlusString);
                }

                return new SvcFile(appid, clsid, SvcFileState.Added, new AtomicFile(fileName));
            }

            // this function wraps the call to the internal method in ServiceModel assembly.
            static IDictionary<string, string> ParseServiceDirective(string serviceText)
            {
                IDictionary<string, string> dictionary = null;

                try
                {
                    Type serviceParser = typeof(ServiceHostFactory).Assembly.GetType("System.ServiceModel.Activation.ServiceParser");
                    dictionary = (IDictionary<string, string>)serviceParser.InvokeMember("ParseServiceDirective", BindingFlags.InvokeMethod | BindingFlags.Static | BindingFlags.NonPublic,
                                        null, null, new object[1] { serviceText }, CultureInfo.InvariantCulture);
                }
                catch (TargetInvocationException e)
                {
                    throw e.InnerException;
                }

                return dictionary;
            }

            public static SvcFile OpenExisting(string fileName)
            {
                if (!File.Exists(fileName))
                {
                    return null;
                }

                string svcFileContents = null;
                Guid appid;
                Guid clsid;
                using (StreamReader sr = File.OpenText(fileName))
                {
                    svcFileContents = sr.ReadToEnd();
                }
                IDictionary<string, string> dictionary = null;
                try
                {
                    dictionary = ParseServiceDirective(svcFileContents);
                }
                catch (Exception e)
                {
                    if (e is NullReferenceException || e is SEHException)
                    {
                        throw e;
                    }
                    ToolConsole.WriteWarning(SR.GetString(SR.SvcFileParsingFailedWithError, fileName, e.Message));
                    return null;
                }

                if (dictionary == null)
                {
                    return null;
                }

                if (!dictionary.ContainsKey(factoryAttributeName) ||
                    !dictionary.ContainsKey(serviceAttributeName))
                {
                    return null;
                }

                string typeName = dictionary[factoryAttributeName];
                Type factoryType = typeof(WasHostedComPlusFactory);
                Type compiledType = factoryType.Assembly.GetType(dictionary[factoryAttributeName], false);
                if (compiledType != factoryType)
                {
                    return null;
                }

                string comPlusText = dictionary[serviceAttributeName];

                string[] parameters = comPlusText.Split(',');
                if (parameters.Length != 2)
                {
                    ToolConsole.WriteWarning(SR.GetString(SR.BadlyFormattedSvcFile, fileName));
                    return null;
                }

                try
                {
                    clsid = new Guid(parameters[0]);
                    appid = new Guid(parameters[1]);
                }
                catch (FormatException)
                {
                    ToolConsole.WriteWarning(SR.GetString(SR.BadlyFormattedAppIDOrClsidInSvcFile, fileName));
                    return null;
                }

                return new SvcFile(appid, clsid, SvcFileState.Existing, new AtomicFile(fileName));
            }

            public Guid Appid
            {
                get { return this.appid; }
            }

            public Guid Clsid
            {
                get { return this.clsid; }
            }

            public SvcFileState State
            {
                get { return this.state; }
            }

            public void Abort()
            {
                this.svcFile.Abort();
            }

            public void Commit()
            {
                this.svcFile.Commit();
            }

            public void Delete()
            {
                Debug.Assert(this.state == SvcFileState.Existing, "this.state == SvcFileState.Existing");
                this.svcFile.Delete();
                this.state = SvcFileState.Deleted;
            }

            public void Prepare()
            {
                this.svcFile.Prepare();
            }
        }
    }
}
