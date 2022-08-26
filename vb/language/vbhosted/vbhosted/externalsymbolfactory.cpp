#include "stdafx.h"

ExternalSymbolFactory::ExternalSymbolFactory(VBHostedAllocator* pAllocator, VbContext* pContext, Compiler* pCompiler, CompilerFile* pCompilerFile)
    : m_pAllocator(pAllocator), 
    m_pContext(pContext),
    m_pCompiler(pCompiler), 
    m_pCompilerFile(pCompilerFile), 
    m_pExternalModule(NULL), 
    m_symbols(m_pCompiler, m_pAllocator->GetNorlsAllocator(), NULL /*LineMarkerTable*/),
    m_htTypeHash(pAllocator->GetNorlsAllocator()),
	m_htRefTypeHash(pAllocator->GetNorlsAllocator())
{
    m_pExtTypeBuilder = new ExternalTypeBuilder(m_pCompiler, m_pCompilerFile, &m_symbols, m_pContext, this);
}

ExternalSymbolFactory::~ExternalSymbolFactory()
{
    ASSERT(m_pExtTypeBuilder, "[ExternalSymbolFactory::~ExternalSymbolFactory] 'm_pExternalTypeBuilder' should never be NULL");
    delete m_pExtTypeBuilder;
}

void ExternalSymbolFactory::Initialize(BCSYM_Class* pExternalModule)
{
    m_pExternalModule = pExternalModule;
}

BCSYM_NamedRoot* ExternalSymbolFactory::GetVariableForName(const STRING* Name)
{
    CComPtr<mscorlib::_Type> spType = NULL;
    CComBSTR name(Name);
    DECLFLAGS flags = DECLF_Public | DECLF_Shared | DECLF_New;
    BCSYM* pTypeSymbol = NULL;
    BCSYM_Variable* pVar = NULL;
    HRESULT hr = S_OK;
    gcroot<System::Type^> pType;

    VerifyInPtrRetNull(Name, "[ExternalSymbolFactory::GetVariableForName] 'Name' parameter is null");

    hr = m_pContext->FindVariable(name, pType);
    if(FAILED(hr) || !pType)
        return NULL;
    //Will return S_FALSE if symbol is not found
    //IfFailThrow(GetType(pType, &pTypeSymbol));

    // Fields of void type are not valid.
    if (System::Void::typeid->Equals(pType))
        return NULL;

    pTypeSymbol = GetExternalTypeBuilder()->GetSymbolForType(pType, NULL, NULL);
    if(!pTypeSymbol)
        return NULL;

    // Consider: casting away const is not good, but fixing it would require large scale changes.
    STRING* nonConstName = const_cast<STRING*>(Name);

    pVar = m_symbols.AllocVariable(false, false);
    m_symbols.GetVariable(
        NULL,       //No location
        nonConstName,
        nonConstName,
        flags,
        VAR_Member,
        pTypeSymbol,
        NULL,       //No expression
        NULL,       //No SymbolsList
        pVar);

    //Note that this is not actually a field
    pVar->SetIsFromScriptScope(true);

    if (pTypeSymbol->IsBad())
    {
        MetaImport::TransferBadSymbol(pVar, pTypeSymbol->PNamedRoot());
        pVar->SetIsBadVariableType(true);
    }

    m_symbols.AddSymbolToHash(m_pExternalModule->GetHash(), pVar, true, true, false);

    return pVar;
}

void ExternalSymbolFactory::ImportTypeChildren(BCSYM_Container* pContainer)
{
    HRESULT hr = S_OK;
    ASSERT(pContainer, "[ExternalSymbolFactory::ImportTypeChildren] 'pContainer' parameter cannot be null");
    if (pContainer && !pContainer->AreChildrenLoaded() && !pContainer->AreChildrenLoading())
    {
        hr = GetExternalTypeBuilder()->SetupChildrenForType(pContainer);
        ASSERT(SUCCEEDED(hr), "[ExternalSymbolFactory::ImportTypeChildren] ExternalTypeBuilder::SetupChildrenForType failed.");
    }
}

