//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  PE builder - manages creating the final PE.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#define VERSION_BUFFER_SIZE 15 // a typical version is: v4.0.20324, so imagine V999.999.91225

//****************************************************************************
// Helpers
//****************************************************************************

typedef HRESULT(__stdcall *TCreateFileGen)(ICeeFileGen**);
typedef HRESULT(__stdcall *TDestroyFileGen)(ICeeFileGen**);

static TCreateFileGen s_CreateFileGen = NULL;
static TDestroyFileGen s_DestroyFileGen = NULL;

class EndEmitGuard : public GuardBase
{
public:
    EndEmitGuard(_In_ PEBuilder *pBuilder, _In_ PEInfo *pInfo) :
      m_pBuilder(pBuilder),
      m_pInfo(pInfo)
    {
    }

    ~EndEmitGuard()
    {
        RunAction();
    }

protected:
    virtual __override
    void DoAction()
    {
        m_pBuilder->EndEmit(m_pInfo);
    }

private:
    EndEmitGuard();
    EndEmitGuard(const EndEmitGuard&);

    PEBuilder *m_pBuilder;
    PEInfo *m_pInfo;
};


//============================================================================
// CodeGenInfo implementation
//============================================================================
CodeGenInfo::CodeGenInfo() : m_pBoundTree(NULL), m_pProc(NULL), m_pSourceFile(NULL)
{
}

CodeGenInfo::CodeGenInfo(const CodeGenInfo & src) : m_pBoundTree(src.m_pBoundTree), m_pProc(src.m_pProc), m_pSourceFile(src.m_pSourceFile)
{
}

CodeGenInfo::CodeGenInfo(ILTree::ILNode * pBoundTree, BCSYM_Proc * pProc, SourceFile * pSourceFile) : m_pBoundTree(pBoundTree), m_pProc(pProc), m_pSourceFile(pSourceFile)
{
}

//============================================================================
// Class that implements IMetaDataError, which we pass to IALink2::Init
// to get warning callbacks.
//============================================================================
class MetaDataError : public IMetaDataError
{
public:
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, _Out_ void **ppv);
    STDMETHOD_(ULONG, AddRef)() { return InterlockedIncrement((long *)&m_cRefs); }
    STDMETHOD_(ULONG, Release)() { return InterlockedDecrement((long *)&m_cRefs); }

    //
    // IMetaDataError method
    //
    STDMETHOD(OnError)(HRESULT hrError, mdToken token);

    //
    // MetaDataError class methods
    //
    MetaDataError(ErrorTable *pErrorTable, _In_z_ STRING *pstrAssemblyName)
    {
        m_cRefs = 0;
        m_pErrorTable = pErrorTable;
        m_pstrAssemblyName = pstrAssemblyName;
    }
    ~MetaDataError()
    {
        VSASSERT(m_cRefs == 0, "Unbalanced refcnt on IMetaDataError!");
    }

private:
    ULONG m_cRefs;
    ErrorTable *m_pErrorTable;
    STRING *m_pstrAssemblyName;
};

//============================================================================
// QI implementation for MetaDataError class.
//============================================================================
STDMETHODIMP MetaDataError::QueryInterface
(
    REFIID riid,
    _Out_ void **ppv
)
{
    void *pv = NULL;

    if (riid == IID_IUnknown)
    {
        pv = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_IMetaDataError)
    {
        pv = static_cast<IMetaDataError *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)pv)->AddRef();

    *ppv = pv;
    return NOERROR;
}


//============================================================================
// This is the real guts of the MetaDataError class.
//============================================================================
STDMETHODIMP MetaDataError::OnError
(
    HRESULT hrError,
    mdToken token
)
{
    // We only want ALink errors
    if (HRESULT_FACILITY(hrError) != FACILITY_ITF)
    {
        return S_FALSE;
    }

    // For now, we only want Warnings
    if (FAILED(hrError))
    {
        return S_FALSE;
    }

    if (m_pstrAssemblyName)
    {
        m_pErrorTable->CreateErrorWithError(WRNID_AssemblyGeneration1, NULL, hrError, m_pstrAssemblyName);
    }
    else
    {
        m_pErrorTable->CreateErrorWithError(WRNID_AssemblyGeneration0, NULL, hrError);
    }

    return S_OK;
}

//============================================================================
// Structure that holds the information we need to pass around to produce
// a PE.  This helps us avoid having to pass a large number of parameters
// to each function.
//============================================================================

struct PEInfo : ZeroInit<PEInfo>
{
    PEInfo(ErrorTable *pErrorTable, _In_z_ STRING *pstrAssemblyName)
        : m_metadataerror(pErrorTable, pstrAssemblyName) { }

    // Where to put the PE.
    ICeeFileGen *m_pFileGen;
    HCEEFILE m_hCeeFile;
    HCEESECTION m_hCeeSection;

    // ALink handles the gory details of building the manifest,
    // embedding/linking resources, creating Win32 resources, etc.
    IALink2 *m_pALink;

    // ALink3 is used for embedding UAC Manifest files
    IALink3 *m_pALink3;

    // MetaDataError implements IMetaDataError, which ALink uses
    // to inform us of warnings.
    MetaDataError m_metadataerror;

    // Token for the assembly or module under construction
    mdToken m_mdAssemblyOrModule;

    // When building a module, we sometimes need to remember the token
    // return by IALink2::AddFile
    mdToken m_mdFileForModule;

    // The entrypoint symbol
    BCSYM_Proc *m_pProcEntryPoint;

    // The Debugging Entry point token
    BCSYM_Proc *m_pProcDebuggingEntryPoint;

    // If not NULL, write the PE to this file.  Otherwise write it to memory.
    STRING *m_pstrFileName;

    // If not NULL, link this resource file into the PE.
    STRING *m_pstrResFileName;

    // Use a WCHAR buffer instead of a STRING* to avoid polluting
    // the StringPool with random filenames that are never
    // referenced again.  Saves 16-50KB per rebuild of Australian
    // Government Solution.  See Dev10 #830085
    WCHAR m_wszTempResFileName[MAX_PATH + 1];

    // If not NULL, this file specifies Win32 shell icon data.
    STRING *m_pstrIconFileName;

    // Does the current project have any errors?
    bool m_hasErrors;

    // What sort of compilation was requested
    PEBuilder::CompileType m_compileType;

#if DEBUG
    // Debug-only DTOR just checks that PEBuilder::EndEmit cleaned up after us
    ~PEInfo()
    {
        VSASSERT(m_pALink == NULL, "Should have been released in PEBuilder::EndEmit");
    }
#endif
};

//============================================================================
// Load the PE emitter helper DLL and create an instance of the helper.
//============================================================================

static void CreateCeeFileGen
(
    Compiler *pCompiler,
    ICeeFileGen **picfg
)
{
#ifndef FEATURE_CORESYSTEM
    if (!s_CreateFileGen)
    {
        HRESULT hr = NOERROR;
        HMODULE h = NULL;

        hr = LegacyActivationShim::LoadLibraryShim(L"mscorpe.dll", NULL, NULL, &h);

        if (h == NULL)
        {
            VSASSERT(FAILED(hr), "NULL handle returned without error code?");
            VSFAIL("Unable to load critical DLL 'mscorpe.dll'. It must be installed in the COM+ directory.");
            VbThrow(HrMakeReplWithError(ERRID_UnableToLoadDll1, hr, L"mscorpe.dll"));
        }

        s_CreateFileGen = (TCreateFileGen) GetProcAddress(h, "CreateICeeFileGen");
        if (!s_CreateFileGen)
        {
            VbThrow(HrMakeReplWithLast(ERRID_BadDllEntrypoint2, WIDE("CreateICeeFileGen"), L"mscorpe.dll"));
        }

        s_DestroyFileGen = (TDestroyFileGen) GetProcAddress(h, "DestroyICeeFileGen");
        if (!s_DestroyFileGen)
        {
            VbThrow(HrMakeReplWithLast(ERRID_BadDllEntrypoint2, WIDE("DestroyICeeFileGen"), L"mscorpe.dll"));
        }
    }

    IfFailThrow((*s_CreateFileGen)(picfg));
#endif
}

//============================================================================
// Destroy an instance of the helper.
//============================================================================

static void DestroyCeeFileGen
(
    ICeeFileGen **picfg
)
{
#ifndef FEATURE_CORESYSTEM
    VSASSERT(s_DestroyFileGen, "mismatched!");

    IfFailThrow((*s_DestroyFileGen)(picfg));

    *picfg = NULL;
#endif
}

#if DEBUG

//============================================================================
// Debug class to make sure that we never map tokens.
//============================================================================

class DebMapper : public IMapToken
{
public:

    DebMapper()
    {
        m_cRefs = 0;
    }

    STDMETHODIMP QueryInterface
    (
        REFIID riid,
        void **ppv
    )
    {
        void *pv = NULL;

        if (riid == IID_IUnknown)
        {
            pv = static_cast<IUnknown *>(this);
        }
        else if (riid == IID_IMapToken)
        {
            pv = static_cast<IMapToken *>(this);
        }
        else
        {
            return E_NOINTERFACE;
        }

        ((IUnknown *)pv)->AddRef();

        *ppv = pv;
        return NOERROR;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement((long *)&m_cRefs);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG cRefs = InterlockedDecrement((long *)&m_cRefs);

        if (cRefs == 0)
        {
            m_cRefs = 1;
            delete this;
        }

        return cRefs;
    }

    STDMETHODIMP Map(mdToken tkImp, mdToken tkEmit)
    {
            VSFAIL("Can't map!");

            return NOERROR;
    }

private:
        unsigned m_cRefs;
};


#endif DEBUG

//*********************************************************************
//
// This CSymMapToken class implemented the IMapToken. It is used to send the
// notifcation to SymbolWriter
//
//*********************************************************************
class CSymMapToken : public IMapToken
{
public:
    STDMETHODIMP QueryInterface
    (
        REFIID riid,
        _Out_ void **ppv
    )
    {
        void *pv = NULL;

        if (riid == IID_IUnknown)
        {
            pv = static_cast<IUnknown *>(this);
        }
        else if (riid == IID_IMapToken)
        {
            pv = static_cast<IMapToken *>(this);
        }
        else
        {
            return E_NOINTERFACE;
        }

        ((IUnknown *)pv)->AddRef();

        *ppv = pv;
        return NOERROR;
    }


    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement((long *)&m_cRefs);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG cRefs = InterlockedDecrement((long *)&m_cRefs);

        if (cRefs == 0)
        {
            m_cRefs = 1;
            delete this;
        }

        return cRefs;
    }

    STDMETHODIMP Map(mdToken tkFrom, mdToken tkTo)
    {
        HRESULT         hr = NOERROR;

        if (m_pTokenMappingTable)
        {
            mdToken type = TypeFromToken(tkFrom);

            // Token types shouldn't generally change in a remap, but it is sometimes OK
            // to map a reference token to a definition. This would happen if we generated a
            // method reference and then later emitted a definition for that method.
            // to do: check to make sure this is one of the NoPIA member tokens, since that's how
            // we expect to arrive in this situation.
            AssertIfFalse(type == TypeFromToken(tkTo) || 
                (type == mdtMemberRef && TypeFromToken(tkTo) == mdtMethodDef) ||
                (type == mdtMemberRef && TypeFromToken(tkTo) == mdtFieldDef));

            switch (type)
            {
                // These def types can be emitted out of order, so the remapped value needs
                // to be tracked for ENC
                case mdtMethodDef:
                case mdtFieldDef:
                    m_pTokenMappingTable->HashAdd(tkFrom, tkTo);
                    break;

                case mdtSignature:
                    VSASSERT( false, "should we be getting signature token remaps?" );
                    m_pTokenMappingTable->HashAdd(tkFrom, tkTo);
                    break;

                default:
                    break;
            }
        }

        return hr;
    }

    CSymMapToken::CSymMapToken
    (
        TokenMappingTable *pTokenMappingTable
    )
    {
        m_cRefs = 0;
        m_pTokenMappingTable = pTokenMappingTable;

    } // CSymMapToken::CSymMapToken()

private:

    unsigned m_cRefs;
    TokenMappingTable *m_pTokenMappingTable;

};

//============================================================================
// Builder class to have all common data and code from PEBuilder and ENCBuilder
//============================================================================
Builder::Builder
(
    Compiler *pCompiler,
    CompilerProject *pProject
)
    :m_pCompiler(pCompiler),
    m_pCompilerProject(pProject),
    m_nra(NORLSLOC),
    m_ProjectHashTable(&m_nra),
    m_pPDBForwardProcCache(NULL),
    m_pModulesCache(NULL)
#if IDE 
    , m_bProcessingTransients(false)
#endif IDE
{
}

void Builder::Destroy()
{
    ClearCaches();
    m_nra.FreeHeap();
}

bool
Builder::GetModulesInRing
(
    _In_    BCSYM_NamespaceRing *pNamespaceRing,
    _Out_   BCSYM_Class ***pppModules,
    _Out_   ULONG *pcModules
)
{
   if (m_pModulesCache)
    {
        ConsolidatedModulesNode *pConsolidatedModulesNode = NULL;

        if (m_pModulesCache->Insert(&pNamespaceRing, &pConsolidatedModulesNode))
        {
            ConsolidateModulesForRing(pNamespaceRing, m_pModulesCache->GetAllocator(), pConsolidatedModulesNode);
        }

        *pppModules = pConsolidatedModulesNode->Modules;
        *pcModules = pConsolidatedModulesNode->cModules;
        return pConsolidatedModulesNode->cModules > 0;
    }

    return 0;
}

void
Builder::ConsolidateModulesForRing
(
    _In_    BCSYM_NamespaceRing *pNamespaceRing,
    _Inout_ NorlsAllocator *pAllocator,
    _Inout_ ConsolidatedModulesNode *pConsolidatedModulesNode
)
{
    BCSYM_Namespace* pFirstNamespace = pNamespaceRing->GetFirstNamespace();
    BCSYM_Namespace* pNamespace = pFirstNamespace;

    CompilerHost *pCompilerHost = m_pCompilerProject->GetCompilerHost();
    ULONG cModules = 0;

    // Count the maximum number of Modules.
    do
    {
        // Only consider namespaces in the same compilerhost.
        if (pNamespace->GetFirstModule() &&
            pCompilerHost == pNamespace->GetCompilerFile()->GetCompilerHost())
        {
            cModules++;
        }

        pNamespace = pNamespace->GetNextNamespace();
    } while (pNamespace != pFirstNamespace);

    if (cModules)
    {
        // Allocate space.
        SafeInt<size_t> cbSize = SafeInt<size_t>(cModules) * sizeof(BCSYM_Class *);

        pConsolidatedModulesNode->Modules = (BCSYM_Class **) pAllocator->Alloc(cbSize.Value());

        // Fill the actual modules
        pNamespace = pFirstNamespace;
        cModules = 0;

        do
        {
            // Only consider namespaces in the same compilerhost.
            if (pNamespace->GetFirstModule() &&
                pCompilerHost == pNamespace->GetCompilerFile()->GetCompilerHost() &&
                m_pCompiler->NamespaceIsAvailable(m_pCompilerProject, pNamespace))
            {
                pConsolidatedModulesNode->Modules[cModules] = pNamespace->GetFirstModule();
                cModules++;
            }

            pNamespace = pNamespace->GetNextNamespace();
        } while (pNamespace != pFirstNamespace);

        pConsolidatedModulesNode->cModules = cModules;
    }
    else
    {
        pConsolidatedModulesNode->Modules = NULL;
        pConsolidatedModulesNode->cModules = 0;
    }
}

void Builder::ClearCaches()
{
    unsigned i;

    m_ProjectHashTable.Clear();
    m_NoPiaTypeDefToTypeRefMap.Clear();

    // clear out the table which caches tokens for the runtime members
    for (i = 1; i < (unsigned)MaxRuntimeMember; i++)
    {
        m_rgRTMemRef[i] = mdTokenNil;
    }

    // clear out the table which caches tokens for the runtime types
    for (i = 1; i < (unsigned)MaxRuntimeClass; i++)
    {
        m_rgRTTypRef[i] = mdTokenNil;
    }
}

#if IDE 

HRESULT Builder::GetSymbolHashKey(BCSYM_NamedRoot *pnamed, SymbolHash *pHash)
{
    AssertIfNull(pHash);
    VerifyOutPtr(pHash);

    if (pnamed)
    {
        int nValsToHash = 1;
        const BYTE *rgpbToBeHashed[2] = { NULL, NULL };
        DWORD rgcbToBeHashed[2] = { 0, 0 };

        StringBuffer sbHashKey;
        pnamed->GetQualifiedEmittedName(&sbHashKey);

        rgpbToBeHashed[0] = reinterpret_cast<const BYTE *>(sbHashKey.GetString());
        rgcbToBeHashed[0] = (sbHashKey.GetStringLength() * sizeof(WCHAR));

        // Note: sbMethod needs to be declared in this scope since its contents may
        //       be placed in rgpbToBeHashed, which is used in the ComputeCRC64 call
        StringBuffer sbMethod;

        if (pnamed->IsProc())
        {
            pnamed->GetBasicRep(m_pCompiler,NULL,&sbMethod);
            if (sbMethod.GetStringLength() > 0)
            {
                rgpbToBeHashed[1] = reinterpret_cast<const BYTE *>(sbMethod.GetString());
                rgcbToBeHashed[1] = (sbMethod.GetStringLength() * sizeof(WCHAR));
                nValsToHash++;
            }
        }
        else if (pnamed->IsStaticLocalBackingField() && pnamed->PStaticLocalBackingField()->GetProcDefiningStatic())
        {
            BCSYM_StaticLocalBackingField *pStaticLocalBackingField = pnamed->PStaticLocalBackingField();
            pStaticLocalBackingField->GetProcDefiningStatic()->GetBasicRep(m_pCompiler, NULL, &sbMethod);
            if (sbMethod.GetStringLength() > 0)
            {
                rgpbToBeHashed[1] = reinterpret_cast<const BYTE *>(sbMethod.GetString());
                rgcbToBeHashed[1] = (sbMethod.GetStringLength() * sizeof(WCHAR));
                nValsToHash++;
            }
        }

        *pHash = ComputeCRC64(rgpbToBeHashed, rgcbToBeHashed, nValsToHash);
    }

    return S_OK;
}

void Builder::CacheMetaToken(BCSYM_NamedRoot *pnamed, mdToken mdtk)
{
    if (pnamed && !IsNilToken(mdtk))
    {
        SourceFile *pSourceFile = pnamed->GetSourceFile();
        if (pSourceFile && pSourceFile->IsValidSourceFile())
        {
            TokenHashTable *pTokenHashTable = NULL;
            if (IsPEBuilder())
            {
                if (m_bProcessingTransients)
                {
                    pTokenHashTable = &(pSourceFile->GetTransientTokenHashTable());
                }
                else
                {
                    pTokenHashTable = &(pSourceFile->GetTokenHashTable());
                }
            }
            else if (IsENCBuilder())
            {
                pTokenHashTable = &(pSourceFile->GetENCTokenHashTable());
            }

            if (pTokenHashTable)
            {
                SymbolHash hash;
                if (SUCCEEDED(GetSymbolHashKey(pnamed, &hash)))
                {
                    mdToken existing;
                    if ( !MultimapUtil::TryGetFirstValue(*pTokenHashTable, hash, __ref existing) )
                    {
                        MultimapUtil::InsertPair( __ref *pTokenHashTable, hash, mdtk);
                    }
                    else if ( existing != mdtk )
                    {
                        pTokenHashTable->erase(hash);
                        MultimapUtil::InsertPair( __ref *pTokenHashTable, hash, mdtk);
                    }
                }
            }
        }
    }
}
#endif IDE

//============================================================================
// Returns true if we're building an assembly, false if building a module.
// 




bool Builder::FEmitAssembly()
{
    // Just defer to our CompilerProject
    return m_pCompilerProject->FEmitAssembly();
}

#if IDE
ENCBuilder *Builder::AsENCBuilder()
{
    ThrowIfFalse(IsENCBuilder());
    return static_cast<ENCBuilder *>(this);
}
#endif IDE

mdToken Builder::GetComClassToken(BCSYM_NamedRoot * pNamed)
{
    VSASSERT(pNamed,"Wrong Input.");

    mdToken tk = mdTokenNil;
    if (IsPEBuilder())
    {
        tk = pNamed->GetComClassToken();
    }
#if IDE
    else if (IsENCBuilder())
    {
        BCSYM_Container * pContainer = pNamed->GetContainer();
        VSASSERT(m_pSourceFile && pContainer,"Wrong State.");
        if (pContainer->GetSourceFile())
        {
            SymbolHash hash;
            if (SUCCEEDED(GetSymbolHashKey(pNamed, &hash)))
            {
                tk = pContainer->GetSourceFile()->GetToken(hash);
            }
        }
        if (IsNilToken(tk))
        {
            tk = pNamed->GetComClassToken();
        }
        VSASSERT(!IsNilToken(tk),"GetComClassToken : Wrong meta token.");
    }
#endif IDE
    else
    {
        VSASSERT(FALSE,"What????");
    }
    return tk;
}

mdToken Builder::GetTypeDef(BCSYM_Container * pContainer)
{
    VSASSERT(pContainer,"Wrong Input");

    mdToken tk = mdTokenNil;
    if (IsPEBuilder())
    {
        mdToken *cachedEmitToken = NULL;
        if (pContainer->GetContainingProject() != m_pCompilerProject)
        {
            cachedEmitToken = (mdToken*)m_ProjectHashTable.Find(pContainer);
        }
        if (cachedEmitToken)
        {
            tk = *cachedEmitToken;
            AssertIfFalse(mdtTypeDef == TypeFromToken(tk));
        }
        else
        {
            tk = (mdToken)pContainer->GetTypeDef();
        }
    }
#if IDE 
    else if (IsENCBuilder())
    {
        ENCBuilder *pENCBuilder = static_cast<ENCBuilder *>(this);

        if (pENCBuilder->LookupTransientToken(pContainer, &tk))
        {
            AssertIfTrue(IsNilToken(tk));
        }
        else if (pContainer->GetSourceFile())
        {
            SymbolHash hash;
            if (SUCCEEDED(GetSymbolHashKey(pContainer, &hash)))
            {
                tk = pContainer->GetSourceFile()->GetToken(hash);
            }
        }
        if (IsNilToken(tk))
        {
            tk = (mdToken)pContainer->GetTypeDef();
        }

        if (!IsNilToken(tk))
        {
            tk = pENCBuilder->MapToken(tk);
        }

        VSASSERT(!IsNilToken(tk),"GetTypeDef : Wrong meta token.");
    }
#endif IDE
    else
    {
        VSASSERT(FALSE,"What????");
    }
    return tk;
}

mdToken Builder::GetToken(BCSYM_NamedRoot * pNamed, bool remap)
{
    VSASSERT(pNamed,"Wrong Input");

    mdToken tk = mdTokenNil;
    if (IsPEBuilder())
    {
        mdToken *cachedEmitToken = NULL;
        if (pNamed->GetContainingProject() != m_pCompilerProject)
        {
            cachedEmitToken = (mdToken*)m_ProjectHashTable.Find(pNamed);
        }
        if (cachedEmitToken)
        {
            tk = *cachedEmitToken;
            AssertIfTrue(mdtTypeRef == TypeFromToken(tk) || mdtMemberRef == TypeFromToken(tk));
        }
        else
        {
            tk = (mdToken)pNamed->GetToken();
        }

        // see if this is a remapped token Bug #84987 - DevDiv Bugs
        if(!IsNilToken(tk) && IsProcedure(pNamed) && remap)
        {
            CompilerFile * fl = pNamed->GetContainingCompilerFile();

            if(fl && fl->IsSourceFile())
            {
                CompilerProject * proj = pNamed->GetContainingProject();

                AssertIfTrue(proj && proj->IsMetaData());

                if(proj) // && proj!=m_pCompilerProject)
                {
                    PEBuilder * bldr = proj->GetPEBuilder();

                    if(bldr && bldr->m_spTokenMappingTable)
                    {
                        mdToken *ptkMapped = bldr->m_spTokenMappingTable->HashFind(tk);

                        if(ptkMapped)
                        {
                            tk = *ptkMapped;
                        }
                    }
                }
            }
        }
    }
#if IDE 
    else if (IsENCBuilder())
    {
        ENCBuilder *pENCBuilder = static_cast<ENCBuilder *>(this);

        BCSYM_Container * pContainer = pNamed->GetPhysicalContainer();
        VSASSERT(pContainer,"Wrong State.");

        if (pContainer->GetSourceFile())
        {
            if (pENCBuilder->LookupTransientToken(pNamed, &tk))
            {
                AssertIfTrue(IsNilToken(tk));
            }
            else
            {
                SymbolHash hash;
                if (SUCCEEDED(GetSymbolHashKey(pNamed, &hash)))
                {
                    tk = pContainer->GetSourceFile()->GetToken(hash);
                    VSASSERT(!IsNilToken(tk),"GetToken : Wrong meta token.");
                }
            }
        }

        if (IsNilToken(tk))
        {
            pContainer = pNamed->GetContainer();
            if (pContainer->GetSourceFile())
            {
                SymbolHash hash;
                if (SUCCEEDED(GetSymbolHashKey(pNamed, &hash)))
                {
                    tk = pContainer->GetSourceFile()->GetToken(hash);
                    VSASSERT(!IsNilToken(tk),"GetToken : Wrong meta token again.");
                }
            }

            if (IsNilToken(tk))
            {
                tk = pNamed->GetToken();
            }
        }

        if (!IsNilToken(tk))
        {
            tk = pENCBuilder->MapToken(tk);
        }

        VSASSERT(!IsNilToken(tk),"GetToken : Wrong meta token.");
    }
#endif IDE
    else
    {
        VSASSERT(FALSE,"What????");
    }
    return tk;
}

/******************************************************************************
;GetToken

Retrieve the appropriate reference token for this parameter. If the parameter
comes from a method defined in the current project, as is normally the case,
then we can simply ask the parameter object for its own token. If the param
came from some other project, though, it is possible that we may have embedded
a copy of it into our output; in that case we will have generated a new output 
token and stored it in the project hash table (see Builder::SetToken). 
******************************************************************************/
mdToken Builder::GetToken(BCSYM_Param *pParam, BCSYM_Proc *pContext)
{
    AssertIfNull(pParam);
    AssertIfNull(pContext);
    mdToken tk = mdTokenNil;
    mdToken *cachedEmitToken = NULL;
    if (pContext->GetContainingProject() != m_pCompilerProject)
    {
        cachedEmitToken = (mdToken*)m_ProjectHashTable.Find(pParam);
    }
    if (cachedEmitToken)
    {
        tk = *cachedEmitToken;
        AssertIfFalse(mdtParamDef == TypeFromToken(tk));
    }
    else
    {
        tk = pParam->GetToken();
    }
    return tk;
}

mdToken Builder::GetComClassEventsToken(BCSYM_Class * pClass)
{
    VSASSERT(pClass,"Wrong Input");

    mdToken tk = mdTokenNil;
    if (IsPEBuilder())
    {
        tk = pClass->GetComClassEventsToken();
    }
#if IDE 
    else if (IsENCBuilder())
    {
        if (pClass->GetSourceFile())
        {
            SymbolHash hash;
            if (SUCCEEDED(GetSymbolHashKey(pClass, &hash)))
            {
                tk = pClass->GetSourceFile()->GetToken(hash);
            }
        }
        if (IsNilToken(tk))
        {
            tk = pClass->GetComClassEventsToken();
        }
        VSASSERT(!IsNilToken(tk),"GetComClassEventsToken : Wrong meta token.");
    }
#endif IDE
    else
    {
        VSASSERT(FALSE,"What????");
    }
    return tk;
}

/******************************************************************************
;SetToken

We create tokens for each symbol we emit. We must keep track of these tokens
so we can create references between metadata entities. Each symbol has a token
field of its own; when emitting symbols from the current project, we will
simply store a symbol's token there. 
When embedding types from some other project into our output, however, we must
not modify the original symbols; in that case we will store the output tokens
in the project hash table, using the symbol pointer as a key.
******************************************************************************/
void Builder::SetToken(BCSYM_NamedRoot *pNamed, mdToken tk)
{
    if (pNamed->GetContainingProject() == m_pCompilerProject)
    {
#if !IDE 
        AssertIfFalse(IsNilToken(pNamed->GetToken()));
#endif
        Symbols::SetToken(pNamed, tk);
    }
    else
    {
#if !IDE 
        // When mapping symbols to references, we use a key structure,
        // not simply the symbol pointer - see MetaEmit::DefineTypeRefByContainer
        // and MetaEmit::DefineMemberRefByName. Therefore, we expect that we will
        // not find the symbol pointer in the hash table.
        AssertIfFalse(NULL == m_ProjectHashTable.Find(pNamed));
#endif
        m_ProjectHashTable.Add(pNamed, tk);
        pNamed->SetAttributesEmitted(false);
    }
}

/******************************************************************************
;SetToken

We create tokens for each parameter we emit. If the parameter belongs to the
current project, we will simply store the token on the parameter symbol itself.
If we are embedding methods from some other project into our output, however,
we must not modify the original symbols: in that case we will store the output
tokens in the project hash table, using the parameter symbol pointer as key.
******************************************************************************/
void Builder::SetToken(BCSYM_Param *pParam, BCSYM_Proc *pContext, mdToken tk)
{
    AssertIfNull(pParam);
    AssertIfNull(pContext);
    if (pContext->GetContainingProject() == m_pCompilerProject)
    {
#if !IDE 
        AssertIfFalse(IsNilToken(pParam->GetToken()));
#endif
        Symbols::SetToken(pParam, tk);
    }
    else
    {
#if !IDE 
        AssertIfFalse(NULL == m_ProjectHashTable.Find(pParam));
#endif
        m_ProjectHashTable.Add(pParam, tk);
        pParam->SetAreAttributesEmitted(false);
    }
}

void Builder::WritePDBImportsList
(
    _In_ ImportedTarget *pImportedTarget,
    ISymUnmanagedWriter *pSymWriter
)
{
    // 

    for (;
         pImportedTarget;
         pImportedTarget = pImportedTarget->m_pNext)
    {
        if (pImportedTarget->m_pTarget && !pImportedTarget->m_pTarget->IsGenericBinding())
        {
            if (pImportedTarget->m_pstrAliasName || StringPool::StringLength(pImportedTarget->m_pstrQualifiedName))
            {
                // Encode imports using leading '@' symbol
                StringBuffer sbImport;
                sbImport.AppendChar(L'@');

                // 'P' for project level import, 'F' for file level import
                sbImport.AppendChar(pImportedTarget->m_IsProjectLevelImport ? L'P' : L'F');

                // 'X' for xml import, 'A' for aliased import, 'T' for type (as opposed to namespace)
                if (pImportedTarget->m_IsXml)
                {
                    sbImport.AppendChar(L'X');
                }
                else if (pImportedTarget->m_pstrAliasName)
                {
                    sbImport.AppendChar(L'A');
                }
                else if (!pImportedTarget->m_pTarget->IsNamespace())
                {
                    sbImport.AppendChar(L'T');
                }
                sbImport.AppendChar(L':');

                // Now encode the namespace (and alias, if one exists)
                if (pImportedTarget->m_pstrAliasName)
                {
                    sbImport.AppendSTRING(pImportedTarget->m_pstrAliasName);
                    sbImport.AppendChar(L'=');
                }
                sbImport.AppendSTRING(pImportedTarget->m_pstrQualifiedName);

                IfFailThrow(pSymWriter->UsingNamespace(sbImport.GetString()));
            }

        }
    }
}
    
void Builder::WriteImportsLists
(
    ISymUnmanagedWriter *pSymWriter,
    BCSYM_Proc *pProc,
    BCSYM_Container *pContainer
)
{
    //
    // Write out the namespaces used in the method
    //
    BCSYM_NamedRoot *psymParent;
    BCSYM_Namespace *psymInnerMostNSParent = NULL;

    // Walk the namespace parents and emit their Imports List
    for (psymParent = pProc->GetPhysicalContainer();
         psymParent;
         psymParent = psymParent->GetPhysicalContainer())
    {
        if (psymParent->IsNamespace())
        {
            if(!IsENCBuilder())
            {
                if(psymInnerMostNSParent == NULL)
                {
                    psymInnerMostNSParent = psymParent->PNamespace();
                }

                PDBForwardProcCacheNode *pForwardProcNode = NULL;
                if(m_pPDBForwardProcCache->Find(&psymInnerMostNSParent, &pForwardProcNode))
                {
                    StringBuffer sbTaggedQualifiedName;

                    WCHAR wszTokenValue[MaxStringLengthForIntToStringConversion];      // 64-bit int can only take a maximum of 20 chars

                    // Convert token to long
                    IfFailThrow(StringCchPrintfW(
                        wszTokenValue,
                        DIM(wszTokenValue),
                        L"%ld",
                        GetToken(pForwardProcNode->m_pForwardProc)));

                    sbTaggedQualifiedName.AppendChar(L'@');
                    sbTaggedQualifiedName.AppendString(wszTokenValue);
                    IfFailThrow(pSymWriter->UsingNamespace(sbTaggedQualifiedName.GetString()));
                    return;
                }
            }

            WritePDBImportsList(psymParent->PNamespace()->GetImports(),
                                pSymWriter);
            WriteNoPiaPdbList(pSymWriter);
        }
    }

    // Write the name of the default namespace (use '*' to distinguish it as the default namespace)
    if (pContainer->GetContainingProject()->GetDefaultNamespace())
    {
        IfFailThrow(pSymWriter->UsingNamespace(m_pCompiler->ConcatStrings(L"*", pContainer->GetContainingProject()->GetDefaultNamespace())));
    }

    STRING *pstrMyNamespace = NULL;
    if(CompareNoCase(pContainer->GetContainingProject()->GetDefaultNamespace(), L"") == 0)
    {
        pstrMyNamespace = STRING_CONST(m_pCompiler, My);
    }
    else
    {
        pstrMyNamespace = m_pCompiler->ConcatStrings(pContainer->GetContainingProject()->GetDefaultNamespace(), L".My");
    }

    // Write the project list	
    WritePDBImportsList(pContainer->GetContainingProject()->GetImportedTargets(),
                        pSymWriter);

    // Write the name of the current namespace

    IfFailThrow(pSymWriter->UsingNamespace(pContainer->GetNameSpace()));

    // This needs to be here so that the information above gets stored in the PDB.
    if(!IsENCBuilder())
    {
        PDBForwardProcCacheNode *pForwardProcNode = NULL;

        if (psymInnerMostNSParent != NULL &&
            m_pPDBForwardProcCache->Insert(&psymInnerMostNSParent, &pForwardProcNode))
        {
            pForwardProcNode->m_pForwardProc = pProc;
        }
    }
}

