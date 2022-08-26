//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Import metadata into the symbol table.
//
//-------------------------------------------------------------------------------------------------

#pragma once

struct SymbolList;
class Compiler;

// Initializes our globals.
HRESULT InitIntrinsics(StringPool *pStringPool);

//-------------------------------------------------------------------------------------------------
//
// Handles importing COM+ metadata into our symbol table.
//
class MetaImport : ZeroInit<MetaImport>
{
    friend class EnCVariableInfoList;

    // Constructor
    MetaImport(Compiler *pCompiler, MetaDataFile *pMetaDataFile, SymbolList *pSymbolList);

public:

    // Import all of the project-level symbols out of a metadata database.
    static void ImportTypes(Compiler *pCompiler,
                            MetaDataFile *pMetaDataFile,
                            SymbolList *pSymbolList,
                            ErrorTable *pErrorTable);

    // Import the base and implemented interfaces
    static void ImportBaseAndImplements(BCSYM_Container *Type, bool LoadNestedTypes);

    // Import a single class into the symbol table.  This is used for demand loading.
    static void ImportTypeChildren(BCSYM_Container *pcontainer);

    static void UpdateImportedChildren( _In_ BCSYM_Container* pContainer );

    static BCSYM *LoadTypeOfTypeForwarder(BCSYM_TypeForwarder *TypeFwd, Compiler *Compiler, unsigned cTypeArity);

    // Load this Module's member names into the STRING table.
    static void ImportMemberNamesInModule(BCSYM_Class *pModule);

    static BCSYM *DecodeTypeInDeclaration(BCSYM_NamedRoot* pNamed, const COR_SIGNATURE* pSignature);

private:

    struct MetaType
    {
        BCSYM_Container *m_pContainer;
        mdTypeDef m_tdNestingContainer;
    };

    struct MetaTypeFwd
    {
        BCSYM_TypeForwarder *m_pTypeFwd;
    };

    struct MetaMember
    {
        mdToken m_tk;
        STRING *m_pstrName;
        DWORD m_dwFlags;
        const COR_SIGNATURE *m_pSignature;
        unsigned long m_cbSignature;
        unsigned long m_ulSlot; //. Vtbl offset
        DWORD m_dwImplFlags;
        DWORD m_dwTypeFlags;
        const void *m_pvValue;  // Value
        ULONG m_cbValue; // Length of Value
    };

    struct SavedMetaMember
    {
        mdToken m_tk;
        BCSYM_NamedRoot *m_pnamed;
    };

    //========================================================================
    // Import project-level information.
    //========================================================================

    // Does the preliminary type loading.
    void DoImportTypes();

    void DoUpdateImportedTypeChildren( _In_ BCSYM_Container* pContainer );

    void CheckWhetherToImportInaccessibleMembers();

#if DEBUG
    void VerifyGenericIListBases(BCSYM_Interface *pInterface);
#endif

    // Loads the metatype array.
    void LoadTypes(MetaType **prgtypes,  unsigned *pcTypes, _Out_ bool *pfMissingTypes);

    // Loads all the type forwarders into the metatypeforwarder array
    void MetaImport::LoadTypeForwarders(MetaTypeFwd **prgtypeFwds, unsigned *pcTypeFwds);

    // Set up type parameters for generic types and methods.
    void SetupGenericParameters(BCSYM_NamedRoot *Symbol, mdToken SymbolToken);

    // Set up constraints for type parameters of generic types and methods
    void SetupGenericParameterTypeConstraints(BCSYM_GenericParam *ListOfGenericParameters,
                                              BCSYM_NamedRoot **ppBadTypeSymbol);

    // Set up the new, class and structure constraints for the given generic type parameter
    void SetupGenericParameterNonTypeConstraints(_Inout_ BCSYM_GenericParam *Parameter, DWORD GenericParamFlags);

    // Create a binding for a generic type.
    BCSYM *BuildGenericBinding(BCSYM *Generic,
                               unsigned long ArgumentCount,
                               _In_count_(ArgumentCount) BCSYM **Arguments,
                               bool AllocAndCopyArgumentsToNewList);