BCSYM_NamedRoot* ExternalSymbolFactory::GetSymbolForName(const STRING* Name, BCSYM_Namespace* pNS)
{
    VerifyInPtrRetNull(Name, "[ExternalSymbolFactory::GetSymbolForName] 'Name' parameter is null");
    VerifyInPtrRetNull(pNS, "[ExternalSymbolFactory::GetSymbolForName] 'pNS' parameter is null");

    //GetNamespaceForName and GetTypeForName -- if successful, will add to parent hash.
    BCSYM_NamedRoot* ns = GetExternalTypeBuilder()->GetNamespaceForName(Name, pNS);
    BCSYM_NamedRoot* nr = GetExternalTypeBuilder()->GetTypeForName(Name, pNS);

    //If nr exists, it will have a reference to ns as the next element.
    // If neither exists, both are null. Simple, no?
    return nr ? nr : ns;
}

HRESULT ExternalSymbolFactory::GetType(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol)
{
    HRESULT hr = S_OK;
    __int64 handleCode = 0;
    gcroot<System::RuntimeTypeHandle^> handleType;
    BCSYM_NamedRoot* pNamedRoot = NULL;

    VerifyInCLRPtr(pType, "[ExternalSymbolFactory::GetType] 'pType' parameter is NULL");
    VerifyOutPtr(ppTypeSymbol, "[ExternalSymbolFactory::GetType] 'ppTypeSymbol' parameter is NULL");

    *ppTypeSymbol = NULL;

    // Will return failure codes on failures.
    // Will return S_OK on successful find or failure to locate pType.
    // pNamedRoot will be NULL if pType is not in reference assemblies.
    IfFailGo(GetReferencedType(pType, &pNamedRoot));

    if (!pNamedRoot)
    {
        IfFailGo(GetTypeScopeType(pType, &pNamedRoot));
        ASSERT(pNamedRoot || hr == S_FALSE, 
            "[ExternalSymbolFactory::GetType] 'pNamedRoot' should be defined OR GetTypeScopeType should have returned S_FALSE");
        ASSERT(!pNamedRoot || hr == S_OK, 
            "[ExternalSymbolFactory::GetType] 'pNamedRoot' should not be defined OR GetTypeScopeType should have returned S_OK");
    }

    *ppTypeSymbol = pNamedRoot;

Error:
    return hr;
}

HRESULT ExternalSymbolFactory::GetTypeScopeType(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol)
{
    HRESULT hr = S_OK;
    __int64 handleCode = 0;
    gcroot<System::RuntimeTypeHandle^> handleType;
    BCSYM_NamedRoot* pNamedRoot = NULL;

    VerifyInCLRPtr(pType, "[ExternalSymbolFactory::GetTypeScopeType] 'pType' parameter is NULL");
    VerifyOutPtr(ppTypeSymbol, "[ExternalSymbolFactory::GetTypeScopeType] 'ppTypeSymbol' parameter is NULL");

    *ppTypeSymbol = NULL;

    handleType = pType->TypeHandle;
    handleCode = handleType->Value.ToInt64();

    if (!m_htTypeHash.GetValue(handleCode, &pNamedRoot))
    {
        IfFailGo(GetTypeScopeType_NoCache(pType, &pNamedRoot));
        ASSERT(pNamedRoot || hr == S_FALSE, 
            "[ExternalSymbolFactory::GetTypeScopeType] 'pNamedRoot' should be defined OR GetTypeFromHost should have returned S_FALSE");
        ASSERT(!pNamedRoot || hr == S_OK, 
            "[ExternalSymbolFactory::GetTypeScopeType] 'pNamedRoot' should not be defined OR GetTypeFromHost should have returned S_OK");

        if (pNamedRoot)
        {
            m_htTypeHash.SetValue(handleCode, pNamedRoot);
        }
    }
#if DEBUG
    else
    {
        ASSERT(pNamedRoot, "[ExternalSymbolFactory::GetTypeScopeType] '*ppTypeSymbol' returned from GetValue should always be defined.");
    }
#endif

    *ppTypeSymbol = pNamedRoot;

Error:
    return hr;
}

