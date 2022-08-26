//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manage emitting metadata.
//
//-------------------------------------------------------------------------------------------------

#pragma once
#include "MetaEmitHelper.h"

class PEBuilder;
struct MetaMemberRef;
struct ArrayKey;
struct GenericMethodInstantiation;
struct GenericTypeInstantiation;

enum CorSigType
{
    stNormal  = 0x01,
    stNDirect = 0x02,
    stRT      = 0x04
};

struct Signature
{
    NorlsMark m_mark;
    BYTE *m_pbSignature;
    unsigned long m_cbSignature;
};

// The different methods we need to call on an array in order to use
// that array.
//
enum ARRAYINFO
{
    ARRAY_Ctor,
    ARRAY_LoadRef,
    ARRAY_LoadAddrRef,
    ARRAY_StoreRef,
    ARRAY_MAX
};

// The stuff we need to keep track of when using an array.
struct ArrayValue
{
    mdMemberRef m_refs[ARRAY_MAX];
    mdTypeSpec m_tkTypeSpec;
};

// NOTE:Microsoft,2/2002,this class is used by pebuilder and encbuilder.PEBuilder is
// to build a VB program with its own IMetaDataEmit/Import and IMetaDataAssemblyEmit/Import.
// ENCBuilder is to build delta meta data, IL block, and PDB with the IMetaDataEmit from
// the debuggee process.
class MetaEmit : ZeroInit<MetaEmit>
{
public:


    MetaEmit(_In_ Builder *pBuilder, _In_ CompilerProject *pCompilerProject, CompilationCaches *pCompilationCaches);

    MetaEmit(_In_ Builder *pBuilder, _In_ CompilerProject *pCompilerProject, IALink2 *pALink, IALink3 *pALink3, mdToken mdAssemblyOrModule, mdFile mdFileForModule);

    ~MetaEmit();

    //========================================================================
    // Helpers for the PE builder.
    //========================================================================

    //========================================================================
    // Type references
    //========================================================================

    // Define a reference to a symbol type
    mdTypeRef DefineTypeRefBySymbol(BCSYM *ptyp, Location *pReferencingLocation);

    // Define a reference to a name.
    mdTypeRef DefineTypeRefByContainer(BCSYM_Container *pType, Location *pReferencingLocation);

    // Define a reference to a project.
    mdAssemblyRef DefineProjectRef(CompilerProject *pProject);

    // Looks up a reference to a project.  The reference must have already
    // been created via DefineProjectRef.  The pstrTypeName
    // arg is used only when reporting an error.
    mdAssemblyRef LookupProjectRef(CompilerProject *pProject, BCSYM_NamedRoot *pType, Location *pReferencingLocation);

    // Define a reference to a module.
    mdModuleRef DefineModuleRef(_In_z_ STRING *pstrModuleName);

    //========================================================================
    // Member declaration
    //========================================================================

    // Define a set of implements.  The lifetime of the returned array
    // is the same as the lifetime of the MetaEmit object.
    //
    mdTypeRef *DefineImplements(BCITER_ImplInterfaces *----lList, mdToken ComClassInterface, bool fEmbedAllMembersOfImplementedEmbeddableInterfaces = false);

    // Define a set of generic parameters.
    void DefineGenericParamsForType(BCSYM_Container *PossiblyGeneric, mdToken GenericToken);
    void DefineGenericParamsNoEmit(BCSYM_NamedRoot *PossiblyGeneric, bool IncludeParentParams, _Out_ unsigned &ParamCount);
    void DefineGenericParamTokens(BCSYM_NamedRoot *PossiblyGeneric, bool IncludeParentParams, _Inout_ unsigned &ParamIndex, bool IsParent = false);
    void EmitGenericParams(BCSYM_NamedRoot *PossiblyGeneric, mdToken GenericToken, bool IncludeParentParams, unsigned ParamIndex);
    void EmitGenericParam(mdToken GenericToken, BCSYM_GenericParam *GenericParam);

    void EmitGenericParamsHelper
    (
        BCSYM_NamedRoot *PossiblyGeneric,
        mdToken GenericToken,
        bool IncludeParentParams
#if DEBUG
        , unsigned &ParamIndex
#endif DEBUG
    );

