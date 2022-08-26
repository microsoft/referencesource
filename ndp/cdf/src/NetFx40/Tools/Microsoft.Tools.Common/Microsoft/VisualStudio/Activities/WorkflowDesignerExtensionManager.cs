// <copyright>
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>

namespace Microsoft.VisualStudio.Activities
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel.Composition.Hosting;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Reflection;

    internal abstract class WorkflowDesignerExtensionManager
    {
        private static MethodInfo internalGetExtensionMethodInfo;
        private static MethodInfo internalGetExtensionsMethodInfo;
        private IExceptionLogger logger;

        internal WorkflowDesignerExtensionManager(IExceptionLogger logger)
        {
            Debug.Assert(logger != null, "Logger should not be null and is ensured by caller.");
            this.logger = logger;
        }

        protected abstract string ExtensionsDirectory
        {
            get;
        }

        private static MethodInfo InternalGetExtensionMethodInfo
        {
            get
            {
                if (WorkflowDesignerExtensionManager.internalGetExtensionMethodInfo == null)
                {
                    // This is a justified private reflection since it access itself only.
                    internalGetExtensionMethodInfo = typeof(WorkflowDesignerExtensionManager).GetMethod("InternalGetExtension", BindingFlags.Instance | BindingFlags.NonPublic);
                }

                return WorkflowDesignerExtensionManager.internalGetExtensionMethodInfo;
            }
        }

        private static MethodInfo InternalGetExtensionsMethodInfo
        {
            get
            {
                if (WorkflowDesignerExtensionManager.internalGetExtensionsMethodInfo == null)
                {
                    // This is a justified private reflection since it access itself only.
                    internalGetExtensionsMethodInfo = typeof(WorkflowDesignerExtensionManager).GetMethod("InternalGetExtensions", BindingFlags.Instance | BindingFlags.NonPublic);
                }

                return WorkflowDesignerExtensionManager.internalGetExtensionsMethodInfo;
            }
        }

        internal static string GetExtensionsDirectory(string relativePathToExtensionsDirectory)
        {
            string assemblyLocation = Assembly.GetExecutingAssembly().Location;
            string assemblyDirectory = Path.GetDirectoryName(assemblyLocation);
            return Path.Combine(assemblyDirectory, relativePathToExtensionsDirectory);
        }

        internal object GetExtension(string extensionTypeAssemblyQualifiedName)
        {
            VSDesignerPerfEventProvider perfEventProvider = new VSDesignerPerfEventProvider();
            perfEventProvider.WriteEvent(VSDesignerPerfEvents.WorkflowDesignerExtensionManagerGetExtensionStart);
            try
            {
                using (ExtensionAssemblyResolvingScope scope = new ExtensionAssemblyResolvingScope(this.logger, this.ExtensionsDirectory))
                {
                    Type hostServiceType = Type.GetType(extensionTypeAssemblyQualifiedName);
                    if (hostServiceType == null)
                    {
                        return null;
                    }

                    return this.GetExtension(hostServiceType);
                }
            }
            finally
            {
                perfEventProvider.WriteEvent(VSDesignerPerfEvents.WorkflowDesignerExtensionManagerGetExtensionEnd);
            }
        }

        internal T GetExtension<T>()
        {
            return this.InternalGetExtension<T>();
        }

        internal IEnumerable<object> GetExtensions(string extensionTypeAssemblyQualifiedName)
        {
            using (ExtensionAssemblyResolvingScope scope = new ExtensionAssemblyResolvingScope(this.logger, this.ExtensionsDirectory))
            {
                Type hostServiceType = Type.GetType(extensionTypeAssemblyQualifiedName);
                if (hostServiceType == null)
                {
                    return null;
                }

                return this.GetExtensions(hostServiceType);
            }
        }

        internal IEnumerable<T> GetExtensions<T>()
        {
            return this.InternalGetExtensions<T>();
        }

        protected abstract void PrepareContext(CompositionContainer container);

        private CompositionContainer PrepareCompositionContainer()
        {
            CompositionContainer container = new CompositionContainer(new DirectoryCatalog(this.ExtensionsDirectory));
            this.PrepareContext(container);
            return container;
        }

        private object GetExtension(Type hostServiceType)
        {
            return WorkflowDesignerExtensionManager.InternalGetExtensionMethodInfo.MakeGenericMethod(hostServiceType).Invoke(this, null);
        }

        private IEnumerable<object> GetExtensions(Type hostServiceType)
        {
            return WorkflowDesignerExtensionManager.InternalGetExtensionsMethodInfo.MakeGenericMethod(hostServiceType).Invoke(this, null) as IEnumerable<object>;
        }

        private T InternalGetExtension<T>()
        {
            if (Directory.Exists(this.ExtensionsDirectory))
            {
                return this.PrepareCompositionContainer().GetExportedValueOrDefault<T>();
            }
            else
            {
                return default(T);
            }
        }

        private IEnumerable<T> InternalGetExtensions<T>()
        {
            if (Directory.Exists(this.ExtensionsDirectory))
            {
                return this.PrepareCompositionContainer().GetExportedValues<T>();
            }
            else
            {
                // Make it convenient for the caller so that they don't have to do NULL check.
                return new Collection<T>();
            }
        }

        private class ExtensionAssemblyResolvingScope : IDisposable
        {
            private string extensionsDirectory;
            private ResolveEventHandler extensionAssemblyResolver;
            private IExceptionLogger logger;

            public ExtensionAssemblyResolvingScope(IExceptionLogger logger, string extensionsDirectory)
            {
                Debug.Assert(logger != null, "Logger should not be null and is ensured by caller.");
                Debug.Assert(extensionsDirectory != null, "extensionsDirectory should not be null and is ensured by caller.");
                this.logger = logger;
                this.extensionsDirectory = extensionsDirectory;
                AppDomain.CurrentDomain.AssemblyResolve += this.ExtensionAssemblyResolver;
            }

            private ResolveEventHandler ExtensionAssemblyResolver
            {
                get
                {
                    if (this.extensionAssemblyResolver == null)
                    {
                        this.extensionAssemblyResolver = new ResolveEventHandler(this.ResolveExtensionAssembly);
                    }

                    return this.extensionAssemblyResolver;
                }
            }

            public void Dispose()
            {
                AppDomain.CurrentDomain.AssemblyResolve -= this.ExtensionAssemblyResolver;
            }

            [SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Justification = "We need to call LoadAssembly here")]
            private Assembly ResolveExtensionAssembly(object sender, ResolveEventArgs args)
            {
                // Find the assembly in extensions path
                string assemblyName = args.Name.Substring(0, args.Name.IndexOf(",", StringComparison.Ordinal));
                string assemblyFullPath = Path.Combine(this.extensionsDirectory, assemblyName + ".dll");
                try
                {
                    return Assembly.LoadFrom(assemblyFullPath);
                }
                catch (Exception ex)
                {
                    if (ExceptionUtility.IsFatal(ex))
                    {
                        throw;
                    }

                    this.logger.LogException(ex);
                    return null;
                }
            }
        }
    }
}