HRESULT ExternalSymbolFactory::GetTypeScopeType_NoCache(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol)
{
    HRESULT hr = S_OK;
    BCSYM_NamedRoot* pTypeSymbol = NULL;

    VerifyInCLRPtr(pType, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'pType' parameter is NULL");
    VerifyOutPtr(ppTypeSymbol, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'ppTypeSymbol' parameter is NULL");
    ASSERT(!*ppTypeSymbol, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] '*ppTypeSymbol' parameter should be NULL"); 

    *ppTypeSymbol = NULL;

    BCSYM_Namespace* pNSCurrent = m_pCompilerFile->GetUnnamedNamespace();
    ASSERT(pNSCurrent, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'pNSCurrent' variable is NULL, no UnnamedNamespace");

    //If the namespace is NULL or empty, the type lives in the unnamed namespace
    if (!System::String::IsNullOrEmpty(pType->Namespace))
    {
        gcroot<System::String^> strNamespace = pType->Namespace;
        ASSERT(strNamespace, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'strNamespace' variable is nullptr");
        pin_ptr<const wchar_t> pstrNS = PtrToStringChars(strNamespace);
        ASSERT(pstrNS, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'pstrNS' variable is NULL, check that strNamespace is not");
        unsigned NameCount = m_pCompiler->CountQualifiedNames(pstrNS);
        ASSERT(NameCount >= 1, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'NameCount' variable is less than one; there should always be at least one Qualified Name.");
        STRING **Names = (STRING **)m_pAllocator->GetNorlsAllocator()->Alloc(VBMath::Multiply(
            sizeof(STRING *), 
            NameCount));
        ASSERT(Names, "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'Names' variable is NULL, Alloc failed without throwing");
        m_pCompiler->SplitQualifiedName(pstrNS, NameCount, Names);

        for (unsigned int i = 0; i < NameCount && pNSCurrent; i++)
        {
            BCSYM_NamedRoot* pNR = Semantics::ImmediateLookup(ViewAsScope(pNSCurrent), Names[i], BINDSPACE_Type);
            ASSERT(!pNR || pNR->IsNamespace(), "[ExternalSymbolFactory::GetTypeScopeType_NoCache] 'pNR' variable is not NULL but not a Namespace");
            IfFalseGo(pNR, E_UNEXPECTED);
            IfFalseGo(pNR->IsNamespace(), E_UNEXPECTED);
            pNSCurrent = pNR->PNamespace();
        }
    }

    IfFalseGo(pNSCurrent, E_UNEXPECTED);

    pTypeSymbol = GetExternalTypeBuilder()->MakeSymbolForType(pType);

    if (pTypeSymbol == NULL)
        hr = S_FALSE;
    else
    {
        hr = S_OK;
        if (!pType->IsNested)
            Symbols::AddSymbolToHash(pNSCurrent->GetHash(), pTypeSymbol, true, false, true);
        *ppTypeSymbol = pTypeSymbol;
    }

Error:
    return hr;
}

HRESULT ExternalSymbolFactory::GetReferencedType(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol)
{
    HRESULT hr = S_OK;
    __int64 handleCode = 0;
    gcroot<System::RuntimeTypeHandle^> handleType;
    BCSYM_NamedRoot* pNamedRoot = NULL;

    VerifyInCLRPtr(pType, "[ExternalSymbolFactory::GetReferencedType] 'pType' parameter is NULL");
    VerifyOutPtr(ppTypeSymbol, "[ExternalSymbolFactory::GetReferencedType] 'ppTypeSymbol' parameter is NULL");

    *ppTypeSymbol = NULL;

    handleType = pType->TypeHandle;
    handleCode = handleType->Value.ToInt64();

    if (!m_htRefTypeHash.GetValue(handleCode, &pNamedRoot))
    {
        IfFailGo(GetReferencedType_NoCache(pType, &pNamedRoot));
        ASSERT(pNamedRoot || hr == S_FALSE, 
            "[ExternalSymbolFactory::GetReferencedType] 'pNamedRoot' should be defined OR GetType should have returned S_FALSE");
        ASSERT(!pNamedRoot || hr == S_OK, 
            "[ExternalSymbolFactory::GetReferencedType] 'pNamedRoot' should not be defined OR GetType should have returned S_OK");

        //By storing a lookup failure (pNamedRoot == NULL), we can prevent repeated lookups for the same System::Type.
        m_htRefTypeHash.SetValue(handleCode, pNamedRoot);
        //Caller wants consistent HRESULT; without this, the first time a pType isn't found, it will return S_FALSE, 
        //but subsequent calls will return S_OK.
        hr = S_OK;
    }

    *ppTypeSymbol = pNamedRoot;

Error:
    return hr;
}
HRESULT ExternalSymbolFactory::GetReferencedType_NoCache(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol)
{
    HRESULT hr = S_OK;

    VerifyInCLRPtr(pType, "[ExternalSymbolFactory::GetReferencedType_NoCache] 'pType' parameter is NULL");
    VerifyOutPtr(ppTypeSymbol, "[ExternalSymbolFactory::GetReferencedType_NoCache] 'ppTypeSymbol' parameter is NULL");

    *ppTypeSymbol = NULL;
    //Check for intrinsic types
    if(pType->IsValueType)
    {
        if(System::Boolean::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetBooleanType()->PNamedRoot();
        else if(System::Byte::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetByteType()->PNamedRoot();
        else if(System::SByte::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetSignedByteType()->PNamedRoot();
        else if(System::Int16::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetShortType()->PNamedRoot();
        else if(System::UInt16::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetUnsignedShortType()->PNamedRoot();
        else if(System::Int32::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetIntegerType()->PNamedRoot();
        else if(System::UInt32::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetUnsignedIntegerType()->PNamedRoot();
        else if(System::Int64::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetLongType()->PNamedRoot();
        else if(System::UInt64::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetUnsignedLongType()->PNamedRoot();
        else if(System::Decimal::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetDecimalType()->PNamedRoot();
        else if(System::Single::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetSingleType()->PNamedRoot();
        else if(System::Double::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetDoubleType()->PNamedRoot();
        else if(System::DateTime::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetDateType()->PNamedRoot();
        else if(System::Char::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetCharType()->PNamedRoot();
        else if(System::String::typeid->Equals(pType))
            *ppTypeSymbol = m_pCompilerFile->GetCompilerHost()->GetFXSymbolProvider()->GetStringType()->PNamedRoot();
        else if(System::Void::typeid->Equals(pType))
        {
            // This check is done after all the above checks to avoid unnecessary check
            // for the more common cases above.
            ASSERT(false, "[ExternalSymbolFactory::GetReferencedType_NoCache] request for Void type unexpected!");
            IfFalseGo(false, E_UNEXPECTED);
        }
    }
    
    if(*ppTypeSymbol == NULL)
    {
        AllProjectsIterator projIterator(m_pCompiler);
        CompilerProject* project = NULL;
        gcroot<System::Reflection::Assembly^> pAssembly;
        pin_ptr<const wchar_t> pChAssemblyName, pChTypeName, pChNamespaceName;
        STRING *pAssemblyNameStr = NULL, *pTypeNameStr = NULL, *pNamespaceNameStr = NULL, *pEmittedTypeNameStr = NULL;
        AssemblyIdentity* identity = NULL;
        unsigned int GenericTypeArity = 0;

        pAssembly = pType->Assembly;
        ASSERT(pAssembly, "[ExternalSymbolFactory::GetReferencedType_NoCache] could not load assembly");
        if(!pAssembly)
        {
            IfFalseGo(false, S_FALSE);
        }

        *ppTypeSymbol = NULL;

        pChAssemblyName = PtrToStringChars(pAssembly->FullName);
        pAssemblyNameStr = m_pCompiler->AddString(pChAssemblyName);
        ASSERT(pAssemblyNameStr, "[ExternalSymbolFactory::GetReferencedType_NoCache] could not load assembly name");

        pChTypeName = PtrToStringChars(pType->Name);
        pEmittedTypeNameStr = m_pCompiler->AddString(pChTypeName);
        ASSERT(pEmittedTypeNameStr, "[ExternalSymbolFactory::GetReferencedType_NoCache] could not load type name");

        pTypeNameStr = GetActualTypeNameFromEmittedTypeName(pEmittedTypeNameStr, -1, m_pCompiler, &GenericTypeArity);

        ASSERT(pTypeNameStr, "[ExternalSymbolFactory::GetReferencedType_NoCache] 'pTypeNameStr' Type name could not be retrieved.");

        // It's valid to have a type in the unnamed namespace
        if (!System::String::IsNullOrEmpty(pType->Namespace))
        {
            pChNamespaceName = PtrToStringChars(pType->Namespace);
            pNamespaceNameStr = m_pCompiler->AddString(pChNamespaceName);
            ASSERT(pNamespaceNameStr, "[ExternalSymbolFactory::GetReferencedType_NoCache] could not load namespace name");
        }

        //Return S_FALSE if type is not found
        hr = S_FALSE;
        while(project = projIterator.Next())
        {
            if(project->IsMetaData())
            {
                identity = project->GetAssemblyIdentity();
                ASSERT(identity, "[ExternalSymbolFactory::GetReferencedType_NoCache] could not load an assembly identity");
                STRING* pTestName = identity->GetAssemblyIdentityString();
                ASSERT(pTestName, "[ExternalSymbolFactory::GetReferencedType_NoCache] could not load an assembly identity's string");
                if(StringPool::IsEqual(pAssemblyNameStr, pTestName))
                {
                    MetaDataFileIterator fileIterator(project);
                    MetaDataFile * file = NULL;

                    while(file = fileIterator.Next())
                    {
                        BCSYM_Namespace * pNamespaceSym = NULL;
                        if (pNamespaceNameStr)
                        {
                            pNamespaceSym = file->GetNamespace(pNamespaceNameStr);
                        }
                        else
                        {
                            pNamespaceSym = file->GetUnnamedNamespace();
                        }

                        if(pNamespaceSym)
                        {
                            Semantics SemanticAnalyzer(
                                m_pAllocator->GetNorlsAllocator(),
                                NULL,  //Error Table
                                m_pCompiler,
                                m_pCompiler->GetDefaultCompilerHost(),
                                NULL, //DependencyManager
                                NULL, //SourceFile
                                NULL, //TransientSymbolStore
                                false);

                            bool fIsBad = false;

                            *ppTypeSymbol = SemanticAnalyzer.LookupInNamespace(
                                pNamespaceSym,
                                pTypeNameStr,
                                NameSearchImmediateOnly | NameSearchTypeReferenceOnly,
                                BINDSPACE_Type,
                                fIsBad,
                                NULL, //GenericBindingContext
                                GenericTypeArity);
                            IfTrueGo(!fIsBad && *ppTypeSymbol, S_OK);
                        }
                    }
                }
            }
        }
    }

Error:

    return hr;
}

HRESULT ExternalSymbolFactory::GetNestedTypeFromReferencedType(gcroot<System::Type^> pType, BCSYM_Container* pContainingSymbol, BCSYM_NamedRoot** ppTypeSymbol)
{
    VerifyInCLRPtr(pType, "[ExternalSymbolFactory::GetNestedType] 'pType' parameter must not be null");
    VerifyInPtr(pContainingSymbol, "[ExternalSymbolFactory::GetNestedType] 'pContainingSymbol' parameter must not be null");
    VerifyOutPtr(ppTypeSymbol, "[ExternalSymbolFactory::GetNestedType] 'ppTypeSymbol' out parameter must not be null");

    ASSERT(!pContainingSymbol->GetExternalSymbol(), "[ExternalSymbolFactory::GetNestedType] 'pContainingSymbol' must not be external");

    HRESULT hr = S_OK;
    pin_ptr<const wchar_t> pChTypeName;
    STRING *pTypeNameStr = NULL, *pEmittedTypeNameStr = NULL;
    unsigned int GenericTypeArity = 0;

    pChTypeName = PtrToStringChars(pType->Name);
    pEmittedTypeNameStr = m_pCompiler->AddString(pChTypeName);
    ASSERT(pEmittedTypeNameStr, "[ExternalSymbolFactory::GetReferencedType] could not load type name");

    pTypeNameStr = GetActualTypeNameFromEmittedTypeName(pEmittedTypeNameStr, -1, m_pCompiler, &GenericTypeArity);

    ASSERT(pTypeNameStr, "[ExternalSymbolFactory::GetReferencedType] 'pTypeNameStr' Type name could not be retrieved.");

    for (BCSYM_NamedRoot* CurrentResult = Semantics::ImmediateLookup(ViewAsScope(pContainingSymbol),
                                                pTypeNameStr, BINDSPACE_Type);
            CurrentResult;
            CurrentResult = CurrentResult->IsType() ?
                CurrentResult->GetNextOfSameName() :
                NULL
        )
    {
        if (Semantics::IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
        {
            *ppTypeSymbol = CurrentResult;
            break;
        }
    }

    ASSERT(*ppTypeSymbol, 
        "[ExternalSymbolFactory::GetReferencedType] Could not retrieve a valid symbol in Containing Symbol for pType");
    IfFalseGo(*ppTypeSymbol, E_UNEXPECTED);

Error:
    return hr;
}

//*************************************************************************
//
// Returns the compiler Symbol for a given type. The difference between
// this function and GetType is that GetType returns the symbols
// corresponding to type definitions whereas GetSymbolForType returns
// the symbols corresponding to the type use.
//
// Examples of cases where these would differ are:
//
//   1. GetType for a generic definition System::Type would return the
//      class or interface symbol corresponding to the definition of
//      the generic whereas GetSymbolForType would return the generic
//      binding for the open constructed type.
//
//   2. GetType would not work generic instantiations whereas this
//      function would return the corresponding generic binding.
//
//   3. GetType would not work for arrays whereas this function will
//      correctly return the corresponding symbol.
//
//   Note that for non-generic class, interfaces, enums, etc. both GetType
//   and this function would return the same symbol since there is no
//   difference in the symbols for the definition of those types vs the
//   symbols required to present the usage of those types.
//
//
//**************************************************************************
BCSYM* ExternalSymbolFactory::GetSymbolForType(gcroot<System::Type^> pType)
{
    return GetExternalTypeBuilder()->GetSymbolForType(pType, NULL, NULL);
}

CompilerFile* ExternalSymbolFactory::GetCompilerFile()
{
    ASSERT(m_pCompilerFile, "[ExternalSymbolFactory::GetCompilerFile] 'm_pCompilerFile' member is null");
    return m_pCompilerFile;
}

VBHostedAllocator* ExternalSymbolFactory::GetHostedAllocator()
{
    ASSERT(m_pAllocator, "[ExternalSymbolFactory::GetHostedAllocator] 'm_pAllocator' member is null");
    return m_pAllocator;
}

ExternalTypeBuilder* ExternalSymbolFactory::GetExternalTypeBuilder()
{
    ASSERT(m_pExtTypeBuilder, "[ExternalSymbolFactory::GetExternalTypeBuilder] 'm_pExtTypeBuilder' member is null");
    return m_pExtTypeBuilder;
}

IVbHostedSymbolSource* ExternalSymbolFactory::GetTypeFactory(BCSYM_Namespace* ns)
{
    ASSERT(ns, "[ExternalSymbolFactory::GetTypeFactory] 'ns' parameter is null");
    ExternalTypeFactory* pNewFactory = new(*m_pAllocator->GetNorlsAllocator()) ExternalTypeFactory(this, ns);
    return (IVbHostedSymbolSource*)pNewFactory;
}

IVbHostedSymbolSource* ExternalSymbolFactory::GetVariableFactory()
{   
    ExternalVariableFactory* pNewFactory = new(*m_pAllocator->GetNorlsAllocator()) ExternalVariableFactory(this);
    return (IVbHostedSymbolSource*)pNewFactory;
}