    // Helper to create a binding for a generic type.
    BCSYM *BuildGenericBindingHelper(BCSYM *Generic,
                                     unsigned long ArgumentCount,
                                     _In_count_(ArgumentCount) BCSYM **Arguments,
                                     _Inout_ unsigned long &StartIndex,
                                     bool AllocAndCopyArgumentsToNewList);

    BCSYM *BindTypeInVBProject(CompilerProject *pVBProject,
                               _In_z_ STRING *pstrNamespace,
                               _In_z_ STRING *pstrTypeName,
                               unsigned cTypeArity);

    BCSYM_Namespace *FindNameSpaceInProject(CompilerProject *pProject,
                                            _In_count_(TotalNumberOfQualifiedNameComponents) STRING **rgpstrQualifiedName,
                                            ULONG TotalNumberOfQualifiedNameComponents);

    BCSYM_NamedRoot *LookupInNamespace(_In_z_ STRING *pstrName,
                                       BCSYM_Namespace *pNamespace,
                                       bool fMatchNamespace,
                                       bool fMatchType,
                                       unsigned cTypeArity);

    // Get the names for a typdef symbols.
    bool GetTypeDefProps(MetaType *rgtypes, unsigned cTypes, bool fMissingTypes, mdTypeDef tdef, bool *pIsClass, _Deref_opt_out_z_ STRING **ppstrName, _Deref_opt_out_z_ STRING **ppstrNameSpace, DECLFLAGS *pDeclFlags, mdToken *ptkExtends, mdTypeDef *ptdNestingContainer);

    // Get the properties for a type forwarder.
    bool GetTypeForwarderProps(CComPtr<IMetaDataAssemblyImport> srpAssemblyImport,
                               mdExportedType tkExp,
                               _Deref_opt_out_z_ STRING **ppstrName,
                               _Deref_opt_out_z_ STRING **ppstrNameSpace,
                               mdAssemblyRef *ptkAssemblyRef);

    // Resolve the base type and implements
    void DoImportBaseAndImplements(_In_ BCSYM_Container *Type);

    // Get the implements list.
    void GetImplementsList(mdTypeDef tdef,
                           _Out_opt_ BCSYM_Implements **p----l,
                           _Out_ BCSYM_NamedRoot **ppBadNamedRoot,
                           bool IgnoreInaccessibleTypes);

    //========================================================================
    // Import type-level information.
    //========================================================================

    // Load the meta data for a type.
    void DoImportTypeChildren(BCSYM_Container *pContainer);

    // Get the children for this type.
    void LoadChildren(_Out_ Vtypes *pvtUnderlyingType);

    // Load the properties, hooking them up with their implementing methods.
    void LoadProperties(SymbolList *psymlist, _In_opt_z_ WCHAR *pwszDefaultProperty);

    // Load the events.
    void LoadEvents(SymbolList *psymlist);

    // Add fake WinRT interface members
    void AddFakeWindowsRuntimeInterfaceMembers(_Inout_ SymbolList *pSymList, _Inout_ SymbolList *pUnbindableSymList);

    // Create an operator symbol associated with a particular method.
    BCSYM_UserDefinedOperator *CreateOperatorSymbol(UserDefinedOperators Operator, BCSYM_MethodDecl *Method);
#if HOSTED
public:
#endif
    static BCSYM_UserDefinedOperator* CreateOperatorSymbol(BCSYM_Container* Container, UserDefinedOperators Operator,
        BCSYM_MethodDecl* Method, Symbols* Symbols, Compiler* Compiler);
#if HOSTED
private:
#endif

    // Finish loading the children.
    void FinishChildren(_Inout_ SymbolList *psymlist, _Inout_ SymbolList *pUnbindableSymList);

    // Load this Module's member names into the STRING table.
    void DoImportMemberNamesInModule(BCSYM_Class *pModule);

    // Get the information for a member.
    bool GetMemberProps(mdToken tk, MetaMember *pmetamember);

