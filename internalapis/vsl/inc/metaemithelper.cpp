#include "MetaEmitHelper.h"
#include <metahost.h>

// ==============================================================================================================
// Helper macros and types for the implementation

// This used to be part of SDK per the file's comments
#include "corhlprpriv.h"

typedef LPCSTR LPCUTF8;
#define InvalidRid(rid) ((rid) == 0)

void DestroyNestedQuickArray(CQuickArray<CQuickArray<WCHAR>> &qarr)
{
    ULONG count = (ULONG)qarr.Size();

    for (ULONG i = 0; i < count; i--)
    {
        qarr[i].Destroy();
    }
}

HRESULT MergeUpdateTokenInSig(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,     // [IN] The assembly emit scope.
    IMetaDataImport *         pMiniMdAssemEmit_Import, 
    IMetaDataEmit *           pMiniMdEmit,          // [IN] The emit scope.
    IMetaDataImport *         pMiniMdEmit_Import, 
    IMetaDataAssemblyEmit *   pMiniMdEmit_AssemblyEmit, 
    IMetaDataAssemblyImport * pCommonAssemImport,   // [IN] Assembly scope where the signature is from.
    IMetaDataImport *         pCommonAssemImport_Import, 
    const void  *             pbHashValue,          // [IN] Hash value for the import assembly.
    ULONG                     cbHashValue,          // [IN] Size in bytes for the hash value.
    IMetaDataImport *         pCommonImport,        // [IN] The scope to merge into the emit scope.
    PCCOR_SIGNATURE           pbSigImp,             // signature from the imported scope
    CQuickBytes *             pqkSigEmit,   // [OUT] translated signature
    ULONG                     cbStartEmit,  // [IN] start point of buffer to write to
    ULONG *                   pcbImp,       // [OUT] total number of bytes consumed from pbSigImp
    ULONG *                   pcbEmit,     // [OUT] total number of bytes write to pqkSigEmit
    TypeForwardersResolver *  pTypeForwardersResolver); // [IN] Functor to resolve type forwarders
// ==============================================================================================================
// ==============================================================================================================
// ==============================================================================================================

HRESULT MetaEmitHelper::DefineImportMember(     // S_OK or error.
    IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the Member.
    const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
    ULONG        cbHashValue,           // [IN] Count of bytes.
    IMetaDataImport * pImport,           // [IN] Import scope, with member.
    mdToken     mbMember,               // [IN] Member in import scope.
    IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the Member is imported.
    mdToken     tkImport,               // [IN] Classref or classdef in emit scope.
    TypeForwardersResolver *pTypeForwardersResolver, // [IN] Functor for resolving type forwarders.
    mdMemberRef *pmr)                   // [OUT] Put member ref here.
{
    HRESULT hr = S_OK;

    _ASSERTE(pImport && pmr);
    _ASSERTE(TypeFromToken(tkImport) == mdtTypeRef || TypeFromToken(tkImport) == mdtModuleRef ||
                IsNilToken(tkImport) || TypeFromToken(tkImport) == mdtTypeSpec);
    _ASSERTE((TypeFromToken(mbMember) == mdtMethodDef && mbMember != mdMethodDefNil) ||
             (TypeFromToken(mbMember) == mdtFieldDef && mbMember != mdFieldDefNil));

    CQuickArray<WCHAR> qbMemberName;    // Name of the imported member.
    CQuickArray<WCHAR> qbScopeName;     // Name of the imported member's scope.
    GUID        mvidImport;             // MVID of the import module.
    GUID        mvidEmit;               // MVID of the emit module.
    ULONG       cchName;                // Length of a name, in wide chars.
    PCCOR_SIGNATURE pvSig;              // Member's signature.
    ULONG       cbSig;                  // Length of member's signature.
    CQuickBytes cqbTranslatedSig;       // Buffer for signature translation.
    ULONG       cbTranslatedSig;        // Length of translated signature.

    IMetaDataImport * pAssemImport_Import = NULL;
    IMetaDataImport * pAssemEmit_Import = NULL;

    if (TypeFromToken(mbMember) == mdtMethodDef)
    {
        do {
            hr = pImport->GetMethodProps(
                mbMember, 
                0,      // ptkClass
                qbMemberName.Ptr(), 
                (ULONG)qbMemberName.MaxSize(), 
                &cchName, 
                NULL,   // pdwAttr
                &pvSig, 
                &cbSig, 
                NULL,   // pulCodeRVA
                NULL);  // pdwImplFlags
            if (qbMemberName.MaxSize() < cchName)
            {
                IfFailGo(qbMemberName.ReSizeNoThrow(cchName));
                continue;
            }
            break;
        } while (1);
    }
    else    // TypeFromToken(mbMember) == mdtFieldDef
    {
        do {
            hr = pImport->GetFieldProps(
                mbMember, 
                NULL,   // ptkClass
                qbMemberName.Ptr(), 
                (ULONG)qbMemberName.MaxSize(), 
                &cchName, 
                NULL,   // pdwAttr
                &pvSig, 
                &cbSig, 
                NULL,   // pdwCPlusTypeFlag
                NULL,   // ppConstantValue
                NULL);  // pcbConstantValue
            if (qbMemberName.MaxSize() < cchName)
            {
                IfFailGo(qbMemberName.ReSizeNoThrow(cchName));
                continue;
            }
            break;
        } while (1);
    }
    IfFailGo(hr);

    IfFailGo(cqbTranslatedSig.ReSizeNoThrow(cbSig * 3));       // Set size conservatively.
    
    if (pAssemImport)
    {
        IfFailGo(pAssemImport->QueryInterface(IID_IMetaDataImport, (void **)&pAssemImport_Import));
        _ASSERTE(pAssemImport_Import != NULL);
    }

    if (pAssemEmit)
    {
        IfFailGo(pAssemEmit->QueryInterface(IID_IMetaDataImport, (void **)&pAssemEmit_Import));
        _ASSERTE(pAssemEmit_Import != NULL);
    }

    IfFailGo(TranslateSigWithScope(
        pAssemImport, 
        pAssemImport_Import, 
        pbHashValue,
        cbHashValue,
        pImport, 
        pvSig, 
        cbSig, 
        pAssemEmit, 
        pAssemEmit_Import, 
        m_pEmit, 
        m_pEmit_Import, 
        m_pEmit_AssemblyEmit, 
        pTypeForwardersResolver,
        (COR_SIGNATURE *)cqbTranslatedSig.Ptr(),
        cbSig * 3, 
        &cbTranslatedSig));

    // Define ModuleRef for imported Member functions

    // Check if the Member being imported is a global function.
    IfFailGo(m_pEmit_Import->GetScopeProps(NULL, 0, NULL, &mvidEmit));
    IfFailGo(pImport->GetScopeProps(NULL, 0, &cchName, &mvidImport));
    if (mvidEmit != mvidImport && IsNilToken(tkImport))
    {
        IfFailGo(qbScopeName.ReSizeNoThrow(cchName));
        IfFailGo(pImport->GetScopeProps(qbScopeName.Ptr(), (ULONG)qbScopeName.MaxSize(), NULL, NULL));
        IfFailGo(m_pEmit->DefineModuleRef(qbScopeName.Ptr(), &tkImport));
    }

    // Define MemberRef base on the name, sig, and parent
    IfFailGo(m_pEmit->DefineMemberRef(
        tkImport, 
        qbMemberName.Ptr(),
        reinterpret_cast<PCCOR_SIGNATURE>(cqbTranslatedSig.Ptr()),
        cbTranslatedSig, 
        pmr));

ErrExit:
    if (pAssemImport_Import != NULL)
    {
        pAssemImport_Import->Release();
    }
    if (pAssemEmit_Import != NULL)
    {
        pAssemEmit_Import->Release();
    }
    return hr;
} // MetaEmitHelper::DefineImportMember

//****************************************************************************
// Get Nesting hierarchy given a TypeDef.
//****************************************************************************
HRESULT GetTDNesterHierarchy(
    IMetaDataImport *                 pCommonImport,    // Scope in which to find the hierarchy.
    mdTypeDef                         td,               // TypeDef whose hierarchy is needed.
    CQuickArray<mdTypeDef> &          qaTdNesters,      // Array of Nesters.
    CQuickArray<CQuickArray<WCHAR>> & qaNames)          // Names of the nesters.
{
    DWORD       dwFlags;
    mdTypeDef   tdNester;
    ULONG       ulNesters;
    HRESULT     hr = NOERROR;

    _ASSERTE(pCommonImport &&
             TypeFromToken(td) == mdtTypeDef &&
             !IsNilToken(td));

    // Set current Nester index to 0.
    ulNesters = 0;
    // The first element in the hierarchy is the TypeDef itself.
    tdNester = td;
    // Bogus initialization to kick off the while loop.
    dwFlags = tdNestedPublic;
    // Loop as long as the TypeDef is a Nested TypeDef.
    while (IsTdNested(dwFlags))
    {
        ULONG cchName;
        
        if (InvalidRid(tdNester))
            IfFailGo(CLDB_E_RECORD_NOTFOUND);
        
        // Update the dynamic arrays.
        ulNesters++;
        
        IfFailGo(qaTdNesters.ReSizeNoThrow(ulNesters));
        qaTdNesters[ulNesters-1] = tdNester;
        
        IfFailGo(qaNames.ReSizeNoThrow(ulNesters));
        CQuickArray<WCHAR> & qaName = qaNames[ulNesters-1];
        qaName.Init();

        // Get the name and namespace for the TypeDef.
        IfFailGo(pCommonImport->GetTypeDefProps(
            tdNester,
            NULL, 
            0, 
            &cchName, 
            &dwFlags,   // pdwTypeDefFlags
            NULL));     // ptkExtends
        _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
        IfFailGo(qaName.ReSizeNoThrow(cchName));
        IfFailGo(pCommonImport->GetTypeDefProps(
            tdNester,
            qaName.Ptr(), 
            (ULONG)qaName.MaxSize(), 
            NULL, 
            &dwFlags,   // pdwTypeDefFlags
            NULL));     // ptkExtends
        
        // This call will fail when we are at the outermost scope. In that case,
        // we do not want to return a failed HR.
        if(FAILED(pCommonImport->GetNestedClassProps(tdNester, &tdNester)))
        {
            tdNester = mdTokenNil;
        }
    }
    // Outermost class must have enclosing of Nil.
    _ASSERTE(IsNilToken(tdNester));
ErrExit:
    return hr;
} // GetTDNesterHierarchy