    void EncodeGenericArguments(BCSYM_GenericBinding *Binding, unsigned stType, bool EncodeParentArguments, Location *pReferencingLocation);

    mdTypeSpec DefineGenericTypeInstantiation(BCSYM_GenericTypeBinding *pBinding, Location *pReferencingLocation);
    mdMethodSpec DefineGenericMethodInstantiation(BCSYM_NamedRoot *pMethod, BCSYM_GenericBinding *pBinding, mdMemberRef tkUnboundMemberRef, Location *pReferencingLocation);

    // Define a method in the metadata.
    void DefineMethod(BCSYM_Proc *pnamed);

    // Define a field.
    void DefineField(BCSYM_Variable *pnamed);

    // Define an event.  This must happen after all methods and fields
    // are defined so we have all the tokens for the event-hookup
    // methods.
    //
    void DefineEvent(BCSYM_EventDecl *pnamed);

    // Define a property.
    void DefineProperty(BCSYM_Property *pnamed);

    // Hook up a method that implements part of an interface with the
    // interface member it implements.
    //
    void DefineInterfaceImplementation(BCSYM_Proc *pProc);

    // Define a vtable gap method - a fake method used when embedding interop types.
    void DefineVtableGap(BCSYM_Container *pContainer, unsigned int vtableGapIndex, unsigned int vtableGapSize);

    //
    // Support for ComClassAttribute
    //

    // Define a method on a com class interface
    void DefineComClassInterfaceMethod(BCSYM_NamedRoot *pnamed, mdTypeDef typedefInterface);

    // Define the properties on a com class interface
    void DefineComClassInterfaceProperty(BCSYM_NamedRoot *pnamed, mdTypeDef typedefInterface);

    // Define pack size, class size and the field offsets for a class
    void DefineClassLayout(BCSYM_Container *pContainer, mdTypeDef tkContainer, bool fExplicitLayout);

    //========================================================================
    // Method references
    //========================================================================

    // Define a reference to a method from a symbol (which has a method def)
    mdMemberRef DefineMemberRefBySymbol(BCSYM_NamedRoot * pnamed, BCSYM_GenericBinding *pBinding, Location *pReferencingLocation, ErrorTable *pAlternateErrorTable = NULL);

     // Define a reference to a VBA runtime method
    mdMemberRef DefineRTMemberRef(RuntimeMembers rtHelper, 
                                  ErrorTable *pErrorTable = NULL,
                                  Location *pErrorLocation = NULL);

    // Define a reference to a method with a name.
    mdMemberRef DefineMemberRefByName(_In_ MetaMemberRef *pmmr, size_t cbSizeRef, BCSYM_NamedRoot *psymNamed, mdTypeRef mdTypeRefParent, bool canUseDefineImportMember = true);

    //========================================================================
    // Array method references
    //========================================================================

    mdTypeSpec DefineTypeSpec(BCSYM_ArrayType *parray, Location *pReferencingLocation);
    mdMemberRef DefineArrayCtorRef(BCSYM_ArrayType * parr, Location *pReferencingLocation);
    mdMemberRef DefineArrayLoadRef(BCSYM_ArrayType * parr, unsigned cDims, Location *pReferencingLocation);
    mdMemberRef DefineArrayLoadAddrRef(BCSYM_ArrayType * parr, unsigned cDims, Location *pReferencingLocation);
    mdMemberRef DefineArrayStoreRef(BCSYM_ArrayType * parr, unsigned cDims, Location *pReferencingLocation);

    //========================================================================
    // Generic data.
    //========================================================================

    mdString AddDataString(_In_count_(cchLen) const WCHAR *wsz, size_t cchLen);

    //========================================================================
    // Custom attributes
    //========================================================================

    // The Debuggable attribute goes on an assembly, and so must be
    // emitteed by ALink.
    void ALinkEmitDebuggableAttribute();

    void ALinkEmitCompilationRelaxationsAttribute();
    void ALinkEmitRuntimeCompatibilityAttribute();

    void ALinkEmitAssemblyFlagsAttribute(int AssmeblyFlags, RuntimeMembers AssemblyFlagsAttributeCtor);