    // Create a method symbol from MetaMember information.
    BCSYM_NamedRoot *LoadMethodDef(_In_ MetaMember *pmetamember, bool hasLocation);

    // Create a variable symbol from MetaMember information
    BCSYM_NamedRoot *LoadFieldDef(_In_ MetaMember *pmetamember, _Out_ Vtypes *pvtUnderlyingType, bool hasLocation);

    // Find the member represented by a token.
    BCSYM_NamedRoot *GetMemberOfToken(mdToken tk);

    // Get the symbol for a typeref token.
    BCSYM *GetTypeOfTypeRef(mdTypeRef td, unsigned cTypeArity);

    // Get the symbol for a type given the name
    BCSYM *GetTypeByName(_In_z_ STRING *pstrTypeName,
                         _In_z_ STRING *pstrNameSpace,
                         unsigned cTypeArity,
                         mdToken tkResolutionScope,
                         _Out_ bool* pfTypeForward = NULL);

    BCSYM *GetTypeByNameInProject(_In_z_ STRING *pstrTypeName,
                                  _In_z_ STRING *pstrNameSpace,
                                  unsigned cTypeArity,
                                  CompilerProject *pProject,
                                  _Out_ bool* pfTypeForward = NULL);

    BCSYM* GetWindowsRuntimeTypeByName(_In_z_ STRING *pstrTypeName,
                                       _In_z_ STRING *pstrNameSpace,
                                       unsigned cTypeArity);

    // Get the symbol for a token.
    BCSYM *GetTypeOfToken(mdToken tk,
                          unsigned cTypeArity,
                          bool ImportBaseAndImplements = false);

    // Get the destination type of a type forwarder.
    BCSYM *GetTypeOfTypeForwarder(BCSYM_TypeForwarder *pTypeFwd, unsigned cTypeArity);

    BCSYM *DoLoadTypeOfTypeForwarder(BCSYM_TypeForwarder *pTypeFwd, unsigned cTypeArity);

    //************************************************************************
    // Signature decoding routines.
    //************************************************************************

    // Decodes a method's signature.
    BCSYM *DecodeMethodSignature(mdMethodDef md,
                                 const COR_SIGNATURE *pbSig,
                                 unsigned long cbSig,
                                 bool ForProperty,
                                 _Inout_ bool &MethodIsBad,
                                 _Out_ BCSYM_Param **ppparamList,
                                 _Out_ PCCOR_SIGNATURE *ppSig,
                                 _Out_ ULONG *pcbSig,
                                 _Inout_ BCSYM_NamedRoot **ppBadTypeSymbol);

    // Decode a field's signature.
    BCSYM *DecodeFieldSignature(const COR_SIGNATURE *pbSig,
                                unsigned long cbSig,
                                _Inout_ bool &FieldIsBad);

    // Pull an integer out of a signature.
    unsigned long DecodeInteger(const COR_SIGNATURE **ppbSig);

    // Pull an token out of a signature.
    mdToken DecodeToken(const COR_SIGNATURE **ppbSig);

    //Consumes the given number of types out of the signatute. This is used to recover from errors
    //encountered in DecodeToken.
    void ConsumeTypes(const COR_SIGNATURE ** ppSig, unsigned long ArgumentCount, _Inout_ bool & TypeIsBad);

    // Find the token that represents this type.
    BCSYM *DecodeType(const COR_SIGNATURE **ppbSig, _Inout_ bool &TypeIsBad);

    // Create a value from COM+ data.
    BCSYM_Expression *DecodeValue(DWORD dwTypeFlag, _In_ const void *pvValue, ULONG cbValue, BCSYM *ptyp);

#if HOSTED
public:
#endif
    static void TransferBadSymbol(_Inout_ BCSYM_NamedRoot *Destination, _In_ BCSYM_NamedRoot *Source);
#if HOSTED
private:
#endif