//****************************************************************************
// Get Nesting hierarchy given a TypeRef.
//****************************************************************************
HRESULT GetTRNesterHierarchy(
    IMetaDataImport *                 pCommonImport,    // Scope in which to find the hierarchy.
    mdTypeRef                         tr,               // [IN] TypeRef whose hierarchy is needed.
    CQuickArray<mdTypeRef> &          qaTrNesters,      // [OUT] Array of Nesters.
    CQuickArray<CQuickArray<WCHAR>> & qaNames)          // [OUT] Names of the nesters.
{
    mdTypeRef   trNester;
    mdToken     tkResolutionScope;
    ULONG       ulNesters;
    HRESULT     hr = S_OK;

    _ASSERTE(pCommonImport &&
             TypeFromToken(tr) == mdtTypeRef &&
             !IsNilToken(tr));

    // Set current Nester index to 0.
    ulNesters = 0;
    // The first element in the hierarchy is the TypeRef itself.
    trNester = tr;
    // Loop as long as the TypeRef is a Nested TypeRef.
    while (TypeFromToken(trNester) == mdtTypeRef && !IsNilToken(trNester))
    {
        ULONG cchName;
        
        // Update the dynamic arrays.
        ulNesters++;
        
        IfFailGo(qaTrNesters.ReSizeNoThrow(ulNesters));
        qaTrNesters[ulNesters-1] = trNester;
        
        IfFailGo(qaNames.ReSizeNoThrow(ulNesters));
        CQuickArray<WCHAR> & qaName = qaNames[ulNesters - 1];
        qaName.Init();

        // Get the name and namespace for the TypeDef.
        IfFailGo(pCommonImport->GetTypeRefProps(
            trNester,
            &tkResolutionScope, 
            NULL,   // wszName
            0,      // cchName
            &cchName));
        _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
        IfFailGo(qaName.ReSizeNoThrow(cchName));
        IfFailGo(pCommonImport->GetTypeRefProps(
            trNester,
            NULL,   // ptkResolutionScope
            qaName.Ptr(), 
            (ULONG)qaName.MaxSize(), 
            NULL));  // pchName
        
        trNester = tkResolutionScope;
    }
ErrExit:
    return hr;
} // GetTRNesterHierarchy

//******************************************************************************
// Given import scope, create a corresponding ModuleRef.
//******************************************************************************
HRESULT CreateModuleRefFromScope(
    IMetaDataEmit *   pMiniMdEmit,          // [IN] Emit scope in which the ModuleRef is to be created.
//    IMetaDataImport * pMiniMdEmit_Import, 
    IMetaDataImport * pCommonImport,        // [IN] Import scope.
    mdModuleRef *     ptkModuleRef)         // [OUT] Output token for ModuleRef.
{
    HRESULT            hr = S_OK;
    CQuickArray<WCHAR> qaName;
    ULONG              cchName;
    
    // Set output to nil.
    *ptkModuleRef = mdTokenNil;

    // Get name of import scope.
    IfFailGo(pCommonImport->GetScopeProps(
        NULL,   // szName
        0,      // cchName
        &cchName, 
        NULL)); // pmvid
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    if (cchName == 0)
    {
        // It the referenced Module does not have a proper name, use the nil token instead.
        // clear the error
        hr = NOERROR;
        
        // It is a bug to create an ModuleRef to an empty name!!!
        *ptkModuleRef = mdTokenNil;
        goto ErrExit;
    }
    IfFailGo(qaName.ReSizeNoThrow(cchName));
    IfFailGo(pCommonImport->GetScopeProps(
        qaName.Ptr(), 
        (ULONG)qaName.MaxSize(), 
        NULL,   // pcchName
        NULL)); // pmvid
    
    IfFailGo(pMiniMdEmit->DefineModuleRef(qaName.Ptr(), ptkModuleRef));
    
ErrExit:
    return hr;
} // CreateModuleRefFromScope

//******************************************************************************
// Given import scope, create a corresponding ModuleRef.
//******************************************************************************
HRESULT CreateModuleRefFromModuleRef(
    IMetaDataEmit *   pMiniMdEmit,          // [IN] Emit scope in which the ModuleRef is to be created.
    IMetaDataImport * pCommonImport,        // [IN] Import scope.
    mdModuleRef       tkModuleRef,          // [IN] ModuleRef token.
    mdModuleRef *     ptkModuleRef)         // [OUT] Output token for ModuleRef.
{
    HRESULT            hr = S_OK;
    CQuickArray<WCHAR> qaName;
    ULONG              cchName;
    
    // Set output to nil.
    *ptkModuleRef = mdTokenNil;
    
    // Get name of the ModuleRef being imported.
    IfFailGo(pCommonImport->GetModuleRefProps(
        tkModuleRef, 
        NULL,   // szName
        0,      // cchName
        &cchName));
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailGo(qaName.ReSizeNoThrow(cchName));
    IfFailGo(pCommonImport->GetModuleRefProps(
        tkModuleRef, 
        qaName.Ptr(), 
        (ULONG)qaName.MaxSize(), 
        NULL)); // pcchName
    
    IfFailGo(pMiniMdEmit->DefineModuleRef(qaName.Ptr(), ptkModuleRef));
    
ErrExit:
    return hr;
} // CreateModuleRefFromModuleRef

//******************************************************************************
// Given a ExportedType and the Assembly emit scope, create a corresponding ModuleRef
// in the give emit scope.  The ExportedType being passed in must belong to the
// Assembly passed in.  Function returns S_FALSE if the ExportedType is implemented
// by the emit scope passed in.
//******************************************************************************
HRESULT CreateModuleRefFromExportedType(
    IMetaDataAssemblyEmit *   pAssemEmit,           // [IN] Import assembly scope.
    IMetaDataAssemblyImport * pAssemEmit_AssemblyImport, 
    IMetaDataEmit *           pMiniMdEmit,          // [IN] Emit scope.
    IMetaDataImport *         pMiniMdEmit_Import, 
    mdExportedType            tkExportedType,       // [IN] ExportedType token in Assembly emit scope.
    mdModuleRef *             ptkModuleRef)         // [OUT] ModuleRef token in the emit scope.
{
    mdFile             tkFile;
    CQuickArray<WCHAR> qaFileName;
    CQuickArray<WCHAR> qaScopeName;
    ULONG              cchName;
    HRESULT            hr = S_OK;
    
    // Set output to nil.
    *ptkModuleRef = mdTokenNil;

    // Get the implementation token for the ExportedType.  It must be a File token
    // since the caller should call this function only on ExportedTypes that resolve
    // to the same Assembly.
    IfFailGo(pAssemEmit_AssemblyImport->GetExportedTypeProps(
        tkExportedType, 
        NULL,   // wszName
        0,      // cchName
        NULL,   // pcchName
        &tkFile, 
        NULL,   // ptkTypeDef
        NULL)); // pdwExportedTypeFlags
    if (TypeFromToken(tkFile) != mdtFile)
    {
        IfFailGo(E_UNEXPECTED);
    }

    // Get the name of the file.
    IfFailGo(pAssemEmit_AssemblyImport->GetFileProps(
        tkFile, 
        NULL,   // wszName
        0,      // cchName
        &cchName, 
        NULL,   // ppbHashValue
        NULL,   // pcbHashValue 
        NULL)); // pdwFileFlags
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailGo(qaFileName.ReSizeNoThrow(cchName));
    IfFailGo(pAssemEmit_AssemblyImport->GetFileProps(
        tkFile, 
        qaFileName.Ptr(), 
        (ULONG)qaFileName.MaxSize(), 
        NULL,   // pcchName
        NULL,   // ppbHashValue
        NULL,   // pcbHashValue 
        NULL)); // pdwFileFlags
    
    // Get the name of the emit scope.
    IfFailGo(pMiniMdEmit_Import->GetScopeProps(NULL, 0, &cchName, NULL));
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailGo(qaScopeName.ReSizeNoThrow(cchName));
    IfFailGo(pMiniMdEmit_Import->GetScopeProps(qaScopeName.Ptr(), (ULONG)qaScopeName.MaxSize(), NULL, NULL));

    // If the file corresponds to the emit scope, return S_FALSE;
    if (wcscmp(qaFileName.Ptr(), qaScopeName.Ptr()) == 0)
        return S_FALSE;
    
    IfFailGo(pMiniMdEmit->DefineModuleRef(qaFileName.Ptr(), ptkModuleRef));
    
ErrExit:
    return hr;
} // CreateModuleRefFromExportedType

//****************************************************************************
// Given a TypeDef or a TypeRef, return the Nesting hierarchy.  The first
// element in the returned array always refers to the class token passed and
// the nesting hierarchy expands outwards from there.
//****************************************************************************
HRESULT GetNesterHierarchy(
    IMetaDataImport *                 pCommonImport,    // Scope in which to find the hierarchy.
    mdToken                           tk,               // TypeDef/TypeRef whose hierarchy is needed.
    CQuickArray<mdToken> &            qaNesters,        // Array of Nesters.
    CQuickArray<CQuickArray<WCHAR>> & qaNames)          // Names of the nesters.
{
    _ASSERTE(pCommonImport &&
             (TypeFromToken(tk) == mdtTypeDef ||
              TypeFromToken(tk) == mdtTypeRef) &&
             !IsNilToken(tk));

    if (TypeFromToken(tk) == mdtTypeDef)
    {
        return GetTDNesterHierarchy(pCommonImport,
                                    tk,
                                    qaNesters,
                                    qaNames);
    }
    else
    {
        return GetTRNesterHierarchy(pCommonImport,
                                    tk,
                                    qaNesters,
                                    qaNames);
    }
} // GetNesterHierarchy

//******************************************************************************
// Given an AssemblyRef and the corresponding scope, create an AssemblyRef in
// the given Module scope and Assembly scope.
//******************************************************************************
HRESULT CreateAssemblyRefFromAssemblyRef(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,     // [IN] Assembly emit scope.
    IMetaDataAssemblyImport * pCommonImport,        // [IN] Scope to import the assembly ref from.
    mdAssemblyRef             tkAssemRef,           // [IN] Assembly ref to be imported.
    mdAssemblyRef *           ptkAssemblyRef)       // [OUT] AssemblyRef in the emit scope.
{
    const void *       pbPublicKeyOrToken;
    ULONG              cbPublicKeyOrToken;
    CQuickArray<WCHAR> qaName;
    ULONG              cchName;
    ASSEMBLYMETADATA   sMetaData;
    CQuickArray<WCHAR> qaLocale;
    const void *       pbHashValue;
    ULONG              cbHashValue;
    DWORD              dwAssemblyRefFlags;
    HRESULT            hr = S_OK;
    mdAssemblyRef      tkNewAssemblyRef = mdTokenNil;

    // Set output to Nil.
    if (ptkAssemblyRef != NULL)
    {
        *ptkAssemblyRef = mdTokenNil;
    }

    // Get import AssemblyRef props.
    sMetaData.szLocale = NULL;
    sMetaData.cbLocale = 0;
    sMetaData.rOS = NULL;
    sMetaData.ulOS = 0;
    sMetaData.rProcessor = NULL;
    sMetaData.ulProcessor = 0;
    
    IfFailGo(pCommonImport->GetAssemblyRefProps(
        tkAssemRef, 
        NULL,       // ppbPublicKeyOrToken
        NULL,       // pcbPublicKeyOrToken
        NULL,       // szName
        0,          // cchName
        &cchName, 
        &sMetaData, 
        NULL,       // ppbHashValue
        NULL,       // pcbHashValue
        NULL));     // pdwAssemblyRefFlags
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailGo(qaName.ReSizeNoThrow(cchName));
    IfFailGo(qaLocale.ReSizeNoThrow(sMetaData.cbLocale));
    
    sMetaData.szLocale = qaLocale.Ptr();
    sMetaData.cbLocale = (ULONG)qaLocale.MaxSize();
    sMetaData.rOS = NULL;
    sMetaData.ulOS = 0;
    sMetaData.rProcessor = NULL;
    sMetaData.ulProcessor = 0;
    
    IfFailGo(pCommonImport->GetAssemblyRefProps(
        tkAssemRef, 
        &pbPublicKeyOrToken, 
        &cbPublicKeyOrToken, 
        qaName.Ptr(), 
        (ULONG)qaName.MaxSize(), 
        NULL,       // pcchName
        &sMetaData, 
        &pbHashValue, 
        &cbHashValue, 
        &dwAssemblyRefFlags));
    
    if (pMiniMdAssemEmit)
    {
        IfFailGo(pMiniMdAssemEmit->DefineAssemblyRef(
            pbPublicKeyOrToken, 
            cbPublicKeyOrToken, 
            qaName.Ptr(), 
            &sMetaData, 
            pbHashValue, 
            cbHashValue, 
            dwAssemblyRefFlags, 
            &tkNewAssemblyRef));
    }

    if (ptkAssemblyRef != NULL)
    {
        *ptkAssemblyRef = tkNewAssemblyRef;
    }

ErrExit:
    return hr;
} // CreateAssemblyRefFromAssemblyRef

