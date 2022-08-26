// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ResourceProvider
**
** Purpose: Stores & retrieves resources for an add-in.  May
**     instantiate the ResourceManager in a new appdomain, etc.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Globalization;
using System.Reflection;
using System.Resources;
using System.Threading;
using System.Diagnostics.Contracts;

namespace System.AddIn
{
    // Resource lookup must be done at the time that someone uses the add-in 
    // token, which is both after discovery has completed and before activation 
    // has occurred.  We may be using a different culture than when we 
    // completed discovery, so this means we must do the resource lookup 
    // ourselves using the current UI culture, constructing our own 
    // ResourceManager for the add-in.  To ensure that we unload the add-in's
    // assembly, we'll create another appdomain here.  This is expensive, but
    // it seems to be the best alternative, short of forcing users to use
    // Win32 file version resources for this purpose (which compromises on 
    // some issues).
#if LOCALIZABLE_ADDIN_ATTRIBUTE  // Disabled in Orcas for scheduling, security & a required fix in red bits
    internal static class ResourceProvider
    {
        private sealed class ResourceLookupWorker : MarshalByRefObject
        {
            public ResourceState Lookup(String addInAssemblyFileName, String resMgrBaseName, String nameResource, String publisherResource, String descriptionResource)
            {
                Contract.Requires(Path.IsPathRooted(addInAssemblyFileName));

                // @TODO: This should be using ReflectionOnlyLoadFrom to ensure we
                // don't run into potential security issues with module constructors,
                // but the V2 ResourceManager uses normal reflection to get the custom
                // attributes on an assembly, so this throws an exception on V2.  
                // In V3, we can change the ResourceManager to call 
                // GetCustomAttributeData instead, and use ReflectionOnlyLoadFrom here.
                Assembly addIn = Assembly.LoadFrom(addInAssemblyFileName);
                ResourceManager resMgr = new ResourceManager(resMgrBaseName, addIn);
                CultureInfo culture = Thread.CurrentThread.CurrentUICulture;
                ResourceState resState = new ResourceState();
                resState.CultureName = culture.Name;
                if (nameResource != null)
                    resState.Name = resMgr.GetString(nameResource, culture);
                if (publisherResource != null)
                    resState.Publisher = resMgr.GetString(publisherResource, culture);
                if (descriptionResource != null)
                    resState.Description = resMgr.GetString(descriptionResource, culture);
                return resState;
            }
        }

        internal static ResourceState LookupResourcesInNewDomain(String addInAssemblyFileName,
            String resMgrBaseName, String nameResource, String publisherResource, String descriptionResource)
        {
            AppDomain domain = AppDomain.CreateDomain("Add-in model resource lookup domain");
            try {
                ResourceLookupWorker worker = (ResourceLookupWorker)domain.CreateInstanceAndUnwrap(
                    typeof(ResourceProvider).Assembly.FullName,
                    typeof(ResourceProvider.ResourceLookupWorker).FullName);
                return worker.Lookup(addInAssemblyFileName, resMgrBaseName, nameResource, publisherResource, descriptionResource);
            }
            finally {
                AppDomain.Unload(domain);
            }
        }

        internal static ResourceState LookupResourcesInCurrentDomain(String addInAssemblyFileName,
            String resMgrBaseName, String nameResource, String publisherResource, String descriptionResource)
        {
            ResourceLookupWorker worker = new ResourceLookupWorker();
            ResourceState state = worker.Lookup(addInAssemblyFileName, resMgrBaseName, nameResource, publisherResource, descriptionResource);
            return state;
        }
    }
#endif // LOCALIZABLE_ADDIN_ATTRIBUTE
}