    static bool HasDiamondReferenceError(BCSYM *pType);

#if HOSTED
public:
#endif
    static DECLFLAGS MapTypeFlagsToDeclFlags(unsigned long TypeFlags, bool fCodeModule);
    static DECLFLAGS MapMethodFlagsToDeclFlags(unsigned long MethodFlags, bool fCodeModule);
    static DECLFLAGS MapFieldFlagsToDeclFlags(unsigned long FieldFlags, bool fCodeModule);
#if HOSTED
private:
#endif

    MetaType *FindType(mdTypeDef td, _In_ MetaType *rgtypes, unsigned cTypes, bool fMissingTypes);

    MetaDataFile *GetMetaDataFile()
    {
        return m_pMetaDataFile;
    }

    void CoerceEnumeratorValues(Vtypes vtUnderlyingType, _In_z_ STRING *strEnumName);

    // Reload/Update Members
    void LoadNewChildren();
    void LoadNewProperties( _Inout_ SymbolList *psymlist, _In_opt_z_ WCHAR *pwszDefaultProperty);
    void LoadNewEvents( _Inout_ SymbolList *psymlist);
    void FinishNewChildren( _Inout_ SymbolList* pSymList);

    void CopyMethodSymbol(_In_ BCSYM_Proc *pOrigMethod, _In_ BCSYM *pContainer, _Inout_ BCSYM_Proc *pNewMethod, _Inout_ SymbolList *pSymList);
    void CopyPropertySymbol(_In_ BCSYM_Property *pOrigProp, _In_ BCSYM *pContainer, _Inout_ BCSYM_Property *pNewProp, _Inout_ SymbolList *pSymList, _Inout_ SymbolList *pUnbindableSymList);
    void CopyEventSymbol(_In_ BCSYM_EventDecl *pOrigEvent, _In_ BCSYM *pContainer, _Inout_ BCSYM_EventDecl *pNewEvent, _Inout_ SymbolList *pSymList, _Inout_ SymbolList *pUnbindableSymList);
    void AddInterfaceMembers(_In_ BCSYM *pInterface, STRINGTree &clashingMembers, _Inout_ SymbolList *pSymList, _Inout_ SymbolList *pUnbindableSymList);
    void GetInterfacesToImplement(_In_ BCSYM *pContainer, STRINGTree &interfaceNames, DynamicArray<BCSYM *> &interfacesToImplement);
    STRING* GetSignature(_In_ BCSYM_Proc *pProc, _In_ BCSYM_GenericBinding *pGenericBinding);
    void GetClashingMembers( DynamicArray<BCSYM *> &interfacesToImplement, STRINGTree &clashingMembers, _In_ SymbolList *pSymList);
    bool ShouldImplementInterface(_In_ BCSYM *pInterface);

    //
    // Data members.
    //

    // Context.
    Compiler *m_pCompiler;

    // More context.
    CompilerHost *m_pCompilerHost;

    // The file we're importing.
    MetaDataFile *m_pMetaDataFile;

    // Where to put the symbols.
    SymbolList *m_pSymbolList;

    // Where to put the errors.
    ErrorTable *m_pErrorTable;

    // Cached IMetaDataImport2, not AddRefed.
    IMetaDataImport *m_pmdImport;
    IMetaDataImport2 *m_pmdImport2;
    IMetaDataWinMDImport *m_pwinmdImport;

    // Local allocator.
    NorlsAllocator m_nraScratch;

    // Symbol allocators for all permanent symbols.
    Symbols m_Symbols;

    //
    // Information only used when loading type-level symbols.
    //

    SavedMetaMember *m_rgmembers;

    unsigned m_cMembers;
    unsigned m_cchNameMax;

    BCSYM_Container *m_pcontainer;
    mdTypeDef m_td;

    // A link to the first generic param of the current method being loaded.
    // NULL for non-generic methods. If this is not set correctly, then any
    // references to the method's generic params in its signature or constraints
    // will not get decoded correctly.
    //
    BCSYM_GenericParam *m_CurrentMethodGenericParams;
};

#if HOSTED
UserDefinedOperators MapToUserDefinedOperator(_In_z_ STRING *MethodName, int ParameterCount, Compiler *Compiler);
#endif
