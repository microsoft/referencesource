#pragma once

#include <cor.h>
#include <corhlpr.h>

// Functor that calls into the compilers to resolve typeforwarders. MetaEmit layer does not understand typeforwarders and compilers do
// since they the list of assemblies being referenced for a compilation.
class TypeForwardersResolver
{
public:
    typedef void (*PfnTypeForwarderResolver)(void *pCompiler, void *pImportScope, mdTypeRef token, IMetaDataAssemblyImport **ppNewScope, const void ** ppbHashValue, ULONG *pcbHashValue);
    PfnTypeForwarderResolver m_pfnTypeForwarderResolver;
    void *m_pCompiler;
    void *m_pImportScope;

    TypeForwardersResolver(PfnTypeForwarderResolver pfnResolver, void *pCompiler, void *pImportScope)
    {
        m_pfnTypeForwarderResolver = pfnResolver;
        m_pCompiler = pCompiler;
        m_pImportScope = pImportScope;
    }

    void Resolve(mdTypeRef token, IMetaDataAssemblyImport **ppNewScope, const void ** ppbHashValue, ULONG *pcbHashValue)
    {
        m_pfnTypeForwarderResolver(m_pCompiler, m_pImportScope, token, ppNewScope, ppbHashValue, pcbHashValue);
    }
};

// ==============================================================================================================
// MetaEmitHelper class
// This class implements DefineImportMember which is a member of the MetaEmit interfaces. This class contains code taken
// from the CLR and teaches it about type forwarders.
class MetaEmitHelper
{
protected:
    IMetaDataEmit *         m_pEmit;
    IMetaDataImport *       m_pEmit_Import;
    IMetaDataAssemblyEmit * m_pEmit_AssemblyEmit;
    
public:
    
    MetaEmitHelper()
    {
        m_pEmit = NULL;
        m_pEmit_Import = NULL;
        m_pEmit_AssemblyEmit = NULL;
    }
    ~MetaEmitHelper()
    {
        if (m_pEmit_Import != NULL)
        {
            m_pEmit_Import->Release();
        }
        if (m_pEmit_AssemblyEmit != NULL)
        {
            m_pEmit_AssemblyEmit->Release();
        }
    }

    HRESULT Initialize(IMetaDataEmit * pEmit)
    {
        HRESULT hr;
        
        _ASSERTE(pEmit != NULL);
        
        _ASSERTE(m_pEmit == NULL);
        
        m_pEmit = pEmit;
        IfFailRet(pEmit->QueryInterface(IID_IMetaDataImport, (void **)&m_pEmit_Import));
        IfFailRet(pEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **)&m_pEmit_AssemblyEmit));

        return S_OK;
    }

public:
    HRESULT DefineImportMember(
        IMetaDataAssemblyImport *pAssemImport,  // [IN] Assemby containing the Member.
        const void  *pbHashValue,           // [IN] Hash Blob for Assembly.
        ULONG        cbHashValue,           // [IN] Count of bytes.
        IMetaDataImport *pImport,           // [IN] Import scope, with member.
        mdToken     mbMember,               // [IN] Member in import scope.
        IMetaDataAssemblyEmit *pAssemEmit,  // [IN] Assembly into which the Member is imported.
        mdToken     tkImport,               // [IN] Classref or classdef in emit scope.
        TypeForwardersResolver *pTypeForwarderResolver, // [IN] Functor for resolving type forwarders.
        mdMemberRef *pmr);                  // [OUT] Put member ref here.

    STDMETHODIMP TranslateSigWithScope(
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
        TypeForwardersResolver *  pTypeForwarderResolver, // [IN] Functor for resolving type forwarders.
        PCOR_SIGNATURE            pvTranslatedSig,  // [OUT] buffer to hold translated signature
        ULONG                     cbTranslatedSigMax,
        ULONG *                   pcbTranslatedSig);    // [OUT] count of bytes in the translated signature
};  // class MetaEmitHelper