    mdCustomAttribute DefineCustomAttributeWithString(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    _In_z_ WCHAR  *pstrStringParam);

    mdCustomAttribute DefineCustomAttributeWithString_String(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    _In_z_ WCHAR *pstrString1Param,
                    _In_z_ WCHAR *pstrString2Param);

    mdCustomAttribute DefineCustomAttributeWithInt16(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    __int16 integerParam);

    mdCustomAttribute DefineCustomAttributeWithInt32(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    int integerParam);

    mdCustomAttribute DefineCustomAttributeWithInt64(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    __int64 integerParam);

    mdCustomAttribute DefineCustomAttributeWithBool(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    bool boolParam);

    mdCustomAttribute DefineDecimalCustomAttribute(
                    mdToken tkObj,
                    DECIMAL DecimalValue);

   mdCustomAttribute DefineCustomAttributeObject(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef,
                    ULONG cbArgBlob,
                    BYTE* argBlob);

    mdCustomAttribute DefineCustomAttribute(
                    mdToken tkObj,
                    mdMemberRef AttributeCtorMemberRef);

    mdCustomAttribute DefineBestFitMappingAttribute(
                    mdToken tkObj,
                    bool param,
                    bool fieldval);

    HRESULT  DefineCustomAttribute(
                    mdToken     tkObj,
                    mdToken     tkType,
                    void const  *pCustomAttribute,
                    ULONG       cbCustomAttribute,
                    mdCustomAttribute *pcv);


    // # chars in string representation of GUID (including terminating NUL).
    static const int c_cchGuid = 37;

    //========================================================================
    // Image building
    //========================================================================

    //
    // Building the IL image for a method must proceed in the following
    // order:
    //
    //  - add the exception blocks to MetaEmit
    //  - get the total size for the method, allocate the memory to
    //    store the image in
    //  - emit the headers and exception information
    //  - emit the IL into the block of memory.
    //  - save out the debug information
    //  - release the image resources

    // Add an exception handling clause to the method.
    void AddEHClause(CorExceptionFlag flags,
                     unsigned         ulTryOffset,
                     unsigned         ulTryLength,
                     unsigned         ulHandlerOffset,
                     unsigned         ulHandlerLength,
                     unsigned         ulFilterOffset,
                     mdTypeRef        typrefType);

    // Gets the size of the block of memory needed to hold the image.
    unsigned ImageSize(unsigned cStackDepth,
                       bool hasLocals,
                       unsigned cbCodeSize);

    // Emit the header and trailer information for this method.  Returns
    // the location to put the IL into.
    //
    BYTE *EmitImage(_Out_ BYTE *pbImage,
                    unsigned cbCodeSize,
                    unsigned cStackDepth,
                    COR_SIGNATURE *pSigLocals,
                    unsigned cbSigLocals);

    // Frees the image's resources.
    void FreeImageResources();

    //========================================================================
    // Signature encoding helpers.
    //========================================================================

    void StartNewSignature(_Out_ Signature *psig);

    mdToken GetComClassToken(BCSYM_NamedRoot * pNamed) { return m_pBuilder->GetComClassToken(pNamed); }
    mdToken GetTypeDef(BCSYM_Container * pContainer) { return m_pBuilder->GetTypeDef(pContainer); }
    mdToken GetToken(BCSYM_NamedRoot * pNamed) { return m_pBuilder->GetToken(pNamed); }
    mdToken GetToken(BCSYM_Param *pParam, BCSYM_Proc *pContext) { return m_pBuilder->GetToken(pParam, pContext); }
    mdToken GetComClassEventsToken(BCSYM_Class * pClass) { return m_pBuilder->GetComClassEventsToken(pClass); }
    void SetToken(BCSYM_NamedRoot *pNamed, mdToken tk) {m_pBuilder->SetToken(pNamed, tk);}
    void SetToken(BCSYM_Param *pParam, BCSYM_Proc *pContext, mdToken tk) {m_pBuilder->SetToken(pParam, pContext, tk);}

    MetaMemberRef *GetMetaMemberRef()
    {
        return (MetaMemberRef *)m_pbSignature;
    }

    ArrayKey *GetArrayKey()
    {
        return (ArrayKey *)m_pbSignature;
    }