//******************************************************************************
// Given the Assembly Import scope, hash value and execution location, create
// a corresponding AssemblyRef in the given assembly and module emit scope.
// Set the output parameter to the AssemblyRef token emitted in the module emit
// scope.
//******************************************************************************
HRESULT 
CreateAssemblyRefFromAssembly(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,    // [IN] Emit assembly scope.
    IMetaDataAssemblyImport * pCommonAssemImport,  // [IN] Assembly import scope.
    const void *              pbHashValue,         // [IN] Hash Blob for Assembly.
    ULONG                     cbHashValue,         // [IN] Count of bytes.
    mdAssemblyRef *           ptkAssemblyRef)      // [OUT] AssemblyRef token.
{
    mdAssembly         tkAssembly;
    const void *       pbPublicKey;
    ULONG              cbPublicKey;
    CQuickArray<WCHAR> qaName;
    ULONG              cchName;
    ASSEMBLYMETADATA   sMetaData;
    CQuickArray<WCHAR> qaLocale;
    DWORD              dwAssemblyFlags;
    HRESULT            hr = S_OK;
    mdAssemblyRef      tkNewAssemblyRef = mdTokenNil;

    const void  * pbPublicKeyToken = NULL;
    ULONG         cbPublicKeyToken = 0;
    
    // Set output to Nil.
    if (ptkAssemblyRef != NULL)
    {
        *ptkAssemblyRef = mdTokenNil;
    }
    
    IfFailGo(pCommonAssemImport->GetAssemblyFromScope(&tkAssembly));
    
    // Get import AssemblyRef props.
    sMetaData.szLocale = NULL;
    sMetaData.cbLocale = 0;
    sMetaData.rOS = NULL;
    sMetaData.ulOS = 0;
    sMetaData.rProcessor = NULL;
    sMetaData.ulProcessor = 0;
    
    IfFailGo(pCommonAssemImport->GetAssemblyProps(
        tkAssembly,  
        NULL,       // ppbPublicKey
        NULL,       // pcbPublicKey
        NULL,       // pulHashAlgId
        NULL,       // wszName
        0,          // cchName
        &cchName, 
        &sMetaData, 
        NULL));     // pdwAssemblyFlags
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailGo(qaName.ReSizeNoThrow(cchName));
    IfFailGo(qaLocale.ReSizeNoThrow(sMetaData.cbLocale));
    
    sMetaData.szLocale = qaLocale.Ptr();
    sMetaData.cbLocale = (ULONG)qaLocale.MaxSize();
    sMetaData.rOS = NULL;
    sMetaData.ulOS = 0;
    sMetaData.rProcessor = NULL;
    sMetaData.ulProcessor = 0;
    
    IfFailGo(pCommonAssemImport->GetAssemblyProps(
        tkAssembly,  
        &pbPublicKey, 
        &cbPublicKey, 
        NULL,       // pulHashAlgId
        qaName.Ptr(), 
        (ULONG)qaName.MaxSize(), 
        NULL,       // pcchName
        &sMetaData, 
        &dwAssemblyFlags));
    
    // Compress the public key into a token.
    if ((pbPublicKey != NULL) && (cbPublicKey != 0))
    {
        _ASSERTE(IsAfPublicKey(dwAssemblyFlags));
        dwAssemblyFlags &= ~afPublicKey;
        IfFailGo(LegacyActivationShim::StrongNameTokenFromPublicKey_HRESULT(
            (BYTE *)pbPublicKey,
            cbPublicKey,
            (BYTE **)&pbPublicKeyToken,
            &cbPublicKeyToken));
    }
    else
    {
        _ASSERTE(!IsAfPublicKey(dwAssemblyFlags));
    }

    if (pMiniMdAssemEmit)
    {
        IfFailGo(pMiniMdAssemEmit->DefineAssemblyRef(
            pbPublicKeyToken, 
            cbPublicKeyToken, 
            qaName.Ptr(), 
            &sMetaData, 
            pbHashValue, 
            cbHashValue, 
            dwAssemblyFlags, 
            &tkNewAssemblyRef));
    }

    if (ptkAssemblyRef != NULL)
    {
        *ptkAssemblyRef = tkNewAssemblyRef; 
    }

ErrExit:
    if (pbPublicKeyToken != NULL)
    {
        LegacyActivationShim::StrongNameFreeBuffer((BYTE *)pbPublicKeyToken);
    }
    return hr;
} // CreateAssemblyRefFromAssembly

//******************************************************************************
// Given an AssemblyRef and the corresponding scope, compare it to see if it
// refers to the given Assembly.
//******************************************************************************
HRESULT CompareAssemblyRefToAssembly(
    IMetaDataAssemblyImport * pCommonAssem1,    // [IN] Scope that defines the AssemblyRef.
    mdAssemblyRef             tkAssemRef,       // [IN] AssemblyRef.
    IMetaDataAssemblyImport * pCommonAssem2)    // [IN] Assembly against which the Ref is compared.
{
    HRESULT hr = S_OK;
    
    const void *       pbPublicKeyOrToken1;
    ULONG              cbPublicKeyOrToken1;
    CQuickArray<WCHAR> qaName1;
    ULONG              cchName1;
    ASSEMBLYMETADATA   sMetaData1;
    CQuickArray<WCHAR> qaLocale1;
    DWORD              dwAssemblyRefFlags1;
    
    const void *       pbPublicKey2;
    ULONG              cbPublicKey2;
    CQuickArray<WCHAR> qaName2;
    ULONG              cchName2;
    ASSEMBLYMETADATA   sMetaData2;
    CQuickArray<WCHAR> qaLocale2;
    DWORD              dwAssemblyFlags2;
    const void *       pbPublicKeyToken2 = NULL;
    ULONG              cbPublicKeyToken2 = 0;
    mdAssembly         tkAssembly;
    
    // Get the AssemblyRef props.
    sMetaData1.szLocale = NULL;
    sMetaData1.cbLocale = 0;
    sMetaData1.rOS = NULL;
    sMetaData1.ulOS = 0;
    sMetaData1.rProcessor = NULL;
    sMetaData1.ulProcessor = 0;
    
    IfFailRet(pCommonAssem1->GetAssemblyRefProps(
        tkAssemRef, 
        NULL,   // ppbPublicKeyOrToken
        NULL,   // pcbPublicKeyOrToken
        NULL,   // wszName
        0,      // cchName
        &cchName1, 
        &sMetaData1, 
        NULL,   // ppbHashValue
        NULL,   // pcbHashValue
        NULL)); // pdwAssemblyRefFlags
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailRet(qaName1.ReSizeNoThrow(cchName1));
    IfFailRet(qaLocale1.ReSizeNoThrow(sMetaData1.cbLocale));
    
    sMetaData1.szLocale = qaLocale1.Ptr();
    sMetaData1.cbLocale = (ULONG)qaLocale1.MaxSize();
    sMetaData1.rOS = NULL;
    sMetaData1.ulOS = 0;
    sMetaData1.rProcessor = NULL;
    sMetaData1.ulProcessor = 0;
    
    IfFailRet(pCommonAssem1->GetAssemblyRefProps(
        tkAssemRef, 
        &pbPublicKeyOrToken1, 
        &cbPublicKeyOrToken1, 
        qaName1.Ptr(), 
        (ULONG)qaName1.MaxSize(), 
        NULL,   // pcchName
        &sMetaData1, 
        NULL,   // ppbHashValue
        NULL,   // pcbHashValue
        &dwAssemblyRefFlags1));
    
    IfFailRet(pCommonAssem2->GetAssemblyFromScope(&tkAssembly));
    
    // Get the Assembly props.
    sMetaData2.szLocale = NULL;
    sMetaData2.cbLocale = 0;
    sMetaData2.rOS = NULL;
    sMetaData2.ulOS = 0;
    sMetaData2.rProcessor = NULL;
    sMetaData2.ulProcessor = 0;
    
    IfFailRet(pCommonAssem2->GetAssemblyProps(
        tkAssembly,  
        NULL,       // ppbPublicKey
        NULL,       // pcbPublicKey
        NULL,       // pulHashAlgId
        NULL,       // wszName
        0,          // cchName
        &cchName2, 
        &sMetaData2, 
        NULL));      // pdwAssemblyFlags
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailRet(qaName2.ReSizeNoThrow(cchName2));
    IfFailRet(qaLocale2.ReSizeNoThrow(sMetaData2.cbLocale));
    
    sMetaData2.szLocale = qaLocale2.Ptr();
    sMetaData2.cbLocale = (ULONG)qaLocale2.MaxSize();
    sMetaData2.rOS = NULL;
    sMetaData2.ulOS = 0;
    sMetaData2.rProcessor = NULL;
    sMetaData2.ulProcessor = 0;
    
    IfFailRet(pCommonAssem2->GetAssemblyProps(
        tkAssembly,  
        &pbPublicKey2, 
        &cbPublicKey2, 
        NULL,       // pulHashAlgId
        qaName2.Ptr(), 
        (ULONG)qaName2.MaxSize(), 
        NULL,       // pcchName
        &sMetaData2, 
        &dwAssemblyFlags2));
    
    // Compare.
    if ((sMetaData1.usMajorVersion != sMetaData2.usMajorVersion) || 
        (sMetaData1.usMinorVersion != sMetaData2.usMinorVersion) || 
        (sMetaData1.usBuildNumber != sMetaData2.usBuildNumber) || 
        (sMetaData1.usRevisionNumber != sMetaData2.usRevisionNumber) || 
        (wcscmp(qaName1.Ptr(), qaName2.Ptr()) != 0) || 
        (wcscmp(qaLocale1.Ptr(), qaLocale2.Ptr()) != 0))
    {
        return S_FALSE;
    }
    
    // Defs always contain a full public key (or no key at all). Refs may have
    // no key, a full public key or a tokenized key.
    if (((cbPublicKeyOrToken1 != 0) && (cbPublicKey2 == 0)) ||
        ((cbPublicKeyOrToken1 == 0) && (cbPublicKey2 != 0)))
    {
        return S_FALSE;
    }
    
    if (cbPublicKeyOrToken1 != 0)
    {
        // If ref contains a full public key we can just directly compare.
        if (IsAfPublicKey(dwAssemblyRefFlags1) &&
            ((cbPublicKeyOrToken1 != cbPublicKey2) ||
             (memcmp(pbPublicKeyOrToken1, pbPublicKey2, cbPublicKeyOrToken1) != 0)))
        {
            return S_FALSE;
        }

        // Otherwise we need to compress the def public key into a token.
        IfFailRet(LegacyActivationShim::StrongNameTokenFromPublicKey_HRESULT(
            (BYTE *)pbPublicKey2,
            cbPublicKey2,
            (BYTE **)&pbPublicKeyToken2,
            &cbPublicKeyToken2));

        BOOL fMatch = ((cbPublicKeyOrToken1 == cbPublicKeyToken2) &&
                       (memcmp(pbPublicKeyOrToken1, pbPublicKeyToken2, cbPublicKeyOrToken1) == 0));
        
        LegacyActivationShim::StrongNameFreeBuffer((BYTE *)pbPublicKeyToken2);
        
        if (!fMatch)
            return S_FALSE;
    }
    
    return S_OK;
} // CompareAssemblyRefToAssembly