//
//  Write out the information needed for each scope
//
void Builder::WriteBlockScopes
(
    _In_ BlockScope * CurrentScope,        // Current Scope
    ISymUnmanagedWriter *pSymWriter,  // ISymWriter for PDB
    BCSYM_Proc *pProc,                // If we're at the procedure level this will be passed in
    BCSYM_Container *pContainer,       // Container
    mdSignature SignatureToken
)
{
    HRESULT hr = E_FAIL;
    CComPtr<ISymUnmanagedWriter2> spSymWriter2;

    // Ignore if this fails; we don't need a ISymUnmanagedWriter2.
    hr = pSymWriter->QueryInterface(IID_ISymUnmanagedWriter2, (void**)&spSymWriter2);

    if (CurrentScope->ScopeMembers.NumberOfEntries() > 0 || pProc)
    {
#if DEBUG
        // check to see if the scope offsets are valid if this scope has members
        if (CurrentScope->ScopeMembers.NumberOfEntries() > 0 && !pProc)
        {
            VSASSERT(CurrentScope->CloseOffset > 0 &&
                     CurrentScope->CloseOffset != CurrentScope->OpenOffset,
                     "WriteBlockScopes: dead scope with members!");
        }
        else if ((CurrentScope->OpenOffset == 0) && (CurrentScope->CloseOffset == 0))
        {
            VSASSERT(CurrentScope->ChildScopeList.NumberOfEntries() == 0,
                     "WriteBlockScopes: this scope can't have child scopes!");
        }
#endif

        unsigned int uiMethodScope;
        IfFailThrow(pSymWriter->OpenScope(CurrentScope->OpenOffset, &uiMethodScope));

        // If we're at the procedure level scope, we have to write out the imports for this scope
        if (pProc)
        {
            VSASSERT(pContainer, "Invalid state");
            WriteImportsLists(pSymWriter, pProc, pContainer);
        }

        CSingleListIter<ScopeMember> IterMembers(&CurrentScope->ScopeMembers);
        ScopeMember * CurrentMember;

        while (CurrentMember = IterMembers.Next())
        {
            if (CurrentMember->fConstant)
            {
                CComVariant VariantValue;
                hr = S_OK;

                CurrentMember->ConstantInfo->Value.VariantFromConstant(VariantValue);

                // This can fail with E_INVALIDARG for very long string constants. Bug
                // VSWhidbey 219279
                //
                // Okay to ignore failures because this is not catastrophic for compilation
                // or debugging. Worse case is that when debugging, these long local constants
                // will not be shown in watch windows, etc.

                // For constants, we use a signature per constant.

                if( spSymWriter2 != NULL && CurrentMember->ConstantInfo->SignatureToken != mdTokenNil )
                {
                    hr = spSymWriter2->DefineConstant2(
                            CurrentMember->ConstantInfo->VariableName,
                            VariantValue,
                            CurrentMember->ConstantInfo->SignatureToken);
                }
                else
                {
                    VSASSERT( false, "We shouldn't be getting to this code path." );
                    hr = pSymWriter->DefineConstant(
                            CurrentMember->ConstantInfo->VariableName,
                            VariantValue,
                            CurrentMember->ConstantInfo->SignatureSize,
                            CurrentMember->ConstantInfo->Signature);
                }

                if (FAILED(hr) && hr != E_INVALIDARG)
                {
                    VbThrow(hr);
                }
            }
            else
            {
                VSASSERT(CurrentMember->MemberInfo->IsActive, "Invalid state");

                if( spSymWriter2 != NULL && SignatureToken != mdTokenNil )
                {
                    IfFailThrow(
                        spSymWriter2->DefineLocalVariable2(
                            CurrentMember->MemberInfo->VariableName,
                            CurrentMember->MemberInfo->Flags,
                            SignatureToken,
                            ADDR_IL_OFFSET,
                            CurrentMember->MemberInfo->SlotNumber,
                            0,
                            0,
                            0,      // start offset (0 means entire scope)
                            0));    // end offset (0 means entire scope)
                }
                else
                {
                    VSASSERT( CurrentMember->MemberInfo->Signature != NULL, "Why is the signature NULL for the ENC scenario?" );
                    IfFailThrow(
                        pSymWriter->DefineLocalVariable(
                            CurrentMember->MemberInfo->VariableName,
                            CurrentMember->MemberInfo->Flags,
                            CurrentMember->MemberInfo->SignatureSize,
                            CurrentMember->MemberInfo->Signature,
                            ADDR_IL_OFFSET,
                            CurrentMember->MemberInfo->SlotNumber,
                            0,
                            0,
                            0,      // start offset (0 means entire scope)
                            0));    // end offset (0 means entire scope)
                }
            }
        }
    }

    if (CurrentScope->ChildScopeList.NumberOfEntries() > 0)
    {
        CSingleListIter<BlockScope> IterScopes(&CurrentScope->ChildScopeList);
        BlockScope * ChildScope;

        while (ChildScope = IterScopes.Next())
        {
            WriteBlockScopes(ChildScope, pSymWriter, NULL, NULL, SignatureToken);
        }
    }

    if (CurrentScope->ScopeMembers.NumberOfEntries() > 0 || pProc)
    {
        IfFailThrow(pSymWriter->CloseScope(CurrentScope->CloseOffset));
    }
}

//============================================================================
// Construct the instance, called wwhen the compiler project itself it
// constructed.
//============================================================================

//============================================================================
// Frees all resources.
//============================================================================

PEBuilder::~PEBuilder()
{
    Destroy();
}

//****************************************************************************
// Public implementation.
//****************************************************************************

//============================================================================
// Generate the type-defining metadata for the project.
//============================================================================

bool PEBuilder::EmitTypeMetadata
(
    ErrorTable *pErrorTableProject  // Project-level errors only
)
{
    m_pErrorTable = pErrorTableProject;
    bool fAborted = false;

    // We will often have the metadata hanging around in memory.
    if (m_State == BS_Empty)
    {
        LoadMetaData();
    }

    VSASSERT(m_pCompilerProject, "Don't use this entrypoint to emit expressions.");

    MetaEmit metaemitHelper(this, m_pCompilerProject, NULL /*Compilation Caches*/);

    // Emit AssemblyRefs to the default libraries
    // LookupProjectRef will add an AssemblyRef if one does not already exist.  It may have been
    // Added already during an incremental compile.
    metaemitHelper.LookupProjectRef(m_pCompilerProject->GetCompilerHost()->GetComPlusProject(), NULL, NULL);

    CompilerProject *pVBRuntimeProject = m_pCompilerProject->GetVBRuntimeProject();

    if (pVBRuntimeProject)
    {
        metaemitHelper.LookupProjectRef(pVBRuntimeProject, NULL, NULL);
    }

    // Loop through all of the files not currently compiled and
    // define all contained types.
    //
    AllEmitableTypesInFileInOrderIterator iter(m_pCompiler);
    BCSYM_Container *pContainer;

    bool hasErrors = m_pCompilerProject->HasErrors_AllowCaching();
    if (m_pCompilerProject->m_dlFiles[CS_Bound].GetFirst())
    {
        DynamicArray<SourceFile *> *pdaSourceFiles = m_pCompilerProject->GetFileCompilationArray();
        ULONG cFiles = pdaSourceFiles->Count();
        ULONG iFileIndex = 0;

        for (iFileIndex = 0; iFileIndex < cFiles; iFileIndex++)
        {
            SourceFile *pSourceFile = pdaSourceFiles->Element(iFileIndex);

            VSASSERT(pSourceFile->GetCompState() == CS_Bound, "File compilation state is invalid");

            // Increment this before we abort so that the decompilation stuff
            // will properly remove the information we've added.
            //
            m_cCompiledFiles++;

            if (!hasErrors && !pSourceFile->CompileAsNeeded())
            {
                SetSourceFile(pSourceFile);

                // Initialize the iterator that takes us over the classes.
                iter.Init(pSourceFile);

                // Emit each of the classes.
                while (pContainer = iter.Next())
                {
                    if (!pContainer->IsNamespace() && pContainer->GetBindingSpace())
                    {
                        // LATER AnthonyL 4/6/01: If DefineContainer can emit errors,
                        //  : we need to hijack PEBuilder.m_pErrorTable to point
                        //  : at a local error table, and then merge those errors
                        //  : into the SourceFile's error table.  See
                        //  : PEBuilder::Compile for an example of this.
                        //
                        // LATER AnthonyL 4/6/01: We need to revisit our error model.
                        //  : The way we propagate errors between ErrorTables is
                        //  : error prone.  And needing to manage the difference
                        //  : between SourceFile errors and CompilerProject errors
                        //  : is a pain.  And having intermediate objects like
                        //  : PEBuilder and MetaEmit contain (or point at) ErrorTables
                        //  : just makes this more confusing.
                        DefineContainer(pContainer);
                    }
                }
            }

            // We do this manually here.
            pSourceFile->_StepToEmitTypes();
        }

        VSASSERT(hasErrors || !m_pCompilerProject->HasErrors_AllowCaching(), "Where did the errors come from?");

        // Loop through all of the files again and emit the definitions of the
        // methods/variables.
        //
        for (iFileIndex = 0; iFileIndex < cFiles; iFileIndex++)
        {

    #if IDE
            // See if we should stop the compiler.
            if (CheckStop(NULL))
            {
                fAborted = true;
                goto Abort;
            }
    #endif IDE

            SourceFile *pSourceFile = pdaSourceFiles->Element(iFileIndex);

            VSASSERT(pSourceFile->GetCompState() == CS_Bound, "File compilation state is invalid");

            if (!hasErrors && !pSourceFile->CompileAsNeeded())
            {
                SetSourceFile(pSourceFile);

                // Initialize the iterator that takes us over the classes.
                iter.Init(pSourceFile);

                // Emit each of the classes.
                while (pContainer = iter.Next())
                {
                    if (!pContainer->IsBad() && !pContainer->IsNamespace())
                    {
                        DefineMembersOfContainer(pContainer, &metaemitHelper);
                    }
                }
            }

            // We do this manually here.
            pSourceFile->_StepToEmitTypeMembers();

    #if IDE 
            // Remember if we actually produced metadata for these methods.  This tells us
            // whether we have to regenerate this stuff when a compile error is fixed.
            //
            pSourceFile->SetHasGeneratedMetadata(!hasErrors);

    #endif
            m_pCompilerProject->m_dlFiles[CS_Bound].Remove(pSourceFile);
            m_pCompilerProject->m_dlFiles[CS_TypesEmitted].InsertLast(pSourceFile);

            pSourceFile->m_cs = CS_TypesEmitted;
    #if IDE 
            pSourceFile->SetDecompilationState(pSourceFile->m_cs);

            // Increment our count of compilation step changes
            m_pCompilerProject->GetCompilerHost()->IncrementCompilationStepCount();
    #endif
            FinishCompileStep(m_pCompiler);
        }
    }

#if IDE
Abort:
#endif

    VSASSERT(hasErrors || !m_pCompilerProject->HasErrors_AllowCaching(), "Where did the errors come from?");
    m_pErrorTable = NULL;

    return fAborted;
}


//============================================================================
// Compile everything in the project.  This routine does a lot of work
// that we'd normally do in CompilerProject::_PromoteToCompiled because
// we need to tightly control the order that this stuff is generated.
//============================================================================

bool PEBuilder::Compile
(
    CompileType compileType,
    _Out_ bool *pfGeneratedOutput,
    ErrorTable *pErrorTableProject,  // For project-level errors only
    BYTE **ppImage
)
{
#if !IDE
    VSASSERT(compileType != CompileType_DeferWritePE, "Can't use DeferWritePE in non-IDE compiler");
    VSASSERT(compileType != CompileType_FinishWritePE, "Can only use FinishWritePE in CompileToDisk()");
#else
    VSASSERT(m_pInfoDeferWritePE == NULL, "We should have already called EndEmit via CompileToDisk or HandleDemoteToTypesEmitted.");
#endif

    PEInfo *pInfo = new (zeromemory) PEInfo(pErrorTableProject, m_pCompilerProject->GetAssemblyName());
    EndEmitGuard guard(this, pInfo);

    BOOL   fAborted = false;
    VSASSERT(m_pCompilerProject, "Don't use this entrypoint to emit expressions.");

    m_pErrorTable = pErrorTableProject;

    AllContainersInFileInOrderIterator iter;
    CompilerFile *pFile = NULL;
    BCSYM_Container *pContainer;

    pInfo->m_hasErrors = m_pCompilerProject->HasErrors_AllowCaching();
    m_pCompilerProject->GetSecAttrSetsToEmit()->Clear();

    VSASSERT(m_SyntheticMethods.size() == 0, "where did these get created?");

    *pfGeneratedOutput = false;

    // Initialize transient symbols
    m_Transients.Init(m_pCompiler, &m_nra);

    // Just create ALink now.  We need one to help emit AssemblyRefs.
    CreateALinkForCompile(pInfo, compileType);

#if !IDE
    // The command-line compiler emits the method bodies as we are compiling
    // so it needs this immediately.  The IDE doesn't emit the bodies
    // until we write the PE so it can put off creating it.  This saves us
    // the bother of creating/destroying the PE writer information
    // until we know that we are definitely going to write one out.
    //
    BeginEmit(pInfo, compileType);
#endif !IDE

    // MetaEmit helper to write out to EXE
    MetaEmit metaemitHelper(this,
                            m_pCompilerProject,
                            pInfo->m_pALink,
                            pInfo->m_pALink3,
                            pInfo->m_mdAssemblyOrModule,
                            pInfo->m_mdFileForModule);

    // Determine entrypoint
    if (!m_pCompilerProject->OutputIsNone() &&
        !m_pCompilerProject->OutputIsLibrary() &&
        !m_pCompilerProject->OutputIsModule())
    {
        pInfo->m_pProcEntryPoint = DetermineEntrypoint(&pInfo->m_pProcDebuggingEntryPoint);
        pInfo->m_hasErrors |= m_pErrorTable->HasErrors();
    }

    // Loop through it all one last time and generate the IL for the method
    // bodies.
    //

#if IDE 
    // Give priority to the preferred file
    DynamicHashTable<CompilerFile *, bool> filesToPrefer;
    SourceFile * pPrefferedFile = OptimizeFileCompilationList(filesToPrefer);
#endif
    
    pFile = m_pCompilerProject->m_dlFiles[CS_TypesEmitted].GetFirst();
    while (pFile)
    {
        if (pFile->IsSourceFile() && !pFile->CompileAsNeeded())
        {
            SourceFile *pSourceFile = pFile->PSourceFile();
            Text TextInSourceFile;

            // Hijack the PEBuilder's ErrorTable to point at the ErrorTable
            // for this file.  This will let CompileMethodsOfContainer and
            // EmitAttributes report errors on the file that's being compiled
            m_pErrorTable = pSourceFile->GetCurrentErrorTable();

            SetSourceFile(pSourceFile);

            // Initialize the iterator that takes us over the classes.
            iter.Init(pSourceFile);
            pContainer = iter.Next();

            if (pContainer)
            {
                IfFailThrow(TextInSourceFile.Init(pSourceFile));
            }

            // VSW#545896
            // The lowest risk way to fix this bug is to ensure that ProcessAllXMLDocCommentNodes runs after
            // Text::Init, so that xml doc errors are not misdetected by Text::Init as previously existing
            // 'file load' errors.
            //
            // As the first step of compiling, process all the XML doccomments so they would be up-to-date.
#if IDE
            XMLParseTreeCache cache;
            if (pContainer)
            {
                cache.SetText(pSourceFile, &TextInSourceFile);
            }
#endif
            pSourceFile->GetXMLDocFile()->BindAllXMLDocCommentNodes();

            //Make sure XML Errors Get accounted before starting codegen
            if (!pInfo->m_hasErrors)
            {
                pInfo->m_hasErrors = pFile->HasActiveErrorsNoLock();
            }

#if IDE 
            // See if we should stop the compiler.
            if (CheckStop(NULL))
            {
                fAborted = true;
                goto Abort;
            }
#endif IDE
            if (pContainer)
            {
                // Emit each of the classes.
                do
                {
                    if (pContainer->IsClass())
                    {
                        // Emit code for contents of container.

                        // Line number tables and PE image buffers need to survive
                        // incremental rebuilds, so line number tables and images
                        // must be allocated with the same lifetimes as their
                        //associated symbols.

                        IfTrueAbort(CompileMethodsOfContainer(pContainer, &TextInSourceFile,
                                                            pInfo, &metaemitHelper));
                    }

#if IDE 
                    if (pSourceFile->GetHasGeneratedMetadata() && !m_pCompilerProject->OutputIsNone())
                    {
                        if (m_pCompilerProject->IsDebuggingInProgress())
                        {
                            m_pCompilerProject->m_fILEmitTasksSkippedDuringDebugging = true;
                        }
                        else
#endif
                        {

                            // Emit custom attributes attached anywhere inside this
                            // top-level container.  Note that we only do this if
                            // this SourceFile has had metadata completely
                            // generated for it.  Otherwise we will AV if we
                            // try to attach attributes to un-emitted/stale tokens.
                            IfTrueAbort(EmitAttributes(pContainer,
                                                    pInfo, &metaemitHelper));
                        }
#if IDE 
                    }
#endif
                } while (pContainer = iter.Next());
            }

            // Save the errors.
            pSourceFile->MergeCurrentErrors();

#if IDE 
            // Merge errors possibly added for the preferred file while processing partial types in the current file.
            // It is OK to do this now because, the preferred file itself was compiled first, so
            // we won't lose the errors we are about to merge.
            if (pPrefferedFile != NULL && pSourceFile != pPrefferedFile && 
                pPrefferedFile->HasPartialTypes() &&
                pSourceFile->HasPartialTypes() &&
                filesToPrefer.Contains(pSourceFile))
            {
                AssertIfFalse(m_pCompilerProject->m_dlFiles[CS_TypesEmitted].GetFirst() == pPrefferedFile);
                
                // don't delete existing errors with this step
                //
                pPrefferedFile->MergeCurrentErrors(false);
            }
#endif

#if IDE 
            // Remember if we actually produced code for these methods.  This tells us
            // whether we have to regenerate this stuff when a compile error is fixed.
            //
            pSourceFile->SetHasGeneratedCode(!pInfo->m_hasErrors);
#endif IDE
        }
        // Delay completing this step so that all its partial types are also processed
        // using the correct symbol allocator, error table, etc.
        //

        if (!pFile->IsSourceFile() ||
            !pFile->PSourceFile()->HasPartialTypes())
        {
            IfTrueAbort(CompleteCompilationTask(pFile));

            CompilerFile *pNextFile = pFile->Next();
            m_pCompilerProject->m_dlFiles[CS_TypesEmitted].Remove(pFile);
            m_pCompilerProject->m_dlFiles[CS_Compiled].InsertLast(pFile);
            pFile = pNextFile;
        }
        else
        {
            pFile = pFile->Next();
        }

    }

    // Complete the processing on all the delayed files containing partial types
    //
    for (pFile = m_pCompilerProject->m_dlFiles[CS_TypesEmitted].GetFirst();
         pFile;
         pFile = m_pCompilerProject->m_dlFiles[CS_TypesEmitted].GetFirst())
    {
        // 



        // Save the errors.

        VSASSERT( pFile->IsSourceFile(),
                        "Why is a metadata file being delay compiled ?");

        SourceFile *pSourceFile = pFile->PSourceFile();

        // don't delete existing errors with this step
        //
        pSourceFile->MergeCurrentErrors(false);

        IfTrueAbort(CompleteCompilationTask(pFile));
        m_pCompilerProject->m_dlFiles[CS_TypesEmitted].Remove(pFile);
        m_pCompilerProject->m_dlFiles[CS_Compiled].InsertLast(pFile);
    }

    // We're done processing the CompilerFiles, so stop hijacking
    // the PEBuilder's ErrorTable and point it back at the ErrorTable
    // for the project. 
    m_pErrorTable = pErrorTableProject;

    // We're done processing the project, so embed local copies of any No-PIA types we encountered.
    // However, do not attempt this if there are errors. Pia types might refer to "bad" symbols in that case,
    // code gen would die. 
    if (!pInfo->m_hasErrors)
    {
        DefinePiaTypes(&metaemitHelper);
    }

#if IDE 
    // The IDE did not create this above.
    BeginEmit(pInfo, compileType);
#endif IDE

    //======================================================================
    // Emit the image.
    //======================================================================

    if (!pInfo->m_hasErrors && !m_pCompilerProject->OutputIsNone())
    {
#if IDE 
        if (m_pCompilerProject->IsDebuggingInProgress())
        {
            m_pCompilerProject->m_fILEmitTasksSkippedDuringDebugging = true;

            // Fix Dev10 #733519 by emitting an updated assembly manifest when compiling during EnC

            VSASSERT(metaemitHelper.VerifyALinkAndTokens(pInfo->m_pALink, pInfo->m_mdAssemblyOrModule, pInfo->m_mdFileForModule),
                "MetaEmit helper and PEInfo have different ALink!");

            // (IDE compiler currently supports only assemblies)
            VSASSERT(!m_pCompilerProject->IsCodeModule(), "Emitting manifest not valid for modules!");

            // Set all of the assembly identity properties
            metaemitHelper.ALinkSetAssemblyProps();

            // Creates the assembly's manifest.
            metaemitHelper.ALinkEmitManifest(pInfo->m_pFileGen, pInfo->m_hCeeFile, pInfo->m_hCeeSection);
        }
        else
#endif IDE
        {
            pInfo->m_pstrResFileName = m_pCompilerProject->m_pstrWin32ResourceFile;
            pInfo->m_pstrIconFileName = m_pCompilerProject->m_pstrWin32IconFile;

            // Write out the final PE.
            // The non-IDE compiler and in-memory compilation will write the full PE now.
            // The normal IDE compiler will call WritePE twice -- the 1st half is to emit
            // necessary assembly metadata into IMetaDataEmit, the 2nd half is to emit
            // the PDB and the DLL. The 2nd half is called later in response to a build
            // from the project system, via CompileToDisk().
            WritePE(pInfo, ppImage, pfGeneratedOutput, &metaemitHelper);

            // If we were doing the 1st half of CompileType_DeferWritePE,
            // move the PEInfo to the next state so it will be completed
            // by CompileToDisk.
            if (pInfo->m_compileType == CompileType_DeferWritePE)
            {
                pInfo->m_compileType = CompileType_FinishWritePE;
                guard.SetIsEnabled(false);
            }

            // Errors or warnings might be causes when writing out the PE. Eg: ALink
            // might return duplicate assembly level attribute errors, etc.
            // So we need to merge those errors if any into the source files'
            // error table so that the errors show up properly linked to the
            // appropriate source fles in the task list.
            // This fixed RTM and Everett bugs where these errors had no location or
            // sourcefile
            //
            for(pFile = m_pCompilerProject->m_dlFiles[CS_Compiled].GetFirst();
                pFile;
                pFile = pFile->Next())
            {
                if (!pFile->IsSourceFile() || pFile->CompileAsNeeded())
                {
                    continue;
                }

                SourceFile *pSourceFile = pFile->PSourceFile();

                ErrorTable *pErrorsGeneratedDuringPEEmit =
                    pSourceFile->GetCurrentErrorTable();

                if (pErrorsGeneratedDuringPEEmit->HasErrors() ||
                    pErrorsGeneratedDuringPEEmit->HasWarnings())
                {
                    /* don't delete existing errors with this step */
                    pSourceFile->MergeCurrentErrors(false, CS_GeneratedCode);
                }
            }
        }
    }

Abort:
    m_pErrorTable = NULL;

    return fAborted;
}


#if IDE 

SourceFile * PEBuilder::OptimizeFileCompilationList(DynamicHashTable<CompilerFile *, bool> & filesToPrefer)
{
    // Give priority to the preferred file
    SourceFile * pPrefferedFile = m_pCompilerProject->GetCompilerHost()->GetPreferredSourceFile();
    
    if (pPrefferedFile != NULL)
    {
        if (pPrefferedFile->GetProject() == m_pCompilerProject)
        {
            // Put this file and other files containing other parts of partial containers in front of the compilation list

            filesToPrefer.SetValue(pPrefferedFile, false);

            // Examine partial containers in this file and grab files containing their main types
            if (pPrefferedFile->HasPartialTypes())
            {
                AllContainersInFileInOrderIterator containers(pPrefferedFile, true);
                BCSYM_Container *pContainer;

                while( pContainer = containers.Next() )
                {
                    BCSYM_Container * pMainType = pContainer->IsPartialTypeAndHasMainType();

                    if (pMainType != NULL)
                    {
                        CompilerFile * pMainTypeFile = pMainType->GetCompilerFile();

                        if (pMainTypeFile != NULL && pMainTypeFile != pPrefferedFile)
                        {
                            filesToPrefer.SetValue(pMainTypeFile, false);
                        }
                    }
                }
            }

            // Let's play safe and mark those files found in CS_TypesEmitted list
            CompilerFile *pFile = m_pCompilerProject->m_dlFiles[CS_TypesEmitted].GetFirst();

            while (pFile)
            {
                if (filesToPrefer.Contains(pFile))
                {
                    filesToPrefer.SetValue(pFile, true);
                }

                pFile = pFile->Next();
            }

            // Let's move marked files now
            HashTableIterator<CompilerFile *, bool, VBAllocWrapper> filesToMoveIterator = filesToPrefer.GetIterator();
            bool movePrefferedFile = false;

            while (filesToMoveIterator.MoveNext())
            {
                KeyValuePair<CompilerFile *, bool> current = filesToMoveIterator.Current();
                
                if (current.Value())
                {
                    pFile = current.Key();
                    
                    if (pFile == pPrefferedFile)
                    {
                        movePrefferedFile = true;
                    }
                    else
                    {
                        m_pCompilerProject->m_dlFiles[CS_TypesEmitted].Remove(pFile);
                        m_pCompilerProject->m_dlFiles[CS_TypesEmitted].InsertFirst(pFile);
                    }
                }
            }

            if (movePrefferedFile)
            {
                // Putting this file as the very first makes sure that it is OK to 
                // merge errors for partial types reported while compiling other preferred files. 
                // We won't lose the errors
                m_pCompilerProject->m_dlFiles[CS_TypesEmitted].Remove(pPrefferedFile);
                m_pCompilerProject->m_dlFiles[CS_TypesEmitted].InsertFirst(pPrefferedFile);
            }
        }
    }

    return pPrefferedFile;
}
#endif



bool PEBuilder::CompleteCompilationTask(CompilerFile *pFile)
{
    bool fAborted = false;

    // We do this manually here.
    IfTrueAbort(pFile->_StepToEmitMethodBodies());

    // Mark this module as being in compiled state.
    VSASSERT(pFile->m_cs == CS_TypesEmitted, "State changed unexpectedly.");
    pFile->m_cs = CS_Compiled;
#if IDE 
    pFile->SetDecompilationState(pFile->m_cs);

    // Increment our count of compilation step changes
    m_pCompilerProject->GetCompilerHost()->IncrementCompilationStepCount();
#endif

    VSDEBUGPRINTIF(VSFSWITCH(fDumpStateChanges), "%S promoted to CS_Compiled.\n", pFile->m_pstrFileName);

    FinishCompileStep(m_pCompiler);

Abort:
    return fAborted;
}

#if IDE 
void PEBuilder::CompileToDisk(bool *pfGeneratedOutput, ErrorTable* pErrorTableProject)
{
    bool fGeneratedOutput = false;
    HRESULT hrCaught = NOERROR;

    if (NeedsCompileToDisk())
    {
        VSASSERT(!m_pInfoDeferWritePE->m_hasErrors, "m_pInfoDeferWritePE should be null if there were errors");
        VSASSERT(m_pInfoDeferWritePE->m_pALink != NULL, "BeginEmit has not been called!");

        {
            EndEmitGuard emitGuard(this, m_pInfoDeferWritePE);
            ClearPointerGuard<ErrorTable> clearPointerGuard(m_pErrorTable);

            // We need an error table
            m_pErrorTable = pErrorTableProject;

            // MetaEmit helper to write out to EXE
            MetaEmit metaemitHelper(this,
                                    m_pCompilerProject,
                                    m_pInfoDeferWritePE->m_pALink,
                                    m_pInfoDeferWritePE->m_pALink3,
                                    m_pInfoDeferWritePE->m_mdAssemblyOrModule,
                                    m_pInfoDeferWritePE->m_mdFileForModule);
            
            // Write out the final PE.
            WritePE(m_pInfoDeferWritePE, NULL, &fGeneratedOutput, &metaemitHelper);
        }

        // Since the assembly was just written to disk, set this flag to true.
        m_IsImageOnDiskUpToDate = true;
    }
    else
    {
        return; // Do nothing if no need to compile to disk.
    }

    *pfGeneratedOutput = fGeneratedOutput;
}

// CompileToDiskAsync
// Called from a CompilerProject during the final stage of a Solution build for this project.
bool PEBuilder::CompileToDiskAsync()
{
    if (!NeedsCompileToDisk())
    {
        return false; // Do nothing if no need to compile to disk.
    }

    VSASSERT(!m_pInfoDeferWritePE->m_hasErrors, "m_pInfoDeferWritePE should be null if there were errors");
    VSASSERT(m_pInfoDeferWritePE->m_pALink != NULL, "BeginEmit has not been called!");

    // Create WritePEHelper to coordinate the work to schedule WritePE on a worker thread.
    CComPtr<WritePEHelper> spWritePEHelper = ComUtil::CreateWithRef<WritePEHelper>();
    spWritePEHelper->Init(
        this, 
        m_pCompilerProject, 
        m_pInfoDeferWritePE->m_pALink,
        m_pInfoDeferWritePE->m_pALink3,
        m_pInfoDeferWritePE->m_mdAssemblyOrModule,
        m_pInfoDeferWritePE->m_mdFileForModule);

    return SUCCEEDED(spWritePEHelper->WritePEAsync());
}

bool PEBuilder::NeedsCompileToDisk() const
{
    if (!m_IsImageOnDiskUpToDate && m_pInfoDeferWritePE != NULL)
    {
        return m_pInfoDeferWritePE->m_compileType == CompileType_FinishWritePE;
    }

    return false;
}

void PEBuilder::HandleDemoteToTypesEmitted()
{
    EndEmit(m_pInfoDeferWritePE);

    m_spTokenMappingTable = nullptr;
    m_SyntheticMethods.clear();
}

#endif

//============================================================================
// Removes the metadata created for this file from MetaEmit.
//============================================================================
#if IDE 
void PEBuilder::RemoveFileMetadata
(
    SourceFile *pSourceFile
)
{
    VSASSERT(pSourceFile->m_cs == CS_Bound, "Removing metadata for a file not in Bound state.");

    // Remember that the file is decompiled.
    VSASSERT(m_cCompiledFiles > 0, "Underflow.");
    m_cCompiledFiles--;

    pSourceFile->GetTokenHashTable().clear();

    // If we have no compiled files and we're not doing edit and continue
    // then destroy our state.  Any decompilations after an initial
    // compile will decompile as well.
    //
    if (m_cCompiledFiles == 0)
    {
        Destroy();
    }
}
#endif  // IDE

//****************************************************************************
// Private implementation.
//****************************************************************************

