// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
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

    [Serializable]
    internal struct InspectionResults
    {
        internal List<PipelineComponent> Components;
        internal Collection<String> Warnings;
    }

    internal sealed class InspectionWorker : MarshalByRefObject
    {
        private String _assemblyFileName;
        private String _pipelineRootDirectory;  
        private PipelineComponentType _currentComponentType;
        private static Assembly SystemAddInInReflectionOnlyContext;
        private static Assembly SystemAddInContractsInReflectionOnlyContext;

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1810:InitializeReferenceTypeStaticFieldsInline", Justification="Initialization needs to be done in this order")]
        static InspectionWorker()
        {
            // Since we're using Reflection-only LoadFrom, we must pre-load 
            // dependent assemblies, such as System.AddIn.dll (this assembly).

            SystemAddInInReflectionOnlyContext = Assembly.ReflectionOnlyLoad(typeof(AddInStore).Assembly.FullName);
            SystemAddInContractsInReflectionOnlyContext = Assembly.ReflectionOnlyLoad(typeof(System.AddIn.Contract.IContract).Assembly.FullName);
            PipelineComponent.SetTypesFromReflectionLoaderContext(SystemAddInInReflectionOnlyContext, SystemAddInContractsInReflectionOnlyContext);
        }

        // Soon, this assembly resolve event won't be used during inspection.
        // But we'll need this exact same code during activation.
        internal Assembly ResolveAssembly(Object sender, ResolveEventArgs args)
        {
            String assemblyRef = args.Name;
            // I need to look in both the directory structure on disk for add-in
            // pipeline components, as well as in the GAC for assemblies like 
            // System.dll, which I haven't pre-loaded into the ReflectionOnly 
            // loader context.  
            // LoadFrom respects publisher policy, but ReflectionOnlyLoadFrom doesn't,
            // but that's OK.
            // Do I have to look in the GAC _first_ to mimic behavior
            // at runtime, which would respect policy when calling LoadFrom?
            // The problem with that is I'll be looking in the GAC for assemblies
            // that won't exist some portion of the time, meaning we have
            // to throw and ---- FileNotFoundExceptions.  The debugging experience
            // and performance are horrible.

            String simpleName = assemblyRef.Substring(0, assemblyRef.IndexOf(','));
            if (String.Equals(simpleName, "System.AddIn"))
                return SystemAddInInReflectionOnlyContext;
            if (String.Equals(simpleName, "System.AddIn.Contract"))
                return SystemAddInContractsInReflectionOnlyContext;
            String rootDir = Path.GetDirectoryName(Path.GetDirectoryName(_assemblyFileName));
            if (_currentComponentType == PipelineComponentType.AddIn)
                rootDir = Path.GetDirectoryName(rootDir);

            List<String> dirsToLookIn = new List<String>();
            switch(_currentComponentType)
            {
                case PipelineComponentType.HostAdapter:
                    // Look in contract directory.  For loading the HAV,
                    // we can't do that at discovery time since we don't know
                    // which directory contains the HAV.  This is why we're
                    // writing our own metadata parser in managed code.  
                    // At activation time, the HAV should already be loaded 
                    // within the host's AppDomain.
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.ContractsDirName));
                    break;

                case PipelineComponentType.Contract:
                    break;

                case PipelineComponentType.AddInAdapter:
                    // Look in contract directory and addin base directory.
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.ContractsDirName));
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.AddInBasesDirName));
                    break;

                case PipelineComponentType.AddInBase:
                    // look for other assemblies in the same folder
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInStore.AddInBasesDirName));
                    // @




                    break;

                    /*
                case PipelineComponentType.AddIn:
                    // Look in both the add-in's directory and the add-in base's
                    // directory.  We do the first by setting the app base for the AppDomain,
                    // but we may not be able to do that if the user created the appdomain.
                    dirsToLookIn.Add(Path.Combine(rootDir, AddInBasesDirName));
                    dirsToLookIn.Add(Path.GetDirectoryName(_assemblyFileName));
                    break;
                    */

                default:
                    System.Diagnostics.Contracts.Contract.Assert(false, "Fell through switch in the inspection assembly resolve event!");
                    break;
            }

            List<String> potentialFileNames = new List<string>(dirsToLookIn.Count * 2);
            foreach (String path in dirsToLookIn)
            {
                String simpleFileName = Path.Combine(path, simpleName);
                String dllName = simpleFileName + ".dll";
                if (File.Exists(dllName))
                    potentialFileNames.Add(dllName);
                else if (File.Exists(simpleFileName + ".exe"))
                    potentialFileNames.Add(simpleFileName + ".exe");
            }

            foreach (String fileName in potentialFileNames)
            {
                try
                {
                    Assembly a = Assembly.ReflectionOnlyLoadFrom(fileName);
                    // We should at least be comparing the public key token
                    // for the two assemblies here.  The version numbers may 
                    // potentially be different, dependent on publisher policy.
                    if (Utils.AssemblyRefEqualsDef(assemblyRef, a.FullName))
                        return a;
                }
                catch (BadImageFormatException)
                {
                }
            }

            // Look in the GAC.  It may not be there, so we need to catch 
            // FileNotFoundException.
            // As an optimization, don't probe in the GAC if the public
            // key token is null, because by definition the assembly 
            // can't be in the GAC.
            if (!assemblyRef.Contains("PublicKeyToken=null"))
            {
                try
                {
                        return Assembly.ReflectionOnlyLoad(assemblyRef);
                }
                catch (FileNotFoundException) {}
                
                try
                {
                    // As a final effort, look in the GAC after appying loader policy. 
                    // This lets us resolves references for assemblies built against an older version of the framework.
                    return Assembly.ReflectionOnlyLoad(AppDomain.CurrentDomain.ApplyPolicy(assemblyRef));
                }
                catch (FileNotFoundException) {}
            }
            
            //Console.WriteLine("Couldn't resolve assembly {0} while loading a {1}", simpleName, _currentComponentType);
            return null;
        }

        // Note that Inspect sets state around for the assembly resolve event in a way that isn't currently threadsafe.
        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="AppDomain.add_ReflectionOnlyAssemblyResolve(System.ResolveEventHandler):System.Void" />
        // <SatisfiesLinkDemand Name="AppDomain.remove_ReflectionOnlyAssemblyResolve(System.ResolveEventHandler):System.Void" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1506:AvoidExcessiveClassCoupling"), 
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Reviewed"),
         System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes"),
         System.Diagnostics.CodeAnalysis.SuppressMessage ("Microsoft.Security", "CA2103:ReviewImperativeSecurity")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal InspectionResults Inspect(PipelineComponentType componentType, string assemblyFileName, string pipelineRootDirectory)
        {
            System.Diagnostics.Contracts.Contract.Requires(assemblyFileName != null);
            System.Diagnostics.Contracts.Contract.Requires(pipelineRootDirectory != null);

            _assemblyFileName = assemblyFileName;
            _pipelineRootDirectory = pipelineRootDirectory;

            // Set up the assembly resolve event.
            _currentComponentType = componentType;
            ResolveEventHandler assemblyResolver = new ResolveEventHandler(ResolveAssembly);
            AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve += assemblyResolver;

            InspectionResults retval = new InspectionResults();
            retval.Components = new List<PipelineComponent>();
            retval.Warnings = new Collection<String>();
            Type[] publicTypes;
            String assemblyName = null;

            // Need to assert again here because we are in a new appdomain
            FileIOPermission permission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery |
                    FileIOPermissionAccess.Read, _pipelineRootDirectory);
            permission.Assert();

            try
            {
                // We want to load the assembly WITHOUT REGARD OF PUBLISHER POLICY.
                // If the directory structure contains v1.0 of a component and v1.1
                // exists in the GAC and is a security fix to v1.0, we still want to
                // inspect v1.0.  (The reason is we have other parts of the 
                // pipeline that were likely compiled against v1.0, not v1.1, and
                // we do type comparisons by comparing the fully qualified assembly 
                // name.)  LoadFrom unfortunately respects policy.  Assembly's
                // ReflectionOnlyLoad(byte[]) doesn't.  ReflectionOnlyLoadFrom(String)
                // does respect policy if you've set DEVPATH, but only as a bug.
                // We don't think setting DEVPATH is interesting.
                Assembly a = Assembly.ReflectionOnlyLoadFrom(_assemblyFileName);
                publicTypes = a.GetTypes();
                assemblyName = a.FullName;
            }
            catch (FileNotFoundException fnf)
            {
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AssemblyLoadFileNotFound, fnf.Message, fnf.FileName));
                return retval;
            }
            catch (Exception e)
            {
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AssemblyLoadThrew, e.GetType().Name, e.Message, _assemblyFileName));
                return retval;
            }
            PipelineComponent component = null;

            String relativeFileName = Utils.MakeRelativePath(_assemblyFileName, _pipelineRootDirectory);
            Type lastType = null;
            try
            {
                // Iterate over public types, looking for the appropriate custom attributes.
                foreach (Type type in publicTypes)
                {
                    component = null;
                    lastType = type;
                    switch (componentType)
                    {
                        case PipelineComponentType.Contract:
                            if (!Utils.HasCustomAttribute(PipelineComponent.ContractAttributeInReflectionLoaderContext, type))
                                continue;

                            component = new ContractComponent(new TypeInfo(type), relativeFileName);
                            break;

                        case PipelineComponentType.AddInAdapter:
                            if (!Utils.HasCustomAttribute(PipelineComponent.AddInAdapterAttributeInReflectionLoaderContext, type))
                                continue;

                            component = new AddInAdapter(new TypeInfo(type), relativeFileName);
                            break;

                        case PipelineComponentType.AddInBase:
                            if (Utils.HasCustomAttribute(PipelineComponent.AddInAttributeInReflectionLoaderContext, type))
                                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AddInInAddInViewFolder, type.Name, _assemblyFileName));

                            if (!Utils.HasCustomAttribute(PipelineComponent.AddInBaseAttributeInReflectionLoaderContext, type))
                                continue;

                            TypeInfo[] activatableAs = null;
                            CustomAttributeData cad = Utils.GetCustomAttributeData(PipelineComponent.AddInBaseAttributeInReflectionLoaderContext, type);
                            foreach(CustomAttributeNamedArgument cana in cad.NamedArguments)
                            {
                                if (cana.MemberInfo.Name == "ActivatableAs")
                                {
                                    CustomAttributeTypedArgument arg = cana.TypedValue;
                                    ReadOnlyCollection<CustomAttributeTypedArgument> types = (ReadOnlyCollection<CustomAttributeTypedArgument>)arg.Value;
                                    activatableAs = new TypeInfo[types.Count];
                                    int i = 0;
                                    foreach (CustomAttributeTypedArgument subArg in types)
                                    {
                                        activatableAs[i++] = new TypeInfo((Type)subArg.Value);
                                    }
                                }
                            }

                            component = new AddInBase(new TypeInfo(type), activatableAs, relativeFileName, assemblyName);

                            break;

                        default:
                            System.Diagnostics.Contracts.Contract.Assert(false, "Fell through switch - unrecognized componentType in InspectionWorker.Inspect");
                            break;
                    }  // switch

                    // If we found a component, make sure it satisfies all of its constraints, and give our
                    // PipelineComponents a chance to initialize state.
                    if (component != null)
                    {
                        if (component.Validate(type, retval.Warnings))
                            retval.Components.Add(component);
                    }
                } // foreach type in the assembly
            } // try
            catch (FileNotFoundException fnf)
            {
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.AssemblyLoadFileNotFound, fnf.Message, fnf.FileName));
                return retval;
            }
            catch (NotImplementedException)
            {
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.NotImplementedFeatureBadCtorParamOrAssembly, 
                    _assemblyFileName, (lastType == null) ? "" : lastType.FullName));
                return retval;
            }
            catch (Exception e)
            {
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.InspectingAssemblyThrew, e.GetType().Name, e.Message, _assemblyFileName));
                return retval;
            }

            AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve -= assemblyResolver;

            if (retval.Components.Count == 0 && _currentComponentType != PipelineComponentType.AddIn && _currentComponentType != PipelineComponentType.AddInBase)
            {
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.NoAddInModelPartsFound, componentType, _assemblyFileName));
            }
#if ADDIN_VERBOSE_WARNINGS
            foreach (PipelineComponent c in retval.Components)
                retval.Warnings.Add(String.Format(CultureInfo.CurrentCulture, "Found a {0}.  Name: {1}  Assembly: {2}", componentType, c.SimpleName, c.AssemblySimpleName));
#endif
            return retval;
        }
    }
}
