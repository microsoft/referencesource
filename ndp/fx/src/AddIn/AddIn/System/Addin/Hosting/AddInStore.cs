// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  AddInStore
**
** Purpose: Finds valid combinations of add-ins and associated
**          classes, like host adaptors, etc.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.Remoting;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.Threading;
using System.AddIn.MiniReflection;
using System.AddIn.Pipeline;
using System.AddIn;
using System.Diagnostics.Contracts;
using TypeInfo=System.AddIn.MiniReflection.TypeInfo;
namespace System.AddIn.Hosting
{
    /*
     * AddInStore on-disk format (first version):
     *  What                                Type    Valid ranges
     *  Version number                      Int32   1
     *  Length of serialized state          Int64   positive
     *  Binary serialized PipelineDeploymentState   byte[]  varies
     */
    public static class AddInStore
    {
        private const String PipelineCacheFileName = "PipelineSegments.store";
        private const String AddInCacheFileName    = "AddIns.store";

        internal const String HostAdaptersDirName = "HostSideAdapters";
        internal const String ContractsDirName = "Contracts";
        internal const String AddInAdaptersDirName = "AddInSideAdapters";
        internal const String AddInBasesDirName = "AddInViews";
        internal const String AddInsDirName = "AddIns";

        private const int StoreFileFormatVersion = 1;

        private const uint HRESULT_FOR_ERROR_SHARING_VIOLATION = 0x80070020;

        // For FindAddins, maintain a cache of add-in root directory names to 
        // deserialized data from that file.  Include the last write time as 
        // well, so that we know when we must invalidate this cache.
        private static readonly Dictionary<String, CacheInfo> StateCache = new Dictionary<String, CacheInfo>();

        private struct CacheInfo 
        {
            internal DeploymentState State;
            internal DateTime FileTimeStamp;  // Creation time of the file, when we read it.
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed")]
        [System.Security.SecurityCritical]
        private static String[] UpdateImpl(String pipelineRootFolderPath, bool demand)
        {
            if (pipelineRootFolderPath == null)
                throw new ArgumentNullException("pipelineRootFolderPath");
            if (pipelineRootFolderPath.Length == 0)
                throw new ArgumentException(Res.PathCantBeEmpty, "pipelineRootFolderPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            // Guard against information disclosure about the filesystem.
            // Full trust implies path discovery, so it is a valid first approximation while we do initial validation
            bool hasPathDiscovery = Utils.HasFullTrust();
            try {
                pipelineRootFolderPath = ValidatePipelineRoot(pipelineRootFolderPath);

                if (demand)
                    new FileIOPermission(FileIOPermissionAccess.Read, pipelineRootFolderPath).Demand();

                hasPathDiscovery = HasPathDiscovery(pipelineRootFolderPath);

                String deploymentStore = Path.Combine(pipelineRootFolderPath, PipelineCacheFileName);

                Collection<String> warningsCollection = new Collection<String>();

                FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery |
                        FileIOPermissionAccess.Read, pipelineRootFolderPath);
                permission.Assert();

                if (!File.Exists(deploymentStore) || PipelineStoreIsOutOfDate(deploymentStore, pipelineRootFolderPath)) {
                    // Build the pipeline cache
                    BuildPipelineCache(pipelineRootFolderPath, warningsCollection);
                }

                //The default location for addins is included implicitly
                String addInDir = Path.Combine(pipelineRootFolderPath, AddInsDirName);
                UpdateAddInsIfExist(addInDir, warningsCollection);

                String[] warnings;
                if (hasPathDiscovery) {
                    warnings = new String[warningsCollection.Count];
                    warningsCollection.CopyTo(warnings, 0);
                }
                else if (warningsCollection.Count > 0) {
                    // there were warnings, but they'll have to run in Full Trust to know what they are
                    warnings = new String[1];
                    warnings[0] = Res.UnspecifiedUpdateWarningsInPartialTrust;
                }
                else {
                    warnings = new String[0];
                }
                return warnings;
            }
            catch (Exception) {
                if (hasPathDiscovery)
                    throw;
                else 
                    throw GetGenericSecurityException();
            }
        }

        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        public static String[] Update(PipelineStoreLocation location)
        {
            if (location != PipelineStoreLocation.ApplicationBase)
                throw new ArgumentException(Res.InvalidPipelineStoreLocation, "location");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String appBase = GetAppBase();
            return UpdateImpl(appBase, false);
        }

        [System.Security.SecurityCritical]
        public static String[] Update(String pipelineRootFolderPath)
        {
            return UpdateImpl(pipelineRootFolderPath, true);
        }