//****************************************************************************
// Given the arrays of names and namespaces for the Nested Type hierarchy,
// find the innermost TypeDef token.  The arrays start with the innermost
// TypeDef and go outwards.
//****************************************************************************
HRESULT FindNestedTypeDef(
    IMetaDataImport * pMiniMd,      // [IN] Scope in which to find the TypeRef.
    CQuickArray<CQuickArray<WCHAR>> & qaNesterNames,    // [IN] Array of Names.
    mdTypeDef         tkEnclosing,  // [IN] Enclosing class for the Outermost TypeDef.
    mdTypeDef *       ptd)          // [OUT] Inner most TypeRef token.
{
    HRESULT hr = S_OK;

    _ASSERTE(qaNesterNames.Size() != 0);

    // Set the output parameter to Nil token.
    *ptd = mdTokenNil;

    // Get count in the hierarchy, the give TypeDef included.
    ULONG cNesters = (ULONG)qaNesterNames.Size();

    // For each nester try to find the corresponding TypeRef in
    // the emit scope.  For the outermost TypeDef enclosing class is Nil.
    for (ULONG ixCurNester = cNesters - 1; ixCurNester != (ULONG)-1; ixCurNester--)
    {
        IfFailGo(pMiniMd->FindTypeDefByName(
            qaNesterNames[ixCurNester].Ptr(),
            tkEnclosing,
            &tkEnclosing));
    }
    *ptd = tkEnclosing;
    
ErrExit:
    return hr;
} // FindNestedTypeDef

//****************************************************************************
// Create the Nesting hierarchy given the array of TypeRef names.  The first
// TypeRef in the array is the innermost TypeRef.
//****************************************************************************
HRESULT CreateNesterHierarchy(
    IMetaDataEmit * pMiniMdEmit,        // [IN] Emit scope to create the Nesters in.
    CQuickArray<CQuickArray<WCHAR>> & qaNesterNames,  // [IN] Array of Nester names.
    mdToken         tkResolutionScope,  // [IN] ResolutionScope for the innermost TypeRef.
    mdTypeRef *     ptr)                // [OUT] Token for the innermost TypeRef.
{
    mdTypeRef tkNester;
    HRESULT   hr = S_OK;

    _ASSERTE(qaNesterNames.Size() != 0);

    // Initialize the output parameter.
    *ptr = mdTypeRefNil;

    // Get count of Nesters in the hierarchy.
    ULONG cNesters = (ULONG)qaNesterNames.Size();

    // For each nester try to find the corresponding TypeRef in the emit scope.
    // For the outermost TypeRef, ResolutionScope is what's passed in.
    if (tkResolutionScope == mdTokenNil)
    {
        tkNester = mdTypeRefNil;
    }
    else
    {
        tkNester = tkResolutionScope;
    }
    for (ULONG ixCurNester = cNesters - 1; ixCurNester != (ULONG)-1; ixCurNester--)
    {
        IfFailGo(pMiniMdEmit->DefineTypeRefByName(
            tkNester,
            qaNesterNames[ixCurNester].Ptr(),
            &tkNester));
        _ASSERTE((hr == S_OK) || (hr == META_S_DUPLICATE));
    }
    *ptr = tkNester;
    
ErrExit:
    return hr;
} // CreateNesterHierarchy

