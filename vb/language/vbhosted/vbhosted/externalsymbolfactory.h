#pragma once

// Needed for mscorlib::_Type.
#import "mscorlib.tlb" raw_interfaces_only raw_native_types named_guids rename("_Module", "_ReflectionModule") rename("ReportEvent", "_ReflectionReportEvent") rename("value", "_value")

#include "VBContext.h"
#include "VBHostedAllocator.h"
#include "VBHostedInterop.h"
#include "..\..\Compiler\Symbols\SymbolTable.h"

//This is needed in ExternalSymbolFactory (TypeHandle->Symbol map) and in
// ExternalTypeBuilder (MethodHandle->Symbol map).
typedef DynamicHashTable<__int64, BCSYM_NamedRoot*, NorlsAllocWrapper> HandleSymHashTable;

class ExternalSymbolTypeFactory;
class ExternalTypeBuilder;

class ExternalSymbolFactory
#if FV_DEADBEEF
    : public Deadbeef<ExternalSymbolFactory> // Must be last base class!
#endif
{
    friend class ExternalTypeBuilder;
public:
    ExternalSymbolFactory(VBHostedAllocator* pAllocator, VbContext* pContext, Compiler* pCompiler, CompilerFile* pCompilerFile);
    virtual ~ExternalSymbolFactory();

    void Initialize(BCSYM_Class* m_pExternalModule);

    BCSYM_NamedRoot* GetSymbolForName(const STRING* Name, BCSYM_Namespace* pNS);
    BCSYM_NamedRoot* GetVariableForName(const STRING* Name);

    void ImportTypeChildren(BCSYM_Container* pContainer);

    IVbHostedSymbolSource* GetTypeFactory(BCSYM_Namespace* ns);
    IVbHostedSymbolSource* GetVariableFactory();

    //Hash helpers
    HRESULT GetType(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol);
	HRESULT GetTypeScopeType(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol);
    //Retrieves Nested Types from a reference assembly
    HRESULT GetNestedTypeFromReferencedType(gcroot<System::Type^> pType, BCSYM_Container* pContainingSymbol, BCSYM_NamedRoot** ppTypeSymbol);


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
    BCSYM* GetSymbolForType(gcroot<System::Type^> pType);

private:
    HRESULT GetReferencedType(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol);
	HRESULT GetReferencedType_NoCache(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol);
    HRESULT GetTypeScopeType_NoCache(gcroot<System::Type^> pType, BCSYM_NamedRoot** ppTypeSymbol);
    
    CompilerFile* GetCompilerFile();
    VBHostedAllocator* GetHostedAllocator();
    ExternalTypeBuilder* GetExternalTypeBuilder();
    
    VbContext* m_pContext;
    VBHostedAllocator* m_pAllocator;
    Compiler* m_pCompiler;
    BCSYM_Class* m_pExternalModule;
    CompilerFile* m_pCompilerFile;
    Symbols m_symbols;
    ExternalTypeBuilder* m_pExtTypeBuilder;
    HandleSymHashTable m_htTypeHash;
	HandleSymHashTable m_htRefTypeHash;
};

class ExternalVariableFactory :
    public IVbHostedSymbolSource
{
    friend class ExternalSymbolFactory;
public:
    //From IVbHostedSymbolSource
    virtual BCSYM_NamedRoot* GetSymbolForName(const STRING* Name)
    {
        return m_ParentFactory->GetVariableForName(Name);
    }

    virtual void ImportTypeChildren(BCSYM_Container* pContainer)
    {
        ASSERT(false, "[ExternalVariableFactory::ImportTypeChildren] Function should never be called.");
    }

private:
    ExternalVariableFactory(ExternalSymbolFactory* ExtSymFactory)
    {
        m_ParentFactory = ExtSymFactory;
    }

    ExternalSymbolFactory* m_ParentFactory;
};

class ExternalTypeFactory :
    public IVbHostedSymbolSource
{
    friend class ExternalSymbolFactory;
public:
    //From IVbHostedSymbolSource
    virtual BCSYM_NamedRoot* GetSymbolForName(const STRING* Name)
    {
        BCSYM_NamedRoot* pSymbol = NULL;
        pSymbol = m_ParentFactory->GetSymbolForName(Name, m_nameSpace);
        return pSymbol;
    }

    virtual void ImportTypeChildren(BCSYM_Container* pContainer)
    {
        ASSERT(pContainer, "[ExternalTypeFactory::ImportTypeChildren] 'pContainer' parameter must not be null");
        m_ParentFactory->ImportTypeChildren(pContainer);
    }

private:
    ExternalTypeFactory(ExternalSymbolFactory* ExtSymFactory, BCSYM_Namespace* ns)
    {
        m_ParentFactory = ExtSymFactory;
        m_nameSpace = ns;
    }

    ExternalSymbolFactory* m_ParentFactory;
    BCSYM_Namespace* m_nameSpace;
};
