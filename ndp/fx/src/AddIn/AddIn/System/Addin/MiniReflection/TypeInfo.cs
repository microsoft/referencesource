// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  TypeInfo
**
** Purpose: Abstracts out the notion of a type, so we can 
**     support generic type parameters & possible "pipeline
**     constraints" based on the types present in the pipeline.
**
===========================================================*/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;
using System.Text;
using System.AddIn.MiniReflection.MetadataReader;
using System.AddIn.Hosting;
using System.Diagnostics.Contracts;

namespace System.AddIn.MiniReflection
{
    // While this is currently just a wrapper for a String (the 
    // assembly-qualified type name), eventually we should store more info
    // here, such as what the generic type parameters are to a class,
    // and perhaps constraint-like info.  Say we have an add-in adapter 
    // w/ a constructor taking an add-in base called AB<int>.  The add-in base
    // of course is AB<T>.  We can only connect from the add-in adapter
    // all the way through the add-in if the add-in subclasses AB<int>.
    // The hope is this class will provide the right level of abstraction to
    // support generic add-in parts in the future.
    [Serializable]
    internal sealed class TypeInfo : IEquatable<TypeInfo>, ISerializable
    {
        [Flags]
        internal enum Representation {
            None = 0,
            Token = 1,
            Name = 2,
            AssemblyQualifiedName = 4,
            ReflectionType = 8,
            TypeRef = 0x10,
        }

        private static volatile ICollection<String> Warnings;
        private const String AssemblyFieldName = "_assembly";
        private const String ModuleFieldName = "_module";
        private const String MDTokenFieldName = "_mdToken";
        private const String TypeNameFieldName = "_typeName";
        private const String NameSpaceFieldName = "_nameSpace";
        private const String AssemblyQualifiedFieldName = "_assemblyQualifiedName";
        private const String RepresentationFieldName = "_representation";
        private const String IsGenericFieldName = "_isGeneric";

        private readonly MiniAssembly _assembly; // note that for TypeInfos coming from TypeRefs, this is the referencing assembly
        private readonly MiniModule _module;
        private readonly MetadataToken _mdToken;
        private volatile String _typeName;
        private volatile String _nameSpace;
        private volatile String _assemblyQualifiedName;
        private readonly static UTF8Encoding s_encoder = new UTF8Encoding(false, true);

        [NonSerialized]
        private readonly Type _reflectionType;  // Don't load Type objects in the wrong AD!
        private volatile Representation _representation;

        private bool _isGeneric;

        internal TypeInfo(MetadataToken typeDef, MiniAssembly assembly)
        {
            System.Diagnostics.Contracts.Contract.Requires(assembly != null);
            System.Diagnostics.Contracts.Contract.Requires(typeDef.Table == MDTables.Tables.TypeDef);

            _mdToken = typeDef;
            _assembly = assembly;
            _representation = Representation.Token;
            PEFileReader peFile = _assembly.PEFileReader;
            MDTables mdScope = peFile.MetaData;

            mdScope.SeekToMDToken(_mdToken);
            peFile.B.ReadUInt32();  // TypeAttributes;
            _typeName = mdScope.ReadString();  // this type's name
            _nameSpace = mdScope.ReadString();  // this type's namespace
            _representation |= Representation.Name;
        }

        internal TypeInfo(MetadataToken typeDef, MiniAssembly assembly, String typeName, String nameSpace)
        {
            System.Diagnostics.Contracts.Contract.Requires(assembly != null);
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(typeName));
            System.Diagnostics.Contracts.Contract.Requires(typeDef.Table == MDTables.Tables.TypeDef);

            _mdToken = typeDef;
            _assembly = assembly;
            _typeName = typeName;
            _nameSpace = nameSpace;
            _assemblyQualifiedName = FullTypeName + ", " + assembly.FullName;
            _representation = Representation.Token | Representation.Name | Representation.AssemblyQualifiedName;
        }

        internal TypeInfo(MetadataToken typeDef, MiniAssembly assembly, MiniModule module, String typeName, String nameSpace, bool isGeneric)
        {
            System.Diagnostics.Contracts.Contract.Requires(assembly != null);
            System.Diagnostics.Contracts.Contract.Requires(module != null);
            System.Diagnostics.Contracts.Contract.Requires(!String.IsNullOrEmpty(typeName));
            System.Diagnostics.Contracts.Contract.Requires(typeDef.Table == MDTables.Tables.TypeDef);

            _mdToken = typeDef;
            _assembly = assembly;
            _module = module;
            _typeName = typeName;
            _nameSpace = nameSpace;
            _assemblyQualifiedName = FullTypeName + ", " + assembly.FullName;
            _representation = Representation.Token | Representation.Name | Representation.AssemblyQualifiedName;
            _isGeneric = isGeneric;
        }

        internal String FullTypeName
        {
            get { return String.IsNullOrEmpty(_nameSpace) ? _typeName : _nameSpace + "." + _typeName; }
        }