HRESULT ImportTypeRef(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,     // [IN] Assembly emit scope.
    IMetaDataImport *         pMiniMdAssemEmit_Import, 
    IMetaDataEmit *           pMiniMdEmit,          // [IN] Module emit scope.
    IMetaDataImport *         pMiniMdEmit_Import, 
    IMetaDataAssemblyImport * pCommonAssemImport,   // [IN] Assembly import scope.
    IMetaDataImport *         pCommonAssemImport_Import, 
    const void *              pbHashValue,      // [IN] Hash value for import assembly.
    ULONG                     cbHashValue,      // [IN] Size in bytes of hash value.
    IMetaDataImport *         pCommonImport,    // [IN] Module import scope.
    mdTypeRef                 trImport,         // [IN] Imported TypeRef.
    mdToken *                 ptkType,          // [OUT] Output token for the imported type in the emit scope.
    TypeForwardersResolver *  pTypeForwardersResolver) // [IN] Functor to resolve type forwarders
{
    CQuickArray<mdTypeDef>          qaNesters;
    CQuickArray<CQuickArray<WCHAR>> qaNesterNames;
    CQuickArray<WCHAR> qbScopeNameEmit;
    ULONG              cchScopeNameEmit;
    GUID        nullguid = GUID_NULL;
    GUID        MvidAssemImport = nullguid;
    GUID        MvidAssemEmit = nullguid;
    GUID        MvidImport = nullguid;
    GUID        MvidEmit = nullguid;
    mdToken     tkOuterImportRes;               // ResolutionScope for the outermost TypeRef in import scope.
    mdToken     tkOuterEmitRes = mdTokenNil;    // ResolutionScope for outermost TypeRef in emit scope.
    HRESULT     hr = S_OK;
    bool        bAssemblyRefFromAssemScope = false;
    IMetaDataAssemblyImport * pMiniMdAssemEmit_AssemblyImport = NULL;
    IMetaDataAssemblyEmit *   pMiniMdEmit_AssemblyEmit = NULL;
    IMetaDataAssemblyImport * pCommonImport_AssemblyImport = NULL;
    
    _ASSERTE(pMiniMdEmit && pCommonImport && ptkType);
    _ASSERTE(TypeFromToken(trImport) == mdtTypeRef);

    // Get MVIDs for import and emit, assembly and module scopes.
    if (pCommonAssemImport_Import != NULL)
    {
        IfFailGo(pCommonAssemImport_Import->GetScopeProps(NULL, 0, NULL, &MvidAssemImport));
    }
    IfFailGo(pCommonImport->GetScopeProps(NULL, 0, NULL, &MvidImport));
    if (pMiniMdAssemEmit_Import != NULL)
    {
        IfFailGo(pMiniMdAssemEmit_Import->GetScopeProps(NULL, 0, NULL, &MvidAssemEmit));
    }
    IfFailGo(pMiniMdEmit_Import->GetScopeProps(NULL, 0, &cchScopeNameEmit, &MvidEmit));
    IfFailGo(qbScopeNameEmit.ReSizeNoThrow(cchScopeNameEmit));
    IfFailGo(pMiniMdEmit_Import->GetScopeProps(qbScopeNameEmit.Ptr(), (ULONG)qbScopeNameEmit.MaxSize(), NULL, NULL));
    
    IfFailGo(pMiniMdEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **)&pMiniMdEmit_AssemblyEmit));
    _ASSERTE(pMiniMdEmit_AssemblyEmit != NULL);
    IfFailGo(pCommonImport->QueryInterface(IID_IMetaDataAssemblyImport, (void **)&pCommonImport_AssemblyImport));
    _ASSERTE(pCommonImport_AssemblyImport != NULL);
    
    if (pMiniMdAssemEmit)
    {
        IfFailGo(pMiniMdAssemEmit->QueryInterface(IID_IMetaDataAssemblyImport, (void **)&pMiniMdAssemEmit_AssemblyImport));
        _ASSERTE(pMiniMdAssemEmit_AssemblyImport != NULL);
    }

    // Get the outermost resolution scope for the TypeRef being imported.
    IfFailGo(GetNesterHierarchy(pCommonImport,
                                trImport,
                                qaNesters,
                                qaNesterNames));
    IfFailGo(pCommonImport->GetTypeRefProps(
        qaNesters[qaNesters.Size() - 1], 
        &tkOuterImportRes, 
        NULL, 
        0, 
        NULL));
    
    IMetaDataAssemblyImport *pTypeForwarderAssemImport = NULL;
    const void * pbRedirHashValue = NULL; 
    ULONG cbRedirHashValue = NULL;
    pTypeForwardersResolver->Resolve(trImport, &pTypeForwarderAssemImport, &pbRedirHashValue, &cbRedirHashValue);

    // Compute the ResolutionScope for the imported type.
    if (MvidAssemImport == MvidAssemEmit && MvidImport == MvidEmit)
    {
        *ptkType = trImport;
        goto ErrExit;
    }
    else if (MvidAssemImport == MvidAssemEmit && MvidImport != MvidEmit)
    {
        // The TypeRef is in the same Assembly but a different module.

        if (IsNilToken(tkOuterImportRes))
        {
            tkOuterEmitRes = tkOuterImportRes;
        }
        else if (TypeFromToken(tkOuterImportRes) == mdtModule)
        {
            // TypeRef resolved to the import module in which its defined.

            // <






            if (pMiniMdAssemEmit == NULL && pCommonAssemImport == NULL)
            {
                tkOuterEmitRes = TokenFromRid(1, mdtModule);
            }
            else
            {
                // Create a ModuleRef corresponding to the import scope.
                IfFailGo(CreateModuleRefFromScope(
                    pMiniMdEmit, 
                    //pMiniMdEmit_Import, 
                    pCommonImport, 
                    &tkOuterEmitRes));
            }
        }
        else if (TypeFromToken(tkOuterImportRes) == mdtAssemblyRef)
        {
            // TypeRef is from a different Assembly.
            // Type is from a different Assembly.
            if (pTypeForwarderAssemImport)
            {
                // Type is from a different Assembly.
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdAssemEmit,
                    pTypeForwarderAssemImport,
                    pbRedirHashValue,
                    cbRedirHashValue,
                    NULL)); // ptkAssemblyRef
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdEmit_AssemblyEmit,
                    pTypeForwarderAssemImport,
                    pbRedirHashValue,
                    cbRedirHashValue,
                    &tkOuterEmitRes));
            }
            else
            {
                // Create a corresponding AssemblyRef in the assembly emit scope.
                IfFailGo(CreateAssemblyRefFromAssemblyRef(
                    pMiniMdAssemEmit, 
                    pCommonImport_AssemblyImport,
                    tkOuterImportRes,
                    NULL)); // ptkAssemblyRef
                // Create a corresponding AssemblyRef in the assembly module scope.
                IfFailGo(CreateAssemblyRefFromAssemblyRef(
                    pMiniMdEmit_AssemblyEmit, 
                    pCommonImport_AssemblyImport,
                    tkOuterImportRes,
                    &tkOuterEmitRes));
            }
        }
        else if (TypeFromToken(tkOuterImportRes) == mdtModuleRef)
        {
            // Get Name of the ModuleRef.
            CQuickArray<WCHAR> qbModuleRefName;
            ULONG              cchModuleRefName;
            IfFailGo(pCommonImport->GetModuleRefProps(tkOuterImportRes, NULL, 0, &cchModuleRefName));
            IfFailGo(qbModuleRefName.ReSizeNoThrow(cchModuleRefName));
            IfFailGo(pCommonImport->GetModuleRefProps(tkOuterImportRes, qbModuleRefName.Ptr(), (ULONG)qbModuleRefName.MaxSize(), NULL));
            
            if (!wcscmp(qbModuleRefName.Ptr(), qbScopeNameEmit.Ptr()))
            {
                // ModuleRef from import scope resolves to the emit scope.
                tkOuterEmitRes = TokenFromRid(1, mdtModule);
            }
            else
            {
                // ModuleRef does not correspond to the emit scope.
                // Create a corresponding ModuleRef.
                IfFailGo(CreateModuleRefFromModuleRef(
                    pMiniMdEmit,
                    pCommonImport,
                    tkOuterImportRes,
                    &tkOuterEmitRes));
            }
        }
    }
    else if (MvidAssemImport != MvidAssemEmit)
    {
        // The TypeDef is from a different Assembly.

        // Import and Emit scopes can't be identical and be from different
        // Assemblies at the same time.
        _ASSERTE(MvidImport != MvidEmit &&
                 "Import scope can't be identical to the Emit scope and be from a different Assembly at the same time.");

        mdToken tkImplementation;       // Implementation token for ExportedType.
        if (IsNilToken(tkOuterImportRes))
        {
            // <



            // Look for a ExportedType entry in the import Assembly.  Its an error
            // if we don't find a ExportedType entry.
            mdExportedType tkExportedType;
            hr = pCommonAssemImport->FindExportedTypeByName(
                                    qaNesterNames[qaNesters.Size() - 1].Ptr(),
                                    mdTokenNil,
                                    &tkExportedType);
            if (SUCCEEDED(hr))
            {
                IfFailGo(pCommonAssemImport->GetExportedTypeProps(
                    tkExportedType, 
                    NULL,   // szName
                    0,      // cchName
                    NULL,   // pchName
                    &tkImplementation, 
                    NULL,   // ptkTypeDef
                    NULL)); // pdwExportedTypeFlags
                if (TypeFromToken(tkImplementation) == mdtFile)
                {
                    // Type is from a different Assembly.
                    IfFailGo(CreateAssemblyRefFromAssembly(
                        pMiniMdAssemEmit,
                        pCommonAssemImport,
                        pbHashValue,
                        cbHashValue,
                        NULL)); // ptkAssemblyRef
                    IfFailGo(CreateAssemblyRefFromAssembly(
                        pMiniMdEmit_AssemblyEmit,
                        pCommonAssemImport,
                        pbHashValue,
                        cbHashValue,
                        &tkOuterEmitRes));
                }
                else if (TypeFromToken(tkImplementation) == mdtAssemblyRef)
                {
                    // This folds into the case where the Type is AssemblyRef.  So
                    // let it fall through to that case.

                    // Remember that this AssemblyRef token is actually from the Manifest scope not
                    // the module scope!!!
                    bAssemblyRefFromAssemScope = true;
                    tkOuterImportRes = tkImplementation;
                }
                else
                    _ASSERTE(!"Unexpected ExportedType implementation token.");
            }
            else
            {
                // In this case, we will just move over the TypeRef with Nil ResolutionScope.
                hr = NOERROR;
                tkOuterEmitRes = mdTokenNil;
            }
        }
        else if (TypeFromToken(tkOuterImportRes) == mdtModule)
        {
            // Type is from a different Assembly.
            if (pTypeForwarderAssemImport)
            {
                // Type is from a different Assembly.
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdAssemEmit,
                    pTypeForwarderAssemImport,
                    pbRedirHashValue,
                    cbRedirHashValue,
                    NULL)); // ptkAssemblyRef
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdEmit_AssemblyEmit,
                    pTypeForwarderAssemImport,
                    pbRedirHashValue,
                    cbRedirHashValue,
                    &tkOuterEmitRes));
            }
            else
            {
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdAssemEmit,
                    pCommonAssemImport,
                    pbHashValue,
                    cbHashValue,
                    NULL)); // ptkAssemblyRef
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdEmit_AssemblyEmit,
                    pCommonAssemImport,
                    pbHashValue,
                    cbHashValue,
                    &tkOuterEmitRes));
            }
        }
        // Not else if, because mdtModule case above could change
        // tkOuterImportRes to an AssemblyRef.
        if (TypeFromToken(tkOuterImportRes) == mdtAssemblyRef)
        {
            mdAssembly tkAssembly;
            // If there is an emit assembly, see if the import assembly ref points to 
            //  it.  If there is no emit assembly, the import assembly, by definition,
            //  does not point to this one.
            if ((pMiniMdAssemEmit == NULL) || 
                FAILED(pMiniMdAssemEmit_AssemblyImport->GetAssemblyFromScope(&tkAssembly)))
            {
                hr = S_FALSE;
            }
            else
            {
                if (bAssemblyRefFromAssemScope)
                {
                    // Check to see if the AssemblyRef resolves to the emit assembly.
                    IfFailGo(CompareAssemblyRefToAssembly(
                        pCommonAssemImport,
                        tkOuterImportRes,
                        pMiniMdAssemEmit_AssemblyImport));
                }
                else
                {
                    // Check to see if the AssemblyRef resolves to the emit assembly.
                    IfFailGo(CompareAssemblyRefToAssembly(
                        pCommonImport_AssemblyImport,
                        tkOuterImportRes,
                        pMiniMdAssemEmit_AssemblyImport));
                }
            }
            
            if (hr == S_OK)
            {
                // The TypeRef being imported is defined in the current Assembly.
                
                // Find the ExportedType for the outermost TypeRef in the Emit assembly.
                mdExportedType tkExportedType;
                hr = pMiniMdAssemEmit_AssemblyImport->FindExportedTypeByName(
                                 qaNesterNames[qaNesters.Size() - 1].Ptr(), 
                                 mdTokenNil,        // Enclosing ExportedType
                                 &tkExportedType);
                if (hr == S_OK)
                {
                    // Create a ModuleRef based on the File name for the ExportedType.
                    // If the ModuleRef corresponds to pMiniMdEmit, the function
                    // will return S_FALSE, in which case set tkOuterEmitRes to
                    // the Module token.
                    IfFailGo(CreateModuleRefFromExportedType(
                        pMiniMdAssemEmit, 
                        pMiniMdAssemEmit_AssemblyImport, 
                        pMiniMdEmit, 
                        pMiniMdEmit_Import, 
                        tkExportedType, 
                        &tkOuterEmitRes));
                    if (hr == S_FALSE)
                    {
                        tkOuterEmitRes = TokenFromRid(1, mdtModule);
                    }
                }
                else if (hr == CLDB_E_RECORD_NOTFOUND)
                {
                    // Find the Type in the Assembly emit scope to cover the
                    // case where ExportedTypes may be implicitly defined.  Its an
                    // error if we can't find the Type at this point.
                    IfFailGo(pMiniMdAssemEmit_Import->FindTypeDefByName(
                        qaNesterNames[qaNesters.Size() - 1].Ptr(), 
                        mdTokenNil,     // Enclosing Type
                        &tkOuterEmitRes)); 
                    tkOuterEmitRes = TokenFromRid(1, mdtModule);
                }
                else
                {
                    _ASSERTE(FAILED(hr));
                    IfFailGo(hr);
                }
            }
            else if (hr == S_FALSE)
            {
                // The TypeRef being imported is from a different Assembly.
                if (bAssemblyRefFromAssemScope)
                {
                    // Create a corresponding AssemblyRef.
                    IfFailGo(CreateAssemblyRefFromAssemblyRef(
                        pMiniMdAssemEmit,
                        pCommonAssemImport,
                        tkOuterImportRes,
                        NULL)); // ptkAssemblyRef
                    IfFailGo(CreateAssemblyRefFromAssemblyRef(
                        pMiniMdEmit_AssemblyEmit,
                        pCommonAssemImport,
                        tkOuterImportRes,
                        &tkOuterEmitRes));
                }
                else
                {
                    if (pTypeForwarderAssemImport)
                    {
                        // Type is from a different Assembly.
                        IfFailGo(CreateAssemblyRefFromAssembly(
                            pMiniMdAssemEmit,
                            pTypeForwarderAssemImport,
                            pbRedirHashValue,
                            cbRedirHashValue,
                            NULL)); // ptkAssemblyRef
                        IfFailGo(CreateAssemblyRefFromAssembly(
                            pMiniMdEmit_AssemblyEmit,
                            pTypeForwarderAssemImport,
                            pbRedirHashValue,
                            cbRedirHashValue,
                            &tkOuterEmitRes));
                    }
                    else
                    {
                        // Create a corresponding AssemblyRef.
                        IfFailGo(CreateAssemblyRefFromAssemblyRef(
                            pMiniMdAssemEmit,
                            pCommonImport_AssemblyImport,
                            tkOuterImportRes,
                            NULL)); // ptkAssemblyRef
                        IfFailGo(CreateAssemblyRefFromAssemblyRef(
                            pMiniMdEmit_AssemblyEmit,
                            pCommonImport_AssemblyImport,
                            tkOuterImportRes,
                            &tkOuterEmitRes));
                    }
                }
            }
            else
            {
                _ASSERTE(FAILED(hr));
                IfFailGo(hr);
            }
        }
        else if (TypeFromToken(tkOuterImportRes) == mdtModuleRef)
        {
            // Type is from a different Assembly.
            IfFailGo(CreateAssemblyRefFromAssembly(
                pMiniMdAssemEmit,
                pCommonAssemImport,
                pbHashValue,
                cbHashValue,
                NULL)); // ptkAssemblyRef
            IfFailGo(CreateAssemblyRefFromAssembly(
                pMiniMdEmit_AssemblyEmit,
                pCommonAssemImport,
                pbHashValue,
                cbHashValue,
                &tkOuterEmitRes));
        }
    }

    // Try to find the TypeDef in the emit scope. If we cannot find the
    // typedef, we need to introduce a typeref.

    // See if the Nested TypeDef is present in the Emit scope.
    hr = CLDB_E_RECORD_NOTFOUND;
    if (TypeFromToken(tkOuterEmitRes) == mdtModule && !IsNilToken(tkOuterEmitRes))
    {
        hr = FindNestedTypeDef(pMiniMdEmit_Import,
                               qaNesterNames,
                               mdTokenNil,
                               ptkType);
        _ASSERTE(SUCCEEDED(hr));
    }

    if (hr == CLDB_E_RECORD_NOTFOUND)
    {
        IfFailGo(CreateNesterHierarchy(pMiniMdEmit,
                                       qaNesterNames,
                                       tkOuterEmitRes,
                                       ptkType));
    }
    else
    {
        IfFailGo(hr);
    }
    
ErrExit:
    if (pMiniMdEmit_AssemblyEmit != NULL)
        pMiniMdEmit_AssemblyEmit->Release();
    if (pCommonImport_AssemblyImport != NULL)
        pCommonImport_AssemblyImport->Release();
    if (pMiniMdAssemEmit_AssemblyImport != NULL)
        pMiniMdAssemEmit_AssemblyImport->Release();
    DestroyNestedQuickArray(qaNesterNames);
    return hr;
} // ImportTypeRef