//============================================================================
// Helper to calculate a container's flags.
//============================================================================
DWORD PEBuilder::GetTypeDefFlags
(
    BCSYM_Container *pContainer
)
{
    DWORD dwTypeDefFlags = 0;

    if (pContainer->GetPWellKnownAttrVals()->GetComImportData())
    {
        dwTypeDefFlags |= tdImport;
    }
    if (TypeHelpers::IsEmbeddableInteropType(pContainer))
    {
        if (pContainer->GetPWellKnownAttrVals()->GetSerializableData())
        {
            dwTypeDefFlags |= tdSerializable;
        }
    }

    // Gather the information.
    switch(pContainer->GetKind())
    {
    case SYM_Class:
        {
            BCSYM_Class *pclass = pContainer->PClass();

            if (pclass->IsEnum())
            {
                dwTypeDefFlags |= tdSealed;
            }
            else if (pclass->IsStruct())
            {
                dwTypeDefFlags |= tdSealed;
                // The user may specify a struct layout using a pseudo-custom attribute.
                // If the user did not specify a custom struct layout, use this symbol's default layout.

                __int16 layoutFlags;

                if (pclass->GetPWellKnownAttrVals()->GetStructLayoutData(&layoutFlags))
                {
                    AssertIfFalse((layoutFlags & (~tdLayoutMask)) == 0);
                    if (layoutFlags != tdLayoutMask) // Test whether layoutFlags is a valid LayoutKind value
                    {
                        dwTypeDefFlags |= (layoutFlags & tdLayoutMask);
                    }
                    else 
                    {
                        dwTypeDefFlags |= tdSequentialLayout;
                    }                      
                }
                else if (!pclass->IsStateMachineClass()) // The async state machine should default to LayoutKind.Auto (Dev11 244076)
                {
                    dwTypeDefFlags |= tdSequentialLayout;
                }

                // Dev10 #702321
                CorTypeAttr charSet = (CorTypeAttr)0;

                if (pclass->GetPWellKnownAttrVals()->GetCharSet(charSet))
                {
                    dwTypeDefFlags |= charSet;
                }
            }
            else
            {
                // flags
                if (pclass->IsNotInheritable() || pclass->IsStdModule())
                {
                    dwTypeDefFlags |= tdSealed;
                }

                if ( !pclass->AreMustOverridesSatisfied() || pclass->IsMustInherit())
                {
                    dwTypeDefFlags |= tdAbstract;
                }
            }

            BCSYM_Proc *SharedConstructor = pclass->GetSharedConstructor(m_pCompiler);

            // If the .cctor is synthetic, then we're only initializing fields,
            // so it's ok not to run it until the first shared field is accessed
            if (SharedConstructor && SharedConstructor->IsSyntheticMethod())
            {
                // Dev10 #672979: Check if it is safe to apply the tdBeforeFieldInit flag.
                // This code is almost a clone of the relevant code in Semantics::InitializeFields.
                bool apply = true;
                
                BCITER_CHILD MemberIterator(pclass);

                for (Declaration *Member = MemberIterator.GetNext();
                     Member && apply;
                     Member = MemberIterator.GetNext())
                {
                    if (IsProcedureDefinition(Member))
                    {
                        // Do we need to generate AddHandler for this method in this constructor?
                        if( !ViewAsProcedure(Member)->IsPartialMethodDeclaration() && ViewAsProcedureDefinition(Member)->IsShared())
                        {
                            BCITER_Handles iterHandles(ViewAsProcedureDefinition(Member));
                            for (HandlesList *Handles = iterHandles.GetNext();
                                 Handles;
                                 Handles = iterHandles.GetNext())
                            {
                                if (!Handles->IsBadHandlesList() && (Handles->IsMyBase() || Handles->IsEventFromMeOrMyClass()) &&
                                    Handles->GetEvent()->IsShared())    //Shared Method Handling Shared MyBase.Event should be added in the shared constructor
                                {
                                    apply = false;
                                    break;
                                }
                            }
                        }
                    }
                }


                if (apply)
                {
                    dwTypeDefFlags |= tdBeforeFieldInit;
                }
            }
        }
        break;

    case SYM_Interface:
        {
            // flags
            dwTypeDefFlags |= tdInterface | tdAbstract;
            if (TypeHelpers::IsEmbeddableInteropType(pContainer))
            {
                dwTypeDefFlags |= tdImport;
            }
        }
        break;

    default:
        VSFAIL("What other kinds of containers do we have?");
    }

    // Shared flags.
    if (pContainer->GetContainer()->IsNamespace())
    {
        // Uses tdXXXX
        // Set the access flags.
        switch(pContainer->GetAccess())
        {
        case ACCESS_Public:
            dwTypeDefFlags |= tdPublic;
            break;

        case ACCESS_Protected:
            VSFAIL("Can't have a non-nested protected type.");
            dwTypeDefFlags |= tdPublic;
            break;

        case ACCESS_Private:
            dwTypeDefFlags |= tdNotPublic;
            break;

        case ACCESS_Friend:
            dwTypeDefFlags |= tdNotPublic;
            break;

        case ACCESS_ProtectedFriend:
            VSFAIL("Can't have a non-nested protected type.");
            dwTypeDefFlags |= tdPublic;
            break;

        default:
            VSFAIL("Unexpected access.");
        }
    }
    else
    {
        // Uses tdXXXX
        // Set the access flags.
        switch(pContainer->GetAccess())
        {
        case ACCESS_Public:
            dwTypeDefFlags |= tdNestedPublic;
            break;

        case ACCESS_Protected:
            dwTypeDefFlags |= tdNestedFamily;
            break;

        case ACCESS_Private:
            dwTypeDefFlags |= tdNestedPrivate;
            break;

        case ACCESS_Friend:
            dwTypeDefFlags |= tdNestedAssembly;
            break;

        case ACCESS_ProtectedFriend:
            dwTypeDefFlags |= tdNestedFamORAssem;
            break;

        default:
            VSFAIL("Unexpected access.");
        }
    }

    return dwTypeDefFlags;
}

//============================================================================
// Set up the PE Builder and prepares it for compiling.  This must be
// called each time we go to produce an EXE.
//============================================================================

void PEBuilder::LoadMetaData()
{
    VSASSERT(!m_pmdEmit, "We already have metadata.");

    // We will often have the metadata hanging around in memory.
    HRESULT hr = NOERROR;
    CComPtr<IMetaDataDispenserEx> pMetaDataDispenser = NULL;
    VARIANT varDuplicates, varRefToDef, varENC, varEmitOrder;

    varDuplicates.vt = varRefToDef.vt = varENC.vt = varEmitOrder.vt = VT_UI4;

    // Common values.
    varRefToDef.lVal = MDRefToDefNone;

    // Get the metadata dispenser, we will use this to get to all of the
    // metadata apis.
    //
    IfFailThrow(m_pCompilerProject->GetCompilerHost()->GetDispenser(&pMetaDataDispenser));

        // Microsoft Set the metadataRuntimeVersion of the output binary to be the
        // same as the value retrieved from mscorlib. This enables the user to configure
        // the runtime they want to target by changing the mscorlib they supply.
        CompilerProject *pComPlusProject = m_pCompilerProject->GetCompilerHost()->GetComPlusProject();

        {
            CComPtr<ICLRMetaHost> metaHost;
            IfFailThrow(PEBuilder::CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&metaHost));
            {
                WCHAR szFull[VERSION_BUFFER_SIZE];
                DWORD bufRet = VERSION_BUFFER_SIZE;

                IfFailThrow(metaHost->GetVersionFromFile(pComPlusProject->GetFileName(), szFull, &bufRet));
                {
                    VARIANT v;
                    V_VT(&v) = VT_BSTR;
                    V_BSTR(&v) = SysAllocString(szFull);
                    pMetaDataDispenser->SetOption(MetaDataRuntimeVersion, &v);
                    SysFreeString(V_BSTR(&v));
                }
            }
        }

        // Note:  defs emitted out of order may be remapped.  For ENC token caching,
        // CSymMapToken::Map handles the remapping and should match what these flags are

        // MDFieldOutOfOrder is added to fix bug724659. For some scenario(suite vbc\EnumEmitOrder), between emitting
        // container and its member there could be some other container in between, which violates the second rule in 
        // 3.9 of "Metadata Unmanaged API". 
        varEmitOrder.lVal = (MDErrorOutOfOrderAll & ~(MDMethodOutOfOrder)) & ~(MDFieldOutOfOrder);


    // Since we now emit ModuleRefs and AssemblyRefs in two code paths
    // (ours and ALink's) we need to set MDDupModuleRef and MDDupAssemblyRef.
    // And since we now emit the assembly manifest going to CS_Compiled (which
    // can occur many times), we also need to set MDDupAssembly.
    // 




    varDuplicates.lVal = MDDupModuleRef | MDDupAssemblyRef | MDDupAssembly | MDDupMethodDef | MDDupTypeRef | MDDupMemberRef;
    varENC.lVal = MDUpdateFull;

    // Set the switches.
    IfFailThrow(pMetaDataDispenser->SetOption(MetaDataCheckDuplicatesFor, &varDuplicates));
    IfFailThrow(pMetaDataDispenser->SetOption(MetaDataRefToDefCheck, &varRefToDef));
    IfFailThrow(pMetaDataDispenser->SetOption(MetaDataSetENC, &varENC));
    IfFailThrow(pMetaDataDispenser->SetOption(MetaDataErrorIfEmitOutOfOrder, &varEmitOrder));

    IfFailThrow(pMetaDataDispenser->DefineScope(CLSID_CorMetaDataRuntime,
                                                0,
                                                IID_IMetaDataEmit2,
                                                (IUnknown **)&m_pmdEmit));

    m_State = BS_IncrementalCompile;

    IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataAssemblyImport,
                                          (void **)&m_pmdAssemblyImport));
}


//============================================================================
// Loads alink.dll and creates an IALink2 interface
//
// This is really just a delay-load wrapper around alink.dll's CreateALink.
//
// Since failing to create our ALink interface is a fatal error, we throw
// instead of trying to report an error through an ErrorTable or
// an HRESULT return value.
//============================================================================
/*static*/
void PEBuilder::CreateALink
(
    IALink2 **ppIALink
)
{
    IALink3 *ptemp = NULL;
    CreateALink(ppIALink, &ptemp);

    // We're not using IALink3 in this case so release it
    RELEASE(ptemp);
}

/*static*/
void PEBuilder::CreateALink
(
    IALink2 **ppIALink,     // OUT: Created interface
    IALink3 **ppIALink3     // OUT: Created interface
)
{
#ifndef FEATURE_CORESYSTEM
    HRESULT hr = S_OK;
    static HRESULT (WINAPI *s_pfnCreateALink)(REFIID, IUnknown **) = NULL;

    // Get the function pointer only once
    if (s_pfnCreateALink == NULL)
    {
        HINSTANCE hinstALink = NULL;

        // Load ALINK.DLL and initialize it. Do this in a late bound way so
        // we load the correct copy via the shim.
        hr = LegacyActivationShim::LoadLibraryShim(L"alink.dll", NULL, NULL, &hinstALink);
        if (hinstALink == NULL)
        {
            VSASSERT(FAILED(hr), "NULL handle returned without error code?");
            VSFAIL("Unable to load critical DLL 'alink.dll'. It must be installed in the COM+ directory.");
            VbThrow(HrMakeReplWithError(ERRID_UnableToLoadDll1, hr, L"alink.dll"));
        }

        s_pfnCreateALink = (HRESULT (WINAPI *)(REFIID, IUnknown**))GetProcAddress(hinstALink, "CreateALink");
        if (s_pfnCreateALink == NULL)
        {
            VbThrow(HrMakeReplWithLast(ERRID_BadDllEntrypoint2, WIDE("CreateALink"), L"alink.dll"));
        }
    }

    // Actualy create IALink2
    hr = (*s_pfnCreateALink)(IID_IALink2, (IUnknown**)ppIALink);
    if (*ppIALink == NULL)
    {
        VSASSERT(FAILED(hr), "How can out IALink be NULL without a failed HRESULT?");
        VbThrow(HrMakeWithError(ERRID_UnableToCreateALinkAPI, hr));
    }

    // Also try and query for IALink3.  We don't return the failure code from
    // this function so make sure to NULL out the interface on failure.
    hr = (*ppIALink)->QueryInterface(IID_IALink3, (void**)ppIALink3);
    if ( FAILED(hr) )
    {
        *ppIALink3 = NULL;
    }
#endif
}

//============================================================================
// Unloads the metadata without destroying any of our internal tables.
//============================================================================

void PEBuilder::UnloadMetaData()
{
    RELEASE(m_pmdEmit);
    RELEASE(m_pmdAssemblyImport);
}

//============================================================================
// Free all of the resources in the PE Builder.
//============================================================================
void PEBuilder::Destroy()
{
    // Prevent us from recursing.
    m_cCompiledFiles = -1;

    // Clear out the metadata.
    UnloadMetaData();

    m_Transients.Clear();

    m_State = BS_Empty;

    Builder::Destroy();

    m_cCompiledFiles = 0;

#if IDE 
    EndEmit(m_pInfoDeferWritePE);
#endif
}

//============================================================================
// Define all of the types in a file.  You must do all of the outer-most
// (non-nested) types first.
//============================================================================

void PEBuilder::DefineContainer
(
    BCSYM_Container *pContainer
)
{
    mdTypeDef tkTypeDef;

    //
    // The information we need to gather to create the class.
    //

    STRING *pstrName = pContainer->GetEmittedName();
    STRING *pstrNameSpace = pContainer->GetNameSpace();

    //
    // Define the class.
    //

    // Nested classes are handled slightly differently.
    if (pContainer->GetParent()->DigThroughAlias()->IsNamespace())
    {
        VSASSERT(pstrName && pstrNameSpace, "Should be set.");

        IfFailThrow(m_pmdEmit->DefineTypeDef(ConcatNameSpaceAndName(m_pCompiler,
                                                                    pstrNameSpace,
                                                                    pstrName), // [IN] Name of TypeDef
                                             GetTypeDefFlags(pContainer),   // [IN] CustomAttribute flags
                                             mdTypeRefNil,                  // [IN] extends this TypeDef or typeref
                                             NULL,                          // [IN] Implements interfaces
                                             &tkTypeDef));                  // [OUT] Put TypeDef token here
    }
    else
    {
        IfFailThrow(m_pmdEmit->DefineNestedType(pstrName,                       // [IN] Name of TypeDef
                                                GetTypeDefFlags(pContainer),    // [IN] CustomAttribute flags
                                                mdTypeRefNil,                   // [IN] extends this TypeDef or typeref
                                                NULL,                           // [IN] Implements interfaces
                                                GetToken(pContainer->GetParent()->DigThroughAlias()->PContainer()),   // [IN] TypeDef token of the enclosing type
                                                &tkTypeDef));                   // [OUT] Put TypeDef token here
    }

    if (pContainer->IsStruct())
    {
        bool bHasFields = false;
        BCSYM_NamedRoot* Symbol = NULL;
        DefineableMembersInAContainer FieldIter(m_pCompiler, pContainer);
        while (Symbol = FieldIter.Next())
        {
            if (Symbol->GetBindingSpace() && Symbol->DigThroughAlias()->IsVariable())
            {
                bHasFields = true;
                break;
            }
        }

        // If we have no fields, we must call SetClassLayout on the structure.

        if (!bHasFields)
        {
            COR_FIELD_OFFSET offsets[1];
            offsets[0].ridOfField = mdFieldDefNil;
            offsets[0].ulOffset = 0;
            IfFailThrow(m_pmdEmit->SetClassLayout(tkTypeDef, 0, offsets, 1));
        }
    }

    VSASSERT(!IsNilToken(tkTypeDef), "Bad COM+ output!");

#if IDE 
    // Remember the token so we can delete it later.
    CacheMetaToken(pContainer,tkTypeDef);
#endif

    //
    // Remember it.
    //
    SetToken(pContainer, tkTypeDef);

    //
    // Define nested interfaces for classes that have the <ComClass()> attribute
    //
    DefineComClassInterfaces(pContainer);
}

void PEBuilder::DefineComClassInterfaces
(
    BCSYM_Container *pContainer
)
{
    if (!pContainer->IsClass())
        return;

    BCSYM_Class* ClassSymbol = pContainer->DigThroughAlias()->PClass();

    // Does this class have the <ComClass()> attribute?
    if (!ClassSymbol->GetPWellKnownAttrVals()->GetComClassData(NULL))
    {
        // Reset the COMClass attribute token of it doesn't exist in the list of tokens used.
        // This needs to be done because it doesn't happen when we decompile below TypesEmitted.
        Symbols::SetComClassToken(ClassSymbol, mdTokenNil);
        return;
    }

    // Get iterators for the members that are valid on the class and events interfaces.
    ComClassMembersInAContainer ClassInterfaceIter(ClassSymbol);
    ComClassEventsInAContainer EventsInterfaceIter(ClassSymbol);

    // If no members can be exposed, just quit. A warning for this situation
    // has already been emitted in VerifyAttributeUsage--but it has location
    // context there, whereas here it has only the project error table.
    if (ClassInterfaceIter.Count() == 0 &&
        EventsInterfaceIter.Count() == 0)
    {
        return;
    }


    // Mark the members of the class and events interface.
    {
        BCSYM_NamedRoot *pnamed;
        BCSYM_Proc *pproc;

        ClassInterfaceIter.Reset();
        while ((pnamed = ClassInterfaceIter.Next()) != NULL)
        {
            pproc = pnamed->DigThroughAlias()->PProc();
            Symbols::SetIsImplementingComClassInterface(pproc);
        }

        EventsInterfaceIter.Reset();
        while ((pnamed = EventsInterfaceIter.Next()) != NULL)
        {
            pproc = pnamed->DigThroughAlias()->PProc();
            Symbols::SetIsImplementingComClassInterface(pproc);
        }
    }

    //
    // The information we need to gather to create the interface.
    //
    STRING* TypeName;
    mdTypeDef TypeDefOut;
    DWORD  TypeDefFlags = tdInterface | tdAbstract | tdNestedPublic;
    mdTypeDef TypeDefContainer = (mdTypeDef)GetToken(pContainer->DigThroughAlias()->PContainer());

    //
    // Define the class interface.
    //


    TypeName = ClassSymbol->GetComClassInterfaceName();
    IfFailThrow(m_pmdEmit->DefineNestedType(TypeName,           // [IN] Name of TypeDef
                                            TypeDefFlags,       // [IN] CustomAttribute flags
                                            mdTypeRefNil,       // [IN] extends this TypeDef or typeref
                                            NULL,               // [IN] Implements interfaces
                                            TypeDefContainer,   // [IN] TypeDef token of the enclosing type
                                            &TypeDefOut));       // [OUT] Put TypeDef token here

    VSASSERT(!IsNilToken(TypeDefOut), "Bad COM+ output!");

#if IDE 
    // Remember the token so we can delete it later.
    CacheMetaToken(ClassSymbol,TypeDefOut);
#endif

    Symbols::SetComClassToken(ClassSymbol, TypeDefOut);


    //
    // Define the events interface, if there are any events.
    //
    if (EventsInterfaceIter.Count() != 0)
    {
        TypeName = ClassSymbol->GetComClassEventsInterfaceName();
        IfFailThrow(m_pmdEmit->DefineNestedType(TypeName,           // [IN] Name of TypeDef
                                                TypeDefFlags,       // [IN] CustomAttribute flags
                                                mdTypeRefNil,       // [IN] extends this TypeDef or typeref
                                                NULL,               // [IN] Implements interfaces
                                                TypeDefContainer,   // [IN] TypeDef token of the enclosing type
                                                &TypeDefOut));       // [OUT] Put TypeDef token here

        VSASSERT(!IsNilToken(TypeDefOut), "Bad COM+ output!");

#if IDE 
        // Remember the token so we can delete it later.
        CacheMetaToken(ClassSymbol,TypeDefOut);
#endif

        Symbols::SetComClassEventsToken(ClassSymbol, TypeDefOut);
    }
}

//
// The <Guid()> attribute gets added for classes that have <ComClass()> specified.
// Use the string the user has specified, or if it is null or empty, generate a new one.
static void GenerateUuidParam(const WCHAR* uuidIn, _Inout_ StringBuffer* pBuffer)
{
    pBuffer->Clear();

    // If uuidIn is null or empty, we will generate a new one
    if (uuidIn == NULL || *uuidIn == L'\0')
    {
        GUID  Guid;
        WCHAR GuidString[40];
        CoCreateGuid(&Guid);
        StringFromGUID2(Guid, GuidString, DIM(GuidString));
        GuidString[37] = L'\0';
        pBuffer->AppendString(&GuidString[1]);
    }
    // Otherwise, use the user's value
    else
    {
        pBuffer->AppendString(uuidIn);
    }
}

bool PEBuilder::CheckConditionalAttributes
(
    BCSYM_ApplAttr *AttributeApplication,
    SourceFile *SourceFile
)
{
    ConditionalString *CurrentString;
    bool Emit = false;

    AttributeApplication->GetAttributeSymbol()->GetPWellKnownAttrVals()->GetConditionalData(&CurrentString);

    if (CurrentString)
    {
        while (CurrentString)
        {
            bool NameIsBad = false;

            Declaration *ConditionalResult =
                Semantics::EnsureNamedRoot
                (
                    Semantics::InterpretName
                    (
                        CurrentString->m_pstrConditional,
                        *(AttributeApplication->GetExpression()->GetLocation()),
                        SourceFile->GetConditionalCompilationConstants()->GetHash(),
                        NULL,   // No Type parameter lookup
                        NameSearchConditionalCompilation | NameSearchIgnoreImports | NameSearchIgnoreExtensionMethods,
                        m_pErrorTable,
                        m_pCompiler,
                        m_pCompilerProject->GetCompilerHost(),
                        SourceFile,
                        NameIsBad,
                        NULL,   // No binding context for conditional constants
                        NULL,
                        -1
                    )
                );

            // 






            if (!NameIsBad && ConditionalResult == NULL)
            {
                // Try the project level CC constants.
                // Do an immediate lookup instead of InterpretName. InterpretName() looks on the symbol
                // locations for the scope of  file defined constants.

                ConditionalResult =
                    Semantics::ImmediateLookup(
                        m_pCompilerProject->GetProjectLevelCondCompScope()->GetHash(),
                        CurrentString->m_pstrConditional,
                        BINDSPACE_Normal);
            }

            if (!NameIsBad &&
                ConditionalResult &&
                !IsBad(ConditionalResult))
            {
                ConstantValue Value =
                    ConditionalResult->PVariableWithValue()->GetExpression()->GetValue();

                switch (Value.TypeCode)
                {
                    case t_bool:
                    case t_i1:
                    case t_ui1:
                    case t_i2:
                    case t_ui2:
                    case t_i4:
                    case t_ui4:
                    case t_i8:
                    case t_ui8:

                        Emit = Value.Integral != 0;
                        break;
                    case t_string:
                        Emit = true;
                        break;
                }

                if (Emit)
                {
                    break;
                }
            }

            CurrentString = CurrentString->m_pConditionalStringNext;
        }
    }
    else
    {
        Emit = true;
    }

    return Emit;
}

//============================================================================
// Emit all of the custom attributes explicitly attached to the given symbol.
//
// 


