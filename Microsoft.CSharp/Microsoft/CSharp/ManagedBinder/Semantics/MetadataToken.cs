// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // Token  definitions
    using mdModule = mdToken;               // Module token (roughly, a scope)
    using mdTypeRef = mdToken;              // TypeRef reference (this or other scope)
    using mdTypeDef = mdToken;              // TypeDef in this scope
    using mdFieldDef = mdToken;             // Field in this scope
    using mdMethodDef = mdToken;            // Method in this scope
    using mdParamDef = mdToken;             // param token
    using mdInterfaceImpl = mdToken;        // interface implementation token

    using mdMemberRef = mdToken;            // MemberRef (this or other scope)
    using mdCustomAttribute = mdToken;      // attribute token
    using mdPermission = mdToken;           // DeclSecurity

    using mdSignature = mdToken;            // Signature object
    using mdEvent = mdToken;                // event token
    using mdProperty = mdToken;             // property token

    using mdModuleRef = mdToken;            // Module reference (for the imported modules)

    // Assembly tokens.
    using mdAssembly = mdToken;             // Assembly token.
    using mdAssemblyRef = mdToken;          // AssemblyRef token.
    using mdFile = mdToken;                 // File token.
    using mdExportedType = mdToken;         // ExportedType token.
    using mdManifestResource = mdToken;     // ManifestResource token.

    using mdTypeSpec = mdToken;             // TypeSpec object

    using mdGenericParam = mdToken;         // formal parameter to generic type or method
    using mdMethodSpec = mdToken;           // instantiation of a generic method
    using mdGenericParamConstraint = mdToken; // constraint on a formal generic parameter

    // Application string.
    using mdString = mdToken;               // User literal string token.

    using mdCPToken = mdToken;              // constantpool token

    enum mdToken
    {
        mdtModule = 0x00000000,       //
        mdtTypeRef = 0x01000000,       //
        mdtTypeDef = 0x02000000,       //
        mdtFieldDef = 0x04000000,       //
        mdtMethodDef = 0x06000000,       //
        mdtParamDef = 0x08000000,       //
        mdtInterfaceImpl = 0x09000000,       //
        mdtMemberRef = 0x0a000000,       //
        mdtCustomAttribute = 0x0c000000,       //
        mdtPermission = 0x0e000000,       //
        mdtSignature = 0x11000000,       //
        mdtEvent = 0x14000000,       //
        mdtProperty = 0x17000000,       //
        mdtModuleRef = 0x1a000000,       //
        mdtTypeSpec = 0x1b000000,       //
        mdtAssembly = 0x20000000,       //
        mdtAssemblyRef = 0x23000000,       //
        mdtFile = 0x26000000,       //
        mdtExportedType = 0x27000000,       //
        mdtManifestResource = 0x28000000,       //
        mdtGenericParam = 0x2a000000,       //
        mdtMethodSpec = 0x2b000000,       //
        mdtGenericParamConstraint = 0x2c000000,

        mdtString = 0x70000000,       //
        mdtName = 0x71000000,       //
        mdtBaseType = 0x72000000,       // Leave this on the high end value. This does not correspond to metadata table
    }

    // Note that this must be kept in sync with System.AttributeTargets.
    enum CorAttributeTargets
    {
        catAssembly = 0x0001,
        catModule = 0x0002,
        catClass = 0x0004,
        catStruct = 0x0008,
        catEnum = 0x0010,
        catConstructor = 0x0020,
        catMethod = 0x0040,
        catProperty = 0x0080,
        catField = 0x0100,
        catEvent = 0x0200,
        catInterface = 0x0400,
        catParameter = 0x0800,
        catDelegate = 0x1000,
        catGenericParameter = 0x4000,

        catAll = catAssembly | catModule | catClass | catStruct | catEnum | catConstructor |
                        catMethod | catProperty | catField | catEvent | catInterface | catParameter | catDelegate | catGenericParameter,
        catClassMembers = catClass | catStruct | catEnum | catConstructor | catMethod | catProperty | catField | catEvent | catDelegate | catInterface,
    }
}