    GenericMethodInstantiation *GetMethodInstantiation()
    {
        return (GenericMethodInstantiation *)m_pbSignature;
    }

    GenericTypeInstantiation *GetTypeInstantiation()
    {
        return (GenericTypeInstantiation *)m_pbSignature;
    }

    COR_SIGNATURE *GetSignature()
    {
        return (COR_SIGNATURE *)m_pbSignature;
    }

    unsigned long GetSigSize()
    {
        return m_cbSignature;
    }

    mdSignature ComputeSignatureToken(
        COR_SIGNATURE *pSigLocals,
        unsigned cbSigLocals
        );

    mdSignature GetSignatureToken()
    {
        return m_SignatureToken;
    }

    void ResetSignatureToken()
    {
        m_SignatureToken = mdTokenNil;
    }

    void EncodeMetaMemberRef(_In_z_ STRING *pstrMemberName, mdTypeRef tr);
    void EncodeMetaMemberRef(BCSYM_NamedRoot* psymMember, mdTypeRef tr);
    void EncodeArrayKey();
    void EncodeMethodInstantiation(_In_z_ STRING *Name, mdMemberRef UnboundMethod);
    void EncodeTypeInstantiation(_In_z_ STRING *Name, mdTypeRef UnboundType);

    void EncodeMethodSignature(BCSYM_Proc *pproc, Location *pReferencingLocation);
    void EncodeFieldSignature(BCSYM_Member * psymMem, Location *pReferencingLocation);
    void EncodePropertySignature(BCSYM_Proc * pproc);
    void EncodeInteger(unsigned long ul);
    void EncodeSignedInteger(long l);
    void EncodeToken(mdToken tk);
    void EncodeType(BCSYM *psymType,
                    Location *pReferencingLocation,
                    Vtypes vtype = t_UNDEF,
                    unsigned stType = stNormal);
    BYTE *EncodeExtendBuffer(size_t cch);

    void EncodeFinishMetaMemberRef();
    void EncodeFinishArrayKey();
    void EncodeFinishMethodInstantiation();
    void EncodeFinishTypeInstantiation();

    void ReleaseSignature(_In_ Signature *psig);

    void EmitAllSecurityAttributes();

    //========================================================================
    // ALink wrappers to generate manifest, link/embed resources, etc.
    //========================================================================
    void    ALinkImportReferences();

    void    AlinkEmitAssemblyPlatformKind();

    void    ALinkEmitResources(ICeeFileGen *pFileGen,
                               HCEEFILE hCeeFile,
                               HCEESECTION hCeeSection
#if IDE 
                              ,bool fEmbeddedOnly = false
#endif IDE
                               );

    ULONG   ALinkEmitOneResource(ICeeFileGen *pFileGen, HCEESECTION hCeeSection, _In_ Resource *presource,
                                DWORD ibThisResource, ErrorTable *pErrorTable);

    void    ALinkEmitAssemblyAttributes();

    void    ALinkSetAssemblyProps();

    void    ALinkCreateWin32ResFile(bool fDLL, _In_z_ STRING *pstrIconFileName, _In_ size_t lengthOfTempFileBuffer, _Deref_out_z_ WCHAR *ppstrTempFile);

    void    ALinkEmitManifest(ICeeFileGen *pFileGen, HCEEFILE hCeeFile, HCEESECTION hCeeSection);

    void    ALinkEmitAssembly();

    void    ALinkPrecloseAssembly();

    void    ALinkSignAssembly();

    void    ALinkEmbedUacManifest();

    mdToken ALinkEmitProjectRef(CompilerProject *pCompilerProject);

    // Are we emitting an assembly or a module?  Just defer to our PEBuilder.
    bool FEmitAssembly() { return m_pBuilder->FEmitAssembly(); }

    Builder * GetBuilder() { return m_pBuilder; }

    CompilationCaches * GetCompilationCaches(){ return m_pCompilationCaches; }

    // Clears the list of exceptions so we can start a new method
    void ClearExceptions();

    // Checks if the type is available and reports an error if not.
    bool CanEmitAttribute(RuntimeMembers attributeCtor, BCSYM *pSym);

#if DEBUG
    // Checks if private members of of MetaEmit are cleared
    bool IsPrivateMembersEmpty();

