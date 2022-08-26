// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  MiniModule
**
** Purpose: Wraps a module, using a managed PE reader to
**     interpret the metadata.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;
using System.AddIn.MiniReflection.MetadataReader;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection
{
    [Serializable]
    internal class MiniModule : IDisposable
    {
        [NonSerialized]
        protected PEFileReader _peFile;
        private MiniAssembly _assembly;
        private String _moduleName;

        protected MiniModule(String peFileName)
        {
            System.Diagnostics.Contracts.Contract.Requires(peFileName != null);

            _peFile = new PEFileReader(peFileName);
            _moduleName = Path.GetFileNameWithoutExtension(peFileName);
        }

        protected MiniModule()
        {
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (_peFile != null)
            {
                if (disposing)
                    _peFile.Dispose();
                _peFile = null;
            }
        }

        // Spec#: this should be protected or private, not public.  Long term, this
        // should be represented as a "model variable".
        public bool IsDisposed {
            [Pure]
            get { return _peFile == null; }
        }

        public MiniAssembly Assembly {
            get {
                System.Diagnostics.Contracts.Contract.Ensures(System.Diagnostics.Contracts.Contract.Result<MiniAssembly>() != null);
                System.Diagnostics.Contracts.Contract.Assert(_assembly != null);
                return _assembly;
            }
            protected internal set { 
                System.Diagnostics.Contracts.Contract.Requires(value != null);
                _assembly = value;
            }
        }

        public String ModuleName {
            get { return _moduleName; }
        }

        internal PEFileReader PEFileReader {
            get {
                if (IsDisposed)
                    throw new ObjectDisposedException(null);
                System.Diagnostics.Contracts.Contract.EndContractBlock();
                return _peFile;
            }
        }

        public IList<MetadataToken> GetGenericTypes()
        {
            // get a list of generic types by looking for Type owners in GenericParam table
            List<MetadataToken> genericTypeTokens = new List<MetadataToken>();
            uint rowsGeneric = _peFile.MetaData.RowsInTable(MDTables.Tables.GenericParam);
            for (uint i = 0; i < rowsGeneric; i++)
            {
                _peFile.MetaData.SeekToRowOfTable(MDTables.Tables.GenericParam, i);
                _peFile.B.ReadUInt16(); // number
                _peFile.B.ReadUInt16(); // flags
                MetadataToken genericTypeToken = _peFile.MetaData.ReadMetadataToken(MDTables.Encodings.TypeOrMethodDef); // owner
                genericTypeTokens.Add(genericTypeToken);
            }

            return genericTypeTokens;
        }

        // Only returns public types.
        public IList<TypeInfo> GetTypesWithAttributeInModule(Type customAttribute)
        {
            return GetTypesWithAttributeInModule(customAttribute, false);
        }

        public IList<TypeInfo> GetTypesWithAttributeInModule(Type customAttribute, bool includePrivate)
        {
            if (IsDisposed)
                throw new ObjectDisposedException(null);
            if (customAttribute == null)
                throw new ArgumentNullException("customAttribute");
            System.Diagnostics.Contracts.Contract.EndContractBlock();

            _peFile.InitMetaData();
            MDTables MetaData = _peFile.MetaData;

            List<TypeInfo> types = new List<TypeInfo>();
            String attributeName = customAttribute.Name;
            String attributeNameSpace = customAttribute.Namespace;

            IList<MetadataToken> genericTypeTokens = GetGenericTypes();

            uint numRows = MetaData.RowsInTable(MDTables.Tables.CustomAttribute);
            for (uint i = 0; i < numRows; i++)
            {
                MetaData.SeekToRowOfTable(MDTables.Tables.CustomAttribute, i);

                // Format: Parent type token, CA type token, value (index into blob heap)
                MetadataToken targetType = MetaData.ReadMetadataToken(MDTables.Encodings.HasCustomAttribute);
                MetadataToken caType = MetaData.ReadMetadataToken(MDTables.Encodings.CustomAttributeType);
                //UInt32 value = MetaData.ReadBlobIndex();
                //Console.WriteLine("CA - Applied to: {0}  CA .ctor: {1}  Value: {2}", targetType, caType, value);
                //Console.WriteLine("CA MD Tokens  Parent: {0}  Type: {1}", targetType.ToMDToken(), caType.ToMDToken());

                // Ensure the custom attribute type is the type we expect
                MetaData.SeekToMDToken(caType);
                String caTypeName = null, caNameSpace = null;
                if (caType.Table != MDTables.Tables.MemberRef)
                {
                    // Custom attribute was defined in the assembly we are currently inspecting?
                    // Ignore it.
                    System.Diagnostics.Contracts.Contract.Assert(caType.Table == MDTables.Tables.MethodDef);
                    continue;
                }
                MetadataToken customAttributeType = MetaData.ReadMetadataToken(MDTables.Encodings.MemberRefParent);
                //Console.WriteLine("   MemberRef: {0}  Type of MemberRef: {1}", caType.ToMDToken(), customAttributeType.ToMDToken());
                MetaData.SeekToMDToken(customAttributeType);
                MetadataToken resolutionScope = MetaData.ReadMetadataToken(MDTables.Encodings.ResolutionScope);
                // @
                caTypeName = MetaData.ReadString();
                if (!String.Equals(caTypeName, attributeName))
                    continue;
                caNameSpace = MetaData.ReadString();
                if (!String.Equals(caNameSpace, attributeNameSpace))
                    continue;

                // Get type name & namespace.
                switch (targetType.Table)
                {
                    case MDTables.Tables.TypeDef:
                        MetaData.SeekToMDToken(targetType);
                        System.Reflection.TypeAttributes flags = (System.Reflection.TypeAttributes) _peFile.B.ReadUInt32();
                        System.Reflection.TypeAttributes vis = flags & System.Reflection.TypeAttributes.VisibilityMask;
                        bool isPublic = vis == System.Reflection.TypeAttributes.Public; // NestedPublic not supported
                        if (!includePrivate && !isPublic) 
                            continue;
                        String typeName = MetaData.ReadString();
                        String nameSpace = MetaData.ReadString();
                        bool isGeneric = genericTypeTokens.Contains(targetType);
                        TypeInfo type = new TypeInfo(targetType, _assembly, this, typeName, nameSpace, isGeneric);
                        types.Add(type);
                        break;

                    default:
                        throw new NotImplementedException(String.Format(CultureInfo.CurrentCulture, Res.UnknownTokenType, targetType.Table, Assembly.FullName));
                }
            }
            return types;
        }
    }
}