void PEBuilder::EmitAttributesAttachedToSymbol
(
    BCSYM      *psym,             // Symbol to process
    mdToken     tokenForSymbol,   // Emitted .NET typedef for this symbol
    BCSYM_Proc *psymContext,      // Defines context for attributes attached to params
    _Inout_ MetaEmit    *pMetaemitHelper,
    bool isNoPIaEmbeddedSymbol // Is this an embedded symbol for NoPia functionality?
)
{
    VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

    AttrVals       *pattrvals;
    BCSYM_ApplAttr *psymApplAttr;
    NorlsAllocator  nraTemp(NORLSLOC);   // Avoid bloating the PEBuilder's allocator
    ULONG           cAttr = 0;              // Count of attributes on this symbol
    ULONG           cSecAttr = 0;           // Count of security attributes on this symbol
    SecAttrInfo    *pSecAttrInfo = NULL;    // Security attributes information for this symbol for emitting later
    ErrorTable     *pErrorTable = NULL;     // Error table for file-specific errors

    // Symbol must be a named root or a parameter
    VSASSERT(psym->IsNamedRoot() || psym->IsParam(), "Must be named root or param.");

    // A nil metadata token means we can't attach custom attributes to this construct
    if (IsNilToken(tokenForSymbol) || psym->AreAttributesEmitted())
    {
        return;     // All done
    }

    //
    // Handle attributes that we need to emit
    //

    // Classes:
    if (psym->IsClass())
    {
        BCSYM_Class *ClassSymbol = psym->PClass();

        // If this is a standard module, mark it as such
        if (ClassSymbol->IsStdModule())
        {
            mdMemberRef StdModuleAttrMemberRef = pMetaemitHelper->DefineRTMemberRef(StandardModuleAttributeCtor);
            if ( StdModuleAttrMemberRef != NULL )
            {
                pMetaemitHelper->DefineCustomAttribute(tokenForSymbol, StdModuleAttrMemberRef );
            }
            else
            {
                pErrorTable = ClassSymbol->GetErrorTableForContext();
                pErrorTable->CreateErrorWithSymbol( ERRID_NoStdModuleAttribute, psym );
            }

            WellKnownAttrVals * pattrVals = ClassSymbol->GetPWellKnownAttrVals();

            if
            (
                ClassSymbol->ContainsExtensionMethods() &&
                (
                    ! pattrVals ||
                    ! pattrVals->GetExtensionData()
                )
            )
            {
                Declaration * pAttributeCtor =
                    Semantics::GetExtensionAttributeCtor
                    (
                        m_pCompiler,
                        m_pCompilerProject->GetCompilerHost(),
                        m_pSourceFile,
                        &nraTemp,
                        psym
                    );

                if (pAttributeCtor)
                {
                    pMetaemitHelper->DefineCustomAttribute
                        (
                            tokenForSymbol,
                            pMetaemitHelper->DefineMemberRefBySymbol
                            (
                                pAttributeCtor,
                                NULL,
                                NULL
                            )
                        );
                }
            }
        }

        // Anonymous Type
        if ((ClassSymbol->IsAnonymousType() || ClassSymbol->IsAnonymousDelegate() || ClosureNameMangler::IsClosureClass(ClassSymbol))&&
            !m_pCompilerProject->GetCompilerHost()->IsStarliteHost())
        {
            // System.Runtime.CompilerServices.CompilerGeneratedAttribute
            if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::CompilerGeneratedAttributeType))
            {
                pMetaemitHelper->DefineCustomAttribute(tokenForSymbol, 
                    pMetaemitHelper->DefineRTMemberRef(CompilerGeneratedAttributeCtor));
            }        
        }

        // If Option Compare Text is on, then write that out (needed by the Immediate window)
        if (ClassSymbol->GetSourceFile() && (ClassSymbol->GetSourceFile()->GetOptionFlags() & OPTION_OptionText))
        {
            mdMemberRef OptionTextAttrMemberRef = pMetaemitHelper->DefineRTMemberRef(OptionTextAttributeCtor);
            if ( OptionTextAttrMemberRef != NULL )
            {
                pMetaemitHelper->DefineCustomAttribute(tokenForSymbol, OptionTextAttrMemberRef );
            }
            else
            {
                pErrorTable = ClassSymbol->GetErrorTableForContext();
                pErrorTable->CreateErrorWithSymbol( ERRID_NoOptionTextAttribute, psym );
            }
        }



        // ComClass attribute implies the following attributes
        // <Guid(clsid), ClassInterface(), ComSourceInterfaces()>
        WellKnownAttrVals::ComClassData ComClassData;
        if (ClassSymbol->GetPWellKnownAttrVals()->GetComClassData(&ComClassData))
        {
            StringBuffer strUuid;

            // Emit GuidAttribute
            if (ComClassData.m_ClassId != NULL && *ComClassData.m_ClassId != L'\0')
            {
                if (pMetaemitHelper->CanEmitAttribute(GuidAttributeCtor, ClassSymbol))
                {
                    GenerateUuidParam(ComClassData.m_ClassId, &strUuid);
                    VSASSERT(Attribute::VerifyGuidAttribute(strUuid.GetString(), false), "The ComClass ClassId should have been validated in VerifyAttributeUsage!");
                    pMetaemitHelper->DefineCustomAttributeWithString(
                        tokenForSymbol,
                        pMetaemitHelper->DefineRTMemberRef(GuidAttributeCtor),
                        strUuid.GetString());
                }
            }

            // Emit ClassInterfaceAttribute
            if (pMetaemitHelper->CanEmitAttribute(ClassInterfaceAttributeCtor, ClassSymbol))
            {
                pMetaemitHelper->DefineCustomAttributeWithInt32(
                    tokenForSymbol,
                    pMetaemitHelper->DefineRTMemberRef(ClassInterfaceAttributeCtor),
                    0); // ClassInterfaceType.None == 0
            }

            // Emit ComSourceInterfaces -- only if this class has any public events
            if (!IsNilToken(GetComClassEventsToken(ClassSymbol)))
            {
                // Build up the assembly-qualified name for the interface type
                StringBuffer TypeName;

                // Start with the name for the containing class
                TypeName.AppendSTRING(
                    m_pCompiler->GetCLRTypeNameEncoderDecoder()->TypeToCLRName(
                        ClassSymbol,
                        m_pCompilerProject,
                        NULL));

                // NOTE: we should not have appended the assembly name since this is an internal class
                VSASSERT(wcschr(TypeName.GetString(), L',') == NULL , "bad com events interface name!");

                // Append the nested interface name using the "+" delimiter
                TypeName.AppendChar(L'+');
                TypeName.AppendSTRING(ClassSymbol->GetComClassEventsInterfaceName());

                // Emit ClassInterfaceAttribute
                if (pMetaemitHelper->CanEmitAttribute(ComSourceInterfacesAttributeCtor, ClassSymbol))
                {
                    pMetaemitHelper->DefineCustomAttributeWithString(
                        tokenForSymbol,
                        pMetaemitHelper->DefineRTMemberRef(ComSourceInterfacesAttributeCtor),
                        TypeName.GetString());
                }
            }

            // Check if we'll need to emit ComVisible for these.
            // We only need to set it on the interface if it is explicitly set on the class.
            bool ComVisible;
            ClassSymbol->GetPWellKnownAttrVals()->GetCOMVisibleData(&ComVisible);

            // Emit attributes for the class interface.
            mdToken tkClassInterface = GetComClassToken(ClassSymbol);
            if (!IsNilToken(tkClassInterface))
            {
                // Emit GuidAttribute
                if (ComClassData.m_InterfaceId != NULL && *ComClassData.m_InterfaceId != L'\0')
                {
                    GenerateUuidParam(ComClassData.m_InterfaceId, &strUuid);
                    VSASSERT(Attribute::VerifyGuidAttribute(strUuid.GetString(), false), "The ComClass InterfaceId should have been validated in VerifyAttributeUsage!");
                    if (pMetaemitHelper->CanEmitAttribute(GuidAttributeCtor, ClassSymbol))
                    {
                        pMetaemitHelper->DefineCustomAttributeWithString(
                            tkClassInterface,
                            pMetaemitHelper->DefineRTMemberRef(GuidAttributeCtor),
                            strUuid.GetString());
                    }
                }

                // Emit ComVisible(True) -- if explicitly set on the class.
                if (ComVisible)
                {
                    if (pMetaemitHelper->CanEmitAttribute(ComVisibleAttributeCtor, ClassSymbol))
                    {
                        pMetaemitHelper->DefineCustomAttributeWithBool(
                            tkClassInterface,
                            pMetaemitHelper->DefineRTMemberRef(ComVisibleAttributeCtor),
                            true);
                    }
                }
            }

            // Emit attributes for the events interface, if there is one.
            mdToken tkEventsInterface = GetComClassEventsToken(ClassSymbol);
            if (!IsNilToken(tkEventsInterface))
            {
                // Emit GuidAttribute
                if (ComClassData.m_EventId != NULL && *ComClassData.m_EventId != L'\0')
                {
                    GenerateUuidParam(ComClassData.m_EventId, &strUuid);
                    VSASSERT(Attribute::VerifyGuidAttribute(strUuid.GetString(), false), "The ComClass EventInterfaceId should have been validated in VerifyAttributeUsage!");
                    if (pMetaemitHelper->CanEmitAttribute(GuidAttributeCtor, ClassSymbol))
                    {
                        pMetaemitHelper->DefineCustomAttributeWithString(
                            tkEventsInterface,
                            pMetaemitHelper->DefineRTMemberRef(GuidAttributeCtor),
                            strUuid.GetString());
                    }
                }

                // Emit InterfaceTypeAttribute
                if (pMetaemitHelper->CanEmitAttribute(InterfaceTypeAttributeCtor, ClassSymbol))
                {
                    pMetaemitHelper->DefineCustomAttributeWithInt32(
                        tkEventsInterface,
                        pMetaemitHelper->DefineRTMemberRef(InterfaceTypeAttributeCtor),
                        2); // ComInterfaceType.InterfaceIsIDispatch = 2
                }
                // Emit ComVisible(True) -- if explicitly set on the class.
                if (ComVisible)
                {
                    if (pMetaemitHelper->CanEmitAttribute(ComVisibleAttributeCtor, ClassSymbol))
                    {
                        pMetaemitHelper->DefineCustomAttributeWithBool(
                            tkEventsInterface,
                            pMetaemitHelper->DefineRTMemberRef(ComVisibleAttributeCtor),
                            true);
                    }
                }
            }

            // Emit DispId attributes for members of both interfaces.
            EmitComClassDispIdAttributes(ClassSymbol, pMetaemitHelper);
        }

        if (psym->PContainer()->IsStateMachineClass())
        {
            if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::CompilerGeneratedAttributeType))
              {
                  pMetaemitHelper->DefineCustomAttribute(
                      tokenForSymbol,
                      pMetaemitHelper->DefineRTMemberRef(CompilerGeneratedAttributeCtor));
              }
        }       
    }

    // Copy attributes from PIA types to their local proxies.
    AssertIfFalse((isNoPIaEmbeddedSymbol && psym->IsType()) == TypeHelpers::IsEmbeddableInteropType(psym));
    if (isNoPIaEmbeddedSymbol && psym->IsType())
    {
        EmitPiaTypeAttributes(psym->PContainer(), tokenForSymbol, pMetaemitHelper);
    }

    // Copy attributes from members of PIA types to the proxy members on the local types.
    // It is only necessary to explicitly copy "well-known" attributes.
     AssertIfFalse((!psym->IsNamedRoot()) ||(!psym->PNamedRoot()->GetContainer()) || 
        ((isNoPIaEmbeddedSymbol && psym->PNamedRoot()->GetContainer()->IsType()) ==
                TypeHelpers::IsEmbeddableInteropType(psym->PNamedRoot()->GetContainer())));
    if (isNoPIaEmbeddedSymbol && psym->IsNamedRoot())
    {
        BCSYM_Container * pContainer = psym->PNamedRoot()->GetContainer();
        
        if (pContainer && pContainer->IsType())
        {
            EmitPiaTypeMemberAttributes(psym, tokenForSymbol, pMetaemitHelper);
        }
    }

    // General containers
    if (psym->IsContainer())
    {
        // Look through the container for a default property
        BCSYM_Container *ContainerSymbol = psym->PContainer();
        BCITER_CHILD ChildIterator(ContainerSymbol);

        for (BCSYM_NamedRoot *Child = ChildIterator.GetNext();
             Child;
             Child = ChildIterator.GetNext())
        {
            if (Child->IsProperty() && Child->PProperty()->IsDefault())
            {
                WCHAR *pwszDefaultMember;
                // chek for conflict with a DefaultMemberAttribute
                if (ContainerSymbol->GetPWellKnownAttrVals()->GetDefaultMemberData(&pwszDefaultMember))
                {
                    if ( !StringPool::IsEqual( m_pCompiler->AddString(pwszDefaultMember), Child->GetName()))
                    {
                        // different name: error
                        pErrorTable = Child->GetErrorTableForContext();
                        pErrorTable->CreateErrorWithSymbol(ERRID_ConflictDefaultPropertyAttribute, psym, ContainerSymbol->GetName());
                        break;
                    }
                    // else: same name: skip emmiting the duplicate
                }
                else
                {
                    // no default member attribute
                // Must write out DefaultMemberAttribute
                    if (pMetaemitHelper->CanEmitAttribute(DefaultMemberAttributeCtor, Child))
                    {
                        pMetaemitHelper->DefineCustomAttributeWithString(tokenForSymbol,
                                                                       pMetaemitHelper->DefineRTMemberRef(DefaultMemberAttributeCtor),
                                                                       Child->GetName());
                    }
                }

                // Ditto for the com class interface
                if (psym->IsClass() && !IsNilToken(GetComClassToken(psym->PClass())))
                {
                    pMetaemitHelper->DefineCustomAttributeWithString(
                                        GetComClassToken(psym->PClass()),
                                        pMetaemitHelper->DefineRTMemberRef(DefaultMemberAttributeCtor),
                                        Child->GetName());
                }

                break;
            }
        }

    }

    // Parameters:
    if (psym->IsParam())
    {
        BCSYM_Param *ParamSymbol = psym->PParam();

        // If this is a paramarray, mark it as such
        if (!isNoPIaEmbeddedSymbol && ParamSymbol->IsParamArray())
        {
            if (pMetaemitHelper->CanEmitAttribute(ParamArrayAttributeCtor, ParamSymbol))
            {
                pMetaemitHelper->DefineCustomAttribute(tokenForSymbol,
                                                       pMetaemitHelper->DefineRTMemberRef(ParamArrayAttributeCtor));
            }

            // Ditto for com class interface
            if (!IsNilToken(ParamSymbol->GetComClassToken()))
            {
                if (pMetaemitHelper->CanEmitAttribute(ParamArrayAttributeCtor, ParamSymbol))
                {
                    pMetaemitHelper->DefineCustomAttribute(
                                    ParamSymbol->GetComClassToken(),
                                    pMetaemitHelper->DefineRTMemberRef(ParamArrayAttributeCtor));
                }
            }
        }

        // If it has a decimal default, emit it
        if (ParamSymbol->IsParamWithValue())
        {
            BCSYM_Expression * DefaultValueExpression = ParamSymbol->PParamWithValue()->GetExpression();

            if (DefaultValueExpression)
            {
                ConstantValue DefaultValue = DefaultValueExpression->GetValue();

                if (DefaultValue.TypeCode == t_decimal)
                {
                    pMetaemitHelper->DefineDecimalCustomAttribute(tokenForSymbol, DefaultValue.Decimal);

                    // Ditto for com class interface
                    if (!IsNilToken(ParamSymbol->GetComClassToken()))
                    {
                        pMetaemitHelper->DefineDecimalCustomAttribute(
                                        ParamSymbol->GetComClassToken(),
                                        DefaultValue.Decimal);
                    }
                }

                if (DefaultValue.TypeCode == t_date)
                {
                    if (pMetaemitHelper->CanEmitAttribute(DateTimeConstantAttributeCtor, ParamSymbol))
                    {
                        pMetaemitHelper->DefineCustomAttributeWithInt64(tokenForSymbol,
                                                                        pMetaemitHelper->DefineRTMemberRef(DateTimeConstantAttributeCtor),
                                                                        DefaultValue.Integral);
                    }

                    // Ditto for com class interface
                    if (!IsNilToken(ParamSymbol->GetComClassToken()))
                    {
                        if (pMetaemitHelper->CanEmitAttribute(DateTimeConstantAttributeCtor, ParamSymbol))
                        {
                            pMetaemitHelper->DefineCustomAttributeWithInt64(ParamSymbol->GetComClassToken(),
                                                                          pMetaemitHelper->DefineRTMemberRef(DateTimeConstantAttributeCtor),
                                                                          DefaultValue.Integral);
                        }
                    }
                }
            }
        }
        else if (isNoPIaEmbeddedSymbol)
        {
            ULONG cbArgBlob;
            BYTE  *argBlob;
            WellKnownAttrVals *wkAttr = ParamSymbol->GetPWellKnownAttrVals();
           
            if (wkAttr && wkAttr->GetDefaultParamValueData(argBlob, cbArgBlob))
            {
                Vtypes vtypArg[2] = {(Vtypes) c_object, t_UNDEF }; // vtypArg stores the argument type, it must be zero terminated.
                BCSYM_NamedRoot *BoundSymbolCtor = GetSymbolForAttribute(DEFAULTPARAMETERVALUEATTRIBUTE, vtypArg, pMetaemitHelper);
                if (BoundSymbolCtor != NULL)
                {
                    pMetaemitHelper->DefineCustomAttributeObject(tokenForSymbol,
                                                                 pMetaemitHelper->DefineMemberRefBySymbol(BoundSymbolCtor,NULL,NULL),
                                                                 cbArgBlob, 
                                                                 argBlob);
                }
            }
        }
    }

    if (!psym->IsSyntheticMethod() &&
        psym->IsProc() && 		
        (psym->PProc()->IsAsyncKeywordUsed() || psym->PProc()->IsIteratorKeywordUsed()))
    {
        // This branch only catchs resumable methods, not resumable lambdas. 
        if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::DebuggerStepThroughAttributeType))
        {
            pMetaemitHelper->DefineCustomAttribute(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(DebuggerStepThroughAttributeCtor));
        }
    }
    else if (psym->IsSyntheticMethod() &&
             (psym->PSyntheticMethod()->IsLambda() || psym->PSyntheticMethod()->IsResumable()))
    {
        if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::DebuggerStepThroughAttributeType) &&
            psym->PSyntheticMethod()->IsLambda() &&
            (psym->PSyntheticMethod()->GetIsRelaxedDelegateLambda() || psym->PProc()->IsAsyncKeywordUsed() || psym->PProc()->IsIteratorKeywordUsed()))
        {
            //DebuggerStepThroughAttribute is also need to apply on async/iterator lambda stub.
            
            pMetaemitHelper->DefineCustomAttribute(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(DebuggerStepThroughAttributeCtor));
        }
        if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::CompilerGeneratedAttributeType))
        {
            pMetaemitHelper->DefineCustomAttribute(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(CompilerGeneratedAttributeCtor));
        }
    }
    else if (psym->IsSyntheticMethod() &&
#if IDE 
        !psym->PProc()->IsENCSynthFunction() &&
#endif
        m_pCompilerProject->GenerateDebugCode() &&
        (!psym->PSyntheticMethod()->GetDebugInfoList()->NumberOfEntries() || // 
         (psym->PSyntheticMethod()->GetContainingCompilerFile() && psym->PSyntheticMethod()->GetContainingCompilerFile()->IsSolutionExtension())))
    {
        if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::DebuggerNonUserCodeAttributeType))
        {
            pMetaemitHelper->DefineCustomAttribute(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(DebuggerNonUserCodeAttributeCtor));
        }
    }   

    // Fields
    if (psym->IsVariable())
    {
        BCSYM_Variable *VariableSymbol = psym->PVariable();

        if (VariableSymbol->GetVarkind() == VAR_Const ||
            VariableSymbol->GetVarkind() == VAR_Member)
        {
            if (VariableSymbol->IsWithEvents())
            {
                if (pMetaemitHelper->CanEmitAttribute(AccessedThroughPropertyAttributeCtor, VariableSymbol))
                {
                    pMetaemitHelper->DefineCustomAttributeWithString(
                        tokenForSymbol,
                        pMetaemitHelper->DefineRTMemberRef(AccessedThroughPropertyAttributeCtor),
                        VariableSymbol->GetName() + 1); // Skip the leading _
                }
            }

            if ( (VariableSymbol->IsAutoPropertyBackingField()) &&
                 (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::CompilerGeneratedAttributeType)))
            {
                // Add the CompilerGenerated attribute to the backing field of an AutoProperty.
                pMetaemitHelper->DefineCustomAttribute(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(CompilerGeneratedAttributeCtor));

                // The following code is added for Bug511211
                // 1. User should have two ways to generate code with and without this attribute
                // 2. It would make more sense if this attribute is generated when the build is intended for debug purpose;  
                //    a build with the following setups, I believe the build is for debug purpose. 
                //      a. /debug
                //      b. /debug:+
                //      c. /debug:full
                //      d. VS -> F5
                // GenerateDebugCode() returns true if and only if for the above four situations. 

                if (m_pCompilerProject->GenerateDebugCode())
                {
                    mdMemberRef dbCtorTok = pMetaemitHelper->DefineRTMemberRef(DebuggerBrowsableAttributeCtor);
                    if (!IsNilToken(dbCtorTok))
                    {
                        // DebuggerBrowsable takes one argument of DebuggerBrowsableState, 
                        // which is an enumeration(Never, Collapsed, RootHidden); a backing field
                        // should not be shown in the debugger, so we should use the value "Never",
                        // which corresponds to 0.
                        pMetaemitHelper->DefineCustomAttributeWithInt32(
                            tokenForSymbol,
                            dbCtorTok,
                            0);
                    }
                }
            }

            // If it has a decimal default, emit it
            if (VariableSymbol->GetVarkind() == VAR_Const)
            {
                BCSYM_Expression * DefaultValueExpression = VariableSymbol->PVariableWithValue()->GetExpression();

                if (DefaultValueExpression)
                {
                    ConstantValue DefaultValue = DefaultValueExpression->GetValue();

                    if (DefaultValue.TypeCode == t_decimal)
                    {
                        pMetaemitHelper->DefineDecimalCustomAttribute(tokenForSymbol, DefaultValue.Decimal);
                    }

                    if (DefaultValue.TypeCode == t_date)
                    {
                        if (pMetaemitHelper->CanEmitAttribute(DateTimeConstantAttributeCtor, VariableSymbol))
                        {
                            pMetaemitHelper->DefineCustomAttributeWithInt64(
                                tokenForSymbol,
                                pMetaemitHelper->DefineRTMemberRef(DateTimeConstantAttributeCtor),
                                DefaultValue.Integral);
                        }
                    }
                }
            }
        }
    }

    //
    // Now process attributes directly applied to this container
    //
    pattrvals = psym->GetPAttrVals();

    // Get a count of attributes on this symbol
    //

    //Dev10 #570413 According to NoPIA spec, the following attributes should be copied for NoPia symbols
    //
    //ComImportAttribute
    //InterfaceTypeAttribute
    //GuidAttribute
    //BestFitMappingAttribute 
    //StructLayoutAttribute
    //FieldOffsetAttribute
    //CoClassAttribute 
    //MarshalAsAttribute
    //LCIDConversionAttribute
    //DispIdAttribute
    //PreserveSigAttribute
    //InAttribute
    //OutAttribute
    //UnmanagedFunctionPointerAttribute 
    //DefaultParameterValueAttribute
    //OptionalAttribute
    //SerializableAttribute
    //
    // Below, we will filter out any other attributes. However, for simplicity, we won't be 
    // differentiating filtering based on the symbol kind (Interface vs. method vs. parameter, etc)
    // Because no harm can come from emitting any of those attributes anyway, I believe.
    
    BCITER_ApplAttrs ApplAttrsIter(psym, false);
    while (psymApplAttr = ApplAttrsIter.GetNext())
    {
        //Dev10 #570413 
        if (isNoPIaEmbeddedSymbol)
        {
            const AttrIndex * pAttrIndex = psymApplAttr->GetAttrIndex();

            if (!pAttrIndex || !pAttrIndex->fCopyForNoPiaEmbeddedSymbols)
            {
                continue;
            }
        }
        
        cAttr++;

        if (psymApplAttr->GetAttributeSymbol()->IsClass() &&
            psymApplAttr->GetAttributeSymbol()->PClass()->IsSecurityAttribute())
        {
            //Dev10 #570413 None of the transferrable attributes are security attributes
            AssertIfTrue(isNoPIaEmbeddedSymbol);
            if (!isNoPIaEmbeddedSymbol)
            {
                cSecAttr++;
            }
        }
    }

    if (cAttr != 0)
    {
        // Allocate space to batch up security attributes.  Worst case is
        // every attribute is a security attribute, so allocate that much.
        // Allocate space for security attribute names, too, for error reporting.
        IfFalseThrow(VBMath::TryMultiply(sizeof(COR_SECATTR), cAttr) &&
            VBMath::TryMultiply(sizeof(STRING *), cAttr) &&
            VBMath::TryMultiply(sizeof(Location *), cAttr) &&
            VBMath::TryMultiply(sizeof(ErrorTable *), cAttr));

        if (cSecAttr > 0)
        {
            pSecAttrInfo = m_pCompilerProject->CreateSecAttrsForSymbol(psym, tokenForSymbol, cSecAttr);
        }

        // Loop over all BCSYM_ApplAttr nodes attached to AttrVals
        ApplAttrsIter.Init(psym, false);
        while (psymApplAttr = ApplAttrsIter.GetNext())
        {
            //Dev10 #570413 
            if (isNoPIaEmbeddedSymbol)
            {
                const AttrIndex * pAttrIndex = psymApplAttr->GetAttrIndex();

                if (!pAttrIndex || !pAttrIndex->fCopyForNoPiaEmbeddedSymbols)
                {
                    continue;
                }
            }

            // The following code shows how vb interprets <NonSerialized> attribute on event.
            // The attribute is moved to the backing field of an event.
            // This is Bug 674331
            if (psym->IsEventDecl() && 
                psymApplAttr->GetAttrIndex() != NULL &&
                psymApplAttr->GetAttrIndex()->Attributes[0].m_attrkind == attrkindNonSerialized_Void)
            {
                if (psym->PEventDecl()->GetDelegateVariable() != NULL)
                {
                    EmitCustomAttribute(
                        psym->PEventDecl()->GetDelegateVariable(),
                        GetToken(psym->PEventDecl()->GetDelegateVariable()),
                        psymApplAttr,
                        ApplAttrsIter.GetNamedContextForCurrentApplAttr()->GetErrorTableForContext(),
                        pMetaemitHelper,
                        &nraTemp,
                        pSecAttrInfo,
                        ApplAttrsIter.GetNamedContextForCurrentApplAttr());
                }
            }
            else
            {
                EmitCustomAttribute(
                    psym,
                    tokenForSymbol,
                    psymApplAttr,
                    ApplAttrsIter.GetNamedContextForCurrentApplAttr()->GetErrorTableForContext(),
                    pMetaemitHelper,
                    &nraTemp,
                    pSecAttrInfo,
                    ApplAttrsIter.GetNamedContextForCurrentApplAttr());
            }
        }
    }

    if (psym->IsProc() && (psym->PProc()->IsAsyncKeywordUsed() || psym->PProc()->IsIteratorKeywordUsed()))
    {
        ResumableMethodToStateMachineAttrMap*  pResumableMethodToStateMachineAttrMap = m_Transients.GetResumableMethodToStateMachineAttrMap();

        BCSYM_ApplAttr** ppResumableStateMachineAttr = pResumableMethodToStateMachineAttrMap->HashFind(psym->PProc());

        if (ppResumableStateMachineAttr && *ppResumableStateMachineAttr)
        {
            EmitCustomAttribute(
                psym,
                tokenForSymbol,
                *ppResumableStateMachineAttr ,
                m_pErrorTable, 
                pMetaemitHelper,
                &nraTemp,
                pSecAttrInfo,
                psym->PNamedRoot());        
        }
    }

    if (psym->IsProc() && psym->PProc()->JITOptimizationsMustBeDisabled())
    {
        // Dev10 #850039 We need to apply extra flags to disable inlining and optimization for this proc.
        DWORD dwImplFlags = 0;
        CComPtr<IMetaDataImport> pimdImport;

        // Read implementation flags from Metadata, by this time they could be set to non-default value.
        IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataImport, (void **)&pimdImport));
        IfFailThrow(pimdImport->GetMethodProps(
                                                tokenForSymbol, 
                                                NULL, // pClass, 
                                                NULL, // wzName, 
                                                0, //cchName, 
                                                NULL, // pchName, 
                                                NULL, // pdwAttr, 
                                                NULL, // ppvSig, 
                                                NULL, // pcbSig, 
                                                NULL, // pulCodeRVA, 
                                                &dwImplFlags));

        pimdImport.Release();
    
        //    miNoInlining        =   0x0008,   // Method may not be inlined.
        //    miNoOptimization    =   0x0040,   // Method may not be optimized.

        // It looks like public\devdiv\inc\corhdr.h hasn't been updated for a long time and it doesn't
        // include miNoOptimization flag. ndp\clr\src\inc\corhdr.h includes the flag.
        const DWORD miNoOptimization = 0x0040;   // Method may not be optimized.
        
        IfFailThrow(m_pmdEmit->SetMethodImplFlags(tokenForSymbol, dwImplFlags | miNoInlining  | miNoOptimization));   
    }

    if (psym->IsNamedRoot())
    {
        Symbols::SetAttributesEmitted(psym->PNamedRoot());
    }
    else
    {
        AssertIfFalse(psym->IsParam());
        Symbols::SetAttributesEmitted(psym->PParam());
    }
}

void PEBuilder::EmitPiaTypeAttributes(BCSYM_Container *psym, mdToken tokenForSymbol, MetaEmit *pMetaemitHelper)
{
    AssertIfNull(psym);
    AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(psym));
    AssertIfNull(pMetaemitHelper);
    long interfaceType = 0;

    // Dev10 633240 - Put the CompilerGenerated attribute on the NOPIA types we define so that 
    // static analysis tools (e.g. fxcop) know that they can be skipped
    if (m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::CompilerGeneratedAttributeType))
    {
        pMetaemitHelper->DefineCustomAttribute( tokenForSymbol, 
                                                pMetaemitHelper->DefineRTMemberRef(CompilerGeneratedAttributeCtor)); 
    }

    if (psym->GetPWellKnownAttrVals()->GetInterfaceTypeData(&interfaceType))
    {
        if (pMetaemitHelper->CanEmitAttribute(InterfaceTypeAttributeCtor, psym))
        {
            pMetaemitHelper->DefineCustomAttributeWithInt32(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(InterfaceTypeAttributeCtor),
                interfaceType);
        }
    }

    bool bestFitMapping = false;
    bool throwOnUnmappableChar = false;
    if (psym->GetPWellKnownAttrVals()->GetBestFitMappingData(&bestFitMapping, &throwOnUnmappableChar))
    {
        if (throwOnUnmappableChar)
        {
            // If the BestFitMappingAttribute included a non-default value for its ThrowOnUnmappableChar property,
            // we must emit a special field initializer value. This fixes Dev10 bug #535429.
            pMetaemitHelper->DefineBestFitMappingAttribute(tokenForSymbol, bestFitMapping, throwOnUnmappableChar);
        }
        else
        {
            // If we don't have to set ThrowOnUnmappableChar, which defaults to false, then we should 
            // just use the ordinary custom attribute helper.
            if (pMetaemitHelper->CanEmitAttribute(BestFitMappingAttributeCtor, psym))
            {
                pMetaemitHelper->DefineCustomAttributeWithBool(
                    tokenForSymbol,
                    pMetaemitHelper->DefineRTMemberRef(BestFitMappingAttributeCtor),
                    bestFitMapping);
            }
        }
    }
    
    WellKnownAttrVals::ComEventInterfaceData ceid;
    if (psym->GetPWellKnownAttrVals()->GetComEventInterfaceData(&ceid))
    {
        // When emitting a com event interface, we have to tweak the parameters: the spec requires that we use
        // the original source interface as both source interface and event provider. Otherwise, we'd have to embed
        // the event provider class too.
        Vtypes vtypArg[] = {(Vtypes)c_class, (Vtypes)c_class, t_UNDEF };
        BCSYM_NamedRoot *BoundSymbolCtor = GetSymbolForAttribute(COMEVENTINTERFACEATTRIBUTE, vtypArg, pMetaemitHelper);
        AssertIfNull(BoundSymbolCtor);
        if (BoundSymbolCtor)
        {
            pMetaemitHelper->DefineCustomAttributeWithString_String(
                tokenForSymbol,
                pMetaemitHelper->DefineMemberRefBySymbol(BoundSymbolCtor, NULL, NULL),
                ceid.m_SourceInterface,
                ceid.m_SourceInterface);
        }
    }

    WCHAR *coClassName = NULL;
    if (psym->GetPWellKnownAttrVals()->GetCoClassData(&coClassName))
    {
        // The interface needs to have a coclass attribute so that we can tell at runtime that it should be
        // instantiable. The attribute cannot refer directly to the coclass, however, because we can't embed
        // classes, and in any case we can't emit a reference to the PIA. In any case, we don't actually need
        // the class name at runtime: we will instead emit a reference to System.Object, as a placeholder.
        Vtypes vtypArg[] = {(Vtypes)c_class, t_UNDEF};
        BCSYM_NamedRoot *BoundSymbolCtor = GetSymbolForAttribute(COCLASSATTRIBUTE, vtypArg, pMetaemitHelper);
        AssertIfNull(BoundSymbolCtor);
        if (BoundSymbolCtor)
        {
            pMetaemitHelper->DefineCustomAttributeWithString(
                tokenForSymbol,
                pMetaemitHelper->DefineMemberRefBySymbol(BoundSymbolCtor, NULL, NULL),
                L"System.Object");
        }
    }
    // If this type has a GuidAttribute, we should emit it. Interfaces will use this attribute for type unification;
    // other types will use the guid value in the TypeIdentifierAttribute. 
    WCHAR *guidData = NULL;
    bool hasGuid = psym->GetPWellKnownAttrVals()->GetGuidData(&guidData);
    if (hasGuid)
    {
        if (pMetaemitHelper->CanEmitAttribute(GuidAttributeCtor, psym))
        {
            pMetaemitHelper->DefineCustomAttributeWithString(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(GuidAttributeCtor),
                guidData);
        }
    }
    // We must emit a TypeIdentifier attribute which connects this local type with the canonical type. 
    // Interfaces usually have a guid attribute, in which case the TypeIdentifier attribute we emit will
    // not need any additional parameters. For interfaces which lack a guid and all other types, we must 
    // emit a TypeIdentifier that has parameters identifying the scope and name of the original type. We 
    // will use the Assembly GUID as the scope identifier.
    if (psym->IsInterface() && hasGuid)
    {
        // This is an interface with a GuidAttribute, so we will generate the no-parameter TypeIdentifier.
        mdMemberRef typeIDAttrToken = pMetaemitHelper->DefineRTMemberRef(TypeIdentifierAttributeCtor);
        if (!IsNilToken(typeIDAttrToken))
        {
            pMetaemitHelper->DefineCustomAttribute(tokenForSymbol, typeIDAttrToken);
        }
        else
        {
            m_pErrorTable->CreateErrorWithSymbol(ERRID_CorlibMissingPIAClasses1, 
                                                 psym, 
                                                 TYPEIDENTIFIERATTRIBUTE);
        }
    }
    else 
    {
        // This is an interface with no GuidAttribute, or some other type, so we will generate the
        // TypeIdentifier with name and scope parameters.
        if (psym->IsInterface() && !psym->GetPWellKnownAttrVals()->GetComEventInterfaceData(NULL))
        {
            // Interfaces used with No-PIA ought to have a guid attribute, or the CLR cannot do type unification. 
            // This interface lacks a guid, so unification probably won't work. We will warn the user about this 
            // condition. I'm not sure why this is a warning rather than an error, but that's what the spec calls for.
            // ...Except Avner can find no trace of it in the spec, which currently calls for a different error.
            m_pErrorTable->CreateErrorWithSymbol(
                    ERRID_NoPIAAttributeMissing2, 
                    psym, 
                    psym->GetName(),
                    GUIDATTRIBUTE_NAME);
        }

        // Iterate through all of the files in this type's containing project, looking for 
        // a GUID attribute attached to the root namespace. If we find one, we'll use it; 
        // otherwise, we expect that we will have reported an error about this assembly, since
        // you can't /link against an assembly which lacks a GuidAttribute.
        FileIterator files(psym->GetContainingProject());
        bool bHasGuid = false;
        WCHAR *assemblyGuid = NULL;
        while (CompilerFile *pFile = files.Next())
        {
            BCSYM_Namespace *psymNamespace = pFile->GetRootNamespace();
            AssertIfNull(psymNamespace);
            if (psymNamespace && psymNamespace->GetPWellKnownAttrVals()->GetGuidData(&assemblyGuid))
            {
                bHasGuid = true;
                break;
            }
        }
        if (bHasGuid)
        {
            mdMemberRef typeIDAttrToken = pMetaemitHelper->DefineRTMemberRef(TypeIdentifierAttributePlusGuidCtor);
            if (!IsNilToken(typeIDAttrToken))
            {
                pMetaemitHelper->DefineCustomAttributeWithString_String(
                    tokenForSymbol, 
                    typeIDAttrToken,
                    assemblyGuid,
                    psym->GetQualifiedName());  // Dev10 #735359 The second argument of type identifier attribute (the name) should be a fully qualified name. 
            }
            else
            {
                m_pErrorTable->CreateErrorWithSymbol(ERRID_CorlibMissingPIAClasses1, 
                                                     psym, 
                                                     TYPEIDENTIFIERATTRIBUTE);
            }
        }
    }

    if (psym->IsInterface() && !psym->GetPWellKnownAttrVals()->GetComImportData() &&
        !psym->GetPWellKnownAttrVals()->GetComEventInterfaceData(NULL))
    {
        // Interfaces used with No-PIA must have either the ComImport attribute or the ComEventInterface attribute.
        // Fix for 475492: must report an error when the ComImportAttribute is missing.
        // Solution is to report the error in the correct error table.
        m_pErrorTable->CreateErrorWithSymbol(
                ERRID_NoPIAAttributeMissing2,
                psym,
                psym->GetName(),
                COMIMPORTATTRIBUTE_NAME);
    }

    if (psym->GetCompilerFile()->IsMetaDataFile())
    {
        WCHAR *pwszDefaultMember;
        if (psym->GetPWellKnownAttrVals()->GetDefaultMemberData(&pwszDefaultMember))
        {  
            if (pMetaemitHelper->CanEmitAttribute(DefaultMemberAttributeCtor, psym))
            {
                pMetaemitHelper->DefineCustomAttributeWithString(tokenForSymbol,
                                                                 pMetaemitHelper->DefineRTMemberRef(DefaultMemberAttributeCtor),
                                                                 pwszDefaultMember);
            }
        }

        if (psym->IsEnum() && psym->GetPWellKnownAttrVals()->GetFlagsData())
        {
            if (pMetaemitHelper->CanEmitAttribute(FlagsAttributeCtor, psym))
            {
                pMetaemitHelper->DefineCustomAttribute(
                    tokenForSymbol,
                    pMetaemitHelper->DefineRTMemberRef(FlagsAttributeCtor));
            }
        }
    }
}

void PEBuilder::EmitPiaTypeMemberAttributes
(
 BCSYM *psym,
 mdToken tokenForSymbol,
 MetaEmit *pMetaemitHelper
)
{
    // MarshalAs is a pseudocustom attribute and is handled elsewhere.
    int lcid = 0;
    if (psym->GetPWellKnownAttrVals()->GetLCIDConversionData(&lcid))
    {
        if (pMetaemitHelper->CanEmitAttribute(LCIDConversionAttributeCtor, psym))
        {
            pMetaemitHelper->DefineCustomAttributeWithInt32(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(LCIDConversionAttributeCtor),
                lcid);
        }
    }
    long dispID = 0;
    if (psym->GetPWellKnownAttrVals()->GetDispIdData(&dispID))
    {
        if (pMetaemitHelper->CanEmitAttribute(DispIdAttributeCtor, psym))
        {
            pMetaemitHelper->DefineCustomAttributeWithInt32(
                tokenForSymbol,
                pMetaemitHelper->DefineRTMemberRef(DispIdAttributeCtor),
                dispID);     
        }
    }
    // FieldOffsetAttribute is yet another pseudocustom attribute; all field offsets are
    // handled at once, via SetClassLayout.
    
}

BCSYM_NamedRoot *PEBuilder::GetSymbolForAttribute(_In_z_ WCHAR *name, Vtypes vtypArg[], MetaEmit *pMetaemitHelper)
{
    BCSYM_NamedRoot *BoundSymbolCtor = NULL;
    BCSYM *BoundSymbol = CompilerHost::GetSymbolForQualifiedName(name,
                                                                 m_pCompilerProject,
                                                                 m_pCompiler,
                                                                 m_pCompilerProject->GetCompilerHost(),
                                                                 pMetaemitHelper->GetCompilationCaches());
    if (BoundSymbol->IsAttribute())
    { 
        BoundSymbolCtor = CompilerHost::GetSymbolForRuntimeClassCtor(
            BoundSymbol->PClass(),
            RTF_METHOD | RTF_CONSTRUCTOR,
            t_void,
            vtypArg,
            m_pCompiler);
    }
    return BoundSymbolCtor;
}

void PEBuilder::EmitAllSecurityAttributes()
{
    CSingleListIter<SecAttrInfo> SecAttrSets(m_pCompilerProject->GetSecAttrSetsToEmit());

    while (SecAttrInfo *pSecAttrInfo = SecAttrSets.Next())
    {
        if (pSecAttrInfo->cSecAttr == 0)
        {
            continue;
        }

        // If we saw any security attributes, emit them now
        HRESULT hrCOM;
        ULONG   iError;

        hrCOM = m_pmdEmit->DefineSecurityAttributeSet(
                                pSecAttrInfo->tkSymbol,
                                pSecAttrInfo->prgSecAttr,
                                pSecAttrInfo->cSecAttr,
                                &iError);

        if (FAILED(hrCOM))
        {
            // If we get back iError < cSecAttr, then we can
            // report an error on that specific security attribute.
            // Otherwise, we need to report a more generic error.
            if (iError < pSecAttrInfo->cSecAttr)
            {
                BCSYM_NamedRoot *NamedContextOfApplAttr = pSecAttrInfo->prgSecAttrErrorInfo[iError].pContext;

                NamedContextOfApplAttr->GetErrorTableForContext()->CreateErrorWithError(
                    ERRID_BadSecurityAttribute1,
                    &pSecAttrInfo->prgSecAttrErrorInfo[iError].Loc,
                    hrCOM,
                    pSecAttrInfo->prgSecAttrErrorInfo[iError].pstrAttrName);
            }
            else
            {
                BCSYM_NamedRoot *NamedContextOfApplAttr =
                    Bindable::GetNamedContextOfSymbolWithApplAttr(pSecAttrInfo->pSymbol);

                NamedContextOfApplAttr->GetErrorTableForContext()->CreateErrorWithSymbolAndError(
                    ERRID_ErrorApplyingSecurityAttribute1,
                    pSecAttrInfo->pSymbol,
                    hrCOM,
                    pSecAttrInfo->pSymbol->GetErrorName(m_pCompiler));
            }
        }
    }

    m_pCompilerProject->GetSecAttrSetsToEmit()->Clear();
}

// Compare two (long) dispids for qsort and bsearch
static int _cdecl CompareDispId( const void *elem1, const void *elem2 )
{
    long value1 = *(long*)elem1;
    long value2 = *(long*)elem2;
    return (value1 - value2);
}