        // security attributes are needed because of mscorlib.GetName() calls
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "disambiguatingJunkForTypeRefOverload")]
        [System.Security.SecuritySafeCritical]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal TypeInfo(MetadataToken typeRefToken, MiniAssembly referencingAssembly, bool disambiguatingJunkForTypeRefOverload)
        {
            System.Diagnostics.Contracts.Contract.Requires(referencingAssembly != null);
            System.Diagnostics.Contracts.Contract.Requires(typeRefToken.Table == MDTables.Tables.TypeRef);

            _mdToken = typeRefToken;
            _assembly = referencingAssembly;
            _representation = Representation.TypeRef | Representation.Name;

            PEFileReader peFile = referencingAssembly.PEFileReader;
            MDTables MetaData = peFile.MetaData;
            MetaData.SeekToMDToken(typeRefToken);
            MetadataToken assemblyRef = MetaData.ReadMetadataToken(MDTables.Encodings.ResolutionScope);
            _typeName = MetaData.ReadString();
            _nameSpace = MetaData.ReadString();

            // Read assembly information
            MetaData.SeekToMDToken(assemblyRef);
            UInt16 major = peFile.B.ReadUInt16();
            UInt16 minor = peFile.B.ReadUInt16();
            UInt16 build = peFile.B.ReadUInt16();
            UInt16 revision = peFile.B.ReadUInt16();
            peFile.B.ReadUInt32(); // assembly flags
            byte[] publicKeyOrToken = MetaData.ReadBlob();
            String simpleName = MetaData.ReadString();
            String culture = MetaData.ReadString();

            // assert so we can get the AssemblyName of mscorlib.dll
            FileIOPermission permission = new FileIOPermission(PermissionState.None);
            permission.AllLocalFiles = FileIOPermissionAccess.PathDiscovery;
            permission.Assert();

            System.Reflection.Assembly mscorlib = typeof(Object).Assembly;
            if (simpleName == "mscorlib" && (culture.Length == 0) && Utils.PublicKeyMatches(mscorlib.GetName(), publicKeyOrToken))
            {
                // Upgrade to using a Type!
                Type t = mscorlib.GetType(FullName, false);
                if (t != null)
                {
                    _reflectionType = t;
                    _representation |= Representation.ReflectionType;
                }
            }

            String typeRefDefiningAssemblyName = String.Format(CultureInfo.InvariantCulture, 
                "{0}, Version={1}.{2}.{3}.{4}, Culture={5}, PublicKeyToken={6}", 
                simpleName, major, minor, build, revision, 
                (culture.Length == 0 ? "neutral" : culture), Utils.PublicKeyToString(publicKeyOrToken));
            _assemblyQualifiedName = FullName + ", " + typeRefDefiningAssemblyName;
            _representation |= Representation.AssemblyQualifiedName;
        }

        internal TypeInfo(Type type)
        {
            System.Diagnostics.Contracts.Contract.Requires(type != null);

            _reflectionType = type;
            _assemblyQualifiedName = type.AssemblyQualifiedName;
            _isGeneric = type.ContainsGenericParameters || type.IsGenericType || type.GetGenericArguments().Length > 0;

            _typeName = type.Name;
            _nameSpace = type.Namespace;
            if (_nameSpace == null)
                _nameSpace = String.Empty;
            _representation = Representation.Name | Representation.AssemblyQualifiedName | Representation.ReflectionType;

            System.Diagnostics.Contracts.Contract.Assert(_assembly == null);  // Don't do this, or we might serialize assemblies!
        }

        internal TypeInfo(SerializationInfo info, StreamingContext context)
        {
            _assembly = (MiniAssembly) info.GetValue(AssemblyFieldName, typeof(MiniAssembly));
            _module = (MiniModule) info.GetValue(ModuleFieldName, typeof(MiniModule));
            _mdToken = (MetadataToken) info.GetValue(MDTokenFieldName, typeof(MetadataToken));
            _typeName = info.GetString(TypeNameFieldName);
            _nameSpace = info.GetString(NameSpaceFieldName);
            _assemblyQualifiedName = info.GetString(AssemblyQualifiedFieldName);
            _representation = (Representation) info.GetInt32(RepresentationFieldName);
            System.Diagnostics.Contracts.Contract.Assert((_representation & Representation.ReflectionType) == 0);
            _isGeneric = info.GetBoolean(IsGenericFieldName);
        }

        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue(AssemblyFieldName, _assembly);
            info.AddValue(ModuleFieldName, _module);
            info.AddValue(MDTokenFieldName, _mdToken);
            info.AddValue(TypeNameFieldName, _typeName);
            info.AddValue(NameSpaceFieldName, _nameSpace);
            info.AddValue(AssemblyQualifiedFieldName, _assemblyQualifiedName);
            Representation r = _representation & ~Representation.ReflectionType;
            info.AddValue(RepresentationFieldName, r);
            info.AddValue(IsGenericFieldName, _isGeneric);
        }

        internal static void SetWarnings(ICollection<String> warnings)
        {
            Warnings = warnings;
        }

        internal Type ReflectionType
        {
            get { 
                System.Diagnostics.Contracts.Contract.Assert(HasReflectionType);
                return _reflectionType;
            }
        }

        private bool HasAssemblyQualifiedName {
            get { return (_representation & Representation.AssemblyQualifiedName) != 0; }
        }

        private bool HasName {
            get { return (_representation & Representation.Name) != 0; }
        }

        private bool HasToken
        {
            [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Justification = "Needed for Contracts conditional compilation")]
            get { return (_representation & Representation.Token) != 0; }
        }

        internal bool HasReflectionType {
            [Pure]
            get { return (_representation & Representation.ReflectionType) != 0; }
        }

        private bool HasTypeRef {
            get { return (_representation & Representation.TypeRef) != 0; }
        }

        internal MiniAssembly Assembly {
            get {
                System.Diagnostics.Contracts.Contract.Ensures(System.Diagnostics.Contracts.Contract.Result<MiniAssembly>() != null);

                // Do not store the assembly in this object - make sure it doesn't
                // leak across appdomains, etc.
                MiniAssembly retVal = _assembly;
                if (retVal == null)
                {
                    System.Diagnostics.Contracts.Contract.Assert(HasReflectionType);
                    retVal = new MiniAssembly(_reflectionType.Assembly);
                }
                return retVal;
            }
        }

        /*
        internal bool IsClass {
            get {
                if (HasReflectionType)
                    return _reflectionType.IsClass;

                if (HasTypeRef) {
                    TypeInfo def = TypeRefToTypeDef(_mdToken, true);
                    return def.IsClass;
                }

                Contract.Assert(HasToken);
                PEFileReader peFile = _assembly.PEFileReader;
                MDTables mdScope = peFile.MetaData;
                mdScope.SeekToMDToken(_mdToken);
                System.Reflection.TypeAttributes attrs = (System.Reflection.TypeAttributes)peFile.B.ReadUInt32();  // TypeAttributes;
                return (attrs & System.Reflection.TypeAttributes.Class) != 0;
            }
        }
        */

        internal bool IsGeneric {
            get { return _isGeneric; }
        }

        public bool IsInterface {
            get {
                if (HasReflectionType)
                    return _reflectionType.IsInterface;

                if (HasTypeRef) {
                    TypeInfo def = TypeRefToTypeDef(_mdToken, true);
                    return def.IsInterface;
                }

                System.Diagnostics.Contracts.Contract.Assert(HasToken);
                PEFileReader peFile = _assembly.PEFileReader;
                MDTables mdScope = peFile.MetaData;
                mdScope.SeekToMDToken(_mdToken);
                System.Reflection.TypeAttributes attrs = (System.Reflection.TypeAttributes) peFile.B.ReadUInt32();  // TypeAttributes;
                return (attrs & System.Reflection.TypeAttributes.Interface) != 0;
            }
        }

        /*
        public bool IsMarshalByRef {
            get {
                if (HasReflectionType)
                    return _reflectionType.IsMarshalByRef;
                throw new NotImplementedException("TypeInfo::IsMarshalByRef on non-Reflection types is not implemented");
            }
        }

        public bool IsSealed {
            get {
                if (HasReflectionType)
                    return _reflectionType.IsSealed;

                if (HasTypeRef) {
                    TypeInfo def = TypeRefToTypeDef(_mdToken, true);
                    return def.IsSealed;
                }

                Contract.Assert(HasToken);
                PEFileReader peFile = _assembly.PEFileReader;
                MDTables mdScope = peFile.MetaData;
                mdScope.SeekToMDToken(_mdToken);
                System.Reflection.TypeAttributes attrs = (System.Reflection.TypeAttributes)peFile.B.ReadUInt32();  // TypeAttributes;
                return (attrs & System.Reflection.TypeAttributes.Sealed) != 0;
            }
        }
        */

        public String Name
        {
            get {
                System.Diagnostics.Contracts.Contract.Assert(HasName);
                return _typeName;
            }
        }

        public String FullName {
            get {
                System.Diagnostics.Contracts.Contract.Assert(HasName);
                if (String.IsNullOrEmpty(_nameSpace))
                    return _typeName;
                return _nameSpace + "." + _typeName;
            }
        }

        public String AssemblyQualifiedName {
            get {
                System.Diagnostics.Contracts.Contract.Assert(HasName);
                if (HasAssemblyQualifiedName) 
                    return _assemblyQualifiedName;

                System.Diagnostics.Contracts.Contract.Assert(!HasTypeRef);

                if (HasName) {
                    _assemblyQualifiedName = FullName + ", " + _assembly.FullName;
                    _representation |= Representation.AssemblyQualifiedName;
                    return _assemblyQualifiedName;
                }
                
                throw new NotImplementedException();
            }
        }

        public String AssemblyName {
            get {
                System.Diagnostics.Contracts.Contract.Assert(_assembly != null || HasAssemblyQualifiedName);

                if (HasAssemblyQualifiedName)
                {
                    int firstComma = _assemblyQualifiedName.IndexOf(',');
                    return _assemblyQualifiedName.Substring(firstComma + 2);
                }
                else
                {
                    System.Diagnostics.Contracts.Contract.Assert(_assembly != null);
                    String asmName = _assembly.FullName;
                    _assemblyQualifiedName = FullName + ", " + _assembly.FullName;
                    _representation |= Representation.AssemblyQualifiedName;
                    return asmName;
                }
            }
        }

        public TypeInfo BaseType {
            get {

                if (HasTypeRef)
                {
                    TypeInfo defInfo = TypeRefToTypeDef(_mdToken, true);
                    return defInfo.BaseType;
                }

                System.Diagnostics.Contracts.Contract.Assert(HasToken);
                System.Diagnostics.Contracts.Contract.Assert(!HasTypeRef);
                PEFileReader peFile = _assembly.PEFileReader;
                MDTables mdScope = peFile.MetaData;

                mdScope.SeekToMDToken(_mdToken);
                peFile.B.ReadUInt32();  // TypeAttributes;
                if (HasName) {
                    mdScope.ReadStringIndex();  // this type's name
                    mdScope.ReadStringIndex();  // this type's namespace
                }
                else {
                    _typeName = mdScope.ReadString();  // this type's name
                    _nameSpace = mdScope.ReadString();  // this type's namespace
                    _representation |= Representation.Name;
                }

                MetadataToken baseClass = peFile.MetaData.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);
                return TypeInfoFromTypeDefOrRef(baseClass);
            }
        }

        private TypeInfo TypeInfoFromTypeDefOrRef(MetadataToken token)
        {
            if (token.Table == MDTables.Tables.TypeRef)
            {
                // Call our TypeRef constructor.  The HAV will be in another assembly.
                return new TypeInfo(token, _assembly, false);
            }
            else if (token.Table == MDTables.Tables.TypeDef)
            {
                // The base is defined in this same assembly, not in the separate HAV.  
                return new TypeInfo(token, _assembly);
            }
            // else it is a TypeSpec -- a generic type
            
            throw new GenericsNotImplementedException();
        }
        

        public MiniConstructorInfo[] GetConstructors()
        {
            return GetConstructors(false);
        }

        public MiniConstructorInfo[] GetConstructors(bool includePrivate)
        {
            System.Diagnostics.Contracts.Contract.Assert(HasToken/* || HasReflectionType*/, "GetConstructors needs a token (or you should uncomment the support for Reflection types)");

            List<MiniConstructorInfo> ctors = new List<MiniConstructorInfo>();

            /*
            if (HasReflectionType) {
                System.Reflection.BindingFlags visibility = System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public;
                if (includePrivate)
                    visibility |= System.Reflection.BindingFlags.NonPublic;
                foreach (System.Reflection.ConstructorInfo ctor in _reflectionType.GetConstructors(visibility))
                    ctors.Add(new MiniConstructorInfo(ctor));
                return ctors.ToArray();
            }
             */

            System.Diagnostics.Contracts.Contract.Assert(_mdToken.Table == MDTables.Tables.TypeDef);

            PEFileReader peFile = _assembly.PEFileReader;
            peFile.InitMetaData();
            MDTables MetaData = peFile.MetaData;

            MetaData.SeekToMDToken(_mdToken);
            System.Reflection.TypeAttributes flags = (System.Reflection.TypeAttributes) peFile.B.ReadUInt32();
            System.Reflection.TypeAttributes vis = System.Reflection.TypeAttributes.VisibilityMask & flags;
            bool isPublic = (vis == System.Reflection.TypeAttributes.Public); // don't support NestedPublic 
            if (!includePrivate && !isPublic)
                return new MiniConstructorInfo[0];
            MetaData.ReadStringIndex();  // typename
            MetaData.ReadStringIndex();  // namespace

            MetadataToken baseClass = MetaData.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);  // Base class            
            uint firstMemberIndex = MetaData.ReadRowIndex(MDTables.Tables.FieldDef); // Field list
            uint firstMethodIndex = MetaData.ReadRowIndex(MDTables.Tables.MethodDef);  // Method list
            uint lastMethodIndex;
            // If this is the last entry in the TypeDef table, then all the rest of the methods in the MethodDef
            // table belong to this type.  Otherwise, look for the methods belonging to the next type.
            if (_mdToken.Index == MetaData.RowsInTable(MDTables.Tables.TypeDef))
            {
                lastMethodIndex = MetaData.RowsInTable(MDTables.Tables.MethodDef);
            }
            else
            {
                MetaData.SeekToRowOfTable(MDTables.Tables.TypeDef, _mdToken.Index);  // Seek to next type (not off by 1!)
                peFile.B.ReadUInt32(); // Flags
                MetaData.ReadStringIndex();  // type name
                MetaData.ReadStringIndex();  // namespace
                MetaData.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);  // Next type's base class
                MetaData.ReadRowIndex(MDTables.Tables.FieldDef);  // field list;
                uint firstMethodOfNextType = MetaData.ReadRowIndex(MDTables.Tables.MethodDef);  // method list
                lastMethodIndex = firstMethodOfNextType - 1;
            }

            // Now walk through list of methods, looking for ones w/ the name ".ctor".
            for (uint i = firstMethodIndex; i <= lastMethodIndex; i++)
            {
                MetadataToken method = new MetadataToken(MDTables.Tables.MethodDef, i);
                MetaData.SeekToMDToken(method);
                UInt32 rva = peFile.B.ReadUInt32();
                UInt16 implFlags = peFile.B.ReadUInt16();  // MethodImplAttributes
                System.Reflection.MethodAttributes attrs = (System.Reflection.MethodAttributes)peFile.B.ReadUInt16();  // Flags - MethodAttributes
                // Visibility check
                if (!includePrivate && (attrs & System.Reflection.MethodAttributes.Public) == 0)
                    continue;
                String methodName = MetaData.ReadString();  // Name
                // @
                if (!String.Equals(methodName, ".ctor"))
                    continue;

                byte[] sig = MetaData.ReadBlob();
                try
                {
                    MiniParameterInfo[] parameters = ParseSig(sig);
                    ctors.Add(new MiniConstructorInfo(parameters));
                }
                catch (GenericsNotImplementedException)
                {
                    // may be caused by a Generic contract.  The user will be warned elsewhere that generic contracts are not supported.
                    /*
                    if (Warnings != null) {
                        lock (Warnings) {
                            Warnings.Add(String.Format(CultureInfo.CurrentCulture, Res.UnparsibleConstructorSignature, this.Name, e.GetType().Name, e.Message));
                        }
                    }
                    */
                }
            } // for each .ctor
            return ctors.ToArray();
        }

        // See the ECMA CLI spec, Partition II, section 23.2.1.
        private MiniParameterInfo[] ParseSig(byte[] sig)
        {
            PEFileReader peFile = _assembly.PEFileReader;
            peFile.InitMetaData();
            MDTables MetaData = peFile.MetaData;

            uint i = 0;
            // The first byte of the Signature holds bits for HASTHIS, EXPLICITTHIS 
            // and calling convention (DEFAULT, VARARG, or GENERIC). These are OR'ed 
            // together.
            i++;

            uint numParams = DecodeInteger(sig, ref i);

            // Skip over return type
            //   Skip custom modifiers.
            do
            {
                if (sig[i] == (byte) CorElementType.CModOpt) i++;
                else if (sig[i] == (byte) CorElementType.CModReqd) i++;
                else break;
            } while (true);
            //   Skip return type.  Note that for constructors, it should be void.
            if (sig[i] == (byte) CorElementType.Void) i++;
            else if (sig[i] == (byte) CorElementType.TypedByRef) i++;
            else
            {
                if (sig[i] == (byte) CorElementType.ByRef) i++;
                ParseType(sig, ref i, false, true);
            }
            
            MiniParameterInfo[] parameters = new MiniParameterInfo[numParams];
            for (uint paramNum = 0; paramNum < numParams; paramNum++)
            {
                parameters[paramNum] = ParseType(sig, ref i, false, false);
                //Console.WriteLine("Read a parameter: {0}", parameters[paramNum]);
            }
            return parameters;
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Justification="bogus warning")]
        private MiniParameterInfo ParseType(byte[] sig, ref uint i,
                     bool allowTypedByRef, bool allowVoid)
        {
            CorElementType b = (CorElementType) sig[i++];
            switch (b)
            {
                case CorElementType.Boolean:
                case CorElementType.Char:
                case CorElementType.I1:
                case CorElementType.U1:
                case CorElementType.I2:
                case CorElementType.U2:
                case CorElementType.I4:
                case CorElementType.U4:
                case CorElementType.I8:
                case CorElementType.U8:
                case CorElementType.R4:
                case CorElementType.R8:
                case CorElementType.I:
                case CorElementType.U:
                case CorElementType.String:
                case CorElementType.Object:
                    return new MiniParameterInfo(b);

                case CorElementType.TypedByRef:
                    //AddPrimitiveTypeName((Int32)B, SB);
                    if (!allowTypedByRef) throw new BadImageFormatException(Res.TypedByRefNotAllowed);//TypedByRef not allowed here
                    throw new NotImplementedException();

                case CorElementType.Void:
                    if (!allowVoid) throw new BadImageFormatException();
                    return new MiniParameterInfo(b);

                case CorElementType.Class:
                case CorElementType.ValueType:
                    return new MiniParameterInfo(b, ParseTypeDefRefOrSpec(sig, ref i));

                case CorElementType.Ptr:
                    // PTR CustomMod* VOID | PTR CustomMod* Type;
                    /*
                    AddCustomMods(Bytes, ref i, SB);
                    SB.Append("Ptr to ");
                    AddType(Bytes, ref i, SB, false, true);   // VOID is allowed here
                     */
                    throw new NotImplementedException();

                case CorElementType.FnPtr:
                    // FNPTR MethodDefSig | FNPTR MethodRefSig;
                    //AddMethodSig(Bytes, ref i, SB);
                    throw new NotImplementedException();
                case CorElementType.SzArray:
                    //AddSzArray(Bytes, ref i, SB);
                    throw new NotImplementedException();
                case CorElementType.Array:
                    //AddArray(Bytes, ref i, SB);
                    throw new NotImplementedException();
                case CorElementType.ByRef:
                    throw new NotImplementedException();
                case CorElementType.GenericInst:
                    // may be caused by the contract being a generic type
                    throw new GenericsNotImplementedException();
                case CorElementType.Var:
                    throw new NotImplementedException();
                    
                default:
                    System.Diagnostics.Contracts.Contract.Assert(false, "Unrecognized CorElementType");
                    throw new BadImageFormatException(String.Format(CultureInfo.CurrentCulture, Res.UnrecognizedCorElementType, b));
            }
        }

        private TypeInfo ParseTypeDefRefOrSpec(byte[] sig, ref uint i)
        {
            // The encoded version of this TypeRef token is made follows up:
            // 1. encode the table that this token the indexes least significant 2 bits.
            //    The bit values to use are 0, 1 and 2, specifying the target table is the 
            //    TypeDef, TypeRef or TypeSpec table, respectively  
            // 2. shift the 3-byte row index (0x000012 in this example) left by 2 bits 
            //    and OR into the 2-bit encoding from step 1;
            // 3. compress the resulting value (see Section 22.2)
            uint compressed = DecodeInteger(sig, ref i);
            uint tableCode = compressed & 0x3;
            uint index = (uint) ((compressed & (~0x3)) / 4);
            MDTables.Tables[] mapToTable = new MDTables.Tables[]{MDTables.Tables.TypeDef, MDTables.Tables.TypeRef, MDTables.Tables.TypeSpec};

            MetadataToken token = new MetadataToken(mapToTable[tableCode], index);
            switch (tableCode)
            {
                case 0:  // TypeDef
                    return new TypeInfo(token, _assembly);
                
                case 1:  // TypeRef
                    return new TypeInfo(token, _assembly, false);  // TypeRef-specific constructor.

                case 2:  // TypeSpec
                    throw new NotImplementedException();

                default:
                    throw new BadImageFormatException(Res.InvalidMetadataToken);
            }
        }

        private static UInt32 DecodeInteger(byte[] bytes, ref uint i)
        {
            UInt32 b = bytes[i];
            i += 1;
            if ((b & 0x80) == 0) return b;
            else if ((b & 0xC0) == 0x80)
            {
                UInt32 nextB = bytes[i];
                i += 1;
                return ((b & 0x7F) * 0x100) | nextB;
            }
            else if ((b & 0xE0) == 0xC0)
            {
                UInt32 nextB, afterNext, final;
                nextB = bytes[i];
                afterNext = bytes[i + 1];
                final = bytes[i + 2];
                i += 3;
                return ((b & 0x1F) * 0x100000) | (nextB * 0x10000) |
                       (afterNext * 0x100) | final;
            }
            else throw new ArgumentOutOfRangeException("bytes",
                              b, Res.CodedIntegerTooLong);// "Coded integer more than 4 bytes long");
        }

        public bool Implements(TypeInfo ifaceType)
        {
            System.Diagnostics.Contracts.Contract.Requires(ifaceType != null);
            System.Diagnostics.Contracts.Contract.Assert(HasToken || HasReflectionType || HasTypeRef);

            if (HasReflectionType)
            {
                System.Diagnostics.Contracts.Contract.Assert(ifaceType.HasAssemblyQualifiedName);
                foreach(Type implementsIFace in _reflectionType.GetInterfaces())
                    if (Utils.FullTypeNameDefEqualsDef(implementsIFace.AssemblyQualifiedName, ifaceType.AssemblyQualifiedName))
                        return true;
                return false;
            }
            
            // This can be a typeref.  ---- the other assembly to parse this.
            // I need to support the HAV in assembly A, the HA in B, the specific contract 
            // interface in assembly C, and IContract in assembly D.  I'm inspecting the HA,
            // and I'll get a typeref for C, and I need to make sure it implements IContract in D.           

            PEFileReader peFile = _assembly.PEFileReader;
            peFile.InitMetaData();
            MDTables thisMetaData = peFile.MetaData;

            if (_mdToken.Table == MDTables.Tables.TypeRef)
            {
                TypeInfo def = TypeRefToTypeDef(_mdToken, true);
                return def.ImplementsHelper(def._assembly.PEFileReader, def._mdToken, ifaceType);
            }
            else
            {
                System.Diagnostics.Contracts.Contract.Assert(_mdToken.Table == MDTables.Tables.TypeDef);
                throw new NotImplementedException();
            }
        }

        // Does a given typedef in the specified PE file implement the specified interface?
        private bool ImplementsHelper(PEFileReader peFile, MetadataToken typeDefToken, TypeInfo ifaceType)
        {
            System.Diagnostics.Contracts.Contract.Requires(typeDefToken.Table == MDTables.Tables.TypeDef);

            MDTables mdScope = peFile.MetaData;

            // Walk through all rules of the interface implementation table, 
            // looking for this typeDefToken.  If we find it, check to see if
            // that row says this type implements the specified interface.
            // If not, continue walking the interface impl table.
            uint numRows = mdScope.RowsInTable(MDTables.Tables.InterfaceImpl);
            for (uint i = 0; i < numRows; i++)
            {
                mdScope.SeekToRowOfTable(MDTables.Tables.InterfaceImpl, i);
                uint typeDefRow = mdScope.ReadRowIndex(MDTables.Tables.TypeDef);
                if (typeDefRow == typeDefToken.Index)
                {
                    // if we implement it, return true.  Else continue.
                    MetadataToken interfaceToken = mdScope.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);
                    if (ifaceType.HasName)
                    {
                        mdScope.SeekToMDToken(interfaceToken);
                        // Interface for IContract should be a typeref.
                        switch (interfaceToken.Table)
                        {
                            case MDTables.Tables.TypeRef:
                                {
                                    MetadataToken resolutionScope = mdScope.ReadMetadataToken(MDTables.Encodings.ResolutionScope);
                                    String ifaceName = mdScope.ReadString();
                                    if (!String.Equals(ifaceName, ifaceType._typeName))
                                        continue;
                                    String ifaceNameSpace = mdScope.ReadString();
                                    if (!String.Equals(ifaceNameSpace, ifaceType._nameSpace))
                                        continue;
                                    if (MiniAssembly.Equals(ifaceType.Assembly, peFile, resolutionScope))
                                        return true;
                                    break;
                                }

                            case MDTables.Tables.TypeDef:
                                {
                                    // This type implements an interface defined in the same assembly.
                                    // This isn't really interesting for the add-in model, based on what
                                    // we've currently designed and our limited use of this class.
                                    mdScope.B.ReadUInt32();  // TypeAttributes
                                    String ifaceName = mdScope.ReadString();
                                    if (!String.Equals(ifaceName, ifaceType._typeName))
                                        continue;
                                    String ifaceNameSpace = mdScope.ReadString();
                                    if (!String.Equals(ifaceNameSpace, ifaceType._nameSpace))
                                        continue;
                                    if (this._assembly.Equals(ifaceType._assembly))
                                        return true;
                                    break;
                                }

                            case MDTables.Tables.TypeSpec:
                                // Since we're only looking for IContract, which is non-generic,
                                // we should ignore this row and move on.  
                                System.Diagnostics.Contracts.Contract.Assert(false, "Checking whether a type implements a TypeSpec is NYI (generic interface?)");
                                break;

                            default:
                                System.Diagnostics.Contracts.Contract.Assert(false, "Support for this interface type is NYI");
                                throw new NotImplementedException(String.Format(CultureInfo.CurrentCulture, Res.UnsupportedInterfaceType, interfaceToken.Table));
                        }
                    }
                    else
                    {
                        throw new NotImplementedException();
                    }
                }
            }
            return false;
        }

 
        internal TypeInfo TypeRefToTypeDef(MetadataToken typeRef, bool throwOnError)
        {
            System.Diagnostics.Contracts.Contract.Requires(typeRef.Table == MDTables.Tables.TypeRef);

            PEFileReader peFile = _assembly.PEFileReader;
            peFile.InitMetaData();
            MDTables thisMetaData = peFile.MetaData;
            thisMetaData.SeekToMDToken(_mdToken);
            MetadataToken resolutionScope = thisMetaData.ReadMetadataToken(MDTables.Encodings.ResolutionScope);
            String refTypeName = thisMetaData.ReadString();  // Name
            String refNamespace = thisMetaData.ReadString();  // Namespace
            System.Diagnostics.Contracts.Contract.Assert(resolutionScope.Table == MDTables.Tables.AssemblyRef);
            MiniAssembly definingAssembly = _assembly.ResolveAssemblyRef(resolutionScope, throwOnError);
            if (definingAssembly != null) {
                return definingAssembly.FindTypeInfo(refTypeName, refNamespace);
            }
            return null;
        }

        internal TypeInfo TryGetTypeDef()
        {
            //If the type came from mscorlib (e.g. MarshalByRefObject) then we have a reflection type.
            //Currently we just return null in this case, but if we later support arbitrary AddInBase classes
            //then we should return the real typedef
            if (HasReflectionType)
                return null;

            if (HasTypeRef)
                return TypeRefToTypeDef(_mdToken, false);

            return this;
        }

        // Get the immediate interfaces on this type.
        public TypeInfo[] GetInterfaces()
        {
            List<TypeInfo> interfaces = new List<TypeInfo>();
            if (HasReflectionType)
            {
                foreach (Type interfaceType in _reflectionType.GetInterfaces())
                    interfaces.Add(new TypeInfo(interfaceType));
            }
            else if (HasTypeRef)
            {
                TypeInfo defInfo = TypeRefToTypeDef(_mdToken, true);
                return defInfo.GetInterfaces();
            }
            else
            {
                System.Diagnostics.Contracts.Contract.Assert(_mdToken.Table == MDTables.Tables.TypeDef);

                PEFileReader peFile = _assembly.PEFileReader;
                MDTables mdScope = peFile.MetaData;

                // Walk through all rows of the interface implementation table, 
                // looking for this _mdToken.  
                uint numRows = mdScope.RowsInTable(MDTables.Tables.InterfaceImpl);
                for (uint i = 0; i < numRows; i++)
                {
                    try
                    {
                        mdScope.SeekToRowOfTable(MDTables.Tables.InterfaceImpl, i);
                        uint typeDefRow = mdScope.ReadRowIndex(MDTables.Tables.TypeDef);
                        if (typeDefRow == _mdToken.Index)
                        {
                            MetadataToken interfaceToken = mdScope.ReadMetadataToken(MDTables.Encodings.TypeDefOrRef);
                            interfaces.Add(TypeInfoFromTypeDefOrRef(interfaceToken));
                        }
                    }
                    catch (GenericsNotImplementedException)  // ignore interfaces such as IComparable<int> that aren't relevant to the addin model
                    {} 
                }
            }
            return interfaces.ToArray();
        }

        public override bool Equals(object obj)
        {
            TypeInfo that = obj as TypeInfo;
            if (that == null)
                return false;
            return this.Equals(that);
        }

        public bool Equals(TypeInfo value)
        {
            if (value == null)
                return false;

            String thisType = AssemblyQualifiedName;
            String thatType = value.AssemblyQualifiedName;

            bool r = Utils.FullTypeNameDefEqualsDef(thisType, thatType);
            return r;
        }

        public override int GetHashCode()
        {
            System.Diagnostics.Contracts.Contract.Assert(HasName);
            return AssemblyQualifiedName.GetHashCode();
        }

        public override String ToString()
        {
            if (_typeName != null)
                return _nameSpace + "::" + _typeName;
            else
                return "MD Token: " + _mdToken.ToString();
        }

 
        // Return the attributes on this type of the given custom attribute type
        internal MiniCustomAttributeInfo[] GetCustomAttributeInfos(Type caReflectedType)
        {
            List<MiniCustomAttributeInfo> result = new List<MiniCustomAttributeInfo>();

            PEFileReader peFile = _assembly.PEFileReader;
            peFile.InitMetaData();
            MDTables metaData = peFile.MetaData;

            uint numRows = metaData.RowsInTable(MDTables.Tables.CustomAttribute);
            for (uint i = 0; i < numRows; i++)
            {
                metaData.SeekToRowOfTable(MDTables.Tables.CustomAttribute, i);

                // Format: Parent type token, CA type token (really the constructor method), value (index into blob heap)
                MetadataToken targetType = metaData.ReadMetadataToken(MDTables.Encodings.HasCustomAttribute);
                MetadataToken caType = metaData.ReadMetadataToken(MDTables.Encodings.CustomAttributeType);
                byte[] caBlob = metaData.ReadBlob();

                if (targetType.Equals(this._mdToken)) {
                    //Console.WriteLine("CA - Applied to: {0}  CA .ctor: {1}  Value: {2}", targetType, caType, value);
                    //Console.WriteLine("CA MD Tokens  Parent: {0}  Type: {1}", targetType.ToMDToken(), caType.ToMDToken());

                    // Ensure the custom attribute type is the type we expect
                    metaData.SeekToMDToken(caType);
                    String caTypeName = null, caNameSpace = null;
                    if (caType.Table != MDTables.Tables.MemberRef)
                    {
                        // Custom attribute was defined in the assembly we are currently inspecting?
                        // Ignore it.
                        System.Diagnostics.Contracts.Contract.Assert(caType.Table == MDTables.Tables.MethodDef);
                        continue;
                    }

                    MetadataToken customAttributeType = metaData.ReadMetadataToken(MDTables.Encodings.MemberRefParent);

                    //Console.WriteLine("   MemberRef: {0}  Type of MemberRef: {1}", caType.ToMDToken(), customAttributeType.ToMDToken());
                    metaData.SeekToMDToken(customAttributeType);
                    MetadataToken resolutionScope = metaData.ReadMetadataToken(MDTables.Encodings.ResolutionScope);
                    caTypeName = metaData.ReadString();
                    caNameSpace = metaData.ReadString();

                    if (caTypeName == caReflectedType.Name && caNameSpace == caReflectedType.Namespace) { 
                        MiniCustomAttributeInfo customAttributeInfo = ParseCustomAttribute(caBlob, caReflectedType);
                        result.Add(customAttributeInfo);
                    }
                }
            }
            return result.ToArray();
        }

        private static MiniCustomAttributeInfo ParseCustomAttribute(byte[] caBlob, Type caReflectedType)
        {
            uint b = 2;  //skip prolog

            System.Reflection.BindingFlags visibility = System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.Public;
            List<MiniCustomAttributeFixedArgInfo> fixedArgs = new List<MiniCustomAttributeFixedArgInfo>();

            // to support multiple constructors we would need to look up the exact one in the method table
            System.Reflection.ConstructorInfo[] ctors = caReflectedType.GetConstructors(visibility);
            if (ctors.Length > 1)
                throw new NotImplementedException();

            System.Reflection.ConstructorInfo ctor = ctors[0];
            foreach(System.Reflection.ParameterInfo parameterInfo in ctor.GetParameters())
            {
                if (parameterInfo.ParameterType == typeof(String))
                {
                    String fixedParamString = ReadSerString(caBlob, ref b);
                    fixedArgs.Add(new MiniCustomAttributeFixedArgInfo(fixedParamString));
                }
                else
                    throw new NotImplementedException();
            }

            byte low = caBlob[b++];
            byte high = caBlob[b++];

            int numNamed = (high * 0x100) | low;

            List<MiniCustomAttributeNamedArgInfo> namedArgs = new List<MiniCustomAttributeNamedArgInfo>();
            for (int j=0; j < numNamed; j++) 
            {
                String propName, value;
                ReadNamedArg(caBlob, ref b, out propName, out value);

                namedArgs.Add(new MiniCustomAttributeNamedArgInfo(CorElementType.String, propName, value));
            }

            return new MiniCustomAttributeInfo(caReflectedType.Name, fixedArgs.ToArray(), namedArgs.ToArray());
        }

        private static string ReadSerString(byte[] bytes, ref uint b)
        {
            if (bytes[b] == 0xFF)
            {
                b++;
                return null;
            }
            uint len = DecodeInteger(bytes, ref b);
            String result = s_encoder.GetString(bytes, (int)b, (int)len);  
            b+= len;
            return result;
        }

        private static void ReadNamedArg(byte[] bytes, ref uint b, out String name, out String value)
        {
            int fieldOrProperty = bytes[b++];
            int fieldOrPropType = bytes[b++];

            if (fieldOrPropType != (int)CorElementType.String)
                throw new NotImplementedException();

            String fieldOrPropName = ReadSerString(bytes, ref b);
            name = fieldOrPropName;

            String fixedArg = ReadSerString(bytes, ref b);
            value = fixedArg;
        }
    }
}