    // Verifies that the private alink and tokens are correct
    bool VerifyALinkAndTokens(IALink2 *pALink, mdToken mdAssemblyOrModule, mdFile mdFileForModule);
#endif DEBUG

    bool IsHighEntropyVA(){ return m_bIsHighEntropyVA; }

    static void ResolveTypeForwarders(_In_ void* pCompilerObj, _In_ void *pImportScope, mdTypeRef token, _Out_ IMetaDataAssemblyImport **ppNewScope, _Out_ const void ** ppbHashValue, _Out_ ULONG *pcbHashValue);
private:
    
    static BCSYM * GetTokenFromHash(_In_ MetaDataFile *pMetaDataFile, mdToken token, _Out_ bool* pfTypeForward = NULL);
    
    //========================================================================
    // Internal implementation.
    //========================================================================

    // Helper for constructor
    void Init(_In_ Builder *pBuilder, _In_ CompilerProject *pCompilerProject, IALink2 *pALink, IALink3 *pALink3, mdToken mdAssemblyOrModule, mdFile mdFileForModule);

    void ConvertConstant(_In_ ConstantValue *Value, _Out_ DWORD *pdwFlag, _Out_ void **ppData, _Out_ ULONG *pcbData);

    // Create ALink object in m_pALink to help emit if not already created
    void GetALinkHelper();

    //========================================================================
    // Datamembers
    //========================================================================

    // Context
    Compiler * m_pCompiler;

    // Master holder of all cross-instance state.
    Builder * m_pBuilder;

    // CompilerProject we are bulid for.
    CompilerProject *m_pCompilerProject;

    CompilationCaches *m_pCompilationCaches;

    // Cached information from the builder.  Not addrefed.
    IMetaDataEmit2 * m_pmdEmit;

    // ALink to help with emitting
    IALink2 *m_pALink;

    // ALink3 to help with UAC manifests
    IALink3 *m_pALink3;

    // If MetaEmit creates ALink, MetaEmit needs to clean it up
    bool m_fReleaseALink;

    // Token of assembly/module we are writting to
    mdToken m_mdAssemblyOrModule;
    mdFile m_mdFileForModule;

    // Current scratch allocator.
    NorlsAllocator    m_nraScratch;
    NorlsAllocator    m_nraSignature;

    // Current signature.
    BYTE *m_pbSignature;
    unsigned m_cbSignature;
    mdSignature m_SignatureToken;

    // Dev10 #676210 These flags are used to keep track of the fact that we referred to an embeddable interop type.
    bool m_TrackReferrencesToEmbeddableInteropTypes;
    bool m_ReferredToEmbeddableInteropType;

    MetaEmitHelper *m_pMetaEmitHelper;
    //
    // Used when emitting exceptions into a method.
    //

    DynamicArray<IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT> m_daExceptions;

    bool m_fNeedBigExceptions;
    bool m_fTinyHeader;
    unsigned m_oExceptionTable;
    bool m_bIsHighEntropyVA;
};


// Helper class to start a new signature for the duration of a scope
class NewMetaEmitSignature
{
public:

    NewMetaEmitSignature(_Inout_ MetaEmit *pMetaemit)
        : m_pMetaemit(pMetaemit)
    {
        pMetaemit->StartNewSignature(&m_Signature);
    }

    ~NewMetaEmitSignature()
    {
        m_pMetaemit->ReleaseSignature(&m_Signature);
    }

private:

    MetaEmit  *m_pMetaemit;

    Signature m_Signature;
};

#if DEBUG
// DEBUG-only helper to validate that the given token is a valid
// assembly or module token as defined by ALink.  Note that
// ALink returns a 1-based assembly token (as does COM+),
// but returns a 0-based module token (don't ask).
inline void DebCheckAssemblyOrModuleToken(mdToken mdAssemblyOrModule)
{
    VSASSERT(mdAssemblyOrModule == TokenFromRid(1, mdtAssembly) ||
             mdAssemblyOrModule == AssemblyIsUBM,
             "Bogus assembly/module token.");
}
#else
#define DebCheckAssemblyOrModuleToken(mdAssemblyOrModule)
#endif