// Check if this proc is a valid GetEnumerator.
// The name and signature must be
// Function GetEnumerator() As System.Collections.IEnumerator.
static bool IsGetEnumerator(BCSYM_Proc *Proc)
{
    bool result = false;

    // Name == GetEnumerator
    if (StringPool::IsEqual(Proc->GetName(), STRING_CONST(Proc->GetCompiler(), ForEachGetEnumerator)))
    {
        // Param list is empty
        if (Proc->GetFirstParam() == NULL)
        {
            // Return type is System.Collections.IEnumerator
            BCSYM *type = Proc->GetCompilerType();
            if (type != NULL && type->IsInterface())
            {
                BCSYM_Interface *Interface = type->DigThroughAlias()->PInterface();
                result = StringPool::IsEqual(
                            Interface->GetQualifiedEmittedName(),
                            STRING_CONST(Interface->GetCompiler(), SystemIEnumerator));
            }
        }
    }

    return result;
}

// Emit dispatch ids for the elements of the com class interfaces
// This handles both the class and the events interfaces.
void PEBuilder::EmitComClassDispIdAttributes
(
    BCSYM_Class *pClass,
    _Inout_ MetaEmit     *pMetaemitHelper
)
{
    MembersInAContainerBase* iters[2] = {NULL, NULL};
    BCSYM_NamedRoot *Named;
    BCSYM_Proc *Proc;
    long NextDispId;
    long DispId;
    long DispIdTemp;

    VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

    // Get iterators for the class and events members
    if (!IsNilToken(GetComClassToken(pClass)))
        iters[0] = new (zeromemory) ComClassMembersInAContainer(pClass);
    if (!IsNilToken(GetComClassEventsToken(pClass)))
        iters[1] = new (zeromemory) ComClassEventsInAContainer(pClass);

    // We'll use this typeref for emitting all DispIdAttributes
    if (!pMetaemitHelper->CanEmitAttribute(GuidAttributeCtor, pClass))
    {
        return;
    }

    mdMemberRef tkDispIdCtor = pMetaemitHelper->DefineRTMemberRef(DispIdAttributeCtor);

    // Loop over the class and the events interfaces
    for (int i = 0; i < 2; i++)
    {
        MembersInAContainerBase* iter = iters[i];
        if (iter == NULL)
            continue;

        // Get the list of IDs that the user has specified explicitly
        DynamicArray<long> UserDispIds;
        iter->Reset();
        while ((Named = iter->Next()) != NULL)
        {
            Proc = Named->DigThroughAlias()->PProc();

            if (Proc->GetPWellKnownAttrVals()->GetDispIdData(&DispId))
            {
                // Validate that the user has not used zero which is reserved
                // for the Default property.
                if (DispId == 0)
                {
                    Proc->GetErrorTableForContext()->CreateErrorWithSymbol(
                        ERRID_ComClassReservedDispIdZero1,
                        Proc,
                        Proc->GetName());
                }
                // Validate that the user has not used negative DispId's which
                // are reserved by COM and the runtime.
                else if (DispId < 0)
                {
                    Proc->GetErrorTableForContext()->CreateErrorWithSymbol(
                        ERRID_ComClassReservedDispId1,
                        Proc,
                        Proc->GetName());
                }

                // Remember the id in our list.
                UserDispIds.AddElement(DispId);
            }
        }

        // Sort the DispId array for quick lookup
        qsort(UserDispIds.Array(), UserDispIds.Count(), sizeof(long), CompareDispId);

        // Now actually emit the dispids, avoiding the ones that the
        // user has already set. Start with DispId 1.
        NextDispId = 1;
        iter->Reset();
        while ((Named = iter->Next()) != NULL)
        {
            Proc = Named->DigThroughAlias()->PProc();

            // Skip Get and Set accessors. They will be set along
            // with the Property statement.
            if (Proc->IsPropertyGet() || Proc->IsPropertySet())
                continue;

            // Get the next valid DispId. Use the next ordinal DispId the the user
            // hasn't already taken. Remember which dispid we are on in case we don't
            // end up using it in this iteration.
            bool DispIdUsed = false;
            long LastDispId = NextDispId;

            do
            {
                DispId = NextDispId++;

            } while (bsearch(
                        &DispId,
                        UserDispIds.Array(),
                        UserDispIds.Count(),
                        sizeof(long),
                        CompareDispId) != NULL);

            // Check for special dispids. Do this after incrementing NextDispId
            // so we will keep the nth item as DispId n.
            if (IsGetEnumerator(Proc))
            {
                DispId = -4; // DISPID_NEWENUM
            }
            else if (Proc->IsProperty() &&
                     Proc->PProperty()->IsDefault())
            {
                DispId = 0; // DISPID_VALUE
            }
            else if (Proc->IsProperty() &&
                     Proc->GetPWellKnownAttrVals()->GetDispIdData(&DispIdTemp))
            {
                DispId = DispIdTemp;    // use the user's Property DispId for the Get and Set
            }

            // If this method is a part of a property, then we'll set the entire
            // property at once--the property, the get and the set.
            if (Proc->IsProperty())
            {
                BCSYM_Property *Prop = Proc->PProperty();

                // Set the Property if not already set.
                if (!Prop->GetPWellKnownAttrVals()->GetDispIdData(&DispIdTemp))
                {
                    VSASSERT(!IsNilToken(GetComClassToken(Prop)), "this property should already have been emitted!");
                    pMetaemitHelper->DefineCustomAttributeWithInt32(
                        GetComClassToken(Prop),
                        tkDispIdCtor,
                        DispId);
                    DispIdUsed = true;
                }

                // Set the Get method if not already set.
                if (Prop->GetProperty() != NULL &&
                    Prop->GetProperty()->GetAccess() == ACCESS_Public &&
                    !Prop->GetProperty()->GetPWellKnownAttrVals()->GetDispIdData(&DispIdTemp))
                {
                    VSASSERT(!IsNilToken(GetComClassToken(Prop->GetProperty())), "this property get should already have been emitted!");
                    pMetaemitHelper->DefineCustomAttributeWithInt32(
                        GetComClassToken(Prop->GetProperty()),
                        tkDispIdCtor,
                        DispId);
                    DispIdUsed = true;
                }

                // Set the Set method if not alrady set.
                if (Prop->SetProperty() != NULL &&
                    Prop->SetProperty()->GetAccess() == ACCESS_Public &&
                    !Prop->SetProperty()->GetPWellKnownAttrVals()->GetDispIdData(&DispIdTemp))
                {
                    VSASSERT(!IsNilToken(GetComClassToken(Prop->SetProperty())), "this property set should already have been emitted!");
                    pMetaemitHelper->DefineCustomAttributeWithInt32(
                        GetComClassToken(Prop->SetProperty()),
                        tkDispIdCtor,
                        DispId);
                    DispIdUsed = true;
                }
            }
            // Set the Method or Event if not already set.
            else if (!Proc->GetPWellKnownAttrVals()->GetDispIdData(&DispIdTemp))
            {
                VSASSERT(Proc->IsMethodImpl() || Proc->IsEventDecl(), "this proc should be a method or an event.");
                VSASSERT(!IsNilToken(GetComClassToken(Proc)), "this proc should already have been emitted!");
                pMetaemitHelper->DefineCustomAttributeWithInt32(
                    GetComClassToken(Proc),
                    tkDispIdCtor,
                    DispId);
                DispIdUsed = true;
            }

            // If we didn't actually use this dispid, then go back
            // to the last one for the next iteration.
            if (!DispIdUsed)
            {
                NextDispId = LastDispId;
            }
        }
    }

    delete iters[0];
    delete iters[1];
}

//============================================================================
// Define the members of a container.  The cached file and its hash
// table must be set up before this call.
//============================================================================

void PEBuilder::DefineMembersOfContainer
(
    BCSYM_Container *pContainer,
    _Inout_ MetaEmit       *pMetaemitHelper
)
{
    VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

    mdTypeDef tkContainer = GetTypeDef(pContainer);

    //
    // Update the information for the container.
    //

    DWORD dwTypeDefFlags = GetTypeDefFlags(pContainer);
    mdTypeRef trExtends = mdTypeRefNil;
    mdTypeRef *rgtrImplements = NULL;
    BCSYM_NamedRoot *Symbol;
 
    pMetaemitHelper->DefineGenericParamsForType(pContainer, tkContainer);

    // Gather the information.
    switch(pContainer->GetKind())
    {
    case SYM_Class:
        {
            BCSYM *psymBase;
            BCSYM_Class *pclass = pContainer->PClass();
            mdToken tkClassInterface;

            tkClassInterface = GetComClassToken(pclass);
            psymBase = pclass->GetBaseClass();

            // The types are explicit.
            trExtends = pMetaemitHelper->DefineTypeRefBySymbol(psymBase, (psymBase ? pclass->GetInheritsLocation() : NULL));

            BCITER_ImplInterfaces implList(pclass);
            rgtrImplements = pMetaemitHelper->DefineImplements(&implList, tkClassInterface);
        }
        break;

    case SYM_Interface:
        {
            BCITER_ImplInterfaces implList(pContainer->PInterface());

            // Interfaces have interfaces that they, in turn, implement.
            const bool fEmbedAllMembersOfImplementedEmbeddableInterfaces = true;
            rgtrImplements = pMetaemitHelper->DefineImplements(&implList, mdTokenNil, fEmbedAllMembersOfImplementedEmbeddableInterfaces);
        }
        break;

    default:
        VSFAIL("What other kinds of containers do we have?");
    }

    //
    // Update the class information.
    //

    IfFailThrow(m_pmdEmit->SetTypeDefProps(tkContainer,         // [IN] The TypeDef
                                           dwTypeDefFlags,      // [IN] TypeDef flags
                                           trExtends,           // [IN] Base TypeDef or TypeRef
                                           rgtrImplements));    // [IN] Implemented interfaces

    // Grumble. The ---- "value__" field has to be emitted first.
    if (pContainer->IsEnum())
    {
        Symbol = pContainer->SimpleBind(NULL, STRING_CONST(m_pCompiler, EnumValueMember));
        VSASSERT(Symbol, "impossible!");

        pMetaemitHelper->DefineField(Symbol->PVariable());
    }
    //
    // Define all of the datamembers and methods of the class.
    //

    DefineableMembersInAContainer FirstIter(m_pCompiler, pContainer);

    while (Symbol = FirstIter.Next())
    {
        if (Symbol->GetBindingSpace())
        {
            BCSYM *pNonAliasedSymbol = Symbol->DigThroughAlias();

            if (pNonAliasedSymbol->IsProc() &&
                (!pNonAliasedSymbol->IsProperty() && !pNonAliasedSymbol->IsEventDecl()))
            {
#if IDE 
                if (!pNonAliasedSymbol->PProc()->IsENCSynthFunction())
#endif
                {
                    pMetaemitHelper->DefineMethod(pNonAliasedSymbol->PProc());
                }
            }
            else if (pNonAliasedSymbol->IsVariable() &&
                     (!pContainer->IsEnum() || pNonAliasedSymbol->PVariable()->GetVarkind() == VAR_Const)) // skip enum's "value__"
            {
                pMetaemitHelper->DefineField(pNonAliasedSymbol->PVariable());
            }
        }
    }

    // Emit the ---- backing fields for any static locals defined within the methods of the class
    if (pContainer->IsClass())
    {
        for(BCSYM_Hash *BackingFieldsHash = pContainer->PClass()->GetBackingFieldsForStatics();
            BackingFieldsHash;
            BackingFieldsHash = BackingFieldsHash->GetNextHash())
        {
            BCITER_HASH_CHILD BackingFieldsIterator(BackingFieldsHash);

            while ( BCSYM_NamedRoot *BackingField = BackingFieldsIterator.GetNext())
            {
                pMetaemitHelper->DefineField(BackingField->PVariable());
            }
        }
    }

    //
    // Define all of the properties and events of the class. We can't do this above
    // because events and properties depend on their dependent methods being
    // emitted already (and having tokens).
    //

    DefineableMembersInAContainer SecondIter(m_pCompiler, pContainer);

    while (Symbol = SecondIter.Next())
    {
        if (Symbol->GetBindingSpace())
        {
            BCSYM *pNonAliasedSymbol = Symbol->DigThroughAlias();

            if (pNonAliasedSymbol->IsProperty())
            {
                pMetaemitHelper->DefineProperty(pNonAliasedSymbol->PProperty());
            }
            else if (pNonAliasedSymbol->IsEventDecl())
            {
                pMetaemitHelper->DefineEvent(pNonAliasedSymbol->PEventDecl());
            }
        }
    }

    //
    // Define all of the members of the com class interface and the
    // com events interface
    if (pContainer->IsClass())
    {
        DefineMembersOfComClassInterfaces(pContainer->PClass(), pMetaemitHelper);
    }
}

#if DEBUG
/******************************************************************************
;RecordLocalTypeTokens

Each symbol may have an associated token. The compiler expects that the symbol 
will either be imported, in which case its associated token represents the 
metadata import value, or that the symbol has been generated from code, in 
which case its associated token represents the metaemit export value. Bug 
#530836 occurred because MetaEmit was not expecting to emit imported symbols, 
and would overwrite the original token values. The bug is now fixed and this
method exists to protect against recurrences. Its job is to record all of the
current token values for each symbol we might reach while emitting local types.
We expect that DefinePiaTypes will invoke this once for each type to be 
emitted. After emitting types, we can use the save list to ensure that the
symbols' tokens have not changed.
******************************************************************************/
static void RecordLocalTypeTokens
(
 BCSYM *pSym,
 DynamicHashTable<BCSYM*, mdToken> &tokenSaveList,
 Compiler *pCompiler
 )
{
    AssertIfNull(pSym);
    if (pSym->IsNamedRoot())
    {
        tokenSaveList.SetValue(pSym, pSym->PNamedRoot()->GetToken());
    } 
    else if (pSym->IsParam())
    {
        // Certain procs related to events re-use parameter lists.
        // In this case we will see the same parameter multiple times. This is OK; we will just
        // make sure it has the same token each time.
        if (!tokenSaveList.Contains(pSym))
        {
            tokenSaveList.SetValue(pSym, pSym->PParam()->GetToken());
        }
        else
        {
            AssertIfFalse(tokenSaveList.GetValue(pSym) == pSym->PParam()->GetToken());
        }
    }
    if (pSym->IsProc())
    {
        for (BCSYM_Param *param = pSym->PProc()->GetFirstParam(); param; param = param->GetNext())
        {
            RecordLocalTypeTokens(param, tokenSaveList, pCompiler);
        }
        if (pSym->PProc()->GetReturnTypeParam())
        {
            RecordLocalTypeTokens(pSym->PProc()->GetReturnTypeParam(), tokenSaveList, pCompiler);
        }
    }
    if (pSym->IsContainer())
    {
        DefineableMembersInAContainer iter(pCompiler, pSym->PContainer());
        while (BCSYM *Symbol = iter.Next())
        {
            RecordLocalTypeTokens(Symbol, tokenSaveList, pCompiler);
        }
    }
}
#endif //DEBUG

#if DEBUG
/******************************************************************************
;ValidateLocalTypeTokens

Check the current token values for this symbol and any other symbols it
contains, to make sure that each one matches the value we saved before emitting
local types. This catches several errors: if MetaEmit somehow overwrote the
original token value, we will detect the change; if MetaEmit added a new type
to the pia type ref cache, we will discover that there is no entry for it on
the save list; and if we fail to reach all of the entries on the save list, we
will know that there is some difference between this method and its complement
above, which suggests that we might also have failed to check some symbols.
******************************************************************************/
static void ValidateLocalTypeTokens
(
 BCSYM *pSym,
 DynamicHashTable<BCSYM*, mdToken> &tokenSaveList,
 Compiler *pCompiler
 )
{
    AssertIfNull(pSym);
    if (pSym->IsNamedRoot())
    {
        AssertIfFalse(tokenSaveList.Contains(pSym));
        AssertIfFalse(pSym->PNamedRoot()->GetToken() == tokenSaveList.GetValue(pSym));
        tokenSaveList.Remove(pSym);
    }
    else if (pSym->IsParam())
    {
        // Event add/remove procs reuse each other's parameter lists.
        // This is weird, but we have to accomodate it. It means that we can't guarantee
        // a given parameter will be on the token list; it might already have been encountered
        // and removed.
        if(tokenSaveList.Contains(pSym))
        {
            AssertIfFalse(pSym->PParam()->GetToken() == tokenSaveList.GetValue(pSym));
            tokenSaveList.Remove(pSym);
        }
    }
    if (pSym->IsProc())
    {
        for (BCSYM_Param *param = pSym->PProc()->GetFirstParam(); param; param = param->GetNext())
        {
            ValidateLocalTypeTokens(param, tokenSaveList, pCompiler);
        }
        if (pSym->PProc()->GetReturnTypeParam())
        {
            ValidateLocalTypeTokens(pSym->PProc()->GetReturnTypeParam(), tokenSaveList, pCompiler);
        }
    }
    if (pSym->IsContainer())
    {
        DefineableMembersInAContainer iter(pCompiler, pSym->PContainer());
        while (BCSYM *Symbol = iter.Next())
        {
            ValidateLocalTypeTokens(Symbol, tokenSaveList, pCompiler);
        }
    }
}
#endif //DEBUG

/******************************************************************************
;DefinePiaTypes

We are done compiling this project. We have emitted all of its types and
method bodies. If the project made any references to interop types from
projects referenced with /link, we must embed local copies of those types.
We expect to have cached all the references to interop types already, so we
can simply iterate through the list and emit a local version of each type.
******************************************************************************/
void PEBuilder::DefinePiaTypes
(
 MetaEmit *pMetaemitHelper
 )
{
    unsigned int cCacheBefore = m_pCompilerProject->CountPiaCacheEntries();

#if DEBUG
    DynamicHashTable<BCSYM*, mdToken> tokenSaveList;
    for (PiaTypeIterator iter = m_pCompilerProject->GetPiaTypeIterator(); iter.MoveNext();)
    {
        RecordLocalTypeTokens(iter.Current(), tokenSaveList, m_pCompiler);
    }
#endif

    // Generate the local type copies, then emit members for each type. We have to generate all of the
    // type definitions first, then the member definitions, because member definitions may otherwise 
    // refer to types we haven't defined yet.

    // #530113 Make sure we don't emit embedded types with duplicate names (case-insensitively).
    DynamicHashTable<STRING_INFO*, STRING*> emittedLocalTypes;

    // Dev10 #697784 we need to finish definition of Pia types in the same order in which TypeDefs were created.
    // Let's sort types in that order.
    DynamicArray<BCSYM_Container*> aTypes; 

    // It turns out that in presence of errors there could be some types in m_NoPiaTypeDefToTypeRefMap,
    // which are not in project's cache. There is also normal situation when there is a type in project's cache, 
    // which is not in m_NoPiaTypeDefToTypeRefMap because it wasn't referred to yet.
    // To make sure we have enough space, we allocate extra memory
    unsigned countOfTypes = m_pCompilerProject->CountPiaTypeCacheEntries() + m_NoPiaTypeDefToTypeRefMap.Count();
    unsigned freeEntry = m_NoPiaTypeDefToTypeRefMap.Count();

    if (countOfTypes > 0)
    {
        aTypes.Grow(countOfTypes);
    }

    BCSYM_Container *pContainer;

    for (PiaTypeIterator iter = m_pCompilerProject->GetPiaTypeIterator(); iter.MoveNext();)
    {
        AssertIfFalse(iter.Current()->IsContainer());
        
        pContainer = iter.Current()->PContainer();
        ThrowIfNull(pContainer);

        STRING *fqn = pContainer->GetQualifiedName();
        STRING_INFO* key = StringPool::Pstrinfo(fqn);
        STRING *sourcePia = pContainer->GetContainingProject()->GetAssemblyName();
        
        if (emittedLocalTypes.Contains(key))
        {
            STRING *otherPia = emittedLocalTypes.GetValue(key);
            m_pErrorTable->CreateError(ERRID_DuplicateLocalTypes3, NULL, fqn, sourcePia, otherPia); 
        }
        else
        {
            emittedLocalTypes.SetValue( key, sourcePia );
        }

        PEBuilder::PiaTypeDefInfo info;

        if (m_NoPiaTypeDefToTypeRefMap.GetValue(pContainer, &info))
        {
            ThrowIfFalse(aTypes.Element(info.m_DefinitionOrder) == NULL);
            aTypes.Element(info.m_DefinitionOrder) = pContainer;
        }
        else
        {
            // It's not legal for a NoPIA type to be contained in another type. This condition will already have been flagged
            // as an error by CachePiaTypeRef. If this check is removed, the test below for TypeFromToken(token) == mdtTypeDef
            // would throw instead, and that's a far less obvious error condition to debug.
            ThrowIfFalse(pContainer->GetContainingClassOrInterface() == NULL);

            // This type hasn't been defined in Metadata yet.
            Location hiddenLocation = Location::GetHiddenLocation();

            // This is a trick, asking for the TypeRef token will actually define type in metadata and return its TypeDef token.
            // This will also properly maintain all our hash tables. BTW, passed in location won't be used, I believe.
            mdTypeRef token = pMetaemitHelper->DefineTypeRefByContainer(pContainer, &hiddenLocation);
            ThrowIfFalse(TypeFromToken(token) == mdtTypeDef);
            AssertIfFalse(m_NoPiaTypeDefToTypeRefMap.GetValue(pContainer).m_DefinitionOrder == freeEntry);

            // Record it at free position;
            ThrowIfFalse(aTypes.Element(freeEntry) == NULL);
            aTypes.Element(freeEntry) = pContainer;
            freeEntry++;
        }
    }

    unsigned i;

    for(i=0; i<freeEntry; i++)
    {
        pContainer = aTypes.Element(i);

        if (pContainer != NULL)
        {
            DefinePiaType(pContainer, pMetaemitHelper);
        }
    }
    
    for(i=0; i<freeEntry; i++)
    {
        pContainer = aTypes.Element(i);

        if (pContainer != NULL)
        {
            DefineMembersOfPiaType(pContainer, pMetaemitHelper);
        }
    }
   
#if DEBUG
    // Verify that MetaEmit did not accidentally overwrite any of the imported token values.
    for (PiaTypeIterator iter = m_pCompilerProject->GetPiaTypeIterator(); iter.MoveNext();)
    {
        ValidateLocalTypeTokens(iter.Current(), tokenSaveList, m_pCompiler);
    }
    // We should have encountered all the items on the token list, with none left over.
    AssertIfFalse(0 == tokenSaveList.Count());
#endif
    // Bug #540163 occurred because we were not comprehensively capturing pia types referenced
    // in member signatures. A symptom of this problem was that we would end up with more type
    // references in the cache after emitting members than we had when we started. This should 
    // never happen: we expect that the pia type ref and member ref caches will not change during 
    // local type generation. This assert helps detect that condition, should some future work
    // happen to introduce a similar bug into MetaEmit. (See MetaEmit::CaptureMemberPiaTypeRefs.)
    unsigned int cCacheAfter = m_pCompilerProject->CountPiaCacheEntries();
    if (cCacheBefore != cCacheAfter)
    {
        AssertIfTrue(ERRID_InternalCompilerError);
        m_pErrorTable->CreateError(ERRID_InternalCompilerError, NULL);
    }
}

void PEBuilder::DefinePiaType
(
 BCSYM_Container *pContainer,
 MetaEmit *pMetaemitHelper
 )
{
    AssertIfNull(pContainer);
    AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(pContainer));

    // Will already have been flagged as error by CachePiaTypeRef
    AssertIfTrue(!pContainer->IsInterface() && !pContainer->IsDelegate() && !pContainer->IsStruct() && !pContainer->IsEnum());

    // Note there is no need to actually define type in Metadata because we are emiting TypeDefs on demand. (Dev10 #697784)

    const bool yesNoPIaEmbeddedSymbol = true;
    EmitAttributesAttachedToSymbol(pContainer, GetTypeDef(pContainer), NULL, pMetaemitHelper, yesNoPIaEmbeddedSymbol);
}

void PEBuilder::DefineMembersOfPiaType
(
    BCSYM_Container *pContainer,
    _Inout_ MetaEmit       *pMetaemitHelper
)
{
    AssertIfNull(pContainer);
    AssertIfNull(pMetaemitHelper);
    AssertIfFalse(TypeHelpers::IsEmbeddableInteropType(pContainer));
    const bool yesNoPIaEmbeddedSymbol = true;
    mdTypeDef tkContainer = GetTypeDef(pContainer);
    pMetaemitHelper->DefineGenericParamsForType(pContainer, tkContainer);

    mdTypeRef trExtends = mdTypeRefNil;
    if (pContainer->IsClass())
    {
        BCSYM *psymBase = pContainer->PClass()->GetBaseClass();
        trExtends = pMetaemitHelper->DefineTypeRefBySymbol(psymBase, pContainer->PClass()->GetInheritsLocation());
    }
    mdTypeRef *rgtrImplements = DefineInterfacesOfPiaType(pContainer, pMetaemitHelper);
    DWORD dwTypeDefFlags = 0;
    CComPtr<IMetaDataImport> pimdImport;

    // Dev10 #702321 Read typedef flags from Metadata, by this time they could be different from what
    // PEBuilder::GetTypeDefFlags returns because some custom attributes are translated into flags.
    IfFailThrow(m_pmdEmit->QueryInterface(IID_IMetaDataImport, (void **)&pimdImport));
    IfFailThrow(pimdImport->GetTypeDefProps(tkContainer, NULL, 0, NULL, &dwTypeDefFlags, NULL));

    pimdImport.Release();
    
    IfFailThrow(m_pmdEmit->SetTypeDefProps(tkContainer,         // [IN] The TypeDef
                                           dwTypeDefFlags,      // [IN] TypeDef flags
                                           trExtends,           // [IN] Base TypeDef or TypeRef
                                           rgtrImplements));    // [IN] Implemented interfaces
    if (pContainer->IsEnum())
    {
        // Special hack for enums: we have to emit a placeholder "value" field or the loader
        // will be unhappy about this type.
        BCSYM *Symbol = pContainer->SimpleBind(NULL, STRING_CONST(m_pCompiler, EnumValueMember));
        AssertIfNull(Symbol);
        AssertIfFalse(Symbol->IsVariable());
        pMetaemitHelper->DefineField(Symbol->PVariable());
    }

    // Iterate through the members of this type.
    // Check each member to see if it should be included in the localtype.
    // We have to emit methods first, and defer properties and events til later, since the
    // properties and events could refer back to methods defined previously.
    SafeInt<unsigned int> vtableGapSize = 0;
    int vtableGapIndex = 1;
    NorlsAllocator ScratchAllocator(NORLSLOC);
    Symbols SymbolFactory(m_pCompiler, &ScratchAllocator, NULL);
    DefineableMembersInAContainer methodIter(m_pCompiler, pContainer);
    while (BCSYM *Symbol = methodIter.Next())
    {
        // MQ Bug 808082
        // The condition (Symbol->IsSyntheticMethod() && Symbol->PProc()->IsEventAccessor()) is added
        //  to capture event accessors, for example addon.

        if ((Symbol->IsMethodDecl() && !Symbol->IsEventDecl()) ||
            (Symbol->IsSyntheticMethod() && Symbol->PProc()->IsEventAccessor()))
        {
            if (m_pCompilerProject->IdentifyPiaTypeMember(Symbol))
            {
                // Member order is determined by the location. We cannot produce accurately-ordered
                // vtables unless every member has a known location.
                AssertIfFalse(Symbol->HasLocation() || !pContainer->IsInterface());
                // If we have skipped over any members, emit a gap-filler entry.
                // This is a fake non-virtual method named "_VtblGapN_X", where N is a unique number
                // distinguishing the gap entry, and X represents the number of vtable slots to skip.
                if (vtableGapSize > 0)
                {
                    pMetaemitHelper->DefineVtableGap(pContainer, vtableGapIndex, vtableGapSize.Value());
                    vtableGapSize = 0;
                    vtableGapIndex++;
                }
                DefineMemberOfContainer(Symbol->PProc(), pMetaemitHelper);
                EmitAttributesOnProcAndParams(Symbol->PProc(), pMetaemitHelper, yesNoPIaEmbeddedSymbol);
            }
            else
            {
                bool vtableGapAdjusted = false;
                
                // Dev10 #659167
                // Vtable gap method is marked as DoesntRequireImplementation during MetaImport
                if (Symbol->PNamedRoot()->DoesntRequireImplementation())
                {
                    // From IMetaDataEmit::DefineMethod documentation (http://msdn.microsoft.com/en-us/library/ms230861(VS.100).aspx)
                    // ----------------------
                    // In the case where one or more slots need to be skipped, such as to preserve parity with a COM interface layout, 
                    // a dummy method is defined to take up the slot or slots in the v-table; set the dwMethodFlags to the mdRTSpecialName 
                    // value of the CorMethodAttr enumeration and specify the name as:
                    //
                    // _VtblGap<SequenceNumber><_CountOfSlots>
                    //
                    // where SequenceNumber is the sequence number of the method and CountOfSlots is the number of slots to skip in the v-table. 
                    // If CountOfSlots is omitted, 1 is assumed.
                    // ----------------------
                    //
                    // From "Partition II Metadata.doc"
                    // ----------------------
                    // For COM Interop, an additional class of method names are permitted:
                    // _VtblGap<SequenceNumber><_CountOfSlots>
                    // where <SequenceNumber> and <CountOfSlots> are decimal numbers
                    // ----------------------
                    
                    wchar_t * name = Symbol->PNamedRoot()->GetName();
                    const wchar_t prefix[] = L"_VtblGap";
                    const size_t prefixLength = _countof(prefix)-1;
                    const int base10 = 10;

                    if (name != NULL && wcslen(name) > prefixLength && 
                        memcmp(name, prefix, prefixLength * sizeof(wchar_t)) == 0 &&
                        name[prefixLength] >= L'0' && name[prefixLength] <= L'9')
                    {
                        name += (prefixLength + 1);

                        // Skip the SequenceNumber
                        while (*name >= L'0' && *name <= L'9')
                        {
                            name++;
                        }

                        if (*name == NULL)
                        {
                            vtableGapAdjusted = true;
                            vtableGapSize++;
                        }
                        else if (*name == L'_' && name[1] >= L'0' && name[1] <= L'9')
                        {
                            //Get CountOfSlots
                            wchar_t * endptr = NULL;
                            unsigned long countOfSlots = wcstoul(name+1,&endptr,base10);

                            // there is no reason to specially check for overflow here bacause
                            // in case of overflow countOfSlots==ULONG_MAX, which will cause overflow on increment.
                            if(*endptr == NULL && countOfSlots > 0) 
                            {
                                vtableGapAdjusted = true;
                                vtableGapSize += countOfSlots;
                            }         
                        }
                    }
                }
                
                if (!vtableGapAdjusted)
                {
                    vtableGapSize++;
                }
            }
        }
    }

    // Now that we are done emitting methods, emit all of the other members.
    DefineableMembersInAContainer otherIter(m_pCompiler, pContainer);
    while (BCSYM *Symbol = otherIter.Next())
    {
#if IDE 
        // When building debug versions in the IDE, the compiler will insert some extra members
        // that support ENC. These make no sense in local types, so we will skip them. We have to
        // check for them explicitly or they will trip the member-validity check that follows.
        if (Symbol->IsSyntheticMethod())
        {
            continue;
        }
        if (Symbol->IsVariable() && Symbol->PVariable()->IsENCTrackingList())
        {
            continue;
        }
#endif
        // Will already have been flagged as error by CachePiaTypeMemberRef
        AssertIfTrue(pContainer->IsStruct() && !pContainer->IsEnum() && 
            (!Symbol->IsVariable() || Symbol->PVariable()->IsShared() || Symbol->PVariable()->GetAccess() != ACCESS_Public));

        // Skip method declarations - we emitted them in the previous loop.
        if (Symbol->IsMethodDecl() && !Symbol->IsEventDecl()) 
        {
            continue;
        }
        // We only include those members which CompilerProject thinks we should include.
        if (!m_pCompilerProject->IdentifyPiaTypeMember(Symbol))
        {
            continue;
        }
        BCSYM_NamedRoot *psr = Symbol->PNamedRoot();
        DefineMemberOfContainer(psr, pMetaemitHelper);
        EmitAttributesAttachedToSymbol(Symbol, GetToken(psr), NULL, pMetaemitHelper, yesNoPIaEmbeddedSymbol);
    }

    if (pContainer->IsStruct())
    {
        // If this is a structure which uses explicit layout and or special pack/class size, we must call
        // SetClassLayout.
        pMetaemitHelper->DefineClassLayout(pContainer, tkContainer, tdExplicitLayout == (dwTypeDefFlags & tdLayoutMask));
    }
}

mdTypeRef *PEBuilder::DefineInterfacesOfPiaType
(
 BCSYM_Container *pContainer,
 MetaEmit *pMetaemitHelper
 )
{
    mdTypeRef *rgtrImplements = NULL;
    AssertIfNull(pContainer);
    if (pContainer->IsInterface())
    {
        // Emit typerefs for all these interface implementations.
        BCITER_ImplInterfaces implList(pContainer->PInterface());
        rgtrImplements = pMetaemitHelper->DefineImplements(&implList, mdTokenNil);
    }
    return rgtrImplements;
}

//============================================================================
// Define one member of a container.  The cached file and its hash
// table must be set up before this call.
//============================================================================