//****************************************************************************
// Given the TypeDef and the corresponding assembly and module import scopes,
// create a corresponding TypeRef in the given emit scope.
//****************************************************************************
HRESULT 
ImportTypeDef(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,         // [IN] Assembly emit scope.
    IMetaDataImport *         pMiniMdAssemEmit_Import, 
    IMetaDataEmit *           pMiniMdEmit,              // [IN] Module emit scope.
    IMetaDataImport *         pMiniMdEmit_Import, 
    IMetaDataAssemblyEmit *   pMiniMdEmit_AssemblyEmit, 
    IMetaDataAssemblyImport * pCommonAssemImport,       // [IN] Assembly import scope.
    IMetaDataImport *         pCommonAssemImport_Import, 
    const void *       pbHashValue,         // [IN] Hash value for import assembly.
    ULONG              cbHashValue,         // [IN] Size in bytes of hash value.
    IMetaDataImport *  pCommonImport,       // [IN] Module import scope.
    mdTypeDef          tdImport,            // [IN] Imported TypeDef.
    bool               fOptimizeToTypeDefs, // [IN] If the import and emit scopes are identical, return the TypeDef.
    mdToken *          ptkType,             // [OUT] Output token for the imported type in the emit scope.
    TypeForwardersResolver *  pTypeForwardersResolver) // [IN] Functor to resolve type forwarders
{
    CQuickArray<mdTypeDef>  qaNesters;
    CQuickArray<CQuickArray<WCHAR>> qaNesterNames;
    GUID        nullguid = GUID_NULL;
    GUID        MvidAssemImport = nullguid;
    GUID        MvidAssemEmit = nullguid;
    GUID        MvidImport = nullguid;
    GUID        MvidEmit = nullguid;
    GUID        GuidImport = GUID_NULL;
    CQuickArray<WCHAR> qaModuleImport;
    ULONG              cchName;
    mdToken     tkOuterRes = mdTokenNil;
    HRESULT     hr = S_OK;

    IMetaDataAssemblyImport *pTypeForwarderAssemImport = NULL;
    const void * pbRedirHashValue = NULL; 
    ULONG cbRedirHashValue = NULL;
    pTypeForwardersResolver->Resolve(tdImport, &pTypeForwarderAssemImport, &pbRedirHashValue, &cbRedirHashValue);

    _ASSERTE(pMiniMdEmit && pCommonImport && ptkType);
    _ASSERTE(TypeFromToken(tdImport) == mdtTypeDef && tdImport != mdTypeDefNil);

    // Get MVIDs for import and emit, assembly and module scopes.
    if (pCommonAssemImport != NULL)
    {
        _ASSERTE(pCommonAssemImport_Import != NULL);
        IfFailGo(pCommonAssemImport_Import->GetScopeProps(NULL, 0, NULL, &MvidAssemImport));
    }
    IfFailGo(pCommonImport->GetScopeProps(NULL, 0, &cchName, &MvidImport));
    _ASSERTE((hr == S_OK) || (hr == CLDB_S_TRUNCATION));
    IfFailGo(qaModuleImport.ReSizeNoThrow(cchName));
    IfFailGo(pCommonImport->GetScopeProps(qaModuleImport.Ptr(), (ULONG)qaModuleImport.MaxSize(), NULL, NULL));
    
    if (pMiniMdAssemEmit != NULL)
    {
        _ASSERTE(pMiniMdAssemEmit_Import != NULL);
        IfFailGo(pMiniMdAssemEmit_Import->GetScopeProps(NULL, 0, NULL, &MvidAssemEmit));
    }
    IfFailGo(pMiniMdEmit_Import->GetScopeProps(NULL, 0, NULL, &MvidEmit));

    if ((MvidAssemImport == MvidAssemEmit) && (MvidImport == MvidEmit))
    {
        // The TypeDef is in the same Assembly and the Same scope.
        if (fOptimizeToTypeDefs)
        {
            *ptkType = tdImport;
            goto ErrExit;
        }
        else
            tkOuterRes = TokenFromRid(1, mdtModule);
    }
    else if (MvidAssemImport == MvidAssemEmit && MvidImport != MvidEmit)
    {
        // The TypeDef is in the same Assembly but a different module.
        
        // Create a ModuleRef corresponding to the import scope.
        IfFailGo(CreateModuleRefFromScope(pMiniMdEmit, pCommonImport, &tkOuterRes));
    }
    else if (MvidAssemImport != MvidAssemEmit)
    {
        if (pCommonAssemImport != NULL)
        {
            // The TypeDef is from a different Assembly.

            // Import and Emit scopes can't be identical and be from different
            // Assemblies at the same time.
            _ASSERTE(MvidImport != MvidEmit &&
                     "Import scope can't be identical to the Emit scope and be from a different Assembly at the same time.");

            if (pTypeForwarderAssemImport)
            {
                // Create an AssemblyRef corresponding to the import scope.
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdAssemEmit,
                    pTypeForwarderAssemImport,
                    pbRedirHashValue,
                    cbRedirHashValue,
                    NULL)); // ptkAssemblyRef
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdEmit_AssemblyEmit,
                    pTypeForwarderAssemImport,
                    pbRedirHashValue,
                    cbRedirHashValue,
                    &tkOuterRes));
            }
            else
            {
                // Create an AssemblyRef corresponding to the import scope.
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdAssemEmit,
                    pCommonAssemImport,
                    pbHashValue,
                    cbHashValue,
                    NULL)); // ptkAssemblyRef
                IfFailGo(CreateAssemblyRefFromAssembly(
                    pMiniMdEmit_AssemblyEmit,
                    pCommonAssemImport,
                    pbHashValue,
                    cbHashValue,
                    &tkOuterRes));
            }
        }
        else
        {
            // <


            tkOuterRes = mdTokenNil;
        }
    }

    // Get the nesting hierarchy for the Type from the import scope and create
    // the corresponding Type hierarchy in the emit scope.  Note that the non-
    // nested class case simply folds into this scheme.

    IfFailGo(GetNesterHierarchy(pCommonImport,
                                tdImport,
                                qaNesters,
                                qaNesterNames));

    IfFailGo(CreateNesterHierarchy(pMiniMdEmit,
                                   qaNesterNames,
                                   tkOuterRes,
                                   ptkType));
ErrExit:
    DestroyNestedQuickArray(qaNesterNames);
    return hr;
} // ImportTypeDef