        [System.Security.SecurityCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity")]
        public static String[] UpdateAddIns(String addInsFolderPath)
        {
            if (addInsFolderPath == null)
                throw new ArgumentNullException("addInsFolderPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            // prevent disclosure of information (e.g. path discovery) in partial trust environments
            bool hasPathDiscovery = Utils.HasFullTrust();

            try
            {
                addInsFolderPath = ValidateAddInPath(addInsFolderPath);
                new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.Write, addInsFolderPath).Demand();
                hasPathDiscovery = HasPathDiscovery(addInsFolderPath);
                Collection<String> warningsCollection = new Collection<String>();
                UpdateAddInsIfExist(addInsFolderPath, warningsCollection);
                String[] warnings;
                if (hasPathDiscovery) {
                    warnings = new String[warningsCollection.Count];
                    warningsCollection.CopyTo(warnings, 0);
                }
                else {
                    warnings= new String[0];
                }
                return warnings;
            }
            catch (Exception) {
                if (hasPathDiscovery)
                    throw;
                else 
                    throw GetGenericSecurityException();
            }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity")]
        [System.Security.SecurityCritical]
        public static String[] RebuildAddIns(String addInsFolderPath)
        {
            if (addInsFolderPath == null)
                throw new ArgumentNullException("addInsFolderPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            bool hasPathDiscovery = false; 
            try
            {
                addInsFolderPath = ValidateAddInPath(addInsFolderPath);
                new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.Write, addInsFolderPath).Demand();
                hasPathDiscovery = HasPathDiscovery(addInsFolderPath);
                String addInStore = Path.Combine(addInsFolderPath, AddInCacheFileName); 

                Collection<String> warningsCollection = new Collection<String>();
                bool consistent = false;
                try {
                    BuildAddInCache(addInsFolderPath, warningsCollection);

                    consistent = true;
                }
                finally {
                    // I'd prefer to not leave around a potentially corrupted file.
                    if (!consistent && File.Exists(addInStore)) {
                        try {
                            File.Delete(addInStore);
                        }
                        catch { }
                    }
                }

                String[] warnings;
                if (hasPathDiscovery) {
                    warnings = new String[warningsCollection.Count];
                    warningsCollection.CopyTo(warnings, 0);
                }
                else 
                    warnings = new String[0];
                return warnings;
            }
            catch (Exception) {
                if (hasPathDiscovery)
                    throw;
                else 
                    throw GetGenericSecurityException();
            }
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static void UpdateAddInsIfExist(String addInsPath, Collection<String> warningsCollection)
        {
            String addInStore = Path.Combine(addInsPath, AddInCacheFileName); 

            FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery |
                    FileIOPermissionAccess.Read, addInsPath);
            permission.Assert();

            if (Directory.Exists(addInsPath))
            {
                if (!File.Exists(addInStore) || AddInStoreIsOutOfDate(addInsPath)) {
                    BuildAddInCache(addInsPath, warningsCollection);
                }
            }
        }

        // Check the last write time for every file in the add-in root directory.
        // I was hoping we could use the last write time for the directory, but
        // that's only updated when you add or remove files.  If you replace an
        // existing file, this time stamp isn't updated (on a local NTFS drive)
        // So, we check every file's time stamp, after checking the directory first.
        private static bool PipelineStoreIsOutOfDate(String deploymentStore, String path)
        {
            DateTime storeTime = File.GetLastWriteTime(deploymentStore);

            String hostAdapterDir = Path.Combine(path, HostAdaptersDirName);
            String contractDir = Path.Combine(path, ContractsDirName);
            String addInAdapterDir = Path.Combine(path, AddInAdaptersDirName);
            String addInBaseDir = Path.Combine(path, AddInBasesDirName);
            String addInDir = Path.Combine(path, AddInsDirName);

            String[] dirs = new String[]{hostAdapterDir, contractDir, addInAdapterDir, addInBaseDir};

            // for efficiency, gather up the file counts while we check the timestamps.
            int[] currentFileCounts = new int[]{0,0,0,0};
            int i = 0;
            foreach (String dir in dirs)
            {
                if (DirectoryNeedsUpdating(dir, storeTime, ref currentFileCounts[i++]))
                    return true;
            }

            // Check if any files have been deleted.  
            // Note that on NTFS, the Last Write Time for the parent folder is updated
            // when there is a deletion, so it is detected above.  The following is needed 
            // for FAT filesystems.
            PipelineDeploymentState state = GetPipelineDeploymentState(path);
            List<int> fileCounts = state.FileCounts;
            for (int j = 0; j < currentFileCounts.Length; j++)
            {
                if (currentFileCounts[j] != state.FileCounts[j])
                {
                    return true;
                }
            }

            return false;
        }

        private static bool AddInStoreIsOutOfDate(String addInPath)
        {
            if (addInPath == null)
                throw new ArgumentNullException("addInPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String storeName = Path.Combine(addInPath, AddInCacheFileName);
            DateTime storeTime = File.GetLastWriteTime(storeName);

            int addInFileCount = 0;
            if (Directory.Exists(addInPath)) {
                foreach (String dir in Directory.GetDirectories(addInPath)) {
                    if (DirectoryNeedsUpdating(dir, storeTime, ref addInFileCount)) {
                        return true;
                    }
                }
            }

            // now check file counts because on FAT filesystems, the timestamps won't be updated 
            // for directories when the contents are deleted.
            AddInDeploymentState addInState = GetAddInDeploymentState(addInPath);

            if (addInState == null)
                return true;

            if (addInFileCount != addInState.FileCount)
                return true;

            return false;
        }

        private static bool DirectoryNeedsUpdating(string path, DateTime storeTime, ref int fileCount)
        {
            if (storeTime < Directory.GetLastWriteTime(path))
                return true;
            foreach(String file in GetExecutableFiles(path)) {
                try {
                    if (storeTime < Directory.GetLastWriteTime(file))
                        return true;
                    fileCount++;
                }
                catch (IOException) { }
                catch (SecurityException) { }
            }
            return false;
        }

        private static List<string> GetExecutableFiles(string path)
        {
            List<string> result = new List<string>();
            result.AddRange(Directory.GetFiles(path, "*.dll"));
            result.AddRange(Directory.GetFiles(path, "*.exe"));
            return result;
        }

        [System.Security.SecurityCritical]
        public static String[] Rebuild(String pipelineRootFolderPath)
        {
            return RebuildImpl(pipelineRootFolderPath, true);
        }

        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        public static String[] Rebuild(PipelineStoreLocation location)
        {
            if (location != PipelineStoreLocation.ApplicationBase)
                throw new ArgumentException(Res.InvalidPipelineStoreLocation, "location");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String appBase = GetAppBase();
            return RebuildImpl(appBase, false);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity")]
        [System.Security.SecurityCritical]
        private static String[] RebuildImpl(String pipelineRootFolderPath, bool demand)
        {
            if (pipelineRootFolderPath == null)
                throw new ArgumentNullException("pipelineRootFolderPath");
            if (pipelineRootFolderPath.Length == 0)
                throw new ArgumentException(Res.PathCantBeEmpty, "pipelineRootFolderPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            bool hasPathDiscovery = Utils.HasFullTrust(); 

            try {
                pipelineRootFolderPath = ValidatePipelineRoot(pipelineRootFolderPath);

                if (demand)
                    new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.Write, pipelineRootFolderPath).Demand();

                hasPathDiscovery = HasPathDiscovery(pipelineRootFolderPath);

                String deploymentStore = Path.Combine(pipelineRootFolderPath, PipelineCacheFileName);
                Collection<String> warningsCollection = new Collection<String>();

                bool consistent = false;
                try {
                    BuildPipelineCache(pipelineRootFolderPath, warningsCollection);

                    //The default location for addins is included implicitly.  Update it if it exists.
                    String addInDir = Path.Combine(pipelineRootFolderPath, AddInsDirName);
                    BuildAddInCache(addInDir, warningsCollection);

                    consistent = true;
                }
                finally {
                    // I'd prefer to not leave around a potentially corrupted file.
                    if (!consistent && File.Exists(deploymentStore)) {
                        try {
                            File.Delete(deploymentStore);
                        }
                        catch { }
                    }
                }

                String[] warnings;
                if (hasPathDiscovery) {
                    warnings = new String[warningsCollection.Count];
                    warningsCollection.CopyTo(warnings, 0);
                }
                else {
                    warnings = new String[0];
                }
                return warnings;
            }
            catch (Exception) {
                if (hasPathDiscovery)
                    throw;
                else
                    throw GetGenericSecurityException();
            }
        }

        internal static List<PartialToken> GetPartialTokens(String pipelineRoot)
        {
            PipelineDeploymentState pipelineState = GetPipelineDeploymentState(pipelineRoot);
            return pipelineState.PartialTokens;
        }

        // This overload that takes no addinFolderPaths exists so that we can have a SafeCritical version for partial-trust hosts
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "InFolder")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        [System.Security.SecuritySafeCritical]
        public static Collection<AddInToken> FindAddIns(Type hostViewOfAddIn, PipelineStoreLocation location)
        {
            if (location != PipelineStoreLocation.ApplicationBase)
                throw new ArgumentException(Res.InvalidPipelineStoreLocation, "location");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String appBase = GetAppBase();
            return FindAddInsImpl(hostViewOfAddIn, appBase, false);
        }
        
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "InFolder")]
        [System.Security.SecurityCritical]
        public static Collection<AddInToken> FindAddIns(Type hostViewOfAddIn, PipelineStoreLocation location, params String[] addInFolderPaths)
        {
            if (location != PipelineStoreLocation.ApplicationBase)
                throw new ArgumentException(Res.InvalidPipelineStoreLocation, "location");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String appBase = GetAppBase();
            return FindAddInsImpl(hostViewOfAddIn, appBase, false, addInFolderPaths);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "InFolder")]
        [System.Security.SecurityCritical]
        public static Collection<AddInToken> FindAddIns(Type hostViewOfAddIn, String pipelineRootFolderPath, params String[] addInFolderPaths)
        {
            return FindAddInsImpl(hostViewOfAddIn, pipelineRootFolderPath, true, addInFolderPaths);
        }
       
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "InFolder")]
        [System.Security.SecurityCritical]
        private static Collection<AddInToken> FindAddInsImpl(Type hostViewOfAddIn, String pipelineRootFolderPath, bool demand, params String[] addInFolderPaths)
        {
            if (hostViewOfAddIn == null)
                throw new ArgumentNullException("hostViewOfAddIn");
            if (pipelineRootFolderPath == null)
                throw new ArgumentNullException("pipelineRootFolderPath");
            if (pipelineRootFolderPath.Length == 0)
                throw new ArgumentException(Res.PathCantBeEmpty, "pipelineRootFolderPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            
            WarnIfGenericHostView(hostViewOfAddIn);

            bool hasPathDiscovery = Utils.HasFullTrust();
            try {
                pipelineRootFolderPath = ValidatePipelineRoot(pipelineRootFolderPath);

                if (demand)
                    new FileIOPermission(FileIOPermissionAccess.Read, pipelineRootFolderPath).Demand();

                hasPathDiscovery = HasPathDiscovery(pipelineRootFolderPath);
                if (addInFolderPaths != null)
                {
                    for (int i = 0; i < addInFolderPaths.Length; i++)
                    {
                        addInFolderPaths[i] = ValidateAddInPath(addInFolderPaths[i]);
                    }
                }

                // Get Pipeline cache
                PipelineDeploymentState pipelineState = GetPipelineDeploymentState(pipelineRootFolderPath);

                Collection<AddInToken> collection = new Collection<AddInToken>();
                TypeInfo havTypeInfo = new TypeInfo(hostViewOfAddIn);
                List<PartialToken> partialTokens = pipelineState.PartialTokens;

                String defaultAddInLocation = Path.Combine(pipelineRootFolderPath, AddInsDirName);

                // Put the default addInLocation in the front only.  
                List<String> locationsInOrder = new List<String>();
                locationsInOrder.Add(defaultAddInLocation);
                if (addInFolderPaths != null)
                {
                    foreach (String addInPath in addInFolderPaths)
                    {
                        if (!String.Equals(defaultAddInLocation, addInPath, StringComparison.OrdinalIgnoreCase))
                            locationsInOrder.Add(addInPath);
                    }
                }

                FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery, pipelineRootFolderPath);
                permission.Assert();

                foreach (String addInLocation in locationsInOrder)
                {
                    AddInDeploymentState addInState = GetAddInDeploymentState(addInLocation);

                    if (addInState != null)
                    {
                        // Find valid AddInTokens.
                        List<AddInToken> validPipelines = ConnectPipelinesWithAddIns(partialTokens, havTypeInfo, addInState);

                        foreach (AddInToken pipeline in validPipelines)
                        {
                            pipeline.PipelineRootDirectory = pipelineRootFolderPath;
                            pipeline.AddInRootDirectory = addInLocation;
                            collection.Add(pipeline);
                        }
                    }
                }

                return collection;
            }
            catch (Exception) {
                if (hasPathDiscovery)
                    throw;
                else
                    throw GetGenericSecurityException();
            }
        }
        