void PEBuilder::DefineMemberOfContainer
(
    BCSYM_NamedRoot *pMember,
    _Inout_ MetaEmit       *pMetaemitHelper
)
{
    BCSYM *pNonAliasedSymbol = pMember->DigThroughAlias();

    BCSYM_Container *pContainer =
        pNonAliasedSymbol->IsNamedRoot() ? pNonAliasedSymbol->PNamedRoot()->GetContainer() : NULL;

    if (pNonAliasedSymbol->IsProc())
    {
        if (pNonAliasedSymbol->IsProperty())
        {
            pMetaemitHelper->DefineProperty(pNonAliasedSymbol->PProperty());
        }
        else if (pNonAliasedSymbol->IsEventDecl())
        {
            pMetaemitHelper->DefineEvent(pNonAliasedSymbol->PEventDecl());
        }
        else
        {
#if IDE 
            if (!pNonAliasedSymbol->PProc()->IsENCSynthFunction())
#endif
            {
                pMetaemitHelper->DefineMethod(pNonAliasedSymbol->PProc());
            }
        }
    }
    else if (pNonAliasedSymbol->IsVariable() &&
             (!pContainer->IsEnum() || pNonAliasedSymbol->PVariable()->GetVarkind() == VAR_Const)) // skip enum's "value__"
    {
        pMetaemitHelper->DefineField(pNonAliasedSymbol->PVariable());
    }
}

//============================================================================
// Define the members of a com class.  The cached file and its hash
// table must be set up before this call. DefineMembersOfContainer should
// be called before this call.
//============================================================================

void PEBuilder::DefineMembersOfComClassInterfaces
(
    BCSYM_Class *pClass,
    _Inout_ MetaEmit     *pMetaemitHelper
)
{
    // Check if we have a com class interface to work with.
    mdToken tkClassInterface = GetComClassToken(pClass);
    if (IsNilToken(tkClassInterface))
    {
        // If we don't have a class interface,
        // we shouldn't have an events interface either.
        VSASSERT(IsNilToken(GetComClassEventsToken(pClass)), "there should be no events interface token--it is ignored!");
        return;
    }

    mdToken tkClass = GetTypeDef(pClass);
    mdToken tkEventInterface = GetComClassEventsToken(pClass);

    VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

    //
    // Define all of the methods of the com class interface.
    //
    {
        BCSYM_NamedRoot *pnamed;

        ComClassMembersInAContainer iter(pClass);

        while (pnamed = iter.Next())
        {
            if (pnamed->GetBindingSpace())
            {
                BCSYM *pNonAliasedSymbol = pnamed->DigThroughAlias();

                if (pNonAliasedSymbol->IsProperty())
                    continue;   // We'll emit all the properties after the accessor members are emitted

                if (pNonAliasedSymbol->IsProc())
                {
                    // Emit the member.
                    // 
                    pMetaemitHelper->DefineComClassInterfaceMethod(pnamed, tkClassInterface);
                }
            }
        }

        //
        // Define all of the properties of the interface.  Custom attributes attached
        // to properties are emitted here.
        //
        iter.Reset();

        while (pnamed = iter.Next())
        {
            if (pnamed->GetBindingSpace())
            {
                BCSYM *pNonAliasedSymbol = pnamed->DigThroughAlias();

                if (pNonAliasedSymbol->IsProperty())
                {
                    // 
                    pMetaemitHelper->DefineComClassInterfaceProperty(pnamed, tkClassInterface);
                }
            }
        }
    }


    //
    // Define the members of the com class events interface
    //
    if (!IsNilToken(tkEventInterface))
    {
        BCSYM_NamedRoot *pnamed;

        ComClassEventsInAContainer iter(pClass);

        while (pnamed = iter.Next())
        {
            if (pnamed->GetBindingSpace())
            {
                BCSYM *pNonAliasedSymbol = pnamed->DigThroughAlias();

                if (pNonAliasedSymbol->IsEventDecl())
                {
                    pMetaemitHelper->DefineComClassInterfaceMethod(pnamed, tkEventInterface);
                }
            }
        }
    }
}


//============================================================================
// Generates bound trees for the provided container and does some error
// checking. This method was split off from CompileMethodsOfContainer to
// support creating synthetic types during code generation.
//============================================================================
bool PEBuilder::GenerateBoundTreesForContainer
(
    BCSYM_Container * pContainer,
    Text * pText,
    _Inout_ PEInfo * pInfo,
    _Out_ MetaEmit * pMetaemitHelper,
    NorlsAllocator * pNraBoundTrees,
    _Inout_ DynamicArray<CodeGenInfo>* pCodeGenInfos
)
{
    bool fAborted = false;
    Text *pSavedText = pText;
    SourceFile * pSourceFile = pContainer->GetSourceFile();
    SourceFile *pSavedSourceFile = pSourceFile;
    ErrorTable * pErrorTable = pSourceFile->GetErrorTable();

    NorlsAllocator nraCycles(NORLSLOC);
    Cycles cycle(m_pCompiler, &nraCycles, pErrorTable, ERRID_SubNewCycle1, ERRID_SubNewCycle2);

    CallGraph callTracking(m_pCompiler);
    CallGraph * pCallTracking = NULL;
    DynamicArray<BCSYM_Proc *> daConstructors;

    bool isDesignerClass = pContainer->IsClass() && Semantics::IsDesignerControlClass(pContainer->PClass()); // We track this for forms, etc. to make sure that InitializeComponent eventually gets called
    if (isDesignerClass)
    {
        pCallTracking = &callTracking;
    }

    BCSYM_Proc *pProc = NULL;
    BCSYM_Proc *pInitializeComponent = NULL;

    Text TempText;
    CompileableMethodsInAContainer iter(pContainer);
    while (pProc = iter.Next())
    {
        // Transients are processed separately. So need to skip here to
        // avoid duplication.
        if (pProc->IsTransient())
        {
            continue;
        }

#if IDE 
        if (pProc->IsENCSynthFunction())
        {
            continue;
        }

#endif

        SourceFile *pSourceFileContainingMethod = pProc->GetSourceFile();

        if (pSourceFile != pSourceFileContainingMethod)
        {
            if (pSourceFileContainingMethod != pSavedSourceFile)
            {
                pSourceFile = pSourceFileContainingMethod;
                pErrorTable = pSourceFile->GetCurrentErrorTable();

                pText = &TempText;
                // 


                IfFailThrow(pText->Init(pSourceFile));
            }
            else
            {
                pSourceFile = pSavedSourceFile;
                pErrorTable = pSourceFile->GetCurrentErrorTable();
                pText = pSavedText;
            }
        }

        ILTree::ILNode * pTree =
            GenerateBoundTreeForProc(
                pProc,
                pContainer,
                pText,
                pInfo,
                pMetaemitHelper,
                pNraBoundTrees,
                pCodeGenInfos,
                &cycle,
                pCallTracking);
#if IDE
        // See if we need to stop this compilation.
        if (CheckStop(NULL)) // if the foreground thread wants to stop the bkd, then let's abort so IDE is more responsive
        {
            fAborted = true;
            goto Abort;
        }
#endif IDE

        if (pTree && isDesignerClass)
        {
            if (pProc->IsInstanceConstructor() && !pProc->IsSyntheticMethod())
            {
                daConstructors.AddElement(pProc);
            }

            if (Semantics::IsInitializeComponent(m_pCompiler, pProc))
            {
                pInitializeComponent = pProc;
            }
        }

        if (!pInfo->m_hasErrors && !m_pCompilerProject->OutputIsNone())
        {
            // Handle any implemented interfaces.
            pMetaemitHelper->DefineInterfaceImplementation(pProc);
        }
    }

    // Report InitializeComponent error
    if (isDesignerClass && pInitializeComponent)
    {
        ULONG iIndex = 0;

        for (iIndex = 0; iIndex < daConstructors.Count(); iIndex++)
        {
            pProc = daConstructors.Element(iIndex);

            if (!callTracking.DoesPathExist(&pProc, &pInitializeComponent))
            {
                StringBuffer sbConstructor;

                pProc->GetBasicRep(
                    m_pCompiler,
                    pContainer,
                    &sbConstructor);

                pProc->GetErrorTableForContext()->CreateErrorWithSymbol(
                    WRNID_ExpectedInitComponentCall2,
                    pProc,
                    sbConstructor.GetString(),
                    pContainer->GetQualifiedName(false));
            }
        }
    }

    // Find constructor call cycles.
    cycle.FindCycles(false, true);

#if IDE 
Abort:
#endif IDE

    return fAborted;
}