//****************************************************************************
// Convert tokens contained in an element type
//****************************************************************************
HRESULT 
MergeUpdateTokenInFieldSig(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,       // [IN] The assembly emit scope.
    IMetaDataImport *         pMiniMdAssemEmit_Import, 
    IMetaDataEmit *           pMiniMdEmit,            // [IN] The emit scope.
    IMetaDataImport *         pMiniMdEmit_Import, 
    IMetaDataAssemblyEmit *   pMiniMdEmit_AssemblyEmit, 
    IMetaDataAssemblyImport * pCommonAssemImport,   // [IN] Assembly scope where the signature is from.
    IMetaDataImport *         pCommonAssemImport_Import, 
    const void  *             pbHashValue,          // [IN] Hash value for the import assembly.
    ULONG                     cbHashValue,          // [IN] Size in bytes for the hash value.
    IMetaDataImport *         pCommonImport,        // [IN] The scope to merge into the emit scope.
    PCCOR_SIGNATURE           pbSigImp,             // signature from the imported scope
    CQuickBytes * pqkSigEmit,   // [OUT] buffer for translated signature
    ULONG         cbStartEmit,  // [IN] start point of buffer to write to
    ULONG *       pcbImp,       // [OUT] total number of bytes consumed from pbSigImp
    ULONG *       pcbEmit,      // [OUT] total number of bytes write to pqkSigEmit
    TypeForwardersResolver *  pTypeForwardersResolver) // [IN] Functor to resolve type forwarders
{

    HRESULT     hr;                     // A result.
    ULONG       cb;                     // count of bytes
    ULONG       cb1;                    // count of bytes
    ULONG       cb2;                    // count of bytes
    ULONG       cbSubTotal;
    ULONG       cbImp;
    ULONG       cbEmit;
    ULONG       cbSrcTotal = 0;         // count of bytes consumed in the imported signature
    ULONG       cbDestTotal = 0;        // count of bytes for the new signature
    ULONG       ulElementType = 0;      // place holder for expanded data
    ULONG       ulData;
    ULONG       ulTemp;
    mdToken     tkRidFrom;              // Original rid
    mdToken     tkRidTo;                // new rid
    int         iData;
    CQuickArray<mdToken> qaNesters;    // Array of Nester tokens.
    CQuickArray<CQuickArray<WCHAR>> qaNesterNames;    // Array of Nester names.

    _ASSERTE(pcbEmit);

    cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &ulElementType);
    cbSrcTotal += cb;

    // count numbers of modifiers
    while (CorIsModifierElementType((CorElementType) ulElementType))
    {
        cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &ulElementType);
        cbSrcTotal += cb;
    }

    // copy ELEMENT_TYPE_* over
    cbDestTotal = cbSrcTotal;
    IfFailGo(pqkSigEmit->ReSizeNoThrow(cbStartEmit + cbDestTotal));
    memcpy(((BYTE *)pqkSigEmit->Ptr()) + cbStartEmit, pbSigImp, cbDestTotal);
    switch (ulElementType)
    {
        case ELEMENT_TYPE_SZARRAY:
            // syntax : SZARRAY <BaseType>

            // conver the base type for the SZARRAY or GENERICARRAY
            IfFailGo(MergeUpdateTokenInFieldSig(
                pMiniMdAssemEmit,           // The assembly emit scope.
                pMiniMdAssemEmit_Import, 
                pMiniMdEmit,                // The emit scope.
                pMiniMdEmit_Import, 
                pMiniMdEmit_AssemblyEmit, 
                pCommonAssemImport,         // The assembly scope where the signature is from.
                pCommonAssemImport_Import, 
                pbHashValue,                // Hash value for the import assembly.
                cbHashValue,                // Size in bytes for the hash value.
                pCommonImport,              // scope to merge into the emit scope.
                &pbSigImp[cbSrcTotal],      // from the imported scope
                pqkSigEmit,                 // [OUT] buffer for translated signature
                cbStartEmit + cbDestTotal,  // [IN] start point of buffer to write to
                &cbImp,                     // [OUT] total number of bytes consumed from pbSigImp
                &cbEmit,                    // [OUT] total number of bytes write to pqkSigEmit
                pTypeForwardersResolver));
            cbSrcTotal += cbImp;
            cbDestTotal += cbEmit;
            break;

        case ELEMENT_TYPE_GENERICINST:
          {
            // syntax : WITH (ELEMENT_TYPE_CLASS | ELEMENT_TYPE_VALUECLASS)  <BaseType>

            IfFailGo(MergeUpdateTokenInFieldSig(
                pMiniMdAssemEmit,           // The assembly emit scope.
                pMiniMdAssemEmit_Import, 
                pMiniMdEmit,                // The emit scope.
                pMiniMdEmit_Import, 
                pMiniMdEmit_AssemblyEmit, 
                pCommonAssemImport,         // The assembly scope where the signature is from.
                pCommonAssemImport_Import, 
                pbHashValue,                // Hash value for the import assembly.
                cbHashValue,                // Size in bytes for the hash value.
                pCommonImport,              // scope to merge into the emit scope.
                &pbSigImp[cbSrcTotal],      // from the imported scope
                pqkSigEmit,                 // [OUT] buffer for translated signature
                cbStartEmit + cbDestTotal,  // [IN] start point of buffer to write to
                &cbImp,                     // [OUT] total number of bytes consumed from pbSigImp
                &cbEmit,                    // [OUT] total number of bytes write to pqkSigEmit
                pTypeForwardersResolver));
            cbSrcTotal += cbImp;
            cbDestTotal += cbEmit;

            // copy over the number of arguments
            ULONG nargs;
            cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &nargs);

            IfFailGo(pqkSigEmit->ReSizeNoThrow(cbStartEmit + cbDestTotal + cb));
            cb1 = CorSigCompressData(nargs, ((BYTE *)pqkSigEmit->Ptr()) + cbStartEmit + cbDestTotal);
            _ASSERTE(cb == cb1);

            cbSrcTotal += cb;
            cbDestTotal += cb1;

            for (ULONG narg = 0; narg < nargs; narg++) {
                IfFailGo(MergeUpdateTokenInFieldSig(
                    pMiniMdAssemEmit,           // The assembly emit scope.
                    pMiniMdAssemEmit_Import, 
                    pMiniMdEmit,                // The emit scope.
                    pMiniMdEmit_Import, 
                    pMiniMdEmit_AssemblyEmit, 
                    pCommonAssemImport,         // The assembly scope where the signature is from.
                    pCommonAssemImport_Import, 
                    pbHashValue,                // Hash value for the import assembly.
                    cbHashValue,                // Size in bytes for the hash value.
                    pCommonImport,              // The scope to merge into the emit scope.
                    &pbSigImp[cbSrcTotal],      // signature from the imported scope
                    pqkSigEmit,                 // [OUT] buffer for translated signature
                    cbStartEmit + cbDestTotal,  // [IN] start point of buffer to write to
                    &cbImp,                     // [OUT] total number of bytes consumed from pbSigImp
                    &cbEmit,                    // [OUT] total number of bytes write to pqkSigEmit
                    pTypeForwardersResolver));
                cbSrcTotal += cbImp;
                cbDestTotal += cbEmit;
            }
         }

         break;

        case ELEMENT_TYPE_MVAR:
        case ELEMENT_TYPE_VAR:
            // syntax : VAR <n>
            // syntax : MVAR <n>

            // after the VAR or MVAR there is an integer indicating which type variable
            //
            cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &ulData);

            IfFailGo(pqkSigEmit->ReSizeNoThrow(cbStartEmit + cbDestTotal + cb));
            cb1 = CorSigCompressData(ulData, ((BYTE *)pqkSigEmit->Ptr()) + cbStartEmit + cbDestTotal);
            _ASSERTE(cb == cb1);

            cbSrcTotal += cb;
            cbDestTotal += cb1;

            break;

        case ELEMENT_TYPE_ARRAY:
            // syntax : ARRAY BaseType <rank> [i size_1... size_i] [j lowerbound_1 ... lowerbound_j]

            // conver the base type for the MDARRAY
            IfFailGo(MergeUpdateTokenInFieldSig(
                pMiniMdAssemEmit,           // The assembly emit scope.
                pMiniMdAssemEmit_Import, 
                pMiniMdEmit,                // The emit scope.
                pMiniMdEmit_Import, 
                pMiniMdEmit_AssemblyEmit, 
                pCommonAssemImport,         // The assembly scope where the signature is from.
                pCommonAssemImport_Import, 
                pbHashValue,                // Hash value for the import assembly.
                cbHashValue,                // Size in bytes for the hash value.
                pCommonImport,              // The scope to merge into the emit scope.
                &pbSigImp[cbSrcTotal],      // signature from the imported scope
                pqkSigEmit,                 // [OUT] buffer for translated signature
                cbStartEmit + cbSrcTotal,   // [IN] start point of buffer to write to
                &cbImp,                     // [OUT] total number of bytes consumed from pbSigImp
                &cbEmit,                    // [OUT] total number of bytes write to pqkSigEmit
                pTypeForwardersResolver));
            cbSrcTotal += cbImp;
            cbDestTotal += cbEmit;

            // Parse for the rank
            cbSubTotal = CorSigUncompressData(&pbSigImp[cbSrcTotal], &ulData);

            // if rank == 0, we are done
            if (ulData != 0)
            {
                // any size of dimension specified?
                cb = CorSigUncompressData(&pbSigImp[cbSrcTotal + cbSubTotal], &ulData);
                cbSubTotal += cb;

                while (ulData--)
                {
                    cb = CorSigUncompressData(&pbSigImp[cbSrcTotal + cbSubTotal], &ulTemp);
                    cbSubTotal += cb;
                }

                // any lower bound specified?
                cb = CorSigUncompressData(&pbSigImp[cbSrcTotal + cbSubTotal], &ulData);
                cbSubTotal += cb;

                while (ulData--)
                {
                    cb = CorSigUncompressSignedInt(&pbSigImp[cbSrcTotal + cbSubTotal], &iData);
                    cbSubTotal += cb;
                }
            }

            // cbSubTotal is now the number of bytes still left to move over
            // cbSrcTotal is where bytes start on the pbSigImp to be copied over
            // cbStartEmit + cbDestTotal is where the destination of copy

            IfFailGo(pqkSigEmit->ReSizeNoThrow(cbStartEmit + cbDestTotal + cbSubTotal));
            memcpy(((BYTE *)pqkSigEmit->Ptr())+cbStartEmit + cbDestTotal, &pbSigImp[cbSrcTotal], cbSubTotal);

            cbSrcTotal = cbSrcTotal + cbSubTotal;
            cbDestTotal = cbDestTotal + cbSubTotal;

            break;
        case ELEMENT_TYPE_FNPTR:
            // function pointer is followed by another complete signature
            IfFailGo(MergeUpdateTokenInSig(
                pMiniMdAssemEmit,           // The assembly emit scope.
                pMiniMdAssemEmit_Import, 
                pMiniMdEmit,                // The emit scope.
                pMiniMdEmit_Import, 
                pMiniMdEmit_AssemblyEmit, 
                pCommonAssemImport,         // The assembly scope where the signature is from.
                pCommonAssemImport_Import, 
                pbHashValue,                // Hash value for the import assembly.
                cbHashValue,                // Size in bytes for the hash value.
                pCommonImport,              // The scope to merge into the emit scope.
                &pbSigImp[cbSrcTotal],      // signature from the imported scope
                pqkSigEmit,                 // [OUT] buffer for translated signature
                cbStartEmit + cbDestTotal,  // [IN] start point of buffer to write to
                &cbImp,                     // [OUT] total number of bytes consumed from pbSigImp
                &cbEmit,                  // [OUT] total number of bytes write to pqkSigEmit
                pTypeForwardersResolver));
            cbSrcTotal += cbImp;
            cbDestTotal += cbEmit;
            break;
        case ELEMENT_TYPE_VALUETYPE:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_CMOD_REQD:
        case ELEMENT_TYPE_CMOD_OPT:

            // syntax for CLASS = ELEMENT_TYPE_CLASS <rid>
            // syntax for VALUE_CLASS = ELEMENT_TYPE_VALUECLASS <rid>

            // now get the embedded typeref token
            cb = CorSigUncompressToken(&pbSigImp[cbSrcTotal], &tkRidFrom);

            {
                // If the token is a TypeDef or a TypeRef, get/create the
                // ResolutionScope for the outermost TypeRef.
                if (TypeFromToken(tkRidFrom) == mdtTypeDef)
                {
                    IfFailGo(ImportTypeDef(
                        pMiniMdAssemEmit, 
                        pMiniMdAssemEmit_Import, 
                        pMiniMdEmit, 
                        pMiniMdEmit_Import, 
                        pMiniMdEmit_AssemblyEmit, 
                        pCommonAssemImport, 
                        pCommonAssemImport_Import, 
                        pbHashValue, 
                        cbHashValue, 
                        pCommonImport, 
                        tkRidFrom, 
                        true,       // Optimize to TypeDef if emit and import scopes are identical.
                        &tkRidTo,
                        pTypeForwardersResolver));
                }
                else if (TypeFromToken(tkRidFrom) == mdtTypeRef)
                {
                    IfFailGo(ImportTypeRef(
                        pMiniMdAssemEmit, 
                        pMiniMdAssemEmit_Import, 
                        pMiniMdEmit, 
                        pMiniMdEmit_Import, 
                        pCommonAssemImport, 
                        pCommonAssemImport_Import, 
                        pbHashValue, 
                        cbHashValue, 
                        pCommonImport, 
                        tkRidFrom, 
                        &tkRidTo,
                        pTypeForwardersResolver));
                }
                else if ( TypeFromToken(tkRidFrom) == mdtTypeSpec )
                {
                    // copy over the TypeSpec
                    PCCOR_SIGNATURE pvTypeSpecSig;
                    ULONG           cbTypeSpecSig;
                    CQuickBytes qkTypeSpecSigEmit;
                    ULONG           cbTypeSpecEmit;

                    IfFailGo(pCommonImport->GetTypeSpecFromToken(
                        tkRidFrom, 
                        &pvTypeSpecSig, 
                        &cbTypeSpecSig));
                    
                                        // Translate the typespec signature before look up
                    IfFailGo(MergeUpdateTokenInFieldSig(
                        pMiniMdAssemEmit,           // The assembly emit scope.
                        pMiniMdAssemEmit_Import, 
                        pMiniMdEmit,                // The emit scope.
                        pMiniMdEmit_Import, 
                        pMiniMdEmit_AssemblyEmit, 
                        pCommonAssemImport,         // The assembly scope where the signature is from.
                        pCommonAssemImport_Import, 
                        pbHashValue,                // Hash value for the import assembly.
                        cbHashValue,                // Size in bytes for the hash value.
                        pCommonImport,              // The scope to merge into the emit scope.
                        pvTypeSpecSig,              // signature from the imported scope
                        &qkTypeSpecSigEmit,         // [OUT] buffer for translated signature
                        0,                          // start from first byte of TypeSpec signature
                        0,                          // don't care how many bytes are consumed
                        &cbTypeSpecEmit,            // [OUT] total number of bytes write to pqkSigEmit
                        pTypeForwardersResolver));

                    IfFailGo(pMiniMdEmit->GetTokenFromTypeSpec(
                        (PCCOR_SIGNATURE)(qkTypeSpecSigEmit.Ptr()), 
                        cbTypeSpecEmit, 
                        &tkRidTo));
                    _ASSERTE(TypeFromToken(tkRidTo) == mdtTypeSpec);
                }
                else
                {
                    _ASSERTE( TypeFromToken(tkRidFrom) == mdtBaseType );

                    // base type is unique across module
                    tkRidTo = tkRidFrom;
                }
            }

            // How many bytes the new rid will consume?
            cb1 = CorSigCompressToken(tkRidTo, &ulData);

            // ensure buffer is big enough
            IfFailGo(pqkSigEmit->ReSizeNoThrow(cbStartEmit + cbDestTotal + cb1));

            // store the new token
            cb2 = CorSigCompressToken(
                    tkRidTo,
                    (ULONG *)( ((BYTE *)pqkSigEmit->Ptr()) + cbStartEmit + cbDestTotal) );

            // inconsistency on CorSigCompressToken and CorSigUncompressToken
            _ASSERTE(cb1 == cb2);

            cbSrcTotal = cbSrcTotal + cb;
            cbDestTotal = cbDestTotal + cb1;

            if ( ulElementType == ELEMENT_TYPE_CMOD_REQD ||
                 ulElementType == ELEMENT_TYPE_CMOD_OPT)
            {
                // need to skip over the base type
                IfFailGo(MergeUpdateTokenInFieldSig(
                    pMiniMdAssemEmit,           // The assembly emit scope.
                    pMiniMdAssemEmit_Import, 
                    pMiniMdEmit,                // The emit scope.
                    pMiniMdEmit_Import, 
                    pMiniMdEmit_AssemblyEmit, 
                    pCommonAssemImport,         // The assembly scope where the signature is from.
                    pCommonAssemImport_Import, 
                    pbHashValue,                // Hash value for the import assembly.
                    cbHashValue,                // Size in bytes for the hash value.
                    pCommonImport,              // The scope to merge into the emit scope.
                    &pbSigImp[cbSrcTotal],      // signature from the imported scope
                    pqkSigEmit,                 // [OUT] buffer for translated signature
                    cbStartEmit + cbDestTotal,  // [IN] start point of buffer to write to
                    &cbImp,                     // [OUT] total number of bytes consumed from pbSigImp
                    &cbEmit,                    // [OUT] total number of bytes write to pqkSigEmit
                    pTypeForwardersResolver));
                cbSrcTotal += cbImp;
                cbDestTotal += cbEmit;
            }

            break;
        default:
            _ASSERTE(cbSrcTotal == cbDestTotal);
            
            if ((ulElementType >= ELEMENT_TYPE_MAX) || 
                (ulElementType == ELEMENT_TYPE_PTR) || 
                (ulElementType == ELEMENT_TYPE_BYREF))// || 
                //(ulElementType == ELEMENT_TYPE_VALUEARRAY_UNSUPPORTED))
            {
                IfFailGo(META_E_BAD_SIGNATURE);
            }
            break;
    }
    if (pcbImp)
        *pcbImp = cbSrcTotal;
    *pcbEmit = cbDestTotal;

