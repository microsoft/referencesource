#pragma once

#include "vcclr.h"
#include "..\..\Compiler\Symbols\SymbolTypes.h"
#include "ExternalSymbolFactory.h"
#include "VbContext.h"
#include "..\..\Shared\Collections.h"

class ExternalTypeBuilder
#if FV_DEADBEEF
    : public Deadbeef<ExternalTypeBuilder> // Must be last base class!
#endif
{
public:
    ExternalTypeBuilder(Compiler* pCompiler, CompilerFile* pCompilerFile, Symbols* pSymbols, VbContext* pContext, ExternalSymbolFactory* pFactory);
    virtual ~ExternalTypeBuilder();

    BCSYM_NamedRoot* GetTypeForName(const STRING* Name, BCSYM_Namespace* pParentNamespace);
    BCSYM_NamedRoot* GetNamespaceForName(const STRING* Name, BCSYM_Namespace* pParentNamespace);

    HRESULT SetupChildrenForType(BCSYM_Container* pTypeSymbol);

    BCSYM_NamedRoot* MakeSymbolForType(gcroot<System::Type^> pType);
    
    // GetSymbolForType should only be used to get the symbol for a type to be used in contexts where a type is referred
    // to (rather than defined). Eg: parameter type, return type, field type, base type, interface implements type, etc.
    // To get the symbol corresponding to the definition for a type, the GetSymbolForTypeDefinition function needs to be
    // used.
    //
    // Note that for generics, this function always (even for generic definitions) returns the instantiated generic i.e.
    // a generic binding. This is required due to bug Dev10 698789.
    //
    BCSYM* GetSymbolForType(gcroot<System::Type^> pType, BCSYM_Container* pContainer, BCSYM_MethodDecl* pMethodContainer);

private:
    BCSYM_Class* MakeClassForType(gcroot<System::Type^> pType);
    BCSYM_Interface* MakeInterfaceForType(gcroot<System::Type^> pType);
    
    HRESULT GetChildrenForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType, SymbolList * pSymbolList, Vtypes *pvtUnderlyingType);
    HRESULT FinishChildren(BCSYM_Container* pContainer, gcroot<System::Type^> pType, SymbolList * pSymbolList);
    
    HRESULT EnsureBaseAndImplementsLoaded(BCSYM_Container* pTypeSymbol);
    HRESULT GetNestedTypesForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType);
    HRESULT GetBaseAndImplementsForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType);
    BCSYM* GetBaseForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType);
    BCSYM_Implements* GetImplementsForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType);

    // GetSymbolForTypeDefinition should only be used to get the symbol corresponding to the definition of a type.
    // Hence for example, for generics, only types corresponding to generic definitions can be passed in.
    //
    // In order to get the symbol for a type to be used in contexts where a type is referred to (rather than defined),
    // the GetSymbolForType function should be used. Example of such contexts: parameter type, return type, field type,
    // base type, interface implements type, etc.
    //
    BCSYM* GetSymbolForTypeDefinition(gcroot<System::Type^> pType, BCSYM_Container* pContainer, BCSYM_MethodDecl* pMethod);

    BCSYM* GetSymbolForGenericType(BCSYM_Container* pContainer, gcroot<System::Type^> pType);
    BCSYM* GetSymbolForGenericMethod(BCSYM_MethodDecl* pMethodDecl, gcroot<System::Type^> pType);

    HRESULT HandleTypeChildrenArray(gcroot<array<System::Reflection::MemberInfo^,1>^> members, BCSYM_Container* pContainer, 
                                    HandleSymHashTable& dhtAccessors, SymbolList* pSymbolList, Vtypes* pvtUnderlyingType, _In_opt_z_ const WCHAR* pwszDefaultProperty);
    BCSYM_NamedRoot* GetSymbolForTypeMember(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& hash, Vtypes* pvtUnderlyingType, _In_opt_z_ const WCHAR* pwszDefaultProperty, bool& FirstDefaultProperty);
    BCSYM_NamedRoot* GetSymbolForTypeMethod(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& hash);
    BCSYM_NamedRoot* GetSymbolForTypeField(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, Vtypes *pvtUnderlyingType);
    BCSYM_NamedRoot* GetSymbolForTypeProperty(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& hash, _In_opt_z_ const WCHAR* pwszDefaultProperty, bool& FirstDefaultProperty);
    BCSYM_NamedRoot* GetSymbolForTypeEvent(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& hash);
    BCSYM_NamedRoot* GetSymbolForTypeNestedType(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember);
    BCSYM_Proc* GetSymbolForAccessor(gcroot<System::Reflection::MethodInfo^> pMethod, HandleSymHashTable& hash);
    
    HRESULT GetParametersForTypeMethod(gcroot<System::Reflection::MethodBase^> pMethodBase, BCSYM** ppReturnType, BCSYM_Param** ppParamList, BCSYM_Container* pContainer, BCSYM_MethodDecl* pMethodDecl);

    void SetupGenericParameters(BCSYM_NamedRoot* pTypeSymbol, gcroot<System::Type^> pType);
    void SetupGenericParameters(BCSYM_NamedRoot* pMethodSymbol, gcroot<System::Reflection::MethodInfo^> pMethod);
    void SetupGenericParameters(BCSYM_NamedRoot* pSymbol, gcroot<array<System::Type^,1>^> GenericArguments, 
                                gcroot<array<System::Type^,1>^> ParentGenericArguments, bool isMethod);
    
    void SetupGenericParameterTypeConstraints(BCSYM_GenericParam* ListOfGenericParameters);
    void SetupGenericParameterNonTypeConstraints(BCSYM_GenericParam* Parameter, System::Reflection::GenericParameterAttributes GenericParamFlags);
    BCSYM* BuildGenericBinding(BCSYM* Generic, unsigned long ArgumentCount, BCSYM** Arguments, bool AllocAndCopyArgumentsToNewList);
    BCSYM* BuildGenericBindingHelper(BCSYM* Generic, unsigned long ArgumentCount, BCSYM** Arguments, unsigned long &StartIndex, bool AllocAndCopyArgumentsToNewList);
    
    static DECLFLAGS DeclFlagsForMethod(gcroot<System::Reflection::MethodBase^> pMethodBase, System::Reflection::MethodAttributes* pAttributes);
    static DECLFLAGS DeclFlagsForType(gcroot<System::Type^> pType);
    static DECLFLAGS DeclFlagsForField(gcroot<System::Reflection::FieldInfo^> pField);

    static bool ShouldImportSymbol(DECLFLAGS DeclFlags);

    BCSYM_Expression* GetSymbolForConstantExpression ( gcroot<System::Object^> value, gcroot<System::Type^> valueType);
    void LoadConstantValue (gcroot<System::Object^> value, gcroot<System::Type^> valueType, ConstantValue* Value);
    HRESULT LoadCustomAttributes(gcroot<System::Reflection::MemberInfo^> pMember, BCSYM* pSym);
    HRESULT LoadCustomAttributes(gcroot<System::Reflection::ParameterInfo^> pParam, BCSYM* pSym);
    HRESULT LoadCustomAttributes(gcroot<System::Collections::Generic::IList<System::Reflection::CustomAttributeData^>^> customAttributes, BCSYM* pSym);
    WCHAR* GetBuffer(gcroot<System::String^> str);

    bool ValidateType(const STRING* requestedTypeName, const STRING* requestedNamespace, gcroot<System::Type^> typeToValidate);

    Compiler* m_pCompiler;
    Symbols* m_pSymbols;
    ExternalSymbolFactory* m_pFactory;
    VbContext* m_pVbContext;
    CompilerFile* m_pCompilerFile;
};