ILTree::ILNode *PEBuilder::GenerateBoundTreeForProc
(
    BCSYM_Proc *pProc,
    BCSYM_Container * pContainer,
    Text * pText,
    _Inout_ PEInfo * pInfo,
    MetaEmit * pMetaemitHelper,
    NorlsAllocator * pNraBoundTrees,
    _Inout_ DynamicArray<CodeGenInfo>* pCodeGenInfos,
    Cycles *pCycle,
    CallGraph *pCallTracking
)
{
    SourceFile * pSourceFile = pProc->GetSourceFile();
    ErrorTable * pErrorTable = pSourceFile->GetCurrentErrorTable();

    ILTree::ILNode * pTree = NULL;

    if (pProc->IsMethodImpl() || pProc->IsSyntheticMethod())
    {
#if IDE 
        Symbols::SetImage(pProc, NULL, 0);
#endif

        if (pProc->GetBindingSpace() != BINDSPACE_IgnoreSymbol)
        {
            pTree = pProc->GetBoundTree();  // For closure methods, bound tree is already generated.
                                            // Not problem with decompilation because the lifetime
                                            // of the closure method (transient symbols) is very short.

            if (!pTree)
            {
                IfFailThrow(
                    pSourceFile->GetBoundMethodBodyTrees(
                        pProc,
                        pNraBoundTrees,
                        pErrorTable,
                        pCycle,
                        pCallTracking,
                        &m_Transients,
                        pText,
                        NULL, // IDE CompilationCaches
                        gmbRecordDependencies | gmbMergeAnonymousTypes,
                        &pTree,
                        NULL,
                        pProc->IsAnyConstructor() && pContainer->GetNextPartialType() ?     // Code in constructors in partial types
                            SourceFile::GetCurrentErrorTableForFile :                           // could be from different files. Eg: Member initializers
                            NULL));
#if IDE    
                if (CheckStop(NULL)) // if the foreground thread wants to stop the bkd, then let's abort so IDE is more responsive
                {
                    return NULL;
                }
#endif IDE
            }

#ifdef DEBUG

            // Microsoft:
            // We've had a few bugs where errors don't get generated in when interpreting
            // the method body, but we neglect to generate errors.
            // For now, just put a debug check. We should make this a real error in VS10
            // once we think about how we want to go about doing this.

            // Walk through the tree to check whether the nodes have an error.
            bool hasErrors = pErrorTable->HasErrors() || pInfo->m_hasErrors;

            // Dev10 #514963
            // If we don't have an error and this is a constructor for a Partial class, 
            // see if files containing other partial declarations have any errors.
            if (!hasErrors && pProc->IsAnyConstructor())
            {
                BCSYM_Container *pPartialType = pContainer->IsPartialTypeAndHasMainType();

                if (pPartialType == NULL)
                {
                    pPartialType = pContainer;
                }

                while (pPartialType != NULL)
                {
                    SourceFile * pPartialSourceFile = pPartialType->GetSourceFile();
                    ErrorTable * pPartialErrorTable = pPartialSourceFile ? pPartialSourceFile->GetCurrentErrorTable() : NULL;

                    if (pPartialErrorTable && pPartialErrorTable->HasErrors())
                    {
                        hasErrors = true;
                        break;
                    }

                    pPartialType = pPartialType->GetNextPartialType();
                }

            }

            //VSASSERT(hasErrors || pTree, L"We should have errors if pTree is null");
            if (!hasErrors && pTree) // pTree was null in File->New Web Site
            {
                BoundTreeBadnessVisitor visitor;
                visitor.Visit(&pTree->AsProcedureBlock());
                bool bMethodBodyHasBadNodes = visitor.TreeIsBad();

                VSASSERT(
                    !bMethodBodyHasBadNodes,
                    L"We have no errors, but we have bad nodes in the tree.");
            }

#endif

            // If this method has DllImportAttribute attached, its body must be empty
            if (pProc->GetPWellKnownAttrVals()->GetDllImportData() && pTree && !pTree->AsProcedureBlock().fEmptyBody)
            {
                // most of the illegal uses of DllImport already reported by VerifyAttributeUssage. Avoid duplicate errors
                if (!(pProc->IsDllDeclare() || pProc->IsPropertyGet() || pProc->IsPropertySet() ||
                    pProc->CreatedByEventDecl() || pProc->GetContainer()->IsInterface() ||
                    IsGenericOrHasGenericParent(pProc) || !pProc->IsShared() ||
                    pProc->IsAsyncKeywordUsed() || pProc->IsIteratorKeywordUsed()))
                {
                    pErrorTable->CreateErrorWithSymbol(ERRID_DllImportOnNonEmptySubOrFunction, pProc);
                }
            }

            // GetBoundMethodBodyTrees will return NULL if we're unable
            // to load the file.  Do nothing.

            if (pTree)
            {
                if (!pProc->IsMustOverrideKeywordUsed() &&
                    !pProc->IsProperty() &&
                    !pProc->IsEventDecl() &&
                    !pProc->IsMethodCodeTypeRuntime() &&
                    !pProc->GetPWellKnownAttrVals()->GetDllImportData() &&
                    !(pProc->GetContainingClass()->GetPWellKnownAttrVals()->GetComImportData() && pTree->AsProcedureBlock().fEmptyBody))
                {
                    pCodeGenInfos->AddElement(CodeGenInfo(pTree, pProc, pSourceFile));
                }
            }
            else
            {
                pInfo->m_hasErrors = true;
            }

            // Do we have errors now?
            // Need to count the errors in the current error table and the alternate error tables
            // because semantic errors could possibly be reported in other error tables when the
            // code for the method comes from different files. Eg. Member init code in constructors
            // when the members are declared in different files.
            //
            if (!pInfo->m_hasErrors)
            {
                VSASSERT(pContainer->GetMainType() == NULL, "Non-Main container unexpected!!!");

                if (pErrorTable->HasErrors())
                {
                    pInfo->m_hasErrors = true;
                }
                else if (pProc->IsAnyConstructor() &&
                        pContainer->GetNextPartialType())
                {
                    for(BCSYM_Container *pCurrentPartial = pContainer;
                        pCurrentPartial;
                        pCurrentPartial = pCurrentPartial->GetNextPartialType())
                    {
                        ErrorTable *CurrentErrors = SourceFile::GetCurrentErrorTableForFile(pCurrentPartial->GetSourceFile());

                        if (CurrentErrors != pErrorTable &&
                            CurrentErrors->HasErrors())
                        {
                            pInfo->m_hasErrors = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    return pTree;
}

bool PEBuilder::GenerateCodeForContainer
(
    _In_ PEInfo * pInfo,
    _Inout_ MetaEmit *pMetaemitHelper,
    _In_ DynamicArray<CodeGenInfo>* pCodeGenInfos
)
{
    bool fAborted = false;

    if (!pInfo->m_hasErrors && !m_pCompilerProject->OutputIsNone())
    {
        BCSYM_Proc * pProc = NULL;
        ILTree::ILNode * pTree = NULL;
        NorlsAllocator * pNraSymbolStorage = NULL;

        for (ULONG index = 0; index < pCodeGenInfos->Count(); ++index)
        {
#if IDE 
            if (m_pCompilerProject->IsDebuggingInProgress())
            {
                m_pCompilerProject->m_fILEmitTasksSkippedDuringDebugging = true;
            }
            else
#endif IDE
            {
#if IDE 
                    IfTrueAbort(CheckStop(NULL));
#endif

                BackupValue<ErrorTable*>backupErrorTable(&m_pErrorTable);

                CodeGenInfo pCurCodeGenInfo = pCodeGenInfos->Element(index);
                if (pCurCodeGenInfo.m_pSourceFile != NULL && pCurCodeGenInfo.m_pSourceFile->GetCurrentErrorTable() != m_pErrorTable)
                {
                  m_pErrorTable = pCurCodeGenInfo.m_pSourceFile->GetCurrentErrorTable();
                }    
            
                NorlsAllocator nraCodeGen (NORLSLOC);

                pProc = pCurCodeGenInfo.m_pProc;

                // If the current procedure is a partial method declaration, don't emit
                // any code for it.

                if( pProc->IsPartialMethodDeclaration() )
                {
                    continue;
                }

                pTree = pCodeGenInfos->Element(index).m_pBoundTree;
                pNraSymbolStorage = pCodeGenInfos->Element(index).m_pSourceFile->SymbolStorage();

                // Start a new signature for method
                NewMetaEmitSignature signature(pMetaemitHelper);

                VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

                CodeGenerator codegen(m_pCompiler, &nraCodeGen, pNraSymbolStorage, pMetaemitHelper, NULL);
                BYTE *pbImage;
                unsigned cbImage;

                // Generates the IL for a method.
                codegen.GenerateMethod(pTree);

                // Get the resulting size of that IL.
                cbImage = codegen.GetImageSize();

                // Allocate the memory to store it in.  The IDE stores it in
                // temporary memory while the command-line compiler
                // allocates it directly into the PE.
                //
#if IDE 
                pbImage = (BYTE *)pNraSymbolStorage->Alloc(cbImage);
                Symbols::SetImage(pProc, pbImage, cbImage);
#else !IDE
                pbImage = AllocatePEStorageForImage(pInfo, pProc, cbImage);
#endif !IDE

                // Fill in the memory.
                codegen.EmitImage(pbImage);
                if (pProc->IsMethodImpl())
                {
                    pProc->PMethodImpl()->SetSignatureToken( pMetaemitHelper->GetSignatureToken() );
                }
                else if (pProc->IsSyntheticMethod())
                {
                    pProc->PSyntheticMethod()->SetSignatureToken( pMetaemitHelper->GetSignatureToken() );

                    if (pProc->GetSourceFile() && 
                        (pProc->PSyntheticMethod()->IsLambda() ||
                        pProc->PSyntheticMethod()->IsResumable()))
                    {
                        m_SyntheticMethods[pProc->GetSourceFile()].push_back(pProc->PSyntheticMethod());
                    }

                }
                pMetaemitHelper->ResetSignatureToken();

                // Clear the exceptions table for next method
                pMetaemitHelper->ClearExceptions();

                backupErrorTable.Restore();
            }
        }
    }

#if IDE 
Abort:
#endif IDE

    return fAborted;
}

bool PEBuilder::ProcessTransientSymbols
(
    BCSYM_Container * pContainer,
    Text * pText,
    _Inout_ PEInfo * pInfo,
    _Inout_ MetaEmit *pMetaemitHelper,
    NorlsAllocator * pNraBoundTrees,
    _Inout_ DynamicArray<CodeGenInfo>* pCodeGenInfos
)
{
    bool fAborted = false;

    // Skip processing transients if there are any errors.
    // This will avoid referring to bad compiler symbols
    // and also to possibly incomplete closures.
    //
    if (pInfo->m_hasErrors)
    {
        return false;
    }

#if IDE 
    m_bProcessingTransients = true;
#endif IDE

    CLinkRangeIter<TransientSymbol> iter(m_Transients.TransactionSymbolIterator());
    TransientSymbol *pTransientSymbol = NULL;

    // Emit the all the transient types first in case any of the members,
    // bases or inherits depend on them.

    while (pTransientSymbol = iter.Next())
    {
        AssertIfNull(pTransientSymbol->m_pTransient);

        if (pTransientSymbol->m_pTransient->IsClass() ||
            pTransientSymbol->m_pTransient->IsInterface())
        {
            DefineContainer(pTransientSymbol->m_pTransient->PContainer());
        }
    }

    // Emit the metadata for all the transient types.

    iter.Reset();
    while (pTransientSymbol = iter.Next())
    {
        AssertIfNull(pTransientSymbol->m_pTransient);

        if (pTransientSymbol->m_pTransient->IsClass() ||
            pTransientSymbol->m_pTransient->IsInterface())
        {
            DefineMembersOfContainer(pTransientSymbol->m_pTransient->PContainer(), pMetaemitHelper);
        }
    }

    // Emit the metadata for any transients that were not part of the
    // above transient types.
    //
    // Given the number of transient that will be present for any container,
    // iterating again here should not cause any noticeable perf issues.

    iter.Reset();
    while (pTransientSymbol = iter.Next())
    {
        AssertIfNull(pTransientSymbol->m_pTransient);

        if (!pTransientSymbol->m_pTransient->IsContainer())
        {
            BCSYM_NamedRoot *pMember = pTransientSymbol->m_pTransient;

            if (IsNilToken(pMember->GetToken()) &&                      // Not yet emitted
                (!pMember->GetContainer()->IsTransientClass() ||        // Member is non-transient class, so need to emit now
                 !IsNilToken(pMember->GetContainer()->GetToken())))     // Member in transient class that has already been emitted,
                                                                        // so need to emit member now.
            {
                if (pMember->IsProperty())
                {
                    if (pMember->PProperty()->GetProperty() &&
                        IsNilToken(pMember->PProperty()->GetProperty()->GetToken()))
                    {
                        DefineMemberOfContainer(pMember->PProperty()->GetProperty(), pMetaemitHelper);
                    }

                    if (pMember->PProperty()->SetProperty() &&
                        IsNilToken(pMember->PProperty()->SetProperty()->GetToken()))
                    {
                        DefineMemberOfContainer(pMember->PProperty()->SetProperty(), pMetaemitHelper);
                    }
                }
                else if (pMember->IsEventDecl())
                {
                    if (pMember->PEventDecl()->GetProcAdd() &&
                        IsNilToken(pMember->PEventDecl()->GetProcAdd()->GetToken()))
                    {
                        DefineMemberOfContainer(pMember->PEventDecl()->GetProcAdd(), pMetaemitHelper);
                    }

                    if (pMember->PEventDecl()->GetProcRemove() &&
                        IsNilToken(pMember->PEventDecl()->GetProcRemove()->GetToken()))
                    {
                        DefineMemberOfContainer(pMember->PEventDecl()->GetProcRemove(), pMetaemitHelper);
                    }

                    if (pMember->PEventDecl()->GetProcFire() &&
                        IsNilToken(pMember->PEventDecl()->GetProcFire()->GetToken()))
                    {
                        DefineMemberOfContainer(pMember->PEventDecl()->GetProcFire(), pMetaemitHelper);
                    }
                }

                DefineMemberOfContainer(pMember, pMetaemitHelper);
            }
        }
    }


    // Generate bound trees for the transient methods

    SourceFile *pSourceFile = pContainer->GetSourceFile();
    ErrorTable *pErrorTable = pSourceFile->GetErrorTable();
    NorlsAllocator nraCycles(NORLSLOC);
    Cycles cycle(m_pCompiler, &nraCycles, pErrorTable, ERRID_SubNewCycle1, ERRID_SubNewCycle2);
    const bool isNoPIaEmbeddedSymbol = false;

#if DEBUG
    int transientCount = m_Transients.SymbolCount();
#endif

    iter.Reset();
    while (pTransientSymbol = iter.Next())
    {
        AssertIfNull(pTransientSymbol->m_pTransient);

        if (pTransientSymbol->m_pTransient->IsClass())
        {
            SourceFile *pFile = pTransientSymbol->m_pTransient->GetSourceFile();

            IfTrueAbort(
                GenerateBoundTreesForContainer(
                    pTransientSymbol->m_pTransient->PClass(),
                    NULL,
                    pInfo,
                    pMetaemitHelper,
                    pNraBoundTrees,
                    pCodeGenInfos));

            VSASSERT(m_Transients.SymbolCount() == transientCount, "Transient classes should not cause other transients to be created");
        }
        else if (pTransientSymbol->m_pTransient->IsProc())
        {
            SourceFile *pFile = pTransientSymbol->m_pTransient->GetContainer()->GetSourceFile();

            GenerateBoundTreeForProc(
                pTransientSymbol->m_pTransient->PProc(),
                pTransientSymbol->m_pTransient->GetContainer(),
                NULL,
                pInfo,
                pMetaemitHelper,
                pNraBoundTrees,
                pCodeGenInfos,
                &cycle,
                NULL);

            VSASSERT(m_Transients.SymbolCount() == transientCount, "Transient procs should not cause other transients to be created");
        }
    }

    // Emit attributes
    iter.Reset();
    while (pTransientSymbol = iter.Next())
    {
        AssertIfNull(pTransientSymbol->m_pTransient);

        if (pTransientSymbol->m_pTransient->IsClass())
        {
            BCSYM_Class *pClass = pTransientSymbol->m_pTransient->PClass();

            // Emit custom attributes attached anywhere inside this class
            EmitAttributes(pClass, pInfo, pMetaemitHelper);
        }
        else if(pTransientSymbol->m_pTransient->IsProc())
        {
#if IDE 
            if (!pTransientSymbol->m_pTransient->PProc()->IsENCSynthFunction())
#endif
            {
                BCSYM_Proc *pProc = pTransientSymbol->m_pTransient->PProc();

                EmitAttributesOnProcAndParams(pProc, pMetaemitHelper, isNoPIaEmbeddedSymbol);
            }
        }
    }

Abort:

#if IDE 
    m_bProcessingTransients = false;
#endif IDE

    return fAborted;
}

bool PEBuilder::CompileMethodsOfContainer
(
    BCSYM_Container *pContainer,
    Text *pTextInSourceFile,
    _Inout_ PEInfo *pInfo,
    _Inout_ MetaEmit *pMetaemitHelper
)
{
    bool fAborted = false;
    NorlsAllocator nraBoundTrees(NORLSLOC);
    DynamicArray<CodeGenInfo> codeGenInfos;

    // Open transaction on transient symbol store so that changes can be rolled back on Abort
    m_Transients.StartTransaction();

    IfTrueAbort(
        GenerateBoundTreesForContainer(
            pContainer,
            pTextInSourceFile,
            pInfo,
            pMetaemitHelper,
            &nraBoundTrees,
            &codeGenInfos));

    IfTrueAbort(
        ProcessTransientSymbols(
            pContainer,
            pTextInSourceFile,
            pInfo,
            pMetaemitHelper,
            &nraBoundTrees,
            &codeGenInfos));

    IfTrueAbort(
        GenerateCodeForContainer(
            pInfo,
            pMetaemitHelper,
            &codeGenInfos));

Abort:

    if (fAborted)
    {
        // Rollback transient symbols created during scope of this container
        m_Transients.AbortTransaction();
    }
    else
    {
        // Commit transient symbols created during scope of this container
        m_Transients.CommitTransaction();
    }

    return fAborted;
}

BCSYM_ApplAttr * PEBuilder::FindFirstAttributeWithType(BCSYM_Param * pParam, BCSYM_NamedRoot * pAttributeType)
{
    VSASSERT(pParam, "FindAttribute was supplied a null parameter.");
    VSASSERT(pAttributeType, "FindAttri----e was supplied a null attribute type.");

    if (pParam && pAttributeType)
    {
        AttrVals * pAttrVals = pParam->GetPAttrVals();
        if (pAttrVals)
        {
            for (BCSYM_ApplAttr * pRet = pParam->GetPAttrVals()->GetPsymApplAttrHead(); pRet; pRet = pRet->GetNext())
            {
                if (BCSYM::AreTypesEqual(pRet->GetAttributeSymbol(), pAttributeType))
                {
                    return pRet;
                }
            }
        }

    }
    return NULL;
}

//============================================================================
// Emit custom attributes attached to this proc and its params.
//============================================================================
void PEBuilder::EmitAttributesOnProcAndParams
(
    BCSYM_Proc *pproc,
    _Inout_ MetaEmit *pMetaemitHelper,
    bool isNoPIaEmbeddedSymbol // Is this an embedded symbol for NoPia functionality?
)
{
    if (pproc != NULL)
    {
        BCSYM_Param *pparam;

        // Handle the proc itself, synthetic methods can have attributes
        EmitAttributesAttachedToSymbol(pproc, GetToken(pproc), NULL, pMetaemitHelper, isNoPIaEmbeddedSymbol);

        // Synthesized procs usually do not have attributes on their parameters
        // but auto properties can have attributes on their return types.
        if (!pproc->IsSyntheticMethod())
        {
            bool bFirst = true;
            BCSYM_Property * pProperty;
            BCSYM_Param * pReturnParam;

            // Handle its parameters
            for (pparam = pproc->GetFirstParam(); pparam; pparam = pparam->GetNext())
            {
                //Microsoft - VSWhidbey #270648
                //We need to propigate MarshalAs attributes
                //from the return type of a property defined in an interface
                //to the "Value" parameter on its setter.
                if
                (
                    bFirst &&
                    pproc->IsPropertySet() &&
                    (pProperty = pproc->GetAssociatedPropertyDef()) &&
                    pproc->GetContainingClassOrInterface()->IsInterface() &&
                    (pReturnParam = pProperty->GetReturnTypeParam()) &&
                    pReturnParam->GetPWellKnownAttrVals()->GetMarshalAsData()
                )
                {
                    NorlsAllocator alloc(NORLSLOC);
                    // The call to GetFXSymbolProvider()->GetType(FX::MarshalAsType) can return NULL
                    // (not be defined for the current platform) and be handled correctly in the logic below.
                    BCSYM_ApplAttr * pMarshalAs = FindFirstAttributeWithType(
                        pProperty->GetReturnTypeParam(),
                        m_pCompilerProject->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::MarshalAsType));

                    VSASSERT(pMarshalAs, "Unabled to find the MarshalAs attribute on the property's return type. This should not be possible because GetMarshalAsData() returned true.");

                    if (pMarshalAs)
                    {
                        EmitCustomAttribute
                        (
                            pparam,
                            GetToken(pparam, pproc),
                            pMarshalAs,
                            pProperty->GetErrorTableForContext(),
                            pMetaemitHelper,
                            &alloc,
                            NULL,
                            NULL
                        );
                    }
                }
                bFirst = false;

                EmitAttributesAttachedToSymbol(pparam, GetToken(pparam, pproc), pproc, pMetaemitHelper, isNoPIaEmbeddedSymbol);
            }
        }

        // Handle the return type
        if ( pproc->GetReturnTypeParam() && 
             ( !pproc->IsSyntheticMethod() ||
               (pproc->PSyntheticMethod()->GetSyntheticKind() == SYNTH_AutoPropertyGet) ||
               pproc->PSyntheticMethod()->IsLambda())
           )
        {
            EmitAttributesAttachedToSymbol(pproc->GetReturnTypeParam(),
                                           GetToken(pproc->GetReturnTypeParam(), pproc),
                                           pproc,
                                           pMetaemitHelper,
                                           isNoPIaEmbeddedSymbol);
        }
    }
}

//============================================================================
// Emit custom attributes attached to this container or its contents
//============================================================================

bool PEBuilder::EmitAttributes
(
    BCSYM_Container *pContainer,
    _Inout_ PEInfo          *pInfo,
    _Inout_ MetaEmit         *pMetaemitHelper
)
{
    bool             fAborted = false;
    BCSYM_NamedRoot *psymChild;
    BCITER_CHILD_ALL iter(pContainer);

    AssertIfTrue(TypeHelpers::IsEmbeddableInteropType(pContainer));
    const bool isNoPIaEmbeddedSymbol = false;
    
    // Emit attributes attached to this container.
    EmitAttributesAttachedToSymbol(pContainer, GetTypeDef(pContainer), NULL, pMetaemitHelper, isNoPIaEmbeddedSymbol);

    // Process the contents of this container
    while (psymChild = iter.GetNext())
    {
        psymChild = psymChild->DigThroughAlias()->PNamedRoot();

        // Ignore containers, top level iterator will get to those eventually.
        if (psymChild->IsContainer())
        {
            continue;
        }

        if (psymChild->IsEventDecl())
        {
            EmitAttributesOnProcAndParams(psymChild->PProc(), pMetaemitHelper, isNoPIaEmbeddedSymbol);
        }
        else if (psymChild->IsProperty())
        {
            BCSYM_Property *pProperty = psymChild->PProperty();

            EmitAttributesAttachedToSymbol(psymChild, GetToken(psymChild), NULL, pMetaemitHelper, isNoPIaEmbeddedSymbol);
        }
        else if (psymChild->IsProc())
        {
#if IDE 
            if (!psymChild->PProc()->IsENCSynthFunction())
#endif
            {
                BCSYM_Proc *pProc = psymChild->PProc();

                EmitAttributesOnProcAndParams(pProc, pMetaemitHelper, isNoPIaEmbeddedSymbol);

                // If this is the startup token, emit STAThreadAttribute (unless STA
                // or MTA is already specified)
                //UV Support: Do note emit if using the starlite libraries.
                if (pProc == pInfo->m_pProcEntryPoint &&
                    !pProc->GetPWellKnownAttrVals()->GetSTAThreadData() &&
                    !pProc->GetPWellKnownAttrVals()->GetMTAThreadData() &&
                    !m_pCompilerProject->GetCompilerHost()->IsStarliteHost())
                {
                    if (IsNilToken(GetToken(pProc)))
                    {
                        VSASSERT(pInfo->m_hasErrors,
                            "How can the startup token be nil if there are no compilation errors?");
                    }
                    else 
                    {
                        VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

                        //Microsoft Some platforms don't define this attribute. Treat this attribute as optional.
                        mdMemberRef staCtorTok = pMetaemitHelper->DefineRTMemberRef(STAThreadAttributeCtor);

                        if (!IsNilToken(staCtorTok))
                        {
                            pMetaemitHelper->DefineCustomAttribute(GetToken(pProc), staCtorTok);
                        }
                    }
                }
            }
        }
        else if (psymChild->IsVariable())
        {
            EmitAttributesAttachedToSymbol(psymChild, GetToken(psymChild), NULL, pMetaemitHelper, isNoPIaEmbeddedSymbol);
        }
        else if (psymChild->IsGenericParam())
        {
            VSASSERT(psymChild->GetPAttrVals() == NULL, "Generic parameter somehow has an attribute.");
        }
        else
        {
            VSFAIL("What else can this child be?");
        }

#if IDE 
        // See if we need to stop this compilation.
        IfTrueAbort(CheckStop(NULL));
#endif IDE

    }

    // 


    // Do we have errors now?

    VSASSERT( !pContainer->IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

    BCSYM_Container *CurrentPartialContainer = pContainer;
    do
    {
        // 

        if (CurrentPartialContainer->GetErrorTableForContext()->HasErrors())
        {
            pInfo->m_hasErrors = true;
            break;
        }
    }
    while (CurrentPartialContainer = CurrentPartialContainer->GetNextPartialType());

#if IDE 
Abort:
#endif IDE

    return fAborted;
}


/*****************************************************************************
;DetermineDebuggingEntrypoint

Figures out what the Debugging entry point to the program

Returns: symbol of valid entry point if found, or NULL if not.
*****************************************************************************/
BCSYM_Proc *PEBuilder::DetermineDebuggingEntrypoint(BCSYM_Proc *pProc)
{
    BCSYM_Proc *psymDebuggingEntryPoint = pProc;
    if (pProc->IsSyntheticMethod())
    {
        BCSYM_Container *pContainer = pProc->GetContainer();
        // Try setting it on the constructor if possible
        if (pContainer->IsClass())
        {
            pProc = pContainer->PClass()->GetFirstInstanceConstructorNoRequiredParameters(m_pCompiler);
            while (pProc && pProc->IsSyntheticMethod())
            {
                pProc = pProc->GetNextInstanceConstructor();
            }

            if (pProc)
            {
                psymDebuggingEntryPoint = pProc;
            }
        }
    }
    return psymDebuggingEntryPoint;
}

/*****************************************************************************
;DetermineEntrypoint

Figures out what the entry point to the program is and generates error messages
depending on whether or not we find an entry point.

Returns: symbol of valid entry point if found, or NULL if not.
         ppProcDebuggingEntryPoint pointer to the debugging Entry Point
*****************************************************************************/
BCSYM_Proc *PEBuilder::DetermineEntrypoint(_Out_ BCSYM_Proc **ppProcDebuggingEntryPoint)
{
    DynamicArray<SubMainInfo> SubMainList;
    STRING *StartupString = m_pCompilerProject->GetStartupType();
    STRING *ContainerNameForErrorMessage;

    *ppProcDebuggingEntryPoint = NULL;

    // startup object not specified, try to find one
    if (!StartupString || (StringPool::StringLength(StartupString) == 0))
    {
        // Look for Sub Main in any class/module
        m_pCompilerProject->FindAllSubMainInProject(&SubMainList, false);
        ContainerNameForErrorMessage = m_pCompilerProject->GetAssemblyName();
    }
    else
    {
        // Look into this one class we know the qualified name for
        if (!m_pCompilerProject->FindClassMain(StartupString, &SubMainList, m_pErrorTable))
        {
            return NULL;
        }

        ContainerNameForErrorMessage = StartupString;
    }

    // Analys the array of Sub Mains that we got back. There are
    //
    // 1. Array has exactly one valid Sub Main, go ahead and use it
    // 2. Array is empty --> Error: No Sub Main found
    // 3. Array has 2 or more valid Sub Mains --> Error: more than one valid Sub Main found.
    // 4. Array has 1 or more invalid Sub Mains --> Error: No valid Sub Main found

    // Case 2: No Sub Mains found.
    if (SubMainList.Count() == 0)
    {
        m_pErrorTable->CreateError(ERRID_StartupCodeNotFound1, NULL, ContainerNameForErrorMessage);
        return NULL;
    }

    unsigned int NubmerOfValidSubMains = 0;
    unsigned int IndexOfLastValidSubMain = 0;

    bool ValidGenericMainsFound = false;

    // Find a valid Sub Main.
    for (unsigned int i = 0; i < SubMainList.Count(); ++i)
    {
        if (SubMainList.Array()[i].m_IsValidEntryPoint)
        {
            ++NubmerOfValidSubMains;
            IndexOfLastValidSubMain = i;
        }
        else if (SubMainList.Array()[i].m_IsInvalidOnlyDueToGenericness)
        {
            ValidGenericMainsFound = true;
        }
    }

    // Case 1: One good Sub Main found, use it!
    if (NubmerOfValidSubMains == 1)
    {
        BCSYM_Proc *pEntrypoint = SubMainList.Array()[IndexOfLastValidSubMain].m_SubMainSymbol;

        // (unless it's async)
        // The rule we follow:
        // First determine the Sub Main using pre-async rules, and give the pre-async errors if there were 0 or >1 results
        // If there was exactly one result, but it was async, then give an error. Otherwise proceed.
        // This doesn't follow the same pattern as "error due to being generic". That's because
        // maybe one day we'll want to allow Async Sub Main but without breaking back-compat.
        if (pEntrypoint->IsAsyncKeywordUsed())
        {
            m_pErrorTable->CreateError(ERRID_AsyncSubMain, pEntrypoint->GetLocation());
            // It's a shame that we don't get error-location-squigglies by this stage. Oh well.
            return NULL;
        }

        // Calculate Debugging Entry Point
        *ppProcDebuggingEntryPoint = DetermineDebuggingEntrypoint(SubMainList.Array()[IndexOfLastValidSubMain].m_SubMainSymbol);
        // Return the Entry point
        return pEntrypoint;
    }

    // Case 3: we generate en error message with all possible Sub Mains listed.
    if (NubmerOfValidSubMains > 1)
    {
        bool IsFirstInList = true;
        STRING *ValidSubMainsList = NULL;

        for (unsigned int i = 0; i < SubMainList.Count(); ++i)
        {
            if (SubMainList.Array()[i].m_IsValidEntryPoint)
            {
                BCSYM_Param *SubMainParam = (SubMainList.Array()[i]).m_SubMainSymbol->GetFirstParam();
                bool ReturnsAValue = (SubMainList.Array()[i]).m_SubMainSymbol->GetType() ? true : false;
                StringBuffer ParameterString;

                // Since we know that this Sub Main is a valid entry point, we know that if it takes an argument, it must be
                // and array of strings, and if it returns anything, it would have to be an Integer.
                ParameterString.AppendChar(WIDE('('));

                if (SubMainParam)
                {
                    SubMainParam->GetBasicRep(m_pCompiler, NULL, &ParameterString);
                }

                ParameterString.AppendChar(WIDE(')'));

                if (IsFirstInList)
                {
                    ValidSubMainsList = m_pCompiler->ConcatStrings(ValidSubMainsList,
                                                                   SubMainList.Array()[i].m_SubMainSymbol->GetQualifiedName(false),
                                                                   ParameterString.GetString(),
                                                                   ReturnsAValue ? L" As Integer" : NULL);
                    IsFirstInList = false;
                }
                else
                {
                    ValidSubMainsList = m_pCompiler->ConcatStrings(ValidSubMainsList,
                                                                   L", ",
                                                                   SubMainList.Array()[i].m_SubMainSymbol->GetQualifiedName(false),
                                                                   ParameterString.GetString(),
                                                                   ReturnsAValue ? L" As Integer" : NULL);
                }
            }
        }

        m_pErrorTable->CreateError(ERRID_MoreThanOneValidMainWasFound2, NULL, ContainerNameForErrorMessage, ValidSubMainsList);
        return NULL;
    }

    // Case 4: Bad Sub Main found, report an error
    ERRID errid;
    if (ValidGenericMainsFound)
    {
        errid = ERRID_GenericSubMainsFound1;
    }
    else
    {
        errid = ERRID_InValidSubMainsFound1;
    }
    
    m_pErrorTable->CreateError(errid, NULL, ContainerNameForErrorMessage);

    return NULL;
}

//============================================================================
// Write out the PE for everything we've compiled.
//============================================================================

void PEBuilder::WritePE
(
    _Inout_ PEInfo *pInfo,
    BYTE **ppImage,
    _Out_ bool *pfGenerated,
    _Inout_ MetaEmit *pMetaemitHelper
)
{
    ICeeFileGen *pFileGen = pInfo->m_pFileGen;
    HCEEFILE hCeeFile = pInfo->m_hCeeFile;
    HCEESECTION hCeeSection = pInfo->m_hCeeSection;
    STRING *pstrFileName = pInfo->m_pstrFileName;
    STRING *pstrResFileName = pInfo->m_pstrResFileName;
    STRING *pstrIconFileName = pInfo->m_pstrIconFileName;
    WCHAR *wszTempResFileName = pInfo->m_wszTempResFileName;

    VSASSERT(pMetaemitHelper->IsPrivateMembersEmpty(), "MetaEmit helper contains old data.");

    // These booleans control what work will be done during this call
    bool fCompileToMemory = false;
    bool fDoFirstHalf = false;
    bool fDoSecondHalf = false;

    switch (pInfo->m_compileType)
    {
    case CompileType_Full:
        fDoFirstHalf = fDoSecondHalf = true;
        break;

    case CompileType_InMemory:
        fCompileToMemory = true;
        fDoFirstHalf = fDoSecondHalf = true;
        if (ppImage == NULL)
        {
            VSFAIL("We require ppImage here for compile to memory!");
            VbThrow(HrMake(ERRID_InternalCompilerError));
        }
        break;

    case CompileType_DeferWritePE:
        fDoFirstHalf = true;
        break;

    case CompileType_FinishWritePE:
        fDoSecondHalf = true;
        break;

    default:
        VSFAIL("CompileType?");
        break;
    }

    *pfGenerated = false;

    //
    // FIRST HALF
    //

    if (fDoFirstHalf)
    {
        VSASSERT(pMetaemitHelper->VerifyALinkAndTokens(pInfo->m_pALink, pInfo->m_mdAssemblyOrModule, pInfo->m_mdFileForModule),
            "MetaEmit helper and PEInfo have different ALink!");



        // Re-import all referenced projects.  We imported these once earlier
        // (see PEBuilder::PreloadAssemblyRefCache), but now we have to do it again
        // so ALink will know about them.
        pMetaemitHelper->ALinkImportReferences();

        // Tell ALink to embed the UAC manifest.  We try this on both assemblies
        // and modules.  It's not valid on modules but it will create an error message
        // for the user indicating they shouldn't be doing that.
        pMetaemitHelper->ALinkEmbedUacManifest();

        // Emit any linked or embedded COM+ resources (Win32 resources
        // are handled separately)
        pMetaemitHelper->ALinkEmitResources(pFileGen, hCeeFile, hCeeSection);

        // Emit all module- and assembly-level custom attributes via ALink
        pMetaemitHelper->ALinkEmitAssemblyAttributes();

        // Emit the platform kind x64, x86, etc. for the output assembly.
        // Need to do this after ALinkImportReferences because we want
        // all reference to be checked for platform mismatches.
        //
        pMetaemitHelper->AlinkEmitAssemblyPlatformKind();

        // Set all of the assembly identity properties.  For the most part,
        // these come from command-line options.  Do this after ALinkEmitAssemblyAttributes
        // so that command-line options over-ride random custom attributes.
        // Note that some of the assembly properties end up in the Win32
        // resource blob, so do this before ALinkCreateWin32ResFile.
        pMetaemitHelper->ALinkSetAssemblyProps();

        if (FEmitAssembly())
        {
            // Create the assembly's manifest.
            pMetaemitHelper->ALinkEmitManifest(pFileGen, hCeeFile, hCeeSection);

            // Emit the security attributes on all the symbols in this project. This
            // process needs to be delayed till after emitting the project's manifest
            // because of Bug VSWhidbey 320892.
            //
            pMetaemitHelper->EmitAllSecurityAttributes();

            // Create the assembly's Win32 resources.
            //
            // If user hasn't specified a Win32 resource file, ask ALink to
            // auto-generate one for us.  Note that we don't do this for
            // in-memory assemblies -- they don't need Win32 resources, so we
            // shouldn't waste time creating them.  This also means we don't
            // have to decide what directory the temp Win32 resources
            // should live in -- a non-trivial task, since in-memory compilation
            // doesn't have an output directory and GetTempPath is severely
            // brain-damaged on Win NT/2000.
            if (pstrResFileName == NULL && !fCompileToMemory)
            {
                pMetaemitHelper->ALinkCreateWin32ResFile(
                                        !m_pCompilerProject->OutputIsExe(),
                                        pstrIconFileName,
                                        _countof(pInfo->m_wszTempResFileName),
                                        pInfo->m_wszTempResFileName);
            }
        }

        // Do we have errors now?
        if (m_pErrorTable->HasErrors())
        {
            pInfo->m_hasErrors = true;
        }

        //
        // Set some PE information
        //

        // set the image flags to distinguish between Agnostic and x86.
        DWORD dwFlags = COMIMAGE_FLAGS_ILONLY;
        if (m_pCompilerProject->GetPlatformKind() == Platform_X86)
        {
            dwFlags |= COMIMAGE_FLAGS_32BITREQUIRED;
        }
        else if (m_pCompilerProject->GetPlatformKind() == Platform_Anycpu32bitpreferred)
        {
            dwFlags |= (COMIMAGE_FLAGS_32BITREQUIRED | COMIMAGE_FLAGS_32BITPREFERRED);
        }
        IfFailThrow(pFileGen->SetComImageFlags(hCeeFile, dwFlags));

        // Set preferred load address for binary
        ULONGLONG loadAddress = m_pCompilerProject->GetLoadAddress();

        if (loadAddress == 0 && m_pCompilerProject->OutputIsLibrary())
        {
            // ICeeFileGen defaults the PE file to use EXE defaults for imageBase.
            // Since we know when the PE file will be a DLL instead, explicitly
            // choose the preferred DLL imageBase instead, if the user has not
            // specified an explicit imageBase.
            if (m_pCompilerProject->GetPlatformKind() == Platform_AMD64 ||
                m_pCompilerProject->GetPlatformKind() == Platform_IA64)
            {
                loadAddress = 0x0000000180000000;
            }
            else
            {
                loadAddress = 0x10000000;
            }
        }

        if (loadAddress != 0)
        {
            // 


            if (m_pCompilerProject->GetPlatformKind() == Platform_AMD64 ||
                m_pCompilerProject->GetPlatformKind() == Platform_IA64)
            {
                IfFailThrow(pFileGen->SetImageBase64(hCeeFile, loadAddress));
            }
            else
            {
                IfFailThrow(pFileGen->SetImageBase(hCeeFile, (size_t)loadAddress));
            }
        }


        // Set preferred file alignment for binary
        if (m_pCompilerProject->GetFileAlignment() != 0)
        {
            IfFailThrow(pFileGen->SetFileAlignment(hCeeFile, m_pCompilerProject->GetFileAlignment()));
        }

        // Set app type (GUI or console) and subsystem version.
        // Subsystem version 4.0 required for WinXP for app-compat reasons.
        // 


        SubsystemVersion subsystemVersion = {4,0};
        
        if (m_pCompilerProject)
        {
            subsystemVersion = m_pCompilerProject->GetSubsystemVersion();
        }
 
        IfFailThrow(pFileGen->SetSubsystem(hCeeFile,
                                           (m_pCompilerProject && m_pCompilerProject->GetOutputType() == OUTPUT_ConsoleEXE) ?
                                                IMAGE_SUBSYSTEM_WINDOWS_CUI :
                                                IMAGE_SUBSYSTEM_WINDOWS_GUI,
                                           subsystemVersion.major,
                                           subsystemVersion.minor));

        WORD DllCharacteristics = 0;
        if (m_pCompilerProject && m_pCompilerProject->GetOutputType() == OUTPUT_AppContainerEXE)
        {          
            DllCharacteristics |= /*IMAGE_DLLCHARACTERISTICS_APPCONTAINER */ 0x1000;
        }

        if (pMetaemitHelper->IsHighEntropyVA())
        {
            DllCharacteristics |= /*IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA*/ 0x20;
        }   

        if ( DllCharacteristics != 0)
        { 
            WriteDllCharacteristics(pInfo, DllCharacteristics);
        }

        // Initialize the scope.
        //
        IfFailThrow(m_pmdEmit->SetModuleProps(m_pCompilerProject->GetScopeName()));

#if IDE 

        //
        // Write out the code blocks for all methods in this project.
        //

        EmitAllCodeBlocks(pInfo);

#endif IDE

        // Tell ALink to emit the assembly.
        if (FEmitAssembly())
        {
            pMetaemitHelper->ALinkEmitAssembly();
        }

#if DEBUG

        // Use our debug mapper to assert if we try to map anything.
        IfFailThrow(m_pmdEmit->SetHandler(new (zeromemory) DebMapper()));

#endif DEBUG
    }

    //
    // SECOND HALF
    //

    if (fDoSecondHalf)
    {
#if IDE 
        // Only increment the FCN counter in the VBA case.
        // Otherwise this now occurs in CompilerPackage::EndProjectCompilation since
        // WritePE can now be called asynchronously.
        if (fCompileToMemory)
        {
            m_pCompilerProject->IncrementFileChangedNotificationCounter();
        }

        // Emit embedded resources if all of them were added after the first half
        // of compilation was completed.
        //
        // Emit any embedded COM+ resources (Win32 resources are handled separately)
        pMetaemitHelper->ALinkEmitResources(pFileGen, hCeeFile, hCeeSection, true /* Emit embedded resources only */);
#endif IDE

        // Tell ALink to close the internal assembly file.
        if (FEmitAssembly())
        {
            pMetaemitHelper->ALinkPrecloseAssembly();
        }

        //
        // Remove the files we're about to generate.
        //
        if (!fCompileToMemory)
        {
            RemoveFile(pstrFileName);
        }

        // Emit the debug information.
        CComPtr<ISymUnmanagedWriter> spWriter;
        WritePDB(pInfo, &spWriter);

        // make sure Symbol writer gets notifications about token remapping
        // Bug #103475 - DevDiv Bugs: in IDE case we need to be notified about token remappings
        // even in no-PDB case.
        m_spTokenMappingTable = RefCountedPtr<TokenMappingTable>(new TokenMappingTable(&m_nra));
        IfFailThrow(pFileGen->AddNotificationHandler(hCeeFile,new (zeromemory) CSymMapToken(m_spTokenMappingTable)));

        // Write the metadata into the PE generator.
        IfFailThrow(pFileGen->EmitMetaDataEx(hCeeFile, m_pmdEmit));
        if( spWriter ) 
        {
            WritePDB2(pInfo, spWriter);
            HRESULT hrClose = spWriter->Close();
            IfFailThrow(hrClose); 
            spWriter.Release();
        }

        // Set random information.
        if (!fCompileToMemory)
        {
            IfFailThrow(pFileGen->SetOutputFileName(hCeeFile, pstrFileName));
        }

        if (pstrResFileName || *wszTempResFileName)
        {
            WCHAR *pstrRealResourceFile;
            MapFile mapfile;

            // Note that the explicit resource file (pstrResFileName) takes
            // precedence over the one we consed up ourselves (wszTempResFileName).
            pstrRealResourceFile =  pstrResFileName ? pstrResFileName : wszTempResFileName;

            // SetResourceFileName doesn't validate the existence of the file,
            // so, if it's missing or bogus, you get a weird error back from
            // GenerateCeeFile, below.  We map the file here to generate
            // an error in the common case (missing file), but can't do much
            // for other cases (corrupted file).
            mapfile.Map(pstrRealResourceFile, m_pErrorTable, ERRID_UnableToOpenResourceFile1);

            IfFailThrow(pFileGen->SetResourceFileName(
                            hCeeFile,
                            pstrRealResourceFile));
        }

        // We don't have to worry about OUTPUT_Module here, as a module isn't
        // an executable, and therefore isn't a DLL.
        IfFailThrow(pFileGen->SetDllSwitch(hCeeFile,
                                           m_pCompilerProject->OutputIsLibrary()));

        IfFailThrow(pFileGen->SetComImageFlags(hCeeFile, COMIMAGE_FLAGS_ILONLY));

        IfFailThrow(pFileGen->SetEntryPoint(hCeeFile, pInfo->m_pProcEntryPoint == NULL ?
                                                       mdTokenNil : GetToken(pInfo->m_pProcEntryPoint, false)));

        if (!pInfo->m_hasErrors)
        {
            if (fCompileToMemory)
            {
                // Output to memory
                VSASSERT( ppImage, "ppImage cannot be NULL if generating to memory");
                IfFailThrow(pFileGen->GenerateCeeMemoryImage(hCeeFile, (void **)ppImage));

                VSASSERT(*ppImage != NULL, "GenerateCeeMemoryImage returns NULL image!!");
            }
            else
            {
                HRESULT hr;
                VSASSERT(pstrFileName, "Invalid state");

                // Output to a file
                hr = pFileGen->GenerateCeeFile(hCeeFile);

                if (FAILED(hr))
                {
                    m_pErrorTable->CreateErrorWithError(ERRID_BadOutputFile1, NULL, hr, pstrFileName);
                    return;
                }
            }

            // Tell ALink to sign the assembly.  We do this whether we're
            // emitting an assembly or a module.
            pMetaemitHelper->ALinkSignAssembly();
        }

        // Delete temp Win32 resource file (if any)
        if (*wszTempResFileName)
        {
            BOOL fRet;

            fRet = DeleteFile(wszTempResFileName);
            VSASSERT(fRet, "DeleteFile failed!");
            pInfo->m_wszTempResFileName[0] = NULL;
        }

        // Generate the XMLDoc file if needed
        if (m_pCompilerProject->IsXMLDocCommentsOn() && !m_pErrorTable->HasErrors())
        {
            m_pCompilerProject->GenerateXMLDocFile();
        }
    }

    // If we emitted any errors, delete the PE now.  Otherwise it would
    // be easy for an automated build system to miss the error and just
    // look for the output binary to determine if the compilation succeeded.
    if (m_pErrorTable->HasErrors())
    {
        RemoveFile(pstrFileName);
    }
    else if (fDoSecondHalf)
    {
        *pfGenerated = true;
    }
}

void PEBuilder::WriteDllCharacteristics(PEInfo *pInfo,WORD DllCharacteristics)
{
    ASSERT(pInfo != NULL, "Must not be NULL");
    PIMAGE_NT_HEADERS ntHeader;
    PIMAGE_SECTION_HEADER sectionHeader;
    ULONG numSections = 0;
    IfFailThrow(pInfo->m_pFileGen->GetHeaderInfo(pInfo->m_hCeeFile, &ntHeader, &sectionHeader, &numSections));
    if (ntHeader)
    {
        ntHeader->OptionalHeader.DllCharacteristics |= DllCharacteristics; 
    }
    return;
}

#if IDE 

//============================================================================
// Write the code blocks for this project to the PE generator.
//============================================================================

void PEBuilder::EmitAllCodeBlocks
(
    PEInfo *pInfo
)
{
    // Don't do anything if we have errors.
    if (pInfo->m_hasErrors)
    {
        return;
    }

    CompilerFile *pFile;
    BCSYM_Container *pContainer;
    BCSYM_Proc *pProc;
    NorlsAllocator nraScratch(NORLSLOC);

    // Loop through all of the files in the project.
    for (pFile = m_pCompilerProject->m_dlFiles[CS_Compiled].GetFirst(); pFile; pFile = pFile->Next())
    {
        if (pFile->IsSourceFile() && !pFile->CompileAsNeeded())
        {
            SourceFile *pSourceFile = pFile->PSourceFile();

            AllContainersInFileInOrderIterator containers;
            containers.Init(pSourceFile, false /* don't include partial containers */, true /* include transient types */);

            // Emit each of the classes.
            while (pContainer = containers.Next())
            {
                CompileableMethodsInAContainer iter(pContainer);

                while (pProc = iter.Next())
                {
                    EmitAllCodeBlocksForProc(pProc, pInfo);
                }
            }
        }
    }

    // Emit all the methods among the transient symbols.
    CSingleListIter<TransientSymbol> TransientsIter(m_Transients.SymbolIterator());

    // 


    ExistanceTree<BCSYM_Proc *> TransientProcsSeen;
    TransientProcsSeen.Init(&nraScratch);

    while (TransientSymbol *pTransient = TransientsIter.Next())
    {
        if (pTransient->m_pTransient->IsProc() && !TransientProcsSeen.Add(pTransient->m_pTransient->PProc()))
        {
            EmitAllCodeBlocksForProc(pTransient->m_pTransient->PProc(), pInfo);
        }
        else if (pTransient->m_pTransient->IsContainer())
        {
            CompileableMethodsInAContainer iter(pTransient->m_pTransient->PContainer());

            while ((pProc = iter.Next()) && !TransientProcsSeen.Add(pProc))
            {
                EmitAllCodeBlocksForProc(pProc, pInfo);
            }
        }
    }
}

void
PEBuilder::EmitAllCodeBlocksForProc
(
    BCSYM_Proc *pProc,
    PEInfo *pInfo
)
{
    if (pProc->IsENCSynthFunction())
    {
        return;
    }

    // If the method is a partial method declaration, don't emit any code
    // blocks for it.

    if (pProc->IsPartialMethodDeclaration())
    {
        return;
    }

    if (!pProc->IsMustOverrideKeywordUsed() &&
        !pProc->IsMethodCodeTypeRuntime() &&
        !pProc->GetPWellKnownAttrVals()->GetDllImportData() &&
        (pProc->IsMethodImpl() || pProc->IsSyntheticMethod()) &&
        !(pProc->GetContainingClass()->GetPWellKnownAttrVals()->GetComImportData() &&
            (
                (pProc->IsMethodImpl() && pProc->PMethodImpl()->GetMethodScope() == NULL) ||
                (pProc->IsSyntheticMethod() && pProc->PSyntheticMethod()->GetMethodScope() == NULL)
            )
            ) &&
        pProc->GetBindingSpace() != BINDSPACE_IgnoreSymbol)
    {
        BYTE *pbImage, *pbImagePE;
        unsigned cbImage;

        if (pProc->IsMethodImpl())
        {
            pProc->PMethodImpl()->GetImage(&pbImage, &cbImage);
        }
        else
        {
            VSASSERT(pProc->IsSyntheticMethod(), "Bad kind.");

            pProc->PSyntheticMethod()->GetImage(&pbImage, &cbImage);
        }

        if (pbImage && cbImage)
        {
            // Allocate space to store the data.
            pbImagePE = AllocatePEStorageForImage(pInfo, pProc, cbImage);

            // Copy it in.
#pragma prefast(suppress: 22105, "Need to update the SAL header for AllocPEStorageForImage to be correct and mark the proper return size")
            memcpy(pbImagePE, pbImage, cbImage);
        }
    }
}
#endif IDE

//============================================================================
// Write the PDB file
//============================================================================
HRESULT
PEBuilder::WritePDB
(
    _In_ PEInfo *pInfo,
    _Deref_out_ ISymUnmanagedWriter **ppWriter
)
{

    HRESULT hr = NOERROR;
    HRESULT hrClose = NOERROR;

    ISymUnmanagedWriter *pSymWriter = NULL;

    //
    // Get rid of any old state.
    //

    // Delete any old PDBs. But only if we're actually going to build
    // one, otherwise don't touch anything.
    if (!m_pCompilerProject->GetInMemoryCompilation())
    {
        WCHAR drive[_MAX_DRIVE];
        WCHAR dir[_MAX_DIR];
        STRING *pstrBuildingPEName;
        size_t SizeOfPEName;

        pstrBuildingPEName = m_pCompilerProject->GetPEName();
        SizeOfPEName = StringPool::StringLength(pstrBuildingPEName);

        TEMPBUF(fname, WCHAR *, (SizeOfPEName+1)*sizeof(WCHAR));
        IfNullThrow(fname);
        // Make sure there's enough room for .pdb
        TEMPBUF(wszFileFrom, WCHAR *, (SizeOfPEName+5)*sizeof(WCHAR));
        IfNullThrow(wszFileFrom);

        _wsplitpath_s(pstrBuildingPEName, drive, _countof(drive), dir, _countof(dir), fname, SizeOfPEName+1, NULL,0);

#pragma warning (push)
#pragma warning (disable:6387) // NULL check for wszFileFrom is above
        _wmakepath_s(wszFileFrom, SizeOfPEName+5, drive, dir, fname, L".pdb");
#pragma warning (pop)

        if (!DeleteFile(wszFileFrom))
        {
            // Can't delete the file
            DWORD dwLastError = GetLastError();
            if (dwLastError != ERROR_FILE_NOT_FOUND)
            {
                hr = HRESULT_FROM_WIN32(dwLastError);
                goto Error;
            }
        }
    }
    else if (m_pCompilerProject->GetPDBStream())
    {
        ULARGE_INTEGER libNewSize;
        libNewSize.HighPart = 0;
        libNewSize.LowPart = 0;

        // initialize the stream to nothing (does a realloc to 0 length)
        m_pCompilerProject->GetPDBStream()->SetSize(libNewSize);
    }

    //
    // If not emitting a PDB, we're done
    //
    if (!m_pCompilerProject->GeneratePDB() ||
        (m_pCompilerProject->GetInMemoryCompilation() && !m_pCompilerProject->GetPDBStream()))
    {
        goto NoPDB;
    }

    //
    // Create the PDB writer.
    //
    HRESULT hRes = CoCreateInstance(CLSID_CorSymWriter_SxS,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ISymUnmanagedWriter,
                              (void**)&pSymWriter);

    if (FAILED(hRes))
    {
        m_pErrorTable->CreateErrorWithError(ERRID_ClassCannotCreated, NULL, hRes, L"CLSID_CorSymWriter");
    }

    IfFailGo(hRes);

    IfFailGo(pSymWriter->Initialize(m_pmdEmit,
                                    m_pCompilerProject->GetPEName(),
                                    m_pCompilerProject->GetPDBStream(),
                                    true));

    //
    // Emit the link between the PE and the PDB.
    //

    if (pInfo->m_compileType != CompileType_InMemory)
    {
        CreateDebugDirectory(pInfo, pSymWriter);
    }

    *ppWriter = pSymWriter;
    return S_OK;

NoPDB:
Error:

    *ppWriter = NULL;
    return E_FAIL;
}

void PEBuilder::WritePDB2
(
    _In_ PEInfo *pInfo,
    ISymUnmanagedWriter * pSymWriter
)
{
    HRESULT hr = NOERROR;
    HRESULT hrClose = NOERROR;
    CComPtr<ISymUnmanagedDocumentWriter> spDocumentWriter = NULL;

    NorlsAllocator  nraTemp(NORLSLOC);
    PDBForwardProcCache ForwardProcCache(&nraTemp);
    ConsolidatedModulesTree ModulesCache(&nraTemp);

    m_pPDBForwardProcCache = &ForwardProcCache;
    m_pModulesCache = &ModulesCache;

    // Set up the entry point
    // We thought that we didn't need to map the entry point, but it looks like we do.
    IfFailThrow(pSymWriter->SetUserEntryPoint(pInfo->m_pProcDebuggingEntryPoint == NULL ?
                                                mdTokenNil : GetToken(pInfo->m_pProcDebuggingEntryPoint, true)));


    //
    // Walk the project and spit the debug information for each method.
    //

    CompilerFile *pFile = NULL;

    // Loop through all of the files again and emit the definitions of the
    // methods/variables.
    //
    for (pFile = m_pCompilerProject->m_dlFiles[CS_Compiled].GetFirst(); pFile; pFile = pFile->Next())
    {
        if (pFile->IsSourceFile() && pFile->GetFileName() && !pFile->CompileAsNeeded())
        {
            // Port SP1 CL 2922610 to VS10

            if ( !pFile->IsSourceFile() || !pFile->GetFileName() || pFile->CompileAsNeeded())
            {
                continue;
            }

            SourceFile *pSourceFile = pFile->PSourceFile();
            BYTE bCheckSum[CRYPT_HASHSIZE];
            memset(bCheckSum,0,CRYPT_HASHSIZE);

            // Create the default document for this file.
            IfFailGo(GetPDBDocumentForFileName(pSymWriter, pSourceFile->GetFileName(), &spDocumentWriter));
            IfFailGo(pSourceFile->GetCryptHash((void *)bCheckSum,CRYPT_HASHSIZE));
            IfFailGo(spDocumentWriter->SetCheckSum(guidSourceHashMD5,CRYPT_HASHSIZE,bCheckSum));

            // Walk the containers.
            AllContainersInFileInOrderIterator iter;
            const bool IncludePartialContainersToo = true;
            iter.Init(pSourceFile, IncludePartialContainersToo);

            BCSYM_Container *pContainer;

            // Emit each of the classes.
            while (pContainer = iter.Next())
            {
                const bool LookOnlyInCurrentPartialType = true;
                CompileableMethodsInAContainer iterProc(pContainer, LookOnlyInCurrentPartialType);
                BCSYM_Proc *pProc;

                while (pProc = iterProc.Next())
                {
                    IfFailGo(WritePDBInfoForProc(pProc,
                                                 pSourceFile,
                                                 pSymWriter,
                                                 spDocumentWriter));
                }
            }

            IfFailGo( WritePDBInfoForLambdas( pSourceFile, pSymWriter, spDocumentWriter ));
            spDocumentWriter.Release();
            m_pPDBForwardProcCache->Clear();
        }

        m_pPDBForwardProcCache->Clear();
    }

    m_pModulesCache->Clear();
    nraTemp.FreeHeap();

#if !IDE
    // write the external checksum directives
    ULONG count = m_pCompilerProject->m_daExternalChecksum.Count();
    CompilerProject::ExternalChecksum *extChecksumArray = m_pCompilerProject->m_daExternalChecksum.Array();
    for (unsigned i = 0 ; i < count; i++)
    {
        // skip the files that have the same name as a source file.
        bool fSkip = false;
        for (pFile = m_pCompilerProject->m_dlFiles[CS_Compiled].GetFirst(); pFile; pFile = pFile->Next())
        {
            if ((pFile->IsSourceFile() && pFile->GetFileName() && !pFile->CompileAsNeeded()) &&
                 StringPool::IsEqual(extChecksumArray[i].FileName,  pFile->GetFileName()))
            {
                fSkip = true;
                break;
            }
        }

        if (!fSkip)
        {
            CComPtr<ISymUnmanagedDocumentWriter> spDocumentWriter2 = NULL;


            // Create the default document for this file.
            IfFailGo(GetPDBDocumentForFileName(pSymWriter, extChecksumArray[i].FileName, &spDocumentWriter2));
            IfFailGo(spDocumentWriter2->SetCheckSum(extChecksumArray[i].Guid,
                                                    extChecksumArray[i].cbChecksumData,
                                                    extChecksumArray[i].pbChecksumData));
        }
    }
#endif

    // If compiling to winmdobj then output to PDB source spans for types and members
    // for better error reporting by WinMDExp.
    if (this->m_pCompilerProject->m_OutputType == OUTPUT_WinMDObj)
    {
        CComPtr<ISymUnmanagedWriter5> spSymWriter5;
        pSymWriter->QueryInterface(&spSymWriter5);

        if (spSymWriter5 && SUCCEEDED(hr = spSymWriter5->OpenMapTokensToSourceSpans()))
        {
            for (pFile = m_pCompilerProject->m_dlFiles[CS_Compiled].GetFirst(); pFile; pFile = pFile->Next())
            {
                if (pFile->IsSourceFile() && pFile->GetFileName() && !pFile->CompileAsNeeded())
                {
                    SourceFile *pSourceFile = pFile->PSourceFile();
                    IfFailGo( WritePDBInfoForTokenSourceSpans( pSourceFile, spSymWriter5 ));
                }
            }

            spSymWriter5->CloseMapTokensToSourceSpans();
        }
    }

    ReleasePDBDocuments();

    m_pPDBForwardProcCache = NULL;
    m_pModulesCache = NULL;
    return;

Error:

#if !IDE
    // Dev10 bug 866530
    // Don't close/Release pSymWriter since the caller will do this work and
    // the IDE crashes when calling Close() a second time.
    if (pSymWriter)
    {
        // We're done with the PDB, flush it.
        hrClose = pSymWriter->Close();
        RELEASE(pSymWriter);
    }
#endif

    if (FAILED(hr))
    {
        if (!m_pCompilerProject->GetInMemoryCompilation())
        {
            WCHAR drive[_MAX_DRIVE];
            WCHAR dir[_MAX_DIR];
            STRING *pstrBuildingPEName;
            size_t SizeOfPEName;

            pstrBuildingPEName = m_pCompilerProject->GetPEName();
            SizeOfPEName = StringPool::StringLength(pstrBuildingPEName);

            TEMPBUF(fname, WCHAR *, (SizeOfPEName+1)*sizeof(WCHAR));
            IfNullThrow(fname);

            // Make sure there's enough room for .pdb
            TEMPBUF(wszFileFrom, WCHAR *, (SizeOfPEName+5)*sizeof(WCHAR));
            IfNullThrow(wszFileFrom);

            _wsplitpath_s(pstrBuildingPEName, drive, _countof(drive), dir, _countof(dir), fname, SizeOfPEName+1, NULL,0);
            _wmakepath_s(wszFileFrom, SizeOfPEName+5, drive, dir, fname, L".pdb");
            m_pErrorTable->CreateErrorWithError(ERRID_BadOutputFile1, NULL, hr, wszFileFrom);
        }
        else
        {
            m_pErrorTable->CreateErrorWithError(ERRID_BadOutputStream, NULL, hr, NULL);
        }
    }

    return;
}

HRESULT PEBuilder::WritePDBInfoForTokenSourceSpans(
    _In_ SourceFile *pSourceFile,
    _In_ ISymUnmanagedWriter5 *pSymWriter5
)
{
    HRESULT hr = S_OK;

    // Only write out this information for WinMDObj files, and only for user
    // source files, not embedded source such as VBCore or My Namespace.

    if (pSourceFile->m_pExtensionData == NULL)
    {
        // Go through the containers and emit source spans for each
        // type and their members

        AllContainersInFileInOrderIterator iter;
        const bool IncludePartialContainersToo = true;
        iter.Init(pSourceFile, IncludePartialContainersToo);

        BCSYM_Container *pContainer;
        while (pContainer = iter.Next())
        {
            // emit this container's mapping if it is a type but not a partial type
            // (we only want one mapping for a partial type, its main type)
            if (!pContainer->IsPartialTypeAndHasMainType())
            {
                WritePDBInfoForTokenSourceSpans(pContainer, pSymWriter5, pSourceFile);
            }

            // emit the mappings for the container's members
            const bool LookOnlyInCurrentPartialType = true;
            DefineableMembersInAContainer iterMember(m_pCompiler, pContainer);
            BCSYM_NamedRoot* pSym;

            while (pSym = iterMember.Next())
            {
                if (pSym->GetBindingSpace())
                {
                    BCSYM* pNonAliasedSymbol = pSym->DigThroughAlias();

                    if (pNonAliasedSymbol->IsProc() ||
                        pNonAliasedSymbol->IsVariable() ||
                        pNonAliasedSymbol->IsProperty() ||
                        pNonAliasedSymbol->IsEventDecl())
                    {
                        WritePDBInfoForTokenSourceSpans(pSym, pSymWriter5, pSourceFile);
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT
PEBuilder::WritePDBInfoForTokenSourceSpans(
    _In_ BCSYM_NamedRoot *pSym,
    _In_ ISymUnmanagedWriter5 *pSymWriter5,
    _In_ SourceFile *pSourceFile
)
{
    HRESULT hr = S_OK;

    mdToken token = GetToken(pSym);
    if (IsNilToken(token))
    {
        return S_OK;
    }

    Location* loc = pSym->GetLocation();
    if (loc == NULL)
    {
        return S_OK;
    }

    long line = loc->GetStartPoint().Line;
    long column = loc->GetStartPoint().Column;
    long endLine = loc->GetEndPoint().Line;
    long endColumn = loc->GetEndPoint().Column;
    STRING* fileName = pSourceFile->GetFileName();

#if !IDE
    if (pSourceFile->IsMappedFile())
    {
        bool hasDebugInfo = false;
        STRING* mappedFileName = NULL;
        long mappedLine = pSourceFile->GetMappedLineInfo(line, &hasDebugInfo, &mappedFileName);

        endLine = endLine - line + mappedLine;
        line = mappedLine;
        fileName = mappedFileName;
    }
#endif

    CComPtr<ISymUnmanagedDocumentWriter> spDocumentWriter;
    IfFailGo(GetPDBDocumentForFileName(pSymWriter5, fileName, &spDocumentWriter));

    hr = pSymWriter5->MapTokenToSourceSpan(token,
                                           spDocumentWriter,
                                           line + 1,
                                           column + 1,
                                           endLine + 1,
                                           endColumn + 1);
Error:
    return hr;
}

HRESULT
PEBuilder::GetPDBDocumentForFileName(
    _In_ ISymUnmanagedWriter* pSymWriter,
    _In_z_ STRING* fileName,
    _Out_ ISymUnmanagedDocumentWriter** ppDocument
)
{
    HRESULT hr = S_OK;

    ISymUnmanagedDocumentWriter* pDocument;
    if (!m_pdbDocs.GetValue(fileName, &pDocument))
    {
        IfFailGo(pSymWriter->DefineDocument(fileName,
                                            &guidVBLang,           // GUID* for VB language,
                                            &guidMicrosoftVendor,  // GUID* for language Vendor,
                                            (GUID *) & CorSym_DocumentType_Text,           // GUID* for document type
                                            &pDocument));
        m_pdbDocs.SetValue(fileName, pDocument);
    }

    *ppDocument = pDocument;
    (*ppDocument)->AddRef();

Error:
    return hr;
}

void
PEBuilder::ReleasePDBDocuments()
{
    auto iter = m_pdbDocs.GetValueIterator();
    while (iter.MoveNext())
    {
        iter.Current()->Release();
    }
    m_pdbDocs.Clear();
}

//---------------------------------------------------------------------------//
//
// Write out all of the NoPia references
//
//---------------------------------------------------------------------------//
void 
Builder::WriteNoPiaPdbList( _In_ ISymUnmanagedWriter* pSymWriter)
{
    ThrowIfNull(pSymWriter);

    // First build up the list
    HRESULT hr = S_OK;  
    ReferenceIterator it(m_pCompilerProject);
    while ( ReferencedProject* pRef = it.NextReference() )
    {
        if ( pRef->m_fEmbedded )
        {
            StringBuffer name;
            name.AppendPrintf(L"&%s", pRef->m_pCompilerProject->GetAssemblyName());
            IfFailGo( pSymWriter->UsingNamespace(name.GetString()) );
        }
    }

Error:
    return;
}

//---------------------------------------------------------------------------//
// Port SP1 CL 2922610 to VS10
// Write out the PDB information for Lambdas
//
//
// Assumptions:
//    Assumes that the document has been defined for the source file
//
// Notes:
//
//   The caller of this function is iterating through all of the source files.
//   If we have synthetic methods that should be emitted for a given sourcefile, 
//   we write the pdb info out.

HRESULT PEBuilder::WritePDBInfoForLambdas(
    SourceFile *pSourceFileBeingProcessed,
    ISymUnmanagedWriter *pSymWriter,
    ISymUnmanagedDocumentWriter *pDocumentWriter
)
{
    HRESULT hr = NOERROR;

    auto pProcs = m_SyntheticMethods[pSourceFileBeingProcessed];
    for(auto it = pProcs.begin(); it != pProcs.end(); ++it)
    {
        IfFailGo(WritePDBInfoForProc(*it, pSourceFileBeingProcessed, pSymWriter, pDocumentWriter));
    }

Error: 
    return hr;
} // WritePDBInfoForLambdas


HRESULT PEBuilder::WritePDBInfoForProc
(
    BCSYM_Proc *pProc,
    SourceFile *pSourceFile,
    ISymUnmanagedWriter *pSymWriter,
    ISymUnmanagedDocumentWriter *pDocumentWriter
)
{
    HRESULT hr = NOERROR;

    if (pProc->GetBindingSpace() == BINDSPACE_IgnoreSymbol)
    {
        return hr;
    }

    if (pProc->IsPartialMethodDeclaration() )
    {
        return hr;
    }

    MethodDebugInfo *pDebugInfo = NULL;
    AsyncMethodDebugInfo *pAsyncDebugInfo = NULL;

    if (pProc->IsSyntheticMethod())
    {
        pDebugInfo  = pProc->PSyntheticMethod()->GetDebugInfoList()->GetFirst();
        pAsyncDebugInfo = pProc->PSyntheticMethod()->GetAsyncDebugInfo();
    }
    else if (pProc->IsMethodImpl())
    {
        pDebugInfo = pProc->PMethodImpl()->GetDebugInfoList()->GetFirst();
        pAsyncDebugInfo = pProc->PMethodImpl()->GetAsyncDebugInfo();
    }

    if (!pDebugInfo)
    {
        return hr;
    }

    mdToken procToken = GetToken(pProc);
    VSASSERT(procToken, "Method has no token.");

    // If the method has no lines, then don't emit any debug info about it.
    // This will happen if the method lies outside a #ExternalSource directive.
    if (pDebugInfo->m_ulCurrNoLines == 0)
    {
        return hr;
    }

    // Open the debug information for this method.
    IfFailGo(pSymWriter->OpenMethod(procToken));

    //
    // Write out the line number table.
    //
    while (pDebugInfo)
    {
        CComPtr<ISymUnmanagedDocumentWriter> spCurrentDocumentWriter;

        VSASSERT( pDebugInfo->m_ulCurrNoLines > 0, "WritePDB: method must have non-zero line count" );

        // Get the document writer for the containing file.
        if (!pDebugInfo->m_pstrDocumentName || StringPool::IsEqual(pSourceFile->GetFileName(), pDebugInfo->m_pstrDocumentName))
        {
            spCurrentDocumentWriter = pDocumentWriter;
        }
        else
        {
            IfFailGo(GetPDBDocumentForFileName(pSymWriter, pDebugInfo->m_pstrDocumentName, &spCurrentDocumentWriter));
        }

        // Write the table.
        IfFailGo(pSymWriter->DefineSequencePoints(spCurrentDocumentWriter,
                                                    pDebugInfo->m_ulCurrNoLines,
                                                    pDebugInfo->m_rglOffsetTable,
                                                    pDebugInfo->m_rglLineTable,
                                                    pDebugInfo->m_rglColumnTable,
                                                    pDebugInfo->m_rglEndLineTable,
                                                    pDebugInfo->m_rglEndColumnTable));

        pDebugInfo = pDebugInfo->Next();
    }

    if (pAsyncDebugInfo)
    {
        CComPtr<ISymUnmanagedAsyncMethodPropertiesWriter> spAsyncWriter;
        pSymWriter->QueryInterface(&spAsyncWriter);

        if (spAsyncWriter)
        {
            if (pAsyncDebugInfo->m_ulNoAwaits > 0)
            {
                DynamicArray<mdToken> breakpointMethods;
                breakpointMethods.Grow(pAsyncDebugInfo->m_ulNoAwaits);
                for (unsigned long i = 0; i < pAsyncDebugInfo->m_ulNoAwaits; i++)
                {
                    breakpointMethods.Element(i) = procToken;
                }

                IfFailGo(spAsyncWriter->DefineAsyncStepInfo(pAsyncDebugInfo->m_ulNoAwaits,
                                                            pAsyncDebugInfo->m_rglYieldOffsets,
                                                            pAsyncDebugInfo->m_rglBreakpointOffsets,
                                                            breakpointMethods.Array()));
            }
            else
            {
                IfFailGo(spAsyncWriter->DefineAsyncStepInfo(0, NULL, NULL, NULL));
            }

            if (pAsyncDebugInfo->m_catchHandlerOffset != (unsigned int)-1)
            {
                IfFailGo(spAsyncWriter->DefineCatchHandlerILOffset(pAsyncDebugInfo->m_catchHandlerOffset));
            }

            IfFailGo(spAsyncWriter->DefineKickoffMethod(GetToken(pAsyncDebugInfo->m_pKickoffProc)));
        }
    }

    //
    // Write out the local information.
    //
    if (pProc->IsMethodImpl())
    {
        WriteBlockScopes(pProc->PMethodImpl()->GetMethodScope(), pSymWriter, pProc, pProc->GetContainer(), pProc->PMethodImpl()->GetSignatureToken());
    }
    else if (pProc->IsSyntheticMethod())
    {
        WriteBlockScopes(pProc->PSyntheticMethod()->GetMethodScope(), pSymWriter, pProc, pProc->GetContainer(), pProc->PSyntheticMethod()->GetSignatureToken());
    }

    // Close the method.
    IfFailGo(pSymWriter->CloseMethod());

Error:
    return hr;
}

//
// The following helpers implement the individual operations needed to
// produce a PE.  The order and timing which these methods get called
// will differ between the IDE build and a command-line compiler
// build.
//

//============================================================================
// Create the pieces needed to emit an EXE.  EndEmit must always be called
// even if this method throws an error.
//============================================================================

void PEBuilder::BeginEmit
(
    _Out_ PEInfo *pInfo,
    CompileType compileType
)
{
    // Preserve the compilation type
    pInfo->m_compileType = compileType;

    // Create the emitter.
    CreateCeeFileGen(m_pCompiler, &pInfo->m_pFileGen);

    // Set the platform type for the PE

    DWORD dwFlags = ICEE_CREATE_FILE_PURE_IL;

    // Get the right platform flags
    switch(m_pCompilerProject->GetPlatformKind())
    {
        case Platform_Anycpu32bitpreferred:
        case Platform_Agnostic:
        case Platform_X86:
            // old 'default' agnostic image.
            // Difference between X86 and Agnostic is in corheader later, not here
            //
            dwFlags = ICEE_CREATE_FILE_PURE_IL;
            break;

        case Platform_AMD64:
            dwFlags = ICEE_CREATE_FILE_PE64 | ICEE_CREATE_MACHINE_AMD64;
            break;

        case Platform_IA64:
            dwFlags = ICEE_CREATE_FILE_PE64 | ICEE_CREATE_MACHINE_IA64;
            break;
        
        case Platform_ARM:
            dwFlags = ICEE_CREATE_FILE_PE32 | 0x800 /* ICEE_CREATE_MACHINE_ARM */;
            break;

        default:
            VSFAIL("platform type not set correctly!!!");
    }

    // Create the section to emit to.
    IfFailThrow(pInfo->m_pFileGen->CreateCeeFileEx(&pInfo->m_hCeeFile, dwFlags));
    IfFailThrow(pInfo->m_pFileGen->GetIlSection(pInfo->m_hCeeFile, &pInfo->m_hCeeSection));

#if IDE 
    // Preserve this instance until EndEmit which will
    // be called at the end of PEBuilder::Compile or later
    // in PEBuilder::CompileToDisk.
    m_pInfoDeferWritePE = (compileType == CompileType_DeferWritePE) ? pInfo : NULL;
#endif
}

// CreateALinkForCompile
//
// Creates an ALink helper object inside pInfo structure if one does not exist.
// Called by PEBuilder::Compile or PEBuilder::BeginEmit and ALink closed by PEBuilder::EndEmit
//
// [out] pInfo: PEInfo structure (used in PEBuilder::Compile) to store ALink object in.
//              Writes pInfo->m_compileType, pInfo->m_pstrFileName, pInfo->m_pALink,
//                     pInfo->m_mdFileForModule, pInfo->m_mdAssemblyOrModule
// [in]  CompileType: Specifies compiling in memory or emitting assembly. Determines how to
//                    create ALink.
//
void PEBuilder::CreateALinkForCompile
(
    _Inout_ PEInfo *pInfo,
    CompileType compileType
)
{
    // Create our ALink interface and do some minimal initialization if not already created
    if (!pInfo->m_pALink)
    {
        IMetaDataDispenserEx *pMetaDataDispenser = NULL;
        STRING *pstrAssemblyName;
        bool fCompileToMemory = (compileType == CompileType_InMemory);
        PlatformKinds PlatformKind = m_pCompilerProject->GetPlatformKind();

        // The file name to write to.
        pInfo->m_pstrFileName = m_pCompilerProject->GetPEName();

        // Assembly name (defaults to the file name)
        pstrAssemblyName = pInfo->m_pstrFileName;

        // Create our ALink interface and do some minimal initialization
        IfFailThrow(m_pCompilerProject->GetCompilerHost()->GetDispenser(&pMetaDataDispenser));

        CreateALink(&pInfo->m_pALink, &pInfo->m_pALink3);
        IfFailThrow(pInfo->m_pALink->Init(pMetaDataDispenser, &pInfo->m_metadataerror));

        if (FEmitAssembly())
        {
            // For perf reasons (and others), we never hash referenced assemblies, so
            // we always specify afNoRefHash.  See VS RAID 207952 for more details.
            AssemblyFlags af = (AssemblyFlags)(afNoRefHash | (fCompileToMemory ? afInMemory : afNone));

            // Building an assembly
            IfFailThrow(pInfo->m_pALink->SetAssemblyFile(
                            pstrAssemblyName,               // IN - filename of the manifest file
                            m_pmdEmit,                      // IN - Emitter interface for this file
                            af,                             // IN: Assembly flags
                            &pInfo->m_mdAssemblyOrModule)); // OUT- Unique ID for this assembly

            // ALink understands that FileID == AssemblyID means we're building
            // a single-file assembly
            pInfo->m_mdFileForModule = pInfo->m_mdAssemblyOrModule;
        }
        else
        {
            // Building a module
            IfFailThrow(pInfo->m_pALink->AddFile(
                            AssemblyIsUBM,                  // IN - Unique ID for the assembly
                            pstrAssemblyName,               // IN - filename of the file to add
                            ffContainsMetaData,             // IN - COM+ FileDef flags
                            m_pmdEmit,                      // IN - Emitter interface for file
                            &pInfo->m_mdFileForModule));    // OUT- unique ID for the file

            // ALink returns a zero-based mdFile token for this case.
            // Map it to AssemblyIsUBM here for all subsequent use.
            VSASSERT(pInfo->m_mdFileForModule == TokenFromRid(0, mdtFile), "Bogus module token from ALink.");
            pInfo->m_mdAssemblyOrModule = AssemblyIsUBM;
        }

        DebCheckAssemblyOrModuleToken(pInfo->m_mdAssemblyOrModule);
        RELEASE(pMetaDataDispenser);
    }
}

//============================================================================
// Allocate the memory to store the IL for a procedure.  This stores the
// IL in a no-release allocator in the IDE and stores it directly in the
// PE in the command-line compiler.
//============================================================================

BYTE *PEBuilder::AllocatePEStorageForImage
(
    _In_ PEInfo *pInfo,
    BCSYM_Proc *pProc,
    unsigned cbImage
)
{
    ICeeFileGen *pFileGen = pInfo->m_pFileGen;
    HCEESECTION hCeeSection = pInfo->m_hCeeSection;
    HCEEFILE hCeeFile = pInfo->m_hCeeFile;
    mdToken tokenProc;

    BYTE *pbTargetImage;

    unsigned long uImageOffset;
    unsigned long uRVA;

    // Get where the data is going to go.
    IfFailThrow(pFileGen->GetSectionDataLen(hCeeSection, &uImageOffset));

    // 4-byte align it.
    uImageOffset = (uImageOffset + 3) & ~3;

    // Get the RVA.
    IfFailThrow(pFileGen->GetMethodRVA(hCeeFile, uImageOffset, &uRVA));

    // Allocate the memory to store the image in.
    IfFailThrow(pFileGen->GetSectionBlock(hCeeSection,
                                          cbImage,
                                          sizeof(DWORD),
                                          (void **)&pbTargetImage));

    memset(pbTargetImage, 0, cbImage);

    // Save the RVA.  Added assert to track down VS RAID 162282.
    tokenProc = GetToken(pProc);
    VSASSERT(TypeFromToken(tokenProc) == mdtMethodDef ||
              TypeFromToken(tokenProc) == mdtFieldDef,
              "Bogus token for proc");
    m_pmdEmit->SetRVA(tokenProc, uRVA);

    return pbTargetImage;
}

//============================================================================
// Create the debug directory inside of the PE and have it point to
// our PDB.
//============================================================================

void PEBuilder::CreateDebugDirectory
(
    _In_ PEInfo *pInfo,
    ISymUnmanagedWriter *pSymWriter
)
{
    ICeeFileGen *pFileGen = pInfo->m_pFileGen;
    HCEESECTION hCeeSection = pInfo->m_hCeeSection;
    HCEEFILE hCeeFile = pInfo->m_hCeeFile;

    IMAGE_DEBUG_DIRECTORY  debugDirIDD;
    DWORD                  debugDirDataSize;
    BYTE                  *debugDirData;
    DWORD                  fileTimeStamp;
    HCEESECTION            sec;
    BYTE                  *de;
    ULONG                  deOffset;
    NorlsAllocator         nraScratch(NORLSLOC);
    CComPtr<ISymUnmanagedWriter4> spSymWriter4;

    // Must use a read/write section for the debug directory
    IfFailThrow(pFileGen->GetSectionCreate(hCeeFile, ".sdata", sdReadWrite, &sec));

    pSymWriter->QueryInterface(&spSymWriter4);

    // Get the debug info from the symbol writer.
    if (spSymWriter4)
    {
        IfFailThrow(spSymWriter4->GetDebugInfoWithPadding(NULL, 0, &debugDirDataSize, NULL));
    }
    else
    {
        IfFailThrow(pSymWriter->GetDebugInfo(NULL, 0, &debugDirDataSize, NULL));
    }

    // Will there even be any debug information?
    if (debugDirDataSize == 0)
    {
        return;
    }

    // Make some room for the data.
    debugDirData = (BYTE *)nraScratch.Alloc(debugDirDataSize);

    // Actually get the data now.
    if (spSymWriter4)
    {
        IfFailThrow(spSymWriter4->GetDebugInfoWithPadding(&debugDirIDD,
                                             debugDirDataSize,
                                             NULL,
                                             debugDirData));
    }
    else
    {
        IfFailThrow(pSymWriter->GetDebugInfo(&debugDirIDD,
                                             debugDirDataSize,
                                             NULL,
                                             debugDirData));
    }

    IfFailThrow(pFileGen->GetFileTimeStamp(hCeeFile, &fileTimeStamp));

    // Fill in the directory entry.
    debugDirIDD.TimeDateStamp = fileTimeStamp;
    debugDirIDD.AddressOfRawData = 0;

    // Grab memory in the section for our stuff.
    IfFailThrow(pFileGen->GetSectionBlock(sec,
                                          sizeof(debugDirIDD) +
                                          debugDirDataSize,
                                          4,
                                          (void**) &de));

    // Where did we get that memory?
    IfFailThrow(pFileGen->GetSectionDataLen(sec, &deOffset));

    deOffset -= (sizeof(debugDirIDD) + debugDirDataSize);

    debugDirIDD.AddressOfRawData = deOffset + sizeof(IMAGE_DEBUG_DIRECTORY);
    IfFailThrow(pFileGen->AddSectionReloc(sec,
                                          deOffset +
                                          offsetof(IMAGE_DEBUG_DIRECTORY,
                                                   AddressOfRawData),
                                          sec, srRelocAbsolute));

    // Setup a reloc so that the address of the raw
    // data is setup correctly.
    debugDirIDD.PointerToRawData = sizeof(debugDirIDD);

    IfFailThrow(pFileGen->AddSectionReloc(sec,
                                          deOffset +
                                          offsetof(IMAGE_DEBUG_DIRECTORY,
                                                   PointerToRawData),
                                          sec, srRelocFilePos));

    // Emit the directory entry.
    IfFailThrow(pFileGen->SetDirectoryEntry(hCeeFile,
                                            sec,
                                            IMAGE_DIRECTORY_ENTRY_DEBUG,
                                            sizeof(debugDirIDD),
                                            deOffset));

    // Copy the debug directory into the section.
    memcpy(de, &debugDirIDD, sizeof(debugDirIDD));
    memcpy(de + sizeof(debugDirIDD), debugDirData, debugDirDataSize);
}

//============================================================================
// Deletes an output file from the disk.
//============================================================================

bool PEBuilder::RemoveFile
(
    _In_opt_z_ WCHAR *wszFileName,
    _In_opt_z_ WCHAR *wszExt
)
{
    if (!wszFileName)
    {
        return false;
    }

    // Don't muck with the filename
    return DeleteFile(wszFileName);
}

//============================================================================
// We're done with the PE emitter.
//============================================================================

void PEBuilder::EndEmit
(
    _In_opt_ PEInfo *pInfo
)
{
    if (pInfo != NULL)
    {
        // Destroy the file.
        if (pInfo->m_hCeeFile)
        {
            pInfo->m_pFileGen->DestroyCeeFile(&pInfo->m_hCeeFile);
        }

        // Destroy the emitter.
        if (pInfo->m_pFileGen)
        {
            DestroyCeeFileGen(&pInfo->m_pFileGen);
        }

        // Destroy the ALink interface
        RELEASE(pInfo->m_pALink);
        RELEASE(pInfo->m_pALink3);

        // Delete temp Win32 resource file (if any)
        if (*pInfo->m_wszTempResFileName)
        {
            BOOL fRet;
            fRet = DeleteFile(pInfo->m_wszTempResFileName);
            VSASSERT(fRet, "DeleteFile failed!");
        }

        delete pInfo;
#if IDE 
        m_pInfoDeferWritePE = NULL;
#endif
    }
}

void PEBuilder::EmitCustomAttribute
(
    BCSYM      * pSymbol,
    mdToken tokenForSymbol,
    BCSYM_ApplAttr * psymApplAttr,
    ErrorTable * pErrorTable,
    _Out_ MetaEmit * pMetaEmitHelper,
    NorlsAllocator * pAlloc,
    SecAttrInfo * pSecAttrInfo,
    BCSYM_NamedRoot * pApplContext
)
{
    mdCustomAttribute   customvalue;
    mdTypeRef           mdtyperefAttr;

    // Bound tree
    ILTree::Expression     *ptreeExprBound;

    // Need all this ---- to convert the bound tree to a COM+ blob
    STRING      *pstrAttrName;
    BCSYM_Class *psymAttrClass;
    BCSYM_Proc  *psymAttrCtor;
    BYTE        *pbArgBlob;
    ULONG        cbArgBlob;
    ULONG        cPosParams;
    ULONG cbSignature = 0;
    COR_SIGNATURE *pSignature = NULL;
    bool         fError;

    // Get the bound tree for this applied attribute
    ptreeExprBound = psymApplAttr->GetBoundTree();

    // If ptreeExprBound, errors have been reported through pErrorTable
    if (!ptreeExprBound)
    {
        return;
    }

    if
    (!
        CheckConditionalAttributes(
            psymApplAttr,
            m_pSourceFile
        )
    )
    {
        return;
    }

#if DEBUG
    if (VSFSWITCH(fDumpBoundMethodTrees))
    {
        BILDUMP dump(m_pCompiler);
        dump.DumpBilTree(ptreeExprBound);
    }
#endif

    Attribute::EncodeBoundTree
    (
        &ptreeExprBound->AsAttributeApplicationExpression(),
        pMetaEmitHelper,
        pAlloc,
        m_pCompiler,
        m_pCompilerProject,
        false,
        &pstrAttrName,
        &psymAttrClass,
        &psymAttrCtor,
        &pbArgBlob,
        &cbArgBlob,
        &cPosParams,
        &pSignature,
        &cbSignature,
        &fError,
        pErrorTable
    );

    VSASSERT(!fError, "Shouldn't fail on anything that got past semantic analysis.");

    // Security attributes are gathered up and emitted later as a set
    if (psymAttrClass->IsSecurityAttribute())
    {
        VSASSERT(pSecAttrInfo, "A security attribute was found but no SecAttrInfo was provided.");

        VSASSERT(!IsNilToken(GetToken(psymAttrCtor)), "Should have COM+ token by now!");

        if (pSecAttrInfo)
        {
            m_pCompilerProject->SaveSecAttr
            (
                pSecAttrInfo,
                pMetaEmitHelper->DefineMemberRefBySymbol
                (
                    psymAttrCtor,
                    NULL,
                    psymApplAttr->GetLocation(),
                    pErrorTable
                ),
                pbArgBlob,
                cbArgBlob,
                pstrAttrName,
                ptreeExprBound->Loc,
                pApplContext
            );
        }
    }
    else
    {
        HRESULT hrCOM;

        // Emit as a normal (non-security) attribute
        mdtyperefAttr = pMetaEmitHelper->DefineMemberRefBySymbol
        (
            psymAttrCtor,
            NULL,
            psymApplAttr->GetLocation(),
            pErrorTable
        );

        hrCOM = pMetaEmitHelper->DefineCustomAttribute
        (
            tokenForSymbol,
            mdtyperefAttr,
            pbArgBlob,
            cbArgBlob,
            &customvalue
        );

        // Duplicate the attribute for the associated com class member
        if (SUCCEEDED(hrCOM))
        {
            mdToken tokenForComClassSymbol = mdTokenNil;

            if (pSymbol->IsNamedRoot() && !pSymbol->IsClass())
            {
                tokenForComClassSymbol = GetComClassToken(pSymbol->PNamedRoot());
            }
            else if (pSymbol->IsParam())
            {
                tokenForComClassSymbol = pSymbol->PParam()->GetComClassToken();
            }
            if (!IsNilToken(tokenForComClassSymbol))
            {
                hrCOM = pMetaEmitHelper->DefineCustomAttribute
                (
                    tokenForComClassSymbol,
                    mdtyperefAttr,
                    pbArgBlob,
                    cbArgBlob,
                    &customvalue
                );
            }
        }

        if (FAILED(hrCOM))
        {
            pErrorTable->CreateErrorWithError
            (
                ERRID_BadAttribute1,
                &ptreeExprBound->Loc,
                hrCOM,
                pstrAttrName
            );
        }
    }
}

TCLRCreateInstance PEBuilder::m_pMSCoreeCLRCreateInstance = NULL;

HRESULT PEBuilder::CLRCreateInstance(REFCLSID clsid, REFIID riid, LPVOID *ppInterface)
{
    if (m_pMSCoreeCLRCreateInstance == NULL)
    {
        HMODULE hMSCoree;

        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN, L"mscoree.dll", &hMSCoree))
        {
            m_pMSCoreeCLRCreateInstance = (TCLRCreateInstance)GetProcAddress(hMSCoree, "CLRCreateInstance");
        }

        if (m_pMSCoreeCLRCreateInstance == NULL)
        {
            VbThrow(GetLastHResultError());
        }
    }

    return (*m_pMSCoreeCLRCreateInstance)(clsid, riid, ppInterface);
}
