// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MiniAssembly
**
** Purpose: Wraps an assembly, using a managed PE reader to
**     interpret the metadata.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Text;
using System.AddIn.MiniReflection.MetadataReader;
using System.Diagnostics;
using System.AddIn.Hosting;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection
{
    [Serializable]
    internal sealed class MiniAssembly : MiniModule
    {
        private enum Representation {
            PEFileReader = 1,
            ReflectionAssembly = 2,
        }

        // When loading other assemblies, which directory should we look in?
        private List<String> _dependencyDirs;
        private Representation _representation;
        private System.Reflection.Assembly _reflectionAssembly;
        private String _fullName;

        public MiniAssembly(String peFileName) : base(peFileName)
        {
            _dependencyDirs = new List<String>();
            _dependencyDirs.Add(Path.GetDirectoryName(peFileName));
            Assembly = this;
            _representation = Representation.PEFileReader;
        }

        public MiniAssembly(System.Reflection.Assembly assembly)
        {
            System.Diagnostics.Contracts.Contract.Requires(assembly != null);

            _reflectionAssembly = assembly;
            _representation = Representation.ReflectionAssembly;
        }

        public List<String> DependencyDirs
        {
            get { return _dependencyDirs; }
        }

        internal bool IsReflectionAssembly {
            get { return (_representation & Representation.ReflectionAssembly) != 0; }
        }

        public MiniModule[] GetModules()
        {
            // There are two forms of "multiple module" assemblies.  The first is a 
            // multi-module assembly where the assembly's ModuleRef lists multiple 
            // different modules.  The second is a multi-file assembly, where an
            // entry in the File metadata table refers to another file on disk and
            // explicitly declares the file contains metadata (and potentially types).
            // ALink can produce .netmodules, which seem to show up as multi-file 
            // assemblies.
            MDTables metaData = _peFile.MetaData;
            
            for(uint i=0; i<metaData.RowsInTable(MDTables.Tables.File); i++)
            {
                metaData.SeekToRowOfTable(MDTables.Tables.File, i);
                MDFileAttributes attrs = (MDFileAttributes) metaData.B.ReadUInt32();
                if ((attrs & MDFileAttributes.ContainsNoMetaData) != 0)
                    continue;
                System.Diagnostics.Contracts.Contract.Assert(attrs == MDFileAttributes.ContainsMetaData);
                throw new NotImplementedException(String.Format(CultureInfo.CurrentCulture, Res.MultiFileAssembliesNotSupported, FullName));
            }

            return new MiniModule[] { this };
        }

        // Only returns public types.
        public IList<TypeInfo> GetTypesWithAttribute(Type customAttribute)
        {
            return GetTypesWithAttribute(customAttribute, false);
        }

        public TypeInfo FindTypeInfo(String typeName, String nameSpace)
        {
            System.Diagnostics.Contracts.Contract.Assert(!IsReflectionAssembly); // Can be implemented using Assembly.GetType
            MetadataToken token = FindTypeDef(_peFile, _peFile.MetaData, typeName, nameSpace);
            return new TypeInfo(token, this, typeName, nameSpace);
        }

        private static MetadataToken FindTypeDef(PEFileReader peFile, MDTables mdScope, String typeName, String nameSpace)
        {
            System.Diagnostics.Contracts.Contract.Requires(typeName != null);

            uint numTypeDefs = mdScope.RowsInTable(MDTables.Tables.TypeDef);
            for (uint i = 0; i < numTypeDefs; i++) {
                mdScope.SeekToRowOfTable(MDTables.Tables.TypeDef, i);
                peFile.B.ReadUInt32();  // TypeAttributes
                String rowTypeName = mdScope.ReadString();
                if (!String.Equals(typeName, rowTypeName))
                    continue;
                String rowNameSpace= mdScope.ReadString();
                if (!String.Equals(nameSpace, rowNameSpace))
                    continue;
                return new MetadataToken(MDTables.Tables.TypeDef, i + 1);
            }
            throw new TypeLoadException(String.Format(CultureInfo.CurrentCulture, Res.CantFindTypeName, nameSpace, typeName));
        }

        // For each module in the assembly, give back all types with the 
        // given attribute, possibly respecting the type's visibility.
        public IList<TypeInfo> GetTypesWithAttribute(Type customAttribute, bool includePrivate)
        {
            if (IsDisposed)
                throw new ObjectDisposedException(null);
            if (customAttribute == null)
                throw new ArgumentNullException("customAttribute");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            List<TypeInfo> types = new List<TypeInfo>();
            foreach (MiniModule module in GetModules())
            {
                IList<TypeInfo> newTypes = module.GetTypesWithAttributeInModule(customAttribute, includePrivate);
                types.AddRange(newTypes);
            }
            return types;
        }

        public MiniAssembly ResolveAssemblyRef(MetadataToken token, bool throwOnError)
        {
            System.Diagnostics.Contracts.Contract.Requires(token.Table == MDTables.Tables.AssemblyRef);
            PEFileReader peFile = this.PEFileReader;
            MDTables metaData = peFile.MetaData;

            metaData.SeekToMDToken(token);
            peFile.B.ReadUInt64();  // Skip 4 parts of the version number.
            peFile.B.ReadUInt32();  // AssemblyFlags
            byte[] publicKeyOrToken = metaData.ReadBlob(); // Public key or token
            String assemblySimpleName = metaData.ReadString(); // simple name
            String cultureName = metaData.ReadString();  // assembly culture
            if (!String.IsNullOrEmpty(cultureName))
                throw new BadImageFormatException(Res.UnexpectedlyLoadingASatellite, FullName);

            if (assemblySimpleName == "mscorlib" && (cultureName.Length == 0 || cultureName == "neutral"))
                return new MiniAssembly(typeof(Object).Assembly);

            MiniAssembly loadedAssembly = Open(assemblySimpleName, _dependencyDirs, throwOnError);

            if (loadedAssembly != null)
            {
                // Check whether the reference to the assembly matches what we actually loaded.
                // We don't respect the "throwOnError" parameter here because if someone does
                // violate this, they've either severely messed up their deployment, or they're
                // attempting a security exploit.
                System.Reflection.AssemblyName loadedAssemblyName = 
                    new System.Reflection.AssemblyName(loadedAssembly.FullName);

                if (!Utils.PublicKeyMatches(loadedAssemblyName, publicKeyOrToken))
                {
                    throw new FileLoadException(String.Format(CultureInfo.CurrentCulture, Res.AssemblyLoadRefDefMismatch, 
                        assemblySimpleName, publicKeyOrToken, loadedAssemblyName.GetPublicKeyToken()));
                }

                if (!String.IsNullOrEmpty(loadedAssemblyName.CultureInfo.Name))
                {
                    throw new FileLoadException(String.Format(CultureInfo.CurrentCulture, Res.AssemblyLoadRefDefMismatch,
                        assemblySimpleName, String.Empty, loadedAssemblyName.CultureInfo.Name));
                }
            }
            return loadedAssembly;
        }

        public static MiniAssembly Open(String simpleName, IList<String> dependencyDirs, bool throwOnError)
        {
            String fileName = FindAssembly(simpleName, dependencyDirs, throwOnError);
            if (!throwOnError && fileName == null)
                return null;
            return new MiniAssembly(fileName);
        }

        private static String FindAssembly(String simpleName, IList<String> searchDirs, bool throwOnError)
        {
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(simpleName));
            System.Diagnostics.Contracts.Contract.Requires(searchDirs != null);

            String libName = simpleName + ".dll";
            String exeName = simpleName + ".exe";
            foreach (String dir in searchDirs)
            {
                String fileName = Path.Combine(dir, libName);
                if (File.Exists(fileName))
                    return fileName;

                fileName = Path.Combine(dir, exeName);
                if (File.Exists(fileName))
                    return fileName;
            }
            if (throwOnError)
                throw new FileNotFoundException(String.Format(CultureInfo.CurrentCulture, Res.FileNotFoundForInspection, simpleName), libName);
            else
                return null;
        }

        internal String FullName {
            get {
                if ((_representation & Representation.ReflectionAssembly) != 0)
                    return AppDomain.CurrentDomain.ApplyPolicy(_reflectionAssembly.FullName);

                if (_fullName == null) {
                    AssemblyInfo assemblyInfo = new AssemblyInfo();
                    _peFile.GetAssemblyInfo(ref assemblyInfo);
                    _fullName = AppDomain.CurrentDomain.ApplyPolicy(assemblyInfo.ToString());
                }

                return _fullName;
            }
        }

        public override bool Equals(object obj)
        {
            MiniAssembly thatAssembly = obj as MiniAssembly;
            if (thatAssembly == null)
                return false;
            // Note that assembly binding redirects and publisher policy will affect
            // versioning (ie binds to 2.0.0.0 redirected to 2.0.1.5).  But for 
            // the Orcas release, we're only planning on using our existing 
            // directory structure, which does not have a great servicing story.
            // As long as we use ReflectionOnlyLoad during discovery, we should 
            // be fetching the exact assembly on disk used to build the entire 
            // pipeline (modulo in-place updates which must keep the same 
            // assembly version number), and we'll load the right version 
            // during activation time.  That should work for Orcas.
            // Long term, we may need an approximately-equals method that gets
            // the assembly info and compares the version number w.r.t. policy.
            return Utils.AssemblyDefEqualsDef(FullName, thatAssembly.FullName);
        }

        // See the comments in Equals - we're doing string comparisons here
        // to compare full type names.
        public static bool Equals(MiniAssembly assemblyA, PEFileReader peFileB, MetadataToken assemblyRefB)
        {
            System.Diagnostics.Contracts.Contract.Requires(assemblyA != null);
            System.Diagnostics.Contracts.Contract.Requires(peFileB != null);
            System.Diagnostics.Contracts.Contract.Requires(assemblyRefB.Table == MDTables.Tables.AssemblyRef);

            String nameA, nameRefB;
            if (assemblyA.IsReflectionAssembly)
                nameA = AppDomain.CurrentDomain.ApplyPolicy(assemblyA._reflectionAssembly.FullName);
            else
            {
                AssemblyInfo assemblyInfoA = new AssemblyInfo();
                assemblyA._peFile.GetAssemblyInfo(ref assemblyInfoA);
                nameA = AppDomain.CurrentDomain.ApplyPolicy(assemblyInfoA.ToString());
            }

            AssemblyInfo assemblyInfoB = ReadAssemblyRef(peFileB, assemblyRefB);
            nameRefB = AppDomain.CurrentDomain.ApplyPolicy(assemblyInfoB.ToString());

            return Utils.AssemblyRefEqualsDef(nameRefB, nameA);
        }

        private static AssemblyInfo ReadAssemblyRef(PEFileReader peFile, MetadataToken assemblyRef)
        {
            System.Diagnostics.Contracts.Contract.Requires(peFile != null);
            System.Diagnostics.Contracts.Contract.Requires(assemblyRef.Table == MDTables.Tables.AssemblyRef);
            MDTables metaData = peFile.MetaData;
            BinaryReader B = metaData.B;

            metaData.SeekToMDToken(assemblyRef);
            UInt16 major = B.ReadUInt16();
            UInt16 minor = B.ReadUInt16();
            UInt16 build = B.ReadUInt16();
            UInt16 revision = B.ReadUInt16();
            Version v = new Version(major, minor, build, revision);
            UInt32 assemblyFlags = B.ReadUInt32();
            byte[] publicKey = metaData.ReadBlob();
            String simpleName = metaData.ReadString();
            String culture = metaData.ReadString();
            if ((culture != null) && (culture.Length == 0)) culture = null;
            return new AssemblyInfo(v, assemblyFlags, publicKey, simpleName, culture);
        }

        public override int GetHashCode()
        {
            return FullName.GetHashCode();
        }
    }
}