        private static void WarnIfGenericHostView(Type hostViewOfAddIn)
        {
            if (hostViewOfAddIn.IsGenericType == true)
            {
                Trace.WriteLine(String.Format(System.Globalization.CultureInfo.CurrentCulture, Res.HostViewUnusableBecauseItIsGeneric, hostViewOfAddIn.Name));
                if(Debugger.IsAttached)
                {
                    Debugger.Break();
                }
            }

        }

        // Find pipelines for a single addin that the host already knows about, as with ClickOnce addins.
        [System.Security.SecurityCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity")]
        public static Collection<AddInToken> FindAddIn(Type hostViewOfAddIn, String pipelineRootFolderPath, String addInFilePath, String addInTypeName)
        {
            if (hostViewOfAddIn == null)
                throw new ArgumentNullException("hostViewOfAddIn");
            if (pipelineRootFolderPath == null)
                throw new ArgumentNullException("pipelineRootFolderPath");
            if (pipelineRootFolderPath.Length == 0)
                throw new ArgumentException(Res.PathCantBeEmpty, "pipelineRootFolderPath");
            if (addInFilePath == null)
                throw new ArgumentNullException("addInFilePath");
            if (addInFilePath.Length == 0)
                throw new ArgumentException(Res.PathCantBeEmpty, "addInFilePath");
            if (addInTypeName == null)
                throw new ArgumentNullException("addInTypeName");
            if (addInTypeName.Length == 0 )
                throw new ArgumentException(Res.TypeCantBeEmpty, "addInTypeName");
            System.Diagnostics.Contracts.Contract.EndContractBlock();
            
            WarnIfGenericHostView(hostViewOfAddIn);

            bool hasPathDiscovery = Utils.HasFullTrust();

            try
            {
                pipelineRootFolderPath = ValidatePipelineRoot(pipelineRootFolderPath);

                new FileIOPermission(FileIOPermissionAccess.Read, pipelineRootFolderPath).Demand();

                string addInFolderPath = Path.GetDirectoryName(addInFilePath);
                ValidateAddInPath(addInFolderPath);
                hasPathDiscovery = HasPathDiscovery(pipelineRootFolderPath) && HasPathDiscovery(addInFolderPath);

                // look for addin in the specified assembly and match up with pipelines where appropriate.
                Collection<String> warningsCollection = new Collection<String>();
                AddIn addIn = DiscoverAddIn(addInFilePath, warningsCollection, addInTypeName);

                //Warnings are usually only returned for Update.  
                foreach (String warning in warningsCollection)
                {
                    Debugger.Log(0, "AddInStore", warning);
                }

                if (addIn == null) {
                    // could be because the file doesn't exist, it doesn't contain an addin, etc.
                    throw new ArgumentException(String.Format(CultureInfo.CurrentCulture, Res.TypeNotFound, addInTypeName, addInFilePath));
                }
                else {
                    // Match with partial tokens
                    PipelineDeploymentState pipelineState = GetPipelineDeploymentState(pipelineRootFolderPath);

                    Collection<AddInToken> validPipelines = new Collection<AddInToken>();
                    foreach (PartialToken partialToken in pipelineState.PartialTokens)
                    {
                        // see if there is a match between any of the addin's known parents and the addInBase for this partial token
                        if (Contains(addIn.AddInBaseTypeInfo, partialToken._addinBase.TypeInfo))
                        {
                            AddInToken pipeline;
                            pipeline = new AddInToken(partialToken._hostAdapter, partialToken._contract, 
                                    partialToken._addinAdapter, partialToken._addinBase, addIn);
                            validPipelines.Add(pipeline);
                        }
                    }

                    // remove anything that doesn't match our hav
                    TypeInfo havTypeInfo = new TypeInfo(hostViewOfAddIn);
                    for (int i = validPipelines.Count - 1; i >= 0; i--)
                    {
                        AddInToken pipeline = validPipelines[i];
                        if (Contains(pipeline.HostAddinViews, havTypeInfo) ) {
                            pipeline.PipelineRootDirectory = pipelineRootFolderPath;
                            pipeline.AddInRootDirectory = Path.GetDirectoryName(addInFilePath);
                            pipeline.ResolvedHostAddInView = havTypeInfo;
                        }
                        else {
                            validPipelines.RemoveAt(i);
                        }
                    }

                    return validPipelines;
                }
            }
            catch (Exception) {
                if (hasPathDiscovery)
                    throw;
                else
                   throw GetGenericSecurityException();
            } 
        }

        [System.Security.SecurityCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity")]
        private static string ValidateAddInPath(String addInPath)
        {
            if (addInPath == null)
                throw new ArgumentNullException("addInPath");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            String fullPath = GetFullPath(addInPath);

            new FileIOPermission(FileIOPermissionAccess.Read, fullPath).Demand();

            if (!Directory.Exists(fullPath))
            {
                if (HasPathDiscovery(fullPath))
                    throw new DirectoryNotFoundException(String.Format(CultureInfo.CurrentCulture, Res.FolderNotFound, addInPath));
                else
                    throw new InvalidPipelineStoreException();
            }

            return fullPath;
        }

        // validate the expected directory structure
        //<SecurityKernel Critical="True" Ring="0">
        //<Asserts Name="Imperative: System.Security.Permissions.FileIOPermission" />
        //<ReferencesCritical Name="Method: HasPathDiscovery():Boolean" Ring="2" />
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static string ValidatePipelineRoot(String pipelineRoot)
        {
            String fullPath = GetFullPath(pipelineRoot);
            FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.Read, fullPath);
            permission.Assert();

            if (!Directory.Exists(fullPath))
            {
                // 
                if (HasPathDiscovery(fullPath))
                    throw new DirectoryNotFoundException(String.Format(CultureInfo.CurrentCulture, Res.FolderNotFound, pipelineRoot));
                else
                    throw new InvalidPipelineStoreException();  
            }

            String[] folders = new String[] {
                Path.Combine(pipelineRoot, HostAdaptersDirName),
                Path.Combine(pipelineRoot, ContractsDirName),
                Path.Combine(pipelineRoot, AddInAdaptersDirName),
                Path.Combine(pipelineRoot, AddInBasesDirName)
            };

            foreach (String folder in folders)
            {
                if (!Directory.Exists(folder))
                {
                    if (HasPathDiscovery(folder))
                        throw new AddInSegmentDirectoryNotFoundException(String.Format(CultureInfo.CurrentCulture, Res.PipelineFolderNotFound, folder) );
                    else
                        throw new InvalidPipelineStoreException();
                }
            }

            return fullPath;
        }

        // To enable code sharing between the Pipeline Cache and the AddIn Cache code paths, 
        // this delegate and these helper methods are used.
        delegate DeploymentState Reader(String path, String filename);
        delegate DeploymentState Builder(String rootDir, Collection<String> warnings);

        // Callers must explicitly set the PipelineRootDirectory before using the Location of any pipeline component.
        private static PipelineDeploymentState GetPipelineDeploymentState(String pipelineRoot)
        {
            return (PipelineDeploymentState)GetDeploymentState(pipelineRoot, PipelineCacheFileName, new Reader(PipelineStateReader), new Builder(BuildPipelineCache));
        }

        private static AddInDeploymentState GetAddInDeploymentState(String addInRoot)
        {
            return (AddInDeploymentState)GetDeploymentState(addInRoot, AddInCacheFileName, new Reader(AddInStateReader), new Builder(BuildAddInCache));
        }

        private static DeploymentState PipelineStateReader(String path, String fileName)
        {
            String fullName = Path.Combine(path, fileName);
            return ReadCache<PipelineDeploymentState>(fullName, true);
        }

        private static DeploymentState AddInStateReader(String path, String fileName)
        {
            String fullName = Path.Combine(path, fileName);
            return ReadCache<AddInDeploymentState>(fullName, false);
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.FileIOPermission" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static DeploymentState GetDeploymentState(String path, String storeFileName, Reader reader, Builder stateBuilder)
        {
            // To make calling FindAddins multiple times quick, store the
            // DeploymentStates in a dictionary, along with the last write time
            // for the files.
            DeploymentState state = null;
            CacheInfo cachedState;
            bool found = false;
            String fileName = Path.Combine(path, storeFileName);

            FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.Read, fileName);
            permission.Assert();

            lock (StateCache){
                found = StateCache.TryGetValue(fileName, out cachedState);
                // Ensure the file is up to date, or remove it from the cache.
                DateTime lastWriteTime = File.GetCreationTime(fileName);
                if (found && lastWriteTime == cachedState.FileTimeStamp)
                    state = cachedState.State;
                else{
                    StateCache.Remove(path);
                }
            }
            if (state == null) {
                DateTime timeStamp = File.GetCreationTime(fileName);

                // Read the cache into memory.  Don't hold the lock while doing this, since it takes
                // a long time and there may be other thread(s) that need to read other cache(s)
                // concurrently.
                try
                {
                    state = reader(path, storeFileName);
                }
                catch (SecurityException)
                {
                    Collection<String> warnings = new Collection<string>();
                    state = stateBuilder(path, warnings);
                }

                // keep the cache in memory for even better perf.
                if (state != null)
                {
                    lock (StateCache) {
                        if (!StateCache.ContainsKey(path)) {
                            cachedState.State = state;
                            cachedState.FileTimeStamp = timeStamp;
                            StateCache[fileName] = cachedState;
                        }
                    }
                }
            }
            return state;
        }

        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: WriteCache(DeploymentState, String, String, DateTime):Void" Ring="1" />
        // <ReferencesCritical Name="Method: Discover(String, PipelineComponentType, PipelineDeploymentState, String, Collection`1<System.String>):Void" Ring="2" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static PipelineDeploymentState BuildPipelineCache(String rootDir, Collection<String> warnings)
        {
            FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery |
                    FileIOPermissionAccess.Read, rootDir);
            permission.Assert();
            // I need a normalized path so I can write out relative paths later.
            // We might as well normalize it now once.
            rootDir = Path.GetFullPath(rootDir);

            // Our deployed addins cache's time stamp should be set to a time
            // before we start grovelling the disk, so that we don't miss new
            // addins (at least on the next call to Update).  
            DateTime timeStamp = DateTime.Now;

            PipelineDeploymentState state = new PipelineDeploymentState();
            // Find all host adapters.
            // Host adapters might be in a HostAdapters directory.  They might also be 
            // in another assembly...  Perhaps one that's currently loaded?
            String hostAdapterDir = Path.Combine(rootDir, HostAdaptersDirName);
            String contractDir = Path.Combine(rootDir, ContractsDirName);
            String addInAdapterDir = Path.Combine(rootDir, AddInAdaptersDirName);
            String addInBaseDir = Path.Combine(rootDir, AddInBasesDirName);
            String addInDir = Path.Combine(rootDir, AddInsDirName);

            int i = 0;
            foreach (String file in Directory.GetFiles(hostAdapterDir)) {
                if (file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
                {
                    DiscoverHostAdapters(file, state.HostAdapters, rootDir, warnings);
                    state.FileCounts[i]++;
                }
            }
            i++;

            foreach (String file in Directory.GetFiles(contractDir)) {
                if (file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
                {
                    Discover(file, PipelineComponentType.Contract, state, rootDir, warnings);
                    state.FileCounts[i]++;
                }
            }
            i++;

            foreach (String file in Directory.GetFiles(addInAdapterDir)) {
                if (file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
                {
                    Discover(file, PipelineComponentType.AddInAdapter, state, rootDir, warnings);
                    state.FileCounts[i]++;
                }
            }

            i++;
            bool foundOne = false;
            foreach (String file in Directory.GetFiles(addInBaseDir)) {
                if (file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
                {
                    if (Discover(file, PipelineComponentType.AddInBase, state, rootDir, warnings))
                        foundOne = true;
                    state.FileCounts[i]++;
                }
            }
            if (!foundOne)
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.NoAddInBaseFound, addInBaseDir));

            // create the partial tokens
            state.ConnectPipeline(warnings);

            // Write the file storing our pipeline components 
            WriteCache(state, rootDir, PipelineCacheFileName, timeStamp);

            return state;
        }

        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: WriteCache(DeploymentState, String, String, DateTime):Void" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification="Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static AddInDeploymentState BuildAddInCache(String rootDir, Collection<String> warnings)
        {
            FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery |
                    FileIOPermissionAccess.Read, rootDir);
            permission.Assert();

            rootDir = Path.GetFullPath(rootDir);

            // Our deployed addins cache's time stamp should be set to a time
            // before we start grovelling the disk, so that we don't miss new
            // addins (at least on the next call to Update).  
            DateTime timeStamp = DateTime.Now;

            AddInDeploymentState state = new AddInDeploymentState();

            try
            {
                foreach (String singleAddinDir in Directory.GetDirectories(rootDir)) {
                    // Look in the add-in directory for add-ins, and warn the user if we didn't
                    // find one valid add-in.  We may find many assemblies that don't contain
                    // add-ins though, and that's fine.  For all the other pipeline components,
                    // we pretty much don't expect them to depend on other assemblies, and this
                    // check can be done elsewhere.
                    int oldNumAddins = state.AddIns.Count;
                    try
                    {
                        bool foundOne = false;
                        List<AddIn> foundAddIns = new List<AddIn>();
                        int fileCount = 0;
                        foreach (String file in Directory.GetFiles(singleAddinDir)) {
                            if (IsDllOrExe(file))
                            {
                                fileCount++;
                                bool found = DiscoverAddIns(file, foundAddIns, rootDir, warnings);
                                if (found)
                                    foundOne = true;
                            }
                        }

                        //  add them at the end only if there were no AddInBases found in the same folder.
                        state.AddIns.AddRange(foundAddIns);
                        state.FileCount += fileCount;

                        if (!foundOne)
                            warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.NoAddInFound, singleAddinDir));
                    }
                    catch (AddInBaseInAddInFolderException ex)
                    {
                        warnings.Add(ex.Message);
                    }
                }

                //Write the file storing the addins;
                WriteCache(state, rootDir, AddInCacheFileName, timeStamp);

                //Warn if there are any files that shouldn't be there
                String[] files = Directory.GetFiles(rootDir);
                String cacheFilePath = Path.Combine(rootDir, AddInCacheFileName);
                foreach (String file in files)
                {
                    if (!cacheFilePath.Equals(file) && IsDllOrExe(file))
                    {
                        warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.FileInAddInFolder, file));
                    }
                }
            }
            catch (DirectoryNotFoundException)
            {
                // the root doesn't exist. 
                return null;
            }
            catch (InvalidPipelineStoreException)
            {
                // the root doesn't exist.
                return null;
            }

            return state;
        }

        private static bool IsDllOrExe(String file)
        {
            return file.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) || file.EndsWith(".exe", StringComparison.OrdinalIgnoreCase);
        }

        // If they have full trust, give a good error message.  Otherwise, prevent information disclosure.
        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: AddInActivator.GetNamedPermissionSet(System.String):System.Security.PermissionSet" Ring="1" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        [System.Security.SecuritySafeCritical]
        internal static bool HasPathDiscovery(String path)
        {
            try
            {
                FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery, path);  
                permission.Demand();
                
                return true;
            }
            catch(Exception)
            {
                return false;
            }
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="Marshal.GetHRForException(System.Exception):System.Int32" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static void WriteCache(DeploymentState state, String path, String fileName, DateTime timeStamp)
        {
            // Separate determining the content of the file from writing the file.
            // We want to try doing the right thing when multiple processes are 
            // calling Update or Rebuild at the same time.  We've chosen to retry
            // three times, with a half second backoff each time.
            MemoryStream cache = new MemoryStream();
            using (MemoryStream serializedData = new MemoryStream())
            {
                String cacheFileName = Path.Combine(path, fileName);

                PermissionSet permissionSet = new PermissionSet(PermissionState.None);
                permissionSet.AddPermission(new SecurityPermission(SecurityPermissionFlag.SerializationFormatter));
                permissionSet.AddPermission(new FileIOPermission(FileIOPermissionAccess.AllAccess, cacheFileName));
                permissionSet.Assert();

                BinaryFormatter formatter = new BinaryFormatter();
                formatter.Serialize(serializedData, state);

                using (BinaryWriter bw = new BinaryWriter(cache))
                {
                    bw.Write(StoreFileFormatVersion);
                    bw.Write(serializedData.Length);
                    System.Diagnostics.Contracts.Contract.Assert(serializedData.Length <= Int32.MaxValue);
                    bw.Write(serializedData.GetBuffer(), 0, (int)serializedData.Length);

                    for (int numTries = 0; numTries < 4; numTries++) {
                        try {
                            using (FileStream s = File.Create(cacheFileName)) {
                                s.Write(cache.GetBuffer(), 0, (int) cache.Length);
                            }
                            // Set the last write time to ensure that any updates to
                            // the cache that were made while we were updating aren't lost.
                            // While setting the last write time requires a handle to the
                            // file, we must explicitly close the file the first time, then 
                            // do this step second, which requires opening another handle.
                            File.SetCreationTime(cacheFileName, timeStamp);
                            break;
                        }
                        catch (IOException e) {
                            // ---- sharing violations, sleep for half a second, and retry.
                            if ((uint) System.Runtime.InteropServices.Marshal.GetHRForException(e) != 
                                HRESULT_FOR_ERROR_SHARING_VIOLATION)
                                throw;
                            Thread.Sleep(500);
                        }
                    }
                }
            }
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="Marshal.GetHRForException(System.Exception):System.Int32" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Justification = "Reviewed")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static T ReadCache<T>(String storeFileName, bool mustExist)
        {
            // we require the caller to be able to control binary serialization, and we pre-emprively demand that here
            new SecurityPermission(SecurityPermissionFlag.SerializationFormatter).Demand();

            new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, storeFileName).Assert();

            BinaryFormatter formatter = new BinaryFormatter();
            T state = default(T);

            // The pipeline cache file must exist, but the addin cache might not
            if (!File.Exists(storeFileName))
            {
                if (mustExist)
                    throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, Res.CantFindDeployedAddInsFile, storeFileName));
                else
                    return state;
            }

            for (int numTries = 0; numTries < 4; numTries++) {
                try {
                    using (Stream s = File.OpenRead(storeFileName))
                    {
                        if (s.Length < 12)
                            throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, Res.DeployedAddInsFileCorrupted, storeFileName));

                        BinaryReader br = new BinaryReader(s);
                        int version = br.ReadInt32();
                        // Let's just assume that for all known versions of this file, 
                        // we'll be able to read it as-is.  We can compare the version number
                        // against StoreFileFormatVersion, but we don't know what to do 
                        // at this point in time.
                        long lengthOfSerializedBlob = br.ReadInt64();

                        try
                        {
                            state = (T)formatter.Deserialize(s);
                        }
                        catch (Exception e)
                        {
                            throw new InvalidOperationException(String.Format(CultureInfo.CurrentCulture, Res.CantDeserializeData, storeFileName), e);
                        }
                    }
                    break;
                }
                catch (IOException e) {
                    // ---- sharing violations, sleep for half a second, and retry.
                    if ((uint) System.Runtime.InteropServices.Marshal.GetHRForException(e) != 
                        HRESULT_FOR_ERROR_SHARING_VIOLATION)
                        throw;
                    Thread.Sleep(500);
                }
            }

            return state;
        }

        //<SecurityKernel Critical="True" Ring="0">
        //<Asserts Name="Imperative: System.Security.Permissions.FileIOPermission" />
        //</SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal static String GetAppBase()
        {
            FileIOPermission permission = new FileIOPermission(PermissionState.None);
            permission.AllFiles = FileIOPermissionAccess.PathDiscovery;
            permission.Assert();
            return AppDomain.CurrentDomain.BaseDirectory;
        }

        // Look in this assembly for the specified type of pipeline component.
        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: InspectionWorker.Inspect(System.AddIn.PipelineComponentType):System.AddIn.Hosting.InspectionResults" Ring="1" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification="Reviewed")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static bool Discover(String assemblyFileName, PipelineComponentType componentType, PipelineDeploymentState state, 
                String rootDir, Collection<String> warnings)
        {
            // Set appBase to a directory that contains no other types.           
            String appBase = GetAppBase();
            
            InspectionResults results;
            AppDomain domain = null;
            try {
                domain = CreateWorkerDomain(appBase);
                ObjectHandle inspectionWorkerHandle = Activator.CreateInstance(domain, typeof(InspectionWorker).Assembly.FullName, typeof(InspectionWorker).FullName);
                InspectionWorker worker = (InspectionWorker)inspectionWorkerHandle.Unwrap();
                results = worker.Inspect(componentType, assemblyFileName, rootDir);
            }
            finally {
                if (domain != null) 
                    Utils.UnloadAppDomain(domain);
            }

            foreach (String warning in results.Warnings)
                warnings.Add(warning);

            List<PipelineComponent> pipelineComponents = results.Components;
            if (pipelineComponents.Count > 0) {
                switch (componentType) {
                    case PipelineComponentType.Contract:
                        state.Contracts.AddRange(new ContravarianceAdapter<PipelineComponent, ContractComponent>(pipelineComponents));
                        break;

                    case PipelineComponentType.AddInAdapter:
                        state.AddInAdapters.AddRange(new ContravarianceAdapter<PipelineComponent, AddInAdapter>(pipelineComponents));
                        break;

                    case PipelineComponentType.AddInBase:
                        state.AddInBases.AddRange(new ContravarianceAdapter<PipelineComponent, AddInBase>(pipelineComponents));
                        break;

                    default:
                        System.Diagnostics.Contracts.Contract.Assert(false, "Fell through switch in Discover!");
                        break;
                }
                return true;
            }
            return false;
        }

        // Assert full trust because we are seeing a full-trust demand here when there is
        // an AppDomainManager configured that has an internal constructor
        [System.Security.SecurityCritical]
        [PermissionSet(SecurityAction.Assert, Unrestricted=true)]
        private static AppDomain CreateWorkerDomain(string appBase)
        {
            return AppDomain.CreateDomain("Add-In Model Discovery worker AD", null, appBase, null, false);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        private static void DiscoverHostAdapters(String assemblyFileName, List<HostAdapter> container, String rootDir, Collection<String> warnings)
        {
            TypeInfo.SetWarnings(warnings);
            int numFound = 0;
            MiniAssembly assembly = null;
            try
            {
                assembly = new MiniAssembly(assemblyFileName);
                String relativeFileName = Utils.MakeRelativePath(assemblyFileName, rootDir);

                assembly.DependencyDirs.Add(Path.Combine(rootDir, ContractsDirName));

                foreach (TypeInfo type in assembly.GetTypesWithAttribute(typeof(HostAdapterAttribute), true))
                {
                    HostAdapter ha = new HostAdapter(type, relativeFileName);
                    if (ha.Validate(type, warnings))
                    {
                        container.Add(ha);
#if ADDIN_VERBOSE_WARNINGS
                        warnings.Add(String.Format(CultureInfo.CurrentCulture, "Found a {0}.  Name: {1}  Assembly: {2}", PipelineComponentType.HostAdapter, ha.TypeInfo.FullName, ha.AssemblySimpleName));
#endif
                        numFound++;
                    }
                }
            }
            catch (GenericsNotImplementedException)
            {
                // could be caused by a generic HAV or a generic contract.  The user will be warned elsewhere.
            }
            catch (Exception e)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.InspectingAssemblyThrew, e.GetType().Name, e.Message, assemblyFileName));
            }
            finally
            {
                if (assembly != null)
                    assembly.Dispose();
                TypeInfo.SetWarnings(null);
            }

            if (numFound == 0) {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.NoAddInModelPartsFound, PipelineComponentType.HostAdapter, assemblyFileName));
            }
        }

        // <SecurityKernel Critical="True" Ring="1">
        // <ReferencesCritical Name="Method: InspectionWorker.Inspect(System.AddIn.PipelineComponentType):System.AddIn.Hosting.InspectionResults" Ring="1" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="System.Collections.Generic.List<System.AddIn.AddIn>")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static AddIn DiscoverAddIn(String assemblyPath, Collection<String> warnings, String fullTypeName)
        {
            TypeInfo.SetWarnings(warnings);
            MiniAssembly assembly = null;
            try
            {
                // Assert permission to the directory so that we can load the assembly and any helper assemblies.
                String directory = Path.GetDirectoryName(assemblyPath);
                FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.Read | FileIOPermissionAccess.PathDiscovery, directory);
                permission.Assert();

                String assemblyFileName = Path.GetFileName(assemblyPath); 
                assembly = new MiniAssembly(assemblyPath);

                int i = fullTypeName.LastIndexOf('.');
                String typeName;
                String nameSpace = String.Empty;
                if (i > 0) {
                    typeName = fullTypeName.Substring(i+1);
                    nameSpace = fullTypeName.Substring(0, i);
                }
                else
                    typeName = fullTypeName;
                
                TypeInfo type = assembly.FindTypeInfo(typeName, nameSpace);

                if (type != null)
                {
                    AddIn addIn = new AddIn(type, assemblyPath, assemblyPath, assembly.FullName);
                    //default the name to the type name
                    addIn.UnlocalizedResourceState.Name = typeName; 
                    if (addIn.Validate(type, warnings))
                        return addIn;
                }
            }
            finally
            {
                if (assembly != null)
                    assembly.Dispose();
                TypeInfo.SetWarnings(null);
            }

            return null;
        }


        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes")]
        private static bool DiscoverAddIns(String assemblyFileName, List<AddIn> container, String rootDir, Collection<String> warnings)
        {
            TypeInfo.SetWarnings(warnings);
            int numFound = 0;
            MiniAssembly assembly = null;
            try
            {
                assembly = new MiniAssembly(assemblyFileName);
                String relativeFileName = Utils.MakeRelativePath(assemblyFileName, rootDir);

                // If there are any addinbases in the wrong place, warn and don't return any addin tokens for this folder.
                foreach (TypeInfo type in assembly.GetTypesWithAttribute(typeof(AddInBaseAttribute), true))
                {
                    throw new AddInBaseInAddInFolderException(
                            String.Format(CultureInfo.CurrentCulture, 
                                Res.ComponentInWrongLocation, assemblyFileName, rootDir));
                }

                foreach (TypeInfo type in assembly.GetTypesWithAttribute(typeof(AddInAttribute)))
                {
                    AddIn addIn = new AddIn(type, relativeFileName, assemblyFileName, assembly.FullName);
                    if (addIn.Validate(type, warnings))
                    {
                        container.Add(addIn);
                    }
#if ADDIN_VERBOSE_WARNINGS
                    warnings.Add(String.Format(CultureInfo.CurrentCulture, "Found a {0}.  Name: {1}  Assembly: {2}", componentType, addIn.TypeInfo.FullName, addIn.AssemblySimpleName));
#endif
                    numFound++;
                }
            }
            catch (GenericsNotImplementedException)
            {
                // The user will be warned elsewhere. 
            } 
            catch (AddInBaseInAddInFolderException)
            {
                throw;
            }
            catch (Exception e)
            {
                warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.InspectingAssemblyThrew, e.GetType().Name, e.Message, assemblyFileName));
            }
            finally
            {
                if (assembly != null)
                    assembly.Dispose();
                TypeInfo.SetWarnings(null);
            }

            return numFound > 0;
        }

        private static List<AddInToken> ConnectPipelinesWithAddIns(List<PartialToken> partialTokens, TypeInfo havType, 
                params AddInDeploymentState[] addInStores )
        {
            List<AddInToken> validPipelines = new List<AddInToken>();
            foreach (PartialToken partialToken in partialTokens)
            {
                foreach (AddInDeploymentState addInDeploymentState in addInStores)
                {
                    foreach (AddIn addin in addInDeploymentState.AddIns)
                    {
                        // see if there is a match between any of the addin's known parents and the addInBase for this partial token
                        if (Contains(addin.AddInBaseTypeInfo, partialToken._addinBase.TypeInfo))
                        {
                            // Record that we were able to use these parts
                            // 
                            partialToken._addinBase.ConnectedToNeighbors = true;
                            addin.ConnectedToNeighbors = true;

                            AddInToken pipeline;
                            if (Contains(partialToken._hostAdapter.HostAddinViews, havType))
                            {
                                pipeline = new AddInToken(partialToken._hostAdapter, partialToken._contract, 
                                        partialToken._addinAdapter, partialToken._addinBase, addin);
                                pipeline.ResolvedHostAddInView = havType;
                                validPipelines.Add(pipeline);
                            }
                        }
                    } // foreach addin
                } // foreach addInDeploymentStore
            }

            RemoveDuplicatePipelines(ref validPipelines);

            return validPipelines;
        }

        [Pure]
        internal static bool Contains(TypeInfo[] array, TypeInfo info)
        {
            foreach (TypeInfo ti in array) {
                if (ti.Equals(info))
                    return true;
            }
            return false;
        }

        private static void RemoveDuplicatePipelines(ref List<AddInToken> validPipelines)
        {
            // Remove any duplicates by creating a new list and only copy the 
            // correct ones to it.  In the end we will replace the given list with the pruned one
            List<AddInToken> result = new List<AddInToken>(validPipelines.Count);

            // Resolve multiple pipelines to the same addin if they don't have qualification data.  Our heuristic will
            // be to alphabetize the pipeline components and pick the last one
            // alphabetically.  The identity for an add-in must be a combination
            // of the HAV's type name and the add-in's name (based on directory 
            // structure, I think).
            //   Look for duplicate add-ins.
            Dictionary<String, AddInToken> uniqueAddIns = new Dictionary<String, AddInToken>(StringComparer.OrdinalIgnoreCase);

            foreach (AddInToken pipeline in validPipelines)
            {
                // Ones with qualification data go straight to the "Valid" list.  Only ones without
                // such data get subjected to further scrutiny.
                if (pipeline.HasQualificationDataOnPipeline) {
                    result.Add(pipeline);
                    continue;
                }

                // Identify add-ins by location on disk, the type name for
                // the add-in, & HAV.  Then sort components based on 
                // assembly names.  (We can have multiple add-ins in the
                // same assembly.)
                String addInID = pipeline.HostViewId;
                AddInToken dupPipeline;

                if (!uniqueAddIns.TryGetValue(addInID, out dupPipeline))
                    uniqueAddIns[addInID] = pipeline;
                else
                {
                    // We know that the add-in and the host add-in view are 
                    // identical now.  Let's pick one of them in a deterministic
                    // fashion, even if we can't find a good heuristic.

                    // @



                    int r = String.CompareOrdinal(pipeline._hostAdapter.TypeInfo.AssemblyQualifiedName,
                        dupPipeline._hostAdapter.TypeInfo.AssemblyQualifiedName);
                    if (r < 0) {
                        continue;
                    }
                    else if (r > 0)
                    {
                        uniqueAddIns[addInID] = pipeline;
                        continue;
                    }

                    r = String.CompareOrdinal(pipeline._contract.TypeInfo.AssemblyQualifiedName,
                        dupPipeline._contract.TypeInfo.AssemblyQualifiedName);
                    if (r < 0)
                    {
                        continue;
                    }
                    else if (r > 0)
                    {
                        uniqueAddIns[addInID] = pipeline;
                        continue;
                    }

                    r = String.CompareOrdinal(pipeline._addinAdapter.TypeInfo.AssemblyQualifiedName,
                        dupPipeline._addinAdapter.TypeInfo.AssemblyQualifiedName);
                    if (r < 0)
                    {
                        continue;
                    }
                    else if (r > 0)
                    {
                        uniqueAddIns[addInID] = pipeline;
                        continue;
                    }

                    r = String.CompareOrdinal(pipeline._addinBase.TypeInfo.AssemblyQualifiedName,
                        dupPipeline._addinBase.TypeInfo.AssemblyQualifiedName);
                    if (r < 0)
                    {
                        continue;
                    }
                    else if (r > 0)
                    {
                        uniqueAddIns[addInID] = pipeline;
                        continue;
                    }
                }
            }
            foreach (AddInToken tok in uniqueAddIns.Values)
            {
                result.Add(tok);
            }

            validPipelines = result;
        }

        // <SecurityKernel Critical="True" Ring="0">
        // <Asserts Name="Imperative: System.Security.Permissions.FileIOPermission" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        private static String GetFullPath(string path)
        {
            FileIOPermission permission = new FileIOPermission(PermissionState.None);
            permission.AllFiles = FileIOPermissionAccess.PathDiscovery;
            permission.Assert();
            return Path.GetFullPath(path);
        }

        // This is used when we we don't want to disclose information to partial-trust hosts.
        private static SecurityException GetGenericSecurityException()
        {
            return new SecurityException(Res.GenericSecurityExceptionMessage);
        }

    }

    [Serializable]
    internal class AddInBaseInAddInFolderException : Exception 
    {
       public AddInBaseInAddInFolderException(String message)
           : base(message) { }

       [SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode")]
       public AddInBaseInAddInFolderException(String message, Exception innerException)
           : base(message, innerException) { }

       protected AddInBaseInAddInFolderException(SerializationInfo info, StreamingContext context)
           : base(info, context) { }

       public AddInBaseInAddInFolderException() { }
    }

}