ErrExit:
    DestroyNestedQuickArray(qaNesterNames);
    return hr;
} // MergeUpdateTokenInFieldSig

//*****************************************************************************
// translating signature from one scope to another scope
//*****************************************************************************
STDMETHODIMP MetaEmitHelper::TranslateSigWithScope(
    IMetaDataAssemblyImport * pAssemImport,     // [IN] importing assembly interface
    IMetaDataImport *         pAssemImport_Import, 
    const void *              pbHashValue,      // [IN] Hash Blob for Assembly.
    ULONG                     cbHashValue,      // [IN] Count of bytes.
    IMetaDataImport *         pImport,          // [IN] importing interface
    PCCOR_SIGNATURE           pbSigBlob,        // [IN] signature in the importing scope
    ULONG                     cbSigBlob,        // [IN] count of bytes of signature
    IMetaDataAssemblyEmit *   pAssemEmit,       // [IN] emit assembly interface
    IMetaDataImport *         pAssemEmit_Import, 
    IMetaDataEmit *           pEmit,            // [IN] emit interface
    IMetaDataImport *         pEmit_Import, 
    IMetaDataAssemblyEmit *   pEmit_AssemblyEmit, 
    TypeForwardersResolver *  pTypeForwardersResolver, // [IN] Functor for resolving type forwarders.
    PCOR_SIGNATURE            pvTranslatedSig,  // [OUT] buffer to hold translated signature
    ULONG                     cbTranslatedSigMax,
    ULONG *                   pcbTranslatedSig) // [OUT] count of bytes in the translated signature
{
    HRESULT     hr = S_OK;

    CQuickBytes qkSigEmit;
    ULONG       cbEmit;
    
    _ASSERTE(pvTranslatedSig && pcbTranslatedSig);

    IfFailGo(MergeUpdateTokenInSig(  // S_OK or error.
            pAssemEmit,     // The assembly emit scope.
            pAssemEmit_Import, 
            pEmit,          // The emit scope.
            pEmit_Import, 
            pEmit_AssemblyEmit, 
            pAssemImport,   // Assembly where the signature is from.
            pAssemImport_Import, 
            pbHashValue,    // Hash value for the import assembly.
            cbHashValue,    // Size in bytes.
            pImport,        // The scope where signature is from.
            pbSigBlob,      // signature from the imported scope
            &qkSigEmit,     // [OUT] translated signature
            0,              // start from first byte of the signature
            0,              // don't care how many bytes consumed
            &cbEmit,        // [OUT] total number of bytes write to pqkSigEmit
            pTypeForwardersResolver)); // [IN] Functor to resolve type forwarders
    memcpy(pvTranslatedSig, qkSigEmit.Ptr(), cbEmit > cbTranslatedSigMax ? cbTranslatedSigMax :cbEmit );
    *pcbTranslatedSig = cbEmit;
    if (cbEmit > cbTranslatedSigMax)
    {
        hr = CLDB_S_TRUNCATION;
    }
    
ErrExit:
    return hr;
} // MetaEmitHelper::TranslateSigWithScope

//****************************************************************************
// convert tokens contained in a COM+ signature
//****************************************************************************
HRESULT MergeUpdateTokenInSig(
    IMetaDataAssemblyEmit *   pMiniMdAssemEmit,     // [IN] The assembly emit scope.
    IMetaDataImport *         pMiniMdAssemEmit_Import, 
    IMetaDataEmit *           pMiniMdEmit,          // [IN] The emit scope.
    IMetaDataImport *         pMiniMdEmit_Import, 
    IMetaDataAssemblyEmit *   pMiniMdEmit_AssemblyEmit, 
    IMetaDataAssemblyImport * pCommonAssemImport,   // [IN] Assembly scope where the signature is from.
    IMetaDataImport *         pCommonAssemImport_Import, 
    const void  *             pbHashValue,          // [IN] Hash value for the import assembly.
    ULONG                     cbHashValue,          // [IN] Size in bytes for the hash value.
    IMetaDataImport *         pCommonImport,        // [IN] The scope to merge into the emit scope.
    PCCOR_SIGNATURE           pbSigImp,             // signature from the imported scope
    CQuickBytes *             pqkSigEmit,   // [OUT] translated signature
    ULONG                     cbStartEmit,  // [IN] start point of buffer to write to
    ULONG *                   pcbImp,       // [OUT] total number of bytes consumed from pbSigImp
    ULONG *                   pcbEmit,      // [OUT] total number of bytes write to pqkSigEmit
    TypeForwardersResolver *  pTypeForwardersResolver) // [IN] Functor to resolve type forwarders
{
    HRESULT     hr = NOERROR;           // A result.
    ULONG       cb;                     // count of bytes
    ULONG       cb1;
    ULONG       cbSrcTotal = 0;         // count of bytes consumed in the imported signature
    ULONG       cbDestTotal = 0;        // count of bytes for the new signature
    ULONG       cbEmit;                 // count of bytes consumed in the imported signature
    ULONG       cbImp;                  // count of bytes for the new signature
    ULONG       cArg = 0;               // count of arguments in the signature
    ULONG       cTyArg = 0;
    ULONG       callingconv = 0;        // calling convention from signature

    _ASSERTE(pcbEmit && pqkSigEmit && pbSigImp);

    // calling convention
    cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &callingconv);
    _ASSERTE((callingconv & IMAGE_CEE_CS_CALLCONV_MASK) < IMAGE_CEE_CS_CALLCONV_MAX);

    // skip over calling convention
    cbSrcTotal += cb;

    if (isCallConv(callingconv, IMAGE_CEE_CS_CALLCONV_FIELD))
    {
        // It is a FieldRef
        cb1 = CorSigCompressData(callingconv, ((BYTE *)pqkSigEmit->Ptr()) + cbStartEmit);

        // compression and uncompression better match
        _ASSERTE(cb == cb1);

        cbDestTotal = cbSrcTotal = cb;
        IfFailGo(MergeUpdateTokenInFieldSig(
            pMiniMdAssemEmit, 
            pMiniMdAssemEmit_Import, 
            pMiniMdEmit, 
            pMiniMdEmit_Import, 
            pMiniMdEmit_AssemblyEmit, 
            pCommonAssemImport, 
            pCommonAssemImport_Import, 
            pbHashValue,
            cbHashValue,
            pCommonImport,
            &pbSigImp[cbSrcTotal],
            pqkSigEmit,                     // output buffer to hold the new sig for the field
            cbStartEmit + cbDestTotal,      // number of bytes already in pqkSigDest
            &cbImp,                         // number of bytes consumed from imported signature
            &cbEmit,                        // number of bytes write to the new signature
            pTypeForwardersResolver));
        *pcbEmit = cbDestTotal + cbEmit;
    }
    else
    {

        // It is a MethodRef
        // count of type arguments
        if (callingconv & IMAGE_CEE_CS_CALLCONV_GENERIC)
        {
            cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &cTyArg);
            cbSrcTotal += cb;
        }

        // count of argument
        cb = CorSigUncompressData(&pbSigImp[cbSrcTotal], &cArg);
        cbSrcTotal += cb;

        // move over the calling convention and the count of arguments
        IfFailGo(pqkSigEmit->ReSizeNoThrow(cbStartEmit + cbSrcTotal));
        memcpy(((BYTE *)pqkSigEmit->Ptr()) + cbStartEmit, pbSigImp, cbSrcTotal);
        cbDestTotal = cbSrcTotal;

        if ( !( isCallConv(callingconv, IMAGE_CEE_CS_CALLCONV_LOCAL_SIG) || isCallConv(callingconv, IMAGE_CEE_CS_CALLCONV_GENERICINST)) )
        {
                // LocalVar sig does not have return type
                // process the return type
                IfFailGo(MergeUpdateTokenInFieldSig(
                    pMiniMdAssemEmit, 
                    pMiniMdAssemEmit_Import, 
                    pMiniMdEmit, 
                    pMiniMdEmit_Import, 
                    pMiniMdEmit_AssemblyEmit, 
                    pCommonAssemImport, 
                    pCommonAssemImport_Import, 
                    pbHashValue,
                    cbHashValue,
                    pCommonImport,
                    &pbSigImp[cbSrcTotal],
                    pqkSigEmit,                     // output buffer to hold the new sig for the field
                    cbStartEmit + cbDestTotal,      // number of bytes already in pqkSigDest
                    &cbImp,                         // number of bytes consumed from imported signature
                    &cbEmit,                        // number of bytes write to the new signature
                    pTypeForwardersResolver));

            // advance the count
            cbSrcTotal += cbImp;
            cbDestTotal += cbEmit;
        }


        while (cArg)
        {
            // process every argument
            IfFailGo(MergeUpdateTokenInFieldSig(
                pMiniMdAssemEmit, 
                pMiniMdAssemEmit_Import, 
                pMiniMdEmit, 
                pMiniMdEmit_Import, 
                pMiniMdEmit_AssemblyEmit, 
                pCommonAssemImport, 
                pCommonAssemImport_Import, 
                pbHashValue,
                cbHashValue,
                pCommonImport,
                &pbSigImp[cbSrcTotal],
                pqkSigEmit,                 // output buffer to hold the new sig for the field
                cbStartEmit + cbDestTotal,
                &cbImp,                     // number of bytes consumed from imported signature
                &cbEmit,                    // number of bytes write to the new signature
                pTypeForwardersResolver));
            cbSrcTotal += cbImp;
            cbDestTotal += cbEmit;
            cArg--;
        }

        // total of number of bytes consumed from imported signature
        if (pcbImp)
            *pcbImp = cbSrcTotal;

        // total number of bytes emitted by this function call to the emitting signature
        *pcbEmit = cbDestTotal;
    }

ErrExit:
    return hr;
} // MergeUpdateTokenInSig
